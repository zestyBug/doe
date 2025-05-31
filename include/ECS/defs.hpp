#if !defined(DEFS_HPP)
#define DEFS_HPP

#include <vector>
#include <algorithm>    // std::sort
#include "cutil/static_array.hpp"
#include "cutil/span.hpp"
#include "cutil/basics.hpp"

namespace ECS
{
    // variable type that can hold component(type) id as integer
    using version_t = uint32_t;

    /// @brief returns true if component contains newer version than system
    /// @param lastVersion last vesion of system that been run
    /// @param version component version in a given chunk
    /// @return true diffrence is greater than zero
    bool didChange(version_t lastVersion, version_t version);
    void updateVersion(version_t& lastVersion);

    // this engine uses uint32 and int32 for all cases,
    // unless it is specified.


    // the value 1 on the first bit is important for determining value from pointer.
    constexpr uint32_t NullArchetypeIndex = 0xfffffff;


    struct TypeID final {
        /** @brief When to use flags?
         * actually you must ask when not to use flags
         * flags make same type components diffrent for technical reasons
         * for example when you want to diffrentiate anable and disabled components
         * only time you may not need to use it is for performance reasons
         * that proven to be wrong!
         */ 
        enum TypeFlags : uint16_t {
            prefab = 0x1,
            disabled = 0x2
        };
        /**
         * leftmost bit measning:
         * 1: invalid value
         * 2: reserved
         * 3: flags and zero sized entities
         */
        uint16_t value=0;
        // flags are archetype specific, hash no effect on data
        uint16_t flag=0;

        TypeID()=default;
        TypeID(const TypeID&)=default;
        TypeID& operator = (const TypeID&)=default;
        TypeID(TypeID&&)=default;
        TypeID& operator = (TypeID&&)=default;
        TypeID(uint16_t v,uint16_t f=0):value{v},flag{f}{};

        bool isPrefab() const {
            return this->flag & prefab;
        }
        bool isDisabled() const {
            return this->flag & disabled;
        }
        uint16_t realIndex() const {
            return this->value & 0x1FFF;
        }
        bool exactSame(const TypeID v) const {
            return this->value == v.value && this->flag == v.flag;
        }
        // can be 1,2,3,...
        static constexpr uint16_t MaxTypeCount = 0x2000;
    };

    struct Entity final {
        constexpr Entity() :_value{null}{}
        constexpr Entity(const int32_t v,version_t version=0):_value{v},_version{version}{}
        constexpr Entity(const Entity&) = default;
        constexpr Entity(Entity&&) = default;
        inline Entity& operator = (const Entity&) = default;
        inline bool operator == (const Entity& v) const {return this->index() == v.index() && this->_version == v._version;}
        inline bool operator != (const Entity& v) const {return !(*this == v);}
        ~Entity() = default;
        // index in entity_value array
        inline int32_t index() const {
            return this->_value;
        }
        inline version_t version() const {
            return this->_version;
        }
        // entity id (index in entity_Value array)
        static constexpr int32_t null = -1;
        // simply checks entity index validity
        inline bool valid() const {
            return this->index() >= 0;
        }
        protected:
        int32_t _value;
        version_t _version=0;
    };

    struct entity_t final {
        // index in Archetype::components
        uint32_t index;
        // NOTE: use the archetype index to validate an entity value
        uint32_t archetype = NullArchetypeIndex;
        // version is changed only entity is destroyed/created
        // to prevent mistake with entites with same entity_t::index field
        version_t version=0;

        entity_t() = default;
        entity_t(const entity_t&) = default;
        entity_t& operator = (const entity_t&) = default;
        // returns true if entity contains (seems) valid archetype indexes.
        inline bool validArchtype() const {
            return this->archetype < NullArchetypeIndex;
        }
    };


    struct comp_info final {
        TypeID value;
        uint32_t size = 0;
        // destructor function
        void (*destructor)(void*) = nullptr;
        // default no-argument constructor function
        void (*constructor)(void*) = nullptr;
    };
    // this value is hard coded means
    // modification in binary file is much harder
    // so new component needs new version of compiled engine
    // also the more component, the more memory is allocated exponentially
    #define COMPOMEN_COUNT 32
    // contains runtime typeinfo,
    // technically array can be erased to reassign type ids
    // unless values are stored somewhere and spreaded
    // WARN: dont access this directly
    // WARN: TypeID index must be 0
    extern static_array<comp_info,COMPOMEN_COUNT> rtti;

    typedef void (*rttiFP)(void*);

    [[nodiscard]] comp_info _new_id(uint32_t size, rttiFP destructor, rttiFP constructor);

    // actuall core of compile-time-type-information
    // returns real index of that type
    template<typename T>
    [[nodiscard]] uint16_t __type_id__()
    {
        if(sizeof(T) > 0x7FFF)
            throw std::length_error("__type_id__(): too large entity");
        static const comp_info value = _new_id(
            sizeof(T),
            sizeof(T) > 0 ? [](void* x){static_cast<T*>(x)->~T();} : (rttiFP)nullptr,
            sizeof(T) > 0 ? [](void* x){new (static_cast<T*>(x)) T();} : (rttiFP)nullptr
        );
        return value.value.realIndex();
    }

    // type can be specialized to fix a type id
    template<typename T>
    inline comp_info& getTypeInfo()
    {
        return rtti[__type_id__<std::remove_const_t<std::remove_reference_t<T>>>()];
    }
    [[nodiscard]] inline comp_info& getTypeInfo(const TypeID id)
    {
        if(unlikely(rtti.size() < id.realIndex()))
            throw std::bad_typeid();
        return rtti[id.realIndex()];
    }
    [[nodiscard]] inline comp_info& getTypeInfo(const uint16_t realIndex)
    {
        if(unlikely(rtti.size() < realIndex))
            throw std::bad_typeid();
        return rtti[realIndex];
    }


    static bool customCompare (TypeID a,TypeID b) {
        if(unlikely(a.value == b.value))
            throw std::bad_typeid();
        return a.value < b.value;
    }
    template<typename ... T>
    static_array<TypeID,sizeof...(T)> _INIT_COMPONENTS_TYPES_() {
        static_array<TypeID,sizeof...(T)> ret;
        (ret.emplace_back(getTypeInfo<T>().value), ...);
        std::sort(ret.begin(),ret.end(),customCompare);
        return ret;
    }
    template<typename ... T>
    static_array<TypeID,sizeof...(T)> _INIT_COMPONENTS_TYPES_RAW_() {
        static_array<TypeID,sizeof...(T)> ret;
        (ret.emplace_back(getTypeInfo<T>().value), ...);
        return ret;
    }
    template<typename ... T>
    span<TypeID> componentTypes() {
        static static_array<TypeID,sizeof...(T)> ret = _INIT_COMPONENTS_TYPES_<T...>();
        return {ret.data(),ret.size()};
    }
    template<typename ... T>
    span<TypeID> componentTypesRaw() {
        static static_array<TypeID,sizeof...(T)> ret = _INIT_COMPONENTS_TYPES_RAW_<T...>();
        return {ret.data(),ret.size()};
    }

}

#endif // DEFS_HPP

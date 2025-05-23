#if !defined(DEFS_HPP)
#define DEFS_HPP

#include <assert.h>
#include <vector>
#include <algorithm>    // std::sort
#include "cutil/StaticArray.hpp"
#include "cutil/bitset.hpp"

namespace DOTS
{
    // variable type that can hold component(type) id as integer
    using version_t = uint32_t;

    // this engine uses uint32 and int32 for all cases,
    // unless it is specified.


    // the value 1 on the first bit is important for determining value from pointer.
    constexpr uint32_t nullArchetypeIndex = 0xfffffff;


    struct TypeIndex {
        enum TypeFlags : uint16_t {
            prefab = 0x1,
            disabled = 0x2
        };
        uint16_t value=0;
        // flags are archetype specific, hash no effect on data
        uint16_t flag=0;

        TypeIndex()=default;
        TypeIndex(const TypeIndex&)=default;
        TypeIndex& operator = (const TypeIndex&)=default;
        TypeIndex(TypeIndex&&)=default;
        TypeIndex& operator = (TypeIndex&&)=default;
        TypeIndex(uint16_t v,uint16_t f=0):value{v},flag{f}{};

        bool isPrefab() const {
            return this->flag & prefab;
        }
        bool isDisabled() const {
            return this->flag & disabled;
        }
        uint16_t realIndex() const {
            return this->value & 0x1FFF;
        }
        bool exactSame(const TypeIndex v){
            return this->value == v.value && this->flag == v.flag;
        }
    };

    struct Entity{
        constexpr Entity() :_value{null}{}
        constexpr Entity(const uint32_t v,version_t version=0):_value{v},_version{version}{}
        constexpr Entity(const Entity&) = default;
        constexpr Entity(Entity&&) = default;
        inline Entity& operator = (const Entity&) = default;
        inline bool operator == (const Entity& v) const {return this->index() == v.index() && this->_version == v._version;}
        inline bool operator != (const Entity& v) const {return !(*this == v);}
        ~Entity() = default;
        // index in entity_value array
        inline uint32_t index() const {
            return this->_value;
        }
        inline version_t version() const {
            return this->_version;
        }
        static constexpr uint32_t maxEntityCount= 0xfffffe;
        // entity id (index in entity_Value array)
        static constexpr uint32_t null = 0xffffff;
        // simply checks entity index validity
        inline bool valid() const {
            return this->index() <= maxEntityCount;
        }
        protected:
        // only use purpose is as index of entity in entity_value array + version
        // Entity >> 24: version
        // Entity & 0xffffff:
        uint32_t _value;
        version_t _version=0;
    };

    struct entity_t {
        // index in Archetype::components
        uint32_t index;
        // NOTE: use the archetype index to validate an entity value
        uint32_t archetype = nullArchetypeIndex;
        // version is changed only entity is destroyed/created
        // to prevent mistake with entites with same entity_t::index field
        version_t version=0;

        entity_t() = default;
        entity_t(const entity_t&) = default;
        entity_t& operator = (const entity_t&) = default;
        // returns true if entity contains (seems) valid archetype indexes.
        inline bool validArchtype() const {
            return this->archetype < nullArchetypeIndex;
        }
    };


    struct comp_info {
        TypeIndex value;
        uint32_t size = 0;
        // destructor function
        void (*destructor)(void*) = nullptr;
        // default no-argument constructor function
        void (*constructor)(void*) = nullptr;
    };
    // must be multiply of 2
    // maximum is 4096
    // this value is hard coded means
    // modification in binary file is much harder
    // so new component needs new version of compiled engine
    // also the more component, the more memory is allocated exponentially
    #define COMPOMEN_COUNT 32
    // contains runtime typeinfo,
    // technically array can be erased to reassign type ids
    // unless values are stored somewhere and spreaded
    // WARN: dont access this directly
    // WARN: TypeIndex index must be 0
    extern StaticArray<comp_info,COMPOMEN_COUNT> rtti;

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
    [[nodiscard]] inline comp_info& getTypeInfo(const TypeIndex id)
    {
        return rtti[id.realIndex()];
    }
    [[nodiscard]] inline comp_info& getTypeInfo(const uint16_t realIndex)
    {
        return rtti.at(realIndex);
    }


    static bool customCompare (TypeIndex a,TypeIndex b) { return a.realIndex() < b.realIndex(); }
    template<typename ... T>
    std::vector<TypeIndex> initComponentTypes() {
        std::vector<TypeIndex> ret;
        ret.reserve(sizeof...(T));
        (ret.emplace_back(getTypeInfo<T>().value), ...);
        std::sort(ret.begin(),ret.end(),customCompare);
        return ret;
    }
    template<typename ... T>
    std::vector<TypeIndex> componentTypes() {
        static std::vector<TypeIndex> ret = initComponentTypes<T...>();
        return ret;
    }

}

#endif // DEFS_HPP

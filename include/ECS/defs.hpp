#if !defined(DEFS_HPP)
#define DEFS_HPP

#include <assert.h>
#include <vector>
#include "cutil/StaticArray.hpp"
#include "cutil/bitset.hpp"
#define ENGINE_VERSION 0.0

namespace DOTS
{
    // variable type that can hold component(type) id as integer
    using version_t = uint32_t;

    // this engine uses uint32 and int32 for all cases,
    // unless it is specified.

    
    struct TypeIndex {
        enum TypeFlags : uint16_t {
            prefab = 0x1,
        };
        uint16_t value=0;
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
        uint16_t realIndex() const {
            return this->value & 0x1FFF;
        }
        inline bool operator == (TypeIndex val) {return this->value == val.value;}
        inline bool operator <  (TypeIndex val) {return this->value <  val.value;}
        inline bool operator <= (TypeIndex val) {return this->value <= val.value;}
        inline bool operator >  (TypeIndex val) {return this->value >  val.value;}
        inline bool operator >= (TypeIndex val) {return this->value >= val.value;}
    };

    
    
    constexpr uint32_t nullArchetypeIndex = 0xfffffff;
    // invalid index and max entity number
    constexpr uint32_t nullEntityIndex = 0xfffffff;
    constexpr uint32_t nullChunkIndex = 0xfffffff;

    struct Entity{
        constexpr Entity() :value{null}{}
        constexpr Entity(const uint32_t v):value{v}{}
        constexpr Entity(const Entity&) = default;
        constexpr Entity(Entity&&) = default;
        inline Entity& operator = (const Entity&) = default;
        inline Entity& operator = (const uint32_t v){this->value=v;return *this;}
        inline bool operator == (const Entity& v) const {return this->index() == v.index();}
        inline bool operator != (const Entity& v) const {return this->index() != v.index();}
        ~Entity() = default;
        // removes version from entity
        inline uint32_t index() const {
            return this->value;
        }
        inline version_t version() const {
            return this->_version;
        }
        inline operator uint32_t& (){
            return this->value;
        }
        inline operator uint32_t () const {
            return this->value;
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
        uint32_t value;
        version_t _version=0;
    };
    
    struct entity_t {
        // index in Archetype::components
        uint32_t index;
        // NOTE: use the archetype index to validate an entity value
        uint32_t archetype = nullArchetypeIndex;
        // version is changed only entity is destroyed/created
        // to prevent mistake with entites with same entity_t::index field
        version_t version = 0;
        
        entity_t() = default;
        entity_t(const entity_t&) = default;
        entity_t& operator = (const entity_t&) = default;
        // returns true if entity belongs to an archetype and contains valid indexes.
        inline bool valid() const {
            return this->archetype < nullArchetypeIndex;
        }
        inline void validate() const {
            if(!this->valid())
                throw std::runtime_error("an entity with invalid index");
            }
        };
        
    struct entity_range {
        uint32_t archetype_index;
        size_t chunk_index;
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
    extern StaticArray<comp_info,COMPOMEN_COUNT> rtti;
    
    
    [[nodiscard]] comp_info _new_id(uint32_t size, void (*destructor)(void*), void (*constructor)(void*)) noexcept
    {
        TypeIndex ti;
        ti.value = rtti.size();
        if(size < 1)
            ti.value |= 0x2000;
        comp_info info{ti, size, destructor, constructor};
        rtti.push(info);
        return info;
    }

    // actuall core of compile-time-type-information
    // returns real index of that type
    template<typename T>
    [[nodiscard]] uint16_t __type_id__()
    {
        if(sizeof(T) > 0x7FFF)
            throw std::length_error("__type_id__(): too large entity");
        static const comp_info value = _new_id(sizeof(T), [](void* x){static_cast<T*>(x)->~T();},[](void* x){new (static_cast<T*>(x)) T();});
        return value.value.realIndex();
    }

    // type can be specialized to fix a type id
    template<typename T>
    [[nodiscard]] inline comp_info& getTypeInfo()
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

    
    // archtype_bitset componentsBitmask(){
    //     archtype_bitset ret{};
    //     (ret.set(getTypeInfo<Args>().index,1) , ...);
    //     return ret;
    // }
    template<typename ... T>
    std::vector<TypeIndex> componentTypes() {
        std::vector<TypeIndex> ret;
        ret.reserve(sizeof...(T));
        (ret.emplace_back(getTypeInfo<T>().index), ...);
        std::sort(ret.begin(),ret.end());
        return std::move(ret);
    }

}

#endif // DEFS_HPP

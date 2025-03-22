#if !defined(DEFS_HPP)
#define DEFS_HPP

#include <assert.h>
#include <vector>
#include "cutil/StaticArray.hpp"
#include "cutil/bitset.hpp"
#define ENGINE_VERSION 0.0

namespace DOTS
{
    // variable type that can hold component id as integer
    typedef uint32_t typeid_t;
    struct comp_info {
        // a unique id, starts from 0
        typeid_t index;
        uint32_t size = 0;
        // destructor function
        void (*destructor)(void*) = nullptr;
    };
    // contains runtime typeinfo,
    // technically array can be erased to reassign type ids
    // unless values are stored somewhere and spreaded
    // WARN: dont access this directly
    extern StaticArray<comp_info,32> rtti;

    // only use purpose is as index of archetype in archetype list
    using archetypeId_t = uint16_t;
    // only use purpose is as index of entity in archetype array
    using entityId_t = uint32_t;
    using version_t = uint8_t;

    constexpr archetypeId_t null_archetype_index = 0xffff;

    struct Entity{
        constexpr Entity() :value{0xffffff}{}
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
            return this->value & 0xffffff;
        }
        inline version_t version() const {
            return (version_t)((this->value >> 24) & 0xff);
        }
        inline operator uint32_t& (){
            return this->value;
        }
        inline operator uint32_t () const {
            return this->value;
        }
        static constexpr uint32_t max_entity_count= 0xfffffe;
        // entity id (index in entity_Value array)
        static constexpr uint32_t null = 0xffffff;
        inline bool valid() const {
            return this->index() < max_entity_count;
        }
    protected:
        // only use purpose is as index of entity in entity_value array + version
        // Entity >> 24: version
        // Entity & 0xffffff:
        uint32_t value;
    };





    struct entity_t {
        // index in Archetype::components
        entityId_t index;
        // NOTE: use the archetype index to validate an entity value
        archetypeId_t archetype = null_archetype_index;
        version_t version = 0;

        entity_t() = default;
        entity_t(const entity_t&) = default;
        entity_t& operator = (const entity_t&) = default;
        // returns true if entity belongs to an archetype and contains valid indexes.
        inline bool valid() const {
            return this->archetype < null_archetype_index;
        }
        inline void validate() const {
            assert(this->valid());
        }
    };

    struct entity_range {
        entityId_t begin;
        entityId_t end;
        archetypeId_t archetype;
    };

    [[nodiscard]] comp_info _new_id(uint32_t size, void (*destructor)(void*)) noexcept
    {
        comp_info info{(uint32_t)rtti.size(), size, destructor};
        rtti.push(info);
        return info;
    }

    // actuall core of compile-time-type-information
    template<typename T>
    [[nodiscard]] comp_info __type_id__()
    {
        static const comp_info value = _new_id(sizeof(T), [](void* x){static_cast<T*>(x)->~T();});
        return value;
    }

    // type can be specialized to fix a type id
    template<typename T>
    [[nodiscard]] inline comp_info type_id()
    {
        return __type_id__<std::remove_const_t<std::remove_reference_t<T>>>();
    }
    [[nodiscard]] inline comp_info type_id(const typeid_t id)
    {
        return rtti[id];
    }

    template<typename...Args>
    bitset componentsBitmask(){
        bitset ret;
        (ret.set(type_id<Args>().index,1) , ...);
        return ret;
    }

}

#endif // DEFS_HPP

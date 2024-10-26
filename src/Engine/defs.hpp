#if !defined(DEFS_HPP)
#define DEFS_HPP

#include "cutil/StaticArray.hpp"

#define ENGINE_VERSION 0.0

namespace DOTS
{
    // type id or type bitmask, can be changed to unordered_set ar bit_set
    using compid_t = uint32_t;

    struct comp_info {
        // a unique id, starts from 0
        compid_t index;
        size_t size;
        // destructor function
        void (*destructor)(void*);
    };
    // contains runtime typeinfo, 
    // technically array can be erased to reassign type ids 
    // unless values are stored somewhere and spreaded
    // WARN: dont access this directlys
    extern StaticArray<comp_info,32> rtti;

    // only use purpose is as index of archtype in archtype list
    using archtypeId_t = uint16_t;
    // only use purpose is as index of entity in entity_value array + version
    // Entity >> 24: version
    // Entity & 0xffffff: 
    using Entity = uint32_t;
    // only use purpose is as index of entity in archtype array
    using entityId_t = uint32_t;
    using version_t = uint8_t;
    struct entity_t {
        // index in Archtype::components
        entityId_t index = 0xffffffff;
        archtypeId_t archtype = 0xffff;
        version_t version = 0;
        entity_t(const entity_t&) = default;
        entity_t& operator = (const entity_t&) = default;
    };
    
    struct entity_range {
        entityId_t begin;
        entityId_t end;
        archtypeId_t archtype;
    };

    constexpr unsigned int null_entity_index = 0xffffff;
    constexpr archtypeId_t null_archtype_index = 0xffff;

    inline Entity get_index(Entity e){
        return e & 0xffffff;
    }
    inline version_t get_version(Entity e){
        return (version_t)((e >> 24) & 0xff);
    }

    [[nodiscard]] comp_info _new_id(size_t size, void (*destructor)(void*)) noexcept
    {
        comp_info info{(compid_t)rtti.size(), size, destructor};
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
    [[nodiscard]] inline comp_info type_id(const compid_t id) 
    {
        return rtti[id];
    }

    // a bitmask
    template<typename T>
    [[nodiscard]] inline compid_t type_bit() 
    {
        return 1 << type_id<T>().index;
    }

}

#endif // DEFS_HPP

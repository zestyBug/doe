#if !defined(DEFS_HPP)
#define DEFS_HPP

#include "cutil/StaticArray.hpp"

namespace DOTS
{
    // type id or type bitmask
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

    // entity_t >> 24: version or archtype index, depending on situation
    // entity_t & 0xffffff: index in Register::entity_value  or Archtype::components, depending on situation
    using entity_t = uint32_t;
    using Entity = uint32_t;
    // index of archtype in archtype list
    using archtypeId_t = uint32_t;
    
    struct entity_range {
        entity_t begin;
        entity_t end;
    };

    inline size_t get_index(entity_t e){
        return e & 0xffffff;
    }
    inline Entity get_version(Entity e){
        return e & 0xff000000;
    }
    inline size_t get_archtype_index(entity_t e){
        return (e >> 24) & 0xff;
    }
    inline size_t get_archtype(entity_t e){
        return e & 0xff000000;
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

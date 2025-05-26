/**
 * this file includes minimum required C++ tools
 */

#if !defined(BASICS_HPP)
#define BASICS_HPP

// some global defenitions
#include <stdio.h>
//memory allocation
#include <stdlib.h>
// heavily dependant
#include <stdint.h>
// heavily dependant
#include <stdexcept>
// new () T() should be global
#include <memory>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)


namespace ECS {

/// @brief alignes array size to 64 byte for cache, perfermance and false sharing issues
/// @param typeSize sizeof single entity
/// @param count number of entities
/// @return new array size
inline size_t alignTo64(size_t typeSize, size_t count){
    return (typeSize*count+0x3F)&0xFFFFFFC0;
}


/// @brief alignes array size to 8 byte for performance in SoA and AoS
/// @param typeSize sizeof single entity
/// @param count number of entities
/// @return new array size
inline size_t alignTo8(size_t typeSize, size_t count){
    return (typeSize*count+0x7)&0xFFFFFFF8;
}

template<typename _Tp=uint8_t>
class allocator
{
    public:
    constexpr allocator() { }
    allocator(const allocator& __a){ }
    allocator& operator=(const allocator&) = default;
    template<typename _Tp1> allocator(const allocator<_Tp1>&) { }
    ~allocator() { }

    _GLIBCXX_NODISCARD
    _Tp* allocate(size_t __n,const void* = static_cast<const void*>(0)) {
        _Tp* ret = (_Tp*) malloc(alignTo64(sizeof(_Tp),__n));
        if(ret == nullptr) throw std::bad_alloc();
        printf("allocator::allocate(): %llu byte in %p\n",__n,ret);
        return ret;
    }

    // nulity safe
    void deallocate(void* __p, size_t=0) {
        if(__p != nullptr){
            printf("allocator::deallocate(): %p\n",__p);
            free(__p);
        }

    }

    bool operator==(const allocator&) { return true; }
    bool operator!=(const allocator&) { return false; }


    _Tp* address(_Tp& __x) const _GLIBCXX_NOEXCEPT
    { return std::__addressof(__x); }

    const _Tp* address(const _Tp& __x) const _GLIBCXX_NOEXCEPT
    { return std::__addressof(__x); }

    size_t max_size() const
    { return _M_max_size(); }

    template<typename _Up, typename... _Args>
    void construct(_Up* __p, _Args&&... __args) noexcept(std::is_nothrow_constructible<_Up, _Args...>::value)
    { ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }

    void construct(_Tp* __p, const _Tp& __val)
    { ::new((void *)__p) _Tp(__val); }

    template<typename _Up>
    void destroy(_Up* __p) noexcept(std::is_nothrow_destructible<_Up>::value)
    { __p->~_Up(); }

    void destroy(_Tp* __p)
    { __p->~_Tp(); }
private:
    size_t _M_max_size() const {return std::size_t(-1) / sizeof(_Tp);}
};

}

#endif // BASICS_HPP

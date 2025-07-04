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
// memset and memcpy are part of this language :)
#include <string.h>
// heavily dependant
#include <stdexcept>
// new () T() should be global
#include <memory>

#if defined(_WIN32) || (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
#define DOE_WIN32 1
#define DOE_UNIX 0
#else
#define DOE_WIN32 0
#define DOE_UNIX 1
#endif

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)


/// @brief alignes array size to 64 byte for cache, perfermance and false sharing issues
/// @param typeSize sizeof single entity
/// @param count number of entities
/// @return new array size
inline uint32_t alignTo64(uint32_t typeSize, uint32_t count){
    return (typeSize*count+0x3F)&0xFFFFFFC0;
}


/// @brief alignes array size to 8 byte for performance in SoA and AoS
/// @param typeSize sizeof single entity
/// @param count number of entities
/// @return new array size
inline uint32_t alignTo8(uint32_t typeSize, uint32_t count){
    return (typeSize*count+0x7)&0xFFFFFFF8;
}
#ifdef DEBUG
extern ssize_t allocator_counter;
#endif
template<typename _Tp=uint8_t>
class allocator
{
    public:
    using value_type=_Tp;
    constexpr allocator() { }
    allocator(const allocator&){ }
    allocator& operator=(const allocator&) = default;
    template<typename _Tp1> allocator(const allocator<_Tp1>&) { }
    ~allocator() { }

    _GLIBCXX_NODISCARD
    _Tp* allocate(size_t __n,const void* = static_cast<const void*>(0)) {
        _Tp* ret = nullptr;
        if(likely(__n > 0)){
            if(__n>0xFFFFFFF)  throw std::bad_alloc();
            __n = alignTo64(sizeof(_Tp),(uint32_t)__n);
        #if DOE_WIN32
            ret = (_Tp*) _aligned_malloc(__n,64);
        #else
            ret = (_Tp*) aligned_alloc(64,__n);
        #endif
            if(ret == nullptr) throw std::bad_alloc();
        #ifdef DEBUG
            allocator_counter++;
        #endif
        }
    #ifdef VERBOSE
        printf("allocator::allocate(): %u byte in %p\n",(uint32_t)__n,ret);
    #endif
        return ret;
    }

    // null safe
    void deallocate(void* __p, uint32_t __n=0) {
        (void)__n;
        if(likely(__p != nullptr)){
        #if DOE_WIN32
            _aligned_free(__p);
        #else
            free(__p);
        #endif
        #ifdef VERBOSE
            printf("allocator::deallocate(): %p\n",__p);
        #endif
        #ifdef DEBUG
            allocator_counter--;
        #endif
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

    void destroy(_Tp* __p)
    { __p->~_Tp(); }

    void destroy(_Tp* __p, uint32_t __n)
    { for (;__n;) __p[--__n].~_Tp(); }
private:
    size_t _M_max_size() const {return std::size_t(-1) / sizeof(_Tp);}
};

#endif // BASICS_HPP

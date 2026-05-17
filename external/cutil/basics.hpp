/**
 * this file includes minimum required C++ tools
 */

#if !defined(BASICS_HPP)
#define BASICS_HPP

#ifdef DEBUG
#include <stdio.h>
#endif
//memory allocation
#include <stdlib.h>
// heavy dependancy
#include <stdint.h>
// memset and memcpy are part of this language :)
#include <string.h>
// heavily dependant
#include <stdexcept>
// new () T() should be global
#include <memory>
#include "ECS/Base/Constants.hpp"

#if defined(__GNUC__) || defined(__clang__)
#define PACK(x) x __attribute__((__packed__))
#elif defined(_MSC_VER)
#define PACK(x) x __pragma(pack(push, 1)) x __pragma(pack(pop))
#else
#error Unsupported compiler
#endif

#if defined(_WIN32) || (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
#define DOE_WIN32 1
#define DOE_UNIX 0
#else
#define DOE_WIN32 0
#define DOE_UNIX 1
#endif
#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif


/// @brief alignes the size to "Constants::CacheLineSize" bytes for perfermance, avoiding false sharing, same cache line race condition issues
/// @attention see ECS::Constants::CacheLineMask for more details.
inline uint32_t alignCacheLineSize(uint32_t size){
    return (size+ECS::Constants::CacheLineFit)&ECS::Constants::CacheLineMask;
}
inline uint32_t alignPointerSize(uint32_t size){
    return (size+((uint32_t)sizeof(void*)-1))&(~((uint32_t)sizeof(void*)-1));
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

    [[nodiscard]]
    _Tp* allocate(size_t __n,const void* = static_cast<const void*>(0)) {
        _Tp* ret;
        __n = sizeof(_Tp) * __n;
        if(__n < 1)      return nullptr;
        if(__n>0x100000)  throw std::bad_alloc();
        #if DOE_WIN32
            ret = (_Tp*) _aligned_malloc(__n,ECS::Constants::CacheLineSize);
        #else
            ret = (_Tp*) aligned_alloc(ECS::Constants::CacheLineSize,__n);
        #endif
            if(ret == nullptr) throw std::bad_alloc();
        #ifdef DEBUG
            allocator_counter++;
        #endif
    #ifdef DEBUG
        printf("allocator::allocate(): %u byte in %p\n",(uint32_t)__n,ret);
    #endif
        return ret;
    }

    // null safe
    void deallocate(void* __p, uint32_t __n=0) {
        (void)__n;
        if(likely(__p != nullptr)){
        #ifdef DEBUG
            printf("allocator::deallocate(): %p\n",__p);
        #endif
        #if DOE_WIN32
            _aligned_free(__p);
        #else
            free(__p);
        #endif
        #ifdef DEBUG
            allocator_counter--;
        #endif
        }

    }

    bool operator==(const allocator&) { return true; }
    bool operator!=(const allocator&) { return false; }


    _Tp* address(_Tp& __x) const _GLIBCXX_NOEXCEPT
    { return std::addressof(__x); }

    const _Tp* address(const _Tp& __x) const _GLIBCXX_NOEXCEPT
    { return std::addressof(__x); }

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

/// @brief smart pointer to hold ownership of an aligned pointer.
/// @tparam Type required to know the size, and destructor function
/// @brief better be safe than sorry
template <typename Type>
class align_ptr
{
    Type *_M_t = nullptr;

    public:
    using pointer	= Type*;
    using element_type = Type;

    public:
    // Constructors.

    /// @brief Default constructor, creates a align_ptr that owns nothing.
    constexpr align_ptr() noexcept : _M_t(nullptr){ }
    
    /** Takes ownership of a pointer.
     *
     * @param __p  A pointer to an object of @c element_type
     */
    explicit align_ptr(Type* __p) noexcept : _M_t(__p){ }

    /// @brief Creates a align_ptr that owns nothing.
    constexpr align_ptr(std::nullptr_t) noexcept : _M_t(nullptr) { }

    /// @brief Move constructor.
    align_ptr(align_ptr&&v){
        reset(v._M_t);
        v._M_t = nullptr;
    }

    /// @brief Destructor, invokes the deleter if the stored pointer is not null.
    ~align_ptr() noexcept
    {
        if(_M_t != nullptr){
            if(!std::is_trivial_v<Type>)
                _M_t->~Type();
            allocator().deallocate(_M_t);
            _M_t = nullptr;
        }
    }

    // Assignment.

    /** @brief Move assignment operator.
     *
     * Invokes the deleter if this object owns a pointer.
     */
    align_ptr& operator=(align_ptr &&v){
        if(this != &v){
            this->reset(v._M_t);
            v._M_t = nullptr;
        }
        return *this;
    }

    /// @brief Reset the %align_ptr to empty, invoking the deleter if necessary.
    inline align_ptr& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    // Observers.

    inline Type* operator->() noexcept
    {
        return get();
    }

    inline const Type* operator->() const noexcept
    {
        return get();
    }

    /// @brief Return the stored pointer as a refrence. safe but slow
    Type& operator*()
    {
        if(unlikely(_M_t == nullptr))
            throw std::runtime_error("operator*(): cant obtain invalid pointer");
        return *_M_t;
    }

    /// @brief Return the stored pointer as a refrence. safe but slow
    const Type& operator*() const
    {
        if(unlikely(_M_t == nullptr))
            throw std::runtime_error("operator*(): cant obtain invalid pointer");
        return *_M_t;
    }

    /// @brief Return the stored pointer.
    inline Type* get() {
        return _M_t;
    }

    /// @brief Return the stored pointer.
    const Type* get() const {
        return _M_t;
    }

    /// @returns true if the stored pointer is not null.
    explicit inline operator bool() const noexcept { return get() == nullptr ? false : true; }

    // Modifiers.

    /// @brief Release ownership of any stored pointer.
    Type* release() noexcept {
        Type * const val = _M_t;
        _M_t = nullptr;
        return val;
    }

    /** @brief Replace the stored pointer.
     *
     * @param __p  The new pointer to store.
     *
     * The deleter will be invoked if a pointer is already owned.
     */
    void reset(Type *__p = nullptr) noexcept
    {
        if (_M_t != nullptr){
            if(!std::is_trivial_v<Type>)
                _M_t->~Type();
            allocator().deallocate(_M_t);
        }
        _M_t = __p;
    }

    /// @brief Exchange the pointer and deleter with another object.
    void swap(align_ptr& __u) noexcept
    {
        Type * const buffer = __u._M_t;
        __u._M_t = this->_M_t;
        this->_M_t = buffer;
    }

    /// @brief Disable copy from lvalue.
    align_ptr(const align_ptr&) = delete;
    align_ptr& operator=(const align_ptr&) = delete;
};
static_assert(sizeof(align_ptr<int>) == sizeof(int*));

/// @brief smart pointer to hold ownership of an aligned array pointer.
/// @warning since this class does not store the array size, it is not responsible for calling constructor/destructor, anyhow.
template <typename Type>
class align_ptr<Type[]>
{
    Type *_M_t = nullptr;

    public:
    using pointer	= Type*;
    using element_type = Type;

    public:
    // Constructors.

    /// @brief Default constructor, creates an align_ptr that owns nothing.
    constexpr align_ptr() noexcept : _M_t(nullptr){ }

    /** Takes ownership of a pointer.
     *
     * @param __p  A pointer to an object of @c element_type
     */
    explicit align_ptr(Type* __p) noexcept : _M_t(__p){ }

    /// @brief Creates a align_ptr that owns nothing.
    constexpr align_ptr(std::nullptr_t) noexcept : _M_t(nullptr) { }

    /// @brief Move constructor.
    align_ptr(align_ptr&&v){
        reset(v._M_t);
        v._M_t = nullptr;
    }

    /// @brief Destructor, invokes the deleter if the stored pointer is not null.
    ~align_ptr() noexcept
    {
        if(_M_t != nullptr)
            allocator().deallocate(_M_t);
        _M_t = nullptr;
    }

    // Assignment.

    /** @brief Move assignment operator.
     *
     * Invokes the deleter if this object owns a pointer.
     */
    align_ptr& operator=(align_ptr &&v){
        if(this != &v){
            this->reset(v._M_t);
            v._M_t = nullptr;
        }
        return *this;
    }

    /// @brief Reset the %align_ptr to empty, invoking the deleter if necessary.
    inline align_ptr& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    // Observers.

    inline operator pointer() noexcept
    {
        if(unlikely(_M_t == nullptr))
            throw std::runtime_error("operator*(): cant obtain invalid pointer");
        return _M_t;
    }

    /// Access an element of owned array.
    _GLIBCXX23_CONSTEXPR
    const Type& operator[](size_t __i) const
    {
        if(unlikely(_M_t == nullptr))
            throw std::runtime_error("operator[](): cant obtain invalid pointer");
        return get()[__i];
    }

    /// Access an element of owned array.
    _GLIBCXX23_CONSTEXPR
    Type& operator[](size_t __i)
    {
        if(unlikely(_M_t == nullptr))
            throw std::runtime_error("operator[](): cant obtain invalid pointer");
        return get()[__i];
    }

    /// @brief Return the stored pointer. safe but slow
    Type* get() {
        if(_M_t == nullptr)
            return nullptr;
        return _M_t;
    }

    /// @brief Return the stored pointer. safe but slow
    const Type* get() const {
        if(_M_t == nullptr)
            return nullptr;
        return _M_t;
    }

    // Modifiers.

    /// @brief Release ownership of any stored pointer.
    Type* release() noexcept {
        Type * const val = _M_t;
        _M_t = nullptr;
        return val;
    }

    /** @brief Replace the stored pointer.
     *
     * @param __p  The new pointer to store.
     *
     * The deleter will be invoked if a pointer is already owned.
     */
    void reset(Type *__p = nullptr) noexcept
    {
        if (_M_t != nullptr){
            allocator().deallocate(_M_t);
        }
        _M_t = __p;
    }

    /// @brief Exchange the pointer and deleter with another object.
    void swap(align_ptr& __u) noexcept
    {
        Type * const buffer = __u._M_t;
        __u._M_t = this->_M_t;
        this->_M_t = buffer;
    }

    /// @brief Disable copy from lvalue.
    align_ptr(const align_ptr&) = delete;
    align_ptr& operator=(const align_ptr&) = delete;
};

namespace detail
{
    template<class>
    constexpr bool is_unbounded_array_v = false;
    template<class T>
    constexpr bool is_unbounded_array_v<T[]> = true;
 
    template<class>
    constexpr bool is_bounded_array_v = false;
    template<class T, std::size_t N>
    constexpr bool is_bounded_array_v<T[N]> = true;
}

template<class T,typename... Args>
std::enable_if_t<!std::is_array<T>::value, align_ptr<T>>
make_align(Args&&... args)
{
    T* ptr = allocator<T>().allocate(1);
    new (ptr) T(args...);
    return align_ptr<T>(ptr);
}

template<class T>
std::enable_if_t<detail::is_unbounded_array_v<T>, align_ptr<T>>
make_align(size_t num)
{
    std::remove_extent_t<T> * __p = allocator<std::remove_extent_t<T>>().allocate(num);
    return align_ptr<T>(__p);
}

template<class T>
std::enable_if_t<detail::is_bounded_array_v<T>>
make_align() = delete;

#endif // BASICS_HPP

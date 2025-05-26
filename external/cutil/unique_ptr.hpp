#if !defined(UNIQUEPTR_HPP)
#define UNIQUEPTR_HPP

#include "basics.hpp"



// supports pointer marking for first bit, it disables destruction
template <typename Type, typename Allocator = ECS::allocator<Type>>
class unique_ptr
{
    Type *_M_t = nullptr;

    public:
    using pointer	= Type*;
    using element_type = Type;

    public:
    // Constructors.

    /// Default constructor, creates a unique_ptr that owns nothing.
    constexpr unique_ptr() noexcept : _M_t(nullptr){ }

    /** Takes ownership of a pointer.
     *
     * @param __p  A pointer to an object of @c element_type
     *
     * The deleter will be value-initialized.
     */
    explicit unique_ptr(Type* __p) noexcept : _M_t(__p){ }

    /// Creates a unique_ptr that owns nothing.
    constexpr unique_ptr(nullptr_t) noexcept : _M_t(nullptr) { }

    /// Move constructor.
    unique_ptr(unique_ptr&&v){
        reset(v._M_t);
        v._M_t = nullptr;
    }

    /// Destructor, invokes the deleter if the stored pointer is not null.
    ~unique_ptr() noexcept
    {
        if(_M_t != nullptr && ((intptr_t)_M_t&1) != 1){
            Allocator().destroy(_M_t);
            Allocator().deallocate(_M_t);
        }
        _M_t = nullptr;
    }

    // Assignment.

    /** @brief Move assignment operator.
     *
     * Invokes the deleter if this object owns a pointer.
     */
    unique_ptr& operator=(unique_ptr &&v){
        if(this != &v){
            _M_t = v._M_t;
            v._M_t = nullptr;
        }
        return *this;
    }

    /// Reset the %unique_ptr to empty, invoking the deleter if necessary.
    unique_ptr&
    operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    // Observers.

    /// @brief Return the stored pointer. safe but slow
    Type* get() const {
        if(_M_t == nullptr || ((intptr_t)_M_t&1) == 1)
            return nullptr;
        return _M_t;
    }
    /// @brief Return the stored pointer. unsafe
    Type* get_raw() const {
        return _M_t;
    }

    /// Return @c true if the stored pointer is not null.
    explicit operator bool() const noexcept { return get() == nullptr ? false : true; }

    // Modifiers.

    /// Release ownership of any stored pointer.
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
        if (_M_t != nullptr && ((intptr_t)_M_t&1) == 0){
            Allocator().destroy(_M_t);
            Allocator().deallocate(_M_t);
        }
        _M_t = __p;
    }

    /// Exchange the pointer and deleter with another object.
    void swap(unique_ptr& __u) noexcept
    {
        Type * const buffer = __u._M_t;
        __u._M_t = this->_M_t;
        this->_M_t = buffer;
    }

    // Disable copy from lvalue.
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
};


#endif // UNIQUEPTR_HPP

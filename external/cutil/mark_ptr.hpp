#if !defined(MARK_PTR_HPP)
#define MARK_PTR_HPP

#include "cutil/basics.hpp"


/// @brief supports pointer marking for first bit, it disables destruction and access
template <typename Type, typename Allocator = allocator<Type>>
class mark_ptr
{
    Type *_M_t = nullptr;

    public:
    using pointer	= Type*;
    using element_type = Type;

    public:
    // Constructors.

    /// @brief Default constructor, creates a mark_ptr that owns nothing.
    constexpr mark_ptr() noexcept : _M_t(nullptr){ }

    /** Takes ownership of a pointer.
     *
     * @param __p  A pointer to an object of @c element_type
     *
     * The deleter will be value-initialized.
     */
    explicit mark_ptr(Type* __p) noexcept : _M_t(__p){ }

    /// @brief Creates a mark_ptr that owns nothing.
    constexpr mark_ptr(nullptr_t) noexcept : _M_t(nullptr) { }

    /// @brief Move constructor.
    mark_ptr(mark_ptr&&v){
        reset(v._M_t);
        v._M_t = nullptr;
    }

    /// @brief Destructor, invokes the deleter if the stored pointer is not null.
    ~mark_ptr() noexcept
    {
        if(_M_t != nullptr && ((intptr_t)_M_t&1) != 1){
            _M_t->~Type();
            Allocator().deallocate(_M_t);
        }
        _M_t = nullptr;
    }

    // Assignment.

    /** @brief Move assignment operator.
     *
     * Invokes the deleter if this object owns a pointer.
     */
    mark_ptr& operator=(mark_ptr &&v){
        if(this != &v){
            _M_t = v._M_t;
            v._M_t = nullptr;
        }
        return *this;
    }

    /// @brief Reset the %mark_ptr to empty, invoking the deleter if necessary.
    inline mark_ptr& operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    // Observers.

    Type& operator*() noexcept
    {
        if(unlikely(_M_t == nullptr || ((intptr_t)_M_t&1) == 1))
            throw std::runtime_error("operator*(): cant obtain invalid pointer");
        return *_M_t;
    }
    
    const Type& operator*() const
    {
        if(unlikely(_M_t == nullptr || ((intptr_t)_M_t&1) == 1))
            throw std::runtime_error("operator*(): cant obtain invalid pointer");
        return *_M_t;
    }

    /// @brief Return the stored pointer. safe but slow
    Type* get() {
        if(_M_t == nullptr || ((intptr_t)_M_t&1) == 1)
            return nullptr;
        return _M_t;
    }

    /// @brief Return the stored pointer. safe but slow
    const Type* get() const {
        if(_M_t == nullptr || ((intptr_t)_M_t&1) == 1)
            return nullptr;
        return _M_t;
    }

    /// @brief Return the stored pointer. unsafe
    inline Type* get_raw() const {
        return _M_t;
    }

    /// Return @c true if the stored pointer is not null.
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
        if (_M_t != nullptr && ((intptr_t)_M_t&1) == 0){
            _M_t->~Type();
            Allocator().deallocate(_M_t);
        }
        _M_t = __p;
    }

    /// @brief Exchange the pointer and deleter with another object.
    void swap(mark_ptr& __u) noexcept
    {
        Type * const buffer = __u._M_t;
        __u._M_t = this->_M_t;
        this->_M_t = buffer;
    }

    /// @brief Disable copy from lvalue.
    mark_ptr(const mark_ptr&) = delete;
    mark_ptr& operator=(const mark_ptr&) = delete;
};


#endif // MARK_PTR_HPP

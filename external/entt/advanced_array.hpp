/**
 * @brief A heavily shrinked version of ENTT library.
 * for single component type.
 */
#ifndef ADVANCED_ARRAY_HPP
#define ADVANCED_ARRAY_HPP 1

#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif

namespace advanced_array {

/*! @brief Alias declaration for type identifiers. */
using id_type = std::uint32_t;


/**
 * @brief Entity traits.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is an accepted entity type.
 */
struct entt_traits;

/**
 * @brief Entity traits for a 32 bits entity identifier.
 *
 * A 32 bits entity identifier
 */
struct entt_traits {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint32_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint16_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 20u;
};

/**
 * @brief Converts an entity type to its underlying type.
 * @tparam Entity The value type.
 * @param entity The value to convert.
 * @return The integral representation of the given value.
 */
template<typename Entity>
[[nodiscard]] constexpr
auto to_integral(const Entity entity) noexcept {
    return static_cast<typename entt_traits::entity_type>(entity);
}

/*! @brief Null object for all entity identifiers.  */
struct null_t
{
    /** @brief Converts the null object to identifiers of any type.*/
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept { return Entity{entt_traits::entity_mask}; }

    [[nodiscard]] constexpr bool operator==(const null_t &) const noexcept {return true;}
    [[nodiscard]] constexpr bool operator!=(const null_t &) const noexcept {return false;}
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity &entity) const noexcept { return !(*this == entity); }
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity &entity) const noexcept {
        return to_integral(entity) == to_integral(static_cast<Entity>(*this));
    }
};



/**
 * @brief A class to use to push around lists of constant values, nothing more.
 * @tparam Value Values provided by the value list.
 */
template<auto... Value>
struct value_list {
    /*! @brief Value list type. */
    using type = value_list;
    /*! @brief Compile-time number of elements in the value list. */
    static constexpr auto size = sizeof...(Value);
};
/**
 * @brief A class to use to push around lists of types, nothing more.
 * @tparam Type Types provided by the type list.
 */
template<typename... Type>
struct type_list {
    /*! @brief Type list type. */
    using type = type_list;
    /*! @brief Compile-time number of elements in the type list. */
    static constexpr auto size = sizeof...(Type);
};



/*! @brief Primary template isn't defined on purpose. */
template<std::size_t, typename>
struct value_list_element;

/**
 * @brief Provides compile-time indexed access to the values of a value list.
 * @tparam Index Index of the value to return.
 * @tparam Value First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<std::size_t Index, auto Value, auto... Other>
struct value_list_element<Index, value_list<Value, Other...>>
    : value_list_element<Index - 1u, value_list<Other...>>
{};

/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam Value First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<auto Value, auto... Other>
struct value_list_element<0u, value_list<Value, Other...>> {
    /*! @brief Searched value. */
    static constexpr auto value = Value;
};
template<std::size_t Index, typename List>
inline constexpr auto value_list_element_v = value_list_element<Index, List>::value;








/**
 * @brief Transcribes the constness of a type to another type.
 * @tparam To The type to which to transcribe the constness.
 * @tparam From The type from which to transcribe the constness.
 */
template<typename To, typename From>
struct constness_as {
    /*! @brief The type resulting from the transcription of the constness. */
    using type = std::remove_const_t<To>;
};
/*! @copydoc constness_as */
template<typename To, typename From>
struct constness_as<To, const From> {
    /*! @brief The type resulting from the transcription of the constness. */
    using type = std::add_const_t<To>;
};
/**
 * @brief Alias template to facilitate the transcription of the constness.
 * @tparam To The type to which to transcribe the constness.
 * @tparam From The type from which to transcribe the constness.
 */
template<typename To, typename From>
using constness_as_t = typename constness_as<To, From>::type;





template<std::size_t N>
struct choice_t : choice_t<N-1>
{};

template<>
struct choice_t<0> {};

template<std::size_t N>
inline constexpr choice_t<N> choice{};






namespace internal {
    template<typename>
    [[nodiscard]] constexpr bool is_equality_comparable(...) { return false; }

    template<typename Type>
    [[nodiscard]] constexpr auto is_equality_comparable(choice_t<0>)
    -> decltype(std::declval<Type>() == std::declval<Type>()) { return true; }

    template<typename Type>
    [[nodiscard]] constexpr auto is_equality_comparable(choice_t<1>)
    -> decltype(std::declval<typename Type::value_type>(), std::declval<Type>() == std::declval<Type>()) {
        return is_equality_comparable<typename Type::value_type>(choice<2>);
    }

    template<typename Type>
    [[nodiscard]] constexpr auto is_equality_comparable(choice_t<2>)
    -> decltype(std::declval<typename Type::mapped_type>(), std::declval<Type>() == std::declval<Type>()) {
        return is_equality_comparable<typename Type::key_type>(choice<2>) && is_equality_comparable<typename Type::mapped_type>(choice<2>);
    }
}

/**
 * @brief Provides the member constant `value` to true if a given type is
 * equality comparable, false otherwise.
 * @tparam Type Potentially equality comparable type.
 */
template<typename Type, typename = void>
struct is_equality_comparable: std::bool_constant<internal::is_equality_comparable<Type>(choice<2>)> {};

template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;



/// @brief Compile-time constant for null entities.
inline constexpr null_t null{};

template<std::size_t Len, std::size_t = alignof(typename std::aligned_storage_t<Len + !Len>)>
class basic_any;

/**
 * @brief A SBO friendly, type-safe container for single values of any type.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 */
template<std::size_t Len, std::size_t Align>
class basic_any {
    enum class operation { COPY, MOVE, DTOR, COMP, ADDR, CADDR, REF, CREF };

    using storage_type = std::aligned_storage_t<Len + !Len, Align>;
    using vtable_type = const void *(const operation, const basic_any &, const void *);

    template<typename Type>
    static constexpr bool in_situ = Len && alignof(Type) <= alignof(storage_type) && sizeof(Type) <= sizeof(storage_type) && std::is_nothrow_move_constructible_v<Type>;

    template<typename Type>
    [[nodiscard]] static bool compare(const void *lhs, const void *rhs) {
        if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
            return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
        } else {
            return lhs == rhs;
        }
    }

    template<typename Type>
    static Type & as(const void *to) {
        return *const_cast<Type *>(static_cast<const Type *>(to));
    }

    template<typename Type>
    static const void * basic_vtable([[maybe_unused]] const operation op, [[maybe_unused]] const basic_any &from, [[maybe_unused]] const void *to) {
        if constexpr(std::is_void_v<Type>) {
            switch(op) {
            case operation::COPY:
            case operation::MOVE:
            case operation::REF:
            case operation::CREF:
                as<basic_any>(to).vtable = from.vtable;
                break;
            default:
                break;
            }
        } else if constexpr(std::is_lvalue_reference_v<Type>) {
            using base_type = std::decay_t<Type>;

            switch(op) {
            case operation::COPY:
                if constexpr(std::is_copy_constructible_v<base_type>) {
                    as<basic_any>(to) = *static_cast<const base_type *>(from.instance);
                }
                break;
            case operation::MOVE:
                as<basic_any>(to).instance = from.instance;
                as<basic_any>(to).vtable = from.vtable;
                [[fallthrough]];
            case operation::DTOR:
                break;
            case operation::COMP:
                return compare<base_type>(from.instance, to) ? to : nullptr;
            case operation::ADDR:
                return std::is_const_v<std::remove_reference_t<Type>> ? nullptr : from.instance;
            case operation::CADDR:
                return from.instance;
            case operation::REF:
                as<basic_any>(to).instance = from.instance;
                as<basic_any>(to).vtable = basic_vtable<Type>;
                break;
            case operation::CREF:
                as<basic_any>(to).instance = from.instance;
                as<basic_any>(to).vtable = basic_vtable<const base_type &>;
                break;
            }
        } else if constexpr(in_situ<Type>) {
            #if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606L
            auto *instance = const_cast<Type *>(std::launder(reinterpret_cast<const Type *>(&from.storage)));
            #else
            auto *instance = const_cast<Type *>(reinterpret_cast<const Type *>(&from.storage));
            #endif

            switch(op) {
            case operation::COPY:
                if constexpr(std::is_copy_constructible_v<Type>) {
                    new (&as<basic_any>(to).storage) Type{std::as_const(*instance)};
                    as<basic_any>(to).vtable = from.vtable;
                }
                break;
            case operation::MOVE:
                new (&as<basic_any>(to).storage) Type{std::move(*instance)};
                as<basic_any>(to).vtable = from.vtable;
                break;
            case operation::DTOR:
                instance->~Type();
                break;
            case operation::COMP:
                return compare<Type>(instance, to) ? to : nullptr;
            case operation::ADDR:
            case operation::CADDR:
                return instance;
            case operation::REF:
                as<basic_any>(to).instance = instance;
                as<basic_any>(to).vtable = basic_vtable<Type &>;
                break;
            case operation::CREF:
                as<basic_any>(to).instance = instance;
                as<basic_any>(to).vtable = basic_vtable<const Type &>;
                break;
            }
        } else {
            switch(op) {
            case operation::COPY:
                if constexpr(std::is_copy_constructible_v<Type>) {
                    as<basic_any>(to).instance = new Type{*static_cast<const Type *>(from.instance)};
                    as<basic_any>(to).vtable = from.vtable;
                }
                break;
            case operation::MOVE:
                as<basic_any>(to).instance = std::exchange(as<basic_any>(&from).instance, nullptr);
                as<basic_any>(to).vtable = from.vtable;
                break;
            case operation::DTOR:
                if constexpr(std::is_array_v<Type>) {
                    delete[] static_cast<const Type *>(from.instance);
                } else {
                    delete static_cast<const Type *>(from.instance);
                }
                break;
            case operation::COMP:
                return compare<Type>(from.instance, to) ? to : nullptr;
            case operation::ADDR:
            case operation::CADDR:
                return from.instance;
            case operation::REF:
                as<basic_any>(to).instance = from.instance;
                as<basic_any>(to).vtable = basic_vtable<Type &>;
                break;
            case operation::CREF:
                as<basic_any>(to).instance = from.instance;
                as<basic_any>(to).vtable = basic_vtable<const Type &>;
                break;
            }
        }

        return nullptr;
    }

    template<typename Type, typename... Args>
    void initialize([[maybe_unused]] Args &&... args) {
        if constexpr(!std::is_void_v<Type>) {
            if constexpr(std::is_lvalue_reference_v<Type>) {
                static_assert(sizeof...(Args) == 1u && (std::is_lvalue_reference_v<Args> && ...), "Invalid arguments");
                instance = (std::addressof(args), ...);
            } else if constexpr(in_situ<Type>) {
                if constexpr(std::is_aggregate_v<Type>) {
                    new (&storage) Type{std::forward<Args>(args)...};
                } else {
                    new (&storage) Type(std::forward<Args>(args)...);
                }
            } else {
                if constexpr(std::is_aggregate_v<Type>) {
                    instance = new Type{std::forward<Args>(args)...};
                } else {
                    instance = new Type(std::forward<Args>(args)...);
                }
            }
        }
    }

public:
    /*! @brief Default constructor. */
    basic_any() ENTT_NOEXCEPT
        : basic_any{std::in_place_type<void>}
    {}

    /**
     * @brief Constructs an any by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit basic_any(std::in_place_type_t<Type>, Args &&... args)
        : instance{},
          vtable{&basic_vtable<Type>}
    {
        initialize<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Constructs an any that holds an unmanaged object.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type>
    basic_any(std::reference_wrapper<Type> value) ENTT_NOEXCEPT
        : basic_any{std::in_place_type<Type &>, value.get()}
    {}

    /**
     * @brief Constructs an any from a given value.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any(Type &&value)
        : basic_any{std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)}
    {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    basic_any(const basic_any &other)
        : basic_any{std::in_place_type<void>}
    {
        other.vtable(operation::COPY, other, this);
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_any(basic_any &&other) ENTT_NOEXCEPT
        : basic_any{std::in_place_type<void>}
    {
        other.vtable(operation::MOVE, other, this);
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~basic_any() {
        vtable(operation::DTOR, *this, nullptr);
    }

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This any object.
     */
    basic_any & operator=(const basic_any &other) {
        vtable(operation::DTOR, *this, nullptr);
        other.vtable(operation::COPY, other, this);
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This any object.
     */
    basic_any & operator=(basic_any &&other) {
        vtable(operation::DTOR, *this, nullptr);
        other.vtable(operation::MOVE, other, this);
        return *this;
    }

    /**
     * @brief Value assignment operator.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     * @return This any object.
     */
    template<typename Type>
    basic_any & operator=(std::reference_wrapper<Type> value) ENTT_NOEXCEPT {
        emplace<Type &>(value.get());
        return *this;
    }

    /**
     * @brief Value assignment operator.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @param value An instance of an object to use to initialize the wrapper.
     * @return This any object.
     */
    template<typename Type>
    std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>, basic_any &>
    operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void * data() const ENTT_NOEXCEPT {
        return vtable(operation::CADDR, *this, nullptr);
    }

    /*! @copydoc data */
    [[nodiscard]] void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(vtable(operation::ADDR, *this, nullptr));
    }

    /**
     * @brief Replaces the contained object by creating a new instance directly.
     * @tparam Type Type of object to use to initialize the wrapper.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        std::exchange(vtable, &basic_vtable<Type>)(operation::DTOR, *this, nullptr);
        initialize<Type>(std::forward<Args>(args)...);
    }

    /*! @brief Destroys contained object */
    void reset() {
        std::exchange(vtable, &basic_vtable<void>)(operation::DTOR, *this, nullptr);
    }

    /**
     * @brief Returns false if a wrapper is empty, true otherwise.
     * @return False if the wrapper is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(vtable(operation::CADDR, *this, nullptr) == nullptr);
    }

    /**
     * @brief Checks if two wrappers differ in their content.
     * @param other Wrapper with which to compare.
     * @return False if the two objects differ in their content, true otherwise.
     */
    bool operator==(const basic_any &other) const ENTT_NOEXCEPT {
        return vtable(operation::COMP, *this, other.data()) == other.data();
    }

    /**
     * @brief Aliasing constructor.
     * @return An any that shares a reference to an unmanaged object.
     */
    [[nodiscard]] basic_any as_ref() ENTT_NOEXCEPT {
        basic_any ref{};
        vtable(operation::REF, *this, &ref);
        return ref;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_any as_ref() const ENTT_NOEXCEPT {
        basic_any ref{};
        vtable(operation::CREF, *this, &ref);
        return ref;
    }

private:
    union { const void *instance; storage_type storage; };
    vtable_type *vtable;
};

template<typename Entity>
class basic_sparse_set {

    static constexpr auto page_size = 4096;

    using traits_type = entt_traits;
    using page_type = std::unique_ptr<Entity[]>;

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

protected:

    class sparse_set_iterator final {
        friend class basic_sparse_set<Entity>;

        using packed_type = std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        sparse_set_iterator(const packed_type &ref, const index_type idx) ENTT_NOEXCEPT
            : packed{&ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        sparse_set_iterator() ENTT_NOEXCEPT = default;

        sparse_set_iterator & operator++() ENTT_NOEXCEPT {return --index, *this;}
        sparse_set_iterator & operator--() ENTT_NOEXCEPT {return ++index, *this;}
        sparse_set_iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {index -= value;return *this;}
        sparse_set_iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {return (*this += -value);}
        [[nodiscard]] reference operator[](const difference_type value) const {const auto pos = size_type(index-value-1u);return (*packed)[pos];}
        [[nodiscard]] bool operator==(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return other.index == index;}
        [[nodiscard]] bool operator!=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return !(*this == other);}
        [[nodiscard]] bool operator< (const sparse_set_iterator &other) const ENTT_NOEXCEPT {return index > other.index;}
        [[nodiscard]] bool operator> (const sparse_set_iterator &other) const ENTT_NOEXCEPT {return index < other.index;}
        [[nodiscard]] bool operator<=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return !(*this > other);}
        [[nodiscard]] bool operator>=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return !(*this < other);}
        [[nodiscard]] pointer operator->() const {const auto pos = size_type(index-1u);return &(*packed)[pos];}
        [[nodiscard]] reference operator*() const {return *operator->();}
    private:
        const packed_type *packed;
        index_type index;
    };

    [[nodiscard]] auto page(const Entity entt) const ENTT_NOEXCEPT {
        return size_type{(to_integral(entt) & traits_type::entity_mask) / page_size};
    }

    [[nodiscard]] auto offset(const Entity entt) const ENTT_NOEXCEPT {
        return size_type{to_integral(entt) & (page_size - 1)};
    }

    [[nodiscard]] page_type & assure(const std::size_t pos) {
        if(!(pos < sparse.size())) {
            sparse.resize(pos+1);
        }

        if(!sparse[pos]) {
            sparse[pos].reset(new entity_type[page_size]);
            // null is safe in all cases for our purposes
            for(auto *first = sparse[pos].get(), *last = first + page_size; first != last; ++first) {
                *first = null;
            }
        }

        return sparse[pos];
    }

protected:
    /*! @brief Swaps two entities in the internal packed array. */
    virtual void swap_at(const std::size_t, const std::size_t) {}

    /*! @brief Attempts to remove an entity from the internal packed array. */
    virtual void swap_and_pop(const std::size_t) {}

public:

    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = const entity_type *;

    /*! @brief Default constructor. */
    basic_sparse_set() = default;

    /*! @brief Default move constructor. */
    basic_sparse_set(basic_sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~basic_sparse_set() = default;

    /*! @brief Default move assignment operator. @return This sparse set. */
    basic_sparse_set & operator=(basic_sparse_set &&) = default;

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        packed.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return packed.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        // conservative approach
        if(packed.empty()) {
            sparse.clear();
        }

        sparse.shrink_to_fit();
        packed.shrink_to_fit();
    }

    /**
     * @brief Returns the extent of a sparse set.
     *
     * The extent of a sparse set is also the size of the internal sparse array.
     * There is no guarantee that the internal packed array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Extent of the sparse set.
     */
    [[nodiscard]] size_type extent() const ENTT_NOEXCEPT {
        return sparse.size() * page_size;
    }

    /**
     * @brief Returns the number of elements in a sparse set.
     *
     * The number of elements is also the size of the internal packed array.
     * There is no guarantee that the internal sparse array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Number of elements.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return packed.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return packed.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Entities are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the internal packed array.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return packed.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first entity of the internal packed
     * array. If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @return An iterator to the first entity of the internal packed array.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = packed.size();
        return iterator{packed, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * internal packed array.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return iterator{packed, {}};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * The returned iterator points to the first entity of the reversed internal
     * packed array. If the sparse set is empty, the returned iterator will be
     * equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed internal packed
     * array.
     */
    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return packed.data();
    }

    /**
     * @brief Returns a reverse iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the reversed internal packed array. Attempting to dereference the
     * returned iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * reversed internal packed array.
     */
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return rbegin() + packed.size();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        const auto curr = page(entt);
        // testing against null permits to avoid accessing the packed array
        return (curr < sparse.size() && sparse[curr] && null != sparse[curr][offset(entt)]);
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    [[nodiscard]] size_type index(const entity_type entt) const {
        if(!contains(entt))
            throw std::invalid_argument("entity does not exist");
        return size_type{to_integral(sparse[page(entt)][offset(entt)])};
    }

    /**
     * @brief Returns the entity at specified location, with bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location if any, a null entity otherwise.
     */
    [[nodiscard]] entity_type at(const size_type pos) const {
        return pos < packed.size() ? packed[pos] : null;
    }

    /**
     * @brief Returns the entity at specified location, without bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        if(pos >= packed.size())
            throw std::out_of_range("invalid range");
        return packed[pos];
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     */
    void emplace(const entity_type entt) {
        if(contains(entt))
            throw std::invalid_argument("entity already exists");
        assure(page(entt))[offset(entt)] = entity_type{static_cast<typename traits_type::entity_type>(packed.size())};
        packed.push_back(entt);
    }

    /**
     * @brief Assigns one or more entities to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void insert(It first, It last) {
        auto next = static_cast<typename traits_type::entity_type>(packed.size());
        packed.insert(packed.end(), first, last);

        for(; first != last; ++first) {
            if(contains(*first))
                throw std::invalid_argument("entity already exists");
            assure(page(*first))[offset(*first)] = entity_type{next++};
        }
    }

    /**
     * @brief Removes an entity from a sparse set.
     *
     * @warning
     * Attempting to remove an entity that doesn't belong to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void remove(const entity_type entt) {
        if(!contains(entt))
            throw std::invalid_argument("entity does not exist");
        auto &ref = sparse[page(entt)][offset(entt)];

        // last chance to use the entity for derived classes and mixins, if any
        swap_and_pop(size_type{to_integral(ref)});

        const auto other = packed.back();
        sparse[page(other)][offset(other)] = ref;
        // if it looks weird, imagine what the subtle bugs it prevents are
        packed.back() = entt;
        if(false)
            throw std::invalid_argument("entity does not exist");
        packed[size_type{to_integral(ref)}] = other;
        ref = null;

        packed.pop_back();
    }

    /**
     * @brief Removes multiple entities from a pool.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    template<typename It>
    void remove(It first, It last, void *ud = nullptr) {
        for(; first != last; ++first) {
            remove(*first, ud);
        }
    }

    /**
     * @copybrief swap_at
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.
     *
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    void swap(const entity_type lhs, const entity_type rhs) {
        const auto from = index(lhs);
        const auto to = index(rhs);
        std::swap(sparse[page(lhs)][offset(lhs)], sparse[page(rhs)][offset(rhs)]);
        std::swap(packed[from], packed[to]);
        swap_at(from, to);
    }

    /**
     * @brief Sort entities according to their order in another sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantees on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const basic_sparse_set &other) {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = packed.size() - 1;

        while(pos && from != to) {
            if(contains(*from)) {
                if(*from != packed[pos]) {
                    swap(packed[pos], *from);
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Clears a sparse set.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void clear(void *ud = nullptr) ENTT_NOEXCEPT {
        remove(begin(), end(), ud);
    }

private:
    std::vector<page_type> sparse;
    std::vector<entity_type> packed;

};

template<typename, typename>
class basic_storage;

/**
 * @brief Basic storage implementation.
 *
 * This class is a refinement of a sparse set that associates an object to an
 * entity. The main purpose of this class is to extend sparse sets to store
 * components in a registry. It guarantees fast access both to the elements and
 * to the entities.
 *
 * @note
 * Entities and objects have the same order. It's guaranteed both in case of raw
 * access (either to entities or objects) and when using random or input access
 * iterators.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that objects are returned in the insertion order when iterate
 * a storage. Do not make assumption on the order in any case.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Therefore, many of the functions
 * normally available for non-empty types will not be available for empty ones.
 *
 * @sa sparse_set<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
class basic_storage: public basic_sparse_set<Entity> {

    static_assert(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>, "The managed type must be at least move constructible and assignable");

    using underlying_type = basic_sparse_set<Entity>;
    using traits_type = entt_traits;

    template<typename Value>
    class storage_iterator final {};

protected:
    /**
     * @copybrief basic_sparse_set::swap_at
     * @param lhs A valid position of an entity within storage.
     * @param rhs A valid position of an entity within storage.
     */
    void swap_at(const std::size_t lhs, const std::size_t rhs) {
        std::swap(instances[lhs], instances[rhs]);
    }

    /**
     * @copybrief basic_sparse_set::swap_and_pop
     * @param pos A valid position of an entity within storage.
     */
    void swap_and_pop(const std::size_t pos) {
        auto other = std::move(instances.back());
        instances[pos] = std::move(other);
        instances.pop_back();
    }

public:
    /*! @brief Type of the objects assigned to entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = storage_iterator<Type>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = storage_iterator<const Type>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = Type *;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = const Type *;

    /**
     * @brief Increases the capacity of a storage.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        underlying_type::reserve(cap);
        instances.reserve(cap);
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        underlying_type::shrink_to_fit();
        instances.shrink_to_fit();
    }

    /**
     * @brief Direct access to the array of objects.
     *
     * The returned pointer is such that range `[raw(), raw() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Objects are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the array of objects.
     */
    [[nodiscard]] const value_type * raw() const ENTT_NOEXCEPT {
        return instances.data();
    }

    /*! @copydoc raw */
    [[nodiscard]] value_type * raw() ENTT_NOEXCEPT {
        return const_cast<value_type *>(std::as_const(*this).raw());
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the internal array.
     * If the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal array.
     */
    [[nodiscard]] const_iterator cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator{instances, pos};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator{instances, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the internal array. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the element following the last instance of the
     * internal array.
     */
    [[nodiscard]] const_iterator cend() const ENTT_NOEXCEPT {
        return const_iterator{instances, {}};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() ENTT_NOEXCEPT {
        return iterator{instances, {}};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * The returned iterator points to the first instance of the reversed
     * internal array. If the storage is empty, the returned iterator will be
     * equal to `rend()`.
     *
     * @return An iterator to the first instance of the reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crbegin() const ENTT_NOEXCEPT {
        return instances.data();
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() ENTT_NOEXCEPT {
        return instances.data();
    }

    /**
     * @brief Returns a reverse iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the reversed internal array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last instance of the
     * reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crend() const ENTT_NOEXCEPT {
        return crbegin() + instances.size();
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const ENTT_NOEXCEPT {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() ENTT_NOEXCEPT {
        return rbegin() + instances.size();
    }

    /**
     * @brief Returns the object assigned to an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The object assigned to the entity.
     */
    [[nodiscard]] const value_type & get(const entity_type entt) const {
        return instances[underlying_type::index(entt)];
    }

    /*! @copydoc get */
    [[nodiscard]] value_type & get(const entity_type entt) {
        return const_cast<value_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * This version accept both types that can be constructed in place directly
     * and types like aggregates that do not work well with a placement new as
     * performed usually under the hood during an _emplace back_.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    value_type & emplace(const entity_type entt, Args &&... args) {
        if constexpr(std::is_aggregate_v<value_type>) {
            instances.push_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        // entity goes after component in case constructor throws
        underlying_type::emplace(entt);
        return instances.back();
    }

    /**
     * @brief Updates the instance assigned to a given entity in-place.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the updated instance.
     */
    template<typename... Func>
    decltype(auto) patch(const entity_type entity, Func &&... func) {
        auto &&instance = instances[this->index(entity)];
        (std::forward<Func>(func)(instance), ...);
        return instance;
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given instance.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the object to construct.
     */
    template<typename It>
    void insert(It first, It last, const value_type &value = {}) {
        instances.insert(instances.end(), std::distance(first, last), value);
        // entities go after components in case constructors throw
        underlying_type::insert(first, last);
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given range.
     *
     * @sa construct
     *
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param from An iterator to the first element of the range of objects.
     * @param to An iterator past the last element of the range of objects.
     */
    template<typename EIt, typename CIt>
    void insert(EIt first, EIt last, CIt from, CIt to) {
        instances.insert(instances.end(), from, to);
        // entities go after components in case constructors throw
        underlying_type::insert(first, last);
    }

private:
    std::vector<value_type> instances;
};

template<typename,typename>
class basic_registry;

/**
 * @brief Defines the component-to-storage conversion.
 * Mixin type to use to add signal support to storage types.
 * it is a shortcut to storage_traits<~>::storage_type
 *
 * Formally:
 *
 * * If the component type is a non-const one, the member typedef type is the
 *   declared storage type.
 * * If the component type is a const one, the member typedef type is the
 *   declared storage type, except it has a const-qualifier added.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
struct storage_traits_type final: public basic_storage<Entity, Type>
{
    /* The type of the underlying storage. */
    using SType = basic_storage<Entity, Type>;
public:
    /*! @brief Underlying value type. */
    using value_type = typename SType::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename SType::entity_type;

    template<typename... Args>
    auto & emplace(const entity_type entity, Args &&... args) {
        SType::emplace(entity, std::forward<Args>(args)...);
        return this->get(entity);
    }
};

/**
 * @brief View.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename,typename>
class basic_view;

/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, the given
 *   component is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pool iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share a reference to the underlying data structure of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class basic_view final {
    using storage_type = storage_traits_type<Entity, Component>;

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = Component;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename basic_sparse_set<Entity>::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename basic_sparse_set<Entity>::reverse_iterator;

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_view() ENTT_NOEXCEPT
        : pools{}
    {}

    /**
     * @brief Constructs a single-type view from a storage class.
     * @param ref The storage for the type to iterate.
     */
    basic_view(storage_type &ref) ENTT_NOEXCEPT
        : pools{&ref}
    {}

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return pools->size();
    }

    /**
     * @brief Checks whether a view is empty.
     * @return True if the view is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return pools->empty();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of components.
     */
    [[nodiscard]] raw_type * raw() const ENTT_NOEXCEPT {
        return pools->raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return pools->data();
    }


    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT
        {return pools->basic_sparse_set<entity_type>::begin();}
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT
        {return pools->basic_sparse_set<entity_type>::end();}

    /**
     * @brief Returns the first entity of the view, if any.
     * @return The first entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the view, if any.
     * @return The last entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = end();
        return it != begin() ? *(--it) : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        const auto it = pools->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a view is properly initialized.
     * @return True if the view is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return  pools != nullptr;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return pools->contains(entt);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The component assigned to the entity.
     */
    [[nodiscard]]
    decltype(auto) get(const entity_type entt) const {
        if(!contains(entt))
            throw std::invalid_argument("entity does not exist");
        return pools->get(entt);
    }
private:
    storage_type * pools;
};

/**
 * @brief Fast and reliable entity-component system.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components, then create
 * views or groups to iterate them.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity,typename Component>
class basic_registry
{
    static_assert(sizeof(Component) > 0, "Exclusion-only views are not supported");
    static_assert(!std::is_const_v<Component>, "Invalid const type");


    using traits_type = entt_traits;
    using storage_type = storage_traits_type<Entity, Component>;

public:

    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    using version_type = typename traits_type::version_type;
    /*! @brief Underlying entity identifier. */
    using value_type = Component;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

private:

    [[nodiscard]]
    const storage_type & pool() const
    {
        return pool_pool;
    }

    [[nodiscard]]
    storage_type & pool()
    {
        return pool_pool;
    }

    entity_type generate_identifier()
    {
        // traits_type::entity_mask is reserved to allow for null identifiers
        if(static_cast<typename traits_type::entity_type>(entities.size()) >= traits_type::entity_mask)
            throw std::runtime_error("run out of entity number");
        return entities.emplace_back(entity_type{static_cast<typename traits_type::entity_type>(entities.size())});
    }

    entity_type recycle_identifier()
    {
        const auto curr = to_integral(available);
        const auto version = to_integral(entities[curr]) & (traits_type::version_mask << traits_type::entity_shift);
        available = entity_type{to_integral(entities[curr]) & traits_type::entity_mask};
        return entities[curr] = entity_type{curr | version};
    }

    void release_entity(const Entity entity)
    {
        const auto entt = to_integral(entity) & traits_type::entity_mask;
        entities[entt] = entity_type{
            to_integral(available) | 
            (typename traits_type::entity_type{static_cast<typename traits_type::version_type>(version(entity) + 1u)} << traits_type::entity_shift)
        };
        available = entity_type{entt};
    }

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    [[nodiscard]] static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity_type{to_integral(entity) & traits_type::entity_mask};
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    [[nodiscard]] static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return version_type(to_integral(entity) >> traits_type::entity_shift);
    }

public:

    /*! @brief Default constructor. */
    basic_registry():pool_pool(){
    };

    /*! @brief Default move constructor. */
    basic_registry(const basic_registry &) = delete;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;


    [[nodiscard]] size_type pool_size() const {
        return pool().size();
    }

    /**
     * @brief Returns the number of entities created so far.
     * @return Number of entities created so far.
     */
    [[nodiscard]] size_t size() const ENTT_NOEXCEPT {
        return entities.size();
    }

    /**
     * @brief Returns the number of entities that a pool has currently
     * allocated space for.
     * @return Capacity of the pool.
     */
    [[nodiscard]] size_type pool_capacity() const {
        return pool().size();
    }

    /**
     * @brief Returns the number of entities that a registry has currently
     * allocated space for.
     * @return Capacity of the registry.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return entities.capacity();
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of possible entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled ones with updated versions.
     *
     * @return A valid entity identifier.
     */
    entity_type create()
        {return null == available ? generate_identifier() : recycle_identifier();}

    /**
     * @brief Assigns the given component to an entity.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the given entity.
     *
     * @warning
     * Attempting to use an invalid entity or to assign a component to an entity
     * that already owns it results in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename... Args>
    decltype(auto) emplace(const entity_type entity, Args &&... args)
    {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        return pool().emplace(entity, std::forward<Args>(args)...);
    }


    /**
     * @brief Destroys an entity.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time.
     *
     * @sa remove_all
     *
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) {
        remove(entity);
        release_entity(entity);
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    [[nodiscard]]
    bool valid(const entity_type entity) const
    {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Removes component from an entity.
     */
    void remove(const entity_type entity) {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        if(pool_pool.contains(entity))
            pool_pool.remove(entity);
    }

    /**
     * @brief Checks if an entity has alocated component.
     * @param entt A valid entity identifier.
     * @return True if entity has component, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return pool_pool.contains(entt);
    }

    /**
     * @brief Returns references to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.
     *
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        return pool().get(entity);
    }

    /*! @copydoc get */
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        return const_cast<Component &>(pool().get(entity));
    }


    /**
     * @brief Returns a view.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Views do their best to iterate the smallest set of candidate entities.
     * In particular:
     *
     * * Single component views are incredibly fast and iterate a packed array
     *   of entities, all of which has the given component.
     * * Multi component views look at the number of entities available for each
     *   component and pick up a reference to the smallest set of candidates to
     *   test for the given components.
     *
     * Views in no way affect the functionalities of the registry nor those of
     * the underlying pools.
     *
     * @note
     * Multi component views are pretty fast. However their performance tend to
     * degenerate when the number of components to iterate grows up and the most
     * of the entities have all the given components.<br/>
     * To get a performance boost, consider using a group instead.
     *
     * @return A newly created view.
     */
    [[nodiscard]] const basic_view<Entity,Component>
    view() const 
    {
        return { pool() };
    }

    [[nodiscard]] basic_view<Entity,Component>
    view() 
    {
        return { pool() };
    }

private:
    mutable storage_type pool_pool;
    std::vector<entity_type> entities{};
    entity_type available{null};
};

/*! @brief Alias declaration for the most common use case. */
using sparse_set = basic_sparse_set<id_type>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using storage = basic_storage<id_type, Args...>;

/*! @brief Alias declaration for the most common use case. */
template<typename Component>
using registry = basic_registry<id_type,Component>;

/*! @brief Default entity identifier. */
using entity = id_type;

}

#endif // ADVANCED_ARRAY_HPP

#ifndef ENTT2_HPP
#define ENTT2_HPP 1

#include <cstdint>
#include <utility>
#include <tuple>
#include <array>
#include <vector>
#include <list>
#include <memory>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <assert.h>
#include <string_view>
#include "cutil/gc_ptr.hpp"


namespace entt2 {

/*! @brief Alias declaration for type identifiers. */
using id_type = std::uint32_t;



















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










/*! @brief Primary template isn't defined on purpose. */
template<std::size_t, typename>
struct type_list_element;
/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam Index Index of the type to return.
 * @tparam Type First type provided by the type list.
 * @tparam Other Other types provided by the type list.
 */
template<std::size_t Index, typename Type, typename... Other>
struct type_list_element<Index, type_list<Type, Other...>>
    : type_list_element<Index - 1u, type_list<Other...>>
{};
/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam Type First type provided by the type list.
 * @tparam Other Other types provided by the type list.
 */
template<typename Type, typename... Other>
struct type_list_element<0u, type_list<Type, Other...>> {
    /*! @brief Searched type. */
    using type = Type;
};
/**
 * @brief Helper type.
 * @tparam Index Index of the type to return.
 * @tparam List Type list to search into.
 */
template<std::size_t Index, typename List>
using type_list_element_t = typename type_list_element<Index, List>::type;






template<std::size_t N>
struct choice_t : choice_t<N - 1>
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
        -> decltype(std::declval<Type>() == std::declval<Type>()) {
        return true;
    }

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
struct is_equality_comparable : std::bool_constant<internal::is_equality_comparable<Type>(choice<2>)> {};

template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;












template<typename Type, typename = void>
struct is_complete : std::false_type {};

template<typename Type>
struct is_complete<Type, std::void_t<decltype(sizeof(Type))>> : std::true_type {};

template<typename Type>
inline constexpr auto is_complete_v = is_complete<Type>::value;


template<typename Type, typename = void>
struct is_empty : std::is_empty<Type> {};

template<typename Type>
inline constexpr auto is_empty_v = is_empty<Type>::value;










/*! @brief Used to wrap a function or a member of a specified type. */
template<auto>
struct connect_arg_t {};

/*! @brief Constant of type connect_arg_t used to disambiguate calls. */
template<auto Func>
inline constexpr connect_arg_t<Func> connect_arg{};







/**
 * @brief Alias for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
struct exclude_t : type_list<Type...> {};

/**
 * @brief Variable template for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
inline constexpr exclude_t<Type...> exclude{};














/**
 * @brief Entity traits.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is an accepted entity type.
 */
template<typename>
struct entt_traits;


/**
 * @brief Entity traits for a 32 bits entity identifier.
 *
 * A 32 bits entity identifier guarantees:
 *
 * * 20 bits for the entity number (suitable for almost all the games).
 * * 12 bit for the version (resets in [0-4095]).
 */
template<>
struct entt_traits<std::uint32_t> {
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
 * @brief Entity traits for a 64 bits entity identifier.
 *
 * A 64 bits entity identifier guarantees:
 *
 * * 32 bits for the entity number (an indecently large number).
 * * 32 bit for the version (an indecently large number).
 */
template<>
struct entt_traits<std::uint64_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint64_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint32_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xFFFFFFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 32u;
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
    return static_cast<typename entt_traits<Entity>::entity_type>(entity);
}


/*! @brief Null object for all entity identifiers.  */
struct null_t
{
    /** @brief Converts the null object to identifiers of any type.*/
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept { return Entity{ entt_traits<Entity>::entity_mask }; }

    [[nodiscard]] constexpr bool operator==(const null_t&) const noexcept { return true; }
    [[nodiscard]] constexpr bool operator!=(const null_t&) const noexcept { return false; }
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity& entity) const noexcept { return !(*this == entity); }
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity& entity) const noexcept {
        return (to_integral(entity) & entt_traits<Entity>::entity_mask) == to_integral(static_cast<Entity>(*this));
    }
};

namespace internal
{
    // for type_seq struct
    struct type_seq final
    {
        [[nodiscard]] static
            id_type next() noexcept
        {
            static std::atomic<id_type> value{};
            return value++;
        }
    };
}


/**
 * @brief Type sequential identifier.
 * @tparam Type Type for which to generate a sequential identifier.
 */
template<typename Type, typename = void>
struct type_seq final
{
    /**
     * @brief Returns the sequential identifier of a given type.
     * @return The sequential identifier of a given type.
     */
    [[nodiscard]] static id_type value() noexcept {
        static const id_type value = internal::type_seq::next();
        return value;
    }
    /*! @copydoc value */
    [[nodiscard]] constexpr operator id_type() const noexcept { return value(); }
};

/*! @brief Implementation specific information about a type. */
class type_info final
{
    template<typename>
    friend type_info type_id() noexcept;

    type_info(id_type seq_v) noexcept
        : seq_value{ seq_v }
    {}
public:
    type_info() noexcept : type_info({}) {}
    type_info(const type_info&) noexcept = default;
    type_info(type_info&&) noexcept = default;
    type_info& operator=(const type_info&) noexcept = default;
    type_info& operator=(type_info&&) noexcept = default;
    // @brief Checks if this object is properly initialized.
    [[nodiscard]] id_type seq() const noexcept { return seq_value; }
    [[nodiscard]] bool operator==(const type_info& other) const noexcept { return seq_value == other.seq_value; }
    [[nodiscard]] inline bool operator!=(const type_info& other) noexcept { return seq_value != other.seq_value; }
private:
    id_type seq_value;
};

/**
 * @brief Returns the type info object for a given type.
 * @tparam Type Type for which to generate a type info object.
 * @return The type info object for the given type.
 */
template<typename Type>
[[nodiscard]]
type_info type_id() noexcept {
    return type_info{
        type_seq<std::remove_cv_t<std::remove_reference_t<Type>>>::value()
    };
}







/// @brief Compile-time constant for null entities.
inline constexpr null_t null{};



/**
 * @brief Shortcut for calling `poly_base<Type>::invoke`.
 * @tparam Member Index of the function to invoke.
 * @tparam Poly A fully defined poly object.
 * @tparam Args Types of arguments to pass to the function.
 * @param self A reference to the poly object that made the call.
 * @param args The arguments to pass to the function.
 * @return The return value of the invoked function, if any.
 */
template<auto Member, typename Poly, typename... Args>
decltype(auto) poly_call(Poly &&self, Args &&... args) {
    return std::forward<Poly>(self).template invoke<Member>(self, std::forward<Args>(args)...);
}





/**
 * @brief Basic poly storage implementation.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct Storage: type_list<type_info() const noexcept>
{
    /**
     * @brief Concept definition.
     * @tparam Base Opaque base class from which to inherit.
     */
    template<typename Base>
    struct type: Base
    {
        /**
         * @brief Returns a type info for the contained objects.
         * @return The type info for the contained objects.
         */
        type_info value_type() const noexcept
            {return poly_call<0>(*this);}
    };

    /**
     * @brief Concept implementation.
     * @tparam Type Type for which to generate an implementation.
     */
    template<typename Type>
    using impl = value_list<&type_id<typename Type::value_type>>;
};




template<typename Entity>
class basic_sparse_set {

    static constexpr auto page_size = 4096;

    static_assert(page_size && ((page_size& (page_size - 1)) == 0), "ENTT_PAGE_SIZE must be a power of two");

    using traits_type = entt_traits<Entity>;
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

        sparse_set_iterator(const packed_type& ref, const index_type idx) noexcept
            : packed{ &ref }, index{ idx }
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::random_access_iterator_tag;

        sparse_set_iterator() noexcept = default;

        sparse_set_iterator& operator++() noexcept { return --index, * this; }
        sparse_set_iterator& operator--() noexcept { return ++index, * this; }
        sparse_set_iterator& operator+=(const difference_type value) noexcept { index -= value; return *this; }
        sparse_set_iterator& operator-=(const difference_type value) noexcept { return (*this += -value); }
        [[nodiscard]] reference operator[](const difference_type value) const { const auto pos = size_type(index - value - 1u); return (*packed)[pos]; }
        [[nodiscard]] bool operator==(const sparse_set_iterator& other) const noexcept { return other.index == index; }
        [[nodiscard]] bool operator!=(const sparse_set_iterator& other) const noexcept { return !(*this == other); }
        [[nodiscard]] bool operator< (const sparse_set_iterator& other) const noexcept { return index > other.index; }
        [[nodiscard]] bool operator> (const sparse_set_iterator& other) const noexcept { return index < other.index; }
        [[nodiscard]] bool operator<=(const sparse_set_iterator& other) const noexcept { return !(*this > other); }
        [[nodiscard]] bool operator>=(const sparse_set_iterator& other) const noexcept { return !(*this < other); }
        [[nodiscard]] pointer operator->() const { const auto pos = size_type(index - 1u); return &(*packed)[pos]; }
        [[nodiscard]] reference operator*() const { return *operator->(); }
    private:
        const packed_type* packed;
        index_type index;
    };

    [[nodiscard]] auto page(const Entity entt) const noexcept {
        return size_type{ (to_integral(entt) & traits_type::entity_mask) / page_size };
    }

    [[nodiscard]] auto offset(const Entity entt) const noexcept {
        return size_type{ to_integral(entt) & (page_size - 1) };
    }

    [[nodiscard]] page_type& assure(const std::size_t pos) {
        if (!(pos < sparse.size())) {
            sparse.resize(pos + 1);
        }

        if (!sparse[pos]) {
            sparse[pos].reset(new entity_type[page_size]);
            // null is safe in all cases for our purposes
            for (auto* first = sparse[pos].get(), *last = first + page_size; first != last; ++first) {
                *first = null;
            }
        }

        return sparse[pos];
    }

protected:
    /*! @brief Swaps two entities in the internal packed array. */
    virtual void swap_at(const std::size_t, const std::size_t) {}

    /*! @brief Attempts to remove an entity from the internal packed array. */
    virtual void swap_and_pop(const std::size_t, void*) {}

public:

    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = const entity_type*;

    /*! @brief Default constructor. */
    basic_sparse_set() = default;

    /*! @brief Default move constructor. */
    basic_sparse_set(basic_sparse_set&&) = default;

    /*! @brief Default destructor. */
    virtual ~basic_sparse_set() = default;

    /*! @brief Default move assignment operator. @return This sparse set. */
    basic_sparse_set& operator=(basic_sparse_set&&) = default;

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
    [[nodiscard]] size_type capacity() const noexcept {
        return packed.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        // conservative approach
        if (packed.empty()) {
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
    [[nodiscard]] size_type extent() const noexcept {
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
    [[nodiscard]] size_type size() const noexcept {
        return packed.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
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
    [[nodiscard]] const entity_type* data() const noexcept {
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
    [[nodiscard]] iterator begin() const noexcept {
        const typename traits_type::difference_type pos = packed.size();
        return iterator{ packed, pos };
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
    [[nodiscard]] iterator end() const noexcept {
        return iterator{ packed, {} };
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
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
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
    [[nodiscard]] reverse_iterator rend() const noexcept {
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
        assert(contains(entt));
        return size_type{ to_integral(sparse[page(entt)][offset(entt)]) };
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
        assert(pos < packed.size());
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
        assert(!contains(entt));
        assure(page(entt))[offset(entt)] = entity_type{ static_cast<typename traits_type::entity_type>(packed.size()) };
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

        for (; first != last; ++first) {
            assert(!contains(*first));
            assure(page(*first))[offset(*first)] = entity_type{ next++ };
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
    void remove(const entity_type entt, void* ud = nullptr) {
        assert(contains(entt));
        auto& ref = sparse[page(entt)][offset(entt)];

        // last chance to use the entity for derived classes and mixins, if any
        swap_and_pop(size_type{ to_integral(ref) }, ud);

        const auto other = packed.back();
        sparse[page(other)][offset(other)] = ref;
        // if it looks weird, imagine what the subtle bugs it prevents are
        assert((packed.back() = entt, true));
        packed[size_type{ to_integral(ref) }] = other;
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
    void remove(It first, It last, void* ud = nullptr) {
        for (; first != last; ++first) {
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
    void respect(const basic_sparse_set& other) {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = packed.size() - 1;

        while (pos && from != to) {
            if (contains(*from)) {
                if (*from != packed[pos]) {
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
    void clear(void* ud = nullptr) noexcept {
        remove(begin(), end(), ud);
    }

private:
    std::vector<page_type> sparse;
    std::vector<entity_type> packed;

};





template<typename, typename, typename>
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
template<typename Entity, typename Type, typename = void>
class basic_storage : public basic_sparse_set<Entity> {

    static_assert(std::is_move_constructible_v<Type>&& std::is_move_assignable_v<Type>, "The managed type must be at least move constructible and assignable");

    using underlying_type = basic_sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

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
    void swap_and_pop(const std::size_t pos, void*) {
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
    using reverse_iterator = Type*;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = const Type*;

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
    [[nodiscard]] const value_type* raw() const noexcept {
        return instances.data();
    }

    /*! @copydoc raw */
    [[nodiscard]] value_type* raw() noexcept {
        return const_cast<value_type*>(std::as_const(*this).raw());
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the internal array.
     * If the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal array.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator{ instances, pos };
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() noexcept {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator{ instances, pos };
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
    [[nodiscard]] const_iterator cend() const noexcept {
        return const_iterator{ instances, {} };
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() noexcept {
        return iterator{ instances, {} };
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
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return instances.data();
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() noexcept {
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
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return crbegin() + instances.size();
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() noexcept {
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
    [[nodiscard]] const value_type& get(const entity_type entt) const {
        return instances[underlying_type::index(entt)];
    }

    /*! @copydoc get */
    [[nodiscard]] value_type& get(const entity_type entt) {
        return const_cast<value_type&>(std::as_const(*this).get(entt));
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
    value_type& emplace(const entity_type entt, Args &&... args) {
        if constexpr (std::is_aggregate_v<value_type>) {
            instances.push_back(Type{ std::forward<Args>(args)... });
        }
        else {
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
        auto&& instance = instances[this->index(entity)];
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
    void insert(It first, It last, const value_type& value = {}) {
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















template<typename>
class basic_registry;











/**
 * @brief Gets the element assigned to an entity from a storage, if any.
 * @tparam Type Storage type.
 * @param container A valid instance of a storage class.
 * @param entity A valid entity identifier.
 * @return A possibly empty tuple containing the requested element.
 */
template<typename Type>
[[nodiscard]] auto get_as_tuple([[maybe_unused]] Type &container, [[maybe_unused]] const typename Type::entity_type entity) {
    static_assert(
        std::is_same_v<std::remove_const_t<Type>,basic_storage<typename Type::entity_type,typename Type::value_type>>,"Invalid storage"
    );

    if constexpr(std::is_void_v<decltype(container.get({}))>) {
        return std::make_tuple();
    } else {
        return std::forward_as_tuple(container.get(entity));
    }
}









/**
 * @brief View.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class basic_view;





template<typename...>
class basic_view;


/**
 * @brief Multi component view.
 *
 * Multi component views iterate over those entities that have at least all the
 * given components in their bags. During initialization, a multi component view
 * looks at the number of entities available for each component and uses the
 * smallest set in order to get a performance boost when iterate.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the view.
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Exclude, typename... Component>
class basic_view<Entity, exclude_t<Exclude...>, Component...> final {
    template<typename Comp>
    using storage_type = constness_as_t<basic_storage<Entity, std::remove_const_t<Comp>>, Comp>;

    using unchecked_type = std::array<const basic_sparse_set<Entity>*, (sizeof...(Component) - 1)>;

    template<typename It>
    class view_iterator final {

        [[nodiscard]] bool valid() const {
            const auto entt = *it;
            return std::all_of(
                unchecked.cbegin(), unchecked.cend(),
                [entt](const basic_sparse_set<Entity>* curr) { return curr->contains(entt); }
            ) && !(std::get<const storage_type<Exclude> *>(filter)->contains(entt) || ...);
        }

    public:
        view_iterator(It from, It to, It curr, unchecked_type other, const std::tuple<const storage_type<Exclude> *...>& ignore) noexcept
            : first{ from },
            last{ to },
            it{ curr },
            unchecked{ other },
            filter{ ignore }
        {
            if (it != last && !valid())
                ++(*this);
        }

        using difference_type = typename std::iterator_traits<It>::difference_type;
        using value_type = typename std::iterator_traits<It>::value_type;
        using pointer = typename std::iterator_traits<It>::pointer;
        using reference = typename std::iterator_traits<It>::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        view_iterator() noexcept
            : view_iterator{ {}, {}, {}, {}, {} }
        {}

        view_iterator& operator++() noexcept { while (++it != last && !valid()); return *this; }
        view_iterator& operator--() noexcept { while (--it != first && !valid()); return *this; }

        [[nodiscard]] bool operator==(const view_iterator& other) const noexcept { return other.it == it; }
        [[nodiscard]] bool operator!=(const view_iterator& other) const noexcept { return !(*this == other); }
        [[nodiscard]] pointer operator->() const { return &*it; }
        [[nodiscard]] reference operator*() const { return *operator->(); }

    private:
        It first;
        It last;
        It it;
        unchecked_type unchecked;
        std::tuple<const storage_type<Exclude> *...> filter;
    };

    class iterable_view final {};

    [[nodiscard]]
    const basic_sparse_set<Entity>* candidate() const noexcept {
        return (std::min)(
            { static_cast<const basic_sparse_set<entity_type> *>(std::get<storage_type<Component> *>(pools))... },
            [](const auto* lhs, const auto* rhs) {return lhs->size() < rhs->size(); }
        );
    }

    [[nodiscard]]
    unchecked_type unchecked(const basic_sparse_set<Entity>* cpool) const {
        std::size_t pos{};
        unchecked_type other{};
        (static_cast<void>(std::get<storage_type<Component> *>(pools) == cpool ? void() : void(other[pos++] = std::get<storage_type<Component> *>(pools))), ...);
        return other;
    }


public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Bidirectional iterator type. */
    using iterator = view_iterator<typename basic_sparse_set<entity_type>::iterator>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = view_iterator<typename basic_sparse_set<entity_type>::reverse_iterator>;


    basic_view() noexcept
        : view{}
    {}
    basic_view(storage_type<Component> &... component, const storage_type<Exclude> &... epool) noexcept
        : pools{ &component... },
        filter{ &epool... },
        view{ candidate() }
    {}

    /**
     * @brief Forces the type to use to drive iterations.
     * @tparam Comp Type of component to use to drive the iteration.
     */
    template<typename Comp>
    void use() const noexcept
    {
        view = std::get<storage_type<Comp> *>(pools);
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const noexcept
    {
        return view->size();
    }


    [[nodiscard]] iterator begin() const {
        return iterator{ view->begin(), view->end(), view->begin(), unchecked(view), filter };
    }
    [[nodiscard]] iterator end() const {
        return iterator{ view->begin(), view->end(), view->end(), unchecked(view), filter };
    }


    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }
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
        const auto it = iterator{ view->begin(), view->end(), view->find(entt), unchecked(view), filter };
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view is properly initialized.
     * @return True if the view is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return view != nullptr;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return (std::get<storage_type<Component> *>(pools)->contains(entt) && ...) && !(std::get<const storage_type<Exclude> *>(filter)->contains(entt) || ...);
    }

    /**
     * @brief Returns the components assigned to the given entity.
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
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        assert(contains(entt));

        if constexpr (sizeof...(Comp) == 0) {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Component> *>(pools), entt)...);
        }
        else if constexpr (sizeof...(Comp) == 1) {
            return (std::get<storage_type<Comp> *>(pools)->get(entt), ...);
        }
        else {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Comp> *>(pools), entt)...);
        }
    }


    /**
     * @brief Combines two views in a _more specific_ one (friend function).
     * @tparam Id A valid entity type (see entt_traits for more details).
     * @tparam ELhs Filter list of the first view.
     * @tparam CLhs Component list of the first view.
     * @tparam ERhs Filter list of the second view.
     * @tparam CRhs Component list of the second view.
     * @return A more specific view.
     */
    template<typename Id, typename... ELhs, typename... CLhs, typename... ERhs, typename... CRhs>
    friend auto operator|(const basic_view<Id, exclude_t<ELhs...>, CLhs...>&, const basic_view<Id, exclude_t<ERhs...>, CRhs...>&);

private:
    const std::tuple<storage_type<Component> *...> pools;
    const std::tuple<const storage_type<Exclude> *...> filter;
    mutable const basic_sparse_set<entity_type>* view;
};























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
class basic_view<Entity, exclude_t<>, Component> final {
    using storage_type = constness_as_t<basic_storage<Entity, std::remove_const_t<Component>>, Component>;


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
    basic_view() noexcept
        : pools{},
        filter{}
    {}

    /**
     * @brief Constructs a single-type view from a storage class.
     * @param ref The storage for the type to iterate.
     */
    basic_view(storage_type& ref) noexcept
        : pools{ &ref },
        filter{}
    {}

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    [[nodiscard]] size_type size() const noexcept {
        return std::get<0>(pools)->size();
    }

    /**
     * @brief Checks whether a view is empty.
     * @return True if the view is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return std::get<0>(pools)->empty();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of components.
     */
    [[nodiscard]] raw_type* raw() const noexcept {
        return std::get<0>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type* data() const noexcept {
        return std::get<0>(pools)->data();
    }


    [[nodiscard]] iterator begin() const noexcept
    {
        return std::get<0>(pools)->basic_sparse_set<entity_type>::begin();
    }
    [[nodiscard]] iterator end() const noexcept
    {
        return std::get<0>(pools)->basic_sparse_set<entity_type>::end();
    }

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
        const auto it = std::get<0>(pools)->find(entt);
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
    [[nodiscard]] explicit operator bool() const noexcept {
        return std::get<0>(pools) != nullptr;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return std::get<0>(pools)->contains(entt);
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
    template<typename... Comp>
    [[nodiscard]]
    decltype(auto) get(const entity_type entt) const {
        assert(contains(entt));

        if constexpr (sizeof...(Comp) == 0) {
            return get_as_tuple(*std::get<0>(pools), entt);
        }
        else {
            static_assert(std::is_same_v<Comp..., Component>, "Invalid component type");
            return std::get<0>(pools)->get(entt);
        }
    }
    /**
     * @brief Combines two views in a _more specific_ one (friend function).
     * @tparam Id A valid entity type (see entt_traits for more details).
     * @tparam ELhs Filter list of the first view.
     * @tparam CLhs Component list of the first view.
     * @tparam ERhs Filter list of the second view.
     * @tparam CRhs Component list of the second view.
     * @return A more specific view.
     */
    template<typename Id, typename... ELhs, typename... CLhs, typename... ERhs, typename... CRhs>
    friend auto operator|(const basic_view<Id, exclude_t<ELhs...>, CLhs...>&, const basic_view<Id, exclude_t<ERhs...>, CRhs...>&);

private:
    const std::tuple<storage_type*> pools;
    const std::tuple<> filter;
};




















/**
 * @brief Deduction guide.
 * @tparam Storage Type of storage classes used to create the view.
 * @param storage The storage for the types to iterate.
 */
template<typename... Storage>
basic_view(Storage &... storage)
-> basic_view<std::common_type_t<typename Storage::entity_type...>, exclude_t<>, constness_as_t<typename Storage::value_type, Storage>...>;


/**
 * @brief Combines two views in a _more specific_ one.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam ELhs Filter list of the first view.
 * @tparam CLhs Component list of the first view.
 * @tparam ERhs Filter list of the second view.
 * @tparam CRhs Component list of the second view.
 * @param lhs A valid reference to the first view.
 * @param rhs A valid reference to the second view.
 * @return A more specific view.
 */
template<typename Entity, typename... ELhs, typename... CLhs, typename... ERhs, typename... CRhs>
[[nodiscard]] auto operator|(const basic_view<Entity, exclude_t<ELhs...>, CLhs...>& lhs, const basic_view<Entity, exclude_t<ERhs...>, CRhs...>& rhs) {
    using view_type = basic_view<Entity, exclude_t<ELhs..., ERhs...>, CLhs..., CRhs...>;
    return std::apply([](auto *... storage) { return view_type{ *storage... }; }, std::tuple_cat(lhs.pools, rhs.pools, lhs.filter, rhs.filter));
}


















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
template<typename Entity>
class basic_registry
{
    using traits_type = entt_traits<Entity>;

    template<typename Component>
    using storage_type = constness_as_t<basic_storage<Entity, std::remove_const_t<Component>>, Component>;

    struct pool_data
    {
        std::unique_ptr<basic_sparse_set<Entity>> pool{};
    };

public:

    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Underlying version type. */
    using version_type = typename entt_traits<Entity>::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

private:

    template<typename Component>
    [[nodiscard]]
    auto* assure() const
    {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>, "Non-decayed types not allowed");
        const auto index = type_seq<Component>::value();

        if (!(index < pools.size()))
            pools.resize(size_type(index) + 1u);

        auto&& pdata = pools[index];
        if (!pdata.pool)
            pdata.pool = std::make_unique<storage_type<Component>>();

        return static_cast<storage_type<Component> *>(pools[index].pool.get());
    }

    template<typename Component>
    [[nodiscard]]
    const storage_type<Component>* pool_if_exists() const
    {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>, "Non-decayed types not allowed");
        const auto index = type_seq<Component>::value();
        return (!(index < pools.size()) || !pools[index].pool) ? nullptr : static_cast<const storage_type<Component> *>(pools[index].pool.get());
    }

    entity_type generate_identifier()
    {
        // traits_type::entity_mask is reserved to allow for null identifiers
        assert(static_cast<typename traits_type::entity_type>(entities.size()) < traits_type::entity_mask);
        return entities.emplace_back(entity_type{ static_cast<typename traits_type::entity_type>(entities.size()) });
    }

    entity_type recycle_identifier()
    {
        const auto curr = to_integral(available);
        const auto version = to_integral(entities[curr]) & (traits_type::version_mask << traits_type::entity_shift);
        available = entity_type{ to_integral(entities[curr]) & traits_type::entity_mask };
        return entities[curr] = entity_type{ curr | version };
    }

    void release_entity(const Entity entity, const typename traits_type::version_type version) {
        const auto entt = to_integral(entity) & traits_type::entity_mask;
        entities[entt] = entity_type{ to_integral(available) | (typename traits_type::entity_type{version} << traits_type::entity_shift) };
        available = entity_type{ entt };
    }

public:

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    [[nodiscard]] static entity_type entity(const entity_type entity) noexcept {
        return entity_type{ to_integral(entity) & traits_type::entity_mask };
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    [[nodiscard]] static version_type version(const entity_type entity) noexcept {
        return version_type(to_integral(entity) >> traits_type::entity_shift);
    }

    /*! @brief Default constructor. */
    basic_registry() = default;

    /*! @brief Default move constructor. */
    basic_registry(const basic_registry&) = delete;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry&&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry& operator=(basic_registry&&) = default;


    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    [[nodiscard]] size_type size() const {
        const auto* cpool = pool_if_exists<Component>();
        return cpool ? cpool->size() : size_type{};
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    void reserve(size_type s) const {
        assure<Component>()->reserve(s);
    }

    /**
     * @brief Returns the number of entities created so far.
     * @return Number of entities created so far.
     */
    [[nodiscard]] size_type size() const noexcept {
        return entities.size();
    }

    /**
     * @brief Returns the capacity of the pool for the given component.
     * @tparam Component Type of component in which one is interested.
     * @return Capacity of the pool of the given component.
     */
    template<typename Component>
    [[nodiscard]] size_type capacity() const {
        const auto* cpool = pool_if_exists<Component>();
        return cpool ? cpool->capacity() : size_type{};
    }

    /**
     * @brief Returns the number of entities that a registry has currently
     * allocated space for.
     * @return Capacity of the registry.
     */
    [[nodiscard]] size_type capacity() const noexcept {
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
    {
        return null == available ? generate_identifier() : recycle_identifier();
    }

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
     * @tparam Component Type of component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace(const entity_type entity, Args &&... args)
    {
        assert(valid(entity));
        return assure<Component>()->emplace(entity, std::forward<Args>(args)...);
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
        destroy(entity, static_cast<typename traits_type::version_type>(version(entity) + 1u));
    }

    /**
     * @brief Destroys an entity.
     *
     * If the entity isn't already destroyed, the suggested version is used
     * instead of the implicitly generated one.
     *
     * @sa remove_all
     *
     * @param entity A valid entity identifier.
     * @param version A desired version upon destruction.
     */
    void destroy(const entity_type entity, const version_type version) {
        remove_all(entity);
        release_entity(entity, version);
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
     * @brief Removes the given components from an entity.
     *
     * @tparam Component Types of components to remove.
     * @tparam entity A valid entity identifier
     */
    template<typename... Component>
    void remove(const entity_type entity) {
        assert(valid(entity));
        static_assert(sizeof...(Component) > 0, "Provide one or more component types");
        (assure<Component>()->remove(entity), ...);
    }

    /**
     * @brief Removes the given components from all the entities in a range.
     *
     * @sa remove
     *
     * @tparam Component Types of components to remove.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename... Component, typename It>
    void remove(It first, It last) {
        assert(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        static_assert(sizeof...(Component) > 0, "Provide one or more component types");
        (assure<Component>()->remove(first, last), ...);
    }

    /**
     * @brief Removes all the components from an entity and makes it orphaned.
     *
     * @warning
     * In case there are listeners that observe the destruction of components
     * and assign other components to the entity in their bodies, the result of
     * invoking this function may not be as expected. In the worst case, it
     * could lead to undefined behavior.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @param entity A valid entity identifier.
     */
    void remove_all(const entity_type entity) {
        assert(valid(entity));

        for (auto pos = pools.size(); pos; --pos)
            if (auto& pdata = pools[pos - 1]; pdata.pool && pdata.pool->contains(entity))
                pdata.pool->remove(entity);
    }

    /**
     * @brief Returns references to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        assert(valid(entity));

        if constexpr (sizeof...(Component) == 1) {
            const auto* cpool = pool_if_exists<std::remove_const_t<Component>...>();
            assert(cpool);
            return cpool->get(entity);
        }
        else {
            return std::forward_as_tuple(get<Component>(entity)...);
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) {
        assert(valid(entity));

        if constexpr (sizeof...(Component) == 1) {
            return (const_cast<Component&>(assure<std::remove_const_t<Component>>()->get(entity)), ...);
        }
        else {
            return std::forward_as_tuple(get<Component>(entity)...);
        }
    }





    /**
     * @brief Returns a view for the given components.
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
     * @tparam Component Type of components used to construct the view.
     * @tparam Exclude Types of components used to filter the view.
     * @return A newly created view.
     */
    template<typename... Component, typename... Exclude>
    [[nodiscard]] basic_view<Entity, exclude_t<Exclude...>, Component...>
        view(exclude_t<Exclude...> = {}) const {
        static_assert(sizeof...(Component) > 0, "Exclusion-only views are not supported");
        static_assert((std::is_const_v<Component> && ...), "Invalid non-const type");
        return { *assure<std::remove_const_t<Component>>()..., *assure<Exclude>()... };
    }

    /*! @copydoc view */
    template<typename... Component, typename... Exclude>
    [[nodiscard]] basic_view<Entity, exclude_t<Exclude...>, Component...>
        view(exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Component) > 0, "Exclusion-only views are not supported");
        return { *assure<std::remove_const_t<Component>>()..., *assure<Exclude>()... };
    }



private:
    mutable std::vector<pool_data> pools{};
    std::vector<entity_type> entities{};
    entity_type available{ null };
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
using registry = basic_registry<id_type>;

/*! @brief Default entity identifier. */
using entity = id_type;
}

#endif

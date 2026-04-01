#if !defined(HashHelper_HPP)
#define HashHelper_HPP

#include "span.hpp"

namespace HashHelper
{
    /// @brief Generates a FNV1A32 hash.
    /// @param data Text(data) to hash.
    /// @param l lenght of text
    /// @return Hash of input string.
    uint32_t FNV1A32(const void *data,size_t l);

    /// @brief Generates a FNV1A32 hash.
    /// @param val Value to hash.
    /// @return Hash of input.
    uint32_t FNV1A32(int32_t val);

    /// @brief Combines a FNV1A32 hash with a value.
    /// @param hash Input Hash.
    /// @param value Value to add to the hash.
    /// @return A combined FNV1A64 hash
    uint32_t CombineFNV1A32(uint32_t hash, uint32_t value);

    template<typename T>
    inline uint32_t FNV1A32(const_span<T> data)
    {
        return FNV1A32(data.data(),data.size_bytes());
    }

    inline uint32_t tgc_hash(uintptr_t v) {
        return (uint32_t) ((13*v) ^ (v >> 16));
    }
}


/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
[[nodiscard]] uint32_t operator""_hash32(const char *str, std::size_t count) noexcept;
/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
[[nodiscard]] uint32_t operator""_hash32(const wchar_t *str, std::size_t count) noexcept;



#endif // HashHelper_HPP

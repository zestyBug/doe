#include <stdint.h>
#include <stdexcept>
#include "span.hpp"

#if !defined(HashHelper_HPP)
#define HashHelper_HPP

class HashHelper final
{
private:
    // http://www.isthe.com/chongo/src/fnv/hash_64a.c

    static constexpr uint64_t FNV1A64OffsetBasis = 0xB00B2FEED969BEDBULL;
    static constexpr uint64_t FNV1A64Prime =       0x100000001b3ULL;
    static constexpr uint32_t FNV1A32OffsetBasis = 0x55555555;
    static constexpr uint32_t FNV1A32Prime =       0xC1CBA8;

public:


    /// @brief Generates a FNV1A64 hash.
    /// @param text Text(data) to hash.
    /// @param l lenght of text
    /// @return Hash of input string.
    static uint64_t FNV1A64(void *text,size_t l)
    {
        uint64_t result = FNV1A64OffsetBasis;
        for (size_t i = 0; i < l; i++)
        {
            result = FNV1A64Prime * (result ^ ( ((uint8_t*)text)[i] & 255));
            result = FNV1A64Prime * (result ^ ( ((uint8_t*)text)[i] >>  8));
        }
        return result;
    }

    /// @brief Generates a FNV1A64 hash.
    /// @param val Value to hash.
    /// @return Hash of input.
    static uint64_t FNV1A64(int32_t val)
    {
        uint64_t result = FNV1A64OffsetBasis;
        {
            result = (((uint64_t)(val & 0x000000FF) >>  0) ^ result) * FNV1A64Prime;
            result = (((uint64_t)(val & 0x0000FF00) >>  8) ^ result) * FNV1A64Prime;
            result = (((uint64_t)(val & 0x00FF0000) >> 16) ^ result) * FNV1A64Prime;
            result = (((uint64_t)(val & 0xFF000000) >> 24) ^ result) * FNV1A64Prime;
        }

        return result;
    }

    /// @brief Combines a FNV1A64 hash with a value.
    /// @param hash Input Hash.
    /// @param value Value to add to the hash.
    /// @return A combined FNV1A64 hash
    static uint64_t CombineFNV1A64(uint64_t hash, uint64_t value)
    {
        hash ^= value;
        hash *= FNV1A64Prime;

        return hash;
    }











    /// @brief Generates a FNV1A32 hash.
    /// @param data Text(data) to hash.
    /// @param l lenght of text
    /// @return Hash of input string.
    static uint32_t FNV1A32(void *data,size_t l)
    {
        uint32_t result = FNV1A32OffsetBasis;
        for (size_t i = 0; i < l; i++)
        {
            result = FNV1A32Prime * (result ^ ( ((uint8_t*)data)[i] & 255));
            result = FNV1A32Prime * (result ^ ( ((uint8_t*)data)[i] >>  8));
        }
        return result;
    }

    /// @brief Generates a FNV1A32 hash.
    /// @param val Value to hash.
    /// @return Hash of input.
    static uint32_t FNV1A32(int32_t val)
    {
        uint32_t result = FNV1A32OffsetBasis;
        {
            result = (((val & 0x000000FF) >>  0) ^ result) * FNV1A32Prime;
            result = (((val & 0x0000FF00) >>  8) ^ result) * FNV1A32Prime;
            result = (((val & 0x00FF0000) >> 16) ^ result) * FNV1A32Prime;
            result = (((val & 0xFF000000) >> 24) ^ result) * FNV1A32Prime;
        }

        return result;
    }

    /// @brief Combines a FNV1A32 hash with a value.
    /// @param hash Input Hash.
    /// @param value Value to add to the hash.
    /// @return A combined FNV1A64 hash
    static uint32_t CombineFNV1A32(uint32_t hash, uint32_t value)
    {
        hash ^= value;
        hash *= FNV1A32Prime;
        return hash;
    }

    template<typename T>
    static uint32_t FNV1A32(span<T> data)
    {
        return FNV1A32(data.data(),data.size_bytes());
    }


};




#endif // HashHelper_HPP

#include <stdint.h>
#include <stdexcept>

#if !defined(HASH128_HPP)
#define HASH128_HPP

class hash128 final
{
private:
    static constexpr char k_LiteralToHex[] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        -1, -1, -1, -1, -1, -1, -1,
        10, 11, 12, 13, 14, 15,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        10, 11, 12, 13, 14, 15,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };
    static constexpr char k_HexToLiteral[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    
    static constexpr int k_GUIDStringLength = 32;

    // http://www.isthe.com/chongo/src/fnv/hash_64a.c

    static constexpr uint64_t kFNV1A64OffsetBasis = 0xCBF29CE484222325ULL;
    static constexpr uint64_t kFNV1A64Prime =       0x100000001b3ULL;
    static constexpr uint64_t kFNV1A32OffsetBasis = 0x55555555;
    static constexpr uint64_t kFNV1A32Prime =       0x811c9dc5;

public:

    struct uint128_t {
        uint32_t x=0;
        uint32_t y=0;
        uint32_t z=0;
        uint32_t w=0;
        inline uint32_t& operator[](size_t i){
            if(i<0 || i>3)
                throw std::out_of_range("out of range: uint128_t::operator[]");
            return ((uint32_t*)this)[i];
        }
    };
    static uint128_t StringToHash(uint8_t* guidString, size_t length, bool guidFormatted = true)
    {
        if (length != k_GUIDStringLength)
            return uint128_t{};

        // Convert every hex char into an int [0...16]
        int hex[k_GUIDStringLength];
        for (int i = 0; i < k_GUIDStringLength; i++)
        {
            uint8_t intValue = guidString[i];
            hex[i] = k_LiteralToHex[intValue];
        }

        uint128_t value;
        if (guidFormatted)
        {
            for (int i = 0; i < 4; i++)
            {
                uint32_t cur = 0;
                for (int j = 7; j >= 0; j--)
                {
                    int curHex = hex[i * 8 + j];
                    if (curHex == -1)
                        return uint128_t{};

                    cur |= (uint32_t)(curHex << (j * 4));
                }
                value[i] = cur;
            }
        }
        else
        {
            int currentHex = 0;
            for (int i = 0; i < 4; ++i)
            {
                uint32_t currentInt = 0;
                for (int j = 0; j < 4; ++j)
                {
                    currentInt |= (uint32_t)(hex[currentHex++] << j * 8 + 4);
                    currentInt |= (uint32_t)(hex[currentHex++] << j * 8);
                }
                value[i] = currentInt;
            }
        }
        return value;
    }


    /// @brief Generates a FNV1A64 hash.
    /// @param text Text(data) to hash.
    /// @param l lenght of text
    /// @return Hash of input string.
    static uint64_t FNV1A64(void *text,size_t l)
    {
        uint64_t result = kFNV1A64OffsetBasis;
        for (size_t i = 0; i < l; i++)
        {
            result = kFNV1A64Prime * (result ^ ( ((uint8_t*)text)[i] & 255));
            result = kFNV1A64Prime * (result ^ ( ((uint8_t*)text)[i] >>  8));
        }
        return result;
    }

    /// @brief Generates a FNV1A64 hash.
    /// @param val Value to hash.
    /// @return Hash of input.
    static uint64_t FNV1A64(int32_t val)
    {
        uint64_t result = kFNV1A64OffsetBasis;
        {
            result = (((uint64_t)(val & 0x000000FF) >>  0) ^ result) * kFNV1A64Prime;
            result = (((uint64_t)(val & 0x0000FF00) >>  8) ^ result) * kFNV1A64Prime;
            result = (((uint64_t)(val & 0x00FF0000) >> 16) ^ result) * kFNV1A64Prime;
            result = (((uint64_t)(val & 0xFF000000) >> 24) ^ result) * kFNV1A64Prime;
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
        hash *= kFNV1A64Prime;

        return hash;
    }

    








    
    /// @brief Generates a FNV1A32 hash.
    /// @param text Text(data) to hash.
    /// @param l lenght of text
    /// @return Hash of input string.
    static uint32_t FNV1A32(void *text,size_t l)
    {
        uint32_t result = kFNV1A32OffsetBasis;
        for (size_t i = 0; i < l; i++)
        {
            result = kFNV1A32Prime * (result ^ ( ((uint8_t*)text)[i] & 255));
            result = kFNV1A32Prime * (result ^ ( ((uint8_t*)text)[i] >>  8));
        }
        return result;
    }

    /// @brief Generates a FNV1A32 hash.
    /// @param val Value to hash.
    /// @return Hash of input.
    static uint32_t FNV1A32(int32_t val)
    {
        uint32_t result = kFNV1A32OffsetBasis;
        {
            result = (((val & 0x000000FF) >>  0) ^ result) * kFNV1A32Prime;
            result = (((val & 0x0000FF00) >>  8) ^ result) * kFNV1A32Prime;
            result = (((val & 0x00FF0000) >> 16) ^ result) * kFNV1A32Prime;
            result = (((val & 0xFF000000) >> 24) ^ result) * kFNV1A32Prime;
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
        hash *= kFNV1A32Prime;
        return hash;
    }


};




#endif // HASH128_HPP

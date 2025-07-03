#include "./HashHelper.hpp"

namespace HashHelper {
    namespace internal
    {
        const uint64_t FNV1A64OffsetBasis = 0xCBF29CE484222325ULL;
        const uint64_t FNV1A64Prime =       0x100000001b3ULL;
        const uint32_t FNV1A32OffsetBasis = 0x811C9DC5;
        const uint32_t FNV1A32Prime =       0x1000193;
    }

    uint64_t FNV1A64(void *text,size_t l)
    {
        uint64_t result = internal::FNV1A64OffsetBasis;
        for (size_t i = 0; i < l; i++)
        {
            result = internal::FNV1A64Prime * (result ^ ( ((uint8_t*)text)[i] & 255));
            result = internal::FNV1A64Prime * (result ^ ( ((uint8_t*)text)[i] >>  8));
        }
        return result;
    }

    /// @brief Generates a FNV1A64 hash.
    /// @param val Value to hash.
    /// @return Hash of input.
    uint64_t FNV1A64(int32_t val)
    {
        uint64_t result = internal::FNV1A64OffsetBasis;
        {
            result = (((uint64_t)(val & 0x000000FF) >>  0) ^ result) * internal::FNV1A64Prime;
            result = (((uint64_t)(val & 0x0000FF00) >>  8) ^ result) * internal::FNV1A64Prime;
            result = (((uint64_t)(val & 0x00FF0000) >> 16) ^ result) * internal::FNV1A64Prime;
            result = (((uint64_t)(val & 0xFF000000) >> 24) ^ result) * internal::FNV1A64Prime;
        }

        return result;
    }

    uint64_t CombineFNV1A64(uint64_t hash, uint64_t value)
    {
        hash ^= value;
        hash *= internal::FNV1A64Prime;

        return hash;
    }

    uint32_t FNV1A32(const void *data,size_t l)
    {
        uint32_t result = internal::FNV1A32OffsetBasis;
        for (size_t i = 0; i < l; i++)
        {
            result = internal::FNV1A32Prime * (result ^ ( ((uint8_t*)data)[i] & 255));
            result = internal::FNV1A32Prime * (result ^ ( ((uint8_t*)data)[i] >>  8));
        }
        return result;
    }

    uint32_t FNV1A32(int32_t val)
    {
        uint32_t result = internal::FNV1A32OffsetBasis;
        {
            result = (((val & 0x000000FF) >>  0) ^ result) * internal::FNV1A32Prime;
            result = (((val & 0x0000FF00) >>  8) ^ result) * internal::FNV1A32Prime;
            result = (((val & 0x00FF0000) >> 16) ^ result) * internal::FNV1A32Prime;
            result = (((val & 0xFF000000) >> 24) ^ result) * internal::FNV1A32Prime;
        }

        return result;
    }
}

[[nodiscard]] uint32_t operator""_hash32(const char *str, std::size_t count) noexcept {
    return HashHelper::FNV1A32((void*)str,count*sizeof(char));
}

[[nodiscard]] uint32_t operator""_hash32(const wchar_t *str, std::size_t count) noexcept {
    return HashHelper::FNV1A32((void*)str,count*sizeof(wchar_t));
}

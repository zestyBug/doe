#if !defined(SHAREDCOMPONENTVALUES_HPP)
#define SHAREDCOMPONENTVALUES_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct SharedComponentValues final {
        void** firstIndex = nullptr;
        uint32_t stride = sizeof(void*);
        void* operator[](uint32_t i) const
        {
            return *(void**)(((uint8_t*)firstIndex) + i * stride);
        }
        bool equalTo(SharedComponentValues otherValues, int sharedComponentCount) const
        {
            for (int i = 0; i < sharedComponentCount; ++i)
                if (otherValues[i] != (*this)[i])
                    return false;
            return true;
        }
        void copyTo(void** dest, int startIndex, int count) const
        {
            for (int i = 0; i < count; ++i)
                dest[i] = (*this)[startIndex + i];
        }
    };
}

#endif
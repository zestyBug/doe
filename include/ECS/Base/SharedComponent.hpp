#if !defined(SHAREDCOMPONENT_HPP)
#define SHAREDCOMPONENT_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct SharedComponentIndex final {
        SharedComponentIndex() = default;
        ~SharedComponentIndex() = default;
        SharedComponentIndex(uint32_t ini):value{ini}{};
        SharedComponentIndex(const SharedComponentIndex&) = default;
        SharedComponentIndex& operator = (const SharedComponentIndex&) = default;
        operator uint32_t (){ return this->value; }
        inline bool isNull() {return value == 0;}
        static constexpr uint32_t IndexMask = 0xFFFF;
        static constexpr uint32_t TypeIndexBitOffset = 16;
        //static constexpr uint32_t UnmanagedFlag = 1 << 31;
    private:
        uint32_t value = 0;
    };
    /// @brief A (pointer) container
    struct SharedComponentValues final {
        SharedComponentIndex* firstIndex = nullptr;
        int32_t stride = sizeof(SharedComponentIndex);
        SharedComponentIndex operator[](int32_t i) const
        {
            return *(SharedComponentIndex*)(((uint8_t*)firstIndex) + i * stride);
        }
        bool equalTo(SharedComponentValues otherValues, int sharedComponentCount) const
        {
            for (int i = 0; i < sharedComponentCount; ++i)
                if (otherValues[i] != (*this)[i])
                    return false;
            return true;
        }
        void copyTo(SharedComponentIndex* dest, int startIndex, int count) const
        {
            for (int i = 0; i < count; ++i)
                dest[i] = (*this)[startIndex + i];
        }
        inline void operator ++ (){
            firstIndex = (SharedComponentIndex*)((uint8_t*)firstIndex + stride);
        }
    };
}

#endif
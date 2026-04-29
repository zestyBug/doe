#if !defined(CONSTANTS_HPP)
#define CONSTANTS_HPP

#include "cutil/basics.hpp"

namespace ECS {
    /// @brief Set of constant values shared by multiple classes and the value can be changed to a valid value depending on situations.
    struct Constants {
        /// @details Considerations: must be power of 2
        static constexpr uint32_t MaximumQueryCount = 1024;
        /// @details Considerations: 1 bit flag for invalid entities.
        static constexpr uint32_t MaximumEntityCount = INT32_MAX;
        /// @brief maximum number of type an archetype can manage, any number higher than this may lead to overflow
        static constexpr uint32_t MaximumArchetypeComponentCount = 0x6f;
        static constexpr uint32_t MaximumArchetypeSharedComponentCount = 16;
        /// @brief Maximum number of unique component types supported by the TypeManager
        /// @details Considerations: 1_ SharedComponentIndex can only use up to 19 bit for the sharead component type
        /// 2_ TypeManager::ClearFlagsMask this number must be power of 2
        /// 3_ we must keep atleast 4 last bits (of an uint32_t) as flags.
        /// 4_ 15 bit for type index in an uint16_t + 1 bit for RW/RO
        /// 5_ the larger the number the larger EntityComponentStore byte size
        static constexpr uint16_t MaximumTypesCount = 1 << 12;
        static constexpr uint32_t MaximumChunkCount = 0x10000;
        static constexpr uint32_t MaxJobCount = 0xFFFFF;
    };
}

#endif // CONSTANTS_HPP

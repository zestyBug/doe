#if !defined(ARCHETYPECHUNKFILTER_HPP)
#define ARCHETYPECHUNKFILTER_HPP

#include "cutil/basics.hpp"
#include "SharedComponent.hpp"

namespace ECS
{
    class Archetype;

    struct ArchetypeChunkFilter
    {
        /// @brief MAGIC NUMBER
        static constexpr uint32_t MaximumSharedComponentCount = 16;
        Archetype* archetype = nullptr;
        SharedComponentIndex sharedComponentValues[MaximumSharedComponentCount];
        ArchetypeChunkFilter(Archetype* archetype, SharedComponentIndex* sharedComponentValues);
        ArchetypeChunkFilter(Archetype* archetype, SharedComponentValues sharedComponentValues);
        inline operator SharedComponentValues(){
            return SharedComponentValues{sharedComponentValues,sizeof(SharedComponentIndex)};
        }
    };
}

#endif
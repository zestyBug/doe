#include "ECS/Base/ArchetypeChunkFilter.hpp"
#include "ECS/Archetype.hpp"
using namespace ECS;


ArchetypeChunkFilter::ArchetypeChunkFilter(Archetype* srcArchetype, SharedComponentIndex* chunkSharedComponentValues)
{
    this->archetype = srcArchetype;
    for (int i = 0; i < srcArchetype->numSharedComponents(); i++)
        this->sharedComponentValues[i] = chunkSharedComponentValues[i];
}
ArchetypeChunkFilter::ArchetypeChunkFilter(Archetype* srcArchetype, SharedComponentValues chunkSharedComponentValues)
{
    this->archetype = srcArchetype;
    for (int i = 0; i < srcArchetype->numSharedComponents(); i++)
        this->sharedComponentValues[i] = chunkSharedComponentValues[i];
}
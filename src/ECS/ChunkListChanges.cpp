#include "ECS/Base/ChunkListChanges.hpp"
#include "ECS/Archetype.hpp"

using namespace ECS;

void ChunkListChanges::trackArchetype(Archetype* archetype)
{
    if (archetype->nextChangedArchetype == nullptr)
    {
        archetype->nextChangedArchetype = head;
        head = archetype;
    }
}
#if !defined(CHUNKLISTCHANGES_HPP)
#define CHUNKLISTCHANGES_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    class Archetype;
    struct ChunkListChanges
    {
        Archetype* head;
        ChunkListChanges():head{nullptr} {}
        void trackArchetype(Archetype* archetype);
    };
}
#endif // CHUNKLISTCHANGES_HPP
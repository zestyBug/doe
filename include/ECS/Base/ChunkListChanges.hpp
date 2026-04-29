#if !defined(CHUNKLISTCHANGES_HPP)
#define CHUNKLISTCHANGES_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    class Archetype;
    /// @brief When we add/remove a chunk to/from an archetype, we need to invalidate any cached chunk list related to that archetype.
    /// Changed archetypes can be tracked with a linked list of archetypes.
    struct ChunkListChanges
    {
        Archetype* head;
        ChunkListChanges():head{nullptr} {}
        void trackArchetype(Archetype* archetype);
    };
}
#endif // CHUNKLISTCHANGES_HPP
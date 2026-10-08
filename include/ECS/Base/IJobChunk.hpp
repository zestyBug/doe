#if !defined(IJOBCHUNK_HPP)
#define IJOBCHUNK_HPP

#include "cutil/basics.hpp"
#include "cutil/span.hpp"

namespace ECS
{
    struct Chunk;
    struct IJobChunk {
        void execute(const Chunk*,span<const uint32_t>){};
    };
}

#endif // ENTITY_HPP

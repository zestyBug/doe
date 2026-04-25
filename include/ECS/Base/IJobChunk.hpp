#if !defined(IJOBCHUNK_HPP)
#define IJOBCHUNK_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct Chunk;
    struct IJobChunk {
        virtual void execute(Chunk*) = 0;
    };
}

#endif // ENTITY_HPP

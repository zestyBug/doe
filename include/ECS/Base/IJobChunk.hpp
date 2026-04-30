#if !defined(IJOBCHUNK_HPP)
#define IJOBCHUNK_HPP

#include "cutil/basics.hpp"
#include "cutil/span.hpp"

namespace ECS
{
    struct Chunk;
    struct IJobChunk {
        void execute(const Chunk*,const_span<uint32_t>){};
    };
}

#endif // ENTITY_HPP

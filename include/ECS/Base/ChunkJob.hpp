#if !defined(CHUNKJOB_HPP)
#define CHUNKJOB_HPP

#include "TypeID.hpp"

namespace ECS {
    struct ChunkJob {
        /// @brief this function is/must be called by job threads
        /// @param pointers pointers to requested types(read+write)
        /// @param count count of enities
        virtual void execute(span<void*> pointers,uint32_t count) = 0;
        virtual const char* name() = 0;
    };
}

#endif // CHUNKJOB_HPP

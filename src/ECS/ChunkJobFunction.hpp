#if !defined(JOBCONTEXT_HPP)
#define JOBCONTEXT_HPP

#include "cutil/span.hpp"
#include "ECS/Archetype.hpp"

namespace ECS
{
    struct ChunkJobContext;
    struct ChunkJobFunction{
        /// @brief allocate and initialize context for the job
        /// @return nullptr on failure
        static align_ptr<uint8_t[]> createContext(
            span<ChunkJobContext> jobs,
            span<mark_ptr<Archetype>> archs,
            Version globalVersion);
        /// @brief a job function to be submitted to thread pool, alongside a context
        /// @param context context that was created by createJobContext
        static size_t function(void* context, size_t);
        /// @brief filter archtypes
        static void proccess(ChunkJobContext*,mark_ptr<Archetype>*,Version,Version,uint32_t);
        /// @brief filter and proccess chunks 
        static void callExecution(ChunkJobContext*,Archetype*,Version,Version);
    };
} // namespace ECS


#endif // JOBCONTEXT_HPP

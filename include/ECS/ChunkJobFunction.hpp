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
        static void* createContext(span<ChunkJobContext> jobs,span<ArchetypeHolder> archs, version_t globalVersion);
        /// @brief a job function to be submitted to thread pool, alongside a context
        /// @param context context that was created by createJobContext
        static size_t function(void* context, size_t) noexcept;
        /// @brief free and destroy context
        /// @param context 
        static void destroyContext(void* context);
        /// @brief filter archtypes
        static void proccess(ChunkJobContext*,ArchetypeHolder*,version_t,version_t,uint32_t);
        /// @brief filter and proccess chunks 
        static void callExecution(ChunkJobContext*,Archetype*,version_t,version_t);
    };
} // namespace ECS


#endif // JOBCONTEXT_HPP

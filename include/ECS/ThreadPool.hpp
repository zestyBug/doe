#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "Base/Job.hpp"

/**
 * Threads RW access and aligning (by cache line size): threads can write to a certain specified objects and oyu should be prepared for them.
 * ArchetypeChunkData address should be aligned since Version is updated by thread.
 * JobDataChunk::readIndex, JobDataChunk::readerLevel and JobDataChunk::activeThreads for sure.
 * JobDataChunk::bitmask can be modifed in any time, so any JobDataChunk must be aligned and JobDataChunk::bitmask must be stored somewhere safe
 * JobDataChunk::beginIndex this array address must be aligned and atomic.
 * JobDataChunk::works this array address and every sine entity of it (optionally) should be aligned.
 * Chunk address and evey signle component array must be aligned, therefore header must be aligned too.
 * AssetsManager itself is not thread safe but AssetsManager::works addresss must aligned, and every single entity must be distanced and aligned. since parallel IO operation.
 */

namespace ECS
{
    struct JobDataChunk;
    struct DOE;
    struct JobsUtility final {
        static void init();
        static void signalQuit();
        static void signalRender();
        /// @brief 
        /// @param context  
        /// @param func function returns zero if need to be executed again
        static JobHandle schedule(const JobParameter&);
        static JobHandle combineDependencies(const_span<JobHandle>);
    };
} // namespace ecs


#endif // THREADPOOL_HPP

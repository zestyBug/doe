#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include <atomic>
#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "JobChunk.hpp"
#include "ComponentDependencyManager.hpp"
#include "Base/Query.hpp"

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

extern "C" {
struct uv_timer_s;
struct uv_async_s;
struct uv__work;
struct uv_work_s;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_work_s uv_work_t;
}
namespace ECS
{
    struct DOE;
    class GraphicSystem;
    struct JobChunkWrapperBase;
    struct ComponentDependencyManager;

    // A scheduled job reference.
    struct Schedule {
        JobChunkWrapperBase *jw;
        EntityQueryImpl qb;
        bool parallel;
    };
    class JobsUtility final {
        friend class  GraphicSystem;
        friend struct JobChunkWrapperBase;
        friend struct ComponentDependencyManager;
    protected:
        struct JobEntry;
        struct JobData;
        std::atomic<uint32_t>  writeIndex = 0;
        std::atomic<uint32_t>  readIndex = 0;
        std::atomic<uint32_t>  readerLevel = 0;
        std::atomic<uint32_t>  activeThreads = 0;
        std::atomic<uint32_t>  capacity = 0;
        std::atomic<uint32_t>  bitmask = 0;
        /// @brief batch begin index to start with
        alignas(Constants::CacheLineSize) align_ptr<JobData[]> jobs = NULL;
        std::atomic<uint32_t>  *beginIndex = NULL;
        /// @brief sorted by dependency. use the handle to find the real index.
        JobHandle              *jobsArray = NULL;
        JobEntry               *buffer = NULL;
        align_ptr<uv_work_t[]> works;
        uv_timer_t             *fixedTimer;
        uv_async_t             *wakecall;
        /// @brief Temporary list of scheduled jobs. filled by systems and are executed at the end of system iteration.
        alignas(Constants::CacheLineSize) std::vector<Schedule> scheduleQueue;
        /// @brief Resolve jobs component dependancies
        ComponentDependencyManager dpm;

        /// @brief signal that new command buffers are availble
        void signalRender();
        void resizeJobPool(uint32_t);
        void prepareJobs();
        JobHandle schedule(const JobParameter&);
        JobHandle combineDependencies(const_span<JobHandle>);


        static void on_fixed_timer(uv_timer_t *);
        /// @brief calls either iterate_systems or queue_jobs if only if all threads are sleeping
        /// @warning must be called in the main thread only
        static void wakeThread(uv_async_t*);
        /// @brief iterate systems and call a event function depending on the bitmap or do nothing
        /// @warning must be called alone and in the main thread only, requires full access to the engine
        static void iterate_systems(ECS::JobsUtility *);
        static void iterate_systems(uv__work *,int);
        /// @brief provokes the threadpool (without checking remainding jobs)
        /// @warning must be called alone and in the main thread only, requires full access to the works list
        /// @see queue_jobs
        /// @details activeThreads must be 1 before calling this function
        static void queue_jobs(uv__work *,int);
        /// @brief prepares works array to be queued into the threadpool
        /// @warning must be running alone in any thread, requires full access to the works list, 
        /// @note dont forget to reserve memory in this->jobs array atleast equal to scheduleQueue.size() before calling this if you are calling this from threadpool
        /// @details activeThreads must be 1 before calling this function
        static void queue_jobs(uv__work *w);
        /// @brief the actual function jobs are handled in.
        static void work_jobs(uv__work *);
        /// @brief ensures that (only) the last worker thread will wake other threads if needed.
        /// @details is called after work_jobs, simply calling wakeThread
        static void after_work_jobs(uv__work *,int);
    public:
        /// @brief initialize threadpool and systems.
        /// @warning dont call signalRender before this
        void init();
        /// @brief gracefully stop the thread pool and the systems.
        void signalQuit();
        void schedule(const Schedule &s){
            scheduleQueue.push_back(s);
        }
    };
    extern align_ptr<JobsUtility> jobsUtility;
} // namespace ecs


#endif // THREADPOOL_HPP

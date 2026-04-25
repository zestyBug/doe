#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "Base/Job.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>

namespace ECS
{
    // thread pool for job system,
    // manages threads and arrays of jobs
    struct ThreadPool final {
        ThreadPool(const uint8_t thread_count);
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        /// @brief make sure isFinished before
        void signalStart();
        void signalStop();
        bool isFinished();
        uint32_t threadCount();
        /// @brief 
        /// @param context  
        /// @param func function returns zero if need to be executed again
        JobHandle schedule(const JobParameter&);
        JobHandle combineDependencies(const_span<JobHandle>);
        void prepareJobs();
        ~ThreadPool();
    private:
        struct JobEntry {
            JobHandle handle;
            uint32_t level;
            inline bool operator <  (const JobEntry& o){return this->level <  o.level;}
            inline bool operator >  (const JobEntry& o){return this->level >  o.level;}
            inline bool operator == (const JobEntry& o){return this->level == o.level;}
            inline bool operator <= (const JobEntry& o){return this->level <= o.level;}
            inline bool operator >= (const JobEntry& o){return this->level >= o.level;}
        };
        struct JobData {
            JobFunctionSignature function;
            void *context = nullptr;
            uint32_t batchCount = 1;
            uint32_t level = 0;
        };
        struct JobDataChunk {
            std::atomic<uint32_t> writeIndex = 0;
            std::atomic<uint32_t> readIndex = 0;
            std::atomic<uint32_t> readerCounter = 0;
            std::atomic<uint32_t> readerLevel = 0;
            std::atomic<uint32_t> capacity = 0;
            /// @brief batch begin index to start with
            std::atomic<uint32_t> *beginIndex = nullptr;
            JobData  *jobs = nullptr;
            /// @brief sorted by dependency. use the handle to find the real index.
            JobHandle *jobsArray = nullptr;
        };

        struct thread_context {
            /// @warning dont delete the pointer while threads are running
            std::atomic<JobDataChunk*> chunk;
            // number of waiting threads
            std::atomic<uint32_t> waitingThreads = 0;
            uint32_t threadCount = 0;
            // for mutual access to thread_context
            std::mutex mutex;
            // for sleeping on no context
            std::condition_variable condition;
        } context{};
        
        // need to keep track of threads so we can join them
        std::vector<std::thread,allocator<std::thread>> worker;
        align_ptr<JobDataChunk> chunk;

        /// @brief job thread main function
        static void func(thread_context*);
        void resizeJobPool(uint32_t);
    };
} // namespace ecs


#endif // THREADPOOL_HPP

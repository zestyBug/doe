#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>

namespace ECS
{
    struct JobHandle {
        friend class ThreadPool;
        // default value is the invalid value
        JobHandle() = default;
        inline operator bool   () const {return this->id>=0;}
        inline bool  operator !() const {return this->id<0;}
        inline bool operator == (const JobHandle& o) const {return this->id == o.id;}
        inline int32_t index  () const {return this->id;}
    private:
        static const uint32_t MaximumCount = INT32_MAX;
        JobHandle(int32_t v):id{v}{}
        inline bool operator <  (const JobHandle& o) const {return this->id <  o.id;}
        inline bool operator >  (const JobHandle& o) const {return this->id >  o.id;}
        inline bool operator <= (const JobHandle& o) const {return this->id <= o.id;}
        inline bool operator >= (const JobHandle& o) const {return this->id >= o.id;}
        int32_t id = -1;
    };
    // thread pool for job system,
    // manages threads and arrays of jobs
    struct ThreadPool final {
        typedef void(*JobFunctionSignature)(void*,uint32_t,uint32_t,JobHandle);
        struct JobParameter {
            JobFunctionSignature function;
            void *context = nullptr;
            uint32_t batchCount = 1;
            JobHandle dependsOn = JobHandle();
        };
        ThreadPool(const uint8_t thread_count);
        /// @brief make sure isFinished before
        void signalStart();
        void signalStop();
        bool isFinished();
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

#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include "cutil/basics.hpp"
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace ECS
{
    // thread pool for job system,
    // manages threads and arrays of jobs
    struct ThreadPool final {
        static constexpr size_t STOP_SIGNAL = UINT64_MAX;
        typedef size_t(JobFunctionSignature)(void*,size_t);
        struct alignas(64) thread_param {
            /// @brief function argumenst are context + previous returns value.
            /// initial index is STOP_SIGNAL and returns STOP_SIGNAL to stop calling anymore.
            JobFunctionSignature *func = nullptr;
            void *context = nullptr;
            // number of waiting threads
            uint32_t alive = true;
            uint32_t done = 0;

            // mutext for sleeping on no context
            std::mutex gmutex;
            // barrier for sleeping on no context
            std::condition_variable barrier;
        } thread_context{};

        // need to keep track of threads so we can join them
        std::vector<std::thread> worker;

        /// @brief job thread main function
        static void func(thread_param*);
    public:
        ThreadPool(const uint32_t thread_count);
        void restart();
        void graceStop();
        void waitInloop();
        bool done();
        /// @brief 
        /// @param context 
        /// @param func function returns zero if need to be executed again
        void submit(JobFunctionSignature *func_ptr, void *context);
        ~ThreadPool();
    };
} // namespace ecs


#endif // THREADPOOL_HPP

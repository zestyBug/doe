#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include "ECS/SystemManager.hpp"
#include "cutil/basics.hpp"
#include "cutil/semaphore.hpp"
#include "cutil/unique_ptr.hpp"
#include <mutex>
#include <thread>
#include <condition_variable>

namespace ECS
{

    class EntityComponentManager;
    class Archetype;
    
    struct IterationIndex {
        /// @brief current job archetype index for new thread that wants to pick a archetype
        uint32_t archetypeIndex = 0 ;
        /// @brief number of archetype proccessed
        uint32_t archetypeProcessed = 0;
    };


    // thread pool for job system,
    // manages threads and arrays of jobs
    struct ThreadPool final {
        // need to keep track of threads so we can join them
        std::vector<std::thread> worker;
        volatile uint32_t destroy = false;

        struct alignas(64){
            // accessing this varialbe needs semaphore
            std::vector<IterationIndex> jobStatues{};
            // each group has it own task queue
            span<jobContext> jobs{};
            span<unique_ptr<Archetype>> archetypes{};
            // current job index 
            volatile uint32_t job_index=0;
            // number of waiting threads
            volatile uint32_t waiting_threads = 0;
            version_t globalSystemVersion=0;
        } thread_info{} ;


        // mutext for access to thread_info
        std::mutex gmutex;
        // barrier for waiting and signaling
        std::condition_variable barrier;

        /// @brief use it only inside the mutex
        /// @note technicaly, if we are looking for new job means old archetypes are fully iterated, 
        /// but there may be some threads still processing old jobs, that new job is dependent on them. 
        /// @return false if required jobs are fully done
        bool needWait(uint32_t jobIndex);
        /// @brief finds a archetype that matches the jobd query
        /// @param indexing current value, represdenting where should we start from 
        /// @param job job context, containing query
        /// @return new updated value for IterationIndex
        IterationIndex findSuitableArchetype(IterationIndex indexing,uint32_t jobIndex);
        /// @brief helper to call callback for a given job and archetype
        void callExecution(uint32_t jobIndex,uint32_t archIndex);
        /// @brief job thread main function
        void func();
    public:
        ThreadPool(const uint32_t thread_count)
            :worker(thread_count)
        {
            std::lock_guard lock(this->gmutex);
            for(auto& t: this->worker)
                t = std::thread{&ThreadPool::func,this};
        }
        void restart(){
            std::lock_guard lock(this->gmutex);
            this->barrier.notify_all();
        }
        void init(SystemState& state,EntityComponentManager &ecm);
        ~ThreadPool(){
            {
                std::lock_guard lock(this->gmutex);
                destroy = true;
            }
            this->barrier.notify_all();
            for(auto& t: this->worker)
                t.join();
            this->worker.clear();
        }
    };
} // namespace ecs


#endif // THREADPOOL_HPP

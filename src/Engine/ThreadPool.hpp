#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "cutil/Semaphore.hpp"
#include "defs.hpp"
#include "defs.hpp"

namespace DOTS
{
    struct Job{
        virtual entity_range next(entity_t) = 0;
        virtual void proc(entity_range) = 0;
    };

    class ThreadPool final {
        // need to keep track of threads so we can join them
        std::vector<std::thread> worker;
        // each group has it own task queue
        std::vector<std::vector<std::unique_ptr<Job>>> group;


        Semaphore finished;
        std::mutex gmutex;
        std::condition_variable barrier;

        volatile bool destroy = false;
        volatile unsigned int waiting_threads = 0;
        volatile unsigned int group_index = 0;
        volatile unsigned int job_index = 0;
        volatile entityId_t entity_index = 0;
        volatile archtypeId_t archtype_index = 0;

        void func(){while(true){
                std::unique_lock lock(this->gmutex);

                unsigned int waiting_threads_buffer = this->waiting_threads;
                const unsigned int group_index_buffer = this->group_index;
                unsigned int job_index_buffer = this->job_index;


                if(this->destroy)
                    return;
                    
                //reached end of groups
                if(group_index_buffer >= this->group.size())
                    goto sleeping_finished;
                
                // this group is empty (barrier)
                else if(this->group[group_index_buffer].size() == 0)
                    goto sleeping;
                
                // checkin  validity of job_index_buffer is not required:
                // 1-It begins with 0, and we have checked that group size is not zero
                // 2-In proccess of increament at the end of every job, validity is checked.
                else {

                    Job *j = this->group[group_index_buffer][job_index_buffer].get();
                    const entity_range entity_index_buffer2 = j->next(entity_t{.index=this->entity_index, .archtype=this->archtype_index});
                    //printf("range: %u,%u\n",entity_index_buffer2.begin,entity_index_buffer2.end);

                    // reached end of a job
                    if(entity_index_buffer2.begin != entity_index_buffer2.end) {
                        this->entity_index   = entity_index_buffer2.end;
                        this->archtype_index = entity_index_buffer2.archtype;
                        lock.unlock();
                        j->proc(entity_index_buffer2);
                    }else{
                        job_index_buffer++;
                        // validity check remainding job in group
                        if(job_index_buffer >= this->group[group_index_buffer].size()){
                            // goto next group.
                            this->group_index=group_index_buffer+1;
                            this->job_index = 0;
                        }else{
                            this->job_index=job_index_buffer;
                        }
                        this->entity_index = 0;
                        this->archtype_index = 0;
                    }
                }
                continue;
            sleeping:
                if(++waiting_threads_buffer < this->worker.size()){
                    goto normal_thread_sleep;
                }else{
                    this->waiting_threads = 0;
                    this->group_index=group_index_buffer+1;
                    this->job_index = 0;
                    // TODO: i think these instruments are pointless, this claim require a analysis.
                    this->entity_index = 0;this->archtype_index = 0;
                    this->barrier.notify_all();
                }
                continue;
            sleeping_finished:
                if(++waiting_threads_buffer >= this->worker.size())
                    this->finished.release();
            normal_thread_sleep:
                this->waiting_threads=waiting_threads_buffer;
                this->barrier.wait(lock);
                continue;
        }}
    public:
        ThreadPool(const uint32_t thread_count)
            :worker(thread_count),finished(0)
        {
            std::lock_guard lock(this->gmutex);
            for(auto& t: this->worker)
                t = std::thread{&ThreadPool::func,this};
            group.reserve(64);
        }
        void wait(){
            this->finished.acquire();
            this->group.clear();
        }
        void restart(){
            std::lock_guard lock(this->gmutex);
            this->waiting_threads = 0;
            this->group_index = 0;
            this->job_index = 0;
            this->entity_index = 0;
            this->barrier.notify_all();
        }
        template<typename Type>
        void addJob(Type* j,size_t group_id){
            if(this->group.size() <= group_id)
                this->group.resize(group_id+1);
            this->group[group_id].emplace_back(j);
        }
        ~ThreadPool(){
            destroy = true;
            this->barrier.notify_all();
            for(auto& t: this->worker)
                t.join();
            this->worker.clear();
        }
    };
} // namespace DOTS


#endif // THREADPOOL_HPP

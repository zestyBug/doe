#include "ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "ECS/Archetype.hpp"

using namespace ECS;



ThreadPool::ThreadPool(const uint32_t thread_count):worker(thread_count)
{
    std::lock_guard lock(this->thread_context.gmutex);
    for(auto& t: this->worker)
        t = std::thread{&ThreadPool::func,&this->thread_context};
}
void ThreadPool::restart(){
    std::lock_guard lock(this->thread_context.gmutex);
    this->thread_context.done = false;
    this->thread_context.barrier.notify_all();
}
void ThreadPool::graceStop(){
    std::lock_guard lock(this->thread_context.gmutex);
    this->thread_context.alive = false;
}
bool ThreadPool::done(){
    return this->thread_context.done;
}
void ThreadPool::waitInloop(){
    while(true){
        {
            //std::lock_guard lock(this->thread_context.gmutex);
            if(this->thread_context.done)
                return;
        }
        //std::this_thread::yield();
    }
}
ThreadPool::~ThreadPool(){
    {
        std::lock_guard lock(this->thread_context.gmutex);
        this->thread_context.alive = false;
    }
    this->thread_context.barrier.notify_all();
    for(auto& t: this->worker)
        t.join();
    this->worker.clear();
}
void ThreadPool::submit(JobFunctionSignature *func_ptr, void *context){
    std::lock_guard lock(this->thread_context.gmutex);
    this->thread_context.func = func_ptr;
    this->thread_context.context = context;
    this->thread_context.done = false;
    this->thread_context.barrier.notify_all();
}
void ThreadPool::func(thread_param *context){
    JobFunctionSignature *functionCopy;
    void *contextCopy;
    size_t buffer=STOP_SIGNAL;
    while(context->alive){
        {
            std::unique_lock<std::mutex> lock(context->gmutex);
            context->barrier.wait(lock, [&] { return !context->done || !context->alive; });
            if (!context->alive)
                break;
            functionCopy = context->func;
            contextCopy = context->context;
        }
        if(functionCopy)
            while(STOP_SIGNAL != (buffer=functionCopy(contextCopy,buffer)))
                std::this_thread::yield();
        if(!context->done){
            std::lock_guard lock(context->gmutex);
            context->done = true;
        }
    }
}
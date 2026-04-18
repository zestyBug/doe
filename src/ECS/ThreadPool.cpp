#include "ECS/ThreadPool.hpp"
#include "ECS/EntityComponentStore.hpp"
#include "ECS/Archetype.hpp"

using namespace ECS;



ThreadPool::ThreadPool(const uint8_t thread_count):worker(thread_count)
{
    resizeJobPool(32);
    this->context.threadCount = thread_count;
    for(auto& t: this->worker)
        t = std::thread{&ThreadPool::func,&this->context};
}
void ThreadPool::signalStart() {
    std::lock_guard lock(this->context.mutex);
    this->chunk->readIndex.store(0);
    this->context.condition.notify_all();
}
void ThreadPool::signalStop() {
    std::lock_guard lock(this->context.mutex);
    this->chunk->readIndex.store(this->chunk->writeIndex.load());
}
bool ThreadPool::isFinished() {
    std::lock_guard lock(this->context.mutex);
    JobDataChunk *cchunk = this->context.chunk.load();
    if(cchunk == nullptr){
        if(this->context.waitingThreads == 0)
            return true;
    }else{
        if(cchunk->readIndex >= cchunk->writeIndex)
            if(this->context.waitingThreads == this->worker.size())
                return true;
    }
    return false;
}
ThreadPool::~ThreadPool(){
    this->context.chunk.store(nullptr);
    this->context.condition.notify_all();
    // chunk pointer is set to null, wait for result
    for(auto& t: this->worker)
        t.join();
}
JobHandle ThreadPool::submit(const JobData& data){
    if(chunk->writeIndex >= chunk->capacity)
        resizeJobPool(chunk->capacity);
    uint32_t index = chunk->writeIndex;
    if(index > JobHandle::MaximumCount)
        throw std::runtime_error("submit(): ThreadPool is full");
    if(data.dependsOn.index() >= (int32_t)index)
        throw std::runtime_error("submit(): invalid dependantOn job handle");
    memcpy(chunk->jobs+index,&data,sizeof(JobData));
    chunk->readIndex++;
    chunk->writeIndex++;
    return JobHandle((int32_t)index);
}
void ThreadPool::prepareJobs(){
    uint32_t count = chunk->writeIndex.load();
    if(count == 0)
        return;
    memset(chunk->finishedCounter,0,sizeof(std::atomic<uint32_t>)*count);
    memset(chunk->beginIndex,0,sizeof(std::atomic<uint32_t>)*count);
    align_ptr<JobEntry[]> buffer = make_align<JobEntry[]>(count);
    for(uint32_t i=0;i<count;i++){
        const int32_t dependency = chunk->jobs[i].dependsOn.index();
        if(dependency >= 0){
            if((uint32_t)dependency >= i)
                throw std::runtime_error("prepareJobs(): internal error");
            buffer[i] = JobEntry{JobHandle(i), buffer[dependency].level + 1};
        }else
            buffer[i] = JobEntry{JobHandle(i), 0};
    }
    std::sort(buffer.get(),buffer.get()+count);
    for(uint32_t i=0;i<count;i++)
        chunk->jobsArray[i] = buffer[i].handle;
}
void ThreadPool::func(thread_context *context){
    while (true)
    {
        JobDataChunk *chunkPtr = context->chunk.load();
        if(chunkPtr == nullptr)
            return;
        uint32_t readIndex = chunkPtr->readIndex.load();
        // no more job
        if(chunkPtr->writeIndex.load() <= readIndex)
        {
            std::unique_lock lock(context->mutex);
            context->waitingThreads++;
            context->condition.wait(lock);
            context->waitingThreads--;
            continue;
        }

        std::atomic<uint32_t> *beginIndexPtr = chunkPtr->beginIndex;
        std::atomic<uint32_t> *finishedCounterPtr = chunkPtr->finishedCounter;
        JobData *jobsPtr = chunkPtr->jobs;
        JobData jobData;
        JobEntry job;

        // copy the job to the stack
        memcpy(&job, chunkPtr->jobsArray+readIndex, sizeof(JobEntry));
        memcpy(&jobData, jobsPtr+job.handle.index(), sizeof(JobData));

        uint32_t batchSize = jobData.batchCount / context->threadCount;
        if(batchSize == 0)
            batchSize = 1;
        uint32_t batchBegin = beginIndexPtr[job.handle.index()].load();
        // a quick batch index validation test before waiting for dependancy
        if(batchBegin >= jobData.batchCount){
            chunkPtr->readIndex.compare_exchange_weak(readIndex,readIndex+1);
            continue;
        }
        // dependency check
        if(jobData.dependsOn.index() >= 0){
            const uint32_t dependencyBatchCount = jobsPtr[jobData.dependsOn.index()].batchCount;
            while (true)
            {
                std::unique_lock lock(context->mutex);
                const uint32_t currentBatchCount = finishedCounterPtr[jobData.dependsOn.index()].load();
                if(currentBatchCount >= dependencyBatchCount)
                    break;
                context->waitingThreads++;
                context->condition.wait(lock);
                context->waitingThreads--;
            }
        }
        // did i wait for a real job or it was just a barrier?
        if(likely(jobData.function != nullptr)){
            while(true) {
                batchBegin = beginIndexPtr[job.handle.index()].fetch_add(batchSize);
                if(batchBegin >= jobData.batchCount)
                    break;
                /// @example for(uint32_t i = batchBegin ; i != batchEnd ; ++i);
                uint32_t batchEnd = batchBegin + batchSize;
                if(batchEnd > jobData.batchCount)
                    batchEnd = jobData.batchCount;
                jobData.function(jobData.context,batchBegin,batchEnd,job.handle);
                const uint32_t batchCount = batchEnd - batchBegin;
                const uint32_t pFinVal = finishedCounterPtr[job.handle.index()].fetch_add(batchCount);
                // i am the last thread on this job
                if((pFinVal + batchCount) == jobData.batchCount){
                    std::lock_guard lock(context->mutex);
                    context->condition.notify_all();
                    break;
                }
            }
        }else{
            const uint32_t pFinVal = finishedCounterPtr[job.handle.index()].exchange(jobData.batchCount);
            beginIndexPtr[job.handle.index()].store(jobData.batchCount);
            if(pFinVal != jobData.batchCount){
                std::lock_guard lock(context->mutex);
                context->condition.notify_all();
            }
        }
        chunkPtr->readIndex.compare_exchange_weak(readIndex,readIndex+1);
    }
}
void ThreadPool::resizeJobPool(uint32_t capacity){
    align_ptr<JobDataChunk> temp;
    
    uint32_t size_temp[5];
    JobDataChunk *ptr1 = this->chunk.get();
    size_temp[0] = 64;
    size_temp[1] = size_temp[0] + alignTo64(sizeof(std::atomic<uint32_t>)*capacity);
    size_temp[2] = size_temp[1] + alignTo64(sizeof(std::atomic<uint32_t>)*capacity);
    size_temp[3] = size_temp[2] + alignTo64(sizeof(JobData)*capacity);
    size_temp[4] = size_temp[3] + alignTo64(sizeof(JobHandle)*capacity);
    JobDataChunk *ptr2 = (JobDataChunk*)allocator().allocate(size_temp[4]);
    temp.reset(ptr2);
    ptr2->writeIndex = 0;
    ptr2->readIndex = 0;
    ptr2->capacity = capacity;
    ptr2->beginIndex = (std::atomic<uint32_t>*)((uint8_t*)ptr2 + size_temp[0]);
    ptr2->finishedCounter = (std::atomic<uint32_t>*)((uint8_t*)ptr2 + size_temp[1]);
    ptr2->jobs = (JobData*)((uint8_t*)ptr2 + size_temp[2]);
    ptr2->jobsArray = (JobHandle*)((uint8_t*)ptr2 + size_temp[3]);
    if(ptr1)
    {
        const uint32_t prevCap = ptr1->capacity;
        if(capacity <= prevCap)
                throw std::runtime_error("");
        ptr2->writeIndex.store(prevCap);
        memcpy(ptr2->jobs, ptr1->jobs, prevCap);
    }
    this->chunk = std::move(temp);
    this->context.chunk.store(ptr2);
}
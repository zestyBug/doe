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
    this->chunk->readerCounter.store(0);
    this->chunk->readerLevel.store(0);
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
JobHandle ThreadPool::schedule(const JobParameter& data){
    if(chunk->writeIndex >= chunk->capacity)
        resizeJobPool(chunk->capacity);
    uint32_t index = chunk->writeIndex;
    if(index > JobHandle::MaximumCount)
        throw std::runtime_error("submit(): ThreadPool is full");
    if(data.dependsOn.index() >= (int32_t)index)
        throw std::runtime_error("submit(): invalid dependantOn job handle");
    chunk->jobs[index].function = data.function;
    chunk->jobs[index].context = data.context;
    chunk->jobs[index].batchCount = data.batchCount;
    if(data.dependsOn.index() >= 0)
        chunk->jobs[index].level = chunk->jobs[data.dependsOn.index()].level + 1;
    else
        chunk->jobs[index].level = 0;
    chunk->readIndex++;
    chunk->writeIndex++;
    return JobHandle((int32_t)index);
}
void ThreadPool::prepareJobs(){
    uint32_t count = chunk->writeIndex.load();
    JobData *jobs = chunk->jobs;
    if(count == 0)
        return;
    memset(chunk->beginIndex,0,sizeof(std::atomic<uint32_t>)*count);
    align_ptr<JobEntry[]> buffer = make_align<JobEntry[]>(count);
    for(uint32_t i=0;i<count;i++)
        buffer[i] = JobEntry{JobHandle(i), jobs[i].level};
    std::sort(buffer.get(),buffer.get()+count);
    for(uint32_t i=0;i<count;i++)
        chunk->jobsArray[i] = buffer[i].handle;
}
void ThreadPool::func(thread_context *context)
{
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
        JobData *jobsPtr = chunkPtr->jobs;
        JobData jobData;
        JobEntry job;

        // copy the job to the stack
        memcpy(&job, chunkPtr->jobsArray+readIndex, sizeof(JobEntry));
        memcpy(&jobData, jobsPtr+job.handle.index(), sizeof(JobData));

        // dependency check
        if(jobData.level > chunkPtr->readerLevel){
            chunkPtr->readerCounter += 1;
            uint32_t threadCountBuffer = context->threadCount;
            if(chunkPtr->readerCounter.compare_exchange_strong(threadCountBuffer,0)){
                chunkPtr->readerLevel += 1;
                std::unique_lock lock(context->mutex);
                context->condition.notify_all();
                continue;
            }else{
                std::unique_lock lock(context->mutex);
                while (jobData.level > chunkPtr->readerLevel)
                {
                    context->waitingThreads++;
                    context->condition.wait(lock);
                    context->waitingThreads--;
                }
            }
        }

        uint32_t batchSize = jobData.batchCount / context->threadCount;
        if(batchSize == 0)
            batchSize = 1;
        uint32_t batchBegin = beginIndexPtr[job.handle.index()].fetch_add(batchSize);
        // a quick batch index validation test before waiting for dependancy
        if(batchBegin >= jobData.batchCount){
            chunkPtr->readIndex.compare_exchange_weak(readIndex,readIndex+1);
            continue;
        }
        // did i wait for a real job or it was just a barrier?
        if(likely(jobData.function != nullptr)){
            while(true) {
                /// @example for(uint32_t i = batchBegin ; i != batchEnd ; ++i);
                uint32_t batchEnd = batchBegin + batchSize;
                if(batchEnd > jobData.batchCount)
                    batchEnd = jobData.batchCount;
                jobData.function(jobData.context,batchBegin,batchEnd,job.handle);
                batchBegin = beginIndexPtr[job.handle.index()].fetch_add(batchSize);
                if(batchBegin >= jobData.batchCount)
                    break;
            }
        }
        chunkPtr->readIndex.compare_exchange_weak(readIndex,readIndex+1);
    }
}
JobHandle ThreadPool::combineDependencies(const_span<JobHandle> jobs){
    JobHandle max = JobHandle();
    uint32_t maxLevel = 0;
    for(const JobHandle &j:jobs){
        if(j.index() < 0)
            continue;
        if((uint32_t)j.index() > this->chunk->writeIndex)
            throw std::invalid_argument("combineDependencies(): array contains invalid JobHandle(s)");
        const uint32_t level = this->chunk->jobs[j.index()].level;
        if(level > maxLevel){
            max = j;
            maxLevel = level;
        }
    }
    return max;
}
void ThreadPool::resizeJobPool(uint32_t capacity){
    align_ptr<JobDataChunk> temp;
    
    uint32_t size_temp[4];
    JobDataChunk *ptr1 = this->chunk.get();
    size_temp[0] = 64;
    size_temp[1] = size_temp[0] + alignTo64(sizeof(std::atomic<uint32_t>)*capacity);
    size_temp[2] = size_temp[1] + alignTo64(sizeof(JobData)*capacity);
    size_temp[3] = size_temp[2] + alignTo64(sizeof(JobEntry)*capacity);
    JobDataChunk *ptr2 = (JobDataChunk*)allocator().allocate(size_temp[3]);
    temp.reset(ptr2);
    ptr2->writeIndex = 0;
    ptr2->readIndex = 0;
    ptr2->capacity = capacity;
    ptr2->beginIndex = (std::atomic<uint32_t>*)((uint8_t*)ptr2 + size_temp[0]);
    ptr2->jobs = (JobData*)((uint8_t*)ptr2 + size_temp[1]);
    ptr2->jobsArray = (JobHandle*)((uint8_t*)ptr2 + size_temp[2]);
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
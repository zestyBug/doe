#include "ECS/ThreadPool.hpp"
#include "ECS/EntityComponentStore.hpp"
#include "ECS/Archetype.hpp"

ECS::internal::ThreadPool ECS::JobsUtility;
using namespace ECS;
using namespace ECS::internal;

uint32_t ThreadPool::threadCount(){
    return this->context.threadCount;
}
void ThreadPool::INIT(uint32_t thread_count)
{
    worker.reserve(thread_count);
    resizeJobPool(32);
    this->context.threadCount = thread_count;
    while(thread_count--)
        worker.emplace_back(&ThreadPool::func,&this->context);
}
void ThreadPool::reset() {
    std::lock_guard lock(this->context.mutex);
    JobDataChunk *chunk = this->context.chunk.load();
    chunk->writeIndex.store(0);
    chunk->readIndex.store(0);
}
void ThreadPool::signalStart() {
    std::lock_guard lock(this->context.mutex);
    JobDataChunk *chunk = this->context.chunk.load();
    chunk->readIndex.store(0);
    chunk->readerCounter.store(0);
    chunk->readerLevel.store(0);
    this->context.condition.notify_all();
}
void ThreadPool::signalStop() {
    std::lock_guard lock(this->context.mutex);
    JobDataChunk *chunk = this->context.chunk.load();
    chunk->readIndex.store(
        chunk->writeIndex.load()
    );
}
bool ThreadPool::isFinished() {
    std::lock_guard lock(this->context.mutex);
    JobDataChunk *chunk = this->context.chunk.load();
    if(chunk == nullptr){
        if(this->context.waitingThreads == 0)
            return true;
    }else{
        if(chunk->readIndex >= chunk->writeIndex)
            if(this->context.waitingThreads == this->worker.size())
                return true;
    }
    return false;
}
ThreadPool::~ThreadPool(){
    allocator().deallocate(this->context.chunk.load());
    this->context.chunk.store(nullptr);
    this->context.condition.notify_all();
    // chunk pointer is set to null, wait for result
    for(auto& t: this->worker)
        t.join();
}
JobHandle ThreadPool::schedule(const JobParameter& data){
    JobDataChunk *chunk = context.chunk.load();
    if(chunk->writeIndex >= chunk->capacity)
        resizeJobPool(chunk->capacity * 2);
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
    JobDataChunk *chunk = this->context.chunk.load();
    JobEntry *buffer = chunk->buffer;
    uint32_t count = chunk->writeIndex.load();
    JobData *jobs = chunk->jobs;
    if(count == 0)
        return;
    memset(chunk->beginIndex, 0, sizeof(std::atomic<uint32_t>)*count);
    for(uint32_t i=0;i<count;i++)
        buffer[i] = JobEntry{JobHandle(i), jobs[i].level};
    std::sort(buffer,buffer+count);
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
    JobDataChunk *chunk = this->context.chunk.load();
    JobHandle max = JobHandle();
    uint32_t maxLevel = 0;
    for(const JobHandle &j:jobs){
        if(j.index() < 0)
            continue;
        if((uint32_t)j.index() > chunk->writeIndex)
            throw std::invalid_argument("combineDependencies(): array contains invalid JobHandle(s)");
        const uint32_t level = chunk->jobs[j.index()].level;
        if(level > maxLevel){
            max = j;
            maxLevel = level;
        }
    }
    return max;
}
void ThreadPool::resizeJobPool(uint32_t capacity){
    JobDataChunk *ptr1 = this->context.chunk.load();
    uint32_t size_temp[5];
    size_temp[0] = 64;
    size_temp[1] = size_temp[0] + alignTo64(sizeof(std::atomic<uint32_t>)*capacity);
    size_temp[2] = size_temp[1] + alignTo64(sizeof(JobData)  *capacity);
    size_temp[3] = size_temp[2] + alignTo64(sizeof(JobHandle)*capacity);
    size_temp[4] = size_temp[3] + alignTo64(sizeof(JobEntry) *capacity);
    align_ptr<JobDataChunk> ptr2;
    ptr2.reset((JobDataChunk*)allocator().allocate(size_temp[4]));
    ptr2->writeIndex = 0;
    ptr2->readIndex = 0;
    ptr2->capacity = capacity;
    ptr2->beginIndex = (std::atomic<uint32_t>*)((uint8_t*)ptr2.get() + size_temp[0]);
    ptr2->jobs       = (JobData*)              ((uint8_t*)ptr2.get() + size_temp[1]);
    ptr2->jobsArray  = (JobHandle*)            ((uint8_t*)ptr2.get() + size_temp[2]);
    ptr2->buffer     = (JobEntry*)             ((uint8_t*)ptr2.get() + size_temp[3]);
    if(ptr1)
    {
        const uint32_t prevCap = ptr1->capacity;
        if(capacity <= prevCap)
                throw std::runtime_error("");
        ptr2->writeIndex.store(prevCap);
        memcpy(ptr2->jobs, ptr1->jobs, prevCap);
    }
    this->context.chunk.store(ptr2.release());
}
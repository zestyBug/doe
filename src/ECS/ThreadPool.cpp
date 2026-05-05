#include "ECS/ThreadPool.hpp"
#include "ECS/Engine.hpp"
#include "uv.h"

align_ptr<ECS::DOE> ECS::sharedEngine;
using namespace ECS;


void empty_work(uv_work_t* req);
void initialize_work(uv_work_t* req, int status);
void do_nothing  (uv__work *context, int status);
/// @brief wake threads to do the created works
void wake_threads();
/// @brief call system update to create some new works
void call_systems();
void work        (uv__work *context);
void work_systems(uv__work *context);


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
    void *context = NULL;
    uint32_t batchCount = 1;
    uint32_t batchStepSize = 1;
    uint32_t level = 0;
};
struct ECS::JobDataChunk {
    void resizeJobPool(uint32_t);
    void prepareJobs();
    JobDataChunk()
    {
        resizeJobPool(32);
        for(uv_work_t &w:works){
            w.work_req.loop = uv_default_loop();
        }
    }
    uint32_t               writeIndex = 0;
    std::atomic<uint32_t>  readIndex = 0;
    std::atomic<uint32_t>  readerLevel = 0;
    std::atomic<uint32_t>  activeThreads = 0;
    uint32_t               capacity = 0;
    uv_work_t              works[640];
    /// @brief batch begin index to start with
    align_ptr<JobData[]>   jobs = NULL;
    std::atomic<uint32_t> *beginIndex = NULL;
    /// @brief sorted by dependency. use the handle to find the real index.
    JobHandle             *jobsArray = NULL;
    JobEntry              *buffer = NULL;
};
JobDataChunk sharedData;


void JobsUtility::init(){
    uv_queue_work(uv_default_loop(),sharedData.works,&empty_work,&initialize_work);
}
JobHandle JobsUtility::schedule(const JobParameter& data){
    if(sharedData.writeIndex >= sharedData.capacity)
        sharedData.resizeJobPool(sharedData.capacity * 2);
    uint32_t index = sharedData.writeIndex;
    if(data.function == NULL || data.batchStepSize < 1  || data.batchCount < 1)
        throw std::invalid_argument("schedule()");
    if(index > JobHandle::MaximumCount)
        throw std::runtime_error("schedule(): ThreadPool is full");
    if(data.dependsOn.index() >= (int32_t)index)
        throw std::runtime_error("schedule(): invalid dependantOn job handle");
    {
        JobData &job = sharedData.jobs[index];
        new (&job) JobData();
        job.function = data.function;
        job.context = data.context;
        job.batchCount = data.batchCount;
        job.batchStepSize = data.batchStepSize;
        if(data.dependsOn.index() >= 0)
            job.level = sharedData.jobs[data.dependsOn.index()].level + 1;
    }
    sharedData.writeIndex++;
    return JobHandle((int32_t)index);
}
void JobDataChunk::prepareJobs(){
    JobEntry *buffer = sharedData.buffer;
    uint32_t count = sharedData.writeIndex;
    JobData *jobs = sharedData.jobs.get();
    if(count < 1)
        return;
    memset(sharedData.beginIndex, 0, sizeof(std::atomic<uint32_t>)*count);
    for(uint32_t i=0;i<count;i++)
        buffer[i] = JobEntry{JobHandle(i), jobs[i].level};
    std::sort(buffer,buffer+count);
    for(uint32_t i=0;i<count;i++)
        sharedData.jobsArray[i] = buffer[i].handle;
}
JobHandle JobsUtility::combineDependencies(const_span<JobHandle> jobs){
    JobHandle max = JobHandle();
    uint32_t maxLevel = 0;
    for(const JobHandle &j:jobs){
        if(j.index() < 0)
            continue;
        if((uint32_t)j.index() > sharedData.writeIndex)
            throw std::invalid_argument("combineDependencies(): array contains invalid JobHandle(s)");
        const uint32_t level = sharedData.jobs[j.index()].level;
        if(level > maxLevel){
            max = j;
            maxLevel = level;
        }
    }
    return max;
}
void JobDataChunk::resizeJobPool(uint32_t capacity){
    if(sharedData.capacity >= capacity)
        throw std::invalid_argument("resizeJobPool(): can't resize to smaller array");
    uint32_t size_temp[5];
    size_temp[0] =                alignTo64(sizeof(JobData)  *capacity);
    size_temp[1] = size_temp[0] + alignTo64(sizeof(uint32_t) *capacity);
    size_temp[2] = size_temp[1] + alignTo64(sizeof(JobHandle)*capacity);
    size_temp[3] = size_temp[2] + alignTo64(sizeof(JobEntry) *capacity);
    align_ptr<JobDataChunk> ptr2{(JobDataChunk*)allocator().allocate(size_temp[3])};
    if(sharedData.jobs.get())
        memcpy(ptr2.get(), sharedData.jobs.get(), sizeof(JobData)*sharedData.capacity);
    sharedData.capacity = capacity;
    sharedData.jobs.reset((JobData*)ptr2.get());
    sharedData.beginIndex = (std::atomic<uint32_t>*)  ((uint8_t*)ptr2.get() + size_temp[0]);
    sharedData.jobsArray  = (JobHandle*)              ((uint8_t*)ptr2.get() + size_temp[1]);
    sharedData.buffer     = (JobEntry*)               ((uint8_t*)ptr2.get() + size_temp[2]);
    ptr2.release();
}



void empty_work(uv_work_t* req){}
void initialize_work(uv_work_t* req, int){
    sharedData.works->loop = uv_default_loop();
    sharedData.works->data = NULL;
    sharedData.works->work_req.loop = uv_default_loop();
    sharedData.works->work_req.done = &do_nothing;
    sharedData.works->work_req.work = &work_systems;
    sharedData.activeThreads++;
    uv_queue_work_slow(sharedData.works);
}
void do_nothing(uv__work *context, int status){
    unsigned int *count = &((uv_work_t *) ((uint8_t*)(context) - offsetof(uv_work_t, work_req)))->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;

    if(unlikely(sharedData.activeThreads.fetch_sub(1) == 1)){
        if(status == UV_ECANCELED)
            return;
        uint32_t readIndex = sharedData.readIndex.load();
        // no more job
        if(sharedData.writeIndex <= readIndex)
            call_systems();
        else
            wake_threads();
    }
}
static uint32_t counter=0;
void call_systems(){
    if(counter > 10000)
        return;
    counter++;
    sharedData.works->loop = uv_default_loop();
    sharedData.works->data = NULL;
    sharedData.works->work_req.loop = uv_default_loop();
    sharedData.works->work_req.done = &do_nothing;
    sharedData.works->work_req.work = &work_systems;
    sharedData.activeThreads++;
    uv_queue_work_slow(sharedData.works);
}
void wake_threads(){
    uv_work_t *begin = sharedData.works + 4;
    uint32_t numWorkerThread = std::min<uint32_t>(uv_num_worker_threads(),60);
    uv_work_t *end = sharedData.works + 4 + numWorkerThread;
    sharedData.activeThreads += numWorkerThread;
    while(begin < end){
        begin->loop = uv_default_loop();
        begin->data = &sharedData;
        begin->work_req.loop = uv_default_loop();
        begin->work_req.done = &do_nothing;
        begin->work_req.work = &work;
        begin++;
    }
    begin = sharedData.works + 4;
    while(begin < end){
        uv_queue_work_slow(begin);
        begin++;
    }
}
void work_systems(uv__work*)
{
    sharedData.writeIndex = 0;
    sharedData.readIndex = 0;
    sharedData.readerLevel = 0;

    sharedEngine->eqm.updateNewArchetypes();
    sharedEngine->dpm.clear();
    align_ptr<ISystem> *begin = sharedEngine->sys.data();
    align_ptr<ISystem> *end = begin + sharedEngine->sys.size();
    while (begin != end)
    {
        begin->get()->OnUpdate(sharedEngine.get());
        begin++;
    }
    sharedData.prepareJobs();
}
void work(uv__work *w)
{
    uv_work_t* arg = (uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req));
    JobDataChunk *chunkPtr = (JobDataChunk*)arg->data;
    while (true)
    {
        uint32_t readIndex = chunkPtr->readIndex.load();
        // no more job
        if(chunkPtr->writeIndex <= readIndex)
            return;

        JobEntry job;

        // copy the job to the stack
        memcpy(&job, chunkPtr->jobsArray+readIndex, sizeof(JobEntry));

        std::atomic<uint32_t> &beginIndexPtr = chunkPtr->beginIndex[job.handle.index()];
        JobData jobData;
        memcpy(&jobData, chunkPtr->jobs.get() + job.handle.index(), sizeof(JobData));

        // dependency check
        if(jobData.level > chunkPtr->readerLevel)
            return;
        uint32_t batchBegin = beginIndexPtr.fetch_add(1);
        // if(likely(jobData.function != NULL)){}
        while(true)
        {
            if(batchBegin >= jobData.batchCount)
                break;
            batchBegin *= jobData.batchStepSize;
            jobData.function(
                jobData.context,
                batchBegin,
                batchBegin+jobData.batchStepSize
            );
            batchBegin = beginIndexPtr.fetch_add(1);
        }
        chunkPtr->readIndex.compare_exchange_weak(readIndex,readIndex+1);
    }
}
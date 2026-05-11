#include "ECS/ThreadPool.hpp"
#include "ECS/Engine.hpp"
#include "uv.h"

align_ptr<ECS::DOE> sharedEngine;
using namespace ECS;

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
enum Request : uint32_t {
    Exit = 1,
    Render = 2,
    Timer = 4
};
struct ECS::JobDataChunk {
    void resizeJobPool(uint32_t);
    void prepareJobs();
    JobDataChunk()
    {
        resizeJobPool(Constants::InitialJobPoolCapacity);
        for(uv_work_t &w:works){
            w.work_req.loop = uv_default_loop();
        }
    }

    uint32_t               writeIndex = 0;
    std::atomic<uint32_t>  readIndex = 0;
    std::atomic<uint32_t>  readerLevel = 0;
    std::atomic<uint32_t>  activeThreads = 0;
    uint32_t               capacity = 0;
    std::atomic<uint32_t>  bitmask;
    /// @brief batch begin index to start with
    align_ptr<JobData[]>   jobs = NULL;
    std::atomic<uint32_t>  *beginIndex = NULL;
    /// @brief sorted by dependency. use the handle to find the real index.
    JobHandle              *jobsArray = NULL;
    JobEntry               *buffer = NULL;
    uv_timer_t             fixedTimer;
    alignas(64) uv_work_t  works[4];
};
JobDataChunk sharedData;

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
    JobEntry *bufferPtr = sharedData.buffer;
    uint32_t count = sharedData.writeIndex;
    JobData *jobsPtr = sharedData.jobs.get();
    if(count < 1)
        return;
    memset(sharedData.beginIndex, 0, sizeof(std::atomic<uint32_t>)*count);
    for(uint32_t i=0;i<count;i++)
        bufferPtr[i] = JobEntry{JobHandle(i), jobsPtr[i].level};
    std::sort(bufferPtr,bufferPtr+count);
    for(uint32_t i=0;i<count;i++)
        sharedData.jobsArray[i] = bufferPtr[i].handle;
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
    uint32_t size_temp[4];
    size_temp[0] =                sizeof(JobData)  *capacity;
    size_temp[1] = size_temp[0] + sizeof(std::atomic<uint32_t>)*capacity;
    size_temp[2] = size_temp[1] + sizeof(JobHandle)*capacity;
    size_temp[3] = size_temp[2] + sizeof(JobEntry) *capacity;
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









/**
 * a
 * b
 * c
 */



void initialize_work(uv_work_t* req);
void initialize_after_work(uv_work_t* req, int);
void on_fixed_timer(uv_timer_t *);
/// @brief Evokes either iterate_systems+queue_jobs or queue_jobs if only if all threads are sleeping
void wakeThread();
/// @brief iterate systems and call a event function depending on the bitmap or do nothing
/// @warning must be running alone, requires full access engine
void iterate_systems(uv__work *);
/// @brief doing nothing
/// @details may be called after iterate_systems
void do_nothing(uv__work *,int);
/// @brief slow stoping the threadpool
/// @warning must be running alone and in the main thread
void on_exit(uv__work *,int);
/// @brief a wrapper to call queue_jobs();
/// @see queue_jobs
/// @details is called after iterate_systems
void queue_jobs(uv__work *,int);
/// @brief prepares works array to be queued into the threadpool
/// @warning must be running alone and in the main thread, requires full access to works list
void queue_jobs();
/// @brief the actual function jobs are handled in.
void work_jobs(uv__work *);
/// @brief ensures that (only) the last worker thread will wake other threads if needed.
/// @details is called after work_jobs, simply calling wakeThread
void after_work_jobs(uv__work *,int);









void JobsUtility::init(){
    sharedData.activeThreads++;
    uv_queue_work(uv_default_loop(),sharedData.works,&initialize_work,&initialize_after_work);
}
void initialize_work(uv_work_t*){
    std::vector<void* (*)(DOE *)> &list = JobsUtility::_get_initialize_list();
    auto &sys = sharedEngine->sys;
    sys.reserve(list.size());
    for(auto func:list){
        sys.emplace_back( (ISystem*)func(sharedEngine.get()) );
    }
}
void initialize_after_work(uv_work_t*, int){
    uv_timer_init(uv_default_loop(), &sharedData.fixedTimer);
    uv_timer_start(&sharedData.fixedTimer, on_fixed_timer, 0, 20);
    sharedData.activeThreads--;
    //queue_fixed_update();
}
void on_fixed_timer(uv_timer_t *) {
    sharedData.bitmask |= Request::Timer;
    wakeThread();
}
void JobsUtility::signalQuit(){
    sharedData.bitmask |= Request::Exit;
    wakeThread();
}
void JobsUtility::signalRender(){
    sharedData.bitmask |= Request::Render;
    wakeThread();
}
void wakeThread(){
    uint32_t expected = 0;
    if(sharedData.activeThreads.compare_exchange_weak(expected,1)){
        if(sharedData.writeIndex <= sharedData.readIndex.load()){
            sharedData.works[0].loop = uv_default_loop();
            sharedData.works[0].data = NULL;
            sharedData.works[0].work_req.loop = uv_default_loop();
            sharedData.works[0].work_req.done = &queue_jobs;
            sharedData.works[0].work_req.work = &iterate_systems;
            uv_queue_work_slow(sharedData.works);
        }else{
            queue_jobs();
        }
    }
}
void iterate_systems(uv__work *w){
    sharedData.writeIndex = 0;
    sharedData.readIndex = 0;
    sharedData.readerLevel = 0;
    sharedEngine->eqm.updateNewArchetypes();
    sharedEngine->dpm.clear();
    align_ptr<ISystem> *begin =         sharedEngine->sys.data();
    align_ptr<ISystem> *end   = begin + sharedEngine->sys.size();
    if(unlikely(sharedData.bitmask & Request::Exit)) {
        while (begin != end){
            (*begin)->OnDestroy(sharedEngine.get());
            begin++;
        }
        w->done = on_exit;
    } else if(sharedData.bitmask & Request::Timer) {
        while (begin != end){
            (*begin)->OnFixedUpdate(sharedEngine.get());
            begin++;
        }
        if(sharedData.writeIndex == 0)
            w->done = do_nothing;
        else
            sharedData.prepareJobs();
        sharedData.bitmask &= ~Request::Timer;
    } else if(sharedData.bitmask & Request::Render) {
        while (begin != end){
            (*begin)->OnUpdate(sharedEngine.get());
            begin++;
        }
        if(sharedData.writeIndex == 0)
            w->done = do_nothing;
        else
            sharedData.prepareJobs();
        sharedData.bitmask &= ~Request::Render;
    } else {
        w->done = do_nothing;
    }
}
void on_exit(uv__work *w,int) {
    unsigned int *count = &((uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req)))->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    // dead locking the JobsUtility for ever
    sharedData.activeThreads++;
    uv_timer_stop(&sharedData.fixedTimer);
}
void do_nothing(uv__work *w,int){
    unsigned int *count = &((uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req)))->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    sharedData.activeThreads--;
}
void queue_jobs()
{
    const uint32_t numWorkerThread = std::min<uint32_t>(
        uv_num_worker_threads(),
        sizeof(sharedData.works)/sizeof(sharedData.works[0])
    );
    uint32_t expected = 1;
    if(unlikely(!sharedData.activeThreads.compare_exchange_weak(expected,numWorkerThread,std::memory_order_relaxed)))
        throw std::runtime_error("queue_jobs(): thread internal error");

    uv_work_t *begin = sharedData.works;
    uv_work_t *end   = sharedData.works + numWorkerThread;
    while(begin < end){
        begin->loop = uv_default_loop();
        begin->data = &sharedData;
        begin->work_req.loop = uv_default_loop();
        begin->work_req.done = &after_work_jobs;
        begin->work_req.work = &work_jobs;
        begin++;
    }
    begin = sharedData.works;
    while(begin < end){
        uv_queue_work_slow(begin);
        begin++;
    }
}
void queue_jobs(uv__work *w,int)
{
    unsigned int *count = &((uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req)))->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    queue_jobs();
}
void work_jobs(uv__work *)
{
    //uv_work_t* arg = (uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req));
    //JobDataChunk &sharedData = *(JobDataChunk*)arg->data;
    while (true)
    {
        uint32_t readIndex = sharedData.readIndex.load();
        // no more job
        if(unlikely(sharedData.writeIndex <= readIndex))
            return;

        JobHandle job = sharedData.jobsArray[readIndex];
        std::atomic<uint32_t> &beginIndexPtr = sharedData.beginIndex[job.index()];
        JobData jobData;
        memcpy(&jobData, sharedData.jobs.get() + job.index(), sizeof(JobData));

        // dependency check
        if(jobData.level > sharedData.readerLevel)
            return;
        // if(likely(jobData.function != NULL)){}
        while(true)
        {
            uint32_t batchBegin = beginIndexPtr.fetch_add(1);
            if(batchBegin >= jobData.batchCount)
                break;
            batchBegin *= jobData.batchStepSize;
            jobData.function(
                jobData.context,
                batchBegin,
                batchBegin+jobData.batchStepSize
            );
        }
        sharedData.readIndex.compare_exchange_weak(readIndex,readIndex+1);
    }
}
void after_work_jobs(uv__work *w,int status){
    unsigned int *count = &((uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req)))->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    if(status)
        JobsUtility::signalQuit();
    if(sharedData.activeThreads.fetch_sub(1) == 1){
        if(sharedData.writeIndex <= sharedData.readIndex.load()){
            if(sharedData.bitmask)
                wakeThread();
        }else{
            wakeThread();
        }
    }
}
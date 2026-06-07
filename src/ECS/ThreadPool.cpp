#include "ECS/ThreadPool.hpp"
#include "ECS/JobChunk.hpp"
#include "ECS/Engine.hpp"
#include "glfw/glfw3.h"
#include "uv.h"

std::unique_ptr<ECS::DOE> sharedEngine;
std::vector<ECS::ISystem*(*)(ECS::DOE&)>& ECS::_get_initialize_list() {
    static std::vector<ISystem*(*)(DOE&)> tests;
    return tests;
}
struct GLFWwindow;
GLFWwindow* window;

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
alignas(Constants::CacheLineSize) struct ECS::JobDataChunk {
    void resizeJobPool(uint32_t);
    void prepareJobs();
    void init()
    {
        uint32_t size[3];
        workCount = uv_num_worker_threads();
        size[0] = alignCacheLineSize(sizeof(uv_work_t)*workCount);
        size[1] = size[0] + alignCacheLineSize(sizeof(uv_timer_t));
        size[2] = size[1] + alignCacheLineSize(sizeof(uv_async_t));
        works = make_align<uv_work_t[]>(size[2]);
        fixedTimer = (uv_timer_t*)((uint8_t*)works.get() + size[0]);
        wakecall = (uv_async_t*)((uint8_t*)works.get() + size[1]);
    }

    std::atomic<uint32_t>  writeIndex = 0;
    std::atomic<uint32_t>  readIndex = 0;
    std::atomic<uint32_t>  readerLevel = 0;
    std::atomic<uint32_t>  activeThreads = 0;
    std::atomic<uint32_t>  capacity = 0;
    std::atomic<uint32_t>  bitmask = 0;
    alignas(Constants::CacheLineSize) uint32_t workCount;
    /// @brief batch begin index to start with
    align_ptr<JobData[]>   jobs = NULL;
    std::atomic<uint32_t>  *beginIndex = NULL;
    /// @brief sorted by dependency. use the handle to find the real index.
    JobHandle              *jobsArray = NULL;
    JobEntry               *buffer = NULL;
    align_ptr<uv_work_t[]> works;
    uv_timer_t             *fixedTimer = NULL;
    uv_async_t             *wakecall = NULL;
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
        return;//throw std::invalid_argument("resizeJobPool(): can't resize to smaller array");
    uint32_t size_temp[4];
    size_temp[0] =                sizeof(JobData)  *capacity;
    size_temp[1] = size_temp[0] + sizeof(std::atomic<uint32_t>)*capacity;
    size_temp[2] = size_temp[1] + sizeof(JobHandle)*capacity;
    size_temp[3] = size_temp[2] + sizeof(JobEntry) *capacity;
    align_ptr<JobDataChunk> ptr2{(JobDataChunk*)allocator().allocate(size_temp[3])};
    if(sharedData.jobs.get())
        memcpy(ptr2.get(), sharedData.jobs.get(), sizeof(JobData)*sharedData.writeIndex);
    sharedData.capacity = capacity;
    sharedData.jobs.reset((JobData*)ptr2.get());
    sharedData.beginIndex = (std::atomic<uint32_t>*)  ((uint8_t*)ptr2.get() + size_temp[0]);
    sharedData.jobsArray  = (JobHandle*)              ((uint8_t*)ptr2.get() + size_temp[1]);
    sharedData.buffer     = (JobEntry*)               ((uint8_t*)ptr2.get() + size_temp[2]);
    ptr2.release();
}









#pragma region Libuv callbacks



void on_fixed_timer(uv_timer_t *);
/// @brief calls either iterate_systems or queue_jobs if only if all threads are sleeping
/// @warning must be called in the main thread only
void wakeThread(uv_async_t*);
/// @brief iterate systems and call a event function depending on the bitmap or do nothing
/// @warning must be called alone and in the main thread only, requires full access to the engine
void iterate_systems();
void iterate_systems(uv__work *w,int);
/// @brief provokes the threadpool (without checking remainding jobs)
/// @warning must be called alone and in the main thread only, requires full access to the works list
/// @see queue_jobs
/// @details activeThreads must be 1 before calling this function
void queue_jobs(uv__work *,int);
/// @brief prepares works array to be queued into the threadpool
/// @warning must be running alone in any thread, requires full access to the works list, 
/// @note dont forget to reserve memory in sharedData.jobs array atleast equal to scheduleQueue.size() before calling this if you are calling this from threadpool
/// @details activeThreads must be 1 before calling this function
void queue_jobs(uv__work *w);
/// @brief the actual function jobs are handled in.
void work_jobs(uv__work *);
/// @brief ensures that (only) the last worker thread will wake other threads if needed.
/// @details is called after work_jobs, simply calling wakeThread
void after_work_jobs(uv__work *,int);

void JobsUtility::init(){
    sharedData.init();
    std::vector<ISystem* (*)(DOE &)> &list = _get_initialize_list();
    auto &sys = sharedEngine->sys;
    sys.reserve(list.size());
    for(auto func:list)
        sys.emplace_back( func(*sharedEngine.get()) );
    uv_timer_init(uv_default_loop(), sharedData.fixedTimer);
    uv_async_init(uv_default_loop(), sharedData.wakecall, wakeThread);
    uv_timer_start(sharedData.fixedTimer, on_fixed_timer, 0, 20);
}
void on_fixed_timer(uv_timer_t *) {
    sharedData.bitmask |= Request::Timer;
    uv_async_send(sharedData.wakecall);
}
void JobsUtility::signalQuit(){
    sharedData.bitmask |= Request::Exit;
    uv_async_send(sharedData.wakecall);
    glfwSetWindowShouldClose(window, 1);
}
void JobsUtility::signalRender(){
    sharedData.bitmask |= Request::Render;
    uv_async_send(sharedData.wakecall);
}
void wakeThread(uv_async_t*){
    uint32_t expected = 0;
    if(sharedData.activeThreads.compare_exchange_weak(expected,1)){
        if(sharedData.writeIndex <= sharedData.readIndex.load()){
            iterate_systems();
        }else{
            queue_jobs(nullptr,0);
        }
    }
}
void iterate_systems(uv__work *w,int) {
    unsigned int *count = &w->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    iterate_systems();
}
void iterate_systems(){
    again:;
    {
        std::unique_ptr<ISystem> *begin =         sharedEngine->sys.data();
        std::unique_ptr<ISystem> *end   = begin + sharedEngine->sys.size();
        if(unlikely(sharedData.bitmask & Request::Exit)) {
            while (begin != end){
                (*begin)->OnDestroy(*sharedEngine);
                begin++;
            }
            uv_timer_stop(sharedData.fixedTimer);
            uv_unref((uv_handle_t*)sharedData.wakecall);
            uv_stop(uv_default_loop());
            return;
        } else if(sharedData.bitmask & Request::Timer) {
            while (begin != end){
                (*begin)->OnFixedUpdate(*sharedEngine);
                begin++;
            }
            sharedData.bitmask &= ~Request::Timer;
        } else if(sharedData.bitmask & Request::Render) {
            while (begin != end){
                (*begin)->OnUpdate(*sharedEngine);
                begin++;
            }
            sharedData.bitmask &= ~Request::Render;
        } else {
            sharedData.activeThreads--;
            return;
        }
    }
    sharedEngine->eqm.updateNewArchetypes();
    sharedEngine->ecs.cleanChangeList();
    if(!sharedEngine->scheduleQueue.empty())
    {
        sharedData.resizeJobPool(sharedEngine->scheduleQueue.size());
        sharedData.works[0].data = NULL;
        sharedData.works[0].work_req.loop = uv_default_loop();
        sharedData.works[0].work_req.done = &queue_jobs;
        sharedData.works[0].work_req.work = &queue_jobs;
        uv_queue_work_slow(sharedData.works);
    }else{
        if(sharedData.bitmask.load())
            goto again;
        else
            sharedData.activeThreads--;
    }
}
void queue_jobs(uv__work *w,int){
    if(w){
        unsigned int *count = &w->loop->active_reqs.count;
        if(*count <= 0)
            throw std::runtime_error("");
        (*count)--;
    }

    uint32_t expected = 1;
    const uint32_t numWorkerThread = std::min<uint32_t>(
        uv_num_worker_threads(),
        sharedData.workCount
    );
    if(unlikely(!sharedData.activeThreads.compare_exchange_weak(expected,numWorkerThread)))
        throw std::runtime_error("queue_jobs(): thread internal error");
    uv_work_t *begin = sharedData.works;
    uv_work_t *end   = sharedData.works + numWorkerThread;
    begin = sharedData.works;
    while(begin < end){
        begin->work_req.work = &work_jobs;
        uv_queue_work_slow(begin);
        begin++;
    }
}
void queue_jobs(uv__work *w){
    sharedData.writeIndex = 0;
    sharedData.readIndex = 0;
    sharedData.readerLevel = 0;
    sharedEngine->dpm.clear();
    for(Schedule sch:sharedEngine->scheduleQueue){
        if(sch.parallel)
            sch.jw->scheduleParallel(sch.qb,sharedEngine->dpm);
        else
            sch.jw->schedule(sch.qb,sharedEngine->dpm);
    }
    sharedEngine->scheduleQueue.clear();
    if(sharedData.writeIndex != 0){
        sharedData.prepareJobs();
        const uint32_t numWorkerThread = std::min<uint32_t>(
            uv_num_worker_threads(),
            sharedData.workCount
        );
        if(unlikely(sharedData.activeThreads.load() != 1))
            throw std::runtime_error("queue_jobs(): thread internal error");

        uv_work_t *begin = sharedData.works;
        uv_work_t *end   = sharedData.works + numWorkerThread;
        while(begin < end) {
            begin->data = &sharedData;
            begin->work_req.loop = uv_default_loop();
            begin->work_req.done = &after_work_jobs;
            begin++;
        }
    }else
        w->done = &iterate_systems;
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
    unsigned int *count = &w->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    if(status)
        JobsUtility::signalQuit();
    if(sharedData.activeThreads.fetch_sub(1) == 1){
        wakeThread(nullptr);
    }
}

#pragma endregion Libuv callbacks
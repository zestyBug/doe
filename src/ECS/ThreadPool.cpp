#include "ECS/ThreadPool.hpp"
#include "ECS/Engine.hpp"
#include "GraphicSystem.hpp"
#include "uv.h"

std::vector<ECS::ISystem*(*)(ECS::DOE&)>& ECS::_get_initialize_list() {
    static std::vector<ISystem*(*)(DOE&)> tests;
    return tests;
}
align_ptr<ECS::JobsUtility> ECS::jobsUtility;

struct ECS::JobsUtility::JobEntry {
    JobHandle handle;
    uint32_t level;
    inline bool operator <  (const JobEntry& o){return this->level <  o.level;}
    inline bool operator >  (const JobEntry& o){return this->level >  o.level;}
    inline bool operator == (const JobEntry& o){return this->level == o.level;}
    inline bool operator <= (const JobEntry& o){return this->level <= o.level;}
    inline bool operator >= (const JobEntry& o){return this->level >= o.level;}
};
struct ECS::JobsUtility::JobData {
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

ECS::JobHandle ECS::JobsUtility::schedule(const JobParameter& data){
    if(this->writeIndex >= this->capacity)
        this->resizeJobPool(this->capacity * 2);
    uint32_t index = this->writeIndex;
    if(data.function == NULL || data.batchStepSize < 1  || data.batchCount < 1)
        throw std::invalid_argument("schedule()");
    if(index > JobHandle::MaximumCount)
        throw std::runtime_error("schedule(): ThreadPool is full");
    if(data.dependsOn.index() >= (int32_t)index)
        throw std::runtime_error("schedule(): invalid dependantOn job handle");
    {
        JobData &job = this->jobs[index];
        new (&job) JobData();
        job.function = data.function;
        job.context = data.context;
        job.batchCount = data.batchCount;
        job.batchStepSize = data.batchStepSize;
        if(data.dependsOn.index() >= 0)
            job.level = this->jobs[data.dependsOn.index()].level + 1;
    }
    this->writeIndex++;
    return JobHandle((int32_t)index);
}
void ECS::JobsUtility::prepareJobs(){
    JobEntry *bufferPtr = this->buffer;
    uint32_t count = this->writeIndex;
    JobData *jobsPtr = this->jobs.get();
    if(count < 1)
        return;
    memset(this->beginIndex, 0, sizeof(std::atomic<uint32_t>)*count);
    for(uint32_t i=0;i<count;i++)
        bufferPtr[i] = JobEntry{JobHandle(i), jobsPtr[i].level};
    std::sort(bufferPtr,bufferPtr+count);
    for(uint32_t i=0;i<count;i++)
        this->jobsArray[i] = bufferPtr[i].handle;
}
ECS::JobHandle ECS::JobsUtility::combineDependencies(const_span<JobHandle> jobs){
    JobHandle max = JobHandle();
    uint32_t maxLevel = 0;
    for(const JobHandle &j:jobs){
        if(j.index() < 0)
            continue;
        if((uint32_t)j.index() > this->writeIndex)
            throw std::invalid_argument("combineDependencies(): array contains invalid JobHandle(s)");
        const uint32_t level = this->jobs[j.index()].level;
        if(level > maxLevel){
            max = j;
            maxLevel = level;
        }
    }
    return max;
}
void ECS::JobsUtility::resizeJobPool(uint32_t newcapacity){
    if(this->capacity >= newcapacity)
        return;//throw std::invalid_argument("resizeJobPool(): can't resize to smaller array");
    uint32_t size_temp[4];
    size_temp[0] =                (uint32_t)sizeof(JobData)               * newcapacity;
    size_temp[1] = size_temp[0] + (uint32_t)sizeof(std::atomic<uint32_t>) * newcapacity;
    size_temp[2] = size_temp[1] + (uint32_t)sizeof(JobHandle)             * newcapacity;
    size_temp[3] = size_temp[2] + (uint32_t)sizeof(JobEntry)              * newcapacity;
    align_ptr<uint8_t> ptr2{allocator().allocate(size_temp[3])};
    if(this->jobs.get())
        memcpy(ptr2.get(), this->jobs.get(), sizeof(JobData)*this->writeIndex);
    this->capacity = newcapacity;
    this->jobs.reset((JobData*)ptr2.get());
    this->beginIndex = (std::atomic<uint32_t>*)  ((uint8_t*)ptr2.get() + size_temp[0]);
    this->jobsArray  = (JobHandle*)              ((uint8_t*)ptr2.get() + size_temp[1]);
    this->buffer     = (JobEntry*)               ((uint8_t*)ptr2.get() + size_temp[2]);
    ptr2.release();
}









void ECS::JobsUtility::init()
{
    {
        uint32_t size[3];
        size[0] = alignCacheLineSize(sizeof(uv_work_t)*uv_num_worker_threads());
        size[1] = size[0] + alignCacheLineSize(sizeof(uv_timer_t));
        size[2] = size[1] + alignCacheLineSize(sizeof(uv_async_t));
        this->works      = make_align<uv_work_t[]>(size[2]);
        this->fixedTimer = (uv_timer_t*)((uint8_t*)works.get() + size[0]);
        this->wakecall   = (uv_async_t*)((uint8_t*)works.get() + size[1]);

        for (uint32_t i = 0; i < uv_num_worker_threads(); i++)
            this->works[i].data = this;
        this->fixedTimer->data = this;
        this->wakecall->data = this;
    }
    this->scheduleQueue.reserve(Constants::InitialJobPoolCapacity);
    std::vector<ISystem* (*)(DOE &)> &list = _get_initialize_list();
    auto &sysList = sharedEngine->sys;
    sysList.reserve(list.size());
    for(auto func:list){
        ISystem *sys;
        try
        {
            sys = func(*sharedEngine.get());
        } catch(const std::exception& e) {
            printf("caught std::exception initializing a system: %s\n",e.what());
            continue;
        }
        sysList.emplace_back(sys);
    }
    sharedEngine->fixedTimeBuffer = sharedEngine->updateTimeBuffer = uv_hrtime();
    uv_timer_init(uv_default_loop(), this->fixedTimer);
    uv_async_init(uv_default_loop(), this->wakecall, wakeThread);
    uv_timer_start(this->fixedTimer, on_fixed_timer, 0, 20);
}
void ECS::JobsUtility::signalQuit(){
    this->bitmask |= Request::Exit;
    uv_async_send(this->wakecall);
}
void ECS::JobsUtility::signalRender(){
    this->bitmask |= Request::Render;
    uv_async_send(this->wakecall);
}








#pragma region Libuv callbacks

void ECS::JobsUtility::on_fixed_timer(uv_timer_t *arg) {
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    thiz->bitmask |= Request::Timer;
    uv_async_send(thiz->wakecall);
}
void ECS::JobsUtility::wakeThread(uv_async_t *arg){
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    uint32_t expected = 0;
    if(thiz->activeThreads.compare_exchange_weak(expected,1)){
        if(thiz->writeIndex <= thiz->readIndex.load()){
            iterate_systems(thiz);
        }else{
            queue_jobs(nullptr,0);
        }
    }
}
void ECS::JobsUtility::iterate_systems(uv__work *w,int) {
    uv_work_t *arg = (uv_work_t*)(((uint8_t*)w)-offsetof(uv_work_t,work_req));
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    unsigned int *count = &w->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    iterate_systems(thiz);
}
void ECS::JobsUtility::iterate_systems(ECS::JobsUtility *thiz){
    again:;
    {
        std::unique_ptr<ECS::ISystem> *begin =         ECS::sharedEngine->sys.data();
        std::unique_ptr<ECS::ISystem> *end   = begin + ECS::sharedEngine->sys.size();
        if(unlikely(thiz->bitmask & Request::Exit)) {
            uv_timer_stop(thiz->fixedTimer);
            uv_unref((uv_handle_t*)&thiz->wakecall);
            uv_stop(uv_default_loop());
            while (begin != end){
                try {
                    (*begin)->OnDestroy(*ECS::sharedEngine);
                } catch(const std::exception& e) {
                    printf("caught std::exception OnDestroy: %s\n",e.what());
                }
                begin++;
            }
            return;
        } else if(thiz->bitmask & Request::Timer) {
            {
                uint64_t realtime = uv_hrtime();
                ECS::sharedEngine->fixedDelta      = (float)(realtime - ECS::sharedEngine->fixedTimeBuffer);
                ECS::sharedEngine->fixedTimeBuffer = realtime;
            }
            while (begin != end){
                try {
                    (*begin)->OnFixedUpdate(*ECS::sharedEngine);
                } catch(const std::exception& e) {
                    printf("caught std::exception OnFixedUpdate: %s\n",e.what());
                    thiz->bitmask |= Request::Exit;
                    break;
                }
                begin++;
            }
            thiz->bitmask &= ~Request::Timer;
        } else if(thiz->bitmask & Request::Render) {
            {
                uint64_t realtime = uv_hrtime();
                ECS::sharedEngine->updateDelta      = (float)(realtime - ECS::sharedEngine->updateTimeBuffer) / 1.0e9;
                ECS::sharedEngine->updateTimeBuffer = realtime;
            }
            ECS::graphics->beginFrame();
            while (begin != end){
                try {
                    (*begin)->OnUpdate(*ECS::sharedEngine);
                } catch(const std::exception& e) {
                    printf("caught std::exception OnUpdate: %s\n",e.what());
                    thiz->bitmask |= Request::Exit;
                    break;
                }
                begin++;
            }
            ECS::graphics->endFrame();
            thiz->bitmask &= ~Request::Render;
        } else {
            thiz->activeThreads--;
            return;
        }
    }
    ECS::sharedEngine->eqm.updateNewArchetypes(ECS::sharedEngine->ecs);
    ECS::sharedEngine->ecs.cleanChangeList();
    if(!thiz->scheduleQueue.empty())
    {
        thiz->resizeJobPool((uint32_t)thiz->scheduleQueue.size());
        thiz->works[0].data = NULL;
        thiz->works[0].work_req.loop = uv_default_loop();
        thiz->works[0].work_req.done = &queue_jobs;
        thiz->works[0].work_req.work = &queue_jobs;
        uv_queue_work_slow(thiz->works);
    }else{
        if(thiz->bitmask.load())
            goto again;
        else
            thiz->activeThreads--;
    }
}
void ECS::JobsUtility::queue_jobs(uv__work *w,int){
    uv_work_t *arg = (uv_work_t*)(((uint8_t*)w)-offsetof(uv_work_t,work_req));
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    if(w){
        unsigned int *count = &w->loop->active_reqs.count;
        if(*count <= 0)
            throw std::runtime_error("");
        (*count)--;
    }

    uint32_t expected = 1;
    const uint32_t numWorkerThread = uv_num_worker_threads();
    if(unlikely(!thiz->activeThreads.compare_exchange_weak(expected,numWorkerThread)))
        throw std::runtime_error("queue_jobs(): thread internal error");
    uv_work_t *begin = thiz->works;
    uv_work_t *end   = thiz->works + numWorkerThread;
    begin = thiz->works;
    while(begin < end){
        begin->work_req.work = &work_jobs;
        uv_queue_work_slow(begin);
        begin++;
    }
}
void ECS::JobsUtility::queue_jobs(uv__work *w){
    uv_work_t *arg = (uv_work_t*)(((uint8_t*)w)-offsetof(uv_work_t,work_req));
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    thiz->writeIndex = 0;
    thiz->readIndex = 0;
    thiz->readerLevel = 0;
    thiz->dpm.clear();
    for(Schedule &sch: thiz->scheduleQueue){
        if(sch.parallel)
            sch.jw->scheduleParallel(sch.qb,thiz->dpm);
        else
            sch.jw->schedule(sch.qb,thiz->dpm);
    }
    thiz->scheduleQueue.clear();
    if(thiz->writeIndex != 0){
        thiz->prepareJobs();
        const uint32_t numWorkerThread = uv_num_worker_threads();
        if(unlikely(thiz->activeThreads.load() != 1))
            throw std::runtime_error("queue_jobs(): thread internal error");

        uv_work_t *begin = thiz->works;
        uv_work_t *end   = thiz->works + numWorkerThread;
        while(begin < end) {
            begin->data = thiz;
            begin->work_req.loop = uv_default_loop();
            begin->work_req.done = &after_work_jobs;
            begin++;
        }
    }else
        w->done = &iterate_systems;
}
void ECS::JobsUtility::work_jobs(uv__work *w)
{
    uv_work_t* arg = (uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req));
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    while (true)
    {
        uint32_t readIndex = thiz->readIndex.load();
        // no more job
        if(unlikely(thiz->writeIndex <= readIndex))
            return;

        JobHandle job = thiz->jobsArray[readIndex];
        std::atomic<uint32_t> &beginIndexPtr = thiz->beginIndex[job.index()];
        JobData jobData;
        memcpy(&jobData, thiz->jobs.get() + job.index(), sizeof(JobData));

        // dependency check
        if(jobData.level > thiz->readerLevel)
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
        thiz->readIndex.compare_exchange_weak(readIndex,readIndex+1);
    }
}
void ECS::JobsUtility::after_work_jobs(uv__work *w,int status)
{
    uv_work_t* arg = (uv_work_t *) ((uint8_t*)(w) - offsetof(uv_work_t, work_req));
    ECS::JobsUtility *thiz = ((ECS::JobsUtility*)(arg->data));
    unsigned int *count = &w->loop->active_reqs.count;
    if(*count <= 0)
        throw std::runtime_error("");
    (*count)--;
    if(status)
        thiz->signalQuit();
    if(thiz->activeThreads.fetch_sub(1) == 1){
        wakeThread(nullptr);
    }
}

#pragma endregion Libuv callbacks
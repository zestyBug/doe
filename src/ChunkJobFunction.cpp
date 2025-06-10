#include "cutil/basics.hpp"
#include "ECS/ChunkJobFunction.hpp"
#include "ECS/DependencyManager.hpp"
#include "ECS/Archetype.hpp"
#include <atomic>
#include <thread>
#include "ECS/ThreadPool.hpp"

struct thread_info {
    ECS::ChunkJobContext  *job=nullptr;
    ECS::ArchetypeHolder  *archetypes=nullptr;
    uint32_t jobCount=0;
    uint32_t archetypeCount=0;
    std::atomic<uint32_t> *jobDependencyCounterBuffer=nullptr;
    std::atomic<uint32_t> *jobIndexQueue=nullptr;
    ECS::version_t globalSystemVersion=0;
    std::atomic<uint32_t> jobQueueRead = 0;
    std::atomic<uint32_t> jobQueueWrite = 0;
    char padding[12];
};
static_assert(sizeof(thread_info)==64);

void* ECS::ChunkJobFunction::createContext(span<ECS::ChunkJobContext> jobs,span<ECS::ArchetypeHolder> archs, ECS::version_t globalVersion)
{
    size_t size = alignTo64(sizeof(std::atomic<uint32_t>),jobs.size());
    thread_info* context = (thread_info *) allocator().allocate(sizeof(thread_info) + size * 2);
    context->job = jobs.data();
    context->archetypes = archs.data();
    context->jobDependencyCounterBuffer = (std::atomic<uint32_t>*)((uint8_t*)context + sizeof(thread_info));
    context->jobIndexQueue = (std::atomic<uint32_t>*)((uint8_t*)context + size + sizeof(thread_info));
    context->jobCount = jobs.size();
    context->archetypeCount = archs.size();
    context->globalSystemVersion = globalVersion;
    context->jobQueueRead = 0;
    context->jobQueueWrite = 0;
    for (size_t i = 0; i < jobs.size(); i++)
        new (context->jobDependencyCounterBuffer+i) std::atomic<uint32_t>(0);
    for (size_t i = 0; i < jobs.size(); i++)
        new (context->jobIndexQueue+i) std::atomic<uint32_t>(0);

    uint32_t counter = 0;
    for(uint32_t i=0;i<jobs.size();i++)
        if(jobs[i].dependencyCount == 0)
            context->jobIndexQueue[counter++].store(i,std::memory_order_relaxed);
    context->jobQueueWrite.store(counter,std::memory_order_relaxed);

    return context;
}

size_t ECS::ChunkJobFunction::function(void* _context, size_t i) noexcept{
    if(!_context)
        return ThreadPool::STOP_SIGNAL;
    thread_info &context = *(thread_info*)_context;

    const uint32_t totalSize = context.jobCount;
    uint32_t readInedex  = context.jobQueueRead.fetch_add(1, std::memory_order_relaxed);
    if(readInedex < totalSize)
    {
        while(readInedex >= context.jobQueueWrite.load(std::memory_order_relaxed))
            std::this_thread::yield();
        ChunkJobContext* job = context.job + context.jobIndexQueue[readInedex].load(std::memory_order_relaxed);
        proccess(
            job,
            context.archetypes,
            context.archetypeCount,
            job->lastVersion
        );
        for(uint32_t ji:job->precedesIndex){
            const uint32_t count = context.jobDependencyCounterBuffer[ji].fetch_add(1,std::memory_order_relaxed);
            // idependencyCount supposed to be non 0 if we are here
            if((count+1) == context.job[ji].dependencyCount){
                const uint32_t wi = context.jobQueueWrite.fetch_add(1,std::memory_order_relaxed);
                context.jobIndexQueue[wi].store(ji);
            }
        }
        return i+1;
    }else
        return ThreadPool::STOP_SIGNAL;
}

void ECS::ChunkJobFunction::destroyContext(void* context){
    allocator().deallocate(context);
}

void ECS::ChunkJobFunction::proccess(
    ECS::ChunkJobContext* job,
    ECS::ArchetypeHolder* archetypes, 
    uint32_t archetypeCount, 
    ECS::version_t v
){
    for (uint32_t archetypeIndex = 0; archetypeIndex < archetypeCount; archetypeIndex++)
    {
        Archetype *arch = archetypes[archetypeIndex].get();
        if(arch)
        {
            {
                uint32_t offset=0;
                JobFilter filter = job->context->getFilter();
                for (size_t i = 0; i < 3; i++)
                {
                    if(!arch->hasComponents({(TypeID*)(filter.types + offset) ,filter.counts[i]}))
                        goto skip;
                    offset+= filter.counts[i];
                }
            }
            callExecution(job,arch,v);
            skip:;
        }
    }
}

void ECS::ChunkJobFunction::callExecution(ECS::ChunkJobContext* job,ECS::Archetype* archetype,ECS::version_t version){
    JobFilter filter = job->context->getFilter();
    const uint32_t argCount = filter.counts[0]+filter.counts[1];
    const uint32_t totalCount = argCount + filter.counts[2];
    const uint32_t chunkCount=(uint32_t) archetype->chunksData.size();
    uint32_t i;
    version_t v;
    uint8_t* buffer;
    const uint32_t countBuffer[2] = {archetype->chunkCapacity,archetype->lastChunkEntityCount};
    void* argsBuffer[argCount];
    size_t argsOffsets[argCount];
    uint16_t indecies[totalCount];

    if(filter.counts[0])
        if(!archetype->getIndecies({(TypeID*)(filter.types),filter.counts[0]},indecies))
            throw std::runtime_error("callExecution(): invalid archetype");
    if(filter.counts[1])
        if(!archetype->getIndecies({(TypeID*)(filter.types + filter.counts[0]),filter.counts[1]},indecies))
            throw std::runtime_error("callExecution(): invalid archetype");
    if(filter.counts[2])
        if(!archetype->getIndecies({(TypeID*)(filter.types + argCount),filter.counts[2]},indecies))
            throw std::runtime_error("callExecution(): invalid archetype");
    
    for (i = 0; i < argCount; i++)
        argsOffsets[i] = archetype->offsets[indecies[i]];

    for(uint32_t chunkIndex=0;chunkIndex<chunkCount;chunkIndex++){
        // check for type version one by one
        for (i = 0; i < filter.counts[2]; i++) {
            v = archetype->chunksVersion.getChangeVersion(indecies[argCount+i],chunkIndex);
            if(!didChange(version,v))
                goto endChunk;
        }
        buffer = (uint8_t*)(archetype->chunksData[chunkIndex].memory);
        for (i = 0; i < argCount; i++)
            argsBuffer[i] = buffer + argsOffsets[i];
        try{
            job->context->execute(
                {argsBuffer,argCount},
                countBuffer[(chunkIndex+1) == chunkCount]
            );
        }catch(const std::exception &e){
            printf("exception while executing %s: %s\n",job->context->name(),e.what());
            break;
        }

        for (i = 0; i < filter.counts[1]; i++)
            archetype->chunksVersion.getChangeVersion(indecies[filter.counts[0]+i],chunkIndex) = version;
        endChunk:;
    }
}
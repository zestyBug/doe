#include "ECS/ThreadPool.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "ECS/Archetype.hpp"

using namespace ECS;

void ThreadPool::init(SystemState& state,EntityComponentManager &ecm){
    std::unique_lock lock(this->gmutex);
    this->thread_info.jobStatues.resize(state.jobs.registeredJobs.size());
    this->thread_info.jobs = state.jobs.registeredJobs;
    this->thread_info.archetypes = ecm.archetypes;
    this->thread_info.job_index = 0;
    this->thread_info.globalSystemVersion = state.globalSystemVersion;
}

bool ThreadPool::needWait(uint32_t jobIndex){
    jobContext *const job = &this->thread_info.jobs[jobIndex];
    for (JobHandle jIndex : job->deps)
        if(jIndex > 0)
        {
            --jIndex;
            IterationIndex buffer = this->thread_info.jobStatues.at(jIndex);
            if(buffer.archetypeIndex > buffer.archetypeProcessed)
                return true;
        }
    return false;
}

IterationIndex ThreadPool::findSuitableArchetype(IterationIndex indexing,uint32_t jobIndex){
    jobContext *const job = &this->thread_info.jobs.at(jobIndex);
    // search for a suitable archetype
    while(indexing.archetypeIndex < this->thread_info.archetypes.size())
    {
        Archetype *arch = this->thread_info.archetypes[indexing.archetypeIndex].get();
        // update it here, in case that may mbe returned
        if(arch)
        {
            uint32_t offset=0;
            TypeID *const ptr    = job->types.data();
            const uint32_t count = (uint32_t)job->types.size();
            for (size_t i = 0; i < 3; i++)
            {
                // DEBUG
                if((offset+job->counts[i]) > count)
                    throw std::runtime_error("findSuitableArchetype(): invalid job query");
                if(!arch->hasComponents({ptr+offset ,job->counts[i]}))
                    goto notFound;
                offset+=job->counts[i];
            }
            return indexing;
            notFound:;
        }
        indexing.archetypeIndex++;
        indexing.archetypeProcessed++;
    }
    return indexing;
}

void ThreadPool::callExecution(uint32_t jobIndex,uint32_t archIndex)
{
    jobContext * job = &this->thread_info.jobs.at(jobIndex);
    Archetype *arch = this->thread_info.archetypes.at(archIndex).get();
    const uint32_t argCount = job->counts[0]+job->counts[1];
    const uint32_t totalCount = argCount+job->counts[2]+job->counts[3];
    
    if(job->types.size() != totalCount)
        throw std::runtime_error("callExecution(): internal error");
    
    uint32_t chunkIndex=0;
    uint32_t chunkCount=(uint32_t)arch->chunksData.size();
    uint16_t indecies[totalCount];
    void* argsBuffer[argCount];

    if(totalCount)
        arch->getIndecies(job->types,indecies);

    for(;chunkIndex < chunkCount;chunkIndex++)
    {
        for (uint32_t i = 0; i < job->counts[2]; i++) {
            const version_t v = arch->chunksVersion.getChangeVersion(indecies[argCount+i],chunkIndex);
            if(!didChange(job->lastSystemVersion,v))
                goto endChunk;
        }
        
        for (uint32_t i = 0; i < argCount; i++)
            argsBuffer[i] = (uint8_t*)(arch->chunksData[chunkIndex].memory) + arch->offsets[indecies[i]];
        
        job->jobFunc(
            job->context,
            {argsBuffer,argCount},
            (chunkIndex+1) == chunkCount ? arch->lastChunkEntityCount : arch->chunkCapacity
        );

        for (uint32_t i = 0; i < job->counts[1]; i++) {
            arch->chunksVersion.getChangeVersion(indecies[job->counts[0]+i],chunkIndex) 
                = this->thread_info.globalSystemVersion;
        }
        endChunk:;
    }
}


void ThreadPool::func(){while(true){
    // for buffering
    uint32_t jobIndex;
    // for buffering
    IterationIndex indexing;
    if(this->destroy)
        return;
    {
        std::unique_lock lock(this->gmutex);
        jobIndex = this->thread_info.job_index;
        // if there is any job
        if(jobIndex < this->thread_info.jobStatues.size()){
            indexing = this->thread_info.jobStatues[jobIndex];
            // if job dependencies are meet
            if(!this->needWait(jobIndex))
            {
                // may som thread are waiting for this job
                this->barrier.notify_all();

                indexing = findSuitableArchetype(indexing,jobIndex);
                
                if(indexing.archetypeIndex < this->thread_info.archetypes.size()){
                    // update new index values
                    const auto buffer = indexing.archetypeIndex++;
                    this->thread_info.jobStatues[jobIndex] = indexing;
                    indexing.archetypeIndex = buffer;
                    goto bEnd;
                }else{
                    // simply discard current job
                    this->thread_info.job_index = jobIndex + 1;
                    continue;
                }
            }
        }
        if(!this->destroy)
            this->barrier.wait(lock);
        continue;
        bEnd:;
    }
    {
        callExecution(jobIndex,indexing.archetypeIndex);
    }
    {
        std::unique_lock lock(this->gmutex);
        this->thread_info.jobStatues[jobIndex].archetypeProcessed++;
    }
}}
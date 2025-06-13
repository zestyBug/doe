#include "ECS/DependencyManager.hpp"
#include "cutil/set.hpp"
#include "cutil/range.hpp"
using namespace ECS;

void DependencyManager::dummyExecute(){
    for(auto&j:registeredJobs){
    #ifdef VERBOSE
        printf("Job: \"%s\"\n",j.context->name());
    #endif
        j.context->execute(span<void*>{},0);
    }
}

ChunkJobHandle DependencyManager::ScheduleJob(
    ChunkJob *context,
    const_span<ChunkJobHandle> jobDependency,
    version_t lastSystemVersion
){
    auto& job = registeredJobs.emplace_back();
    
    ChunkJobHandle jobID = (uint32_t)registeredJobs.size();
    if(jobID >= MaxJobCount)
        throw std::runtime_error("ScheduleJob(): maximum job count limit reached");

    /* 2) list dependencies */

    JobFilter filter = context->getFilter();

    vector_set<ChunkJobHandle> depsBuffer{16};

    uint32_t index=0;
    for (const uint32_t _ : range(filter.counts[0])) {
        ChunkJobHandle writeJob = lastWriteJob.at(filter.types[index++].realIndex());
        if (writeJob > 0)
            depsBuffer.insert(writeJob);
    }
    for (const uint32_t _ : range(filter.counts[1])) {
        const uint16_t realIndex = filter.types[index++].realIndex();
        ChunkJobHandle writeJob = lastWriteJob.at(realIndex);
        const auto& readJobs = lastReadJobs[realIndex];
        index++;
        if (writeJob > 0)
            depsBuffer.insert(writeJob);
        else for(const ChunkJobHandle j:readJobs)
            if (j > 0)
                depsBuffer.insert(j);
    }
    for (const ChunkJobHandle j : jobDependency)
        depsBuffer.insert(j);
    
    uint32_t depCount = 0;
    for(const ChunkJobHandle jh : depsBuffer.raw())
        if(0 < jh && jh < jobID){
            depCount++;
            registeredJobs.at(jh-1).precedesIndex.push_back(jobID-1);
        }

    /* 3) update */

    index=0;
    for (const uint32_t _ : range(filter.counts[0])) {
        const uint16_t realIndex = filter.types[index++].realIndex();
        // if any write job was here, it 
        lastWriteJob.at(realIndex) = -1;
        lastReadJobs[realIndex].push_back(jobID);
    }
    for (const uint32_t _ : range(filter.counts[1])) {
        const uint16_t realIndex = filter.types[index++].realIndex();
        lastWriteJob.at(realIndex) = jobID;
        lastReadJobs[realIndex].clear(); // Clear readers after write
    }

    /* 4) meta data */

    job.dependencyCount = depCount;
    job.lastVersion = lastSystemVersion;
    job.context = context;

    return (ChunkJobHandle)jobID;
}





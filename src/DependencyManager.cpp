#include "ECS/DependencyManager.hpp"
using namespace ECS;

void DependencyManager::dummyExecute(){
    for(auto&j:registeredJobs){
        printf("Job: \"%s\", Dependency: %i\n");
        j.context->execute(span<void*>{},0);
    }
}

JobHandle DependencyManager::ScheduleJob(
    ChunkJob *context,
    span<TypeID> readTypes,
    span<TypeID> writeTypes,
    span<TypeID> changeFilter,
    span<JobHandle> jobDependency,
    version_t lastSystemVersion
){
    auto& job = registeredJobs.emplace_back();
    
    uint32_t jobID = (uint32_t)registeredJobs.size();
    if(jobID > MaxJobCount)
        throw std::runtime_error("ScheduleJob(): maximum job count limit reached");

    /* 2) list dependencies */

    vector_set<JobHandle> depsBuffer{16};
    
    for (const auto& type : readTypes) {
        JobHandle writeJob = lastWriteJob.at(type.realIndex());
        if (writeJob > 0)
            depsBuffer.insert(writeJob);
    }
    for (const auto& type : writeTypes) {
        JobHandle writeJob = lastWriteJob.at(type.realIndex());
        const auto& readJobs = lastReadJobs[type.realIndex()];
        if (writeJob > 0)
            depsBuffer.insert(writeJob);
        else for(const JobHandle j:readJobs)
            if (j > 0)
                depsBuffer.insert(j);
    }
    for (const JobHandle j : jobDependency)
        depsBuffer.insert(j);
    
    uint32_t depCount = 0;
    for(const JobHandle jh : depsBuffer.raw())
        if(jh > 0)
            if(!(0 < jh && jh < jobID)){
                depCount++;
                registeredJobs.at(jh-1).precedes.push_back(jobID);
            }

    /* 3) update */

    for (const auto& type : readTypes) {
        // if any write job was here, it 
        lastWriteJob.at(type.realIndex()) = -1;
        lastReadJobs[type.realIndex()].push_back(jobID);
    }
    for (const auto& type : writeTypes) {
        lastWriteJob.at(type.realIndex()) = jobID;
        lastReadJobs[type.realIndex()].clear(); // Clear readers after write
    }

    /* 4) meta data */

    job.counts[0] = readTypes.size();
    job.counts[1] = writeTypes.size();
    job.counts[2] = changeFilter.size();
    
    job.types = (TypeID*) malloc(job.counts[0]+job.counts[1]+job.counts[2]);
    memcpy(
        job.types,
        readTypes.data(),
        job.counts[0]*sizeof(TypeID)
    );
    memcpy(
        job.types + job.counts[0],
        writeTypes.data(),
        job.counts[1]*sizeof(TypeID)
    );
    memcpy(
        job.types + job.counts[0] + job.counts[1],
        changeFilter.data(),
        job.counts[2]*sizeof(TypeID)
    );

    job.dependencyCount = depCount;
    job.lastVersion = lastSystemVersion;
    job.context = context;

    return (JobHandle)jobID;
}





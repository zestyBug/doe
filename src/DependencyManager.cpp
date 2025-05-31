#include "ECS/DependencyManager.hpp"
using namespace ECS;

JobHandle DependencyManager::ScheduleJob(
    ChunkJob *context,
    ChunkJob::JobFunc jobFunc,
    const std::string& name,
    span<TypeID> readTypes,
    span<TypeID> writeTypes,
    span<TypeID> changeFilter,
    span<JobHandle> jobDependency,
    version_t lastSystemVersion
){
    {
        ssize_t jobID = (ssize_t)registeredJobs.size();
        for (JobHandle j : jobDependency)
            if(!(0 < j && j <= jobID))
                throw std::invalid_argument("ScheduleJob(): invalid dependencies");
    }
    auto& job = registeredJobs.emplace_back();
    
    uint32_t jobID = (uint32_t)registeredJobs.size();
    if(jobID > MaxJobCount)
        throw std::runtime_error("ScheduleJob(): maximum job count limit reached");

    /* 2) list dependencies */
    
    for (const auto& type : readTypes) {
        JobHandle writeJob = lastWriteJob.at(type.realIndex());
        if (writeJob > 0)
            job.deps.insert(writeJob);
    }
    for (const auto& type : writeTypes) {
        JobHandle writeJob = lastWriteJob.at(type.realIndex());
        const auto& readJobs = lastReadJobs[type.realIndex()];
        if (writeJob > 0)
            job.deps.insert(writeJob);
        else for(const JobHandle j:readJobs)
            if (j > 0)
                job.deps.insert(j);
    }
    for (const JobHandle j : jobDependency)
        job.deps.insert(j);

    /* 3) update */

    for (const auto& type : readTypes) {
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
    job.counts[3] = 0;
    
    job.types.resize(job.counts[0]+job.counts[1]+job.counts[2]+job.counts[3]);
    memcpy(
        job.types.data(),
        readTypes.data(),
        job.counts[0]*sizeof(TypeID)
    );
    memcpy(
        job.types.data() + job.counts[0],
        writeTypes.data(),
        job.counts[1]*sizeof(TypeID)
    );
    memcpy(
        job.types.data() + job.counts[0] + job.counts[1],
        changeFilter.data(),
        job.counts[2]*sizeof(TypeID)
    );

    job.context = context;
    job.jobFunc = jobFunc;
    memcpy(job.name,name.c_str(),std::min<size_t>(15,name.size()));
    job.name[15] = '\0';
    job.lastSystemVersion = lastSystemVersion;

    return (JobHandle)jobID;
}





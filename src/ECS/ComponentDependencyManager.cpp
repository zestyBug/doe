#include "ECS/ComponentDependencyManager.hpp"
#include "ECS/Base/ArchetypeQuery.hpp"
#include "ECS/ThreadPool.hpp"
#include "cutil/set.hpp"
#include "cutil/range.hpp"
using namespace ECS;

#ifndef ENABLE_SIMPLE_SYSTEM_DEPENDENCIES

JobHandle ComponentDependencyManager::getRO(uint32_t index){
    return jobRW[index];
}
JobHandle ComponentDependencyManager::getRW(uint32_t index){
    JobHandle temp = JobHandle();
    if(jobsROCount[index] > 0){
        return threadPool->combineDependencies({jobsRO[index],jobsROCount[index]});
    }else{
        return jobRW[index];
    }
}
void ComponentDependencyManager::insertRO(JobHandle job,uint32_t index){
    jobsRO[index][jobsROCount[index]++] = job;
    if(jobsROCount[index] >= MaximumReaderPerType){
        jobRW[index] = threadPool->combineDependencies({jobsRO[index],jobsROCount[index]});
        jobsROCount[index] = 0;
    }
}
void ComponentDependencyManager::insertRW(JobHandle job,uint32_t index){
    jobRW[index] = job;
    jobsROCount[index] = 0;
}
JobHandle ComponentDependencyManager::getDependency(ArchetypeQuery &query)
{
    const uint32_t counter = query.count[1] + query.count[2];
    if(counter < 1)
        return JobHandle();
    JobHandle dependancies[counter];
    uint32_t dependanciesIndex = 0;

    for (uint32_t i = 0; i < query.count[1]; i++){
        uint16_t type = query._all[i];
        if(type & ArchetypeQuery::WriteFlag)
            dependancies[dependanciesIndex++] = getRW(type & ArchetypeQuery::IndexMask);
        else
            dependancies[dependanciesIndex++] = getRO(type & ArchetypeQuery::IndexMask);
    }
    for (uint32_t i = 0; i < query.count[2]; i++){
        uint16_t type = query._any[i];
        if(type & ArchetypeQuery::WriteFlag)
            dependancies[dependanciesIndex++] = getRW(type & ArchetypeQuery::IndexMask);
        else
            dependancies[dependanciesIndex++] = getRO(type & ArchetypeQuery::IndexMask);
    }
    threadPool->combineDependencies({dependancies, dependanciesIndex});
}
void ComponentDependencyManager::addDependency(JobHandle job, ArchetypeQuery &query)
{
    const uint32_t counter = query.count[1] + query.count[2];
    if(counter < 1)
        return;

    for (uint32_t i = 0; i < query.count[1]; i++){
        uint16_t type = query._all[i];
        if(type & ArchetypeQuery::WriteFlag)
            insertRW(job, type & ArchetypeQuery::IndexMask);
        else
            insertRO(job, type & ArchetypeQuery::IndexMask);
    }
    for (uint32_t i = 0; i < query.count[2]; i++){
        uint16_t type = query._any[i];
        if(type & ArchetypeQuery::WriteFlag)
            insertRW(job, type & ArchetypeQuery::IndexMask);
        else
            insertRO(job, type & ArchetypeQuery::IndexMask);
    }
}
void ComponentDependencyManager::clear(){
    for (uint32_t i = 0; i < TypeID::MaximumTypesCount; i++)
        this->jobsROCount[i] = 0;
    for (uint32_t i = 0; i < TypeID::MaximumTypesCount; i++)
        this->jobRW[i] = JobHandle();
}

#endif
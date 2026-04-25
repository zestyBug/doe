#if !defined(DEPENDENCYMANAGER_HPP)
#define DEPENDENCYMANAGER_HPP

#include "cutil/basics.hpp"
#include "Base/TypeID.hpp"
#include "Base/Version.hpp"
#include "Base/Job.hpp"

namespace ECS
{
struct ThreadPool;
struct ArchetypeQuery;
#define ENABLE_SIMPLE_SYSTEM_DEPENDENCIES 1
#ifdef ENABLE_SIMPLE_SYSTEM_DEPENDENCIES
struct ComponentDependencyManager {
    ThreadPool *threadPool;
    JobHandle dependency;

    static constexpr int32_t MaxJobCount = 0xFFFFF;

    ComponentDependencyManager(ThreadPool *tp):threadPool{tp}{}
    ComponentDependencyManager(const ComponentDependencyManager&) = default;
    void clear(){
        dependency = JobHandle();
    }
    JobHandle getDependency(ArchetypeQuery &){return dependency;}
    // Schedules a job with dependencies
    void addDependency(JobHandle job,ArchetypeQuery &){dependency = job;}
private:
};
}
#else
/// @brief job schedule buffer
struct ComponentDependencyManager {
    ThreadPool *threadPool;
    static const uint32_t MaximumReaderPerType = 16;
    std::array<JobHandle                      ,TypeID::MaximumTypesCount> jobRW;
    std::array<uint32_t                       ,TypeID::MaximumTypesCount> jobsROCount;
    std::array<JobHandle[MaximumReaderPerType],TypeID::MaximumTypesCount> jobsRO;

    static constexpr int32_t MaxJobCount = 0xFFFFF;

    ComponentDependencyManager(ThreadPool *tp):threadPool{tp}{}
    ComponentDependencyManager(const ComponentDependencyManager&) = default;
    void clear();
    JobHandle getDependency(
        ArchetypeQuery &query
    );
    // Schedules a job with dependencies
    void addDependency(
        JobHandle job,
        ArchetypeQuery &query
    );
private:
    void insertRO(JobHandle job,uint32_t index);
    void insertRW(JobHandle job,uint32_t index);
    JobHandle getRO(uint32_t index);
    JobHandle getRW(uint32_t index);
};
}
#endif

#endif // DEPENDENCYMANAGER_HPP

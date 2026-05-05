#if !defined(DEPENDENCYMANAGER_HPP)
#define DEPENDENCYMANAGER_HPP

#include <array>
#include "cutil/basics.hpp"
#include "Base/Version.hpp"
#include "Base/TypeID.hpp"
#include "Base/Job.hpp"
#include "Base/Constants.hpp"

namespace ECS
{
struct EntityQueryData;

#if !defined(ENABLE_SIMPLE_SYSTEM_DEPENDENCIES)
#define ENABLE_SIMPLE_SYSTEM_DEPENDENCIES 0
#endif

#if ENABLE_SIMPLE_SYSTEM_DEPENDENCIES
struct ComponentDependencyManager {
    JobHandle dependency;struct DependencyHandle
        {
            public JobHandle WriteFence;
            public int       NumReadFences;
            public TypeIndex TypeIndex;
        }

    ComponentDependencyManager() = default;
    ComponentDependencyManager(const ComponentDependencyManager&) = default;
    void clear(){
        dependency = JobHandle();
    }
    JobHandle getDependency(const EntityQueryData &){return dependency;}
    // Schedules a job with dependencies
    void addDependency(JobHandle job,const EntityQueryData &){dependency = job;}
private:
};
}
#else
/// @brief job schedule buffer
struct ComponentDependencyManager {
    struct DependencyHandle
    {
        JobHandle writeFence;
        uint32_t  numReadFences;
        TypeID    type;
    };
    static const uint32_t MaximumReadJobHandle = 16;
    static const uint16_t NullTypeIndex = 0xFFFF;
    uint32_t                                                                 dependencyHandlesCount = 0;
    std::array<DependencyHandle               ,Constants::MaximumTypesCount> dependencyHandles;
    /// @brief Indexed by TypeID, contains index of that TypeID in the dependencyHandles and readJobFences arrays.
    std::array<uint16_t                       ,Constants::MaximumTypesCount> typeArrayIndices;
    std::array<JobHandle[MaximumReadJobHandle],Constants::MaximumTypesCount> readJobFences;

    ComponentDependencyManager() = default;
    ComponentDependencyManager(const ComponentDependencyManager&) = default;
    void clear();
    JobHandle getDependency(
        const EntityQueryData &query
    );
    // Schedules a job with dependencies
    JobHandle addDependency(
        JobHandle job,
        const EntityQueryData &query
    );
    JobHandle combineReadDependencies(uint32_t typeArrayIndex);
private:
    uint32_t getTypeArrayIndex(TypeID type);
};
}
#endif

#endif // DEPENDENCYMANAGER_HPP

#if !defined(DEPENDENCYMANAGER_HPP)
#define DEPENDENCYMANAGER_HPP

#include "cutil/basics.hpp"
#include "ECS/defs.hpp"
#include <vector>
#include "cutil/set.hpp"
#include "cutil/unique_ptr.hpp"

namespace ECS{
    
struct ChunkJob {
    typedef void(*JobFunc)(void*,span<void*>,uint32_t);
};

/// @brief Simulated JobHandle (like Unity ECS JobHandle), 
/// anything below 1 is ninvalid value
using JobHandle = int;
struct jobContext {
    vector_set<JobHandle> deps{16};
    std::vector<TypeID> types;
    // [0]: read, [1]: write, [2]: filter
    uint32_t counts[4];
    ChunkJob *context;
    ChunkJob::JobFunc jobFunc;
    char name[16];
    // system that pulled this job, usually must be globalSystemVersion-1
    version_t lastSystemVersion;
};

/// @brief job schedule buffer
struct DependencyManager {
    // if these numbers goes any higher, use std::map<TypeID,...> and std::set<JobHandle> instead

    std::vector<jobContext> registeredJobs{};
    std::array<JobHandle,TypeID::MaxTypeCount> lastWriteJob{};
    std::vector<JobHandle,allocator<JobHandle>> lastReadJobs[TypeID::MaxTypeCount];

    DependencyManager(){
        //
    };
    ~DependencyManager() = default;

    static constexpr size_t MaxJobCount = 0xFFFFF;


    void dummyExecute();

    // Schedules a job with dependencies
    JobHandle ScheduleJob(
        ChunkJob *context,
        ChunkJob::JobFunc jobFunc,
        const std::string& name = "",
        span<TypeID> readTypes = {},
        span<TypeID> writeTypes = {},
        span<TypeID> changeFilter = {},
        span<JobHandle> jobDependency = {},
        version_t lastSystemVersion = 0
    );
};
}

#endif // DEPENDENCYMANAGER_HPP

#if !defined(DEPENDENCYMANAGER_HPP)
#define DEPENDENCYMANAGER_HPP

#include "cutil/basics.hpp"
#include "ECS/defs.hpp"
#include <vector>
#include "cutil/unique_ptr.hpp"

namespace ECS
{

struct JobFilter {
    const TypeID *types;
    // read, write, changeFilter
    uint32_t counts[3];
};

struct ChunkJob {
    /// @brief this function is/must be called by job threads
    /// @param pointers pointers to requested types(read+write)
    /// @param count count of enities
    virtual void execute(span<void*> pointers,uint32_t count) = 0;
    virtual const char* name() = 0;
    virtual JobFilter getFilter() = 0;
};

/// @brief Simulated JobHandle (like Unity ECS JobHandle), 
/// anything below 1 is ninvalid value
using ChunkJobHandle = int;
struct ChunkJobContext {
    std::vector<uint32_t> precedesIndex{};
    ChunkJob *context = nullptr;
    uint32_t dependencyCount = 0;
    // system that pulled this job, usually must be globalSystemVersion-1
    version_t lastVersion = 0;
    //char padding[24];

    ChunkJobContext() = default;
    ChunkJobContext(const ChunkJobContext&) = delete;
    ChunkJobContext(ChunkJobContext&&) = default;
    ~ChunkJobContext() = default;
};
static_assert(sizeof(ChunkJobContext)==40);

/// @brief job schedule buffer
struct DependencyManager {
    // if these numbers goes any higher, use std::map<TypeID,...> and std::set<JobHandle> instead
    std::vector<ChunkJobContext,allocator<ChunkJobContext>> registeredJobs{};
    std::array<ChunkJobHandle,TypeID::MaxTypeCount> lastWriteJob{};
    std::vector<ChunkJobHandle,allocator<ChunkJobHandle>> lastReadJobs[TypeID::MaxTypeCount];

    DependencyManager(){
        //
    };
    ~DependencyManager() = default;

    static constexpr int32_t MaxJobCount = 0xFFFFF;

    void clear(){
        this->registeredJobs.clear();
        memset(lastWriteJob.data(),0,lastWriteJob.size()*sizeof(ChunkJobHandle));
        for(auto&rj :lastReadJobs){
            rj.clear();
        }
    }
    void dummyExecute();

    // Schedules a job with dependencies
    ChunkJobHandle ScheduleJob(
        ChunkJob *context,
        const_span<ChunkJobHandle> jobDependency = {},
        version_t lastSystemVersion = 0
    );
};
}

#endif // DEPENDENCYMANAGER_HPP

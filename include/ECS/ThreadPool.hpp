#if !defined(THREADPOOL_HPP)
#define THREADPOOL_HPP

#include "cutil/basics.hpp"
#include "cutil/span.hpp"
#include "Base/Job.hpp"
#include <atomic>
#include <vector>

namespace ECS
{
    struct JobDataChunk;
    struct JobsUtility final {
        static void init();
        /// @brief 
        /// @param context  
        /// @param func function returns zero if need to be executed again
        static JobHandle schedule(const JobParameter&);
        static JobHandle combineDependencies(const_span<JobHandle>);
    };
} // namespace ecs


#endif // THREADPOOL_HPP

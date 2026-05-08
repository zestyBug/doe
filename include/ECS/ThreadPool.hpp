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
    struct DOE;
    struct JobsUtility final {
        static inline std::vector<void*(*)(DOE*)>& _get_initialize_list() {
            static std::vector<void*(*)(DOE*)> tests;
            return tests;
        }
        template<typename S>
        struct SystemRegister {
            SystemRegister() {
                _get_initialize_list().emplace_back(&init);
            }
        private:
            static void* init(DOE *e){
                S *ptr = allocator<S>().allocate(1);
                new (ptr) S(e);
                return ptr;
            }
        };
        static void init();
        static void signalQuit();
        static void signalRender();
        /// @brief 
        /// @param context  
        /// @param func function returns zero if need to be executed again
        static JobHandle schedule(const JobParameter&);
        static JobHandle combineDependencies(const_span<JobHandle>);
    };
} // namespace ecs


#endif // THREADPOOL_HPP

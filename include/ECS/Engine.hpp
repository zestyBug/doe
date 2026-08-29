#if !defined(ENGINE_HPP)
#define ENGINE_HPP

#include "AssetsManager.hpp"
#include "ResourceManager.hpp"
#include "EntityComponentStore.hpp"
#include "ComponentDependencyManager.hpp"
#include "EntityQueryManager.hpp"
#include "Base/ISystem.hpp"

namespace ECS
{
    struct JobChunkWrapperBase;
    // A scheduled job reference.
    struct Schedule {
        JobChunkWrapperBase *jw;
        EntityQueryImpl qb;
        bool parallel;
    };
    struct DOE {
        /// @brief Entities & components are stored here
        EntityComponentStore ecs;
        /// @brief Resolve jobs component dependancies (Intenal)
        ComponentDependencyManager dpm;
        /// @brief Entiry query are stored here
        EntityQueryManager eqm;
        uint64_t fixedTimeBuffer = 0;
        uint64_t updateTimeBuffer = 0;
        double fixedDelta = 0;
        double updateDelta = 0;
        /// @brief List of systems (Intenal)
        std::vector<std::unique_ptr<ISystem>> sys;
        /// @brief Temporary list of scheduled jobs. filled by systems and are executed at the end of system iteration.
        std::vector<Schedule> scheduleQueue;
        /// @brief Manages loading resources from budles. An asset is a bundle of resources  (Intenal)
        AssetsManager am;
        /// @brief Manages initialization, refrence counting and destruction of multiple resource types
        ResourceManager rm;
        DOE(){
            sys.reserve(Constants::InitialSystemCapacity);
            scheduleQueue.reserve(Constants::InitialJobPoolCapacity);
        };
    };
    extern std::unique_ptr<DOE> sharedEngine;
} // namespace ECS


#endif // ENGINE_HPP

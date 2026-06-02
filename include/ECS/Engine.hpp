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
    struct Schedule {
        JobChunkWrapperBase *jw;
        EntityQueryImpl qb;
        bool parallel;
    };
    struct DOE {
        EntityComponentStore ecs;
        ComponentDependencyManager dpm;
        EntityQueryManager eqm{&ecs};
        std::vector<std::unique_ptr<ISystem>> sys;
        std::vector<Schedule> scheduleQueue;
        AssetsManager am;
        ResourceManager rm;
        DOE(){
            sys.reserve(Constants::InitialSystemCapacity);
            scheduleQueue.reserve(Constants::InitialJobPoolCapacity);
        };
    };
} // namespace ECS


#endif // ENGINE_HPP

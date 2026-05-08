#if !defined(ENGINE_HPP)
#define ENGINE_HPP

#include "EntityComponentStore.hpp"
#include "ComponentDependencyManager.hpp"
#include "EntityQueryManager.hpp"
#include "Base/ISystem.hpp"

namespace ECS
{
    struct DOE {
        EntityComponentStore ecs;
        ComponentDependencyManager dpm;
        EntityQueryManager eqm{&ecs};
        std::vector<align_ptr<ISystem>,allocator<align_ptr<ISystem>>> sys;
        DOE() {
            sys.reserve(Constants::InitialArchetypeArraySize);
        };
    };
} // namespace ECS


#endif // ENGINE_HPP

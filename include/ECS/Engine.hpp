#if !defined(ENGINE_HPP)
#define ENGINE_HPP

#include "./ThreadPool.hpp"
#include "./EntityComponentStore.hpp"
#include "./ComponentDependencyManager.hpp"
#include "./Base/ISystem.hpp"

namespace ECS
{
    struct DOE {
        EntityComponentStore ecs;
        ThreadPool tp;
        ComponentDependencyManager dpm;
        std::vector<align_ptr<ISystem>,allocator<align_ptr<ISystem>>> sys;
        DOE():tp{2},dpm{&tp}{};
    };
} // namespace ECS


#endif // ENGINE_HPP

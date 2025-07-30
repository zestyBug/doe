#if !defined(VULKAN_HPP)
#define VULKAN_HPP

#include "ECS/SystemManager.hpp"

namespace ECS
{
    struct example_system : ECS::System {
        ECS::version_t systemVersion = 0;
        example_system();
        void onUpdate(ECS::SystemState&);
    };
} // namespace ECS


#endif // VULKAN_HPP

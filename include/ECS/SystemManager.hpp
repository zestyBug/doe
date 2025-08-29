#if !defined(SYSTEM_MANAGER_HPP)
#define SYSTEM_MANAGER_HPP

#include "cutil/basics.hpp"
#include "ECS/defs.hpp"
#include "ECS/DependencyManager.hpp"
#include "ECS/EntityComponentManager.hpp"
#include "entt/advanced_array.hpp"

namespace ECS
{

struct SystemState;

struct System {
    const char* name;
    void *ctx;
    int32_t (*onUpdate)(void *ctx,SystemState&);
    void (*onDestroy)(void *ctx,SystemState&);
};


struct SystemState final {
    void *context=nullptr;
    advanced_array::registry<System> systems{};
    DependencyManager jobs{};
    EntityComponentManager entities{};
};


} // namespace ECS


#endif // SYSTEM_MANAGER_HPP

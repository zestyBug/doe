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
    const char* name="System";
    void (*onUpdate)(System*,SystemState&)=nullptr;
    virtual ~System() {}
};

struct SystemManager final
{
    advanced_array::registry<unique_ptr<System,allocator<System>>> systems;
};


struct SystemState final {
    void *context=nullptr;
    SystemManager manager{};
    DependencyManager jobs{};
    EntityComponentManager entities{};
};


} // namespace ECS


#endif // SYSTEM_MANAGER_HPP

#if !defined(SYSTEM_MANAGER_HPP)
#define SYSTEM_MANAGER_HPP

#include "cutil/basics.hpp"
#include "ECS/defs.hpp"
#include "ECS/DependencyManager.hpp"
#include "advanced_array/advanced_array.hpp"

namespace ECS
{

struct ThreadPool;

struct SystemState {
    DependencyManager jobs{};
    version_t globalSystemVersion;
};

struct System {
    version_t systemVersion = 0;
    virtual ~System() {};
    virtual void onStart(SystemState&){};
    virtual void onUpdate(SystemState&){};
    virtual void onStop(SystemState&){};
};

struct SystemManager
{
    advanced_array::registry<unique_ptr<System>> systems;
    
    // todo: something causes compiler to hang for long time, when i move this functions to src file!

    void startAll(SystemState& state){
        auto view = systems.view();
        for (auto value:view)
            if(System *s = view.get(value).get();s != nullptr){
                s->onStop(state);
                s->systemVersion = state.globalSystemVersion;
            }
    }
    void stopAll(SystemState& state){
        auto view = systems.view();
        for (auto value:view)
            if(System *s = view.get(value).get();s != nullptr){
                s->onStop(state);
                s->systemVersion = state.globalSystemVersion;
            }
    }
    void updateAll(SystemState& state){
        auto view = systems.view();
        for (auto value:view)
            if(System *s = view.get(value).get();s != nullptr){
                s->onUpdate(state);
                s->systemVersion = state.globalSystemVersion;
            }
    }
};



} // namespace ECS


#endif // SYSTEM_MANAGER_HPP

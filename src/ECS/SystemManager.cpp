#include "ECS/SystemManager.hpp"
using namespace ECS;




void SystemManager::startAll(SystemState& state){
    auto view = systems.view();
    for (auto value:view)
        if(System *s = view.get(value).get();s != nullptr){
            s->onStop(state);
            s->systemVersion = state.globalSystemVersion;
        }
}
void SystemManager::stopAll(SystemState& state){
    auto view = systems.view();
    for (auto value:view)
        if(System *s = view.get(value).get();s != nullptr){
            s->onStop(state);
            s->systemVersion = state.globalSystemVersion;
        }
}
void SystemManager::updateAll(SystemState& state){
    auto view = systems.view();
    for (auto value:view)
        if(System *s = view.get(value).get();s != nullptr){
            s->onUpdate(state);
            s->systemVersion = state.globalSystemVersion;
        }
}
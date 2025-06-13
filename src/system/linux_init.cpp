#include "ECS/SystemManager.hpp"

struct windows_system : ECS::System {
    uint64_t count = 0;
    ECS::version_t systemVersion = 0;
    windows_system(){
        // printf("windows_system::windows_system()\n");
        onUpdate = (void(*)(ECS::System*,ECS::SystemState&)) &windows_system::updateCB;
    }
    void updateCB(ECS::SystemState&){
        count++;
        if((count & 0xFFFFF) == 0)
            printf("windows_system::onUpdate(): %ul\n",count);
    }
};
void add_initial_systems(ECS::SystemState &ss){
    auto ws = ss.manager.systems.create();
    ss.manager.systems.emplace(ws,new windows_system());
}
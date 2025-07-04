#include "example.hpp"
using namespace ECS;
example_system::example_system(){
    // printf("example_system::example_system()\n");
    onUpdate = (void(*)(ECS::System*,ECS::SystemState&)) &example_system::updateCB;
}
void example_system::updateCB(ECS::SystemState&){
}
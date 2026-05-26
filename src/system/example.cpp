#include "example.hpp"
#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"

ECS::SystemRegister<ExampleSystem> _{};
void ExampleSystem::OnFixedUpdate(ECS::DOE&){
    counter++;
    if(counter == 100)
        ECS::JobsUtility::signalQuit();
}
ExampleSystem::ExampleSystem(ECS::DOE &e):ISystem{e}{
}
struct example_system {
    ECS::Version systemVersion = 0;
};
#include "example.hpp"

struct example_system {
    ECS::Version systemVersion = 0;
};

int32_t onUpdate(example_system *ctx,ECS::SystemState&);
void onDestroy(example_system *ctx,ECS::SystemState&);

void initExampleSystem(ECS::SystemState& ss){
    example_system *ptr=allocator<example_system>().allocate(1);
    ECS::System &sys = ss.systems.emplace(ss.systems.create());
    sys.name = "example";
    sys.ctx = ptr;
    sys.onUpdate = (int32_t (*)(void *,ECS::SystemState&)) onUpdate;
    sys.onDestroy = (void (*)(void *,ECS::SystemState&)) onDestroy;
}

int32_t onUpdate(example_system *ctx,ECS::SystemState&)
{
}

void onDestroy(example_system *ctx,ECS::SystemState&)
{
}
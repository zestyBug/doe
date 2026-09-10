#if !defined(EXAMPLE_HPP)
#define EXAMPLE_HPP

#include "ECS/Base/ISystem.hpp"
#include "ECS/Engine.hpp"
#include "uv.h"

struct ExampleSystem : ECS::ISystem{
    int counter = 0;
    ECS::EntityQueryImpl query;
    ExampleSystem(ECS::DOE&);
    void OnFixedUpdate(ECS::DOE&);
    void OnUpdate(ECS::DOE&);
    void OnDestroy(ECS::DOE&);
    ~ExampleSystem();
};

#endif // EXAMPLE_HPP

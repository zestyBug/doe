#if !defined(EXAMPLE_HPP)
#define EXAMPLE_HPP

#include "ECS/Base/ISystem.hpp"
#include "ECS/Engine.hpp"
#include "uv.h"

struct ExampleSystem : ECS::ISystem{
    int counter = 0;
    ECS::EntityQueryImpl query;
    ExampleSystem(ECS::Engine&);
    void OnFixedUpdate(ECS::Engine&);
    void OnUpdate(ECS::Engine&);
    void OnDestroy(ECS::Engine&);
    ~ExampleSystem();
};

#endif // EXAMPLE_HPP

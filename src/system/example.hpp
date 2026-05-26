#if !defined(EXAMPLE_HPP)
#define EXAMPLE_HPP

#include "ECS/Base/ISystem.hpp"

struct ExampleSystem : ECS::ISystem{
    int counter = 0;
    ExampleSystem(ECS::DOE&);
    void OnFixedUpdate(ECS::DOE&);
};

#endif // EXAMPLE_HPP

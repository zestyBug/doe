#include "Engine/ECS/Archetype.hpp"
StaticArray<DOTS::comp_info,32> DOTS::rtti;
int main(int argc, char const *argv[])
{(void)argc;(void)argv;
    DOTS::Archetype a;
    a.initialize(DOTS::componentsBitmask<int,float>());
    auto e1 = a.createEntity(0);
    auto e2 = a.createEntity(1);
    float* ptr=a.getComponent<float>(e1);
    *ptr=1;
    printf("memory address:    %p\n",ptr);
    /* code */
    return 0;
}

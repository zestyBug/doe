#if !defined(EXAMPLE_HPP)
#define EXAMPLE_HPP

#include "ECS/Base/ISystem.hpp"
#include "ECS/Engine.hpp"
#include "vulkan/VKContext.hpp"
#include "uv.h"

struct ExampleSystem : ECS::ISystem{
    int counter = 0;
    ECS::VKContext vk;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uv_thread_t gthread;
    uv_sem_t glock;
    static void gFunc(void *arg);
    ExampleSystem(ECS::DOE&);
    void OnFixedUpdate(ECS::DOE&);
    void OnUpdate(ECS::DOE&);
    void OnDestroy(ECS::DOE&);
    ~ExampleSystem();
};

#endif // EXAMPLE_HPP

#if !defined(GRAPHICSYSTEM_HPP)
#define GRAPHICSYSTEM_HPP

#include "cutil/basics.hpp"
#include "vulkan/VKContext.hpp"
#include "ECS/Base/ISystem.hpp"
#include <atomic>
#include "uv.h"

namespace ECS
{
    struct Engine;
}

/**
 * @brief This class manipulates VKContext and ImGui into rendering.
 * Using render command buffer and a seperate thread.
 * This class has bidirectional communication with JobsUtility.
 */
struct GraphicSystem : ECS::ISystem  {
private:
    ECS::VKContext vk{};
    // signal this semaphore once the surface is ready for rendering
    uv_thread_t gthread{};
    static void gFunc(void *arg);
    uv_sem_t glock{};
    std::atomic<uint32_t> running = 1;
public:
    /// @warning requires JobsUtility initialized
    GraphicSystem(ECS::Engine&);
    void BeforeUpdate(ECS::Engine&);
    void AfterUpdate(ECS::Engine&);
    void OnDestroy(ECS::Engine&);
};


#endif // GRAPHICSYSTEM_HPP

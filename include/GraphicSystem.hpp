#if !defined(GRAPHICSYSTEM_HPP)
#define GRAPHICSYSTEM_HPP

#include "cutil/basics.hpp"
#include "vulkan/VKContext.hpp"
#include <atomic>
#include "uv.h"

namespace ECS
{
    class GraphicSystem {
        VKContext vk{};
        // signal this semaphore once the surface is ready for rendering
        uv_thread_t gthread{};
        static void gFunc(void *arg);
        uv_sem_t glock{};
    public:
        void contextInit();
        void beginFrame();
        void endFrame();
        void contextDestroy();
    };
    extern align_ptr<GraphicSystem> graphics;
} // namespace ECS


#endif // GRAPHICSYSTEM_HPP

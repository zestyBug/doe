#if !defined(WINDOW_HPP)
#define WINDOW_HPP

#include "cutil/basics.hpp"
#include "vulkan/VKContext.hpp"
#include <atomic>
#include "uv.h"

namespace ECS
{
    alignas(Constants::CacheLineSize) struct Window {
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
        HWND hWnd;
        HINSTANCE hInstance;
    #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        ::Display *display;
        ::Visual *visual;
        ::VisualID visualId;
        ::Window window;
    #else
    #error
    #endif
        int width=0, height=0;
        alignas(Constants::CacheLineSize) VKContext vk{};
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        volatile uint32_t img_index = 0;
        uv_sem_t glock{};
        uv_thread_t gthread{};
        alignas(Constants::CacheLineSize) std::atomic<bool> running;
        static void gFunc(void *arg);
        void contextInit();
        void beginFrame();
        void endFrame();
        void contextDestroy();
    };
    extern Window sharedWindow;
} // namespace ECS


#endif // WINDOW_HPP

#if !defined(WINDOW_HPP)
#define WINDOW_HPP

#include "cutil/basics.hpp"
#include "vulkan/VKContext.hpp"
#include <atomic>

namespace ECS
{
    struct alignas(Constants::CacheLineSize) Window {
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
        std::atomic<uint32_t> running = 1;
        std::atomic<uint32_t> width=0, height=0;
    };
    extern Window sharedWindow;
} // namespace ECS


#endif // WINDOW_HPP

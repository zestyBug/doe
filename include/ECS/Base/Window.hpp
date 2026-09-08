#if !defined(WINDOW_HPP)
#define WINDOW_HPP

#include "cutil/basics.hpp"
#include "vulkan/vk_platform.h"

namespace ECS
{
    struct Window {
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
        int width, height;
    };
    extern Window sharedWindow;
} // namespace ECS


#endif // WINDOW_HPP

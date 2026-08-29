#if !defined(WINDOW_HPP)
#define WINDOW_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct Window {
    #if defined(_WIN32) || (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
    #else
        void *display;
        void *visual;
        unsigned long visualId;
        unsigned long window;
    #endif
        int width, height;
    };
    extern Window sharedWindow;
} // namespace ECS


#endif // WINDOW_HPP

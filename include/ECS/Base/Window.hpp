#if !defined(WINDOW_HPP)
#define WINDOW_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct Window {
        void *display;
        void *visual;
        unsigned long visualId;
        unsigned long window;
        int width, height;
    };
    extern Window sharedWindow;
} // namespace ECS


#endif // WINDOW_HPP

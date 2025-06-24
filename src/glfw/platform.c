//========================================================================
// GLFW 3.4 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2018 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#include <string.h>
#include <stdlib.h>

// These construct a string literal from individual numeric constants
#define _GLFW_CONCAT_VERSION(m, n, r) #m "." #n "." #r
#define _GLFW_MAKE_VERSION(m, n, r) _GLFW_CONCAT_VERSION(m, n, r)

//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

static const struct
{
    int ID;
} supportedPlatforms[] =
{
#if defined(_GLFW_WIN32)
    { GLFW_PLATFORM_WIN32 },
#endif
#if defined(_GLFW_COCOA)
    { GLFW_PLATFORM_COCOA },
#endif
#if defined(_GLFW_WAYLAND)
    { GLFW_PLATFORM_WAYLAND },
#endif
#if defined(_GLFW_X11)
    { GLFW_PLATFORM_X11 },
#endif
};

GLFWbool _glfwSelectPlatform()
{
    const size_t count = sizeof(supportedPlatforms) / sizeof(supportedPlatforms[0]);
    size_t i;
    if (count == 0)
    {
        _glfwInputError(GLFW_PLATFORM_UNAVAILABLE, "This binary supports no platform");
        return GLFW_FALSE;
    }
    else if (count != 1)
    {
        _glfwInputError(GLFW_PLATFORM_UNAVAILABLE, "The binary must only support one platform, compile again");
        return GLFW_FALSE;
    }

    return _glfwConnect();
}

//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////
GLFWAPI const char* glfwGetVersionString(void)
{
    return _GLFW_MAKE_VERSION(GLFW_VERSION_MAJOR,
                              GLFW_VERSION_MINOR,
                              GLFW_VERSION_REVISION)
#if defined(_GLFW_WIN32)
        " Win32 "
#endif
#if defined(_GLFW_WAYLAND)
        " Wayland"
#endif
#if defined(_GLFW_X11)
        " X11 "
#endif
        " Null"
#if defined(__MINGW64_VERSION_MAJOR)
        " MinGW-w64"
#elif defined(__MINGW32__)
        " MinGW"
#elif defined(_MSC_VER)
        " VisualC"
#endif
#if defined(_GLFW_USE_HYBRID_HPG) || defined(_GLFW_USE_OPTIMUS_HPG)
        " hybrid-GPU"
#endif
#if defined(_POSIX_MONOTONIC_CLOCK)
        " monotonic"
#endif
#if defined(_GLFW_BUILD_DLL)
#if defined(_WIN32)
        " DLL"
#elif defined(__APPLE__)
        " dynamic"
#else
        " shared"
#endif
#endif
        ;
}


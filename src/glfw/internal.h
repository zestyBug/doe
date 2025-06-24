//========================================================================
// GLFW 3.4 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2019 Camilla Löwy <elmindreda@glfw.org>
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

#pragma once

#include "glfw/glfw3.h"

#define GLFW_INCLUDE_NONE

#define _GLFW_INSERT_FIRST      0
#define _GLFW_INSERT_LAST       1

#define _GLFW_POLL_PRESENCE     0
#define _GLFW_POLL_AXES         1
#define _GLFW_POLL_BUTTONS      2
#define _GLFW_POLL_ALL          (_GLFW_POLL_AXES | _GLFW_POLL_BUTTONS)

#define _GLFW_MESSAGE_SIZE      1024

typedef int GLFWbool;
typedef void (*GLFWproc)(void);

typedef struct _GLFWerror       _GLFWerror;
typedef struct _GLFWinitconfig  _GLFWinitconfig;
typedef struct _GLFWwndconfig   _GLFWwndconfig;
typedef struct _GLFWwindow      _GLFWwindow;
typedef struct _GLFWplatform    _GLFWplatform;
typedef struct _GLFWlibrary     _GLFWlibrary;
typedef struct _GLFWmonitor     _GLFWmonitor;
typedef struct _GLFWcursor      _GLFWcursor;
typedef struct _GLFWmapelement  _GLFWmapelement;
typedef struct _GLFWmapping     _GLFWmapping;
typedef struct _GLFWjoystick    _GLFWjoystick;
typedef struct _GLFWtls         _GLFWtls;
typedef struct _GLFWmutex       _GLFWmutex;

#include "platform.h"

// Checks for whether the library has been initialized
#define _GLFW_REQUIRE_INIT()                         \
    if (!_glfw.initialized)                          \
    {                                                \
        _glfwInputError(GLFW_NOT_INITIALIZED, NULL); \
        return;                                      \
    }
#define _GLFW_REQUIRE_INIT_OR_RETURN(x)              \
    if (!_glfw.initialized)                          \
    {                                                \
        _glfwInputError(GLFW_NOT_INITIALIZED, NULL); \
        return x;                                    \
    }

// Swaps the provided pointers
#define _GLFW_SWAP(type, x, y) \
    {                          \
        type t;                \
        t = x;                 \
        x = y;                 \
        y = t;                 \
    }

// Per-thread error structure
//
struct _GLFWerror
{
    _GLFWerror*     next;
    int             code;
    char            description[_GLFW_MESSAGE_SIZE];
};

// Initialization configuration
//
// Parameters relating to the initialization of the library
//
struct _GLFWinitconfig
{
    GLFWbool      hatButtons;
#if defined(_GLFW_X11)
    struct {
        GLFWbool  xcbVulkanSurface;
    } x11;
#endif
#if defined(_GLFW_WAYLAND)
    struct {
        int       libdecorMode;
    } wl;
#endif
};

// Window configuration
//
// Parameters relating to the creation of the window but not directly related
// to the framebuffer.  This is used to pass window creation parameters from
// shared code to the platform API.
//
struct _GLFWwndconfig
{
    int           xpos;
    int           ypos;
    int           width;
    int           height;
    const char*   title;
    GLFWbool      resizable;
    GLFWbool      visible;
    GLFWbool      decorated;
    GLFWbool      focused;
    GLFWbool      autoIconify;
    GLFWbool      floating;
    GLFWbool      maximized;
    GLFWbool      centerCursor;
    GLFWbool      focusOnShow;
    GLFWbool      mousePassthrough;
    GLFWbool      scaleToMonitor;
    GLFWbool      scaleFramebuffer;
#if defined(_GLFW_X11)
    struct {
        char      className[256];
        char      instanceName[256];
    } x11;
#endif
#if defined(_GLFW_WIN32)
    struct {
        GLFWbool  keymenu;
        GLFWbool  showDefault;
    } win32;
#endif
#if defined(_GLFW_WAYLAND)
    struct {
        char      appId[256];
    } wl;
#endif
};

// Window and context structure
//
struct _GLFWwindow
{
    struct _GLFWwindow* next;

    // Window settings and state
    GLFWbool            resizable;
    GLFWbool            decorated;
    GLFWbool            autoIconify;
    GLFWbool            floating;
    GLFWbool            focusOnShow;
    GLFWbool            mousePassthrough;
    GLFWbool            shouldClose;
    void*               userPointer;
    GLFWbool            doublebuffer;
    GLFWvidmode         videoMode;
    _GLFWmonitor*       monitor;
    _GLFWcursor*        cursor;
    char*               title;

    int                 minwidth, minheight;
    int                 maxwidth, maxheight;
    int                 numer, denom;

    GLFWbool            stickyKeys;
    GLFWbool            stickyMouseButtons;
    GLFWbool            lockKeyMods;
    int                 cursorMode;
    char                mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
    char                keys[GLFW_KEY_LAST + 1];
    // Virtual cursor position when cursor is disabled
    double              virtualCursorPosX, virtualCursorPosY;
    GLFWbool            rawMouseMotion;


    struct {
        GLFWwindowposfun          pos;
        GLFWwindowsizefun         size;
        GLFWwindowclosefun        close;
        GLFWwindowrefreshfun      refresh;
        GLFWwindowfocusfun        focus;
        GLFWwindowiconifyfun      iconify;
        GLFWwindowmaximizefun     maximize;
        GLFWframebuffersizefun    fbsize;
        GLFWwindowcontentscalefun scale;
        GLFWmousebuttonfun        mouseButton;
        GLFWcursorposfun          cursorPos;
        GLFWcursorenterfun        cursorEnter;
        GLFWscrollfun             scroll;
        GLFWkeyfun                key;
        GLFWcharfun               character;
        GLFWcharmodsfun           charmods;
        GLFWdropfun               drop;
    } callbacks;

    // This is defined in platform.h
    GLFW_PLATFORM_WINDOW_STATE
};

// Monitor structure
//
struct _GLFWmonitor
{
    char            name[128];
    void*           userPointer;

    // Physical dimensions in millimeters.
    int             widthMM, heightMM;

    // The window whose video mode is current on this monitor
    _GLFWwindow*    window;

    GLFWvidmode*    modes;
    int             modeCount;
    GLFWvidmode     currentMode;

    GLFWgammaramp   originalRamp;
    GLFWgammaramp   currentRamp;

    // This is defined in platform.h
    GLFW_PLATFORM_MONITOR_STATE
};

// Cursor structure
//
struct _GLFWcursor
{
    _GLFWcursor*    next;
    // This is defined in platform.h
    GLFW_PLATFORM_CURSOR_STATE
};

// Gamepad mapping element structure
//
struct _GLFWmapelement
{
    uint8_t         type;
    uint8_t         index;
    int8_t          axisScale;
    int8_t          axisOffset;
};

// Gamepad mapping structure
//
struct _GLFWmapping
{
    char            name[128];
    char            guid[33];
    _GLFWmapelement buttons[15];
    _GLFWmapelement axes[6];
};

// Joystick structure
//
struct _GLFWjoystick
{
    GLFWbool        allocated;
    GLFWbool        connected;
    float*          axes;
    int             axisCount;
    unsigned char*  buttons;
    int             buttonCount;
    unsigned char*  hats;
    int             hatCount;
    char            name[128];
    void*           userPointer;
    char            guid[33];
    _GLFWmapping*   mapping;

    // This is defined in platform.h
    GLFW_PLATFORM_JOYSTICK_STATE
};

// Platform API structure
//



// init
GLFWbool _glfwInitOS(void);
void _glfwTerminateOS(void);
void _glfwGetCursorPosOS(_GLFWwindow*,double*,double*);
void _glfwSetCursorPosOS(_GLFWwindow*,double,double);
void _glfwSetCursorModeOS(_GLFWwindow*,int);
void _glfwSetRawMouseMotionOS(_GLFWwindow*,GLFWbool);
GLFWbool _glfwRawMouseMotionSupportedOS(void);
GLFWbool _glfwCreateCursorOS(_GLFWcursor*,const GLFWimage*,int,int);
GLFWbool _glfwCreateStandardCursorOS(_GLFWcursor*,int);
void _glfwDestroyCursorOS(_GLFWcursor*);
void _glfwSetCursorOS(_GLFWwindow*,_GLFWcursor*);
const char* _glfwGetScancodeNameOS(int);
int _glfwgetKeyScancodeOS(int);
void _glfwsetClipboardStringOS(const char*);
const char* _glfwGetClipboardStringOS(void);
GLFWbool _glfwInitJoysticksOS(void);
void _glfwTerminateJoysticksOS(void);
GLFWbool _glfwPollJoystickOS(_GLFWjoystick*,int);
const char* _glfwGetMappingNameOS(void);
void _glfwUpdateGamepadGUIDOS(char*);
void _glfwFreeMonitorOS(_GLFWmonitor*);
void _glfwGetMonitorPosOS(_GLFWmonitor*,int*,int*);
void _glfwGetMonitorContentScaleOS(_GLFWmonitor*,float*,float*);
void _glfwGetMonitorWorkareaOS(_GLFWmonitor*,int*,int*,int*,int*);
GLFWvidmode* _glfwGetVideoModesOS(_GLFWmonitor*,int*);
GLFWbool _glfwGetVideoModeOS(_GLFWmonitor*,GLFWvidmode*);
GLFWbool _glfwGetGammaRampOS(_GLFWmonitor*,GLFWgammaramp*);
void _glfwSetGammaRampOS(_GLFWmonitor*,const GLFWgammaramp*);
GLFWbool _glfwCreateWindowOS(_GLFWwindow*,const _GLFWwndconfig*);
void _glfwDestroyWindowOS(_GLFWwindow*);
void _glfwSetWindowTitleOS(_GLFWwindow*,const char*);
void _glfwSetWindowIconOS(_GLFWwindow*,int,const GLFWimage*);
void _glfwGetWindowPosOS(_GLFWwindow*,int*,int*);
void _glfwSetWindowPosOS(_GLFWwindow*,int,int);
void _glfwGetWindowSizeOS(_GLFWwindow*,int*,int*);
void _glfwSetWindowSizeOS(_GLFWwindow*,int,int);
void _glfwSetWindowSizeLimitsOS(_GLFWwindow*,int,int,int,int);
void _glfwSetWindowAspectRatioOS(_GLFWwindow*,int,int);
void _glfwGetFramebufferSizeOS(_GLFWwindow*,int*,int*);
void _glfwGetWindowFrameSizeOS(_GLFWwindow*,int*,int*,int*,int*);
void _glfwGetWindowContentScaleOS(_GLFWwindow*,float*,float*);
void _glfwIconifyWindowOS(_GLFWwindow*);
void _glfwRestoreWindowOS(_GLFWwindow*);
void _glfwMaximizeWindowOS(_GLFWwindow*);
void _glfwShowWindowOS(_GLFWwindow*);
void _glfwHideWindowOS(_GLFWwindow*);
void _glfwRequestWindowAttentionOS(_GLFWwindow*);
void _glfwFocusWindowOS(_GLFWwindow*);
void _glfwSetWindowMonitorOS(_GLFWwindow*,_GLFWmonitor*,int,int,int,int,int);
GLFWbool _glfwWindowFocusedOS(_GLFWwindow*);
GLFWbool _glfwWindowIconifiedOS(_GLFWwindow*);
GLFWbool _glfwWindowVisibleOS(_GLFWwindow*);
GLFWbool _glfwWindowMaximizedOS(_GLFWwindow*);
GLFWbool _glfwWindowHoveredOS(_GLFWwindow*);
GLFWbool _glfwFramebufferTransparentOS(_GLFWwindow*);
float _glfwGetWindowOpacityOS(_GLFWwindow*);
void _glfwSetWindowResizableOS(_GLFWwindow*,GLFWbool);
void _glfwSetWindowDecoratedOS(_GLFWwindow*,GLFWbool);
void _glfwSetWindowFloatingOS(_GLFWwindow*,GLFWbool);
void _glfwSetWindowOpacityOS(_GLFWwindow*,float);
void _glfwSetWindowMousePassthroughOS(_GLFWwindow*,GLFWbool);
void _glfwPollEventsOS(void);
void _glfwWaitEventsOS(void);
void _glfwWaitEventsTimeoutOS(double);
void _glfwPostEmptyEventOS(void);



// Library global data
//
struct _GLFWlibrary
{
    GLFWbool            initialized;


    struct {
        _GLFWinitconfig init;
        _GLFWwndconfig  window;
        int             refreshRate;
    } hints;

    _GLFWcursor*        cursorListHead;
    _GLFWwindow*        windowListHead;
    _GLFWwindow         window;

    _GLFWmonitor**      monitors;
    int                 monitorCount;
#if defined(GLFW_BUILD_LINUX_JOYSTICK)
    GLFWbool            joysticksInitialized;
    _GLFWjoystick       joysticks[GLFW_JOYSTICK_LAST + 1];
    _GLFWmapping*       mappings;
    int                 mappingCount;
#endif


    struct {
        uint64_t        offset;
        // This is defined in platform.h
        GLFW_PLATFORM_LIBRARY_TIMER_STATE
    } timer;

    struct {
        GLFWmonitorfun  monitor;
    #if defined(GLFW_BUILD_LINUX_JOYSTICK)
        GLFWjoystickfun joystick;
    #endif
    } callbacks;

    // These are defined in platform.h
    GLFW_PLATFORM_LIBRARY_WINDOW_STATE
    GLFW_PLATFORM_LIBRARY_JOYSTICK_STATE
};

// Global state shared between compilation units of GLFW
//
extern _GLFWlibrary _glfw;


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

void _glfwPlatformInitTimer(void);
uint64_t _glfwPlatformGetTimerValue(void);
uint64_t _glfwPlatformGetTimerFrequency(void);

void* _glfwPlatformLoadModule(const char* path);
void _glfwPlatformFreeModule(void* module);
GLFWproc _glfwPlatformGetModuleSymbol(void* module, const char* name);


//////////////////////////////////////////////////////////////////////////
//////                         GLFW event API                       //////
//////////////////////////////////////////////////////////////////////////

void _glfwInputWindowFocus(_GLFWwindow* window, GLFWbool focused);
void _glfwInputWindowPos(_GLFWwindow* window, int xpos, int ypos);
void _glfwInputWindowSize(_GLFWwindow* window, int width, int height);
void _glfwInputFramebufferSize(_GLFWwindow* window, int width, int height);
void _glfwInputWindowContentScale(_GLFWwindow* window,
                                  float xscale, float yscale);
void _glfwInputWindowIconify(_GLFWwindow* window, GLFWbool iconified);
void _glfwInputWindowMaximize(_GLFWwindow* window, GLFWbool maximized);
void _glfwInputWindowDamage(_GLFWwindow* window);
void _glfwInputWindowCloseRequest(_GLFWwindow* window);
void _glfwInputWindowMonitor(_GLFWwindow* window, _GLFWmonitor* monitor);

void _glfwInputKey(_GLFWwindow* window,
                   int key, int scancode, int action, int mods);
void _glfwInputChar(_GLFWwindow* window,
                    uint32_t codepoint, int mods, GLFWbool plain);
void _glfwInputScroll(_GLFWwindow* window, double xoffset, double yoffset);
void _glfwInputMouseClick(_GLFWwindow* window, int button, int action, int mods);
void _glfwInputCursorPos(_GLFWwindow* window, double xpos, double ypos);
void _glfwInputCursorEnter(_GLFWwindow* window, GLFWbool entered);
void _glfwInputDrop(_GLFWwindow* window, int count, const char** names);
void _glfwInputJoystick(_GLFWjoystick* js, int event);
void _glfwInputJoystickAxis(_GLFWjoystick* js, int axis, float value);
void _glfwInputJoystickButton(_GLFWjoystick* js, int button, char value);
void _glfwInputJoystickHat(_GLFWjoystick* js, int hat, char value);

void _glfwInputMonitor(_GLFWmonitor* monitor, int action, int placement);
void _glfwInputMonitorWindow(_GLFWmonitor* monitor, _GLFWwindow* window);

#if defined(__GNUC__)
void _glfwInputError(int code, const char* format, ...)
    __attribute__((format(printf, 2, 3)));
#else
void _glfwInputError(int code, const char* format, ...);
#endif


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

GLFWbool _glfwSelectPlatform();

GLFWbool _glfwStringInExtensionString(const char* string, const char* extensions);

const GLFWvidmode* _glfwChooseVideoMode(_GLFWmonitor* monitor,
                                        const GLFWvidmode* desired);
int _glfwCompareVideoModes(const GLFWvidmode* first, const GLFWvidmode* second);
_GLFWmonitor* _glfwAllocMonitor(const char* name, int widthMM, int heightMM);
void _glfwFreeMonitor(_GLFWmonitor* monitor);
void _glfwAllocGammaArrays(GLFWgammaramp* ramp, unsigned int size);
void _glfwFreeGammaArrays(GLFWgammaramp* ramp);
void _glfwSplitBPP(int bpp, int* red, int* green, int* blue);

void _glfwInitGamepadMappings(void);
_GLFWjoystick* _glfwAllocJoystick(const char* name,
                                  const char* guid,
                                  int axisCount,
                                  int buttonCount,
                                  int hatCount);
void _glfwFreeJoystick(_GLFWjoystick* js);
void _glfwCenterCursorInContentArea(_GLFWwindow* window);


size_t _glfwEncodeUTF8(char* s, uint32_t codepoint);
char** _glfwParseUriList(char* text, int* count);

char* _glfw_strdup(const char* source);
int _glfw_min(int a, int b);
int _glfw_max(int a, int b);

void* _glfw_calloc(size_t count, size_t size);
void* _glfw_realloc(void* pointer, size_t size);
void _glfw_free(void* pointer);


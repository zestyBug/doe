/*------------------------------------------------------------------------
 * A demonstration of OpenGL in a  ARGB window
 *    => support for composited window transparency
 *
 * (c) 2011 by Wolfgang 'datenwolf' Draxinger
 *     See me at comp.graphics.api.opengl and StackOverflow.com

 * License agreement: This source code is provided "as is". You
 * can use this source code however you want for your own personal
 * use. If you give this source code to anybody else then you must
 * leave this message in it.
 *
 * This program is based on the simplest possible
 * Linux OpenGL program by FTB (see info below)

  The simplest possible Linux OpenGL program? Maybe...

  (c) 2002 by FTB. See me in comp.graphics.api.opengl

  --
  <\___/>
  / O O \
  \_____/  FTB.

------------------------------------------------------------------------*/
//#define _GNU_SOURCE

#include "ECS/Engine.hpp"
#include "ECS/ThreadPool.hpp"
#include "ECS/Base/Window.hpp"
#include "uv.h"
#include "imgui.h"

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>

#undef None
#undef Bool
#undef Always
static constexpr int None=0;
typedef int Bool;
std::unique_ptr<ECS::DOE> ECS::sharedEngine;
ECS::Window ECS::sharedWindow;

static Atom del_atom;


static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
{
	return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
}

static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    (void)dpy;(void)ev;
    fputs("Error at context creation\n", stderr);
    return 0;
}

static void WndProc(uv_poll_t *handle, int status, int events);

int main(int argc, char *argv[])
{
	//Window;
    int screen;
    Window root;

    /* Avoid locale-related number parsing issues */
    setlocale(LC_NUMERIC, "C");

    ECS::sharedWindow.display = XOpenDisplay(NULL);
    if (!ECS::sharedWindow.display)
        throw std::runtime_error("Couldn't connect to X server\n");
    screen = DefaultScreen(ECS::sharedWindow.display);
    root = RootWindow(ECS::sharedWindow.display, screen);
	ECS::sharedWindow.visual   = DefaultVisual(ECS::sharedWindow.display, screen);
    ECS::sharedWindow.width    = DisplayWidth (ECS::sharedWindow.display, screen)/2;
	ECS::sharedWindow.height   = DisplayHeight(ECS::sharedWindow.display, screen)/2;
    ECS::sharedWindow.visualId = XVisualIDFromVisual((Visual *)ECS::sharedWindow.visual);

    {
        int attr_mask;
        XSetWindowAttributes attr;
        attr.colormap = None;
        attr.background_pixmap = None;
        attr.border_pixmap = None;
        attr.border_pixel = 0;
        attr.event_mask = EnterWindowMask | LeaveWindowMask | OwnerGrabButtonMask | ExposureMask | VisibilityChangeMask | StructureNotifyMask | PropertyChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask;
        //	CWBackPixmap|
        attr_mask = CWColormap | CWBorderPixel | CWEventMask;
        ECS::sharedWindow.window = XCreateWindow((Display *)ECS::sharedWindow.display, root, 0, 0, ECS::sharedWindow.width, ECS::sharedWindow.height, 0, CopyFromParent, InputOutput, CopyFromParent, attr_mask, &attr);
        if(!ECS::sharedWindow.window)
            throw std::runtime_error("Couldn't create the window\n");
    }

    {
        const char *title = "Title";
        XEvent event;
        XTextProperty textprop;
        XSizeHints hints;
        XWMHints *startup_state;
        textprop.value = (unsigned char*)title;
        textprop.encoding = XA_STRING;
        textprop.format = 8;
        textprop.nitems = strlen(title);
        hints.x = 0;
        hints.y = 0;
        hints.width = ECS::sharedWindow.width;
        hints.height = ECS::sharedWindow.height;
        hints.flags = USPosition|USSize;
        startup_state = XAllocWMHints();
        startup_state->initial_state = NormalState;
        startup_state->flags = StateHint;
        XSetStandardProperties((Display*)ECS::sharedWindow.display, (Window)ECS::sharedWindow.window, "xogl", "xogl", None, argv, argc, NULL);
        XSetWMProperties((Display*)ECS::sharedWindow.display, (Window)ECS::sharedWindow.window, &textprop, &textprop, NULL, 0, &hints, startup_state, NULL);
        XFree(startup_state);

        XMapWindow((Display*)ECS::sharedWindow.display, (Window)ECS::sharedWindow.window);
        XIfEvent((Display*)ECS::sharedWindow.display, &event, WaitForMapNotify, (XPointer)&ECS::sharedWindow.window);

        if ((del_atom = XInternAtom((Display*)ECS::sharedWindow.display, "WM_DELETE_WINDOW", 0)) != None) {
            XSetWMProtocols((Display*)ECS::sharedWindow.display, (Window)ECS::sharedWindow.window, &del_atom, 1);
        }
    }

    uv_setup_args(argc,argv);
    uv_loop_t *loop = uv_default_loop();
    uv_poll_t x11_poll;
    uv_poll_init(loop, &x11_poll, XConnectionNumber((Display*)ECS::sharedWindow.display));
    uv_poll_start(&x11_poll, UV_READABLE, &WndProc);

    ImGui::CreateContext();
    {
        ImGuiIO& io=ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable some options
        io.BackendPlatformUserData = nullptr;
        io.BackendPlatformName = "imgui_impl_my";
    }
    ECS::sharedEngine = std::make_unique<ECS::DOE>();
    ECS::TypeManager::Initialize();
    ECS::JobsUtility::init();

    uv_run(loop, UV_RUN_DEFAULT);

    uv_poll_stop(&x11_poll);
    ECS::sharedEngine.reset();
    ImGui::DestroyContext();
    uv_loop_close(loop);
    uv_library_shutdown();

#ifdef DEBUG
    // one for the Threadpool jobs + 2 for TypeManager
    if(allocator_counter){
        printf("Memory leak count %li\n",allocator_counter);
    }
#endif

    XDestroyWindow((Display*)ECS::sharedWindow.display, (Window)ECS::sharedWindow.window);
    XCloseDisplay((Display*)ECS::sharedWindow.display);
	return 0;
}

ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiModKey(unsigned int keycode);
ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiKey(unsigned int keycode);
void WndProc(uv_poll_t *handle, int status, int events)
{
    XEvent event;
    ImGuiIO& io=ImGui::GetIO();
    while (XPending((Display*)ECS::sharedWindow.display))
    {
        XNextEvent((Display*)ECS::sharedWindow.display, &event);
        KeySym unic;
        switch (event.type)
        {
        case ConfigureNotify:
            io.DisplaySize = ImVec2((float)event.xconfigure.width, (float)event.xconfigure.height);
            break;
        case KeyPress:
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiModKey(event.xkey.keycode), 1);
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiKey(event.xkey.keycode), true);
            unic=XLookupKeysym(&event.xkey, 0);

            if ((0x20<=unic && unic<=0x7e)
            || (0xa0<=unic && unic<=0xff)
            || (0x1a1<=unic && unic<=0x1ff)
            || (0x2a1<=unic && unic<=0x2fe)
            ){
                io.AddInputCharacter(unic);
            }
            break;
        case KeyRelease:
            //XMaskEvent()
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiModKey(event.xkey.keycode), 0);
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiKey(event.xkey.keycode), false);
            break;
        /*
            1 = left button
            2 = middle button (pressing the scroll wheel)
            3 = right button
            4 = turn scroll wheel up
            5 = turn scroll wheel down
            6 = push scroll wheel left
            7 = push scroll wheel right
            8 = 4th button (aka browser backward button)
            9 = 5th button (aka browser forward button)
        */
        case ButtonPress:
            if (Button1==event.xbutton.button)
            	io.AddMouseButtonEvent(0, true);
            if (Button2==event.xbutton.button)
            	io.AddMouseButtonEvent(2, true);
            if (Button3==event.xbutton.button)
            	io.AddMouseButtonEvent(1, true);
            else if (event.xbutton.button==Button4)
            	io.AddMouseWheelEvent(0.0f, 0.5);
            else if (event.xbutton.button==Button5)
            	io.AddMouseWheelEvent(0.0f,-0.5);
            break;
        case ButtonRelease:
            if (Button1==event.xbutton.button)
            	io.AddMouseButtonEvent(0, false);
            if (Button2==event.xbutton.button)
            	io.AddMouseButtonEvent(2, false);
            if (Button3==event.xbutton.button)
            	io.AddMouseButtonEvent(1, false);
            else if (event.xbutton.button==Button4)
            	io.AddMouseWheelEvent(0.0f, 0.5);
            else if (event.xbutton.button==Button5)
            	io.AddMouseWheelEvent(0.0f,-0.5);
            break;
        case MotionNotify:
            io.AddMousePosEvent((float)event.xmotion.x,(float)event.xmotion.y);
            break;
        case Expose:
            break;
        case ClientMessage:
            if ((unsigned)(event.xclient.data.l[0]) == del_atom)
                ECS::JobsUtility::signalQuit();
            break;
        }
    }
}



ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiModKey(unsigned int keycode)
{
    switch (keycode)
	{
	case 37:
	case 105:
        return ImGuiMod_Ctrl;
	case 64:
	case 108:
        return ImGuiMod_Alt;
	case 50:
	case 62:
        return ImGuiMod_Shift;
    default:
        return ImGuiKey_None;
    }
}
ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiKey(unsigned int keycode)
{
	switch (keycode)
	{
	//ImGuiKey_0
	//case XK_0 ... XK_9: return (ImGuiKey)key+488;


	case 9: return ImGuiKey_Escape;
	case 10 ... 18: return (ImGuiKey)(ImGuiKey_1+(keycode-10));
	case 19: return ImGuiKey_0;
	case 20: return ImGuiKey_Minus;
	case 21: return ImGuiKey_Equal;
	case 22: return ImGuiKey_Backspace;


	case 23: return ImGuiKey_Tab;
	case 24: return ImGuiKey_Q;
	case 25: return ImGuiKey_W;
	case 26: return ImGuiKey_E;
	case 27: return ImGuiKey_R;
	case 28: return ImGuiKey_T;
	case 29: return ImGuiKey_Y;
	case 30: return ImGuiKey_U;
	case 31: return ImGuiKey_I;
	case 32: return ImGuiKey_O;
	case 33: return ImGuiKey_P;
	case 34: return ImGuiKey_LeftBracket;
	case 35: return ImGuiKey_RightBracket;

	case 36: return ImGuiKey_Enter;
	case 37: return ImGuiKey_LeftCtrl;
	case 38: return ImGuiKey_A;
	case 39: return ImGuiKey_S;
	case 40: return ImGuiKey_D;
	case 41: return ImGuiKey_F;
	case 42: return ImGuiKey_G;
	case 43: return ImGuiKey_H;
	case 44: return ImGuiKey_J;
	case 45: return ImGuiKey_K;
	case 46: return ImGuiKey_L;
	case 47: return ImGuiKey_Semicolon;
	case 48: return ImGuiKey_Comma;
	case 49: return ImGuiKey_GraveAccent;

	case 50: return ImGuiKey_LeftShift;
	case 51: return ImGuiKey_Backslash;
	case 52: return ImGuiKey_Z;
	case 53: return ImGuiKey_X;
	case 54: return ImGuiKey_C;
	case 55: return ImGuiKey_V;
	case 56: return ImGuiKey_B;
	case 57: return ImGuiKey_N;
	case 58: return ImGuiKey_M;
	case 59: return ImGuiKey_Apostrophe;
	case 60: return ImGuiKey_Period;
	case 61: return ImGuiKey_Slash;
	case 62: return ImGuiKey_RightShift;
	case 63: return ImGuiKey_KeypadMultiply;
	case 64: return ImGuiKey_LeftAlt;
	case 65: return ImGuiKey_Space;
	case 66: return ImGuiKey_CapsLock;

	case 67 ... 76: return (ImGuiKey)(ImGuiKey_F1+(keycode-67));


	case 77:        return ImGuiKey_NumLock;
	case 79 ... 81: return (ImGuiKey)(ImGuiKey_Keypad7+(keycode-79));
	case 82:        return ImGuiKey_KeypadSubtract;
	case 83 ... 85: return (ImGuiKey)(ImGuiKey_Keypad4+(keycode-83));
	case 86:        return ImGuiKey_KeypadAdd;
	case 87 ... 89: return (ImGuiKey)(ImGuiKey_Keypad1+(keycode-87));
	case 90:        return ImGuiKey_Keypad0;
	case 91:        return ImGuiKey_KeypadDecimal;


	case 95: return ImGuiKey_F11;
	case 96: return ImGuiKey_F12;

	case 104: return ImGuiKey_KeypadEnter;
	case 105: return ImGuiKey_RightCtrl;
	case 106: return ImGuiKey_KeypadDivide;
	case 108: return ImGuiKey_RightAlt;

	case 110: return ImGuiKey_Home;
	case 111: return ImGuiKey_UpArrow;
	case 113: return ImGuiKey_LeftArrow;
	case 112: return ImGuiKey_PageUp;
	case 114: return ImGuiKey_RightArrow;
	case 115: return ImGuiKey_End;
	case 116: return ImGuiKey_DownArrow;
	case 117: return ImGuiKey_PageDown;
	case 118: return ImGuiKey_Insert;
	case 119: return ImGuiKey_Delete;
	default:
	return ImGuiKey_None;
	}
}
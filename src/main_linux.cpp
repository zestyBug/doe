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

#define Button6 6
#define Button7 7
#undef None
#undef Bool
#undef Always
static constexpr int None=0;
typedef int Bool;
std::unique_ptr<ECS::DOE> ECS::sharedEngine;
ECS::Window ECS::sharedWindow;

static Atom del_atom;
XIM xim;
XIC xic;


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

    xim = XOpenIM((Display*)ECS::sharedWindow.display, NULL, NULL, NULL);
    xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, ECS::sharedWindow.window, XNFocusWindow, ECS::sharedWindow.window, NULL);

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


// Decode a Unicode code point from a UTF-8 stream
// Based on cutef8 by Jeff Bezanson (Public Domain)
//
static uint32_t decodeUTF8(const char* s){
    uint32_t codepoint = 0, count = 0;
    static const uint32_t offsets[] = {
        0x00000000u, 0x00003080u, 0x000e2080u,
        0x03c82080u, 0xfa082080u, 0x82082080u
    };
    do {
        codepoint = (codepoint << 6) + (unsigned char) *s;
        (s)++;
        count++;
    } while ((*s & 0xc0) == 0x80);

    if(likely(count <= 6))
        return codepoint - offsets[count - 1];
    return 0;
}

ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiModKey(unsigned int keysym);
ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiKey   (unsigned int keysym);
void WndProc(uv_poll_t *handle, int, int)
{
    XEvent event;
    ImGuiIO& io=ImGui::GetIO();
    char utf8[64];
    KeySym keysym;
    Status status;
    int len;
    while (XPending((Display*)ECS::sharedWindow.display))
    {
        XNextEvent((Display*)ECS::sharedWindow.display, &event);
        switch (event.type)
        {
        case ConfigureNotify:
            io.DisplaySize = ImVec2((float)event.xconfigure.width, (float)event.xconfigure.height);
            break;
        case KeyPress:
            keysym=XLookupKeysym(&event.xkey, 0);
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiModKey(keysym), true);
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiKey   (keysym), true);

            len = Xutf8LookupString(xic, &event.xkey, utf8, sizeof(utf8), NULL, &status);
            if (status == XLookupChars || status == XLookupBoth)
            {
                utf8[len] = '\0';
                io.AddInputCharacter(decodeUTF8(utf8));
            }
            break;
        case KeyRelease:
            //XMaskEvent()
            keysym=XLookupKeysym(&event.xkey, 0);
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiModKey(keysym), 0);
            io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiKey(keysym), false);
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
            if      (event.xbutton.button == Button1)
            	io.AddMouseButtonEvent(0, true);
            else if (event.xbutton.button == Button2)
            	io.AddMouseButtonEvent(2, true);
            else if (event.xbutton.button == Button3)
            	io.AddMouseButtonEvent(1, true);
            else if (event.xbutton.button == Button4)
            	io.AddMouseWheelEvent(0.0f, 0.5f);
            else if (event.xbutton.button == Button5)
            	io.AddMouseWheelEvent(0.0f, -0.5f);
            else if (event.xbutton.button == Button6)
                io.AddMouseWheelEvent(0.5f, 0.0f);
            else if (event.xbutton.button == Button7)
                io.AddMouseWheelEvent(-0.5f, 0.0f);
            break;
        case ButtonRelease:
            if      (event.xbutton.button == Button1)
            	io.AddMouseButtonEvent(0, false);
            else if (event.xbutton.button == Button2)
            	io.AddMouseButtonEvent(2, false);
            else if (event.xbutton.button == Button3)
            	io.AddMouseButtonEvent(1, false);
            else if (event.xbutton.button == Button4)
            	io.AddMouseWheelEvent(0.0f, 0.5f);
            else if (event.xbutton.button == Button5)
            	io.AddMouseWheelEvent(0.0f, -0.5f);
            else if (event.xbutton.button == Button6)
                io.AddMouseWheelEvent(0.5f, 0.0f);
            else if (event.xbutton.button == Button7)
                io.AddMouseWheelEvent(-0.5f, 0.0f);
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



ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiModKey(unsigned int keysym)
{
    switch (keysym)
	{
    // XK_Caps_Lock
	case XK_Control_L:
	case XK_Control_R:
        return ImGuiMod_Ctrl;
	case XK_Alt_L:
	case XK_Alt_R:
        return ImGuiMod_Alt;
	case XK_Shift_L:
	case XK_Shift_R:
        return ImGuiMod_Shift;
    default:
        return ImGuiMod_None;
    }
}
ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiKey(unsigned int keysym)
{
	switch (keysym)
	{
    case XK_bracketleft: return ImGuiKey_LeftBracket;
    case XK_backslash: return ImGuiKey_Backslash;
    case XK_bracketright: return ImGuiKey_RightBracket;
    case XK_grave: return ImGuiKey_GraveAccent;
    case XK_a ... XK_z: return (ImGuiKey)(ImGuiKey_A+(keysym-XK_a));
    case XK_space: return ImGuiKey_Space;
    case XK_apostrophe: return ImGuiKey_Apostrophe;
    case XK_plus: return ImGuiKey_Equal;
    case XK_comma: return ImGuiKey_Comma;
    case XK_minus: return ImGuiKey_Minus;
    case XK_period: return ImGuiKey_Period;
    case XK_slash: return ImGuiKey_Slash;
    case XK_0 ... XK_9: return (ImGuiKey)(ImGuiKey_0+(keysym-XK_0));
    case XK_semicolon: return ImGuiKey_Semicolon;
	case XK_F1 ... XK_F24: return (ImGuiKey)(ImGuiKey_F1+(keysym-XK_F1));

	case XK_BackSpace: return ImGuiKey_Backspace;
	case XK_Tab: return ImGuiKey_Tab;
    case XK_Return: return ImGuiKey_Enter;
	case XK_Escape: return ImGuiKey_Escape;
    case XK_Delete: return ImGuiKey_Delete;

    case XK_Home: return ImGuiKey_Home;
    case XK_Left: return ImGuiKey_LeftArrow;
    case XK_Up: return ImGuiKey_UpArrow;
    case XK_Right: return ImGuiKey_RightArrow;
    case XK_Down: return ImGuiKey_DownArrow;
    case XK_Page_Up: return ImGuiKey_PageUp;
    case XK_Page_Down: return ImGuiKey_PageDown;
    case XK_End: return ImGuiKey_End;

	case XK_Shift_L: return ImGuiKey_LeftShift;
	case XK_Shift_R: return ImGuiKey_RightShift;
	case XK_Control_L: return ImGuiKey_LeftCtrl;
	case XK_Control_R: return ImGuiKey_RightCtrl;
	case XK_Caps_Lock: return ImGuiKey_CapsLock;
    case XK_Alt_L: return ImGuiKey_LeftAlt;
    case XK_Alt_R: return ImGuiKey_RightAlt;
    case XK_Super_L: return ImGuiKey_LeftSuper;
    case XK_Super_R: return ImGuiKey_RightSuper;

    case XK_Insert: return ImGuiKey_Insert;
    case XK_Num_Lock: return ImGuiKey_NumLock;

    case XK_KP_Enter: return ImGuiKey_KeypadEnter;
    case XK_KP_Home: return ImGuiKey_Keypad7;
    case XK_KP_Left: return ImGuiKey_Keypad4;
    case XK_KP_Up: return ImGuiKey_Keypad8;
    case XK_KP_Right: return ImGuiKey_Keypad6;
    case XK_KP_Down: return ImGuiKey_Keypad2;
    case XK_KP_Page_Up: return ImGuiKey_Keypad9;
    case XK_KP_Page_Down: return ImGuiKey_Keypad3;
    case XK_KP_End: return ImGuiKey_Keypad1;
    case XK_KP_Begin: return ImGuiKey_Keypad5;
    case XK_KP_Insert: return ImGuiKey_Keypad0;
    case XK_KP_Delete: return ImGuiKey_KeypadDecimal;
    case XK_KP_Multiply: return ImGuiKey_KeypadMultiply;
    case XK_KP_Add: return ImGuiKey_KeypadAdd;
    case XK_KP_Subtract: return ImGuiKey_KeypadSubtract;
    case XK_KP_Divide: return ImGuiKey_KeypadDivide;
	default: return ImGuiKey_None;
	}
}
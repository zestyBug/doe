//========================================================================
// GLFW 3.4 X11 - www.glfw.org
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

#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#include "internal.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xcursor/Xcursor.h>

// The XRandR extension provides mode setting and gamma control
#include <X11/extensions/Xrandr.h>

// The Xkb extension provides improved keyboard support
#include <X11/XKBlib.h>

// The Xinerama extension provides legacy monitor indices
#include <X11/extensions/Xinerama.h>

// The XInput extension provides raw mouse motion input
#include <X11/extensions/XInput2.h>

// The Shape extension provides custom window shapes
#include <X11/extensions/shape.h>

typedef XClassHint* (* PFN_XAllocClassHint)(void);
typedef XSizeHints* (* PFN_XAllocSizeHints)(void);
typedef XWMHints* (* PFN_XAllocWMHints)(void);
typedef int (* PFN_XChangeProperty)(Display*,Window,Atom,Atom,int,int,const unsigned char*,int);
typedef int (* PFN_XChangeWindowAttributes)(Display*,Window,unsigned long,XSetWindowAttributes*);
typedef Bool (* PFN_XCheckIfEvent)(Display*,XEvent*,Bool(*)(Display*,XEvent*,XPointer),XPointer);
typedef Bool (* PFN_XCheckTypedWindowEvent)(Display*,Window,int,XEvent*);
typedef int (* PFN_XCloseDisplay)(Display*);
typedef Status (* PFN_XCloseIM)(XIM);
typedef int (* PFN_XConvertSelection)(Display*,Atom,Atom,Atom,Window,Time);
typedef Colormap (* PFN_XCreateColormap)(Display*,Window,Visual*,int);
typedef Cursor (* PFN_XCreateFontCursor)(Display*,unsigned int);
typedef XIC (* PFN_XCreateIC)(XIM,...);
typedef Region (* PFN_XCreateRegion)(void);
typedef Window (* PFN_XCreateWindow)(Display*,Window,int,int,unsigned int,unsigned int,unsigned int,int,unsigned int,Visual*,unsigned long,XSetWindowAttributes*);
typedef int (* PFN_XDefineCursor)(Display*,Window,Cursor);
typedef int (* PFN_XDeleteContext)(Display*,XID,XContext);
typedef int (* PFN_XDeleteProperty)(Display*,Window,Atom);
typedef void (* PFN_XDestroyIC)(XIC);
typedef int (* PFN_XDestroyRegion)(Region);
typedef int (* PFN_XDestroyWindow)(Display*,Window);
typedef int (* PFN_XDisplayKeycodes)(Display*,int*,int*);
typedef int (* PFN_XEventsQueued)(Display*,int);
typedef Bool (* PFN_XFilterEvent)(XEvent*,Window);
typedef int (* PFN_XFindContext)(Display*,XID,XContext,XPointer*);
typedef int (* PFN_XFlush)(Display*);
typedef int (* PFN_XFree)(void*);
typedef int (* PFN_XFreeColormap)(Display*,Colormap);
typedef int (* PFN_XFreeCursor)(Display*,Cursor);
typedef void (* PFN_XFreeEventData)(Display*,XGenericEventCookie*);
typedef int (* PFN_XGetErrorText)(Display*,int,char*,int);
typedef Bool (* PFN_XGetEventData)(Display*,XGenericEventCookie*);
typedef char* (* PFN_XGetICValues)(XIC,...);
typedef char* (* PFN_XGetIMValues)(XIM,...);
typedef int (* PFN_XGetInputFocus)(Display*,Window*,int*);
typedef KeySym* (* PFN_XGetKeyboardMapping)(Display*,KeyCode,int,int*);
typedef int (* PFN_XGetScreenSaver)(Display*,int*,int*,int*,int*);
typedef Window (* PFN_XGetSelectionOwner)(Display*,Atom);
typedef XVisualInfo* (* PFN_XGetVisualInfo)(Display*,long,XVisualInfo*,int*);
typedef Status (* PFN_XGetWMNormalHints)(Display*,Window,XSizeHints*,long*);
typedef Status (* PFN_XGetWindowAttributes)(Display*,Window,XWindowAttributes*);
typedef int (* PFN_XGetWindowProperty)(Display*,Window,Atom,long,long,Bool,Atom,Atom*,int*,unsigned long*,unsigned long*,unsigned char**);
typedef int (* PFN_XGrabPointer)(Display*,Window,Bool,unsigned int,int,int,Window,Cursor,Time);
typedef Status (* PFN_XIconifyWindow)(Display*,Window,int);
typedef Status (* PFN_XInitThreads)(void);
typedef Atom (* PFN_XInternAtom)(Display*,const char*,Bool);
typedef int (* PFN_XLookupString)(XKeyEvent*,char*,int,KeySym*,XComposeStatus*);
typedef int (* PFN_XMapRaised)(Display*,Window);
typedef int (* PFN_XMapWindow)(Display*,Window);
typedef int (* PFN_XMoveResizeWindow)(Display*,Window,int,int,unsigned int,unsigned int);
typedef int (* PFN_XMoveWindow)(Display*,Window,int,int);
typedef int (* PFN_XNextEvent)(Display*,XEvent*);
typedef Display* (* PFN_XOpenDisplay)(const char*);
typedef XIM (* PFN_XOpenIM)(Display*,XrmDatabase*,char*,char*);
typedef int (* PFN_XPeekEvent)(Display*,XEvent*);
typedef int (* PFN_XPending)(Display*);
typedef Bool (* PFN_XQueryExtension)(Display*,const char*,int*,int*,int*);
typedef Bool (* PFN_XQueryPointer)(Display*,Window,Window*,Window*,int*,int*,int*,int*,unsigned int*);
typedef int (* PFN_XRaiseWindow)(Display*,Window);
typedef Bool (* PFN_XRegisterIMInstantiateCallback)(Display*,void*,char*,char*,XIDProc,XPointer);
typedef int (* PFN_XResizeWindow)(Display*,Window,unsigned int,unsigned int);
typedef char* (* PFN_XResourceManagerString)(Display*);
typedef int (* PFN_XSaveContext)(Display*,XID,XContext,const char*);
typedef int (* PFN_XSelectInput)(Display*,Window,long);
typedef Status (* PFN_XSendEvent)(Display*,Window,Bool,long,XEvent*);
typedef int (* PFN_XSetClassHint)(Display*,Window,XClassHint*);
typedef XErrorHandler (* PFN_XSetErrorHandler)(XErrorHandler);
typedef void (* PFN_XSetICFocus)(XIC);
typedef char* (* PFN_XSetIMValues)(XIM,...);
typedef int (* PFN_XSetInputFocus)(Display*,Window,int,Time);
typedef char* (* PFN_XSetLocaleModifiers)(const char*);
typedef int (* PFN_XSetScreenSaver)(Display*,int,int,int,int);
typedef int (* PFN_XSetSelectionOwner)(Display*,Atom,Window,Time);
typedef int (* PFN_XSetWMHints)(Display*,Window,XWMHints*);
typedef void (* PFN_XSetWMNormalHints)(Display*,Window,XSizeHints*);
typedef Status (* PFN_XSetWMProtocols)(Display*,Window,Atom*,int);
typedef Bool (* PFN_XSupportsLocale)(void);
typedef int (* PFN_XSync)(Display*,Bool);
typedef Bool (* PFN_XTranslateCoordinates)(Display*,Window,Window,int,int,int*,int*,Window*);
typedef int (* PFN_XUndefineCursor)(Display*,Window);
typedef int (* PFN_XUngrabPointer)(Display*,Time);
typedef int (* PFN_XUnmapWindow)(Display*,Window);
typedef void (* PFN_XUnsetICFocus)(XIC);
typedef VisualID (* PFN_XVisualIDFromVisual)(Visual*);
typedef int (* PFN_XWarpPointer)(Display*,Window,Window,int,int,unsigned int,unsigned int,int,int);
typedef void (* PFN_XkbFreeKeyboard)(XkbDescPtr,unsigned int,Bool);
typedef void (* PFN_XkbFreeNames)(XkbDescPtr,unsigned int,Bool);
typedef XkbDescPtr (* PFN_XkbGetMap)(Display*,unsigned int,unsigned int);
typedef Status (* PFN_XkbGetNames)(Display*,unsigned int,XkbDescPtr);
typedef Status (* PFN_XkbGetState)(Display*,unsigned int,XkbStatePtr);
typedef KeySym (* PFN_XkbKeycodeToKeysym)(Display*,KeyCode,int,int);
typedef Bool (* PFN_XkbQueryExtension)(Display*,int*,int*,int*,int*,int*);
typedef Bool (* PFN_XkbSelectEventDetails)(Display*,unsigned int,unsigned int,unsigned long,unsigned long);
typedef Bool (* PFN_XkbSetDetectableAutoRepeat)(Display*,Bool,Bool*);
typedef void (* PFN_XrmDestroyDatabase)(XrmDatabase);
typedef Bool (* PFN_XrmGetResource)(XrmDatabase,const char*,const char*,char**,XrmValue*);
typedef XrmDatabase (* PFN_XrmGetStringDatabase)(const char*);
typedef void (* PFN_XrmInitialize)(void);
typedef XrmQuark (* PFN_XrmUniqueQuark)(void);
typedef Bool (* PFN_XUnregisterIMInstantiateCallback)(Display*,void*,char*,char*,XIDProc,XPointer);
typedef int (* PFN_Xutf8LookupString)(XIC,XKeyPressedEvent*,char*,int,KeySym*,Status*);
typedef void (* PFN_Xutf8SetWMProperties)(Display*,Window,const char*,const char*,char**,int,XSizeHints*,XWMHints*,XClassHint*);
#define XAllocClassHint _glfw.x11.xlib.AllocClassHint
#define XAllocSizeHints _glfw.x11.xlib.AllocSizeHints
#define XAllocWMHints _glfw.x11.xlib.AllocWMHints
#define XChangeProperty _glfw.x11.xlib.ChangeProperty
#define XChangeWindowAttributes _glfw.x11.xlib.ChangeWindowAttributes
#define XCheckIfEvent _glfw.x11.xlib.CheckIfEvent
#define XCheckTypedWindowEvent _glfw.x11.xlib.CheckTypedWindowEvent
#define XCloseDisplay _glfw.x11.xlib.CloseDisplay
#define XCloseIM _glfw.x11.xlib.CloseIM
#define XConvertSelection _glfw.x11.xlib.ConvertSelection
#define XCreateColormap _glfw.x11.xlib.CreateColormap
#define XCreateFontCursor _glfw.x11.xlib.CreateFontCursor
#define XCreateIC _glfw.x11.xlib.CreateIC
#define XCreateRegion _glfw.x11.xlib.CreateRegion
#define XCreateWindow _glfw.x11.xlib.CreateWindow
#define XDefineCursor _glfw.x11.xlib.DefineCursor
#define XDeleteContext _glfw.x11.xlib.DeleteContext
#define XDeleteProperty _glfw.x11.xlib.DeleteProperty
#define XDestroyIC _glfw.x11.xlib.DestroyIC
#define XDestroyRegion _glfw.x11.xlib.DestroyRegion
#define XDestroyWindow _glfw.x11.xlib.DestroyWindow
#define XDisplayKeycodes _glfw.x11.xlib.DisplayKeycodes
#define XEventsQueued _glfw.x11.xlib.EventsQueued
#define XFilterEvent _glfw.x11.xlib.FilterEvent
#define XFindContext _glfw.x11.xlib.FindContext
#define XFlush _glfw.x11.xlib.Flush
#define XFree _glfw.x11.xlib.Free
#define XFreeColormap _glfw.x11.xlib.FreeColormap
#define XFreeCursor _glfw.x11.xlib.FreeCursor
#define XFreeEventData _glfw.x11.xlib.FreeEventData
#define XGetErrorText _glfw.x11.xlib.GetErrorText
#define XGetEventData _glfw.x11.xlib.GetEventData
#define XGetICValues _glfw.x11.xlib.GetICValues
#define XGetIMValues _glfw.x11.xlib.GetIMValues
#define XGetInputFocus _glfw.x11.xlib.GetInputFocus
#define XGetKeyboardMapping _glfw.x11.xlib.GetKeyboardMapping
#define XGetScreenSaver _glfw.x11.xlib.GetScreenSaver
#define XGetSelectionOwner _glfw.x11.xlib.GetSelectionOwner
#define XGetVisualInfo _glfw.x11.xlib.GetVisualInfo
#define XGetWMNormalHints _glfw.x11.xlib.GetWMNormalHints
#define XGetWindowAttributes _glfw.x11.xlib.GetWindowAttributes
#define XGetWindowProperty _glfw.x11.xlib.GetWindowProperty
#define XGrabPointer _glfw.x11.xlib.GrabPointer
#define XIconifyWindow _glfw.x11.xlib.IconifyWindow
#define XInternAtom _glfw.x11.xlib.InternAtom
#define XLookupString _glfw.x11.xlib.LookupString
#define XMapRaised _glfw.x11.xlib.MapRaised
#define XMapWindow _glfw.x11.xlib.MapWindow
#define XMoveResizeWindow _glfw.x11.xlib.MoveResizeWindow
#define XMoveWindow _glfw.x11.xlib.MoveWindow
#define XNextEvent _glfw.x11.xlib.NextEvent
#define XOpenIM _glfw.x11.xlib.OpenIM
#define XPeekEvent _glfw.x11.xlib.PeekEvent
#define XPending _glfw.x11.xlib.Pending
#define XQueryExtension _glfw.x11.xlib.QueryExtension
#define XQueryPointer _glfw.x11.xlib.QueryPointer
#define XRaiseWindow _glfw.x11.xlib.RaiseWindow
#define XRegisterIMInstantiateCallback _glfw.x11.xlib.RegisterIMInstantiateCallback
#define XResizeWindow _glfw.x11.xlib.ResizeWindow
#define XResourceManagerString _glfw.x11.xlib.ResourceManagerString
#define XSaveContext _glfw.x11.xlib.SaveContext
#define XSelectInput _glfw.x11.xlib.SelectInput
#define XSendEvent _glfw.x11.xlib.SendEvent
#define XSetClassHint _glfw.x11.xlib.SetClassHint
#define XSetErrorHandler _glfw.x11.xlib.SetErrorHandler
#define XSetICFocus _glfw.x11.xlib.SetICFocus
#define XSetIMValues _glfw.x11.xlib.SetIMValues
#define XSetInputFocus _glfw.x11.xlib.SetInputFocus
#define XSetLocaleModifiers _glfw.x11.xlib.SetLocaleModifiers
#define XSetScreenSaver _glfw.x11.xlib.SetScreenSaver
#define XSetSelectionOwner _glfw.x11.xlib.SetSelectionOwner
#define XSetWMHints _glfw.x11.xlib.SetWMHints
#define XSetWMNormalHints _glfw.x11.xlib.SetWMNormalHints
#define XSetWMProtocols _glfw.x11.xlib.SetWMProtocols
#define XSupportsLocale _glfw.x11.xlib.SupportsLocale
#define XSync _glfw.x11.xlib.Sync
#define XTranslateCoordinates _glfw.x11.xlib.TranslateCoordinates
#define XUndefineCursor _glfw.x11.xlib.UndefineCursor
#define XUngrabPointer _glfw.x11.xlib.UngrabPointer
#define XUnmapWindow _glfw.x11.xlib.UnmapWindow
#define XUnsetICFocus _glfw.x11.xlib.UnsetICFocus
#define XVisualIDFromVisual _glfw.x11.xlib.VisualIDFromVisual
#define XWarpPointer _glfw.x11.xlib.WarpPointer
#define XkbFreeKeyboard _glfw.x11.xkb.FreeKeyboard
#define XkbFreeNames _glfw.x11.xkb.FreeNames
#define XkbGetMap _glfw.x11.xkb.GetMap
#define XkbGetNames _glfw.x11.xkb.GetNames
#define XkbGetState _glfw.x11.xkb.GetState
#define XkbKeycodeToKeysym _glfw.x11.xkb.KeycodeToKeysym
#define XkbQueryExtension _glfw.x11.xkb.QueryExtension
#define XkbSelectEventDetails _glfw.x11.xkb.SelectEventDetails
#define XkbSetDetectableAutoRepeat _glfw.x11.xkb.SetDetectableAutoRepeat
#define XrmDestroyDatabase _glfw.x11.xrm.DestroyDatabase
#define XrmGetResource _glfw.x11.xrm.GetResource
#define XrmGetStringDatabase _glfw.x11.xrm.GetStringDatabase
#define XrmUniqueQuark _glfw.x11.xrm.UniqueQuark
#define XUnregisterIMInstantiateCallback _glfw.x11.xlib.UnregisterIMInstantiateCallback
#define Xutf8LookupString _glfw.x11.xlib.utf8LookupString
#define Xutf8SetWMProperties _glfw.x11.xlib.utf8SetWMProperties

typedef XRRCrtcGamma* (* PFN_XRRAllocGamma)(int);
typedef void (* PFN_XRRFreeCrtcInfo)(XRRCrtcInfo*);
typedef void (* PFN_XRRFreeGamma)(XRRCrtcGamma*);
typedef void (* PFN_XRRFreeOutputInfo)(XRROutputInfo*);
typedef void (* PFN_XRRFreeScreenResources)(XRRScreenResources*);
typedef XRRCrtcGamma* (* PFN_XRRGetCrtcGamma)(Display*,RRCrtc);
typedef int (* PFN_XRRGetCrtcGammaSize)(Display*,RRCrtc);
typedef XRRCrtcInfo* (* PFN_XRRGetCrtcInfo) (Display*,XRRScreenResources*,RRCrtc);
typedef XRROutputInfo* (* PFN_XRRGetOutputInfo)(Display*,XRRScreenResources*,RROutput);
typedef RROutput (* PFN_XRRGetOutputPrimary)(Display*,Window);
typedef XRRScreenResources* (* PFN_XRRGetScreenResourcesCurrent)(Display*,Window);
typedef Bool (* PFN_XRRQueryExtension)(Display*,int*,int*);
typedef Status (* PFN_XRRQueryVersion)(Display*,int*,int*);
typedef void (* PFN_XRRSelectInput)(Display*,Window,int);
typedef Status (* PFN_XRRSetCrtcConfig)(Display*,XRRScreenResources*,RRCrtc,Time,int,int,RRMode,Rotation,RROutput*,int);
typedef void (* PFN_XRRSetCrtcGamma)(Display*,RRCrtc,XRRCrtcGamma*);
typedef int (* PFN_XRRUpdateConfiguration)(XEvent*);
#define XRRAllocGamma _glfw.x11.randr.AllocGamma
#define XRRFreeCrtcInfo _glfw.x11.randr.FreeCrtcInfo
#define XRRFreeGamma _glfw.x11.randr.FreeGamma
#define XRRFreeOutputInfo _glfw.x11.randr.FreeOutputInfo
#define XRRFreeScreenResources _glfw.x11.randr.FreeScreenResources
#define XRRGetCrtcGamma _glfw.x11.randr.GetCrtcGamma
#define XRRGetCrtcGammaSize _glfw.x11.randr.GetCrtcGammaSize
#define XRRGetCrtcInfo _glfw.x11.randr.GetCrtcInfo
#define XRRGetOutputInfo _glfw.x11.randr.GetOutputInfo
#define XRRGetOutputPrimary _glfw.x11.randr.GetOutputPrimary
#define XRRGetScreenResourcesCurrent _glfw.x11.randr.GetScreenResourcesCurrent
#define XRRQueryExtension _glfw.x11.randr.QueryExtension
#define XRRQueryVersion _glfw.x11.randr.QueryVersion
#define XRRSelectInput _glfw.x11.randr.SelectInput
#define XRRSetCrtcConfig _glfw.x11.randr.SetCrtcConfig
#define XRRSetCrtcGamma _glfw.x11.randr.SetCrtcGamma
#define XRRUpdateConfiguration _glfw.x11.randr.UpdateConfiguration

typedef XcursorImage* (* PFN_XcursorImageCreate)(int,int);
typedef void (* PFN_XcursorImageDestroy)(XcursorImage*);
typedef Cursor (* PFN_XcursorImageLoadCursor)(Display*,const XcursorImage*);
typedef char* (* PFN_XcursorGetTheme)(Display*);
typedef int (* PFN_XcursorGetDefaultSize)(Display*);
typedef XcursorImage* (* PFN_XcursorLibraryLoadImage)(const char*,const char*,int);
#define XcursorImageCreate _glfw.x11.xcursor.ImageCreate
#define XcursorImageDestroy _glfw.x11.xcursor.ImageDestroy
#define XcursorImageLoadCursor _glfw.x11.xcursor.ImageLoadCursor
#define XcursorGetTheme _glfw.x11.xcursor.GetTheme
#define XcursorGetDefaultSize _glfw.x11.xcursor.GetDefaultSize
#define XcursorLibraryLoadImage _glfw.x11.xcursor.LibraryLoadImage

typedef Bool (* PFN_XineramaIsActive)(Display*);
typedef Bool (* PFN_XineramaQueryExtension)(Display*,int*,int*);
typedef XineramaScreenInfo* (* PFN_XineramaQueryScreens)(Display*,int*);
#define XineramaIsActive _glfw.x11.xinerama.IsActive
#define XineramaQueryExtension _glfw.x11.xinerama.QueryExtension
#define XineramaQueryScreens _glfw.x11.xinerama.QueryScreens

typedef XID xcb_window_t;
typedef XID xcb_visualid_t;
typedef struct xcb_connection_t xcb_connection_t;
typedef xcb_connection_t* (* PFN_XGetXCBConnection)(Display*);
#define XGetXCBConnection _glfw.x11.x11xcb.GetXCBConnection

typedef Bool (* PFN_XF86VidModeQueryExtension)(Display*,int*,int*);
typedef Bool (* PFN_XF86VidModeGetGammaRamp)(Display*,int,int,unsigned short*,unsigned short*,unsigned short*);
typedef Bool (* PFN_XF86VidModeSetGammaRamp)(Display*,int,int,unsigned short*,unsigned short*,unsigned short*);
typedef Bool (* PFN_XF86VidModeGetGammaRampSize)(Display*,int,int*);
#define XF86VidModeQueryExtension _glfw.x11.vidmode.QueryExtension
#define XF86VidModeGetGammaRamp _glfw.x11.vidmode.GetGammaRamp
#define XF86VidModeSetGammaRamp _glfw.x11.vidmode.SetGammaRamp
#define XF86VidModeGetGammaRampSize _glfw.x11.vidmode.GetGammaRampSize

typedef Status (* PFN_XIQueryVersion)(Display*,int*,int*);
typedef int (* PFN_XISelectEvents)(Display*,Window,XIEventMask*,int);
#define XIQueryVersion _glfw.x11.xi.QueryVersion
#define XISelectEvents _glfw.x11.xi.SelectEvents

typedef Bool (* PFN_XRenderQueryExtension)(Display*,int*,int*);
typedef Status (* PFN_XRenderQueryVersion)(Display*dpy,int*,int*);
typedef XRenderPictFormat* (* PFN_XRenderFindVisualFormat)(Display*,Visual const*);
#define XRenderQueryExtension _glfw.x11.xrender.QueryExtension
#define XRenderQueryVersion _glfw.x11.xrender.QueryVersion
#define XRenderFindVisualFormat _glfw.x11.xrender.FindVisualFormat

typedef Bool (* PFN_XShapeQueryExtension)(Display*,int*,int*);
typedef Status (* PFN_XShapeQueryVersion)(Display*dpy,int*,int*);
typedef void (* PFN_XShapeCombineRegion)(Display*,Window,int,int,int,Region,int);
typedef void (* PFN_XShapeCombineMask)(Display*,Window,int,int,int,Pixmap,int);

#define XShapeQueryExtension _glfw.x11.xshape.QueryExtension
#define XShapeQueryVersion _glfw.x11.xshape.QueryVersion
#define XShapeCombineRegion _glfw.x11.xshape.ShapeCombineRegion
#define XShapeCombineMask _glfw.x11.xshape.ShapeCombineMask

#include "xkb_unicode.h"
#include "posix_poll.h"

#define GLFW_X11_WINDOW_STATE           _GLFWwindowX11 x11;
#define GLFW_X11_LIBRARY_WINDOW_STATE   _GLFWlibraryX11 x11;
#define GLFW_X11_MONITOR_STATE          _GLFWmonitorX11 x11;
#define GLFW_X11_CURSOR_STATE           _GLFWcursorX11 x11;

// X11-specific per-window data
//
typedef struct _GLFWwindowX11
{
    Colormap        colormap;
    Window          handle;
    Window          parent;
    XIC             ic;

    GLFWbool        overrideRedirect;
    GLFWbool        iconified;
    GLFWbool        maximized;

    // Whether the visual supports framebuffer transparency
    GLFWbool        transparent;

    // Cached position and size used to filter out duplicate events
    int             width, height;
    int             xpos, ypos;

    // The last received cursor position, regardless of source
    int             lastCursorPosX, lastCursorPosY;
    // The last position the cursor was warped to by GLFW
    int             warpCursorPosX, warpCursorPosY;

    // The time of the last KeyPress event per keycode, for discarding
    // duplicate key events generated for some keys by ibus
    Time            keyPressTimes[256];
} _GLFWwindowX11;

// X11-specific global data
//
typedef struct _GLFWlibraryX11
{
    Display*        display;
    int             screen;
    Window          root;

    // System content scale
    float           contentScaleX, contentScaleY;
    // Helper window for IPC
    Window          helperWindowHandle;
    // Invisible cursor for hidden cursor mode
    Cursor          hiddenCursorHandle;
    // Context for mapping window XIDs to _GLFWwindow pointers
    XContext        context;
    // XIM input method
    XIM             im;
    // The previous X error handler, to be restored later
    XErrorHandler   errorHandler;
    // Most recent error code received by X error handler
    int             errorCode;
    // Primary selection string (while the primary selection is owned)
    char*           primarySelectionString;
    // Clipboard string (while the selection is owned)
    char*           clipboardString;
    // Key name string
    char            keynames[GLFW_KEY_LAST + 1][5];
    // X11 keycode to GLFW key LUT
    short int       keycodes[256];
    // GLFW key to X11 keycode LUT
    short int       scancodes[GLFW_KEY_LAST + 1];
    // Where to place the cursor when re-enabled
    double          restoreCursorPosX, restoreCursorPosY;
    // The window whose disabled cursor mode is active
    _GLFWwindow*    disabledCursorWindow;
    int             emptyEventPipe[2];

    // Window manager atoms
    Atom            NET_SUPPORTED;
    Atom            NET_SUPPORTING_WM_CHECK;
    Atom            WM_PROTOCOLS;
    Atom            WM_STATE;
    Atom            WM_DELETE_WINDOW;
    Atom            NET_WM_NAME;
    Atom            NET_WM_ICON_NAME;
    Atom            NET_WM_ICON;
    Atom            NET_WM_PID;
    Atom            NET_WM_PING;
    Atom            NET_WM_WINDOW_TYPE;
    Atom            NET_WM_WINDOW_TYPE_NORMAL;
    Atom            NET_WM_STATE;
    Atom            NET_WM_STATE_ABOVE;
    Atom            NET_WM_STATE_FULLSCREEN;
    Atom            NET_WM_STATE_MAXIMIZED_VERT;
    Atom            NET_WM_STATE_MAXIMIZED_HORZ;
    Atom            NET_WM_STATE_DEMANDS_ATTENTION;
    Atom            NET_WM_BYPASS_COMPOSITOR;
    Atom            NET_WM_FULLSCREEN_MONITORS;
    Atom            NET_WM_WINDOW_OPACITY;
    Atom            NET_WM_CM_Sx;
    Atom            NET_WORKAREA;
    Atom            NET_CURRENT_DESKTOP;
    Atom            NET_ACTIVE_WINDOW;
    Atom            NET_FRAME_EXTENTS;
    Atom            NET_REQUEST_FRAME_EXTENTS;
    Atom            MOTIF_WM_HINTS;

    // Xdnd (drag and drop) atoms
    Atom            XdndAware;
    Atom            XdndEnter;
    Atom            XdndPosition;
    Atom            XdndStatus;
    Atom            XdndActionCopy;
    Atom            XdndDrop;
    Atom            XdndFinished;
    Atom            XdndSelection;
    Atom            XdndTypeList;
    Atom            text_uri_list;

    // Selection (clipboard) atoms
    Atom            TARGETS;
    Atom            MULTIPLE;
    Atom            INCR;
    Atom            CLIPBOARD;
    Atom            PRIMARY;
    Atom            CLIPBOARD_MANAGER;
    Atom            SAVE_TARGETS;
    Atom            NULL_;
    Atom            UTF8_STRING;
    Atom            COMPOUND_STRING;
    Atom            ATOM_PAIR;
    Atom            GLFW_SELECTION;

    struct {
        void*       handle;
        GLFWbool    utf8;
        PFN_XAllocClassHint AllocClassHint;
        PFN_XAllocSizeHints AllocSizeHints;
        PFN_XAllocWMHints AllocWMHints;
        PFN_XChangeProperty ChangeProperty;
        PFN_XChangeWindowAttributes ChangeWindowAttributes;
        PFN_XCheckIfEvent CheckIfEvent;
        PFN_XCheckTypedWindowEvent CheckTypedWindowEvent;
        PFN_XCloseDisplay CloseDisplay;
        PFN_XCloseIM CloseIM;
        PFN_XConvertSelection ConvertSelection;
        PFN_XCreateColormap CreateColormap;
        PFN_XCreateFontCursor CreateFontCursor;
        PFN_XCreateIC CreateIC;
        PFN_XCreateRegion CreateRegion;
        PFN_XCreateWindow CreateWindow;
        PFN_XDefineCursor DefineCursor;
        PFN_XDeleteContext DeleteContext;
        PFN_XDeleteProperty DeleteProperty;
        PFN_XDestroyIC DestroyIC;
        PFN_XDestroyRegion DestroyRegion;
        PFN_XDestroyWindow DestroyWindow;
        PFN_XDisplayKeycodes DisplayKeycodes;
        PFN_XEventsQueued EventsQueued;
        PFN_XFilterEvent FilterEvent;
        PFN_XFindContext FindContext;
        PFN_XFlush Flush;
        PFN_XFree Free;
        PFN_XFreeColormap FreeColormap;
        PFN_XFreeCursor FreeCursor;
        PFN_XFreeEventData FreeEventData;
        PFN_XGetErrorText GetErrorText;
        PFN_XGetEventData GetEventData;
        PFN_XGetICValues GetICValues;
        PFN_XGetIMValues GetIMValues;
        PFN_XGetInputFocus GetInputFocus;
        PFN_XGetKeyboardMapping GetKeyboardMapping;
        PFN_XGetScreenSaver GetScreenSaver;
        PFN_XGetSelectionOwner GetSelectionOwner;
        PFN_XGetVisualInfo GetVisualInfo;
        PFN_XGetWMNormalHints GetWMNormalHints;
        PFN_XGetWindowAttributes GetWindowAttributes;
        PFN_XGetWindowProperty GetWindowProperty;
        PFN_XGrabPointer GrabPointer;
        PFN_XIconifyWindow IconifyWindow;
        PFN_XInternAtom InternAtom;
        PFN_XLookupString LookupString;
        PFN_XMapRaised MapRaised;
        PFN_XMapWindow MapWindow;
        PFN_XMoveResizeWindow MoveResizeWindow;
        PFN_XMoveWindow MoveWindow;
        PFN_XNextEvent NextEvent;
        PFN_XOpenIM OpenIM;
        PFN_XPeekEvent PeekEvent;
        PFN_XPending Pending;
        PFN_XQueryExtension QueryExtension;
        PFN_XQueryPointer QueryPointer;
        PFN_XRaiseWindow RaiseWindow;
        PFN_XRegisterIMInstantiateCallback RegisterIMInstantiateCallback;
        PFN_XResizeWindow ResizeWindow;
        PFN_XResourceManagerString ResourceManagerString;
        PFN_XSaveContext SaveContext;
        PFN_XSelectInput SelectInput;
        PFN_XSendEvent SendEvent;
        PFN_XSetClassHint SetClassHint;
        PFN_XSetErrorHandler SetErrorHandler;
        PFN_XSetICFocus SetICFocus;
        PFN_XSetIMValues SetIMValues;
        PFN_XSetInputFocus SetInputFocus;
        PFN_XSetLocaleModifiers SetLocaleModifiers;
        PFN_XSetScreenSaver SetScreenSaver;
        PFN_XSetSelectionOwner SetSelectionOwner;
        PFN_XSetWMHints SetWMHints;
        PFN_XSetWMNormalHints SetWMNormalHints;
        PFN_XSetWMProtocols SetWMProtocols;
        PFN_XSupportsLocale SupportsLocale;
        PFN_XSync Sync;
        PFN_XTranslateCoordinates TranslateCoordinates;
        PFN_XUndefineCursor UndefineCursor;
        PFN_XUngrabPointer UngrabPointer;
        PFN_XUnmapWindow UnmapWindow;
        PFN_XUnsetICFocus UnsetICFocus;
        PFN_XVisualIDFromVisual VisualIDFromVisual;
        PFN_XWarpPointer WarpPointer;
        PFN_XUnregisterIMInstantiateCallback UnregisterIMInstantiateCallback;
        PFN_Xutf8LookupString utf8LookupString;
        PFN_Xutf8SetWMProperties utf8SetWMProperties;
    } xlib;

    struct {
        PFN_XrmDestroyDatabase DestroyDatabase;
        PFN_XrmGetResource GetResource;
        PFN_XrmGetStringDatabase GetStringDatabase;
        PFN_XrmUniqueQuark UniqueQuark;
    } xrm;

    struct {
        GLFWbool    available;
        void*       handle;
        int         eventBase;
        int         errorBase;
        int         major;
        int         minor;
        GLFWbool    gammaBroken;
        GLFWbool    monitorBroken;
        PFN_XRRAllocGamma AllocGamma;
        PFN_XRRFreeCrtcInfo FreeCrtcInfo;
        PFN_XRRFreeGamma FreeGamma;
        PFN_XRRFreeOutputInfo FreeOutputInfo;
        PFN_XRRFreeScreenResources FreeScreenResources;
        PFN_XRRGetCrtcGamma GetCrtcGamma;
        PFN_XRRGetCrtcGammaSize GetCrtcGammaSize;
        PFN_XRRGetCrtcInfo GetCrtcInfo;
        PFN_XRRGetOutputInfo GetOutputInfo;
        PFN_XRRGetOutputPrimary GetOutputPrimary;
        PFN_XRRGetScreenResourcesCurrent GetScreenResourcesCurrent;
        PFN_XRRQueryExtension QueryExtension;
        PFN_XRRQueryVersion QueryVersion;
        PFN_XRRSelectInput SelectInput;
        PFN_XRRSetCrtcConfig SetCrtcConfig;
        PFN_XRRSetCrtcGamma SetCrtcGamma;
        PFN_XRRUpdateConfiguration UpdateConfiguration;
    } randr;

    struct {
        GLFWbool     available;
        GLFWbool     detectable;
        int          majorOpcode;
        int          eventBase;
        int          errorBase;
        int          major;
        int          minor;
        unsigned int group;
        PFN_XkbFreeKeyboard FreeKeyboard;
        PFN_XkbFreeNames FreeNames;
        PFN_XkbGetMap GetMap;
        PFN_XkbGetNames GetNames;
        PFN_XkbGetState GetState;
        PFN_XkbKeycodeToKeysym KeycodeToKeysym;
        PFN_XkbQueryExtension QueryExtension;
        PFN_XkbSelectEventDetails SelectEventDetails;
        PFN_XkbSetDetectableAutoRepeat SetDetectableAutoRepeat;
    } xkb;

    struct {
        int         count;
        int         timeout;
        int         interval;
        int         blanking;
        int         exposure;
    } saver;

    struct {
        int         version;
        Window      source;
        Atom        format;
    } xdnd;

    struct {
        void*       handle;
        PFN_XcursorImageCreate ImageCreate;
        PFN_XcursorImageDestroy ImageDestroy;
        PFN_XcursorImageLoadCursor ImageLoadCursor;
        PFN_XcursorGetTheme GetTheme;
        PFN_XcursorGetDefaultSize GetDefaultSize;
        PFN_XcursorLibraryLoadImage LibraryLoadImage;
    } xcursor;

    struct {
        GLFWbool    available;
        void*       handle;
        int         major;
        int         minor;
        PFN_XineramaIsActive IsActive;
        PFN_XineramaQueryExtension QueryExtension;
        PFN_XineramaQueryScreens QueryScreens;
    } xinerama;

    struct {
        void*       handle;
        PFN_XGetXCBConnection GetXCBConnection;
    } x11xcb;

    struct {
        GLFWbool    available;
        void*       handle;
        int         eventBase;
        int         errorBase;
        PFN_XF86VidModeQueryExtension QueryExtension;
        PFN_XF86VidModeGetGammaRamp GetGammaRamp;
        PFN_XF86VidModeSetGammaRamp SetGammaRamp;
        PFN_XF86VidModeGetGammaRampSize GetGammaRampSize;
    } vidmode;

    struct {
        GLFWbool    available;
        void*       handle;
        int         majorOpcode;
        int         eventBase;
        int         errorBase;
        int         major;
        int         minor;
        PFN_XIQueryVersion QueryVersion;
        PFN_XISelectEvents SelectEvents;
    } xi;

    struct {
        GLFWbool    available;
        void*       handle;
        int         major;
        int         minor;
        int         eventBase;
        int         errorBase;
        PFN_XRenderQueryExtension QueryExtension;
        PFN_XRenderQueryVersion QueryVersion;
        PFN_XRenderFindVisualFormat FindVisualFormat;
    } xrender;

    struct {
        GLFWbool    available;
        void*       handle;
        int         major;
        int         minor;
        int         eventBase;
        int         errorBase;
        PFN_XShapeQueryExtension QueryExtension;
        PFN_XShapeCombineRegion ShapeCombineRegion;
        PFN_XShapeQueryVersion QueryVersion;
        PFN_XShapeCombineMask ShapeCombineMask;
    } xshape;
} _GLFWlibraryX11;

// X11-specific per-monitor data
//
typedef struct _GLFWmonitorX11
{
    RROutput        output;
    RRCrtc          crtc;
    RRMode          oldMode;

    // Index of corresponding Xinerama screen,
    // for EWMH full screen window placement
    int             index;
} _GLFWmonitorX11;

// X11-specific per-cursor data
//
typedef struct _GLFWcursorX11
{
    Cursor handle;
} _GLFWcursorX11;


GLFWbool _glfwConnect();
int _glfwInitOS(void);
void _glfwTerminateOS(void);

GLFWbool _glfwCreateWindowOS(_GLFWwindow* window, const _GLFWwndconfig* wndconfig);
void _glfwDestroyWindowOS(_GLFWwindow* window);
void _glfwSetWindowTitleOS(_GLFWwindow* window, const char* title);
void _glfwSetWindowIconOS(_GLFWwindow* window, int count, const GLFWimage* images);
void _glfwGetWindowPosOS(_GLFWwindow* window, int* xpos, int* ypos);
void _glfwSetWindowPosOS(_GLFWwindow* window, int xpos, int ypos);
void _glfwGetWindowSizeOS(_GLFWwindow* window, int* width, int* height);
void _glfwSetWindowSizeOS(_GLFWwindow* window, int width, int height);
void _glfwSetWindowSizeLimitsOS(_GLFWwindow* window, int minwidth, int minheight, int maxwidth, int maxheight);
void _glfwSetWindowAspectRatioOS(_GLFWwindow* window, int numer, int denom);
void _glfwGetFramebufferSizeOS(_GLFWwindow* window, int* width, int* height);
void _glfwGetWindowFrameSizeOS(_GLFWwindow* window, int* left, int* top, int* right, int* bottom);
void _glfwGetWindowContentScaleOS(_GLFWwindow* window, float* xscale, float* yscale);
void _glfwIconifyWindowOS(_GLFWwindow* window);
void _glfwRestoreWindowOS(_GLFWwindow* window);
void _glfwMaximizeWindowOS(_GLFWwindow* window);
void _glfwShowWindowOS(_GLFWwindow* window);
void _glfwHideWindowOS(_GLFWwindow* window);
void _glfwRequestWindowAttentionOS(_GLFWwindow* window);
void _glfwFocusWindowOS(_GLFWwindow* window);
void _glfwSetWindowMonitorOS(_GLFWwindow* window, _GLFWmonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate);
GLFWbool _glfwWindowFocusedOS(_GLFWwindow* window);
GLFWbool _glfwWindowIconifiedOS(_GLFWwindow* window);
GLFWbool _glfwWindowVisibleOS(_GLFWwindow* window);
GLFWbool _glfwWindowMaximizedOS(_GLFWwindow* window);
GLFWbool _glfwWindowHoveredOS(_GLFWwindow* window);
GLFWbool _glfwFramebufferTransparentOS(_GLFWwindow* window);
void _glfwSetWindowResizableOS(_GLFWwindow* window, GLFWbool enabled);
void _glfwSetWindowDecoratedOS(_GLFWwindow* window, GLFWbool enabled);
void _glfwSetWindowFloatingOS(_GLFWwindow* window, GLFWbool enabled);
float _glfwGetWindowOpacityOS(_GLFWwindow* window);
void _glfwSetWindowOpacityOS(_GLFWwindow* window, float opacity);
void _glfwSetWindowMousePassthroughOS(_GLFWwindow* window, GLFWbool enabled);

void _glfwSetRawMouseMotionOS(_GLFWwindow *window, GLFWbool enabled);
GLFWbool _glfwRawMouseMotionSupportedOS(void);

void _glfwPollEventsOS(void);
void _glfwWaitEventsOS(void);
void _glfwWaitEventsTimeoutOS(double timeout);
void _glfwPostEmptyEventOS(void);

void _glfwGetCursorPosOS(_GLFWwindow* window, double* xpos, double* ypos);
void _glfwSetCursorPosOS(_GLFWwindow* window, double xpos, double ypos);
void _glfwSetCursorModeOS(_GLFWwindow* window, int mode);
const char* _glfwGetScancodeNameOS(int scancode);
int _glfwGetKeyScancodeOS(int key);
GLFWbool _glfwCreateCursorOS(_GLFWcursor* cursor, const GLFWimage* image, int xhot, int yhot);
GLFWbool _glfwCreateStandardCursorOS(_GLFWcursor* cursor, int shape);
void _glfwDestroyCursorOS(_GLFWcursor* cursor);
void _glfwSetCursorOS(_GLFWwindow* window, _GLFWcursor* cursor);
void _glfwSetClipboardStringOS(const char* string);
const char* _glfwGetClipboardStringOS(void);

void _glfwFreeMonitorOS(_GLFWmonitor* monitor);
void _glfwGetMonitorPosOS(_GLFWmonitor* monitor, int* xpos, int* ypos);
void _glfwGetMonitorContentScaleOS(_GLFWmonitor* monitor, float* xscale, float* yscale);
void _glfwGetMonitorWorkareaOS(_GLFWmonitor* monitor, int* xpos, int* ypos, int* width, int* height);
GLFWvidmode* _glfwGetVideoModesOS(_GLFWmonitor* monitor, int* count);
GLFWbool _glfwGetVideoModeOS(_GLFWmonitor* monitor, GLFWvidmode* mode);
GLFWbool _glfwGetGammaRampOS(_GLFWmonitor* monitor, GLFWgammaramp* ramp);
void _glfwSetGammaRampOS(_GLFWmonitor* monitor, const GLFWgammaramp* ramp);

void _glfwPollMonitors(void);
void _glfwSetVideoMode(_GLFWmonitor* monitor, const GLFWvidmode* desired);
void _glfwRestoreVideoMode(_GLFWmonitor* monitor);

Cursor _glfwCreateNativeCursor(const GLFWimage* image, int xhot, int yhot);

unsigned long _glfwGetWindowProperty(Window window,
                                        Atom property,
                                        Atom type,
                                        unsigned char** value);
GLFWbool _glfwIsVisualTransparent(Visual* visual);

void _glfwGrabErrorHandler(void);
void _glfwReleaseErrorHandler(void);
void _glfwInputErrorX11(int error, const char* message);

void _glfwPushSelectionToManager(void);
void _glfwCreateInputContext(_GLFWwindow* window);



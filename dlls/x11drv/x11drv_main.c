/*
 * X11DRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/cursorfont.h>
#include "ts_xlib.h"
#include "ts_xutil.h"

#include "winbase.h"

#include "callback.h"
#include "clipboard.h"
#include "debugtools.h"
#include "gdi.h"
#include "monitor.h"
#include "options.h"
#include "user.h"
#include "win.h"
#include "x11drv.h"


static USER_DRIVER user_driver =
{
    /* event functions */
    X11DRV_EVENT_Synchronize,
    X11DRV_EVENT_CheckFocus,
    X11DRV_EVENT_UserRepaintDisable,
    /* keyboard functions */
    X11DRV_KEYBOARD_Init,
    X11DRV_KEYBOARD_VkKeyScan,
    X11DRV_KEYBOARD_MapVirtualKey,
    X11DRV_KEYBOARD_GetKeyNameText,
    X11DRV_KEYBOARD_ToAscii,
    X11DRV_KEYBOARD_GetBeepActive,
    X11DRV_KEYBOARD_SetBeepActive,
    X11DRV_KEYBOARD_Beep,
    X11DRV_KEYBOARD_GetDIState,
    X11DRV_KEYBOARD_GetDIData,
    X11DRV_KEYBOARD_GetKeyboardConfig,
    X11DRV_KEYBOARD_SetKeyboardConfig,
    /* mouse functions */
    X11DRV_MOUSE_Init,
    X11DRV_MOUSE_SetCursor,
    X11DRV_MOUSE_MoveCursor,
    X11DRV_MOUSE_EnableWarpPointer,
    /* screen saver functions */
    X11DRV_GetScreenSaveActive,
    X11DRV_SetScreenSaveActive,
    X11DRV_GetScreenSaveTimeout,
    X11DRV_SetScreenSaveTimeout,
    /* windowing functions */
    X11DRV_IsSingleWindow
};

static XKeyboardState keyboard_state;

Display *display;
Screen *screen;
Visual *visual;
int screen_depth;
Window root_window;

/***********************************************************************
 *		error_handler
 */
static int error_handler(Display *display, XErrorEvent *error_evt)
{
    DebugBreak();  /* force an entry in the debugger */
    return 0;
}


/***********************************************************************
 *		create_desktop
 *
 * Create the desktop window for the --desktop mode.
 */
static void create_desktop( const char *geometry )
{
    int x = 0, y = 0, flags;
    unsigned int width = 640, height = 480;  /* Default size = 640x480 */
    char *name = "Wine desktop";
    XSizeHints *size_hints;
    XWMHints   *wm_hints;
    XClassHint *class_hints;
    XSetWindowAttributes win_attr;
    XTextProperty window_name;
    Atom XA_WM_DELETE_WINDOW;

    flags = TSXParseGeometry( geometry, &x, &y, &width, &height );
    MONITOR_PrimaryMonitor.rect.right  = width;
    MONITOR_PrimaryMonitor.rect.bottom = height;

    /* Create window */
  
    win_attr.background_pixel = BlackPixel(display, 0);
    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                          PointerMotionMask | ButtonPressMask |
                          ButtonReleaseMask | EnterWindowMask;
    win_attr.cursor = TSXCreateFontCursor( display, XC_top_left_arrow );

    root_window = TSXCreateWindow( display, DefaultRootWindow(display),
                                   x, y, width, height, 0,
                                   CopyFromParent, InputOutput, CopyFromParent,
                                   CWBackPixel | CWEventMask | CWCursor, &win_attr);
  
    /* Set window manager properties */

    size_hints  = TSXAllocSizeHints();
    wm_hints    = TSXAllocWMHints();
    class_hints = TSXAllocClassHint();
    if (!size_hints || !wm_hints || !class_hints)
    {
        MESSAGE("Not enough memory for window manager hints.\n" );
        ExitProcess(1);
    }
    size_hints->min_width = size_hints->max_width = width;
    size_hints->min_height = size_hints->max_height = height;
    size_hints->flags = PMinSize | PMaxSize;
    if (flags & (XValue | YValue)) size_hints->flags |= USPosition;
    if (flags & (WidthValue | HeightValue)) size_hints->flags |= USSize;
    else size_hints->flags |= PSize;

    wm_hints->flags = InputHint | StateHint;
    wm_hints->input = True;
    wm_hints->initial_state = NormalState;
    class_hints->res_name = (char *)argv0;
    class_hints->res_class = "Wine";

    TSXStringListToTextProperty( &name, 1, &window_name );
    TSXSetWMProperties( display, root_window, &window_name, &window_name,
                        Options.argv, Options.argc, size_hints, wm_hints, class_hints );
    XA_WM_DELETE_WINDOW = TSXInternAtom( display, "WM_DELETE_WINDOW", False );
    TSXSetWMProtocols( display, root_window, &XA_WM_DELETE_WINDOW, 1 );
    TSXFree( size_hints );
    TSXFree( wm_hints );
    TSXFree( class_hints );

    /* Map window */

    TSXMapWindow( display, root_window );
}

/* Created so that XOpenIM can be called using the 'large stack' */
static void XOpenIM_large_stack(void)
{
  TSXOpenIM(display,NULL,NULL,NULL);
}

/***********************************************************************
 *           X11DRV process initialisation routine
 */
static void process_attach(void)
{
    USER_Driver      = &user_driver;
    CLIPBOARD_Driver = &X11DRV_CLIPBOARD_Driver;
    WND_Driver       = &X11DRV_WND_Driver;

    /* Open display */
  
    if (!(display = TSXOpenDisplay( Options.display )))
    {
        MESSAGE( "%s: Can't open display: %s\n",
                 argv0, Options.display ? Options.display : "(none specified)" );
        ExitProcess(1);
    }
    fcntl( ConnectionNumber(display), F_SETFD, 1 ); /* set close on exec flag */
    screen = DefaultScreenOfDisplay( display );
    visual = DefaultVisual( display, DefaultScreen(display) );
    root_window = DefaultRootWindow( display );

    /* Initialize screen depth */

    screen_depth = PROFILE_GetWineIniInt( "x11drv", "ScreenDepth", 0 );
    if (screen_depth)  /* depth specified */
    {
        int depth_count, i;
        int *depth_list = TSXListDepths(display, DefaultScreen(display), &depth_count);
        for (i = 0; i < depth_count; i++)
            if (depth_list[i] == screen_depth) break;
        TSXFree( depth_list );
        if (i >= depth_count)
	{
            MESSAGE( "%s: Depth %d not supported on this screen.\n", argv0, screen_depth );
            ExitProcess(1);
	}
    }
    else screen_depth = DefaultDepthOfScreen( screen );

    /* tell the libX11 that we will do input method handling ourselves
     * that keep libX11 from doing anything whith dead keys, allowing Wine
     * to have total control over dead keys, that is this line allows
     * them to work in Wine, even whith a libX11 including the dead key
     * patches from Th.Quinot (http://Web.FdN.FR/~tquinot/dead-keys.en.html)
     */
    CALL_LARGE_STACK( XOpenIM_large_stack, NULL );

    if (Options.synchronous) XSetErrorHandler( error_handler );

    MONITOR_PrimaryMonitor.rect.left   = 0;
    MONITOR_PrimaryMonitor.rect.top    = 0;
    MONITOR_PrimaryMonitor.rect.right  = WidthOfScreen( screen );
    MONITOR_PrimaryMonitor.rect.bottom = HeightOfScreen( screen );
    MONITOR_PrimaryMonitor.depth       = screen_depth;

    if (Options.desktopGeometry)
    {
        Options.managed = FALSE;
        create_desktop( Options.desktopGeometry );
    }

    /* initialize GDI */
    X11DRV_GDI_Initialize();

    /* save keyboard setup */
    TSXGetKeyboardControl(display, &keyboard_state);

    /* initialize event handling */
    X11DRV_EVENT_Init();
}


/***********************************************************************
 *           X11DRV process termination routine
 */
static void process_detach(void)
{
    /* restore keyboard setup */
    XKeyboardControl keyboard_value;
  
    keyboard_value.key_click_percent = keyboard_state.key_click_percent;
    keyboard_value.bell_percent      = keyboard_state.bell_percent;
    keyboard_value.bell_pitch        = keyboard_state.bell_pitch;
    keyboard_value.bell_duration     = keyboard_state.bell_duration;
    keyboard_value.auto_repeat_mode  = keyboard_state.global_auto_repeat;
  
    XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent | 
                           KBBellPitch | KBBellDuration | KBAutoRepeatMode, &keyboard_value);

    /* cleanup GDI */
    X11DRV_GDI_Finalize();

#if 0  /* FIXME */

    /* close the display */
    XCloseDisplay( display );
    display = NULL;

    USER_Driver      = NULL;
    CLIPBOARD_Driver = NULL;
    WND_Driver       = NULL;
#endif
}


/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI X11DRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    static int process_count;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!process_count++) process_attach();
        break;
    case DLL_PROCESS_DETACH:
        if (!--process_count) process_detach();
        break;
    }
    return TRUE;
}

/***********************************************************************
 *              X11DRV_GetScreenSaveActive
 *
 * Returns the active status of the screen saver
 */
BOOL X11DRV_GetScreenSaveActive(void)
{
    int timeout, temp;
    TSXGetScreenSaver(display, &timeout, &temp, &temp, &temp);
    return timeout != 0;
}

/***********************************************************************
 *              X11DRV_SetScreenSaveActive
 *
 * Activate/Deactivate the screen saver
 */
void X11DRV_SetScreenSaveActive(BOOL bActivate)
{
    if(bActivate)
        TSXActivateScreenSaver(display);
    else
        TSXResetScreenSaver(display);
}

/***********************************************************************
 *              X11DRV_GetScreenSaveTimeout
 *
 * Return the screen saver timeout
 */
int X11DRV_GetScreenSaveTimeout(void)
{
    int timeout, temp;
    TSXGetScreenSaver(display, &timeout, &temp, &temp, &temp);
    return timeout;
}

/***********************************************************************
 *              X11DRV_SetScreenSaveTimeout
 *
 * Set the screen saver timeout
 */
void X11DRV_SetScreenSaveTimeout(int nTimeout)
{
    TSXSetScreenSaver(display, nTimeout, 60, DefaultBlanking, DefaultExposures);
}

/***********************************************************************
 *              X11DRV_IsSingleWindow
 */
BOOL X11DRV_IsSingleWindow(void)
{
    return (root_window != DefaultRootWindow(display));
}

/*
 * X11DRV desktop window handling
 *
 * Copyright 2001 Alexandre Julliard
 */

#include "config.h"
#include <X11/cursorfont.h>

#include "ts_xlib.h"

#include "wine/winuser16.h"
#include "win.h"
#include "x11drv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);


/* desktop window procedure */
static LRESULT WINAPI desktop_winproc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch(message)
    {
    case WM_ERASEBKGND:
        PaintDesktop( (HDC)wParam );
        ValidateRect( hwnd, NULL );
        break;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE) ExitWindows( 0, 0 );
        break;

    case WM_SETCURSOR:
        return SetCursor( LoadCursorA( 0, IDC_ARROWA ) );

    case WM_NCHITTEST:
        return HTCLIENT;
    }
    return 0;
}


/* desktop window manager thread */
static DWORD CALLBACK desktop_thread( LPVOID driver_data )
{
    Display *display;
    MSG msg;
    HWND hwnd;
    WND *win;

    NtCurrentTeb()->driver_data = driver_data;
    display = thread_display();
    hwnd = GetDesktopWindow();

    /* patch the desktop window queue to point to our queue */
    win = WIN_FindWndPtr( hwnd );
    win->hmemTaskQ = InitThreadInput16( 0, 0 );
    X11DRV_register_window( display, hwnd, win->pDriverData );
    WIN_ReleaseWndPtr( win );

    SetWindowLongW( hwnd, GWL_WNDPROC, (LONG)desktop_winproc );
    wine_tsx11_lock();
    XSetWMProtocols( display, root_window, &wmDeleteWindow, 1 );
    XMapWindow( display, root_window );
    wine_tsx11_unlock();

    while (GetMessageW( &msg, hwnd, 0, 0 )) DispatchMessageW( &msg );
    return 0;
}


/***********************************************************************
 *		X11DRV_create_desktop_thread
 *
 * Create the thread that manages the desktop window
 */
void X11DRV_create_desktop_thread(void)
{
    HANDLE handle = CreateThread( NULL, 0, desktop_thread, NtCurrentTeb()->driver_data, 0, NULL );
    if (!handle)
    {
        MESSAGE( "Could not create desktop thread\n" );
        ExitProcess(1);
    }
    /* we transferred our driver data to the new thread */
    NtCurrentTeb()->driver_data = NULL;
    CloseHandle( handle );
}


/***********************************************************************
 *		X11DRV_create_desktop
 *
 * Create the X11 desktop window for the desktop mode.
 */
Window X11DRV_create_desktop( XVisualInfo *desktop_vi, const char *geometry )
{
    int x = 0, y = 0, flags;
    unsigned int width = 640, height = 480;  /* Default size = 640x480 */
    char *name = "Wine desktop";
    XSizeHints *size_hints;
    XWMHints   *wm_hints;
    XClassHint *class_hints;
    XSetWindowAttributes win_attr;
    XTextProperty window_name;
    Window win;
    Display *display = thread_display();

    wine_tsx11_lock();
    flags = XParseGeometry( geometry, &x, &y, &width, &height );
    screen_width  = width;
    screen_height = height;

    /* Create window */
    win_attr.background_pixel = BlackPixel(display, 0);
    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                          PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
    win_attr.cursor = XCreateFontCursor( display, XC_top_left_arrow );

    if (desktop_vi)
        win_attr.colormap = XCreateColormap( display, DefaultRootWindow(display),
                                             visual, AllocNone );
    else
        win_attr.colormap = None;

    win = XCreateWindow( display, DefaultRootWindow(display),
                         x, y, width, height, 0, screen_depth, InputOutput, visual,
                         CWBackPixel | CWEventMask | CWCursor | CWColormap, &win_attr );

    /* Set window manager properties */
    size_hints  = XAllocSizeHints();
    wm_hints    = XAllocWMHints();
    class_hints = XAllocClassHint();
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
    class_hints->res_name  = "wine";
    class_hints->res_class = "Wine";

    XStringListToTextProperty( &name, 1, &window_name );
    XSetWMProperties( display, win, &window_name, &window_name,
                      NULL, 0, size_hints, wm_hints, class_hints );
    XFree( size_hints );
    XFree( wm_hints );
    XFree( class_hints );
    XFlush( display );
    wine_tsx11_unlock();
    return win;
}

/*
 * X11DRV desktop window handling
 *
 * Copyright 2001 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <X11/cursorfont.h>
#include <X11/Xlib.h>

#include "wine/winuser16.h"
#include "win.h"
#include "ddrawi.h"
#include "x11drv.h"
#include "x11ddraw.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);


/* desktop window procedure */
static LRESULT WINAPI desktop_winproc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch(message)
    {
    case WM_NCCREATE:
        SystemParametersInfoA( SPI_SETDESKPATTERN, -1, NULL, FALSE );
        SetDeskWallPaper( (LPSTR)-1 );
        return TRUE;

    case WM_ERASEBKGND:
        PaintDesktop( (HDC)wParam );
        return TRUE;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE) ExitWindows( 0, 0 );
        break;

    case WM_SETCURSOR:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_ARROW ) );

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
    Atom atom = x11drv_atom(WM_DELETE_WINDOW);

    TlsSetValue( thread_data_tls_index, driver_data );
    display = thread_display();
    hwnd = GetDesktopWindow();

    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)desktop_winproc );
    wine_tsx11_lock();
    XSaveContext( display, root_window, winContext, (char *)hwnd );
    XChangeProperty ( display, root_window, x11drv_atom(WM_PROTOCOLS),
                      XA_ATOM, 32, PropModeReplace, (unsigned char *)&atom, 1 );
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
    HANDLE handle = CreateThread( NULL, 0, desktop_thread,
                                  TlsGetValue( thread_data_tls_index ), 0, &desktop_tid );
    if (!handle)
    {
        MESSAGE( "Could not create desktop thread\n" );
        ExitProcess(1);
    }
    /* we transferred our driver data to the new thread */
    TlsSetValue( thread_data_tls_index, NULL );
    CloseHandle( handle );
}


/* data for resolution changing */
static LPDDHALMODEINFO dd_modes;
static unsigned int dd_mode_count;

static unsigned int max_width;
static unsigned int max_height;

static const unsigned int widths[]  = {320, 400, 512, 640, 800, 1024, 1152, 1280, 1400, 1600};
static const unsigned int heights[] = {200, 300, 384, 480, 600,  768,  864, 1024, 1050, 1200};
#define NUM_DESKTOP_MODES (sizeof(widths) / sizeof(widths[0]))

/* create the mode structures */
static void make_modes(void)
{
    int i;
    /* original specified desktop size */
    X11DRV_Settings_AddOneMode(screen_width, screen_height, 0, 0);
    for (i=0; i<NUM_DESKTOP_MODES; i++)
    {
        if ( (widths[i] <= max_width) && (heights[i] <= max_height) )
        {
            if ( ( (widths[i] != max_width) || (heights[i] != max_height) ) &&
                 ( (widths[i] != screen_width) || (heights[i] != screen_height) ) )
            {
                /* only add them if they are smaller than the root window and unique */
                X11DRV_Settings_AddOneMode(widths[i], heights[i], 0, 0);
            }
        }
    }
    if ((max_width != screen_width) && (max_height != screen_height))
    {
        /* root window size (if different from desktop window) */
        X11DRV_Settings_AddOneMode(max_width, max_height, 0, 0);
    }
}

/***********************************************************************
 *		X11DRV_resize_desktop
 *
 * Reset the desktop window size and WM hints
 */
static int X11DRV_resize_desktop( unsigned int width, unsigned int height )
{
    XSizeHints *size_hints;
    Display *display = thread_display();
    Window w = root_window;
    /* set up */
    wine_tsx11_lock();
    size_hints  = XAllocSizeHints();
    if (!size_hints)
    {
        ERR("Not enough memory for window manager hints.\n" );
        wine_tsx11_unlock();
        return 0;
    }
    size_hints->min_width = size_hints->max_width = width;
    size_hints->min_height = size_hints->max_height = height;
    size_hints->flags = PMinSize | PMaxSize | PSize;

    /* do the work */
    XSetWMNormalHints( display, w, size_hints );
    XResizeWindow( display, w, width, height );

    /* clean up */
    XFree( size_hints );
    XFlush( display );
    wine_tsx11_unlock();
    X11DRV_handle_desktop_resize( width, height );
    return 1;
}

static int X11DRV_desktop_GetCurrentMode(void)
{
    unsigned int i;
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    for (i=0; i<dd_mode_count; i++)
    {
        if ( (screen_width == dd_modes[i].dwWidth) &&
             (screen_height == dd_modes[i].dwHeight) && 
             (dwBpp == dd_modes[i].dwBPP))
            return i;
    }
    ERR("In unknown mode, returning default\n");
    return 0;
}

static void X11DRV_desktop_SetCurrentMode(int mode)
{
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    TRACE("Resizing Wine desktop window to %ldx%ld\n", dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    X11DRV_resize_desktop(dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    if (dwBpp != dd_modes[mode].dwBPP)
    {
        FIXME("Cannot change screen BPP from %ld to %ld\n", dwBpp, dd_modes[mode].dwBPP);
    }
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
    char *name = GetCommandLineA();
    XSizeHints *size_hints;
    XWMHints   *wm_hints;
    XClassHint *class_hints;
    XSetWindowAttributes win_attr;
    XTextProperty window_name;
    Window win;
    Display *display = thread_display();

    wine_tsx11_lock();
    flags = XParseGeometry( geometry, &x, &y, &width, &height );
    max_width = screen_width;
    max_height = screen_height;
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
    /* initialize the available resolutions */
    dd_modes = X11DRV_Settings_SetHandlers("desktop", 
                                           X11DRV_desktop_GetCurrentMode, 
                                           X11DRV_desktop_SetCurrentMode, 
                                           NUM_DESKTOP_MODES+2, 1);
    make_modes();
    X11DRV_Settings_AddDepthModes();
    dd_mode_count = X11DRV_Settings_GetModeCount();
    X11DRV_Settings_SetDefaultMode(0);
    return win;
}

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

#include "ts_xlib.h"

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
        ValidateRect( hwnd, NULL );
        break;

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
    WND *win;

    NtCurrentTeb()->driver_data = driver_data;
    display = thread_display();
    hwnd = GetDesktopWindow();

    /* patch the desktop window queue to point to our queue */
    win = WIN_GetPtr( hwnd );
    win->tid = GetCurrentThreadId();
    X11DRV_register_window( display, hwnd, win->pDriverData );
    WIN_ReleasePtr( win );

    SetWindowLongW( hwnd, GWL_WNDPROC, (LONG)desktop_winproc );
    wine_tsx11_lock();
    XSetWMProtocols( display, root_window, &wmDeleteWindow, 1 );
    XMapWindow( display, root_window );
    wine_tsx11_unlock();

    SendMessageW( hwnd, WM_NCCREATE, 0, 0 /* should be CREATESTRUCT */ );

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
    HANDLE handle = CreateThread( NULL, 0, desktop_thread, NtCurrentTeb()->driver_data,
                                  0, &desktop_tid );
    if (!handle)
    {
        MESSAGE( "Could not create desktop thread\n" );
        ExitProcess(1);
    }
    /* we transferred our driver data to the new thread */
    NtCurrentTeb()->driver_data = NULL;
    CloseHandle( handle );
}


/* data for resolution changing */
static LPDDHALMODEINFO dd_modes;
static int nmodes;

static unsigned int max_width;
static unsigned int max_height;

static const unsigned int widths[]  = {320, 512, 640, 800, 1024, 1152, 1280, 1600};
static const unsigned int heights[] = {200, 384, 480, 600,  768,  864, 1024, 1200};
static const unsigned int depths[]  = {8, 16, 32};

/* fill in DD mode info for one mode*/
static void make_one_mode (LPDDHALMODEINFO info, unsigned int width, unsigned int height, unsigned int bpp)
{
    info->dwWidth        = width;
    info->dwHeight       = height;
    info->wRefreshRate   = 0;
    info->lPitch         = 0;
    info->dwBPP          = bpp;
    info->wFlags         = 0;
    info->dwRBitMask     = 0;
    info->dwGBitMask     = 0;
    info->dwBBitMask     = 0;
    info->dwAlphaBitMask = 0;
    TRACE("initialized mode %d: %dx%dx%d\n", nmodes, width, height, bpp);
}

/* create the mode structures */
static void make_modes(void)
{
    int i,j;
    int max_modes = (3+1)*(8+2);
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    nmodes = 0;
    dd_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * max_modes);
    /* original specified desktop size */
    make_one_mode(&dd_modes[nmodes++], screen_width, screen_height, dwBpp);
    for (i=0; i<8; i++)
    {
        if ( (widths[i] <= max_width) && (heights[i] <= max_height) )
        {
            if ( ( (widths[i] != max_width) || (heights[i] != max_height) ) &&
                 ( (widths[i] != screen_width) || (heights[i] != screen_height) ) )
            {
                /* only add them if they are smaller than the root window and unique */
                make_one_mode(&dd_modes[nmodes++], widths[i], heights[i], dwBpp);
            }
        }
    }
    if ((max_width != screen_width) && (max_height != screen_height))
    {
        /* root window size (if different from desktop window) */
        make_one_mode(&dd_modes[nmodes++], max_width, max_height, dwBpp);
    }
    max_modes = nmodes;
    for (j=0; j<3; j++)
    {
        if (depths[j] != dwBpp)
        {
            for (i=0; i < max_modes; i++)
            {
                make_one_mode(&dd_modes[nmodes++], dd_modes[i].dwWidth, dd_modes[i].dwHeight, depths[j]);
            }
        }
    }
}

/***********************************************************************
 *		X11DRV_resize_desktop
 *
 * Reset the desktop window size and WM hints
 */
int X11DRV_resize_desktop( unsigned int width, unsigned int height )
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
    screen_width  = width;
    screen_height = height;
#if 0 /* FIXME */
    SYSMETRICS_Set( SM_CXSCREEN, width );
    SYSMETRICS_Set( SM_CYSCREEN, height );
#else
    FIXME("Need to update SYSMETRICS after resizing display (now %dx%d)\n", 
          width, height);
#endif

    /* clean up */
    XFree( size_hints );
    XFlush( display );
    wine_tsx11_unlock();
    return 1;
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
    make_modes();
    return win;
}

void X11DRV_desktop_SetCurrentMode(int mode)
{
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    if (mode < nmodes)
    {
        TRACE("Resizing Wine desktop window to %ldx%ld\n", dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
        X11DRV_resize_desktop(dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
        if (dwBpp != dd_modes[mode].dwBPP)
        {
            FIXME("Cannot change screen BPP from %ld to %ld\n", dwBpp, dd_modes[mode].dwBPP);
        }
    }
}

int X11DRV_desktop_GetCurrentMode(void)
{
    int i;
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    for (i=0; i<nmodes; i++)
    {
        if ( (screen_width == dd_modes[i].dwWidth) &&
             (screen_height == dd_modes[i].dwHeight) && 
             (dwBpp == dd_modes[i].dwBPP))
            return i;
    }
    ERR("In unknown mode, returning default\n");
    return 0;
}

/* ChangeDisplaySettings and related functions */

/* implementation of EnumDisplaySettings for desktop */
BOOL X11DRV_desktop_EnumDisplaySettingsExW( LPCWSTR name, DWORD n, LPDEVMODEW devmode, DWORD flags)
{
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    devmode->dmDisplayFlags = 0;
    devmode->dmDisplayFrequency = 85;
    devmode->dmSize = sizeof(DEVMODEW);
    if (n==(DWORD)-1)
    {
        devmode->dmBitsPerPel = dwBpp;
        devmode->dmPelsHeight = screen_height;
        devmode->dmPelsWidth  = screen_width;
        devmode->dmFields = (DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL);
        TRACE("mode %ld (current) -- returning current %ldx%ldx%ldbpp\n", n,
              devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel);
        return TRUE;
    }
    else if (n==(DWORD)-2)
    {
        devmode->dmBitsPerPel = dwBpp;
        devmode->dmPelsHeight = dd_modes[0].dwHeight;
        devmode->dmPelsWidth  = dd_modes[0].dwWidth;
        devmode->dmFields = (DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL);
        TRACE("mode %ld (registry) -- returning default %ldx%ldx%ldbpp\n", n,
              devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel);
        return TRUE;
    } 
    else if (n < nmodes)
    {
        devmode->dmPelsWidth = dd_modes[n].dwWidth;
        devmode->dmPelsHeight = dd_modes[n].dwHeight;
        devmode->dmBitsPerPel = dd_modes[n].dwBPP;
        devmode->dmFields = (DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL);
        TRACE("mode %ld -- %ldx%ldx%ldbpp\n", n,
              devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel);
        return TRUE;
    }
    TRACE("mode %ld -- not present\n", n);
    return FALSE;
}

/* implementation of ChangeDisplaySettings for desktop */
LONG X11DRV_desktop_ChangeDisplaySettingsExW( LPCWSTR devname, LPDEVMODEW devmode,
                                      HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    DWORD i;
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    if (devmode==NULL)
    {
        X11DRV_desktop_SetCurrentMode(0);
        return DISP_CHANGE_SUCCESSFUL;
    }

    for (i = 0; i < nmodes; i++)
    {
        if (devmode->dmFields & DM_BITSPERPEL)
        {
            if (devmode->dmBitsPerPel != dd_modes[i].dwBPP)
                continue;
        }
        if (devmode->dmFields & DM_PELSWIDTH)
        {
            if (devmode->dmPelsWidth != dd_modes[i].dwWidth)
                continue;
        }
        if (devmode->dmFields & DM_PELSHEIGHT)
        {
            if (devmode->dmPelsHeight != dd_modes[i].dwHeight)
                continue;
        }
        /* we have a valid mode */
        TRACE("Requested display settings match mode %ld\n", i);
        X11DRV_desktop_SetCurrentMode(i);
        return DISP_CHANGE_SUCCESSFUL;
    }

    /* no valid modes found */
    ERR("No matching mode found!\n");
    return DISP_CHANGE_BADMODE;
}

/* DirectDraw HAL stuff */

static DWORD PASCAL X11DRV_desktop_SetMode(LPDDHAL_SETMODEDATA data)
{
    TRACE("Mode %ld requested by DDHAL\n", data->dwModeIndex);
    X11DRV_desktop_SetCurrentMode(data->dwModeIndex);
    X11DRV_DDHAL_SwitchMode(data->dwModeIndex, NULL, NULL);
    data->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

int X11DRV_desktop_CreateDriver(LPDDHALINFO info)
{
    if (!nmodes) return 0; /* no desktop */

    TRACE("Setting up Desktop mode for DDRAW\n");
    info->dwNumModes = nmodes;
    info->lpModeInfo = dd_modes;
    X11DRV_DDHAL_SwitchMode(X11DRV_desktop_GetCurrentMode(), NULL, NULL);
    info->lpDDCallbacks->SetMode = X11DRV_desktop_SetMode;
    return TRUE;
}

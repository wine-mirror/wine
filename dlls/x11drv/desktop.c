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

static const unsigned int widths[] =  {320, 640, 800, 1024, 1280, 1600};
static const unsigned int heights[] = {200, 480, 600,  768, 1024, 1200};

/* fill in DD mode info for one mode*/
static void make_one_mode (LPDDHALMODEINFO info, unsigned int width, unsigned int height)
{
    info->dwWidth        = width;
    info->dwHeight       = height;
    info->wRefreshRate   = 0;
    info->lPitch         = 0;
    info->dwBPP          = 0;
    info->wFlags         = 0;
    info->dwRBitMask     = 0;
    info->dwGBitMask     = 0;
    info->dwBBitMask     = 0;
    info->dwAlphaBitMask = 0;
    TRACE("initialized mode %dx%d\n", width, height);
}

/* create the mode structures */
static void make_modes(void)
{
    int i;
    nmodes = 2;
    for (i=0; i<6; i++)
    {
        if ( (widths[i] <= max_width) && (heights[i] <= max_height) ) nmodes++;
    }
    dd_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * nmodes);
    /* mode 0 is the original specified desktop size */
    make_one_mode(&dd_modes[0], screen_width, screen_height);
    /* mode 1 is the root window size */
    make_one_mode(&dd_modes[1], max_width, max_height);
    /* these modes are all the standard modes smaller than the root window */
    for (i=2; i<nmodes; i++)
    {
        make_one_mode(&dd_modes[i], widths[i-2], heights[i-2]);
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
    if (mode < nmodes)
    {
        X11DRV_resize_desktop(dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    }
}

int X11DRV_desktop_GetCurrentMode(void)
{
    int i;
    for (i=0; i<nmodes; i++)
    {
        if ( (screen_width == dd_modes[i].dwWidth) &&
             (screen_height == dd_modes[i].dwHeight) )
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
    if (n==0 || n == (DWORD)-1 || n == (DWORD)-2)
    {
        devmode->dmBitsPerPel = dwBpp;
        devmode->dmPelsHeight = dd_modes[0].dwHeight;
        devmode->dmPelsWidth  = dd_modes[0].dwWidth;
        devmode->dmFields = (DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL);
        TRACE("mode %ld -- returning default %ldx%ldx%ldbpp\n", n,
              devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel);
        return TRUE;
    }
    if (n <= nmodes)
    {
        devmode->dmPelsWidth = dd_modes[n].dwWidth;
        devmode->dmPelsHeight = dd_modes[n].dwHeight;
        devmode->dmBitsPerPel = dwBpp;
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
            if (devmode->dmBitsPerPel != dwBpp)
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
        TRACE("Matches mode %ld\n", i);
        X11DRV_desktop_SetCurrentMode(i);
#if 0 /* FIXME */
        SYSMETRICS_Set( SM_CXSCREEN, devmode->dmPelsWidth );
        SYSMETRICS_Set( SM_CYSCREEN, devmode->dmPelsHeight );
#endif
        return DISP_CHANGE_SUCCESSFUL;
    }

    /* no valid modes found */
    ERR("No matching mode found!\n");
    return DISP_CHANGE_BADMODE;
}

/* DirectDraw HAL stuff */

static DWORD PASCAL X11DRV_desktop_SetMode(LPDDHAL_SETMODEDATA data)
{
    X11DRV_desktop_SetCurrentMode(data->dwModeIndex);
    X11DRV_DDHAL_SwitchMode(data->dwModeIndex, NULL, NULL);
    data->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

int X11DRV_desktop_CreateDriver(LPDDHALINFO info)
{
    if (!nmodes) return 0; /* no desktop */

    info->dwNumModes = nmodes;
    info->lpModeInfo = dd_modes;
    X11DRV_DDHAL_SwitchMode(X11DRV_desktop_GetCurrentMode(), NULL, NULL);
    info->lpDDCallbacks->SetMode = X11DRV_desktop_SetMode;
    return TRUE;
}

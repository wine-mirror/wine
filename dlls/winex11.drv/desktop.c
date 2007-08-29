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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <X11/cursorfont.h>
#include <X11/Xlib.h>

#include "win.h"
#include "ddrawi.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

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
    X11DRV_Settings_AddOneMode(screen_width, screen_height, 0, 60);
    for (i=0; i<NUM_DESKTOP_MODES; i++)
    {
        if ( (widths[i] <= max_width) && (heights[i] <= max_height) )
        {
            if ( ( (widths[i] != max_width) || (heights[i] != max_height) ) &&
                 ( (widths[i] != screen_width) || (heights[i] != screen_height) ) )
            {
                /* only add them if they are smaller than the root window and unique */
                X11DRV_Settings_AddOneMode(widths[i], heights[i], 0, 60);
            }
        }
    }
    if ((max_width != screen_width) && (max_height != screen_height))
    {
        /* root window size (if different from desktop window) */
        X11DRV_Settings_AddOneMode(max_width, max_height, 0, 60);
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

static LONG X11DRV_desktop_SetCurrentMode(int mode)
{
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    if (dwBpp != dd_modes[mode].dwBPP)
    {
        FIXME("Cannot change screen BPP from %d to %d\n", dwBpp, dd_modes[mode].dwBPP);
        /* Ignore the depth missmatch
         *
         * Some (older) applications require a specific bit depth, this will allow them
         * to run. X11drv performs a color depth conversion if needed.
         */
    }
    TRACE("Resizing Wine desktop window to %dx%d\n", dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    X11DRV_resize_desktop(dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    return DISP_CHANGE_SUCCESSFUL;
}

/***********************************************************************
 *		X11DRV_init_desktop
 *
 * Setup the desktop when not using the root window.
 */
void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height )
{
    root_window = win;
    max_width = screen_width;
    max_height = screen_height;
    screen_width  = width;
    screen_height = height;
    xinerama_init();

    /* initialize the available resolutions */
    dd_modes = X11DRV_Settings_SetHandlers("desktop", 
                                           X11DRV_desktop_GetCurrentMode, 
                                           X11DRV_desktop_SetCurrentMode, 
                                           NUM_DESKTOP_MODES+2, 1);
    make_modes();
    X11DRV_Settings_AddDepthModes();
    dd_mode_count = X11DRV_Settings_GetModeCount();
    X11DRV_Settings_SetDefaultMode(0);
}


/***********************************************************************
 *		X11DRV_create_desktop
 *
 * Create the X11 desktop window for the desktop mode.
 */
Window X11DRV_create_desktop( UINT width, UINT height )
{
    XSetWindowAttributes win_attr;
    Window win;
    Display *display = thread_display();

    wine_tsx11_lock();

    /* Create window */
    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                          PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
    win_attr.cursor = XCreateFontCursor( display, XC_top_left_arrow );

    if (visual != DefaultVisual( display, DefaultScreen(display) ))
        win_attr.colormap = XCreateColormap( display, DefaultRootWindow(display),
                                             visual, AllocNone );
    else
        win_attr.colormap = None;

    win = XCreateWindow( display, DefaultRootWindow(display),
                         0, 0, width, height, 0, screen_depth, InputOutput, visual,
                         CWEventMask | CWCursor | CWColormap, &win_attr );
    XFlush( display );
    wine_tsx11_unlock();
    if (win != None) X11DRV_init_desktop( win, width, height );
    return win;
}

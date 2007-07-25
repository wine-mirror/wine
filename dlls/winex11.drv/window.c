/*
 * Window related functions
 *
 * Copyright 1993, 1994, 1995, 1996, 2001 Alexandre Julliard
 * Copyright 1993 David Metcalfe
 * Copyright 1995, 1996 Alex Korobka
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"

#include "x11drv.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "win.h"
#include "mwm.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

/* X context to associate a struct x11drv_win_data to an hwnd */
static XContext win_data_context;

static const char whole_window_prop[] = "__wine_x11_whole_window";
static const char icon_window_prop[]  = "__wine_x11_icon_window";
static const char managed_prop[]      = "__wine_x11_managed";
static const char visual_id_prop[]    = "__wine_x11_visual_id";

/* for XDG systray icons */
#define SYSTEM_TRAY_REQUEST_DOCK    0

/***********************************************************************
 *		is_window_managed
 *
 * Check if a given window should be managed
 */
static inline BOOL is_window_managed( HWND hwnd )
{
    DWORD style, ex_style;

    if (!managed_mode) return FALSE;
    /* tray window is always managed */
    ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );
    if (ex_style & WS_EX_TRAYWINDOW) return TRUE;
    /* child windows are not managed */
    style = GetWindowLongW( hwnd, GWL_STYLE );
    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD) return FALSE;
    /* windows with caption are managed */
    if ((style & WS_CAPTION) == WS_CAPTION) return TRUE;
    /* tool windows are not managed  */
    if (ex_style & WS_EX_TOOLWINDOW) return FALSE;
    /* windows with thick frame are managed */
    if (style & WS_THICKFRAME) return TRUE;
    /* application windows are managed */
    if (ex_style & WS_EX_APPWINDOW) return TRUE;
    if (style & WS_POPUP)
    {
        RECT rect;

        /* popup with sysmenu == caption are managed */
        if (style & WS_SYSMENU) return TRUE;
        /* full-screen popup windows are managed */
        GetWindowRect( hwnd, &rect );
        if ((rect.right - rect.left) == screen_width && (rect.bottom - rect.top) == screen_height)
            return TRUE;
    }
    /* default: not managed */
    return FALSE;
}


/***********************************************************************
 *		X11DRV_is_window_rect_mapped
 *
 * Check if the X whole window should be mapped based on its rectangle
 */
BOOL X11DRV_is_window_rect_mapped( const RECT *rect )
{
    /* don't map if rect is empty */
    if (IsRectEmpty( rect )) return FALSE;

    /* don't map if rect is off-screen */
    if (rect->left >= virtual_screen_rect.right ||
        rect->top >= virtual_screen_rect.bottom ||
        rect->right <= virtual_screen_rect.left ||
        rect->bottom <= virtual_screen_rect.top)
        return FALSE;

    return TRUE;
}


/***********************************************************************
 *              get_window_attributes
 *
 * Fill the window attributes structure for an X window.
 */
static int get_window_attributes( Display *display, struct x11drv_win_data *data,
                                  XSetWindowAttributes *attr )
{
    if (!data->managed &&
        root_window == DefaultRootWindow( display ) &&
        data->whole_window != root_window &&
        is_window_managed( data->hwnd ))
    {
        data->managed = TRUE;
        SetPropA( data->hwnd, managed_prop, (HANDLE)1 );
    }
    attr->override_redirect = !data->managed;
    attr->colormap          = X11DRV_PALETTE_PaletteXColormap;
    attr->save_under        = ((GetClassLongW( data->hwnd, GCL_STYLE ) & CS_SAVEBITS) != 0);
    attr->cursor            = x11drv_thread_data()->cursor;
    return (CWOverrideRedirect | CWSaveUnder | CWColormap | CWCursor);
}


/***********************************************************************
 *              X11DRV_sync_window_style
 *
 * Change the X window attributes when the window style has changed.
 */
void X11DRV_sync_window_style( Display *display, struct x11drv_win_data *data )
{
    XSetWindowAttributes attr;
    int mask = get_window_attributes( display, data, &attr );

    wine_tsx11_lock();
    if (data->whole_window != DefaultRootWindow(display))
        XChangeWindowAttributes( display, data->whole_window, mask, &attr );
    wine_tsx11_unlock();
}


/***********************************************************************
 *              get_window_changes
 *
 * fill the window changes structure
 */
static int get_window_changes( XWindowChanges *changes, const RECT *old, const RECT *new )
{
    int mask = 0;

    if (old->right - old->left != new->right - new->left )
    {
        if (!(changes->width = new->right - new->left)) changes->width = 1;
        mask |= CWWidth;
    }
    if (old->bottom - old->top != new->bottom - new->top)
    {
        if (!(changes->height = new->bottom - new->top)) changes->height = 1;
        mask |= CWHeight;
    }
    if (old->left != new->left)
    {
        changes->x = new->left;
        mask |= CWX;
    }
    if (old->top != new->top)
    {
        changes->y = new->top;
        mask |= CWY;
    }
    return mask;
}


/***********************************************************************
 *              create_icon_window
 */
static Window create_icon_window( Display *display, struct x11drv_win_data *data )
{
    XSetWindowAttributes attr;

    attr.event_mask = (ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                       ButtonPressMask | ButtonReleaseMask | EnterWindowMask);
    attr.bit_gravity = NorthWestGravity;
    attr.backing_store = NotUseful/*WhenMapped*/;
    attr.colormap      = X11DRV_PALETTE_PaletteXColormap; /* Needed due to our visual */

    wine_tsx11_lock();
    data->icon_window = XCreateWindow( display, root_window, 0, 0,
                                       GetSystemMetrics( SM_CXICON ),
                                       GetSystemMetrics( SM_CYICON ),
                                       0, screen_depth,
                                       InputOutput, visual,
                                       CWEventMask | CWBitGravity | CWBackingStore | CWColormap, &attr );
    XSaveContext( display, data->icon_window, winContext, (char *)data->hwnd );
    wine_tsx11_unlock();

    TRACE( "created %lx\n", data->icon_window );
    SetPropA( data->hwnd, icon_window_prop, (HANDLE)data->icon_window );
    return data->icon_window;
}



/***********************************************************************
 *              destroy_icon_window
 */
static void destroy_icon_window( Display *display, struct x11drv_win_data *data )
{
    if (!data->icon_window) return;
    if (x11drv_thread_data()->cursor_window == data->icon_window)
        x11drv_thread_data()->cursor_window = None;
    wine_tsx11_lock();
    XDeleteContext( display, data->icon_window, winContext );
    XDestroyWindow( display, data->icon_window );
    data->icon_window = 0;
    wine_tsx11_unlock();
    RemovePropA( data->hwnd, icon_window_prop );
}


/***********************************************************************
 *              set_icon_hints
 *
 * Set the icon wm hints
 */
static void set_icon_hints( Display *display, struct x11drv_win_data *data,
                            XWMHints *hints, HICON hIcon )
{
    if (data->hWMIconBitmap) DeleteObject( data->hWMIconBitmap );
    if (data->hWMIconMask) DeleteObject( data->hWMIconMask);
    data->hWMIconBitmap = 0;
    data->hWMIconMask = 0;

    if (!data->managed)
    {
        destroy_icon_window( display, data );
        hints->flags &= ~(IconPixmapHint | IconMaskHint | IconWindowHint);
    }
    else if (!hIcon)
    {
        if (!data->icon_window) create_icon_window( display, data );
        hints->icon_window = data->icon_window;
        hints->flags = (hints->flags & ~(IconPixmapHint | IconMaskHint)) | IconWindowHint;
    }
    else
    {
        HBITMAP hbmOrig;
        RECT rcMask;
        BITMAP bmMask;
        ICONINFO ii;
        HDC hDC;

        GetIconInfo(hIcon, &ii);

        GetObjectA(ii.hbmMask, sizeof(bmMask), &bmMask);
        rcMask.top    = 0;
        rcMask.left   = 0;
        rcMask.right  = bmMask.bmWidth;
        rcMask.bottom = bmMask.bmHeight;

        hDC = CreateCompatibleDC(0);
        hbmOrig = SelectObject(hDC, ii.hbmMask);
        InvertRect(hDC, &rcMask);
        SelectObject(hDC, ii.hbmColor);  /* force the color bitmap to x11drv mode too */
        SelectObject(hDC, hbmOrig);
        DeleteDC(hDC);

        data->hWMIconBitmap = ii.hbmColor;
        data->hWMIconMask = ii.hbmMask;

        hints->icon_pixmap = X11DRV_get_pixmap(data->hWMIconBitmap);
        hints->icon_mask = X11DRV_get_pixmap(data->hWMIconMask);
        destroy_icon_window( display, data );
        hints->flags = (hints->flags & ~IconWindowHint) | IconPixmapHint | IconMaskHint;
    }
}

/***********************************************************************
 *              systray_dock_window
 *
 * Docks the given X window with the NETWM system tray.
 */
static void systray_dock_window( Display *display, struct x11drv_win_data *data )
{
    static Atom systray_atom;
    Window systray_window;

    wine_tsx11_lock();
    if (!systray_atom)
    {
        if (DefaultScreen( display ) == 0)
            systray_atom = x11drv_atom(_NET_SYSTEM_TRAY_S0);
        else
        {
            char systray_buffer[29]; /* strlen(_NET_SYSTEM_TRAY_S4294967295)+1 */
            sprintf( systray_buffer, "_NET_SYSTEM_TRAY_S%u", DefaultScreen( display ) );
            systray_atom = XInternAtom( display, systray_buffer, False );
        }
    }
    systray_window = XGetSelectionOwner( display, systray_atom );
    wine_tsx11_unlock();

    TRACE("Docking tray icon %p\n", data->hwnd);

    if (systray_window != None)
    {
        XEvent ev;
        unsigned long info[2];
                
        /* Put the window offscreen so it isn't mapped. The window _cannot_ be 
         * mapped if we intend to dock with an XEMBED tray. If the window is 
         * mapped when we dock, it may become visible as a child of the root 
         * window after it docks, which isn't the proper behavior. 
         *
         * For more information on this problem, see
         * http://standards.freedesktop.org/xembed-spec/latest/ar01s04.html */
        
        SetWindowPos( data->hwnd, NULL, virtual_screen_rect.right + 1, virtual_screen_rect.bottom + 1,
                      0, 0, SWP_NOZORDER | SWP_NOSIZE );

        /* set XEMBED protocol data on the window */
        info[0] = 0; /* protocol version */
        info[1] = 1; /* mapped = true */
        
        wine_tsx11_lock();
        XChangeProperty( display, data->whole_window,
                         x11drv_atom(_XEMBED_INFO),
                         x11drv_atom(_XEMBED_INFO), 32, PropModeReplace,
                         (unsigned char*)info, 2 );
        wine_tsx11_unlock();
    
        /* send the docking request message */
        ZeroMemory( &ev, sizeof(ev) ); 
        ev.xclient.type = ClientMessage;
        ev.xclient.window = systray_window;
        ev.xclient.message_type = x11drv_atom( _NET_SYSTEM_TRAY_OPCODE );
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = CurrentTime;
        ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
        ev.xclient.data.l[2] = data->whole_window;
        ev.xclient.data.l[3] = 0;
        ev.xclient.data.l[4] = 0;
        
        wine_tsx11_lock();
        XSendEvent( display, systray_window, False, NoEventMask, &ev );
        wine_tsx11_unlock();

    }
    else
    {
        int val = 1;

        /* fall back to he KDE hints if the WM doesn't support XEMBED'ed
         * systrays */
        
        wine_tsx11_lock();
        XChangeProperty( display, data->whole_window, 
                         x11drv_atom(KWM_DOCKWINDOW),
                         x11drv_atom(KWM_DOCKWINDOW), 32, PropModeReplace,
                         (unsigned char*)&val, 1 );
        XChangeProperty( display, data->whole_window,
                         x11drv_atom(_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR),
                         XA_WINDOW, 32, PropModeReplace,
                         (unsigned char*)&data->whole_window, 1 );
       wine_tsx11_unlock();
    }
}


/***********************************************************************
 *              set_size_hints
 *
 * set the window size hints
 */
static void set_size_hints( Display *display, struct x11drv_win_data *data, DWORD style )
{
    XSizeHints* size_hints;

    if ((size_hints = XAllocSizeHints()))
    {
        size_hints->flags = 0;

        if (data->hwnd != GetDesktopWindow())  /* don't force position of desktop */
        {
            size_hints->win_gravity = StaticGravity;
            size_hints->x = data->whole_rect.left;
            size_hints->y = data->whole_rect.top;
            size_hints->flags |= PWinGravity | PPosition;
        }

        if ( !(style & WS_THICKFRAME) )
        {
            size_hints->max_width = data->whole_rect.right - data->whole_rect.left;
            size_hints->max_height = data->whole_rect.bottom - data->whole_rect.top;
            size_hints->min_width = size_hints->max_width;
            size_hints->min_height = size_hints->max_height;
            size_hints->flags |= PMinSize | PMaxSize;
        }
        XSetWMNormalHints( display, data->whole_window, size_hints );
        XFree( size_hints );
    }
}


/***********************************************************************
 *              get_process_name
 *
 * get the name of the current process for setting class hints
 */
static char *get_process_name(void)
{
    static char *name;

    if (!name)
    {
        WCHAR module[MAX_PATH];
        DWORD len = GetModuleFileNameW( 0, module, MAX_PATH );
        if (len && len < MAX_PATH)
        {
            char *ptr;
            WCHAR *p, *appname = module;

            if ((p = strrchrW( appname, '/' ))) appname = p + 1;
            if ((p = strrchrW( appname, '\\' ))) appname = p + 1;
            len = WideCharToMultiByte( CP_UNIXCP, 0, appname, -1, NULL, 0, NULL, NULL );
            if ((ptr = HeapAlloc( GetProcessHeap(), 0, len )))
            {
                WideCharToMultiByte( CP_UNIXCP, 0, appname, -1, ptr, len, NULL, NULL );
                name = ptr;
            }
        }
    }
    return name;
}


/***********************************************************************
 *              X11DRV_set_wm_hints
 *
 * Set the window manager hints for a newly-created window
 */
void X11DRV_set_wm_hints( Display *display, struct x11drv_win_data *data )
{
    Window group_leader;
    XClassHint *class_hints;
    XWMHints* wm_hints;
    Atom protocols[3];
    Atom window_type;
    MwmHints mwm_hints;
    Atom dndVersion = 4;
    int i;
    DWORD style = GetWindowLongW( data->hwnd, GWL_STYLE );
    DWORD ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );
    HWND owner = GetWindow( data->hwnd, GW_OWNER );
    char *process_name = get_process_name();

    if (data->hwnd == GetDesktopWindow())
    {
        if (data->whole_window == DefaultRootWindow(display)) return;
        /* force some styles for the desktop to get the correct decorations */
        style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        owner = 0;
    }

    /* transient for hint */
    if (owner)
    {
        Window owner_win = X11DRV_get_whole_window( owner );
        wine_tsx11_lock();
        XSetTransientForHint( display, data->whole_window, owner_win );
        wine_tsx11_unlock();
        group_leader = owner_win;
    }
    else group_leader = data->whole_window;

    wine_tsx11_lock();

    /* wm protocols */
    i = 0;
    protocols[i++] = x11drv_atom(WM_DELETE_WINDOW);
    protocols[i++] = x11drv_atom(_NET_WM_PING);
    if (use_take_focus) protocols[i++] = x11drv_atom(WM_TAKE_FOCUS);
    XChangeProperty( display, data->whole_window, x11drv_atom(WM_PROTOCOLS),
                     XA_ATOM, 32, PropModeReplace, (unsigned char *)protocols, i );

    /* class hints */
    if ((class_hints = XAllocClassHint()))
    {
        static char wine[] = "Wine";

        class_hints->res_name = process_name;
        class_hints->res_class = wine;
        XSetClassHint( display, data->whole_window, class_hints );
        XFree( class_hints );
    }

    /* size hints */
    set_size_hints( display, data, style );

    /* set the WM_CLIENT_MACHINE and WM_LOCALE_NAME properties */
    XSetWMProperties(display, data->whole_window, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    /* set the pid. together, these properties are needed so the window manager can kill us if we freeze */
    i = getpid();
    XChangeProperty(display, data->whole_window, x11drv_atom(_NET_WM_PID),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&i, 1);

    /* set the WM_WINDOW_TYPE */
    window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_NORMAL);
    if (ex_style & WS_EX_TOOLWINDOW) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_UTILITY);
    else if (style & WS_THICKFRAME) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_NORMAL);
    else if (style & WS_DLGFRAME) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_DIALOG);
    else if (ex_style & WS_EX_DLGMODALFRAME) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_DIALOG);

    XChangeProperty(display, data->whole_window, x11drv_atom(_NET_WM_WINDOW_TYPE),
		    XA_ATOM, 32, PropModeReplace, (unsigned char*)&window_type, 1);

    mwm_hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    mwm_hints.functions = MWM_FUNC_MOVE;
    if (style & WS_THICKFRAME) mwm_hints.functions |= MWM_FUNC_RESIZE;
    if (style & WS_MINIMIZEBOX) mwm_hints.functions |= MWM_FUNC_MINIMIZE;
    if (style & WS_MAXIMIZEBOX) mwm_hints.functions |= MWM_FUNC_MAXIMIZE;
    if (style & WS_SYSMENU)    mwm_hints.functions |= MWM_FUNC_CLOSE;
    mwm_hints.decorations = 0;
    if ((style & WS_CAPTION) == WS_CAPTION) 
    {
	mwm_hints.decorations |= MWM_DECOR_TITLE;
	if (style & WS_SYSMENU) mwm_hints.decorations |= MWM_DECOR_MENU;
	if (style & WS_MINIMIZEBOX) mwm_hints.decorations |= MWM_DECOR_MINIMIZE;
	if (style & WS_MAXIMIZEBOX) mwm_hints.decorations |= MWM_DECOR_MAXIMIZE;
    }
    if (ex_style & WS_EX_DLGMODALFRAME) mwm_hints.decorations |= MWM_DECOR_BORDER;
    else if (style & WS_THICKFRAME) mwm_hints.decorations |= MWM_DECOR_BORDER | MWM_DECOR_RESIZEH;
    else if ((style & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME) mwm_hints.decorations |= MWM_DECOR_BORDER;
    else if (style & WS_BORDER) mwm_hints.decorations |= MWM_DECOR_BORDER;
    else if (!(style & (WS_CHILD|WS_POPUP))) mwm_hints.decorations |= MWM_DECOR_BORDER;

    XChangeProperty( display, data->whole_window, x11drv_atom(_MOTIF_WM_HINTS),
                     x11drv_atom(_MOTIF_WM_HINTS), 32, PropModeReplace,
                     (unsigned char*)&mwm_hints, sizeof(mwm_hints)/sizeof(long) );

    XChangeProperty( display, data->whole_window, x11drv_atom(XdndAware),
                     XA_ATOM, 32, PropModeReplace, (unsigned char*)&dndVersion, 1 );

    wm_hints = XAllocWMHints();
    wine_tsx11_unlock();

    /* wm hints */
    if (wm_hints)
    {
        wm_hints->flags = InputHint | StateHint | WindowGroupHint;
        wm_hints->input = !(style & WS_DISABLED);

        set_icon_hints( display, data, wm_hints,
                        (HICON)GetClassLongPtrW( data->hwnd, GCLP_HICON ) );

        wm_hints->initial_state = (style & WS_MINIMIZE) ? IconicState : NormalState;
        wm_hints->window_group = group_leader;

        wine_tsx11_lock();
        XSetWMHints( display, data->whole_window, wm_hints );
        XFree(wm_hints);
        wine_tsx11_unlock();
    }
}


/***********************************************************************
 *              X11DRV_set_iconic_state
 *
 * Set the X11 iconic state according to the window style.
 */
void X11DRV_set_iconic_state( HWND hwnd )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    RECT rect;
    XWMHints* wm_hints;
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL iconic = (style & WS_MINIMIZE) != 0;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->whole_window || data->whole_window == DefaultRootWindow(display)) return;

    GetWindowRect( hwnd, &rect );

    wine_tsx11_lock();

    if (!(wm_hints = XGetWMHints( display, data->whole_window ))) wm_hints = XAllocWMHints();
    wm_hints->flags |= StateHint | IconPositionHint;
    wm_hints->initial_state = iconic ? IconicState : NormalState;
    wm_hints->icon_x = rect.left - virtual_screen_rect.left;
    wm_hints->icon_y = rect.top - virtual_screen_rect.top;
    XSetWMHints( display, data->whole_window, wm_hints );

    if (style & WS_VISIBLE)
    {
        if (iconic)
            XIconifyWindow( display, data->whole_window, DefaultScreen(display) );
        else
            if (X11DRV_is_window_rect_mapped( &rect ))
                XMapWindow( display, data->whole_window );
    }

    XFree(wm_hints);
    wine_tsx11_unlock();
}


/***********************************************************************
 *		X11DRV_window_to_X_rect
 *
 * Convert a rect from client to X window coordinates
 */
void X11DRV_window_to_X_rect( struct x11drv_win_data *data, RECT *rect )
{
    RECT rc;

    if (!data->managed) return;
    if (IsRectEmpty( rect )) return;

    rc.top = rc.bottom = rc.left = rc.right = 0;

    AdjustWindowRectEx( &rc, GetWindowLongW( data->hwnd, GWL_STYLE ) & ~(WS_HSCROLL|WS_VSCROLL),
                        FALSE, GetWindowLongW( data->hwnd, GWL_EXSTYLE ) );

    rect->left   -= rc.left;
    rect->right  -= rc.right;
    rect->top    -= rc.top;
    rect->bottom -= rc.bottom;
    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *		X11DRV_X_to_window_rect
 *
 * Opposite of X11DRV_window_to_X_rect
 */
void X11DRV_X_to_window_rect( struct x11drv_win_data *data, RECT *rect )
{
    if (!data->managed) return;
    if (IsRectEmpty( rect )) return;

    AdjustWindowRectEx( rect, GetWindowLongW( data->hwnd, GWL_STYLE ) & ~(WS_HSCROLL|WS_VSCROLL),
                        FALSE, GetWindowLongW( data->hwnd, GWL_EXSTYLE ));

    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *		X11DRV_sync_window_position
 *
 * Synchronize the X window position with the Windows one
 */
void X11DRV_sync_window_position( Display *display, struct x11drv_win_data *data,
                                  UINT swp_flags, const RECT *new_client_rect,
                                  const RECT *new_whole_rect )
{
    XWindowChanges changes;
    int mask;
    RECT old_whole_rect;

    old_whole_rect = data->whole_rect;
    data->whole_rect = *new_whole_rect;

    data->client_rect = *new_client_rect;
    OffsetRect( &data->client_rect, -data->whole_rect.left, -data->whole_rect.top );

    if (!data->whole_window || data->lock_changes) return;

    mask = get_window_changes( &changes, &old_whole_rect, &data->whole_rect );

    if (!(swp_flags & SWP_NOZORDER))
    {
        /* find window that this one must be after */
        HWND prev = GetWindow( data->hwnd, GW_HWNDPREV );
        while (prev && !(GetWindowLongW( prev, GWL_STYLE ) & WS_VISIBLE))
            prev = GetWindow( prev, GW_HWNDPREV );
        if (!prev)  /* top child */
        {
            changes.stack_mode = Above;
            mask |= CWStackMode;
        }
        else
        {
            /* should use stack_mode Below but most window managers don't get it right */
            /* so move it above the next one in Z order */
            HWND next = GetWindow( data->hwnd, GW_HWNDNEXT );
            while (next && !(GetWindowLongW( next, GWL_STYLE ) & WS_VISIBLE))
                next = GetWindow( next, GW_HWNDNEXT );
            if (next)
            {
                changes.stack_mode = Above;
                changes.sibling = X11DRV_get_whole_window(next);
                mask |= CWStackMode | CWSibling;
            }
        }
    }

    if (mask)
    {
        DWORD style = GetWindowLongW( data->hwnd, GWL_STYLE );

        TRACE( "setting win %lx pos %d,%d,%dx%d after %lx changes=%x\n",
               data->whole_window, data->whole_rect.left, data->whole_rect.top,
               data->whole_rect.right - data->whole_rect.left,
               data->whole_rect.bottom - data->whole_rect.top, changes.sibling, mask );

        wine_tsx11_lock();
        if (mask & (CWWidth|CWHeight)) set_size_hints( display, data, style );
        if (mask & CWX) changes.x -= virtual_screen_rect.left;
        if (mask & CWY) changes.y -= virtual_screen_rect.top;
        XReconfigureWMWindow( display, data->whole_window,
                              DefaultScreen(display), mask, &changes );
        wine_tsx11_unlock();
    }
}


/**********************************************************************
 *		create_whole_window
 *
 * Create the whole X window for a given window
 */
static Window create_whole_window( Display *display, struct x11drv_win_data *data, DWORD style )
{
    int cx, cy, mask;
    XSetWindowAttributes attr;
    XIM xim;
    RECT rect;

    rect = data->window_rect;
    X11DRV_window_to_X_rect( data, &rect );

    if (!(cx = rect.right - rect.left)) cx = 1;
    if (!(cy = rect.bottom - rect.top)) cy = 1;

    mask = get_window_attributes( display, data, &attr );

    /* set the attributes that don't change over the lifetime of the window */
    attr.bit_gravity   = NorthWestGravity;
    attr.backing_store = NotUseful;
    attr.event_mask    = (ExposureMask | PointerMotionMask |
                          ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
                          KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                          FocusChangeMask | KeymapStateMask);
    mask |= CWBitGravity | CWBackingStore | CWEventMask;

    wine_tsx11_lock();

    data->whole_rect = rect;
    data->whole_window = XCreateWindow( display, root_window,
                                        rect.left - virtual_screen_rect.left,
                                        rect.top - virtual_screen_rect.top,
                                        cx, cy, 0, screen_depth, InputOutput,
                                        visual, mask, &attr );

    if (!data->whole_window)
    {
        wine_tsx11_unlock();
        return 0;
    }
    XSaveContext( display, data->whole_window, winContext, (char *)data->hwnd );

    /* non-maximized child must be at bottom of Z order */
    if ((style & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
    {
        XWindowChanges changes;
        changes.stack_mode = Below;
        XConfigureWindow( display, data->whole_window, CWStackMode, &changes );
    }
    wine_tsx11_unlock();

    xim = x11drv_thread_data()->xim;
    if (xim) data->xic = X11DRV_CreateIC( xim, display, data->whole_window );

    X11DRV_set_wm_hints( display, data );

    SetPropA( data->hwnd, whole_window_prop, (HANDLE)data->whole_window );
    return data->whole_window;
}


/**********************************************************************
 *		destroy_whole_window
 *
 * Destroy the whole X window for a given window.
 */
static void destroy_whole_window( Display *display, struct x11drv_win_data *data )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();

    if (!data->whole_window) return;

    TRACE( "win %p xwin %lx\n", data->hwnd, data->whole_window );
    if (thread_data->cursor_window == data->whole_window) thread_data->cursor_window = None;
    wine_tsx11_lock();
    XDeleteContext( display, data->whole_window, winContext );
    if (data->whole_window != DefaultRootWindow(display))
        XDestroyWindow( display, data->whole_window );
    data->whole_window = 0;
    if (data->xic)
    {
        XUnsetICFocus( data->xic );
        XDestroyIC( data->xic );
    }
    /* Outlook stops processing messages after destroying a dialog, so we need an explicit flush */
    XFlush( display );
    wine_tsx11_unlock();
    RemovePropA( data->hwnd, whole_window_prop );
}


/*****************************************************************
 *		SetWindowText   (X11DRV.@)
 */
void X11DRV_SetWindowText( HWND hwnd, LPCWSTR text )
{
    Display *display = thread_display();
    UINT count;
    char *buffer;
    char *utf8_buffer;
    Window win;
    XTextProperty prop;

    if ((win = X11DRV_get_whole_window( hwnd )) && win != DefaultRootWindow(display))
    {
        /* allocate new buffer for window text */
        count = WideCharToMultiByte(CP_UNIXCP, 0, text, -1, NULL, 0, NULL, NULL);
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, count )))
        {
            ERR("Not enough memory for window text\n");
            return;
        }
        WideCharToMultiByte(CP_UNIXCP, 0, text, -1, buffer, count, NULL, NULL);

        count = WideCharToMultiByte(CP_UTF8, 0, text, strlenW(text), NULL, 0, NULL, NULL);
        if (!(utf8_buffer = HeapAlloc( GetProcessHeap(), 0, count )))
        {
            ERR("Not enough memory for window text in UTF-8\n");
            HeapFree( GetProcessHeap(), 0, buffer );
            return;
        }
        WideCharToMultiByte(CP_UTF8, 0, text, strlenW(text), utf8_buffer, count, NULL, NULL);

        wine_tsx11_lock();
	if (XmbTextListToTextProperty( display, &buffer, 1, XStdICCTextStyle, &prop ) == Success)
	{
	    XSetWMName( display, win, &prop );
	    XSetWMIconName( display, win, &prop );
	    XFree( prop.value );
	}
        /*
        Implements a NET_WM UTF-8 title. It should be without a trailing \0,
        according to the standard
        ( http://www.pps.jussieu.fr/~jch/software/UTF8_STRING/UTF8_STRING.text ).
        */
        XChangeProperty( display, win, x11drv_atom(_NET_WM_NAME), x11drv_atom(UTF8_STRING),
                         8, PropModeReplace, (unsigned char *) utf8_buffer, count);
        wine_tsx11_unlock();

        HeapFree( GetProcessHeap(), 0, utf8_buffer );
        HeapFree( GetProcessHeap(), 0, buffer );
    }
}


/***********************************************************************
 *		DestroyWindow   (X11DRV.@)
 */
void X11DRV_DestroyWindow( HWND hwnd )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    Display *display = thread_data->display;
    struct x11drv_win_data *data;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    free_window_dce( data );
    destroy_whole_window( display, data );
    destroy_icon_window( display, data );

    if (thread_data->last_focus == hwnd) thread_data->last_focus = 0;
    if (data->hWMIconBitmap) DeleteObject( data->hWMIconBitmap );
    if (data->hWMIconMask) DeleteObject( data->hWMIconMask);
    wine_tsx11_lock();
    XDeleteContext( display, (XID)hwnd, win_data_context );
    wine_tsx11_unlock();
    HeapFree( GetProcessHeap(), 0, data );
}


static struct x11drv_win_data *alloc_win_data( Display *display, HWND hwnd )
{
    struct x11drv_win_data *data;

    if ((data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data))))
    {
        data->hwnd          = hwnd;
        data->whole_window  = 0;
        data->icon_window   = 0;
        data->xic           = 0;
        data->managed       = FALSE;
        data->dce           = NULL;
        data->lock_changes  = 0;
        data->hWMIconBitmap = 0;
        data->hWMIconMask   = 0;

        wine_tsx11_lock();
        if (!winContext) winContext = XUniqueContext();
        if (!win_data_context) win_data_context = XUniqueContext();
        XSaveContext( display, (XID)hwnd, win_data_context, (char *)data );
        wine_tsx11_unlock();
    }
    return data;
}


/* fill in the desktop X window id in the x11drv_win_data structure */
static void get_desktop_xwin( Display *display, struct x11drv_win_data *data )
{
    Window win = (Window)GetPropA( data->hwnd, whole_window_prop );

    if (win)
    {
        unsigned int width, height;

        /* retrieve the real size of the desktop */
        SERVER_START_REQ( get_window_rectangles )
        {
            req->handle = data->hwnd;
            wine_server_call( req );
            width  = reply->window.right - reply->window.left;
            height = reply->window.bottom - reply->window.top;
        }
        SERVER_END_REQ;
        data->whole_window = win;
        if (win != root_window) X11DRV_init_desktop( win, width, height );
    }
    else
    {
        VisualID visualid;

        wine_tsx11_lock();
        visualid = XVisualIDFromVisual(visual);
        wine_tsx11_unlock();
        SetPropA( data->hwnd, whole_window_prop, (HANDLE)root_window );
        SetPropA( data->hwnd, visual_id_prop, (HANDLE)visualid );
        data->whole_window = root_window;
        X11DRV_SetWindowPos( data->hwnd, 0, &virtual_screen_rect, &virtual_screen_rect,
                               SWP_NOZORDER, NULL );
        if (root_window != DefaultRootWindow( display ))
        {
            data->managed = TRUE;
            SetPropA( data->hwnd, managed_prop, (HANDLE)1 );
        }
    }
}

/**********************************************************************
 *		CreateDesktopWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateDesktopWindow( HWND hwnd )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;

    if (!(data = alloc_win_data( display, hwnd ))) return FALSE;

    get_desktop_xwin( display, data );

    return TRUE;
}


/**********************************************************************
 *		CreateWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateWindow( HWND hwnd, CREATESTRUCTA *cs, BOOL unicode )
{
    Display *display = thread_display();
    WND *wndPtr;
    struct x11drv_win_data *data;
    HWND insert_after;
    RECT rect;
    DWORD style;
    CBT_CREATEWNDA cbtc;
    CREATESTRUCTA cbcs;
    BOOL ret = FALSE;

    if (!(data = alloc_win_data( display, hwnd ))) return FALSE;

    if (cs->cx > 65535)
    {
        ERR( "invalid window width %d\n", cs->cx );
        cs->cx = 65535;
    }
    if (cs->cy > 65535)
    {
        ERR( "invalid window height %d\n", cs->cy );
        cs->cy = 65535;
    }
    if (cs->cx < 0)
    {
        ERR( "invalid window width %d\n", cs->cx );
        cs->cx = 0;
    }
    if (cs->cy < 0)
    {
        ERR( "invalid window height %d\n", cs->cy );
        cs->cy = 0;
    }

    /* initialize the dimensions before sending WM_GETMINMAXINFO */
    SetRect( &rect, cs->x, cs->y, cs->x + cs->cx, cs->y + cs->cy );
    X11DRV_SetWindowPos( hwnd, 0, &rect, &rect, SWP_NOZORDER, NULL );

    /* create an X window if it's a top level window */
    if (GetAncestor( hwnd, GA_PARENT ) == GetDesktopWindow())
    {
        if (!create_whole_window( display, data, cs->style )) goto failed;
    }
    else if (hwnd == GetDesktopWindow())
    {
        get_desktop_xwin( display, data );
    }

    /* get class or window DC if needed */
    alloc_window_dce( data );

    /* Call the WH_CBT hook */

    /* the window style passed to the hook must be the real window style,
     * rather than just the window style that the caller to CreateWindowEx
     * passed in, so we have to copy the original CREATESTRUCT and get the
     * the real style. */
    cbcs = *cs;
    cbcs.style = GetWindowLongW(hwnd, GWL_STYLE);

    cbtc.lpcs = &cbcs;
    cbtc.hwndInsertAfter = HWND_TOP;
    if (HOOK_CallHooks( WH_CBT, HCBT_CREATEWND, (WPARAM)hwnd, (LPARAM)&cbtc, unicode ))
    {
        TRACE("CBT-hook returned !0\n");
        goto failed;
    }

    /* Send the WM_GETMINMAXINFO message and fix the size if needed */
    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
    {
        POINT maxSize, maxPos, minTrack, maxTrack;

        WINPOS_GetMinMaxInfo( hwnd, &maxSize, &maxPos, &minTrack, &maxTrack);
        if (maxTrack.x < cs->cx) cs->cx = maxTrack.x;
        if (maxTrack.y < cs->cy) cs->cy = maxTrack.y;
        if (cs->cx < 0) cs->cx = 0;
        if (cs->cy < 0) cs->cy = 0;

        SetRect( &rect, cs->x, cs->y, cs->x + cs->cx, cs->y + cs->cy );
        if (!X11DRV_SetWindowPos( hwnd, 0, &rect, &rect, SWP_NOZORDER, NULL )) return FALSE;
    }

    /* send WM_NCCREATE */
    TRACE( "hwnd %p cs %d,%d %dx%d\n", hwnd, cs->x, cs->y, cs->cx, cs->cy );
    if (unicode)
        ret = SendMessageW( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
    else
        ret = SendMessageA( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
    if (!ret)
    {
        WARN("aborted by WM_xxCREATE!\n");
        return FALSE;
    }

    /* make sure the window is still valid */
    if (!(data = X11DRV_get_win_data( hwnd ))) return FALSE;
    if (data->whole_window) X11DRV_sync_window_style( display, data );

    /* send WM_NCCALCSIZE */
    rect = data->window_rect;
    SendMessageW( hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&rect );

    if (!(wndPtr = WIN_GetPtr(hwnd))) return FALSE;

    /* yes, even if the CBT hook was called with HWND_TOP */
    insert_after = (wndPtr->dwStyle & WS_CHILD) ? HWND_BOTTOM : HWND_TOP;

    X11DRV_SetWindowPos( hwnd, insert_after, &wndPtr->rectWindow, &rect, 0, NULL );

    TRACE( "win %p window %d,%d,%d,%d client %d,%d,%d,%d whole %d,%d,%d,%d X client %d,%d,%d,%d xwin %x\n",
           hwnd, wndPtr->rectWindow.left, wndPtr->rectWindow.top,
           wndPtr->rectWindow.right, wndPtr->rectWindow.bottom,
           wndPtr->rectClient.left, wndPtr->rectClient.top,
           wndPtr->rectClient.right, wndPtr->rectClient.bottom,
           data->whole_rect.left, data->whole_rect.top,
           data->whole_rect.right, data->whole_rect.bottom,
           data->client_rect.left, data->client_rect.top,
           data->client_rect.right, data->client_rect.bottom,
           (unsigned int)data->whole_window );

    WIN_ReleasePtr( wndPtr );

    if (unicode)
        ret = (SendMessageW( hwnd, WM_CREATE, 0, (LPARAM)cs ) != -1);
    else
        ret = (SendMessageA( hwnd, WM_CREATE, 0, (LPARAM)cs ) != -1);

    if (!ret) return FALSE;

    NotifyWinEvent(EVENT_OBJECT_CREATE, hwnd, OBJID_WINDOW, 0);

    /* Send the size messages */

    if (!(wndPtr = WIN_GetPtr(hwnd)) || wndPtr == WND_OTHER_PROCESS) return FALSE;
    if (!(wndPtr->flags & WIN_NEED_SIZE))
    {
        RECT rect = wndPtr->rectClient;
        WIN_ReleasePtr( wndPtr );
        /* send it anyway */
        if (((rect.right-rect.left) <0) ||((rect.bottom-rect.top)<0))
            WARN("sending bogus WM_SIZE message 0x%08x\n",
                 MAKELONG(rect.right-rect.left, rect.bottom-rect.top));
        SendMessageW( hwnd, WM_SIZE, SIZE_RESTORED,
                      MAKELONG(rect.right-rect.left, rect.bottom-rect.top));
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG( rect.left, rect.top ) );
    }
    else WIN_ReleasePtr( wndPtr );

    /* Show the window, maximizing or minimizing if needed */

    style = GetWindowLongW( hwnd, GWL_STYLE );
    if (style & (WS_MINIMIZE | WS_MAXIMIZE))
    {
        extern UINT WINPOS_MinMaximize( HWND hwnd, UINT cmd, LPRECT rect ); /*FIXME*/

        RECT newPos;
        UINT swFlag = (style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;
        WIN_SetStyle( hwnd, 0, WS_MAXIMIZE | WS_MINIMIZE );
        swFlag = WINPOS_MinMaximize( hwnd, swFlag, &newPos );

        swFlag |= SWP_FRAMECHANGED; /* Frame always gets changed */
        if (!(style & WS_VISIBLE) || (style & WS_CHILD) || GetActiveWindow()) swFlag |= SWP_NOACTIVATE;

        SetWindowPos( hwnd, 0, newPos.left, newPos.top,
                      newPos.right, newPos.bottom, swFlag );
    }

    /* Dock system tray windows. */
    /* Dock after the window is created so we don't have problems calling
     * SetWindowPos. */
    if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_TRAYWINDOW)
        systray_dock_window( display, data );

    return TRUE;

 failed:
    X11DRV_DestroyWindow( hwnd );
    return FALSE;
}


/***********************************************************************
 *		X11DRV_get_win_data
 *
 * Return the X11 data structure associated with a window.
 */
struct x11drv_win_data *X11DRV_get_win_data( HWND hwnd )
{
    char *data;

    if (!hwnd || XFindContext( thread_display(), (XID)hwnd, win_data_context, &data )) data = NULL;
    return (struct x11drv_win_data *)data;
}


/***********************************************************************
 *		X11DRV_get_whole_window
 *
 * Return the X window associated with the full area of a window
 */
Window X11DRV_get_whole_window( HWND hwnd )
{
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data) return (Window)GetPropA( hwnd, whole_window_prop );
    return data->whole_window;
}


/***********************************************************************
 *		X11DRV_get_ic
 *
 * Return the X input context associated with a window
 */
XIC X11DRV_get_ic( HWND hwnd )
{
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data) return 0;
    return data->xic;
}


/*****************************************************************
 *		SetParent   (X11DRV.@)
 */
void X11DRV_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    Display *display = thread_display();
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data) return;
    if (parent == old_parent) return;

    if (parent != GetDesktopWindow()) /* a child window */
    {
        if (old_parent == GetDesktopWindow())
        {
            /* destroy the old X windows */
            destroy_whole_window( display, data );
            destroy_icon_window( display, data );
            if (data->managed)
            {
                data->managed = FALSE;
                RemovePropA( data->hwnd, managed_prop );
            }
        }
    }
    else  /* new top level window */
    {
        /* FIXME: we ignore errors since we can't really recover anyway */
        create_whole_window( display, data, GetWindowLongW( hwnd, GWL_STYLE ) );
    }
}


/*****************************************************************
 *		SetFocus   (X11DRV.@)
 *
 * Set the X focus.
 * Explicit colormap management seems to work only with OLVWM.
 */
void X11DRV_SetFocus( HWND hwnd )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    XWindowAttributes win_attr;

    /* Only mess with the X focus if there's */
    /* no desktop window and if the window is not managed by the WM. */
    if (root_window != DefaultRootWindow(display)) return;

    if (!hwnd)  /* If setting the focus to 0, uninstall the colormap */
    {
        wine_tsx11_lock();
        if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
            XUninstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
        wine_tsx11_unlock();
        return;
    }

    hwnd = GetAncestor( hwnd, GA_ROOT );

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (data->managed || !data->whole_window) return;

    /* Set X focus and install colormap */
    wine_tsx11_lock();
    if (XGetWindowAttributes( display, data->whole_window, &win_attr ) &&
        (win_attr.map_state == IsViewable))
    {
        /* If window is not viewable, don't change anything */

        /* we must not use CurrentTime (ICCCM), so try to use last message time instead */
        /* FIXME: this is not entirely correct */
        XSetInputFocus( display, data->whole_window, RevertToParent,
                        /* CurrentTime */
                        GetMessageTime() - EVENT_x11_time_to_win32_time(0));
        if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
            XInstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
    }
    wine_tsx11_unlock();
}


/**********************************************************************
 *		SetWindowIcon (X11DRV.@)
 *
 * hIcon or hIconSm has changed (or is being initialised for the
 * first time). Complete the X11 driver-specific initialisation
 * and set the window hints.
 *
 * This is not entirely correct, may need to create
 * an icon window and set the pixmap as a background
 */
void X11DRV_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    XWMHints* wm_hints;

    if (type != ICON_BIG) return;  /* nothing to do here */

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->whole_window) return;
    if (!data->managed) return;

    wine_tsx11_lock();
    if (!(wm_hints = XGetWMHints( display, data->whole_window ))) wm_hints = XAllocWMHints();
    wine_tsx11_unlock();
    if (wm_hints)
    {
        set_icon_hints( display, data, wm_hints, icon );
        wine_tsx11_lock();
        XSetWMHints( display, data->whole_window, wm_hints );
        XFree( wm_hints );
        wine_tsx11_unlock();
    }
}

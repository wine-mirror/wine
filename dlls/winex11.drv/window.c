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
#ifdef HAVE_LIBXSHAPE
#include <X11/extensions/shape.h>
#endif /* HAVE_LIBXSHAPE */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"

#include "x11drv.h"
#include "xcomposite.h"
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
static const char client_window_prop[]= "__wine_x11_client_window";
static const char icon_window_prop[]  = "__wine_x11_icon_window";
static const char fbconfig_id_prop[]  = "__wine_x11_fbconfig_id";
static const char gl_drawable_prop[]  = "__wine_x11_gl_drawable";
static const char pixmap_prop[]       = "__wine_x11_pixmap";
static const char managed_prop[]      = "__wine_x11_managed";
static const char visual_id_prop[]    = "__wine_x11_visual_id";

/* for XDG systray icons */
#define SYSTEM_TRAY_REQUEST_DOCK    0

extern int usexcomposite;

extern void WIN_invalidate_dce( HWND hwnd, const RECT *rect );  /* FIXME: to be removed */

/***********************************************************************
 *		is_window_managed
 *
 * Check if a given window should be managed
 */
BOOL is_window_managed( HWND hwnd, UINT swp_flags, const RECT *window_rect )
{
    DWORD style, ex_style;

    if (!managed_mode) return FALSE;

    /* child windows are not managed */
    style = GetWindowLongW( hwnd, GWL_STYLE );
    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD) return FALSE;
    /* activated windows are managed */
    if (!(swp_flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW))) return TRUE;
    if (hwnd == GetActiveWindow()) return TRUE;
    /* windows with caption are managed */
    if ((style & WS_CAPTION) == WS_CAPTION) return TRUE;
    /* tool windows are not managed  */
    ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );
    if (ex_style & WS_EX_TOOLWINDOW) return FALSE;
    /* windows with thick frame are managed */
    if (style & WS_THICKFRAME) return TRUE;
    /* application windows are managed */
    if (ex_style & WS_EX_APPWINDOW) return TRUE;
    if (style & WS_POPUP)
    {
        /* popup with sysmenu == caption are managed */
        if (style & WS_SYSMENU) return TRUE;
        /* full-screen popup windows are managed */
        if ((window_rect->right - window_rect->left) == screen_width &&
            (window_rect->bottom - window_rect->top) == screen_height)
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
 *              get_mwm_decorations
 */
static unsigned long get_mwm_decorations( DWORD style, DWORD ex_style )
{
    unsigned long ret = 0;

    if (ex_style & WS_EX_TOOLWINDOW) return 0;

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        ret |= MWM_DECOR_TITLE | MWM_DECOR_BORDER;
        if (style & WS_SYSMENU) ret |= MWM_DECOR_MENU;
        if (style & WS_MINIMIZEBOX) ret |= MWM_DECOR_MINIMIZE;
        if (style & WS_MAXIMIZEBOX) ret |= MWM_DECOR_MAXIMIZE;
    }
    if (ex_style & WS_EX_DLGMODALFRAME) ret |= MWM_DECOR_BORDER;
    else if (style & WS_THICKFRAME) ret |= MWM_DECOR_BORDER | MWM_DECOR_RESIZEH;
    else if ((style & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME) ret |= MWM_DECOR_BORDER;
    return ret;
}


/***********************************************************************
 *		get_x11_rect_offset
 *
 * Helper for X11DRV_window_to_X_rect and X11DRV_X_to_window_rect.
 */
static void get_x11_rect_offset( struct x11drv_win_data *data, RECT *rect )
{
    DWORD style, ex_style, style_mask = 0, ex_style_mask = 0;
    unsigned long decor;

    rect->top = rect->bottom = rect->left = rect->right = 0;

    style = GetWindowLongW( data->hwnd, GWL_STYLE );
    ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );
    decor = get_mwm_decorations( style, ex_style );

    if (decor & MWM_DECOR_TITLE) style_mask |= WS_CAPTION;
    if (decor & MWM_DECOR_BORDER)
    {
        style_mask |= WS_DLGFRAME | WS_THICKFRAME;
        ex_style_mask |= WS_EX_DLGMODALFRAME;
    }

    AdjustWindowRectEx( rect, style & style_mask, FALSE, ex_style & ex_style_mask );
}


/***********************************************************************
 *              get_window_attributes
 *
 * Fill the window attributes structure for an X window.
 */
static int get_window_attributes( Display *display, struct x11drv_win_data *data,
                                  XSetWindowAttributes *attr )
{
    attr->override_redirect = !data->managed;
    attr->colormap          = X11DRV_PALETTE_PaletteXColormap;
    attr->save_under        = ((GetClassLongW( data->hwnd, GCL_STYLE ) & CS_SAVEBITS) != 0);
    attr->cursor            = x11drv_thread_data()->cursor;
    attr->bit_gravity       = NorthWestGravity;
    attr->backing_store     = NotUseful;
    attr->event_mask        = (ExposureMask | PointerMotionMask |
                               ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
                               KeyPressMask | KeyReleaseMask | FocusChangeMask | KeymapStateMask);
    if (data->managed) attr->event_mask |= StructureNotifyMask;

    return (CWOverrideRedirect | CWSaveUnder | CWColormap | CWCursor |
            CWEventMask | CWBitGravity | CWBackingStore);
}


/***********************************************************************
 *              create_client_window
 */
static Window create_client_window( Display *display, struct x11drv_win_data *data, XVisualInfo *vis )
{
    int cx, cy, mask;
    XSetWindowAttributes attr;
    Window client;

    attr.bit_gravity = NorthWestGravity;
    attr.win_gravity = NorthWestGravity;
    attr.backing_store = NotUseful;
    attr.event_mask = (ExposureMask | PointerMotionMask |
                       ButtonPressMask | ButtonReleaseMask | EnterWindowMask);
    mask = CWEventMask | CWBitGravity | CWWinGravity | CWBackingStore;

    if ((cx = data->client_rect.right - data->client_rect.left) <= 0) cx = 1;
    if ((cy = data->client_rect.bottom - data->client_rect.top) <= 0) cy = 1;

    wine_tsx11_lock();

    if (vis)
    {
        attr.colormap = XCreateColormap( display, root_window, vis->visual,
                                         (vis->class == PseudoColor || vis->class == GrayScale ||
                                          vis->class == DirectColor) ? AllocAll : AllocNone );
        mask |= CWColormap;
    }

    client = XCreateWindow( display, data->whole_window,
                            data->client_rect.left - data->whole_rect.left,
                            data->client_rect.top - data->whole_rect.top,
                            cx, cy, 0, screen_depth, InputOutput,
                            vis ? vis->visual : visual, mask, &attr );
    if (!client)
    {
        wine_tsx11_unlock();
        return 0;
    }

    if (data->client_window)
    {
        XDeleteContext( display, data->client_window, winContext );
        XDestroyWindow( display, data->client_window );
    }
    data->client_window = client;

    if (data->colormap) XFreeColormap( display, data->colormap );
    data->colormap = vis ? attr.colormap : 0;

    XMapWindow( display, data->client_window );
    XSaveContext( display, data->client_window, winContext, (char *)data->hwnd );
    wine_tsx11_unlock();

    SetPropA( data->hwnd, client_window_prop, (HANDLE)data->client_window );
    return data->client_window;
}


/***********************************************************************
 *              X11DRV_sync_window_style
 *
 * Change the X window attributes when the window style has changed.
 */
void X11DRV_sync_window_style( Display *display, struct x11drv_win_data *data )
{
    if (data->whole_window != root_window)
    {
        XSetWindowAttributes attr;
        int mask = get_window_attributes( display, data, &attr );

        wine_tsx11_lock();
        XChangeWindowAttributes( display, data->whole_window, mask, &attr );
        wine_tsx11_unlock();
    }
}


/***********************************************************************
 *              sync_window_region
 *
 * Update the X11 window region.
 */
static void sync_window_region( Display *display, struct x11drv_win_data *data, HRGN hrgn )
{
#ifdef HAVE_LIBXSHAPE
    if (!data->whole_window) return;

    if (!hrgn)
    {
        wine_tsx11_lock();
        XShapeCombineMask( display, data->whole_window, ShapeBounding, 0, 0, None, ShapeSet );
        wine_tsx11_unlock();
    }
    else
    {
        RGNDATA *pRegionData = X11DRV_GetRegionData( hrgn, 0 );
        if (pRegionData)
        {
            wine_tsx11_lock();
            XShapeCombineRectangles( display, data->whole_window, ShapeBounding,
                                     data->window_rect.left - data->whole_rect.left,
                                     data->window_rect.top - data->whole_rect.top,
                                     (XRectangle *)pRegionData->Buffer,
                                     pRegionData->rdh.nCount, ShapeSet, YXBanded );
            wine_tsx11_unlock();
            HeapFree(GetProcessHeap(), 0, pRegionData);
        }
    }
#endif  /* HAVE_LIBXSHAPE */
}


/***********************************************************************
 *              sync_window_text
 */
static void sync_window_text( Display *display, Window win, const WCHAR *text )
{
    UINT count;
    char *buffer, *utf8_buffer;
    XTextProperty prop;

    /* allocate new buffer for window text */
    count = WideCharToMultiByte(CP_UNIXCP, 0, text, -1, NULL, 0, NULL, NULL);
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, count ))) return;
    WideCharToMultiByte(CP_UNIXCP, 0, text, -1, buffer, count, NULL, NULL);

    count = WideCharToMultiByte(CP_UTF8, 0, text, strlenW(text), NULL, 0, NULL, NULL);
    if (!(utf8_buffer = HeapAlloc( GetProcessHeap(), 0, count )))
    {
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


/***********************************************************************
 *              X11DRV_set_win_format
 */
BOOL X11DRV_set_win_format( HWND hwnd, XID fbconfig_id )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    XVisualInfo *vis;
    int w, h;

    if (!(data = X11DRV_get_win_data(hwnd)) &&
        !(data = X11DRV_create_win_data(hwnd))) return FALSE;

    if (data->fbconfig_id) return FALSE;  /* can't change it twice */

    wine_tsx11_lock();
    vis = visual_from_fbconfig_id(fbconfig_id);
    wine_tsx11_unlock();
    if (!vis) return FALSE;

    if (data->whole_window)
    {
        Window client = data->client_window;

        if (vis->visualid != XVisualIDFromVisual(visual))
        {
            client = create_client_window( display, data, vis );
            TRACE( "re-created client window %lx for %p fbconfig %lx\n", client, data->hwnd, fbconfig_id );
        }
        wine_tsx11_lock();
        XFree(vis);
        wine_tsx11_unlock();
        if (client) goto done;
        return FALSE;
    }

    w = data->client_rect.right - data->client_rect.left;
    h = data->client_rect.bottom - data->client_rect.top;

    if(w <= 0) w = 1;
    if(h <= 0) h = 1;

#ifdef SONAME_LIBXCOMPOSITE
    if(usexcomposite)
    {
        XSetWindowAttributes attrib;
        Window parent = X11DRV_get_whole_window( GetAncestor( hwnd, GA_ROOT ));

        if (!parent) parent = root_window;
        wine_tsx11_lock();
        data->colormap = XCreateColormap(display, parent, vis->visual,
                                         (vis->class == PseudoColor ||
                                          vis->class == GrayScale ||
                                          vis->class == DirectColor) ?
                                         AllocAll : AllocNone);
        attrib.override_redirect = True;
        attrib.colormap = data->colormap;
        XInstallColormap(gdi_display, attrib.colormap);

        data->gl_drawable = XCreateWindow(display, parent, -w, 0, w, h, 0,
                                          vis->depth, InputOutput, vis->visual,
                                          CWColormap | CWOverrideRedirect,
                                          &attrib);
        if(data->gl_drawable)
        {
            pXCompositeRedirectWindow(display, data->gl_drawable,
                                      CompositeRedirectManual);
            XMapWindow(display, data->gl_drawable);
        }
        XFree(vis);
        wine_tsx11_unlock();
    }
    else
#endif
    {
        WARN("XComposite is not available, using GLXPixmap hack\n");

        wine_tsx11_lock();
        data->pixmap = XCreatePixmap(display, root_window, w, h, vis->depth);
        if(!data->pixmap)
        {
            XFree(vis);
            wine_tsx11_unlock();
            return FALSE;
        }

        data->gl_drawable = create_glxpixmap(display, vis, data->pixmap);
        if(!data->gl_drawable)
        {
            XFreePixmap(display, data->pixmap);
            data->pixmap = 0;
        }
        XFree(vis);
        wine_tsx11_unlock();
        if (data->pixmap) SetPropA(hwnd, pixmap_prop, (HANDLE)data->pixmap);
    }

    if (!data->gl_drawable) return FALSE;

    TRACE("Created GL drawable 0x%lx, using FBConfigID 0x%lx\n",
          data->gl_drawable, fbconfig_id);
    SetPropA(hwnd, gl_drawable_prop, (HANDLE)data->gl_drawable);

done:
    data->fbconfig_id = fbconfig_id;
    SetPropA(hwnd, fbconfig_id_prop, (HANDLE)data->fbconfig_id);
    wine_tsx11_lock();
    XFlush( display );
    wine_tsx11_unlock();
    WIN_invalidate_dce( hwnd, NULL );
    return TRUE;
}

/***********************************************************************
 *              sync_gl_drawable
 */
static void sync_gl_drawable(Display *display, struct x11drv_win_data *data)
{
    int w = data->client_rect.right - data->client_rect.left;
    int h = data->client_rect.bottom - data->client_rect.top;
    XVisualInfo *vis;
    Drawable glxp;
    Pixmap pix;

    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    TRACE("Resizing GL drawable 0x%lx to %dx%d\n", data->gl_drawable, w, h);
#ifdef SONAME_LIBXCOMPOSITE
    if(usexcomposite)
    {
        wine_tsx11_lock();
        XMoveResizeWindow(display, data->gl_drawable, -w, 0, w, h);
        wine_tsx11_unlock();
        return;
    }
#endif

    wine_tsx11_lock();

    vis = visual_from_fbconfig_id(data->fbconfig_id);
    if(!vis)
    {
        wine_tsx11_unlock();
        return;
    }

    pix = XCreatePixmap(display, root_window, w, h, vis->depth);
    if(!pix)
    {
        ERR("Failed to create pixmap for offscreen rendering\n");
        XFree(vis);
        wine_tsx11_unlock();
        return;
    }

    glxp = create_glxpixmap(display, vis, pix);
    if(!glxp)
    {
        ERR("Failed to create drawable for offscreen rendering\n");
        XFreePixmap(display, pix);
        XFree(vis);
        wine_tsx11_unlock();
        return;
    }

    XFree(vis);

    mark_drawable_dirty(data->gl_drawable, glxp);

    XFreePixmap(display, data->pixmap);
    destroy_glxpixmap(display, data->gl_drawable);

    data->pixmap = pix;
    data->gl_drawable = glxp;

    wine_tsx11_unlock();

    SetPropA(data->hwnd, gl_drawable_prop, (HANDLE)data->gl_drawable);
    SetPropA(data->hwnd, pixmap_prop, (HANDLE)data->pixmap);
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
        if ((changes->width = new->right - new->left) <= 0) changes->width = 1;
        mask |= CWWidth;
    }
    if (old->bottom - old->top != new->bottom - new->top)
    {
        if ((changes->height = new->bottom - new->top) <= 0) changes->height = 1;
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
static void set_icon_hints( Display *display, struct x11drv_win_data *data, HICON hIcon )
{
    XWMHints *hints = data->wm_hints;

    if (data->hWMIconBitmap) DeleteObject( data->hWMIconBitmap );
    if (data->hWMIconMask) DeleteObject( data->hWMIconMask);
    data->hWMIconBitmap = 0;
    data->hWMIconMask = 0;

    if (!hIcon)
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
 *              wine_make_systray_window   (X11DRV.@)
 *
 * Docks the given X window with the NETWM system tray.
 */
void X11DRV_make_systray_window( HWND hwnd )
{
    static Atom systray_atom;
    Display *display = thread_display();
    struct x11drv_win_data *data;
    Window systray_window;

    if (!(data = X11DRV_get_win_data( hwnd )) &&
        !(data = X11DRV_create_win_data( hwnd ))) return;

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
                      0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE );

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
 *              set_initial_wm_hints
 *
 * Set the window manager hints that don't change over the lifetime of a window.
 */
static void set_initial_wm_hints( Display *display, struct x11drv_win_data *data )
{
    int i;
    Atom protocols[3];
    Atom dndVersion = 4;
    XClassHint *class_hints;
    char *process_name = get_process_name();

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

    /* set the WM_CLIENT_MACHINE and WM_LOCALE_NAME properties */
    XSetWMProperties(display, data->whole_window, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    /* set the pid. together, these properties are needed so the window manager can kill us if we freeze */
    i = getpid();
    XChangeProperty(display, data->whole_window, x11drv_atom(_NET_WM_PID),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&i, 1);

    XChangeProperty( display, data->whole_window, x11drv_atom(XdndAware),
                     XA_ATOM, 32, PropModeReplace, (unsigned char*)&dndVersion, 1 );

    data->wm_hints = XAllocWMHints();
    wine_tsx11_unlock();

    if (data->wm_hints)
    {
        HICON icon = (HICON)SendMessageW( data->hwnd, WM_GETICON, ICON_BIG, 0 );
        if (!icon) icon = (HICON)GetClassLongPtrW( data->hwnd, GCLP_HICON );
        data->wm_hints->flags = 0;
        set_icon_hints( display, data, icon );
    }
}


/***********************************************************************
 *              X11DRV_set_wm_hints
 *
 * Set the window manager hints for a newly-created window
 */
void X11DRV_set_wm_hints( Display *display, struct x11drv_win_data *data )
{
    Window group_leader;
    Atom window_type;
    MwmHints mwm_hints;
    DWORD style = GetWindowLongW( data->hwnd, GWL_STYLE );
    DWORD ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );
    HWND owner = GetWindow( data->hwnd, GW_OWNER );

    if (data->hwnd == GetDesktopWindow())
    {
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

    /* size hints */
    set_size_hints( display, data, style );

    /* set the WM_WINDOW_TYPE */
    window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_NORMAL);
    if (ex_style & WS_EX_TOOLWINDOW) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_UTILITY);
    else if (style & WS_THICKFRAME) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_NORMAL);
    else if (style & WS_DLGFRAME) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_DIALOG);
    else if (ex_style & WS_EX_DLGMODALFRAME) window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_DIALOG);

    XChangeProperty(display, data->whole_window, x11drv_atom(_NET_WM_WINDOW_TYPE),
		    XA_ATOM, 32, PropModeReplace, (unsigned char*)&window_type, 1);

    mwm_hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    mwm_hints.decorations = get_mwm_decorations( style, ex_style );
    mwm_hints.functions = MWM_FUNC_MOVE;
    if (style & WS_THICKFRAME)  mwm_hints.functions |= MWM_FUNC_RESIZE;
    if (style & WS_MINIMIZEBOX) mwm_hints.functions |= MWM_FUNC_MINIMIZE;
    if (style & WS_MAXIMIZEBOX) mwm_hints.functions |= MWM_FUNC_MAXIMIZE;
    if (style & WS_SYSMENU)     mwm_hints.functions |= MWM_FUNC_CLOSE;

    XChangeProperty( display, data->whole_window, x11drv_atom(_MOTIF_WM_HINTS),
                     x11drv_atom(_MOTIF_WM_HINTS), 32, PropModeReplace,
                     (unsigned char*)&mwm_hints, sizeof(mwm_hints)/sizeof(long) );

    /* wm hints */
    if (data->wm_hints)
    {
        data->wm_hints->flags |= InputHint | StateHint | WindowGroupHint;
        data->wm_hints->input = !(style & WS_DISABLED);
        data->wm_hints->initial_state = (style & WS_MINIMIZE) ? IconicState : NormalState;
        data->wm_hints->window_group = group_leader;
        XSetWMHints( display, data->whole_window, data->wm_hints );
    }

    wine_tsx11_unlock();
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
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL iconic = (style & WS_MINIMIZE) != 0;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->whole_window) return;

    GetWindowRect( hwnd, &rect );

    wine_tsx11_lock();

    if (data->wm_hints)
    {
        data->wm_hints->flags |= StateHint | IconPositionHint;
        data->wm_hints->initial_state = iconic ? IconicState : NormalState;
        data->wm_hints->icon_x = rect.left - virtual_screen_rect.left;
        data->wm_hints->icon_y = rect.top - virtual_screen_rect.top;
        XSetWMHints( display, data->whole_window, data->wm_hints );
    }

    if (data->mapped)
    {
        if (iconic)
            XIconifyWindow( display, data->whole_window, DefaultScreen(display) );
        else
            if (X11DRV_is_window_rect_mapped( &rect ))
                XMapWindow( display, data->whole_window );
    }

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

    get_x11_rect_offset( data, &rc );

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
    RECT rc;

    if (!data->managed) return;
    if (IsRectEmpty( rect )) return;

    get_x11_rect_offset( data, &rc );

    rect->left   += rc.left;
    rect->right  += rc.right;
    rect->top    += rc.top;
    rect->bottom += rc.bottom;
    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *		X11DRV_sync_window_position
 *
 * Synchronize the X window position with the Windows one
 */
void X11DRV_sync_window_position( Display *display, struct x11drv_win_data *data,
                                  UINT swp_flags, const RECT *old_client_rect,
                                  const RECT *old_whole_rect )
{
    XWindowChanges changes;
    int mask = get_window_changes( &changes, old_whole_rect, &data->whole_rect );

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

    /* only the size is allowed to change for the desktop window */
    if (data->whole_window == root_window) mask &= CWWidth | CWHeight;

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


/***********************************************************************
 *		X11DRV_sync_client_position
 *
 * Synchronize the X client window position with the Windows one
 */
void X11DRV_sync_client_position( Display *display, struct x11drv_win_data *data,
                                  UINT swp_flags, const RECT *old_client_rect,
                                  const RECT *old_whole_rect )
{
    int mask;
    XWindowChanges changes;
    RECT old = *old_client_rect;
    RECT new = data->client_rect;

    OffsetRect( &old, -old_whole_rect->left, -old_whole_rect->top );
    OffsetRect( &new, -data->whole_rect.left, -data->whole_rect.top );
    if (!(mask = get_window_changes( &changes, &old, &new ))) return;

    if (data->client_window)
    {
        TRACE( "setting client win %lx pos %d,%d,%dx%d changes=%x\n",
               data->client_window, new.left, new.top,
               new.right - new.left, new.bottom - new.top, mask );
        wine_tsx11_lock();
        XConfigureWindow( display, data->client_window, mask, &changes );
        wine_tsx11_unlock();
    }

    if (data->gl_drawable && (mask & (CWWidth|CWHeight))) sync_gl_drawable( display, data );

    /* make sure the changes get to the server before we start painting */
    if (data->client_window || data->gl_drawable)
    {
        wine_tsx11_lock();
        XFlush(display);
        wine_tsx11_unlock();
    }
}


/**********************************************************************
 *		create_whole_window
 *
 * Create the whole X window for a given window
 */
static Window create_whole_window( Display *display, struct x11drv_win_data *data )
{
    int cx, cy, mask;
    XSetWindowAttributes attr;
    XIM xim;
    WCHAR text[1024];
    HRGN hrgn;

    if (!(cx = data->window_rect.right - data->window_rect.left)) cx = 1;
    if (!(cy = data->window_rect.bottom - data->window_rect.top)) cy = 1;

    if (!data->managed && is_window_managed( data->hwnd, SWP_NOACTIVATE, &data->window_rect ))
    {
        TRACE( "making win %p/%lx managed\n", data->hwnd, data->whole_window );
        data->managed = TRUE;
        SetPropA( data->hwnd, managed_prop, (HANDLE)1 );
    }

    mask = get_window_attributes( display, data, &attr );

    wine_tsx11_lock();

    data->whole_rect = data->window_rect;
    data->whole_window = XCreateWindow( display, root_window,
                                        data->window_rect.left - virtual_screen_rect.left,
                                        data->window_rect.top - virtual_screen_rect.top,
                                        cx, cy, 0, screen_depth, InputOutput,
                                        visual, mask, &attr );

    if (data->whole_window) XSaveContext( display, data->whole_window, winContext, (char *)data->hwnd );
    wine_tsx11_unlock();

    if (!data->whole_window) return 0;

    if (!create_client_window( display, data, NULL ))
    {
        wine_tsx11_lock();
        XDeleteContext( display, data->whole_window, winContext );
        XDestroyWindow( display, data->whole_window );
        data->whole_window = 0;
        wine_tsx11_unlock();
        return 0;
    }

    xim = x11drv_thread_data()->xim;
    if (xim) data->xic = X11DRV_CreateIC( xim, display, data->whole_window );

    set_initial_wm_hints( display, data );
    X11DRV_set_wm_hints( display, data );

    SetPropA( data->hwnd, whole_window_prop, (HANDLE)data->whole_window );

    /* set the window text */
    if (!InternalGetWindowText( data->hwnd, text, sizeof(text)/sizeof(WCHAR) )) text[0] = 0;
    sync_window_text( display, data->whole_window, text );

    /* set the window region */
    if ((hrgn = CreateRectRgn( 0, 0, 0, 0 )))
    {
        if (GetWindowRgn( data->hwnd, hrgn ) != ERROR) sync_window_region( display, data, hrgn );
        DeleteObject( hrgn );
    }
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

    TRACE( "win %p xwin %lx/%lx\n", data->hwnd, data->whole_window, data->client_window );
    if (thread_data->cursor_window == data->whole_window ||
        thread_data->cursor_window == data->client_window)
        thread_data->cursor_window = None;
    wine_tsx11_lock();
    XDeleteContext( display, data->whole_window, winContext );
    XDeleteContext( display, data->client_window, winContext );
    XDestroyWindow( display, data->whole_window );
    data->whole_window = data->client_window = 0;
    if (data->xic)
    {
        XUnsetICFocus( data->xic );
        XDestroyIC( data->xic );
    }
    /* Outlook stops processing messages after destroying a dialog, so we need an explicit flush */
    XFlush( display );
    XFree( data->wm_hints );
    data->wm_hints = NULL;
    wine_tsx11_unlock();
    RemovePropA( data->hwnd, whole_window_prop );
    RemovePropA( data->hwnd, client_window_prop );
}


/*****************************************************************
 *		SetWindowText   (X11DRV.@)
 */
void X11DRV_SetWindowText( HWND hwnd, LPCWSTR text )
{
    Display *display = thread_display();
    Window win;

    if ((win = X11DRV_get_whole_window( hwnd )) && win != DefaultRootWindow(display))
        sync_window_text( display, win, text );
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

    if (data->pixmap)
    {
        destroy_glxpixmap(display, data->gl_drawable);
        wine_tsx11_lock();
        XFreePixmap(display, data->pixmap);
        wine_tsx11_unlock();
    }
    else if (data->gl_drawable)
    {
        wine_tsx11_lock();
        XDestroyWindow(display, data->gl_drawable);
        wine_tsx11_unlock();
    }

    destroy_whole_window( display, data );
    destroy_icon_window( display, data );

    if (data->colormap)
    {
        wine_tsx11_lock();
        XFreeColormap( display, data->colormap );
        wine_tsx11_unlock();
    }

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

    if ((data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data))))
    {
        data->hwnd = hwnd;
        wine_tsx11_lock();
        if (!winContext) winContext = XUniqueContext();
        if (!win_data_context) win_data_context = XUniqueContext();
        XSaveContext( display, (XID)hwnd, win_data_context, (char *)data );
        wine_tsx11_unlock();
    }
    return data;
}


/* initialize the desktop window id in the desktop manager process */
static struct x11drv_win_data *create_desktop_win_data( Display *display, HWND hwnd )
{
    struct x11drv_win_data *data;
    VisualID visualid;

    if (!(data = alloc_win_data( display, hwnd ))) return NULL;
    wine_tsx11_lock();
    visualid = XVisualIDFromVisual(visual);
    wine_tsx11_unlock();
    data->whole_window = data->client_window = root_window;
    data->managed = TRUE;
    SetPropA( data->hwnd, managed_prop, (HANDLE)1 );
    SetPropA( data->hwnd, whole_window_prop, (HANDLE)root_window );
    SetPropA( data->hwnd, client_window_prop, (HANDLE)root_window );
    SetPropA( data->hwnd, visual_id_prop, (HANDLE)visualid );
    set_initial_wm_hints( display, data );
    return data;
}

/**********************************************************************
 *		CreateDesktopWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateDesktopWindow( HWND hwnd )
{
    unsigned int width, height;

    /* retrieve the real size of the desktop */
    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = hwnd;
        wine_server_call( req );
        width  = reply->window.right - reply->window.left;
        height = reply->window.bottom - reply->window.top;
    }
    SERVER_END_REQ;

    if (!width && !height)  /* not initialized yet */
    {
        SERVER_START_REQ( set_window_pos )
        {
            req->handle        = hwnd;
            req->previous      = 0;
            req->flags         = SWP_NOZORDER;
            req->window.left   = virtual_screen_rect.left;
            req->window.top    = virtual_screen_rect.top;
            req->window.right  = virtual_screen_rect.right;
            req->window.bottom = virtual_screen_rect.bottom;
            req->client        = req->window;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else
    {
        Window win = (Window)GetPropA( hwnd, whole_window_prop );
        if (win && win != root_window) X11DRV_init_desktop( win, width, height );
    }
    return TRUE;
}


/**********************************************************************
 *		CreateWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateWindow( HWND hwnd )
{
    Display *display = thread_display();
    DWORD style;

    if (hwnd == GetDesktopWindow() && root_window != DefaultRootWindow( display ))
    {
        /* the desktop win data can't be created lazily */
        if (!create_desktop_win_data( display, hwnd )) return FALSE;
    }

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

    return TRUE;
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
 *		X11DRV_create_win_data
 *
 * Create an X11 data window structure for an existing window.
 */
struct x11drv_win_data *X11DRV_create_win_data( HWND hwnd )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    HWND parent;

    if (!(parent = GetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop */
    if (!(data = alloc_win_data( display, hwnd ))) return NULL;

    GetWindowRect( hwnd, &data->window_rect );
    MapWindowPoints( 0, parent, (POINT *)&data->window_rect, 2 );
    data->whole_rect = data->window_rect;
    GetClientRect( hwnd, &data->client_rect );
    MapWindowPoints( hwnd, parent, (POINT *)&data->client_rect, 2 );

    if (parent == GetDesktopWindow())
    {
        if (!create_whole_window( display, data ))
        {
            HeapFree( GetProcessHeap(), 0, data );
            return NULL;
        }
        TRACE( "win %p/%lx/%lx window %s whole %s client %s\n",
               hwnd, data->whole_window, data->client_window, wine_dbgstr_rect( &data->window_rect ),
               wine_dbgstr_rect( &data->whole_rect ), wine_dbgstr_rect( &data->client_rect ));
    }
    return data;
}


/***********************************************************************
 *		X11DRV_get_whole_window
 *
 * Return the X window associated with the full area of a window
 */
Window X11DRV_get_whole_window( HWND hwnd )
{
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data)
    {
        if (hwnd == GetDesktopWindow()) return root_window;
        return (Window)GetPropA( hwnd, whole_window_prop );
    }
    return data->whole_window;
}


/***********************************************************************
 *		X11DRV_get_client_window
 *
 * Return the X window associated with the client area of a window
 */
Window X11DRV_get_client_window( HWND hwnd )
{
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data)
    {
        if (hwnd == GetDesktopWindow()) return root_window;
        return (Window)GetPropA( hwnd, client_window_prop );
    }
    return data->client_window;
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


/***********************************************************************
 *		X11DRV_GetDC   (X11DRV.@)
 */
void X11DRV_GetDC( HDC hdc, HWND hwnd, HWND top, const RECT *win_rect,
                   const RECT *top_rect, DWORD flags )
{
    struct x11drv_escape_set_drawable escape;
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    escape.code        = X11DRV_SET_DRAWABLE;
    escape.mode        = IncludeInferiors;
    escape.fbconfig_id = 0;
    escape.gl_drawable = 0;
    escape.pixmap      = 0;

    if (top == hwnd && data && IsIconic( hwnd ) && data->icon_window)
    {
        escape.drawable = data->icon_window;
    }
    else if (top == hwnd && (flags & DCX_WINDOW))
    {
        escape.drawable = data ? data->whole_window : X11DRV_get_whole_window( hwnd );
    }
    else
    {
        escape.drawable    = X11DRV_get_client_window( top );
        escape.fbconfig_id = data ? data->fbconfig_id : (XID)GetPropA( hwnd, fbconfig_id_prop );
        escape.gl_drawable = data ? data->gl_drawable : (Drawable)GetPropA( hwnd, gl_drawable_prop );
        escape.pixmap      = data ? data->pixmap : (Pixmap)GetPropA( hwnd, pixmap_prop );
    }

    escape.dc_rect.left         = win_rect->left - top_rect->left;
    escape.dc_rect.top          = win_rect->top - top_rect->top;
    escape.dc_rect.right        = win_rect->right - top_rect->left;
    escape.dc_rect.bottom       = win_rect->bottom - top_rect->top;
    escape.drawable_rect.left   = top_rect->left;
    escape.drawable_rect.top    = top_rect->top;
    escape.drawable_rect.right  = top_rect->right;
    escape.drawable_rect.bottom = top_rect->bottom;

    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}


/***********************************************************************
 *		X11DRV_ReleaseDC  (X11DRV.@)
 */
void X11DRV_ReleaseDC( HWND hwnd, HDC hdc )
{
    struct x11drv_escape_set_drawable escape;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = root_window;
    escape.mode = IncludeInferiors;
    escape.drawable_rect = virtual_screen_rect;
    SetRect( &escape.dc_rect, 0, 0, virtual_screen_rect.right - virtual_screen_rect.left,
             virtual_screen_rect.bottom - virtual_screen_rect.top );
    escape.fbconfig_id = 0;
    escape.gl_drawable = 0;
    escape.pixmap = 0;
    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
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
        create_whole_window( display, data );
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
    XWindowChanges changes;

    /* If setting the focus to 0, uninstall the colormap */
    if (!hwnd && root_window == DefaultRootWindow(display))
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
    changes.stack_mode = Above;
    XConfigureWindow( display, data->whole_window, CWStackMode, &changes );
    if (root_window == DefaultRootWindow(display))
    {
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

    if (type != ICON_BIG) return;  /* nothing to do here */

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->whole_window) return;
    if (!data->managed) return;

    if (data->wm_hints)
    {
        set_icon_hints( display, data, icon );
        wine_tsx11_lock();
        XSetWMHints( display, data->whole_window, data->wm_hints );
        wine_tsx11_unlock();
    }
}


/***********************************************************************
 *		SetWindowRgn  (X11DRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
int X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    struct x11drv_win_data *data;

    if ((data = X11DRV_get_win_data( hwnd )))
    {
        sync_window_region( thread_display(), data, hrgn );
    }
    else if (GetWindowThreadProcessId( hwnd, NULL ) != GetCurrentThreadId())
    {
        FIXME( "not supported on other thread window %p\n", hwnd );
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return TRUE;
}

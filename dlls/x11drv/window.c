/*
 * Window related functions
 *
 * Copyright 1993, 1994, 1995, 1996, 2001 Alexandre Julliard
 * Copyright 1993 David Metcalfe
 * Copyright 1995, 1996 Alex Korobka
 */

#include "config.h"

#include "ts_xlib.h"
#include "ts_xutil.h"

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "debugtools.h"
#include "x11drv.h"
#include "win.h"
#include "dce.h"
#include "options.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

extern Pixmap X11DRV_BITMAP_Pixmap( HBITMAP );

#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_THICKFRAME)))

#define HAS_THICKFRAME(style,exStyle) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_THINFRAME(style) \
    (((style) & WS_BORDER) || !((style) & (WS_CHILD | WS_POPUP)))

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

Atom wmProtocols = None;
Atom wmDeleteWindow = None;
Atom wmTakeFocus = None;
Atom dndProtocol = None;
Atom dndSelection = None;
Atom wmChangeState = None;
Atom kwmDockWindow = None;
Atom _kde_net_wm_system_tray_window_for = None; /* KDE 2 Final */


/***********************************************************************
 *		is_window_managed
 *
 * Check if a given window should be managed
 */
inline static BOOL is_window_managed( WND *win )
{
    if (!Options.managed) return FALSE;

    /* tray window is always managed */
    if (win->dwExStyle & WS_EX_TRAYWINDOW) return TRUE;
    /* child windows are not managed */
    if (win->dwStyle & WS_CHILD) return FALSE;
    /* tool windows are not managed */
    if (win->dwExStyle & WS_EX_TOOLWINDOW) return FALSE;
    /* windows with caption or thick frame are managed */
    if ((win->dwStyle & WS_CAPTION) == WS_CAPTION) return TRUE;
    if (win->dwStyle & WS_THICKFRAME) return TRUE;
    /* default: not managed */
    return FALSE;
}


/***********************************************************************
 *		is_window_top_level
 *
 * Check if a given window is a top level X11 window
 */
inline static BOOL is_window_top_level( WND *win )
{
    return (root_window == DefaultRootWindow(gdi_display) &&
            win->parent->hwndSelf == GetDesktopWindow());
}


/***********************************************************************
 *              get_window_attributes
 *
 * Fill the window attributes structure for an X window
 */
static int get_window_attributes( WND *win, XSetWindowAttributes *attr )
{
    BOOL is_top_level = is_window_top_level( win );
    BOOL managed = is_top_level && is_window_managed( win );

    if (managed) win->dwExStyle |= WS_EX_MANAGED;
    else win->dwExStyle &= ~WS_EX_MANAGED;

    attr->override_redirect = !managed;
    attr->bit_gravity       = ForgetGravity;
    attr->win_gravity       = NorthWestGravity;
    attr->colormap          = X11DRV_PALETTE_PaletteXColormap;
    attr->backing_store     = NotUseful/*WhenMapped*/;
    attr->save_under        = ((win->clsStyle & CS_SAVEBITS) != 0);
    attr->cursor            = None;
    attr->event_mask        = (ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                               ButtonPressMask | ButtonReleaseMask);
    if (is_window_top_level( win )) attr->event_mask |= StructureNotifyMask | FocusChangeMask;

    return (CWBitGravity | CWWinGravity | CWBackingStore | CWOverrideRedirect |
            CWSaveUnder | CWEventMask | CWColormap | CWCursor);
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
static Window create_icon_window( Display *display, WND *win )
{
    struct x11drv_win_data *data = win->pDriverData;
    XSetWindowAttributes attr;

    attr.event_mask = (ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                       ButtonPressMask | ButtonReleaseMask);
    attr.bit_gravity = NorthWestGravity;
    attr.backing_store = NotUseful/*WhenMapped*/;

    data->icon_window = TSXCreateWindow( display, root_window, 0, 0,
                                         GetSystemMetrics( SM_CXICON ),
                                         GetSystemMetrics( SM_CYICON ),
                                         0, screen_depth,
                                         InputOutput, visual,
                                         CWEventMask | CWBitGravity | CWBackingStore, &attr );
    XSaveContext( display, data->icon_window, winContext, (char *)win->hwndSelf );
    TRACE( "created %lx\n", data->icon_window );
    return data->icon_window;
}



/***********************************************************************
 *              destroy_icon_window
 */
inline static void destroy_icon_window( Display *display, struct x11drv_win_data *data )
{
    if (!data->icon_window) return;
    XDeleteContext( display, data->icon_window, winContext );
    XDestroyWindow( display, data->icon_window );
    data->icon_window = 0;
}


/***********************************************************************
 *              set_icon_hints
 *
 * Set the icon wm hints
 */
static void set_icon_hints( Display *display, WND *wndPtr, XWMHints *hints )
{
    X11DRV_WND_DATA *data = wndPtr->pDriverData;
    HICON hIcon = GetClassLongA( wndPtr->hwndSelf, GCL_HICON );

    if (data->hWMIconBitmap) DeleteObject( data->hWMIconBitmap );
    if (data->hWMIconMask) DeleteObject( data->hWMIconMask);

    if (!hIcon)
    {
        data->hWMIconBitmap = 0;
        data->hWMIconMask = 0;
        if (!data->icon_window) create_icon_window( display, wndPtr );
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

        X11DRV_CreateBitmap(ii.hbmMask);
        X11DRV_CreateBitmap(ii.hbmColor);

        GetObjectA(ii.hbmMask, sizeof(bmMask), &bmMask);
        rcMask.top    = 0;
        rcMask.left   = 0;
        rcMask.right  = bmMask.bmWidth;
        rcMask.bottom = bmMask.bmHeight;

        hDC = CreateCompatibleDC(0);
        hbmOrig = SelectObject(hDC, ii.hbmMask);
        InvertRect(hDC, &rcMask);
        SelectObject(hDC, hbmOrig);
        DeleteDC(hDC);

        data->hWMIconBitmap = ii.hbmColor;
        data->hWMIconMask = ii.hbmMask;

        hints->icon_pixmap = X11DRV_BITMAP_Pixmap(data->hWMIconBitmap);
        hints->icon_mask = X11DRV_BITMAP_Pixmap(data->hWMIconMask);
        destroy_icon_window( display, data );
        hints->flags = (hints->flags & ~IconWindowHint) | IconPixmapHint | IconMaskHint;
    }
}


/***********************************************************************
 *              set_size_hints
 *
 * set the window size hints
 */
static void set_size_hints( Display *display, WND *win )
{
    XSizeHints* size_hints;
    struct x11drv_win_data *data = win->pDriverData;

    if ((size_hints = XAllocSizeHints()))
    {
        size_hints->win_gravity = StaticGravity;
        size_hints->x = data->whole_rect.left;
        size_hints->y = data->whole_rect.top;
        size_hints->flags = PWinGravity | PPosition;

        if (HAS_DLGFRAME( win->dwStyle, win->dwExStyle ))
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
 *              set_wm_hints
 *
 * Set the window manager hints for a newly-created window
 */
inline static void set_wm_hints( Display *display, WND *win )
{
    struct x11drv_win_data *data = win->pDriverData;
    Window group_leader;
    XClassHint *class_hints;
    XWMHints* wm_hints;
    Atom protocols[2];
    int i;

    wine_tsx11_lock();

    /* wm protocols */
    i = 0;
    protocols[i++] = wmDeleteWindow;
    if (wmTakeFocus) protocols[i++] = wmTakeFocus;
    XSetWMProtocols( display, data->whole_window, protocols, i );

    /* class hints */
    if ((class_hints = XAllocClassHint()))
    {
        class_hints->res_name = "wine";
        class_hints->res_class = "Wine";
        XSetClassHint( display, data->whole_window, class_hints );
        XFree( class_hints );
    }

    /* transient for hint */
    if (win->owner)
    {
        struct x11drv_win_data *owner_data = win->owner->pDriverData;
        XSetTransientForHint( display, data->whole_window, owner_data->whole_window );
        group_leader = owner_data->whole_window;
    }
    else group_leader = data->whole_window;

    /* wm hints */
    if ((wm_hints = XAllocWMHints()))
    {
        wm_hints->flags = InputHint | StateHint | WindowGroupHint;
        /* use globally active model if take focus is supported,
         * passive model otherwise (cf. ICCCM) */
        wm_hints->input = !wmTakeFocus;

        set_icon_hints( display, win, wm_hints );

        wm_hints->initial_state = (win->dwStyle & WS_MINIMIZE) ? IconicState : NormalState;
        wm_hints->window_group = group_leader;

        XSetWMHints( display, data->whole_window, wm_hints );
        XFree(wm_hints);
    }

    /* size hints */
    set_size_hints( display, win );

    /* systray properties (KDE only for now) */
    if (win->dwExStyle & WS_EX_TRAYWINDOW)
    {
        int val = 1;
        if (kwmDockWindow != None)
            TSXChangeProperty( display, data->whole_window, kwmDockWindow, kwmDockWindow,
                               32, PropModeReplace, (char*)&val, 1 );
        if (_kde_net_wm_system_tray_window_for != None)
            TSXChangeProperty( display, data->whole_window, _kde_net_wm_system_tray_window_for,
                               XA_WINDOW, 32, PropModeReplace, (char*)&data->whole_window, 1 );
    }

    wine_tsx11_unlock();
}


/***********************************************************************
 *              X11DRV_set_iconic_state
 *
 * Set the X11 iconic state according to the window style.
 */
void X11DRV_set_iconic_state( WND *win )
{
    Display *display = thread_display();
    struct x11drv_win_data *data = win->pDriverData;
    XWMHints* wm_hints;
    BOOL iconic = IsIconic( win->hwndSelf );

    if (!(win->dwExStyle & WS_EX_MANAGED))
    {
        if (iconic) TSXUnmapWindow( display, data->client_window );
        else TSXMapWindow( display, data->client_window );
    }

    wine_tsx11_lock();

    if (!(wm_hints = XGetWMHints( display, data->whole_window ))) wm_hints = XAllocWMHints();
    wm_hints->flags |= StateHint | IconPositionHint;
    wm_hints->initial_state = iconic ? IconicState : NormalState;
    wm_hints->icon_x = win->rectWindow.left;
    wm_hints->icon_y = win->rectWindow.top;
    XSetWMHints( display, data->whole_window, wm_hints );

    if (win->dwStyle & WS_VISIBLE)
    {
        if (iconic)
            XIconifyWindow( display, data->whole_window, DefaultScreen(display) );
        else
            if (!IsRectEmpty( &win->rectWindow )) XMapWindow( display, data->whole_window );
    }

    XFree(wm_hints);
    wine_tsx11_unlock();
}


/***********************************************************************
 *		X11DRV_window_to_X_rect
 *
 * Convert a rect from client to X window coordinates
 */
void X11DRV_window_to_X_rect( WND *win, RECT *rect )
{
    if (!(win->dwExStyle & WS_EX_MANAGED)) return;
    if (win->dwStyle & WS_ICONIC) return;
    if (IsRectEmpty( rect )) return;

    if (HAS_THICKFRAME( win->dwStyle, win->dwExStyle ))
        InflateRect( rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
    else if (HAS_DLGFRAME( win->dwStyle, win->dwExStyle ))
        InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME) );
    else if (HAS_THINFRAME( win->dwStyle ))
        InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );

    if ((win->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        if (win->dwExStyle & WS_EX_TOOLWINDOW)
            rect->top += GetSystemMetrics(SM_CYSMCAPTION);
        else
            rect->top += GetSystemMetrics(SM_CYCAPTION);
    }

    if (win->dwExStyle & WS_EX_CLIENTEDGE)
        InflateRect( rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE) );
    if (win->dwExStyle & WS_EX_STATICEDGE)
        InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );

    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *		X11DRV_X_to_window_rect
 *
 * Opposite of X11DRV_window_to_X_rect
 */
void X11DRV_X_to_window_rect( WND *win, RECT *rect )
{
    if (!(win->dwExStyle & WS_EX_MANAGED)) return;
    if (win->dwStyle & WS_ICONIC) return;
    if (IsRectEmpty( rect )) return;

    if (HAS_THICKFRAME( win->dwStyle, win->dwExStyle ))
        InflateRect( rect, GetSystemMetrics(SM_CXFRAME), GetSystemMetrics(SM_CYFRAME) );
    else if (HAS_DLGFRAME( win->dwStyle, win->dwExStyle ))
        InflateRect( rect, GetSystemMetrics(SM_CXDLGFRAME), GetSystemMetrics(SM_CYDLGFRAME) );
    else if (HAS_THINFRAME( win->dwStyle ))
        InflateRect( rect, GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER) );

    if ((win->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        if (win->dwExStyle & WS_EX_TOOLWINDOW)
            rect->top -= GetSystemMetrics(SM_CYSMCAPTION);
        else
            rect->top -= GetSystemMetrics(SM_CYCAPTION);
    }

    if (win->dwExStyle & WS_EX_CLIENTEDGE)
        InflateRect( rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE) );
    if (win->dwExStyle & WS_EX_STATICEDGE)
        InflateRect( rect, GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER) );

    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *		X11DRV_sync_whole_window_position
 *
 * Synchronize the X whole window position with the Windows one
 */
int X11DRV_sync_whole_window_position( Display *display, WND *win, int zorder )
{
    XWindowChanges changes;
    int mask;
    struct x11drv_win_data *data = win->pDriverData;
    RECT whole_rect = win->rectWindow;

    X11DRV_window_to_X_rect( win, &whole_rect );
    mask = get_window_changes( &changes, &data->whole_rect, &whole_rect );

    if (zorder)
    {
        /* find window that this one must be after */
        WND *prev = win->parent->child;
        if (prev == win)  /* top child */
        {
            changes.stack_mode = Above;
            mask |= CWStackMode;
        }
        else
        {
            while (prev && prev->next != win) prev = prev->next;
            if (prev)
            {
                changes.stack_mode = Below;
                changes.sibling = get_whole_window(prev);
                mask |= CWStackMode | CWSibling;
            }
            else ERR( "previous window not found for %x, list corrupted?\n", win->hwndSelf );
        }
    }

    data->whole_rect = whole_rect;

    if (mask)
    {
        TRACE( "setting win %lx pos %d,%d,%dx%d after %lx changes=%x\n",
               data->whole_window, whole_rect.left, whole_rect.top,
               whole_rect.right - whole_rect.left, whole_rect.bottom - whole_rect.top,
               changes.sibling, mask );
        wine_tsx11_lock();
        XSync( gdi_display, False );  /* flush graphics operations before moving the window */
        if (is_window_top_level( win ))
        {
            if (mask & (CWWidth|CWHeight)) set_size_hints( display, win );
            XReconfigureWMWindow( display, data->whole_window,
                                  DefaultScreen(display), mask, &changes );
        }
        else XConfigureWindow( display, data->whole_window, mask, &changes );
        wine_tsx11_unlock();
    }
    return mask;
}


/***********************************************************************
 *		X11DRV_sync_client_window_position
 *
 * Synchronize the X client window position with the Windows one
 */
int X11DRV_sync_client_window_position( Display *display, WND *win )
{
    XWindowChanges changes;
    int mask;
    struct x11drv_win_data *data = win->pDriverData;
    RECT client_rect = win->rectClient;

    OffsetRect( &client_rect, -data->whole_rect.left, -data->whole_rect.top );
    /* client rect cannot be empty */
    if (client_rect.top >= client_rect.bottom) client_rect.bottom = client_rect.top + 1;
    if (client_rect.left >= client_rect.right) client_rect.right = client_rect.left + 1;

    if ((mask = get_window_changes( &changes, &data->client_rect, &client_rect )))
    {
        TRACE( "setting win %lx pos %d,%d,%dx%d (was %d,%d,%dx%d) after %lx changes=%x\n",
               data->client_window, client_rect.left, client_rect.top,
               client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
               data->client_rect.left, data->client_rect.top,
               data->client_rect.right - data->client_rect.left,
               data->client_rect.bottom - data->client_rect.top,
               changes.sibling, mask );
        data->client_rect = client_rect;
        wine_tsx11_lock();
        XSync( gdi_display, False );  /* flush graphics operations before moving the window */
        XConfigureWindow( display, data->client_window, mask, &changes );
        wine_tsx11_unlock();
    }
    return mask;
}


/***********************************************************************
 *		X11DRV_register_window
 *
 * Associate an X window to a HWND.
 */
void X11DRV_register_window( Display *display, HWND hwnd, struct x11drv_win_data *data )
{
    wine_tsx11_lock();
    XSaveContext( display, data->whole_window, winContext, (char *)hwnd );
    XSaveContext( display, data->client_window, winContext, (char *)hwnd );
    wine_tsx11_unlock();
}


/**********************************************************************
 *		create_desktop
 */
static void create_desktop( Display *display, WND *wndPtr )
{
    X11DRV_WND_DATA *data = wndPtr->pDriverData;

    wine_tsx11_lock();
    winContext     = XUniqueContext();
    wmProtocols    = XInternAtom( display, "WM_PROTOCOLS", False );
    wmDeleteWindow = XInternAtom( display, "WM_DELETE_WINDOW", False );
/*    wmTakeFocus    = XInternAtom( display, "WM_TAKE_FOCUS", False );*/
    wmTakeFocus = 0;  /* not yet */
    dndProtocol = XInternAtom( display, "DndProtocol" , False );
    dndSelection = XInternAtom( display, "DndSelection" , False );
    wmChangeState = XInternAtom (display, "WM_CHANGE_STATE", False);
    kwmDockWindow = XInternAtom( display, "KWM_DOCKWINDOW", False );
    _kde_net_wm_system_tray_window_for = XInternAtom( display, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False );
    wine_tsx11_unlock();

    data->whole_window = data->client_window = root_window;
    if (root_window != DefaultRootWindow(display))
    {
        wndPtr->flags |= WIN_NATIVE;
        X11DRV_create_desktop_thread();
    }
}


/**********************************************************************
 *		create_whole_window
 *
 * Create the whole X window for a given window
 */
static Window create_whole_window( Display *display, WND *win )
{
    struct x11drv_win_data *data = win->pDriverData;
    int cx, cy, mask;
    XSetWindowAttributes attr;
    Window parent;
    RECT rect;
    BOOL is_top_level = is_window_top_level( win );

    mask = get_window_attributes( win, &attr );

    rect = win->rectWindow;
    X11DRV_window_to_X_rect( win, &rect );

    if (!(cx = rect.right - rect.left)) cx = 1;
    if (!(cy = rect.bottom - rect.top)) cy = 1;

    parent = get_client_window( win->parent );

    wine_tsx11_lock();

    if (is_top_level) attr.cursor = X11DRV_GetCursor( display, GlobalLock16(GetCursor()) );
    data->whole_rect = rect;
    data->whole_window = XCreateWindow( display, parent, rect.left, rect.top, cx, cy,
                                        0, screen_depth, InputOutput, visual,
                                        mask, &attr );
    if (attr.cursor) XFreeCursor( display, attr.cursor );

    if (!data->whole_window) goto done;

    /* non-maximized child must be at bottom of Z order */
    if ((win->dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
    {
        XWindowChanges changes;
        changes.stack_mode = Below;
        XConfigureWindow( display, data->whole_window, CWStackMode, &changes );
    }

    if (is_top_level && !attr.override_redirect) set_wm_hints( display, win );

 done:
    wine_tsx11_unlock();
    return data->whole_window;
}


/**********************************************************************
 *		create_client_window
 *
 * Create the client window for a given window
 */
static Window create_client_window( Display *display, WND *win )
{
    struct x11drv_win_data *data = win->pDriverData;
    RECT rect;
    XSetWindowAttributes attr;

    rect = win->rectWindow;
    SendMessageW( win->hwndSelf, WM_NCCALCSIZE, FALSE, (LPARAM)&rect );

    if (rect.left > rect.right || rect.top > rect.bottom) rect = win->rectWindow;
    if (rect.top >= rect.bottom) rect.bottom = rect.top + 1;
    if (rect.left >= rect.right) rect.right = rect.left + 1;

    win->rectClient = rect;
    OffsetRect( &rect, -data->whole_rect.left, -data->whole_rect.top );
    data->client_rect = rect;

    attr.event_mask = (ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                       ButtonPressMask | ButtonReleaseMask);
    attr.bit_gravity = (win->clsStyle & (CS_VREDRAW | CS_HREDRAW)) ?
                       ForgetGravity : NorthWestGravity;
    attr.backing_store = NotUseful/*WhenMapped*/;

    data->client_window = TSXCreateWindow( display, data->whole_window,
                                           rect.left, rect.top,
                                           rect.right - rect.left,
                                           rect.bottom - rect.top,
                                           0, screen_depth,
                                           InputOutput, visual,
                                           CWEventMask | CWBitGravity | CWBackingStore, &attr );
    if (data->client_window) TSXMapWindow( display, data->client_window );
    return data->client_window;
}


/*****************************************************************
 *		SetWindowText   (X11DRV.@)
 */
BOOL X11DRV_SetWindowText( HWND hwnd, LPCWSTR text )
{
    Display *display = thread_display();
    UINT count;
    char *buffer;
    static UINT text_cp = (UINT)-1;
    Window win;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return FALSE;
    if ((win = get_whole_window(wndPtr)))
    {
        if (text_cp == (UINT)-1)
        {
            text_cp = PROFILE_GetWineIniInt("x11drv", "TextCP", CP_ACP);
            TRACE("text_cp = %u\n", text_cp);
        }

        /* allocate new buffer for window text */
        count = WideCharToMultiByte(text_cp, 0, text, -1, NULL, 0, NULL, NULL);
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, count * sizeof(WCHAR) )))
        {
            ERR("Not enough memory for window text\n");
            WIN_ReleaseWndPtr( wndPtr );
            return FALSE;
        }
        WideCharToMultiByte(text_cp, 0, text, -1, buffer, count, NULL, NULL);

        wine_tsx11_lock();
        XStoreName( display, win, buffer );
        XSetIconName( display, win, buffer );
        wine_tsx11_unlock();

        HeapFree( GetProcessHeap(), 0, buffer );
    }
    WIN_ReleaseWndPtr( wndPtr );
    return TRUE;
}


/***********************************************************************
 *		DestroyWindow   (X11DRV.@)
 */
BOOL X11DRV_DestroyWindow( HWND hwnd )
{
    Display *display = thread_display();
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    X11DRV_WND_DATA *data = wndPtr->pDriverData;

    if (!data) goto done;

    if (data->whole_window)
    {
        TRACE( "win %x xwin %lx/%lx\n", hwnd, data->whole_window, data->client_window );
        wine_tsx11_lock();
        XSync( gdi_display, False );  /* flush any reference to this drawable in GDI queue */
        XDeleteContext( display, data->whole_window, winContext );
        XDeleteContext( display, data->client_window, winContext );
        XDestroyWindow( display, data->whole_window );  /* this destroys client too */
        destroy_icon_window( display, data );
        wine_tsx11_unlock();
    }

    if (data->hWMIconBitmap) DeleteObject( data->hWMIconBitmap );
    if (data->hWMIconMask) DeleteObject( data->hWMIconMask);
    HeapFree( GetProcessHeap(), 0, data );
    wndPtr->pDriverData = NULL;
 done:
    WIN_ReleaseWndPtr( wndPtr );
    return TRUE;
}


/**********************************************************************
 *		CreateWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateWindow( HWND hwnd, CREATESTRUCTA *cs )
{
    Display *display = thread_display();
    WND *wndPtr;
    struct x11drv_win_data *data;
    BOOL ret = FALSE;

    if (!(data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data)))) return FALSE;
    data->whole_window  = 0;
    data->client_window = 0;
    data->icon_window   = 0;
    data->hWMIconBitmap = 0;
    data->hWMIconMask   = 0;

    wndPtr = WIN_FindWndPtr( hwnd );
    wndPtr->pDriverData = data;
    wndPtr->flags |= WIN_NATIVE;
    WIN_ReleaseWndPtr( wndPtr );

    if (IsWindowUnicode( hwnd ))
        ret = SendMessageW( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
    else
        ret = SendMessageA( hwnd, WM_NCCREATE, 0, (LPARAM)cs );

    if (!ret) goto failed;

    TRACE( "hwnd %x cs %d,%d %dx%d\n", hwnd, cs->x, cs->y, cs->cx, cs->cy );

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return FALSE;

    if (!wndPtr->parent)
    {
        create_desktop( display, wndPtr );
        WIN_ReleaseWndPtr( wndPtr );
        return TRUE;
    }

    if (!create_whole_window( display, wndPtr )) goto failed;
    if (!create_client_window( display, wndPtr )) goto failed;
    TSXSync( display, False );

    TRACE( "win %x window %d,%d,%d,%d client %d,%d,%d,%d whole %d,%d,%d,%d X client %d,%d,%d,%d xwin %x/%x\n",
           hwnd, wndPtr->rectWindow.left, wndPtr->rectWindow.top,
           wndPtr->rectWindow.right, wndPtr->rectWindow.bottom,
           wndPtr->rectClient.left, wndPtr->rectClient.top,
           wndPtr->rectClient.right, wndPtr->rectClient.bottom,
           data->whole_rect.left, data->whole_rect.top,
           data->whole_rect.right, data->whole_rect.bottom,
           data->client_rect.left, data->client_rect.top,
           data->client_rect.right, data->client_rect.bottom,
           (unsigned int)data->whole_window, (unsigned int)data->client_window );

    if ((wndPtr->dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
        WIN_LinkWindow( hwnd, HWND_BOTTOM );
    else
        WIN_LinkWindow( hwnd, HWND_TOP );

    if (wndPtr->text) X11DRV_SetWindowText( hwnd, wndPtr->text );
    X11DRV_register_window( display, hwnd, data );
    WIN_ReleaseWndPtr( wndPtr );

    if (IsWindowUnicode( hwnd ))
        ret = (SendMessageW( hwnd, WM_CREATE, 0, (LPARAM)cs ) != -1);
    else
        ret = (SendMessageA( hwnd, WM_CREATE, 0, (LPARAM)cs ) != -1);

    if (!ret)
    {
        WIN_UnlinkWindow( hwnd );
        goto failed;
    }

    /* Send the size messages */

    if (!(wndPtr->flags & WIN_NEED_SIZE))
    {
        /* send it anyway */
        if (((wndPtr->rectClient.right-wndPtr->rectClient.left) <0)
            ||((wndPtr->rectClient.bottom-wndPtr->rectClient.top)<0))
            WARN("sending bogus WM_SIZE message 0x%08lx\n",
                 MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                          wndPtr->rectClient.bottom-wndPtr->rectClient.top));
        SendMessageW( hwnd, WM_SIZE, SIZE_RESTORED,
                      MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                               wndPtr->rectClient.bottom-wndPtr->rectClient.top));
        SendMessageW( hwnd, WM_MOVE, 0,
                      MAKELONG( wndPtr->rectClient.left, wndPtr->rectClient.top ) );
    }

    /* Show the window, maximizing or minimizing if needed */

    if (wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE))
    {
        extern UINT WINPOS_MinMaximize( HWND hwnd, UINT cmd, LPRECT rect ); /*FIXME*/

        RECT newPos;
        UINT swFlag = (wndPtr->dwStyle & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;
        wndPtr->dwStyle &= ~(WS_MAXIMIZE | WS_MINIMIZE);
        WINPOS_MinMaximize( hwnd, swFlag, &newPos );
        swFlag = ((wndPtr->dwStyle & WS_CHILD) || GetActiveWindow())
            ? SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED
            : SWP_NOZORDER | SWP_FRAMECHANGED;
        SetWindowPos( hwnd, 0, newPos.left, newPos.top,
                      newPos.right, newPos.bottom, swFlag );
    }

    return TRUE;


 failed:
    X11DRV_DestroyWindow( wndPtr->hwndSelf );
    WIN_ReleaseWndPtr( wndPtr );
    return FALSE;
}


/***********************************************************************
 *		X11DRV_get_client_window
 *
 * Return the X window associated with the client area of a window
 */
Window X11DRV_get_client_window( HWND hwnd )
{
    Window ret = 0;
    WND *win = WIN_FindWndPtr( hwnd );
    if (win)
    {
        struct x11drv_win_data *data = win->pDriverData;
        ret = data->client_window;
        WIN_ReleaseWndPtr( win );
    }
    return ret;
}


/***********************************************************************
 *		X11DRV_get_whole_window
 *
 * Return the X window associated with the full area of a window
 */
Window X11DRV_get_whole_window( HWND hwnd )
{
    Window ret = 0;
    WND *win = WIN_FindWndPtr( hwnd );
    if (win)
    {
        struct x11drv_win_data *data = win->pDriverData;
        ret = data->whole_window;
        WIN_ReleaseWndPtr( win );
    }
    return ret;
}


/***********************************************************************
 *		X11DRV_get_top_window
 *
 * Return the X window associated with the top-level parent of a window
 */
Window X11DRV_get_top_window( HWND hwnd )
{
    Window ret = 0;
    WND *win = WIN_FindWndPtr( hwnd );
    while (win && win->parent->hwndSelf != GetDesktopWindow())
        WIN_UpdateWndPtr( &win, win->parent );
    if (win)
    {
        struct x11drv_win_data *data = win->pDriverData;
        ret = data->whole_window;
        WIN_ReleaseWndPtr( win );
    }
    return ret;
}


/*****************************************************************
 *		SetParent   (X11DRV.@)
 */
HWND X11DRV_SetParent( HWND hwnd, HWND parent )
{
    Display *display = thread_display();
    WND *wndPtr;
    WND *pWndParent;
    DWORD dwStyle;
    HWND retvalue;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return 0;

    dwStyle = wndPtr->dwStyle;

    if (!parent) parent = GetDesktopWindow();

    if (!(pWndParent = WIN_FindWndPtr(parent)))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return 0;
    }

    /* Windows hides the window first, then shows it again
     * including the WM_SHOWWINDOW messages and all */
    if (dwStyle & WS_VISIBLE) ShowWindow( hwnd, SW_HIDE );

    retvalue = wndPtr->parent->hwndSelf;  /* old parent */
    if (pWndParent != wndPtr->parent)
    {
        struct x11drv_win_data *data = wndPtr->pDriverData;
        int mask;
        XSetWindowAttributes attr;

        WIN_UnlinkWindow(wndPtr->hwndSelf);
        wndPtr->parent = pWndParent;
        WIN_LinkWindow(wndPtr->hwndSelf, HWND_TOP);

        mask = get_window_attributes( wndPtr, &attr );
        if (is_window_top_level( wndPtr ) && !attr.override_redirect)
            set_wm_hints( display, wndPtr );

        if (parent != GetDesktopWindow()) /* a child window */
        {
            if (!(wndPtr->dwStyle & WS_CHILD) && wndPtr->wIDmenu)
            {
                DestroyMenu( (HMENU)wndPtr->wIDmenu );
                wndPtr->wIDmenu = 0;
            }
        }

        wine_tsx11_lock();
        XChangeWindowAttributes( display, data->whole_window, mask, &attr );
        XReparentWindow( display, data->whole_window, get_client_window(pWndParent),
                         data->whole_rect.left, data->whole_rect.top );
        wine_tsx11_unlock();
    }
    WIN_ReleaseWndPtr( pWndParent );
    WIN_ReleaseWndPtr( wndPtr );

    /* SetParent additionally needs to make hwnd the topmost window
       in the x-order and send the expected WM_WINDOWPOSCHANGING and
       WM_WINDOWPOSCHANGED notification messages. 
    */
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                  SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|
                  ((dwStyle & WS_VISIBLE)?SWP_SHOWWINDOW:0));
    /* FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
     * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE */

    return retvalue;
}


/*******************************************************************
 *		EnableWindow   (X11DRV.@)
 */
BOOL X11DRV_EnableWindow( HWND hwnd, BOOL enable )
{
    Display *display = thread_display();
    XWMHints *wm_hints;
    WND *wndPtr;
    BOOL retvalue;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    retvalue = ((wndPtr->dwStyle & WS_DISABLED) != 0);

    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
        /* Enable window */
        wndPtr->dwStyle &= ~WS_DISABLED;

        if (wndPtr->dwExStyle & WS_EX_MANAGED)
        {
            wine_tsx11_lock();
            if (!(wm_hints = XGetWMHints( display, get_whole_window(wndPtr) )))
                wm_hints = XAllocWMHints();
            if (wm_hints)
            {
                wm_hints->flags |= InputHint;
                wm_hints->input = TRUE;
                XSetWMHints( display, get_whole_window(wndPtr), wm_hints );
                XFree(wm_hints);
            }
            wine_tsx11_unlock();
        }

        SendMessageA( hwnd, WM_ENABLE, TRUE, 0 );
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
        SendMessageA( wndPtr->hwndSelf, WM_CANCELMODE, 0, 0 );

        /* Disable window */
        wndPtr->dwStyle |= WS_DISABLED;

        if (wndPtr->dwExStyle & WS_EX_MANAGED)
        {
            wine_tsx11_lock();
            if (!(wm_hints = XGetWMHints( display, get_whole_window(wndPtr) )))
                wm_hints = XAllocWMHints();
            if (wm_hints)
            {
                wm_hints->flags |= InputHint;
                wm_hints->input = FALSE;
                XSetWMHints( display, get_whole_window(wndPtr), wm_hints );
                XFree(wm_hints);
            }
            wine_tsx11_unlock();
        }

        if (hwnd == GetFocus())
            SetFocus( 0 );  /* A disabled window can't have the focus */

        if (hwnd == GetCapture())
            ReleaseCapture();  /* A disabled window can't capture the mouse */

        SendMessageA( hwnd, WM_ENABLE, FALSE, 0 );
    }
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
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
    XWindowAttributes win_attr;
    Window win;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WND *w = wndPtr;

    if (!wndPtr) return;

    /* Only mess with the X focus if there's */
    /* no desktop window and if the window is not managed by the WM. */
    if (root_window != DefaultRootWindow(display)) goto done;

    while (w && !get_whole_window(w)) w = w->parent;
    if (!w) goto done;
    if (w->dwExStyle & WS_EX_MANAGED) goto done;

    if (!hwnd)  /* If setting the focus to 0, uninstall the colormap */
    {
        if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
            TSXUninstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
    }
    else if ((win = get_whole_window(w)))
    {
        /* Set X focus and install colormap */
        wine_tsx11_lock();
        if (XGetWindowAttributes( display, win, &win_attr ) &&
            (win_attr.map_state == IsViewable))
        {
            /* If window is not viewable, don't change anything */

            /* we must not use CurrentTime (ICCCM), so try to use last message time instead */
            /* FIXME: this is not entirely correct */
            XSetInputFocus( display, win, RevertToParent,
                            /*CurrentTime*/ GetMessageTime() + X11DRV_server_startticks );
            if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
                XInstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
        }
        wine_tsx11_unlock();
    }

 done:
    WIN_ReleaseWndPtr( wndPtr );
}


/**********************************************************************
 *		X11DRV_SetWindowIcon
 *
 * hIcon or hIconSm has changed (or is being initialised for the
 * first time). Complete the X11 driver-specific initialisation
 * and set the window hints.
 *
 * This is not entirely correct, may need to create
 * an icon window and set the pixmap as a background
 */
HICON X11DRV_SetWindowIcon( HWND hwnd, HICON icon, BOOL small )
{
    Display *display = thread_display();
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    int index = small ? GCL_HICONSM : GCL_HICON;
    HICON old;

    if (!wndPtr) return 0;

    old = GetClassLongW( hwnd, index );
    SetClassLongW( hwnd, index, icon );

    SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE |
                  SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );

    if (wndPtr->dwExStyle & WS_EX_MANAGED)
    {
        Window win = get_whole_window(wndPtr);
        XWMHints* wm_hints = TSXGetWMHints( display, win );

        if (!wm_hints) wm_hints = TSXAllocWMHints();
        if (wm_hints)
        {
            set_icon_hints( display, wndPtr, wm_hints );
            TSXSetWMHints( display, win, wm_hints );
            TSXFree( wm_hints );
        }
    }

    WIN_ReleaseWndPtr( wndPtr );
    return old;
}

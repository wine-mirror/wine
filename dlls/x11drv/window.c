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

DEFAULT_DEBUG_CHANNEL(win);

extern Cursor X11DRV_MOUSE_XCursor;  /* current X cursor */
extern Pixmap X11DRV_BITMAP_Pixmap( HBITMAP );

#define HAS_DLGFRAME(style,exStyle) \
((!((style) & WS_THICKFRAME)) && (((style) & WS_DLGFRAME) || ((exStyle) & WS_EX_DLGMODALFRAME)))

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

Atom wmProtocols = None;
Atom wmDeleteWindow = None;
Atom dndProtocol = None;
Atom dndSelection = None;
Atom wmChangeState = None;
Atom kwmDockWindow = None;
Atom _kde_net_wm_system_tray_window_for = None; /* KDE 2 Final */


/***********************************************************************
 *		register_window
 *
 * Associate an X window to a HWND.
 */
static void register_window( HWND hwnd, Window win )
{
    if (!winContext) winContext = TSXUniqueContext();
    TSXSaveContext( display, win, winContext, (char *)hwnd );
    TSXSetWMProtocols( display, win, &wmDeleteWindow, 1 );
}


/***********************************************************************
 *              set_wm_hint
 *
 * Set a window manager hint.
 */
static void set_wm_hint( Window win, int hint, int val )
{
    XWMHints* wm_hints = TSXGetWMHints( display, win );
    if (!wm_hints) wm_hints = TSXAllocWMHints();
    if (wm_hints)
    {
        wm_hints->flags = hint;
        switch( hint )
        {
        case InputHint:
            wm_hints->input = val;
            break;

        case StateHint:
            wm_hints->initial_state = val;
            break;

        case IconPixmapHint:
            wm_hints->icon_pixmap = (Pixmap)val;
            break;

        case IconWindowHint:
            wm_hints->icon_window = (Window)val;
            break;
        }
        TSXSetWMHints( display, win, wm_hints );
        TSXFree(wm_hints);
    }
}


/***********************************************************************
 *              set_icon_hints
 *
 * Set the icon wm hints
 */
static void set_icon_hints( WND *wndPtr, XWMHints *hints )
{
    X11DRV_WND_DATA *data = wndPtr->pDriverData;
    HICON hIcon = GetClassLongA( wndPtr->hwndSelf, GCL_HICON );

    if (data->hWMIconBitmap) DeleteObject( data->hWMIconBitmap );
    if (data->hWMIconMask) DeleteObject( data->hWMIconMask);

    if (!hIcon)
    {
        data->hWMIconBitmap = 0;
        data->hWMIconMask = 0;
        hints->flags &= ~(IconPixmapHint | IconMaskHint);
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
        hints->flags |= IconPixmapHint | IconMaskHint;
    }
}


/***********************************************************************
 *		dock_window
 *
 * Set the X Property of the window that tells the windowmanager we really
 * want to be in the systray
 *
 * KDE: set "KWM_DOCKWINDOW", type "KWM_DOCKWINDOW" to 1 before a window is 
 * 	mapped.
 *
 * all others: to be added ;)
 */
inline static void dock_window( Window win )
{
    int data = 1;
    if (kwmDockWindow != None)
        TSXChangeProperty( display, win, kwmDockWindow, kwmDockWindow,
                           32, PropModeReplace, (char*)&data, 1 );
    if (_kde_net_wm_system_tray_window_for != None)
        TSXChangeProperty( display, win, _kde_net_wm_system_tray_window_for, XA_WINDOW,
                           32, PropModeReplace, (char*)&win, 1 );
}


/**********************************************************************
 *		create_desktop
 */
static void create_desktop(WND *wndPtr)
{
    X11DRV_WND_DATA *data = wndPtr->pDriverData;

    wmProtocols = TSXInternAtom( display, "WM_PROTOCOLS", True );
    wmDeleteWindow = TSXInternAtom( display, "WM_DELETE_WINDOW", True );
    dndProtocol = TSXInternAtom( display, "DndProtocol" , False );
    dndSelection = TSXInternAtom( display, "DndSelection" , False );
    wmChangeState = TSXInternAtom (display, "WM_CHANGE_STATE", False);
    kwmDockWindow = TSXInternAtom( display, "KWM_DOCKWINDOW", False );
    _kde_net_wm_system_tray_window_for = TSXInternAtom( display, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False );

    data->window = root_window;
    if (root_window != DefaultRootWindow(display)) wndPtr->flags |= WIN_NATIVE;
    register_window( wndPtr->hwndSelf, root_window );
}


/**********************************************************************
 *		CreateWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateWindow( HWND hwnd )
{
    X11DRV_WND_DATA *data;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    int x = wndPtr->rectWindow.left;
    int y = wndPtr->rectWindow.top;
    int cx = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    int cy = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    if (!(data = HeapAlloc(GetProcessHeap(), 0, sizeof(X11DRV_WND_DATA))))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return FALSE;
    }
    data->window = 0;
    wndPtr->pDriverData = data;

    if (!wndPtr->parent)
    {
        create_desktop( wndPtr );
        WIN_ReleaseWndPtr( wndPtr );
        return TRUE;
    }

    /* Create the X window (only for top-level windows, and then only */
    /* when there's no desktop window) */

    if ((root_window == DefaultRootWindow(display))
        && (wndPtr->parent->hwndSelf == GetDesktopWindow()))
    {
        Window    wGroupLeader;
        XWMHints* wm_hints;
        XSetWindowAttributes win_attr;

        /* Create "managed" windows only if a title bar or resizable */
        /* frame is required. */
        if (WIN_WindowNeedsWMBorder(wndPtr->dwStyle, wndPtr->dwExStyle))
        {
            win_attr.event_mask = ExposureMask | KeyPressMask |
                KeyReleaseMask | PointerMotionMask |
                ButtonPressMask | ButtonReleaseMask |
                FocusChangeMask | StructureNotifyMask;
            win_attr.override_redirect = FALSE;
            wndPtr->dwExStyle |= WS_EX_MANAGED;
        }
        else
        {
            win_attr.event_mask = ExposureMask | KeyPressMask |
                KeyReleaseMask | PointerMotionMask |
                ButtonPressMask | ButtonReleaseMask |
                FocusChangeMask;
            win_attr.override_redirect = TRUE;
        }
        wndPtr->flags |= WIN_NATIVE;

        win_attr.bit_gravity   = (wndPtr->clsStyle & (CS_VREDRAW | CS_HREDRAW)) ? ForgetGravity : NorthWestGravity;
        win_attr.colormap      = X11DRV_PALETTE_PaletteXColormap;
        win_attr.backing_store = NotUseful;
        win_attr.save_under    = ((wndPtr->clsStyle & CS_SAVEBITS) != 0);
        win_attr.cursor        = X11DRV_MOUSE_XCursor;

        data->hWMIconBitmap = 0;
        data->hWMIconMask = 0;
        data->bit_gravity = win_attr.bit_gravity;

        /* Zero-size X11 window hack.  X doesn't like them, and will crash */
        /* with a BadValue unless we do something ugly like this. */
        /* Zero size window won't be mapped  */
        if (cx <= 0) cx = 1;
        if (cy <= 0) cy = 1;

        data->window = TSXCreateWindow( display, root_window,
                                        x, y, cx, cy,
                                        0, screen_depth,
                                        InputOutput, visual,
                                        CWEventMask | CWOverrideRedirect |
                                        CWColormap | CWCursor | CWSaveUnder |
                                        CWBackingStore | CWBitGravity,
                                        &win_attr );

        if(!(wGroupLeader = X11DRV_WND_GetXWindow(wndPtr)))
        {
            HeapFree( GetProcessHeap(), 0, data );
            WIN_ReleaseWndPtr( wndPtr );
            return FALSE;
        }

        /* If we are the systray, we need to be managed to be noticed by KWM */
        if (wndPtr->dwExStyle & WS_EX_TRAYWINDOW) dock_window( data->window );

        if (wndPtr->dwExStyle & WS_EX_MANAGED)
        {
            XClassHint *class_hints = TSXAllocClassHint();
            XSizeHints* size_hints = TSXAllocSizeHints();

            if (class_hints)
            {
                class_hints->res_name = "wineManaged";
                class_hints->res_class = "Wine";
                TSXSetClassHint( display, data->window, class_hints );
                TSXFree (class_hints);
            }

            if (size_hints)
            {
                size_hints->win_gravity = StaticGravity;
                size_hints->x = x;
                size_hints->y = y;
                size_hints->flags = PWinGravity|PPosition;

                if (HAS_DLGFRAME(wndPtr->dwStyle,wndPtr->dwExStyle))
                {
                    size_hints->min_width = size_hints->max_width = cx;
                    size_hints->min_height = size_hints->max_height = cy;
                    size_hints->flags |= PMinSize | PMaxSize;
                }

                TSXSetWMSizeHints( display, X11DRV_WND_GetXWindow(wndPtr), 
                                   size_hints, XA_WM_NORMAL_HINTS );
                TSXFree(size_hints);
            }
        }

        if (wndPtr->owner)  /* Get window owner */
        {
            Window w = X11DRV_WND_FindXWindow( wndPtr->owner );
            if (w != None)
            {
                TSXSetTransientForHint( display, X11DRV_WND_GetXWindow(wndPtr), w );
                wGroupLeader = w;
            }
        }

        if ((wm_hints = TSXAllocWMHints()))
        {
            wm_hints->flags = InputHint | StateHint | WindowGroupHint;
            wm_hints->input = True;

            if (wndPtr->dwExStyle & WS_EX_MANAGED)
            {
                set_icon_hints( wndPtr, wm_hints );
                wm_hints->initial_state = (wndPtr->dwStyle & WS_MINIMIZE)
                    ? IconicState : NormalState;
            }
            else
                wm_hints->initial_state = NormalState;
            wm_hints->window_group = wGroupLeader;

            TSXSetWMHints( display, X11DRV_WND_GetXWindow(wndPtr), wm_hints );
            TSXFree(wm_hints);
        }
        register_window( hwnd, data->window );
    }
    WIN_ReleaseWndPtr( wndPtr );
    return TRUE;
}


/***********************************************************************
 *		DestroyWindow   (X11DRV.@)
 */
BOOL X11DRV_DestroyWindow( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    X11DRV_WND_DATA *data = wndPtr->pDriverData;
    Window w;

    if (data && (w = data->window))
    {
        XEvent xe;
        TSXDeleteContext( display, w, winContext );
        TSXDestroyWindow( display, w );
        while( TSXCheckWindowEvent(display, w, NoEventMask, &xe) );

        data->window = None;
        if( data->hWMIconBitmap )
        {
            DeleteObject( data->hWMIconBitmap );
            data->hWMIconBitmap = 0;
        }
        if( data->hWMIconMask )
        {
            DeleteObject( data->hWMIconMask);
            data->hWMIconMask= 0;
        }
    }
    HeapFree( GetProcessHeap(), 0, data );
    wndPtr->pDriverData = NULL;
    WIN_ReleaseWndPtr( wndPtr );
    return TRUE;
}


/*****************************************************************
 *		SetParent   (X11DRV.@)
 */
HWND X11DRV_SetParent( HWND hwnd, HWND parent )
{
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
        if ( X11DRV_WND_GetXWindow(wndPtr) )
        {
            /* Toplevel window needs to be reparented.  Used by Tk 8.0 */
            TSXDestroyWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
            ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = None;
        }
        WIN_UnlinkWindow(wndPtr->hwndSelf);
        wndPtr->parent = pWndParent;

        /* Create an X counterpart for reparented top-level windows
	     * when not in the desktop mode. */
        if (parent == GetDesktopWindow())
        {
            if(root_window == DefaultRootWindow(display))
                X11DRV_CreateWindow(wndPtr->hwndSelf);
        }
        else /* a child window */
        {
            if( !( wndPtr->dwStyle & WS_CHILD ) )
            {
                if( wndPtr->wIDmenu != 0)
                {
                    DestroyMenu( (HMENU) wndPtr->wIDmenu );
                    wndPtr->wIDmenu = 0;
                }
            }
        }
        WIN_LinkWindow(wndPtr->hwndSelf, HWND_TOP);
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
    WND *wndPtr;
    BOOL retvalue;
    Window w;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    retvalue = ((wndPtr->dwStyle & WS_DISABLED) != 0);

    if (enable && (wndPtr->dwStyle & WS_DISABLED))
    {
        /* Enable window */
        wndPtr->dwStyle &= ~WS_DISABLED;

        if ((wndPtr->dwExStyle & WS_EX_MANAGED) && (w = X11DRV_WND_GetXWindow( wndPtr )))
            set_wm_hint( w, InputHint, TRUE );

        SendMessageA( hwnd, WM_ENABLE, TRUE, 0 );
    }
    else if (!enable && !(wndPtr->dwStyle & WS_DISABLED))
    {
        SendMessageA( wndPtr->hwndSelf, WM_CANCELMODE, 0, 0 );

        /* Disable window */
        wndPtr->dwStyle |= WS_DISABLED;

        if ((wndPtr->dwExStyle & WS_EX_MANAGED) && (w = X11DRV_WND_GetXWindow( wndPtr )))
            set_wm_hint( w, InputHint, FALSE );

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
    XWindowAttributes win_attr;
    Window win;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WND *w = wndPtr;

    if (!wndPtr) return;

    /* Only mess with the X focus if there's */
    /* no desktop window and if the window is not managed by the WM. */
    if (root_window != DefaultRootWindow(display)) goto done;

    while (w && !((X11DRV_WND_DATA *) w->pDriverData)->window)
        w = w->parent;
    if (!w) w = wndPtr;
    if (w->dwExStyle & WS_EX_MANAGED) goto done;

    if (!hwnd)  /* If setting the focus to 0, uninstall the colormap */
    {
        if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
            TSXUninstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
    }
    else if ((win = X11DRV_WND_FindXWindow(wndPtr)))
    {
        /* Set X focus and install colormap */
        if (TSXGetWindowAttributes( display, win, &win_attr ) &&
            (win_attr.map_state == IsViewable))
        {
            /* If window is not viewable, don't change anything */
            TSXSetInputFocus( display, win, RevertToParent, CurrentTime );
            if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
                TSXInstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
            X11DRV_Synchronize();
        }
    }

 done:
    WIN_ReleaseWndPtr( wndPtr );
}


/*****************************************************************
 *		SetWindowText   (X11DRV.@)
 */
BOOL X11DRV_SetWindowText( HWND hwnd, LPCWSTR text )
{
    UINT count;
    char *buffer;
    static UINT text_cp = (UINT)-1;
    Window win;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return FALSE;
    if ((win = X11DRV_WND_GetXWindow(wndPtr)))
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

        TSXStoreName( display, win, buffer );
        TSXSetIconName( display, win, buffer );
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    WIN_ReleaseWndPtr( wndPtr );
    return TRUE;
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
        Window win = X11DRV_WND_GetXWindow(wndPtr);
        XWMHints* wm_hints = TSXGetWMHints( display, win );

        if (!wm_hints) wm_hints = TSXAllocWMHints();
        if (wm_hints)
        {
            set_icon_hints( wndPtr, wm_hints );
            TSXSetWMHints( display, win, wm_hints );
            TSXFree( wm_hints );
        }
    }

    WIN_ReleaseWndPtr( wndPtr );
    return old;
}


/*************************************************************************
 *             fix_caret
 */
static BOOL fix_caret(HWND hWnd, LPRECT lprc, UINT flags)
{
   HWND hCaret = CARET_GetHwnd();

   if( hCaret )
   {
       RECT rc;
       CARET_GetRect( &rc );
       if( hCaret == hWnd ||
          (flags & SW_SCROLLCHILDREN && IsChild(hWnd, hCaret)) )
       {
           POINT pt;
           pt.x = rc.left;
           pt.y = rc.top;
           MapWindowPoints( hCaret, hWnd, (LPPOINT)&rc, 2 );
           if( IntersectRect(lprc, lprc, &rc) )
           {
               HideCaret(0);
               lprc->left = pt.x;
               lprc->top = pt.y;
               return TRUE;
           }
       }
   }
   return FALSE;
}


/*************************************************************************
 *		ScrollWindowEx   (X11DRV.@)
 */
INT X11DRV_ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                           const RECT *rect, const RECT *clipRect,
                           HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags )
{
    INT  retVal = NULLREGION;
    BOOL bCaret = FALSE, bOwnRgn = TRUE;
    RECT rc, cliprc;
    WND*   wnd = WIN_FindWndPtr( hwnd );

    if( !wnd || !WIN_IsWindowDrawable( wnd, TRUE ))
    {
        retVal = ERROR;
        goto END;
    }

    GetClientRect(hwnd, &rc);
    if (rect) IntersectRect(&rc, &rc, rect);

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (!IsRectEmpty(&cliprc) && (dx || dy))
    {
        HDC   hDC;
        BOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
        HRGN  hrgnClip = CreateRectRgnIndirect(&cliprc);
        HRGN  hrgnTemp = CreateRectRgnIndirect(&rc);
        RECT  caretrc;

        TRACE("%04x, %d,%d hrgnUpdate=%04x rcUpdate = %p cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d) %04x\n",
              (HWND16)hwnd, dx, dy, hrgnUpdate, rcUpdate,
              clipRect?clipRect->left:0, clipRect?clipRect->top:0,
              clipRect?clipRect->right:0, clipRect?clipRect->bottom:0,
              rc.left, rc.top, rc.right, rc.bottom, (UINT16)flags );

        caretrc = rc;
        bCaret = fix_caret(hwnd, &caretrc, flags);

        if( hrgnUpdate ) bOwnRgn = FALSE;
        else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

        hDC = GetDCEx( hwnd, hrgnClip, DCX_CACHE | DCX_USESTYLE |
                         DCX_KEEPCLIPRGN | DCX_INTERSECTRGN |
                       ((flags & SW_SCROLLCHILDREN) ? DCX_NOCLIPCHILDREN : 0) );
        if (hDC)
        {
            X11DRV_WND_SurfaceCopy(wnd,hDC,dx,dy,&rc,bUpdate);
            if( bUpdate )
            {
                DC* dc;

                if( (dc = DC_GetDCPtr(hDC)) )
                {
                    OffsetRgn( hrgnTemp, dc->DCOrgX, dc->DCOrgY );
                    CombineRgn( hrgnTemp, hrgnTemp, dc->hVisRgn,
                                RGN_AND );
                    OffsetRgn( hrgnTemp, -dc->DCOrgX, -dc->DCOrgY );
                    CombineRgn( hrgnUpdate, hrgnTemp, hrgnClip,
                                RGN_AND );
                    OffsetRgn( hrgnTemp, dx, dy );
                    retVal =
                        CombineRgn( hrgnUpdate, hrgnUpdate, hrgnTemp,
                                    RGN_DIFF );

                    if( rcUpdate ) GetRgnBox( hrgnUpdate, rcUpdate );
                    GDI_ReleaseObj( hDC );
                }
            }
            ReleaseDC(hwnd, hDC);
        }

        if( wnd->hrgnUpdate > 1 )
        {
            /* Takes into account the fact that some damages may have
               occured during the scroll. */
            CombineRgn( hrgnTemp, wnd->hrgnUpdate, 0, RGN_COPY );
            OffsetRgn( hrgnTemp, dx, dy );
            CombineRgn( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
            CombineRgn( wnd->hrgnUpdate, wnd->hrgnUpdate, hrgnTemp, RGN_OR );
        }

        if( flags & SW_SCROLLCHILDREN )
        {
            RECT r;
            WND *w;
            for( w =WIN_LockWndPtr(wnd->child); w; WIN_UpdateWndPtr(&w, w->next))
            {
                r = w->rectWindow;
                if( !rect || IntersectRect(&r, &r, &rc) )
                    SetWindowPos(w->hwndSelf, 0, w->rectWindow.left + dx,
                                 w->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
                                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                                 SWP_DEFERERASE );
            }
        }

        if( flags & (SW_INVALIDATE | SW_ERASE) )
            RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                          ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

        if( bCaret )
        {
            SetCaretPos( caretrc.left + dx, caretrc.top + dy );
            ShowCaret(0);
        }

        if( bOwnRgn && hrgnUpdate ) DeleteObject( hrgnUpdate );
        DeleteObject( hrgnClip );
        DeleteObject( hrgnTemp );
    }
END:
    WIN_ReleaseWndPtr(wnd);
    return retVal;
}

/*
 * TTY window driver
 *
 * Copyright 1998,1999 Patrik Stridvall
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

#include "ttydrv.h"
#include "ntstatus.h"
#include "win.h"
#include "winpos.h"
#include "wownt32.h"
#include "wine/wingdi16.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

#define SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

/***********************************************************************
 *		get_server_visible_region
 */
static HRGN get_server_visible_region( HWND hwnd, HWND top, UINT flags )
{
    RGNDATA *data;
    NTSTATUS status;
    HRGN ret = 0;
    size_t size = 256;

    do
    {
        if (!(data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 ))) return 0;
        SERVER_START_REQ( get_visible_region )
        {
            req->window  = hwnd;
            req->top_win = top;
            req->flags   = flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                ret = ExtCreateRegion( NULL, size, data );
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           set_window_pos
 *
 * Set a window position and Z order.
 */
static void set_window_pos( HWND hwnd, HWND insert_after, const RECT *rectWindow,
                            const RECT *rectClient, UINT swp_flags )
{
    WND *win = WIN_GetPtr( hwnd );
    BOOL ret;

    if (!win) return;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) ERR( "cannot set rectangles of other process window %p\n", hwnd );
        return;
    }
    SERVER_START_REQ( set_window_pos )
    {
        req->handle        = hwnd;
        req->top_win       = 0;
        req->previous      = insert_after;
        req->flags         = swp_flags;
        req->window.left   = rectWindow->left;
        req->window.top    = rectWindow->top;
        req->window.right  = rectWindow->right;
        req->window.bottom = rectWindow->bottom;
        req->client.left   = rectClient->left;
        req->client.top    = rectClient->top;
        req->client.right  = rectClient->right;
        req->client.bottom = rectClient->bottom;
        ret = !wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret)
    {
        win->rectWindow = *rectWindow;
        win->rectClient = *rectClient;

        TRACE( "win %p window (%ld,%ld)-(%ld,%ld) client (%ld,%ld)-(%ld,%ld)\n", hwnd,
               rectWindow->left, rectWindow->top, rectWindow->right, rectWindow->bottom,
               rectClient->left, rectClient->top, rectClient->right, rectClient->bottom );
    }
    WIN_ReleasePtr( win );
}


/**********************************************************************
 *		CreateWindow   (TTYDRV.@)
 */
BOOL TTYDRV_CreateWindow( HWND hwnd, CREATESTRUCTA *cs, BOOL unicode )
{
    BOOL ret;
    RECT rect;
    HWND parent, hwndLinkAfter;
    CBT_CREATEWNDA cbtc;

    TRACE("(%p)\n", hwnd);

    /* initialize the dimensions before sending WM_GETMINMAXINFO */
    SetRect( &rect, cs->x, cs->y, cs->x + cs->cx, cs->y + cs->cy );
    set_window_pos( hwnd, 0, &rect, &rect, SWP_NOZORDER );

    parent = GetAncestor( hwnd, GA_PARENT );
    if (!parent)  /* desktop window */
    {
        SetPropA( hwnd, "__wine_ttydrv_window", root_window );
        return TRUE;
    }

#ifdef WINE_CURSES
    /* Only create top-level windows */
    if (parent == GetDesktopWindow())
    {
        WINDOW *window;
        const INT cellWidth=8, cellHeight=8; /* FIXME: Hardcoded */

        window = subwin( root_window, cs->cy/cellHeight, cs->cx/cellWidth,
                         cs->y/cellHeight, cs->x/cellWidth);
        werase(window);
        wrefresh(window);
        SetPropA( hwnd, "__wine_ttydrv_window", window );
    }
#else /* defined(WINE_CURSES) */
    FIXME("(%p): stub\n", hwnd);
#endif /* defined(WINE_CURSES) */

    /* Call the WH_CBT hook */

    hwndLinkAfter = ((cs->style & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD) ? HWND_BOTTOM : HWND_TOP;

    cbtc.lpcs = cs;
    cbtc.hwndInsertAfter = hwndLinkAfter;
    if (HOOK_CallHooks( WH_CBT, HCBT_CREATEWND, (WPARAM)hwnd, (LPARAM)&cbtc, unicode ))
    {
        TRACE("CBT-hook returned !0\n");
        return FALSE;
    }

    if (unicode)
    {
        ret = SendMessageW( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
        if (ret) ret = (SendMessageW( hwnd, WM_CREATE, 0, (LPARAM)cs ) != -1);
    }
    else
    {
        ret = SendMessageA( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
        if (ret) ret = (SendMessageA( hwnd, WM_CREATE, 0, (LPARAM)cs ) != -1);
    }
    if (ret) NotifyWinEvent(EVENT_OBJECT_CREATE, hwnd, OBJID_WINDOW, 0);
    return ret;
}

/***********************************************************************
 *		DestroyWindow   (TTYDRV.@)
 */
BOOL TTYDRV_DestroyWindow( HWND hwnd )
{
#ifdef WINE_CURSES
    WINDOW *window = GetPropA( hwnd, "__wine_ttydrv_window" );

    TRACE("(%p)\n", hwnd);

    if (window && window != root_window) delwin(window);
#else /* defined(WINE_CURSES) */
    FIXME("(%p): stub\n", hwnd);
#endif /* defined(WINE_CURSES) */
    return TRUE;
}


/***********************************************************************
 *		GetDC   (TTYDRV.@)
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
BOOL TTYDRV_GetDC( HWND hwnd, HDC hdc, HRGN hrgn, DWORD flags )
{
    struct ttydrv_escape_set_drawable escape;

    if(flags & DCX_WINDOW)
    {
        RECT rect;
        GetWindowRect( hwnd, &rect );
        escape.org.x = rect.left;
        escape.org.y = rect.top;
    }
    else
    {
        escape.org.x = escape.org.y = 0;
        MapWindowPoints( hwnd, 0, &escape.org, 1 );
    }

    escape.code = TTYDRV_SET_DRAWABLE;
    ExtEscape( hdc, TTYDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) ||
        SetHookFlags16( HDC_16(hdc), DCHF_VALIDATEVISRGN ))  /* DC was dirty */
    {
        /* need to recompute the visible region */
        HRGN visRgn = get_server_visible_region( hwnd, GetDesktopWindow(), flags );

        if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
            CombineRgn( visRgn, visRgn, hrgn, (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

        SelectVisRgn16( HDC_16(hdc), HRGN_16(visRgn) );
        DeleteObject( visRgn );
    }
    return TRUE;
}


/***********************************************************************
 *		SetWindowPos   (TTYDRV.@)
 */
BOOL TTYDRV_SetWindowPos( WINDOWPOS *winpos )
{
    WND *wndPtr;
    RECT newWindowRect, newClientRect;
    UINT wvrFlags = 0;
    BOOL retvalue;
    HWND hwndActive = GetForegroundWindow();

    TRACE( "hwnd %p, swp (%i,%i)-(%i,%i) flags %08x\n",
           winpos->hwnd, winpos->x, winpos->y,
           winpos->x + winpos->cx, winpos->y + winpos->cy, winpos->flags);

    /* ------------------------------------------------------------------------ CHECKS */

      /* Check window handle */

    if (winpos->hwnd == GetDesktopWindow()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( winpos->hwnd ))) return FALSE;

    TRACE("\tcurrent (%ld,%ld)-(%ld,%ld), style %08x\n",
          wndPtr->rectWindow.left, wndPtr->rectWindow.top,
          wndPtr->rectWindow.right, wndPtr->rectWindow.bottom, (unsigned)wndPtr->dwStyle );

    /* Fix redundant flags */

    if(wndPtr->dwStyle & WS_VISIBLE)
        winpos->flags &= ~SWP_SHOWWINDOW;
    else
    {
        if (!(winpos->flags & SWP_SHOWWINDOW)) winpos->flags |= SWP_NOREDRAW;
        winpos->flags &= ~SWP_HIDEWINDOW;
    }

    if ( winpos->cx < 0 ) winpos->cx = 0;
    if ( winpos->cy < 0 ) winpos->cy = 0;

    if ((wndPtr->rectWindow.right - wndPtr->rectWindow.left == winpos->cx) &&
        (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top == winpos->cy))
        winpos->flags |= SWP_NOSIZE;    /* Already the right size */

    if ((wndPtr->rectWindow.left == winpos->x) && (wndPtr->rectWindow.top == winpos->y))
        winpos->flags |= SWP_NOMOVE;    /* Already the right position */

    if (winpos->hwnd == hwndActive)
        winpos->flags |= SWP_NOACTIVATE;   /* Already active */
    else if ( (wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD )
    {
        if(!(winpos->flags & SWP_NOACTIVATE)) /* Bring to the top when activating */
        {
            winpos->flags &= ~SWP_NOZORDER;
            winpos->hwndInsertAfter = HWND_TOP;
            goto Pos;
        }
    }

    /* Check hwndInsertAfter */

      /* FIXME: TOPMOST not supported yet */
    if ((winpos->hwndInsertAfter == HWND_TOPMOST) ||
        (winpos->hwndInsertAfter == HWND_NOTOPMOST)) winpos->hwndInsertAfter = HWND_TOP;

    /* hwndInsertAfter must be a sibling of the window */
    if ((winpos->hwndInsertAfter != HWND_TOP) && (winpos->hwndInsertAfter != HWND_BOTTOM))
    {
        if (GetAncestor( winpos->hwndInsertAfter, GA_PARENT ) != wndPtr->parent)
        {
            retvalue = FALSE;
            goto END;
        }
        /* don't need to change the Zorder of hwnd if it's already inserted
         * after hwndInsertAfter or when inserting hwnd after itself.
         */
        if ((winpos->hwnd == winpos->hwndInsertAfter) ||
            (winpos->hwnd == GetWindow( winpos->hwndInsertAfter, GW_HWNDNEXT )))
            winpos->flags |= SWP_NOZORDER;
    }

 Pos:  /* ------------------------------------------------------------------------ MAIN part */

      /* Send WM_WINDOWPOSCHANGING message */

    if (!(winpos->flags & SWP_NOSENDCHANGING))
        SendMessageA( wndPtr->hwndSelf, WM_WINDOWPOSCHANGING, 0, (LPARAM)winpos );

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = (wndPtr->dwStyle & WS_MINIMIZE) ? wndPtr->rectWindow
                                                    : wndPtr->rectClient;

    if (!(winpos->flags & SWP_NOSIZE))
    {
        newWindowRect.right  = newWindowRect.left + winpos->cx;
        newWindowRect.bottom = newWindowRect.top + winpos->cy;
    }
    if (!(winpos->flags & SWP_NOMOVE))
    {
        newWindowRect.left    = winpos->x;
        newWindowRect.top     = winpos->y;
        newWindowRect.right  += winpos->x - wndPtr->rectWindow.left;
        newWindowRect.bottom += winpos->y - wndPtr->rectWindow.top;

        OffsetRect( &newClientRect, winpos->x - wndPtr->rectWindow.left,
                    winpos->y - wndPtr->rectWindow.top );
    }

    if( winpos->hwndInsertAfter == HWND_TOP )
    {
        if (GetWindow( wndPtr->hwndSelf, GW_HWNDFIRST ) == wndPtr->hwndSelf)
            winpos->flags |= SWP_NOZORDER;
    }
    else
        if( winpos->hwndInsertAfter == HWND_BOTTOM )
        {
            if (!GetWindow( wndPtr->hwndSelf, GW_HWNDNEXT ))
                winpos->flags |= SWP_NOZORDER;
        }
        else
            if( !(winpos->flags & SWP_NOZORDER) )
                if( GetWindow(winpos->hwndInsertAfter, GW_HWNDNEXT) == wndPtr->hwndSelf )
                    winpos->flags |= SWP_NOZORDER;

    /* Common operations */

      /* Send WM_NCCALCSIZE message to get new client area */
    if( (winpos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
        NCCALCSIZE_PARAMS params;
        WINDOWPOS winposCopy;

        params.rgrc[0] = newWindowRect;
        params.rgrc[1] = wndPtr->rectWindow;
        params.rgrc[2] = wndPtr->rectClient;
        params.lppos = &winposCopy;
        winposCopy = *winpos;

        wvrFlags = SendMessageW( winpos->hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params );

        TRACE( "%ld,%ld-%ld,%ld\n", params.rgrc[0].left, params.rgrc[0].top,
               params.rgrc[0].right, params.rgrc[0].bottom );

        /* If the application send back garbage, ignore it */
        if (params.rgrc[0].left <= params.rgrc[0].right &&
            params.rgrc[0].top <= params.rgrc[0].bottom)
            newClientRect = params.rgrc[0];

         /* FIXME: WVR_ALIGNxxx */

        if( newClientRect.left != wndPtr->rectClient.left ||
            newClientRect.top != wndPtr->rectClient.top )
            winpos->flags &= ~SWP_NOCLIENTMOVE;

        if( (newClientRect.right - newClientRect.left !=
             wndPtr->rectClient.right - wndPtr->rectClient.left) ||
            (newClientRect.bottom - newClientRect.top !=
             wndPtr->rectClient.bottom - wndPtr->rectClient.top) )
            winpos->flags &= ~SWP_NOCLIENTSIZE;
    }

    /* FIXME: actually do something with WVR_VALIDRECTS */

    set_window_pos( winpos->hwnd, winpos->hwndInsertAfter,
                    &newWindowRect, &newClientRect, winpos->flags );

    if( winpos->flags & SWP_SHOWWINDOW )
        WIN_SetStyle( winpos->hwnd, WS_VISIBLE, 0 );
    else if( winpos->flags & SWP_HIDEWINDOW )
        WIN_SetStyle( winpos->hwnd, 0, WS_VISIBLE );

    /* ------------------------------------------------------------------------ FINAL */

    /* repaint invalidated region (if any)
     *
     * FIXME: if SWP_NOACTIVATE is not set then set invalid regions here without any painting
     *        and force update after ChangeActiveWindow() to avoid painting frames twice.
     */

    if( !(winpos->flags & SWP_NOREDRAW) )
    {
        RedrawWindow( wndPtr->parent, NULL, 0,
                      RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN );
        if (wndPtr->parent == GetDesktopWindow())
            RedrawWindow( wndPtr->parent, NULL, 0,
                          RDW_ERASENOW | RDW_NOCHILDREN );
    }

    if (!(winpos->flags & SWP_NOACTIVATE)) SetActiveWindow( winpos->hwnd );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    TRACE("\tstatus flags = %04x\n", winpos->flags & SWP_AGG_STATUSFLAGS);

    if ((((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE) &&
          !(winpos->flags & SWP_NOSENDCHANGING)) )
        SendMessageA( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)winpos );

    retvalue = TRUE;
 END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *              WINPOS_MinMaximize   (internal)
 *
 *Lifted from x11 driver
 */
static UINT WINPOS_MinMaximize( HWND hwnd, UINT cmd, LPRECT rect )
{
    UINT swpFlags = 0;
    WINDOWPLACEMENT wpl;

    TRACE("%p %u\n", hwnd, cmd );
    FIXME("(%p): stub\n", hwnd);

    wpl.length = sizeof(wpl);
    GetWindowPlacement( hwnd, &wpl );

    /* If I glark this right, yields an immutable window*/
    swpFlags = SWP_NOSIZE | SWP_NOMOVE;

    /*cmd handling goes here.  see dlls/x1drv/winpos.c*/

    return swpFlags;
}

/***********************************************************************
 *              ShowWindow   (TTYDRV.@)
 *
 *Lifted from x11 driver
 *Sets the specified windows' show state.
 */
BOOL TTYDRV_ShowWindow( HWND hwnd, INT cmd )
{
    WND* 	wndPtr = WIN_FindWndPtr( hwnd );
    BOOL 	wasVisible, showFlag;
    RECT 	newPos = {0, 0, 0, 0};
    UINT 	swp = 0;

    if (!wndPtr) return FALSE;
    hwnd = wndPtr->hwndSelf;  /* make it a full handle */

    TRACE("hwnd=%p, cmd=%d\n", hwnd, cmd);

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) goto END;
	    swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE |
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swp |= SWP_SHOWWINDOW;
            /* fall through */
	case SW_MINIMIZE:
            swp |= SWP_FRAMECHANGED;
            if( !(wndPtr->dwStyle & WS_MINIMIZE) )
		 swp |= WINPOS_MinMaximize( hwnd, SW_MINIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
            swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if( !(wndPtr->dwStyle & WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( hwnd, SW_MAXIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
            break;

	case SW_SHOWNA:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOW:
	    swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;

	    /*
	     * ShowWindow has a little peculiar behavior that if the
	     * window is already the topmost window, it will not
	     * activate it.
	     */
	    if (GetTopWindow(NULL)==hwnd && (wasVisible || GetActiveWindow() == hwnd))
	      swp |= SWP_NOACTIVATE;

	    break;

	case SW_SHOWNOACTIVATE:
            swp |= SWP_NOZORDER;
            if (GetActiveWindow()) swp |= SWP_NOACTIVATE;
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
	case SW_RESTORE:
	    swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;

            if( wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( hwnd, SW_RESTORE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    showFlag = (cmd != SW_HIDE);
    if (showFlag != wasVisible)
    {
        SendMessageA( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) goto END;
    }

    /* We can't activate a child window */
    if ((wndPtr->dwStyle & WS_CHILD) &&
        !(wndPtr->dwExStyle & WS_EX_MDICHILD))
        swp |= SWP_NOACTIVATE | SWP_NOZORDER;

    SetWindowPos( hwnd, HWND_TOP, newPos.left, newPos.top,
                  newPos.right, newPos.bottom, LOWORD(swp) );
    if (cmd == SW_HIDE)
    {
        /* FIXME: This will cause the window to be activated irrespective
         * of whether it is owned by the same thread. Has to be done
         * asynchronously.
         */

        if (hwnd == GetActiveWindow())
            WINPOS_ActivateOtherWindow(hwnd);

        /* Revert focus to parent */
        if (hwnd == GetFocus() || IsChild(hwnd, GetFocus()))
            SetFocus( GetParent(hwnd) );
    }
    if (!IsWindow( hwnd )) goto END;
    else if( wndPtr->dwStyle & WS_MINIMIZE ) WINPOS_ShowIconTitle( hwnd, TRUE );

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
	SendMessageA( hwnd, WM_SIZE, wParam,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	SendMessageA( hwnd, WM_MOVE, 0,
		   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    }

END:
    WIN_ReleaseWndPtr(wndPtr);
    return wasVisible;
}

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
static HRGN get_server_visible_region( HWND hwnd, UINT flags )
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
static BOOL set_window_pos( HWND hwnd, HWND insert_after, const RECT *rectWindow,
                            const RECT *rectClient, UINT swp_flags )
{
    WND *win = WIN_GetPtr( hwnd );
    BOOL ret;

    if (!win) return FALSE;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) ERR( "cannot set rectangles of other process window %p\n", hwnd );
        return FALSE;
    }
    SERVER_START_REQ( set_window_pos )
    {
        req->handle        = hwnd;
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
    if (win == WND_DESKTOP) return ret;
    if (ret)
    {
        win->rectWindow = *rectWindow;
        win->rectClient = *rectClient;

        TRACE( "win %p window (%ld,%ld)-(%ld,%ld) client (%ld,%ld)-(%ld,%ld)\n", hwnd,
               rectWindow->left, rectWindow->top, rectWindow->right, rectWindow->bottom,
               rectClient->left, rectClient->top, rectClient->right, rectClient->bottom );
    }
    WIN_ReleasePtr( win );
    return ret;
}


/**********************************************************************
 *		CreateDesktopWindow   (TTYDRV.@)
 */
BOOL TTYDRV_CreateDesktopWindow( HWND hwnd )
{
    RECT rect;

    SetRect( &rect, 0, 0, cell_width * screen_cols, cell_height * screen_rows );
    set_window_pos( hwnd, 0, &rect, &rect, SWP_NOZORDER );
    SetPropA( hwnd, "__wine_ttydrv_window", root_window );
    return TRUE;
}

/**********************************************************************
 *		CreateWindow   (TTYDRV.@)
 */
BOOL TTYDRV_CreateWindow( HWND hwnd, CREATESTRUCTA *cs, BOOL unicode )
{
    BOOL ret;
    RECT rect;
    HWND hwndLinkAfter;
    CBT_CREATEWNDA cbtc;

    TRACE("(%p)\n", hwnd);

    /* initialize the dimensions before sending WM_GETMINMAXINFO */
    SetRect( &rect, cs->x, cs->y, cs->x + cs->cx, cs->y + cs->cy );
    set_window_pos( hwnd, 0, &rect, &rect, SWP_NOZORDER );

#ifdef WINE_CURSES
    /* Only create top-level windows */
    if (GetAncestor( hwnd, GA_PARENT ) == GetDesktopWindow())
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
        HRGN visRgn = get_server_visible_region( hwnd, flags );

        if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
            CombineRgn( visRgn, visRgn, hrgn, (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

        SelectVisRgn16( HDC_16(hdc), HRGN_16(visRgn) );
        DeleteObject( visRgn );
    }
    return TRUE;
}


/* fix redundant flags and values in the WINDOWPOS structure */
static BOOL fixup_flags( WINDOWPOS *winpos )
{
    HWND parent;
    WND *wndPtr = WIN_GetPtr( winpos->hwnd );
    BOOL ret = TRUE;

    if (!wndPtr || wndPtr == WND_OTHER_PROCESS)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    winpos->hwnd = wndPtr->hwndSelf;  /* make it a full handle */

    /* Finally make sure that all coordinates are valid */
    if (winpos->x < -32768) winpos->x = -32768;
    else if (winpos->x > 32767) winpos->x = 32767;
    if (winpos->y < -32768) winpos->y = -32768;
    else if (winpos->y > 32767) winpos->y = 32767;

    if (winpos->cx < 0) winpos->cx = 0;
    else if (winpos->cx > 32767) winpos->cx = 32767;
    if (winpos->cy < 0) winpos->cy = 0;
    else if (winpos->cy > 32767) winpos->cy = 32767;

    parent = GetAncestor( winpos->hwnd, GA_PARENT );
    if (!IsWindowVisible( parent )) winpos->flags |= SWP_NOREDRAW;

    if (wndPtr->dwStyle & WS_VISIBLE) winpos->flags &= ~SWP_SHOWWINDOW;
    else
    {
        winpos->flags &= ~SWP_HIDEWINDOW;
        if (!(winpos->flags & SWP_SHOWWINDOW)) winpos->flags |= SWP_NOREDRAW;
    }

    if ((wndPtr->rectWindow.right - wndPtr->rectWindow.left == winpos->cx) &&
        (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top == winpos->cy))
        winpos->flags |= SWP_NOSIZE;    /* Already the right size */

    if ((wndPtr->rectWindow.left == winpos->x) && (wndPtr->rectWindow.top == winpos->y))
        winpos->flags |= SWP_NOMOVE;    /* Already the right position */

    if ((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)
    {
        if (!(winpos->flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW))) /* Bring to the top when activating */
        {
            winpos->flags &= ~SWP_NOZORDER;
            winpos->hwndInsertAfter = HWND_TOP;
        }
    }

    /* Check hwndInsertAfter */
    if (winpos->flags & SWP_NOZORDER) goto done;

    /* fix sign extension */
    if (winpos->hwndInsertAfter == (HWND)0xffff) winpos->hwndInsertAfter = HWND_TOPMOST;
    else if (winpos->hwndInsertAfter == (HWND)0xfffe) winpos->hwndInsertAfter = HWND_NOTOPMOST;

      /* FIXME: TOPMOST not supported yet */
    if ((winpos->hwndInsertAfter == HWND_TOPMOST) ||
        (winpos->hwndInsertAfter == HWND_NOTOPMOST)) winpos->hwndInsertAfter = HWND_TOP;

    /* hwndInsertAfter must be a sibling of the window */
    if (winpos->hwndInsertAfter == HWND_TOP)
    {
        if (GetWindow(winpos->hwnd, GW_HWNDFIRST) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_BOTTOM)
    {
        if (GetWindow(winpos->hwnd, GW_HWNDLAST) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else
    {
        if (GetAncestor( winpos->hwndInsertAfter, GA_PARENT ) != parent) ret = FALSE;
        else
        {
            /* don't need to change the Zorder of hwnd if it's already inserted
             * after hwndInsertAfter or when inserting hwnd after itself.
             */
            if ((winpos->hwnd == winpos->hwndInsertAfter) ||
                (winpos->hwnd == GetWindow( winpos->hwndInsertAfter, GW_HWNDNEXT )))
                winpos->flags |= SWP_NOZORDER;
        }
    }
 done:
    WIN_ReleasePtr( wndPtr );
    return ret;
}


/***********************************************************************
 *           SWP_DoNCCalcSize
 */
static UINT SWP_DoNCCalcSize( WINDOWPOS* pWinpos, const RECT* pNewWindowRect, RECT* pNewClientRect )
{
    UINT wvrFlags = 0;
    WND *wndPtr;

    if (!(wndPtr = WIN_GetPtr( pWinpos->hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;

      /* Send WM_NCCALCSIZE message to get new client area */
    if( (pWinpos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
        NCCALCSIZE_PARAMS params;
        WINDOWPOS winposCopy;

        params.rgrc[0] = *pNewWindowRect;
        params.rgrc[1] = wndPtr->rectWindow;
        params.rgrc[2] = wndPtr->rectClient;
        params.lppos = &winposCopy;
        winposCopy = *pWinpos;
        WIN_ReleasePtr( wndPtr );

        wvrFlags = SendMessageW( pWinpos->hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params );

        *pNewClientRect = params.rgrc[0];

        if (!(wndPtr = WIN_GetPtr( pWinpos->hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;

        TRACE( "hwnd %p old win %s old client %s new win %s new client %s\n", pWinpos->hwnd,
               wine_dbgstr_rect(&wndPtr->rectWindow), wine_dbgstr_rect(&wndPtr->rectClient),
               wine_dbgstr_rect(pNewWindowRect), wine_dbgstr_rect(pNewClientRect) );

        if( pNewClientRect->left != wndPtr->rectClient.left ||
            pNewClientRect->top != wndPtr->rectClient.top )
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;

        if( (pNewClientRect->right - pNewClientRect->left !=
             wndPtr->rectClient.right - wndPtr->rectClient.left))
            pWinpos->flags &= ~SWP_NOCLIENTSIZE;
        else
            wvrFlags &= ~WVR_HREDRAW;

        if (pNewClientRect->bottom - pNewClientRect->top !=
             wndPtr->rectClient.bottom - wndPtr->rectClient.top)
            pWinpos->flags &= ~SWP_NOCLIENTSIZE;
        else
            wvrFlags &= ~WVR_VREDRAW;
    }
    else
    {
        if (!(pWinpos->flags & SWP_NOMOVE) &&
            (pNewClientRect->left != wndPtr->rectClient.left ||
             pNewClientRect->top != wndPtr->rectClient.top))
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;
    }

    WIN_ReleasePtr( wndPtr );
    return wvrFlags;
}


struct move_owned_info
{
    HWND owner;
    HWND insert_after;
};

static BOOL CALLBACK move_owned_popups( HWND hwnd, LPARAM lparam )
{
    struct move_owned_info *info = (struct move_owned_info *)lparam;

    if (hwnd == info->owner) return FALSE;
    if ((GetWindowLongW( hwnd, GWL_STYLE ) & WS_POPUP) &&
        GetWindow( hwnd, GW_OWNER ) == info->owner)
    {
        SetWindowPos( hwnd, info->insert_after, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                      SWP_NOSENDCHANGING | SWP_DEFERERASE );
        info->insert_after = hwnd;
    }
    return TRUE;
}

/***********************************************************************
 *           SWP_DoOwnedPopups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 *
 * FIXME: hide/show owned popups when owner visibility changes.
 */
static HWND SWP_DoOwnedPopups(HWND hwnd, HWND hwndInsertAfter)
{
    HWND owner = GetWindow( hwnd, GW_OWNER );
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    struct move_owned_info info;

    TRACE("(%p) hInsertAfter = %p\n", hwnd, hwndInsertAfter );

    if ((style & WS_POPUP) && owner)
    {
        /* make sure this popup stays above the owner */

        if( hwndInsertAfter != HWND_TOP )
        {
            HWND hwndLocalPrev = HWND_TOP;
            HWND prev = GetWindow( owner, GW_HWNDPREV );

            while (prev && prev != hwndInsertAfter)
            {
                if (hwndLocalPrev == HWND_TOP && GetWindowLongW( prev, GWL_STYLE ) & WS_VISIBLE)
                    hwndLocalPrev = prev;
                prev = GetWindow( prev, GW_HWNDPREV );
            }
            if (!prev) hwndInsertAfter = hwndLocalPrev;
        }
    }
    else if (style & WS_CHILD) return hwndInsertAfter;

    info.owner = hwnd;
    info.insert_after = hwndInsertAfter;
    EnumWindows( move_owned_popups, (LPARAM)&info );
    return info.insert_after;
}


/***********************************************************************
 *           SWP_DoWinPosChanging
 */
static BOOL SWP_DoWinPosChanging( WINDOWPOS* pWinpos, RECT* pNewWindowRect, RECT* pNewClientRect )
{
    WND *wndPtr;

    /* Send WM_WINDOWPOSCHANGING message */

    if (!(pWinpos->flags & SWP_NOSENDCHANGING))
        SendMessageW( pWinpos->hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)pWinpos );

    if (!(wndPtr = WIN_GetPtr( pWinpos->hwnd )) || wndPtr == WND_OTHER_PROCESS) return FALSE;

    /* Calculate new position and size */

    *pNewWindowRect = wndPtr->rectWindow;
    *pNewClientRect = (wndPtr->dwStyle & WS_MINIMIZE) ? wndPtr->rectWindow
                                                    : wndPtr->rectClient;

    if (!(pWinpos->flags & SWP_NOSIZE))
    {
        pNewWindowRect->right  = pNewWindowRect->left + pWinpos->cx;
        pNewWindowRect->bottom = pNewWindowRect->top + pWinpos->cy;
    }
    if (!(pWinpos->flags & SWP_NOMOVE))
    {
        pNewWindowRect->left    = pWinpos->x;
        pNewWindowRect->top     = pWinpos->y;
        pNewWindowRect->right  += pWinpos->x - wndPtr->rectWindow.left;
        pNewWindowRect->bottom += pWinpos->y - wndPtr->rectWindow.top;

        OffsetRect( pNewClientRect, pWinpos->x - wndPtr->rectWindow.left,
                                    pWinpos->y - wndPtr->rectWindow.top );
    }
    pWinpos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

    TRACE( "hwnd %p, after %p, swp %d,%d %dx%d flags %08x\n",
           pWinpos->hwnd, pWinpos->hwndInsertAfter, pWinpos->x, pWinpos->y,
           pWinpos->cx, pWinpos->cy, pWinpos->flags );
    TRACE( "current %s style %08lx new %s\n",
           wine_dbgstr_rect( &wndPtr->rectWindow ), wndPtr->dwStyle,
           wine_dbgstr_rect( pNewWindowRect ));

    WIN_ReleasePtr( wndPtr );
    return TRUE;
}


/***********************************************************************
 *		SetWindowPos   (TTYDRV.@)
 */
BOOL TTYDRV_SetWindowPos( WINDOWPOS *winpos )
{
    RECT newWindowRect, newClientRect;
    UINT orig_flags;

    TRACE( "hwnd %p, after %p, swp %d,%d %dx%d flags %08x\n",
           winpos->hwnd, winpos->hwndInsertAfter, winpos->x, winpos->y,
           winpos->cx, winpos->cy, winpos->flags);

    orig_flags = winpos->flags;
    winpos->flags &= ~SWP_WINE_NOHOSTMOVE;

    /* Check window handle */
    if (winpos->hwnd == GetDesktopWindow()) return FALSE;

    /* First make sure that coordinates are valid for WM_WINDOWPOSCHANGING */
    if (!(winpos->flags & SWP_NOMOVE))
    {
        if (winpos->x < -32768) winpos->x = -32768;
        else if (winpos->x > 32767) winpos->x = 32767;
        if (winpos->y < -32768) winpos->y = -32768;
        else if (winpos->y > 32767) winpos->y = 32767;
    }
    if (!(winpos->flags & SWP_NOSIZE))
    {
        if (winpos->cx < 0) winpos->cx = 0;
        else if (winpos->cx > 32767) winpos->cx = 32767;
        if (winpos->cy < 0) winpos->cy = 0;
        else if (winpos->cy > 32767) winpos->cy = 32767;
    }

    if (!SWP_DoWinPosChanging( winpos, &newWindowRect, &newClientRect )) return FALSE;

    /* Fix redundant flags */
    if (!fixup_flags( winpos )) return FALSE;

    if((winpos->flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) != SWP_NOZORDER)
    {
        if (GetAncestor( winpos->hwnd, GA_PARENT ) == GetDesktopWindow())
            winpos->hwndInsertAfter = SWP_DoOwnedPopups( winpos->hwnd, winpos->hwndInsertAfter );
    }

    /* Common operations */

    SWP_DoNCCalcSize( winpos, &newWindowRect, &newClientRect );

    if (!set_window_pos( winpos->hwnd, winpos->hwndInsertAfter,
                         &newWindowRect, &newClientRect, orig_flags ))
        return FALSE;

    if( winpos->flags & SWP_HIDEWINDOW )
        HideCaret(winpos->hwnd);
    else if (winpos->flags & SWP_SHOWWINDOW)
        ShowCaret(winpos->hwnd);

    if (!(winpos->flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW)))
    {
        /* child windows get WM_CHILDACTIVATE message */
        if ((GetWindowLongW( winpos->hwnd, GWL_STYLE ) & (WS_CHILD | WS_POPUP)) == WS_CHILD)
            SendMessageA( winpos->hwnd, WM_CHILDACTIVATE, 0, 0 );
        else
            SetForegroundWindow( winpos->hwnd );
    }

      /* And last, send the WM_WINDOWPOSCHANGED message */

    TRACE("\tstatus flags = %04x\n", winpos->flags & SWP_AGG_STATUSFLAGS);

    if (((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE))
    {
        /* WM_WINDOWPOSCHANGED is sent even if SWP_NOSENDCHANGING is set
           and always contains final window position.
         */
        winpos->x = newWindowRect.left;
        winpos->y = newWindowRect.top;
        winpos->cx = newWindowRect.right - newWindowRect.left;
        winpos->cy = newWindowRect.bottom - newWindowRect.top;
        SendMessageW( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)winpos );
    }

    return TRUE;
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
    WND *wndPtr;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL wasVisible = (style & WS_VISIBLE) != 0;
    BOOL showFlag = TRUE;
    RECT newPos = {0, 0, 0, 0};
    UINT swp = 0;


    TRACE("hwnd=%p, cmd=%d, wasVisible %d\n", hwnd, cmd, wasVisible);

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) return FALSE;
            showFlag = FALSE;
            swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
        case SW_FORCEMINIMIZE: /* FIXME: Does not work if thread is hung. */
            swp |= SWP_SHOWWINDOW;
            /* fall through */
	case SW_MINIMIZE:
            swp |= SWP_FRAMECHANGED;
            if( !(style & WS_MINIMIZE) )
		 swp |= WINPOS_MinMaximize( hwnd, SW_MINIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
            swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if( !(style & WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( hwnd, SW_MAXIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
            break;

	case SW_SHOWNA:
            swp |= SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (style & WS_CHILD) swp |= SWP_NOZORDER;
            break;
	case SW_SHOW:
            if (wasVisible) return TRUE;
	    swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_RESTORE:
	    swp |= SWP_FRAMECHANGED;
            /* fall through */
	case SW_SHOWNOACTIVATE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
	    swp |= SWP_SHOWWINDOW;

            if (style & (WS_MINIMIZE | WS_MAXIMIZE))
		 swp |= WINPOS_MinMaximize( hwnd, SW_RESTORE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    if (showFlag != wasVisible || cmd == SW_SHOWNA)
    {
        SendMessageW( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) return wasVisible;
    }

    /* ShowWindow won't activate a not being maximized child window */
    if ((style & WS_CHILD) && cmd != SW_MAXIMIZE)
        swp |= SWP_NOACTIVATE | SWP_NOZORDER;

    SetWindowPos( hwnd, HWND_TOP, newPos.left, newPos.top,
                  newPos.right, newPos.bottom, LOWORD(swp) );
    if (cmd == SW_HIDE)
    {
        HWND hFocus;

        /* FIXME: This will cause the window to be activated irrespective
         * of whether it is owned by the same thread. Has to be done
         * asynchronously.
         */

        if (hwnd == GetActiveWindow())
            WINPOS_ActivateOtherWindow(hwnd);

        /* Revert focus to parent */
        hFocus = GetFocus();
        if (hwnd == hFocus || IsChild(hwnd, hFocus))
        {
            HWND parent = GetAncestor(hwnd, GA_PARENT);
            if (parent == GetDesktopWindow()) parent = 0;
            SetFocus(parent);
        }
    }

    if (IsIconic(hwnd)) WINPOS_ShowIconTitle( hwnd, TRUE );

    if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return wasVisible;

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;
        RECT client = wndPtr->rectClient;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
        WIN_ReleasePtr( wndPtr );

        SendMessageW( hwnd, WM_SIZE, wParam,
                      MAKELONG( client.right - client.left, client.bottom - client.top ));
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG( client.left, client.top ));
    }
    else WIN_ReleasePtr( wndPtr );

    return wasVisible;
}

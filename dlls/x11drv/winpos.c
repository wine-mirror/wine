/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995, 2001 Alexandre Julliard
 * Copyright 1995, 1996, 1999 Alex Korobka
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

#include <X11/Xlib.h>
#ifdef HAVE_LIBXSHAPE
#include <X11/IntrinsicP.h>
#include <X11/extensions/shape.h>
#endif /* HAVE_LIBXSHAPE */
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "ntstatus.h"
#include "wownt32.h"
#include "wine/wingdi16.h"

#include "x11drv.h"
#include "win.h"
#include "winpos.h"
#include "dce.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

#define SWP_AGG_NOPOSCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

#define SWP_EX_NOCOPY       0x0001
#define SWP_EX_PAINTSELF    0x0002
#define SWP_EX_NONCLIENT    0x0004

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9   /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10   /* move via keyboard */


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
 *           get_top_clipping_window
 *
 * Get the top window to clip against (i.e. the top parent that has
 * an associated X window).
 */
static HWND get_top_clipping_window( HWND hwnd )
{
    HWND ret = GetAncestor( hwnd, GA_ROOT );
    if (!ret) ret = GetDesktopWindow();
    return ret;
}


/***********************************************************************
 *           X11DRV_Expose
 */
void X11DRV_Expose( HWND hwnd, XExposeEvent *event )
{
    RECT rect;
    struct x11drv_win_data *data;
    int flags = RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN;

    TRACE( "win %p (%lx) %d,%d %dx%d\n",
           hwnd, event->window, event->x, event->y, event->width, event->height );

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    rect.left   = event->x;
    rect.top    = event->y;
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    if (rect.left < data->client_rect.left ||
        rect.top < data->client_rect.top ||
        rect.right > data->client_rect.right ||
        rect.bottom > data->client_rect.bottom) flags |= RDW_FRAME;

    /* make position relative to client area instead of window */
    OffsetRect( &rect, -data->client_rect.left, -data->client_rect.top );

    RedrawWindow( hwnd, &rect, 0, flags );
}


/***********************************************************************
 *		GetDC (X11DRV.@)
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
BOOL X11DRV_GetDC( HWND hwnd, HDC hdc, HRGN hrgn, DWORD flags )
{
    HWND top = get_top_clipping_window( hwnd );
    struct x11drv_escape_set_drawable escape;
    struct x11drv_win_data *data;

    escape.mode = IncludeInferiors;
    /* don't clip siblings if using parent clip region */
    if (flags & DCX_PARENTCLIP) flags &= ~DCX_CLIPSIBLINGS;

    if (top != hwnd || !(data = X11DRV_get_win_data( hwnd )))
    {
        POINT client_offset;

        if (flags & DCX_WINDOW)
        {
            RECT rect;
            GetWindowRect( hwnd, &rect );
            escape.org.x = rect.left;
            escape.org.y = rect.top;
            MapWindowPoints( 0, top, &escape.org, 1 );
            escape.drawable_org.x = rect.left - escape.org.x;
            escape.drawable_org.y = rect.top - escape.org.y;
        }
        else
        {
            escape.org.x = escape.org.y = 0;
            escape.drawable_org.x = escape.drawable_org.y = 0;
            MapWindowPoints( hwnd, top, &escape.org, 1 );
            MapWindowPoints( top, 0, &escape.drawable_org, 1 );
        }

        /* now make origins relative to the X window and not the client area */
        client_offset = X11DRV_get_client_area_offset( top );
        escape.org.x += client_offset.x;
        escape.org.y += client_offset.y;
        escape.drawable_org.x -= client_offset.x;
        escape.drawable_org.y -= client_offset.y;
        escape.drawable = X11DRV_get_whole_window( top );
    }
    else
    {
        if (IsIconic( hwnd ))
        {
            escape.drawable = data->icon_window ? data->icon_window : data->whole_window;
            escape.org.x = 0;
            escape.org.y = 0;
            escape.drawable_org = escape.org;
            MapWindowPoints( hwnd, 0, &escape.drawable_org, 1 );
        }
        else
        {
            escape.drawable = data->whole_window;
            escape.drawable_org.x = data->whole_rect.left;
            escape.drawable_org.y = data->whole_rect.top;
            if (flags & DCX_WINDOW)
            {
                escape.org.x = data->window_rect.left - data->whole_rect.left;
                escape.org.y = data->window_rect.top - data->whole_rect.top;
            }
            else
            {
                escape.org.x = data->client_rect.left;
                escape.org.y = data->client_rect.top;
            }
        }
    }

    escape.code = X11DRV_SET_DRAWABLE;
    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) ||
        SetHookFlags16( HDC_16(hdc), DCHF_VALIDATEVISRGN ))  /* DC was dirty */
    {
        /* need to recompute the visible region */
        HRGN visRgn = get_server_visible_region( hwnd, top, flags );

        if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
            CombineRgn( visRgn, visRgn, hrgn, (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

        SelectVisRgn16( HDC_16(hdc), HRGN_16(visRgn) );
        DeleteObject( visRgn );
    }
    return TRUE;
}


/***********************************************************************
 *		ReleaseDC (X11DRV.@)
 */
void X11DRV_ReleaseDC( HWND hwnd, HDC hdc )
{
    struct x11drv_escape_set_drawable escape;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = root_window;
    escape.mode = IncludeInferiors;
    escape.org.x = escape.org.y = 0;
    escape.drawable_org.x = escape.drawable_org.y = 0;

    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
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
 *           get_valid_rects
 *
 * Compute the valid rects from the old and new client rect and WVR_* flags.
 * Helper for WM_NCCALCSIZE handling.
 */
static inline void get_valid_rects( const RECT *old_client, const RECT *new_client, UINT flags,
                                    RECT *valid )
{
    int cx, cy;

    if (flags & WVR_REDRAW)
    {
        SetRectEmpty( &valid[0] );
        SetRectEmpty( &valid[1] );
        return;
    }

    if (flags & WVR_VALIDRECTS)
    {
        if (!IntersectRect( &valid[0], &valid[0], new_client ) ||
            !IntersectRect( &valid[1], &valid[1], old_client ))
        {
            SetRectEmpty( &valid[0] );
            SetRectEmpty( &valid[1] );
            return;
        }
        flags = WVR_ALIGNLEFT | WVR_ALIGNTOP;
    }
    else
    {
        valid[0] = *new_client;
        valid[1] = *old_client;
    }

    /* make sure the rectangles have the same size */
    cx = min( valid[0].right - valid[0].left, valid[1].right - valid[1].left );
    cy = min( valid[0].bottom - valid[0].top, valid[1].bottom - valid[1].top );

    if (flags & WVR_ALIGNBOTTOM)
    {
        valid[0].top = valid[0].bottom - cy;
        valid[1].top = valid[1].bottom - cy;
    }
    else
    {
        valid[0].bottom = valid[0].top + cy;
        valid[1].bottom = valid[1].top + cy;
    }
    if (flags & WVR_ALIGNRIGHT)
    {
        valid[0].left = valid[0].right - cx;
        valid[1].left = valid[1].right - cx;
    }
    else
    {
        valid[0].right = valid[0].left + cx;
        valid[1].right = valid[1].left + cx;
    }
}


/***********************************************************************
 *           SWP_DoNCCalcSize
 */
static UINT SWP_DoNCCalcSize( WINDOWPOS* pWinpos, const RECT* pNewWindowRect, RECT* pNewClientRect,
                              RECT *validRects )
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

        TRACE( "(%ld,%ld)-(%ld,%ld)\n", params.rgrc[0].left, params.rgrc[0].top,
               params.rgrc[0].right, params.rgrc[0].bottom );

        /* If the application sends back garbage, ignore it */

        if (params.rgrc[0].left < pNewWindowRect->left) params.rgrc[0].left = pNewWindowRect->left;
        if (params.rgrc[0].top < pNewWindowRect->top) params.rgrc[0].top = pNewWindowRect->top;
        if (params.rgrc[0].right > pNewWindowRect->right) params.rgrc[0].right = pNewWindowRect->right;
        if (params.rgrc[0].bottom > pNewWindowRect->bottom) params.rgrc[0].bottom = pNewWindowRect->bottom;

        if (params.rgrc[0].left <= params.rgrc[0].right &&
            params.rgrc[0].top <= params.rgrc[0].bottom)
            *pNewClientRect = params.rgrc[0];

        if (!(wndPtr = WIN_GetPtr( pWinpos->hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;

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

        validRects[0] = params.rgrc[1];
        validRects[1] = params.rgrc[2];
    }
    else
    {
        if (!(pWinpos->flags & SWP_NOMOVE) &&
            (pNewClientRect->left != wndPtr->rectClient.left ||
             pNewClientRect->top != wndPtr->rectClient.top))
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;
    }

    if (pWinpos->flags & (SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_SHOWWINDOW | SWP_HIDEWINDOW))
    {
        SetRectEmpty( &validRects[0] );
        SetRectEmpty( &validRects[1] );
    }
    else get_valid_rects( &wndPtr->rectClient, pNewClientRect, wvrFlags, validRects );

    WIN_ReleasePtr( wndPtr );
    return wvrFlags;
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
    HWND *list = NULL;
    HWND owner = GetWindow( hwnd, GW_OWNER );
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );

    WARN("(%p) hInsertAfter = %p\n", hwnd, hwndInsertAfter );

    if ((style & WS_POPUP) && owner)
    {
        /* make sure this popup stays above the owner */

        HWND hwndLocalPrev = HWND_TOP;

        if( hwndInsertAfter != HWND_TOP )
        {
            if ((list = WIN_ListChildren( GetDesktopWindow() )))
            {
                int i;
                for (i = 0; list[i]; i++)
                {
                    if (list[i] == owner) break;
                    if (list[i] != hwnd) hwndLocalPrev = list[i];
                    if (hwndLocalPrev == hwndInsertAfter) break;
                }
                hwndInsertAfter = hwndLocalPrev;
            }
        }
    }
    else if (style & WS_CHILD) return hwndInsertAfter;

    if (!list) list = WIN_ListChildren( GetDesktopWindow() );
    if (list)
    {
        int i;
        for (i = 0; list[i]; i++)
        {
            if (list[i] == hwnd) break;
            if ((GetWindowLongW( list[i], GWL_STYLE ) & WS_POPUP) &&
                GetWindow( list[i], GW_OWNER ) == hwnd)
            {
                SetWindowPos( list[i], hwndInsertAfter, 0, 0, 0, 0,
                              SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                              SWP_NOSENDCHANGING | SWP_DEFERERASE );
                hwndInsertAfter = list[i];
            }
        }
        HeapFree( GetProcessHeap(), 0, list );
    }

    return hwndInsertAfter;
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
 *		SetWindowStyle   (X11DRV.@)
 *
 * Update the X state of a window to reflect a style change
 */
void X11DRV_SetWindowStyle( HWND hwnd, DWORD old_style )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    DWORD new_style, changed;

    if (hwnd == GetDesktopWindow()) return;
    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    new_style = GetWindowLongW( hwnd, GWL_STYLE );
    changed = new_style ^ old_style;

    if (changed & WS_VISIBLE)
    {
        if (data->whole_window && X11DRV_is_window_rect_mapped( &data->window_rect ))
        {
            if (new_style & WS_VISIBLE)
            {
                TRACE( "mapping win %p\n", hwnd );
                X11DRV_sync_window_style( display, data );
                X11DRV_set_wm_hints( display, data );
                wine_tsx11_lock();
                XMapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
            /* we don't unmap windows, that causes trouble with the window manager */
        }
        DCE_InvalidateDCE( hwnd, &data->window_rect );
    }

    if (changed & WS_DISABLED)
    {
        if (data->whole_window && data->managed)
        {
            XWMHints *wm_hints;
            wine_tsx11_lock();
            if (!(wm_hints = XGetWMHints( display, data->whole_window )))
                wm_hints = XAllocWMHints();
            if (wm_hints)
            {
                wm_hints->flags |= InputHint;
                wm_hints->input = !(new_style & WS_DISABLED);
                XSetWMHints( display, data->whole_window, wm_hints );
                XFree(wm_hints);
            }
            wine_tsx11_unlock();
        }
    }
}


/***********************************************************************
 *           X11DRV_set_window_pos
 *
 * Set a window position and Z order.
 */
BOOL X11DRV_set_window_pos( HWND hwnd, HWND insert_after, const RECT *rectWindow,
                            const RECT *rectClient, UINT swp_flags, const RECT *valid_rects )
{
    struct x11drv_win_data *data;
    HWND top = get_top_clipping_window( hwnd );
    RECT new_whole_rect;
    WND *win;
    DWORD old_style, new_style;
    BOOL ret;

    if (!(data = X11DRV_get_win_data( hwnd ))) return FALSE;

    new_whole_rect = *rectWindow;
    X11DRV_window_to_X_rect( data, &new_whole_rect );

    if (!IsRectEmpty( &valid_rects[0] ))
    {
        int x_offset = 0, y_offset = 0;

        if (data->whole_window)
        {
            /* the X server will move the bits for us */
            x_offset = data->whole_rect.left - new_whole_rect.left;
            y_offset = data->whole_rect.top - new_whole_rect.top;
        }

        if (x_offset != valid_rects[1].left - valid_rects[0].left ||
            y_offset != valid_rects[1].top - valid_rects[0].top)
        {
            /* FIXME: should copy the window bits here */
            valid_rects = NULL;
        }
    }

    if (!(win = WIN_GetPtr( hwnd ))) return FALSE;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) ERR( "cannot set rectangles of other process window %p\n", hwnd );
        return FALSE;
    }
    old_style = win->dwStyle;
    SERVER_START_REQ( set_window_pos )
    {
        req->handle        = hwnd;
        req->top_win       = top;
        req->previous      = insert_after;
        req->flags         = swp_flags & ~SWP_WINE_NOHOSTMOVE;
        req->window.left   = rectWindow->left;
        req->window.top    = rectWindow->top;
        req->window.right  = rectWindow->right;
        req->window.bottom = rectWindow->bottom;
        req->client.left   = rectClient->left;
        req->client.top    = rectClient->top;
        req->client.right  = rectClient->right;
        req->client.bottom = rectClient->bottom;
        if (!IsRectEmpty( &valid_rects[0] ))
            wine_server_add_data( req, valid_rects, 2 * sizeof(*valid_rects) );
        ret = !wine_server_call( req );
        new_style = reply->new_style;
    }
    SERVER_END_REQ;

    if (ret)
    {
        Display *display = thread_display();

        /* invalidate DCEs */

        if ((((swp_flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) && (new_style & WS_VISIBLE)) ||
             (swp_flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)))
        {
            RECT rect;
            UnionRect( &rect, rectWindow, &win->rectWindow );
            DCE_InvalidateDCE( hwnd, &rect );
        }

        win->rectWindow   = *rectWindow;
        win->rectClient   = *rectClient;
        win->dwStyle      = new_style;
        data->window_rect = *rectWindow;

        TRACE( "win %p window %s client %s style %08lx\n",
               hwnd, wine_dbgstr_rect(rectWindow), wine_dbgstr_rect(rectClient), new_style );

        /* FIXME: copy the valid bits */

        if (data->whole_window)
        {
            if ((old_style & WS_VISIBLE) && !(new_style & WS_VISIBLE))
            {
                /* window got hidden, unmap it */
                TRACE( "unmapping win %p\n", hwnd );
                wine_tsx11_lock();
                XUnmapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
            else if ((new_style & WS_VISIBLE) && !X11DRV_is_window_rect_mapped( rectWindow ))
            {
                /* resizing to zero size or off screen -> unmap */
                TRACE( "unmapping zero size or off-screen win %p\n", hwnd );
                wine_tsx11_lock();
                XUnmapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
        }

        X11DRV_sync_window_position( display, data, swp_flags, rectClient, &new_whole_rect );

        if (data->whole_window)
        {
            if (!(old_style & WS_VISIBLE) && (new_style & WS_VISIBLE))
            {
                /* window got shown, map it */
                if (X11DRV_is_window_rect_mapped( rectWindow ))
                {
                    TRACE( "mapping win %p\n", hwnd );
                    X11DRV_sync_window_style( display, data );
                    X11DRV_set_wm_hints( display, data );
                    wine_tsx11_lock();
                    XMapWindow( display, data->whole_window );
                    wine_tsx11_unlock();
                }
            }
            else if ((new_style & WS_VISIBLE) && X11DRV_is_window_rect_mapped( rectWindow ))
            {
                /* resizing from zero size to non-zero -> map */
                TRACE( "mapping non zero size or off-screen win %p\n", hwnd );
                wine_tsx11_lock();
                XMapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
            wine_tsx11_lock();
            XFlush( display );  /* FIXME: should not be necessary */
            wine_tsx11_unlock();
        }
    }
    WIN_ReleasePtr( win );
    return ret;
}


/***********************************************************************
 *		SetWindowPos   (X11DRV.@)
 */
BOOL X11DRV_SetWindowPos( WINDOWPOS *winpos )
{
    RECT newWindowRect, newClientRect, valid_rects[2];
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

    SWP_DoNCCalcSize( winpos, &newWindowRect, &newClientRect, valid_rects );

    if (!X11DRV_set_window_pos( winpos->hwnd, winpos->hwndInsertAfter,
                                &newWindowRect, &newClientRect, orig_flags, valid_rects ))
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
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 */
static POINT WINPOS_FindIconPos( WND* wndPtr, POINT pt )
{
    RECT rectParent;
    HWND *list;
    short x, y, xspacing, yspacing;

    GetClientRect( wndPtr->parent, &rectParent );
    if ((pt.x >= rectParent.left) && (pt.x + GetSystemMetrics(SM_CXICON) < rectParent.right) &&
        (pt.y >= rectParent.top) && (pt.y + GetSystemMetrics(SM_CYICON) < rectParent.bottom))
        return pt;  /* The icon already has a suitable position */

    xspacing = GetSystemMetrics(SM_CXICONSPACING);
    yspacing = GetSystemMetrics(SM_CYICONSPACING);

    list = WIN_ListChildren( wndPtr->parent );
    y = rectParent.bottom;
    for (;;)
    {
        x = rectParent.left;
        do
        {
            /* Check if another icon already occupies this spot */
            /* FIXME: this is completely inefficient */
            if (list)
            {
                int i;
                WND *childPtr;

                for (i = 0; list[i]; i++)
                {
                    if (list[i] == wndPtr->hwndSelf) continue;
                    if (!IsIconic( list[i] )) continue;
                    if (!(childPtr = WIN_GetPtr( list[i] )) || childPtr == WND_OTHER_PROCESS)
                        continue;
                    if ((childPtr->rectWindow.left < x + xspacing) &&
                        (childPtr->rectWindow.right >= x) &&
                        (childPtr->rectWindow.top <= y) &&
                        (childPtr->rectWindow.bottom > y - yspacing))
                    {
                        WIN_ReleasePtr( childPtr );
                        break;  /* There's a window in there */
                    }
                    WIN_ReleasePtr( childPtr );
                }
                if (list[i])
                {
                    /* found something here, try next spot */
                    x += xspacing;
                    continue;
                }
            }

            /* No window was found, so it's OK for us */
            pt.x = x + (xspacing - GetSystemMetrics(SM_CXICON)) / 2;
            pt.y = y - (yspacing + GetSystemMetrics(SM_CYICON)) / 2;
            HeapFree( GetProcessHeap(), 0, list );
            return pt;

        } while(x <= rectParent.right-xspacing);
        y -= yspacing;
    }
}





UINT WINPOS_MinMaximize( HWND hwnd, UINT cmd, LPRECT rect )
{
    WND *wndPtr;
    UINT swpFlags = 0;
    POINT size;
    LONG old_style;
    WINDOWPLACEMENT wpl;

    TRACE("%p %u\n", hwnd, cmd );

    wpl.length = sizeof(wpl);
    GetWindowPlacement( hwnd, &wpl );

    if (HOOK_CallHooks( WH_CBT, HCBT_MINMAX, (WPARAM)hwnd, cmd, TRUE ))
        return SWP_NOSIZE | SWP_NOMOVE;

    if (IsIconic( hwnd ))
    {
        if (cmd == SW_MINIMIZE) return SWP_NOSIZE | SWP_NOMOVE;
        if (!SendMessageA( hwnd, WM_QUERYOPEN, 0, 0 )) return SWP_NOSIZE | SWP_NOMOVE;
        swpFlags |= SWP_NOCOPYBITS;
    }

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;

    size.x = wndPtr->rectWindow.left;
    size.y = wndPtr->rectWindow.top;

    switch( cmd )
    {
    case SW_MINIMIZE:
        if( wndPtr->dwStyle & WS_MAXIMIZE) wndPtr->flags |= WIN_RESTORE_MAX;
        else wndPtr->flags &= ~WIN_RESTORE_MAX;

        WIN_SetStyle( hwnd, WS_MINIMIZE, WS_MAXIMIZE );

        X11DRV_set_iconic_state( hwnd );

        wpl.ptMinPosition = WINPOS_FindIconPos( wndPtr, wpl.ptMinPosition );

        SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                 GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON) );
        swpFlags |= SWP_NOCOPYBITS;
        break;

    case SW_MAXIMIZE:
        WINPOS_GetMinMaxInfo( hwnd, &size, &wpl.ptMaxPosition, NULL, NULL );

        old_style = WIN_SetStyle( hwnd, WS_MAXIMIZE, WS_MINIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            WINPOS_ShowIconTitle( hwnd, FALSE );
            X11DRV_set_iconic_state( hwnd );
        }
        SetRect( rect, wpl.ptMaxPosition.x, wpl.ptMaxPosition.y, size.x, size.y );
        break;

    case SW_RESTORE:
        old_style = WIN_SetStyle( hwnd, 0, WS_MINIMIZE | WS_MAXIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            WINPOS_ShowIconTitle( hwnd, FALSE );
            X11DRV_set_iconic_state( hwnd );

            if( wndPtr->flags & WIN_RESTORE_MAX)
            {
                /* Restore to maximized position */
                WINPOS_GetMinMaxInfo( hwnd, &size, &wpl.ptMaxPosition, NULL, NULL);
                WIN_SetStyle( hwnd, WS_MAXIMIZE, 0 );
                SetRect( rect, wpl.ptMaxPosition.x, wpl.ptMaxPosition.y, size.x, size.y );
                break;
            }
        }
        else if (!(old_style & WS_MAXIMIZE)) break;

        /* Restore to normal position */

        *rect = wpl.rcNormalPosition;
        rect->right -= rect->left;
        rect->bottom -= rect->top;

        break;
    }

    WIN_ReleaseWndPtr( wndPtr );
    return swpFlags;
}


/***********************************************************************
 *              ShowWindow   (X11DRV.@)
 */
BOOL X11DRV_ShowWindow( HWND hwnd, INT cmd )
{
    WND* 	wndPtr = WIN_FindWndPtr( hwnd );
    BOOL 	wasVisible, showFlag;
    RECT 	newPos = {0, 0, 0, 0};
    UINT 	swp = 0;

    if (!wndPtr) return FALSE;
    hwnd = wndPtr->hwndSelf;  /* make it a full handle */

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;

    TRACE("hwnd=%p, cmd=%d, wasVisible %d\n", hwnd, cmd, wasVisible);

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) goto END;
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
            swp |= SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if( wndPtr->dwStyle & WS_CHILD) swp |= SWP_NOZORDER;
            break;
	case SW_SHOW:
            if (wasVisible) goto END;

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

            if( wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( hwnd, SW_RESTORE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    showFlag = (cmd != SW_HIDE);
    if (showFlag != wasVisible || cmd == SW_SHOWNA)
    {
        SendMessageW( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) goto END;
    }

    /* ShowWindow won't activate a not being maximized child window */
    if ((wndPtr->dwStyle & WS_CHILD) && cmd != SW_MAXIMIZE)
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


/**********************************************************************
 *		X11DRV_MapNotify
 */
void X11DRV_MapNotify( HWND hwnd, XMapEvent *event )
{
    struct x11drv_win_data *data;
    HWND hwndFocus = GetFocus();
    WND *win;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    if (!(win = WIN_GetPtr( hwnd ))) return;

    if (data->managed && (win->dwStyle & WS_VISIBLE) && (win->dwStyle & WS_MINIMIZE))
    {
        int x, y;
        unsigned int width, height, border, depth;
        Window root, top;
        RECT rect;
        LONG style = WS_VISIBLE;

        /* FIXME: hack */
        wine_tsx11_lock();
        XGetGeometry( event->display, data->whole_window, &root, &x, &y, &width, &height,
                        &border, &depth );
        XTranslateCoordinates( event->display, data->whole_window, root, 0, 0, &x, &y, &top );
        wine_tsx11_unlock();
        rect.left   = x;
        rect.top    = y;
        rect.right  = x + width;
        rect.bottom = y + height;
        X11DRV_X_to_window_rect( data, &rect );

        DCE_InvalidateDCE( hwnd, &data->window_rect );

        if (win->flags & WIN_RESTORE_MAX) style |= WS_MAXIMIZE;
        WIN_SetStyle( hwnd, style, WS_MINIMIZE );
        WIN_ReleasePtr( win );

        SendMessageA( hwnd, WM_SHOWWINDOW, SW_RESTORE, 0 );
        SetWindowPos( hwnd, 0, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
                      SWP_NOZORDER | SWP_WINE_NOHOSTMOVE );
    }
    else WIN_ReleasePtr( win );
    if (hwndFocus && IsChild( hwnd, hwndFocus )) X11DRV_SetFocus(hwndFocus);  /* FIXME */
}


/**********************************************************************
 *              X11DRV_UnmapNotify
 */
void X11DRV_UnmapNotify( HWND hwnd, XUnmapEvent *event )
{
    struct x11drv_win_data *data;
    WND *win;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    if (!(win = WIN_GetPtr( hwnd ))) return;

    if ((win->dwStyle & WS_VISIBLE) && data->managed &&
        X11DRV_is_window_rect_mapped( &win->rectWindow ))
    {
        if (win->dwStyle & WS_MAXIMIZE)
            win->flags |= WIN_RESTORE_MAX;
        else
            win->flags &= ~WIN_RESTORE_MAX;

        WIN_SetStyle( hwnd, WS_MINIMIZE, WS_MAXIMIZE );
        WIN_ReleasePtr( win );

        EndMenu();
        SendMessageA( hwnd, WM_SHOWWINDOW, SW_MINIMIZE, 0 );
        SetWindowPos( hwnd, 0, 0, 0, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_WINE_NOHOSTMOVE );
    }
    else WIN_ReleasePtr( win );
}


/***********************************************************************
 *           query_zorder
 *
 * Synchronize internal z-order with the window manager's.
 */
static Window __get_common_ancestor( Display *display, Window A, Window B,
                                     Window** children, unsigned* total )
{
    /* find the real root window */

    Window      root, *childrenB;
    unsigned    totalB;

    wine_tsx11_lock();
    while( A != B && A && B )
    {
      XQueryTree( display, A, &root, &A, children, total );
      XQueryTree( display, B, &root, &B, &childrenB, &totalB );
      if( childrenB ) XFree( childrenB );
      if( *children ) XFree( *children ), *children = NULL;
    }

    if( A && B )
    {
        XQueryTree( display, A, &root, &B, children, total );
        wine_tsx11_unlock();
        return A;
    }
    wine_tsx11_unlock();
    return 0 ;
}

static Window __get_top_decoration( Display *display, Window w, Window ancestor )
{
    Window*     children, root, prev = w, parent = w;
    unsigned    total;

    wine_tsx11_lock();
    do
    {
        w = parent;
        XQueryTree( display, w, &root, &parent, &children, &total );
        if( children ) XFree( children );
    } while( parent && parent != ancestor );
    wine_tsx11_unlock();
    TRACE("\t%08x -> %08x\n", (unsigned)prev, (unsigned)w );
    return ( parent ) ? w : 0 ;
}

static unsigned __td_lookup( Window w, Window* list, unsigned max )
{
    unsigned    i;
    for( i = max; i > 0; i-- ) if( list[i - 1] == w ) break;
    return i;
}

static HWND query_zorder( Display *display, HWND hWndCheck)
{
    HWND      hwndInsertAfter = HWND_TOP;
    Window      w, parent, *children = NULL;
    unsigned    total, check, pos, best;
    HWND *list = WIN_ListChildren( GetDesktopWindow() );
    HWND hwndA = 0, hwndB = 0;
    int i;

    /* find at least two managed windows */
    if (!list) return 0;
    for (i = 0; list[i]; i++)
    {
        if (!(GetWindowLongW( list[i], GWL_STYLE ) & WS_VISIBLE)) continue;
        if (!GetPropA( list[i], "__wine_x11_managed" )) continue;
        if (!hwndA) hwndA = list[i];
        else
        {
            hwndB = list[i];
            break;
        }
    }
    if (!hwndA || !hwndB) goto done;

    parent = __get_common_ancestor( display, X11DRV_get_whole_window(hwndA),
                                    X11DRV_get_whole_window(hwndB), &children, &total );
    if( parent && children )
    {
        /* w is the ancestor if hWndCheck that is a direct descendant of 'parent' */

        w = __get_top_decoration( display, X11DRV_get_whole_window(hWndCheck), parent );

        if( w != children[total-1] ) /* check if at the top */
        {
            /* X child at index 0 is at the bottom, at index total-1 is at the top */
            check = __td_lookup( w, children, total );
            best = total;

            /* go through all windows in Wine z-order... */
            for (i = 0; list[i]; i++)
            {
                if (list[i] == hWndCheck) continue;
                if (!GetPropA( list[i], "__wine_x11_managed" )) continue;
                if (!(w = __get_top_decoration( display, X11DRV_get_whole_window(list[i]),
                                                parent ))) continue;
                pos = __td_lookup( w, children, total );
                if( pos < best && pos > check )
                {
                    /* find a nearest Wine window precedes hWndCheck in the real z-order */
                    best = pos;
                    hwndInsertAfter = list[i];
                }
                if( best - check == 1 ) break;
            }
        }
    }
    wine_tsx11_lock();
    if( children ) XFree( children );
    wine_tsx11_unlock();

 done:
    HeapFree( GetProcessHeap(), 0, list );
    return hwndInsertAfter;
}


/***********************************************************************
 *		X11DRV_handle_desktop_resize
 */
void X11DRV_handle_desktop_resize( unsigned int width, unsigned int height )
{
    RECT rect;
    HWND hwnd = GetDesktopWindow();

    screen_width  = width;
    screen_height = height;
    TRACE("desktop %p change to (%dx%d)\n", hwnd, width, height);
    SetRect( &rect, 0, 0, width, height );
    X11DRV_set_window_pos( hwnd, 0, &rect, &rect, SWP_NOZORDER|SWP_NOMOVE|SWP_WINE_NOHOSTMOVE, NULL );
    SendMessageTimeoutW( HWND_BROADCAST, WM_DISPLAYCHANGE, screen_depth,
                         MAKELPARAM( width, height ), SMTO_ABORTIFHUNG, 2000, NULL );
}


/***********************************************************************
 *		X11DRV_ConfigureNotify
 */
void X11DRV_ConfigureNotify( HWND hwnd, XConfigureEvent *event )
{
    HWND oldInsertAfter;
    struct x11drv_win_data *data;
    RECT rect;
    WINDOWPOS winpos;
    int x = event->x, y = event->y;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    /* Get geometry */

    if (!event->send_event)  /* normal event, need to map coordinates to the root */
    {
        Window child;
        wine_tsx11_lock();
        XTranslateCoordinates( event->display, data->whole_window, root_window,
                               0, 0, &x, &y, &child );
        wine_tsx11_unlock();
    }
    rect.left   = x;
    rect.top    = y;
    rect.right  = x + event->width;
    rect.bottom = y + event->height;
    TRACE( "win %p new X rect %ld,%ld,%ldx%ld (event %d,%d,%dx%d)\n",
           hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
           event->x, event->y, event->width, event->height );
    X11DRV_X_to_window_rect( data, &rect );

    winpos.hwnd  = hwnd;
    winpos.x     = rect.left;
    winpos.y     = rect.top;
    winpos.cx    = rect.right - rect.left;
    winpos.cy    = rect.bottom - rect.top;
    winpos.flags = SWP_NOACTIVATE;

    /* Get Z-order (FIXME) */

    winpos.hwndInsertAfter = query_zorder( event->display, hwnd );

    /* needs to find the first Visible Window above the current one */
    oldInsertAfter = hwnd;
    for (;;)
    {
        oldInsertAfter = GetWindow( oldInsertAfter, GW_HWNDPREV );
        if (!oldInsertAfter)
        {
            oldInsertAfter = HWND_TOP;
            break;
        }
        if (GetWindowLongA( oldInsertAfter, GWL_STYLE ) & WS_VISIBLE) break;
    }

    /* Compare what has changed */

    GetWindowRect( hwnd, &rect );
    if (rect.left == winpos.x && rect.top == winpos.y) winpos.flags |= SWP_NOMOVE;
    else
        TRACE( "%p moving from (%ld,%ld) to (%d,%d)\n",
               hwnd, rect.left, rect.top, winpos.x, winpos.y );

    if ((rect.right - rect.left == winpos.cx && rect.bottom - rect.top == winpos.cy) ||
        IsIconic(hwnd) ||
        (IsRectEmpty( &rect ) && winpos.cx == 1 && winpos.cy == 1))
        winpos.flags |= SWP_NOSIZE;
    else
        TRACE( "%p resizing from (%ldx%ld) to (%dx%d)\n",
               hwnd, rect.right - rect.left, rect.bottom - rect.top,
               winpos.cx, winpos.cy );

    if (winpos.hwndInsertAfter == oldInsertAfter) winpos.flags |= SWP_NOZORDER;
    else
        TRACE( "%p restacking from after %p to after %p\n",
               hwnd, oldInsertAfter, winpos.hwndInsertAfter );

    /* if nothing changed, don't do anything */
    if (winpos.flags == (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) return;

    SetWindowPos( hwnd, winpos.hwndInsertAfter, winpos.x, winpos.y,
                  winpos.cx, winpos.cy, winpos.flags | SWP_WINE_NOHOSTMOVE );
}


/***********************************************************************
 *		SetWindowRgn  (X11DRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
int X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    struct x11drv_win_data *data;

    if (!(data = X11DRV_get_win_data( hwnd )))
    {
        if (IsWindow( hwnd ))
            FIXME( "not supported on other thread window %p\n", hwnd );
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

#ifdef HAVE_LIBXSHAPE
    if (data->whole_window)
    {
        Display *display = thread_display();

        if (!hrgn)
        {
            wine_tsx11_lock();
            XShapeCombineMask( display, data->whole_window,
                               ShapeBounding, 0, 0, None, ShapeSet );
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
                                         pRegionData->rdh.nCount,
                                         ShapeSet, YXBanded );
                wine_tsx11_unlock();
                HeapFree(GetProcessHeap(), 0, pRegionData);
            }
        }
    }
#endif  /* HAVE_LIBXSHAPE */

    return TRUE;
}


/***********************************************************************
 *           draw_moving_frame
 *
 * Draw the frame used when moving or resizing window.
 *
 * FIXME:  This causes problems in Win95 mode.  (why?)
 */
static void draw_moving_frame( HDC hdc, RECT *rect, BOOL thickframe )
{
    if (thickframe)
    {
        const int width = GetSystemMetrics(SM_CXFRAME);
        const int height = GetSystemMetrics(SM_CYFRAME);

        HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
        PatBlt( hdc, rect->left, rect->top,
                rect->right - rect->left - width, height, PATINVERT );
        PatBlt( hdc, rect->left, rect->top + height, width,
                rect->bottom - rect->top - height, PATINVERT );
        PatBlt( hdc, rect->left + width, rect->bottom - 1,
                rect->right - rect->left - width, -height, PATINVERT );
        PatBlt( hdc, rect->right - 1, rect->top, -width,
                rect->bottom - rect->top - height, PATINVERT );
        SelectObject( hdc, hbrush );
    }
    else DrawFocusRect( hdc, rect );
}


/***********************************************************************
 *           start_size_move
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG start_size_move( HWND hwnd, WPARAM wParam, POINT *capturePoint, LONG style )
{
    LONG hittest = 0;
    POINT pt;
    MSG msg;
    RECT rectWindow;

    GetWindowRect( hwnd, &rectWindow );

    if ((wParam & 0xfff0) == SC_MOVE)
    {
        /* Move pointer at the center of the caption */
        RECT rect = rectWindow;
        /* Note: to be exactly centered we should take the different types
         * of border into account, but it shouldn't make more that a few pixels
         * of difference so let's not bother with that */
        rect.top += GetSystemMetrics(SM_CYBORDER);
        if (style & WS_SYSMENU)
            rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MINIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MAXIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        pt.x = (rect.right + rect.left) / 2;
        pt.y = rect.top + GetSystemMetrics(SM_CYSIZE)/2;
        hittest = HTCAPTION;
        *capturePoint = pt;
    }
    else  /* SC_SIZE */
    {
        while(!hittest)
        {
            GetMessageW( &msg, 0, WM_KEYFIRST, WM_MOUSELAST );
            if (CallMsgFilterW( &msg, MSGF_SIZE )) continue;

            switch(msg.message)
            {
            case WM_MOUSEMOVE:
                pt = msg.pt;
                hittest = SendMessageW( hwnd, WM_NCHITTEST, 0, MAKELONG( pt.x, pt.y ) );
                if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT)) hittest = 0;
                break;

            case WM_LBUTTONUP:
                return 0;

            case WM_KEYDOWN:
                switch(msg.wParam)
                {
                case VK_UP:
                    hittest = HTTOP;
                    pt.x =(rectWindow.left+rectWindow.right)/2;
                    pt.y = rectWindow.top + GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_DOWN:
                    hittest = HTBOTTOM;
                    pt.x =(rectWindow.left+rectWindow.right)/2;
                    pt.y = rectWindow.bottom - GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_LEFT:
                    hittest = HTLEFT;
                    pt.x = rectWindow.left + GetSystemMetrics(SM_CXFRAME) / 2;
                    pt.y =(rectWindow.top+rectWindow.bottom)/2;
                    break;
                case VK_RIGHT:
                    hittest = HTRIGHT;
                    pt.x = rectWindow.right - GetSystemMetrics(SM_CXFRAME) / 2;
                    pt.y =(rectWindow.top+rectWindow.bottom)/2;
                    break;
                case VK_RETURN:
                case VK_ESCAPE: return 0;
                }
            }
        }
        *capturePoint = pt;
    }
    SetCursorPos( pt.x, pt.y );
    SendMessageW( hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           set_movesize_capture
 */
static void set_movesize_capture( HWND hwnd )
{
    HWND previous = 0;

    SERVER_START_REQ( set_capture_window )
    {
        req->handle = hwnd;
        req->flags  = CAPTURE_MOVESIZE;
        if (!wine_server_call_err( req ))
        {
            previous = reply->previous;
            hwnd = reply->full_handle;
        }
    }
    SERVER_END_REQ;
    if (previous && previous != hwnd)
        SendMessageW( previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd );
}

/***********************************************************************
 *           X11DRV_WMMoveResizeWindow
 *
 * Tells the window manager to initiate a move or resize operation.
 *
 * SEE
 *  http://freedesktop.org/Standards/wm-spec/1.3/ar01s04.html
 *  or search for "_NET_WM_MOVERESIZE"
 */
static void X11DRV_WMMoveResizeWindow( HWND hwnd, int x, int y, int dir )
{
    XEvent xev;
    Display *display = thread_display();

    xev.xclient.type = ClientMessage;
    xev.xclient.window = X11DRV_get_whole_window(hwnd);
    xev.xclient.message_type = x11drv_atom(_NET_WM_MOVERESIZE);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = x; /* x coord */
    xev.xclient.data.l[1] = y; /* y coord */
    xev.xclient.data.l[2] = dir; /* direction */
    xev.xclient.data.l[3] = 1; /* button */
    xev.xclient.data.l[4] = 0; /* unused */

    /* need to ungrab the pointer that may have been automatically grabbed
     * with a ButtonPress event */
    wine_tsx11_lock();
    XUngrabPointer( display, CurrentTime );
    XSendEvent(display, root_window, False, SubstructureNotifyMask, &xev);
    wine_tsx11_unlock();
}

/***********************************************************************
 *           SysCommandSizeMove   (X11DRV.@)
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
void X11DRV_SysCommandSizeMove( HWND hwnd, WPARAM wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect, origRect;
    HDC hdc;
    HWND parent;
    LONG hittest = (LONG)(wParam & 0x0f);
    WPARAM syscommand = wParam & 0xfff0;
    HCURSOR hDragCursor = 0, hOldCursor = 0;
    POINT minTrack, maxTrack;
    POINT capturePoint, pt;
    LONG style = GetWindowLongA( hwnd, GWL_STYLE );
    BOOL    thickframe = HAS_THICKFRAME( style );
    BOOL    iconic = style & WS_MINIMIZE;
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = FALSE;
    BOOL grab;
    Window parent_win, whole_win;
    Display *old_gdi_display = NULL;
    Display *display = thread_display();
    struct x11drv_win_data *data;

    pt.x = (short)LOWORD(dwPoint);
    pt.y = (short)HIWORD(dwPoint);
    capturePoint = pt;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd)) return;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    /* if we are managed then we let the WM do all the work */
    if (data->managed)
    {
        int dir;
        if (syscommand == SC_MOVE)
        {
            if (!hittest) dir = _NET_WM_MOVERESIZE_MOVE_KEYBOARD;
            else dir = _NET_WM_MOVERESIZE_MOVE;
        }
        else if (!hittest) dir = _NET_WM_MOVERESIZE_SIZE_KEYBOARD;
        else
            switch (hittest)
            {
            case WMSZ_LEFT:        dir = _NET_WM_MOVERESIZE_SIZE_LEFT; break;
            case WMSZ_RIGHT:       dir = _NET_WM_MOVERESIZE_SIZE_RIGHT; break;
            case WMSZ_TOP:         dir = _NET_WM_MOVERESIZE_SIZE_TOP; break;
            case WMSZ_TOPLEFT:     dir = _NET_WM_MOVERESIZE_SIZE_TOPLEFT; break;
            case WMSZ_TOPRIGHT:    dir = _NET_WM_MOVERESIZE_SIZE_TOPRIGHT; break;
            case WMSZ_BOTTOM:      dir = _NET_WM_MOVERESIZE_SIZE_BOTTOM; break;
            case WMSZ_BOTTOMLEFT:  dir = _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT; break;
            case WMSZ_BOTTOMRIGHT: dir = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT; break;
            default:
                ERR("Invalid hittest value: %ld\n", hittest);
                dir = _NET_WM_MOVERESIZE_SIZE_KEYBOARD;
            }
        X11DRV_WMMoveResizeWindow( hwnd, pt.x, pt.y, dir );
        return;
    }

    SystemParametersInfoA(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);

    if (syscommand == SC_MOVE)
    {
        if (!hittest) hittest = start_size_move( hwnd, wParam, &capturePoint, style );
        if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
        if ( hittest && (syscommand != SC_MOUSEMENU) )
            hittest += (HTLEFT - WMSZ_LEFT);
        else
        {
            set_movesize_capture( hwnd );
            hittest = start_size_move( hwnd, wParam, &capturePoint, style );
            if (!hittest)
            {
                set_movesize_capture(0);
                return;
            }
        }
    }

      /* Get min/max info */

    WINPOS_GetMinMaxInfo( hwnd, NULL, NULL, &minTrack, &maxTrack );
    GetWindowRect( hwnd, &sizingRect );
    if (style & WS_CHILD)
    {
        parent = GetParent(hwnd);
        /* make sizing rect relative to parent */
        MapWindowPoints( 0, parent, (POINT*)&sizingRect, 2 );
        GetClientRect( parent, &mouseRect );
    }
    else
    {
        parent = 0;
        SetRect(&mouseRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }
    origRect = sizingRect;

    if (ON_LEFT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.right-maxTrack.x );
        mouseRect.right = min( mouseRect.right, sizingRect.right-minTrack.x );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.left+minTrack.x );
        mouseRect.right = min( mouseRect.right, sizingRect.left+maxTrack.x );
    }
    if (ON_TOP_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.bottom-maxTrack.y );
        mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.top+minTrack.y );
        mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
    if (parent) MapWindowPoints( parent, 0, (LPPOINT)&mouseRect, 2 );

    /* Retrieve a default cache DC (without using the window style) */
    hdc = GetDCEx( parent, 0, DCX_CACHE );

    if( iconic ) /* create a cursor for dragging */
    {
        hDragCursor = (HCURSOR)GetClassLongPtrW( hwnd, GCLP_HICON);
        if( !hDragCursor ) hDragCursor = (HCURSOR)SendMessageW( hwnd, WM_QUERYDRAGICON, 0, 0L);
        if( !hDragCursor ) iconic = FALSE;
    }

    /* repaint the window before moving it around */
    RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    SendMessageA( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
    set_movesize_capture( hwnd );

    /* grab the server only when moving top-level windows without desktop */
    grab = (!DragFullWindows && !parent && (root_window == DefaultRootWindow(gdi_display)));

    if (grab)
    {
        wine_tsx11_lock();
        XSync( gdi_display, False );
        XGrabServer( display );
        XSync( display, False );
        /* switch gdi display to the thread display, since the server is grabbed */
        old_gdi_display = gdi_display;
        gdi_display = display;
        wine_tsx11_unlock();
    }
    whole_win = X11DRV_get_whole_window( GetAncestor(hwnd,GA_ROOT) );
    parent_win = parent ? X11DRV_get_whole_window( GetAncestor(parent,GA_ROOT) ) : root_window;

    wine_tsx11_lock();
    XGrabPointer( display, whole_win, False,
                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                  GrabModeAsync, GrabModeAsync, parent_win, None, CurrentTime );
    wine_tsx11_unlock();

    while(1)
    {
        int dx = 0, dy = 0;

        if (!GetMessageW( &msg, 0, WM_KEYFIRST, WM_MOUSELAST )) break;
        if (CallMsgFilterW( &msg, MSGF_SIZE )) continue;

        /* Exit on button-up, Return, or Esc */
        if ((msg.message == WM_LBUTTONUP) ||
            ((msg.message == WM_KEYDOWN) &&
             ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

        if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
            continue;  /* We are not interested in other messages */

        pt = msg.pt;

        if (msg.message == WM_KEYDOWN) switch(msg.wParam)
        {
        case VK_UP:    pt.y -= 8; break;
        case VK_DOWN:  pt.y += 8; break;
        case VK_LEFT:  pt.x -= 8; break;
        case VK_RIGHT: pt.x += 8; break;
        }

        pt.x = max( pt.x, mouseRect.left );
        pt.x = min( pt.x, mouseRect.right );
        pt.y = max( pt.y, mouseRect.top );
        pt.y = min( pt.y, mouseRect.bottom );

        dx = pt.x - capturePoint.x;
        dy = pt.y - capturePoint.y;

        if (dx || dy)
        {
            if( !moved )
            {
                moved = TRUE;

                if( iconic ) /* ok, no system popup tracking */
                {
                    hOldCursor = SetCursor(hDragCursor);
                    ShowCursor( TRUE );
                    WINPOS_ShowIconTitle( hwnd, FALSE );
                }
                else if(!DragFullWindows)
                    draw_moving_frame( hdc, &sizingRect, thickframe );
            }

            if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
            else
            {
                RECT newRect = sizingRect;
                WPARAM wpSizingHit = 0;

                if (hittest == HTCAPTION) OffsetRect( &newRect, dx, dy );
                if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
                else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
                if (ON_TOP_BORDER(hittest)) newRect.top += dy;
                else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
                if(!iconic && !DragFullWindows) draw_moving_frame( hdc, &sizingRect, thickframe );
                capturePoint = pt;

                /* determine the hit location */
                if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                    wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
                SendMessageA( hwnd, WM_SIZING, wpSizingHit, (LPARAM)&newRect );

                if (!iconic)
                {
                    if(!DragFullWindows)
                        draw_moving_frame( hdc, &newRect, thickframe );
                    else
                        SetWindowPos( hwnd, 0, newRect.left, newRect.top,
                                      newRect.right - newRect.left,
                                      newRect.bottom - newRect.top,
                                      ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
                }
                sizingRect = newRect;
            }
        }
    }

    set_movesize_capture(0);
    if( iconic )
    {
        if( moved ) /* restore cursors, show icon title later on */
        {
            ShowCursor( FALSE );
            SetCursor( hOldCursor );
        }
    }
    else if (moved && !DragFullWindows)
        draw_moving_frame( hdc, &sizingRect, thickframe );

    ReleaseDC( parent, hdc );

    wine_tsx11_lock();
    XUngrabPointer( display, CurrentTime );
    if (grab)
    {
        XSync( display, False );
        XUngrabServer( display );
        XSync( display, False );
        gdi_display = old_gdi_display;
    }
    wine_tsx11_unlock();

    if (HOOK_CallHooks( WH_CBT, HCBT_MOVESIZE, (WPARAM)hwnd, (LPARAM)&sizingRect, TRUE ))
        moved = FALSE;

    SendMessageA( hwnd, WM_EXITSIZEMOVE, 0, 0 );
    SendMessageA( hwnd, WM_SETVISIBLE, !IsIconic(hwnd), 0L);

    /* window moved or resized */
    if (moved)
    {
        /* if the moving/resizing isn't canceled call SetWindowPos
         * with the new position or the new size of the window
         */
        if (!((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
        {
            /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
            if(!DragFullWindows)
                SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
                              sizingRect.right - sizingRect.left,
                              sizingRect.bottom - sizingRect.top,
                              ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
        else
        { /* restore previous size/position */
            if(DragFullWindows)
                SetWindowPos( hwnd, 0, origRect.left, origRect.top,
                              origRect.right - origRect.left,
                              origRect.bottom - origRect.top,
                              ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
    }

    if (IsIconic(hwnd))
    {
        /* Single click brings up the system menu when iconized */

        if( !moved )
        {
            if(style & WS_SYSMENU )
                SendMessageA( hwnd, WM_SYSCOMMAND,
                              SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x,pt.y));
        }
        else WINPOS_ShowIconTitle( hwnd, TRUE );
    }
}

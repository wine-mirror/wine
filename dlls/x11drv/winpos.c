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
#include "wownt32.h"

#include "x11drv.h"
#include "win.h"
#include "winpos.h"
#include "dce.h"
#include "cursoricon.h"
#include "nonclient.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

#define SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

#define SWP_EX_NOCOPY       0x0001
#define SWP_EX_PAINTSELF    0x0002
#define SWP_EX_NONCLIENT    0x0004

#define HAS_THICKFRAME(style,exStyle) \
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


/***********************************************************************
 *		clip_children
 *
 * Clip all children of a given window out of the visible region
 */
static int clip_children( HWND parent, HWND last, HRGN hrgn, int whole_window )
{
    HWND *list;
    WND *ptr;
    HRGN rectRgn;
    int i, x, y, ret = SIMPLEREGION;

    /* first check if we have anything to do */
    if (!(list = WIN_ListChildren( parent ))) return ret;

    if (whole_window)
    {
        WND *win = WIN_FindWndPtr( parent );
        x = win->rectWindow.left - win->rectClient.left;
        y = win->rectWindow.top - win->rectClient.top;
        WIN_ReleaseWndPtr( win );
    }
    else x = y = 0;

    rectRgn = CreateRectRgn( 0, 0, 0, 0 );

    for (i = 0; list[i] && list[i] != last; i++)
    {
        if (!(ptr = WIN_FindWndPtr( list[i] ))) continue;
        if ((ptr->dwStyle & WS_VISIBLE) && !(ptr->dwExStyle & WS_EX_TRANSPARENT))
        {
            SetRectRgn( rectRgn, ptr->rectWindow.left + x, ptr->rectWindow.top + y,
                        ptr->rectWindow.right + x, ptr->rectWindow.bottom + y );
            if ((ret = CombineRgn( hrgn, hrgn, rectRgn, RGN_DIFF )) == NULLREGION)
            {
                WIN_ReleaseWndPtr( ptr );
                break;  /* no need to go on, region is empty */
            }
        }
        WIN_ReleaseWndPtr( ptr );
    }
    DeleteObject( rectRgn );
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/***********************************************************************
 *		get_visible_region
 *
 * Compute the visible region of a window
 */
static HRGN get_visible_region( WND *win, HWND top, UINT flags, int mode )
{
    HRGN rgn;
    RECT rect;
    int xoffset, yoffset;
    X11DRV_WND_DATA *data = win->pDriverData;

    if (flags & DCX_WINDOW)
    {
        xoffset = win->rectWindow.left;
        yoffset = win->rectWindow.top;
    }
    else
    {
        xoffset = win->rectClient.left;
        yoffset = win->rectClient.top;
    }

    if (flags & DCX_PARENTCLIP)
        GetClientRect( win->parent, &rect );
    else if (flags & DCX_WINDOW)
        rect = data->whole_rect;
    else
        rect = win->rectClient;

    /* vis region is relative to the start of the client/window area */
    OffsetRect( &rect, -xoffset, -yoffset );

    if (!(rgn = CreateRectRgn( rect.left, rect.top, rect.right, rect.bottom ))) return 0;

    if ((flags & DCX_CLIPCHILDREN) && (mode != ClipByChildren))
    {
        /* we need to clip children by hand */
        if (clip_children( win->hwndSelf, 0, rgn, (flags & DCX_WINDOW) ) == NULLREGION) return rgn;
    }

    if (top && top != win->hwndSelf)  /* need to clip siblings of ancestors */
    {
        WND *parent, *ptr = WIN_FindWndPtr( win->hwndSelf );
        HRGN tmp = 0;

        OffsetRgn( rgn, xoffset, yoffset );
        for (;;)
        {
            if (ptr->dwStyle & WS_CLIPSIBLINGS)
            {
                if (clip_children( ptr->parent, ptr->hwndSelf, rgn, FALSE ) == NULLREGION) break;
            }
            if (ptr->hwndSelf == top) break;
            if (!(parent = WIN_FindWndPtr( ptr->parent ))) break;
            WIN_ReleaseWndPtr( ptr );
            ptr = parent;
            /* clip to parent client area */
            if (tmp) SetRectRgn( tmp, 0, 0, ptr->rectClient.right - ptr->rectClient.left,
                                 ptr->rectClient.bottom - ptr->rectClient.top );
            else tmp = CreateRectRgn( 0, 0, ptr->rectClient.right - ptr->rectClient.left,
                                      ptr->rectClient.bottom - ptr->rectClient.top );
            CombineRgn( rgn, rgn, tmp, RGN_AND );
            OffsetRgn( rgn, ptr->rectClient.left, ptr->rectClient.top );
            xoffset += ptr->rectClient.left;
            yoffset += ptr->rectClient.top;
        }
        WIN_ReleaseWndPtr( ptr );
        /* make it relative to the target window again */
        OffsetRgn( rgn, -xoffset, -yoffset );
        if (tmp) DeleteObject( tmp );
    }

    return rgn;
}


/***********************************************************************
 *		get_covered_region
 *
 * Compute the portion of 'rgn' that is covered by non-clipped siblings.
 * This is the area that is covered from X point of view, but may still need
 * to be exposed.
 * 'rgn' must be relative to the client area of the parent of 'win'.
 */
static int get_covered_region( WND *win, HRGN rgn )
{
    HRGN tmp;
    int ret;
    WND *parent, *ptr = WIN_FindWndPtr( win->hwndSelf );
    int xoffset = 0, yoffset = 0;

    tmp = CreateRectRgn( 0, 0, 0, 0 );
    CombineRgn( tmp, rgn, 0, RGN_COPY );

    /* to make things easier we actually build the uncovered
     * area by removing all siblings and then we subtract that
     * from the total region to get the covered area */
    for (;;)
    {
        if (!(ptr->dwStyle & WS_CLIPSIBLINGS))
        {
            if (clip_children( ptr->parent, ptr->hwndSelf, tmp, FALSE ) == NULLREGION) break;
        }
        if (!(parent = WIN_FindWndPtr( ptr->parent ))) break;
        WIN_ReleaseWndPtr( ptr );
        ptr = parent;
        OffsetRgn( tmp, ptr->rectClient.left, ptr->rectClient.top );
        xoffset += ptr->rectClient.left;
        yoffset += ptr->rectClient.top;
    }
    WIN_ReleaseWndPtr( ptr );
    /* make it relative to the target window again */
    OffsetRgn( tmp, -xoffset, -yoffset );

    /* now subtract the computed region from the original one */
    ret = CombineRgn( rgn, rgn, tmp, RGN_DIFF );
    DeleteObject( tmp );
    return ret;
}


/***********************************************************************
 *		expose_window
 *
 * Expose a region of a given window.
 */
static void expose_window( HWND hwnd, RECT *rect, HRGN rgn, int flags )
{
    POINT offset;
    HWND top = 0;
    HWND *list;
    int i;

    /* find the top most parent that doesn't clip children or siblings and
     * invalidate the area on its parent, including all children */
    if ((list = WIN_ListParents( hwnd )))
    {
        HWND current = hwnd;
        LONG style = GetWindowLongW( hwnd, GWL_STYLE );
        for (i = 0; list[i] && list[i] != GetDesktopWindow(); i++)
        {
            if (!(style & WS_CLIPSIBLINGS)) top = current;
            style = GetWindowLongW( list[i], GWL_STYLE );
            if (!(style & WS_CLIPCHILDREN)) top = current;
            current = list[i];
        }

        if (top)
        {
            /* find the parent of the top window, reusing the parent list */
            if (top == hwnd) i = 0;
            else
            {
                for (i = 0; list[i]; i++) if (list[i] == top) break;
                if (list[i] && list[i+1]) i++;
            }
            if (list[i] != GetDesktopWindow()) top = list[i];
            flags &= ~RDW_FRAME;  /* parent will invalidate children frame anyway */
            flags |= RDW_ALLCHILDREN;
        }
        HeapFree( GetProcessHeap(), 0, list );
    }

    if (!top) top = hwnd;

    /* make coords relative to top */
    offset.x = offset.y = 0;
    MapWindowPoints( hwnd, top, &offset, 1 );

    if (rect)
    {
        OffsetRect( rect, offset.x, offset.y );
        RedrawWindow( top, rect, 0, flags );
    }
    else
    {
        OffsetRgn( rgn, offset.x, offset.y );
        RedrawWindow( top, NULL, rgn, flags );
    }
}


/***********************************************************************
 *		expose_covered_parent_area
 *
 * Expose the parent area that has been uncovered by moving/hiding a
 * given window, but that is still covered by other siblings (the area
 * not covered by siblings will be exposed automatically by X).
 */
static void expose_covered_parent_area( WND *win, const RECT *old_rect )
{
    int ret = SIMPLEREGION;
    HRGN hrgn = CreateRectRgnIndirect( old_rect );

    if (win->dwStyle & WS_VISIBLE)
    {
        HRGN tmp = CreateRectRgnIndirect( &win->rectWindow );
        ret = CombineRgn( hrgn, hrgn, tmp, RGN_DIFF );
        DeleteObject( tmp );
    }

    if (ret != NULLREGION)
    {
        if (get_covered_region( win, hrgn ) != NULLREGION)
            expose_window( win->parent, NULL, hrgn, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
    }
    DeleteObject( hrgn );
}


/***********************************************************************
 *		expose_covered_window_area
 *
 * Expose the area of a window that is covered by other siblings.
 */
static void expose_covered_window_area( WND *win, const RECT *old_client_rect, BOOL frame )
{
    HRGN hrgn;
    int ret = SIMPLEREGION;

    if (frame)
        hrgn = CreateRectRgn( win->rectWindow.left - win->rectClient.left,
                              win->rectWindow.top - win->rectClient.top,
                              win->rectWindow.right - win->rectWindow.left,
                              win->rectWindow.bottom - win->rectWindow.top );
    else
        hrgn = CreateRectRgn( 0, 0,
                              win->rectClient.right - win->rectClient.left,
                              win->rectClient.bottom - win->rectClient.top );

    /* if the client rect didn't move we don't need to repaint it all */
    if (old_client_rect->left == win->rectClient.left &&
        old_client_rect->top == win->rectClient.top)
    {
        RECT rc;

        if (IntersectRect( &rc, old_client_rect, &win->rectClient ))
        {
            HRGN tmp;
            /* subtract the unchanged client area from the region to expose */
            OffsetRect( &rc, -win->rectClient.left, -win->rectClient.top );
            if ((tmp = CreateRectRgnIndirect( &rc )))
            {
                ret = CombineRgn( hrgn, hrgn, tmp, RGN_DIFF );
                DeleteObject( tmp );
            }
        }
    }

    if (ret != NULLREGION)
    {
        if (get_covered_region( win, hrgn ) != NULLREGION)
            expose_window( win->hwndSelf, NULL, hrgn,
                           RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
    }

    DeleteObject( hrgn );
}


/***********************************************************************
 *           X11DRV_Expose
 */
void X11DRV_Expose( HWND hwnd, XExposeEvent *event )
{
    RECT rect;
    struct x11drv_win_data *data;
    int flags = RDW_INVALIDATE | RDW_ERASE;
    WND *win;

    TRACE( "win %p (%lx) %d,%d %dx%d\n",
           hwnd, event->window, event->x, event->y, event->width, event->height );

    rect.left   = event->x;
    rect.top    = event->y;
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    if (!(win = WIN_GetPtr( hwnd ))) return;
    data = win->pDriverData;

    if (event->window != data->client_window)  /* whole window or icon window */
    {
        flags |= RDW_FRAME;
        /* make position relative to client area instead of window */
        OffsetRect( &rect, -data->client_rect.left, -data->client_rect.top );
    }
    WIN_ReleasePtr( win );

    expose_window( hwnd, &rect, 0, flags );
}


/***********************************************************************
 *		GetDC (X11DRV.@)
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
BOOL X11DRV_GetDC( HWND hwnd, HDC hdc, HRGN hrgn, DWORD flags )
{
    WND *win = WIN_GetPtr( hwnd );
    HWND top = 0;
    X11DRV_WND_DATA *data = win->pDriverData;
    struct x11drv_escape_set_drawable escape;
    BOOL visible;

    escape.mode = IncludeInferiors;
    /* don't clip siblings if using parent clip region */
    if (flags & DCX_PARENTCLIP) flags &= ~DCX_CLIPSIBLINGS;

    /* find the top parent in the hierarchy that isn't clipping siblings */
    visible = (win->dwStyle & WS_VISIBLE) != 0;

    if (visible)
    {
        HWND *list = WIN_ListParents( hwnd );
        if (list)
        {
            int i;
            for (i = 0; list[i] != GetDesktopWindow(); i++)
            {
                LONG style = GetWindowLongW( list[i], GWL_STYLE );
                if (!(style & WS_VISIBLE))
                {
                    visible = FALSE;
                    top = 0;
                    break;
                }
                if (!(style & WS_CLIPSIBLINGS)) top = list[i];
            }
            HeapFree( GetProcessHeap(), 0, list );
        }
        if (!top && visible && !(flags & DCX_CLIPSIBLINGS)) top = hwnd;
    }

    if (top)
    {
        HWND parent = GetAncestor( top, GA_PARENT );
        escape.org.x = escape.org.y = 0;
        if (flags & DCX_WINDOW)
        {
            escape.org.x = win->rectWindow.left - win->rectClient.left;
            escape.org.y = win->rectWindow.top - win->rectClient.top;
        }
        MapWindowPoints( hwnd, parent, &escape.org, 1 );
        escape.drawable_org.x = escape.drawable_org.y = 0;
        MapWindowPoints( parent, 0, &escape.drawable_org, 1 );
        /* have to use the parent so that we include siblings */
        if (parent) escape.drawable = X11DRV_get_client_window( parent );
        else escape.drawable = root_window;
    }
    else
    {
        if (IsIconic( hwnd ))
        {
            escape.drawable = data->icon_window ? data->icon_window : data->whole_window;
            escape.org.x = 0;
            escape.org.y = 0;
            escape.drawable_org = escape.org;
        }
        else if (flags & DCX_WINDOW)
        {
            escape.drawable = data->whole_window;
            escape.org.x = win->rectWindow.left - data->whole_rect.left;
            escape.org.y = win->rectWindow.top - data->whole_rect.top;
            escape.drawable_org.x = data->whole_rect.left - win->rectClient.left;
            escape.drawable_org.y = data->whole_rect.top - win->rectClient.top;
        }
        else
        {
            escape.drawable = data->client_window;
            escape.org.x = 0;
            escape.org.y = 0;
            escape.drawable_org = escape.org;
            if (flags & DCX_CLIPCHILDREN) escape.mode = ClipByChildren;  /* can use X11 clipping */
        }
        MapWindowPoints( hwnd, 0, &escape.drawable_org, 1 );
    }

    escape.code = X11DRV_SET_DRAWABLE;
    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) ||
        SetHookFlags16( HDC_16(hdc), DCHF_VALIDATEVISRGN ))  /* DC was dirty */
    {
        /* need to recompute the visible region */
        HRGN visRgn;

        if (visible)
        {
            visRgn = get_visible_region( win, top, flags, escape.mode );

            if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
                CombineRgn( visRgn, visRgn, hrgn,
                            (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
        }
        else visRgn = CreateRectRgn( 0, 0, 0, 0 );

        SelectVisRgn16( HDC_16(hdc), HRGN_16(visRgn) );
        DeleteObject( visRgn );
    }

    WIN_ReleasePtr( win );
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
        SendMessageA( pWinpos->hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)pWinpos );

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
    WIN_ReleasePtr( wndPtr );
    return TRUE;
}

/***********************************************************************
 *           SWP_DoNCCalcSize
 */
static UINT SWP_DoNCCalcSize( WINDOWPOS* pWinpos, RECT* pNewWindowRect, RECT* pNewClientRect )
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

        /* If the application send back garbage, ignore it */
        if (params.rgrc[0].left <= params.rgrc[0].right &&
            params.rgrc[0].top <= params.rgrc[0].bottom)
            *pNewClientRect = params.rgrc[0];

         /* FIXME: WVR_ALIGNxxx */

        if (!(wndPtr = WIN_GetPtr( pWinpos->hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;

        if( pNewClientRect->left != wndPtr->rectClient.left ||
            pNewClientRect->top != wndPtr->rectClient.top )
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;

        if( (pNewClientRect->right - pNewClientRect->left !=
             wndPtr->rectClient.right - wndPtr->rectClient.left) ||
            (pNewClientRect->bottom - pNewClientRect->top !=
             wndPtr->rectClient.bottom - wndPtr->rectClient.top) )
            pWinpos->flags &= ~SWP_NOCLIENTSIZE;
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
    WND *wndPtr = WIN_GetPtr( winpos->hwnd );
    BOOL ret = TRUE;

    if (!wndPtr || wndPtr == WND_OTHER_PROCESS)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    winpos->hwnd = wndPtr->hwndSelf;  /* make it a full handle */

    if (wndPtr->dwStyle & WS_VISIBLE) winpos->flags &= ~SWP_SHOWWINDOW;
    else
    {
        winpos->flags &= ~SWP_HIDEWINDOW;
        if (!(winpos->flags & SWP_SHOWWINDOW)) winpos->flags |= SWP_NOREDRAW;
    }

    if (winpos->cx < 0) winpos->cx = 0;
    if (winpos->cy < 0) winpos->cy = 0;

    if ((wndPtr->rectWindow.right - wndPtr->rectWindow.left == winpos->cx) &&
        (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top == winpos->cy))
        winpos->flags |= SWP_NOSIZE;    /* Already the right size */

    if ((wndPtr->rectWindow.left == winpos->x) && (wndPtr->rectWindow.top == winpos->y))
        winpos->flags |= SWP_NOMOVE;    /* Already the right position */

    if (winpos->hwnd == GetForegroundWindow())
        winpos->flags |= SWP_NOACTIVATE;   /* Already active */
    else if ((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)
    {
        if (!(winpos->flags & SWP_NOACTIVATE)) /* Bring to the top when activating */
        {
            winpos->flags &= ~SWP_NOZORDER;
            winpos->hwndInsertAfter = HWND_TOP;
            goto done;
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
    if ((winpos->hwndInsertAfter != HWND_TOP) && (winpos->hwndInsertAfter != HWND_BOTTOM))
    {
        if (GetAncestor( winpos->hwndInsertAfter, GA_PARENT ) != wndPtr->parent) ret = FALSE;
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
 *		set_visible_style
 *
 * Set/clear the WS_VISIBLE style of a window and map/unmap the X window.
 */
static void set_visible_style( HWND hwnd, BOOL set )
{
    WND *win;

    if (!(win = WIN_GetPtr( hwnd ))) return;
    if (win == WND_OTHER_PROCESS) return;

    TRACE( "hwnd %p (%lx) set %d visible %d empty %d\n",
           hwnd, get_whole_window(win),
           set, (win->dwStyle & WS_VISIBLE) != 0, IsRectEmpty(&win->rectWindow) );

    if (set)
    {
        if (win->dwStyle & WS_VISIBLE) goto done;
        WIN_SetStyle( hwnd, win->dwStyle | WS_VISIBLE );
        if (!IsRectEmpty( &win->rectWindow ) && get_whole_window(win) && is_window_top_level(win))
        {
            Display *display = thread_display();
            X11DRV_sync_window_style( display, win );
            X11DRV_set_wm_hints( display, win );
            TRACE( "mapping win %p\n", hwnd );
            wine_tsx11_lock();
            XMapWindow( display, get_whole_window(win) );
            wine_tsx11_unlock();
        }
    }
    else
    {
        if (!(win->dwStyle & WS_VISIBLE)) goto done;
        WIN_SetStyle( hwnd, win->dwStyle & ~WS_VISIBLE );
        if (!IsRectEmpty( &win->rectWindow ) && get_whole_window(win) && is_window_top_level(win))
        {
            TRACE( "unmapping win %p\n", hwnd );
            wine_tsx11_lock();
            XUnmapWindow( thread_display(), get_whole_window(win) );
            wine_tsx11_unlock();
        }
    }
 done:
    WIN_ReleasePtr( win );
}


/***********************************************************************
 *		SetWindowStyle   (X11DRV.@)
 *
 * Update the X state of a window to reflect a style change
 */
void X11DRV_SetWindowStyle( HWND hwnd, LONG oldStyle )
{
    Display *display = thread_display();
    WND *wndPtr;
    LONG changed;

    if (hwnd == GetDesktopWindow()) return;
    if (!(wndPtr = WIN_GetPtr( hwnd ))) return;
    if (wndPtr == WND_OTHER_PROCESS) return;

    changed = wndPtr->dwStyle ^ oldStyle;

    if (changed & WS_VISIBLE)
    {
        if (!IsRectEmpty( &wndPtr->rectWindow ))
        {
            if (wndPtr->dwStyle & WS_VISIBLE)
            {
                TRACE( "mapping win %p\n", hwnd );
                if (is_window_top_level(wndPtr))
                {
                    X11DRV_sync_window_style( display, wndPtr );
                    X11DRV_set_wm_hints( display, wndPtr );
                }
                wine_tsx11_lock();
                XMapWindow( display, get_whole_window(wndPtr) );
                wine_tsx11_unlock();
            }
            else if (!is_window_top_level(wndPtr))  /* don't unmap managed windows */
            {
                TRACE( "unmapping win %p\n", hwnd );
                wine_tsx11_lock();
                XUnmapWindow( display, get_whole_window(wndPtr) );
                wine_tsx11_unlock();
            }
        }
    }

    if (changed & WS_DISABLED)
    {
        if (wndPtr->dwExStyle & WS_EX_MANAGED)
        {
            XWMHints *wm_hints;
            wine_tsx11_lock();
            if (!(wm_hints = XGetWMHints( display, get_whole_window(wndPtr) )))
                wm_hints = XAllocWMHints();
            if (wm_hints)
            {
                wm_hints->flags |= InputHint;
                wm_hints->input = !(wndPtr->dwStyle & WS_DISABLED);
                XSetWMHints( display, get_whole_window(wndPtr), wm_hints );
                XFree(wm_hints);
            }
            wine_tsx11_unlock();
        }
    }
    WIN_ReleasePtr(wndPtr);
}


/***********************************************************************
 *		SetWindowPos   (X11DRV.@)
 */
BOOL X11DRV_SetWindowPos( WINDOWPOS *winpos )
{
    WND *wndPtr;
    RECT newWindowRect, newClientRect;
    RECT oldWindowRect, oldClientRect;
    UINT wvrFlags = 0;
    BOOL bChangePos;

    TRACE( "hwnd %p, after %p, swp %d,%d %dx%d flags %08x\n",
           winpos->hwnd, winpos->hwndInsertAfter, winpos->x, winpos->y,
           winpos->cx, winpos->cy, winpos->flags);

    bChangePos = !(winpos->flags & SWP_WINE_NOHOSTMOVE);
    winpos->flags &= ~SWP_WINE_NOHOSTMOVE;

    /* Check window handle */
    if (winpos->hwnd == GetDesktopWindow()) return FALSE;

    if (!SWP_DoWinPosChanging( winpos, &newWindowRect, &newClientRect )) return FALSE;

    /* Fix redundant flags */
    if (!fixup_flags( winpos )) return FALSE;

    if (!(wndPtr = WIN_FindWndPtr( winpos->hwnd ))) return FALSE;

    TRACE("\tcurrent (%ld,%ld)-(%ld,%ld), style %08x\n",
          wndPtr->rectWindow.left, wndPtr->rectWindow.top,
          wndPtr->rectWindow.right, wndPtr->rectWindow.bottom, (unsigned)wndPtr->dwStyle );

    if((winpos->flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) != SWP_NOZORDER)
    {
        if (GetAncestor( winpos->hwnd, GA_PARENT ) == GetDesktopWindow())
            winpos->hwndInsertAfter = SWP_DoOwnedPopups( winpos->hwnd, winpos->hwndInsertAfter );
    }

    /* Common operations */

    wvrFlags = SWP_DoNCCalcSize( winpos, &newWindowRect, &newClientRect );

    if(!(winpos->flags & SWP_NOZORDER) && winpos->hwnd != winpos->hwndInsertAfter)
    {
        HWND parent = GetAncestor( winpos->hwnd, GA_PARENT );
        if (parent) WIN_LinkWindow( winpos->hwnd, parent, winpos->hwndInsertAfter );
    }

    /* Reset active DCEs */

    if( (((winpos->flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) &&
         wndPtr->dwStyle & WS_VISIBLE) ||
        (winpos->flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)) )
    {
        RECT rect;

        UnionRect(&rect, &newWindowRect, &wndPtr->rectWindow);
        DCE_InvalidateDCE(wndPtr->hwndSelf, &rect);
    }

    oldWindowRect = wndPtr->rectWindow;
    oldClientRect = wndPtr->rectClient;

    /* Find out if we have to redraw the whole client rect */

    if( oldClientRect.bottom - oldClientRect.top ==
        newClientRect.bottom - newClientRect.top ) wvrFlags &= ~WVR_VREDRAW;

    if( oldClientRect.right - oldClientRect.left ==
        newClientRect.right - newClientRect.left ) wvrFlags &= ~WVR_HREDRAW;

    /* FIXME: actually do something with WVR_VALIDRECTS */

    WIN_SetRectangles( winpos->hwnd, &newWindowRect, &newClientRect );

    if (get_whole_window(wndPtr))  /* don't do anything if X window not created yet */
    {
        Display *display = thread_display();

        if (!(winpos->flags & SWP_SHOWWINDOW) && (winpos->flags & SWP_HIDEWINDOW))
        {
            /* clear the update region */
            RedrawWindow( winpos->hwnd, NULL, 0, RDW_VALIDATE | RDW_NOFRAME |
                          RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ALLCHILDREN );
            set_visible_style( winpos->hwnd, FALSE );
        }
        else if ((wndPtr->dwStyle & WS_VISIBLE) &&
                 !IsRectEmpty( &oldWindowRect ) && IsRectEmpty( &newWindowRect ))
        {
            /* resizing to zero size -> unmap */
            TRACE( "unmapping zero size win %p\n", winpos->hwnd );
            wine_tsx11_lock();
            XUnmapWindow( display, get_whole_window(wndPtr) );
            wine_tsx11_unlock();
        }

        wine_tsx11_lock();
        if (bChangePos)
            X11DRV_sync_whole_window_position( display, wndPtr, !(winpos->flags & SWP_NOZORDER) );
        else
        {
            struct x11drv_win_data *data = wndPtr->pDriverData;
            data->whole_rect = wndPtr->rectWindow;
            X11DRV_window_to_X_rect( wndPtr, &data->whole_rect );
        }

        if (X11DRV_sync_client_window_position( display, wndPtr ) ||
            (winpos->flags & SWP_FRAMECHANGED))
        {
            /* if we moved the client area, repaint the whole non-client window */
            XClearArea( display, get_whole_window(wndPtr), 0, 0, 0, 0, True );
            winpos->flags |= SWP_FRAMECHANGED;
        }
        if (winpos->flags & SWP_SHOWWINDOW)
        {
            set_visible_style( winpos->hwnd, TRUE );
        }
        else if ((wndPtr->dwStyle & WS_VISIBLE) &&
                 IsRectEmpty( &oldWindowRect ) && !IsRectEmpty( &newWindowRect ))
        {
            /* resizing from zero size to non-zero -> map */
            TRACE( "mapping non zero size win %p\n", winpos->hwnd );
            XMapWindow( display, get_whole_window(wndPtr) );
        }
        XFlush( display );  /* FIXME: should not be necessary */
        wine_tsx11_unlock();
    }
    else  /* no X window, simply toggle the window style */
    {
        if (winpos->flags & SWP_SHOWWINDOW)
            set_visible_style( winpos->hwnd, TRUE );
        else if (winpos->flags & SWP_HIDEWINDOW)
            set_visible_style( winpos->hwnd, FALSE );
    }

    /* manually expose the areas that X won't expose because they are still covered by something */

    if (!(winpos->flags & SWP_SHOWWINDOW))
        expose_covered_parent_area( wndPtr, &oldWindowRect );

    if (wndPtr->dwStyle & WS_VISIBLE)
        expose_covered_window_area( wndPtr, &oldClientRect, winpos->flags & SWP_FRAMECHANGED );

    WIN_ReleaseWndPtr(wndPtr);

    if (wvrFlags & WVR_REDRAW) RedrawWindow( winpos->hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE );

    if( winpos->flags & SWP_HIDEWINDOW )
        HideCaret(winpos->hwnd);
    else if (winpos->flags & SWP_SHOWWINDOW)
        ShowCaret(winpos->hwnd);

    if (!(winpos->flags & SWP_NOACTIVATE))
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
        SendMessageA( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)winpos );
        /* WM_WINDOWPOSCHANGED is send even if SWP_NOSENDCHANGING is set */

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
                    if (!(childPtr = WIN_FindWndPtr( list[i] ))) continue;
                    if ((childPtr->rectWindow.left < x + xspacing) &&
                        (childPtr->rectWindow.right >= x) &&
                        (childPtr->rectWindow.top <= y) &&
                        (childPtr->rectWindow.bottom > y - yspacing))
                    {
                        WIN_ReleaseWndPtr( childPtr );
                        break;  /* There's a window in there */
                    }
                    WIN_ReleaseWndPtr( childPtr );
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

        WIN_SetStyle( hwnd, (wndPtr->dwStyle & ~WS_MAXIMIZE) | WS_MINIMIZE );

        X11DRV_set_iconic_state( wndPtr );

        wpl.ptMinPosition = WINPOS_FindIconPos( wndPtr, wpl.ptMinPosition );

        SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                 GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON) );
        swpFlags |= SWP_NOCOPYBITS;
        break;

    case SW_MAXIMIZE:
        WINPOS_GetMinMaxInfo( hwnd, &size, &wpl.ptMaxPosition, NULL, NULL );

        old_style = WIN_SetStyle( hwnd, (wndPtr->dwStyle & ~WS_MINIMIZE) | WS_MAXIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            WINPOS_ShowIconTitle( hwnd, FALSE );
            X11DRV_set_iconic_state( wndPtr );
        }
        SetRect( rect, wpl.ptMaxPosition.x, wpl.ptMaxPosition.y, size.x, size.y );
        break;

    case SW_RESTORE:
        old_style = WIN_SetStyle( hwnd, wndPtr->dwStyle & ~(WS_MINIMIZE|WS_MAXIMIZE) );
        if (old_style & WS_MINIMIZE)
        {
            WINPOS_ShowIconTitle( hwnd, FALSE );
            X11DRV_set_iconic_state( wndPtr );

            if( wndPtr->flags & WIN_RESTORE_MAX)
            {
                /* Restore to maximized position */
                WINPOS_GetMinMaxInfo( hwnd, &size, &wpl.ptMaxPosition, NULL, NULL);
                WIN_SetStyle( hwnd, wndPtr->dwStyle | WS_MAXIMIZE );
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
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
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


/**********************************************************************
 *		X11DRV_MapNotify
 */
void X11DRV_MapNotify( HWND hwnd, XMapEvent *event )
{
    HWND hwndFocus = GetFocus();
    WND *win;

    if (!(win = WIN_GetPtr( hwnd ))) return;

    if ((win->dwStyle & WS_VISIBLE) &&
        (win->dwStyle & WS_MINIMIZE) &&
        (win->dwExStyle & WS_EX_MANAGED))
    {
        int x, y;
        unsigned int width, height, border, depth;
        Window root, top;
        RECT rect;
        LONG style = (win->dwStyle & ~(WS_MINIMIZE|WS_MAXIMIZE)) | WS_VISIBLE;

        /* FIXME: hack */
        wine_tsx11_lock();
        XGetGeometry( event->display, get_whole_window(win), &root, &x, &y, &width, &height,
                        &border, &depth );
        XTranslateCoordinates( event->display, get_whole_window(win), root, 0, 0, &x, &y, &top );
        wine_tsx11_unlock();
        rect.left   = x;
        rect.top    = y;
        rect.right  = x + width;
        rect.bottom = y + height;
        X11DRV_X_to_window_rect( win, &rect );

        DCE_InvalidateDCE( hwnd, &win->rectWindow );

        if (win->flags & WIN_RESTORE_MAX) style |= WS_MAXIMIZE;
        WIN_SetStyle( hwnd, style );
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
    WND *win;

    if (!(win = WIN_GetPtr( hwnd ))) return;

    if ((win->dwStyle & WS_VISIBLE) && (win->dwExStyle & WS_EX_MANAGED))
    {
        if (win->dwStyle & WS_MAXIMIZE)
            win->flags |= WIN_RESTORE_MAX;
        else
            win->flags &= ~WIN_RESTORE_MAX;

        WIN_SetStyle( hwnd, (win->dwStyle & ~WS_MAXIMIZE) | WS_MINIMIZE );
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
        if (!(GetWindowLongW( list[i], GWL_EXSTYLE ) & WS_EX_MANAGED)) continue;
        if (!(GetWindowLongW( list[i], GWL_STYLE ) & WS_VISIBLE)) continue;
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
                if (!(GetWindowLongW( list[i], GWL_EXSTYLE ) & WS_EX_MANAGED)) continue;
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
 *		X11DRV_ConfigureNotify
 */
void X11DRV_ConfigureNotify( HWND hwnd, XConfigureEvent *event )
{
    HWND oldInsertAfter;
    struct x11drv_win_data *data;
    WND *win;
    RECT rect;
    WINDOWPOS winpos;
    int x = event->x, y = event->y;

    if (!(win = WIN_GetPtr( hwnd ))) return;
    data = win->pDriverData;

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
    X11DRV_X_to_window_rect( win, &rect );
    WIN_ReleasePtr( win );

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
    WND *wndPtr;

    if ((wndPtr = WIN_GetPtr( hwnd )) == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd ))
            FIXME( "not supported on other process window %p\n", hwnd );
        wndPtr = NULL;
    }
    if (!wndPtr)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    if (wndPtr->hrgnWnd == hrgn)
    {
        WIN_ReleasePtr( wndPtr );
        return TRUE;
    }

    if (wndPtr->hrgnWnd)
    {
        /* delete previous region */
        DeleteObject(wndPtr->hrgnWnd);
        wndPtr->hrgnWnd = 0;
    }
    wndPtr->hrgnWnd = hrgn;

#ifdef HAVE_LIBXSHAPE
    {
        Display *display = thread_display();
        X11DRV_WND_DATA *data = wndPtr->pDriverData;

        if (data->whole_window)
        {
            if (!hrgn)
            {
                wine_tsx11_lock();
                XShapeCombineMask( display, data->whole_window,
                                   ShapeBounding, 0, 0, None, ShapeSet );
                wine_tsx11_unlock();
            }
            else
            {
                XRectangle *aXRect;
                int x_offset, y_offset;
                DWORD size;
                DWORD dwBufferSize = GetRegionData(hrgn, 0, NULL);
                PRGNDATA pRegionData = HeapAlloc(GetProcessHeap(), 0, dwBufferSize);
                if (!pRegionData)
                {
                    WIN_ReleasePtr( wndPtr );
                    return TRUE;
                }
                GetRegionData(hrgn, dwBufferSize, pRegionData);
                size = pRegionData->rdh.nCount;
                x_offset = wndPtr->rectWindow.left - data->whole_rect.left;
                y_offset = wndPtr->rectWindow.top - data->whole_rect.top;
                /* convert region's "Windows rectangles" to XRectangles */
                aXRect = HeapAlloc(GetProcessHeap(), 0, size * sizeof(*aXRect) );
                if (aXRect)
                {
                    XRectangle* pCurrRect = aXRect;
                    RECT *pRect = (RECT*) pRegionData->Buffer;
                    for (; pRect < ((RECT*) pRegionData->Buffer) + size ; ++pRect, ++pCurrRect)
                    {
                        pCurrRect->x      = pRect->left + x_offset;
                        pCurrRect->y      = pRect->top + y_offset;
                        pCurrRect->height = pRect->bottom - pRect->top;
                        pCurrRect->width  = pRect->right  - pRect->left;

                        TRACE("Rectangle %04d of %04ld data: X=%04d, Y=%04d, Height=%04d, Width=%04d.\n",
                              pRect - (RECT*) pRegionData->Buffer,
                              size,
                              pCurrRect->x,
                              pCurrRect->y,
                              pCurrRect->height,
                              pCurrRect->width);
                    }

                    /* shape = non-rectangular windows (X11/extensions) */
                    wine_tsx11_lock();
                    XShapeCombineRectangles( display, data->whole_window, ShapeBounding,
                                             0, 0, aXRect,
                                             pCurrRect - aXRect, ShapeSet, YXBanded );
                    wine_tsx11_unlock();
                    HeapFree(GetProcessHeap(), 0, aXRect );
                }
                HeapFree(GetProcessHeap(), 0, pRegionData);
            }
        }
    }
#endif  /* HAVE_LIBXSHAPE */

    WIN_ReleasePtr( wndPtr );
    if (redraw) RedrawWindow( hwnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE );
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
        RECT rect;
        NC_GetInsideRect( hwnd, &rect );
        if (style & WS_SYSMENU)
            rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MINIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MAXIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        pt.x = rectWindow.left + (rect.right - rect.left) / 2;
        pt.y = rectWindow.top + rect.top + GetSystemMetrics(SM_CYSIZE)/2;
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
                hittest = NC_HandleNCHitTest( hwnd, msg.pt );
                if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT))
                    hittest = 0;
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
    NC_HandleSetCursor( hwnd, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
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
    HCURSOR hDragCursor = 0, hOldCursor = 0;
    POINT minTrack, maxTrack;
    POINT capturePoint, pt;
    LONG style = GetWindowLongA( hwnd, GWL_STYLE );
    LONG exstyle = GetWindowLongA( hwnd, GWL_EXSTYLE );
    BOOL    thickframe = HAS_THICKFRAME( style, exstyle );
    BOOL    iconic = style & WS_MINIMIZE;
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = FALSE;
    BOOL grab;
    int iWndsLocks;
    Display *old_gdi_display = NULL;
    Display *display = thread_display();

    SystemParametersInfoA(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);

    pt.x = (short)LOWORD(dwPoint);
    pt.y = (short)HIWORD(dwPoint);
    capturePoint = pt;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd) || (exstyle & WS_EX_MANAGED)) return;

    if ((wParam & 0xfff0) == SC_MOVE)
    {
        if (!hittest) hittest = start_size_move( hwnd, wParam, &capturePoint, style );
        if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
        if (!thickframe) return;
        if ( hittest && ((wParam & 0xfff0) != SC_MOUSEMENU) )
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
        hDragCursor = (HCURSOR)GetClassLongA( hwnd, GCL_HICON);
        if( !hDragCursor ) hDragCursor = (HCURSOR)SendMessageA( hwnd, WM_QUERYDRAGICON, 0, 0L);
        if( !hDragCursor ) iconic = FALSE;
    }

    /* repaint the window before moving it around */
    RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    SendMessageA( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
    set_movesize_capture( hwnd );

    /* grab the server only when moving top-level windows without desktop */
    grab = (!DragFullWindows && !parent && (root_window == DefaultRootWindow(gdi_display)));

    wine_tsx11_lock();
    if (grab)
    {
        XSync( gdi_display, False );
        XGrabServer( display );
        XSync( display, False );
        /* switch gdi display to the thread display, since the server is grabbed */
        old_gdi_display = gdi_display;
        gdi_display = display;
    }
    XGrabPointer( display, X11DRV_get_whole_window(hwnd), False,
                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                  GrabModeAsync, GrabModeAsync,
                  parent ? X11DRV_get_client_window(parent) : root_window,
                  None, CurrentTime );
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
                    else {
                        /* To avoid any deadlocks, all the locks on the windows
			   structures must be suspended before the SetWindowPos */
                        iWndsLocks = WIN_SuspendWndsLock();
                        SetWindowPos( hwnd, 0, newRect.left, newRect.top,
                                      newRect.right - newRect.left,
                                      newRect.bottom - newRect.top,
                                      ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
                        WIN_RestoreWndsLock(iWndsLocks);
                    }
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
        /* To avoid any deadlocks, all the locks on the windows
	   structures must be suspended before the SetWindowPos */
        iWndsLocks = WIN_SuspendWndsLock();

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

        WIN_RestoreWndsLock(iWndsLocks);
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


/***********************************************************************
 *		ForceWindowRaise   (X11DRV.@)
 *
 * Raise a window on top of the X stacking order, while preserving
 * the correct Windows Z order.
 *
 * FIXME: this should go away.
 */
void X11DRV_ForceWindowRaise( HWND hwnd )
{
    int i = 0;
    HWND *list;
    XWindowChanges winChanges;
    Display *display = thread_display();
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return;

    if ((wndPtr->dwExStyle & WS_EX_MANAGED) ||
        wndPtr->parent != GetDesktopWindow() ||
        IsRectEmpty( &wndPtr->rectWindow ) ||
        !get_whole_window(wndPtr))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return;
    }
    WIN_ReleaseWndPtr( wndPtr );

    /* Raise all windows up to wndPtr according to their Z order.
     * (it would be easier with sibling-related Below but it doesn't
     * work very well with SGI mwm for instance)
     */
    winChanges.stack_mode = Above;
    if (!(list = WIN_ListChildren( GetDesktopWindow() ))) return;
    while (list[i] && list[i] != hwnd) i++;
    if (list[i])
    {
        for ( ; i >= 0; i--)
        {
            WND *ptr = WIN_FindWndPtr( list[i] );
            if (!ptr) continue;
            if (!IsRectEmpty( &ptr->rectWindow ) && get_whole_window(ptr))
            {
                wine_tsx11_lock();
                XReconfigureWMWindow( display, get_whole_window(ptr), 0, CWStackMode, &winChanges );
                wine_tsx11_unlock();
            }
            WIN_ReleaseWndPtr( ptr );
        }
    }
    HeapFree( GetProcessHeap(), 0, list );
}

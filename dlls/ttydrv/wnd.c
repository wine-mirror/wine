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

#include "gdi.h"
#include "ttydrv.h"
#include "win.h"
#include "winpos.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

#define SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

/**********************************************************************
 *		CreateWindow   (TTYDRV.@)
 */
BOOL TTYDRV_CreateWindow( HWND hwnd, CREATESTRUCTA *cs, BOOL unicode )
{
    BOOL ret;
    HWND hwndLinkAfter;
    CBT_CREATEWNDA cbtc;
    WND *wndPtr = WIN_GetPtr( hwnd );

    TRACE("(%p)\n", hwnd);

    if (!wndPtr->parent)  /* desktop window */
    {
        wndPtr->pDriverData = root_window;
        WIN_ReleasePtr( wndPtr );
        return TRUE;
    }

#ifdef WINE_CURSES
    /* Only create top-level windows */
    if (!(wndPtr->dwStyle & WS_CHILD))
    {
        WINDOW *window;
        const INT cellWidth=8, cellHeight=8; /* FIXME: Hardcoded */

        int x = wndPtr->rectWindow.left;
        int y = wndPtr->rectWindow.top;
        int cx = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
        int cy = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

        window = subwin( root_window, cy/cellHeight, cx/cellWidth,
                         y/cellHeight, x/cellWidth);
        werase(window);
        wrefresh(window);
        wndPtr->pDriverData = window;
    }
#else /* defined(WINE_CURSES) */
    FIXME("(%p): stub\n", hwnd);
#endif /* defined(WINE_CURSES) */
    WIN_ReleasePtr( wndPtr );

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
    return ret;
}

/***********************************************************************
 *		DestroyWindow   (TTYDRV.@)
 */
BOOL TTYDRV_DestroyWindow( HWND hwnd )
{
#ifdef WINE_CURSES
    WND *wndPtr = WIN_GetPtr( hwnd );
    WINDOW *window = wndPtr->pDriverData;

    TRACE("(%p)\n", hwnd);

    if (window && window != root_window) delwin(window);
    wndPtr->pDriverData = NULL;
    WIN_ReleasePtr( wndPtr );
#else /* defined(WINE_CURSES) */
    FIXME("(%p): stub\n", hwnd);
#endif /* defined(WINE_CURSES) */
    return TRUE;
}


/***********************************************************************
 *           DCE_GetVisRect
 *
 * Calculate the visible rectangle of a window (i.e. the client or
 * window area clipped by the client area of all ancestors) in the
 * corresponding coordinates. Return FALSE if the visible region is empty.
 */
static BOOL DCE_GetVisRect( WND *wndPtr, BOOL clientArea, RECT *lprect )
{
    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
        INT xoffset = lprect->left;
        INT yoffset = lprect->top;

        while ((wndPtr = WIN_FindWndPtr( GetAncestor(wndPtr->hwndSelf,GA_PARENT) )))
        {
            if ( (wndPtr->dwStyle & (WS_ICONIC | WS_VISIBLE)) != WS_VISIBLE )
            {
                WIN_ReleaseWndPtr(wndPtr);
                goto fail;
            }

            xoffset += wndPtr->rectClient.left;
            yoffset += wndPtr->rectClient.top;
            OffsetRect( lprect, wndPtr->rectClient.left,
                        wndPtr->rectClient.top );

            if( (wndPtr->rectClient.left >= wndPtr->rectClient.right) ||
                (wndPtr->rectClient.top >= wndPtr->rectClient.bottom) ||
                (lprect->left >= wndPtr->rectClient.right) ||
                (lprect->right <= wndPtr->rectClient.left) ||
                (lprect->top >= wndPtr->rectClient.bottom) ||
                (lprect->bottom <= wndPtr->rectClient.top) )
            {
                WIN_ReleaseWndPtr(wndPtr);
                goto fail;
            }

            lprect->left = max( lprect->left, wndPtr->rectClient.left );
            lprect->right = min( lprect->right, wndPtr->rectClient.right );
            lprect->top = max( lprect->top, wndPtr->rectClient.top );
            lprect->bottom = min( lprect->bottom, wndPtr->rectClient.bottom );

            WIN_ReleaseWndPtr(wndPtr);
        }
        OffsetRect( lprect, -xoffset, -yoffset );
        return TRUE;
    }

fail:
    SetRectEmpty( lprect );
    return FALSE;
}


/***********************************************************************
 *           DCE_AddClipRects
 *
 * Go through the linked list of windows from pWndStart to pWndEnd,
 * adding to the clip region the intersection of the target rectangle
 * with an offset window rectangle.
 */
static void DCE_AddClipRects( HWND parent, HWND end, HRGN hrgnClip, LPRECT lpRect, int x, int y )
{
    RECT rect;
    WND *pWnd;
    int i;
    HWND *list = WIN_ListChildren( parent );
    HRGN hrgn = 0;

    if (!list) return;
    for (i = 0; list[i]; i++)
    {
        if (list[i] == end) break;
        if (!(pWnd = WIN_FindWndPtr( list[i] ))) continue;
        if (pWnd->dwStyle & WS_VISIBLE)
        {
            rect.left = pWnd->rectWindow.left + x;
            rect.top = pWnd->rectWindow.top + y;
            rect.right = pWnd->rectWindow.right + x;
            rect.bottom = pWnd->rectWindow.bottom + y;
            if( IntersectRect( &rect, &rect, lpRect ))
            {
                if (!hrgn) hrgn = CreateRectRgnIndirect( &rect );
                else SetRectRgn( hrgn, rect.left, rect.top, rect.right, rect.bottom );
                CombineRgn( hrgnClip, hrgnClip, hrgn, RGN_OR );
            }
        }
        WIN_ReleaseWndPtr( pWnd );
    }
    if (hrgn) DeleteObject( hrgn );
    HeapFree( GetProcessHeap(), 0, list );
}


/***********************************************************************
 *           DCE_GetVisRgn
 *
 * Return the visible region of a window, i.e. the client or window area
 * clipped by the client area of all ancestors, and then optionally
 * by siblings and children.
 */
static HRGN DCE_GetVisRgn( HWND hwnd, WORD flags, HWND hwndChild, WORD cflags )
{
    HRGN hrgnVis = 0;
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WND *childWnd = WIN_FindWndPtr( hwndChild );

    /* Get visible rectangle and create a region with it. */

    if (wndPtr && DCE_GetVisRect(wndPtr, !(flags & DCX_WINDOW), &rect))
    {
        if((hrgnVis = CreateRectRgnIndirect( &rect )))
        {
            HRGN hrgnClip = CreateRectRgn( 0, 0, 0, 0 );
            INT xoffset, yoffset;

            if( hrgnClip )
            {
                /* Compute obscured region for the visible rectangle by
		 * clipping children, siblings, and ancestors. Note that
		 * DCE_GetVisRect() returns a rectangle either in client
		 * or in window coordinates (for DCX_WINDOW request). */

                if (flags & DCX_CLIPCHILDREN)
                {
                    if( flags & DCX_WINDOW )
                    {
                        /* adjust offsets since child window rectangles are
			 * in client coordinates */

                        xoffset = wndPtr->rectClient.left - wndPtr->rectWindow.left;
                        yoffset = wndPtr->rectClient.top - wndPtr->rectWindow.top;
                    }
                    else
                        xoffset = yoffset = 0;

                    DCE_AddClipRects( wndPtr->hwndSelf, 0, hrgnClip, &rect, xoffset, yoffset );
                }

                /* We may need to clip children of child window, if a window with PARENTDC
		 * class style and CLIPCHILDREN window style (like in Free Agent 16
		 * preference dialogs) gets here, we take the region for the parent window
		 * but apparently still need to clip the children of the child window... */

                if( (cflags & DCX_CLIPCHILDREN) && childWnd)
                {
                    if( flags & DCX_WINDOW )
                    {
                        /* adjust offsets since child window rectangles are
			 * in client coordinates */

                        xoffset = wndPtr->rectClient.left - wndPtr->rectWindow.left;
                        yoffset = wndPtr->rectClient.top - wndPtr->rectWindow.top;
                    }
                    else
                        xoffset = yoffset = 0;

                    /* client coordinates of child window */
                    xoffset += childWnd->rectClient.left;
                    yoffset += childWnd->rectClient.top;

                    DCE_AddClipRects( childWnd->hwndSelf, 0, hrgnClip,
                                      &rect, xoffset, yoffset );
                }

                /* sibling window rectangles are in client
		 * coordinates of the parent window */

                if (flags & DCX_WINDOW)
                {
                    xoffset = -wndPtr->rectWindow.left;
                    yoffset = -wndPtr->rectWindow.top;
                }
                else
                {
                    xoffset = -wndPtr->rectClient.left;
                    yoffset = -wndPtr->rectClient.top;
                }

                if (flags & DCX_CLIPSIBLINGS && wndPtr->parent )
                    DCE_AddClipRects( wndPtr->parent, wndPtr->hwndSelf,
                                      hrgnClip, &rect, xoffset, yoffset );

                /* Clip siblings of all ancestors that have the
                 * WS_CLIPSIBLINGS style
		 */

                while (wndPtr->parent)
                {
                    WND *ptr = WIN_FindWndPtr( wndPtr->parent );
                    WIN_ReleaseWndPtr( wndPtr );
                    wndPtr = ptr;
                    xoffset -= wndPtr->rectClient.left;
                    yoffset -= wndPtr->rectClient.top;
                    if(wndPtr->dwStyle & WS_CLIPSIBLINGS && wndPtr->parent)
                    {
                        DCE_AddClipRects( wndPtr->parent, wndPtr->hwndSelf,
                                          hrgnClip, &rect, xoffset, yoffset );
                    }
                }

                /* Now once we've got a jumbo clip region we have
		 * to substract it from the visible rectangle.
	         */
                CombineRgn( hrgnVis, hrgnVis, hrgnClip, RGN_DIFF );
                DeleteObject( hrgnClip );
            }
            else
            {
                DeleteObject( hrgnVis );
                hrgnVis = 0;
            }
        }
    }
    else
        hrgnVis = CreateRectRgn(0, 0, 0, 0); /* empty */
    WIN_ReleaseWndPtr(wndPtr);
    WIN_ReleaseWndPtr(childWnd);
    return hrgnVis;
}


/***********************************************************************
 *		GetDC   (TTYDRV.@)
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
BOOL TTYDRV_GetDC( HWND hwnd, HDC hdc, HRGN hrgn, DWORD flags )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    HRGN hrgnVisible = 0;
    POINT org;

    if (!wndPtr) return FALSE;

    if(flags & DCX_WINDOW)
    {
        org.x = wndPtr->rectWindow.left;
        org.y = wndPtr->rectWindow.top;
    }
    else
    {
        org.x = wndPtr->rectClient.left;
        org.y = wndPtr->rectClient.top;
    }

    SetDCOrg16( HDC_16(hdc), org.x, org.y );

    if (SetHookFlags16( HDC_16(hdc), DCHF_VALIDATEVISRGN ))  /* DC was dirty */
    {
        if (flags & DCX_PARENTCLIP)
        {
            WND *parentPtr = WIN_FindWndPtr( wndPtr->parent );

            if( wndPtr->dwStyle & WS_VISIBLE && !(parentPtr->dwStyle & WS_MINIMIZE) )
            {
                DWORD dcxFlags;

                if( parentPtr->dwStyle & WS_CLIPSIBLINGS )
                    dcxFlags = DCX_CLIPSIBLINGS | (flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
                else
                    dcxFlags = flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);

                hrgnVisible = DCE_GetVisRgn( parentPtr->hwndSelf, dcxFlags,
                                             wndPtr->hwndSelf, flags );
                if( flags & DCX_WINDOW )
                    OffsetRgn( hrgnVisible, -wndPtr->rectWindow.left,
                               -wndPtr->rectWindow.top );
                else
                    OffsetRgn( hrgnVisible, -wndPtr->rectClient.left,
                               -wndPtr->rectClient.top );
            }
            else
                hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );
            WIN_ReleaseWndPtr(parentPtr);
        }
        else
        {
            hrgnVisible = DCE_GetVisRgn( hwnd, flags, 0, 0 );
            OffsetRgn( hrgnVisible, org.x, org.y );
        }
        SelectVisRgn16( HDC_16(hdc), HRGN_16(hrgnVisible) );
    }

    /* apply additional region operation (if any) */

    if( flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) )
    {
        if( !hrgnVisible ) hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );

        TRACE("\tsaved VisRgn, clipRgn = %p\n", hrgn);

        SaveVisRgn16( HDC_16(hdc) );
        CombineRgn( hrgnVisible, hrgn, 0, RGN_COPY );
        OffsetRgn( hrgnVisible, org.x, org.y );
        CombineRgn( hrgnVisible, HRGN_32(InquireVisRgn16(HDC_16(hdc))), hrgnVisible,
                      (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
        SelectVisRgn16(HDC_16(hdc), HRGN_16(hrgnVisible));
    }

    if (hrgnVisible) DeleteObject( hrgnVisible );

    WIN_ReleaseWndPtr( wndPtr );
    return TRUE;
}


/***********************************************************************
 *		SetWindowPos   (TTYDRV.@)
 */
BOOL TTYDRV_SetWindowPos( WINDOWPOS *winpos )
{
    WND *wndPtr;
    RECT newWindowRect, newClientRect;
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
        WND* wnd = WIN_FindWndPtr(winpos->hwndInsertAfter);

        if( wnd ) {
            if( wnd->parent != wndPtr->parent )
            {
                retvalue = FALSE;
                WIN_ReleaseWndPtr(wnd);
                goto END;
            }
            /* don't need to change the Zorder of hwnd if it's already inserted
             * after hwndInsertAfter or when inserting hwnd after itself.
             */
            if ((winpos->hwnd == winpos->hwndInsertAfter) ||
                (winpos->hwnd == GetWindow( winpos->hwndInsertAfter, GW_HWNDNEXT )))
                winpos->flags |= SWP_NOZORDER;
        }
        WIN_ReleaseWndPtr(wnd);
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

        SendMessageW( winpos->hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params );

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

    if(!(winpos->flags & SWP_NOZORDER) && winpos->hwnd != winpos->hwndInsertAfter)
    {
        HWND parent = GetAncestor( winpos->hwnd, GA_PARENT );
        if (parent) WIN_LinkWindow( winpos->hwnd, parent, winpos->hwndInsertAfter );
    }

    /* FIXME: actually do something with WVR_VALIDRECTS */

    WIN_SetRectangles( winpos->hwnd, &newWindowRect, &newClientRect );

    if( winpos->flags & SWP_SHOWWINDOW )
        WIN_SetStyle( winpos->hwnd, wndPtr->dwStyle | WS_VISIBLE );
    else if( winpos->flags & SWP_HIDEWINDOW )
        WIN_SetStyle( winpos->hwnd, wndPtr->dwStyle & ~WS_VISIBLE );

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

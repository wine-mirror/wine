/*
 * TTY window driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 */

#include "config.h"

#include "gdi.h"
#include "ttydrv.h"
#include "region.h"
#include "win.h"
#include "winpos.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

WND_DRIVER TTYDRV_WND_Driver =
{
  TTYDRV_WND_ForceWindowRaise,
  TTYDRV_WND_PreSizeMove,
  TTYDRV_WND_PostSizeMove,
  TTYDRV_WND_ScrollWindow,
  TTYDRV_WND_SetHostAttr
};

#define SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

/**********************************************************************
 *		CreateWindow   (TTYDRV.@)
 */
BOOL TTYDRV_CreateWindow( HWND hwnd )
{
#ifdef WINE_CURSES
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WINDOW *window;
    INT cellWidth=8, cellHeight=8; /* FIXME: Hardcoded */

    TRACE("(%x)\n", hwnd);

    /* Only create top-level windows */
    if (wndPtr->dwStyle & WS_CHILD)
    {
        WIN_ReleaseWndPtr( wndPtr );
        return TRUE;
    }

    if (!wndPtr->parent)  /* desktop */
        window = root_window;
    else
    {
        int x = wndPtr->rectWindow.left;
        int y = wndPtr->rectWindow.top;
        int cx = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
        int cy = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

        window = subwin( root_window, cy/cellHeight, cx/cellWidth,
                         y/cellHeight, x/cellWidth);
        werase(window);
        wrefresh(window);
    }
    wndPtr->pDriverData = window;
    WIN_ReleaseWndPtr( wndPtr );
#else /* defined(WINE_CURSES) */
    FIXME("(%x): stub\n", hwnd);
#endif /* defined(WINE_CURSES) */
    return TRUE;
}

/***********************************************************************
 *		DestroyWindow   (TTYDRV.@)
 */
BOOL TTYDRV_DestroyWindow( HWND hwnd )
{
#ifdef WINE_CURSES
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WINDOW *window = wndPtr->pDriverData;

    TRACE("(%x)\n", hwnd);

    if (window && window != root_window) delwin(window);
    wndPtr->pDriverData = NULL;
    WIN_ReleaseWndPtr( wndPtr );
#else /* defined(WINE_CURSES) */
    FIXME("(%x): stub\n", hwnd);
#endif /* defined(WINE_CURSES) */
    return TRUE;
}

/***********************************************************************
 *		TTYDRV_WND_ForceWindowRaise
 */
void TTYDRV_WND_ForceWindowRaise(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/*****************************************************************
 *		TTYDRV_WND_PreSizeMove
 */
void TTYDRV_WND_PreSizeMove(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/*****************************************************************
 *		 TTYDRV_WND_PostSizeMove
 */
void TTYDRV_WND_PostSizeMove(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/*****************************************************************
 *		 TTYDRV_WND_ScrollWindow
 */
void TTYDRV_WND_ScrollWindow( WND *wndPtr, HDC hdc, INT dx, INT dy, 
                              const RECT *clipRect, BOOL bUpdate)
{
  FIXME("(%p, %x, %d, %d, %p, %d): stub\n", 
	wndPtr, hdc, dx, dy, clipRect, bUpdate);
}

/***********************************************************************
 *              TTYDRV_WND_SetHostAttr
 */
BOOL TTYDRV_WND_SetHostAttr(WND *wndPtr, INT attr, INT value)
{
  FIXME("(%p): stub\n", wndPtr);

  return TRUE;
}


/***********************************************************************
 *           DCE_OffsetVisRgn
 *
 * Change region from DC-origin relative coordinates to screen coords.
 */

static void DCE_OffsetVisRgn( HDC hDC, HRGN hVisRgn )
{
    DC *dc;
    if (!(dc = DC_GetDCPtr( hDC ))) return;

    OffsetRgn( hVisRgn, dc->DCOrgX, dc->DCOrgY );

    GDI_ReleaseObj( hDC );
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

        while ((wndPtr = WIN_LockWndPtr(wndPtr->parent)))
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
static BOOL DCE_AddClipRects( WND *pWndStart, WND *pWndEnd,
                              HRGN hrgnClip, LPRECT lpRect, int x, int y )
{
    RECT rect;

    for (WIN_LockWndPtr(pWndStart); (pWndStart && (pWndStart != pWndEnd)); WIN_UpdateWndPtr(&pWndStart,pWndStart->next))
    {
        if( !(pWndStart->dwStyle & WS_VISIBLE) ) continue;

        rect.left = pWndStart->rectWindow.left + x;
        rect.top = pWndStart->rectWindow.top + y;
        rect.right = pWndStart->rectWindow.right + x;
        rect.bottom = pWndStart->rectWindow.bottom + y;

        if( IntersectRect( &rect, &rect, lpRect ))
        {
            if(!REGION_UnionRectWithRgn( hrgnClip, &rect )) break;
        }
    }
    WIN_ReleaseWndPtr(pWndStart);
    return (pWndStart == pWndEnd);
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

                if( (flags & DCX_CLIPCHILDREN) && wndPtr->child )
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

                    DCE_AddClipRects( wndPtr->child, NULL, hrgnClip, &rect, xoffset, yoffset );
                }

                /* We may need to clip children of child window, if a window with PARENTDC
		 * class style and CLIPCHILDREN window style (like in Free Agent 16
		 * preference dialogs) gets here, we take the region for the parent window
		 * but apparently still need to clip the children of the child window... */

                if( (cflags & DCX_CLIPCHILDREN) && childWnd && childWnd->child )
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

                    DCE_AddClipRects( childWnd->child, NULL, hrgnClip,
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
                    DCE_AddClipRects( wndPtr->parent->child,
                                      wndPtr, hrgnClip, &rect, xoffset, yoffset );

                /* Clip siblings of all ancestors that have the
                 * WS_CLIPSIBLINGS style
		 */

                while (wndPtr->parent)
                {
                    WIN_UpdateWndPtr(&wndPtr,wndPtr->parent);
                    xoffset -= wndPtr->rectClient.left;
                    yoffset -= wndPtr->rectClient.top;
                    if(wndPtr->dwStyle & WS_CLIPSIBLINGS && wndPtr->parent)
                    {
                        DCE_AddClipRects( wndPtr->parent->child, wndPtr,
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
    DC *dc;
    BOOL updateVisRgn;
    HRGN hrgnVisible = 0;

    if (!wndPtr) return FALSE;

    if (!(dc = DC_GetDCPtr( hdc )))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return FALSE;
    }

    if(flags & DCX_WINDOW)
    {
        dc->DCOrgX = wndPtr->rectWindow.left;
        dc->DCOrgY = wndPtr->rectWindow.top;
    }
    else
    {
        dc->DCOrgX = wndPtr->rectClient.left;
        dc->DCOrgY = wndPtr->rectClient.top;
    }
    updateVisRgn = (dc->flags & DC_DIRTY) != 0;
    GDI_ReleaseObj( hdc );

    if (updateVisRgn)
    {
        if (flags & DCX_PARENTCLIP)
        {
            WND *parentPtr = WIN_LockWndPtr(wndPtr->parent);

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
                DCE_OffsetVisRgn( hdc, hrgnVisible );
            }
            else
                hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );
            WIN_ReleaseWndPtr(parentPtr);
        }
        else
        {
            hrgnVisible = DCE_GetVisRgn( hwnd, flags, 0, 0 );
            DCE_OffsetVisRgn( hdc, hrgnVisible );
        }
        SelectVisRgn16( hdc, hrgnVisible );
    }

    /* apply additional region operation (if any) */

    if( flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) )
    {
        if( !hrgnVisible ) hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );

        TRACE("\tsaved VisRgn, clipRgn = %04x\n", hrgn);

        SaveVisRgn16( hdc );
        CombineRgn( hrgnVisible, hrgn, 0, RGN_COPY );
        DCE_OffsetVisRgn( hdc, hrgnVisible );
        CombineRgn( hrgnVisible, InquireVisRgn16( hdc ), hrgnVisible,
                      (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
        SelectVisRgn16( hdc, hrgnVisible );
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

    TRACE( "hwnd %04x, swp (%i,%i)-(%i,%i) flags %08x\n",
           winpos->hwnd, winpos->x, winpos->y,
           winpos->x + winpos->cx, winpos->y + winpos->cy, winpos->flags);

    /* ------------------------------------------------------------------------ CHECKS */

      /* Check window handle */

    if (winpos->hwnd == GetDesktopWindow()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( winpos->hwnd ))) return FALSE;

    TRACE("\tcurrent (%i,%i)-(%i,%i), style %08x\n",
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
            if(( wnd->next == wndPtr ) || (winpos->hwnd == winpos->hwndInsertAfter))
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
        winpos->flags |= ( wndPtr->parent->child == wndPtr)? SWP_NOZORDER: 0;
    else
        if( winpos->hwndInsertAfter == HWND_BOTTOM )
            winpos->flags |= ( wndPtr->next )? 0: SWP_NOZORDER;
        else
            if( !(winpos->flags & SWP_NOZORDER) )
                if( GetWindow(winpos->hwndInsertAfter, GW_HWNDNEXT) == wndPtr->hwndSelf )
                    winpos->flags |= SWP_NOZORDER;

    /* Common operations */

      /* Send WM_NCCALCSIZE message to get new client area */
    if( (winpos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
         WINPOS_SendNCCalcSize( winpos->hwnd, TRUE, &newWindowRect,
                                &wndPtr->rectWindow, &wndPtr->rectClient,
                                winpos, &newClientRect );
    }

    if(!(winpos->flags & SWP_NOZORDER) && winpos->hwnd != winpos->hwndInsertAfter)
    {
        if ( WIN_UnlinkWindow( winpos->hwnd ) )
            WIN_LinkWindow( winpos->hwnd, winpos->hwndInsertAfter );
    }

    /* FIXME: actually do something with WVR_VALIDRECTS */

    wndPtr->rectWindow = newWindowRect;
    wndPtr->rectClient = newClientRect;

    if( winpos->flags & SWP_SHOWWINDOW )
    {
        wndPtr->dwStyle |= WS_VISIBLE;
    }
    else if( winpos->flags & SWP_HIDEWINDOW )
    {
        wndPtr->dwStyle &= ~WS_VISIBLE;
    }

    /* ------------------------------------------------------------------------ FINAL */

    /* repaint invalidated region (if any)
     *
     * FIXME: if SWP_NOACTIVATE is not set then set invalid regions here without any painting
     *        and force update after ChangeActiveWindow() to avoid painting frames twice.
     */

    if( !(winpos->flags & SWP_NOREDRAW) )
    {
        RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0,
                      RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN );
        if (wndPtr->parent->hwndSelf == GetDesktopWindow() ||
            wndPtr->parent->parent->hwndSelf == GetDesktopWindow())
        {
            RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0,
                          RDW_ERASENOW | RDW_NOCHILDREN );
        }
    }

    if (!(winpos->flags & SWP_NOACTIVATE))
        WINPOS_ChangeActiveWindow( winpos->hwnd, FALSE );

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

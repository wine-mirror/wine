/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995, 2001 Alexandre Julliard
 * Copyright 1995, 1996, 1999 Alex Korobka
 */

#include "config.h"

#include "ts_xlib.h"
#include "ts_shape.h"

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "x11drv.h"
#include "hook.h"
#include "win.h"
#include "winpos.h"
#include "region.h"
#include "dce.h"
#include "cursoricon.h"
#include "nonclient.h"
#include "message.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win);


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

        while( !(wndPtr->flags & WIN_NATIVE) &&
               ( wndPtr = WIN_LockWndPtr(wndPtr->parent)) )
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

    if (X11DRV_WND_GetXWindow(pWndStart)) return TRUE; /* X will do the clipping */

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
 *		GetDC   (X11DRV.@)
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
BOOL X11DRV_GetDC( HWND hwnd, HDC hdc, HRGN hrgn, DWORD flags )
{
    WND *w, *wndPtr = WIN_FindWndPtr(hwnd);
    DC *dc;
    X11DRV_PDEVICE *physDev;
    INT dcOrgXCopy = 0, dcOrgYCopy = 0;
    BOOL offsetClipRgn = FALSE;
    BOOL updateVisRgn;
    HRGN hrgnVisible = 0;

    if (!wndPtr) return FALSE;

    if (!(dc = DC_GetDCPtr( hdc )))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return FALSE;
    }

    physDev = (X11DRV_PDEVICE *)dc->physDev;

    /*
     * This function change the coordinate system (DCOrgX,DCOrgY)
     * values. When it moves the origin, other data like the current clipping
     * region will not be moved to that new origin. In the case of DCs that are class
     * or window DCs that clipping region might be a valid value from a previous use
     * of the DC and changing the origin of the DC without moving the clip region
     * results in a clip region that is not placed properly in the DC.
     * This code will save the dc origin, let the SetDrawable
     * modify the origin and reset the clipping. When the clipping is set,
     * it is moved according to the new DC origin.
     */
    if ( (wndPtr->clsStyle & (CS_OWNDC | CS_CLASSDC)) && (dc->hClipRgn > 0))
    {
        dcOrgXCopy = dc->DCOrgX;
        dcOrgYCopy = dc->DCOrgY;
        offsetClipRgn = TRUE;
    }

    if (flags & DCX_WINDOW)
    {
        dc->DCOrgX  = wndPtr->rectWindow.left;
        dc->DCOrgY  = wndPtr->rectWindow.top;
    }
    else
    {
        dc->DCOrgX  = wndPtr->rectClient.left;
        dc->DCOrgY  = wndPtr->rectClient.top;
    }

    w = wndPtr;
    while (!X11DRV_WND_GetXWindow(w))
    {
        w = w->parent;
        dc->DCOrgX += w->rectClient.left;
        dc->DCOrgY += w->rectClient.top;
    }
    dc->DCOrgX -= w->rectWindow.left;
    dc->DCOrgY -= w->rectWindow.top;

    /* reset the clip region, according to the new origin */
    if ( offsetClipRgn )
    {
        OffsetRgn(dc->hClipRgn, dc->DCOrgX - dcOrgXCopy,dc->DCOrgY - dcOrgYCopy);
    }

    physDev->drawable = X11DRV_WND_GetXWindow(w);

#if 0
    /* This is needed when we reuse a cached DC because
     * SetDCState() called by ReleaseDC() screws up DC
     * origins for child windows.
     */

    if( bSetClipOrigin )
        TSXSetClipOrigin( display, physDev->gc, dc->DCOrgX, dc->DCOrgY );
#endif

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
            if ((hwnd == GetDesktopWindow()) && (root_window != DefaultRootWindow(thread_display())))
                hrgnVisible = CreateRectRgn( 0, 0, GetSystemMetrics(SM_CXSCREEN),
                                             GetSystemMetrics(SM_CYSCREEN) );
            else
            {
                hrgnVisible = DCE_GetVisRgn( hwnd, flags, 0, 0 );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
            }
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
 *           SWP_DoSimpleFrameChanged
 *
 * NOTE: old and new client rect origins are identical, only
 *	 extents may have changed. Window extents are the same.
 */
static void SWP_DoSimpleFrameChanged( WND* wndPtr, RECT* pOldClientRect,
                                      WORD swpFlags, UINT uFlags )
{
    INT i = 0;
    RECT rect;
    HRGN hrgn = 0;

    if( !(swpFlags & SWP_NOCLIENTSIZE) )
    {
        /* Client rect changed its position/size, most likely a scrollar
	 * was added/removed.
	 *
	 * FIXME: WVR alignment flags
	 */

        if( wndPtr->rectClient.right >  pOldClientRect->right ) /* right edge */
        {
            i++;
            rect.top = 0;
            rect.bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
            rect.right = wndPtr->rectClient.right - wndPtr->rectClient.left;
            if(!(uFlags & SWP_EX_NOCOPY))
                rect.left = pOldClientRect->right - wndPtr->rectClient.left;
            else
            {
                rect.left = 0;
                goto redraw;
            }
        }

        if( wndPtr->rectClient.bottom > pOldClientRect->bottom ) /* bottom edge */
        {
            if( i )
                hrgn = CreateRectRgnIndirect( &rect );
            rect.left = 0;
            rect.right = wndPtr->rectClient.right - wndPtr->rectClient.left;
            rect.bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
            if(!(uFlags & SWP_EX_NOCOPY))
                rect.top = pOldClientRect->bottom - wndPtr->rectClient.top;
            else
                rect.top = 0;
            if( i++ )
                REGION_UnionRectWithRgn( hrgn, &rect );
        }

        if( i == 0 && (uFlags & SWP_EX_NOCOPY) ) /* force redraw anyway */
        {
            rect = wndPtr->rectWindow;
            OffsetRect( &rect, wndPtr->rectWindow.left - wndPtr->rectClient.left,
                        wndPtr->rectWindow.top - wndPtr->rectClient.top );
            i++;
        }
    }

    if( i )
    {
    redraw:
        PAINT_RedrawWindow( wndPtr->hwndSelf, &rect, hrgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE |
                            RDW_ERASENOW | RDW_ALLCHILDREN, RDW_EX_TOPFRAME | RDW_EX_USEHRGN );
    }
    else
    {
        WIN_UpdateNCRgn(wndPtr, 0, UNC_UPDATE | UNC_ENTIRE);
    }

    if( hrgn > 1 )
        DeleteObject( hrgn );
}

/***********************************************************************
 *           SWP_DoWinPosChanging
 */
static BOOL SWP_DoWinPosChanging( WND* wndPtr, WINDOWPOS* pWinpos,
                                  RECT* pNewWindowRect, RECT* pNewClientRect )
{
      /* Send WM_WINDOWPOSCHANGING message */

    if (!(pWinpos->flags & SWP_NOSENDCHANGING))
        SendMessageA( wndPtr->hwndSelf, WM_WINDOWPOSCHANGING, 0, (LPARAM)pWinpos );

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
    return TRUE;
}

/***********************************************************************
 *           SWP_DoNCCalcSize
 */
static UINT SWP_DoNCCalcSize( WND* wndPtr, WINDOWPOS* pWinpos,
                              RECT* pNewWindowRect, RECT* pNewClientRect, WORD f)
{
    UINT wvrFlags = 0;

      /* Send WM_NCCALCSIZE message to get new client area */
    if( (pWinpos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
         wvrFlags = WINPOS_SendNCCalcSize( pWinpos->hwnd, TRUE, pNewWindowRect,
                                    &wndPtr->rectWindow, &wndPtr->rectClient,
                                    pWinpos, pNewClientRect );

         /* FIXME: WVR_ALIGNxxx */

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
      if( !(f & SWP_NOMOVE) && (pNewClientRect->left != wndPtr->rectClient.left ||
                                pNewClientRect->top != wndPtr->rectClient.top) )
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;
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
static HWND SWP_DoOwnedPopups(WND* pDesktop, WND* wndPtr, HWND hwndInsertAfter, WORD flags)
{
    WND *w = WIN_LockWndPtr(pDesktop->child);

    WARN("(%04x) hInsertAfter = %04x\n", wndPtr->hwndSelf, hwndInsertAfter );

    if( (wndPtr->dwStyle & WS_POPUP) && wndPtr->owner )
    {
        /* make sure this popup stays above the owner */

        HWND hwndLocalPrev = HWND_TOP;

        if( hwndInsertAfter != HWND_TOP )
        {
            while( w && w != wndPtr->owner )
            {
                if (w != wndPtr) hwndLocalPrev = w->hwndSelf;
                if( hwndLocalPrev == hwndInsertAfter ) break;
                WIN_UpdateWndPtr(&w,w->next);
            }
            hwndInsertAfter = hwndLocalPrev;
        }
    }
    else if( wndPtr->dwStyle & WS_CHILD )
        goto END;

    WIN_UpdateWndPtr(&w, pDesktop->child);

    while( w )
    {
        if( w == wndPtr ) break;

        if( (w->dwStyle & WS_POPUP) && w->owner == wndPtr )
        {
            SetWindowPos(w->hwndSelf, hwndInsertAfter, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_DEFERERASE);
            hwndInsertAfter = w->hwndSelf;
        }
        WIN_UpdateWndPtr(&w, w->next);
    }

END:
    WIN_ReleaseWndPtr(w);
    return hwndInsertAfter;
}

/***********************************************************************
 *	     SWP_CopyValidBits
 *
 * Make window look nice without excessive repainting
 *
 * visible and update regions are in window coordinates
 * client and window rectangles are in parent client coordinates
 *
 * Returns: uFlags and a dirty region in *pVisRgn.
 */
static UINT SWP_CopyValidBits( WND* Wnd, HRGN* pVisRgn,
                               LPRECT lpOldWndRect,
                               LPRECT lpOldClientRect, UINT uFlags )
{
    RECT r;
    HRGN newVisRgn, dirtyRgn;
    INT  my = COMPLEXREGION;
    DWORD dflags;

    TRACE("\tnew wnd=(%i %i-%i %i) old wnd=(%i %i-%i %i), %04x\n",
          Wnd->rectWindow.left, Wnd->rectWindow.top,
          Wnd->rectWindow.right, Wnd->rectWindow.bottom,
          lpOldWndRect->left, lpOldWndRect->top,
          lpOldWndRect->right, lpOldWndRect->bottom, *pVisRgn);
    TRACE("\tnew client=(%i %i-%i %i) old client=(%i %i-%i %i)\n",
          Wnd->rectClient.left, Wnd->rectClient.top,
          Wnd->rectClient.right, Wnd->rectClient.bottom,
          lpOldClientRect->left, lpOldClientRect->top,
          lpOldClientRect->right,lpOldClientRect->bottom );

    if( Wnd->hrgnUpdate == 1 )
        uFlags |= SWP_EX_NOCOPY; /* whole window is invalid, nothing to copy */

    dflags = DCX_WINDOW;
    if(Wnd->dwStyle & WS_CLIPSIBLINGS)
        dflags |= DCX_CLIPSIBLINGS;
    newVisRgn = DCE_GetVisRgn( Wnd->hwndSelf, dflags, 0, 0);

    dirtyRgn = CreateRectRgn( 0, 0, 0, 0 );

    if( !(uFlags & SWP_EX_NOCOPY) ) /* make sure dst region covers only valid bits */
        my = CombineRgn( dirtyRgn, newVisRgn, *pVisRgn, RGN_AND );

    if( (my == NULLREGION) || (uFlags & SWP_EX_NOCOPY) )
    {
    nocopy:

        TRACE("\twon't copy anything!\n");

        /* set dirtyRgn to the sum of old and new visible regions
         * in parent client coordinates */

        OffsetRgn( newVisRgn, Wnd->rectWindow.left, Wnd->rectWindow.top );
        OffsetRgn( *pVisRgn, lpOldWndRect->left, lpOldWndRect->top );

        CombineRgn(*pVisRgn, *pVisRgn, newVisRgn, RGN_OR );
    }
    else /* copy valid bits to a new location */
    {
        INT  dx, dy, ow, oh, nw, nh, ocw, ncw, och, nch;
        HRGN hrgnValid = dirtyRgn; /* non-empty intersection of old and new visible rgns */

        /* subtract already invalid region inside Wnd from the dst region */

        if( Wnd->hrgnUpdate )
            if( CombineRgn( hrgnValid, hrgnValid, Wnd->hrgnUpdate, RGN_DIFF) == NULLREGION )
                goto nocopy;

        /* check if entire window can be copied */

        ow = lpOldWndRect->right - lpOldWndRect->left;
        oh = lpOldWndRect->bottom - lpOldWndRect->top;
        nw = Wnd->rectWindow.right - Wnd->rectWindow.left;
        nh = Wnd->rectWindow.bottom - Wnd->rectWindow.top;

        ocw = lpOldClientRect->right - lpOldClientRect->left;
        och = lpOldClientRect->bottom - lpOldClientRect->top;
        ncw = Wnd->rectClient.right  - Wnd->rectClient.left;
        nch = Wnd->rectClient.bottom  - Wnd->rectClient.top;

        if(  (ocw != ncw) || (och != nch) ||
             ( ow !=  nw) || ( oh !=  nh) ||
             ((lpOldClientRect->top - lpOldWndRect->top)   !=
              (Wnd->rectClient.top - Wnd->rectWindow.top)) ||
             ((lpOldClientRect->left - lpOldWndRect->left) !=
              (Wnd->rectClient.left - Wnd->rectWindow.left)) )
        {
            if(uFlags & SWP_EX_PAINTSELF)
            {
                /* movement relative to the window itself */
                dx = (Wnd->rectClient.left - Wnd->rectWindow.left) -
                    (lpOldClientRect->left - lpOldWndRect->left) ;
                dy = (Wnd->rectClient.top - Wnd->rectWindow.top) -
                    (lpOldClientRect->top - lpOldWndRect->top) ;
            }
            else
            {
                /* movement relative to the parent's client area */
                dx = Wnd->rectClient.left - lpOldClientRect->left;
                dy = Wnd->rectClient.top - lpOldClientRect->top;
            }

            /* restrict valid bits to the common client rect */

            r.left = Wnd->rectClient.left - Wnd->rectWindow.left;
            r.top = Wnd->rectClient.top  - Wnd->rectWindow.top;
            r.right = r.left + min( ocw, ncw );
            r.bottom = r.top + min( och, nch );

            REGION_CropRgn( hrgnValid, hrgnValid, &r,
                            (uFlags & SWP_EX_PAINTSELF) ? NULL : (POINT*)&(Wnd->rectWindow));
            GetRgnBox( hrgnValid, &r );
            if( IsRectEmpty( &r ) )
                goto nocopy;
            r = *lpOldClientRect;
        }
        else
        {
            if(uFlags & SWP_EX_PAINTSELF) {
                /*
                 * with SWP_EX_PAINTSELF, the window repaints itself. Since a window can't move
                 * relative to itself, only the client area can change.
                 * if the client rect didn't change, there's nothing to do.
                 */
                dx = 0;
                dy = 0;
            }
            else
            {
                dx = Wnd->rectWindow.left - lpOldWndRect->left;
                dy = Wnd->rectWindow.top -  lpOldWndRect->top;
                OffsetRgn( hrgnValid, Wnd->rectWindow.left, Wnd->rectWindow.top );
            }
            r = *lpOldWndRect;
        }

        if( !(uFlags & SWP_EX_PAINTSELF) )
        {
            /* Move remaining regions to parent coordinates */
            OffsetRgn( newVisRgn, Wnd->rectWindow.left, Wnd->rectWindow.top );
            OffsetRgn( *pVisRgn,  lpOldWndRect->left, lpOldWndRect->top );
        }
        else
            OffsetRect( &r, -lpOldWndRect->left, -lpOldWndRect->top );

        TRACE("\tcomputing dirty region!\n");

        /* Compute combined dirty region (old + new - valid) */
        CombineRgn( *pVisRgn, *pVisRgn, newVisRgn, RGN_OR);
        CombineRgn( *pVisRgn, *pVisRgn, hrgnValid, RGN_DIFF);

        /* Blt valid bits, r is the rect to copy  */

        if( dx || dy )
        {
            RECT rClip;
            HDC hDC;

            /* get DC and clip rect with drawable rect to avoid superfluous expose events
               from copying clipped areas */

            if( uFlags & SWP_EX_PAINTSELF )
            {
                hDC = GetDCEx( Wnd->hwndSelf, hrgnValid, DCX_WINDOW | DCX_CACHE |
                               DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_CLIPSIBLINGS );
                rClip.right = nw; rClip.bottom = nh;
            }
            else
            {
                hDC = GetDCEx( Wnd->parent->hwndSelf, hrgnValid, DCX_CACHE |
                               DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_CLIPSIBLINGS );
                rClip.right = Wnd->parent->rectClient.right - Wnd->parent->rectClient.left;
                rClip.bottom = Wnd->parent->rectClient.bottom - Wnd->parent->rectClient.top;
            }
            rClip.left = rClip.top = 0;

            if( oh > nh ) r.bottom = r.top  + nh;
            if( ow < nw ) r.right = r.left  + nw;

            if( IntersectRect( &r, &r, &rClip ) )
            {
                X11DRV_WND_SurfaceCopy( Wnd->parent, hDC, dx, dy, &r, TRUE );

                /* When you copy the bits without repainting, parent doesn't
                   get validated appropriately. Therefore, we have to validate
                   the parent with the windows' updated region when the
                   parent's update region is not empty. */

                if (Wnd->parent->hrgnUpdate != 0 && !(Wnd->parent->dwStyle & WS_CLIPCHILDREN))
                {
                    OffsetRect(&r, dx, dy);
                    ValidateRect(Wnd->parent->hwndSelf, &r);
                }
            }
            ReleaseDC( (uFlags & SWP_EX_PAINTSELF) ?
                       Wnd->hwndSelf :  Wnd->parent->hwndSelf, hDC);
        }
    }

    /* *pVisRgn now points to the invalidated region */

    DeleteObject(newVisRgn);
    DeleteObject(dirtyRgn);
    return uFlags;
}


/***********************************************************************
 *		SetWindowPos   (X11DRV.@)
 */
BOOL X11DRV_SetWindowPos( WINDOWPOS *winpos )
{
    WND *wndPtr,*wndTemp;
    RECT newWindowRect, newClientRect;
    RECT oldWindowRect, oldClientRect;
    HRGN visRgn = 0;
    UINT wvrFlags = 0, uFlags = 0;
    BOOL retvalue, resync = FALSE, bChangePos;
    HWND hwndActive = GetForegroundWindow();

    TRACE( "hwnd %04x, swp (%i,%i)-(%i,%i) flags %08x\n",
           winpos->hwnd, winpos->x, winpos->y,
           winpos->x + winpos->cx, winpos->y + winpos->cy, winpos->flags);

    bChangePos = !(winpos->flags & SWP_WINE_NOHOSTMOVE);
    winpos->flags &= ~SWP_WINE_NOHOSTMOVE;

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

    SWP_DoWinPosChanging( wndPtr, winpos, &newWindowRect, &newClientRect );

    if((winpos->flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) != SWP_NOZORDER)
    {
        if( wndPtr->parent == WIN_GetDesktop() )
            winpos->hwndInsertAfter = SWP_DoOwnedPopups( wndPtr->parent, wndPtr,
                                                         winpos->hwndInsertAfter, winpos->flags );
        WIN_ReleaseDesktop();
    }

    if(!(wndPtr->flags & WIN_NATIVE) )
    {
        if( winpos->hwndInsertAfter == HWND_TOP )
            winpos->flags |= ( wndPtr->parent->child == wndPtr)? SWP_NOZORDER: 0;
        else
            if( winpos->hwndInsertAfter == HWND_BOTTOM )
                winpos->flags |= ( wndPtr->next )? 0: SWP_NOZORDER;
            else
                if( !(winpos->flags & SWP_NOZORDER) )
                    if( GetWindow(winpos->hwndInsertAfter, GW_HWNDNEXT) == wndPtr->hwndSelf )
                        winpos->flags |= SWP_NOZORDER;

        if( !(winpos->flags & (SWP_NOREDRAW | SWP_SHOWWINDOW)) &&
            ((winpos->flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW | SWP_FRAMECHANGED))
             != (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)) )
        {
            /* get a previous visible region for SWP_CopyValidBits() */
            DWORD dflags = DCX_WINDOW;

            if (wndPtr->dwStyle & WS_CLIPSIBLINGS)
                dflags |= DCX_CLIPSIBLINGS;

            visRgn = DCE_GetVisRgn(winpos->hwnd, dflags, 0, 0);
        }
    }

    /* Common operations */

    wvrFlags = SWP_DoNCCalcSize( wndPtr, winpos, &newWindowRect, &newClientRect, winpos->flags );

    if(!(winpos->flags & SWP_NOZORDER) && winpos->hwnd != winpos->hwndInsertAfter)
    {
        if ( WIN_UnlinkWindow( winpos->hwnd ) )
            WIN_LinkWindow( winpos->hwnd, winpos->hwndInsertAfter );
    }

    /* Reset active DCEs */

    if( (((winpos->flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) &&
         wndPtr->dwStyle & WS_VISIBLE) ||
        (winpos->flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)) )
    {
        RECT rect;

        UnionRect(&rect, &newWindowRect, &wndPtr->rectWindow);
        DCE_InvalidateDCE(wndPtr, &rect);
    }

    oldWindowRect = wndPtr->rectWindow;
    oldClientRect = wndPtr->rectClient;

    /* Find out if we have to redraw the whole client rect */

    if( oldClientRect.bottom - oldClientRect.top ==
        newClientRect.bottom - newClientRect.top ) wvrFlags &= ~WVR_VREDRAW;

    if( oldClientRect.right - oldClientRect.left ==
        newClientRect.right - newClientRect.left ) wvrFlags &= ~WVR_HREDRAW;

    if( (winpos->flags & SWP_NOCOPYBITS) ||
        (!(winpos->flags & SWP_NOCLIENTSIZE) &&
         (wvrFlags >= WVR_HREDRAW) && (wvrFlags < WVR_VALIDRECTS)) )
    {
        uFlags |= SWP_EX_NOCOPY;
    }
/*
 *  Use this later in CopyValidBits()
 *
    else if( 0  )
	uFlags |= SWP_EX_NONCLIENT;
 */

    /* FIXME: actually do something with WVR_VALIDRECTS */

    wndPtr->rectWindow = newWindowRect;
    wndPtr->rectClient = newClientRect;

    if (wndPtr->flags & WIN_NATIVE)  /* -------------------------------------------- hosted window */
    {
        BOOL bCallDriver = TRUE;
        HWND tempInsertAfter = winpos->hwndInsertAfter;

        winpos->hwndInsertAfter = winpos->hwndInsertAfter;

        if( !(winpos->flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOREDRAW)) )
        {
            /* This is the only place where we need to force repainting of the contents
               of windows created by the host window system, all other cases go through the
               expose event handling */

            if( (winpos->flags & (SWP_NOSIZE | SWP_FRAMECHANGED)) == (SWP_NOSIZE | SWP_FRAMECHANGED) )
            {
                winpos->cx = newWindowRect.right - newWindowRect.left;
                winpos->cy = newWindowRect.bottom - newWindowRect.top;

                X11DRV_WND_SetWindowPos(wndPtr, winpos, bChangePos);
                winpos->hwndInsertAfter = tempInsertAfter;
                bCallDriver = FALSE;

                if( winpos->flags & SWP_NOCLIENTMOVE )
                    SWP_DoSimpleFrameChanged(wndPtr, &oldClientRect, winpos->flags, uFlags );
                else
                {
                    /* client area moved but window extents remained the same, copy valid bits */

                    visRgn = CreateRectRgn( 0, 0, winpos->cx, winpos->cy );
                    uFlags = SWP_CopyValidBits( wndPtr, &visRgn, &oldWindowRect, &oldClientRect,
                                                uFlags | SWP_EX_PAINTSELF );
                }
            }
        }

        if( bCallDriver )
        {
            if( !(winpos->flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOREDRAW)) )
            {
                if( (oldClientRect.left - oldWindowRect.left == newClientRect.left - newWindowRect.left) &&
                    (oldClientRect.top - oldWindowRect.top == newClientRect.top - newWindowRect.top) &&
                    !(uFlags & SWP_EX_NOCOPY) )
                {
                    /* The origin of the client rect didn't move so we can try to repaint
                     * only the nonclient area by setting bit gravity hint for the host window system.
                     */

                    if( !(wndPtr->dwExStyle & WS_EX_MANAGED) )
                    {
                        HRGN hrgn = CreateRectRgn( 0, 0, newWindowRect.right - newWindowRect.left,
                                                   newWindowRect.bottom - newWindowRect.top);
                        RECT rcn = newClientRect;
                        RECT rco = oldClientRect;

                        OffsetRect( &rcn, -newWindowRect.left, -newWindowRect.top );
                        OffsetRect( &rco, -oldWindowRect.left, -oldWindowRect.top );
                        IntersectRect( &rcn, &rcn, &rco );
                        visRgn = CreateRectRgnIndirect( &rcn );
                        CombineRgn( visRgn, hrgn, visRgn, RGN_DIFF );
                        DeleteObject( hrgn );
                        uFlags = SWP_EX_PAINTSELF;
                    }
                    X11DRV_WND_SetGravity(wndPtr, NorthWestGravity );
                }
            }

            X11DRV_WND_SetWindowPos(wndPtr, winpos, bChangePos);
            X11DRV_WND_SetGravity(wndPtr, ForgetGravity );
            winpos->hwndInsertAfter = tempInsertAfter;
        }

        if( winpos->flags & SWP_SHOWWINDOW )
        {
            HWND focus, curr;

            wndPtr->dwStyle |= WS_VISIBLE;

            if (wndPtr->dwExStyle & WS_EX_MANAGED) resync = TRUE;

            /* focus was set to unmapped window, reset host focus
             * since the window is now visible */

            focus = curr = GetFocus();
            while (curr)
            {
                if (curr == winpos->hwnd)
                {
                    X11DRV_SetFocus(focus);
                    break;
                }
                curr = GetParent(curr);
            }
        }
    }
    else  /* -------------------------------------------- emulated window */
    {
        if( winpos->flags & SWP_SHOWWINDOW )
        {
            wndPtr->dwStyle |= WS_VISIBLE;
            uFlags |= SWP_EX_PAINTSELF;
            visRgn = 1; /* redraw the whole window */
        }
        else if( !(winpos->flags & SWP_NOREDRAW) )
        {
            if( winpos->flags & SWP_HIDEWINDOW )
            {
                if( visRgn > 1 ) /* map to parent */
                    OffsetRgn( visRgn, oldWindowRect.left, oldWindowRect.top );
                else
                    visRgn = 0;
            }
            else
            {
                if( (winpos->flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE )
                    uFlags = SWP_CopyValidBits(wndPtr, &visRgn, &oldWindowRect,
                                               &oldClientRect, uFlags);
                else
                {
                    /* nothing moved, redraw frame if needed */

                    if( winpos->flags & SWP_FRAMECHANGED )
                        SWP_DoSimpleFrameChanged( wndPtr, &oldClientRect, winpos->flags, uFlags );
                    if( visRgn )
                    {
                        DeleteObject( visRgn );
                        visRgn = 0;
                    }
                }
            }
        }
    }

    if( winpos->flags & SWP_HIDEWINDOW )
    {
        wndPtr->dwStyle &= ~WS_VISIBLE;
    }

    if (winpos->hwnd == CARET_GetHwnd())
    {
        if( winpos->flags & SWP_HIDEWINDOW )
            HideCaret(winpos->hwnd);
        else if (winpos->flags & SWP_SHOWWINDOW)
            ShowCaret(winpos->hwnd);
    }

    /* ------------------------------------------------------------------------ FINAL */

    if (wndPtr->flags & WIN_NATIVE)
        X11DRV_Synchronize();  /* Synchronize with the host window system */

    wndTemp = WIN_GetDesktop();

    /* repaint invalidated region (if any)
     *
     * FIXME: if SWP_NOACTIVATE is not set then set invalid regions here without any painting
     *        and force update after ChangeActiveWindow() to avoid painting frames twice.
     */

    if( visRgn )
    {
        if( !(winpos->flags & SWP_NOREDRAW) )
        {

            /*  Use PAINT_RedrawWindow to explicitly force an invalidation of the window,
		its parent and sibling and so on, and then erase the parent window
		background if the parent is either a top-level window or its parent's parent
		is top-level window. Rely on the system to repaint other affected
		windows later on.  */
            if( uFlags & SWP_EX_PAINTSELF )
            {
                PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, (visRgn == 1) ? 0 : visRgn,
                                    RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN,
                                    RDW_EX_XYWINDOW | RDW_EX_USEHRGN );
            }
            else
            {
                PAINT_RedrawWindow( wndPtr->parent->hwndSelf, NULL, (visRgn == 1) ? 0 : visRgn,
                                    RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN,
                                    RDW_EX_USEHRGN );
            }

            if(wndPtr -> parent == wndTemp || wndPtr->parent->parent == wndTemp )
            {
                RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0,
                              RDW_ERASENOW | RDW_NOCHILDREN );
            }
        }
        if( visRgn != 1 )
            DeleteObject( visRgn );
    }

    WIN_ReleaseDesktop();

    if (!(winpos->flags & SWP_NOACTIVATE))
        WINPOS_ChangeActiveWindow( winpos->hwnd, FALSE );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    TRACE("\tstatus flags = %04x\n", winpos->flags & SWP_AGG_STATUSFLAGS);

    if ( resync ||
         (((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE) &&
          !(winpos->flags & SWP_NOSENDCHANGING)) )
    {
        SendMessageA( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)winpos );
        if (resync) X11DRV_Synchronize();
    }

    retvalue = TRUE;
 END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *		SetWindowRgn  (X11DRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
BOOL X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    int ret = FALSE;

    if (!wndPtr) return FALSE;

    if (wndPtr->hrgnWnd == hrgn)
    {
        ret = TRUE;
        goto done;
    }

    if (hrgn) /* verify that region really exists */
    {
        if (GetRgnBox( hrgn, &rect ) == ERROR) goto done;
    }

    if (wndPtr->hrgnWnd)
    {
        /* delete previous region */
        DeleteObject(wndPtr->hrgnWnd);
        wndPtr->hrgnWnd = 0;
    }
    wndPtr->hrgnWnd = hrgn;

    /* Size the window to the rectangle of the new region (if it isn't NULL) */
    if (hrgn) SetWindowPos( hwnd, 0, rect.left, rect.top,
                            rect.right  - rect.left, rect.bottom - rect.top,
                            SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOACTIVATE |
                            SWP_NOZORDER | (redraw ? 0 : SWP_NOREDRAW) );
#ifdef HAVE_LIBXSHAPE
    {
        Display *display = thread_display();
        Window win = X11DRV_WND_GetXWindow(wndPtr);

        if (win)
        {
            if (!hrgn)
            {
                TSXShapeCombineMask( display, win, ShapeBounding, 0, 0, None, ShapeSet );
            }
            else
            {
                XRectangle *aXRect;
                DWORD size;
                DWORD dwBufferSize = GetRegionData(hrgn, 0, NULL);
                PRGNDATA pRegionData = HeapAlloc(GetProcessHeap(), 0, dwBufferSize);
                if (!pRegionData) goto done;

                GetRegionData(hrgn, dwBufferSize, pRegionData);
                size = pRegionData->rdh.nCount;
                /* convert region's "Windows rectangles" to XRectangles */
                aXRect = HeapAlloc(GetProcessHeap(), 0, size * sizeof(*aXRect) );
                if (aXRect)
                {
                    XRectangle* pCurrRect = aXRect;
                    RECT *pRect = (RECT*) pRegionData->Buffer;
                    for (; pRect < ((RECT*) pRegionData->Buffer) + size ; ++pRect, ++pCurrRect)
                    {
                        pCurrRect->x      = pRect->left;
                        pCurrRect->y      = pRect->top;
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
                    TSXShapeCombineRectangles( display, win, ShapeBounding,
                                               0, 0, aXRect,
                                               pCurrRect - aXRect, ShapeSet, YXBanded );
                    HeapFree(GetProcessHeap(), 0, aXRect );
                }
                HeapFree(GetProcessHeap(), 0, pRegionData);
            }
        }
    }
#endif  /* HAVE_LIBXSHAPE */

    ret = TRUE;

 done:
    WIN_ReleaseWndPtr(wndPtr);
    return ret;
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
static LONG start_size_move( WND* wndPtr, WPARAM wParam, POINT *capturePoint )
{
    LONG hittest = 0;
    POINT pt;
    MSG msg;
    RECT rectWindow;

    GetWindowRect(wndPtr->hwndSelf,&rectWindow);

    if ((wParam & 0xfff0) == SC_MOVE)
    {
        /* Move pointer at the center of the caption */
        RECT rect;
        NC_GetInsideRect( wndPtr->hwndSelf, &rect );
        if (wndPtr->dwStyle & WS_SYSMENU)
            rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
        if (wndPtr->dwStyle & WS_MINIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
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
            MSG_InternalGetMessage( &msg, 0, 0, WM_KEYFIRST, WM_MOUSELAST,
                                    MSGF_SIZE, PM_REMOVE, FALSE, NULL );
            switch(msg.message)
            {
            case WM_MOUSEMOVE:
                hittest = NC_HandleNCHitTest( wndPtr->hwndSelf, msg.pt );
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
    NC_HandleSetCursor( wndPtr->hwndSelf,
                        wndPtr->hwndSelf, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           X11DRV_SysCommandSizeMove   (X11DRV.@)
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
void X11DRV_SysCommandSizeMove( HWND hwnd, WPARAM wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect, origRect;
    HDC hdc;
    LONG hittest = (LONG)(wParam & 0x0f);
    HCURSOR16 hDragCursor = 0, hOldCursor = 0;
    POINT minTrack, maxTrack;
    POINT capturePoint, pt;
    WND *     wndPtr = WIN_FindWndPtr( hwnd );
    BOOL    thickframe = HAS_THICKFRAME( wndPtr->dwStyle, wndPtr->dwExStyle );
    BOOL    iconic = wndPtr->dwStyle & WS_MINIMIZE;
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = FALSE;
    BOOL grab;
    int iWndsLocks;
    Display *old_gdi_display = NULL;
    Display *display = thread_display();

    SystemParametersInfoA(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);

    pt.x = SLOWORD(dwPoint);
    pt.y = SHIWORD(dwPoint);
    capturePoint = pt;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd) ||
        (wndPtr->dwExStyle & WS_EX_MANAGED)) goto END;

    if ((wParam & 0xfff0) == SC_MOVE)
    {
        if (!hittest) hittest = start_size_move( wndPtr, wParam, &capturePoint );
        if (!hittest) goto END;
    }
    else  /* SC_SIZE */
    {
        if (!thickframe) goto END;
        if ( hittest && hittest != HTSYSMENU ) hittest += 2;
        else
        {
            SetCapture(hwnd);
            hittest = start_size_move( wndPtr, wParam, &capturePoint );
            if (!hittest)
            {
                ReleaseCapture();
                goto END;
            }
        }
    }

      /* Get min/max info */

    WINPOS_GetMinMaxInfo( wndPtr, NULL, NULL, &minTrack, &maxTrack );
    sizingRect = wndPtr->rectWindow;
    origRect = sizingRect;
    if (wndPtr->dwStyle & WS_CHILD)
        GetClientRect( wndPtr->parent->hwndSelf, &mouseRect );
    else
        SetRect(&mouseRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
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
    if (wndPtr->dwStyle & WS_CHILD)
    {
        MapWindowPoints( wndPtr->parent->hwndSelf, 0, (LPPOINT)&mouseRect, 2 );
    }
    SendMessageA( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (GetCapture() != hwnd) SetCapture( hwnd );

    if (wndPtr->parent && (wndPtr->parent->hwndSelf != GetDesktopWindow()))
    {
          /* Retrieve a default cache DC (without using the window style) */
        hdc = GetDCEx( wndPtr->parent->hwndSelf, 0, DCX_CACHE );
    }
    else
        hdc = GetDC( 0 );

    if( iconic ) /* create a cursor for dragging */
    {
        HICON hIcon = GetClassLongA( hwnd, GCL_HICON);
        if(!hIcon) hIcon = (HICON)SendMessageA( hwnd, WM_QUERYDRAGICON, 0, 0L);
        if( hIcon ) hDragCursor =  CURSORICON_IconToCursor( hIcon, TRUE );
        if( !hDragCursor ) iconic = FALSE;
    }

    /* repaint the window before moving it around */
    RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    /* grab the server only when moving top-level windows without desktop */
    grab = (!DragFullWindows && (root_window == DefaultRootWindow(gdi_display)) &&
            (wndPtr->parent->hwndSelf == GetDesktopWindow()));
    if (grab)
    {
        wine_tsx11_lock();
        XSync( gdi_display, False );
        XGrabServer( display );
        /* switch gdi display to the thread display, since the server is grabbed */
        old_gdi_display = gdi_display;
        gdi_display = display;
        wine_tsx11_unlock();
    }

    while(1)
    {
        int dx = 0, dy = 0;

        MSG_InternalGetMessage( &msg, 0, 0, WM_KEYFIRST, WM_MOUSELAST,
                                MSGF_SIZE, PM_REMOVE, FALSE, NULL );

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
                    WINPOS_ShowIconTitle( wndPtr, FALSE );
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

    ReleaseCapture();
    if( iconic )
    {
        if( moved ) /* restore cursors, show icon title later on */
        {
            ShowCursor( FALSE );
            SetCursor( hOldCursor );
        }
        DestroyCursor( hDragCursor );
    }
    else if (moved && !DragFullWindows)
        draw_moving_frame( hdc, &sizingRect, thickframe );

    if (wndPtr->parent && (wndPtr->parent->hwndSelf != GetDesktopWindow()))
        ReleaseDC( wndPtr->parent->hwndSelf, hdc );
    else
        ReleaseDC( 0, hdc );

    if (grab)
    {
        wine_tsx11_lock();
        XSync( display, False );
        XUngrabServer( display );
        gdi_display = old_gdi_display;
        wine_tsx11_unlock();
    }

    if (HOOK_CallHooksA( WH_CBT, HCBT_MOVESIZE, hwnd, (LPARAM)&sizingRect ))
        sizingRect = wndPtr->rectWindow;

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
            if( wndPtr->dwStyle & WS_SYSMENU )
                SendMessageA( hwnd, WM_SYSCOMMAND,
                              SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x,pt.y));
        }
        else WINPOS_ShowIconTitle( wndPtr, TRUE );
    }

END:
    WIN_ReleaseWndPtr(wndPtr);
}

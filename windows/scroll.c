/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#include <stdlib.h>

#include "wine/winuser16.h"
#include "winuser.h"
#include "dc.h"
#include "win.h"
#include "gdi.h"
#include "dce.h"
#include "region.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(scroll)

/*************************************************************************
 *             ScrollWindow16   (USER.61)
 */
void WINAPI ScrollWindow16(HWND16 hwnd, INT16 dx, INT16 dy, const RECT16 *rect,
                           const RECT16 *clipRect )
{
    RECT rect32, clipRect32;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ScrollWindow( hwnd, dx, dy, rect ? &rect32 : NULL,
                    clipRect ? &clipRect32 : NULL );
}

/*************************************************************************
 *             ScrollWindow32   (USER32.450)
 *
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy,
                              const RECT *rect, const RECT *clipRect )
{
    return
        (ERROR != ScrollWindowEx( hwnd, dx, dy, rect, clipRect, 0, NULL,
                                    (rect ? 0 : SW_SCROLLCHILDREN) |
                                    SW_INVALIDATE ));
}

/*************************************************************************
 *             ScrollDC16   (USER.221)
 */
BOOL16 WINAPI ScrollDC16( HDC16 hdc, INT16 dx, INT16 dy, const RECT16 *rect,
                          const RECT16 *cliprc, HRGN16 hrgnUpdate,
                          LPRECT16 rcUpdate )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (cliprc) CONV_RECT16TO32( cliprc, &clipRect32 );
    ret = ScrollDC( hdc, dx, dy, rect ? &rect32 : NULL,
                      cliprc ? &clipRect32 : NULL, hrgnUpdate, &rcUpdate32 );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
}


/*************************************************************************
 *             ScrollDC32   (USER32.449)
 * 
 *   Only the hrgnUpdate is return in device coordinate.
 *   rcUpdate must be returned in logical coordinate to comply with win API.
 *
 */
BOOL WINAPI ScrollDC( HDC hdc, INT dx, INT dy, const RECT *rc,
                          const RECT *prLClip, HRGN hrgnUpdate,
                          LPRECT rcUpdate )
{
    RECT rect, rClip, rSrc;
    POINT src, dest;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    TRACE("%04x %d,%d hrgnUpdate=%04x rcUpdate = %p cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d)\n",
                   (HDC16)hdc, dx, dy, hrgnUpdate, rcUpdate, 
		   prLClip ? prLClip->left : 0, prLClip ? prLClip->top : 0, prLClip ? prLClip->right : 0, prLClip ? prLClip->bottom : 0,
		   rc ? rc->left : 0, rc ? rc->top : 0, rc ? rc->right : 0, rc ? rc->bottom : 0 );

    if ( !dc || !hdc ) return FALSE;

/*
    TRACE(scroll,"\t[wndOrgX=%i, wndExtX=%i, vportOrgX=%i, vportExtX=%i]\n",
          dc->wndOrgX, dc->wndExtX, dc->vportOrgX, dc->vportExtX );
    TRACE(scroll,"\t[wndOrgY=%i, wndExtY=%i, vportOrgY=%i, vportExtY=%i]\n",
	  dc->wndOrgY, dc->wndExtY, dc->vportOrgY, dc->vportExtY );
*/

    /* compute device clipping region */

    if ( rc )
	rect = *rc;
    else /* maybe we should just return FALSE? */
	GetClipBox( hdc, &rect );

    if (prLClip)
	IntersectRect( &rClip,&rect,prLClip );
    else
        rClip = rect;

    rSrc = rClip;
    OffsetRect( &rSrc, -dx, -dy );
    IntersectRect( &rSrc, &rSrc, &rect );

    if(dc->w.hVisRgn)
    {
        if (!IsRectEmpty(&rSrc))
        {
            dest.x = (src.x = rSrc.left) + dx;
            dest.y = (src.y = rSrc.top) + dy;

            /* copy bits */

            if (!BitBlt( hdc, dest.x, dest.y,
                           rSrc.right - rSrc.left, rSrc.bottom - rSrc.top,
                           hdc, src.x, src.y, SRCCOPY))
            {
                GDI_HEAP_UNLOCK( hdc );
                return FALSE;
            }
        }

        /* compute update areas */

        if (hrgnUpdate || rcUpdate)
        {
            HRGN hrgn =
              (hrgnUpdate) ? hrgnUpdate : CreateRectRgn( 0,0,0,0 );
            HRGN hrgn2;

            dx = XLPTODP ( dc, rect.left + dx) - XLPTODP ( dc, rect.left);
            dy = YLPTODP ( dc, rect.top + dy) - YLPTODP ( dc, rect.top);
            LPtoDP( hdc, (LPPOINT)&rect, 2 );
            LPtoDP( hdc, (LPPOINT)&rClip, 2 );
            hrgn2 = CreateRectRgnIndirect( &rect );
            OffsetRgn( hrgn2, dc->w.DCOrgX, dc->w.DCOrgY );
            CombineRgn( hrgn2, hrgn2, dc->w.hVisRgn, RGN_AND );
            OffsetRgn( hrgn2, -dc->w.DCOrgX, -dc->w.DCOrgY );
            SetRectRgn( hrgn, rClip.left, rClip.top,
                          rClip.right, rClip.bottom );
            CombineRgn( hrgn, hrgn, hrgn2, RGN_AND );
            OffsetRgn( hrgn2, dx, dy );
            CombineRgn( hrgn, hrgn, hrgn2, RGN_DIFF );

            if( rcUpdate )
	    {
		GetRgnBox( hrgn, rcUpdate );

		/* Put the rcUpdate in logical coordinate */
		DPtoLP( hdc, (LPPOINT)rcUpdate, 2 );
	    }
            if (!hrgnUpdate) DeleteObject( hrgn );
            DeleteObject( hrgn2 );

        }
    }
    else
    {
       if (hrgnUpdate) SetRectRgn(hrgnUpdate, 0, 0, 0, 0);
       if (rcUpdate) SetRectEmpty(rcUpdate);
    }

    GDI_HEAP_UNLOCK( hdc );
    return TRUE;
}


/*************************************************************************
 *             ScrollWindowEx16   (USER.319)
 */
INT16 WINAPI ScrollWindowEx16( HWND16 hwnd, INT16 dx, INT16 dy,
                               const RECT16 *rect, const RECT16 *clipRect,
                               HRGN16 hrgnUpdate, LPRECT16 rcUpdate,
                               UINT16 flags )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ret = ScrollWindowEx( hwnd, dx, dy, rect ? &rect32 : NULL,
                            clipRect ? &clipRect32 : NULL, hrgnUpdate,
                            (rcUpdate) ? &rcUpdate32 : NULL, flags );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
}

/*************************************************************************
 *             SCROLL_FixCaret
 */
static BOOL SCROLL_FixCaret(HWND hWnd, LPRECT lprc, UINT flags)
{
   HWND hCaret = CARET_GetHwnd();

   if( hCaret )
   {
       RECT	rc;
       CARET_GetRect( &rc );
       if( hCaret == hWnd ||
          (flags & SW_SCROLLCHILDREN && IsChild(hWnd, hCaret)) )
       {
           POINT     pt;

           pt.x = rc.left; pt.y = rc.top;
           MapWindowPoints( hCaret, hWnd, (LPPOINT)&rc, 2 );
           if( IntersectRect(lprc, lprc, &rc) )
           {
               HideCaret(0);
  	       lprc->left = pt.x; lprc->top = pt.y;
	       return TRUE;
           }
       }
   }
   return FALSE;
}

/*************************************************************************
 *             ScrollWindowEx32   (USER32.451)
 *
 * NOTE: Use this function instead of ScrollWindow32
 */
INT WINAPI ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                               const RECT *rect, const RECT *clipRect,
                               HRGN hrgnUpdate, LPRECT rcUpdate,
                               UINT flags )
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
	DC*	dc;
	HDC	hDC;
	BOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
	HRGN  hrgnClip = CreateRectRgnIndirect(&cliprc);
        HRGN  hrgnTemp = CreateRectRgnIndirect(&rc);
        RECT  caretrc;

TRACE("%04x, %d,%d hrgnUpdate=%04x rcUpdate = %p \
cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d) %04x\n",             
(HWND16)hwnd, dx, dy, hrgnUpdate, rcUpdate,
clipRect?clipRect->left:0, clipRect?clipRect->top:0, clipRect?clipRect->right:0, clipRect?clipRect->bottom:0,
rc.left, rc.top, rc.right, rc.bottom, (UINT16)flags );

        caretrc = rc; 
	bCaret = SCROLL_FixCaret(hwnd, &caretrc, flags);

	if( hrgnUpdate ) bOwnRgn = FALSE;
        else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

	hDC = GetDCEx( hwnd, hrgnClip, DCX_CACHE | DCX_USESTYLE |
                         DCX_KEEPCLIPRGN | DCX_INTERSECTRGN |
		       ((flags & SW_SCROLLCHILDREN) ? DCX_NOCLIPCHILDREN : 0) );
	if( (dc = (DC *)GDI_GetObjPtr(hDC, DC_MAGIC)) )
	{
            if (dc->w.hVisRgn) {
                wnd->pDriver->pSurfaceCopy(wnd,dc,dx,dy,&rc,bUpdate);

                if( bUpdate )
                    {
                        OffsetRgn( hrgnTemp, dc->w.DCOrgX, dc->w.DCOrgY );
                        CombineRgn( hrgnTemp, hrgnTemp, dc->w.hVisRgn,
                                      RGN_AND );
                        OffsetRgn( hrgnTemp, -dc->w.DCOrgX, -dc->w.DCOrgY );
                        CombineRgn( hrgnUpdate, hrgnTemp, hrgnClip,
                                      RGN_AND );
                        OffsetRgn( hrgnTemp, dx, dy );
                        retVal =
                            CombineRgn( hrgnUpdate, hrgnUpdate, hrgnTemp,
                                          RGN_DIFF );

                        if( rcUpdate ) GetRgnBox( hrgnUpdate, rcUpdate );
                    }
            }
            ReleaseDC(hwnd, hDC);
            GDI_HEAP_UNLOCK( hDC );
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
	    RECT	r;
	    WND* 	w;
	    for( w =WIN_LockWndPtr(wnd->child); w; WIN_UpdateWndPtr(&w, w->next))
	    {
		 CONV_RECT16TO32( &w->rectWindow, &r );
	         if( IntersectRect(&r, &r, &rc) )
		     SetWindowPos(w->hwndSelf, 0, w->rectWindow.left + dx,
				    w->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
				    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
				    SWP_DEFERERASE );
	    }
	}

	if( flags & (SW_INVALIDATE | SW_ERASE) )
	    PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
		((flags & SW_ERASE) ? RDW_ERASENOW : 0) | ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ), 0 );

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

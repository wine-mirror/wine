/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#include <stdlib.h>
#include "winuser.h"
#include "class.h"
#include "dc.h"
#include "win.h"
#include "gdi.h"
#include "dce.h"
#include "region.h"
#include "sysmetrics.h"
#include "debug.h"

/*************************************************************************
 *             ScrollWindow16   (USER.61)
 */
void WINAPI ScrollWindow16(HWND16 hwnd, INT16 dx, INT16 dy, const RECT16 *rect,
                           const RECT16 *clipRect )
{
    RECT32 rect32, clipRect32;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ScrollWindow32( hwnd, dx, dy, rect ? &rect32 : NULL,
                    clipRect ? &clipRect32 : NULL );
}

/*************************************************************************
 *             ScrollWindow32   (USER32.450)
 *
 */
BOOL32 WINAPI ScrollWindow32( HWND32 hwnd, INT32 dx, INT32 dy,
                              const RECT32 *rect, const RECT32 *clipRect )
{
    return
        (ERROR != ScrollWindowEx32( hwnd, dx, dy, rect, clipRect, 0, NULL,
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
    RECT32 rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (cliprc) CONV_RECT16TO32( cliprc, &clipRect32 );
    ret = ScrollDC32( hdc, dx, dy, rect ? &rect32 : NULL,
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
BOOL32 WINAPI ScrollDC32( HDC32 hdc, INT32 dx, INT32 dy, const RECT32 *rc,
                          const RECT32 *prLClip, HRGN32 hrgnUpdate,
                          LPRECT32 rcUpdate )
{
    RECT32 rect, rClip, rSrc;
    POINT32 src, dest;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    TRACE(scroll,"%04x %d,%d hrgnUpdate=%04x rcUpdate = %p cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d)\n",
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
	GetClipBox32( hdc, &rect );

    if (prLClip)
	IntersectRect32( &rClip,&rect,prLClip );
    else
        rClip = rect;

    rSrc = rClip;
    OffsetRect32( &rSrc, -dx, -dy );
    IntersectRect32( &rSrc, &rSrc, &rect );

    if(dc->w.hVisRgn)
    {
        if (!IsRectEmpty32(&rSrc))
        {
            dest.x = (src.x = rSrc.left) + dx;
            dest.y = (src.y = rSrc.top) + dy;

            /* copy bits */

            if (!BitBlt32( hdc, dest.x, dest.y,
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
            HRGN32 hrgn =
              (hrgnUpdate) ? hrgnUpdate : CreateRectRgn32( 0,0,0,0 );
            HRGN32 hrgn2;

            dx = XLPTODP ( dc, rect.left + dx) - XLPTODP ( dc, rect.left);
            dy = YLPTODP ( dc, rect.top + dy) - YLPTODP ( dc, rect.top);
            LPtoDP32( hdc, (LPPOINT32)&rect, 2 );
            LPtoDP32( hdc, (LPPOINT32)&rClip, 2 );
            hrgn2 = CreateRectRgnIndirect32( &rect );
            OffsetRgn32( hrgn2, dc->w.DCOrgX, dc->w.DCOrgY );
            CombineRgn32( hrgn2, hrgn2, dc->w.hVisRgn, RGN_AND );
            OffsetRgn32( hrgn2, -dc->w.DCOrgX, -dc->w.DCOrgY );
            SetRectRgn32( hrgn, rClip.left, rClip.top,
                          rClip.right, rClip.bottom );
            CombineRgn32( hrgn, hrgn, hrgn2, RGN_AND );
            OffsetRgn32( hrgn2, dx, dy );
            CombineRgn32( hrgn, hrgn, hrgn2, RGN_DIFF );

            if( rcUpdate )
	    {
		GetRgnBox32( hrgn, rcUpdate );

		//Put the rcUpdate in logical coordinate
		DPtoLP32( hdc, (LPPOINT32)rcUpdate, 2 );
	    }
            if (!hrgnUpdate) DeleteObject32( hrgn );
            DeleteObject32( hrgn2 );

        }
    }
    else
    {
       if (hrgnUpdate) SetRectRgn32(hrgnUpdate, 0, 0, 0, 0);
       if (rcUpdate) SetRectEmpty32(rcUpdate);
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
    RECT32 rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ret = ScrollWindowEx32( hwnd, dx, dy, rect ? &rect32 : NULL,
                            clipRect ? &clipRect32 : NULL, hrgnUpdate,
                            (rcUpdate) ? &rcUpdate32 : NULL, flags );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
}

/*************************************************************************
 *             SCROLL_FixCaret
 */
static BOOL32 SCROLL_FixCaret(HWND32 hWnd, LPRECT32 lprc, UINT32 flags)
{
   HWND32 hCaret = CARET_GetHwnd();

   if( hCaret )
   {
       RECT32	rc;
       CARET_GetRect( &rc );
       if( hCaret == hWnd ||
          (flags & SW_SCROLLCHILDREN && IsChild32(hWnd, hCaret)) )
       {
           POINT32     pt;

           pt.x = rc.left; pt.y = rc.top;
           MapWindowPoints32( hCaret, hWnd, (LPPOINT32)&rc, 2 );
           if( IntersectRect32(lprc, lprc, &rc) )
           {
               HideCaret32(0);
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
INT32 WINAPI ScrollWindowEx32( HWND32 hwnd, INT32 dx, INT32 dy,
                               const RECT32 *rect, const RECT32 *clipRect,
                               HRGN32 hrgnUpdate, LPRECT32 rcUpdate,
                               UINT32 flags )
{
    INT32  retVal = NULLREGION;
    BOOL32 bCaret = FALSE, bOwnRgn = TRUE;
    RECT32 rc, cliprc;
    WND*   wnd = WIN_FindWndPtr( hwnd );

    if( !wnd || !WIN_IsWindowDrawable( wnd, TRUE )) return ERROR;

    GetClientRect32(hwnd, &rc);
    if (rect) IntersectRect32(&rc, &rc, rect);

    if (clipRect) IntersectRect32(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (!IsRectEmpty32(&cliprc) && (dx || dy))
    {
	DC*	dc;
	HDC32	hDC;
	BOOL32  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
	HRGN32  hrgnClip = CreateRectRgnIndirect32(&cliprc);
        HRGN32  hrgnTemp = CreateRectRgnIndirect32(&rc);
        RECT32  caretrc;

TRACE(scroll,"%04x, %d,%d hrgnUpdate=%04x rcUpdate = %p \
cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d) %04x\n",             
(HWND16)hwnd, dx, dy, hrgnUpdate, rcUpdate,
clipRect?clipRect->left:0, clipRect?clipRect->top:0, clipRect?clipRect->right:0, clipRect?clipRect->bottom:0,
rc.left, rc.top, rc.right, rc.bottom, (UINT16)flags );

        caretrc = rc; 
	bCaret = SCROLL_FixCaret(hwnd, &caretrc, flags);

	if( hrgnUpdate ) bOwnRgn = FALSE;
        else if( bUpdate ) hrgnUpdate = CreateRectRgn32( 0, 0, 0, 0 );

	hDC = GetDCEx32( hwnd, hrgnClip, DCX_CACHE | DCX_USESTYLE |
                         DCX_KEEPCLIPRGN | DCX_INTERSECTRGN |
		       ((flags & SW_SCROLLCHILDREN) ? DCX_NOCLIPCHILDREN : 0) );
	if( (dc = (DC *)GDI_GetObjPtr(hDC, DC_MAGIC)) )
	{
            if (dc->w.hVisRgn) {
                wnd->pDriver->pScrollWindow(wnd,dc,dx,dy,&rc,bUpdate);

                if( bUpdate )
                    {
                        OffsetRgn32( hrgnTemp, dc->w.DCOrgX, dc->w.DCOrgY );
                        CombineRgn32( hrgnTemp, hrgnTemp, dc->w.hVisRgn,
                                      RGN_AND );
                        OffsetRgn32( hrgnTemp, -dc->w.DCOrgX, -dc->w.DCOrgY );
                        CombineRgn32( hrgnUpdate, hrgnTemp, hrgnClip,
                                      RGN_AND );
                        OffsetRgn32( hrgnTemp, dx, dy );
                        retVal =
                            CombineRgn32( hrgnUpdate, hrgnUpdate, hrgnTemp,
                                          RGN_DIFF );

                        if( rcUpdate ) GetRgnBox32( hrgnUpdate, rcUpdate );
                    }
            }
            ReleaseDC32(hwnd, hDC);
            GDI_HEAP_UNLOCK( hDC );
        }

	if( wnd->hrgnUpdate > 1 )
	{
            /* Takes into account the fact that some damages may have
               occured during the scroll. */
            CombineRgn32( hrgnTemp, wnd->hrgnUpdate, 0, RGN_COPY );
            OffsetRgn32( hrgnTemp, dx, dy );
            CombineRgn32( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
            CombineRgn32( wnd->hrgnUpdate, wnd->hrgnUpdate, hrgnTemp, RGN_OR );
	}

	if( flags & SW_SCROLLCHILDREN )
	{
	    RECT32	r;
	    WND* 	w;
	    for( w = wnd->child; w; w = w->next )
	    {
		 CONV_RECT16TO32( &w->rectWindow, &r );
	         if( IntersectRect32(&r, &r, &rc) )
		     SetWindowPos32(w->hwndSelf, 0, w->rectWindow.left + dx,
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
	    SetCaretPos32( caretrc.left + dx, caretrc.top + dy );
	    ShowCaret32(0);
	}

	if( bOwnRgn && hrgnUpdate ) DeleteObject32( hrgnUpdate );
	DeleteObject32( hrgnClip );
        DeleteObject32( hrgnTemp );
    }
    return retVal;
}

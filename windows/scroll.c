/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#include <stdlib.h>
#include "windows.h"
#include "class.h"
#include "dc.h"
#include "win.h"
#include "gdi.h"
#include "dce.h"
#include "region.h"
#include "sysmetrics.h"
#include "debug.h"
#include "x11drv.h"

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
 * FIXME: verify clipping region calculations
 */
BOOL32 WINAPI ScrollWindow32( HWND32 hwnd, INT32 dx, INT32 dy,
                              const RECT32 *rect, const RECT32 *clipRect )
{
    HDC32  	hdc;
    HRGN32 	hrgnUpdate,hrgnClip;
    RECT32 	rc, cliprc;
    HWND32 	hCaretWnd = CARET_GetHwnd();
    WND*	wndScroll = WIN_FindWndPtr( hwnd );

    TRACE(scroll,"hwnd=%04x, dx=%d, dy=%d, lpRect =%p clipRect=%i,%i,%i,%i\n", 
                   hwnd, dx, dy, rect,
                   clipRect ? clipRect->left : 0,
                   clipRect ? clipRect->top : 0,
                   clipRect ? clipRect->right : 0, 
                   clipRect ? clipRect->bottom : 0 );

    if ( !wndScroll || !WIN_IsWindowDrawable( wndScroll, TRUE ) ) return TRUE;

    if ( !rect ) /* do not clip children */
       {
	  GetClientRect32(hwnd, &rc);
	  hrgnClip = CreateRectRgnIndirect32( &rc );

          if ((hCaretWnd == hwnd) || IsChild32(hwnd,hCaretWnd))
              HideCaret32(hCaretWnd);
          else hCaretWnd = 0;
 
	  hdc = GetDCEx32(hwnd, hrgnClip, DCX_CACHE | DCX_CLIPSIBLINGS);
          DeleteObject32( hrgnClip );
       }
    else	/* clip children */
       {
	  CopyRect32(&rc, rect);

          if (hCaretWnd == hwnd) HideCaret32(hCaretWnd);
          else hCaretWnd = 0;

	  hdc = GetDCEx32( hwnd, 0, DCX_CACHE | DCX_USESTYLE );
       }

    if (clipRect == NULL)
	GetClientRect32(hwnd, &cliprc);
    else
	CopyRect32(&cliprc, clipRect);

    hrgnUpdate = CreateRectRgn32( 0, 0, 0, 0 );
    ScrollDC32( hdc, dx, dy, &rc, &cliprc, hrgnUpdate, NULL );
    ReleaseDC32(hwnd, hdc);

    if( !rect )		/* move child windows and update region */
    { 
      WND*	wndPtr;

      if( wndScroll->hrgnUpdate > 1 )
	OffsetRgn32( wndScroll->hrgnUpdate, dx, dy );

      for (wndPtr = wndScroll->child; wndPtr; wndPtr = wndPtr->next)
        SetWindowPos32(wndPtr->hwndSelf, 0, wndPtr->rectWindow.left + dx,
                       wndPtr->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
                       SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                       SWP_DEFERERASE );
    }

    PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_ALLCHILDREN |
                 RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW, RDW_C_USEHRGN );

    DeleteObject32( hrgnUpdate );
    if( hCaretWnd ) 
    {
	POINT32	pt;
	GetCaretPos32(&pt);
	pt.x += dx; pt.y += dy;
	SetCaretPos32(pt.x, pt.y);
	ShowCaret32(hCaretWnd);
    }
    return TRUE;
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
 * Both 'rc' and 'prLClip' are in logical units but update info is 
 * returned in device coordinates.
 */
BOOL32 WINAPI ScrollDC32( HDC32 hdc, INT32 dx, INT32 dy, const RECT32 *rc,
                          const RECT32 *prLClip, HRGN32 hrgnUpdate,
                          LPRECT32 rcUpdate )
{
    RECT32 rClip;
    POINT32 src, dest;
    INT32  ldx, ldy;
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
	rClip = *rc;
    else /* maybe we should just return FALSE? */
	GetClipBox32( hdc, &rClip );

    if (prLClip)
	IntersectRect32(&rClip,&rClip,prLClip);

    if( rClip.left >= rClip.right || rClip.top >= rClip.bottom )
    {
        GDI_HEAP_UNLOCK( hdc );
	return FALSE;
    }
   
    SaveVisRgn( hdc );
    IntersectVisRect( hdc, rClip.left, rClip.top, 
                           rClip.right, rClip.bottom ); 


    /* translate coordinates */

    ldx = dx * dc->wndExtX / dc->vportExtX;
    ldy = dy * dc->wndExtY / dc->vportExtY;

    if (dx > 0)
	dest.x = (src.x = rClip.left) + ldx;
    else
	src.x = (dest.x = rClip.left) - ldx;

    if (dy > 0)
	dest.y = (src.y = rClip.top) + ldy;
    else
	src.y = (dest.y = rClip.top) - ldy;

    /* copy bits */

    if( rClip.right - rClip.left > ldx &&
	rClip.bottom - rClip.top > ldy )
    {
	ldx = rClip.right - rClip.left - ldx;
	ldy = rClip.bottom - rClip.top - ldy;

	if (!BitBlt32( hdc, dest.x, dest.y, ldx, ldy,
		       hdc, src.x, src.y, SRCCOPY))
	{
	    GDI_HEAP_UNLOCK( hdc );
	    return FALSE;
	}
    }

    /* restore clipping region */

    RestoreVisRgn( hdc );


    /* compute update areas */

    if ( (hrgnUpdate || rcUpdate) && dc->w.hVisRgn )
    {
	HRGN32 hrgn = (hrgnUpdate) ? hrgnUpdate : CreateRectRgn32( 0,0,0,0 );
        HRGN32 hrgnClip;

        LPtoDP32( hdc, (LPPOINT32)&rClip, 2 );
        OffsetRect32( &rClip, dc->w.DCOrgX, dc->w.DCOrgY );
        hrgnClip = CreateRectRgnIndirect32( &rClip );
        
        CombineRgn32( hrgn, dc->w.hVisRgn, hrgnClip, RGN_AND );
        OffsetRgn32( hrgn, dx, dy );
        CombineRgn32( hrgn, dc->w.hVisRgn, hrgn, RGN_DIFF );
        CombineRgn32( hrgn, hrgn, hrgnClip, RGN_AND );
        OffsetRgn32( hrgn, -dc->w.DCOrgX, -dc->w.DCOrgY );

        if( rcUpdate ) GetRgnBox32( hrgnUpdate, rcUpdate );

	if (!hrgnUpdate) DeleteObject32( hrgn );
        DeleteObject32( hrgnClip );     
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

    if (rect == NULL) GetClientRect32(hwnd, &rc);
    else rc = *rect;

    if (clipRect) IntersectRect32(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (!IsRectEmpty32(&cliprc) && (dx || dy))
    {
	DC*	dc;
	HDC32	hDC;
	BOOL32  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
	HRGN32  hrgnClip = CreateRectRgnIndirect32(&cliprc);

TRACE(scroll,"%04x, %d,%d hrgnUpdate=%04x rcUpdate = %p \
cliprc = (%d,%d-%d,%d), rc=(%d,%d-%d,%d) %04x\n",             
(HWND16)hwnd, dx, dy, hrgnUpdate, rcUpdate,
clipRect?clipRect->left:0, clipRect?clipRect->top:0, clipRect?clipRect->right:0, clipRect?clipRect->bottom:0,
rect?rect->left:0, rect?rect->top:0, rect ?rect->right:0, rect ?rect->bottom:0, (UINT16)flags );

	rc = cliprc;
	bCaret = SCROLL_FixCaret(hwnd, &rc, flags);

	if( hrgnUpdate ) bOwnRgn = FALSE;
        else if( bUpdate ) hrgnUpdate = CreateRectRgn32( 0, 0, 0, 0 );

	hDC = GetDCEx32( hwnd, hrgnClip, DCX_CACHE | DCX_USESTYLE | 
		       ((flags & SW_SCROLLCHILDREN) ? DCX_NOCLIPCHILDREN : 0) );
	if( (dc = (DC *)GDI_GetObjPtr(hDC, DC_MAGIC)) )
	{
	    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
	    POINT32 dst, src;

	    if( dx > 0 ) dst.x = (src.x = dc->w.DCOrgX + cliprc.left) + dx;
	    else src.x = (dst.x = dc->w.DCOrgX + cliprc.left) - dx;

	    if( dy > 0 ) dst.y = (src.y = dc->w.DCOrgY + cliprc.top) + dy;
	    else src.y = (dst.y = dc->w.DCOrgY + cliprc.top) - dy;

            if ((cliprc.right - cliprc.left > abs(dx)) &&
                (cliprc.bottom - cliprc.top > abs(dy)))
            {
                if (bUpdate) /* handles non-Wine windows hanging over the scrolled area */
                    TSXSetGraphicsExposures( display, physDev->gc, True );
                TSXSetFunction( display, physDev->gc, GXcopy );
                TSXCopyArea( display, physDev->drawable, physDev->drawable,
			     physDev->gc, src.x, src.y,
			     cliprc.right - cliprc.left - abs(dx),
                             cliprc.bottom - cliprc.top - abs(dy),
			     dst.x, dst.y );
                if (bUpdate)
                    TSXSetGraphicsExposures( display, physDev->gc, False );
            }

	    if( dc->w.hVisRgn && bUpdate )
	    {
                OffsetRgn32( hrgnClip, dc->w.DCOrgX, dc->w.DCOrgY );
		CombineRgn32( hrgnUpdate, dc->w.hVisRgn, hrgnClip, RGN_AND );
		OffsetRgn32( hrgnUpdate, dx, dy );
		CombineRgn32( hrgnUpdate, dc->w.hVisRgn, hrgnUpdate, RGN_DIFF );
		CombineRgn32( hrgnUpdate, hrgnUpdate, hrgnClip, RGN_AND );
                OffsetRgn32( hrgnUpdate, -dc->w.DCOrgX, -dc->w.DCOrgY );

		if( rcUpdate ) GetRgnBox32( hrgnUpdate, rcUpdate );
	    }
	    ReleaseDC32(hwnd, hDC);
	    GDI_HEAP_UNLOCK( hDC );
	}

	if( wnd->hrgnUpdate > 1 )
	{
	    if( rect || clipRect )
	    {
		if( (CombineRgn32( hrgnClip, hrgnClip, 
				   wnd->hrgnUpdate, RGN_AND ) != NULLREGION) )
		{
		    CombineRgn32( wnd->hrgnUpdate, wnd->hrgnUpdate, hrgnClip, RGN_DIFF );
		    OffsetRgn32( hrgnClip, dx, dy );
		    CombineRgn32( wnd->hrgnUpdate, wnd->hrgnUpdate, hrgnClip, RGN_OR );
		}
	    }
	    else  
		OffsetRgn32( wnd->hrgnUpdate, dx, dy );
	}

	if( flags & SW_SCROLLCHILDREN )
	{
	    RECT32	r;
	    WND* 	w;
	    for( w = wnd->child; w; w = w->next )
	    {
		 CONV_RECT16TO32( &w->rectWindow, &r );
	         if( !clipRect || IntersectRect32(&r, &r, &cliprc) )
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
	    SetCaretPos32( rc.left + dx, rc.top + dy );
	    ShowCaret32(0);
	}

	if( bOwnRgn && hrgnUpdate ) DeleteObject32( hrgnUpdate );
	DeleteObject32( hrgnClip );
    }
    return retVal;
}


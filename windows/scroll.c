/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <stdlib.h>
#include "wintypes.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "region.h"
#include "sysmetrics.h"
#include "stddebug.h"
#include "debug.h"

extern HWND32 CARET_GetHwnd();			/* windows/caret.c */
extern void CLIPPING_UpdateGCRegion(DC* );	/* objects/clipping.c */

static int RgnType;


/*************************************************************************
 *             ScrollWindow16   (USER.61)
 */
void ScrollWindow16( HWND16 hwnd, INT16 dx, INT16 dy, const RECT16 *rect,
                     const RECT16 *clipRect )
{
    RECT32 rect32, clipRect32;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ScrollWindow32( hwnd, dx, dy, rect ? &rect32 : NULL,
                    clipRect ? &clipRect32 : NULL );
}


/*************************************************************************
 *             ScrollWindow32   (USER32.449)
 */
BOOL32 ScrollWindow32( HWND32 hwnd, INT32 dx, INT32 dy, const RECT32 *rect,
                       const RECT32 *clipRect )
{
    HDC32  	hdc;
    HRGN32 	hrgnUpdate,hrgnClip;
    RECT32 	rc, cliprc;
    HWND32 	hCaretWnd = CARET_GetHwnd();
    WND*	wndScroll = WIN_FindWndPtr( hwnd );

    dprintf_scroll(stddeb,"ScrollWindow: hwnd=%04x, dx=%d, dy=%d, lpRect =%p clipRect=%i,%i,%i,%i\n", 
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

          if ((hCaretWnd == hwnd) || IsChild(hwnd,hCaretWnd))
              HideCaret(hCaretWnd);
          else hCaretWnd = 0;
 
	  hdc = GetDCEx32(hwnd, hrgnClip, DCX_CACHE | DCX_CLIPSIBLINGS);
          DeleteObject32( hrgnClip );
       }
    else	/* clip children */
       {
	  CopyRect32(&rc, rect);

          if (hCaretWnd == hwnd) HideCaret(hCaretWnd);
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
        SetWindowPos(wndPtr->hwndSelf, 0, wndPtr->rectWindow.left + dx,
                     wndPtr->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                     SWP_DEFERERASE );
    }

    PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_ALLCHILDREN |
			    RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW, RDW_C_USEHRGN );

    DeleteObject32( hrgnUpdate );
    if( hCaretWnd ) ShowCaret(hCaretWnd);
    return TRUE;
}


/*************************************************************************
 *             ScrollDC16   (USER.221)
 */
BOOL16 ScrollDC16( HDC16 hdc, INT16 dx, INT16 dy, const RECT16 *rect,
                   const RECT16 *cliprc, HRGN16 hrgnUpdate, LPRECT16 rcUpdate )
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
 *             ScrollDC32   (USER32.448)
 */
BOOL32 ScrollDC32( HDC32 hdc, INT32 dx, INT32 dy, const RECT32 *rc,
                   const RECT32 *cliprc, HRGN32 hrgnUpdate, LPRECT32 rcUpdate )
{
    HRGN32 hrgnClip = 0;
    HRGN32 hrgnScrollClip = 0;
    RECT32 rectClip;
    POINT32 src, dest;
    INT32 width, height;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    dprintf_scroll(stddeb,"ScrollDC: dx=%d dy=%d, hrgnUpdate=%04x rcUpdate = %p cliprc = %p, rc=%d %d %d %d\n",
                   dx, dy, hrgnUpdate, rcUpdate, cliprc, rc ? rc->left : 0,
                   rc ? rc->top : 0, rc ? rc->right : 0, rc ? rc->bottom : 0 );

    if ( !dc || !hdc ) return FALSE;

    /* set clipping region */

    if ( !rc ) GetClipBox32( hdc, &rectClip );
    else rectClip = *rc;

    if (cliprc)
	IntersectRect32(&rectClip,&rectClip,cliprc);

    if( rectClip.left >= rectClip.right || rectClip.top >= rectClip.bottom )
	return FALSE;
    
    hrgnClip = GetClipRgn(hdc);
    hrgnScrollClip = CreateRectRgnIndirect32(&rectClip);

    if( hrgnClip )
      {
        /* save a copy and change cliprgn directly */

        CombineRgn32( hrgnScrollClip, hrgnClip, 0, RGN_COPY );
        SetRectRgn( hrgnClip, rectClip.left, rectClip.top,
                    rectClip.right, rectClip.bottom );

	CLIPPING_UpdateGCRegion( dc );
      }
    else
        SelectClipRgn32( hdc, hrgnScrollClip );

    /* translate coordinates */

    if (dx > 0)
    {
	src.x = XDPTOLP(dc, rectClip.left);
	dest.x = XDPTOLP(dc, rectClip.left + abs(dx));
    }
    else
    {
	src.x = XDPTOLP(dc, rectClip.left + abs(dx));
	dest.x = XDPTOLP(dc, rectClip.left);
    }
    if (dy > 0)
    {
	src.y = YDPTOLP(dc, rectClip.top);
	dest.y = YDPTOLP(dc, rectClip.top + abs(dy));
    }
    else
    {
	src.y = YDPTOLP(dc, rectClip.top + abs(dy));
	dest.y = YDPTOLP(dc, rectClip.top);
    }

    width = rectClip.right - rectClip.left - abs(dx);
    height = rectClip.bottom - rectClip.top - abs(dy);

    /* copy bits */

    if (!BitBlt32( hdc, dest.x, dest.y, width, height, hdc, src.x, src.y, 
                   SRCCOPY))
	return FALSE;

    /* compute update areas */

    if (hrgnUpdate || rcUpdate)
    {
	HRGN32 hrgn1 = (hrgnUpdate) ? hrgnUpdate : CreateRectRgn32( 0,0,0,0 );

	if( dc->w.hVisRgn )
	{
	  CombineRgn32( hrgn1, dc->w.hVisRgn, 0, RGN_COPY );
	  CombineRgn32( hrgn1, hrgn1, hrgnClip ? hrgnClip : hrgnScrollClip,
                        RGN_AND );
	  OffsetRgn32( hrgn1, dx, dy );
	  CombineRgn32( hrgn1, dc->w.hVisRgn, hrgn1, RGN_DIFF );
	  RgnType = CombineRgn32( hrgn1, hrgn1,
                                  hrgnClip ? hrgnClip : hrgnScrollClip,
                                  RGN_AND );
	}
	else
	{
	  RECT32 rect;

          rect = rectClip;				/* vertical band */
          if (dx > 0) rect.right = rect.left + dx;
          else if (dx < 0) rect.left = rect.right + dx;
          else SetRectEmpty32( &rect );
          SetRectRgn( hrgn1, rect.left, rect.top, rect.right, rect.bottom );

          rect = rectClip;				/* horizontal band */
          if (dy > 0) rect.bottom = rect.top + dy;
          else if (dy < 0) rect.top = rect.bottom + dy;
          else SetRectEmpty32( &rect );

          RgnType = REGION_UnionRectWithRgn( hrgn1, &rect );
	}

	if (rcUpdate) GetRgnBox32( hrgn1, rcUpdate );
	if (!hrgnUpdate) DeleteObject32( hrgn1 );
    }

    /* restore clipping region */

    SelectClipRgn32( hdc, hrgnClip ? hrgnScrollClip : 0 );
    DeleteObject32( hrgnScrollClip );     

    return TRUE;
}


/*************************************************************************
 *             ScrollWindowEx16   (USER.319)
 */
INT16 ScrollWindowEx16( HWND16 hwnd, INT16 dx, INT16 dy, const RECT16 *rect,
                        const RECT16 *clipRect, HRGN16 hrgnUpdate,
                        LPRECT16 rcUpdate, UINT16 flags )
{
    RECT32 rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (clipRect) CONV_RECT16TO32( clipRect, &clipRect32 );
    ret = ScrollWindowEx32( hwnd, dx, dy, rect ? &rect32 : NULL,
                            clipRect ? &clipRect32 : NULL, hrgnUpdate,
                            &rcUpdate32, flags );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
}


/*************************************************************************
 *             ScrollWindowEx32   (USER32.450)
 *
 * FIXME: broken, is there a program that actually uses it?
 *
 */
INT32 ScrollWindowEx32( HWND32 hwnd, INT32 dx, INT32 dy, const RECT32 *rect,
                        const RECT32 *clipRect, HRGN32 hrgnUpdate,
                        LPRECT32 rcUpdate, UINT32 flags )
{
    HDC32 hdc;
    RECT32 rc, cliprc;

    fprintf( stderr, "ScrollWindowEx: not fully implemented\n" );

    dprintf_scroll(stddeb,"ScrollWindowEx: dx=%d, dy=%d, wFlags=%04x\n",
                   dx, dy, flags);

    hdc = GetDC32(hwnd);

    if (rect == NULL)
	GetClientRect32(hwnd, &rc);
    else
	CopyRect32(&rc, rect);
    if (clipRect == NULL)
	GetClientRect32(hwnd, &cliprc);
    else
	CopyRect32(&cliprc, clipRect);

    ScrollDC32( hdc, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate );

    if (flags | SW_INVALIDATE)
    {
	PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                        ((flags & SW_ERASE) ? RDW_ERASENOW : 0), 0 );
    }

    ReleaseDC32(hwnd, hdc);
    return RgnType;
}

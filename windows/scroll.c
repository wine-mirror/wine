/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#include <stdlib.h>
#include "wintypes.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "region.h"
#include "sysmetrics.h"
#include "stddebug.h"
/* #define DEBUG_SCROLL */
#include "debug.h"

extern HWND CARET_GetHwnd();			/* windows/caret.c */
extern void CLIPPING_UpdateGCRegion(DC* );	/* objects/clipping.c */

static int RgnType;

/*************************************************************************
 *             ScrollWindow         (USER.61)
 *
 */
void ScrollWindow(HWND hwnd, short dx, short dy, LPRECT16 rect, LPRECT16 clipRect)
{
    HDC32  	hdc;
    HRGN32 	hrgnUpdate,hrgnClip;
    RECT16 	rc, cliprc;
    HWND 	hCaretWnd = CARET_GetHwnd();
    WND*	wndScroll = WIN_FindWndPtr( hwnd );

    dprintf_scroll(stddeb,"ScrollWindow: hwnd=%04x, dx=%d, dy=%d, lpRect =%08lx clipRect=%i,%i,%i,%i\n", 
	    hwnd, dx, dy, (LONG)rect, (int)((clipRect)?clipRect->left:0),
                                (int)((clipRect)?clipRect->top:0),
                                (int)((clipRect)?clipRect->right:0), 
                                (int)((clipRect)?clipRect->bottom:0));

    if ( !wndScroll || !WIN_IsWindowDrawable( wndScroll, TRUE ) ) return;

    if ( !rect ) /* do not clip children */
       {
	  GetClientRect16(hwnd, &rc);
	  hrgnClip = CreateRectRgnIndirect16( &rc );

          if ((hCaretWnd == hwnd) || IsChild(hwnd,hCaretWnd))
              HideCaret(hCaretWnd);
          else hCaretWnd = 0;
 
	  hdc = GetDCEx32(hwnd, hrgnClip, DCX_CACHE | DCX_CLIPSIBLINGS);
          DeleteObject(hrgnClip);
       }
    else	/* clip children */
       {
	  GetClientRect16(hwnd,&rc);
	  CopyRect16(&rc, rect);

          if (hCaretWnd == hwnd) HideCaret(hCaretWnd);
          else hCaretWnd = 0;

	  hdc = GetDC32(hwnd);
       }

    if (clipRect == NULL)
	GetClientRect16(hwnd, &cliprc);
    else
	CopyRect16(&cliprc, clipRect);

    hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
    ScrollDC(hdc, dx, dy, &rc, &cliprc, hrgnUpdate, NULL);
    ReleaseDC32(hwnd, hdc);

    if( !rect )		/* move child windows and update region */
    { 
      WND*	wndPtr;

      if( wndScroll->hrgnUpdate > 1 )
	OffsetRgn( wndScroll->hrgnUpdate, dx, dy );

      for (wndPtr = wndScroll->child; wndPtr; wndPtr = wndPtr->next)
        SetWindowPos(wndPtr->hwndSelf, 0, wndPtr->rectWindow.left + dx,
                     wndPtr->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                     SWP_DEFERERASE );
    }

    PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_ALLCHILDREN |
			    RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW, RDW_C_USEHRGN );

    DeleteObject(hrgnUpdate);
    if( hCaretWnd ) ShowCaret(hCaretWnd);
}


/*************************************************************************
 *             ScrollDC         (USER.221)
 *
 */
BOOL ScrollDC(HDC16 hdc, short dx, short dy, LPRECT16 rc, LPRECT16 cliprc,
	      HRGN32 hrgnUpdate, LPRECT16 rcUpdate)
{
    HRGN32 hrgnClip = 0;
    HRGN32 hrgnScrollClip = 0;
    RECT16	rectClip;
    POINT16 	src, dest;
    short width, height;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    dprintf_scroll(stddeb,"ScrollDC: dx=%d dy=%d, hrgnUpdate=%04x rcUpdate = %p cliprc = %p, rc=%d %d %d %d\n",
                   dx, dy, hrgnUpdate, rcUpdate, cliprc, rc ? rc->left : 0,
                   rc ? rc->top : 0, rc ? rc->right : 0, rc ? rc->bottom : 0 );

    if ( !dc || !hdc ) return FALSE;

    /* set clipping region */

    if ( !rc ) GetClipBox16( hdc, &rectClip );
    else rectClip = *rc;

    if (cliprc)
	IntersectRect16(&rectClip,&rectClip,cliprc);

    if( rectClip.left >= rectClip.right || rectClip.top >= rectClip.bottom )
	return FALSE;
    
    hrgnClip = GetClipRgn(hdc);
    hrgnScrollClip = CreateRectRgnIndirect16(&rectClip);

    if( hrgnClip )
      {
        /* save a copy and change cliprgn directly */

        CombineRgn( hrgnScrollClip, hrgnClip, 0, RGN_COPY );
        SetRectRgn( hrgnClip, rectClip.left, rectClip.top, rectClip.right, rectClip.bottom );

	CLIPPING_UpdateGCRegion( dc );
      }
    else
        SelectClipRgn( hdc, hrgnScrollClip );

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

    if (!BitBlt(hdc, dest.x, dest.y, width, height, hdc, src.x, src.y, 
		SRCCOPY))
	return FALSE;

    /* compute update areas */

    if (hrgnUpdate || rcUpdate)
    {
	HRGN32 hrgn1 = (hrgnUpdate)?hrgnUpdate:CreateRectRgn( 0,0,0,0 );

	if( dc->w.hVisRgn )
	{
	  CombineRgn( hrgn1, dc->w.hVisRgn, 0, RGN_COPY);
	  CombineRgn( hrgn1, hrgn1, (hrgnClip)?hrgnClip:hrgnScrollClip, RGN_AND);
	  OffsetRgn( hrgn1, dx, dy );
	  CombineRgn( hrgn1, dc->w.hVisRgn, hrgn1, RGN_DIFF);
	  RgnType = CombineRgn( hrgn1, hrgn1, (hrgnClip)?hrgnClip:hrgnScrollClip, RGN_AND);
	}
	else
	{
	  RECT16	rect;

          rect = rectClip;				/* vertical band */
          if (dx > 0) rect.right = rect.left + dx;
          else if (dx < 0) rect.left = rect.right + dx;
          else SetRectEmpty16( &rect );
          SetRectRgn( hrgn1, rect.left, rect.top, rect.right, rect.bottom );

          rect = rectClip;				/* horizontal band */
          if (dy > 0) rect.bottom = rect.top + dy;
          else if (dy < 0) rect.top = rect.bottom + dy;
          else SetRectEmpty16( &rect );

          RgnType = REGION_UnionRectWithRgn( hrgn1, &rect );
	}

	if (rcUpdate) GetRgnBox16( hrgn1, rcUpdate );
	if (!hrgnUpdate) DeleteObject( hrgn1 );
    }

    /* restore clipping region */

    SelectClipRgn( hdc, (hrgnClip)?hrgnScrollClip:0 );
    DeleteObject( hrgnScrollClip );     

    return TRUE;
}


/*************************************************************************
 *             ScrollWindowEx       (USER.319)
 *
 * FIXME: broken, is there a program that actually uses it?
 *
 */

int ScrollWindowEx(HWND hwnd, short dx, short dy, LPRECT16 rect, LPRECT16 clipRect,
		   HRGN32 hrgnUpdate, LPRECT16 rcUpdate, WORD flags)
{
    HDC32 hdc;
    RECT16 rc, cliprc;

    dprintf_scroll(stddeb,"ScrollWindowEx: dx=%d, dy=%d, wFlags=%04x\n",dx, dy, flags);

    hdc = GetDC32(hwnd);

    if (rect == NULL)
	GetClientRect16(hwnd, &rc);
    else
	CopyRect16(&rc, rect);
    if (clipRect == NULL)
	GetClientRect16(hwnd, &cliprc);
    else
	CopyRect16(&cliprc, clipRect);

    ScrollDC(hdc, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate);

    if (flags | SW_INVALIDATE)
    {
	PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                        ((flags & SW_ERASE) ? RDW_ERASENOW : 0), 0 );
    }

    ReleaseDC32(hwnd, hdc);
    return RgnType;
}

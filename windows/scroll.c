/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995
 *
 *
 */

#include <stdlib.h>
#include "wintypes.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "sysmetrics.h"
#include "stddebug.h"
/* #define DEBUG_SCROLL */
#include "debug.h"



extern HRGN DCE_GetVisRgn(HWND, WORD);
extern HWND CARET_GetHwnd();

static int RgnType;


/* -----------------------------------------------------------------------
 *	       SCROLL_TraceChildren
 * 
 * Returns a region invalidated by children, siblings, and/or ansectors
 * in the window rectangle or client rectangle
 *
 * dcx can have DCX_WINDOW, DCX_CLIPCHILDREN, DCX_CLIPSIBLINGS set
 */

HRGN	SCROLL_TraceChildren( HWND hScroll, short dx, short dy, WORD dcx)
{
 WND	       *wndScroll = WIN_FindWndPtr( hScroll ); 
 HRGN		hRgnWnd;
 HRGN		hUpdateRgn,hCombineRgn;

 if( !wndScroll || ( !dx && !dy) ) return 0;

 if( dcx & DCX_WINDOW )
	 hRgnWnd   = CreateRectRgnIndirect16(&wndScroll->rectWindow);
 else
	{
	 RECT32 rect;

	 GetClientRect32(hScroll,&rect);
 	 hRgnWnd   = CreateRectRgnIndirect32(&rect);
	}

 hUpdateRgn  = DCE_GetVisRgn( hScroll, dcx );
 hCombineRgn = CreateRectRgn(0,0,0,0);

 if( !hUpdateRgn || !hCombineRgn )
      {
	DeleteObject( hUpdateRgn? hUpdateRgn : hCombineRgn);
	DeleteObject(hRgnWnd);
	return 0;
      }

 OffsetRgn( hUpdateRgn, dx, dy);
 CombineRgn(hCombineRgn, hRgnWnd, hUpdateRgn, RGN_DIFF);
 
 DeleteObject(hRgnWnd);
 DeleteObject(hUpdateRgn);

 return hCombineRgn;
}


/* ----------------------------------------------------------------------
 *	       SCROLL_ScrollChildren
 */
BOOL	SCROLL_ScrollChildren( HWND hScroll, short dx, short dy)
{
 WND           *wndPtr = WIN_FindWndPtr(hScroll);
 HRGN		hUpdateRgn;
 BOOL		b = 0;

 if( !wndPtr || ( !dx && !dy )) return 0;

 dprintf_scroll(stddeb,"SCROLL_ScrollChildren: hwnd %04x dx=%i dy=%i\n",hScroll,dx,dy);

 /* get a region in client rect invalidated by siblings and ansectors */
 hUpdateRgn = SCROLL_TraceChildren(hScroll, dx , dy, DCX_CLIPSIBLINGS);

   /* update children coordinates */
   for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
   {
	/* we can check if window intersects with clipRect parameter
	 * and do not move it if not - just a thought.     - AK
	 */
	SetWindowPos(wndPtr->hwndSelf, 0, wndPtr->rectWindow.left + dx,
                     wndPtr->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                     SWP_DEFERERASE );
  } 

 /* invalidate uncovered region and paint frames */
 b = RedrawWindow32( hScroll, NULL, hUpdateRgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE |
				              RDW_ERASENOW | RDW_ALLCHILDREN ); 

 DeleteObject( hUpdateRgn);
 return b;
}


/*************************************************************************
 *             ScrollWindow         (USER.61)
 *
 * FIXME: a bit broken
 */

void ScrollWindow(HWND hwnd, short dx, short dy, LPRECT16 rect, LPRECT16 clipRect)
{
    HDC  hdc;
    HRGN hrgnUpdate,hrgnClip;
    RECT16 rc, cliprc;
    HWND hCaretWnd = CARET_GetHwnd();

    dprintf_scroll(stddeb,"ScrollWindow: dx=%d, dy=%d, lpRect =%08lx clipRect=%i,%i,%i,%i\n", 
	    dx, dy, (LONG)rect, (int)((clipRect)?clipRect->left:0),
                                (int)((clipRect)?clipRect->top:0),
                                (int)((clipRect)?clipRect->right:0), 
                                (int)((clipRect)?clipRect->bottom:0));

    /* if rect is NULL children have to be moved */
    if ( !rect )
       {
	GetClientRect16(hwnd, &rc);
	  hrgnClip = CreateRectRgnIndirect16( &rc );

          if ((hCaretWnd == hwnd) || IsChild(hwnd,hCaretWnd))
              HideCaret(hCaretWnd);
          else hCaretWnd = 0;
 
	  /* children will be Blt'ed too */
	  hdc      = GetDCEx(hwnd, hrgnClip, DCX_CACHE | DCX_CLIPSIBLINGS);
          DeleteObject(hrgnClip);
       }
    else
       {
	  GetClientRect16(hwnd,&rc);
	  dprintf_scroll(stddeb,"\trect=%i %i %i %i client=%i %i %i %i\n",
			 (int)rect->left,(int)rect->top,(int)rect->right,
			 (int)rect->bottom,(int)rc.left,(int)rc.top,
			 (int)rc.right,(int)rc.bottom);

          if (hCaretWnd == hwnd) HideCaret(hCaretWnd);
          else hCaretWnd = 0;
          CopyRect16(&rc, rect);
	  hdc = GetDC(hwnd);
       }

    if (clipRect == NULL)
	GetClientRect16(hwnd, &cliprc);
    else
	CopyRect16(&cliprc, clipRect);

    hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
    ScrollDC(hdc, dx, dy, &rc, &cliprc, hrgnUpdate, NULL);
    ReleaseDC(hwnd, hdc);

    if( !rect )
      {
         /* FIXME: this doesn't take into account hrgnUpdate */
         if( !SCROLL_ScrollChildren(hwnd,dx,dy) )
	     InvalidateRgn(hwnd, hrgnUpdate, TRUE);
      }
    else
      {
        HRGN hrgnInv = SCROLL_TraceChildren(hwnd,dx,dy,DCX_CLIPCHILDREN |
						       DCX_CLIPSIBLINGS );
        if( hrgnInv )
        {
	    CombineRgn(hrgnUpdate,hrgnInv,hrgnUpdate,RGN_OR);
            DeleteObject(hrgnInv);
        }

        RedrawWindow32( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW);
      }

    DeleteObject(hrgnUpdate);
    if( hCaretWnd ) ShowCaret(hCaretWnd);
}


/*************************************************************************
 *             ScrollDC         (USER.221)
 *
 * FIXME: half-broken
 */

BOOL ScrollDC(HDC hdc, short dx, short dy, LPRECT16 rc, LPRECT16 cliprc,
	      HRGN hrgnUpdate, LPRECT16 rcUpdate)
{
    HRGN hrgnClip;
    POINT16 src, dest;
    short width, height;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    dprintf_scroll(stddeb,"ScrollDC: dx=%d dy=%d, hrgnUpdate=%04x rcUpdate = %p cliprc = %p, rc=%d %d %d %d\n",
                   dx, dy, hrgnUpdate, rcUpdate, cliprc, rc ? rc->left : 0,
                   rc ? rc->top : 0, rc ? rc->right : 0, rc ? rc->bottom : 0 );

    if (rc == NULL)
	return FALSE;

    if (!dc) 
    { 
        fprintf(stdnimp,"ScrollDC: Invalid HDC\n");
        return FALSE;
    }

    if (cliprc)
    {
	hrgnClip = CreateRectRgnIndirect16(cliprc);
	SelectClipRgn(hdc, hrgnClip);
    }

    if (dx > 0)
    {
	src.x = XDPTOLP(dc, rc->left);
	dest.x = XDPTOLP(dc, rc->left + abs(dx));
    }
    else
    {
	src.x = XDPTOLP(dc, rc->left + abs(dx));
	dest.x = XDPTOLP(dc, rc->left);
    }
    if (dy > 0)
    {
	src.y = YDPTOLP(dc, rc->top);
	dest.y = YDPTOLP(dc, rc->top + abs(dy));
    }
    else
    {
	src.y = YDPTOLP(dc, rc->top + abs(dy));
	dest.y = YDPTOLP(dc, rc->top);
    }

    width = rc->right - rc->left - abs(dx);
    height = rc->bottom - rc->top - abs(dy);

    if (!BitBlt(hdc, dest.x, dest.y, width, height, hdc, src.x, src.y, 
		SRCCOPY))
	return FALSE;

    if (hrgnUpdate)
    {
	HRGN hrgn1,hrgn2;

	if (dx > 0)
	    hrgn1 = CreateRectRgn(rc->left, rc->top, rc->left+dx, rc->bottom);
	else if (dx < 0)
	    hrgn1 = CreateRectRgn(rc->right+dx, rc->top, rc->right, 
				  rc->bottom);
	else
	    hrgn1 = CreateRectRgn(0, 0, 0, 0);

	if (dy > 0)
	    hrgn2 = CreateRectRgn(rc->left, rc->top, rc->right, rc->top+dy);
	else if (dy < 0)
	    hrgn2 = CreateRectRgn(rc->left, rc->bottom+dy, rc->right, 
				  rc->bottom);
	else
	    hrgn2 = CreateRectRgn(0, 0, 0, 0);

	RgnType = CombineRgn(hrgnUpdate, hrgn1, hrgn2, RGN_OR);
	DeleteObject(hrgn1);
	DeleteObject(hrgn2);
        if (rcUpdate) GetRgnBox16( hrgnUpdate, rcUpdate );
    }
    else if (rcUpdate)
    {
	RECT16 rx,ry;

	rx = ry = *rc;
	if( dx > 0 )  	  rx.right = rc->left+dx; 
	else if (dx < 0)  rx.left = rc->right+dx; 
	else SetRectEmpty16( &rx );

        if( dy > 0 )      ry.bottom = rc->top+dy;
        else if (dy < 0)  ry.top = rc->bottom+dy;
        else SetRectEmpty16( &ry );

	UnionRect16( rcUpdate, &rx, &ry );
    }

    return TRUE;
}


/*************************************************************************
 *             ScrollWindowEx       (USER.319)
 *
 * FIXME: broken
 *
 * SCROLL_TraceChildren can help
 */

int ScrollWindowEx(HWND hwnd, short dx, short dy, LPRECT16 rect, LPRECT16 clipRect,
		   HRGN hrgnUpdate, LPRECT16 rcUpdate, WORD flags)
{
    HDC hdc;
    RECT16 rc, cliprc;

    dprintf_scroll(stddeb,"ScrollWindowEx: dx=%d, dy=%d, wFlags=%04x\n",dx, dy, flags);

    hdc = GetDC(hwnd);

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
	RedrawWindow32( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                        ((flags & SW_ERASE) ? RDW_ERASENOW : 0));
    }

    ReleaseDC(hwnd, hdc);
    return RgnType;
}

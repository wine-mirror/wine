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



extern DCE_GetVisRgn(HWND, WORD);

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
	 hRgnWnd   = CreateRectRgnIndirect(&wndScroll->rectWindow);
 else
	{
	 RECT rect;

	 GetClientRect(hScroll,&rect);
 	 hRgnWnd   = CreateRectRgnIndirect(&rect);
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
 HWND		hWnd   = wndPtr->hwndChild;
 HRGN		hUpdateRgn;
 BOOL		b = 0;

 if( !wndPtr || ( !dx && !dy )) return 0;

 dprintf_scroll(stddeb,"SCROLL_ScrollChildren: hwnd "NPFMT" dx=%i dy=%i\n",hScroll,dx,dy);

 /* get a region in client rect invalidated by siblings and ansectors */
 hUpdateRgn = SCROLL_TraceChildren(hScroll, dx , dy, DCX_CLIPSIBLINGS);

 /* update children coordinates */
 while( hWnd )
  {
	wndPtr = WIN_FindWndPtr( hWnd );

	/* we can check if window intersects with clipRect parameter
	 * and do not move it if not - just a thought.     - AK
	 */

	SetWindowPos(hWnd,0,wndPtr->rectWindow.left + dx,
			    wndPtr->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
			    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
			    SWP_DEFERERASE );

	hWnd = wndPtr->hwndNext;
  } 

 /* invalidate uncovered region and paint frames */
 b = RedrawWindow( hScroll, NULL, hUpdateRgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE |
				              RDW_ERASENOW | RDW_ALLCHILDREN ); 

 DeleteObject( hUpdateRgn);
 return b;
}


/*************************************************************************
 *             ScrollWindow         (USER.61)
 *
 * FIXME: a bit broken
 */

void ScrollWindow(HWND hwnd, short dx, short dy, LPRECT rect, LPRECT clipRect)
{
    HDC hdc;
    HRGN hrgnUpdate,hrgnClip;
    RECT rc, cliprc;

    dprintf_scroll(stddeb,"ScrollWindow: dx=%d, dy=%d, lpRect =%08lx clipRect=%i,%i,%i,%i\n", 
	    dx, dy, (LONG)rect, (clipRect)?clipRect->left:0,
                                (clipRect)?clipRect->top:0,
                                (clipRect)?clipRect->right:0, 
                                (clipRect)?clipRect->bottom:0);

    /* if rect is NULL children have to be moved */
    if ( !rect )
       {
	GetClientRect(hwnd, &rc);
	  hrgnClip = CreateRectRgnIndirect( &rc );

	  /* children will be Blt'ed too */
	  hdc      = GetDCEx(hwnd, hrgnClip, DCX_CACHE | DCX_CLIPSIBLINGS);
          DeleteObject(hrgnClip);
       }
    else
       {
	  GetClientRect(hwnd,&rc);
	  dprintf_scroll(stddeb,"\trect=%i %i %i %i client=%i %i %i %i\n",
			 rect->left,rect->top,rect->right,rect->bottom,rc.left,rc.top,
			 rc.right,rc.bottom);

	CopyRect(&rc, rect);
	  hdc = GetDC(hwnd);
       }

    if (clipRect == NULL)
	GetClientRect(hwnd, &cliprc);
    else
	CopyRect(&cliprc, clipRect);

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
	    HRGN hrgnCombine = CreateRectRgn(0,0,0,0);

	    CombineRgn(hrgnCombine,hrgnInv,hrgnUpdate,RGN_OR);
	    dprintf_scroll(stddeb,"ScrollWindow: hrgnComb="NPFMT" hrgnInv="NPFMT" hrgnUpd="NPFMT"\n",
	 					           hrgnCombine,hrgnInv,hrgnUpdate);

	    DeleteObject(hrgnUpdate); DeleteObject(hrgnInv);
	    hrgnUpdate = hrgnCombine;
	  }

        RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW);
      }

    DeleteObject(hrgnUpdate);
}


/*************************************************************************
 *             ScrollDC         (USER.221)
 *
 * FIXME: half-broken
 */

BOOL ScrollDC(HDC hdc, short dx, short dy, LPRECT rc, LPRECT cliprc,
	      HRGN hrgnUpdate, LPRECT rcUpdate)
{
    HRGN hrgnClip, hrgn1, hrgn2;
    POINT src, dest;
    short width, height;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    dprintf_scroll(stddeb,"ScrollDC: dx=%d dy=%d, hrgnUpdate="NPFMT" rc=%i %i %i %i\n",
                                     dx,dy,hrgnUpdate,(rc)?rc->left:0,
				                      (rc)?rc->top:0,
				                      (rc)?rc->right:0,
				                      (rc)?rc->bottom:0); 

    if (rc == NULL)
	return FALSE;

    if (cliprc)
    {
	hrgnClip = CreateRectRgnIndirect(cliprc);
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
    }

    if (rcUpdate) GetRgnBox( hrgnUpdate, rcUpdate );
    return TRUE;
}


/*************************************************************************
 *             ScrollWindowEx       (USER.319)
 *
 * FIXME: broken
 *
 * SCROLL_TraceChildren can help
 */

int ScrollWindowEx(HWND hwnd, short dx, short dy, LPRECT rect, LPRECT clipRect,
		   HRGN hrgnUpdate, LPRECT rcUpdate, WORD flags)
{
    HDC hdc;
    RECT rc, cliprc;

    dprintf_scroll(stddeb,"ScrollWindowEx: dx=%d, dy=%d, wFlags="NPFMT"\n",dx, dy, flags);

    hdc = GetDC(hwnd);

    if (rect == NULL)
	GetClientRect(hwnd, &rc);
    else
	CopyRect(&rc, rect);
    if (clipRect == NULL)
	GetClientRect(hwnd, &cliprc);
    else
	CopyRect(&cliprc, clipRect);

    ScrollDC(hdc, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate);

    if (flags | SW_INVALIDATE)
    {
	RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                      ((flags & SW_ERASE) ? RDW_ERASENOW : 0));
    }

    ReleaseDC(hwnd, hdc);
    return RgnType;
}

/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *			 1999 Alex Korobka
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
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "user.h"
#include "win.h"
#include "message.h"
#include "dce.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(nonclient);

/* client rect in window coordinates */

#define GETCLIENTRECTW( wnd, r )	(r).left = (wnd)->rectClient.left - (wnd)->rectWindow.left; \
					(r).top = (wnd)->rectClient.top - (wnd)->rectWindow.top; \
					(r).right = (wnd)->rectClient.right - (wnd)->rectWindow.left; \
					(r).bottom = (wnd)->rectClient.bottom - (wnd)->rectWindow.top

  /* PAINT_RedrawWindow() control flags */
#define RDW_EX_DELAY_NCPAINT    0x0020

  /* WIN_UpdateNCRgn() flags */
#define UNC_CHECK		0x0001
#define UNC_ENTIRE		0x0002
#define UNC_REGION		0x0004
#define UNC_UPDATE		0x0008
#define UNC_DELAY_NCPAINT       0x0010
#define UNC_IN_BEGINPAINT       0x0020

  /* Last COLOR id */
#define COLOR_MAX   COLOR_GRADIENTINACTIVECAPTION

HPALETTE (WINAPI *pfnGDISelectPalette)(HDC hdc, HPALETTE hpal, WORD bkgnd ) = NULL;
UINT (WINAPI *pfnGDIRealizePalette)(HDC hdc) = NULL;


/***********************************************************************
 *           add_paint_count
 *
 * Add an increment (normally 1 or -1) to the current paint count of a window.
 */
static void add_paint_count( HWND hwnd, int incr )
{
    SERVER_START_REQ( inc_window_paint_count )
    {
        req->handle = hwnd;
        req->incr   = incr;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           crop_rgn
 *
 * hSrc: 	Region to crop.
 * lpRect: 	Clipping rectangle.
 *
 * hDst: Region to hold the result (a new region is created if it's 0).
 *       Allowed to be the same region as hSrc in which case everything
 *	 will be done in place, with no memory reallocations.
 *
 * Returns: hDst if success, 0 otherwise.
 */
static HRGN crop_rgn( HRGN hDst, HRGN hSrc, const RECT *rect )
{
    HRGN h = CreateRectRgnIndirect( rect );
    if (hDst == 0) hDst = h;
    CombineRgn( hDst, hSrc, h, RGN_AND );
    if (hDst != h) DeleteObject( h );
    return hDst;
}


/***********************************************************************
 *           WIN_HaveToDelayNCPAINT
 *
 * Currently, in the Wine painting mechanism, the WM_NCPAINT message
 * is generated as soon as a region intersecting the non-client area
 * of a window is invalidated.
 *
 * This technique will work fine for all windows whose parents
 * have the WS_CLIPCHILDREN style. When the parents have that style,
 * they are not going to override the contents of their children.
 * However, when the parent doesn't have that style, Windows relies
 * on a "painter's algorithm" to display the contents of the windows.
 * That is, windows are painted from back to front. This includes the
 * non-client area.
 *
 * This method looks at the current state of a window to determine
 * if the sending of the WM_NCPAINT message should be delayed until
 * the BeginPaint call.
 *
 * PARAMS:
 *   wndPtr   - A Locked window pointer to the window we're
 *              examining.
 *   uncFlags - This is the flag passed to the WIN_UpdateNCRgn
 *              function. This is a shortcut for the cases when
 *              we already know when to avoid scanning all the
 *              parents of a window. If you already know that this
 *              window's NCPAINT should be delayed, set the
 *              UNC_DELAY_NCPAINT flag for this parameter.
 *
 *              This shortcut behavior is implemented in the
 *              RDW_Paint() method.
 *
 */
static BOOL WIN_HaveToDelayNCPAINT( HWND hwnd, UINT uncFlags)
{
  /*
   * Test the shortcut first. (duh)
   */
  if (uncFlags & UNC_DELAY_NCPAINT)
    return TRUE;

  /*
   * The UNC_IN_BEGINPAINT flag is set in the BeginPaint
   * method only. This is another shortcut to avoid going
   * up the parent's chain of the window to finally
   * figure-out that none of them has an invalid region.
   */
  if (uncFlags & UNC_IN_BEGINPAINT)
    return FALSE;

  /*
   * Scan all the parents of this window to find a window
   * that doesn't have the WS_CLIPCHILDREN style and that
   * has an invalid region.
   */
  while ((hwnd = GetAncestor( hwnd, GA_PARENT )))
  {
      WND* parentWnd = WIN_FindWndPtr( hwnd );
      if (parentWnd && !(parentWnd->dwStyle & WS_CLIPCHILDREN) && parentWnd->hrgnUpdate)
      {
          WIN_ReleaseWndPtr( parentWnd );
          return TRUE;
      }
      WIN_ReleaseWndPtr( parentWnd );
  }
  return FALSE;
}

/***********************************************************************
 *           WIN_UpdateNCRgn
 *
 *  Things to do:
 *	Send WM_NCPAINT if required (when nonclient is invalid or UNC_ENTIRE flag is set)
 *	Crop hrgnUpdate to a client rect, especially if it 1.
 *	If UNC_REGION is set return update region for the client rect.
 *
 *  NOTE: UNC_REGION is mainly for the RDW_Paint() chunk that sends WM_ERASEBKGND message.
 *	  The trick is that when the returned region handle may be different from hRgn.
 *	  In this case the old hRgn must be considered gone. BUT, if the returned value
 *	  is 1 then the hRgn is preserved and RDW_Paint() will have to get
 *	  a DC without extra clipping region.
 */
static HRGN WIN_UpdateNCRgn(WND* wnd, HRGN hRgn, UINT uncFlags )
{
    RECT  r;
    HRGN  hClip = 0;
    HRGN  hrgnRet = 0;

    TRACE_(nonclient)("hwnd %p [%p] hrgn %p, unc %04x, ncf %i\n",
                      wnd->hwndSelf, wnd->hrgnUpdate, hRgn, uncFlags, wnd->flags & WIN_NEEDS_NCPAINT);

    /* desktop window doesn't have a nonclient area */
    if(wnd->hwndSelf == GetDesktopWindow())
    {
        wnd->flags &= ~WIN_NEEDS_NCPAINT;
	if( wnd->hrgnUpdate > (HRGN)1 )
        {
            if (!hRgn) hRgn = CreateRectRgn( 0, 0, 0, 0 );
            CombineRgn( hRgn, wnd->hrgnUpdate, 0, RGN_COPY );
            hrgnRet = hRgn;
        }
	else
	{
	    hrgnRet = wnd->hrgnUpdate;
	}
        return hrgnRet;
    }

    if ((wnd->hwndSelf == GetForegroundWindow()) &&
        !(wnd->flags & WIN_NCACTIVATED) )
    {
	wnd->flags |= WIN_NCACTIVATED;
	uncFlags |= UNC_ENTIRE;
    }

    /*
     * If the window's non-client area needs to be painted,
     */
    if ( ( wnd->flags & WIN_NEEDS_NCPAINT ) &&
	 !WIN_HaveToDelayNCPAINT(wnd->hwndSelf, uncFlags) )
    {
	    RECT r2, r3;

	    wnd->flags &= ~WIN_NEEDS_NCPAINT;
	    GETCLIENTRECTW( wnd, r );

	    TRACE_(nonclient)( "\tclient box (%ld,%ld-%ld,%ld), hrgnUpdate %p\n",
				r.left, r.top, r.right, r.bottom, wnd->hrgnUpdate );
	    if( wnd->hrgnUpdate > (HRGN)1 )
	    {
		/* Check if update rgn overlaps with nonclient area */

		GetRgnBox( wnd->hrgnUpdate, &r2 );
		UnionRect( &r3, &r2, &r );
		if( r3.left != r.left || r3.top != r.top ||
		    r3.right != r.right || r3.bottom != r.bottom ) /* it does */
		{
		    /* crop hrgnUpdate, save old one in hClip - the only
		     * case that places a valid region handle in hClip */

		    hClip = wnd->hrgnUpdate;
		    wnd->hrgnUpdate = crop_rgn( hRgn, hClip, &r );
		    if( uncFlags & UNC_REGION ) hrgnRet = hClip;
		}

		if( uncFlags & UNC_CHECK )
		{
		    GetRgnBox( wnd->hrgnUpdate, &r3 );
		    if( IsRectEmpty( &r3 ) )
		    {
			/* delete the update region since all invalid
			 * parts were in the nonclient area */

			DeleteObject( wnd->hrgnUpdate );
			wnd->hrgnUpdate = 0;
			if(!(wnd->flags & WIN_INTERNAL_PAINT))
			    add_paint_count( wnd->hwndSelf, -1 );

			wnd->flags &= ~WIN_NEEDS_ERASEBKGND;
		    }
		}

		if(!hClip && wnd->hrgnUpdate ) goto copyrgn;
	    }
	    else
	    if( wnd->hrgnUpdate == (HRGN)1 )/* entire window */
	    {
		if( uncFlags & UNC_UPDATE ) wnd->hrgnUpdate = CreateRectRgnIndirect( &r );
		if( uncFlags & UNC_REGION ) hrgnRet = (HRGN)1;
		uncFlags |= UNC_ENTIRE;
	    }
    }
    else /* no WM_NCPAINT unless forced */
    {
	if( wnd->hrgnUpdate >  (HRGN)1 )
	{
copyrgn:
	    if( uncFlags & UNC_REGION )
            {
                if (!hRgn) hRgn = CreateRectRgn( 0, 0, 0, 0 );
                CombineRgn( hRgn, wnd->hrgnUpdate, 0, RGN_COPY );
                hrgnRet = hRgn;
            }
	}
	else
	if( wnd->hrgnUpdate == (HRGN)1 && (uncFlags & UNC_UPDATE) )
	{
	    GETCLIENTRECTW( wnd, r );
	    wnd->hrgnUpdate = CreateRectRgnIndirect( &r );
	    if( uncFlags & UNC_REGION ) hrgnRet = (HRGN)1;
	}
    }

    if(!hClip && (uncFlags & UNC_ENTIRE) )
    {
	/* still don't do anything if there is no nonclient area */
	hClip = (HRGN)(memcmp( &wnd->rectWindow, &wnd->rectClient, sizeof(RECT) ) != 0);
    }

    if( hClip ) /* NOTE: WM_NCPAINT allows wParam to be 1 */
    {
        if ( hClip == hrgnRet && hrgnRet > (HRGN)1 ) {
	    hClip = CreateRectRgn( 0, 0, 0, 0 );
	    CombineRgn( hClip, hrgnRet, 0, RGN_COPY );
	}

	SendMessageA( wnd->hwndSelf, WM_NCPAINT, (WPARAM)hClip, 0L );
	if( (hClip > (HRGN)1) && (hClip != hRgn) && (hClip != hrgnRet) )
	    DeleteObject( hClip );
	/*
         * Since all Window locks are suspended while processing the WM_NCPAINT
         * we want to make sure the window still exists before continuing.
	 */
        if (!IsWindow(wnd->hwndSelf))
        {
	  DeleteObject(hrgnRet);
	  hrgnRet=0;
        }
    }

    TRACE_(nonclient)("returning %p (hClip = %p, hrgnUpdate = %p)\n", hrgnRet, hClip, wnd->hrgnUpdate );

    return hrgnRet;
}


/***********************************************************************
 * 		RDW_ValidateParent [RDW_UpdateRgns() helper]
 *
 *  Validate the portions of parents that are covered by a validated child
 *  wndPtr = child
 */
static void RDW_ValidateParent(WND *wndChild)
{
    HWND parent;
    HRGN hrg;

    if (wndChild->hrgnUpdate == (HRGN)1 ) {
        RECT r;
        r.left = 0;
        r.top = 0;
        r.right = wndChild->rectWindow.right - wndChild->rectWindow.left;
        r.bottom = wndChild->rectWindow.bottom - wndChild->rectWindow.top;
        hrg = CreateRectRgnIndirect( &r );
    } else
        hrg = wndChild->hrgnUpdate;

    parent = GetAncestor( wndChild->hwndSelf, GA_PARENT );
    while (parent && parent != GetDesktopWindow())
    {
        WND *wndParent = WIN_FindWndPtr( parent );
        if (wndParent && !(wndParent->dwStyle & WS_CLIPCHILDREN))
        {
            if (wndParent->hrgnUpdate != 0)
            {
                POINT ptOffset;
                RECT rect, rectParent;
                if( wndParent->hrgnUpdate == (HRGN)1 )
                {
                   RECT r;

                   r.left = 0;
                   r.top = 0;
                   r.right = wndParent->rectWindow.right - wndParent->rectWindow.left;
                   r.bottom = wndParent->rectWindow.bottom - wndParent->rectWindow.top;

                   wndParent->hrgnUpdate = CreateRectRgnIndirect( &r );
                }
                /* we must offset the child region by the offset of the child rect in the parent */
                GetWindowRect(wndParent->hwndSelf, &rectParent);
                GetWindowRect(wndChild->hwndSelf, &rect);
                ptOffset.x = rect.left - rectParent.left;
                ptOffset.y = rect.top - rectParent.top;
                OffsetRgn( hrg, ptOffset.x, ptOffset.y );
                if (CombineRgn( wndParent->hrgnUpdate, wndParent->hrgnUpdate, hrg, RGN_DIFF ) == NULLREGION)
                {
                    /* the update region has become empty */
                    DeleteObject( wndParent->hrgnUpdate );
                    wndParent->hrgnUpdate = 0;
                    wndParent->flags &= ~WIN_NEEDS_ERASEBKGND;
                    if( !(wndParent->flags & WIN_INTERNAL_PAINT) )
                        add_paint_count( wndParent->hwndSelf, -1 );
                }
                OffsetRgn( hrg, -ptOffset.x, -ptOffset.y );
            }
        }
        WIN_ReleaseWndPtr( wndParent );
        parent = GetAncestor( parent, GA_PARENT );
    }
    if (hrg != wndChild->hrgnUpdate) DeleteObject( hrg );
}

/***********************************************************************
 * 		RDW_UpdateRgns [RedrawWindow() helper]
 *
 *  Walks the window tree and adds/removes parts of the hRgn to/from update
 *  regions of windows that overlap it. Also, manages internal paint flags.
 *
 *  NOTE: Walks the window tree so the caller must lock it.
 *	  MUST preserve hRgn (can modify but then has to restore).
 */
static void RDW_UpdateRgns( WND* wndPtr, HRGN hRgn, UINT flags, BOOL firstRecursLevel )
{
    /*
     * Called only when one of the following is set:
     * (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT)
     */

    BOOL bHadOne =  wndPtr->hrgnUpdate && hRgn;
    BOOL bChildren =  (!(flags & RDW_NOCHILDREN) && !(wndPtr->dwStyle & WS_MINIMIZE) &&
                       ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) );
    RECT r;

    r.left = 0;
    r.top = 0;
    r.right = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    r.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    TRACE("\thwnd %p [%p] -> hrgn [%p], flags [%04x]\n", wndPtr->hwndSelf, wndPtr->hrgnUpdate, hRgn, flags );

    if( flags & RDW_INVALIDATE )
    {
	if( hRgn > (HRGN)1 )
	{
	    switch ((UINT)wndPtr->hrgnUpdate)
	    {
		default:
			CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hRgn, RGN_OR );
			/* fall through */
		case 0:
			wndPtr->hrgnUpdate = crop_rgn( wndPtr->hrgnUpdate,
                                                       wndPtr->hrgnUpdate ? wndPtr->hrgnUpdate : hRgn,
                                                       &r );
			if( !bHadOne )
			{
			    GetRgnBox( wndPtr->hrgnUpdate, &r );
			    if( IsRectEmpty( &r ) )
			    {
				DeleteObject( wndPtr->hrgnUpdate );
				wndPtr->hrgnUpdate = 0;
			        goto end;
			    }
			}
			break;
		case 1:	/* already an entire window */
		        break;
	    }
	}
	else if( hRgn == (HRGN)1 )
	{
	    if( wndPtr->hrgnUpdate > (HRGN)1 )
		DeleteObject( wndPtr->hrgnUpdate );
	    wndPtr->hrgnUpdate = (HRGN)1;
	}
	else
	    hRgn = wndPtr->hrgnUpdate;	/* this is a trick that depends on code in PAINT_RedrawWindow() */

	if( !bHadOne && !(wndPtr->flags & WIN_INTERNAL_PAINT) )
            add_paint_count( wndPtr->hwndSelf, 1 );

	if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;
	if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags    |= RDW_FRAME;
    }
    else if( flags & RDW_VALIDATE )
    {
	if( wndPtr->hrgnUpdate )
	{
	    if( hRgn > (HRGN)1 )
	    {
		if( wndPtr->hrgnUpdate == (HRGN)1 )
		    wndPtr->hrgnUpdate = CreateRectRgnIndirect( &r );

		if( CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hRgn, RGN_DIFF )
		    == NULLREGION )
                {
                    DeleteObject( wndPtr->hrgnUpdate );
                    wndPtr->hrgnUpdate = 0;
                }
	    }
	    else /* validate everything */
	    {
		if( wndPtr->hrgnUpdate > (HRGN)1 ) DeleteObject( wndPtr->hrgnUpdate );
		wndPtr->hrgnUpdate = 0;
	    }

	    if( !wndPtr->hrgnUpdate )
	    {
		wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
		if( !(wndPtr->flags & WIN_INTERNAL_PAINT) )
                    add_paint_count( wndPtr->hwndSelf, -1 );
	    }
	}

	if (flags & RDW_NOFRAME) wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
	if (flags & RDW_NOERASE) wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;

    }

    if ((firstRecursLevel) && (wndPtr->hrgnUpdate != 0) && (flags & RDW_UPDATENOW))
        RDW_ValidateParent(wndPtr); /* validate parent covered by region */

    /* in/validate child windows that intersect with the region if it
     * is a valid handle. */

    if( flags & (RDW_INVALIDATE | RDW_VALIDATE) )
    {
        HWND *list;
	if( hRgn > (HRGN)1 && bChildren && (list = WIN_ListChildren( wndPtr->hwndSelf )))
	{
	    POINT ptTotal, prevOrigin = {0,0};
            POINT ptClient;
            INT i;

            ptClient.x = wndPtr->rectClient.left - wndPtr->rectWindow.left;
            ptClient.y = wndPtr->rectClient.top - wndPtr->rectWindow.top;

            for(i = ptTotal.x = ptTotal.y = 0; list[i]; i++)
            {
                WND *wnd = WIN_FindWndPtr( list[i] );
                if (!wnd) continue;
                if( wnd->dwStyle & WS_VISIBLE )
                {
                    POINT ptOffset;

                    r.left = wnd->rectWindow.left + ptClient.x;
                    r.right = wnd->rectWindow.right + ptClient.x;
                    r.top = wnd->rectWindow.top + ptClient.y;
                    r.bottom = wnd->rectWindow.bottom + ptClient.y;

                    ptOffset.x = r.left - prevOrigin.x;
                    ptOffset.y = r.top - prevOrigin.y;
                    OffsetRect( &r, -ptTotal.x, -ptTotal.y );

                    if( RectInRegion( hRgn, &r ) )
                    {
                        OffsetRgn( hRgn, -ptOffset.x, -ptOffset.y );
                        RDW_UpdateRgns( wnd, hRgn, flags, FALSE );
                        prevOrigin.x = r.left + ptTotal.x;
                        prevOrigin.y = r.top + ptTotal.y;
                        ptTotal.x += ptOffset.x;
                        ptTotal.y += ptOffset.y;
                    }
                }
                WIN_ReleaseWndPtr( wnd );
            }
            HeapFree( GetProcessHeap(), 0, list );
            OffsetRgn( hRgn, ptTotal.x, ptTotal.y );
            bChildren = 0;
	}
    }

    /* handle hRgn == 1 (alias for entire window) and/or internal paint recursion */

    if( bChildren )
    {
        HWND *list;
        if ((list = WIN_ListChildren( wndPtr->hwndSelf )))
        {
            INT i;
            for (i = 0; list[i]; i++)
            {
                WND *wnd = WIN_FindWndPtr( list[i] );
                if (!wnd) continue;
                if( wnd->dwStyle & WS_VISIBLE )
                    RDW_UpdateRgns( wnd, hRgn, flags, FALSE );
                WIN_ReleaseWndPtr( wnd );
            }
            HeapFree( GetProcessHeap(), 0, list );
        }
    }

end:

    /* Set/clear internal paint flag */

    if (flags & RDW_INTERNALPAINT)
    {
        if ( !wndPtr->hrgnUpdate && !(wndPtr->flags & WIN_INTERNAL_PAINT))
            add_paint_count( wndPtr->hwndSelf, 1 );
        wndPtr->flags |= WIN_INTERNAL_PAINT;
    }
    else if (flags & RDW_NOINTERNALPAINT)
    {
        if ( !wndPtr->hrgnUpdate && (wndPtr->flags & WIN_INTERNAL_PAINT))
            add_paint_count( wndPtr->hwndSelf, -1 );
        wndPtr->flags &= ~WIN_INTERNAL_PAINT;
    }
}

/***********************************************************************
 *           RDW_Paint [RedrawWindow() helper]
 *
 * Walks the window tree and paints/erases windows that have
 * nonzero update regions according to redraw flags. hrgn is a scratch
 * region passed down during recursion. Must not be 1.
 *
 */
static HRGN RDW_Paint( WND* wndPtr, HRGN hrgn, UINT flags, UINT ex )
{
/* NOTE: wndPtr is locked by caller.
 *
 * FIXME: Windows uses WM_SYNCPAINT to cut down the number of intertask
 * SendMessage() calls. This is a comment inside DefWindowProc() source
 * from 16-bit SDK:
 *
 *   This message avoids lots of inter-app message traffic
 *   by switching to the other task and continuing the
 *   recursion there.
 *
 * wParam         = flags
 * LOWORD(lParam) = hrgnClip
 * HIWORD(lParam) = hwndSkip  (not used; always NULL)
 *
 */
    HDC  hDC;
    HWND hWnd = wndPtr->hwndSelf;
    BOOL bIcon = ((wndPtr->dwStyle & WS_MINIMIZE) && GetClassLongA(hWnd, GCL_HICON));

      /* Erase/update the window itself ... */

    TRACE("\thwnd %p [%p] -> hrgn [%p], flags [%04x]\n", hWnd, wndPtr->hrgnUpdate, hrgn, flags );

    /*
     * Check if this window should delay it's processing of WM_NCPAINT.
     * See WIN_HaveToDelayNCPAINT for a description of the mechanism
     */
    if ((ex & RDW_EX_DELAY_NCPAINT) || WIN_HaveToDelayNCPAINT(wndPtr->hwndSelf, 0) )
	ex |= RDW_EX_DELAY_NCPAINT;

    if (flags & RDW_UPDATENOW)
    {
        if (wndPtr->hrgnUpdate) /* wm_painticon wparam is 1 */
            SendMessageW( hWnd, (bIcon) ? WM_PAINTICON : WM_PAINT, bIcon, 0 );
    }
    else if (flags & RDW_ERASENOW)
    {
	UINT dcx = DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN | DCX_WINDOWPAINT | DCX_CACHE;
	HRGN hrgnRet;

	hrgnRet = WIN_UpdateNCRgn(wndPtr,
				  hrgn,
				  UNC_REGION | UNC_CHECK |
				  ((ex & RDW_EX_DELAY_NCPAINT) ? UNC_DELAY_NCPAINT : 0) );

        if( hrgnRet )
	{
	    if( hrgnRet > (HRGN)1 ) hrgn = hrgnRet; else hrgnRet = 0; /* entire client */
	    if( wndPtr->flags & WIN_NEEDS_ERASEBKGND )
	    {
		if( bIcon ) dcx |= DCX_WINDOW;
		else
		if( hrgnRet )
		    OffsetRgn( hrgnRet, wndPtr->rectWindow.left - wndPtr->rectClient.left,
			                wndPtr->rectWindow.top  - wndPtr->rectClient.top);
		else
		    dcx &= ~DCX_INTERSECTRGN;
		if (( hDC = GetDCEx( hWnd, hrgnRet, dcx )) )
		{
		    if (SendMessageW( hWnd, (bIcon) ? WM_ICONERASEBKGND : WM_ERASEBKGND,
                                      (WPARAM)hDC, 0 ))
		    wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
		    ReleaseDC( hWnd, hDC );
		}
            }
        }
    }

    if( !IsWindow(hWnd) ) return hrgn;

      /* ... and its child windows */

    if(!(flags & RDW_NOCHILDREN) && !(wndPtr->dwStyle & WS_MINIMIZE) &&
       ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) )
    {
        HWND *list, *phwnd;

	if( (list = WIN_ListChildren( wndPtr->hwndSelf )) )
	{
	    for (phwnd = list; *phwnd; phwnd++)
	    {
                if (!(wndPtr = WIN_FindWndPtr( *phwnd ))) continue;
                if ( (wndPtr->dwStyle & WS_VISIBLE) &&
                     (wndPtr->hrgnUpdate || (wndPtr->flags & WIN_INTERNAL_PAINT)) )
                    hrgn = RDW_Paint( wndPtr, hrgn, flags, ex );
                WIN_ReleaseWndPtr(wndPtr);
	    }
            HeapFree( GetProcessHeap(), 0, list );
	}
    }

    return hrgn;
}


/***********************************************************************
 *           dump_rdw_flags
 */
static void dump_rdw_flags(UINT flags)
{
    TRACE("flags:");
    if (flags & RDW_INVALIDATE) TRACE(" RDW_INVALIDATE");
    if (flags & RDW_INTERNALPAINT) TRACE(" RDW_INTERNALPAINT");
    if (flags & RDW_ERASE) TRACE(" RDW_ERASE");
    if (flags & RDW_VALIDATE) TRACE(" RDW_VALIDATE");
    if (flags & RDW_NOINTERNALPAINT) TRACE(" RDW_NOINTERNALPAINT");
    if (flags & RDW_NOERASE) TRACE(" RDW_NOERASE");
    if (flags & RDW_NOCHILDREN) TRACE(" RDW_NOCHILDREN");
    if (flags & RDW_ALLCHILDREN) TRACE(" RDW_ALLCHILDREN");
    if (flags & RDW_UPDATENOW) TRACE(" RDW_UPDATENOW");
    if (flags & RDW_ERASENOW) TRACE(" RDW_ERASENOW");
    if (flags & RDW_FRAME) TRACE(" RDW_FRAME");
    if (flags & RDW_NOFRAME) TRACE(" RDW_NOFRAME");

#define RDW_FLAGS \
    (RDW_INVALIDATE | \
    RDW_INTERNALPAINT | \
    RDW_ERASE | \
    RDW_VALIDATE | \
    RDW_NOINTERNALPAINT | \
    RDW_NOERASE | \
    RDW_NOCHILDREN | \
    RDW_ALLCHILDREN | \
    RDW_UPDATENOW | \
    RDW_ERASENOW | \
    RDW_FRAME | \
    RDW_NOFRAME)

    if (flags & ~RDW_FLAGS) TRACE(" %04x", flags & ~RDW_FLAGS);
    TRACE("\n");
#undef RDW_FLAGS
}


/***********************************************************************
 *		RedrawWindow (USER32.@)
 */
BOOL WINAPI RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                              HRGN hrgnUpdate, UINT flags )
{
    HRGN hRgn = 0;
    RECT r, r2;
    POINT pt;
    WND* wndPtr;

    if (!hwnd) hwnd = GetDesktopWindow();

    /* check if the window or its parents are visible/not minimized */

    if (!WIN_IsWindowDrawable( hwnd, !(flags & RDW_FRAME) )) return TRUE;

    /* process pending events and messages before painting */
    if (flags & RDW_UPDATENOW)
        MsgWaitForMultipleObjects( 0, NULL, FALSE, 0, QS_ALLINPUT );

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (TRACE_ON(win))
    {
	if( hrgnUpdate )
	{
	    GetRgnBox( hrgnUpdate, &r );
            TRACE( "%p (%p) NULL %p box (%ld,%ld-%ld,%ld) flags=%04x\n",
	          hwnd, wndPtr->hrgnUpdate, hrgnUpdate, r.left, r.top, r.right, r.bottom, flags );
	}
	else
	{
	    if( rectUpdate )
		r = *rectUpdate;
	    else
		SetRectEmpty( &r );
	    TRACE( "%p (%p) %s %ld,%ld-%ld,%ld %p flags=%04x\n",
                   hwnd, wndPtr->hrgnUpdate, rectUpdate ? "rect" : "NULL", r.left,
                   r.top, r.right, r.bottom, hrgnUpdate, flags );
	}
        dump_rdw_flags(flags);
    }

    /* prepare an update region in window coordinates */

    if (((flags & (RDW_INVALIDATE|RDW_FRAME)) == (RDW_INVALIDATE|RDW_FRAME)) ||
        ((flags & (RDW_VALIDATE|RDW_NOFRAME)) == (RDW_VALIDATE|RDW_NOFRAME)))
	r = wndPtr->rectWindow;
    else
	r = wndPtr->rectClient;

    pt.x = wndPtr->rectClient.left - wndPtr->rectWindow.left;
    pt.y = wndPtr->rectClient.top - wndPtr->rectWindow.top;
    OffsetRect( &r, -wndPtr->rectClient.left, -wndPtr->rectClient.top );

    if (flags & RDW_INVALIDATE)  /* ------------------------- Invalidate */
    {
	/* If the window doesn't have hrgnUpdate we leave hRgn zero
	 * and put a new region straight into wndPtr->hrgnUpdate
	 * so that RDW_UpdateRgns() won't have to do any extra work.
	 */

	if( hrgnUpdate )
	{
	    if( wndPtr->hrgnUpdate )
            {
                hRgn = CreateRectRgn( 0, 0, 0, 0 );
                CombineRgn( hRgn, hrgnUpdate, 0, RGN_COPY );
                OffsetRgn( hRgn, pt.x, pt.y );
            }
	    else
            {
		wndPtr->hrgnUpdate = crop_rgn( 0, hrgnUpdate, &r );
                OffsetRgn( wndPtr->hrgnUpdate, pt.x, pt.y );
            }
	}
	else if( rectUpdate )
	{
	    if( !IntersectRect( &r2, &r, rectUpdate ) ) goto END;
	    OffsetRect( &r2, pt.x, pt.y );
	    if( wndPtr->hrgnUpdate == 0 )
		wndPtr->hrgnUpdate = CreateRectRgnIndirect( &r2 );
	    else
		hRgn = CreateRectRgnIndirect( &r2 );
	}
	else /* entire window or client depending on RDW_FRAME */
	{
	    if( flags & RDW_FRAME )
	    {
                if (wndPtr->hrgnUpdate) hRgn = (HRGN)1;
                else wndPtr->hrgnUpdate = (HRGN)1;
	    }
	    else
	    {
		GETCLIENTRECTW( wndPtr, r2 );
                if( wndPtr->hrgnUpdate == 0 )
                    wndPtr->hrgnUpdate = CreateRectRgnIndirect( &r2 );
                else
                    hRgn = CreateRectRgnIndirect( &r2 );
	    }
	}
    }
    else if (flags & RDW_VALIDATE)  /* ------------------------- Validate */
    {
	/* In this we cannot leave with zero hRgn */
	if( hrgnUpdate )
	{
	    hRgn = crop_rgn( hRgn, hrgnUpdate,  &r );
            OffsetRgn( hRgn, pt.x, pt.y );
	    GetRgnBox( hRgn, &r2 );
	    if( IsRectEmpty( &r2 ) ) goto END;
	}
	else if( rectUpdate )
	{
	    if( !IntersectRect( &r2, &r, rectUpdate ) ) goto END;
		OffsetRect( &r2, pt.x, pt.y );
	    hRgn = CreateRectRgnIndirect( &r2 );
	}
	else /* entire window or client depending on RDW_NOFRAME */
        {
	    if( flags & RDW_NOFRAME )
		hRgn = (HRGN)1;
	    else
	    {
		GETCLIENTRECTW( wndPtr, r2 );
                hRgn = CreateRectRgnIndirect( &r2 );
            }
        }
    }

    /* At this point hRgn is either an update region in window coordinates or 1 or 0 */

    RDW_UpdateRgns( wndPtr, hRgn, flags, TRUE );

    /* Erase/update windows, from now on hRgn is a scratch region */

    hRgn = RDW_Paint( wndPtr, (hRgn == (HRGN)1) ? 0 : hRgn, flags, 0 );

END:
    if( hRgn > (HRGN)1 && (hRgn != hrgnUpdate) )
	DeleteObject(hRgn );
    WIN_ReleaseWndPtr(wndPtr);
    return TRUE;
}


/***********************************************************************
 *		UpdateWindow (USER32.@)
 */
BOOL WINAPI UpdateWindow( HWND hwnd )
{
    return RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/***********************************************************************
 *		InvalidateRgn (USER32.@)
 */
BOOL WINAPI InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    return RedrawWindow(hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *		InvalidateRect (USER32.@)
 */
BOOL WINAPI InvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    return RedrawWindow( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *		ValidateRgn (USER32.@)
 */
BOOL WINAPI ValidateRgn( HWND hwnd, HRGN hrgn )
{
    return RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *		ValidateRect (USER32.@)
 */
BOOL WINAPI ValidateRect( HWND hwnd, const RECT *rect )
{
    return RedrawWindow( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *		GetUpdateRect (USER32.@)
 */
BOOL WINAPI GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    BOOL retvalue;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate > (HRGN)1)
	{
	    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
            if (GetUpdateRgn( hwnd, hrgn, erase ) == ERROR)
            {
                retvalue = FALSE;
                goto END;
            }
	    GetRgnBox( hrgn, rect );
	    DeleteObject( hrgn );
	    if (GetClassLongA(wndPtr->hwndSelf, GCL_STYLE) & CS_OWNDC)
	    {
		if (GetMapMode(wndPtr->dce->hDC) != MM_TEXT)
		{
		    DPtoLP (wndPtr->dce->hDC, (LPPOINT)rect,  2);
		}
	    }
	}
	else
	if( wndPtr->hrgnUpdate == (HRGN)1 )
	{
	    GetClientRect( hwnd, rect );
	    if (erase) RedrawWindow( hwnd, NULL, 0, RDW_FRAME | RDW_ERASENOW | RDW_NOCHILDREN );
	}
	else
	    SetRectEmpty( rect );
    }
    retvalue = (wndPtr->hrgnUpdate >= (HRGN)1);
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *		GetUpdateRgn (USER32.@)
 */
INT WINAPI GetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    INT retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (wndPtr->hrgnUpdate == 0)
    {
        SetRectRgn( hrgn, 0, 0, 0, 0 );
        retval = NULLREGION;
        goto END;
    }
    else
    if (wndPtr->hrgnUpdate == (HRGN)1)
    {
	SetRectRgn( hrgn, 0, 0, wndPtr->rectClient.right - wndPtr->rectClient.left,
				wndPtr->rectClient.bottom - wndPtr->rectClient.top );
	retval = SIMPLEREGION;
    }
    else
    {
	retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY );
	OffsetRgn( hrgn, wndPtr->rectWindow.left - wndPtr->rectClient.left,
			 wndPtr->rectWindow.top - wndPtr->rectClient.top );
    }
    if (erase) RedrawWindow( hwnd, NULL, 0, RDW_ERASENOW | RDW_NOCHILDREN );
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *		ExcludeUpdateRgn (USER32.@)
 */
INT WINAPI ExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    RECT rect;
    WND * wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return ERROR;

    if (wndPtr->hrgnUpdate)
    {
	INT ret;
	HRGN hrgn = CreateRectRgn(wndPtr->rectWindow.left - wndPtr->rectClient.left,
				      wndPtr->rectWindow.top - wndPtr->rectClient.top,
				      wndPtr->rectWindow.right - wndPtr->rectClient.left,
				      wndPtr->rectWindow.bottom - wndPtr->rectClient.top);
	if( wndPtr->hrgnUpdate > (HRGN)1 )
	{
	    CombineRgn(hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY);
	    OffsetRgn(hrgn, wndPtr->rectWindow.left - wndPtr->rectClient.left,
			    wndPtr->rectWindow.top - wndPtr->rectClient.top );
	}

	/* do ugly coordinate translations in dce.c */

	ret = DCE_ExcludeRgn( hdc, hwnd, hrgn );
	DeleteObject( hrgn );
        WIN_ReleaseWndPtr(wndPtr);
	return ret;
    }
    WIN_ReleaseWndPtr(wndPtr);
    return GetClipBox( hdc, &rect );
}



/***********************************************************************
 *		FillRect (USER.81)
 * NOTE
 *   The Win16 variant doesn't support special color brushes like
 *   the Win32 one, despite the fact that Win16, as well as Win32,
 *   supports special background brushes for a window class.
 */
INT16 WINAPI FillRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH prevBrush;

    /* coordinates are logical so we cannot fast-check 'rect',
     * it will be done later in the PatBlt().
     */

    if (!(prevBrush = SelectObject( HDC_32(hdc), HBRUSH_32(hbrush) ))) return 0;
    PatBlt( HDC_32(hdc), rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( HDC_32(hdc), prevBrush );
    return 1;
}


/***********************************************************************
 *		FillRect (USER32.@)
 */
INT WINAPI FillRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;

    if (hbrush <= (HBRUSH) (COLOR_MAX + 1)) {
	hbrush = GetSysColorBrush( (INT) hbrush - 1 );
    }

    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *		InvertRect (USER.82)
 */
void WINAPI InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt( HDC_32(hdc), rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *		InvertRect (USER32.@)
 */
BOOL WINAPI InvertRect( HDC hdc, const RECT *rect )
{
    return PatBlt( hdc, rect->left, rect->top,
		     rect->right - rect->left, rect->bottom - rect->top,
		     DSTINVERT );
}


/***********************************************************************
 *		FrameRect (USER32.@)
 */
INT WINAPI FrameRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;
    RECT r = *rect;

    if ( (r.right <= r.left) || (r.bottom <= r.top) ) return 0;
    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;

    PatBlt( hdc, r.left, r.top, 1,
	      r.bottom - r.top, PATCOPY );
    PatBlt( hdc, r.right - 1, r.top, 1,
	      r.bottom - r.top, PATCOPY );
    PatBlt( hdc, r.left, r.top,
	      r.right - r.left, 1, PATCOPY );
    PatBlt( hdc, r.left, r.bottom - 1,
	      r.right - r.left, 1, PATCOPY );

    SelectObject( hdc, prevBrush );
    return TRUE;
}


/***********************************************************************
 *		FrameRect (USER.83)
 */
INT16 WINAPI FrameRect16( HDC16 hdc, const RECT16 *rect16, HBRUSH16 hbrush )
{
    RECT rect;
    CONV_RECT16TO32( rect16, &rect );
    return FrameRect( HDC_32(hdc), &rect, HBRUSH_32(hbrush) );
}


/***********************************************************************
 *		DrawFocusRect (USER.466)
 */
void WINAPI DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT rect32;
    CONV_RECT16TO32( rc, &rect32 );
    DrawFocusRect( HDC_32(hdc), &rect32 );
}


/***********************************************************************
 *		DrawFocusRect (USER32.@)
 *
 * FIXME: PatBlt(PATINVERT) with background brush.
 */
BOOL WINAPI DrawFocusRect( HDC hdc, const RECT* rc )
{
    HBRUSH hOldBrush;
    HPEN hOldPen, hNewPen;
    INT oldDrawMode, oldBkMode;

    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    hNewPen = CreatePen(PS_ALTERNATE, 1, GetSysColor(COLOR_WINDOWTEXT));
    hOldPen = SelectObject(hdc, hNewPen);
    oldDrawMode = SetROP2(hdc, R2_XORPEN);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);

    Rectangle(hdc, rc->left, rc->top, rc->right, rc->bottom);

    SetBkMode(hdc, oldBkMode);
    SetROP2(hdc, oldDrawMode);
    SelectObject(hdc, hOldPen);
    DeleteObject(hNewPen);
    SelectObject(hdc, hOldBrush);

    return TRUE;
}


/**********************************************************************
 *		DrawAnimatedRects (USER32.@)
 */
BOOL WINAPI DrawAnimatedRects( HWND hwnd, INT idAni,
                                   const RECT* lprcFrom,
                                   const RECT* lprcTo )
{
    FIXME_(win)("(%p,%d,%p,%p): stub\n",hwnd,idAni,lprcFrom,lprcTo);
    return TRUE;
}


/**********************************************************************
 *          PAINTING_DrawStateJam
 *
 * Jams in the requested type in the dc
 */
static BOOL PAINTING_DrawStateJam(HDC hdc, UINT opcode,
                                  DRAWSTATEPROC func, LPARAM lp, WPARAM wp,
                                  LPRECT rc, UINT dtflags, BOOL unicode )
{
    HDC memdc;
    HBITMAP hbmsave;
    BOOL retval;
    INT cx = rc->right - rc->left;
    INT cy = rc->bottom - rc->top;

    switch(opcode)
    {
    case DST_TEXT:
    case DST_PREFIXTEXT:
        if(unicode)
            return DrawTextW(hdc, (LPWSTR)lp, (INT)wp, rc, dtflags);
        else
            return DrawTextA(hdc, (LPSTR)lp, (INT)wp, rc, dtflags);

    case DST_ICON:
        return DrawIcon(hdc, rc->left, rc->top, (HICON)lp);

    case DST_BITMAP:
        memdc = CreateCompatibleDC(hdc);
        if(!memdc) return FALSE;
        hbmsave = (HBITMAP)SelectObject(memdc, (HBITMAP)lp);
        if(!hbmsave)
        {
            DeleteDC(memdc);
            return FALSE;
        }
        retval = BitBlt(hdc, rc->left, rc->top, cx, cy, memdc, 0, 0, SRCCOPY);
        SelectObject(memdc, hbmsave);
        DeleteDC(memdc);
        return retval;

    case DST_COMPLEX:
        if(func) {
	    BOOL bRet;
	    /* DRAWSTATEPROC assumes that it draws at the center of coordinates  */

	    OffsetViewportOrgEx(hdc, rc->left, rc->top, NULL);
            bRet = func(hdc, lp, wp, cx, cy);
	    /* Restore origin */
	    OffsetViewportOrgEx(hdc, -rc->left, -rc->top, NULL);
	    return bRet;
	} else
            return FALSE;
    }
    return FALSE;
}

/**********************************************************************
 *      PAINTING_DrawState()
 */
static BOOL PAINTING_DrawState(HDC hdc, HBRUSH hbr, DRAWSTATEPROC func, LPARAM lp, WPARAM wp,
                               INT x, INT y, INT cx, INT cy, UINT flags, BOOL unicode )
{
    HBITMAP hbm, hbmsave;
    HFONT hfsave;
    HBRUSH hbsave, hbrtmp = 0;
    HDC memdc;
    RECT rc;
    UINT dtflags = DT_NOCLIP;
    COLORREF fg, bg;
    UINT opcode = flags & 0xf;
    INT len = wp;
    BOOL retval, tmp;

    if((opcode == DST_TEXT || opcode == DST_PREFIXTEXT) && !len)    /* The string is '\0' terminated */
    {
        if(unicode)
            len = strlenW((LPWSTR)lp);
        else
            len = strlen((LPSTR)lp);
    }

    /* Find out what size the image has if not given by caller */
    if(!cx || !cy)
    {
        SIZE s;
        CURSORICONINFO *ici;
	BITMAP bm;

        switch(opcode)
        {
        case DST_TEXT:
        case DST_PREFIXTEXT:
            if(unicode)
                retval = GetTextExtentPoint32W(hdc, (LPWSTR)lp, len, &s);
            else
                retval = GetTextExtentPoint32A(hdc, (LPSTR)lp, len, &s);
            if(!retval) return FALSE;
            break;

        case DST_ICON:
            ici = (CURSORICONINFO *)GlobalLock16((HGLOBAL16)lp);
            if(!ici) return FALSE;
            s.cx = ici->nWidth;
            s.cy = ici->nHeight;
            GlobalUnlock16((HGLOBAL16)lp);
            break;

        case DST_BITMAP:
	    if(!GetObjectA((HBITMAP)lp, sizeof(bm), &bm))
	        return FALSE;
            s.cx = bm.bmWidth;
            s.cy = bm.bmHeight;
            break;

        case DST_COMPLEX: /* cx and cy must be set in this mode */
            return FALSE;
	}

        if(!cx) cx = s.cx;
        if(!cy) cy = s.cy;
    }

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + cx;
    rc.bottom = y + cy;

    if(flags & DSS_RIGHT)    /* This one is not documented in the win32.hlp file */
        dtflags |= DT_RIGHT;
    if(opcode == DST_TEXT)
        dtflags |= DT_NOPREFIX;

    /* For DSS_NORMAL we just jam in the image and return */
    if((flags & 0x7ff0) == DSS_NORMAL)
    {
        return PAINTING_DrawStateJam(hdc, opcode, func, lp, len, &rc, dtflags, unicode);
    }

    /* For all other states we need to convert the image to B/W in a local bitmap */
    /* before it is displayed */
    fg = SetTextColor(hdc, RGB(0, 0, 0));
    bg = SetBkColor(hdc, RGB(255, 255, 255));
    hbm = NULL; hbmsave = NULL;
    memdc = NULL; hbsave = NULL;
    retval = FALSE; /* assume failure */

    /* From here on we must use "goto cleanup" when something goes wrong */
    hbm     = CreateBitmap(cx, cy, 1, 1, NULL);
    if(!hbm) goto cleanup;
    memdc   = CreateCompatibleDC(hdc);
    if(!memdc) goto cleanup;
    hbmsave = (HBITMAP)SelectObject(memdc, hbm);
    if(!hbmsave) goto cleanup;
    rc.left = rc.top = 0;
    rc.right = cx;
    rc.bottom = cy;
    if(!FillRect(memdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH))) goto cleanup;
    SetBkColor(memdc, RGB(255, 255, 255));
    SetTextColor(memdc, RGB(0, 0, 0));
    hfsave  = (HFONT)SelectObject(memdc, GetCurrentObject(hdc, OBJ_FONT));

    /* DST_COMPLEX may draw text as well,
     * so we must be sure that correct font is selected
     */
    if(!hfsave && (opcode <= DST_PREFIXTEXT)) goto cleanup;
    tmp = PAINTING_DrawStateJam(memdc, opcode, func, lp, len, &rc, dtflags, unicode);
    if(hfsave) SelectObject(memdc, hfsave);
    if(!tmp) goto cleanup;

    /* This state cause the image to be dithered */
    if(flags & DSS_UNION)
    {
        hbsave = (HBRUSH)SelectObject(memdc, CACHE_GetPattern55AABrush());
        if(!hbsave) goto cleanup;
        tmp = PatBlt(memdc, 0, 0, cx, cy, 0x00FA0089);
        SelectObject(memdc, hbsave);
        if(!tmp) goto cleanup;
    }

    if (flags & DSS_DISABLED)
       hbrtmp = CreateSolidBrush(GetSysColor(COLOR_3DHILIGHT));
    else if (flags & DSS_DEFAULT)
       hbrtmp = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));

    /* Draw light or dark shadow */
    if (flags & (DSS_DISABLED|DSS_DEFAULT))
    {
       if(!hbrtmp) goto cleanup;
       hbsave = (HBRUSH)SelectObject(hdc, hbrtmp);
       if(!hbsave) goto cleanup;
       if(!BitBlt(hdc, x+1, y+1, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
       SelectObject(hdc, hbsave);
       DeleteObject(hbrtmp);
       hbrtmp = 0;
    }

    if (flags & DSS_DISABLED)
    {
       hbr = hbrtmp = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
       if(!hbrtmp) goto cleanup;
    }
    else if (!hbr)
    {
       hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
    }

    hbsave = (HBRUSH)SelectObject(hdc, hbr);

    if(!BitBlt(hdc, x, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;

    retval = TRUE; /* We succeeded */

cleanup:
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);

    if(hbsave)  SelectObject(hdc, hbsave);
    if(hbmsave) SelectObject(memdc, hbmsave);
    if(hbrtmp)  DeleteObject(hbrtmp);
    if(hbm)     DeleteObject(hbm);
    if(memdc)   DeleteDC(memdc);

    return retval;
}

/**********************************************************************
 *		DrawStateA (USER32.@)
 */
BOOL WINAPI DrawStateA(HDC hdc, HBRUSH hbr,
                   DRAWSTATEPROC func, LPARAM ldata, WPARAM wdata,
                   INT x, INT y, INT cx, INT cy, UINT flags)
{
    return PAINTING_DrawState(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, FALSE);
}

/**********************************************************************
 *		DrawStateW (USER32.@)
 */
BOOL WINAPI DrawStateW(HDC hdc, HBRUSH hbr,
                   DRAWSTATEPROC func, LPARAM ldata, WPARAM wdata,
                   INT x, INT y, INT cx, INT cy, UINT flags)
{
    return PAINTING_DrawState(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, TRUE);
}


/***********************************************************************
 *		SelectPalette (Not a Windows API)
 */
HPALETTE WINAPI SelectPalette( HDC hDC, HPALETTE hPal, BOOL bForceBackground )
{
    WORD wBkgPalette = 1;

    if (!bForceBackground && (hPal != GetStockObject(DEFAULT_PALETTE)))
    {
        HWND hwnd = WindowFromDC( hDC );
        if (hwnd)
        {
            HWND hForeground = GetForegroundWindow();
            /* set primary palette if it's related to current active */
            if (hForeground == hwnd || IsChild(hForeground,hwnd)) wBkgPalette = 0;
        }
    }
    return pfnGDISelectPalette( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *		UserRealizePalette (USER32.@)
 */
UINT WINAPI UserRealizePalette( HDC hDC )
{
    UINT realized = pfnGDIRealizePalette( hDC );

    /* do not send anything if no colors were changed */
    if (realized && IsDCCurrentPalette16( HDC_16(hDC) ))
    {
        /* send palette change notification */
        HWND hWnd = WindowFromDC( hDC );
        if (hWnd) SendMessageTimeoutW( HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0,
                                       SMTO_ABORTIFHUNG, 2000, NULL );
    }
    return realized;
}

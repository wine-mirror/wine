/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *			 1999 Alex Korobka
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "region.h"
#include "win.h"
#include "queue.h"
#include "dce.h"
#include "heap.h"
#include "debugtools.h"
#include "cache.h"

DEFAULT_DEBUG_CHANNEL(win);
DECLARE_DEBUG_CHANNEL(nonclient);

/* client rect in window coordinates */

#define GETCLIENTRECTW( wnd, r )	(r).left = (wnd)->rectClient.left - (wnd)->rectWindow.left; \
					(r).top = (wnd)->rectClient.top - (wnd)->rectWindow.top; \
					(r).right = (wnd)->rectClient.right - (wnd)->rectWindow.left; \
					(r).bottom = (wnd)->rectClient.bottom - (wnd)->rectWindow.top

  /* Last COLOR id */
#define COLOR_MAX   COLOR_GRADIENTINACTIVECAPTION

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC


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
static BOOL WIN_HaveToDelayNCPAINT(
  WND* wndPtr, 
  UINT uncFlags)
{
  WND* parentWnd = NULL;

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
  parentWnd = WIN_LockWndPtr(wndPtr->parent);

  while (parentWnd!=NULL)
  {
    if ( ((parentWnd->dwStyle & WS_CLIPCHILDREN) == 0) &&
	 (parentWnd->hrgnUpdate != 0) )
    {
      WIN_ReleaseWndPtr(parentWnd);
      return TRUE;
    }

    WIN_UpdateWndPtr(&parentWnd, parentWnd->parent);    
  }

  WIN_ReleaseWndPtr(parentWnd);

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
HRGN WIN_UpdateNCRgn(WND* wnd, HRGN hRgn, UINT uncFlags )
{
    RECT  r;
    HRGN  hClip = 0;
    HRGN  hrgnRet = 0;

    TRACE_(nonclient)("hwnd %04x [%04x] hrgn %04x, unc %04x, ncf %i\n", 
                      wnd->hwndSelf, wnd->hrgnUpdate, hRgn, uncFlags, wnd->flags & WIN_NEEDS_NCPAINT);

    /* desktop window doesn't have a nonclient area */
    if(wnd == WIN_GetDesktop()) 
    {
        wnd->flags &= ~WIN_NEEDS_NCPAINT;
	if( wnd->hrgnUpdate > 1 )
	    hrgnRet = REGION_CropRgn( hRgn, wnd->hrgnUpdate, NULL, NULL );
	else 
	{
	    hrgnRet = wnd->hrgnUpdate;
	}
        WIN_ReleaseDesktop();
        return hrgnRet;
    }
    WIN_ReleaseDesktop();

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
	 !WIN_HaveToDelayNCPAINT(wnd, uncFlags) )
    {
	    RECT r2, r3;

	    wnd->flags &= ~WIN_NEEDS_NCPAINT;  
	    GETCLIENTRECTW( wnd, r );

	    TRACE_(nonclient)( "\tclient box (%i,%i-%i,%i), hrgnUpdate %04x\n", 
				r.left, r.top, r.right, r.bottom, wnd->hrgnUpdate );
	    if( wnd->hrgnUpdate > 1 )
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
		    wnd->hrgnUpdate = REGION_CropRgn( hRgn, hClip, &r, NULL );
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
			    QUEUE_DecPaintCount( wnd->hmemTaskQ );

			wnd->flags &= ~WIN_NEEDS_ERASEBKGND;
		    }
		}

		if(!hClip && wnd->hrgnUpdate ) goto copyrgn;
	    }
	    else 
	    if( wnd->hrgnUpdate == 1 )/* entire window */
	    {
		if( uncFlags & UNC_UPDATE ) wnd->hrgnUpdate = CreateRectRgnIndirect( &r );
		if( uncFlags & UNC_REGION ) hrgnRet = 1;
		uncFlags |= UNC_ENTIRE;
	    }
    }
    else /* no WM_NCPAINT unless forced */
    {
	if( wnd->hrgnUpdate >  1 )
	{
copyrgn:
	    if( uncFlags & UNC_REGION )
		hrgnRet = REGION_CropRgn( hRgn, wnd->hrgnUpdate, NULL, NULL );
	}
	else
	if( wnd->hrgnUpdate == 1 && (uncFlags & UNC_UPDATE) )
	{
	    GETCLIENTRECTW( wnd, r ); 
	    wnd->hrgnUpdate = CreateRectRgnIndirect( &r );
	    if( uncFlags & UNC_REGION ) hrgnRet = 1;
	}
    }

    if(!hClip && (uncFlags & UNC_ENTIRE) )
    {
	/* still don't do anything if there is no nonclient area */
	hClip = (memcmp( &wnd->rectWindow, &wnd->rectClient, sizeof(RECT) ) != 0);
    }

    if( hClip ) /* NOTE: WM_NCPAINT allows wParam to be 1 */
    {
        if ( hClip == hrgnRet && hrgnRet > 1 ) {
	    hClip = CreateRectRgn( 0, 0, 0, 0 );
	    CombineRgn( hClip, hrgnRet, 0, RGN_COPY );
	}

	SendMessageA( wnd->hwndSelf, WM_NCPAINT, hClip, 0L );
	if( (hClip > 1) && (hClip != hRgn) && (hClip != hrgnRet) )
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

    TRACE_(nonclient)("returning %04x (hClip = %04x, hrgnUpdate = %04x)\n", hrgnRet, hClip, wnd->hrgnUpdate );

    return hrgnRet;
}


/***********************************************************************
 *           BeginPaint16    (USER.39)
 */
HDC16 WINAPI BeginPaint16( HWND16 hwnd, LPPAINTSTRUCT16 lps ) 
{
    BOOL bIcon;
    HRGN hrgnUpdate;
    RECT16 clipRect, clientRect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;

    bIcon = (wndPtr->dwStyle & WS_MINIMIZE && GetClassWord(wndPtr->hwndSelf, GCW_HICON));

    wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;

    /* send WM_NCPAINT and make sure hrgnUpdate is a valid rgn handle */
    WIN_UpdateNCRgn( wndPtr, 0, UNC_UPDATE | UNC_IN_BEGINPAINT);

    /*
     * Make sure the window is still a window. All window locks are suspended
     * when the WM_NCPAINT is sent.
     */
    if (!IsWindow(wndPtr->hwndSelf))
    {
        WIN_ReleaseWndPtr(wndPtr);
        return 0;
    }

    if( ((hrgnUpdate = wndPtr->hrgnUpdate) != 0) || (wndPtr->flags & WIN_INTERNAL_PAINT))
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );

    wndPtr->hrgnUpdate = 0;
    wndPtr->flags &= ~WIN_INTERNAL_PAINT;

    HideCaret( hwnd );

    TRACE("hrgnUpdate = %04x, \n", hrgnUpdate);

    if (GetClassWord16(wndPtr->hwndSelf, GCW_STYLE) & CS_PARENTDC)
    {
        /* Don't clip the output to the update region for CS_PARENTDC window */
	if( hrgnUpdate ) 
	    DeleteObject(hrgnUpdate);
        lps->hdc = GetDCEx16( hwnd, 0, DCX_WINDOWPAINT | DCX_USESTYLE |
                              (bIcon ? DCX_WINDOW : 0) );
    }
    else
    {
	if( hrgnUpdate ) /* convert to client coordinates */
	    OffsetRgn( hrgnUpdate, wndPtr->rectWindow.left - wndPtr->rectClient.left,
			           wndPtr->rectWindow.top - wndPtr->rectClient.top );
        lps->hdc = GetDCEx16(hwnd, hrgnUpdate, DCX_INTERSECTRGN |
                             DCX_WINDOWPAINT | DCX_USESTYLE | (bIcon ? DCX_WINDOW : 0) );
	/* ReleaseDC() in EndPaint() will delete the region */
    }

    TRACE("hdc = %04x\n", lps->hdc);

    if (!lps->hdc)
    {
        WARN("GetDCEx() failed in BeginPaint(), hwnd=%04x\n", hwnd);
        WIN_ReleaseWndPtr(wndPtr);
        return 0;
    }

    /* It is possible that the clip box is bigger than the window itself,
       if CS_PARENTDC flag is set. Windows never return a paint rect bigger
       than the window itself, so we need to intersect the cliprect with
       the window  */
    
    GetClipBox16( lps->hdc, &clipRect );
    GetClientRect16( hwnd, &clientRect );

    /* The rect obtained by GetClipBox is in logical, so make the client in logical to*/
    DPtoLP16(lps->hdc, (LPPOINT16) &clientRect, 2);    

    IntersectRect16(&lps->rcPaint, &clientRect, &clipRect);

    TRACE("box = (%i,%i - %i,%i)\n", lps->rcPaint.left, lps->rcPaint.top,
		    lps->rcPaint.right, lps->rcPaint.bottom );

    if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
    {
        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
        lps->fErase = !SendMessage16(hwnd, (bIcon) ? WM_ICONERASEBKGND
                                                   : WM_ERASEBKGND,
                                     (WPARAM16)lps->hdc, 0 );
    }
    else lps->fErase = TRUE;

    WIN_ReleaseWndPtr(wndPtr);
    return lps->hdc;
}


/***********************************************************************
 *           BeginPaint32    (USER32.10)
 */
HDC WINAPI BeginPaint( HWND hwnd, PAINTSTRUCT *lps )
{
    PAINTSTRUCT16 ps;

    BeginPaint16( hwnd, &ps );
    lps->hdc            = (HDC)ps.hdc;
    lps->fErase         = ps.fErase;
    lps->rcPaint.top    = ps.rcPaint.top;
    lps->rcPaint.left   = ps.rcPaint.left;
    lps->rcPaint.right  = ps.rcPaint.right;
    lps->rcPaint.bottom = ps.rcPaint.bottom;
    lps->fRestore       = ps.fRestore;
    lps->fIncUpdate     = ps.fIncUpdate;
    return lps->hdc;
}


/***********************************************************************
 *           EndPaint16    (USER.40)
 */
BOOL16 WINAPI EndPaint16( HWND16 hwnd, const PAINTSTRUCT16* lps )
{
    ReleaseDC16( hwnd, lps->hdc );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *           EndPaint32    (USER32.176)
 */
BOOL WINAPI EndPaint( HWND hwnd, const PAINTSTRUCT *lps )
{
    ReleaseDC( hwnd, lps->hdc );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *           FillWindow    (USER.324)
 */
void WINAPI FillWindow16( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc, HBRUSH16 hbrush )
{
    RECT16 rect;
    GetClientRect16( hwnd, &rect );
    DPtoLP16( hdc, (LPPOINT16)&rect, 2 );
    PaintRect16( hwndParent, hwnd, hdc, hbrush, &rect );
}


/***********************************************************************
 *	     PAINT_GetControlBrush
 */
static HBRUSH16 PAINT_GetControlBrush( HWND hParent, HWND hWnd, HDC16 hDC, UINT16 ctlType )
{
    HBRUSH16 bkgBrush = (HBRUSH16)SendMessageA( hParent, WM_CTLCOLORMSGBOX + ctlType, 
							     (WPARAM)hDC, (LPARAM)hWnd );
    if( !IsGDIObject16(bkgBrush) )
	bkgBrush = DEFWND_ControlColor( hDC, ctlType );
    return bkgBrush;
}


/***********************************************************************
 *           PaintRect    (USER.325)
 */
void WINAPI PaintRect16( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc,
                       HBRUSH16 hbrush, const RECT16 *rect)
{
    if( hbrush <= CTLCOLOR_MAX ) 
    {
	if( hwndParent )
	    hbrush = PAINT_GetControlBrush( hwndParent, hwnd, hdc, (UINT16)hbrush );
	else 
	    return;
    }
    if( hbrush ) 
	FillRect16( hdc, rect, hbrush );
}


/***********************************************************************
 *           GetControlBrush    (USER.326)
 */
HBRUSH16 WINAPI GetControlBrush16( HWND16 hwnd, HDC16 hdc, UINT16 ctlType )
{
    WND* wndPtr = WIN_FindWndPtr( hwnd );
    HBRUSH16 retvalue;

    if((ctlType <= CTLCOLOR_MAX) && wndPtr )
    {
	WND* parent;
	if( wndPtr->dwStyle & WS_POPUP ) parent = WIN_LockWndPtr(wndPtr->owner);
	else parent = WIN_LockWndPtr(wndPtr->parent);
	if( !parent ) parent = wndPtr;
	retvalue = (HBRUSH16)PAINT_GetControlBrush( parent->hwndSelf, hwnd, hdc, ctlType );
        WIN_ReleaseWndPtr(parent);
        goto END;
    }
    retvalue = (HBRUSH16)0;
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 * 		RDW_ValidateParent [RDW_UpdateRgns() helper] 
 *
 *  Validate the portions of parent that are covered by a validated child
 *  wndPtr = child
 */
void  RDW_ValidateParent(WND *wndChild)  
{
    WND *wndParent = WIN_LockWndPtr(wndChild->parent);
    WND *wndDesktop = WIN_GetDesktop();

    if ((wndParent) && (wndParent != wndDesktop) && !(wndParent->dwStyle & WS_CLIPCHILDREN))
    {
        HRGN hrg;
        if (wndChild->hrgnUpdate == 1 )
        {
            RECT r;

	    r.left = 0;
	    r.top = 0;
	    r.right = wndChild->rectWindow.right - wndChild->rectWindow.left;
	    r.bottom = wndChild->rectWindow.bottom - wndChild->rectWindow.top;

            hrg = CreateRectRgnIndirect( &r );
        }
        else
	   hrg = wndChild->hrgnUpdate;
        if (wndParent->hrgnUpdate != 0)
        {
            POINT ptOffset;
            RECT rect, rectParent;
            if( wndParent->hrgnUpdate == 1 )
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
            CombineRgn( wndParent->hrgnUpdate, wndParent->hrgnUpdate, hrg, RGN_DIFF );
            OffsetRgn( hrg, -ptOffset.x, -ptOffset.y );
        }
        if (hrg != wndChild->hrgnUpdate) DeleteObject( hrg );
    }
    WIN_ReleaseWndPtr(wndParent);
    WIN_ReleaseDesktop();
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
    BOOL bChildren =  ( wndPtr->child && !(flags & RDW_NOCHILDREN) && !(wndPtr->dwStyle & WS_MINIMIZE) 
			&& ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) );
    RECT r;

    r.left = 0;
    r.top = 0;
    r.right = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    r.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    TRACE("\thwnd %04x [%04x] -> hrgn [%04x], flags [%04x]\n", wndPtr->hwndSelf, wndPtr->hrgnUpdate, hRgn, flags );

    if( flags & RDW_INVALIDATE )
    {
	if( hRgn > 1 )
	{
	    switch( wndPtr->hrgnUpdate )
	    {
		default:
			CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hRgn, RGN_OR );
			/* fall through */
		case 0:
			wndPtr->hrgnUpdate = REGION_CropRgn( wndPtr->hrgnUpdate, 
							     wndPtr->hrgnUpdate ? wndPtr->hrgnUpdate : hRgn, 
							     &r, NULL );
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
	else if( hRgn == 1 )
	{
	    if( wndPtr->hrgnUpdate > 1 )
		DeleteObject( wndPtr->hrgnUpdate );
	    wndPtr->hrgnUpdate = 1;
	}
	else
	    hRgn = wndPtr->hrgnUpdate;	/* this is a trick that depends on code in PAINT_RedrawWindow() */

	if( !bHadOne && !(wndPtr->flags & WIN_INTERNAL_PAINT) )
            QUEUE_IncPaintCount( wndPtr->hmemTaskQ );

	if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;
	if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags    |= RDW_FRAME;
    }
    else if( flags & RDW_VALIDATE )
    {
	if( wndPtr->hrgnUpdate )
	{
	    if( hRgn > 1 )
	    {
		if( wndPtr->hrgnUpdate == 1 )
		    wndPtr->hrgnUpdate = CreateRectRgnIndirect( &r );

		if( CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hRgn, RGN_DIFF )
		    == NULLREGION )
		    goto EMPTY;
	    }
	    else /* validate everything */
	    {
		if( wndPtr->hrgnUpdate > 1 )
		{
EMPTY:
		    DeleteObject( wndPtr->hrgnUpdate );
		}
		wndPtr->hrgnUpdate = 0;
	    }

	    if( !wndPtr->hrgnUpdate )
	    {
		wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
		if( !(wndPtr->flags & WIN_INTERNAL_PAINT) )
		    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
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
	if( hRgn > 1 && bChildren )
	{
            WND* wnd = wndPtr->child;
	    POINT ptTotal, prevOrigin = {0,0};
            POINT ptClient;

            ptClient.x = wndPtr->rectClient.left - wndPtr->rectWindow.left;
            ptClient.y = wndPtr->rectClient.top - wndPtr->rectWindow.top;

            for( ptTotal.x = ptTotal.y = 0; wnd; wnd = wnd->next )
            {
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
            }
            OffsetRgn( hRgn, ptTotal.x, ptTotal.y );
	    bChildren = 0;
	}
    }

    /* handle hRgn == 1 (alias for entire window) and/or internal paint recursion */

    if( bChildren )
    {
	WND* wnd;
	for( wnd = wndPtr->child; wnd; wnd = wnd->next )
	     if( wnd->dwStyle & WS_VISIBLE )
		 RDW_UpdateRgns( wnd, hRgn, flags, FALSE );
    }

end:

    /* Set/clear internal paint flag */

    if (flags & RDW_INTERNALPAINT)
    {
        if ( !wndPtr->hrgnUpdate && !(wndPtr->flags & WIN_INTERNAL_PAINT))
            QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
        wndPtr->flags |= WIN_INTERNAL_PAINT;
    }
    else if (flags & RDW_NOINTERNALPAINT)
    {
        if ( !wndPtr->hrgnUpdate && (wndPtr->flags & WIN_INTERNAL_PAINT))
            QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
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
    BOOL bIcon = ((wndPtr->dwStyle & WS_MINIMIZE) && GetClassWord(wndPtr->hwndSelf, GCW_HICON)); 

      /* Erase/update the window itself ... */

    TRACE("\thwnd %04x [%04x] -> hrgn [%04x], flags [%04x]\n", hWnd, wndPtr->hrgnUpdate, hrgn, flags );

    /*
     * Check if this window should delay it's processing of WM_NCPAINT.
     * See WIN_HaveToDelayNCPAINT for a description of the mechanism
     */
    if ((ex & RDW_EX_DELAY_NCPAINT) || WIN_HaveToDelayNCPAINT(wndPtr, 0) )
	ex |= RDW_EX_DELAY_NCPAINT;

    if (flags & RDW_UPDATENOW)
    {
        if (wndPtr->hrgnUpdate) /* wm_painticon wparam is 1 */
            SendMessage16( hWnd, (bIcon) ? WM_PAINTICON : WM_PAINT, bIcon, 0 );
    }
    else if ((flags & RDW_ERASENOW) || (ex & RDW_EX_TOPFRAME))
    {
	UINT dcx = DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN | DCX_WINDOWPAINT | DCX_CACHE;
	HRGN hrgnRet;

	hrgnRet = WIN_UpdateNCRgn(wndPtr, 
				  hrgn, 
				  UNC_REGION | UNC_CHECK | 
				  ((ex & RDW_EX_TOPFRAME) ? UNC_ENTIRE : 0) |
				  ((ex & RDW_EX_DELAY_NCPAINT) ? UNC_DELAY_NCPAINT : 0) ); 

        if( hrgnRet )
	{
	    if( hrgnRet > 1 ) hrgn = hrgnRet; else hrgnRet = 0; /* entire client */
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
		    if (SendMessage16( hWnd, (bIcon) ? WM_ICONERASEBKGND
                                       : WM_ERASEBKGND, (WPARAM16)hDC, 0 ))
		    wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
		    ReleaseDC( hWnd, hDC );
		}
            }
        }
    }

    if( !IsWindow(hWnd) ) return hrgn;
    ex &= ~RDW_EX_TOPFRAME;

      /* ... and its child windows */

    if( wndPtr->child && !(flags & RDW_NOCHILDREN) && !(wndPtr->dwStyle & WS_MINIMIZE) 
	&& ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) )
    {
	WND** list, **ppWnd;

	if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	{
            wndPtr = NULL;
	    for (ppWnd = list; *ppWnd; ppWnd++)
	    {
		WIN_UpdateWndPtr(&wndPtr,*ppWnd);
		if (!IsWindow(wndPtr->hwndSelf)) continue;
		    if ( (wndPtr->dwStyle & WS_VISIBLE) &&
			 (wndPtr->hrgnUpdate || (wndPtr->flags & WIN_INTERNAL_PAINT)) )
		        hrgn = RDW_Paint( wndPtr, hrgn, flags, ex );
	    }
            WIN_ReleaseWndPtr(wndPtr);
	    WIN_ReleaseWinArray(list);
	}
    }

    return hrgn;
}

/***********************************************************************
 *           PAINT_RedrawWindow
 *
 */
BOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                           HRGN hrgnUpdate, UINT flags, UINT ex )
{
    HRGN hRgn = 0;
    RECT r, r2;
    POINT pt;
    WND* wndPtr;

    if (!hwnd) hwnd = GetDesktopWindow();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    /* check if the window or its parents are visible/not minimized */

    if (!WIN_IsWindowDrawable( wndPtr, !(flags & RDW_FRAME) ) )
    {
        WIN_ReleaseWndPtr(wndPtr);
        return TRUE; 
    }

    if (TRACE_ON(win))
    {
	if( hrgnUpdate )
	{
	    GetRgnBox( hrgnUpdate, &r );
            TRACE( "%04x (%04x) NULL %04x box (%i,%i-%i,%i) flags=%04x, exflags=%04x\n", 
	          hwnd, wndPtr->hrgnUpdate, hrgnUpdate, r.left, r.top, r.right, r.bottom, flags, ex);
	}
	else
	{
	    if( rectUpdate )
		r = *rectUpdate;
	    else
		SetRectEmpty( &r );
	    TRACE( "%04x (%04x) %s %d,%d-%d,%d %04x flags=%04x, exflags=%04x\n",
			hwnd, wndPtr->hrgnUpdate, rectUpdate ? "rect" : "NULL", r.left, 
			r.top, r.right, r.bottom, hrgnUpdate, flags, ex );
	}
    }

    /* prepare an update region in window coordinates */

    if( flags & RDW_FRAME )
	r = wndPtr->rectWindow;
    else
	r = wndPtr->rectClient;

    if( ex & RDW_EX_XYWINDOW )
    {
	pt.x = pt.y = 0;
        OffsetRect( &r, -wndPtr->rectWindow.left, -wndPtr->rectWindow.top );
    }
    else
    {
	pt.x = wndPtr->rectClient.left - wndPtr->rectWindow.left;
	pt.y = wndPtr->rectClient.top - wndPtr->rectWindow.top;
	OffsetRect( &r, -wndPtr->rectClient.left, -wndPtr->rectClient.top );
    }

    if (flags & RDW_INVALIDATE)  /* ------------------------- Invalidate */
    {
	/* If the window doesn't have hrgnUpdate we leave hRgn zero
	 * and put a new region straight into wndPtr->hrgnUpdate
	 * so that RDW_UpdateRgns() won't have to do any extra work.
	 */

	if( hrgnUpdate )
	{
	    if( wndPtr->hrgnUpdate )
	        hRgn = REGION_CropRgn( 0, hrgnUpdate, NULL, &pt );
	    else 
		wndPtr->hrgnUpdate = REGION_CropRgn( 0, hrgnUpdate, &r, &pt ); 
	}
	else if( rectUpdate )
	{
	    if( !IntersectRect( &r2, &r, rectUpdate ) ) goto END;
	    OffsetRect( &r2, pt.x, pt.y );

rect2i:
	    if( wndPtr->hrgnUpdate == 0 )
		wndPtr->hrgnUpdate = CreateRectRgnIndirect( &r2 );
	    else
		hRgn = CreateRectRgnIndirect( &r2 );
	}
	else /* entire window or client depending on RDW_FRAME */
	{
	    if( flags & RDW_FRAME )
	    {
		if( wndPtr->hrgnUpdate )
		    DeleteObject( wndPtr->hrgnUpdate );
		wndPtr->hrgnUpdate = 1;
	    }
	    else
	    {
		GETCLIENTRECTW( wndPtr, r2 );
		goto rect2i;
	    }
	}
    }
    else if (flags & RDW_VALIDATE)  /* ------------------------- Validate */
    {
	/* In this we cannot leave with zero hRgn */
	if( hrgnUpdate )
	{
	    hRgn = REGION_CropRgn( hRgn, hrgnUpdate,  &r, &pt );
	    GetRgnBox( hRgn, &r2 );
	    if( IsRectEmpty( &r2 ) ) goto END;
	}
	else if( rectUpdate )
	{
	    if( !IntersectRect( &r2, &r, rectUpdate ) ) goto END;
		OffsetRect( &r2, pt.x, pt.y );
rect2v:
	    hRgn = CreateRectRgnIndirect( &r2 );
	}
	else /* entire window or client depending on RDW_FRAME */
        {
	    if( flags & RDW_FRAME ) 
		hRgn = 1;
	    else
	    {
		GETCLIENTRECTW( wndPtr, r2 );
		goto rect2v;
            }
        }
    }

    /* At this point hRgn is either an update region in window coordinates or 1 or 0 */

    RDW_UpdateRgns( wndPtr, hRgn, flags, TRUE );

    /* Erase/update windows, from now on hRgn is a scratch region */

    hRgn = RDW_Paint( wndPtr, (hRgn == 1) ? 0 : hRgn, flags, ex );

END:
    if( hRgn > 1 && (hRgn != hrgnUpdate) )
	DeleteObject(hRgn );
    WIN_ReleaseWndPtr(wndPtr);
    return TRUE;
}


/***********************************************************************
 *           RedrawWindow32    (USER32.426)
 */
BOOL WINAPI RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                              HRGN hrgnUpdate, UINT flags )
{
    return PAINT_RedrawWindow( hwnd, rectUpdate, hrgnUpdate, flags, 0 );
}


/***********************************************************************
 *           RedrawWindow16    (USER.290)
 */
BOOL16 WINAPI RedrawWindow16( HWND16 hwnd, const RECT16 *rectUpdate,
                              HRGN16 hrgnUpdate, UINT16 flags )
{
    if (rectUpdate)
    {
        RECT r;
        CONV_RECT16TO32( rectUpdate, &r );
        return (BOOL16)RedrawWindow( (HWND)hwnd, &r, hrgnUpdate, flags );
    }
    return (BOOL16)PAINT_RedrawWindow( (HWND)hwnd, NULL, 
				       (HRGN)hrgnUpdate, flags, 0 );
}


/***********************************************************************
 *           UpdateWindow16   (USER.124)
 */
void WINAPI UpdateWindow16( HWND16 hwnd )
{
    PAINT_RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           UpdateWindow32   (USER32.567)
 */
void WINAPI UpdateWindow( HWND hwnd )
{
    PAINT_RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           InvalidateRgn16   (USER.126)
 */
void WINAPI InvalidateRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 erase )
{
    PAINT_RedrawWindow((HWND)hwnd, NULL, (HRGN)hrgn, 
		       RDW_INVALIDATE | (erase ? RDW_ERASE : 0), 0 );
}


/***********************************************************************
 *           InvalidateRgn32   (USER32.329)
 */
BOOL WINAPI InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    return PAINT_RedrawWindow(hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0), 0 );
}


/***********************************************************************
 *           InvalidateRect16   (USER.125)
 */
void WINAPI InvalidateRect16( HWND16 hwnd, const RECT16 *rect, BOOL16 erase )
{
    RedrawWindow16( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           InvalidateRect32   (USER32.328)
 */
BOOL WINAPI InvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    return PAINT_RedrawWindow( hwnd, rect, 0, 
			       RDW_INVALIDATE | (erase ? RDW_ERASE : 0), 0 );
}


/***********************************************************************
 *           ValidateRgn16   (USER.128)
 */
void WINAPI ValidateRgn16( HWND16 hwnd, HRGN16 hrgn )
{
    PAINT_RedrawWindow( (HWND)hwnd, NULL, (HRGN)hrgn, 
			RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}


/***********************************************************************
 *           ValidateRgn32   (USER32.572)
 */
void WINAPI ValidateRgn( HWND hwnd, HRGN hrgn )
{
    PAINT_RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}


/***********************************************************************
 *           ValidateRect16   (USER.127)
 */
void WINAPI ValidateRect16( HWND16 hwnd, const RECT16 *rect )
{
    RedrawWindow16( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *           ValidateRect32   (USER32.571)
 */
void WINAPI ValidateRect( HWND hwnd, const RECT *rect )
{
    PAINT_RedrawWindow( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}


/***********************************************************************
 *           GetUpdateRect16   (USER.190)
 */
BOOL16 WINAPI GetUpdateRect16( HWND16 hwnd, LPRECT16 rect, BOOL16 erase )
{
    RECT r;
    BOOL16 ret;

    if (!rect) return GetUpdateRect( hwnd, NULL, erase );
    ret = GetUpdateRect( hwnd, &r, erase );
    CONV_RECT32TO16( &r, rect );
    return ret;
}


/***********************************************************************
 *           GetUpdateRect32   (USER32.297)
 */
BOOL WINAPI GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    BOOL retvalue;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate > 1)
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
	if( wndPtr->hrgnUpdate == 1 )
	{
	    GetClientRect( hwnd, rect );
	    if (erase) RedrawWindow( hwnd, NULL, 0, RDW_FRAME | RDW_ERASENOW | RDW_NOCHILDREN );
	}
	else 
	    SetRectEmpty( rect );
    }
    retvalue = (wndPtr->hrgnUpdate >= 1);
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *           GetUpdateRgn16   (USER.237)
 */
INT16 WINAPI GetUpdateRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 erase )
{
    return GetUpdateRgn( hwnd, hrgn, erase );
}


/***********************************************************************
 *           GetUpdateRgn    (USER32.298)
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
    if (wndPtr->hrgnUpdate == 1)
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
 *           ExcludeUpdateRgn16   (USER.238)
 */
INT16 WINAPI ExcludeUpdateRgn16( HDC16 hdc, HWND16 hwnd )
{
    return ExcludeUpdateRgn( hdc, hwnd );
}


/***********************************************************************
 *           ExcludeUpdateRgn32   (USER32.195)
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
	if( wndPtr->hrgnUpdate > 1 )
	{
	    CombineRgn(hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY);
	    OffsetRgn(hrgn, wndPtr->rectWindow.left - wndPtr->rectClient.left, 
			    wndPtr->rectWindow.top - wndPtr->rectClient.top );
	}

	/* do ugly coordinate translations in dce.c */

	ret = DCE_ExcludeRgn( hdc, wndPtr, hrgn );
	DeleteObject( hrgn );
        WIN_ReleaseWndPtr(wndPtr);
	return ret;
    } 
    WIN_ReleaseWndPtr(wndPtr);
    return GetClipBox( hdc, &rect );
}



/***********************************************************************
 *           FillRect16    (USER.81)
 * NOTE
 *   The Win16 variant doesn't support special color brushes like
 *   the Win32 one, despite the fact that Win16, as well as Win32,
 *   supports special background brushes for a window class.
 */
INT16 WINAPI FillRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH16 prevBrush;

    /* coordinates are logical so we cannot fast-check 'rect',
     * it will be done later in the PatBlt().
     */

    if (!(prevBrush = SelectObject16( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject16( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           FillRect    (USER32.197)
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
 *           InvertRect16    (USER.82)
 */
void WINAPI InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *           InvertRect    (USER32.330)
 */
BOOL WINAPI InvertRect( HDC hdc, const RECT *rect )
{
    return PatBlt( hdc, rect->left, rect->top,
		     rect->right - rect->left, rect->bottom - rect->top, 
		     DSTINVERT );
}


/***********************************************************************
 *           FrameRect    (USER32.203)
 */
INT WINAPI FrameRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;
    RECT r = *rect;

    LPtoDP(hdc, (POINT *)&r, 2);

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
 *           FrameRect16    (USER.83)
 */
INT16 WINAPI FrameRect16( HDC16 hdc, const RECT16 *rect16, HBRUSH16 hbrush )
{
    RECT rect;
    CONV_RECT16TO32( rect16, &rect );
    return FrameRect( hdc, &rect, hbrush );
}


/***********************************************************************
 *           DrawFocusRect16    (USER.466)
 */
void WINAPI DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT rect32;
    CONV_RECT16TO32( rc, &rect32 );
    DrawFocusRect( hdc, &rect32 );
}


/***********************************************************************
 *           DrawFocusRect    (USER32.156)
 *
 * FIXME: PatBlt(PATINVERT) with background brush.
 */
BOOL WINAPI DrawFocusRect( HDC hdc, const RECT* rc )
{
    HBRUSH hOldBrush;
    HPEN hOldPen, hNewPen;
    INT oldDrawMode, oldBkMode;

    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    hNewPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_WINDOWTEXT));
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
 *          DrawAnimatedRects16  (USER.448)
 */
BOOL16 WINAPI DrawAnimatedRects16( HWND16 hwnd, INT16 idAni,
                                   const RECT16* lprcFrom,
                                   const RECT16* lprcTo )
{
    RECT rcFrom32, rcTo32;

    rcFrom32.left	= (INT)lprcFrom->left;
    rcFrom32.top	= (INT)lprcFrom->top;
    rcFrom32.right	= (INT)lprcFrom->right;
    rcFrom32.bottom	= (INT)lprcFrom->bottom;

    rcTo32.left		= (INT)lprcTo->left;
    rcTo32.top		= (INT)lprcTo->top;
    rcTo32.right	= (INT)lprcTo->right;
    rcTo32.bottom	= (INT)lprcTo->bottom;

    return DrawAnimatedRects((HWND)hwnd, (INT)idAni, &rcFrom32, &rcTo32);
}


/**********************************************************************
 *          DrawAnimatedRects  (USER32.153)
 */
BOOL WINAPI DrawAnimatedRects( HWND hwnd, int idAni,
                                   const RECT* lprcFrom,
                                   const RECT* lprcTo )
{
    FIXME_(win)("(0x%x,%d,%p,%p): stub\n",hwnd,idAni,lprcFrom,lprcTo);
    return TRUE;
}


/**********************************************************************
 *          PAINTING_DrawStateJam
 *
 * Jams in the requested type in the dc
 */
static BOOL PAINTING_DrawStateJam(HDC hdc, UINT opcode,
                                    DRAWSTATEPROC func, LPARAM lp, WPARAM wp, 
                                    LPRECT rc, UINT dtflags,
                                    BOOL unicode, BOOL _32bit)
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
        else if(_32bit)
            return DrawTextA(hdc, (LPSTR)lp, (INT)wp, rc, dtflags);
        else
            return DrawTextA(hdc, (LPSTR)PTR_SEG_TO_LIN(lp), (INT)wp, rc, dtflags);

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
        if(func)
            if(_32bit)
                return func(hdc, lp, wp, cx, cy);
            else
                return (BOOL)((DRAWSTATEPROC16)func)((HDC16)hdc, (LPARAM)lp, (WPARAM16)wp, (INT16)cx, (INT16)cy);
        else
            return FALSE;
    }
    return FALSE;
}

/**********************************************************************
 *      PAINTING_DrawState()
 */
static BOOL PAINTING_DrawState(HDC hdc, HBRUSH hbr, 
                                   DRAWSTATEPROC func, LPARAM lp, WPARAM wp,
                                   INT x, INT y, INT cx, INT cy, 
                                   UINT flags, BOOL unicode, BOOL _32bit)
{
    HBITMAP hbm, hbmsave;
    HFONT hfsave;
    HBRUSH hbsave;
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
            len = lstrlenW((LPWSTR)lp);
        else if(_32bit)
            len = lstrlenA((LPSTR)lp);
        else
            len = lstrlenA((LPSTR)PTR_SEG_TO_LIN(lp));
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
            else if(_32bit)
                retval = GetTextExtentPoint32A(hdc, (LPSTR)lp, len, &s);
            else
                retval = GetTextExtentPoint32A(hdc, PTR_SEG_TO_LIN(lp), len, &s);
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
        return PAINTING_DrawStateJam(hdc, opcode, func, lp, len, &rc, dtflags, unicode, _32bit);
    }

    /* For all other states we need to convert the image to B/W in a local bitmap */
    /* before it is displayed */
    fg = SetTextColor(hdc, RGB(0, 0, 0));
    bg = SetBkColor(hdc, RGB(255, 255, 255));
    hbm = (HBITMAP)NULL; hbmsave = (HBITMAP)NULL;
    memdc = (HDC)NULL; hbsave = (HBRUSH)NULL;
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
    if(!hfsave && (opcode == DST_TEXT || opcode == DST_PREFIXTEXT)) goto cleanup;
    tmp = PAINTING_DrawStateJam(memdc, opcode, func, lp, len, &rc, dtflags, unicode, _32bit);
    if(hfsave) SelectObject(memdc, hfsave);
    if(!tmp) goto cleanup;
    
    /* These states cause the image to be dithered */
    if(flags & (DSS_UNION|DSS_DISABLED))
    {
        hbsave = (HBRUSH)SelectObject(memdc, CACHE_GetPattern55AABrush());
        if(!hbsave) goto cleanup;
        tmp = PatBlt(memdc, 0, 0, cx, cy, 0x00FA0089);
        if(hbsave) SelectObject(memdc, hbsave);
        if(!tmp) goto cleanup;
    }

    hbsave = (HBRUSH)SelectObject(hdc, hbr ? hbr : GetStockObject(WHITE_BRUSH));
    if(!hbsave) goto cleanup;
    
    if(!BitBlt(hdc, x, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
    
    /* DSS_DEFAULT makes the image boldface */
    if(flags & DSS_DEFAULT)
    {
        if(!BitBlt(hdc, x+1, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
    }

    retval = TRUE; /* We succeeded */
    
cleanup:    
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);

    if(hbsave)  SelectObject(hdc, hbsave);
    if(hbmsave) SelectObject(memdc, hbmsave);
    if(hbm)     DeleteObject(hbm);
    if(memdc)   DeleteDC(memdc);

    return retval;
}

/**********************************************************************
 *      DrawStateA()   (USER32.162)
 */
BOOL WINAPI DrawStateA(HDC hdc, HBRUSH hbr,
                   DRAWSTATEPROC func, LPARAM ldata, WPARAM wdata,
                   INT x, INT y, INT cx, INT cy, UINT flags)
{
    return PAINTING_DrawState(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, FALSE, TRUE);
}

/**********************************************************************
 *      DrawStateW()   (USER32.163)
 */
BOOL WINAPI DrawStateW(HDC hdc, HBRUSH hbr,
                   DRAWSTATEPROC func, LPARAM ldata, WPARAM wdata,
                   INT x, INT y, INT cx, INT cy, UINT flags)
{
    return PAINTING_DrawState(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, TRUE, TRUE);
}

/**********************************************************************
 *      DrawState16()   (USER.449)
 */
BOOL16 WINAPI DrawState16(HDC16 hdc, HBRUSH16 hbr,
                   DRAWSTATEPROC16 func, LPARAM ldata, WPARAM16 wdata,
                   INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags)
{
    return PAINTING_DrawState(hdc, hbr, (DRAWSTATEPROC)func, ldata, wdata, x, y, cx, cy, flags, FALSE, FALSE);
}



/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *			 1999 Alex Korobka
 *
 */

#include "region.h"
#include "win.h"
#include "queue.h"
#include "dce.h"
#include "heap.h"
#include "debugtools.h"
#include "wine/winuser16.h"

DECLARE_DEBUG_CHANNEL(nonclient)
DECLARE_DEBUG_CHANNEL(win)

/* client rect in window coordinates */

#define GETCLIENTRECTW( wnd, r )	(r).left = (wnd)->rectClient.left - (wnd)->rectWindow.left; \
					(r).top = (wnd)->rectClient.top - (wnd)->rectWindow.top; \
					(r).right = (wnd)->rectClient.right - (wnd)->rectWindow.left; \
					(r).bottom = (wnd)->rectClient.bottom - (wnd)->rectWindow.top

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC


/***********************************************************************
 *           WIN_UpdateNCRgn
 *
 * NOTE: Caller is responsible for the returned region.
 */
HRGN WIN_UpdateNCRgn(WND* wnd, BOOL bUpdate, BOOL bForceEntire )
{
    HRGN hClip = 0;

    TRACE_(nonclient)("hwnd %04x, hrgnUpdate %04x, ncf %i\n", 
                      wnd->hwndSelf, wnd->hrgnUpdate, (wnd->flags & WIN_NEEDS_NCPAINT)!=0 );

    /* desktop window doesn't have nonclient area */
    if(wnd == WIN_GetDesktop()) 
    {
        wnd->flags &= ~WIN_NEEDS_NCPAINT;
        WIN_ReleaseDesktop();
        return 0;
    }
    WIN_ReleaseDesktop();

    if ((wnd->hwndSelf == GetActiveWindow()) &&
        !(wnd->flags & WIN_NCACTIVATED) )
    {
	wnd->flags |= WIN_NCACTIVATED;
	bForceEntire = TRUE;
    }

    if( (wnd->flags & WIN_NEEDS_NCPAINT) && wnd->hrgnUpdate )
    {
	RECT r, r2, r3;

	GETCLIENTRECTW( wnd, r );

	TRACE_(nonclient)("\tclient box (%i,%i-%i,%i)\n", r.left, r.top, r.right, r.bottom );

	if( wnd->hrgnUpdate > 1 )
	{
	        GetRgnBox( wnd->hrgnUpdate, &r2 );
	        UnionRect( &r3, &r2, &r );
	        if( r3.left != r.left || r3.top != r.top || 
		    r3.right != r.right || r3.bottom != r.bottom )
	        {
	 	    hClip = CreateRectRgn( 0, 0, 0, 0 );
		    CombineRgn( hClip, wnd->hrgnUpdate, 0, RGN_COPY );
	        }
	}
	else 
		hClip = wnd->hrgnUpdate;

	if( wnd->hrgnUpdate > 1 )
	{
	    REGION_CropRgn( wnd->hrgnUpdate, wnd->hrgnUpdate, &r, NULL );

	    if( bUpdate )
	    {
		GetRgnBox( wnd->hrgnUpdate, &r3 );
		if( IsRectEmpty( &r3 ) )
		{
		    /* delete update region since all invalid 
		     * parts were in the nonclient area */

		    DeleteObject( wnd->hrgnUpdate );
		    wnd->hrgnUpdate = 0;
		    if(!(wnd->flags & WIN_INTERNAL_PAINT))
			QUEUE_DecPaintCount( wnd->hmemTaskQ );
		    wnd->flags &= ~WIN_NEEDS_ERASEBKGND;
		}
	    }
	}
	else /* entire client rect */
	   wnd->hrgnUpdate = CreateRectRgnIndirect( &r );
    }

    if(!hClip && bForceEntire ) hClip = 1;
    wnd->flags &= ~WIN_NEEDS_NCPAINT;

    if( hClip )
	SendMessageA( wnd->hwndSelf, WM_NCPAINT, hClip, 0L );

    return hClip;
}


/***********************************************************************
 *           BeginPaint16    (USER.39)
 */
HDC16 WINAPI BeginPaint16( HWND16 hwnd, LPPAINTSTRUCT16 lps ) 
{
    BOOL bIcon;
    HRGN hrgnUpdate;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;

    bIcon = (wndPtr->dwStyle & WS_MINIMIZE && wndPtr->class->hIcon);

    wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;

    if( (hrgnUpdate = WIN_UpdateNCRgn( wndPtr, FALSE, FALSE )) > 1 ) 
	DeleteObject( hrgnUpdate );

    if( ((hrgnUpdate = wndPtr->hrgnUpdate) != 0) || (wndPtr->flags & WIN_INTERNAL_PAINT))
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );

    wndPtr->hrgnUpdate = 0;
    wndPtr->flags &= ~WIN_INTERNAL_PAINT;

    HideCaret( hwnd );

    TRACE_(win)("hrgnUpdate = %04x, \n", hrgnUpdate);

    if (wndPtr->class->style & CS_PARENTDC)
    {
        /* Don't clip the output to the update region for CS_PARENTDC window */
	if( hrgnUpdate > 1 )
	    DeleteObject(hrgnUpdate);
        lps->hdc = GetDCEx16( hwnd, 0, DCX_WINDOWPAINT | DCX_USESTYLE |
                              (bIcon ? DCX_WINDOW : 0) );
    }
    else
    {
	if( hrgnUpdate )
	    OffsetRgn( hrgnUpdate, wndPtr->rectWindow.left - wndPtr->rectClient.left,
			           wndPtr->rectWindow.top - wndPtr->rectClient.top );
        lps->hdc = GetDCEx16(hwnd, hrgnUpdate, DCX_INTERSECTRGN |
                             DCX_WINDOWPAINT | DCX_USESTYLE |
                             (bIcon ? DCX_WINDOW : 0) );
    }

    TRACE_(win)("hdc = %04x\n", lps->hdc);

    if (!lps->hdc)
    {
        WARN_(win)("GetDCEx() failed in BeginPaint(), hwnd=%04x\n", hwnd);
        WIN_ReleaseWndPtr(wndPtr);
        return 0;
    }

    GetClipBox16( lps->hdc, &lps->rcPaint );

    TRACE_(win)("box = (%i,%i - %i,%i)\n", lps->rcPaint.left, lps->rcPaint.top,
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
 *           PAINT_RedrawWindow
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
BOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                           HRGN hrgnUpdate, UINT flags, UINT ex )
{
    BOOL bIcon;
    HRGN hrgn = 0, hrgn2 = 0;
    RECT r, r2;
    POINT pt;
    WND* wndPtr;
    WND **list, **ppWnd;

    if (!hwnd) hwnd = GetDesktopWindow();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!WIN_IsWindowDrawable( wndPtr, !(flags & RDW_FRAME) ) )
    {
        WIN_ReleaseWndPtr(wndPtr);
        return TRUE;  /* No redraw needed */
    }

    bIcon = (wndPtr->dwStyle & WS_MINIMIZE && wndPtr->class->hIcon);
    if (rectUpdate)
    {
        TRACE_(win)("%04x (%04x) %d,%d-%d,%d %04x flags=%04x, exflags=%04x\n",
                    hwnd, wndPtr->hrgnUpdate, rectUpdate->left, rectUpdate->top,
                    rectUpdate->right, rectUpdate->bottom, hrgnUpdate, flags, ex );
    }
    else
    {
	if( hrgnUpdate ) GetRgnBox( hrgnUpdate, &r );
	else SetRectEmpty( &r );
        TRACE_(win)("%04x (%04x) NULL %04x box (%i,%i-%i,%i) flags=%04x, exflags=%04x\n", 
	      hwnd, wndPtr->hrgnUpdate, hrgnUpdate, r.left, r.top, r.right, r.bottom, flags, ex);
    }

    if( flags & RDW_FRAME )
	r = wndPtr->rectWindow;
    else
	r = wndPtr->rectClient;
    if( ex & RDW_EX_XYWINDOW )
        OffsetRect( &r, -wndPtr->rectWindow.left, -wndPtr->rectWindow.top );
    else
	OffsetRect( &r, -wndPtr->rectClient.left, -wndPtr->rectClient.top );

    /* r is the rectangle we crop the supplied update rgn/rect with */

    GETCLIENTRECTW( wndPtr, r2 );
    pt.x = r2.left; pt.y = r2.top;

    if (flags & RDW_INVALIDATE)  /* ------------------------- Invalidate */
    {
	BOOL bHasUpdateRgn = (BOOL)wndPtr->hrgnUpdate;

	/* wndPtr->hrgnUpdate is in window coordinates, parameters are
	 * in client coordinates unless RDW_EX_XYWINDOW is set. 
	 */

	if( hrgnUpdate )
	{
	    hrgn = REGION_CropRgn( 0, hrgnUpdate, &r, (ex & RDW_EX_XYWINDOW) ? NULL : &pt );
	    GetRgnBox( hrgn, &r2 );
	    if( IsRectEmpty( &r2 ) )
		goto END;

	    if( wndPtr->hrgnUpdate == 0 )
	    {
		wndPtr->hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
		CombineRgn( wndPtr->hrgnUpdate, hrgn, 0, RGN_COPY );
	    }
	    else
		if( wndPtr->hrgnUpdate > 1 )
		    CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hrgn, RGN_OR );
	}
	else if( rectUpdate )
	{
	    if( !IntersectRect( &r2, &r, rectUpdate ) )
		goto END;
	    if( !(ex & RDW_EX_XYWINDOW) )
		OffsetRect( &r2, pt.x, pt.y );

rect2i:     /* r2 contains a rect to add to the update region */

	    hrgn = CreateRectRgnIndirect( &r2 );
	    if( wndPtr->hrgnUpdate == 0 )
		wndPtr->hrgnUpdate = CreateRectRgnIndirect( &r2 );
	    else
		if( wndPtr->hrgnUpdate > 1 )
		    REGION_UnionRectWithRgn( wndPtr->hrgnUpdate, &r2 );
	}
	else /* entire window or client depending on RDW_FRAME */
	{
	    hrgn = 1;
	    if( flags & RDW_FRAME )
	    {
		if( wndPtr->hrgnUpdate )
		    DeleteObject( wndPtr->hrgnUpdate );
		wndPtr->hrgnUpdate = 1;
	    }
	    else /* by default r2 contains client rect in window coordinates */
		goto rect2i;
	}

	if( !bHasUpdateRgn && wndPtr->hrgnUpdate && !(wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_IncPaintCount( wndPtr->hmemTaskQ );

	if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;
        if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags |= RDW_FRAME;  /* Force children frame invalidation */
    }
    else if (flags & RDW_VALIDATE)  /* ------------------------- Validate */
    {
        if (wndPtr->hrgnUpdate) /* need an update region in order to validate anything */
        {
	    if( hrgnUpdate || rectUpdate )
	    {
		if( hrgnUpdate )
		{
		    hrgn = REGION_CropRgn( hrgn, hrgnUpdate,  &r, (ex & RDW_EX_XYWINDOW) ? NULL : &pt );
		    GetRgnBox( hrgn, &r2 );
		    if( IsRectEmpty( &r2 ) )
			goto END;
		}
		else
		{
		    if( !IntersectRect( &r2, &r, rectUpdate ) )
			goto END;
		    if( !(ex & RDW_EX_XYWINDOW) )
			OffsetRect( &r2, pt.x, pt.y );
rect2v:
		    hrgn = CreateRectRgnIndirect( &r2 );
		}

		if( wndPtr->hrgnUpdate == 1 )
		{
		    wndPtr->hrgnUpdate = CreateRectRgn( 0, 0,
					 wndPtr->rectWindow.right - wndPtr->rectWindow.left,
					 wndPtr->rectWindow.bottom - wndPtr->rectWindow.top );
		}

		if( CombineRgn( wndPtr->hrgnUpdate, 
				wndPtr->hrgnUpdate, hrgn, RGN_DIFF ) == NULLREGION )
		{
		    DeleteObject( wndPtr->hrgnUpdate );
		    wndPtr->hrgnUpdate = 0;
		}
	    }
	    else /* entire window or client depending on RDW_FRAME */
            {
		wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;

		hrgn = 1;
		if( flags & RDW_FRAME ) 
		{
		    if( wndPtr->hrgnUpdate != 1 )
			DeleteObject( wndPtr->hrgnUpdate );
		    wndPtr->hrgnUpdate = 0;
		}
		else /* by default r2 contains client rect in window coordinates */
		    goto rect2v;
            }

            if (!wndPtr->hrgnUpdate &&  /* No more update region */
		!(wndPtr->flags & WIN_INTERNAL_PAINT) )
		    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
        }
        if (flags & RDW_NOFRAME) wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
	if (flags & RDW_NOERASE) wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
    }

    /* At this point hrgn contains new update region in window coordinates */

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

      /* Erase/update window */

    if (flags & RDW_UPDATENOW)
    {
        if (wndPtr->hrgnUpdate) /* wm_painticon wparam is 1 */
            SendMessage16( hwnd, (bIcon) ? WM_PAINTICON : WM_PAINT, bIcon, 0 );
    }
    else if ((flags & RDW_ERASENOW) || (ex & RDW_EX_TOPFRAME))
    {
	hrgn2 = WIN_UpdateNCRgn( wndPtr, TRUE, (ex & RDW_EX_TOPFRAME) );

        if( wndPtr->flags & WIN_NEEDS_ERASEBKGND )
        {
            HDC hdc;

	    if( hrgn2 > 1 )
	    {
		OffsetRgn( hrgn2, -pt.x, -pt.y );
		GetRgnBox( hrgn2, &r2 );
	    }
	    else
	    {
		hrgn2 = CreateRectRgn( 0, 0, 0, 0 );
		CombineRgn( hrgn2, wndPtr->hrgnUpdate, 0, RGN_COPY );
		OffsetRgn( hrgn2, -pt.x, -pt.y );
	    }
	    hdc = GetDCEx( hwnd, hrgn2,
                                 DCX_INTERSECTRGN | DCX_USESTYLE |
                                 DCX_KEEPCLIPRGN | DCX_WINDOWPAINT |
                                 (bIcon ? DCX_WINDOW : 0) );
            if (hdc)
            {
               if (SendMessage16( hwnd, (bIcon) ? WM_ICONERASEBKGND
						: WM_ERASEBKGND,
                                  (WPARAM16)hdc, 0 ))
                  wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
               ReleaseDC( hwnd, hdc );
            }
        }
    }

    if ( !IsWindow( hwnd ) )
    {
        WIN_ReleaseWndPtr(wndPtr);
        return TRUE;
    }

      /* Recursively process children */

    if (!(flags & RDW_NOCHILDREN) &&
        ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) &&
	!(wndPtr->dwStyle & WS_MINIMIZE) )
    {
        if( hrgnUpdate || rectUpdate )
	{
	   if( hrgn2 <= 1 )
	       hrgn2 = (ex & RDW_EX_USEHRGN) ? hrgnUpdate : 0;

           if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	   {
		POINT	delta = pt;

		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    WIN_UpdateWndPtr(&wndPtr,*ppWnd);
		    if (!IsWindow(wndPtr->hwndSelf)) continue;
		    if (wndPtr->dwStyle & WS_VISIBLE)
		    {
			r.left = wndPtr->rectWindow.left + delta.x;
			r.top = wndPtr->rectWindow.top + delta.y;
			r.right = wndPtr->rectWindow.right + delta.x;
			r.bottom = wndPtr->rectWindow.bottom + delta.y;

			pt.x = -r.left; pt.y = -r.top;

			hrgn2 = REGION_CropRgn( hrgn2, hrgn, &r, &pt );

			GetRgnBox( hrgn2, &r2 );
			if( !IsRectEmpty( &r2 ) )
			{
			    PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, hrgn2, flags,
						RDW_EX_USEHRGN | RDW_EX_XYWINDOW );
			}
		    }
		}
                WIN_ReleaseWinArray(list);
	   }
	}
        else
        {
	    if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	    {
		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    WIN_UpdateWndPtr(&wndPtr,*ppWnd);
		    if (IsWindow( wndPtr->hwndSelf ))
			PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, flags, 0 );
		}
                WIN_ReleaseWinArray(list);
	    }
	}

    }

END:
    if( hrgn2 > 1 && (hrgn2 != hrgnUpdate) )
	DeleteObject( hrgn2 );
    if( hrgn > 1 && (hrgn != hrgnUpdate) )
	DeleteObject( hrgn );
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
	    if (wndPtr->class->style & CS_OWNDC)
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



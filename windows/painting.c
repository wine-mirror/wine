/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *
 * FIXME: Do not repaint full nonclient area all the time. Instead, compute 
 *	  intersection with hrgnUpdate (which should be moved from client to 
 *	  window coords as well, lookup 'the pain' comment in the winpos.c).
 */

#include "win.h"
#include "queue.h"
#include "dce.h"
#include "heap.h"
#include "debug.h"
#include "wine/winuser16.h"

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC

/***********************************************************************
 *           WIN_UpdateNCArea
 *
 */
void WIN_UpdateNCArea(WND* wnd, BOOL bUpdate)
{
    POINT16 pt = {0, 0}; 
    HRGN hClip = 1;

    TRACE(nonclient,"hwnd %04x, hrgnUpdate %04x\n", 
                      wnd->hwndSelf, wnd->hrgnUpdate );

    /* desktop window doesn't have nonclient area */
    if(wnd == WIN_GetDesktop()) 
    {
        wnd->flags &= ~WIN_NEEDS_NCPAINT;
        WIN_ReleaseDesktop();
        return;
    }
    WIN_ReleaseDesktop();

    if( wnd->hrgnUpdate > 1 )
    {
	ClientToScreen16(wnd->hwndSelf, &pt);

        hClip = CreateRectRgn( 0, 0, 0, 0 );
        if (!CombineRgn( hClip, wnd->hrgnUpdate, 0, RGN_COPY ))
        {
            DeleteObject(hClip);
            hClip = 1;
        }
	else
	    OffsetRgn( hClip, pt.x, pt.y );

        if (bUpdate)
        {
	    /* exclude non-client area from update region */
            HRGN hrgn = CreateRectRgn( 0, 0,
                                 wnd->rectClient.right - wnd->rectClient.left,
                                 wnd->rectClient.bottom - wnd->rectClient.top);

            if (hrgn && (CombineRgn( wnd->hrgnUpdate, wnd->hrgnUpdate,
                                       hrgn, RGN_AND) == NULLREGION))
            {
                DeleteObject( wnd->hrgnUpdate );
                wnd->hrgnUpdate = 1;
            }

            DeleteObject( hrgn );
        }
    }

    wnd->flags &= ~WIN_NEEDS_NCPAINT;

    if ((wnd->hwndSelf == GetActiveWindow()) &&
        !(wnd->flags & WIN_NCACTIVATED))
    {
        wnd->flags |= WIN_NCACTIVATED;
        if( hClip > 1) DeleteObject( hClip );
        hClip = 1;
    }

    if (hClip) SendMessage16( wnd->hwndSelf, WM_NCPAINT, hClip, 0L );

    if (hClip > 1) DeleteObject( hClip );
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

    if (wndPtr->flags & WIN_NEEDS_NCPAINT) WIN_UpdateNCArea( wndPtr, TRUE );

    if (((hrgnUpdate = wndPtr->hrgnUpdate) != 0) ||
        (wndPtr->flags & WIN_INTERNAL_PAINT))
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );

    wndPtr->hrgnUpdate = 0;
    wndPtr->flags &= ~WIN_INTERNAL_PAINT;

    HideCaret( hwnd );

    TRACE(win,"hrgnUpdate = %04x, \n", hrgnUpdate);

    /* When bIcon is TRUE hrgnUpdate is automatically in window coordinates
     * (because rectClient == rectWindow for WS_MINIMIZE windows).
     */

    if (wndPtr->class->style & CS_PARENTDC)
    {
        /* Don't clip the output to the update region for CS_PARENTDC window */
	if(hrgnUpdate > 1)
	    DeleteObject(hrgnUpdate);
        lps->hdc = GetDCEx16( hwnd, 0, DCX_WINDOWPAINT | DCX_USESTYLE |
                              (bIcon ? DCX_WINDOW : 0) );
    }
    else
    {
        lps->hdc = GetDCEx16(hwnd, hrgnUpdate, DCX_INTERSECTRGN |
                             DCX_WINDOWPAINT | DCX_USESTYLE |
                             (bIcon ? DCX_WINDOW : 0) );
    }

    TRACE(win,"hdc = %04x\n", lps->hdc);

    if (!lps->hdc)
    {
        WARN(win, "GetDCEx() failed in BeginPaint(), hwnd=%04x\n", hwnd);
        WIN_ReleaseWndPtr(wndPtr);
        return 0;
    }

    GetClipBox16( lps->hdc, &lps->rcPaint );

TRACE(win,"box = (%i,%i - %i,%i)\n", lps->rcPaint.left, lps->rcPaint.top,
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
 * All in all, a prime candidate for a rewrite.
 */
BOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                           HRGN hrgnUpdate, UINT flags, UINT control )
{
    BOOL bIcon;
    HRGN hrgn;
    RECT rectClient;
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
        TRACE(win, "%04x %d,%d-%d,%d %04x flags=%04x\n",
                    hwnd, rectUpdate->left, rectUpdate->top,
                    rectUpdate->right, rectUpdate->bottom, hrgnUpdate, flags );
    }
    else
    {
        TRACE(win, "%04x NULL %04x flags=%04x\n", hwnd, hrgnUpdate, flags);
    }

    GetClientRect( hwnd, &rectClient );

    if (flags & RDW_INVALIDATE)  /* Invalidate */
    {
        int rgnNotEmpty = COMPLEXREGION;

        if (wndPtr->hrgnUpdate > 1)  /* Is there already an update region? */
        {
            if ((hrgn = hrgnUpdate) == 0)
                hrgn = CreateRectRgnIndirect( rectUpdate ? rectUpdate :
                                                &rectClient );
            rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate,
                                        hrgn, RGN_OR );
            if (!hrgnUpdate) DeleteObject( hrgn );
        }
        else  /* No update region yet */
        {
            if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
            if (hrgnUpdate)
            {
                wndPtr->hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
                rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, hrgnUpdate,
                                            0, RGN_COPY );
            }
            else wndPtr->hrgnUpdate = CreateRectRgnIndirect( rectUpdate ?
                                                    rectUpdate : &rectClient );
        }
	
        if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;

        /* restrict update region to client area (FIXME: correct?) */
        if (wndPtr->hrgnUpdate)
        {
            HRGN clientRgn = CreateRectRgnIndirect( &rectClient );
            rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, clientRgn, 
                                        wndPtr->hrgnUpdate, RGN_AND );
            DeleteObject( clientRgn );
        }

	/* check for bogus update region */ 
	if ( rgnNotEmpty == NULLREGION )
	   {
	     wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
	     DeleteObject( wndPtr->hrgnUpdate );
	     wndPtr->hrgnUpdate=0;
             if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                   QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
	   }
	else
             if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags |= RDW_FRAME;  /* Force children frame invalidation */
    }
    else if (flags & RDW_VALIDATE)  /* Validate */
    {
          /* We need an update region in order to validate anything */
        if (wndPtr->hrgnUpdate > 1)
        {
            if (!hrgnUpdate && !rectUpdate)
            {
                  /* Special case: validate everything */
                DeleteObject( wndPtr->hrgnUpdate );
                wndPtr->hrgnUpdate = 0;
            }
            else
            {
                if ((hrgn = hrgnUpdate) == 0)
                    hrgn = CreateRectRgnIndirect( rectUpdate );
                if (CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate,
                                  hrgn, RGN_DIFF ) == NULLREGION)
                {
                    DeleteObject( wndPtr->hrgnUpdate );
                    wndPtr->hrgnUpdate = 0;
                }
                if (!hrgnUpdate) DeleteObject( hrgn );
            }
            if (!wndPtr->hrgnUpdate)  /* No more update region */
		if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
		    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
        }
        if (flags & RDW_NOFRAME) wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
	if (flags & RDW_NOERASE) wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
    }

      /* Set/clear internal paint flag */

    if (flags & RDW_INTERNALPAINT)
    {
	if ( wndPtr->hrgnUpdate <= 1 && !(wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags |= WIN_INTERNAL_PAINT;	    
    }
    else if (flags & RDW_NOINTERNALPAINT)
    {
	if ( wndPtr->hrgnUpdate <= 1 && (wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags &= ~WIN_INTERNAL_PAINT;
    }

      /* Erase/update window */

    if (flags & RDW_UPDATENOW)
    {
        if (wndPtr->hrgnUpdate) /* wm_painticon wparam is 1 */
            SendMessage16( hwnd, (bIcon) ? WM_PAINTICON : WM_PAINT, bIcon, 0 );
    }
    else if (flags & RDW_ERASENOW)
    {
        if (wndPtr->flags & WIN_NEEDS_NCPAINT)
	    WIN_UpdateNCArea( wndPtr, FALSE);

        if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
        {
            HDC hdc = GetDCEx( hwnd, wndPtr->hrgnUpdate,
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

      /* Recursively process children */

    if (!(flags & RDW_NOCHILDREN) &&
        ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) &&
	!(wndPtr->dwStyle & WS_MINIMIZE) )
    {
        if ( hrgnUpdate || rectUpdate )
	{
            if (!(hrgn = CreateRectRgn( 0, 0, 0, 0 )))
	{
                WIN_ReleaseWndPtr(wndPtr);
                return TRUE;
            }
	   if( !hrgnUpdate )
           {
	        control |= (RDW_C_DELETEHRGN | RDW_C_USEHRGN);
 	        if( !(hrgnUpdate = CreateRectRgnIndirect( rectUpdate )) )
                {
                    DeleteObject( hrgn );
                    WIN_ReleaseWndPtr(wndPtr);
                    return TRUE;
                }
           }
           if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	   {
		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    WIN_UpdateWndPtr(&wndPtr,*ppWnd);
		    if (!IsWindow(wndPtr->hwndSelf)) continue;
		    if (wndPtr->dwStyle & WS_VISIBLE)
		    {
			SetRectRgn( hrgn, 
				wndPtr->rectWindow.left, wndPtr->rectWindow.top, 
				wndPtr->rectWindow.right, wndPtr->rectWindow.bottom );
			if (CombineRgn( hrgn, hrgn, hrgnUpdate, RGN_AND ))
			{
			    OffsetRgn( hrgn, -wndPtr->rectClient.left,
                                        -wndPtr->rectClient.top );
			    PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, hrgn, flags,
                                         RDW_C_USEHRGN );
			}
		    }
		}
                WIN_ReleaseWinArray(list);
	   }
	   DeleteObject( hrgn );
	   if (control & RDW_C_DELETEHRGN) DeleteObject( hrgnUpdate );
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
	else SetRectEmpty( rect );
    }
    retvalue = (wndPtr->hrgnUpdate > 1);
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
 *           GetUpdateRgn32   (USER32.298)
 */
INT WINAPI GetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    INT retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (wndPtr->hrgnUpdate <= 1)
    {
        SetRectRgn( hrgn, 0, 0, 0, 0 );
        retval = NULLREGION;
        goto END;
    }
    retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY );
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
				      wndPtr->rectClient.right - wndPtr->rectClient.left,
				      wndPtr->rectClient.bottom - wndPtr->rectClient.top);
	if( wndPtr->hrgnUpdate > 1 )
	    CombineRgn(hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY);

	/* do ugly coordinate translations in dce.c */

	ret = DCE_ExcludeRgn( hdc, wndPtr, hrgn );
	DeleteObject( hrgn );
        WIN_ReleaseWndPtr(wndPtr);
	return ret;
    } 
    WIN_ReleaseWndPtr(wndPtr);
    return GetClipBox( hdc, &rect );
}



/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *
 * FIXME: Do not repaint full nonclient area all the time. Instead, compute 
 *	  intersection with hrgnUpdate (which should be moved from client to 
 *	  window coords as well, lookup 'the pain' comment in the winpos.c).
 */

#include <stdio.h>
#include "win.h"
#include "queue.h"
#include "gdi.h"
#include "dce.h"
#include "heap.h"
#include "stddebug.h"
/* #define DEBUG_WIN */
#include "debug.h"

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC

/***********************************************************************
 *           WIN_UpdateNCArea
 *
 */
void WIN_UpdateNCArea(WND* wnd, BOOL32 bUpdate)
{
    POINT16 pt = {0, 0}; 
    HRGN32 hClip = 1;

    dprintf_nonclient(stddeb,"NCUpdate: hwnd %04x, hrgnUpdate %04x\n", 
                      wnd->hwndSelf, wnd->hrgnUpdate );

    /* desktop window doesn't have nonclient area */
    if(wnd == WIN_GetDesktop()) 
    {
        wnd->flags &= ~WIN_NEEDS_NCPAINT;
        return;
    }

    if( wnd->hrgnUpdate > 1 )
    {
	ClientToScreen16(wnd->hwndSelf, &pt);

        hClip = CreateRectRgn32( 0, 0, 0, 0 );
        if (!CombineRgn32( hClip, wnd->hrgnUpdate, 0, RGN_COPY ))
        {
            DeleteObject32(hClip);
            hClip = 1;
        }
	else
	    OffsetRgn32( hClip, pt.x, pt.y );

        if (bUpdate)
        {
	    /* exclude non-client area from update region */
            HRGN32 hrgn = CreateRectRgn32( 0, 0,
                                 wnd->rectClient.right - wnd->rectClient.left,
                                 wnd->rectClient.bottom - wnd->rectClient.top);

            if (hrgn && (CombineRgn32( wnd->hrgnUpdate, wnd->hrgnUpdate,
                                       hrgn, RGN_AND) == NULLREGION))
            {
                DeleteObject32( wnd->hrgnUpdate );
                wnd->hrgnUpdate = 1;
            }

            DeleteObject32( hrgn );
        }
    }

    wnd->flags &= ~WIN_NEEDS_NCPAINT;

    if ((wnd->hwndSelf == GetActiveWindow32()) &&
        !(wnd->flags & WIN_NCACTIVATED))
    {
        wnd->flags |= WIN_NCACTIVATED;
        if( hClip > 1) DeleteObject32( hClip );
        hClip = 1;
    }

    if (hClip) SendMessage16( wnd->hwndSelf, WM_NCPAINT, hClip, 0L );

    if (hClip > 1) DeleteObject32( hClip );
}


/***********************************************************************
 *           BeginPaint16    (USER.39)
 */
HDC16 WINAPI BeginPaint16( HWND16 hwnd, LPPAINTSTRUCT16 lps ) 
{
    BOOL32 bIcon;
    HRGN32 hrgnUpdate;
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

    HideCaret32( hwnd );

    dprintf_win(stddeb,"hrgnUpdate = %04x, ", hrgnUpdate);

    /* When bIcon is TRUE hrgnUpdate is automatically in window coordinates
     * (because rectClient == rectWindow for WS_MINIMIZE windows).
     */

    if (wndPtr->class->style & CS_PARENTDC)
    {
        /* Don't clip the output to the update region for CS_PARENTDC window */
	if(hrgnUpdate > 1)
	    DeleteObject32(hrgnUpdate);
        lps->hdc = GetDCEx16( hwnd, 0, DCX_WINDOWPAINT | DCX_USESTYLE |
                              (bIcon ? DCX_WINDOW : 0) );
    }
    else
    {
        lps->hdc = GetDCEx16(hwnd, hrgnUpdate, DCX_INTERSECTRGN |
                             DCX_WINDOWPAINT | DCX_USESTYLE |
                             (bIcon ? DCX_WINDOW : 0) );
    }

    dprintf_win(stddeb,"hdc = %04x\n", lps->hdc);

    if (!lps->hdc)
    {
        fprintf(stderr, "GetDCEx() failed in BeginPaint(), hwnd=%04x\n", hwnd);
        return 0;
    }

    GetRgnBox16( InquireVisRgn(lps->hdc), &lps->rcPaint );

dprintf_win(stddeb,"box = (%i,%i - %i,%i)\n", lps->rcPaint.left, lps->rcPaint.top,
		    lps->rcPaint.right, lps->rcPaint.bottom );

    DPtoLP16( lps->hdc, (LPPOINT16)&lps->rcPaint, 2 );

    if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
    {
        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
        lps->fErase = !SendMessage16(hwnd, (bIcon) ? WM_ICONERASEBKGND
                                                   : WM_ERASEBKGND,
                                     (WPARAM16)lps->hdc, 0 );
    }
    else lps->fErase = TRUE;

    return lps->hdc;
}


/***********************************************************************
 *           BeginPaint32    (USER32.9)
 */
HDC32 WINAPI BeginPaint32( HWND32 hwnd, PAINTSTRUCT32 *lps )
{
    PAINTSTRUCT16 ps;

    BeginPaint16( hwnd, &ps );
    lps->hdc            = (HDC32)ps.hdc;
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
    ShowCaret32( hwnd );
    return TRUE;
}


/***********************************************************************
 *           EndPaint32    (USER32.175)
 */
BOOL32 WINAPI EndPaint32( HWND32 hwnd, const PAINTSTRUCT32 *lps )
{
    ReleaseDC32( hwnd, lps->hdc );
    ShowCaret32( hwnd );
    return TRUE;
}


/***********************************************************************
 *           FillWindow    (USER.324)
 */
void WINAPI FillWindow( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc, HBRUSH16 hbrush )
{
    RECT16 rect;
    GetClientRect16( hwnd, &rect );
    DPtoLP16( hdc, (LPPOINT16)&rect, 2 );
    PaintRect( hwndParent, hwnd, hdc, hbrush, &rect );
}


/***********************************************************************
 *	     PAINT_GetControlBrush
 */
static HBRUSH16 PAINT_GetControlBrush( HWND32 hParent, HWND32 hWnd, HDC16 hDC, UINT16 ctlType )
{
    HBRUSH16 bkgBrush = (HBRUSH16)SendMessage32A( hParent, WM_CTLCOLORMSGBOX + ctlType, 
							     (WPARAM32)hDC, (LPARAM)hWnd );
    if( !IsGDIObject(bkgBrush) )
	bkgBrush = DEFWND_ControlColor( hDC, ctlType );
    return bkgBrush;
}


/***********************************************************************
 *           PaintRect    (USER.325)
 */
void WINAPI PaintRect( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc,
                       HBRUSH16 hbrush, const RECT16 *rect)
{
    if( hbrush <= CTLCOLOR_MAX ) 
	if( hwndParent )
	    hbrush = PAINT_GetControlBrush( hwndParent, hwnd, hdc, (UINT16)hbrush );
	else 
	    return;
    if( hbrush ) 
	FillRect16( hdc, rect, hbrush );
}


/***********************************************************************
 *           GetControlBrush    (USER.326)
 */
HBRUSH16 WINAPI GetControlBrush( HWND16 hwnd, HDC16 hdc, UINT16 ctlType )
{
    WND* wndPtr = WIN_FindWndPtr( hwnd );

    if((ctlType <= CTLCOLOR_MAX) && wndPtr )
    {
	WND* parent;
	if( wndPtr->dwStyle & WS_POPUP ) parent = wndPtr->owner;
	else parent = wndPtr->parent;
	if( !parent ) parent = wndPtr;
	return (HBRUSH16)PAINT_GetControlBrush( parent->hwndSelf, hwnd, hdc, ctlType );
    }
    return (HBRUSH16)0;
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
BOOL32 PAINT_RedrawWindow( HWND32 hwnd, const RECT32 *rectUpdate,
                           HRGN32 hrgnUpdate, UINT32 flags, UINT32 control )
{
    BOOL32 bIcon;
    HRGN32 hrgn;
    RECT32 rectClient;
    WND* wndPtr;
    WND **list, **ppWnd;

    if (!hwnd) hwnd = GetDesktopWindow32();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!WIN_IsWindowDrawable( wndPtr, !(flags & RDW_FRAME) ) )
        return TRUE;  /* No redraw needed */

    bIcon = (wndPtr->dwStyle & WS_MINIMIZE && wndPtr->class->hIcon);
    if (rectUpdate)
    {
        dprintf_win(stddeb, "RedrawWindow: %04x %d,%d-%d,%d %04x flags=%04x\n",
                    hwnd, rectUpdate->left, rectUpdate->top,
                    rectUpdate->right, rectUpdate->bottom, hrgnUpdate, flags );
    }
    else
    {
        dprintf_win(stddeb, "RedrawWindow: %04x NULL %04x flags=%04x\n",
                     hwnd, hrgnUpdate, flags);
    }

    GetClientRect32( hwnd, &rectClient );

    if (flags & RDW_INVALIDATE)  /* Invalidate */
    {
        int rgnNotEmpty = COMPLEXREGION;

        if (wndPtr->hrgnUpdate > 1)  /* Is there already an update region? */
        {
            if ((hrgn = hrgnUpdate) == 0)
                hrgn = CreateRectRgnIndirect32( rectUpdate ? rectUpdate :
                                                &rectClient );
            rgnNotEmpty = CombineRgn32( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate,
                                        hrgn, RGN_OR );
            if (!hrgnUpdate) DeleteObject32( hrgn );
        }
        else  /* No update region yet */
        {
            if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
            if (hrgnUpdate)
            {
                wndPtr->hrgnUpdate = CreateRectRgn32( 0, 0, 0, 0 );
                rgnNotEmpty = CombineRgn32( wndPtr->hrgnUpdate, hrgnUpdate,
                                            0, RGN_COPY );
            }
            else wndPtr->hrgnUpdate = CreateRectRgnIndirect32( rectUpdate ?
                                                    rectUpdate : &rectClient );
        }
	
        if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;

        /* restrict update region to client area (FIXME: correct?) */
        if (wndPtr->hrgnUpdate)
        {
            HRGN32 clientRgn = CreateRectRgnIndirect32( &rectClient );
            rgnNotEmpty = CombineRgn32( wndPtr->hrgnUpdate, clientRgn, 
                                        wndPtr->hrgnUpdate, RGN_AND );
            DeleteObject32( clientRgn );
        }

	/* check for bogus update region */ 
	if ( rgnNotEmpty == NULLREGION )
	   {
	     wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
	     DeleteObject32( wndPtr->hrgnUpdate );
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
                DeleteObject32( wndPtr->hrgnUpdate );
                wndPtr->hrgnUpdate = 0;
            }
            else
            {
                if ((hrgn = hrgnUpdate) == 0)
                    hrgn = CreateRectRgnIndirect32( rectUpdate );
                if (CombineRgn32( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate,
                                  hrgn, RGN_DIFF ) == NULLREGION)
                {
                    DeleteObject32( wndPtr->hrgnUpdate );
                    wndPtr->hrgnUpdate = 0;
                }
                if (!hrgnUpdate) DeleteObject32( hrgn );
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
            HDC32 hdc = GetDCEx32( hwnd, wndPtr->hrgnUpdate,
                                   DCX_INTERSECTRGN | DCX_USESTYLE |
                                   DCX_KEEPCLIPRGN | DCX_WINDOWPAINT |
                                   (bIcon ? DCX_WINDOW : 0) );
            if (hdc)
            {
               if (SendMessage16( hwnd, (bIcon) ? WM_ICONERASEBKGND
						: WM_ERASEBKGND,
                                  (WPARAM16)hdc, 0 ))
                  wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
               ReleaseDC32( hwnd, hdc );
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
	   if (!(hrgn = CreateRectRgn32( 0, 0, 0, 0 ))) return TRUE;
	   if( !hrgnUpdate )
           {
	        control |= (RDW_C_DELETEHRGN | RDW_C_USEHRGN);
 	        if( !(hrgnUpdate = CreateRectRgnIndirect32( rectUpdate )) )
                {
                    DeleteObject32( hrgn );
                    return TRUE;
                }
           }
           if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	   {
		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    wndPtr = *ppWnd;
		    if (!IsWindow32(wndPtr->hwndSelf)) continue;
		    if (wndPtr->dwStyle & WS_VISIBLE)
		    {
			SetRectRgn32( hrgn, 
				wndPtr->rectWindow.left, wndPtr->rectWindow.top, 
				wndPtr->rectWindow.right, wndPtr->rectWindow.bottom );
			if (CombineRgn32( hrgn, hrgn, hrgnUpdate, RGN_AND ))
			{
			    OffsetRgn32( hrgn, -wndPtr->rectClient.left,
                                        -wndPtr->rectClient.top );
			    PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, hrgn, flags,
                                         RDW_C_USEHRGN );
			}
		    }
		}
		HeapFree( SystemHeap, 0, list );
	   }
	   DeleteObject32( hrgn );
	   if (control & RDW_C_DELETEHRGN) DeleteObject32( hrgnUpdate );
	}
        else
        {
	    if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	    {
		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    wndPtr = *ppWnd;
		    if (IsWindow32( wndPtr->hwndSelf ))
			PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, flags, 0 );
		}
	        HeapFree( SystemHeap, 0, list );
	    }
	}

    }
    return TRUE;
}


/***********************************************************************
 *           RedrawWindow32    (USER32.425)
 */
BOOL32 WINAPI RedrawWindow32( HWND32 hwnd, const RECT32 *rectUpdate,
                              HRGN32 hrgnUpdate, UINT32 flags )
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
        RECT32 r;
        CONV_RECT16TO32( rectUpdate, &r );
        return (BOOL16)RedrawWindow32( (HWND32)hwnd, &r, hrgnUpdate, flags );
    }
    return (BOOL16)PAINT_RedrawWindow( (HWND32)hwnd, NULL, 
				       (HRGN32)hrgnUpdate, flags, 0 );
}


/***********************************************************************
 *           UpdateWindow16   (USER.124)
 */
void WINAPI UpdateWindow16( HWND16 hwnd )
{
    PAINT_RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           UpdateWindow32   (USER32.566)
 */
void WINAPI UpdateWindow32( HWND32 hwnd )
{
    PAINT_RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           InvalidateRgn16   (USER.126)
 */
void WINAPI InvalidateRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 erase )
{
    PAINT_RedrawWindow((HWND32)hwnd, NULL, (HRGN32)hrgn, 
		       RDW_INVALIDATE | (erase ? RDW_ERASE : 0), 0 );
}


/***********************************************************************
 *           InvalidateRgn32   (USER32.328)
 */
void WINAPI InvalidateRgn32( HWND32 hwnd, HRGN32 hrgn, BOOL32 erase )
{
    PAINT_RedrawWindow(hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0), 0 );
}


/***********************************************************************
 *           InvalidateRect16   (USER.125)
 */
void WINAPI InvalidateRect16( HWND16 hwnd, const RECT16 *rect, BOOL16 erase )
{
    RedrawWindow16( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           InvalidateRect32   (USER32.327)
 */
void WINAPI InvalidateRect32( HWND32 hwnd, const RECT32 *rect, BOOL32 erase )
{
    PAINT_RedrawWindow( hwnd, rect, 0, 
			RDW_INVALIDATE | (erase ? RDW_ERASE : 0), 0 );
}


/***********************************************************************
 *           ValidateRgn16   (USER.128)
 */
void WINAPI ValidateRgn16( HWND16 hwnd, HRGN16 hrgn )
{
    PAINT_RedrawWindow( (HWND32)hwnd, NULL, (HRGN32)hrgn, 
			RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}


/***********************************************************************
 *           ValidateRgn32   (USER32.571)
 */
void WINAPI ValidateRgn32( HWND32 hwnd, HRGN32 hrgn )
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
 *           ValidateRect32   (USER32.570)
 */
void WINAPI ValidateRect32( HWND32 hwnd, const RECT32 *rect )
{
    PAINT_RedrawWindow( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}


/***********************************************************************
 *           GetUpdateRect16   (USER.190)
 */
BOOL16 WINAPI GetUpdateRect16( HWND16 hwnd, LPRECT16 rect, BOOL16 erase )
{
    RECT32 r;
    BOOL16 ret;

    if (!rect) return GetUpdateRect32( hwnd, NULL, erase );
    ret = GetUpdateRect32( hwnd, &r, erase );
    CONV_RECT32TO16( &r, rect );
    return ret;
}


/***********************************************************************
 *           GetUpdateRect32   (USER32.296)
 */
BOOL32 WINAPI GetUpdateRect32( HWND32 hwnd, LPRECT32 rect, BOOL32 erase )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate > 1)
	{
	    HRGN32 hrgn = CreateRectRgn32( 0, 0, 0, 0 );
	    if (GetUpdateRgn32( hwnd, hrgn, erase ) == ERROR) return FALSE;
	    GetRgnBox32( hrgn, rect );
	    DeleteObject32( hrgn );
	}
	else SetRectEmpty32( rect );
    }
    return (wndPtr->hrgnUpdate > 1);
}


/***********************************************************************
 *           GetUpdateRgn16   (USER.237)
 */
INT16 WINAPI GetUpdateRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 erase )
{
    return GetUpdateRgn32( hwnd, hrgn, erase );
}


/***********************************************************************
 *           GetUpdateRgn32   (USER32.297)
 */
INT32 WINAPI GetUpdateRgn32( HWND32 hwnd, HRGN32 hrgn, BOOL32 erase )
{
    INT32 retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (wndPtr->hrgnUpdate <= 1)
    {
        SetRectRgn32( hrgn, 0, 0, 0, 0 );
        return NULLREGION;
    }
    retval = CombineRgn32( hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY );
    if (erase) RedrawWindow32( hwnd, NULL, 0, RDW_ERASENOW | RDW_NOCHILDREN );
    return retval;
}


/***********************************************************************
 *           ExcludeUpdateRgn16   (USER.238)
 */
INT16 WINAPI ExcludeUpdateRgn16( HDC16 hdc, HWND16 hwnd )
{
    return ExcludeUpdateRgn32( hdc, hwnd );
}


/***********************************************************************
 *           ExcludeUpdateRgn32   (USER32.194)
 */
INT32 WINAPI ExcludeUpdateRgn32( HDC32 hdc, HWND32 hwnd )
{
    RECT32 rect;
    WND * wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return ERROR;

    if (wndPtr->hrgnUpdate)
    {
	INT32 ret;
	HRGN32 hrgn = CreateRectRgn32(wndPtr->rectWindow.left - wndPtr->rectClient.left,
				      wndPtr->rectWindow.top - wndPtr->rectClient.top,
				      wndPtr->rectClient.right - wndPtr->rectClient.left,
				      wndPtr->rectClient.bottom - wndPtr->rectClient.top);
	if( wndPtr->hrgnUpdate > 1 )
	    CombineRgn32(hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY);

	/* do ugly coordinate translations in dce.c */

	ret = DCE_ExcludeRgn( hdc, wndPtr, hrgn );
	DeleteObject32( hrgn );
	return ret;
    } 
    return GetClipBox32( hdc, &rect );
}



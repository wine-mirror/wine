/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <X11/Xlib.h>

#include "win.h"
#include "queue.h"
#include "gdi.h"
#include "dce.h"
#include "stddebug.h"
/* #define DEBUG_WIN */
#include "debug.h"

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC

/***********************************************************************
 *           WIN_UpdateNCArea
 *
 */
void WIN_UpdateNCArea(WND* wnd, BOOL bUpdate)
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

        hClip = CreateRectRgn( 0, 0, 0, 0 );
        if (!CombineRgn(hClip, wnd->hrgnUpdate, 0, RGN_COPY) )
        {
            DeleteObject(hClip);
            hClip = 1;
        }
	else
	    OffsetRgn(hClip, pt.x, pt.y);

        if (bUpdate)
        {
	    /* exclude non-client area from update region */
            HRGN32 hrgn = CreateRectRgn(0, 0, wnd->rectClient.right - wnd->rectClient.left,
					    wnd->rectClient.bottom - wnd->rectClient.top);

            if (hrgn && (CombineRgn(wnd->hrgnUpdate, wnd->hrgnUpdate,
                                    hrgn, RGN_AND) == NULLREGION))
            {
                DeleteObject(wnd->hrgnUpdate);
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
        if( hClip > 1) DeleteObject(hClip);
        hClip = 1;
    }

    if (hClip) SendMessage16( wnd->hwndSelf, WM_NCPAINT, hClip, 0L );

    if (hClip > 1) DeleteObject( hClip );
}


/***********************************************************************
 *           BeginPaint16    (USER.39)
 */
HDC16 BeginPaint16( HWND16 hwnd, LPPAINTSTRUCT16 lps ) 
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

    HideCaret( hwnd );

    dprintf_win(stddeb,"hrgnUpdate = %04x, ", hrgnUpdate);

    /* When bIcon is TRUE hrgnUpdate is automatically in window coordinates
     * (because rectClient == rectWindow for WS_MINIMIZE windows).
     */

    lps->hdc = GetDCEx16(hwnd, hrgnUpdate, DCX_INTERSECTRGN | DCX_WINDOWPAINT |
                         DCX_USESTYLE | (bIcon ? DCX_WINDOW : 0) );

    dprintf_win(stddeb,"hdc = %04x\n", lps->hdc);

    if (!lps->hdc)
    {
        fprintf(stderr, "GetDCEx() failed in BeginPaint(), hwnd=%04x\n", hwnd);
        return 0;
    }

    GetRgnBox16( InquireVisRgn(lps->hdc), &lps->rcPaint );
    DPtoLP16( lps->hdc, (LPPOINT16)&lps->rcPaint, 2 );

    if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
    {
        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
        lps->fErase = !SendMessage16(hwnd, (bIcon) ? WM_ICONERASEBKGND
                                                   : WM_ERASEBKGND,
                                     (WPARAM)lps->hdc, 0 );
    }
    else lps->fErase = TRUE;

    return lps->hdc;
}


/***********************************************************************
 *           BeginPaint32    (USER32.9)
 */
HDC32 BeginPaint32( HWND32 hwnd, PAINTSTRUCT32 *lps )
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
BOOL16 EndPaint16( HWND16 hwnd, const PAINTSTRUCT16* lps )
{
    ReleaseDC16( hwnd, lps->hdc );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *           EndPaint32    (USER32.175)
 */
BOOL32 EndPaint32( HWND32 hwnd, const PAINTSTRUCT32 *lps )
{
    ReleaseDC32( hwnd, lps->hdc );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *           FillWindow    (USER.324)
 */
void FillWindow( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc, HBRUSH16 hbrush )
{
    RECT16 rect;
    GetClientRect16( hwnd, &rect );
    DPtoLP16( hdc, (LPPOINT16)&rect, 2 );
    PaintRect( hwndParent, hwnd, hdc, hbrush, &rect );
}


/***********************************************************************
 *           PaintRect    (USER.325)
 */
void PaintRect( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc,
                HBRUSH16 hbrush, const RECT16 *rect)
{
      /* Send WM_CTLCOLOR message if needed */

    if ((UINT32)hbrush <= CTLCOLOR_MAX)
    {
	if (!hwndParent) return;
	hbrush = (HBRUSH16)SendMessage32A( hwndParent, 
                                           WM_CTLCOLORMSGBOX + (UINT32)hbrush,
                                           (WPARAM)hdc, (LPARAM)hwnd );
    }
    if (hbrush) FillRect16( hdc, rect, hbrush );
}


/***********************************************************************
 *           GetControlBrush    (USER.326)
 */
HBRUSH16 GetControlBrush( HWND hwnd, HDC hdc, WORD control )
{
    return (HBRUSH16)SendMessage32A( GetParent32(hwnd), WM_CTLCOLOR+control,
                                     (WPARAM)hdc, (LPARAM)hwnd );
}


/***********************************************************************
 *           PAINT_RedrawWindow
 *
 * Note: Windows uses WM_SYNCPAINT to cut down the number of intertask
 * SendMessage() calls. From SDK:
 *   This message avoids lots of inter-app message traffic
 *   by switching to the other task and continuing the
 *   recursion there.
 * 
 * wParam         = flags
 * LOWORD(lParam) = hrgnClip
 * HIWORD(lParam) = hwndSkip  (not used; always NULL)
 */
BOOL32 PAINT_RedrawWindow( HWND32 hwnd, const RECT32 *rectUpdate,
                           HRGN32 hrgnUpdate, UINT32 flags, UINT32 control )
{
    BOOL32 bIcon;
    HRGN32 hrgn;
    RECT32 rectClient;
    WND* wndPtr;

    if (!hwnd) hwnd = GetDesktopWindow32();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!IsWindowVisible(hwnd) || (wndPtr->flags & WIN_NO_REDRAW))
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

    if (wndPtr->class->style & CS_PARENTDC)
    {
        GetClientRect32( wndPtr->parent->hwndSelf, &rectClient );
        OffsetRect32( &rectClient, -wndPtr->rectClient.left,
                      -wndPtr->rectClient.top );
    }
    else GetClientRect32( hwnd, &rectClient );

    if (flags & RDW_INVALIDATE)  /* Invalidate */
    {
        int rgnNotEmpty = COMPLEXREGION;

        if (wndPtr->hrgnUpdate > 1)  /* Is there already an update region? */
        {
            if ((hrgn = hrgnUpdate) == 0)
                hrgn = CreateRectRgnIndirect32( rectUpdate ? rectUpdate :
                                                &rectClient );
            rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hrgn, RGN_OR );
            if (!hrgnUpdate) DeleteObject( hrgn );
        }
        else  /* No update region yet */
        {
            if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
            if (hrgnUpdate)
            {
                wndPtr->hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
                rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, hrgnUpdate, 0, RGN_COPY );
            }
            else wndPtr->hrgnUpdate = CreateRectRgnIndirect32( rectUpdate ?
                                                    rectUpdate : &rectClient );
        }
	
        if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;

	/* check for bogus update region */ 
	if ( rgnNotEmpty == NULLREGION )
	   {
	     wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
	     DeleteObject(wndPtr->hrgnUpdate);
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
                    hrgn = CreateRectRgnIndirect32( rectUpdate );
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
            HDC32 hdc = GetDCEx32( hwnd, wndPtr->hrgnUpdate,
                                   DCX_INTERSECTRGN | DCX_USESTYLE |
                                   DCX_KEEPCLIPRGN | DCX_WINDOWPAINT |
                                   (bIcon ? DCX_WINDOW : 0) );
            if (hdc)
            {
               if (SendMessage16( hwnd, (bIcon) ? WM_ICONERASEBKGND
						: WM_ERASEBKGND,
                                  (WPARAM)hdc, 0 ))
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
	   if( !(hrgn = CreateRectRgn( 0, 0, 0, 0 )) ) return TRUE;
	   if( !hrgnUpdate )
	     {
	        control |= (RDW_C_DELETEHRGN | RDW_C_USEHRGN);
 	        if( !(hrgnUpdate = CreateRectRgnIndirect32( rectUpdate )) )
                {
                    DeleteObject( hrgn );
                    return TRUE;
                }
	     }
           for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
	     if( wndPtr->dwStyle & WS_VISIBLE )
	       {
                   if (wndPtr->class->style & CS_PARENTDC)
                   {
                       if (!CombineRgn( hrgn, hrgnUpdate, 0, RGN_COPY ))
                           continue;
                   }
                   else
                   {
                       SetRectRgn( hrgn, wndPtr->rectWindow.left,
                                   wndPtr->rectWindow.top,
                                   wndPtr->rectWindow.right,
                                   wndPtr->rectWindow.bottom);
                       if (!CombineRgn( hrgn, hrgn, hrgnUpdate, RGN_AND ))
                           continue;
                   }
#if 0
                   if( control & RDW_C_USEHRGN &&
                       wndPtr->dwStyle & WS_CLIPSIBLINGS ) 
                       CombineRgn( hrgnUpdate, hrgnUpdate, hrgn, RGN_DIFF );
#endif

                   OffsetRgn( hrgn, -wndPtr->rectClient.left,
                                 -wndPtr->rectClient.top );
                   PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, hrgn, flags, RDW_C_USEHRGN );
               }
	   DeleteObject( hrgn );
	   if( control & RDW_C_DELETEHRGN ) DeleteObject( hrgnUpdate );
	}
	else for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
		  PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, flags, 0 );

    }
    return TRUE;
}


/***********************************************************************
 *           RedrawWindow32    (USER32.425)
 */
BOOL32 RedrawWindow32( HWND32 hwnd, const RECT32 *rectUpdate,
                       HRGN32 hrgnUpdate, UINT32 flags )
{
    WND* wnd = WIN_FindWndPtr( hwnd );

    /* check if there is something to redraw */

    return ( wnd && WIN_IsWindowDrawable( wnd, !(flags & RDW_FRAME) ) )
           ? PAINT_RedrawWindow( hwnd, rectUpdate, hrgnUpdate, flags, 0 )
	   : 1;
}


/***********************************************************************
 *           RedrawWindow16    (USER.290)
 */
BOOL16 RedrawWindow16( HWND16 hwnd, const RECT16 *rectUpdate,
                       HRGN16 hrgnUpdate, UINT16 flags )
{
    if (rectUpdate)
    {
        RECT32 r;
        CONV_RECT16TO32( rectUpdate, &r );
        return (BOOL16)RedrawWindow32( (HWND32)hwnd, &r, hrgnUpdate, flags );
    }
    return (BOOL16)RedrawWindow32( (HWND32)hwnd, NULL, hrgnUpdate, flags );
}


/***********************************************************************
 *           UpdateWindow   (USER.124) (USER32.566)
 */
void UpdateWindow( HWND32 hwnd )
{
    RedrawWindow32( hwnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN );
}


/***********************************************************************
 *           InvalidateRgn   (USER.126) (USER32.328)
 */
void InvalidateRgn( HWND32 hwnd, HRGN32 hrgn, BOOL32 erase )
{
    RedrawWindow32(hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           InvalidateRect16   (USER.125)
 */
void InvalidateRect16( HWND16 hwnd, const RECT16 *rect, BOOL16 erase )
{
    RedrawWindow16( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           InvalidateRect32   (USER32.327)
 */
void InvalidateRect32( HWND32 hwnd, const RECT32 *rect, BOOL32 erase )
{
    RedrawWindow32( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           ValidateRgn   (USER.128) (USER32.571)
 */
void ValidateRgn( HWND32 hwnd, HRGN32 hrgn )
{
    RedrawWindow32( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *           ValidateRect16   (USER.127)
 */
void ValidateRect16( HWND16 hwnd, const RECT16 *rect )
{
    RedrawWindow16( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *           ValidateRect32   (USER32.570)
 */
void ValidateRect32( HWND32 hwnd, const RECT32 *rect )
{
    RedrawWindow32( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *           GetUpdateRect16   (USER.190)
 */
BOOL16 GetUpdateRect16( HWND16 hwnd, LPRECT16 rect, BOOL16 erase )
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
BOOL32 GetUpdateRect32( HWND32 hwnd, LPRECT32 rect, BOOL32 erase )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate > 1)
	{
	    HRGN32 hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (GetUpdateRgn( hwnd, hrgn, erase ) == ERROR) return FALSE;
	    GetRgnBox32( hrgn, rect );
	    DeleteObject( hrgn );
	}
	else SetRectEmpty32( rect );
    }
    return (wndPtr->hrgnUpdate > 1);
}


/***********************************************************************
 *           GetUpdateRgn   (USER.237) (USER32.297)
 */
INT16 GetUpdateRgn( HWND32 hwnd, HRGN32 hrgn, BOOL32 erase )
{
    INT16 retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (wndPtr->hrgnUpdate <= 1)
    {
        SetRectRgn( hrgn, 0, 0, 0, 0 );
        return NULLREGION;
    }
    retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY );
    if (erase) RedrawWindow32( hwnd, NULL, 0, RDW_ERASENOW | RDW_NOCHILDREN );
    return retval;
}


/***********************************************************************
 *           ExcludeUpdateRgn   (USER.238) (USER32.194)
 */
INT16 ExcludeUpdateRgn( HDC32 hdc, HWND32 hwnd )
{
    INT16 retval = ERROR;
    HRGN32 hrgn;
    WND * wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return ERROR;
    if ((hrgn = CreateRectRgn( 0, 0, 0, 0 )) != 0)
    {
	retval = CombineRgn( hrgn, InquireVisRgn(hdc),
			     (wndPtr->hrgnUpdate>1)?wndPtr->hrgnUpdate:0,
			     (wndPtr->hrgnUpdate>1)?RGN_DIFF:RGN_COPY);
	if (retval) SelectVisRgn( hdc, hrgn );
	DeleteObject( hrgn );
    }
    return retval;
}



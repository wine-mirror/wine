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
#include "stddebug.h"
/* #define DEBUG_WIN */
#include "debug.h"

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC


/***********************************************************************
 *           BeginPaint    (USER.39)
 */
HDC BeginPaint( HWND hwnd, LPPAINTSTRUCT lps ) 
{
    HRGN hrgnUpdate;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;

    hrgnUpdate = wndPtr->hrgnUpdate;  /* Save update region */
    if (!hrgnUpdate)    /* Create an empty region */
	if (!(hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 ))) return 0;

    if (wndPtr->hrgnUpdate || (wndPtr->flags & WIN_INTERNAL_PAINT))
	QUEUE_DecPaintCount( wndPtr->hmemTaskQ );

    wndPtr->hrgnUpdate = 0;
    wndPtr->flags &= ~(WIN_NEEDS_BEGINPAINT | WIN_INTERNAL_PAINT);

    HideCaret( hwnd );

    if (wndPtr->flags & WIN_NEEDS_NCPAINT)
    {
        wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
        SendMessage( hwnd, WM_NCPAINT, 0, 0 );
    }

    lps->hdc = GetDCEx( hwnd, hrgnUpdate, DCX_INTERSECTRGN | DCX_USESTYLE );
    DeleteObject( hrgnUpdate );
    if (!lps->hdc)
    {
        fprintf(stderr, "GetDCEx() failed in BeginPaint(), hwnd=%04x\n", hwnd);
        return 0;
    }

    GetRgnBox( InquireVisRgn(lps->hdc), &lps->rcPaint );
    DPtoLP( lps->hdc, (LPPOINT)&lps->rcPaint, 2 );

    if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
    {
        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
        lps->fErase = !SendMessage( hwnd, WM_ERASEBKGND, (WPARAM)lps->hdc, 0 );
    }
    else lps->fErase = TRUE;

    return lps->hdc;
}


/***********************************************************************
 *           EndPaint    (USER.40)
 */
BOOL EndPaint( HWND hwnd, const PAINTSTRUCT* lps )
{
    ReleaseDC( hwnd, lps->hdc );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *           FillWindow    (USER.324)
 */
void FillWindow( HWND hwndParent, HWND hwnd, HDC hdc, HBRUSH hbrush )
{
    RECT rect;
    GetClientRect( hwnd, &rect );
    DPtoLP( hdc, (LPPOINT)&rect, 2 );
    PaintRect( hwndParent, hwnd, hdc, hbrush, &rect );
}


/***********************************************************************
 *           PaintRect    (USER.325)
 */
void PaintRect(HWND hwndParent, HWND hwnd, HDC hdc, HBRUSH hbrush, LPRECT rect)
{
      /* Send WM_CTLCOLOR message if needed */

    if ((DWORD)hbrush <= CTLCOLOR_MAX)
    {
	if (!hwndParent) return;
#ifdef WINELIB32
	hbrush = (HBRUSH)SendMessage( hwndParent, 
				      WM_CTLCOLORMSGBOX+(DWORD)hbrush,
				      (WPARAM)hdc, (LPARAM)hwnd );
#else
	hbrush = (HBRUSH)SendMessage( hwndParent, WM_CTLCOLOR,
				      hdc, MAKELONG( hwnd, hbrush ) );
#endif
    }
    if (hbrush) FillRect( hdc, rect, hbrush );
}


/***********************************************************************
 *           GetControlBrush    (USER.326)
 */
HBRUSH GetControlBrush( HWND hwnd, HDC hdc, WORD control )
{
#ifdef WINELIB32
    return (HBRUSH)SendMessage( GetParent(hwnd), WM_CTLCOLOR+control,
                                (WPARAM)hdc, (LPARAM)hwnd );
#else
    return (HBRUSH)SendMessage( GetParent(hwnd), WM_CTLCOLOR,
                                hdc, MAKELONG( hwnd, control ) );
#endif
}


/***********************************************************************
 *           RedrawWindow    (USER.290)
 */
BOOL RedrawWindow( HWND hwnd, LPRECT rectUpdate, HRGN hrgnUpdate, UINT flags )
{
    HRGN hrgn;
    RECT rectClient;
    WND * wndPtr;

    if (!hwnd) hwnd = GetDesktopWindow();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!IsWindowVisible(hwnd) || (wndPtr->flags & WIN_NO_REDRAW))
        return TRUE;  /* No redraw needed */

    if (rectUpdate)
    {
        dprintf_win(stddeb, "RedrawWindow: %04x %d,%d-%d,%d %04x flags=%04x\n",
                    hwnd, rectUpdate->left, rectUpdate->top,
                    rectUpdate->right, rectUpdate->bottom, hrgnUpdate, flags );
    }
    else
    {
        dprintf_win( stddeb, "RedrawWindow: %04x NULL %04x flags=%04x\n",
                     hwnd, hrgnUpdate, flags);
    }
    GetClientRect( hwnd, &rectClient );

    if (flags & RDW_INVALIDATE)  /* Invalidate */
    {
        if (wndPtr->hrgnUpdate)  /* Is there already an update region? */
        {
            if ((hrgn = hrgnUpdate) == 0)
                hrgn = CreateRectRgnIndirect( rectUpdate ? rectUpdate :
                                              &rectClient );
            CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate, hrgn, RGN_OR );
            if (!hrgnUpdate) DeleteObject( hrgn );
        }
        else  /* No update region yet */
        {
            if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
            if (hrgnUpdate)
            {
                wndPtr->hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
                CombineRgn( wndPtr->hrgnUpdate, hrgnUpdate, 0, RGN_COPY );
            }
            else wndPtr->hrgnUpdate = CreateRectRgnIndirect( rectUpdate ?
                                                    rectUpdate : &rectClient );
        }
        if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;
        if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags |= RDW_FRAME;  /* Force invalidating the frame of children */
    }
    else if (flags & RDW_VALIDATE)  /* Validate */
    {
          /* We need an update region in order to validate anything */
        if (wndPtr->hrgnUpdate)
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
	if (!wndPtr->hrgnUpdate && !(wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags |= WIN_INTERNAL_PAINT;	    
    }
    else if (flags & RDW_NOINTERNALPAINT)
    {
	if (!wndPtr->hrgnUpdate && (wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags &= ~WIN_INTERNAL_PAINT;
    }

      /* Erase/update window */

    if (flags & RDW_UPDATENOW) SendMessage( hwnd, WM_PAINT, 0, 0 );
    else if (flags & RDW_ERASENOW)
    {
        if (wndPtr->flags & WIN_NEEDS_NCPAINT)
        {
            wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
            SendMessage( hwnd, WM_NCPAINT, 0, 0 );
        }
        if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
        {
            HDC hdc = GetDCEx( hwnd, wndPtr->hrgnUpdate,
                               DCX_INTERSECTRGN | DCX_USESTYLE );
            if (hdc)
            {
              /* Don't send WM_ERASEBKGND to icons */
              /* (WM_ICONERASEBKGND is sent during processing of WM_NCPAINT) */
                if (!(wndPtr->dwStyle & WS_MINIMIZE)
                    || !WIN_CLASS_INFO(wndPtr).hIcon)
                {
                    if (SendMessage( hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0 ))
                        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
                }
                ReleaseDC( hwnd, hdc );
            }
        }
    }

      /* Recursively process children */

    if (!(flags & RDW_NOCHILDREN) &&
	((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)))
    {
	if (hrgnUpdate)
	{
	    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (!hrgn) return TRUE;
	    for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
	    {
		CombineRgn( hrgn, hrgnUpdate, 0, RGN_COPY );
		OffsetRgn( hrgn, -wndPtr->rectClient.left,
			         -wndPtr->rectClient.top );
		RedrawWindow( wndPtr->hwndSelf, NULL, hrgn, flags );
	    }
	    DeleteObject( hrgn );
	}
	else
	{
	    RECT rect;		
	    for (wndPtr = wndPtr->child; wndPtr; wndPtr = wndPtr->next)
	    {
		if (rectUpdate)
		{
		    rect = *rectUpdate;
		    OffsetRect( &rect, -wndPtr->rectClient.left,
			               -wndPtr->rectClient.top );
		    RedrawWindow( wndPtr->hwndSelf, &rect, 0, flags );
		}
		else RedrawWindow( wndPtr->hwndSelf, NULL, 0, flags );
	    }
	}
    }
    return TRUE;
}


/***********************************************************************
 *           UpdateWindow   (USER.124)
 */
void UpdateWindow( HWND hwnd )
{
    RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN );
}


/***********************************************************************
 *           InvalidateRgn   (USER.126)
 */
void InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           InvalidateRect   (USER.125)
 */
void InvalidateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    RedrawWindow( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *           ValidateRgn   (USER.128)
 */
void ValidateRgn( HWND hwnd, HRGN hrgn )
{
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *           ValidateRect   (USER.127)
 */
void ValidateRect( HWND hwnd, LPRECT rect )
{
    RedrawWindow( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/***********************************************************************
 *           GetUpdateRect   (USER.190)
 */
BOOL GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate)
	{
	    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (GetUpdateRgn( hwnd, hrgn, erase ) == ERROR) return FALSE;
	    GetRgnBox( hrgn, rect );
	    DeleteObject( hrgn );
	}
	else SetRectEmpty( rect );
    }
    return (wndPtr->hrgnUpdate != 0);
}


/***********************************************************************
 *           GetUpdateRgn   (USER.237)
 */
int GetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    int retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (!wndPtr->hrgnUpdate)
    {
        SetRectRgn( hrgn, 0, 0, 0, 0 );
        return NULLREGION;
    }
    retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY );
    if (erase) RedrawWindow( hwnd, NULL, 0, RDW_ERASENOW | RDW_NOCHILDREN );
    return retval;
}


/***********************************************************************
 *           ExcludeUpdateRgn   (USER.238)
 */
int ExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    int retval = ERROR;
    HRGN hrgn;
    WND * wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return ERROR;
    if ((hrgn = CreateRectRgn( 0, 0, 0, 0 )) != 0)
    {
	retval = CombineRgn( hrgn, InquireVisRgn(hdc),
			     wndPtr->hrgnUpdate, RGN_DIFF );
	if (retval) SelectVisRgn( hdc, hrgn );
	DeleteObject( hrgn );
    }
    return retval;
}



/*
 * Window painting functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdio.h>
#include <X11/Xlib.h>

#include "win.h"
#include "message.h"
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
	MSG_DecPaintCount( wndPtr->hmemTaskQ );

    wndPtr->hrgnUpdate = 0;
    wndPtr->flags &= ~(WIN_NEEDS_BEGINPAINT | WIN_INTERNAL_PAINT);

    if (wndPtr->flags & WIN_NEEDS_NCPAINT)
    {
        SendMessage( hwnd, WM_NCPAINT, 0, 0 );
        wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
    }

    lps->hdc = GetDCEx( hwnd, hrgnUpdate, DCX_INTERSECTRGN | DCX_USESTYLE );
    DeleteObject( hrgnUpdate );
    if (!lps->hdc)
    {
        fprintf( stderr, "GetDCEx() failed in BeginPaint(), hwnd=%x\n", hwnd );
        return 0;
    }

    GetRgnBox( InquireVisRgn(lps->hdc), &lps->rcPaint );
    DPtoLP( lps->hdc, (LPPOINT)&lps->rcPaint, 2 );

    if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
    {
        lps->fErase = !SendMessage( hwnd, WM_ERASEBKGND, lps->hdc, 0 );
        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
    }
    else lps->fErase = TRUE;

    return lps->hdc;
}


/***********************************************************************
 *           EndPaint    (USER.40)
 */
void EndPaint( HWND hwnd, LPPAINTSTRUCT lps )
{
    ReleaseDC( hwnd, lps->hdc );
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

    if (hbrush <= CTLCOLOR_MAX)
    {
	if (!hwndParent) return;
	hbrush = (HBRUSH)SendMessage( hwndParent, WM_CTLCOLOR,
				      hdc, MAKELONG( hwnd, hbrush ) );
    }
    if (hbrush) FillRect( hdc, rect, hbrush );
}


/***********************************************************************
 *           RedrawWindow    (USER.290)
 */
BOOL RedrawWindow( HWND hwnd, LPRECT rectUpdate, HRGN hrgnUpdate, UINT flags )
{
    HRGN tmpRgn, hrgn = 0;
    RECT rectClient, rectWindow;
    WND * wndPtr;

    if (!hwnd) hwnd = GetDesktopWindow();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (!(wndPtr->dwStyle & WS_VISIBLE) || (wndPtr->flags & WIN_NO_REDRAW))
        return TRUE;  /* No redraw needed */

    if (rectUpdate)
    {
        dprintf_win( stddeb, "RedrawWindow: %x %d,%d-%d,%d %x flags=%04x\n",
                     hwnd, rectUpdate->left, rectUpdate->top,
                     rectUpdate->right, rectUpdate->bottom, hrgnUpdate, flags);
    }
    else
    {
        dprintf_win( stddeb, "RedrawWindow: %x NULL %x flags=%04x\n",
                     hwnd, hrgnUpdate, flags);
    }
    GetClientRect( hwnd, &rectClient );
    rectWindow = wndPtr->rectWindow;
    OffsetRect(&rectWindow, -wndPtr->rectClient.left, -wndPtr->rectClient.top);

    if (flags & RDW_INVALIDATE)  /* Invalidate */
    {
	if (hrgnUpdate)  /* Invalidate a region */
	{
	    if (flags & RDW_FRAME) tmpRgn = CreateRectRgnIndirect(&rectWindow);
	    else tmpRgn = CreateRectRgnIndirect( &rectClient );
	    if (!tmpRgn) return FALSE;
	    hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (CombineRgn( hrgn, hrgnUpdate, tmpRgn, RGN_AND ) == NULLREGION)
	    {
		DeleteObject( hrgn );
		hrgn = 0;
	    }
	    DeleteObject( tmpRgn );
	}
	else  /* Invalidate a rectangle */
	{
	    RECT rect;
	    if (flags & RDW_FRAME)
	    {
		if (rectUpdate) IntersectRect( &rect, rectUpdate, &rectWindow);
		else rect = rectWindow;
	    }
	    else
	    {
		if (rectUpdate) IntersectRect( &rect, rectUpdate, &rectClient);
		else rect = rectClient;
	    }
	    if (!IsRectEmpty(&rect)) hrgn = CreateRectRgnIndirect( &rect );
	}

	  /* Set update region */

	if (hrgn)
	{
	    if (!wndPtr->hrgnUpdate)
	    {
		wndPtr->hrgnUpdate = hrgn;
		if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
		    MSG_IncPaintCount( wndPtr->hmemTaskQ );
	    }
	    else
	    {
		tmpRgn = CreateRectRgn( 0, 0, 0, 0 );
		CombineRgn( tmpRgn, wndPtr->hrgnUpdate, hrgn, RGN_OR );
		DeleteObject( wndPtr->hrgnUpdate );
		DeleteObject( hrgn );
		wndPtr->hrgnUpdate = tmpRgn;
	    }
	}
        if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;
        if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags |= RDW_FRAME;  /* Force invalidating the frame of children */
    }
    else if (flags & RDW_VALIDATE)  /* Validate */
    {
	if (flags & RDW_NOERASE) wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
	if (!(hrgn = CreateRectRgn( 0, 0, 0, 0 ))) return FALSE;

	  /* Remove frame from update region */

	if (wndPtr->hrgnUpdate && (flags & RDW_NOFRAME))
	{
	    if (!(tmpRgn = CreateRectRgnIndirect( &rectClient )))
		return FALSE;
	    if (CombineRgn(hrgn,tmpRgn,wndPtr->hrgnUpdate,RGN_AND) == NULLREGION)
	    {
		DeleteObject( hrgn );
		hrgn = 0;
	    }
	    DeleteObject( tmpRgn );
	    DeleteObject( wndPtr->hrgnUpdate );
	    wndPtr->hrgnUpdate = hrgn;
	    hrgn = CreateRectRgn( 0, 0, 0, 0 );
	}

	  /* Set update region */

	if (wndPtr->hrgnUpdate)
	{
	    int res;
	    if (hrgnUpdate)  /* Validate a region */
	    {
		res = CombineRgn(hrgn,wndPtr->hrgnUpdate,hrgnUpdate,RGN_DIFF);
	    }
	    else  /* Validate a rectangle */
	    {
		if (rectUpdate) tmpRgn = CreateRectRgnIndirect( rectUpdate );
		else tmpRgn = CreateRectRgnIndirect( &rectWindow );
		res = CombineRgn( hrgn, wndPtr->hrgnUpdate, tmpRgn, RGN_DIFF );
		DeleteObject( tmpRgn );
	    }
	    DeleteObject( wndPtr->hrgnUpdate );
	    if (res == NULLREGION)
	    {
		DeleteObject( hrgn );
		wndPtr->hrgnUpdate = 0;
		if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
		    MSG_DecPaintCount( wndPtr->hmemTaskQ );
	    }
	    else wndPtr->hrgnUpdate = hrgn;
	}
    }

      /* Set/clear internal paint flag */

    if (flags & RDW_INTERNALPAINT)
    {
	if (!wndPtr->hrgnUpdate && !(wndPtr->flags & WIN_INTERNAL_PAINT))
	    MSG_IncPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags |= WIN_INTERNAL_PAINT;	    
    }
    else if (flags & RDW_NOINTERNALPAINT)
    {
	if (!wndPtr->hrgnUpdate && (wndPtr->flags & WIN_INTERNAL_PAINT))
	    MSG_DecPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags &= ~WIN_INTERNAL_PAINT;
    }

      /* Erase/update window */

    if (flags & RDW_UPDATENOW) SendMessage( hwnd, WM_PAINT, 0, 0 );
    else if (flags & RDW_ERASENOW)
    {
        if (wndPtr->flags & WIN_NEEDS_NCPAINT)
        {
            SendMessage( hwnd, WM_NCPAINT, 0, 0 );
            wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
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
                    if (SendMessage( hwnd, WM_ERASEBKGND, hdc, 0 ))
                        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
                }
                ReleaseDC( hwnd, hdc );
            }
        }
    }

      /* Recursively process children */

    if (!(flags & RDW_NOCHILDREN) &&
	((flags && RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)))
    {
	if (hrgnUpdate)
	{
	    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (!hrgn) return TRUE;
	    for (hwnd = wndPtr->hwndChild; (hwnd); hwnd = wndPtr->hwndNext)
	    {
		if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
		CombineRgn( hrgn, hrgnUpdate, 0, RGN_COPY );
		OffsetRgn( hrgn, -wndPtr->rectClient.left,
			         -wndPtr->rectClient.top );
		RedrawWindow( hwnd, NULL, hrgn, flags );
	    }
	    DeleteObject( hrgn );
	}
	else
	{
	    RECT rect;		
	    for (hwnd = wndPtr->hwndChild; (hwnd); hwnd = wndPtr->hwndNext)
	    {
		if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
		if (rectUpdate)
		{
		    rect = *rectUpdate;
		    OffsetRect( &rect, -wndPtr->rectClient.left,
			               -wndPtr->rectClient.top );
		    RedrawWindow( hwnd, &rect, 0, flags );
		}
		else RedrawWindow( hwnd, NULL, 0, flags );
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
    HRGN hrgnClip;
    int retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (!wndPtr->hrgnUpdate)
    {
        SetRectRgn( hrgn, 0, 0, 0, 0 );
        return NULLREGION;
    }
    hrgnClip = CreateRectRgn( 0, 0,
                             wndPtr->rectClient.right-wndPtr->rectClient.left,
                             wndPtr->rectClient.bottom-wndPtr->rectClient.top);
    if (!hrgnClip) return ERROR;
    retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, hrgnClip, RGN_AND );
    if (erase) RedrawWindow( hwnd, NULL, 0, RDW_ERASENOW | RDW_NOCHILDREN );
    DeleteObject( hrgnClip );
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



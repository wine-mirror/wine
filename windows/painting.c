/*
 * Window painting functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <math.h>
#include <X11/Xlib.h>

#include "win.h"
#include "message.h"

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

    if (!(lps->hdc = GetDCEx( hwnd, wndPtr->hrgnUpdate,
			      DCX_INTERSECTRGN | DCX_USESTYLE ))) return 0;
    GetRgnBox( InquireVisRgn(lps->hdc), &lps->rcPaint );

    if (wndPtr->hrgnUpdate)
    {
	wndPtr->hrgnUpdate = 0;
	MSG_DecPaintCount( wndPtr->hmemTaskQ );
    }
    wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;

    SendMessage( hwnd, WM_NCPAINT, hrgnUpdate, 0 );
    if (hrgnUpdate) DeleteObject( hrgnUpdate );

    if (!(wndPtr->flags & WIN_ERASE_UPDATERGN)) lps->fErase = TRUE;
    else lps->fErase = !SendMessage( hwnd, WM_ERASEBKGND, lps->hdc, 0 );
    
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
				      hdc, hwnd | (hbrush << 16) );
    }
    if (hbrush) FillRect( hdc, rect, hbrush );
}

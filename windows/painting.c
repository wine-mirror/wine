/*
 * Window painting functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <math.h>
#include <X11/Xlib.h>

#include "win.h"

  /* Last CTLCOLOR id */
#define CTLCOLOR_MAX   CTLCOLOR_STATIC


/***********************************************************************
 *           BeginPaint    (USER.39)
 */
HDC BeginPaint( HWND hwnd, LPPAINTSTRUCT lps ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;

    lps->hdc = GetDC( hwnd );
    if (!lps->hdc) return 0;
    
    SelectVisRgn( lps->hdc, wndPtr->hrgnUpdate );
    if (wndPtr->hrgnUpdate)
    {
	GetRgnBox( wndPtr->hrgnUpdate, &lps->rcPaint );
	DeleteObject( wndPtr->hrgnUpdate );
	wndPtr->hrgnUpdate = 0;
    }
    else
    {
	lps->rcPaint.left   = 0;
	lps->rcPaint.top    = 0;
	lps->rcPaint.right  = wndPtr->rectClient.right-wndPtr->rectClient.left;
	lps->rcPaint.bottom = wndPtr->rectClient.bottom-wndPtr->rectClient.top;
    }    

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
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
    rect.bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
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

/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include "win.h"
#include "sysmetrics.h"


/***********************************************************************
 *           NC_AdjustRect
 *
 * Compute the size of the window rectangle from the size of the
 * client rectangle.
 */
static void NC_AdjustRect( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    if ((style & WS_DLGFRAME) && 
	((exStyle & WS_EX_DLGMODALFRAME) || !(style & WS_BORDER)))
	InflateRect( rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
    else if (style & WS_THICKFRAME)
	InflateRect( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );

    if (style & WS_BORDER)
	InflateRect( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER );

    if ((style & WS_CAPTION) == WS_CAPTION)
	rect->top -= SYSMETRICS_CYCAPTION - 1;

    if (menu) rect->top -= SYSMETRICS_CYMENU + 1;

    if (style & WS_VSCROLL) rect->right  += SYSMETRICS_CXVSCROLL;
    if (style & WS_HSCROLL) rect->bottom += SYSMETRICS_CYHSCROLL;
}


/***********************************************************************
 *           AdjustWindowRect    (USER.102)
 */
void AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *           AdjustWindowRectEx    (USER.454)
 */
void AdjustWindowRectEx( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
      /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	style |= WS_CAPTION;
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

#ifdef DEBUG_NONCLIENT
    printf( "AdjustWindowRectEx: (%d,%d)-(%d,%d) %08x %d %08x\n",
      rect->left, rect->top, rect->right, rect->bottom, style, menu, exStyle );
#endif

    NC_AdjustRect( rect, style, menu, exStyle );
}


/***********************************************************************
 *           NC_HandleNCCalcSize
 *
 * Handle a WM_NCCALCSIZE message. Called from DefWindowProc().
 */
LONG NC_HandleNCCalcSize( HWND hwnd, NCCALCSIZE_PARAMS *params )
{
    RECT tmpRect = { 0, 0, 0, 0 };
    BOOL hasMenu;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    if (!wndPtr) return 0;

    hasMenu = (!(wndPtr->dwStyle & WS_CHILD)) && (wndPtr->wIDmenu != 0);

    NC_AdjustRect( &tmpRect, wndPtr->dwStyle, hasMenu, wndPtr->dwExStyle );
    
    params->rgrc[0].left   -= tmpRect.left;
    params->rgrc[0].top    -= tmpRect.top;
    params->rgrc[0].right  -= tmpRect.right;
    params->rgrc[0].bottom -= tmpRect.bottom;
    return 0;
}


/***********************************************************************
 *           NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG NC_HandleNCHitTest( HWND hwnd, POINT pt )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return HTERROR;

#ifdef DEBUG_NONCLIENT
    printf( "NC_HandleNCHitTest: hwnd=%x pt=%d,%d\n", hwnd, pt.x, pt.y );
#endif

    if (hwnd == GetCapture()) return HTCLIENT;
    GetWindowRect( hwnd, &rect );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;
    ScreenToClient( hwnd, &pt );
    GetClientRect( hwnd, &rect );
    
    if (PtInRect( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */
    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += SYSMETRICS_CXVSCROLL;
	if (PtInRect( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */
    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += SYSMETRICS_CYHSCROLL;
	if (PtInRect( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - SYSMETRICS_CXVSCROLL)) return HTSIZE;
	    return HTHSCROLL;
	}
    }

      /* Check menu */
    if ((!(wndPtr->dwStyle & WS_CHILD)) && (wndPtr->wIDmenu != 0))
    {
	rect.top -= SYSMETRICS_CYMENU + 1;
	if (PtInRect( &rect, pt )) return HTMENU;
    }

      /* Check caption */
    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
	rect.top -= SYSMETRICS_CYCAPTION - 1;
	if (PtInRect( &rect, pt ))
	{
	      /* Check system menu */
	    if ((wndPtr->dwStyle & WS_SYSMENU) && (pt.x <= SYSMETRICS_CXSIZE))
		return HTSYSMENU;
	      /* Check maximize box */
	    if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
		rect.right -= SYSMETRICS_CXSIZE + 1;
	    if (pt.x >= rect.right) return HTMAXBUTTON;
	      /* Check minimize box */
	    if (wndPtr->dwStyle & WS_MINIMIZEBOX)
		rect.right -= SYSMETRICS_CXSIZE + 1;
	    if (pt.x >= rect.right) return HTMINBUTTON;
	    return HTCAPTION;
	}
    }
    
      /* Check non-sizing border */
    if (!(wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME))
	return HTBORDER;

      /* Check top sizing border */
    if (pt.y < rect.top)
    {
	if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTTOPLEFT;
	if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTTOPRIGHT;
	return HTTOP;
    }

      /* Check bottom sizing border */
    if (pt.y >= rect.bottom)
    {
	if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTBOTTOMLEFT;
	if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTBOTTOMRIGHT;
	return HTBOTTOM;
    }
    
      /* Check left sizing border */
    if (pt.x < rect.left)
    {
	if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPLEFT;
	if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMLEFT;
	return HTLEFT;
    }

      /* Check right sizing border */
    if (pt.x >= rect.right)
    {
	if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPRIGHT;
	if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMRIGHT;
	return HTRIGHT;
    }

      /* Should never get here */
    return HTERROR;
}


/***********************************************************************
 *           NC_DrawFrame
 *
 * Draw a window frame inside the given rectangle, and update the rectangle.
 * The correct pen and brush must be selected in the DC.
 */
static void NC_DrawFrame( HDC hdc, RECT *rect, BOOL dlgFrame )
{
    short width, height, tmp;

    if (dlgFrame)
    {
	width = SYSMETRICS_CXDLGFRAME;
	height = SYSMETRICS_CYDLGFRAME;
    }
    else
    {
	width = SYSMETRICS_CXFRAME - 1;
	height = SYSMETRICS_CYFRAME - 1;
    }

      /* Draw frame */
    PatBlt( hdc, rect->left, rect->top,
	    rect->right - rect->left, height, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
	    width, rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom,
	    rect->right - rect->left, -height, PATCOPY );
    PatBlt( hdc, rect->right, rect->top,
	    -width, rect->bottom - rect->top, PATCOPY );

    if (dlgFrame)
    {
	InflateRect( rect, -width, -height );
	return;
    }
    
      /* Draw inner rectangle */
    MoveTo( hdc, rect->left+width, rect->top+height );
    LineTo( hdc, rect->right-width-1, rect->top+height );
    LineTo( hdc, rect->right-width-1, rect->bottom-height-1 );
    LineTo( hdc, rect->left+width, rect->bottom-height-1 );
    LineTo( hdc, rect->left+width, rect->top+height );

      /* Draw the decorations */
    tmp = rect->top + SYSMETRICS_CYFRAME + SYSMETRICS_CYSIZE;
    MoveTo( hdc, rect->left, tmp);
    LineTo( hdc, rect->left+width, tmp );
    MoveTo( hdc, rect->right-width-1, tmp );
    LineTo( hdc, rect->right-1, tmp );

    tmp = rect->bottom - 1 - SYSMETRICS_CYFRAME - SYSMETRICS_CYSIZE;
    MoveTo( hdc, rect->left, tmp );
    LineTo( hdc, rect->left+width, tmp );
    MoveTo( hdc, rect->right-width-1, tmp );
    LineTo( hdc, rect->right-1, tmp );

    tmp = rect->left + SYSMETRICS_CXFRAME + SYSMETRICS_CXSIZE;
    MoveTo( hdc, tmp, rect->top );
    LineTo( hdc, tmp, rect->top+height );
    MoveTo( hdc, tmp, rect->bottom-height-1 );
    LineTo( hdc, tmp, rect->bottom-1 );

    tmp = rect->right - 1 - SYSMETRICS_CXFRAME - SYSMETRICS_CYSIZE;
    MoveTo( hdc, tmp, rect->top );
    LineTo( hdc, tmp, rect->top+height );
    MoveTo( hdc, tmp, rect->bottom-height-1 );
    LineTo( hdc, tmp, rect->bottom-1 );

    InflateRect( rect, -width-1, -height-1 );
}


/***********************************************************************
 *           NC_DrawCaption
 *
 * Draw the window caption.
 * The correct pen for the window frame must be selected in the DC.
 */
static void NC_DrawCaption( HDC hdc, RECT *rect, HWND hwnd, DWORD style )
{
    RECT r;
    HBRUSH hbrushCaption, hbrushButtons;
    char buffer[256];

    hbrushButtons = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
    hbrushCaption = CreateSolidBrush( GetSysColor( COLOR_ACTIVECAPTION ) );

    MoveTo( hdc, rect->left, rect->bottom );
    LineTo( hdc, rect->right-1, rect->bottom );

    /* We should probably use OEM bitmaps (OBM_*) here */
    if (style & WS_SYSMENU)
    {
	r = *rect;
	r.right = r.left + SYSMETRICS_CYSIZE;
	FillRect( hdc, &r, hbrushButtons );
	MoveTo( hdc, r.right, r.top );
	LineTo( hdc, r.right, r.bottom );
	rect->left += SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MAXIMIZEBOX)
    {
	r = *rect;
	r.left = r.right - SYSMETRICS_CXSIZE;
	FillRect( hdc, &r, hbrushButtons );
	MoveTo( hdc, r.left-1, r.top );
	LineTo( hdc, r.left-1, r.bottom );
	DrawReliefRect( hdc, r, 2, 0 );
	rect->right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX)
    {
	r = *rect;
	r.left = r.right - SYSMETRICS_CXSIZE;
	FillRect( hdc, &r, hbrushButtons );
	MoveTo( hdc, r.left-1, r.top );
	LineTo( hdc, r.left-1, r.bottom );
	DrawReliefRect( hdc, r, 2, 0 );
	rect->right -= SYSMETRICS_CXSIZE + 1;
    }

    FillRect( hdc, rect, hbrushCaption );

    if (GetWindowText( hwnd, buffer, 256 ))
    {
	SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	DrawText( hdc, buffer, -1, rect,
		 DT_SINGLELINE | DT_CENTER | DT_VCENTER );
    }

    DeleteObject( hbrushButtons );
    DeleteObject( hbrushCaption );
}


/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND hwnd, HRGN hrgn )
{
    HDC hdc;
    RECT rect;
    HBRUSH hbrushBorder = 0;
    HPEN hpenFrame = 0;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

#ifdef DEBUG_NONCLIENT
    printf( "NC_HandleNCPaint: %d %d\n", hwnd, hrgn );
#endif

    if (!wndPtr || !hrgn) return 0;
    if (!(wndPtr->dwStyle & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)))
	return 0;  /* Nothing to do! */

    if (hrgn == 1) hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    else hdc = GetDCEx( hwnd, hrgn, DCX_CACHE | DCX_WINDOW | DCX_INTERSECTRGN);
    if (!hdc) return 0;
    if (ExcludeVisRect( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC( hwnd, hdc );
	return 0;
    }

    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    hpenFrame = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME) );
    SelectObject( hdc, hpenFrame );
    hbrushBorder = CreateSolidBrush( GetSysColor(COLOR_ACTIVEBORDER) );
    SelectObject( hdc, hbrushBorder );

    if ((wndPtr->dwStyle & WS_BORDER) || (wndPtr->dwStyle & WS_DLGFRAME))
    {
	MoveTo( hdc, 0, 0 );
	LineTo( hdc, rect.right-1, 0 );
	LineTo( hdc, rect.right-1, rect.bottom-1 );
	LineTo( hdc, 0, rect.bottom-1 );
	LineTo( hdc, 0, 0 );
	InflateRect( &rect, -1, -1 );
    }

    if ((wndPtr->dwStyle & WS_DLGFRAME) &&
	((wndPtr->dwExStyle & WS_EX_DLGMODALFRAME) || 
	 !(wndPtr->dwStyle & WS_BORDER))) NC_DrawFrame( hdc, &rect, TRUE );
    else if (wndPtr->dwStyle & WS_THICKFRAME) NC_DrawFrame(hdc, &rect, FALSE);

    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
	RECT r = rect;
	rect.top += SYSMETRICS_CYSIZE + 1;
	r.bottom = rect.top - 1;
	if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
	{
	    HBRUSH hbrushWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	    HBRUSH hbrushOld = SelectObject( hdc, hbrushWindow );
	    PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1, PATCOPY );
	    PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	    PatBlt( hdc, r.left, r.top, r.right-r.left, 1, PATCOPY );
	    r.left++;
	    r.right--;
	    r.top++;
	    SelectObject( hdc, hbrushOld );
	    DeleteObject( hbrushWindow );
	}
	NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle );
    }

    if (wndPtr->dwStyle & (WS_VSCROLL | WS_HSCROLL))
    {
	HBRUSH hbrushScroll = CreateSolidBrush( GetSysColor(COLOR_SCROLLBAR) );
	HBRUSH hbrushOld = SelectObject( hdc, hbrushScroll );
	if (wndPtr->dwStyle & WS_VSCROLL)
	    PatBlt( hdc, rect.right - SYSMETRICS_CXVSCROLL, rect.top,
		    SYSMETRICS_CXVSCROLL, rect.bottom-rect.top, PATCOPY );
	if (wndPtr->dwStyle & WS_HSCROLL)
	    PatBlt( hdc, rect.left, rect.bottom - SYSMETRICS_CYHSCROLL,
		    rect.right-rect.left, SYSMETRICS_CYHSCROLL, PATCOPY );
	SelectObject( hdc, hbrushOld );
	DeleteObject( hbrushScroll );
    }    

    ReleaseDC( hwnd, hdc );
    if (hbrushBorder) DeleteObject( hbrushBorder );
    if (hpenFrame) DeleteObject( hpenFrame );    
    return 0;
}


/***********************************************************************
 *           NC_HandleNCMouseMsg
 *
 * Handle a non-client mouse message. Called from DefWindowProc().
 */
LONG NC_HandleNCMouseMsg( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    switch(wParam)  /* Hit test code */
    {
    case HTSYSMENU:
	if (msg == WM_NCLBUTTONDBLCLK)
	    return SendMessage( hwnd, WM_SYSCOMMAND, SC_CLOSE, lParam );
	break;
    }
    return 0;
}


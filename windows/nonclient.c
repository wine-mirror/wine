/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include "win.h"
#include "sysmetrics.h"


static HBITMAP hbitmapClose = 0;
static HBITMAP hbitmapMinimize = 0;
static HBITMAP hbitmapMinimizeD = 0;
static HBITMAP hbitmapMaximize = 0;
static HBITMAP hbitmapMaximizeD = 0;
static HBITMAP hbitmapRestore = 0;
static HBITMAP hbitmapRestoreD = 0;

  /* Hit test code returned when the mouse is captured. */
  /* Used to direct NC mouse messages to the correct part of the window. */
static WORD captureHitTest = HTCLIENT; 

  /* Point where the current capture started. Used for SC_SIZE and SC_MOVE. */
static POINT capturePoint;

  /* Current window rectangle when a move or resize is in progress. */
static RECT sizingRect;

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((style) & WS_DLGFRAME) && \
     (((exStyle) & WS_EX_DLGMODALFRAME) || !((style) & WS_BORDER)))

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_MENU(w)  (!((w)->dwStyle & WS_CHILD) && ((w)->wIDmenu != 0))

/***********************************************************************
 *           NC_AdjustRect
 *
 * Compute the size of the window rectangle from the size of the
 * client rectangle.
 */
static void NC_AdjustRect( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    if (HAS_DLGFRAME( style, exStyle ))
	InflateRect( rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
    else
    {
	if (HAS_THICKFRAME(style))
	    InflateRect( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );
	if (style & WS_BORDER)
	    InflateRect( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER );
    }

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
    WND *wndPtr = WIN_FindWndPtr( hwnd );    

    if (!wndPtr) return 0;

    NC_AdjustRect( &tmpRect, wndPtr->dwStyle,
		   HAS_MENU(wndPtr), wndPtr->dwExStyle );
    
    params->rgrc[0].left   -= tmpRect.left;
    params->rgrc[0].top    -= tmpRect.top;
    params->rgrc[0].right  -= tmpRect.right;
    params->rgrc[0].bottom -= tmpRect.bottom;
    return 0;
}


/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */
static void NC_GetInsideRect( HWND hwnd, RECT *rect )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

      /* Remove frame from rectangle */
    if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
	InflateRect( rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
    else
    {
	if (HAS_THICKFRAME( wndPtr->dwStyle ))
	    InflateRect( rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
	if (wndPtr->dwStyle & WS_BORDER)
	    InflateRect( rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER );
    }
}


/***********************************************************************
 *           NC_InternalNCHitTest
 *
 * Perform the hit test calculation, but whithout testing the capture
 * window.
 */
static LONG NC_InternalNCHitTest( HWND hwnd, POINT pt )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return HTERROR;

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
    if (HAS_MENU(wndPtr))
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
    if (!HAS_THICKFRAME( wndPtr->dwStyle )) return HTBORDER;

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
 *           NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG NC_HandleNCHitTest( HWND hwnd, POINT pt )
{
#ifdef DEBUG_NONCLIENT
    printf( "NC_HandleNCHitTest: hwnd=%x pt=%d,%d\n", hwnd, pt.x, pt.y );
#endif
    if (hwnd == GetCapture()) return captureHitTest;
    return NC_InternalNCHitTest( hwnd, pt );
}


/***********************************************************************
 *           NC_DrawSysButton
 */
static void NC_DrawSysButton( HWND hwnd, HDC hdc )
{
    RECT rect;
    BOOL down;
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
	down = ((GetCapture() == hwnd) && (captureHitTest == HTSYSMENU));
	SelectObject( hdcMem, hbitmapClose );
	BitBlt( hdc, rect.left-1, rect.top-1, SYSMETRICS_CXSIZE+1,
	       SYSMETRICS_CYSIZE+1, hdcMem, 0, 0, down ? NOTSRCCOPY : SRCCOPY);
	DeleteDC( hdcMem );
    }
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND hwnd, HDC hdc )
{
    RECT rect;
    BOOL down;
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
	down = ((GetCapture() == hwnd) && (captureHitTest == HTMAXBUTTON));
	if (IsZoomed(hwnd))
	    SelectObject( hdcMem, down ? hbitmapRestoreD : hbitmapRestore );
	else SelectObject( hdcMem, down ? hbitmapMaximizeD : hbitmapMaximize );
	BitBlt( hdc, rect.right - SYSMETRICS_CXSIZE - 1, rect.top - 1,
	      SYSMETRICS_CXSIZE+2, SYSMETRICS_CYSIZE+2, hdcMem, 0, 0, SRCCOPY);
	DeleteDC( hdcMem );
    }
}


/***********************************************************************
 *           NC_DrawMinButton
 */
static void NC_DrawMinButton( HWND hwnd, HDC hdc )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;	
	if ((GetCapture() == hwnd) && (captureHitTest == HTMINBUTTON))
	    SelectObject( hdcMem, hbitmapMinimizeD );
	else SelectObject( hdcMem, hbitmapMinimize );
	BitBlt( hdc, rect.right - SYSMETRICS_CXSIZE - 1, rect.top - 1,
	      SYSMETRICS_CXSIZE+2, SYSMETRICS_CYSIZE+2, hdcMem, 0, 0, SRCCOPY);
	DeleteDC( hdcMem );
    }
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
 *           NC_DrawMovingFrame
 *
 * Draw the frame used when moving or resizing window.
 */
static void NC_DrawMovingFrame( HWND hwnd, RECT *rect )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdc = GetDC( 0 );

    if (HAS_THICKFRAME( wndPtr->dwStyle ))
    {
	SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
	PatBlt( hdc, rect->left, rect->top,
	        rect->right - rect->left - SYSMETRICS_CXFRAME,
	        SYSMETRICS_CYFRAME, PATINVERT );
	PatBlt( hdc, rect->left, rect->top + SYSMETRICS_CYFRAME,
	        SYSMETRICS_CXFRAME, 
	        rect->bottom - rect->top - SYSMETRICS_CYFRAME, PATINVERT );
	PatBlt( hdc, rect->left + SYSMETRICS_CXFRAME, rect->bottom,
	        rect->right - rect->left - SYSMETRICS_CXFRAME,
	        -SYSMETRICS_CYFRAME, PATINVERT );
	PatBlt( hdc, rect->right, rect->top, -SYSMETRICS_CXFRAME, 
	        rect->bottom - rect->top - SYSMETRICS_CYFRAME, PATINVERT );
    }
    else DrawFocusRect( hdc, rect );
    ReleaseDC( 0, hdc );
}


/***********************************************************************
 *           NC_DrawCaption
 *
 * Draw the window caption.
 * The correct pen for the window frame must be selected in the DC.
 */
static void NC_DrawCaption( HDC hdc, RECT *rect, HWND hwnd, DWORD style )
{
    RECT r = *rect;
    HBRUSH hbrushCaption;
    char buffer[256];

    if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmap( 0, MAKEINTRESOURCE(OBM_CLOSE) )))
	    return;
	hbitmapMinimize  = LoadBitmap( 0, MAKEINTRESOURCE(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmap( 0, MAKEINTRESOURCE(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmap( 0, MAKEINTRESOURCE(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmap( 0, MAKEINTRESOURCE(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmap( 0, MAKEINTRESOURCE(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmap( 0, MAKEINTRESOURCE(OBM_RESTORED) );
    }
    
    hbrushCaption = CreateSolidBrush( GetSysColor( COLOR_ACTIVECAPTION ) );

    MoveTo( hdc, r.left, r.bottom );
    LineTo( hdc, r.right-1, r.bottom );

    if (style & WS_SYSMENU)
    {
	NC_DrawSysButton( hwnd, hdc );
	r.left += SYSMETRICS_CXSIZE + 1;
	MoveTo( hdc, r.left - 1, r.top );
	LineTo( hdc, r.left - 1, r.bottom );
    }
    if (style & WS_MAXIMIZEBOX)
    {
	NC_DrawMaxButton( hwnd, hdc );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX)
    {
	NC_DrawMinButton( hwnd, hdc );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }

    FillRect( hdc, &r, hbrushCaption );

    if (GetWindowText( hwnd, buffer, 256 ))
    {
	SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	DrawText( hdc, buffer, -1, &r,
		 DT_SINGLELINE | DT_CENTER | DT_VCENTER );
    }

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
 *           NC_HandleNCLButtonDown
 *
 * Handle a WM_NCLBUTTONDOWN message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDown( HWND hwnd, WORD wParam, LONG lParam )
{
    HDC hdc = GetWindowDC( hwnd );

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	if (GetCapture() != hwnd)
	    SendMessage( hwnd, WM_SYSCOMMAND, SC_MOVE, lParam );
	break;

    case HTSYSMENU:
	captureHitTest = wParam;
	SetCapture( hwnd );
	NC_DrawSysButton( hwnd, hdc );
	break;

    case HTMENU:
	break;

    case HTHSCROLL:
	if (GetCapture() != hwnd)
	    SendMessage( hwnd, WM_SYSCOMMAND, SC_HSCROLL, lParam );
	break;

    case HTVSCROLL:
	if (GetCapture() != hwnd)
	    SendMessage( hwnd, WM_SYSCOMMAND, SC_VSCROLL, lParam );
	break;

    case HTMINBUTTON:
	captureHitTest = wParam;
	SetCapture( hwnd );
	NC_DrawMinButton( hwnd, hdc );
	break;

    case HTMAXBUTTON:
	captureHitTest = wParam;
	SetCapture( hwnd );
	NC_DrawMaxButton( hwnd, hdc );
	break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
	if (GetCapture() != hwnd)
	    SendMessage( hwnd, WM_SYSCOMMAND,
			 SC_SIZE + wParam - HTLEFT + 1, lParam );
	break;

    case HTBORDER:
	break;
    }

    ReleaseDC( hwnd, hdc );
    return 0;
}


/***********************************************************************
 *           NC_HandleNCLButtonUp
 *
 * Handle a WM_NCLBUTTONUP message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonUp( HWND hwnd, WORD wParam, LONG lParam )
{
    HDC hdc;
    WORD hittest;

    if (hwnd != GetCapture()) return 0;

    ReleaseCapture();
    captureHitTest = HTCLIENT;
    hdc = GetWindowDC( hwnd );
    hittest = NC_InternalNCHitTest( hwnd, MAKEPOINT(lParam) );

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:  /* End of window moving */
	NC_DrawMovingFrame( hwnd, &sizingRect );
	SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top, 0, 0,
		      SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
	break;

    case HTSYSMENU:
	NC_DrawSysButton( hwnd, hdc );
	break;

    case HTMENU:
    case HTHSCROLL:
    case HTVSCROLL:
	break;

    case HTMINBUTTON:
	NC_DrawMinButton( hwnd, hdc );
	if (hittest == HTMINBUTTON)
	    SendMessage( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, lParam );
	break;

    case HTMAXBUTTON:
	NC_DrawMaxButton( hwnd, hdc );
	if (hittest == HTMAXBUTTON)
	{
	    if (IsZoomed(hwnd))
		SendMessage( hwnd, WM_SYSCOMMAND, SC_RESTORE, lParam );
	    else SendMessage( hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, lParam );
	}
	break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:  /* End of window resizing */
	NC_DrawMovingFrame( hwnd, &sizingRect );
	SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
		      sizingRect.right - sizingRect.left,
		      sizingRect.bottom - sizingRect.top,
		      SWP_NOACTIVATE | SWP_NOZORDER );
	break;

    case HTBORDER:
	    break;
    }
    ReleaseDC( hwnd, hdc );
    return 0;
}


/***********************************************************************
 *           NC_HandleNCLButtonDblClk
 *
 * Handle a WM_NCLBUTTONDBLCLK message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDblClk( HWND hwnd, WORD wParam, LONG lParam )
{
    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, lParam );
	break;

    case HTSYSMENU:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_CLOSE, lParam );
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCMouseMove
 *
 * Handle a WM_NCMOUSEMOVE message. Called from DefWindowProc().
 */
LONG NC_HandleNCMouseMove( HWND hwnd, WORD wParam, POINT pt )
{
    RECT newRect;

    if (hwnd != GetCapture()) return 0;
    newRect = sizingRect;

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	OffsetRect( &newRect, pt.x - capturePoint.x, pt.y - capturePoint.y);
	break;

    case HTLEFT:
	newRect.left += pt.x - capturePoint.x;
	break;

    case HTRIGHT:
	newRect.right += pt.x - capturePoint.x;
	break;

    case HTTOP:
	newRect.top += pt.y - capturePoint.y;
	break;

    case HTTOPLEFT:
	newRect.left += pt.x - capturePoint.x;
	newRect.top  += pt.y - capturePoint.y;
	break;

    case HTTOPRIGHT:
	newRect.right += pt.x - capturePoint.x;
	newRect.top   += pt.y - capturePoint.y;
	break;

    case HTBOTTOM:
	newRect.bottom += pt.y - capturePoint.y;
	break;

    case HTBOTTOMLEFT:
	newRect.left   += pt.x - capturePoint.x;
	newRect.bottom += pt.y - capturePoint.y;
	break;

    case HTBOTTOMRIGHT:
	newRect.right  += pt.x - capturePoint.x;
	newRect.bottom += pt.y - capturePoint.y;
	break;
 
    default:
	return 0;  /* Nothing to do */
   }

    NC_DrawMovingFrame( hwnd, &sizingRect );
    NC_DrawMovingFrame( hwnd, &newRect );
    capturePoint = pt;
    sizingRect = newRect;

    return 0;
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LONG NC_HandleSysCommand( HWND hwnd, WORD wParam, POINT pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

#ifdef DEBUG_NONCLIENT
    printf( "Handling WM_SYSCOMMAND %x %d,%d\n", wParam, pt.x, pt.y );
#endif

    switch (wParam & 0xfff0)
    {
    case SC_SIZE:
	if (!HAS_THICKFRAME(wndPtr->dwStyle)) break;
	if (IsZoomed(hwnd) || IsIconic(hwnd)) break;
	if (wParam & 0x0f) captureHitTest = (wParam & 0x0f) + HTLEFT - 1;
	else captureHitTest = HTBORDER;
	capturePoint = pt;
	SetCapture( hwnd );
	GetWindowRect( hwnd, &sizingRect );
	NC_DrawMovingFrame( hwnd, &sizingRect );
	break;

    case SC_MOVE:
	if (!(wndPtr->dwStyle & WS_CAPTION)) break;
	if (IsZoomed(hwnd) || IsIconic(hwnd)) break;
	captureHitTest = HTCAPTION;
	capturePoint = pt;
	SetCapture( hwnd );
	GetWindowRect( hwnd, &sizingRect );
	NC_DrawMovingFrame( hwnd, &sizingRect );
	break;

    case SC_MINIMIZE:
	ShowWindow( hwnd, SW_MINIMIZE );
	break;

    case SC_MAXIMIZE:
	ShowWindow( hwnd, SW_MAXIMIZE );
	break;

    case SC_RESTORE:
	ShowWindow( hwnd, SW_RESTORE );
	break;

    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
	break;

    case SC_CLOSE:
	return SendMessage( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
    case SC_MOUSEMENU:
    case SC_KEYMENU:
    case SC_ARRANGE:
	break;

    case SC_TASKLIST:
    case SC_SCREENSAVE:
    case SC_HOTKEY:
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LONG NC_HandleSetCursor( HWND hwnd, WORD wParam, LONG lParam )
{
    if (hwnd != wParam) return 0;  /* Don't set the cursor for child windows */

    switch(LOWORD(lParam))
    {
    case HTERROR:
	{
	    WORD msg = HIWORD( lParam );
	    if ((msg == WM_LBUTTONDOWN) || (msg == WM_MBUTTONDOWN) ||
		(msg == WM_RBUTTONDOWN))
		MessageBeep(0);
	}
	break;

    case HTCLIENT:
	{
	    WND *wndPtr = WIN_FindWndPtr( hwnd );
	    if (wndPtr && wndPtr->hCursor)
	    {
		SetCursor( wndPtr->hCursor );
		return TRUE;
	    }
	}
	break;

    case HTLEFT:
    case HTRIGHT:
	SetCursor( LoadCursor( 0, IDC_SIZEWE ) );
	return TRUE;

    case HTTOP:
    case HTBOTTOM:
	SetCursor( LoadCursor( 0, IDC_SIZENS ) );
	return TRUE;

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	SetCursor( LoadCursor( 0, IDC_SIZENWSE ) );
	return TRUE;

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	SetCursor( LoadCursor( 0, IDC_SIZENESW ) );
	return TRUE;
    }

    /* Default cursor: arrow */
    SetCursor( LoadCursor( 0, IDC_ARROW ) );
    return TRUE;
}

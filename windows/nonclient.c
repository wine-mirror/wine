/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include "win.h"
#include "message.h"
#include "sysmetrics.h"
#include "user.h"
#include "scroll.h"


static HBITMAP hbitmapClose = 0;
static HBITMAP hbitmapMinimize = 0;
static HBITMAP hbitmapMinimizeD = 0;
static HBITMAP hbitmapMaximize = 0;
static HBITMAP hbitmapMaximizeD = 0;
static HBITMAP hbitmapRestore = 0;
static HBITMAP hbitmapRestoreD = 0;

extern void NC_TrackSysMenu( HWND hwnd ); /* menu.c */
extern void WINPOS_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
			    POINT *minTrack, POINT *maxTrack );  /* winpos.c */

extern Display * display;


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
    {
	InflateRect( rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
	if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME) InflateRect( rect, -1, 0);
    }
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
    if (hwnd == GetCapture()) return HTCLIENT;
    return NC_InternalNCHitTest( hwnd, pt );
}


/***********************************************************************
 *           NC_DrawSysButton
 */
static void NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
	SelectObject( hdcMem, hbitmapClose );
	BitBlt( hdc, rect.left, rect.top, SYSMETRICS_CXSIZE,
	       SYSMETRICS_CYSIZE, hdcMem, 1, 1, down ? NOTSRCCOPY : SRCCOPY );
	DeleteDC( hdcMem );
    }
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
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
static void NC_DrawMinButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;	
	if (down) SelectObject( hdcMem, hbitmapMinimizeD );
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
	width = SYSMETRICS_CXDLGFRAME - 1;
	height = SYSMETRICS_CYDLGFRAME - 1;
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
static void NC_DrawMovingFrame( HDC hdc, RECT *rect, BOOL thickframe )
{
    if (thickframe)
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
}


/***********************************************************************
 *           NC_DrawCaption
 *
 * Draw the window caption.
 * The correct pen for the window frame must be selected in the DC.
 */
static void NC_DrawCaption( HDC hdc, RECT *rect, HWND hwnd,
			    DWORD style, BOOL active )
{
    RECT r = *rect;
    HBRUSH hbrushCaption;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
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
    
    if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
    {
	HBRUSH hbrushWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	HBRUSH hbrushOld = SelectObject( hdc, hbrushWindow );
	PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
	PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
	r.left++;
	r.right--;
	SelectObject( hdc, hbrushOld );
	DeleteObject( hbrushWindow );
    }

    if (active)
	hbrushCaption = CreateSolidBrush( GetSysColor(COLOR_ACTIVECAPTION) );
    else hbrushCaption = CreateSolidBrush( GetSysColor(COLOR_INACTIVECAPTION));

    MoveTo( hdc, r.left, r.bottom );
    LineTo( hdc, r.right-1, r.bottom );

    if (style & WS_SYSMENU)
    {
	NC_DrawSysButton( hwnd, hdc, FALSE );
	r.left += SYSMETRICS_CXSIZE + 1;
	MoveTo( hdc, r.left - 1, r.top );
	LineTo( hdc, r.left - 1, r.bottom );
    }
    if (style & WS_MAXIMIZEBOX)
    {
	NC_DrawMaxButton( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX)
    {
	NC_DrawMinButton( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }

    FillRect( hdc, &r, hbrushCaption );
    DeleteObject( hbrushCaption );

    if (GetWindowText( hwnd, buffer, 256 ))
    {
	if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	DrawText( hdc, buffer, -1, &r, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }
}


/***********************************************************************
 *           NC_DoNCPaint
 *
 * Paint the non-client area.
 */
static void NC_DoNCPaint( HWND hwnd, HRGN hrgn, BOOL active )
{
    HDC hdc;
    RECT rect, rect2;
    HBRUSH hbrushBorder = 0;
    HPEN hpenFrame = 0;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

#ifdef DEBUG_NONCLIENT
    printf( "NC_HandleNCPaint: %d %d\n", hwnd, hrgn );
#endif

    if (!wndPtr || !hrgn) return;
    if (!(wndPtr->dwStyle & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)))
	return;  /* Nothing to do! */

    if (hrgn == 1) hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    else hdc = GetDCEx( hwnd, hrgn, DCX_CACHE | DCX_WINDOW | DCX_INTERSECTRGN);
    if (!hdc) return;
    if (ExcludeVisRect( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC( hwnd, hdc );
	return;
    }

    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    hpenFrame = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME) );
    SelectObject( hdc, hpenFrame );
    if (active)
	hbrushBorder = CreateSolidBrush( GetSysColor(COLOR_ACTIVEBORDER) );
    else hbrushBorder = CreateSolidBrush( GetSysColor(COLOR_INACTIVEBORDER) );
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

    if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
	NC_DrawFrame( hdc, &rect, TRUE );
    else if (wndPtr->dwStyle & WS_THICKFRAME)
	NC_DrawFrame(hdc, &rect, FALSE);

    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
	RECT r = rect;
	rect.top += SYSMETRICS_CYSIZE + 1;
	r.bottom = rect.top - 1;
	NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle, active );
    }

    if (wndPtr->dwStyle & (WS_VSCROLL | WS_HSCROLL))
    {
	if (wndPtr->dwStyle & WS_VSCROLL) {
	    int bottom = rect.bottom;
	    if (wndPtr->dwStyle & WS_HSCROLL) bottom -= SYSMETRICS_CYHSCROLL;
	    SetRect(&rect2, rect.right - SYSMETRICS_CXVSCROLL, rect.top,
		    rect.right, bottom); 
	    StdDrawScrollBar(hwnd, hdc, SB_VERT, &rect2, (LPHEADSCROLL)wndPtr->VScroll);
	    }
	if (wndPtr->dwStyle & WS_HSCROLL) {
	    int right = rect.right;
	    if (wndPtr->dwStyle & WS_VSCROLL) right -= SYSMETRICS_CYVSCROLL;
	    SetRect(&rect2, rect.left, rect.bottom - SYSMETRICS_CYHSCROLL,
		    right, rect.bottom);
	    StdDrawScrollBar(hwnd, hdc, SB_HORZ, &rect2, (LPHEADSCROLL)wndPtr->HScroll);
	    }
/*
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
*/
    }    

    ReleaseDC( hwnd, hdc );
    if (hbrushBorder) DeleteObject( hbrushBorder );
    if (hpenFrame) DeleteObject( hpenFrame );    
}


/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND hwnd, HRGN hrgn )
{
    NC_DoNCPaint( hwnd, hrgn, (hwnd == GetActiveWindow()) );
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LONG NC_HandleNCActivate( HWND hwnd, WORD wParam )
{
    NC_DoNCPaint( hwnd, (HRGN)1, wParam );
    return TRUE;
}


/***********************************************************************
 *           NC_DoSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
static void NC_DoSizeMove( HWND hwnd, WORD wParam, POINT pt )
{
    MSG msg;
    WORD hittest;
    RECT sizingRect;
    HDC hdc;
    BOOL thickframe;
    POINT minTrack, maxTrack, capturePoint = pt;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if (IsZoomed(hwnd) || IsIconic(hwnd) || !IsWindowVisible(hwnd)) return;
    hittest = wParam & 0x0f;
    thickframe = HAS_THICKFRAME( wndPtr->dwStyle );

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	if (!(wndPtr->dwStyle & WS_CAPTION)) return;
	if (!hittest)
	{
	      /* Move pointer at the center of the caption */
	    RECT rect;
	    POINT point;
	    NC_GetInsideRect( hwnd, &rect );
	    if (wndPtr->dwStyle & WS_SYSMENU)
		rect.left += SYSMETRICS_CXSIZE + 1;
	    if (wndPtr->dwStyle & WS_MINIMIZEBOX)
		rect.right -= SYSMETRICS_CXSIZE + 1;
	    if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
		rect.right -= SYSMETRICS_CXSIZE + 1;
	    point.x = wndPtr->rectWindow.left + (rect.right - rect.left) / 2;
	    point.y = wndPtr->rectWindow.top + rect.top + SYSMETRICS_CYSIZE/2;
	    if (wndPtr->dwStyle & WS_CHILD)
		ClientToScreen( wndPtr->hwndParent, &point );
	    SetCursorPos( point.x, point.y );
	    hittest = HTCAPTION;
	    capturePoint = point;
	}
    }
    else  /* SC_SIZE */
    {
	if (!thickframe) return;
	if (hittest) hittest += HTLEFT-1;
    }

    WINPOS_GetMinMaxInfo( hwnd, NULL, NULL, &minTrack, &maxTrack );
    SendMessage( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (wndPtr->dwStyle & WS_CHILD) hdc = GetDC( wndPtr->hwndParent );
    else
    {  /* Grab the server only when moving top-level windows */
	hdc = GetDC( 0 );
	XGrabServer( display );
    }
    SetCapture( hwnd );    
    sizingRect = wndPtr->rectWindow;
    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    while(1)
    {
	int dx = 0, dy = 0;

	MSG_GetHardwareMessage( &msg );

	  /* Exit on button-up, Return, or Esc */
	if ((msg.message == WM_LBUTTONUP) ||
	    ((msg.message == WM_KEYDOWN) && 
	     ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

	if (wndPtr->dwStyle & WS_CHILD)
	    ScreenToClient( wndPtr->hwndParent, &msg.pt );

	switch(msg.message)
	{
	case WM_MOUSEMOVE:
	    dx = msg.pt.x - capturePoint.x;
	    dy = msg.pt.y - capturePoint.y;
	    break;

	case WM_KEYDOWN:
	    switch(msg.wParam)
	    {
	        case VK_UP:    msg.pt.y -= 8; break;
		case VK_DOWN:  msg.pt.y += 8; break;
		case VK_LEFT:  msg.pt.x -= 8; break;
		case VK_RIGHT: msg.pt.x += 8; break;		
	    }
	    SetCursorPos( msg.pt.x, msg.pt.y );
	    break;
	}	

	if (dx || dy)
	{
	    RECT newRect = sizingRect;
	    switch(hittest)
	    {
	    case HTCAPTION:
		OffsetRect( &newRect, dx, dy );
		break;
	    case HTLEFT:
		newRect.left += dx;
		break;
	    case HTRIGHT:
		newRect.right += dx;
		break;
	    case HTTOP:
		newRect.top += dy;
		break;
	    case HTTOPLEFT:
		newRect.left += dx;
		newRect.top  += dy;
		break;
	    case HTTOPRIGHT:
		newRect.right += dx;
		newRect.top   += dy;
		break;
	    case HTBOTTOM:
		newRect.bottom += dy;
		break;
	    case HTBOTTOMLEFT:
		newRect.left   += dx;
		newRect.bottom += dy;
		break;
	    case HTBOTTOMRIGHT:
		newRect.right  += dx;
		newRect.bottom += dy;
		break; 
	    }	    
	    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
	    NC_DrawMovingFrame( hdc, &newRect, thickframe );
	    capturePoint = msg.pt;
	    sizingRect = newRect;
	}
    }

    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
    ReleaseCapture();
    if (wndPtr->dwStyle & WS_CHILD) ReleaseDC( wndPtr->hwndParent, hdc );
    else
    {
	ReleaseDC( 0, hdc );
	XUngrabServer( display );
    }
    SendMessage( hwnd, WM_EXITSIZEMOVE, 0, 0 );

      /* If Esc key, don't move the window */
    if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) return;

    if (hittest != HTCAPTION)
	SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
		     sizingRect.right - sizingRect.left,
		     sizingRect.bottom - sizingRect.top,
		     SWP_NOACTIVATE | SWP_NOZORDER );
    else SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top, 0, 0,
		      SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
}


/***********************************************************************
 *           NC_TrackMinMaxBox
 *
 * Track a mouse button press on the minimize or maximize box.
 */
static void NC_TrackMinMaxBox( HWND hwnd, WORD wParam )
{
    MSG msg;
    HDC hdc = GetWindowDC( hwnd );
    BOOL pressed = TRUE;

    SetCapture( hwnd );
    if (wParam == HTMINBUTTON) NC_DrawMinButton( hwnd, hdc, TRUE );
    else NC_DrawMaxButton( hwnd, hdc, TRUE );

    do
    {
	BOOL oldstate = pressed;
	MSG_GetHardwareMessage( &msg );

	pressed = (NC_InternalNCHitTest( hwnd, msg.pt ) == wParam);
	if (pressed != oldstate)
	{
	    if (wParam == HTMINBUTTON) NC_DrawMinButton( hwnd, hdc, pressed );
	    else NC_DrawMaxButton( hwnd, hdc, pressed );	    
	}
    } while (msg.message != WM_LBUTTONUP);

    if (wParam == HTMINBUTTON) NC_DrawMinButton( hwnd, hdc, FALSE );
    else NC_DrawMaxButton( hwnd, hdc, FALSE );

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    if (wParam == HTMINBUTTON) 
	SendMessage( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, *(LONG*)&msg.pt );
    else
	SendMessage( hwnd, WM_SYSCOMMAND, 
		  IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, *(LONG*)&msg.pt );
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
	SendMessage( hwnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam );
	break;

    case HTSYSMENU:
	NC_DrawSysButton( hwnd, hdc, TRUE );
	NC_TrackSysMenu(hwnd);
	break;

    case HTMENU:
	break;

    case HTHSCROLL:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_HSCROLL, lParam );
	break;

    case HTVSCROLL:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_VSCROLL, lParam );
	break;

    case HTMINBUTTON:
    case HTMAXBUTTON:
	NC_TrackMinMaxBox( hwnd, wParam );
	break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_SIZE + wParam - HTLEFT+1, lParam);
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

    if (wndPtr->dwStyle & WS_CHILD) ScreenToClient( wndPtr->hwndParent, &pt );

    switch (wParam & 0xfff0)
    {
    case SC_SIZE:
    case SC_MOVE:
	NC_DoSizeMove( hwnd, wParam, pt );
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
	break;
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



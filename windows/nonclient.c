/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include "win.h"
#include "class.h"
#include "message.h"
#include "sysmetrics.h"
#include "user.h"
#include "scroll.h"
#include "menu.h"
#include "syscolor.h"

static HBITMAP hbitmapClose = 0;
static HBITMAP hbitmapMDIClose = 0;
static HBITMAP hbitmapMinimize = 0;
static HBITMAP hbitmapMinimizeD = 0;
static HBITMAP hbitmapMaximize = 0;
static HBITMAP hbitmapMaximizeD = 0;
static HBITMAP hbitmapRestore = 0;
static HBITMAP hbitmapRestoreD = 0;

extern void WINPOS_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
			    POINT *minTrack, POINT *maxTrack );  /* winpos.c */
extern void CURSOR_SetWinCursor( HWND hwnd, HCURSOR hcursor );   /* cursor.c */


  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((style) & WS_DLGFRAME) && \
     (((exStyle) & WS_EX_DLGMODALFRAME) || !((style) & WS_BORDER)))

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_MENU(w)  (!((w)->dwStyle & WS_CHILD) && ((w)->wIDmenu != 0))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))

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
void NC_GetInsideRect( HWND hwnd, RECT *rect )
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
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem = CreateCompatibleDC( hdc );
    if (hdcMem)
    {
	NC_GetInsideRect( hwnd, &rect );
	if (wndPtr->dwStyle & WS_CHILD)
		SelectObject( hdcMem, hbitmapMDIClose );
	else
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
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    char buffer[256];

    if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmap( 0, MAKEINTRESOURCE(OBM_CLOSE) )))
	    return;
	if (!(hbitmapMDIClose = LoadBitmap( 0, MAKEINTRESOURCE(OBM_OLD_CLOSE) )))
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
	HBRUSH hbrushOld = SelectObject( hdc, sysColorObjects.hbrushWindow );
	PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
	PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
	r.left++;
	r.right--;
	SelectObject( hdc, hbrushOld );
    }

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

    FillRect( hdc, &r, active ? sysColorObjects.hbrushActiveCaption : 
	                        sysColorObjects.hbrushInactiveCaption );

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
 * 'hrgn' is the update rgn to use (in client coords) or 1 if no update rgn.
 */
void NC_DoNCPaint( HWND hwnd, HRGN hrgn, BOOL active, BOOL suppress_menupaint )
{
    HDC hdc;
    RECT rect, rect2;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

#ifdef DEBUG_NONCLIENT
    printf( "NC_DoNCPaint: %d %d\n", hwnd, hrgn );
#endif
    if (!IsWindowVisible(hwnd)) return;
    if (!wndPtr || !hrgn) return;
    if (!(wndPtr->dwStyle & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)))
	return;  /* Nothing to do! */

    if (hrgn == 1) hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    else
    {
	  /* Make region relative to window area */
	int xoffset = wndPtr->rectWindow.left - wndPtr->rectClient.left;
	int yoffset = wndPtr->rectWindow.top - wndPtr->rectClient.top;
	OffsetRgn( hrgn, -xoffset, -yoffset );
	hdc = GetDCEx( hwnd, hrgn, DCX_CACHE | DCX_WINDOW | DCX_INTERSECTRGN);
	OffsetRgn( hrgn, xoffset, yoffset );  /* Restore region */
    }
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

    SelectObject( hdc, sysColorObjects.hpenWindowFrame );
    SelectObject( hdc, active ? sysColorObjects.hbrushActiveBorder :
		                sysColorObjects.hbrushInactiveBorder );

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

    if (wndPtr->wIDmenu != 0 &&
	(wndPtr->dwStyle & WS_CHILD) != WS_CHILD) {
	LPPOPUPMENU	lpMenu = (LPPOPUPMENU) GlobalLock(wndPtr->wIDmenu);
	if (lpMenu != NULL) {
		int oldHeight;
		CopyRect(&rect2, &rect);
		/* Default MenuBar height */
		if (lpMenu->Height == 0) lpMenu->Height = SYSMETRICS_CYMENU + 1;
		oldHeight = lpMenu->Height;
		rect2.bottom = rect2.top + oldHeight; 
		StdDrawMenuBar(hdc, &rect2, lpMenu, suppress_menupaint);
		if (oldHeight != lpMenu->Height) {
			printf("NC_DoNCPaint // menubar changed oldHeight=%d != lpMenu->Height=%d\n",
									oldHeight, lpMenu->Height);
			/* Reduce ClientRect according to MenuBar height */
			wndPtr->rectClient.top -= oldHeight;
			wndPtr->rectClient.top += lpMenu->Height;
			}
		GlobalUnlock(wndPtr->wIDmenu);
		}
	}

    if (wndPtr->dwStyle & (WS_VSCROLL | WS_HSCROLL)) {
 	if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->VScroll != NULL) &&
	    (wndPtr->scroll_flags & 0x0001)) {
 	    int bottom = rect.bottom;
 	    if ((wndPtr->dwStyle & WS_HSCROLL) && (wndPtr->scroll_flags & 0x0001))
			bottom -= SYSMETRICS_CYHSCROLL;
	    SetRect(&rect2, rect.right - SYSMETRICS_CXVSCROLL, 
	    	rect.top, rect.right, bottom); 
	    if (wndPtr->dwStyle & WS_CAPTION) rect.top += SYSMETRICS_CYSIZE;
	    if (wndPtr->wIDmenu != 0 && (wndPtr->dwStyle & WS_CHILD) != WS_CHILD) 
	    	rect2.top += SYSMETRICS_CYMENU + 1;
 	    StdDrawScrollBar(hwnd, hdc, SB_VERT, &rect2, (LPHEADSCROLL)wndPtr->VScroll);
 	    }
 	if ((wndPtr->dwStyle & WS_HSCROLL) && wndPtr->HScroll != NULL &&
	    (wndPtr->scroll_flags & 0x0002)) {
	    int right = rect.right;
	    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->scroll_flags & 0x0001))
			right -= SYSMETRICS_CYVSCROLL;
	    SetRect(&rect2, rect.left, rect.bottom - SYSMETRICS_CYHSCROLL,
		    right, rect.bottom);
	    StdDrawScrollBar(hwnd, hdc, SB_HORZ, &rect2, (LPHEADSCROLL)wndPtr->HScroll);
	    }

	if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL) &&
	    (wndPtr->scroll_flags & 0x0003) == 0x0003) {
		RECT r = rect;
		r.left = r.right - SYSMETRICS_CXVSCROLL;
		r.top  = r.bottom - SYSMETRICS_CYHSCROLL;
		FillRect( hdc, &r, sysColorObjects.hbrushScrollbar );
		}
    }    

    ReleaseDC( hwnd, hdc );
}


NC_DoNCPaintIcon(HWND hwnd)
{
      WND *wndPtr = WIN_FindWndPtr(hwnd);
      PAINTSTRUCT ps;
      HDC hdc;
      int ret;
      DC *dc;
      GC testgc;
      int s;
      char buffer[256];

      printf("painting icon\n");
      if (wndPtr == NULL) {
              printf("argh, can't find an icon to draw\n");
              return;
      }
      hdc = BeginPaint(hwnd, &ps);

      ret = DrawIcon(hdc, 100/2 - 16, 0, wndPtr->hIcon);
      printf("ret is %d\n", ret);

      if (s=GetWindowText(hwnd, buffer, 256))
      {
          /*SetBkColor(hdc, TRANSPARENT); */
          TextOut(hdc, 0, 32, buffer, s);
      }
      EndPaint(hwnd, &ps);

      printf("done painting icon\n");
      
}


LONG NC_HandleNCPaintIcon( HWND hwnd )
{
    NC_DoNCPaintIcon(hwnd);
    return 0;
}



/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND hwnd, HRGN hrgn )
{
    NC_DoNCPaint( hwnd, hrgn, hwnd == GetActiveWindow(), FALSE );
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LONG NC_HandleNCActivate( HWND hwnd, WORD wParam )
{
    NC_DoNCPaint( hwnd, (HRGN)1, wParam, FALSE );
    return TRUE;
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
	    WND *wndPtr;
	    CLASS *classPtr;
	    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
	    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) break;
	    if (classPtr->wc.hCursor)
	    {
		CURSOR_SetWinCursor( hwnd, classPtr->wc.hCursor );
		return TRUE;
	    }
	}
	break;

    case HTLEFT:
    case HTRIGHT:
	CURSOR_SetWinCursor( hwnd, LoadCursor( 0, IDC_SIZEWE ) );
	return TRUE;

    case HTTOP:
    case HTBOTTOM:
	CURSOR_SetWinCursor( hwnd, LoadCursor( 0, IDC_SIZENS ) );
	return TRUE;

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	CURSOR_SetWinCursor( hwnd, LoadCursor( 0, IDC_SIZENWSE ) );
	return TRUE;

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	CURSOR_SetWinCursor( hwnd, LoadCursor( 0, IDC_SIZENESW ) );
	return TRUE;
    }

    /* Default cursor: arrow */
    CURSOR_SetWinCursor( hwnd, LoadCursor( 0, IDC_ARROW ) );
    return TRUE;
}


/***********************************************************************
 *           NC_StartSizeMove
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG NC_StartSizeMove( HWND hwnd, WORD wParam, POINT *capturePoint )
{
    LONG hittest = 0;
    POINT pt;
    MSG msg;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	  /* Move pointer at the center of the caption */
	RECT rect;
	NC_GetInsideRect( hwnd, &rect );
	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.left += SYSMETRICS_CXSIZE + 1;
	if (wndPtr->dwStyle & WS_MINIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;
	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;
	pt.x = wndPtr->rectWindow.left + (rect.right - rect.left) / 2;
	pt.y = wndPtr->rectWindow.top + rect.top + SYSMETRICS_CYSIZE/2;
	if (wndPtr->dwStyle & WS_CHILD)
	    ClientToScreen( wndPtr->hwndParent, &pt );
	hittest = HTCAPTION;
    }
    else  /* SC_SIZE */
    {
	SetCapture(hwnd);
	while(!hittest)
	{
	    MSG_GetHardwareMessage( &msg );
	    switch(msg.message)
	    {
	    case WM_MOUSEMOVE:
		hittest = NC_InternalNCHitTest( hwnd, msg.pt );
		pt = msg.pt;
		if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT))
		    hittest = 0;
		break;

	    case WM_LBUTTONUP:
		return 0;

	    case WM_KEYDOWN:
		switch(msg.wParam)
		{
		case VK_UP:
		    hittest = HTTOP;
		    pt.x =(wndPtr->rectWindow.left+wndPtr->rectWindow.right)/2;
		    pt.y = wndPtr->rectWindow.top + SYSMETRICS_CYFRAME / 2;
		    break;
		case VK_DOWN:
		    hittest = HTBOTTOM;
		    pt.x =(wndPtr->rectWindow.left+wndPtr->rectWindow.right)/2;
		    pt.y = wndPtr->rectWindow.bottom - SYSMETRICS_CYFRAME / 2;
		    break;
		case VK_LEFT:
		    hittest = HTLEFT;
		    pt.x = wndPtr->rectWindow.left + SYSMETRICS_CXFRAME / 2;
		    pt.y =(wndPtr->rectWindow.top+wndPtr->rectWindow.bottom)/2;
		    break;
		case VK_RIGHT:
		    hittest = HTRIGHT;
		    pt.x = wndPtr->rectWindow.right - SYSMETRICS_CXFRAME / 2;
		    pt.y =(wndPtr->rectWindow.top+wndPtr->rectWindow.bottom)/2;
		    break;
		case VK_RETURN:
		case VK_ESCAPE: return 0;
		}
	    }
	}
    }
    *capturePoint = pt;
    SetCursorPos( capturePoint->x, capturePoint->y );
    NC_HandleSetCursor( hwnd, hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           NC_DoSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
static void NC_DoSizeMove( HWND hwnd, WORD wParam, POINT pt )
{
    MSG msg;
    LONG hittest;
    RECT sizingRect, mouseRect;
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
	if (!hittest) hittest = NC_StartSizeMove( hwnd, wParam, &capturePoint );
	if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
	if (!thickframe) return;
	if (hittest) hittest += HTLEFT-1;
	else
	{
	    SetCapture(hwnd);
	    hittest = NC_StartSizeMove( hwnd, wParam, &capturePoint );
	    if (!hittest)
	    {
		ReleaseCapture();
		return;
	    }
	}
    }

      /* Get min/max info */

    WINPOS_GetMinMaxInfo( hwnd, NULL, NULL, &minTrack, &maxTrack );
    sizingRect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	GetClientRect( wndPtr->hwndParent, &mouseRect );
    else SetRect( &mouseRect, 0, 0, SYSMETRICS_CXSCREEN, SYSMETRICS_CYSCREEN );
    if (ON_LEFT_BORDER(hittest))
    {
	mouseRect.left  = max( mouseRect.left, sizingRect.right-maxTrack.x );
	mouseRect.right = min( mouseRect.right, sizingRect.right-minTrack.x );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
	mouseRect.left  = max( mouseRect.left, sizingRect.left+minTrack.x );
	mouseRect.right = min( mouseRect.right, sizingRect.left+maxTrack.x );
    }
    if (ON_TOP_BORDER(hittest))
    {
	mouseRect.top    = max( mouseRect.top, sizingRect.bottom-maxTrack.y );
	mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
	mouseRect.top    = max( mouseRect.top, sizingRect.top+minTrack.y );
	mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
    SendMessage( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (GetCapture() != hwnd) SetCapture( hwnd );    

    if (wndPtr->dwStyle & WS_CHILD) hdc = GetDC( wndPtr->hwndParent );
    else
    {  /* Grab the server only when moving top-level windows without desktop */
	hdc = GetDC( 0 );
	if (rootWindow == DefaultRootWindow(display)) XGrabServer( display );
    }
    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    while(1)
    {
	int dx = 0, dy = 0;

	MSG_GetHardwareMessage( &msg );

	  /* Exit on button-up, Return, or Esc */
	if ((msg.message == WM_LBUTTONUP) ||
	    ((msg.message == WM_KEYDOWN) && 
	     ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

	if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
	    continue;  /* We are not interested in other messages */

	pt = msg.pt;
	if (wndPtr->dwStyle & WS_CHILD)
	    ScreenToClient( wndPtr->hwndParent, &pt );

	
	if (msg.message == WM_KEYDOWN) switch(msg.wParam)
	{
	    case VK_UP:    pt.y -= 8; break;
	    case VK_DOWN:  pt.y += 8; break;
	    case VK_LEFT:  pt.x -= 8; break;
	    case VK_RIGHT: pt.x += 8; break;		
	}

	pt.x = max( pt.x, mouseRect.left );
	pt.x = min( pt.x, mouseRect.right );
	pt.y = max( pt.y, mouseRect.top );
	pt.y = min( pt.y, mouseRect.bottom );

	dx = pt.x - capturePoint.x;
	dy = pt.y - capturePoint.y;

	if (dx || dy)
	{
	    if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
	    else
	    {
		RECT newRect = sizingRect;

		if (hittest == HTCAPTION) OffsetRect( &newRect, dx, dy );
		if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
		else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
		if (ON_TOP_BORDER(hittest)) newRect.top += dy;
		else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
		NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
		NC_DrawMovingFrame( hdc, &newRect, thickframe );
		capturePoint = pt;
		sizingRect = newRect;
	    }
	}
    }

    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
    ReleaseCapture();
    if (wndPtr->dwStyle & WS_CHILD) ReleaseDC( wndPtr->hwndParent, hdc );
    else
    {
	ReleaseDC( 0, hdc );
	if (rootWindow == DefaultRootWindow(display)) XUngrabServer( display );
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
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND hwnd, WORD wParam, POINT pt )
{
    MSG 	msg;
    WORD 	scrollbar;
    if ((wParam & 0xfff0) == SC_HSCROLL)
    {
	if ((wParam & 0x0f) != HTHSCROLL) return;
	scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
	if ((wParam & 0x0f) != HTVSCROLL) return;
	scrollbar = SB_VERT;
    }

    ScreenToClient( hwnd, &pt );
    ScrollBarButtonDown( hwnd, scrollbar, pt.x, pt.y );
    SetCapture( hwnd );

    do
    {
	MSG_GetHardwareMessage( &msg );
	ScreenToClient( msg.hwnd, &msg.pt );
	switch(msg.message)
	{
	case WM_LBUTTONUP:
	    ScrollBarButtonUp( hwnd, scrollbar, msg.pt.x, msg.pt.y );
	    break;
	case WM_MOUSEMOVE:
	    ScrollBarMouseMove(hwnd, scrollbar, msg.wParam, msg.pt.x,msg.pt.y);
	    break;
	}
    } while (msg.message != WM_LBUTTONUP);
    ReleaseCapture();
}


/***********************************************************************
 *           NC_TrackMouseMenuBar
 *
 * Track a mouse events for the MenuBar.
 */
static void NC_TrackMouseMenuBar( HWND hwnd, WORD wParam, POINT pt )
{
    WND		*wndPtr;
    LPPOPUPMENU lppop;
    MSG 	msg;
    wndPtr = WIN_FindWndPtr(hwnd);
    lppop = (LPPOPUPMENU)GlobalLock(wndPtr->wIDmenu);
#ifdef DEBUG_MENU
    printf("NC_TrackMouseMenuBar // wndPtr=%08X lppop=%08X !\n", wndPtr, lppop);
#endif
    ScreenToClient(hwnd, &pt);
    pt.y += lppop->rect.bottom;
    SetCapture(hwnd);
    MenuButtonDown(hwnd, lppop, pt.x, pt.y);
    GlobalUnlock(wndPtr->wIDmenu);
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
	SendMessage( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU, lParam );
	break;

    case HTHSCROLL:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
	break;

    case HTVSCROLL:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
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
	ICON_Iconify( hwnd );
	/*ShowWindow( hwnd, SW_MINIMIZE );*/
	break;

    case SC_MAXIMIZE:
	ShowWindow( hwnd, SW_MAXIMIZE );
	break;

    case SC_RESTORE:
	ICON_Deiconify(hwnd);
	ShowWindow( hwnd, SW_RESTORE );
	break;

    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
	break;

    case SC_CLOSE:
	return SendMessage( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
    if (wndPtr->dwStyle & WS_CHILD) ClientToScreen(wndPtr->hwndParent, &pt);
	NC_TrackScrollBar( hwnd, wParam, pt );
	break;

    case SC_MOUSEMENU:
	NC_TrackMouseMenuBar( hwnd, wParam, pt );
	break;

    case SC_KEYMENU:
/*	NC_KeyMenuBar( hwnd, wParam, pt ); */
	break;
	
    case SC_ARRANGE:
	break;

    case SC_TASKLIST:
    case SC_SCREENSAVE:
    case SC_HOTKEY:
	break;
    }
    return 0;
}



/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include "win.h"
#include "class.h"
#include "message.h"
#include "sysmetrics.h"
#include "user.h"
#include "shell.h"
#include "dialog.h"
#include "syscolor.h"
#include "menu.h"
#include "winpos.h"
#include "scroll.h"
#include "nonclient.h"
#include "graphics.h"
#include "selectors.h"
#include "stackframe.h"
#include "stddebug.h"
/* #define DEBUG_NONCLIENT */
#include "debug.h"
#include "options.h"


static HBITMAP hbitmapClose = 0;
static HBITMAP hbitmapMinimize = 0;
static HBITMAP hbitmapMinimizeD = 0;
static HBITMAP hbitmapMaximize = 0;
static HBITMAP hbitmapMaximizeD = 0;
static HBITMAP hbitmapRestore = 0;
static HBITMAP hbitmapRestoreD = 0;

#define SC_ABOUTWINE    	(SC_SCREENSAVE+1)

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_BORDER)))

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
    if (style & WS_ICONIC) return;  /* Nothing to change for an icon */

    /* Decide if the window will be managed (see CreateWindowEx) */
    if (!(Options.managed && ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
          (exStyle & WS_EX_DLGMODALFRAME))))
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
            rect->top -= SYSMETRICS_CYCAPTION - SYSMETRICS_CYBORDER;
    }
    if (menu) rect->top -= SYSMETRICS_CYMENU + SYSMETRICS_CYBORDER;

    if (style & WS_VSCROLL) rect->right  += SYSMETRICS_CXVSCROLL;
    if (style & WS_HSCROLL) rect->bottom += SYSMETRICS_CYHSCROLL;
}


/***********************************************************************
 *           AdjustWindowRect    (USER.102)
 */
BOOL AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    return AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *           AdjustWindowRectEx    (USER.454)
 */
BOOL AdjustWindowRectEx( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
      /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	style |= WS_CAPTION;
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

    dprintf_nonclient(stddeb, "AdjustWindowRectEx: (%ld,%ld)-(%ld,%ld) %08lx %d %08lx\n",
      (LONG)rect->left, (LONG)rect->top, (LONG)rect->right, (LONG)rect->bottom,
      style, menu, exStyle );

    NC_AdjustRect( rect, style, menu, exStyle );
    return TRUE;
}


/*******************************************************************
 *         NC_GetMinMaxInfo
 *
 * Get the minimized and maximized information for a window.
 */
void NC_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
                       POINT *minTrack, POINT *maxTrack )
{
    MINMAXINFO MinMax;
    short xinc, yinc;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

      /* Compute default values */

    MinMax.ptMaxSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxSize.y = SYSMETRICS_CYSCREEN;
    MinMax.ptMinTrackSize.x = SYSMETRICS_CXMINTRACK;
    MinMax.ptMinTrackSize.y = SYSMETRICS_CYMINTRACK;
    MinMax.ptMaxTrackSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxTrackSize.y = SYSMETRICS_CYSCREEN;

    if (wndPtr->flags & WIN_MANAGED) xinc = yinc = 0;
    else if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        xinc = SYSMETRICS_CXDLGFRAME;
        yinc = SYSMETRICS_CYDLGFRAME;
    }
    else
    {
        xinc = yinc = 0;
	if (HAS_THICKFRAME(wndPtr->dwStyle))
        {
            xinc += SYSMETRICS_CXFRAME;
            yinc += SYSMETRICS_CYFRAME;
        }
	if (wndPtr->dwStyle & WS_BORDER)
        {
            xinc += SYSMETRICS_CXBORDER;
            yinc += SYSMETRICS_CYBORDER;
        }
    }
    MinMax.ptMaxSize.x += 2 * xinc;
    MinMax.ptMaxSize.y += 2 * yinc;

    /* Note: The '+' in the following test should really be a ||, but
     * that would cause gcc-2.7.0 to generate incorrect code.
     */
    if ((wndPtr->ptMaxPos.x != -1) + (wndPtr->ptMaxPos.y != -1))
        MinMax.ptMaxPosition = wndPtr->ptMaxPos;
    else
    {
        MinMax.ptMaxPosition.x = -xinc;
        MinMax.ptMaxPosition.y = -yinc;
    }

    SendMessage( hwnd, WM_GETMINMAXINFO, 0, (LPARAM)MAKE_SEGPTR(&MinMax) );

      /* Some sanity checks */

    dprintf_nonclient(stddeb, 
		      "NC_GetMinMaxInfo: %d %d / %d %d / %d %d / %d %d\n",
		      (int)MinMax.ptMaxSize.x,(int)MinMax.ptMaxSize.y,
		      (int)MinMax.ptMaxPosition.x,(int)MinMax.ptMaxPosition.y,
		      (int)MinMax.ptMaxTrackSize.x,(int)MinMax.ptMaxTrackSize.y,
		      (int)MinMax.ptMinTrackSize.x,(int)MinMax.ptMinTrackSize.y);
    MinMax.ptMaxTrackSize.x = MAX( MinMax.ptMaxTrackSize.x,
				   MinMax.ptMinTrackSize.x );
    MinMax.ptMaxTrackSize.y = MAX( MinMax.ptMaxTrackSize.y,
				   MinMax.ptMinTrackSize.y );
    
    if (maxSize) *maxSize = MinMax.ptMaxSize;
    if (maxPos) *maxPos = MinMax.ptMaxPosition;
    if (minTrack) *minTrack = MinMax.ptMinTrackSize;
    if (maxTrack) *maxTrack = MinMax.ptMaxTrackSize;
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
    NC_AdjustRect( &tmpRect, wndPtr->dwStyle, FALSE, wndPtr->dwExStyle );
    params->rgrc[0].left   -= tmpRect.left;
    params->rgrc[0].top    -= tmpRect.top;
    params->rgrc[0].right  -= tmpRect.right;
    params->rgrc[0].bottom -= tmpRect.bottom;

    if (HAS_MENU(wndPtr))
    {
	params->rgrc[0].top += MENU_GetMenuBarHeight( hwnd,
				  params->rgrc[0].right - params->rgrc[0].left,
				  -tmpRect.left, -tmpRect.top ) + 1;
    }
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

    if (wndPtr->dwStyle & WS_ICONIC) return;  /* No border to remove */
    if (wndPtr->flags & WIN_MANAGED) return;

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
 *           NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG NC_HandleNCHitTest( HWND hwnd, POINT pt )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return HTERROR;

    dprintf_nonclient(stddeb, "NC_HandleNCHitTest: hwnd="NPFMT" pt=%ld,%ld\n",
		      hwnd, (LONG)pt.x, (LONG)pt.y );

    GetWindowRect( hwnd, &rect );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        /* Check borders */
        if (HAS_THICKFRAME( wndPtr->dwStyle ))
        {
            InflateRect( &rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
            if (wndPtr->dwStyle & WS_BORDER)
                InflateRect(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
            if (!PtInRect( &rect, pt ))
            {
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
            }
        }
        else  /* No thick frame */
        {
            if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
                InflateRect(&rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
            else if (wndPtr->dwStyle & WS_BORDER)
                InflateRect(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
            if (!PtInRect( &rect, pt )) return HTBORDER;
        }

        /* Check caption */

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            rect.top += SYSMETRICS_CYCAPTION - 1;
            if (!PtInRect( &rect, pt ))
            {
                /* Check system menu */
                if (wndPtr->dwStyle & WS_SYSMENU)
                    rect.left += SYSMETRICS_CXSIZE;
                if (pt.x <= rect.left) return HTSYSMENU;
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
    }

      /* Check client area */

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
		(pt.x >= rect.right - SYSMETRICS_CXVSCROLL))
		return HTSIZE;
	    return HTHSCROLL;
	}
    }

      /* Check menu bar */

    if (HAS_MENU(wndPtr))
    {
	if ((pt.y < 0) && (pt.x >= 0) && (pt.x < rect.right))
	    return HTMENU;
    }

      /* Should never get here */
    return HTERROR;
}


/***********************************************************************
 *           NC_DrawSysButton
 */
void NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;
    HBITMAP hbitmap;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    NC_GetInsideRect( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    hbitmap = SelectObject( hdcMem, hbitmapClose );
    BitBlt( hdc, rect.left, rect.top, SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
            hdcMem, (wndPtr->dwStyle & WS_CHILD) ? SYSMETRICS_CXSIZE : 0, 0,
            down ? NOTSRCCOPY : SRCCOPY );
    SelectObject( hdcMem, hbitmap );
    DeleteDC( hdcMem );
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    NC_GetInsideRect( hwnd, &rect );
    GRAPH_DrawBitmap( hdc, (IsZoomed(hwnd) ?
			    (down ? hbitmapRestoreD : hbitmapRestore) :
			    (down ? hbitmapMaximizeD : hbitmapMaximize)),
		     rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		     0, 0, SYSMETRICS_CXSIZE+1, SYSMETRICS_CYSIZE );
}


/***********************************************************************
 *           NC_DrawMinButton
 */
static void NC_DrawMinButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    NC_GetInsideRect( hwnd, &rect );
    if (wndPtr->dwStyle & WS_MAXIMIZEBOX) rect.right -= SYSMETRICS_CXSIZE + 1;
    GRAPH_DrawBitmap( hdc, (down ? hbitmapMinimizeD : hbitmapMinimize),
		     rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		     0, 0, SYSMETRICS_CXSIZE+1, SYSMETRICS_CYSIZE );
}


/***********************************************************************
 *           NC_DrawFrame
 *
 * Draw a window frame inside the given rectangle, and update the rectangle.
 * The correct pen for the frame must be selected in the DC.
 */
static void NC_DrawFrame( HDC hdc, RECT *rect, BOOL dlgFrame, BOOL active )
{
    short width, height, tmp;

    if (dlgFrame)
    {
	width = SYSMETRICS_CXDLGFRAME - 1;
	height = SYSMETRICS_CYDLGFRAME - 1;
        SelectObject( hdc, active ? sysColorObjects.hbrushActiveCaption :
                                    sysColorObjects.hbrushInactiveCaption );
    }
    else
    {
	width = SYSMETRICS_CXFRAME - 1;
	height = SYSMETRICS_CYFRAME - 1;
        SelectObject( hdc, active ? sysColorObjects.hbrushActiveBorder :
                                    sysColorObjects.hbrushInactiveBorder );
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
 */
void NC_DoNCPaint( HWND hwnd, BOOL active, BOOL suppress_menupaint )
{
    HDC hdc;
    RECT rect;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

    dprintf_nonclient(stddeb, "NC_DoNCPaint: "NPFMT" %d\n", hwnd, active );
    if (!wndPtr || !(wndPtr->dwStyle & WS_VISIBLE)) return; /* Nothing to do */

    if (!(hdc = GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW ))) return;

    /*
     * If this is an icon, we don't want to do any more nonclient painting
     * of the window manager.
     * If there is a class icon to draw, draw it
     */
    if (IsIconic(hwnd))
    {
        HICON hIcon = WIN_CLASS_INFO(wndPtr).hIcon;
        if (hIcon)  
        {
            SendMessage(hwnd, WM_ICONERASEBKGND, (WPARAM)hdc, 0);
            DrawIcon(hdc, 0, 0, hIcon);
        }
        ReleaseDC(hwnd, hdc);
        return;
    }

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

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        if ((wndPtr->dwStyle & WS_BORDER) || (wndPtr->dwStyle & WS_DLGFRAME) ||
            (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
        {
            MoveTo( hdc, 0, 0 );
            LineTo( hdc, rect.right-1, 0 );
            LineTo( hdc, rect.right-1, rect.bottom-1 );
            LineTo( hdc, 0, rect.bottom-1 );
            LineTo( hdc, 0, 0 );
            InflateRect( &rect, -1, -1 );
        }

        if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
            NC_DrawFrame( hdc, &rect, TRUE, active );
        else if (wndPtr->dwStyle & WS_THICKFRAME)
            NC_DrawFrame(hdc, &rect, FALSE, active );

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            RECT r = rect;
            r.bottom = rect.top + SYSMETRICS_CYSIZE;
            rect.top += SYSMETRICS_CYSIZE + SYSMETRICS_CYBORDER;
            NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle, active );
        }
    }

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + SYSMETRICS_CYMENU;  /* default height */
	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint );
    }

      /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL) SCROLL_DrawScrollBar(hwnd, hdc, SB_VERT);
    if (wndPtr->dwStyle & WS_HSCROLL) SCROLL_DrawScrollBar(hwnd, hdc, SB_HORZ);

      /* Draw the "size-box" */

    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - SYSMETRICS_CXVSCROLL + 1;
        r.top  = r.bottom - SYSMETRICS_CYHSCROLL + 1;
        FillRect( hdc, &r, sysColorObjects.hbrushScrollbar );
    }    

    ReleaseDC( hwnd, hdc );
}



/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    NC_DoNCPaint( hwnd, wndPtr->flags & WIN_NCACTIVATED, FALSE );
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LONG NC_HandleNCActivate( HWND hwnd, WPARAM wParam )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    if (wParam != 0) wndPtr->flags |= WIN_NCACTIVATED;
    else wndPtr->flags &= ~WIN_NCACTIVATED;

    NC_DoNCPaint( hwnd, (wParam != 0), FALSE );
    return TRUE;
}


/***********************************************************************
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LONG NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if (hwnd != (HWND)wParam) return 0;  /* Don't set the cursor for child windows */

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
		SetCursor( classPtr->wc.hCursor );
		return TRUE;
	    }
	    else return FALSE;
	}

    case HTLEFT:
    case HTRIGHT:
	return (LONG)SetCursor( LoadCursor( 0, IDC_SIZEWE ) );

    case HTTOP:
    case HTBOTTOM:
	return (LONG)SetCursor( LoadCursor( 0, IDC_SIZENS ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	return (LONG)SetCursor( LoadCursor( 0, IDC_SIZENWSE ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	return (LONG)SetCursor( LoadCursor( 0, IDC_SIZENESW ) );
    }

    /* Default cursor: arrow */
    return (LONG)SetCursor( LoadCursor( 0, IDC_ARROW ) );
}


/***********************************************************************
 *           NC_TrackSysMenu
 *
 * Track a mouse button press on the system menu.
 */
static void NC_TrackSysMenu( HWND hwnd, HDC hdc, POINT pt )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    int iconic = wndPtr->dwStyle & WS_MINIMIZE;

    if (!(wndPtr->dwStyle & WS_SYSMENU)) return;
    /* If window has a menu, track the menu bar normally if it not minimized */
    if (HAS_MENU(wndPtr) && !iconic) MENU_TrackMouseMenuBar( hwnd, pt );
    else
    {
	  /* Otherwise track the system menu like a normal popup menu */
	NC_GetInsideRect( hwnd, &rect );
	OffsetRect( &rect, wndPtr->rectWindow.left, wndPtr->rectWindow.top );
	if (wndPtr->dwStyle & WS_CHILD)
	    ClientToScreen( wndPtr->hwndParent, (POINT *)&rect );
	rect.right = rect.left + SYSMETRICS_CXSIZE;
	rect.bottom = rect.top + SYSMETRICS_CYSIZE;
	if (!iconic) NC_DrawSysButton( hwnd, hdc, TRUE );
	TrackPopupMenu( wndPtr->hSysMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
		        rect.left, rect.bottom, 0, hwnd, &rect );
	if (!iconic) NC_DrawSysButton( hwnd, hdc, FALSE );
    }
}


/***********************************************************************
 *           NC_StartSizeMove
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG NC_StartSizeMove( HWND hwnd, WPARAM wParam, POINT *capturePoint )
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
		hittest = NC_HandleNCHitTest( hwnd, msg.pt );
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
    NC_HandleSetCursor( hwnd, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
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
    int moved = 0;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd) ||
        (wndPtr->flags & WIN_MANAGED)) return;
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

    NC_GetMinMaxInfo( hwnd, NULL, NULL, &minTrack, &maxTrack );
    sizingRect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	GetClientRect( wndPtr->hwndParent, &mouseRect );
    else SetRect( &mouseRect, 0, 0, SYSMETRICS_CXSCREEN, SYSMETRICS_CYSCREEN );
    if (ON_LEFT_BORDER(hittest))
    {
	mouseRect.left  = MAX( mouseRect.left, sizingRect.right-maxTrack.x );
	mouseRect.right = MIN( mouseRect.right, sizingRect.right-minTrack.x );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
	mouseRect.left  = MAX( mouseRect.left, sizingRect.left+minTrack.x );
	mouseRect.right = MIN( mouseRect.right, sizingRect.left+maxTrack.x );
    }
    if (ON_TOP_BORDER(hittest))
    {
	mouseRect.top    = MAX( mouseRect.top, sizingRect.bottom-maxTrack.y );
	mouseRect.bottom = MIN( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
	mouseRect.top    = MAX( mouseRect.top, sizingRect.top+minTrack.y );
	mouseRect.bottom = MIN( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
    SendMessage( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (GetCapture() != hwnd) SetCapture( hwnd );    

    if (wndPtr->dwStyle & WS_CHILD)
    {
          /* Retrieve a default cache DC (without using the window style) */
        hdc = GetDCEx( wndPtr->hwndParent, 0, DCX_CACHE );
    }
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

	pt.x = MAX( pt.x, mouseRect.left );
	pt.x = MIN( pt.x, mouseRect.right );
	pt.y = MAX( pt.y, mouseRect.top );
	pt.y = MIN( pt.y, mouseRect.bottom );

	dx = pt.x - capturePoint.x;
	dy = pt.y - capturePoint.y;

	if (dx || dy)
	{
            moved = 1;
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
    SendMessage( hwnd, WM_SETVISIBLE, !IsIconic(hwnd), 0L);

    /* Single click brings up the system menu when iconized */

    if (!moved && (wndPtr->dwStyle & WS_MINIMIZE))
    {
        NC_TrackSysMenu( hwnd, hdc, pt );
        return;
    }

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

	pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
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
    MSG msg;
    WORD scrollbar;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

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

    pt.x -= wndPtr->rectWindow.left;
    pt.y -= wndPtr->rectWindow.top;
    SetCapture( hwnd );
    SCROLL_HandleScrollEvent( hwnd, scrollbar, WM_LBUTTONDOWN, pt );

    do
    {
        GetMessage( MAKE_SEGPTR(&msg), 0, 0, 0 );
	switch(msg.message)
	{
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
        case WM_SYSTIMER:
            pt.x = LOWORD(msg.lParam) + wndPtr->rectClient.left - 
	      wndPtr->rectWindow.left;
            pt.y = HIWORD(msg.lParam) + wndPtr->rectClient.top - 
	      wndPtr->rectWindow.top;
            SCROLL_HandleScrollEvent( hwnd, scrollbar, msg.message, pt );
	    break;
        default:
            TranslateMessage( &msg );
            DispatchMessage( &msg );
            break;
	}
        if (!IsWindow( hwnd ))
        {
            ReleaseCapture();
            break;
        }
    } while (msg.message != WM_LBUTTONUP);
}

/***********************************************************************
 *           NC_HandleNCLButtonDown
 *
 * Handle a WM_NCLBUTTONDOWN message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    HDC hdc = GetWindowDC( hwnd );
    POINT pt = { LOWORD(lParam), HIWORD(lParam) };

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	SendMessage( hwnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam );
	break;

    case HTSYSMENU:
	NC_TrackSysMenu( hwnd, hdc, pt );
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
LONG NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    /*
     * if this is an icon, send a restore since we are handling
     * a double click
     */
    if (IsIconic(hwnd))
    {
      SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, lParam);
      return 0;
    } 

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */

        if( GetWindowLong( hwnd , GWL_STYLE) & WS_MAXIMIZEBOX )
            SendMessage( hwnd, WM_SYSCOMMAND,
                         IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, lParam );
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
LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    dprintf_nonclient(stddeb, "Handling WM_SYSCOMMAND %lx %ld,%ld\n", 
		      (DWORD)wParam, (LONG)pt.x, (LONG)pt.y );

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
	NC_TrackScrollBar( hwnd, wParam, pt );
	break;

    case SC_MOUSEMENU:
	MENU_TrackMouseMenuBar( hwnd, pt );
	break;

    case SC_KEYMENU:
	MENU_TrackKbdMenuBar( hwnd, wParam );
	break;
	
    case SC_ARRANGE:
	break;

    case SC_TASKLIST:
	WinExec( "taskman.exe", SW_SHOWNORMAL ); 
	break;

    case SC_HOTKEY:
	break;

    case SC_SCREENSAVE:
	if (wParam == SC_ABOUTWINE)
	{   
	  extern const char people[];
	  ShellAbout(hwnd,"WINE",people,0);
        }
	break;
    }
    return 0;
}

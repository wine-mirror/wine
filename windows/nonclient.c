/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include "win.h"
#include "message.h"
#include "sysmetrics.h"
#include "user.h"
#include "heap.h"
#include "shell.h"
#include "dialog.h"
#include "syscolor.h"
#include "menu.h"
#include "winpos.h"
#include "hook.h"
#include "scroll.h"
#include "nonclient.h"
#include "graphics.h"
#include "queue.h"
#include "selectors.h"
#include "stddebug.h"
#include "debug.h"
#include "options.h"


static HBITMAP16 hbitmapClose = 0;
static HBITMAP16 hbitmapMinimize = 0;
static HBITMAP16 hbitmapMinimizeD = 0;
static HBITMAP16 hbitmapMaximize = 0;
static HBITMAP16 hbitmapMaximizeD = 0;
static HBITMAP16 hbitmapRestore = 0;
static HBITMAP16 hbitmapRestoreD = 0;

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

extern HCURSOR16 CURSORICON_IconToCursor( HICON16, BOOL32 );

/***********************************************************************
 *           NC_AdjustRect
 *
 * Compute the size of the window rectangle from the size of the
 * client rectangle.
 */
static void NC_AdjustRect( LPRECT16 rect, DWORD style, BOOL32 menu,
                           DWORD exStyle )
{
    if(style & WS_ICONIC) return;
    /* Decide if the window will be managed (see CreateWindowEx) */
    if (!(Options.managed && !(style & WS_CHILD) &&
          ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
           (exStyle & WS_EX_DLGMODALFRAME))))
    {
        if (HAS_DLGFRAME( style, exStyle ))
            InflateRect16(rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
        else
        {
            if (HAS_THICKFRAME(style))
                InflateRect16( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );
            if (style & WS_BORDER)
                InflateRect16( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER);
        }

        if ((style & WS_CAPTION) == WS_CAPTION)
            rect->top -= SYSMETRICS_CYCAPTION - SYSMETRICS_CYBORDER;
    }
    if (menu) rect->top -= SYSMETRICS_CYMENU + SYSMETRICS_CYBORDER;

    if (style & WS_VSCROLL) rect->right  += SYSMETRICS_CXVSCROLL;
    if (style & WS_HSCROLL) rect->bottom += SYSMETRICS_CYHSCROLL;
}


/***********************************************************************
 *           AdjustWindowRect16    (USER.102)
 */
BOOL16 AdjustWindowRect16( LPRECT16 rect, DWORD style, BOOL16 menu )
{
    return AdjustWindowRectEx16( rect, style, menu, 0 );
}


/***********************************************************************
 *           AdjustWindowRect32    (USER32.)
 */
BOOL32 AdjustWindowRect32( LPRECT32 rect, DWORD style, BOOL32 menu )
{
    return AdjustWindowRectEx32( rect, style, menu, 0 );
}


/***********************************************************************
 *           AdjustWindowRectEx16    (USER.454)
 */
BOOL16 AdjustWindowRectEx16( LPRECT16 rect, DWORD style,
                             BOOL16 menu, DWORD exStyle )
{
      /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	style |= WS_CAPTION;
    style &= (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME | WS_CHILD);
    exStyle &= WS_EX_DLGMODALFRAME;
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

    dprintf_nonclient(stddeb, "AdjustWindowRectEx: (%d,%d)-(%d,%d) %08lx %d %08lx\n",
                      rect->left, rect->top, rect->right, rect->bottom,
                      style, menu, exStyle );

    NC_AdjustRect( rect, style, menu, exStyle );
    return TRUE;
}


/***********************************************************************
 *           AdjustWindowRectEx32    (USER32.)
 */
BOOL32 AdjustWindowRectEx32( LPRECT32 rect, DWORD style,
                             BOOL32 menu, DWORD exStyle )
{
    RECT16 rect16;
    BOOL32 ret;

    CONV_RECT32TO16( rect, &rect16 );
    ret = AdjustWindowRectEx16( &rect16, style, (BOOL16)menu, exStyle );
    CONV_RECT16TO32( &rect16, rect );
    return ret;
}


/*******************************************************************
 *         NC_GetMinMaxInfo
 *
 * Get the minimized and maximized information for a window.
 */
void NC_GetMinMaxInfo( WND *wndPtr, POINT16 *maxSize, POINT16 *maxPos,
                       POINT16 *minTrack, POINT16 *maxTrack )
{
    MINMAXINFO16 *MinMax;
    short xinc, yinc;

    if (!(MinMax = SEGPTR_NEW(MINMAXINFO16))) return;

      /* Compute default values */

    MinMax->ptMaxSize.x = SYSMETRICS_CXSCREEN;
    MinMax->ptMaxSize.y = SYSMETRICS_CYSCREEN;
    MinMax->ptMinTrackSize.x = SYSMETRICS_CXMINTRACK;
    MinMax->ptMinTrackSize.y = SYSMETRICS_CYMINTRACK;
    MinMax->ptMaxTrackSize.x = SYSMETRICS_CXSCREEN;
    MinMax->ptMaxTrackSize.y = SYSMETRICS_CYSCREEN;

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
    MinMax->ptMaxSize.x += 2 * xinc;
    MinMax->ptMaxSize.y += 2 * yinc;

    /* Note: The '+' in the following test should really be a ||, but
     * that would cause gcc-2.7.0 to generate incorrect code.
     */
    if ((wndPtr->ptMaxPos.x != -1) + (wndPtr->ptMaxPos.y != -1))
        MinMax->ptMaxPosition = wndPtr->ptMaxPos;
    else
    {
        MinMax->ptMaxPosition.x = -xinc;
        MinMax->ptMaxPosition.y = -yinc;
    }

    SendMessage16( wndPtr->hwndSelf, WM_GETMINMAXINFO, 0,
                   (LPARAM)SEGPTR_GET(MinMax) );

      /* Some sanity checks */

    dprintf_nonclient(stddeb, 
		      "NC_GetMinMaxInfo: %d %d / %d %d / %d %d / %d %d\n",
		      MinMax->ptMaxSize.x, MinMax->ptMaxSize.y,
                      MinMax->ptMaxPosition.x, MinMax->ptMaxPosition.y,
		      MinMax->ptMaxTrackSize.x, MinMax->ptMaxTrackSize.y,
		      MinMax->ptMinTrackSize.x, MinMax->ptMinTrackSize.y);
    MinMax->ptMaxTrackSize.x = MAX( MinMax->ptMaxTrackSize.x,
				   MinMax->ptMinTrackSize.x );
    MinMax->ptMaxTrackSize.y = MAX( MinMax->ptMaxTrackSize.y,
				   MinMax->ptMinTrackSize.y );
    
    if (maxSize) *maxSize = MinMax->ptMaxSize;
    if (maxPos) *maxPos = MinMax->ptMaxPosition;
    if (minTrack) *minTrack = MinMax->ptMinTrackSize;
    if (maxTrack) *maxTrack = MinMax->ptMaxTrackSize;
    SEGPTR_FREE(MinMax);
}


/***********************************************************************
 *           NC_HandleNCCalcSize
 *
 * Handle a WM_NCCALCSIZE message. Called from DefWindowProc().
 */
LONG NC_HandleNCCalcSize( WND *pWnd, RECT16 *winRect )
{
    RECT16 tmpRect = { 0, 0, 0, 0 };
    LONG result = 0;

    if (pWnd->class->style & CS_VREDRAW) result |= WVR_VREDRAW;
    if (pWnd->class->style & CS_HREDRAW) result |= WVR_HREDRAW;

    if( !( pWnd->dwStyle & WS_MINIMIZE ) )
    {
      NC_AdjustRect( &tmpRect, pWnd->dwStyle, FALSE, pWnd->dwExStyle );
      winRect->left   -= tmpRect.left;
      winRect->top    -= tmpRect.top;
      winRect->right  -= tmpRect.right;
      winRect->bottom -= tmpRect.bottom;

      if (HAS_MENU(pWnd))
      {
	winRect->top += MENU_GetMenuBarHeight( pWnd->hwndSelf,
                                               winRect->right - winRect->left,
                                              -tmpRect.left, -tmpRect.top ) + 1;
      }
    }
    return result;
}


/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */
static void NC_GetInsideRect( HWND32 hwnd, RECT32 *rect )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    if ((wndPtr->dwStyle & WS_ICONIC) || (wndPtr->flags & WIN_MANAGED)) return;

      /* Remove frame from rectangle */
    if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
	InflateRect32( rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
	if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
            InflateRect32( rect, -1, 0 );
    }
    else
    {
	if (HAS_THICKFRAME( wndPtr->dwStyle ))
	    InflateRect32( rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
	if (wndPtr->dwStyle & WS_BORDER)
	    InflateRect32( rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER );
    }
}


/***********************************************************************
 *           NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG NC_HandleNCHitTest( HWND32 hwnd, POINT16 pt )
{
    RECT16 rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return HTERROR;

    dprintf_nonclient(stddeb, "NC_HandleNCHitTest: hwnd=%04x pt=%d,%d\n",
		      hwnd, pt.x, pt.y );

    GetWindowRect16( hwnd, &rect );
    if (!PtInRect16( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        /* Check borders */
        if (HAS_THICKFRAME( wndPtr->dwStyle ))
        {
            InflateRect16( &rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
            if (wndPtr->dwStyle & WS_BORDER)
                InflateRect16(&rect,-SYSMETRICS_CXBORDER,-SYSMETRICS_CYBORDER);
            if (!PtInRect16( &rect, pt ))
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
                InflateRect16(&rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
            else if (wndPtr->dwStyle & WS_BORDER)
                InflateRect16(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
            if (!PtInRect16( &rect, pt )) return HTBORDER;
        }

        /* Check caption */

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            rect.top += SYSMETRICS_CYCAPTION - 1;
            if (!PtInRect16( &rect, pt ))
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

    ScreenToClient16( hwnd, &pt );
    GetClientRect16( hwnd, &rect );
    if (PtInRect16( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += SYSMETRICS_CXVSCROLL;
	if (PtInRect16( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += SYSMETRICS_CYHSCROLL;
	if (PtInRect16( &rect, pt ))
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
void NC_DrawSysButton( HWND32 hwnd, HDC32 hdc, BOOL32 down )
{
    RECT32 rect;
    HDC32 hdcMem;
    HBITMAP32 hbitmap;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC32( hdc );
      hbitmap = SelectObject32( hdcMem, hbitmapClose );
      BitBlt32(hdc, rect.left, rect.top, SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
               hdcMem, (wndPtr->dwStyle & WS_CHILD) ? SYSMETRICS_CXSIZE : 0, 0,
               down ? NOTSRCCOPY : SRCCOPY );
      SelectObject32( hdcMem, hbitmap );
      DeleteDC32( hdcMem );
    }
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND32 hwnd, HDC16 hdc, BOOL32 down )
{
    RECT32 rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      GRAPH_DrawBitmap( hdc, (IsZoomed32(hwnd) ?
			     (down ? hbitmapRestoreD : hbitmapRestore) :
			     (down ? hbitmapMaximizeD : hbitmapMaximize)),
		        rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		        0, 0, SYSMETRICS_CXSIZE+1, SYSMETRICS_CYSIZE );
    }
}


/***********************************************************************
 *           NC_DrawMinButton
 */
static void NC_DrawMinButton( HWND32 hwnd, HDC16 hdc, BOOL32 down )
{
    RECT32 rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      if (wndPtr->dwStyle & WS_MAXIMIZEBOX) rect.right -= SYSMETRICS_CXSIZE+1;
      GRAPH_DrawBitmap( hdc, (down ? hbitmapMinimizeD : hbitmapMinimize),
		        rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		        0, 0, SYSMETRICS_CXSIZE+1, SYSMETRICS_CYSIZE );
    }
}


/***********************************************************************
 *           NC_DrawFrame
 *
 * Draw a window frame inside the given rectangle, and update the rectangle.
 * The correct pen for the frame must be selected in the DC.
 */
static void NC_DrawFrame( HDC32 hdc, RECT32 *rect, BOOL32 dlgFrame,
                          BOOL32 active )
{
    INT32 width, height;

    if (dlgFrame)
    {
	width = SYSMETRICS_CXDLGFRAME - 1;
	height = SYSMETRICS_CYDLGFRAME - 1;
        SelectObject32( hdc, active ? sysColorObjects.hbrushActiveCaption :
                                      sysColorObjects.hbrushInactiveCaption );
    }
    else
    {
	width = SYSMETRICS_CXFRAME - 1;
	height = SYSMETRICS_CYFRAME - 1;
        SelectObject32( hdc, active ? sysColorObjects.hbrushActiveBorder :
                                      sysColorObjects.hbrushInactiveBorder );
    }

      /* Draw frame */
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, height, PATCOPY );
    PatBlt32( hdc, rect->left, rect->top,
              width, rect->bottom - rect->top, PATCOPY );
    PatBlt32( hdc, rect->left, rect->bottom,
              rect->right - rect->left, -height, PATCOPY );
    PatBlt32( hdc, rect->right, rect->top,
              -width, rect->bottom - rect->top, PATCOPY );

    if (dlgFrame)
    {
	InflateRect32( rect, -width, -height );
    } 
    else
    {
	POINT32 lpt[16];
    
      /* Draw inner rectangle */

	GRAPH_DrawRectangle( hdc, rect->left + width,
                                  rect->top + height,
                                  rect->right - rect->left - 2*width ,
                                  rect->bottom - rect->top - 2*height,
                                  (HPEN32)0 );

      /* Draw the decorations */

	lpt[4].x = lpt[0].x = rect->left;
	lpt[5].x = lpt[1].x = rect->left + width;
	lpt[6].x = lpt[2].x = rect->right - width - 1;
	lpt[7].x = lpt[3].x = rect->right - 1;

	lpt[0].y = lpt[1].y = lpt[2].y = lpt[3].y = 
		  rect->top + SYSMETRICS_CYFRAME + SYSMETRICS_CYSIZE;
	lpt[4].y = lpt[5].y = lpt[6].y = lpt[7].y =
		  rect->bottom - 1 - SYSMETRICS_CYFRAME - SYSMETRICS_CYSIZE;

        lpt[8].x = lpt[9].x = lpt[10].x = lpt[11].x =
		  rect->left + SYSMETRICS_CXFRAME + SYSMETRICS_CXSIZE;
	lpt[12].x = lpt[13].x = lpt[14].x = lpt[15].x = 
		  rect->right - 1 - SYSMETRICS_CXFRAME - SYSMETRICS_CYSIZE;

	lpt[12].y = lpt[8].y = rect->top; 
	lpt[13].y = lpt[9].y = rect->top + height;
	lpt[14].y = lpt[10].y = rect->bottom - height - 1; 
	lpt[15].y = lpt[11].y = rect->bottom - 1;

	GRAPH_DrawLines( hdc, lpt, 8, (HPEN32)0 );	/* 8 is the maximum */
	InflateRect32( rect, -width - 1, -height - 1 );
    }
}


/***********************************************************************
 *           NC_DrawMovingFrame
 *
 * Draw the frame used when moving or resizing window.
 */
static void NC_DrawMovingFrame( HDC16 hdc, RECT16 *rect, BOOL32 thickframe )
{
    if (thickframe) FastWindowFrame( hdc, rect, SYSMETRICS_CXFRAME,
                                     SYSMETRICS_CYFRAME, PATINVERT );
    else DrawFocusRect16( hdc, rect );
}


/***********************************************************************
 *           NC_DrawCaption
 *
 * Draw the window caption.
 * The correct pen for the window frame must be selected in the DC.
 */
static void NC_DrawCaption( HDC32 hdc, RECT32 *rect, HWND32 hwnd,
			    DWORD style, BOOL32 active )
{
    RECT32 r = *rect;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    char buffer[256];

    if (wndPtr->flags & WIN_MANAGED) return;

    if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_CLOSE) )))
	    return;
	hbitmapMinimize  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_RESTORED) );
    }
    
    if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
    {
	HBRUSH32 hbrushOld = SelectObject32(hdc, sysColorObjects.hbrushWindow);
	PatBlt32( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
	PatBlt32( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	PatBlt32( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
	r.left++;
	r.right--;
	SelectObject32( hdc, hbrushOld );
    }

    MoveTo( hdc, r.left, r.bottom );
    LineTo32( hdc, r.right-1, r.bottom );

    if (style & WS_SYSMENU)
    {
	NC_DrawSysButton( hwnd, hdc, FALSE );
	r.left += SYSMETRICS_CXSIZE + 1;
	MoveTo( hdc, r.left - 1, r.top );
	LineTo32( hdc, r.left - 1, r.bottom );
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

    FillRect32( hdc, &r, active ? sysColorObjects.hbrushActiveCaption : 
	                          sysColorObjects.hbrushInactiveCaption );

    if (GetWindowText32A( hwnd, buffer, sizeof(buffer) ))
    {
	if (active) SetTextColor32( hdc, GetSysColor32( COLOR_CAPTIONTEXT ) );
	else SetTextColor32( hdc, GetSysColor32( COLOR_INACTIVECAPTIONTEXT ) );
	SetBkMode32( hdc, TRANSPARENT );
	DrawText32A( hdc, buffer, -1, &r,
                     DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX );
    }
}


/***********************************************************************
 *           NC_DoNCPaint
 *
 * Paint the non-client area. clip is currently unused.
 */
void NC_DoNCPaint( HWND32 hwnd, HRGN32 clip, BOOL32 suppress_menupaint )
{
    HDC32 hdc;
    RECT32 rect;
    BOOL32 active;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr || wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return; /* Nothing to do */

    active  = wndPtr->flags & WIN_NCACTIVATED;

    dprintf_nonclient(stddeb, "NC_DoNCPaint: %04x %d\n", hwnd, active );

    if (!(hdc = GetDCEx32( hwnd, 0, DCX_USESTYLE | DCX_WINDOW ))) return;

    if (ExcludeVisRect( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC32( hwnd, hdc );
	return;
    }

    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    SelectObject32( hdc, sysColorObjects.hpenWindowFrame );

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        if ((wndPtr->dwStyle & WS_BORDER) || (wndPtr->dwStyle & WS_DLGFRAME) ||
            (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
        {
            GRAPH_DrawRectangle( hdc, 0, 0,
                                 rect.right, rect.bottom, (HPEN32)0 );
            InflateRect32( &rect, -1, -1 );
        }

        if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
            NC_DrawFrame( hdc, &rect, TRUE, active );
        else if (wndPtr->dwStyle & WS_THICKFRAME)
            NC_DrawFrame(hdc, &rect, FALSE, active );

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            RECT32 r = rect;
            r.bottom = rect.top + SYSMETRICS_CYSIZE;
            rect.top += SYSMETRICS_CYSIZE + SYSMETRICS_CYBORDER;
            NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle, active );
        }
    }

    if (HAS_MENU(wndPtr))
    {
	RECT32 r = rect;
	r.bottom = rect.top + SYSMETRICS_CYMENU;  /* default height */
	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint );
    }

      /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE );
    if (wndPtr->dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE );

      /* Draw the "size-box" */

    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT32 r = rect;
        r.left = r.right - SYSMETRICS_CXVSCROLL + 1;
        r.top  = r.bottom - SYSMETRICS_CYHSCROLL + 1;
        FillRect32( hdc, &r, sysColorObjects.hbrushScrollbar );
    }    

    ReleaseDC32( hwnd, hdc );
}



/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND32 hwnd , HRGN32 clip)
{
    NC_DoNCPaint( hwnd, clip, FALSE );
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LONG NC_HandleNCActivate( WND *wndPtr, WPARAM16 wParam )
{
    WORD wStateChange;

    if( wParam ) wStateChange = !(wndPtr->flags & WIN_NCACTIVATED);
    else wStateChange = wndPtr->flags & WIN_NCACTIVATED;

    if( wStateChange )
    {
      if (wParam) wndPtr->flags |= WIN_NCACTIVATED;
      else wndPtr->flags &= ~WIN_NCACTIVATED;

      if( wndPtr->dwStyle & WS_MINIMIZE )
	PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW, 0 );
      else
	NC_DoNCPaint( wndPtr->hwndSelf, (HRGN32)1, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LONG NC_HandleSetCursor( HWND32 hwnd, WPARAM16 wParam, LPARAM lParam )
{
    if (hwnd != (HWND32)wParam) return 0;  /* Don't set the cursor for child windows */

    switch(LOWORD(lParam))
    {
    case HTERROR:
	{
	    WORD msg = HIWORD( lParam );
	    if ((msg == WM_LBUTTONDOWN) || (msg == WM_MBUTTONDOWN) ||
		(msg == WM_RBUTTONDOWN))
		MessageBeep32(0);
	}
	break;

    case HTCLIENT:
	{
	    WND *wndPtr;
	    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
	    if (wndPtr->class->hCursor)
	    {
		SetCursor16( wndPtr->class->hCursor );
		return TRUE;
	    }
	    else return FALSE;
	}

    case HTLEFT:
    case HTRIGHT:
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZEWE ) );

    case HTTOP:
    case HTBOTTOM:
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZENS ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZENWSE ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZENESW ) );
    }

    /* Default cursor: arrow */
    return (LONG)SetCursor16( LoadCursor16( 0, IDC_ARROW ) );
}

/***********************************************************************
 *           NC_GetSysPopupPos
 */
BOOL32 NC_GetSysPopupPos( WND* wndPtr, RECT32* rect )
{
  if( wndPtr->hSysMenu )
  {
      if( wndPtr->dwStyle & WS_MINIMIZE )
	  GetWindowRect32( wndPtr->hwndSelf, rect );
      else
      {
  	  NC_GetInsideRect( wndPtr->hwndSelf, rect );
  	  OffsetRect32( rect, wndPtr->rectWindow.left, wndPtr->rectWindow.top);
  	  if (wndPtr->dwStyle & WS_CHILD)
     	      ClientToScreen32( wndPtr->parent->hwndSelf, (POINT32 *)rect );
          rect->right = rect->left + SYSMETRICS_CXSIZE;
          rect->bottom = rect->top + SYSMETRICS_CYSIZE;
      }
      return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *           NC_TrackSysMenu
 *
 * Track a mouse button press on the system menu.
 */
static void NC_TrackSysMenu( HWND32 hwnd, POINT16 pt )
{
    WND*	wndPtr = WIN_FindWndPtr( hwnd );
    
    if (wndPtr->dwStyle & WS_SYSMENU)
    {
	int	iconic, on = 1;

	iconic = wndPtr->dwStyle & WS_MINIMIZE;

	if( !iconic ) 
	{
	     HDC16	hdc = GetWindowDC32(hwnd);
	     RECT32	rect;
             RECT16     rTrack;
	     BOOL32 	bNew, bTrack = TRUE;
	     MSG16	msg;
	    
	     NC_GetSysPopupPos( wndPtr, &rect );
             CONV_RECT32TO16( &rect, &rTrack );
	     MapWindowPoints16( 0, hwnd, (LPPOINT16)&rTrack, 2 );

	     /* track mouse while waiting for WM_LBUTTONUP */

	     NC_DrawSysButton( hwnd, hdc, bTrack );
	     SetCapture32(hwnd);
	     do
	     {
		msg.message = WM_NULL;
		PeekMessage16( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
		if( msg.message == WM_MOUSEMOVE )
		{
		    if( (bNew = PtInRect16(&rTrack, MAKEPOINT16(msg.lParam))) )
		    {   if( bTrack ) continue; }
		    else 
		    {   if(!bTrack ) continue; }
		    NC_DrawSysButton( hwnd, hdc, bTrack = bNew);
		}
	     } while( msg.message != WM_LBUTTONUP );

	     ReleaseCapture();
	     ReleaseDC32(hwnd, hdc);
	     on = PtInRect16(&rTrack, MAKEPOINT16(msg.lParam));
	} 

	if( on ) 
	    SendMessage16( hwnd, WM_SYSCOMMAND, 
			   SC_MOUSEMENU + HTSYSMENU, *((LPARAM*)&pt));
    }
}


/***********************************************************************
 *           NC_StartSizeMove
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG NC_StartSizeMove( HWND32 hwnd, WPARAM16 wParam,
                              POINT16 *capturePoint )
{
    LONG hittest = 0;
    POINT16 pt;
    MSG16 msg;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	  /* Move pointer at the center of the caption */
	RECT32 rect;
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
	    ClientToScreen16( wndPtr->parent->hwndSelf, &pt );
	hittest = HTCAPTION;
    }
    else  /* SC_SIZE */
    {
	SetCapture32(hwnd);
	while(!hittest)
	{
            MSG_InternalGetMessage( &msg, 0, 0, MSGF_SIZE, PM_REMOVE, FALSE );
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
    SetCursorPos32( capturePoint->x, capturePoint->y );
    NC_HandleSetCursor( hwnd, (WPARAM16)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           NC_DoSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
static void NC_DoSizeMove( HWND32 hwnd, WORD wParam, POINT16 pt )
{
    MSG16 msg;
    RECT16 sizingRect, mouseRect;
    HDC32 hdc;
    LONG hittest = (LONG)(wParam & 0x0f);
    HCURSOR16 hDragCursor = 0, hOldCursor = 0;
    POINT16   minTrack, maxTrack, capturePoint = pt;
    WND *     wndPtr = WIN_FindWndPtr( hwnd );
    BOOL32    thickframe = HAS_THICKFRAME( wndPtr->dwStyle );
    BOOL32    iconic = wndPtr->dwStyle & WS_MINIMIZE;
    int       moved = 0;

    if (IsZoomed32(hwnd) || !IsWindowVisible32(hwnd) ||
        (wndPtr->flags & WIN_MANAGED)) return;

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
	    SetCapture32(hwnd);
	    hittest = NC_StartSizeMove( hwnd, wParam, &capturePoint );
	    if (!hittest)
	    {
		ReleaseCapture();
		return;
	    }
	}
    }

      /* Get min/max info */

    NC_GetMinMaxInfo( wndPtr, NULL, NULL, &minTrack, &maxTrack );
    sizingRect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	GetClientRect16( wndPtr->parent->hwndSelf, &mouseRect );
    else SetRect16(&mouseRect, 0, 0, SYSMETRICS_CXSCREEN, SYSMETRICS_CYSCREEN);
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
    SendMessage16( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (GetCapture32() != hwnd) SetCapture32( hwnd );    

    if (wndPtr->dwStyle & WS_CHILD)
    {
          /* Retrieve a default cache DC (without using the window style) */
        hdc = GetDCEx32( wndPtr->parent->hwndSelf, 0, DCX_CACHE );
    }
    else
    {  /* Grab the server only when moving top-level windows without desktop */
	hdc = GetDC32( 0 );
	if (rootWindow == DefaultRootWindow(display)) XGrabServer( display );
    }

    if( iconic )
    {
      HICON16 hIcon = (wndPtr->class->hIcon)
                      ? wndPtr->class->hIcon
                      : (HICON16)SendMessage16( hwnd, WM_QUERYDRAGICON, 0, 0L);
      if( hIcon )
      {
        hDragCursor =  CURSORICON_IconToCursor( hIcon, TRUE );
        hOldCursor = SetCursor32(hDragCursor);
        ShowCursor32( TRUE );
      } else iconic = FALSE;
    }

    if( !iconic ) NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    while(1)
    {
        int dx = 0, dy = 0;

        MSG_InternalGetMessage( &msg, 0, 0, MSGF_SIZE, PM_REMOVE, FALSE );

	  /* Exit on button-up, Return, or Esc */
	if ((msg.message == WM_LBUTTONUP) ||
	    ((msg.message == WM_KEYDOWN) && 
	     ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

	if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
	    continue;  /* We are not interested in other messages */

	pt = msg.pt;
	if (wndPtr->dwStyle & WS_CHILD)
	    ScreenToClient16( wndPtr->parent->hwndSelf, &pt );
	
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
	    if (msg.message == WM_KEYDOWN) SetCursorPos32( pt.x, pt.y );
	    else
	    {
		RECT16 newRect = sizingRect;

		if (hittest == HTCAPTION) OffsetRect16( &newRect, dx, dy );
		if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
		else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
		if (ON_TOP_BORDER(hittest)) newRect.top += dy;
		else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
		if( !iconic )
		{
		  NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
		  NC_DrawMovingFrame( hdc, &newRect, thickframe );
		}
		capturePoint = pt;
		sizingRect = newRect;
	    }
	}
    }

    ReleaseCapture();
    if( iconic )
    {
      ShowCursor32( FALSE );
      SetCursor32(hOldCursor);
      if( hDragCursor ) DestroyCursor32( hDragCursor );
    }
    else
      NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    if (wndPtr->dwStyle & WS_CHILD)
        ReleaseDC32( wndPtr->parent->hwndSelf, hdc );
    else
    {
	ReleaseDC32( 0, hdc );
	if (rootWindow == DefaultRootWindow(display)) XUngrabServer( display );
    }

    if (HOOK_IsHooked( WH_CBT ))
    {
       RECT16* pr = SEGPTR_NEW(RECT16);
       if( pr )
       {
	 *pr = sizingRect;
	  if( HOOK_CallHooks16( WH_CBT, HCBT_MOVESIZE, hwnd,
			        (LPARAM)SEGPTR_GET(pr)) )
	      sizingRect = wndPtr->rectWindow;
	  else
	      sizingRect = *pr;
	  SEGPTR_FREE(pr);
       }
    }
    SendMessage16( hwnd, WM_EXITSIZEMOVE, 0, 0 );
    SendMessage16( hwnd, WM_SETVISIBLE, !IsIconic16(hwnd), 0L);

    /* Single click brings up the system menu when iconized */

    if (!moved && (wndPtr->dwStyle & WS_MINIMIZE))
    {
        NC_TrackSysMenu( hwnd, pt );
        return;
    }

      /* If Esc key, don't move the window */
    if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) return;

    if (hittest != HTCAPTION)
	SetWindowPos32( hwnd, 0, sizingRect.left, sizingRect.top,
		        sizingRect.right - sizingRect.left,
		        sizingRect.bottom - sizingRect.top,
		        SWP_NOACTIVATE | SWP_NOZORDER );
    else SetWindowPos32( hwnd, 0, sizingRect.left, sizingRect.top, 0, 0,
		         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
}


/***********************************************************************
 *           NC_TrackMinMaxBox
 *
 * Track a mouse button press on the minimize or maximize box.
 */
static void NC_TrackMinMaxBox( HWND32 hwnd, WORD wParam )
{
    MSG16 msg;
    HDC32 hdc = GetWindowDC32( hwnd );
    BOOL32 pressed = TRUE;

    SetCapture32( hwnd );
    if (wParam == HTMINBUTTON) NC_DrawMinButton( hwnd, hdc, TRUE );
    else NC_DrawMaxButton( hwnd, hdc, TRUE );

    do
    {
	BOOL32 oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, 0, PM_REMOVE, FALSE );

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
    ReleaseDC32( hwnd, hdc );
    if (!pressed) return;

    if (wParam == HTMINBUTTON) 
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, *(LONG*)&msg.pt );
    else
	SendMessage16( hwnd, WM_SYSCOMMAND, 
		  IsZoomed32(hwnd) ? SC_RESTORE:SC_MAXIMIZE, *(LONG*)&msg.pt );
}


/***********************************************************************
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND32 hwnd, WPARAM32 wParam, POINT32 pt )
{
    MSG16 *msg;
    INT32 scrollbar;
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

    if (!(msg = SEGPTR_NEW(MSG16))) return;
    pt.x -= wndPtr->rectWindow.left;
    pt.y -= wndPtr->rectWindow.top;
    SetCapture32( hwnd );
    SCROLL_HandleScrollEvent( hwnd, scrollbar, WM_LBUTTONDOWN, pt );

    do
    {
        GetMessage16( SEGPTR_GET(msg), 0, 0, 0 );
	switch(msg->message)
	{
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
        case WM_SYSTIMER:
            pt.x = LOWORD(msg->lParam) + wndPtr->rectClient.left - 
	      wndPtr->rectWindow.left;
            pt.y = HIWORD(msg->lParam) + wndPtr->rectClient.top - 
	      wndPtr->rectWindow.top;
            SCROLL_HandleScrollEvent( hwnd, scrollbar, msg->message, pt );
	    break;
        default:
            TranslateMessage16( msg );
            DispatchMessage16( msg );
            break;
	}
        if (!IsWindow32( hwnd ))
        {
            ReleaseCapture();
            break;
        }
    } while (msg->message != WM_LBUTTONUP);
    SEGPTR_FREE(msg);
}

/***********************************************************************
 *           NC_HandleNCLButtonDown
 *
 * Handle a WM_NCLBUTTONDOWN message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDown( HWND32 hwnd, WPARAM16 wParam, LPARAM lParam )
{
    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam );
	break;

    case HTSYSMENU:
        NC_TrackSysMenu( hwnd, MAKEPOINT16(lParam) );
	break;

    case HTMENU:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU, lParam );
	break;

    case HTHSCROLL:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
	break;

    case HTVSCROLL:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
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
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_SIZE + wParam - HTLEFT+1, lParam);
	break;

    case HTBORDER:
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCLButtonDblClk
 *
 * Handle a WM_NCLBUTTONDBLCLK message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM16 wParam, LPARAM lParam )
{
    /*
     * if this is an icon, send a restore since we are handling
     * a double click
     */
    if (pWnd->dwStyle & WS_MINIMIZE)
    {
        SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_RESTORE, lParam );
        return 0;
    } 

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */
        if (pWnd->dwStyle & WS_MAXIMIZEBOX)
            SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND,
                      (pWnd->dwStyle & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE,
                      lParam );
	break;

    case HTSYSMENU:
        if (!(pWnd->class->style & CS_NOCLOSE))
            SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_CLOSE, lParam );
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LONG NC_HandleSysCommand( HWND32 hwnd, WPARAM16 wParam, POINT16 pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    POINT32 pt32;

    dprintf_nonclient(stddeb, "Handling WM_SYSCOMMAND %x %d,%d\n", 
		      wParam, pt.x, pt.y );

    if (wndPtr->dwStyle & WS_CHILD && wParam != SC_KEYMENU )
        ScreenToClient16( wndPtr->parent->hwndSelf, &pt );

    switch (wParam & 0xfff0)
    {
    case SC_SIZE:
    case SC_MOVE:
	NC_DoSizeMove( hwnd, wParam, pt );
	break;

    case SC_MINIMIZE:
	ShowWindow32( hwnd, SW_MINIMIZE ); 
	break;

    case SC_MAXIMIZE:
	ShowWindow32( hwnd, SW_MAXIMIZE );
	break;

    case SC_RESTORE:
	ShowWindow32( hwnd, SW_RESTORE );
	break;

    case SC_CLOSE:
	return SendMessage16( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
        CONV_POINT16TO32( &pt, &pt32 );
	NC_TrackScrollBar( hwnd, wParam, pt32 );
	break;

    case SC_MOUSEMENU:
        CONV_POINT16TO32( &pt, &pt32 );
        MENU_TrackMouseMenuBar( wndPtr, wParam & 0x000F, pt32 );
	break;

    case SC_KEYMENU:
	MENU_TrackKbdMenuBar( wndPtr , wParam , pt.x );
	break;
	
    case SC_TASKLIST:
	WinExec32( "taskman.exe", SW_SHOWNORMAL ); 
	break;

    case SC_SCREENSAVE:
	if (wParam == SC_ABOUTWINE)
	{   
            extern const char people[];
            ShellAbout32A(hwnd,"Wine",people,0);
        }
	break;
  
    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
 	/* FIXME: unimplemented */
        break;
    }
    return 0;
}

/*		
 * Scrollbar control
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994, 1996 Alexandre Julliard
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "syscolor.h"
#include "sysmetrics.h"
#include "scroll.h"
#include "graphics.h"
#include "heap.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"


static HBITMAP32 hUpArrow = 0;
static HBITMAP32 hDnArrow = 0;
static HBITMAP32 hLfArrow = 0;
static HBITMAP32 hRgArrow = 0;
static HBITMAP32 hUpArrowD = 0;
static HBITMAP32 hDnArrowD = 0;
static HBITMAP32 hLfArrowD = 0;
static HBITMAP32 hRgArrowD = 0;
static HBITMAP32 hUpArrowI = 0;
static HBITMAP32 hDnArrowI = 0;
static HBITMAP32 hLfArrowI = 0;
static HBITMAP32 hRgArrowI = 0;

#define TOP_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_UP) ? hUpArrowI : ((pressed) ? hUpArrowD:hUpArrow))
#define BOTTOM_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_DOWN) ? hDnArrowI : ((pressed) ? hDnArrowD:hDnArrow))
#define LEFT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_LEFT) ? hLfArrowI : ((pressed) ? hLfArrowD:hLfArrow))
#define RIGHT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_RIGHT) ? hRgArrowI : ((pressed) ? hRgArrowD:hRgArrow))


  /* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4  

  /* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 6

  /* Delay (in ms) before first repetition when holding the button down */
#define SCROLL_FIRST_DELAY   200

  /* Delay (in ms) between scroll repetitions */
#define SCROLL_REPEAT_DELAY  50

  /* Scroll timer id */
#define SCROLL_TIMER   0

  /* Scroll-bar hit testing */
enum SCROLL_HITTEST
{
    SCROLL_NOWHERE,      /* Outside the scroll bar */
    SCROLL_TOP_ARROW,    /* Top or left arrow */
    SCROLL_TOP_RECT,     /* Rectangle between the top arrow and the thumb */
    SCROLL_THUMB,        /* Thumb rectangle */
    SCROLL_BOTTOM_RECT,  /* Rectangle between the thumb and the bottom arrow */
    SCROLL_BOTTOM_ARROW  /* Bottom or right arrow */
};

 /* Thumb-tracking info */
static HWND32 SCROLL_TrackingWin = 0;
static INT32  SCROLL_TrackingBar = 0;
static INT32  SCROLL_TrackingPos = 0;
static INT32  SCROLL_TrackingVal = 0;

/* Is the moving thumb being displayed? */
static BOOL32 SCROLL_MovingThumb = FALSE;

/***********************************************************************
 *           SCROLL_LoadBitmaps
 */
static void SCROLL_LoadBitmaps(void)
{
    hUpArrow  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_UPARROW) );
    hDnArrow  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_DNARROW) );
    hLfArrow  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_LFARROW) );
    hRgArrow  = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_RGARROW) );
    hUpArrowD = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_UPARROWD) );
    hDnArrowD = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_DNARROWD) );
    hLfArrowD = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_LFARROWD) );
    hRgArrowD = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_RGARROWD) );
    hUpArrowI = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_UPARROWI) );
    hDnArrowI = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_DNARROWI) );
    hLfArrowI = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_LFARROWI) );
    hRgArrowI = LoadBitmap16( 0, MAKEINTRESOURCE(OBM_RGARROWI) );
}


/***********************************************************************
 *           SCROLL_GetPtrScrollInfo
 */
static SCROLLBAR_INFO *SCROLL_GetPtrScrollInfo( WND* wndPtr, INT32 nBar )
{
    SCROLLBAR_INFO *infoPtr;

    if (!wndPtr) return NULL;
    switch(nBar)
    {
        case SB_HORZ: infoPtr = (SCROLLBAR_INFO *)wndPtr->pHScroll; break;
        case SB_VERT: infoPtr = (SCROLLBAR_INFO *)wndPtr->pVScroll; break;
        case SB_CTL:  infoPtr = (SCROLLBAR_INFO *)wndPtr->wExtra; break;
        default:      return NULL;
    }

    if (!infoPtr)  /* Create the info structure if needed */
    {
        if ((infoPtr = HeapAlloc( SystemHeap, 0, sizeof(SCROLLBAR_INFO) )))
        {
            infoPtr->MinVal = infoPtr->CurVal = infoPtr->Page = 0;
            infoPtr->MaxVal = 100;
            infoPtr->flags  = ESB_ENABLE_BOTH;
            if (nBar == SB_HORZ) wndPtr->pHScroll = infoPtr;
            else wndPtr->pVScroll = infoPtr;
        }
        if (!hUpArrow) SCROLL_LoadBitmaps();
    }
    return infoPtr;
}


/***********************************************************************
 *           SCROLL_GetScrollInfo
 */
static SCROLLBAR_INFO *SCROLL_GetScrollInfo( HWND32 hwnd, INT32 nBar )
{
   WND *wndPtr = WIN_FindWndPtr( hwnd );
   return SCROLL_GetPtrScrollInfo( wndPtr, nBar );
}


/***********************************************************************
 *           SCROLL_GetScrollBarRect
 *
 * Compute the scroll bar rectangle, in drawing coordinates (i.e. client
 * coords for SB_CTL, window coords for SB_VERT and SB_HORZ).
 * 'arrowSize' returns the width or height of an arrow (depending on
 * the orientation of the scrollbar), 'thumbSize' returns the size of
 * the thumb, and 'thumbPos' returns the position of the thumb
 * relative to the left or to the top.
 * Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL32 SCROLL_GetScrollBarRect( HWND32 hwnd, INT32 nBar, RECT32 *lprect,
                                       INT32 *arrowSize, INT32 *thumbSize,
                                       INT32 *thumbPos )
{
    INT32 pixels;
    BOOL32 vertical;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    switch(nBar)
    {
      case SB_HORZ:
        lprect->left   = wndPtr->rectClient.left - wndPtr->rectWindow.left - 1;
        lprect->top    = wndPtr->rectClient.bottom - wndPtr->rectWindow.top;
        lprect->right  = wndPtr->rectClient.right - wndPtr->rectWindow.left +1;
        lprect->bottom = lprect->top + SYSMETRICS_CYHSCROLL + 1;
        vertical = FALSE;
	break;

      case SB_VERT:
        lprect->left   = wndPtr->rectClient.right - wndPtr->rectWindow.left;
        lprect->top    = wndPtr->rectClient.top - wndPtr->rectWindow.top - 1;
        lprect->right  = lprect->left + SYSMETRICS_CXVSCROLL + 1;
        lprect->bottom = wndPtr->rectClient.bottom - wndPtr->rectWindow.top +1;
        vertical = TRUE;
	break;

      case SB_CTL:
	GetClientRect32( hwnd, lprect );
        vertical = ((wndPtr->dwStyle & SBS_VERT) != 0);
	break;

    default:
        return FALSE;
    }

    if (vertical) pixels = lprect->bottom - lprect->top;
    else pixels = lprect->right - lprect->left;

    if (pixels <= 2*SYSMETRICS_CXVSCROLL + SCROLL_MIN_RECT)
    {
        if (pixels > SCROLL_MIN_RECT)
            *arrowSize = (pixels - SCROLL_MIN_RECT) / 2;
        else
            *arrowSize = 0;
        *thumbPos = *thumbSize = 0;
    }
    else
    {
        SCROLLBAR_INFO *info = SCROLL_GetPtrScrollInfo( wndPtr, nBar );

        *arrowSize = SYSMETRICS_CXVSCROLL;
        pixels -= 2 * SYSMETRICS_CXVSCROLL;

        if (info->Page)
        {
            *thumbSize = pixels * info->Page / (info->MaxVal-info->MinVal+1);
            if (*thumbSize < SCROLL_MIN_THUMB) *thumbSize = SCROLL_MIN_THUMB;
        }
        else *thumbSize = SYSMETRICS_CXVSCROLL;

        if (((pixels -= *thumbSize + 1) < 0) ||
            ((info->flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH))
        {
            /* Rectangle too small or scrollbar disabled -> no thumb */
            *thumbPos = *thumbSize = 0;
        }
        else
        {
            INT32 max = info->MaxVal - MAX( info->Page-1, 0 );
            if (info->MinVal >= max)
                *thumbPos = *arrowSize;
            else
                *thumbPos = *arrowSize + pixels * (info->CurVal-info->MinVal) /
                                                  (max - info->MinVal);
        }
    }
    return vertical;
}


/***********************************************************************
 *           SCROLL_GetThumbVal
 *
 * Compute the current scroll position based on the thumb position in pixels
 * from the top of the scroll-bar.
 */
static UINT32 SCROLL_GetThumbVal( SCROLLBAR_INFO *infoPtr, RECT32 *rect,
                                  BOOL32 vertical, INT32 pos )
{
    INT32 thumbSize;
    INT32 pixels = vertical ? rect->bottom-rect->top : rect->right-rect->left;

    if ((pixels -= 2*SYSMETRICS_CXVSCROLL) <= 0) return infoPtr->MinVal;

    if (infoPtr->Page)
    {
        thumbSize = pixels * infoPtr->Page/(infoPtr->MaxVal-infoPtr->MinVal+1);
        if (thumbSize < SCROLL_MIN_THUMB) thumbSize = SCROLL_MIN_THUMB;
    }
    else thumbSize = SYSMETRICS_CXVSCROLL;

    if ((pixels -= thumbSize + 1) <= 0) return infoPtr->MinVal;

    pos = MAX( 0, pos - SYSMETRICS_CXVSCROLL );
    if (pos > pixels) pos = pixels;

    if (!infoPtr->Page) pos *= infoPtr->MaxVal - infoPtr->MinVal;
    else pos *= infoPtr->MaxVal - infoPtr->MinVal - infoPtr->Page + 1;
    return infoPtr->MinVal + ((pos + pixels / 2) / pixels);
}


/***********************************************************************
 *           SCROLL_HitTest
 *
 * Scroll-bar hit testing (don't confuse this with WM_NCHITTEST!).
 */
static enum SCROLL_HITTEST SCROLL_HitTest( HWND32 hwnd, INT32 nBar,
                                           POINT32 pt )
{
    INT32 arrowSize, thumbSize, thumbPos;
    RECT32 rect;

    BOOL32 vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                           &arrowSize, &thumbSize, &thumbPos );
    if (!PtInRect32( &rect, pt )) return SCROLL_NOWHERE;

    if (vertical)
    {
        if (pt.y <= rect.top + arrowSize + 1) return SCROLL_TOP_ARROW;
        if (pt.y >= rect.bottom - arrowSize) return SCROLL_BOTTOM_ARROW;
        if (!thumbPos) return SCROLL_TOP_RECT;
        pt.y -= rect.top;
        if (pt.y < thumbPos) return SCROLL_TOP_RECT;
        if (pt.y > thumbPos + thumbSize) return SCROLL_BOTTOM_RECT;
        return SCROLL_THUMB;
    }
    else  /* horizontal */
    {
        if (pt.x <= rect.left + arrowSize) return SCROLL_TOP_ARROW;
        if (pt.x >= rect.right - arrowSize) return SCROLL_BOTTOM_ARROW;
        if (!thumbPos) return SCROLL_TOP_RECT;
        pt.x -= rect.left;
        if (pt.x < thumbPos) return SCROLL_TOP_RECT;
        if (pt.x > thumbPos + thumbSize) return SCROLL_BOTTOM_RECT;
        return SCROLL_THUMB;
    }
}


/***********************************************************************
 *           SCROLL_DrawArrows
 *
 * Draw the scroll bar arrows.
 */
static void SCROLL_DrawArrows( HDC32 hdc, SCROLLBAR_INFO *infoPtr,
                               RECT32 *rect, INT32 arrowSize, BOOL32 vertical,
                               BOOL32 top_pressed, BOOL32 bottom_pressed )
{
    HDC32 hdcMem = CreateCompatibleDC32( hdc );
    HBITMAP32 hbmpPrev = SelectObject32( hdcMem, vertical ?
                                    TOP_ARROW(infoPtr->flags, top_pressed)
                                    : LEFT_ARROW(infoPtr->flags, top_pressed));
    SetStretchBltMode32( hdc, STRETCH_DELETESCANS );
    StretchBlt32( hdc, rect->left, rect->top,
                  vertical ? rect->right-rect->left : arrowSize+1,
                  vertical ? arrowSize+1 : rect->bottom-rect->top,
                  hdcMem, 0, 0,
                  SYSMETRICS_CXVSCROLL + 1, SYSMETRICS_CYHSCROLL + 1,
                  SRCCOPY );

    SelectObject32( hdcMem, vertical ?
                    BOTTOM_ARROW( infoPtr->flags, bottom_pressed )
                    : RIGHT_ARROW( infoPtr->flags, bottom_pressed ) );
    if (vertical)
        StretchBlt32( hdc, rect->left, rect->bottom - arrowSize - 1,
                      rect->right - rect->left, arrowSize + 1,
                      hdcMem, 0, 0,
                      SYSMETRICS_CXVSCROLL + 1, SYSMETRICS_CYHSCROLL + 1,
                      SRCCOPY );
    else
        StretchBlt32( hdc, rect->right - arrowSize - 1, rect->top,
                      arrowSize + 1, rect->bottom - rect->top,
                      hdcMem, 0, 0,
                      SYSMETRICS_CXVSCROLL + 1, SYSMETRICS_CYHSCROLL + 1,
                      SRCCOPY );
    SelectObject32( hdcMem, hbmpPrev );
    DeleteDC32( hdcMem );
}


/***********************************************************************
 *           SCROLL_DrawMovingThumb
 *
 * Draw the moving thumb rectangle.
 */
static void SCROLL_DrawMovingThumb( HDC32 hdc, RECT32 *rect, BOOL32 vertical,
                                    INT32 arrowSize, INT32 thumbSize )
{
    RECT32 r = *rect;
    if (vertical)
    {
        r.top += SCROLL_TrackingPos;
        if (r.top < rect->top + arrowSize) r.top = rect->top + arrowSize;
        if (r.top + thumbSize >= rect->bottom - arrowSize)
            r.top = rect->bottom - arrowSize - thumbSize - 1;
        r.bottom = r.top + thumbSize + 1;
    }
    else
    {
        r.left += SCROLL_TrackingPos;
        if (r.left < rect->left + arrowSize) r.left = rect->left + arrowSize;
        if (r.left + thumbSize >= rect->right - arrowSize)
            r.left = rect->right - arrowSize - thumbSize - 1;
        r.right = r.left + thumbSize + 1;
    }
    InflateRect32( &r, -1, -1 );
    DrawFocusRect32( hdc, &r );
    SCROLL_MovingThumb = !SCROLL_MovingThumb;
}


/***********************************************************************
 *           SCROLL_DrawInterior
 *
 * Draw the scroll bar interior (everything except the arrows).
 */
static void SCROLL_DrawInterior( HWND32 hwnd, HDC32 hdc, INT32 nBar, 
                                 RECT32 *rect, INT32 arrowSize,
                                 INT32 thumbSize, INT32 thumbPos,
                                 UINT32 flags, BOOL32 vertical,
                                 BOOL32 top_selected, BOOL32 bottom_selected )
{
    RECT32 r;

      /* Select the correct brush and pen */

    SelectObject32( hdc, sysColorObjects.hpenWindowFrame );
    if ((flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
    {
          /* This ought to be the color of the parent window */
        SelectObject32( hdc, sysColorObjects.hbrushWindow );
    }
    else
    {
        if (nBar == SB_CTL)  /* Only scrollbar controls send WM_CTLCOLOR */
        {
            HBRUSH32 hbrush = SendMessage32A(GetParent32(hwnd),
                                             WM_CTLCOLORSCROLLBAR, hdc, hwnd );
            SelectObject32( hdc, hbrush );
        }
        else SelectObject32( hdc, sysColorObjects.hbrushScrollbar );
    }

      /* Calculate the scroll rectangle */

    r = *rect;
    if (vertical)
    {
        r.top    += arrowSize;
        r.bottom -= arrowSize;
    }
    else
    {
        r.left  += arrowSize;
        r.right -= arrowSize;
    }

      /* Draw the scroll bar frame */

    MoveTo( hdc, r.left, r.top );
    LineTo32( hdc, r.right-1, r.top );
    LineTo32( hdc, r.right-1, r.bottom-1 );
    LineTo32( hdc, r.left, r.bottom-1 );
    LineTo32( hdc, r.left, r.top );

      /* Draw the scroll rectangles and thumb */

    if (!thumbPos)  /* No thumb to draw */
    {
        PatBlt32( hdc, r.left+1, r.top+1, r.right - r.left - 2,
                  r.bottom - r.top - 2, PATCOPY );
        return;
    }

    if (vertical)
    {
        PatBlt32( hdc, r.left + 1, r.top + 1,
                  r.right - r.left - 2,
                  thumbPos - arrowSize,
                  top_selected ? 0x0f0000 : PATCOPY );
        r.top += thumbPos - arrowSize;
        PatBlt32( hdc, r.left + 1, r.top + thumbSize + 1,
                  r.right - r.left - 2,
                  r.bottom - r.top - thumbSize - 2,
                  bottom_selected ? 0x0f0000 : PATCOPY );
        r.bottom = r.top + thumbSize + 1;
    }
    else  /* horizontal */
    {
        PatBlt32( hdc, r.left + 1, r.top + 1,
                  thumbPos - arrowSize,
                  r.bottom - r.top - 2,
                  top_selected ? 0x0f0000 : PATCOPY );
        r.left += thumbPos - arrowSize;
        PatBlt32( hdc, r.left + thumbSize + 1, r.top + 1,
                  r.right - r.left - thumbSize - 2,
                  r.bottom - r.top - 2,
                  bottom_selected ? 0x0f0000 : PATCOPY );
        r.right = r.left + thumbSize + 1;
    }

      /* Draw the thumb */

    SelectObject32( hdc, sysColorObjects.hbrushBtnFace );
    Rectangle32( hdc, r.left, r.top, r.right, r.bottom );
    InflateRect32( &r, -1, -1 );
    GRAPH_DrawReliefRect( hdc, &r, 1, 2, FALSE );
    if (SCROLL_MovingThumb &&
        (SCROLL_TrackingWin == hwnd) &&
        (SCROLL_TrackingBar == nBar))
    {
        SCROLL_DrawMovingThumb( hdc, rect, vertical, arrowSize, thumbSize );
        SCROLL_MovingThumb = TRUE;
    }
}


/***********************************************************************
 *           SCROLL_DrawScrollBar
 *
 * Redraw the whole scrollbar.
 */
void SCROLL_DrawScrollBar( HWND32 hwnd, HDC32 hdc, INT32 nBar, BOOL32 arrows )
{
    INT32 arrowSize, thumbSize, thumbPos;
    RECT32 rect;
    BOOL32 vertical;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SCROLLBAR_INFO *infoPtr = SCROLL_GetPtrScrollInfo( wndPtr, nBar );

    if (!wndPtr || !infoPtr ||
        ((nBar == SB_VERT) && !(wndPtr->dwStyle & WS_VSCROLL)) ||
        ((nBar == SB_HORZ) && !(wndPtr->dwStyle & WS_HSCROLL))) return;

    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbSize, &thumbPos );
      /* Draw the arrows */

    if (arrows && arrowSize) SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize,
                                                vertical, FALSE, FALSE );
    
    SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                         thumbPos, infoPtr->flags, vertical, FALSE, FALSE );
}


/***********************************************************************
 *           SCROLL_RefreshScrollBar
 *
 * Repaint the scroll bar interior after a SetScrollRange() or
 * SetScrollPos() call.
 */
static void SCROLL_RefreshScrollBar( HWND32 hwnd, INT32 nBar, BOOL32 arrows )
{
    HDC32 hdc = GetDCEx32( hwnd, 0,
                           DCX_CACHE | ((nBar == SB_CTL) ? 0 : DCX_WINDOW) );
    if (!hdc) return;
    SCROLL_DrawScrollBar( hwnd, hdc, nBar, arrows );
    ReleaseDC32( hwnd, hdc );
}


/***********************************************************************
 *           SCROLL_HandleKbdEvent
 *
 * Handle a keyboard event (only for SB_CTL scrollbars).
 */
static void SCROLL_HandleKbdEvent( HWND32 hwnd, WPARAM32 wParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WPARAM32 msg;
    
    switch(wParam)
    {
    case VK_PRIOR: msg = SB_PAGEUP; break;
    case VK_NEXT:  msg = SB_PAGEDOWN; break;
    case VK_HOME:  msg = SB_TOP; break;
    case VK_END:   msg = SB_BOTTOM; break;
    case VK_UP:    msg = SB_LINEUP; break;
    case VK_DOWN:  msg = SB_LINEDOWN; break;
    default:
        return;
    }
    SendMessage32A( GetParent32(hwnd),
                    (wndPtr->dwStyle & SBS_VERT) ? WM_VSCROLL : WM_HSCROLL,
                    msg, hwnd );
}


/***********************************************************************
 *           SCROLL_HandleScrollEvent
 *
 * Handle a mouse or timer event for the scrollbar.
 * 'pt' is the location of the mouse event in client (for SB_CTL) or
 * windows coordinates.
 */
void SCROLL_HandleScrollEvent( HWND32 hwnd, INT32 nBar, UINT32 msg, POINT32 pt)
{
      /* Previous mouse position for timer events */
    static POINT32 prevPt;
      /* Hit test code of the last button-down event */
    static enum SCROLL_HITTEST trackHitTest;
      /* Thumb position when tracking started. */
    static UINT32 trackThumbPos;
      /* Position in the scroll-bar of the last button-down event. */
    static INT32 lastClickPos;
      /* Position in the scroll-bar of the last mouse event. */
    static INT32 lastMousePos;

    enum SCROLL_HITTEST hittest;
    HWND32 hwndOwner, hwndCtl;
    BOOL32 vertical;
    INT32 arrowSize, thumbSize, thumbPos;
    RECT32 rect;
    HDC32 hdc;

    SCROLLBAR_INFO *infoPtr = SCROLL_GetScrollInfo( hwnd, nBar );
    if (!infoPtr) return;
    if ((trackHitTest == SCROLL_NOWHERE) && (msg != WM_LBUTTONDOWN)) return;

    hdc = GetDCEx32( hwnd, 0, DCX_CACHE | ((nBar == SB_CTL) ? 0 : DCX_WINDOW));
    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbSize, &thumbPos );
    hwndOwner = (nBar == SB_CTL) ? GetParent32(hwnd) : hwnd;
    hwndCtl   = (nBar == SB_CTL) ? hwnd : 0;

    switch(msg)
    {
      case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
          trackHitTest  = hittest = SCROLL_HitTest( hwnd, nBar, pt );
          lastClickPos  = vertical ? (pt.y - rect.top) : (pt.x - rect.left);
          lastMousePos  = lastClickPos;
          trackThumbPos = thumbPos;
          prevPt = pt;
          SetCapture32( hwnd );
          if (nBar == SB_CTL) SetFocus32( hwnd );
          break;

      case WM_MOUSEMOVE:
          hittest = SCROLL_HitTest( hwnd, nBar, pt );
          prevPt = pt;
          break;

      case WM_LBUTTONUP:
          hittest = SCROLL_NOWHERE;
          ReleaseCapture();
          break;

      case WM_SYSTIMER:
          pt = prevPt;
          hittest = SCROLL_HitTest( hwnd, nBar, pt );
          break;

      default:
          return;  /* Should never happen */
    }

    dprintf_scroll( stddeb, "ScrollBar Event: hwnd=%04x bar=%d msg=%x pt=%d,%d hit=%d\n",
                    hwnd, nBar, msg, pt.x, pt.y, hittest );

    switch(trackHitTest)
    {
    case SCROLL_NOWHERE:  /* No tracking in progress */
        break;

    case SCROLL_TOP_ARROW:
        SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical,
                           (hittest == trackHitTest), FALSE );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_LINEUP, hwndCtl );
                SetSystemTimer32( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC32)0 );
            }
        }
        else KillSystemTimer32( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_TOP_RECT:
        SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                             thumbPos, infoPtr->flags, vertical,
                             (hittest == trackHitTest), FALSE );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_PAGEUP, hwndCtl );
                SetSystemTimer32( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC32)0 );
            }
        }
        else KillSystemTimer32( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_THUMB:
        if (msg == WM_LBUTTONDOWN)
        {
            SCROLL_TrackingWin = hwnd;
            SCROLL_TrackingBar = nBar;
            SCROLL_TrackingPos = trackThumbPos + lastMousePos - lastClickPos;
            SCROLL_DrawMovingThumb(hdc, &rect, vertical, arrowSize, thumbSize);
        }
        else if (msg == WM_LBUTTONUP)
        {
            SCROLL_TrackingWin = 0;
            SCROLL_MovingThumb = FALSE;
            SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                                 thumbPos, infoPtr->flags, vertical,
                                 FALSE, FALSE );
        }
        else  /* WM_MOUSEMOVE */
        {
            UINT32 pos;

            if (!PtInRect32( &rect, pt )) pos = lastClickPos;
            else pos = vertical ? (pt.y - rect.top) : (pt.x - rect.left);
            if (pos != lastMousePos)
            {
                SCROLL_DrawMovingThumb( hdc, &rect, vertical,
                                        arrowSize, thumbSize );
                lastMousePos = pos;
                SCROLL_TrackingPos = trackThumbPos + pos - lastClickPos;
                SCROLL_TrackingVal = SCROLL_GetThumbVal( infoPtr, &rect,
                                                         vertical,
                                                         SCROLL_TrackingPos );
                SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                MAKEWPARAM( SB_THUMBTRACK, SCROLL_TrackingVal),
                                hwndCtl );
                SCROLL_DrawMovingThumb( hdc, &rect, vertical,
                                        arrowSize, thumbSize );
            }
        }
        break;
        
    case SCROLL_BOTTOM_RECT:
        SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                             thumbPos, infoPtr->flags, vertical,
                             FALSE, (hittest == trackHitTest) );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_PAGEDOWN, hwndCtl );
                SetSystemTimer32( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC32)0 );
            }
        }
        else KillSystemTimer32( hwnd, SCROLL_TIMER );
        break;
        
    case SCROLL_BOTTOM_ARROW:
        SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical,
                           FALSE, (hittest == trackHitTest) );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_LINEDOWN, hwndCtl );
                SetSystemTimer32( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC32)0 );
            }
        }
        else KillSystemTimer32( hwnd, SCROLL_TIMER );
        break;
    }

    if (msg == WM_LBUTTONUP)
    {
        if (trackHitTest == SCROLL_THUMB)
        {
            UINT32 val = SCROLL_GetThumbVal( infoPtr, &rect, vertical,
                                 trackThumbPos + lastMousePos - lastClickPos );
            SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                            MAKEWPARAM( SB_THUMBPOSITION, val ), hwndCtl );
        }
        else
            SendMessage32A( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                            SB_ENDSCROLL, hwndCtl );
        trackHitTest = SCROLL_NOWHERE;  /* Terminate tracking */
    }

    ReleaseDC32( hwnd, hdc );
}


/***********************************************************************
 *           ScrollBarWndProc
 */
LRESULT ScrollBarWndProc( HWND32 hwnd, UINT32 message, WPARAM32 wParam,
                          LPARAM lParam )
{
    switch(message)
    {
    case WM_CREATE:
        {
	    CREATESTRUCT32A *lpCreat = (CREATESTRUCT32A *)lParam;
            if (lpCreat->style & SBS_SIZEBOX)
            {
                fprintf( stdnimp, "Unimplemented style SBS_SIZEBOX.\n" );
                return 0;  /* FIXME */
            }
            
	    if (lpCreat->style & SBS_VERT)
            {
                if (lpCreat->style & SBS_LEFTALIGN)
                    MoveWindow32( hwnd, lpCreat->x, lpCreat->y,
                                  SYSMETRICS_CXVSCROLL+1, lpCreat->cy, FALSE );
                else if (lpCreat->style & SBS_RIGHTALIGN)
                    MoveWindow32( hwnd, 
                                  lpCreat->x+lpCreat->cx-SYSMETRICS_CXVSCROLL-1,
                                  lpCreat->y,
                                  SYSMETRICS_CXVSCROLL+1, lpCreat->cy, FALSE );
            }
            else  /* SBS_HORZ */
            {
                if (lpCreat->style & SBS_TOPALIGN)
                    MoveWindow32( hwnd, lpCreat->x, lpCreat->y,
                                  lpCreat->cx, SYSMETRICS_CYHSCROLL+1, FALSE );
                else if (lpCreat->style & SBS_BOTTOMALIGN)
                    MoveWindow32( hwnd, 
                                  lpCreat->x,
                                  lpCreat->y+lpCreat->cy-SYSMETRICS_CYHSCROLL-1,
                                  lpCreat->cx, SYSMETRICS_CYHSCROLL+1, FALSE );
            }
        }
        if (!hUpArrow) SCROLL_LoadBitmaps();
        dprintf_scroll( stddeb, "ScrollBar creation, hwnd=%04x\n", hwnd );
        return 0;
	
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_SYSTIMER:
        {
            POINT32 pt;
            CONV_POINT16TO32( (POINT16 *)&lParam, &pt );
            SCROLL_HandleScrollEvent( hwnd, SB_CTL, message, pt );
        }
        break;

    case WM_KEYDOWN:
        SCROLL_HandleKbdEvent( hwnd, wParam );
        break;

    case WM_ERASEBKGND:
         return 1;

    case WM_GETDLGCODE:
         return DLGC_WANTARROWS; /* Windows returns this value */

    case WM_PAINT:
        {
            PAINTSTRUCT32 ps;
            HDC32 hdc = BeginPaint32( hwnd, &ps );
            SCROLL_DrawScrollBar( hwnd, hdc, SB_CTL, TRUE );
            EndPaint32( hwnd, &ps );
        }
        break;

    case SBM_SETPOS16:
    case SBM_SETPOS32:
        return SetScrollPos32( hwnd, SB_CTL, wParam, (BOOL32)lParam );

    case SBM_GETPOS16:
    case SBM_GETPOS32:
        return GetScrollPos32( hwnd, SB_CTL );

    case SBM_SETRANGE16:
        SetScrollRange32( hwnd, SB_CTL, LOWORD(lParam), HIWORD(lParam),
                          wParam  /* FIXME: Is this correct? */ );
        return 0;

    case SBM_SETRANGE32:
        SetScrollRange32( hwnd, SB_CTL, wParam, lParam, FALSE );
        return 0;  /* FIXME: return previous position */

    case SBM_GETRANGE16:
        /* FIXME */
        fprintf( stderr, "ScrollBar: don't know how to handle SBM_GETRANGE16 (wp=%04x,lp=%08lx)\n",
                 wParam, lParam );
        return 0;

    case SBM_GETRANGE32:
        GetScrollRange32( hwnd, SB_CTL, (LPINT32)wParam, (LPINT32)lParam );
        return 0;

    case SBM_ENABLE_ARROWS16:
    case SBM_ENABLE_ARROWS32:
        return EnableScrollBar32( hwnd, SB_CTL, wParam );

    case SBM_SETRANGEREDRAW32:
        SetScrollRange32( hwnd, SB_CTL, wParam, lParam, TRUE );
        return 0;  /* FIXME: return previous position */
        
    case SBM_SETSCROLLINFO32:
        return SetScrollInfo32( hwnd, SB_CTL, (SCROLLINFO *)lParam, wParam );

    case SBM_GETSCROLLINFO32:
        return GetScrollInfo32( hwnd, SB_CTL, (SCROLLINFO *)lParam );

    case 0x00e5:
    case 0x00e7:
    case 0x00e8:
    case 0x00eb:
    case 0x00ec:
    case 0x00ed:
    case 0x00ee:
    case 0x00ef:
        fprintf(stderr, "ScrollBar: unknown Win32 msg %04x wp=%08x lp=%08lx\n",
                message, wParam, lParam );
        break;

    default:
        if (message >= WM_USER)
            fprintf( stderr, "ScrollBar: unknown msg %04x wp=%04x lp=%08lx\n",
                     message, wParam, lParam );
        return DefWindowProc32A( hwnd, message, wParam, lParam );
    }
    return 0;
}


/*************************************************************************
 *           SetScrollInfo16   (USER.475)
 */
INT16 SetScrollInfo16( HWND16 hwnd, INT16 nBar, const SCROLLINFO *info,
                       BOOL16 bRedraw )
{
    return (INT16)SetScrollInfo32( hwnd, nBar, info, bRedraw );
}


/*************************************************************************
 *           SetScrollInfo32   (USER32.500)
 */
INT32 SetScrollInfo32( HWND32 hwnd, INT32 nBar, const SCROLLINFO *info,
                       BOOL32 bRedraw )
{
    SCROLLBAR_INFO *infoPtr;
    UINT32 new_flags;
    BOOL32 repaint_arrows = FALSE;

    if (!(infoPtr = SCROLL_GetScrollInfo(hwnd, nBar))) return 0;
    if (info->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL)) return 0;
    if ((info->cbSize != sizeof(*info)) &&
        (info->cbSize != sizeof(*info)-sizeof(info->nTrackPos))) return 0;

    dprintf_scroll( stddeb, "SetScrollInfo: hwnd=%04x bar=%d", hwnd, nBar );

    /* Set the page size */

    if (info->fMask & SIF_PAGE)
    {
        dprintf_scroll( stddeb, " page=%d", info->nPage );
        infoPtr->Page = info->nPage;
    }

    /* Set the scroll pos */

    if (info->fMask & SIF_POS)
    {
        dprintf_scroll( stddeb, " pos=%d", info->nPos );
        infoPtr->CurVal = info->nPos;
    }

    /* Set the scroll range */

    if (info->fMask & SIF_RANGE)
    {
        dprintf_scroll( stddeb, " min=%d max=%d", info->nMin, info->nMax );

        /* Invalid range -> range is set to (0,0) */
        if ((info->nMin > info->nMax) ||
            ((UINT32)(info->nMax - info->nMin) >= 0x80000000))
        {
            infoPtr->MinVal = 0;
            infoPtr->MaxVal = 0;
        }
        else
        {
            infoPtr->MinVal = info->nMin;
            infoPtr->MaxVal = info->nMax;
        }
    }

    /* Make sure the page size is valid */

    if (infoPtr->Page < 0) infoPtr->Page = 0;
    else if (infoPtr->Page > infoPtr->MaxVal - infoPtr->MinVal + 1 )
        infoPtr->Page = infoPtr->MaxVal - infoPtr->MinVal + 1;

    /* Make sure the pos is inside the range */

    if (infoPtr->CurVal < infoPtr->MinVal)
        infoPtr->CurVal = infoPtr->MinVal;
    else if (infoPtr->CurVal > infoPtr->MaxVal - MAX( infoPtr->Page-1, 0 ))
        infoPtr->CurVal = infoPtr->MaxVal - MAX( infoPtr->Page-1, 0 );

    dprintf_scroll( stddeb, "\n   new values: page=%d pos=%d min=%d max=%d\n",
                    infoPtr->Page, infoPtr->CurVal,
                    infoPtr->MinVal, infoPtr->MaxVal );

    /* Check if the scrollbar should be hidden or disabled */

    new_flags = infoPtr->flags;
    if (infoPtr->MinVal >= infoPtr->MaxVal - MAX( infoPtr->Page-1, 0 ))
    {
        /* Hide or disable scroll-bar */
        if (info->fMask & SIF_DISABLENOSCROLL) new_flags = ESB_DISABLE_BOTH;
        else if (nBar != SB_CTL)
        {
            ShowScrollBar32( hwnd, nBar, FALSE );
            bRedraw = FALSE;  /* No need to repaint anything */
        }
    }
    else  /* Show and enable scroll-bar */
    {
        new_flags = 0;
        if (nBar != SB_CTL) ShowScrollBar32( hwnd, nBar, TRUE );
    }

    if (infoPtr->flags != new_flags)
    {
        infoPtr->flags = new_flags;
        repaint_arrows = TRUE;
    }

    if (bRedraw || repaint_arrows)
        SCROLL_RefreshScrollBar( hwnd, nBar, repaint_arrows );

    /* Return current position */

    return infoPtr->CurVal;
}


/*************************************************************************
 *           GetScrollInfo16   (USER.476)
 */
BOOL16 GetScrollInfo16( HWND16 hwnd, INT16 nBar, LPSCROLLINFO info )
{
    return GetScrollInfo32( hwnd, nBar, info );
}


/*************************************************************************
 *           GetScrollInfo32   (USER32.283)
 */
BOOL32 GetScrollInfo32( HWND32 hwnd, INT32 nBar, LPSCROLLINFO info )
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return FALSE;
    if (info->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL)) return FALSE;
    if ((info->cbSize != sizeof(*info)) &&
        (info->cbSize != sizeof(*info)-sizeof(info->nTrackPos))) return FALSE;

    if (info->fMask & SIF_PAGE) info->nPage = infoPtr->Page;
    if (info->fMask & SIF_POS) info->nPos = infoPtr->CurVal;
    if ((info->fMask & SIF_TRACKPOS) && (info->cbSize == sizeof(*info)))
        info->nTrackPos = (SCROLL_TrackingWin==hwnd) ? SCROLL_TrackingVal : 0;
    if (info->fMask & SIF_RANGE)
    {
	info->nMin = infoPtr->MinVal;
	info->nMax = infoPtr->MaxVal;
    }
    return (info->fMask & SIF_ALL) != 0;
}


/*************************************************************************
 *           SetScrollPos16   (USER.62)
 */
INT16 SetScrollPos16( HWND16 hwnd, INT16 nBar, INT16 nPos, BOOL16 bRedraw )
{
    return (INT16)SetScrollPos32( hwnd, nBar, nPos, bRedraw );
}


/*************************************************************************
 *           SetScrollPos32   (USER32.501)
 */
INT32 SetScrollPos32( HWND32 hwnd, INT32 nBar, INT32 nPos, BOOL32 bRedraw )
{
    SCROLLINFO info;
    SCROLLBAR_INFO *infoPtr;
    INT32 oldPos;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return 0;
    oldPos      = infoPtr->CurVal;
    info.cbSize = sizeof(info);
    info.nPos   = nPos;
    info.fMask  = SIF_POS;
    SetScrollInfo32( hwnd, nBar, &info, bRedraw );
    return oldPos;
}


/*************************************************************************
 *           GetScrollPos16   (USER.63)
 */
INT16 GetScrollPos16( HWND16 hwnd, INT16 nBar )
{
    return (INT16)GetScrollPos32( hwnd, nBar );
}


/*************************************************************************
 *           GetScrollPos32   (USER32.284)
 */
INT32 GetScrollPos32( HWND32 hwnd, INT32 nBar )
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return 0;
    return infoPtr->CurVal;
}


/*************************************************************************
 *           SetScrollRange16   (USER.64)
 */
void SetScrollRange16( HWND16 hwnd, INT16 nBar, INT16 MinVal, INT16 MaxVal,
                       BOOL16 bRedraw )
{
    /* Invalid range -> range is set to (0,0) */
    if ((INT32)MaxVal - (INT32)MinVal > 0x7fff) MinVal = MaxVal = 0;
    SetScrollRange32( hwnd, nBar, MinVal, MaxVal, bRedraw );
}


/*************************************************************************
 *           SetScrollRange32   (USER32.502)
 */
BOOL32 SetScrollRange32( HWND32 hwnd, INT32 nBar, INT32 MinVal, INT32 MaxVal,
                         BOOL32 bRedraw )
{
    SCROLLINFO info;

    info.cbSize = sizeof(info);
    info.nMin   = MinVal;
    info.nMax   = MaxVal;
    info.fMask  = SIF_RANGE;
    SetScrollInfo32( hwnd, nBar, &info, bRedraw );
    return TRUE;
}


/*************************************************************************
 *	     SCROLL_SetNCSbState
 *
 * This is for CalcChildScroll in windows/mdi.c
 */
DWORD SCROLL_SetNCSbState(WND* wndPtr, int vMin, int vMax, int vPos,
				       int hMin, int hMax, int hPos)
{
  SCROLLBAR_INFO  *infoPtr = SCROLL_GetPtrScrollInfo(wndPtr, SB_VERT);
 
  wndPtr->dwStyle |= (WS_VSCROLL | WS_HSCROLL);

  if( vMin >= vMax ) 
    { vMin = vMax;
      wndPtr->dwStyle &= ~WS_VSCROLL; }
  if( vPos > vMax ) vPos = vMax; else if( vPos < vMin ) vPos = vMin;
  infoPtr->MinVal = vMin;
  infoPtr->MaxVal = vMax;
  infoPtr->CurVal = vPos;

  infoPtr = SCROLL_GetPtrScrollInfo(wndPtr, SB_HORZ);

  if( hMin >= hMax )
    { hMin = hMax;
      wndPtr->dwStyle &= ~WS_HSCROLL; }
  if( hPos > hMax ) hPos = hMax; else if( hPos < hMin ) hPos = hMin;
  infoPtr->MinVal = hMin;
  infoPtr->MaxVal = hMax;
  infoPtr->CurVal = hPos;

  return wndPtr->dwStyle & (WS_VSCROLL | WS_HSCROLL);
}


/*************************************************************************
 *           GetScrollRange16   (USER.65)
 */
BOOL16 GetScrollRange16( HWND16 hwnd, INT16 nBar, LPINT16 lpMin, LPINT16 lpMax)
{
    INT32 min, max;
    BOOL16 ret = GetScrollRange32( hwnd, nBar, &min, &max );
    if (lpMin) *lpMin = min;
    if (lpMax) *lpMax = max;
    return ret;
}


/*************************************************************************
 *           GetScrollRange32   (USER32.285)
 */
BOOL32 GetScrollRange32( HWND32 hwnd, INT32 nBar, LPINT32 lpMin, LPINT32 lpMax)
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar )))
    {
        if (lpMin) lpMin = 0;
        if (lpMax) lpMax = 0;
        return FALSE;
    }
    if (lpMin) *lpMin = infoPtr->MinVal;
    if (lpMax) *lpMax = infoPtr->MaxVal;
    return TRUE;
}


/*************************************************************************
 *           ShowScrollBar16   (USER.267)
 */
void ShowScrollBar16( HWND16 hwnd, INT16 nBar, BOOL16 fShow )
{
    ShowScrollBar32( hwnd, nBar, fShow );
}


/*************************************************************************
 *           ShowScrollBar32   (USER32.531)
 */
BOOL32 ShowScrollBar32( HWND32 hwnd, INT32 nBar, BOOL32 fShow )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return FALSE;
    dprintf_scroll( stddeb, "ShowScrollBar: hwnd=%04x bar=%d on=%d\n",
                    hwnd, nBar, fShow );

    switch(nBar)
    {
    case SB_CTL:
        ShowWindow32( hwnd, fShow ? SW_SHOW : SW_HIDE );
        return TRUE;

    case SB_HORZ:
        if (fShow)
        {
            if (wndPtr->dwStyle & WS_HSCROLL) return TRUE;
            wndPtr->dwStyle |= WS_HSCROLL;
        }
        else  /* hide it */
        {
            if (!(wndPtr->dwStyle & WS_HSCROLL)) return TRUE;
            wndPtr->dwStyle &= ~WS_HSCROLL;
        }
        break;

    case SB_VERT:
        if (fShow)
        {
            if (wndPtr->dwStyle & WS_VSCROLL) return TRUE;
            wndPtr->dwStyle |= WS_VSCROLL;
        }
        else  /* hide it */
        {
            if (!(wndPtr->dwStyle & WS_VSCROLL)) return TRUE;
            wndPtr->dwStyle &= ~WS_VSCROLL;
        }
        break;

    case SB_BOTH:
        if (fShow)
        {
            if ((wndPtr->dwStyle & WS_HSCROLL)
                && (wndPtr->dwStyle & WS_VSCROLL)) return TRUE;
            wndPtr->dwStyle |= WS_HSCROLL | WS_VSCROLL;
        }
        else  /* hide it */
        {
            if (!(wndPtr->dwStyle & WS_HSCROLL)
                && !(wndPtr->dwStyle & WS_VSCROLL)) return TRUE;
            wndPtr->dwStyle &= ~(WS_HSCROLL | WS_VSCROLL);
        }
        break;

    default:
        return TRUE;  /* Nothing to do! */
    }
    SetWindowPos32( hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE
                    | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    return TRUE;
}


/*************************************************************************
 *           EnableScrollBar16   (USER.482)
 */
BOOL16 EnableScrollBar16( HWND16 hwnd, INT16 nBar, UINT16 flags )
{
    return EnableScrollBar32( hwnd, nBar, flags );
}


/*************************************************************************
 *           EnableScrollBar32   (USER32.170)
 */
BOOL32 EnableScrollBar32( HWND32 hwnd, INT32 nBar, UINT32 flags )
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return FALSE;
    dprintf_scroll( stddeb, "EnableScrollBar: %04x %d %d\n",
                    hwnd, nBar, flags );
    flags &= ESB_DISABLE_BOTH;
    if (infoPtr->flags == flags) return FALSE;
    infoPtr->flags = flags;

    /* Redraw the whole scroll bar */
    SCROLL_RefreshScrollBar( hwnd, nBar, TRUE );
    return TRUE;
}

/*		
 * Interface code to SCROLLBAR widget
 *
 * Copyright  Martin Ayotte, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 * Small fixes and implemented SB_THUMBPOSITION
 * by Peter Broadhurst, 940611
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "syscolor.h"
#include "sysmetrics.h"
#include "scroll.h"
#include "user.h"
#include "graphics.h"
#include "win.h"
#include "stddebug.h"
/* #define DEBUG_SCROLL */
#include "debug.h"


static HBITMAP hUpArrow = 0;
static HBITMAP hDnArrow = 0;
static HBITMAP hLfArrow = 0;
static HBITMAP hRgArrow = 0;
static HBITMAP hUpArrowD = 0;
static HBITMAP hDnArrowD = 0;
static HBITMAP hLfArrowD = 0;
static HBITMAP hRgArrowD = 0;
static HBITMAP hUpArrowI = 0;
static HBITMAP hDnArrowI = 0;
static HBITMAP hLfArrowI = 0;
static HBITMAP hRgArrowI = 0;

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

  /* Delay (in ms) before first repetition when holding the button down */
#define SCROLL_FIRST_DELAY   200

  /* Delay (in ms) between scroll repetitions */
#define SCROLL_REPEAT_DELAY  100

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
static HWND hwndTracking = 0;
static int nBarTracking = 0;
static UINT uTrackingPos = 0;

/***********************************************************************
 *           SCROLL_LoadBitmaps
 */
static void SCROLL_LoadBitmaps(void)
{
    hUpArrow  = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_UPARROW));
    hDnArrow  = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_DNARROW));
    hLfArrow  = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_LFARROW));
    hRgArrow  = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RGARROW));
    hUpArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_UPARROWD));
    hDnArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_DNARROWD));
    hLfArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_LFARROWD));
    hRgArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RGARROWD));
    hUpArrowI = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_UPARROWI));
    hDnArrowI = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_DNARROWI));
    hLfArrowI = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_LFARROWI));
    hRgArrowI = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RGARROWI));
}

/***********************************************************************
 *           SCROLL_GetPtrScrollInfo
 */
static SCROLLINFO *SCROLL_GetPtrScrollInfo( WND* wndPtr, int nBar )
{
    HANDLE handle;

    if (!wndPtr) return NULL;
    switch(nBar)
    {
        case SB_HORZ: handle = wndPtr->hHScroll; break;
        case SB_VERT: handle = wndPtr->hVScroll; break;
        case SB_CTL:  return (SCROLLINFO *)wndPtr->wExtra;
        default:      return NULL;
    }

    if (!handle)  /* Create the info structure if needed */
    {
        if ((handle = USER_HEAP_ALLOC( sizeof(SCROLLINFO) )))
        {
            SCROLLINFO *infoPtr = (SCROLLINFO *) USER_HEAP_LIN_ADDR( handle );
            infoPtr->MinVal = infoPtr->CurVal = 0;
            infoPtr->MaxVal = 100;
            infoPtr->flags  = ESB_ENABLE_BOTH;
            if (nBar == SB_HORZ) wndPtr->hHScroll = handle;
            else wndPtr->hVScroll = handle;
        }
        if (!hUpArrow) SCROLL_LoadBitmaps();
    }
    return (SCROLLINFO *) USER_HEAP_LIN_ADDR( handle );
}

/***********************************************************************
 *           SCROLL_GetScrollInfo
 */
static SCROLLINFO *SCROLL_GetScrollInfo( HWND hwnd, int nBar )
{
   WND *wndPtr = WIN_FindWndPtr( hwnd );
   return SCROLL_GetPtrScrollInfo( wndPtr, nBar );
}


/***********************************************************************
 *           SCROLL_GetScrollBarRect
 *
 * Compute the scroll bar rectangle, in drawing coordinates (i.e. client
 * coords for SB_CTL, window coords for SB_VERT and SB_HORZ).
 * 'arrowSize' returns the width or height of an arrow (depending on the
 * orientation of the scrollbar), and 'thumbPos' returns the position of
 * the thumb relative to the left or to the top.
 * Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL SCROLL_GetScrollBarRect( HWND hwnd, int nBar, RECT *lprect,
                                     WORD *arrowSize, WORD *thumbPos )
{
    int pixels;
    BOOL vertical;
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
	GetClientRect( hwnd, lprect );
        vertical = ((wndPtr->dwStyle & SBS_VERT) != 0);
	break;

    default:
        return FALSE;
    }

    if (vertical) pixels = lprect->bottom - lprect->top;
    else pixels = lprect->right - lprect->left;

    if (pixels > 2*SYSMETRICS_CXVSCROLL + SCROLL_MIN_RECT)
    {
        *arrowSize = SYSMETRICS_CXVSCROLL;
    }
    else if (pixels > SCROLL_MIN_RECT)
    {
        *arrowSize = (pixels - SCROLL_MIN_RECT) / 2;
    }
    else *arrowSize = 0;
    
    if ((pixels -= 3*SYSMETRICS_CXVSCROLL+1) > 0)
    {
        SCROLLINFO *info = SCROLL_GetPtrScrollInfo( wndPtr, nBar );
        if ((info->flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
            *thumbPos = 0;
        else if (info->MinVal == info->MaxVal)
            *thumbPos = *arrowSize;
        else
            *thumbPos = *arrowSize + pixels * (info->CurVal - info->MinVal) /
                                              (info->MaxVal - info->MinVal);
    }
    else *thumbPos = 0;
    return vertical;
}


/***********************************************************************
 *           SCROLL_GetThumbVal
 *
 * Compute the current scroll position based on the thumb position in pixels
 * from the top of the scroll-bar.
 */
static UINT SCROLL_GetThumbVal( SCROLLINFO *infoPtr, RECT *rect,
                                BOOL vertical, WORD pos )
{
    int pixels = vertical ? rect->bottom-rect->top : rect->right-rect->left;
    if ((pixels -= 3*SYSMETRICS_CXVSCROLL+1) <= 0) return infoPtr->MinVal;
    pos = MAX( 0, pos - SYSMETRICS_CXVSCROLL );
    if (pos > pixels) pos = pixels;
    dprintf_scroll(stddeb,"GetThumbVal: pos=%d ret=%d\n", pos,
                   (infoPtr->MinVal
            + (UINT)(((int)pos * (infoPtr->MaxVal-infoPtr->MinVal) + pixels/2)
                     / pixels)) );
    return (infoPtr->MinVal
            + (UINT)(((int)pos * (infoPtr->MaxVal-infoPtr->MinVal) + pixels/2)
                     / pixels));
}


/***********************************************************************
 *           SCROLL_HitTest
 *
 * Scroll-bar hit testing (don't confuse this with WM_NCHITTEST!).
 */
static enum SCROLL_HITTEST SCROLL_HitTest( HWND hwnd, int nBar, POINT pt )
{
    WORD arrowSize, thumbPos;
    RECT rect;

    BOOL vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                             &arrowSize, &thumbPos );
    if (!PtInRect( &rect, pt )) return SCROLL_NOWHERE;

    if (vertical)
    {
        if (pt.y <= rect.top + arrowSize + 1) return SCROLL_TOP_ARROW;
        if (pt.y >= rect.bottom - arrowSize) return SCROLL_BOTTOM_ARROW;
        if (!thumbPos) return SCROLL_TOP_RECT;
        pt.y -= rect.top;
        if (pt.y < (INT)thumbPos) return SCROLL_TOP_RECT;
        if (pt.y > thumbPos+SYSMETRICS_CYHSCROLL) return SCROLL_BOTTOM_RECT;
        return SCROLL_THUMB;
    }
    else  /* horizontal */
    {
        if (pt.x <= rect.left + arrowSize) return SCROLL_TOP_ARROW;
        if (pt.x >= rect.right - arrowSize) return SCROLL_BOTTOM_ARROW;
        if (!thumbPos) return SCROLL_TOP_RECT;
        pt.x -= rect.left;
        if (pt.x < (INT)thumbPos) return SCROLL_TOP_RECT;
        if (pt.x > thumbPos+SYSMETRICS_CXVSCROLL) return SCROLL_BOTTOM_RECT;
        return SCROLL_THUMB;
    }
}


/***********************************************************************
 *           SCROLL_DrawArrows
 *
 * Draw the scroll bar arrows.
 */
static void SCROLL_DrawArrows( HDC hdc, SCROLLINFO *infoPtr, RECT *rect,
                               WORD arrowSize, BOOL vertical,
                               BOOL top_pressed, BOOL bottom_pressed )
{
    HDC hdcMem = CreateCompatibleDC( hdc );
    HBITMAP hbmpPrev = SelectObject( hdcMem, vertical ?
                                    TOP_ARROW(infoPtr->flags, top_pressed)
                                    : LEFT_ARROW(infoPtr->flags, top_pressed));
    SetStretchBltMode( hdc, STRETCH_DELETESCANS );
    StretchBlt( hdc, rect->left, rect->top,
                vertical ? rect->right-rect->left : arrowSize+1,
                vertical ? arrowSize+1 : rect->bottom-rect->top,
                hdcMem, 0, 0,
                SYSMETRICS_CXVSCROLL + 1, SYSMETRICS_CYHSCROLL + 1,
                SRCCOPY );

    SelectObject( hdcMem, vertical ?
                  BOTTOM_ARROW( infoPtr->flags, bottom_pressed )
                  : RIGHT_ARROW( infoPtr->flags, bottom_pressed ) );
    if (vertical)
        StretchBlt( hdc, rect->left, rect->bottom - arrowSize - 1,
                   rect->right - rect->left, arrowSize + 1,
                   hdcMem, 0, 0,
                   SYSMETRICS_CXVSCROLL + 1, SYSMETRICS_CYHSCROLL + 1,
                   SRCCOPY );
    else
        StretchBlt( hdc, rect->right - arrowSize - 1, rect->top,
                   arrowSize + 1, rect->bottom - rect->top,
                   hdcMem, 0, 0,
                   SYSMETRICS_CXVSCROLL + 1, SYSMETRICS_CYHSCROLL + 1,
                   SRCCOPY );
    SelectObject( hdcMem, hbmpPrev );
    DeleteDC( hdcMem );
}


/***********************************************************************
 *           SCROLL_DrawMovingThumb
 *
 * Draw the moving thumb rectangle.
 */
static void SCROLL_DrawMovingThumb( HDC hdc, RECT *rect, BOOL vertical,
                                    WORD arrowSize, WORD thumbPos )
{
    RECT r = *rect;
    if (vertical)
    {
        r.top += thumbPos;
        if (r.top < rect->top + arrowSize) r.top = rect->top + arrowSize;
        if (r.top + SYSMETRICS_CYHSCROLL >= rect->bottom - arrowSize)
            r.top = rect->bottom - arrowSize - SYSMETRICS_CYHSCROLL - 1;
        r.bottom = r.top + SYSMETRICS_CYHSCROLL + 1;
    }
    else
    {
        r.left += thumbPos;
        if (r.left < rect->left + arrowSize) r.left = rect->left + arrowSize;
        if (r.left + SYSMETRICS_CXVSCROLL >= rect->right - arrowSize)
            r.left = rect->right - arrowSize - SYSMETRICS_CXVSCROLL - 1;
        r.right = r.left + SYSMETRICS_CXVSCROLL + 1;
    }
    InflateRect( &r, -1, -1 );
    DrawFocusRect( hdc, &r );
}


/***********************************************************************
 *           SCROLL_DrawInterior
 *
 * Draw the scroll bar interior (everything except the arrows).
 */
static void SCROLL_DrawInterior( HWND hwnd, HDC hdc, int nBar, RECT *rect,
                                 WORD arrowSize, WORD thumbPos, WORD flags,
                                 BOOL vertical, BOOL top_selected,
                                 BOOL bottom_selected )
{
    RECT r;

      /* Select the correct brush and pen */

    SelectObject( hdc, sysColorObjects.hpenWindowFrame );
    if ((flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
    {
          /* This ought to be the color of the parent window */
        SelectObject( hdc, sysColorObjects.hbrushWindow );
    }
    else
    {
        if (nBar == SB_CTL)  /* Only scrollbar controls send WM_CTLCOLOR */
        {
#ifdef WINELIB32
            HBRUSH hbrush = SendMessage( GetParent(hwnd), WM_CTLCOLORSCROLLBAR,
                                         hdc, hwnd );
#else
            HBRUSH hbrush = SendMessage( GetParent(hwnd), WM_CTLCOLOR, hdc,
                                         MAKELONG(hwnd, CTLCOLOR_SCROLLBAR) );
#endif
            SelectObject( hdc, hbrush );
        }
        else SelectObject( hdc, sysColorObjects.hbrushScrollbar );
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
    LineTo( hdc, r.right-1, r.top );
    LineTo( hdc, r.right-1, r.bottom-1 );
    LineTo( hdc, r.left, r.bottom-1 );
    LineTo( hdc, r.left, r.top );

      /* Draw the scroll rectangles and thumb */

    if (!thumbPos)  /* No thumb to draw */
    {
        PatBlt( hdc, r.left+1, r.top+1, r.right - r.left - 2,
                r.bottom - r.top - 2, PATCOPY );
        return;
    }

    if (vertical)
    {
        PatBlt( hdc, r.left + 1, r.top + 1,
                r.right - r.left - 2,
                thumbPos - arrowSize,
                top_selected ? 0x0f0000 : PATCOPY );
        r.top += thumbPos - arrowSize;
        PatBlt( hdc, r.left + 1, r.top + SYSMETRICS_CYHSCROLL + 1,
                r.right - r.left - 2,
                r.bottom - r.top - SYSMETRICS_CYHSCROLL - 2,
                bottom_selected ? 0x0f0000 : PATCOPY );
        r.bottom = r.top + SYSMETRICS_CYHSCROLL + 1;
    }
    else  /* horizontal */
    {
        PatBlt( hdc, r.left + 1, r.top + 1,
                thumbPos - arrowSize,
                r.bottom - r.top - 2,
                top_selected ? 0x0f0000 : PATCOPY );
        r.left += thumbPos - arrowSize;
        PatBlt( hdc, r.left + SYSMETRICS_CYHSCROLL + 1, r.top + 1,
                r.right - r.left - SYSMETRICS_CYHSCROLL - 2,
                r.bottom - r.top - 2,
                bottom_selected ? 0x0f0000 : PATCOPY );
        r.right = r.left + SYSMETRICS_CXVSCROLL + 1;
    }

      /* Draw the thumb */

    SelectObject( hdc, sysColorObjects.hbrushBtnFace );
    Rectangle( hdc, r.left, r.top, r.right, r.bottom );
    InflateRect( &r, -1, -1 );
    GRAPH_DrawReliefRect( hdc, &r, 1, 2, FALSE );
    if ((hwndTracking == hwnd) && (nBarTracking == nBar))
        SCROLL_DrawMovingThumb( hdc, rect, vertical, arrowSize, uTrackingPos);
}


/***********************************************************************
 *           SCROLL_DrawScrollBar
 *
 * Redraw the whole scrollbar.
 */
void SCROLL_DrawScrollBar( HWND hwnd, HDC hdc, int nBar )
{
    WORD arrowSize, thumbPos;
    RECT rect;
    BOOL vertical;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SCROLLINFO *infoPtr = SCROLL_GetPtrScrollInfo( wndPtr, nBar );

    if (!wndPtr || !infoPtr ||
        ((nBar == SB_VERT) && !(wndPtr->dwStyle & WS_VSCROLL)) ||
        ((nBar == SB_HORZ) && !(wndPtr->dwStyle & WS_HSCROLL))) return;

    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbPos );
      /* Draw the arrows */

    if (arrowSize) SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize,
                                      vertical, FALSE, FALSE );
    
    SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbPos,
                         infoPtr->flags, vertical, FALSE, FALSE );
}


/***********************************************************************
 *           SCROLL_RefreshScrollBar
 *
 * Repaint the scroll bar interior after a SetScrollRange() or
 * SetScrollPos() call.
 */
static void SCROLL_RefreshScrollBar( HWND hwnd, int nBar )
{
    WORD arrowSize, thumbPos;
    RECT rect;
    BOOL vertical;
    HDC hdc;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SCROLLINFO *infoPtr = SCROLL_GetPtrScrollInfo( wndPtr, nBar );

    if (!wndPtr || !infoPtr ||
        ((nBar == SB_VERT) && !(wndPtr->dwStyle & WS_VSCROLL)) ||
        ((nBar == SB_HORZ) && !(wndPtr->dwStyle & WS_HSCROLL))) return;

    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbPos );
    hdc = (nBar == SB_CTL) ? GetDC(hwnd) : GetWindowDC(hwnd);
    if (!hdc) return;
    SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbPos,
                         infoPtr->flags, vertical, FALSE, FALSE );
    ReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           SCROLL_HandleKbdEvent
 *
 * Handle a keyboard event (only for SB_CTL scrollbars).
 */
static void SCROLL_HandleKbdEvent( HWND hwnd, WORD wParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WPARAM msg;
    
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
#ifdef WINELIB32
    SendMessage( GetParent(hwnd),
                 (wndPtr->dwStyle & SBS_VERT) ? WM_VSCROLL : WM_HSCROLL,
                 msg, hwnd );
#else
    SendMessage( GetParent(hwnd),
                 (wndPtr->dwStyle & SBS_VERT) ? WM_VSCROLL : WM_HSCROLL,
                 msg, MAKELONG( 0, hwnd ));
#endif
}


/***********************************************************************
 *           SCROLL_HandleScrollEvent
 *
 * Handle a mouse or timer event for the scrollbar.
 * 'pt' is the location of the mouse event in client (for SB_CTL) or
 * windows coordinates.
 */
void SCROLL_HandleScrollEvent( HWND hwnd, int nBar, WORD msg, POINT pt )
{
      /* Previous mouse position for timer events */
    static POINT prevPt;
      /* Hit test code of the last button-down event */
    static enum SCROLL_HITTEST trackHitTest;
      /* Thumb position when tracking started. */
    static UINT trackThumbPos;
      /* Position in the scroll-bar of the last button-down event. */
    static int lastClickPos;
      /* Position in the scroll-bar of the last mouse event. */
    static int lastMousePos;

    enum SCROLL_HITTEST hittest;
    HWND hwndOwner, hwndCtl;
    BOOL vertical;
    WORD arrowSize, thumbPos;
    RECT rect;
    HDC hdc;

    SCROLLINFO *infoPtr = SCROLL_GetScrollInfo( hwnd, nBar );
    if (!infoPtr) return;
    if ((trackHitTest == SCROLL_NOWHERE) && (msg != WM_LBUTTONDOWN)) return;

    hdc = (nBar == SB_CTL) ? GetDC(hwnd) : GetWindowDC(hwnd);
    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbPos );
    hwndOwner = (nBar == SB_CTL) ? GetParent(hwnd) : hwnd;
    hwndCtl   = (nBar == SB_CTL) ? hwnd : 0;

    switch(msg)
    {
      case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
          trackHitTest  = hittest = SCROLL_HitTest( hwnd, nBar, pt );
          lastClickPos  = vertical ? (pt.y - rect.top) : (pt.x - rect.left);
          lastMousePos  = lastClickPos;
          trackThumbPos = thumbPos;
          prevPt = pt;
          SetCapture( hwnd );
          if (nBar == SB_CTL) SetFocus( hwnd );
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
#ifdef WINELIB32
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEUP, hwndCtl );
#else
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEUP, MAKELONG( 0, hwndCtl ));
#endif
                SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                (FARPROC)0 );
            }
        }
        else KillSystemTimer( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_TOP_RECT:
        SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbPos,
                             infoPtr->flags, vertical,
                             (hittest == trackHitTest), FALSE );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
#ifdef WINELIB32
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_PAGEUP, hwndCtl );
#else
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_PAGEUP, MAKELONG( 0, hwndCtl ));
#endif
                SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                (FARPROC)0 );
            }
        }
        else KillSystemTimer( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_THUMB:
        if (msg == WM_LBUTTONDOWN)
        {
            SCROLL_DrawMovingThumb( hdc, &rect, vertical, arrowSize,
                                 trackThumbPos + lastMousePos - lastClickPos );
            hwndTracking = hwnd;
            nBarTracking = nBar;
        }
        else if (msg == WM_LBUTTONUP)
        {
            hwndTracking = 0;
            SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbPos,
                                 infoPtr->flags, vertical, FALSE, FALSE );
        }
        else  /* WM_MOUSEMOVE */
        {
            UINT pos, val;

            if (!PtInRect( &rect, pt )) pos = lastClickPos;
            else pos = vertical ? (pt.y - rect.top) : (pt.x - rect.left);
            if (pos != lastMousePos)
            {
                SCROLL_DrawMovingThumb( hdc, &rect, vertical, arrowSize,
                                 trackThumbPos + lastMousePos - lastClickPos );
                lastMousePos = pos;
                val = SCROLL_GetThumbVal( infoPtr, &rect, vertical,
                                 trackThumbPos + lastMousePos - lastClickPos );
                /* Save tracking info */
                uTrackingPos = trackThumbPos + pos - lastClickPos;
#ifdef WINELIB32
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             MAKEWPARAM(SB_THUMBTRACK,val), hwndCtl );
#else
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_THUMBTRACK, MAKELONG( val, hwndCtl ));
#endif
                SCROLL_DrawMovingThumb( hdc, &rect, vertical,
                                        arrowSize, uTrackingPos );
            }
        }
        break;
        
    case SCROLL_BOTTOM_RECT:
        SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbPos,
                             infoPtr->flags, vertical,
                             FALSE, (hittest == trackHitTest) );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
#ifdef WINELIB32
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_PAGEDOWN, hwndCtl );
#else
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_PAGEDOWN, MAKELONG( 0, hwndCtl ));
#endif
                SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                (FARPROC)0 );
            }
        }
        else KillSystemTimer( hwnd, SCROLL_TIMER );
        break;
        
    case SCROLL_BOTTOM_ARROW:
        SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical,
                           FALSE, (hittest == trackHitTest) );
        if (hittest == trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
#ifdef WINELIB32
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEDOWN, hwndCtl );
#else
                SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEDOWN, MAKELONG( 0, hwndCtl ));
#endif
                SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                (FARPROC)0 );
            }
        }
        else KillSystemTimer( hwnd, SCROLL_TIMER );
        break;
    }

    if (msg == WM_LBUTTONUP)
    {
        if (trackHitTest == SCROLL_THUMB)
        {
            UINT val = SCROLL_GetThumbVal( infoPtr, &rect, vertical,
                                 trackThumbPos + lastMousePos - lastClickPos );
#ifdef WINELIB32
            SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                         MAKEWPARAM(SB_THUMBPOSITION,val), hwndCtl );
#else
            SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                         SB_THUMBPOSITION, MAKELONG( val, hwndCtl ) );
#endif
        }
        else
#ifdef WINELIB32
            SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                         SB_ENDSCROLL, hwndCtl );
#else
            SendMessage( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                         SB_ENDSCROLL, MAKELONG( 0, hwndCtl ) );
#endif
        trackHitTest = SCROLL_NOWHERE;  /* Terminate tracking */
    }

    ReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           ScrollBarWndProc
 */
LONG ScrollBarWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    POINT Pt;
    Pt.x = LOWORD(lParam); Pt.y = HIWORD(lParam);
    /* ^ Can't use MAKEPOINT macro in WINELIB32 */

    switch(message)
    {
    case WM_CREATE:
        {
	    CREATESTRUCT *lpCreat = (CREATESTRUCT *)PTR_SEG_TO_LIN(lParam);
            if (lpCreat->style & SBS_SIZEBOX)
            {
                fprintf( stdnimp, "Unimplemented style SBS_SIZEBOX.\n" );
                return 0;  /* FIXME */
            }
            
	    if (lpCreat->style & SBS_VERT)
            {
                if (lpCreat->style & SBS_LEFTALIGN)
                    MoveWindow( hwnd, lpCreat->x, lpCreat->y,
                                SYSMETRICS_CXVSCROLL+1, lpCreat->cy, FALSE );
                else if (lpCreat->style & SBS_RIGHTALIGN)
                    MoveWindow( hwnd, 
                                lpCreat->x+lpCreat->cx-SYSMETRICS_CXVSCROLL-1,
                                lpCreat->y,
                                SYSMETRICS_CXVSCROLL + 1, lpCreat->cy, FALSE );
            }
            else  /* SBS_HORZ */
            {
                if (lpCreat->style & SBS_TOPALIGN)
                    MoveWindow( hwnd, lpCreat->x, lpCreat->y,
                                lpCreat->cx, SYSMETRICS_CYHSCROLL+1, FALSE );
                else if (lpCreat->style & SBS_BOTTOMALIGN)
                    MoveWindow( hwnd, 
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
        SCROLL_HandleScrollEvent( hwnd, SB_CTL, message, Pt );
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
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );
            SCROLL_DrawScrollBar( hwnd, hdc, SB_CTL );
            EndPaint( hwnd, &ps );
        }
        break;

    case 0x400: /* SB_SETSCROLLPOS */
    case 0x401: /* SB_GETSCROLLPOS */
    case 0x402: /* SB_GETSCROLLRANGE */
    case 0x403: /* SB_ENABLE */
    case 0x404: /* SB_REDRAW */
        fprintf(stdnimp,"ScrollBarWndProc: undocumented message %04x, please report\n", message );

    default:
        return DefWindowProc( hwnd, message, wParam, lParam );
    }
    return 0;
}


/*************************************************************************
 *           SetScrollPos   (USER.62)
 */
int SetScrollPos( HWND hwnd, int nBar, int nPos, BOOL bRedraw )
{
    SCROLLINFO *infoPtr;
    INT oldPos;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return 0;

    dprintf_scroll( stddeb,"SetScrollPos min=%d max=%d pos=%d\n", 
                    infoPtr->MinVal, infoPtr->MaxVal, nPos );

    if (nPos < infoPtr->MinVal) nPos = infoPtr->MinVal;
    else if (nPos > infoPtr->MaxVal) nPos = infoPtr->MaxVal;
    oldPos = infoPtr->CurVal;
    infoPtr->CurVal = nPos;
    if (bRedraw) SCROLL_RefreshScrollBar( hwnd, nBar );
    return oldPos;
}


/*************************************************************************
 *           GetScrollPos   (USER.63)
 */
int GetScrollPos( HWND hwnd, int nBar )
{
    SCROLLINFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return 0;
    return infoPtr->CurVal;
}


/*************************************************************************
 *           SetScrollRange   (USER.64)
 */
void SetScrollRange(HWND hwnd, int nBar, int MinVal, int MaxVal, BOOL bRedraw)
{
    SCROLLINFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return;

    dprintf_scroll( stddeb,"SetScrollRange hwnd=%04x bar=%d min=%d max=%d\n",
                    hwnd, nBar, MinVal, MaxVal );

      /* Invalid range -> range is set to (0,0) */
    if ((MinVal > MaxVal) || ((long)MaxVal - MinVal > 32767L))
        MinVal = MaxVal = 0;
    if (infoPtr->CurVal < MinVal) infoPtr->CurVal = MinVal;
    else if (infoPtr->CurVal > MaxVal) infoPtr->CurVal = MaxVal;
    infoPtr->MinVal = MinVal;
    infoPtr->MaxVal = MaxVal;

      /* Non-client scroll-bar is hidden if min==max */
    if (nBar != SB_CTL) ShowScrollBar( hwnd, nBar, (MinVal != MaxVal) );
    if (bRedraw) SCROLL_RefreshScrollBar( hwnd, nBar );
}

/*************************************************************************
 *	     SCROLL_SetNCSbState
 *
 * This is for CalcChildScroll in windows/mdi.c
 */
DWORD SCROLL_SetNCSbState(WND* wndPtr, int vMin, int vMax, int vPos,
				       int hMin, int hMax, int hPos)
{
  SCROLLINFO  *infoPtr = SCROLL_GetPtrScrollInfo(wndPtr, SB_VERT);
 
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
 *           GetScrollRange   (USER.65)
 */
void GetScrollRange(HWND hwnd, int nBar, LPINT lpMin, LPINT lpMax)
{
    SCROLLINFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return;
    if (lpMin) *lpMin = infoPtr->MinVal;
    if (lpMax) *lpMax = infoPtr->MaxVal;
}


/*************************************************************************
 *           ShowScrollBar   (USER.267)
 */
void ShowScrollBar( HWND hwnd, WORD wBar, BOOL fShow )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return;
    dprintf_scroll( stddeb, "ShowScrollBar: hwnd=%04x bar=%d on=%d\n", hwnd, wBar, fShow );

    switch(wBar)
    {
    case SB_CTL:
        ShowWindow( hwnd, fShow ? SW_SHOW : SW_HIDE );
        return;

    case SB_HORZ:
        if (fShow)
        {
            if (wndPtr->dwStyle & WS_HSCROLL) return;
            wndPtr->dwStyle |= WS_HSCROLL;
        }
        else  /* hide it */
        {
            if (!(wndPtr->dwStyle & WS_HSCROLL)) return;
            wndPtr->dwStyle &= ~WS_HSCROLL;
        }
        break;

    case SB_VERT:
        if (fShow)
        {
            if (wndPtr->dwStyle & WS_VSCROLL) return;
            wndPtr->dwStyle |= WS_VSCROLL;
        }
        else  /* hide it */
        {
            if (!(wndPtr->dwStyle & WS_VSCROLL)) return;
            wndPtr->dwStyle &= ~WS_VSCROLL;
        }
        break;

    case SB_BOTH:
        if (fShow)
        {
            if ((wndPtr->dwStyle & WS_HSCROLL)
                && (wndPtr->dwStyle & WS_VSCROLL)) return;
            wndPtr->dwStyle |= WS_HSCROLL | WS_VSCROLL;
        }
        else  /* hide it */
        {
            if (!(wndPtr->dwStyle & WS_HSCROLL)
                && !(wndPtr->dwStyle & WS_HSCROLL)) return;
            wndPtr->dwStyle &= ~(WS_HSCROLL | WS_VSCROLL);
        }
        break;

    default:
        return;  /* Nothing to do! */
    }
    SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE
                 | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
}


/*************************************************************************
 *           EnableScrollBar   (USER.482)
 */
BOOL EnableScrollBar( HWND hwnd, UINT nBar, UINT flags )
{
    SCROLLINFO *infoPtr;
    HDC hdc;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return FALSE;
    dprintf_scroll( stddeb, "EnableScrollBar: %04x %d %d\n", hwnd, nBar, flags );
    flags &= ESB_DISABLE_BOTH;
    if (infoPtr->flags == flags) return FALSE;
    infoPtr->flags = flags;

      /* Redraw the whole scroll bar */
    hdc = (nBar == SB_CTL) ? GetDC(hwnd) : GetWindowDC(hwnd);
    SCROLL_DrawScrollBar( hwnd, hdc, nBar );
    ReleaseDC( hwnd, hdc );
    return TRUE;
}

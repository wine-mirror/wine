/*
 * Scrollbar control
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994, 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "controls.h"
#include "win.h"
#include "wine/debug.h"
#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);

typedef struct scroll_info SCROLLBAR_INFO, *LPSCROLLBAR_INFO;
typedef struct scroll_bar_win_data SCROLLBAR_WNDDATA;

/* data for window that has (one or two) scroll bars */
typedef struct
{
    SCROLLBAR_INFO horz;
    SCROLLBAR_INFO vert;
} WINSCROLLBAR_INFO, *LPWINSCROLLBAR_INFO;

#define SCROLLBAR_MAGIC 0x5c6011ba

  /* Overlap between arrows and thumb */
#define SCROLL_ARROW_THUMB_OVERLAP 0

/*********************************************************************
 * scrollbar class descriptor
 */
const struct builtin_class_descr SCROLL_builtin_class =
{
    L"ScrollBar",           /* name */
    CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC, /* style  */
    WINPROC_SCROLLBAR,      /* proc */
    sizeof(SCROLLBAR_WNDDATA), /* extra */
    IDC_ARROW,              /* cursor */
    0                       /* brush */
};

/***********************************************************************
 *           SCROLL_GetInternalInfo

 * Returns pointer to internal SCROLLBAR_INFO structure for nBar
 * or NULL if failed (f.i. scroll bar does not exist yet)
 * If alloc is TRUE and the struct does not exist yet, create it.
 */
SCROLLBAR_INFO *SCROLL_GetInternalInfo( HWND hwnd, INT nBar, BOOL alloc )
{
    SCROLLBAR_INFO *infoPtr = NULL;
    WND *wndPtr = WIN_GetPtr( hwnd );

    if (!wndPtr || wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return NULL;
    switch(nBar)
    {
        case SB_HORZ:
            if (wndPtr->pScroll) infoPtr = &((LPWINSCROLLBAR_INFO)wndPtr->pScroll)->horz;
            break;
        case SB_VERT:
            if (wndPtr->pScroll) infoPtr = &((LPWINSCROLLBAR_INFO)wndPtr->pScroll)->vert;
            break;
        case SB_CTL:
            if (wndPtr->cbWndExtra >= sizeof(SCROLLBAR_WNDDATA))
            {
                SCROLLBAR_WNDDATA *data = (SCROLLBAR_WNDDATA*)wndPtr->wExtra;
                if (data->magic == SCROLLBAR_MAGIC)
                    infoPtr = &data->info;
            }
            if (!infoPtr) WARN("window is not a scrollbar control\n");
            break;
        case SB_BOTH:
            WARN("with SB_BOTH\n");
            break;
    }

    if (!infoPtr && alloc)
    {
        WINSCROLLBAR_INFO *winInfoPtr;

        if (nBar != SB_HORZ && nBar != SB_VERT)
            WARN("Cannot initialize nBar=%d\n",nBar);
        else if ((winInfoPtr = HeapAlloc( GetProcessHeap(), 0, sizeof(WINSCROLLBAR_INFO) )))
        {
            /* Set default values */
            winInfoPtr->horz.minVal = 0;
            winInfoPtr->horz.curVal = 0;
            winInfoPtr->horz.page = 0;
            /* From MSDN and our own tests:
             * max for a standard scroll bar is 100 by default. */
            winInfoPtr->horz.maxVal = 100;
            winInfoPtr->horz.flags  = ESB_ENABLE_BOTH;
            winInfoPtr->vert = winInfoPtr->horz;
            wndPtr->pScroll = winInfoPtr;
            infoPtr = nBar == SB_HORZ ? &winInfoPtr->horz : &winInfoPtr->vert;
        }
    }
    WIN_ReleasePtr( wndPtr );
    return infoPtr;
}


/***********************************************************************
 *           SCROLL_DrawArrows
 *
 * Draw the scroll bar arrows.
 */
static void SCROLL_DrawArrows( HDC hdc, UINT flags,
                               RECT *rect, INT arrowSize, BOOL vertical,
                               BOOL top_pressed, BOOL bottom_pressed )
{
  RECT r;

  r = *rect;
  if( vertical )
    r.bottom = r.top + arrowSize;
  else
    r.right = r.left + arrowSize;

  DrawFrameControl( hdc, &r, DFC_SCROLL,
		    (vertical ? DFCS_SCROLLUP : DFCS_SCROLLLEFT)
		    | (top_pressed ? (DFCS_PUSHED | DFCS_FLAT) : 0 )
		    | (flags & ESB_DISABLE_LTUP ? DFCS_INACTIVE : 0 ) );

  r = *rect;
  if( vertical )
    r.top = r.bottom-arrowSize;
  else
    r.left = r.right-arrowSize;

  DrawFrameControl( hdc, &r, DFC_SCROLL,
		    (vertical ? DFCS_SCROLLDOWN : DFCS_SCROLLRIGHT)
		    | (bottom_pressed ? (DFCS_PUSHED | DFCS_FLAT) : 0 )
		    | (flags & ESB_DISABLE_RTDN ? DFCS_INACTIVE : 0) );
}

/***********************************************************************
 *           SCROLL_DrawInterior
 *
 * Draw the scroll bar interior (everything except the arrows).
 */
static void SCROLL_DrawInterior( HWND hwnd, HDC hdc, INT nBar,
                                 RECT *rect, INT arrowSize,
                                 INT thumbSize, INT thumbPos, BOOL vertical,
                                 BOOL top_selected, BOOL bottom_selected )
{
    RECT r;
    HPEN hSavePen;
    HBRUSH hSaveBrush,hBrush;

      /* Select the correct brush and pen */

    /* Only scrollbar controls send WM_CTLCOLORSCROLLBAR.
     * The window-owned scrollbars need to call DEFWND_ControlColor
     * to correctly setup default scrollbar colors
     */
    if (nBar == SB_CTL) {
        hBrush = (HBRUSH)SendMessageW( GetParent(hwnd), WM_CTLCOLORSCROLLBAR,
                                       (WPARAM)hdc,(LPARAM)hwnd);
    } else {
        hBrush = DEFWND_ControlColor( hdc, CTLCOLOR_SCROLLBAR );
    }
    hSavePen = SelectObject( hdc, SYSCOLOR_GetPen(COLOR_WINDOWFRAME) );
    hSaveBrush = SelectObject( hdc, hBrush );

      /* Calculate the scroll rectangle */

    r = *rect;
    if (vertical)
    {
        r.top    += arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
        r.bottom -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
    }
    else
    {
        r.left  += arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
        r.right -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
    }

      /* Draw the scroll bar frame */

      /* Draw the scroll rectangles and thumb */

    if (!thumbPos)  /* No thumb to draw */
    {
        PatBlt( hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, PATCOPY );

        /* cleanup and return */
        SelectObject( hdc, hSavePen );
        SelectObject( hdc, hSaveBrush );
        return;
    }

    if (vertical)
    {
        PatBlt( hdc, r.left, r.top, r.right - r.left,
                thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP),
                top_selected ? 0x0f0000 : PATCOPY );
        r.top += thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
        PatBlt( hdc, r.left, r.top + thumbSize, r.right - r.left,
                r.bottom - r.top - thumbSize,
                bottom_selected ? 0x0f0000 : PATCOPY );
        r.bottom = r.top + thumbSize;
    }
    else  /* horizontal */
    {
        PatBlt( hdc, r.left, r.top,
                thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP),
                r.bottom - r.top, top_selected ? 0x0f0000 : PATCOPY );
        r.left += thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
        PatBlt( hdc, r.left + thumbSize, r.top, r.right - r.left - thumbSize,
                r.bottom - r.top, bottom_selected ? 0x0f0000 : PATCOPY );
        r.right = r.left + thumbSize;
    }

      /* Draw the thumb */

    SelectObject( hdc, GetSysColorBrush(COLOR_BTNFACE) );
    Rectangle( hdc, r.left+1, r.top+1, r.right-1, r.bottom-1 );
    DrawEdge( hdc, &r, EDGE_RAISED, BF_RECT );

    /* cleanup */
    SelectObject( hdc, hSavePen );
    SelectObject( hdc, hSaveBrush );
}

void WINAPI USER_ScrollBarDraw( HWND hwnd, HDC hdc, INT nBar, enum SCROLL_HITTEST hit_test,
                                const struct SCROLL_TRACKING_INFO *tracking_info, BOOL arrows,
                                BOOL interior, RECT *rect, UINT enable_flags, INT arrowSize,
                                INT thumbPos, INT thumbSize, BOOL vertical )
{
    if (nBar == SB_CTL)
    {
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );

        if (style & SBS_SIZEGRIP)
        {
            RECT rc = *rect;

            FillRect( hdc, &rc, GetSysColorBrush( COLOR_BTNFACE ) );
            rc.left = max( rc.left, rc.right - GetSystemMetrics( SM_CXVSCROLL ) - 1 );
            rc.top = max( rc.top, rc.bottom - GetSystemMetrics( SM_CYHSCROLL ) - 1 );
            DrawFrameControl( hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP );
            return;
        }

        if (style & SBS_SIZEBOX)
        {
            FillRect( hdc, rect, GetSysColorBrush( COLOR_BTNFACE ) );
            return;
        }
    }

    /* Draw the arrows */
    if (arrows && arrowSize)
    {
        if (vertical == tracking_info->vertical && GetCapture() == hwnd)
            SCROLL_DrawArrows( hdc, enable_flags, rect, arrowSize, vertical,
                               hit_test == tracking_info->hit_test && hit_test == SCROLL_TOP_ARROW,
                               hit_test == tracking_info->hit_test && hit_test == SCROLL_BOTTOM_ARROW );
	else
            SCROLL_DrawArrows( hdc, enable_flags, rect, arrowSize, vertical, FALSE, FALSE );
    }

    if (interior)
    {
        if (vertical == tracking_info->vertical && GetCapture() == hwnd)
        {
            SCROLL_DrawInterior( hwnd, hdc, nBar, rect, arrowSize, thumbSize, thumbPos, vertical,
                                 hit_test == tracking_info->hit_test && hit_test == SCROLL_TOP_RECT,
                                 hit_test == tracking_info->hit_test && hit_test == SCROLL_BOTTOM_RECT );
        }
        else
        {
            SCROLL_DrawInterior( hwnd, hdc, nBar, rect, arrowSize, thumbSize, thumbPos,
                                 vertical, FALSE, FALSE );
        }
    }

    /* if scroll bar has focus, reposition the caret */
    if(hwnd==GetFocus() && (nBar==SB_CTL))
    {
        if (!vertical)
        {
            SetCaretPos(thumbPos + 1, rect->top + 1);
        }
        else
        {
            SetCaretPos(rect->top + 1, thumbPos + 1);
        }
    }
}

/***********************************************************************
 *           SCROLL_HandleKbdEvent
 *
 * Handle a keyboard event (only for SB_CTL scrollbars with focus).
 *
 * PARAMS
 *    hwnd   [I] Handle of window with scrollbar(s)
 *    wParam [I] Variable input including enable state
 *    lParam [I] Variable input including input point
 */
static void SCROLL_HandleKbdEvent(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p wParam=%Id lParam=%Id\n", hwnd, wParam, lParam);

    /* hide caret on first KEYDOWN to prevent flicker */
    if ((lParam & PFD_DOUBLEBUFFER_DONTCARE) == 0)
        NtUserHideCaret( hwnd );

    switch(wParam)
    {
    case VK_PRIOR: wParam = SB_PAGEUP; break;
    case VK_NEXT:  wParam = SB_PAGEDOWN; break;
    case VK_HOME:  wParam = SB_TOP; break;
    case VK_END:   wParam = SB_BOTTOM; break;
    case VK_UP:    wParam = SB_LINEUP; break;
    case VK_DOWN:  wParam = SB_LINEDOWN; break;
    case VK_LEFT:  wParam = SB_LINEUP; break;
    case VK_RIGHT: wParam = SB_LINEDOWN; break;
    default: return;
    }
    SendMessageW(GetParent(hwnd),
        ((GetWindowLongW( hwnd, GWL_STYLE ) & SBS_VERT) ?
            WM_VSCROLL : WM_HSCROLL), wParam, (LPARAM)hwnd);
}


/*************************************************************************
 *           SCROLL_GetScrollPos
 *
 *  Internal helper for the API function
 *
 * PARAMS
 *    hwnd [I]  Handle of window with scrollbar(s)
 *    nBar [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 */
static INT SCROLL_GetScrollPos(HWND hwnd, INT nBar)
{
    LPSCROLLBAR_INFO infoPtr = SCROLL_GetInternalInfo(hwnd, nBar, FALSE);
    return infoPtr ? infoPtr->curVal: 0;
}


/*************************************************************************
 *           SCROLL_GetScrollRange
 *
 *  Internal helper for the API function
 *
 * PARAMS
 *    hwnd  [I]  Handle of window with scrollbar(s)
 *    nBar  [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 *    lpMin [O]  Where to store minimum value
 *    lpMax [O]  Where to store maximum value
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
static BOOL SCROLL_GetScrollRange(HWND hwnd, INT nBar, LPINT lpMin, LPINT lpMax)
{
    LPSCROLLBAR_INFO infoPtr = SCROLL_GetInternalInfo(hwnd, nBar, FALSE);

    if (lpMin) *lpMin = infoPtr ? infoPtr->minVal : 0;
    if (lpMax) *lpMax = infoPtr ? infoPtr->maxVal : 0;

    return TRUE;
}


LRESULT WINAPI USER_ScrollBarProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    if (!IsWindow( hwnd )) return 0;

    switch(message)
    {
    case WM_KEYDOWN:
        SCROLL_HandleKbdEvent(hwnd, wParam, lParam);
        break;

    case WM_KEYUP:
        NtUserShowCaret( hwnd );
        break;

    case WM_ENABLE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_CREATE:
    case WM_ERASEBKGND:
    case WM_GETDLGCODE:
    case WM_PAINT:
    case SBM_SETRANGEREDRAW:
    case SBM_SETRANGE:
    case SBM_GETSCROLLINFO:
    case SBM_GETSCROLLBARINFO:
    case SBM_SETSCROLLINFO:
        return NtUserMessageCall( hwnd, message, wParam, lParam, 0, NtUserScrollBarWndProc, !unicode );

    case WM_SETCURSOR:
        if (GetWindowLongW( hwnd, GWL_STYLE ) & SBS_SIZEGRIP)
        {
            ULONG_PTR cursor = (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) ? IDC_SIZENESW : IDC_SIZENWSE;
            return (LRESULT)NtUserSetCursor( LoadCursorA( 0, (LPSTR)cursor ));
        }
        return DefWindowProcW( hwnd, message, wParam, lParam );

    case SBM_SETPOS:
        return SetScrollPos( hwnd, SB_CTL, wParam, (BOOL)lParam );

    case SBM_GETPOS:
       return SCROLL_GetScrollPos(hwnd, SB_CTL);

    case SBM_GETRANGE:
        return SCROLL_GetScrollRange(hwnd, SB_CTL, (LPINT)wParam, (LPINT)lParam);

    case SBM_ENABLE_ARROWS:
        return NtUserEnableScrollBar( hwnd, SB_CTL, wParam );

    case 0x00e5:
    case 0x00e7:
    case 0x00e8:
    case 0x00ec:
    case 0x00ed:
    case 0x00ee:
    case 0x00ef:
        ERR("unknown Win32 msg %04x wp=%08Ix lp=%08Ix\n",
		    message, wParam, lParam );
        break;

    default:
        if (message >= WM_USER)
            WARN("unknown msg %04x wp=%04Ix lp=%08Ix\n",
			 message, wParam, lParam );
        if (unicode)
            return DefWindowProcW( hwnd, message, wParam, lParam );
        else
            return DefWindowProcA( hwnd, message, wParam, lParam );
    }
    return 0;
}

/***********************************************************************
 *           ScrollBarWndProc_common
 */
LRESULT ScrollBarWndProc_common( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
     return user_api->pScrollBarWndProc( hwnd, msg, wParam, lParam, unicode );
}


/*************************************************************************
 *           GetScrollInfo   (USER32.@)
 *
 * GetScrollInfo can be used to retrieve the position, upper bound,
 * lower bound, and page size of a scrollbar control.
 *
 * PARAMS
 *  hwnd [I]  Handle of window with scrollbar(s)
 *  nBar [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 *  info [IO] fMask specifies which values to retrieve
 *
 * RETURNS
 *  TRUE if SCROLLINFO is filled
 *  ( if nBar is SB_CTL, GetScrollInfo returns TRUE even if nothing
 *  is filled)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetScrollInfo(HWND hwnd, INT nBar, LPSCROLLINFO info)
{
    TRACE("hwnd=%p nBar=%d info=%p\n", hwnd, nBar, info);

    /* Refer SB_CTL requests to the window */
    if (nBar == SB_CTL)
    {
        SendMessageW(hwnd, SBM_GETSCROLLINFO, 0, (LPARAM)info);
        return TRUE;
    }
    return NtUserGetScrollInfo( hwnd, nBar, info );
}


/*************************************************************************
 *           SetScrollPos   (USER32.@)
 *
 * Sets the current position of the scroll thumb.
 *
 * PARAMS
 *    hwnd    [I]  Handle of window with scrollbar(s)
 *    nBar    [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 *    nPos    [I]  New value
 *    bRedraw [I]  Should scrollbar be redrawn afterwards?
 *
 * RETURNS
 *    Success: Scrollbar position
 *    Failure: 0
 *
 * REMARKS
 *    Note the ambiguity when 0 is returned.  Use GetLastError
 *    to make sure there was an error (and to know which one).
 */
INT WINAPI DECLSPEC_HOTPATCH SetScrollPos( HWND hwnd, INT nBar, INT nPos, BOOL bRedraw)
{
    SCROLLINFO info;
    SCROLLBAR_INFO *infoPtr;
    INT oldPos = 0;

    if ((infoPtr = SCROLL_GetInternalInfo( hwnd, nBar, FALSE ))) oldPos = infoPtr->curVal;
    info.cbSize = sizeof(info);
    info.nPos   = nPos;
    info.fMask  = SIF_POS;
    NtUserSetScrollInfo( hwnd, nBar, &info, bRedraw );
    return oldPos;
}


/*************************************************************************
 *           GetScrollPos   (USER32.@)
 *
 * Gets the current position of the scroll thumb.
 *
 * PARAMS
 *    hwnd    [I]  Handle of window with scrollbar(s)
 *    nBar    [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 *
 * RETURNS
 *    Success: Current position
 *    Failure: 0
 *
 * REMARKS
 *    There is ambiguity when 0 is returned.  Use GetLastError
 *    to make sure there was an error (and to know which one).
 */
INT WINAPI DECLSPEC_HOTPATCH GetScrollPos(HWND hwnd, INT nBar)
{
    TRACE("hwnd=%p nBar=%d\n", hwnd, nBar);

    /* Refer SB_CTL requests to the window */
    if (nBar == SB_CTL)
        return SendMessageW(hwnd, SBM_GETPOS, 0, 0);
    else
        return SCROLL_GetScrollPos(hwnd, nBar);
}


/*************************************************************************
 *           SetScrollRange   (USER32.@)
 * The SetScrollRange function sets the minimum and maximum scroll box positions 
 * If nMinPos and nMaxPos is the same value, the scroll bar will hide
 *
 * Sets the range of the scroll bar.
 *
 * PARAMS
 *    hwnd    [I]  Handle of window with scrollbar(s)
 *    nBar    [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 *    minVal  [I]  New minimum value
 *    maxVal  [I]  New Maximum value
 *    bRedraw [I]  Should scrollbar be redrawn afterwards?
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetScrollRange(HWND hwnd, INT nBar, INT minVal, INT maxVal, BOOL bRedraw)
{
    SCROLLINFO info;
 
    TRACE("hwnd=%p nBar=%d min=%d max=%d, bRedraw=%d\n", hwnd, nBar, minVal, maxVal, bRedraw);

    info.cbSize = sizeof(info);
    info.fMask  = SIF_RANGE;
    info.nMin   = minVal;
    info.nMax   = maxVal;
    NtUserSetScrollInfo( hwnd, nBar, &info, bRedraw );
    return TRUE;
}


/*************************************************************************
 *           GetScrollRange   (USER32.@)
 *
 * Gets the range of the scroll bar.
 *
 * PARAMS
 *    hwnd    [I]  Handle of window with scrollbar(s)
 *    nBar    [I]  One of SB_HORZ, SB_VERT, or SB_CTL
 *    lpMin   [O]  Where to store minimum value
 *    lpMax   [O]  Where to store maximum value
 *
 * RETURNS
 *    TRUE if values is filled
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetScrollRange(HWND hwnd, INT nBar, LPINT lpMin, LPINT lpMax)
{
    TRACE("hwnd=%p nBar=%d lpMin=%p lpMax=%p\n", hwnd, nBar, lpMin, lpMax);

    /* Refer SB_CTL requests to the window */
    if (nBar == SB_CTL)
        SendMessageW(hwnd, SBM_GETRANGE, (WPARAM)lpMin, (LPARAM)lpMax);
    else
        SCROLL_GetScrollRange(hwnd, nBar, lpMin, lpMax);

    return TRUE;
}

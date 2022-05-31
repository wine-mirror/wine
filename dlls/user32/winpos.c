/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995, 1996, 1999 Alex Korobka
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

#include <string.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "winerror.h"
#include "controls.h"
#include "win.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))


/***********************************************************************
 *		SwitchToThisWindow (USER32.@)
 */
void WINAPI SwitchToThisWindow( HWND hwnd, BOOL alt_tab )
{
    if (IsIconic( hwnd )) NtUserShowWindow( hwnd, SW_RESTORE );
    else BringWindowToTop( hwnd );
}


/***********************************************************************
 *		GetWindowRect (USER32.@)
 */
BOOL WINAPI GetWindowRect( HWND hwnd, LPRECT rect )
{
    BOOL ret = NtUserGetWindowRect( hwnd, rect );
    if (ret) TRACE( "hwnd %p %s\n", hwnd, wine_dbgstr_rect(rect) );
    return ret;
}


/***********************************************************************
 *		GetWindowRgn (USER32.@)
 */
int WINAPI GetWindowRgn( HWND hwnd, HRGN hrgn )
{
    return NtUserGetWindowRgnEx( hwnd, hrgn, 0 );
}

/***********************************************************************
 *		GetWindowRgnBox (USER32.@)
 */
int WINAPI GetWindowRgnBox( HWND hwnd, LPRECT prect )
{
    int ret = ERROR;
    HRGN hrgn;

    if (!prect)
        return ERROR;

    if ((hrgn = CreateRectRgn(0, 0, 0, 0)))
    {
        if ((ret = GetWindowRgn( hwnd, hrgn )) != ERROR )
            ret = GetRgnBox( hrgn, prect );
        DeleteObject(hrgn);
    }

    return ret;
}


/***********************************************************************
 *		GetClientRect (USER32.@)
 */
BOOL WINAPI GetClientRect( HWND hwnd, LPRECT rect )
{
    return NtUserGetClientRect( hwnd, rect );
}


/*******************************************************************
 *		WindowFromPoint (USER32.@)
 */
HWND WINAPI WindowFromPoint( POINT pt )
{
    return NtUserWindowFromPoint( pt.x, pt.y );
}


/*******************************************************************
 *		ChildWindowFromPoint (USER32.@)
 */
HWND WINAPI ChildWindowFromPoint( HWND parent, POINT pt )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y, CWP_ALL );
}

/*******************************************************************
 *		RealChildWindowFromPoint (USER32.@)
 */
HWND WINAPI RealChildWindowFromPoint( HWND parent, POINT pt )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y,
                                         CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE );
}

/*******************************************************************
 *		ChildWindowFromPointEx (USER32.@)
 */
HWND WINAPI ChildWindowFromPointEx( HWND parent, POINT pt, UINT flags )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y, flags );
}


/*******************************************************************
 *		MapWindowPoints (USER32.@)
 */
INT WINAPI MapWindowPoints( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count )
{
    return NtUserMapWindowPoints( hwnd_from, hwnd_to, points, count );
}


/*******************************************************************
 *		ClientToScreen (USER32.@)
 */
BOOL WINAPI ClientToScreen( HWND hwnd, POINT *pt )
{
    return NtUserClientToScreen( hwnd, pt );
}


/*******************************************************************
 *		ScreenToClient (USER32.@)
 */
BOOL WINAPI ScreenToClient( HWND hwnd, POINT *pt )
{
    return NtUserScreenToClient( hwnd, pt );
}


/***********************************************************************
 *		IsIconic (USER32.@)
 */
BOOL WINAPI IsIconic(HWND hWnd)
{
    return (GetWindowLongW( hWnd, GWL_STYLE ) & WS_MINIMIZE) != 0;
}


/***********************************************************************
 *		IsZoomed (USER32.@)
 */
BOOL WINAPI IsZoomed(HWND hWnd)
{
    return (GetWindowLongW( hWnd, GWL_STYLE ) & WS_MAXIMIZE) != 0;
}


/*******************************************************************
 *		AllowSetForegroundWindow (USER32.@)
 */
BOOL WINAPI AllowSetForegroundWindow( DWORD procid )
{
    /* FIXME: If Win98/2000 style SetForegroundWindow behavior is
     * implemented, then fix this function. */
    return TRUE;
}


/*******************************************************************
 *		LockSetForegroundWindow (USER32.@)
 */
BOOL WINAPI LockSetForegroundWindow( UINT lockcode )
{
    /* FIXME: If Win98/2000 style SetForegroundWindow behavior is
     * implemented, then fix this function. */
    return TRUE;
}


/***********************************************************************
 *		BringWindowToTop (USER32.@)
 */
BOOL WINAPI BringWindowToTop( HWND hwnd )
{
    return NtUserSetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
}


/***********************************************************************
 *		GetInternalWindowPos (USER32.@)
 */
UINT WINAPI GetInternalWindowPos( HWND hwnd, LPRECT rectWnd,
                                      LPPOINT ptIcon )
{
    WINDOWPLACEMENT wndpl;

    wndpl.length = sizeof(wndpl);
    if (NtUserGetWindowPlacement( hwnd, &wndpl ))
    {
	if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
	if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
	return wndpl.showCmd;
    }
    return 0;
}


/***********************************************************************
 *		AnimateWindow (USER32.@)
 *		Shows/Hides a window with an animation
 *		NO ANIMATION YET
 */
BOOL WINAPI AnimateWindow(HWND hwnd, DWORD dwTime, DWORD dwFlags)
{
	FIXME("partial stub\n");

	/* If trying to show/hide and it's already   *
	 * shown/hidden or invalid window, fail with *
	 * invalid parameter                         */
	if(!IsWindow(hwnd) ||
	   (IsWindowVisible(hwnd) && !(dwFlags & AW_HIDE)) ||
	   (!IsWindowVisible(hwnd) && (dwFlags & AW_HIDE)))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	NtUserShowWindow( hwnd, (dwFlags & AW_HIDE) ? SW_HIDE : ((dwFlags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA) );

	return TRUE;
}


/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static BOOL can_activate_window( HWND hwnd )
{
    LONG style;

    if (!hwnd) return FALSE;
    style = GetWindowLongW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return !(style & WS_DISABLED);
}


/*******************************************************************
 *         WINPOS_ActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
void WINPOS_ActivateOtherWindow(HWND hwnd)
{
    HWND hwndTo, fg;

    if ((GetWindowLongW( hwnd, GWL_STYLE ) & WS_POPUP) && (hwndTo = GetWindow( hwnd, GW_OWNER )))
    {
        hwndTo = NtUserGetAncestor( hwndTo, GA_ROOT );
        if (can_activate_window( hwndTo )) goto done;
    }

    hwndTo = hwnd;
    for (;;)
    {
        if (!(hwndTo = GetWindow( hwndTo, GW_HWNDNEXT ))) break;
        if (can_activate_window( hwndTo )) goto done;
    }

    hwndTo = GetTopWindow( 0 );
    for (;;)
    {
        if (hwndTo == hwnd)
        {
            hwndTo = 0;
            break;
        }
        if (can_activate_window( hwndTo )) goto done;
        if (!(hwndTo = GetWindow( hwndTo, GW_HWNDNEXT ))) break;
    }

 done:
    fg = NtUserGetForegroundWindow();
    TRACE("win = %p fg = %p\n", hwndTo, fg);
    if (!fg || (hwnd == fg))
    {
        if (SetForegroundWindow( hwndTo )) return;
    }
    if (!NtUserSetActiveWindow( hwndTo )) NtUserSetActiveWindow( 0 );
}


/***********************************************************************
 *		BeginDeferWindowPos (USER32.@)
 */
HDWP WINAPI BeginDeferWindowPos( INT count )
{
    return NtUserBeginDeferWindowPos( count );
}


/***********************************************************************
 *		DeferWindowPos (USER32.@)
 */
HDWP WINAPI DeferWindowPos( HDWP hdwp, HWND hwnd, HWND after, INT x, INT y,
                            INT cx, INT cy, UINT flags )
{
    return NtUserDeferWindowPosAndBand( hdwp, hwnd, after, x, y, cx, cy, flags, 0, 0 );
}


/***********************************************************************
 *		EndDeferWindowPos (USER32.@)
 */
BOOL WINAPI EndDeferWindowPos( HDWP hdwp )
{
    return NtUserEndDeferWindowPosEx( hdwp, FALSE );
}


/***********************************************************************
 *		ArrangeIconicWindows (USER32.@)
 */
UINT WINAPI ArrangeIconicWindows( HWND parent )
{
    return NtUserArrangeIconicWindows( parent );
}


/***********************************************************************
 *           draw_moving_frame
 *
 * Draw the frame used when moving or resizing window.
 */
static void draw_moving_frame( HWND parent, HDC hdc, RECT *screen_rect, BOOL thickframe )
{
    RECT rect = *screen_rect;

    if (parent) MapWindowPoints( 0, parent, (POINT *)&rect, 2 );
    if (thickframe)
    {
        const int width = GetSystemMetrics(SM_CXFRAME);
        const int height = GetSystemMetrics(SM_CYFRAME);

        HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
        PatBlt( hdc, rect.left, rect.top,
                rect.right - rect.left - width, height, PATINVERT );
        PatBlt( hdc, rect.left, rect.top + height, width,
                rect.bottom - rect.top - height, PATINVERT );
        PatBlt( hdc, rect.left + width, rect.bottom - 1,
                rect.right - rect.left - width, -height, PATINVERT );
        PatBlt( hdc, rect.right - 1, rect.top, -width,
                rect.bottom - rect.top - height, PATINVERT );
        SelectObject( hdc, hbrush );
    }
    else DrawFocusRect( hdc, &rect );
}


/***********************************************************************
 *           start_size_move
 *
 * Initialization of a move or resize, when initiated from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG start_size_move( HWND hwnd, WPARAM wParam, POINT *capturePoint, LONG style )
{
    LONG hittest = 0;
    POINT pt;
    MSG msg;
    RECT rectWindow;

    GetWindowRect( hwnd, &rectWindow );

    if ((wParam & 0xfff0) == SC_MOVE)
    {
        /* Move pointer at the center of the caption */
        RECT rect = rectWindow;
        /* Note: to be exactly centered we should take the different types
         * of border into account, but it shouldn't make more than a few pixels
         * of difference so let's not bother with that */
        rect.top += GetSystemMetrics(SM_CYBORDER);
        if (style & WS_SYSMENU)
            rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MINIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MAXIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        pt.x = (rect.right + rect.left) / 2;
        pt.y = rect.top + GetSystemMetrics(SM_CYSIZE)/2;
        hittest = HTCAPTION;
        *capturePoint = pt;
    }
    else  /* SC_SIZE */
    {
        NtUserSetCursor( LoadCursorW( 0, (LPWSTR)IDC_SIZEALL ) );
        pt.x = pt.y = 0;
        while(!hittest)
        {
            if (!GetMessageW( &msg, 0, 0, 0 )) return 0;
            if (NtUserCallMsgFilter( &msg, MSGF_SIZE )) continue;

            switch(msg.message)
            {
            case WM_MOUSEMOVE:
                pt.x = min( max( msg.pt.x, rectWindow.left ), rectWindow.right - 1 );
                pt.y = min( max( msg.pt.y, rectWindow.top ), rectWindow.bottom - 1 );
                hittest = SendMessageW( hwnd, WM_NCHITTEST, 0, MAKELONG( pt.x, pt.y ) );
                if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT)) hittest = 0;
                break;

            case WM_LBUTTONUP:
                return 0;

            case WM_KEYDOWN:
                switch(msg.wParam)
                {
                case VK_UP:
                    hittest = HTTOP;
                    pt.x =(rectWindow.left+rectWindow.right)/2;
                    pt.y = rectWindow.top + GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_DOWN:
                    hittest = HTBOTTOM;
                    pt.x =(rectWindow.left+rectWindow.right)/2;
                    pt.y = rectWindow.bottom - GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_LEFT:
                    hittest = HTLEFT;
                    pt.x = rectWindow.left + GetSystemMetrics(SM_CXFRAME) / 2;
                    pt.y =(rectWindow.top+rectWindow.bottom)/2;
                    break;
                case VK_RIGHT:
                    hittest = HTRIGHT;
                    pt.x = rectWindow.right - GetSystemMetrics(SM_CXFRAME) / 2;
                    pt.y =(rectWindow.top+rectWindow.bottom)/2;
                    break;
                case VK_RETURN:
                case VK_ESCAPE:
                    return 0;
                }
                break;
            default:
                TranslateMessage( &msg );
                DispatchMessageW( &msg );
                break;
            }
        }
        *capturePoint = pt;
    }
    NtUserSetCursorPos( pt.x, pt.y );
    SendMessageW( hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           WINPOS_SysCommandSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
void WINPOS_SysCommandSizeMove( HWND hwnd, WPARAM wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect, origRect;
    HDC hdc;
    HWND parent;
    LONG hittest = (LONG)(wParam & 0x0f);
    WPARAM syscommand = wParam & 0xfff0;
    MINMAXINFO minmax;
    POINT capturePoint, pt;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL    thickframe = HAS_THICKFRAME( style );
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = TRUE;
    HMONITOR mon = 0;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd)) return;

    pt.x = (short)LOWORD(dwPoint);
    pt.y = (short)HIWORD(dwPoint);
    capturePoint = pt;
    NtUserClipCursor( NULL );

    TRACE("hwnd %p command %04Ix, hittest %ld, pos %ld,%ld\n",
          hwnd, syscommand, hittest, pt.x, pt.y);

    if (syscommand == SC_MOVE)
    {
        if (!hittest) hittest = start_size_move( hwnd, wParam, &capturePoint, style );
        if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
        if ( hittest && (syscommand != SC_MOUSEMENU) )
            hittest += (HTLEFT - WMSZ_LEFT);
        else
        {
            set_capture_window( hwnd, GUI_INMOVESIZE, NULL );
            hittest = start_size_move( hwnd, wParam, &capturePoint, style );
            if (!hittest)
            {
                set_capture_window( 0, GUI_INMOVESIZE, NULL );
                return;
            }
        }
    }

      /* Get min/max info */

    minmax = NtUserGetMinMaxInfo( hwnd );
    WIN_GetRectangles( hwnd, COORDS_PARENT, &sizingRect, NULL );
    origRect = sizingRect;
    if (style & WS_CHILD)
    {
        parent = GetParent(hwnd);
        GetClientRect( parent, &mouseRect );
        MapWindowPoints( parent, 0, (LPPOINT)&mouseRect, 2 );
        MapWindowPoints( parent, 0, (LPPOINT)&sizingRect, 2 );
    }
    else
    {
        parent = 0;
        mouseRect = get_virtual_screen_rect();
        mon = MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
    }

    if (ON_LEFT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.right-minmax.ptMaxTrackSize.x+capturePoint.x-sizingRect.left );
        mouseRect.right = min( mouseRect.right, sizingRect.right-minmax.ptMinTrackSize.x+capturePoint.x-sizingRect.left );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.left+minmax.ptMinTrackSize.x+capturePoint.x-sizingRect.right );
        mouseRect.right = min( mouseRect.right, sizingRect.left+minmax.ptMaxTrackSize.x+capturePoint.x-sizingRect.right );
    }
    if (ON_TOP_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.bottom-minmax.ptMaxTrackSize.y+capturePoint.y-sizingRect.top );
        mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minmax.ptMinTrackSize.y+capturePoint.y-sizingRect.top);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.top+minmax.ptMinTrackSize.y+capturePoint.y-sizingRect.bottom );
        mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+minmax.ptMaxTrackSize.y+capturePoint.y-sizingRect.bottom );
    }

    /* Retrieve a default cache DC (without using the window style) */
    hdc = NtUserGetDCEx( parent, 0, DCX_CACHE );

    /* we only allow disabling the full window drag for child windows */
    if (parent) SystemParametersInfoW( SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0 );

    /* repaint the window before moving it around */
    NtUserRedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    SendMessageW( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
    set_capture_window( hwnd, GUI_INMOVESIZE, NULL );

    while(1)
    {
        int dx = 0, dy = 0;

        if (!GetMessageW( &msg, 0, 0, 0 )) break;
        if (NtUserCallMsgFilter( &msg, MSGF_SIZE )) continue;

        /* Exit on button-up, Return, or Esc */
        if ((msg.message == WM_LBUTTONUP) ||
            ((msg.message == WM_KEYDOWN) &&
             ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

        if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
            continue;  /* We are not interested in other messages */
        }

        pt = msg.pt;

        if (msg.message == WM_KEYDOWN) switch(msg.wParam)
        {
        case VK_UP:    pt.y -= 8; break;
        case VK_DOWN:  pt.y += 8; break;
        case VK_LEFT:  pt.x -= 8; break;
        case VK_RIGHT: pt.x += 8; break;
        }

        pt.x = max( pt.x, mouseRect.left );
        pt.x = min( pt.x, mouseRect.right - 1 );
        pt.y = max( pt.y, mouseRect.top );
        pt.y = min( pt.y, mouseRect.bottom - 1 );

        if (!parent)
        {
            HMONITOR newmon;
            MONITORINFO info;

            if ((newmon = MonitorFromPoint( pt, MONITOR_DEFAULTTONULL )))
                mon = newmon;

            info.cbSize = sizeof(info);
            if (mon && GetMonitorInfoW( mon, &info ))
            {
                pt.x = max( pt.x, info.rcWork.left );
                pt.x = min( pt.x, info.rcWork.right - 1 );
                pt.y = max( pt.y, info.rcWork.top );
                pt.y = min( pt.y, info.rcWork.bottom - 1 );
            }
        }

        dx = pt.x - capturePoint.x;
        dy = pt.y - capturePoint.y;

        if (dx || dy)
        {
            if( !moved )
            {
                moved = TRUE;
                if (!DragFullWindows)
                    draw_moving_frame( parent, hdc, &sizingRect, thickframe );
            }

            if (msg.message == WM_KEYDOWN) NtUserSetCursorPos( pt.x, pt.y );
            else
            {
                if (!DragFullWindows) draw_moving_frame( parent, hdc, &sizingRect, thickframe );
                if (hittest == HTCAPTION || hittest == HTBORDER) OffsetRect( &sizingRect, dx, dy );
                if (ON_LEFT_BORDER(hittest)) sizingRect.left += dx;
                else if (ON_RIGHT_BORDER(hittest)) sizingRect.right += dx;
                if (ON_TOP_BORDER(hittest)) sizingRect.top += dy;
                else if (ON_BOTTOM_BORDER(hittest)) sizingRect.bottom += dy;
                capturePoint = pt;

                /* determine the hit location */
                if (syscommand == SC_SIZE && hittest != HTBORDER)
                {
                    WPARAM wpSizingHit = 0;

                    if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                        wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
                    SendMessageW( hwnd, WM_SIZING, wpSizingHit, (LPARAM)&sizingRect );
                }
                else
                    SendMessageW( hwnd, WM_MOVING, 0, (LPARAM)&sizingRect );

                if (!DragFullWindows)
                    draw_moving_frame( parent, hdc, &sizingRect, thickframe );
                else
                {
                    RECT rect = sizingRect;
                    MapWindowPoints( 0, parent, (POINT *)&rect, 2 );
                    NtUserSetWindowPos( hwnd, 0, rect.left, rect.top,
                                        rect.right - rect.left, rect.bottom - rect.top,
                                        (hittest == HTCAPTION) ? SWP_NOSIZE : 0 );
                }
            }
        }
    }

    if (moved && !DragFullWindows)
    {
        draw_moving_frame( parent, hdc, &sizingRect, thickframe );
    }

    set_capture_window( 0, GUI_INMOVESIZE, NULL );
    NtUserReleaseDC( parent, hdc );
    if (parent) MapWindowPoints( 0, parent, (POINT *)&sizingRect, 2 );

    if (HOOK_CallHooks( WH_CBT, HCBT_MOVESIZE, (WPARAM)hwnd, (LPARAM)&sizingRect, TRUE ))
        moved = FALSE;

    SendMessageW( hwnd, WM_EXITSIZEMOVE, 0, 0 );
    SendMessageW( hwnd, WM_SETVISIBLE, !IsIconic(hwnd), 0L);

    /* window moved or resized */
    if (moved)
    {
        /* if the moving/resizing isn't canceled call SetWindowPos
         * with the new position or the new size of the window
         */
        if (!((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
        {
            /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
            if (!DragFullWindows)
                NtUserSetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
                                    sizingRect.right - sizingRect.left,
                                    sizingRect.bottom - sizingRect.top,
                                    ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
        else
        { /* restore previous size/position */
            if(DragFullWindows)
                NtUserSetWindowPos( hwnd, 0, origRect.left, origRect.top,
                                    origRect.right - origRect.left,
                                    origRect.bottom - origRect.top,
                                    ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
    }

    if (IsIconic(hwnd))
    {
        /* Single click brings up the system menu when iconized */

        if( !moved )
        {
            if(style & WS_SYSMENU )
                SendMessageW( hwnd, WM_SYSCOMMAND,
                              SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x,pt.y));
        }
    }
}

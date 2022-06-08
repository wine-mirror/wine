/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 * Copyright 1995 Alex Korobka
 * Copyright 2022 Jacek Caban for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


void fill_rect( HDC dc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prev_brush;

    if (hbrush <= (HBRUSH)(COLOR_MENUBAR + 1)) hbrush = get_sys_color_brush( HandleToULong(hbrush) - 1 );

    prev_brush = NtGdiSelectBrush( dc, hbrush );
    NtGdiPatBlt( dc, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    if (prev_brush) NtGdiSelectBrush( dc, prev_brush );
}

/* see DrawFocusRect */
static BOOL draw_focus_rect( HDC hdc, const RECT *rc )
{
    DWORD prev_draw_mode, prev_bk_mode;
    HPEN prev_pen, pen;
    HBRUSH prev_brush;

    prev_brush = NtGdiSelectBrush(hdc, GetStockObject(NULL_BRUSH));
    pen = NtGdiExtCreatePen( PS_COSMETIC|PS_ALTERNATE, 1, BS_SOLID,
                             0, 0, 0, 0, NULL, 0, FALSE, NULL );
    prev_pen = NtGdiSelectPen(hdc, pen);
    NtGdiGetAndSetDCDword( hdc, NtGdiSetROP2, R2_NOT, &prev_draw_mode );
    NtGdiGetAndSetDCDword( hdc, NtGdiSetBkMode, TRANSPARENT, &prev_bk_mode );

    NtGdiRectangle( hdc, rc->left, rc->top, rc->right, rc->bottom );

    NtGdiGetAndSetDCDword( hdc, NtGdiSetBkMode, prev_bk_mode, NULL );
    NtGdiGetAndSetDCDword( hdc, NtGdiSetROP2, prev_draw_mode, NULL );
    NtGdiSelectPen( hdc, prev_pen );
    NtGdiDeleteObjectApp( pen );
    NtGdiSelectBrush( hdc, prev_brush );
    return TRUE;
}

/***********************************************************************
 *           AdjustWindowRectEx (win32u.so)
 */
BOOL WINAPI AdjustWindowRectEx( RECT *rect, DWORD style, BOOL menu, DWORD ex_style )
{
    NONCLIENTMETRICSW ncm;
    int adjust = 0;

    ncm.cbSize = sizeof(ncm);
    NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );

    if ((ex_style & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        adjust = 1; /* for the outer frame always present */
    else if ((ex_style & WS_EX_DLGMODALFRAME) || (style & (WS_THICKFRAME|WS_DLGFRAME)))
        adjust = 2; /* outer */

    if (style & WS_THICKFRAME)
        adjust += ncm.iBorderWidth + ncm.iPaddedBorderWidth; /* The resize border */

    if ((style & (WS_BORDER|WS_DLGFRAME)) || (ex_style & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect( rect, adjust, adjust );

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (ex_style & WS_EX_TOOLWINDOW)
            rect->top -= ncm.iSmCaptionHeight + 1;
        else
            rect->top -= ncm.iCaptionHeight + 1;
    }
    if (menu) rect->top -= ncm.iMenuHeight + 1;

    if (ex_style & WS_EX_CLIENTEDGE)
        InflateRect( rect, get_system_metrics(SM_CXEDGE), get_system_metrics(SM_CYEDGE) );
    return TRUE;
}

static BOOL set_window_text( HWND hwnd, const void *text, BOOL ansi )
{
    static const WCHAR emptyW[] = { 0 };
    WCHAR *str;
    WND *win;

    /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
     * may have child window IDs instead of window name */
    if (text && IS_INTRESOURCE(text)) return FALSE;

    if (text)
    {
        if (ansi) str = towstr( text );
        else str = wcsdup( text );
        if (!str) return FALSE;
    }
    else str = NULL;

    TRACE( "%s\n", debugstr_w(str) );

    if (!(win = get_win_ptr( hwnd )))
    {
        free( str );
        return FALSE;
    }

    free( win->text );
    win->text = str;
    SERVER_START_REQ( set_window_text )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (str) wine_server_add_data( req, str, lstrlenW( str ) * sizeof(WCHAR) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    release_win_ptr( win );

    user_driver->pSetWindowText( hwnd, str ? str : emptyW );

    return TRUE;
}

static HICON get_window_icon( HWND hwnd, WPARAM type )
{
    HICON ret;
    WND *win;

    if (!(win = get_win_ptr( hwnd ))) return 0;

    switch(type)
    {
    case ICON_SMALL:
        ret = win->hIconSmall;
        break;
    case ICON_BIG:
        ret = win->hIcon;
        break;
    case ICON_SMALL2:
        ret = win->hIconSmall ? win->hIconSmall : win->hIconSmall2;
        break;
    default:
        ret = 0;
        break;
    }

    release_win_ptr( win );
    return ret;
}

static HICON set_window_icon( HWND hwnd, WPARAM type, HICON icon )
{
    HICON ret = 0;
    WND *win;

    if (!(win = get_win_ptr( hwnd ))) return 0;

    switch (type)
    {
    case ICON_SMALL:
        ret = win->hIconSmall;
        if (ret && !icon && win->hIcon)
        {
            win->hIconSmall2 = CopyImage( win->hIcon, IMAGE_ICON,
                                          get_system_metrics( SM_CXSMICON ),
                                          get_system_metrics( SM_CYSMICON ), 0 );
        }
        else if (icon && win->hIconSmall2)
        {
            NtUserDestroyCursor( win->hIconSmall2, 0 );
            win->hIconSmall2 = NULL;
        }
        win->hIconSmall = icon;
        break;

    case ICON_BIG:
        ret = win->hIcon;
        if (win->hIconSmall2)
        {
            NtUserDestroyCursor( win->hIconSmall2, 0 );
            win->hIconSmall2 = NULL;
        }
        if (icon && !win->hIconSmall)
        {
            win->hIconSmall2 = CopyImage( icon, IMAGE_ICON,
                                          get_system_metrics( SM_CXSMICON ),
                                          get_system_metrics( SM_CYSMICON ), 0 );
        }
        win->hIcon = icon;
        break;
    }
    release_win_ptr( win );

    user_driver->pSetWindowIcon( hwnd, type, icon );
    return ret;
}

static LONG handle_window_pos_changing( HWND hwnd, WINDOWPOS *winpos )
{
    LONG style = get_window_long( hwnd, GWL_STYLE );

    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((style & WS_THICKFRAME) || ((style & (WS_POPUP | WS_CHILD)) == 0))
    {
        MINMAXINFO info = get_min_max_info( hwnd );
        winpos->cx = min( winpos->cx, info.ptMaxTrackSize.x );
        winpos->cy = min( winpos->cy, info.ptMaxTrackSize.y );
        if (!(style & WS_MINIMIZE))
        {
            winpos->cx = max( winpos->cx, info.ptMinTrackSize.x );
            winpos->cy = max( winpos->cy, info.ptMinTrackSize.y );
        }
    }
    return 0;
}

/***********************************************************************
 *           draw_moving_frame
 *
 * Draw the frame used when moving or resizing window.
 */
static void draw_moving_frame( HWND parent, HDC hdc, RECT *screen_rect, BOOL thickframe )
{
    RECT rect = *screen_rect;

    if (parent) map_window_points( 0, parent, (POINT *)&rect, 2, get_thread_dpi() );
    if (thickframe)
    {
        const int width = get_system_metrics( SM_CXFRAME );
        const int height = get_system_metrics( SM_CYFRAME );

        HBRUSH hbrush = NtGdiSelectBrush( hdc, GetStockObject( GRAY_BRUSH ));
        NtGdiPatBlt( hdc, rect.left, rect.top,
                     rect.right - rect.left - width, height, PATINVERT );
        NtGdiPatBlt( hdc, rect.left, rect.top + height, width,
                     rect.bottom - rect.top - height, PATINVERT );
        NtGdiPatBlt( hdc, rect.left + width, rect.bottom - 1,
                     rect.right - rect.left - width, -height, PATINVERT );
        NtGdiPatBlt( hdc, rect.right - 1, rect.top, -width,
                     rect.bottom - rect.top - height, PATINVERT );
        NtGdiSelectBrush( hdc, hbrush );
    }
    else draw_focus_rect( hdc, &rect );
}

/***********************************************************************
 *           start_size_move
 *
 * Initialization of a move or resize, when initiated from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG start_size_move( HWND hwnd, WPARAM wparam, POINT *capture_point, LONG style )
{
    RECT window_rect;
    LONG hittest = 0;
    POINT pt;
    MSG msg;

    get_window_rect( hwnd, &window_rect, get_thread_dpi() );

    if ((wparam & 0xfff0) == SC_MOVE)
    {
        /* Move pointer at the center of the caption */
        RECT rect = window_rect;
        /* Note: to be exactly centered we should take the different types
         * of border into account, but it shouldn't make more than a few pixels
         * of difference so let's not bother with that */
        rect.top += get_system_metrics( SM_CYBORDER );
        if (style & WS_SYSMENU)     rect.left  += get_system_metrics( SM_CXSIZE ) + 1;
        if (style & WS_MINIMIZEBOX) rect.right -= get_system_metrics( SM_CXSIZE ) + 1;
        if (style & WS_MAXIMIZEBOX) rect.right -= get_system_metrics( SM_CXSIZE ) + 1;
        pt.x = (rect.right + rect.left) / 2;
        pt.y = rect.top + get_system_metrics( SM_CYSIZE ) / 2;
        hittest = HTCAPTION;
        *capture_point = pt;
    }
    else  /* SC_SIZE */
    {
        HCURSOR cursor;
        cursor = LoadImageW( 0, (WCHAR *)IDC_SIZEALL, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE );
        NtUserSetCursor( cursor );
        pt.x = pt.y = 0;
        while (!hittest)
        {
            if (!NtUserGetMessage( &msg, 0, 0, 0 )) return 0;
            if (NtUserCallMsgFilter( &msg, MSGF_SIZE )) continue;

            switch (msg.message)
            {
            case WM_MOUSEMOVE:
                pt.x = min( max( msg.pt.x, window_rect.left ), window_rect.right - 1 );
                pt.y = min( max( msg.pt.y, window_rect.top ), window_rect.bottom - 1 );
                hittest = send_message( hwnd, WM_NCHITTEST, 0, MAKELONG( pt.x, pt.y ));
                if (hittest < HTLEFT || hittest > HTBOTTOMRIGHT) hittest = 0;
                break;

            case WM_LBUTTONUP:
                return 0;

            case WM_KEYDOWN:
                switch (msg.wParam)
                {
                case VK_UP:
                    hittest = HTTOP;
                    pt.x = (window_rect.left + window_rect.right) / 2;
                    pt.y = window_rect.top + get_system_metrics( SM_CYFRAME ) / 2;
                    break;
                case VK_DOWN:
                    hittest = HTBOTTOM;
                    pt.x = (window_rect.left + window_rect.right) / 2;
                    pt.y = window_rect.bottom - get_system_metrics( SM_CYFRAME ) / 2;
                    break;
                case VK_LEFT:
                    hittest = HTLEFT;
                    pt.x = window_rect.left + get_system_metrics( SM_CXFRAME ) / 2;
                    pt.y = (window_rect.top + window_rect.bottom) / 2;
                    break;
                case VK_RIGHT:
                    hittest = HTRIGHT;
                    pt.x = window_rect.right - get_system_metrics( SM_CXFRAME ) / 2;
                    pt.y = (window_rect.top + window_rect.bottom) / 2;
                    break;
                case VK_RETURN:
                case VK_ESCAPE:
                    return 0;
                }
                break;
            default:
                NtUserTranslateMessage( &msg, 0 );
                NtUserDispatchMessage( &msg );
                break;
            }
        }
        *capture_point = pt;
    }
    NtUserSetCursorPos( pt.x, pt.y );
    send_message( hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}

static BOOL on_left_border( int hit )
{
    return hit == HTLEFT || hit == HTTOPLEFT || hit == HTBOTTOMLEFT;
}

static BOOL on_right_border( int hit )
{
    return hit == HTRIGHT || hit == HTTOPRIGHT || hit == HTBOTTOMRIGHT;
}

static BOOL on_top_border( int hit )
{
    return hit == HTTOP || hit == HTTOPLEFT || hit == HTTOPRIGHT;
}

static BOOL on_bottom_border( int hit )
{
    return hit == HTBOTTOM || hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT;
}

/***********************************************************************
 *           sys_command_size_move
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
static void sys_command_size_move( HWND hwnd, WPARAM wparam )
{
    DWORD msg_pos = NtUserGetThreadInfo()->message_pos;
    BOOL thickframe, drag_full_windows = TRUE, moved = FALSE;
    RECT sizing_rect, mouse_rect, orig_rect;
    LONG hittest = wparam & 0x0f;
    WPARAM syscommand = wparam & 0xfff0;
    LONG style = get_window_long( hwnd, GWL_STYLE );
    POINT capture_point, pt;
    MINMAXINFO minmax;
    HMONITOR mon = 0;
    HWND parent;
    UINT dpi;
    HDC hdc;
    MSG msg;

    if (is_zoomed( hwnd ) || !is_window_visible( hwnd )) return;

    thickframe = (style & WS_THICKFRAME) && !((style & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME);

    pt.x = (short)LOWORD(msg_pos);
    pt.y = (short)HIWORD(msg_pos);
    capture_point = pt;
    NtUserClipCursor( NULL );

    TRACE( "hwnd %p command %04lx, hittest %d, pos %d,%d\n",
           hwnd, syscommand, hittest, pt.x, pt.y );

    if (syscommand == SC_MOVE)
    {
        if (!hittest) hittest = start_size_move( hwnd, wparam, &capture_point, style );
        if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
        if (hittest && syscommand != SC_MOUSEMENU)
            hittest += (HTLEFT - WMSZ_LEFT);
        else
        {
            set_capture_window( hwnd, GUI_INMOVESIZE, NULL );
            hittest = start_size_move( hwnd, wparam, &capture_point, style );
            if (!hittest)
            {
                set_capture_window( 0, GUI_INMOVESIZE, NULL );
                return;
            }
        }
    }

    minmax = get_min_max_info( hwnd );
    dpi = get_thread_dpi();
    get_window_rects( hwnd, COORDS_PARENT, &sizing_rect, NULL, dpi );
    orig_rect = sizing_rect;
    if (style & WS_CHILD)
    {
        parent = get_parent( hwnd );
        get_client_rect( parent, &mouse_rect );
        map_window_points( parent, 0, (POINT *)&mouse_rect, 2, dpi );
        map_window_points( parent, 0, (POINT *)&sizing_rect, 2, dpi );
    }
    else
    {
        parent = 0;
        mouse_rect = get_virtual_screen_rect( get_thread_dpi() );
        mon = monitor_from_point( pt, MONITOR_DEFAULTTONEAREST, dpi );
    }

    if (on_left_border( hittest ))
    {
        mouse_rect.left = max( mouse_rect.left,
                sizing_rect.right - minmax.ptMaxTrackSize.x + capture_point.x - sizing_rect.left );
        mouse_rect.right = min( mouse_rect.right,
                sizing_rect.right - minmax.ptMinTrackSize.x + capture_point.x - sizing_rect.left );
    }
    else if (on_right_border( hittest ))
    {
        mouse_rect.left  = max( mouse_rect.left,
                sizing_rect.left + minmax.ptMinTrackSize.x + capture_point.x - sizing_rect.right );
        mouse_rect.right = min( mouse_rect.right,
                sizing_rect.left + minmax.ptMaxTrackSize.x + capture_point.x - sizing_rect.right );
    }

    if (on_top_border( hittest ))
    {
        mouse_rect.top = max( mouse_rect.top,
                sizing_rect.bottom - minmax.ptMaxTrackSize.y + capture_point.y - sizing_rect.top );
        mouse_rect.bottom = min( mouse_rect.bottom,
                sizing_rect.bottom - minmax.ptMinTrackSize.y + capture_point.y - sizing_rect.top );
    }
    else if (on_bottom_border( hittest ))
    {
        mouse_rect.top = max( mouse_rect.top,
                sizing_rect.top + minmax.ptMinTrackSize.y + capture_point.y - sizing_rect.bottom );
        mouse_rect.bottom = min( mouse_rect.bottom,
                sizing_rect.top + minmax.ptMaxTrackSize.y + capture_point.y - sizing_rect.bottom );
    }

    /* Retrieve a default cache DC (without using the window style) */
    hdc = NtUserGetDCEx( parent, 0, DCX_CACHE );

    /* we only allow disabling the full window drag for child windows */
    if (parent) NtUserSystemParametersInfo( SPI_GETDRAGFULLWINDOWS, 0, &drag_full_windows, 0 );

    /* repaint the window before moving it around */
    NtUserRedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    send_message( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
    set_capture_window( hwnd, GUI_INMOVESIZE, NULL );

    for (;;)
    {
        int dx = 0, dy = 0;

        if (!NtUserGetMessage( &msg, 0, 0, 0 )) break;
        if (NtUserCallMsgFilter( &msg, MSGF_SIZE )) continue;

        /* Exit on button-up, Return, or Esc */
        if (msg.message == WM_LBUTTONUP ||
            (msg.message == WM_KEYDOWN && (msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE)))
            break;

        if (msg.message != WM_KEYDOWN && msg.message != WM_MOUSEMOVE)
        {
            NtUserTranslateMessage( &msg, 0 );
            NtUserDispatchMessage( &msg );
            continue;  /* We are not interested in other messages */
        }

        pt = msg.pt;

        if (msg.message == WM_KEYDOWN)
        {
            switch (msg.wParam)
            {
            case VK_UP:    pt.y -= 8; break;
            case VK_DOWN:  pt.y += 8; break;
            case VK_LEFT:  pt.x -= 8; break;
            case VK_RIGHT: pt.x += 8; break;
            }
        }

        pt.x = max( pt.x, mouse_rect.left );
        pt.x = min( pt.x, mouse_rect.right - 1 );
        pt.y = max( pt.y, mouse_rect.top );
        pt.y = min( pt.y, mouse_rect.bottom - 1 );

        if (!parent)
        {
            HMONITOR newmon;
            MONITORINFO info;

            if ((newmon = monitor_from_point( pt, MONITOR_DEFAULTTONULL, get_thread_dpi() )))
                mon = newmon;

            info.cbSize = sizeof(info);
            if (mon && get_monitor_info( mon, &info ))
            {
                pt.x = max( pt.x, info.rcWork.left );
                pt.x = min( pt.x, info.rcWork.right - 1 );
                pt.y = max( pt.y, info.rcWork.top );
                pt.y = min( pt.y, info.rcWork.bottom - 1 );
            }
        }

        dx = pt.x - capture_point.x;
        dy = pt.y - capture_point.y;

        if (dx || dy)
        {
            if (!moved)
            {
                moved = TRUE;
                if (!drag_full_windows)
                    draw_moving_frame( parent, hdc, &sizing_rect, thickframe );
            }

            if (msg.message == WM_KEYDOWN) NtUserSetCursorPos( pt.x, pt.y );
            else
            {
                if (!drag_full_windows) draw_moving_frame( parent, hdc, &sizing_rect, thickframe );
                if (hittest == HTCAPTION || hittest == HTBORDER) OffsetRect( &sizing_rect, dx, dy );
                if (on_left_border( hittest )) sizing_rect.left += dx;
                else if (on_right_border( hittest )) sizing_rect.right += dx;
                if (on_top_border( hittest )) sizing_rect.top += dy;
                else if (on_bottom_border( hittest )) sizing_rect.bottom += dy;
                capture_point = pt;

                /* determine the hit location */
                if (syscommand == SC_SIZE && hittest != HTBORDER)
                {
                    WPARAM sizing_hit = 0;

                    if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                        sizing_hit = WMSZ_LEFT + (hittest - HTLEFT);
                    send_message( hwnd, WM_SIZING, sizing_hit, (LPARAM)&sizing_rect );
                }
                else
                    send_message( hwnd, WM_MOVING, 0, (LPARAM)&sizing_rect );

                if (!drag_full_windows)
                    draw_moving_frame( parent, hdc, &sizing_rect, thickframe );
                else
                {
                    RECT rect = sizing_rect;
                    map_window_points( 0, parent, (POINT *)&rect, 2, get_thread_dpi() );
                    NtUserSetWindowPos( hwnd, 0, rect.left, rect.top,
                                        rect.right - rect.left, rect.bottom - rect.top,
                                        hittest == HTCAPTION ? SWP_NOSIZE : 0 );
                }
            }
        }
    }

    if (moved && !drag_full_windows)
        draw_moving_frame( parent, hdc, &sizing_rect, thickframe );

    set_capture_window( 0, GUI_INMOVESIZE, NULL );
    NtUserReleaseDC( parent, hdc );
    if (parent) map_window_points( 0, parent, (POINT *)&sizing_rect, 2, get_thread_dpi() );

    if (call_hooks( WH_CBT, HCBT_MOVESIZE, (WPARAM)hwnd, (LPARAM)&sizing_rect, TRUE ))
        moved = FALSE;

    send_message( hwnd, WM_EXITSIZEMOVE, 0, 0 );
    send_message( hwnd, WM_SETVISIBLE, !is_iconic(hwnd), 0 );

    /* window moved or resized */
    if (moved)
    {
        /* if the moving/resizing isn't canceled call SetWindowPos
         * with the new position or the new size of the window
         */
        if (!(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) )
        {
            /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
            if (!drag_full_windows)
                NtUserSetWindowPos( hwnd, 0, sizing_rect.left, sizing_rect.top,
                                    sizing_rect.right - sizing_rect.left,
                                    sizing_rect.bottom - sizing_rect.top,
                                    hittest == HTCAPTION ? SWP_NOSIZE : 0 );
        }
        else
        {
            /* restore previous size/position */
            if (drag_full_windows)
                NtUserSetWindowPos( hwnd, 0, orig_rect.left, orig_rect.top,
                                    orig_rect.right - orig_rect.left,
                                    orig_rect.bottom - orig_rect.top,
                                    hittest == HTCAPTION ? SWP_NOSIZE : 0 );
        }
    }

    if (is_iconic(hwnd) && !moved && (style & WS_SYSMENU))
    {
        /* Single click brings up the system menu when iconized */
        send_message( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x, pt.y) );
    }
}

static LRESULT handle_sys_command( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    if (!is_window_enabled( hwnd )) return 0;

    if (call_hooks( WH_CBT, HCBT_SYSCOMMAND, wparam, lparam, TRUE ))
        return 0;

    if (!user_driver->pSysCommand( hwnd, wparam, lparam ))
        return 0;

    switch (wparam & 0xfff0)
    {
    case SC_SIZE:
    case SC_MOVE:
        sys_command_size_move( hwnd, wparam );
        break;

    case SC_MINIMIZE:
        show_owned_popups( hwnd, FALSE );
        NtUserShowWindow( hwnd, SW_MINIMIZE );
        break;

    case SC_MAXIMIZE:
        if (is_iconic(hwnd)) show_owned_popups( hwnd, TRUE );
        NtUserShowWindow( hwnd, SW_MAXIMIZE );
        break;

    case SC_RESTORE:
        if (is_iconic( hwnd )) show_owned_popups( hwnd, TRUE );
        NtUserShowWindow( hwnd, SW_RESTORE );
        break;

    default:
        return 1; /* handle on client side */
    }
    return 0;
}

LRESULT default_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_NCCREATE:
        if (lparam)
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
            set_window_text( hwnd, cs->lpszName, ansi );
            result = 1;
        }
        break;

    case WM_NCDESTROY:
        {
            WND *win = get_win_ptr( hwnd );
            if (!win) return 0;
            free( win->text );
            win->text = NULL;
            if (user_callbacks) user_callbacks->free_win_ptr( win );
            win->pScroll = NULL;
            release_win_ptr( win );
            break;
        }

    case WM_WINDOWPOSCHANGING:
        return handle_window_pos_changing( hwnd, (WINDOWPOS *)lparam );

    case WM_PAINTICON:
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = NtUserBeginPaint( hwnd, &ps );
            if (hdc)
            {
                HICON icon;
                if (is_iconic(hwnd) && ((icon = UlongToHandle( get_class_long( hwnd, GCLP_HICON, FALSE )))))
                {
                    RECT rc;
                    int x, y;

                    get_client_rect( hwnd, &rc );
                    x = (rc.right - rc.left - get_system_metrics( SM_CXICON )) / 2;
                    y = (rc.bottom - rc.top - get_system_metrics( SM_CYICON )) / 2;
                    TRACE( "Painting class icon: vis rect=(%s)\n", wine_dbgstr_rect(&ps.rcPaint) );
                    NtUserDrawIconEx( hdc, x, y, icon, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE );
                }
                NtUserEndPaint( hwnd, &ps );
            }
            break;
        }

    case WM_SYNCPAINT:
        NtUserRedrawWindow ( hwnd, NULL, 0, RDW_ERASENOW | RDW_ERASE | RDW_ALLCHILDREN );
        return 0;

    case WM_SETREDRAW:
        if (wparam) set_window_style( hwnd, WS_VISIBLE, 0 );
        else
        {
            NtUserRedrawWindow( hwnd, NULL, 0, RDW_ALLCHILDREN | RDW_VALIDATE );
            set_window_style( hwnd, 0, WS_VISIBLE );
        }
        return 0;

    case WM_CLOSE:
        NtUserDestroyWindow( hwnd );
        return 0;

    case WM_MOUSEACTIVATE:
        if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)
        {
            result = send_message( get_parent(hwnd), WM_MOUSEACTIVATE, wparam, lparam );
            if (result) break;
        }

        /* Caption clicks are handled by NC_HandleNCLButtonDown() */
        result = HIWORD(lparam) == WM_LBUTTONDOWN && LOWORD(lparam) == HTCAPTION ?
            MA_NOACTIVATE : MA_ACTIVATE;
        break;

    case WM_ACTIVATE:
        /* The default action in Windows is to set the keyboard focus to
         * the window, if it's being activated and not minimized */
        if (LOWORD(wparam) != WA_INACTIVE && !is_iconic( hwnd )) NtUserSetFocus( hwnd );
        break;

    case WM_MOUSEWHEEL:
        if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)
            result = send_message( get_parent( hwnd ), WM_MOUSEWHEEL, wparam, lparam );
        break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
        {
            RECT rect;
            HDC hdc = (HDC)wparam;
            HBRUSH hbr = UlongToHandle( get_class_long( hwnd, GCLP_HBRBACKGROUND, FALSE ));
            if (!hbr) break;

            if (get_class_long( hwnd, GCL_STYLE, FALSE ) & CS_PARENTDC)
            {
                /* can't use GetClipBox with a parent DC or we fill the whole parent */
                get_client_rect( hwnd, &rect );
                NtGdiTransformPoints( hdc, (POINT *)&rect, (POINT *)&rect, 1, NtGdiDPtoLP );
            }
            else NtGdiGetAppClipBox( hdc, &rect );
            fill_rect( hdc, &rect, hbr );
            return 1;
        }

    case WM_GETDLGCODE:
        break;

    case WM_SETTEXT:
        result = set_window_text( hwnd, (void *)lparam, ansi );
        break;

    case WM_SETICON:
        result = (LRESULT)set_window_icon( hwnd, wparam, (HICON)lparam );
        break;

    case WM_GETICON:
        result = (LRESULT)get_window_icon( hwnd, wparam );
        break;

    case WM_SYSCOMMAND:
        result = handle_sys_command( hwnd, wparam, lparam );
        break;
    }

    return result;
}

LRESULT desktop_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static const WCHAR wine_display_device_guidW[] =
        {'_','_','w','i','n','e','_','d','i','s','p','l','a','y','_','d','e','v','i','c','e',
         '_','g','u','i','d',0};

    switch (msg)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        const GUID *guid = cs->lpCreateParams;

        if (guid)
        {
            ATOM atom = 0;
            char buffer[37];
            WCHAR bufferW[37];

            if (NtUserGetAncestor( hwnd, GA_PARENT )) return FALSE;  /* refuse to create non-desktop window */

            sprintf( buffer, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     (unsigned int)guid->Data1, guid->Data2, guid->Data3,
                     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
            NtAddAtom( bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR), &atom );
            NtUserSetProp( hwnd, wine_display_device_guidW, ULongToHandle( atom ) );
        }
        return TRUE;
    }
    case WM_NCCALCSIZE:
        return 0;

    default:
        if (msg >= WM_USER && hwnd == get_desktop_window())
            return user_driver->pDesktopWindowProc( hwnd, msg, wparam, lparam );
    }

    return default_window_proc( hwnd, msg, wparam, lparam, FALSE );
}

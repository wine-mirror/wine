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

#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


#define DRAG_FILE  0x454c4946

/* bits in the dwKeyData */
#define KEYDATA_ALT             0x2000
#define KEYDATA_PREVSTATE       0x4000

static short f10_key = 0;
static short menu_sys_key = 0;

static BOOL has_dialog_frame( UINT style, UINT ex_style )
{
    return (ex_style & WS_EX_DLGMODALFRAME) || ((style & WS_DLGFRAME) && !(style & WS_THICKFRAME));
}

static BOOL has_thick_frame( UINT style, UINT ex_style )
{
    return (style & WS_THICKFRAME) && (style & (WS_DLGFRAME|WS_BORDER)) != WS_DLGFRAME;
}

static BOOL has_thin_frame( UINT style )
{
    return (style & WS_BORDER) || !(style & (WS_CHILD | WS_POPUP));
}

static BOOL has_big_frame( UINT style, UINT ex_style )
{
    return (style & (WS_THICKFRAME | WS_DLGFRAME)) || (ex_style & WS_EX_DLGMODALFRAME);
}

static BOOL has_static_outer_frame( UINT ex_style )
{
    return (ex_style & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE;
}

static BOOL has_menu( HWND hwnd, UINT style )
{
    return (style & (WS_CHILD | WS_POPUP)) != WS_CHILD && get_menu( hwnd );
}

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

static const signed char lt_inner_normal[] = {
    -1,           -1,                 -1,                 -1,
    -1,           COLOR_BTNHIGHLIGHT, COLOR_BTNHIGHLIGHT, -1,
    -1,           COLOR_3DDKSHADOW,   COLOR_3DDKSHADOW,   -1,
    -1,           -1,                 -1,                 -1
};

static const signed char lt_outer_normal[] = {
    -1,                 COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1,
    COLOR_BTNHIGHLIGHT, COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1,
    COLOR_3DDKSHADOW,   COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1,
    -1,                 COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1
};

static const signed char rb_inner_normal[] = {
    -1,           -1,                -1,              -1,
    -1,           COLOR_BTNSHADOW,   COLOR_BTNSHADOW, -1,
    -1,           COLOR_3DLIGHT,     COLOR_3DLIGHT,   -1,
    -1,           -1,                -1,              -1
};

static const signed char rb_outer_normal[] = {
    -1,              COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    COLOR_BTNSHADOW, COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    COLOR_3DLIGHT,   COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    -1,              COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1
};

static const signed char ltrb_outer_mono[] = {
    -1,           COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
};

static const signed char ltrb_inner_mono[] = {
    -1, -1,           -1,           -1,
    -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
    -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
    -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
};

BOOL draw_rect_edge( HDC hdc, RECT *rc, UINT type, UINT flags, UINT width )
{
    int lbi_offset = 0, lti_offset = 0, rti_offset = 0, rbi_offset = 0;
    signed char lt_inner, lt_outer, rb_inner, rb_outer;
    HBRUSH lti_brush, lto_brush, rbi_brush, rbo_brush;
    RECT inner_rect = *rc, rect;
    BOOL retval;

    retval = !((type & BDR_INNER) == BDR_INNER || (type & BDR_OUTER) == BDR_OUTER) &&
        !(flags & (BF_FLAT|BF_MONO));

    lti_brush = lto_brush = rbi_brush = rbo_brush = GetStockObject( NULL_BRUSH );

    /* Determine the colors of the edges */
    lt_inner = lt_inner_normal[type & (BDR_INNER|BDR_OUTER)];
    lt_outer = lt_outer_normal[type & (BDR_INNER|BDR_OUTER)];
    rb_inner = rb_inner_normal[type & (BDR_INNER|BDR_OUTER)];
    rb_outer = rb_outer_normal[type & (BDR_INNER|BDR_OUTER)];

    if ((flags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)   lbi_offset = width;
    if ((flags & BF_TOPRIGHT) == BF_TOPRIGHT)       rti_offset = width;
    if ((flags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT) rbi_offset = width;
    if ((flags & BF_TOPLEFT) == BF_TOPLEFT)         lti_offset = width;

    if (lt_inner != -1) lti_brush = get_sys_color_brush( lt_inner );
    if (lt_outer != -1) lto_brush = get_sys_color_brush( lt_outer );
    if (rb_inner != -1) rbi_brush = get_sys_color_brush( rb_inner );
    if (rb_outer != -1) rbo_brush = get_sys_color_brush( rb_outer );

    /* Draw the outer edge */
    if (flags & BF_TOP)
    {
        rect = inner_rect;
        rect.bottom = rect.top + width;
        fill_rect( hdc, &rect, lto_brush );
    }
    if (flags & BF_LEFT)
    {
        rect = inner_rect;
        rect.right = rect.left + width;
        fill_rect( hdc, &rect, lto_brush );
    }
    if (flags & BF_BOTTOM)
    {
        rect = inner_rect;
        rect.top = rect.bottom - width;
        fill_rect( hdc, &rect, rbo_brush );
    }
    if (flags & BF_RIGHT)
    {
        rect = inner_rect;
        rect.left = rect.right - width;
        fill_rect( hdc, &rect, rbo_brush );
    }

    /* Draw the inner edge */
    if (flags & BF_TOP)
    {
        SetRect( &rect, inner_rect.left + lti_offset, inner_rect.top + width,
                 inner_rect.right - rti_offset, inner_rect.top + 2 * width );
        fill_rect( hdc, &rect, lti_brush );
    }
    if (flags & BF_LEFT)
    {
        SetRect( &rect, inner_rect.left + width, inner_rect.top + lti_offset,
                 inner_rect.left + 2 * width, inner_rect.bottom - lbi_offset );
        fill_rect( hdc, &rect, lti_brush );
    }
    if (flags & BF_BOTTOM)
    {
        SetRect( &rect, inner_rect.left + lbi_offset, inner_rect.bottom - 2 * width,
                 inner_rect.right - rbi_offset, inner_rect.bottom - width );
        fill_rect( hdc, &rect, rbi_brush );
    }
    if (flags & BF_RIGHT)
    {
        SetRect( &rect, inner_rect.right - 2 * width, inner_rect.top + rti_offset,
                 inner_rect.right - width, inner_rect.bottom - rbi_offset );
        fill_rect( hdc, &rect, rbi_brush );
    }

    if (((flags & BF_MIDDLE) && retval) || (flags & BF_ADJUST))
    {
        int add = (ltrb_inner_mono[type & (BDR_INNER|BDR_OUTER)] != -1 ? width : 0)
                + (ltrb_outer_mono[type & (BDR_INNER|BDR_OUTER)] != -1 ? width : 0);

        if (flags & BF_LEFT)   inner_rect.left   += add;
        if (flags & BF_RIGHT)  inner_rect.right  -= add;
        if (flags & BF_TOP)    inner_rect.top    += add;
        if (flags & BF_BOTTOM) inner_rect.bottom -= add;

        if ((flags & BF_MIDDLE) && retval)
        {
            fill_rect( hdc, &inner_rect, get_sys_color_brush( flags & BF_MONO ?
                                                              COLOR_WINDOW : COLOR_BTNFACE ));
        }

        if (flags & BF_ADJUST) *rc = inner_rect;
    }

    return retval;
}

/* see AdjustWindowRectExForDpi */
BOOL adjust_window_rect( RECT *rect, DWORD style, BOOL menu, DWORD ex_style, UINT dpi )
{
    NONCLIENTMETRICSW ncm = {.cbSize = sizeof(ncm)};
    int adjust = 0;

    NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );

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

static int get_window_text( HWND hwnd, WCHAR *str, int count )
{
    int ret;

    if (is_current_process_window( hwnd ))
    {
        /* FIXME: use packed send message */
        ret = send_message( hwnd, WM_GETTEXT, count, (LPARAM)str );
    }
    else
    {
        /* when window belongs to other process, don't send a message */
        ret = NtUserInternalGetWindowText( hwnd, str, count );
    }

    return ret;
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

static LRESULT handle_set_cursor( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    UINT cursor_id = IDC_ARROW;
    HCURSOR cursor;

    hwnd = get_full_window_handle( (HWND)wparam );

    switch((short)LOWORD( lparam ))
    {
    case HTERROR:
        {
            WORD msg = HIWORD( lparam );
            if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN ||
                msg == WM_RBUTTONDOWN || msg == WM_XBUTTONDOWN)
                NtUserMessageBeep( 0 );
        }
        break;

    case HTCLIENT:
        cursor = (HCURSOR)get_class_long_ptr( hwnd, GCLP_HCURSOR, FALSE );
        if (!cursor) return FALSE;
        NtUserSetCursor( cursor );
        return TRUE;

    case HTLEFT:
    case HTRIGHT:
        cursor_id = IDC_SIZEWE;
        break;

    case HTTOP:
    case HTBOTTOM:
        cursor_id = IDC_SIZENS;
        break;

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
        cursor_id = IDC_SIZENWSE;
        break;

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
        cursor_id = IDC_SIZENESW;
    }

    cursor = LoadImageW( 0, MAKEINTRESOURCEW( cursor_id ), IMAGE_CURSOR,
                         0, 0, LR_SHARED | LR_DEFAULTSIZE );
    return (LRESULT)NtUserSetCursor( cursor );
}

static LONG handle_window_pos_changing( HWND hwnd, WINDOWPOS *winpos )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );

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

static void handle_window_pos_changed( HWND hwnd, const WINDOWPOS *winpos )
{
    RECT rect;

    get_client_rect_rel( hwnd, COORDS_PARENT, &rect, get_thread_dpi() );
    if (!(winpos->flags & SWP_NOCLIENTMOVE))
        send_message( hwnd, WM_MOVE, 0, MAKELONG( rect.left, rect.top ));

    if (!(winpos->flags & SWP_NOCLIENTSIZE) || (winpos->flags & SWP_STATECHANGED))
    {
        if (is_iconic( hwnd ))
        {
            send_message( hwnd, WM_SIZE, SIZE_MINIMIZED, 0 );
        }
        else
        {
            WPARAM wp = is_zoomed( hwnd ) ? SIZE_MAXIMIZED : SIZE_RESTORED;
            send_message( hwnd, WM_SIZE, wp,
                          MAKELONG( rect.right-rect.left, rect.bottom-rect.top ));
        }
    }
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
    UINT hittest = wparam & 0x0f;
    UINT syscommand = wparam & 0xfff0;
    UINT style = get_window_long( hwnd, GWL_STYLE );
    POINT capture_point, pt;
    MINMAXINFO minmax;
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

    TRACE( "hwnd %p command %04x, hittest %d, pos %d,%d\n",
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
    get_window_rect_rel( hwnd, COORDS_PARENT, &sizing_rect, dpi );
    orig_rect = sizing_rect;
    if (style & WS_CHILD)
    {
        parent = get_parent( hwnd );
        get_client_rect( parent, &mouse_rect, get_thread_dpi() );
        map_window_points( parent, 0, (POINT *)&mouse_rect, 2, dpi );
        map_window_points( parent, 0, (POINT *)&sizing_rect, 2, dpi );
    }
    else
    {
        parent = 0;
        mouse_rect = get_virtual_screen_rect( get_thread_dpi(), MDT_DEFAULT );
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

            /* It's possible that the window proc that handled the dispatch consumed a
             * WM_LBUTTONUP. Detect that and terminate the loop as if we'd gotten it. */
            if (!(NtUserGetKeyState( VK_LBUTTON ) & 0x8000))
            {
                DWORD last_pos = NtUserGetThreadInfo()->message_pos;
                pt.x = ((int)(short)LOWORD( last_pos ));
                pt.y = ((int)(short)HIWORD( last_pos ));
                break;
            }

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
            MONITORINFO info;
            RECT rect;

            SetRect( &rect, pt.x, pt.y, pt.x, pt.y );
            info = monitor_info_from_rect( rect, get_thread_dpi() );
            pt.x = max( pt.x, info.rcWork.left );
            pt.x = min( pt.x, info.rcWork.right - 1 );
            pt.y = max( pt.y, info.rcWork.top );
            pt.y = min( pt.y, info.rcWork.bottom - 1 );
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

    if (call_hooks( WH_CBT, HCBT_MOVESIZE, (WPARAM)hwnd, (LPARAM)&sizing_rect, sizeof(sizing_rect) ))
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

/***********************************************************************
 *           track_nc_scroll_bar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void track_nc_scroll_bar( HWND hwnd, WPARAM wparam, POINT pt )
{
    int scrollbar;

    if ((wparam & 0xfff0) == SC_HSCROLL)
    {
        if ((wparam & 0x0f) != HTHSCROLL) return;
        scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
        if ((wparam & 0x0f) != HTVSCROLL) return;
        scrollbar = SB_VERT;
    }
    track_scroll_bar( hwnd, scrollbar, pt );
}

static LRESULT handle_sys_command( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    POINT pos;
    RECT rect;

    TRACE( "hwnd %p WM_SYSCOMMAND %lx %lx\n", hwnd, (long)wparam, lparam );

    if (!is_window_enabled( hwnd )) return 0;

    if (call_hooks( WH_CBT, HCBT_SYSCOMMAND, wparam, lparam, 0 ))
        return 0;

    pos.x = (short)LOWORD( NtUserGetThreadInfo()->message_pos );
    pos.y = (short)HIWORD( NtUserGetThreadInfo()->message_pos );
    NtUserLogicalToPerMonitorDPIPhysicalPoint( hwnd, &pos );
    SetRect( &rect, pos.x, pos.y, pos.x, pos.y );
    rect = map_rect_virt_to_raw( rect, 0 );
    pos.x = rect.left;
    pos.y = rect.top;

    if (!user_driver->pSysCommand( hwnd, wparam, lparam, &pos ))
        return 0;

    switch (wparam & 0xfff0)
    {
    case SC_SIZE:
    case SC_MOVE:
        sys_command_size_move( hwnd, wparam );
        break;

    case SC_MINIMIZE:
        NtUserShowOwnedPopups( hwnd, FALSE );
        NtUserShowWindow( hwnd, SW_MINIMIZE );
        break;

    case SC_MAXIMIZE:
        if (is_iconic(hwnd)) NtUserShowOwnedPopups( hwnd, TRUE );
        NtUserShowWindow( hwnd, SW_MAXIMIZE );
        break;

    case SC_CLOSE:
        return send_message( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
        {
            POINT pt;
            pt.x = (short)LOWORD( lparam );
            pt.y = (short)HIWORD( lparam );
            track_nc_scroll_bar( hwnd, wparam, pt );
        }
        break;

    case SC_MOUSEMENU:
        track_mouse_menu_bar( hwnd, wparam & 0x000F, (short)LOWORD(lparam), (short)HIWORD(lparam) );
        break;

    case SC_KEYMENU:
        track_keyboard_menu_bar( hwnd, wparam, lparam );
        break;

    case SC_RESTORE:
        if (is_iconic( hwnd )) NtUserShowOwnedPopups( hwnd, TRUE );
        NtUserShowWindow( hwnd, SW_RESTORE );
        break;

    case SC_TASKLIST:
    case SC_SCREENSAVE:
        return 1; /* FIXME: handle on client side */

    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
        FIXME( "unimplemented WM_SYSCOMMAND %04lx\n", (long)wparam );
        break;

    default:
        return 1; /* handle on client side */
    }
    return 0;
}

/* Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any). */
static void get_inside_rect( HWND hwnd, enum coords_relative relative, RECT *rect,
                             DWORD style, DWORD ex_style )
{
    get_window_rect_rel( hwnd, relative, rect, get_thread_dpi() );

    /* Remove frame from rectangle */
    if (has_thick_frame( style, ex_style ))
    {
        InflateRect( rect, -get_system_metrics( SM_CXFRAME ), -get_system_metrics( SM_CYFRAME ));
    }
    else if (has_dialog_frame( style, ex_style ))
    {
        InflateRect( rect, -get_system_metrics( SM_CXDLGFRAME ), -get_system_metrics( SM_CYDLGFRAME ));
    }
    else if (has_thin_frame( style ))
    {
        InflateRect( rect, -get_system_metrics( SM_CXBORDER ), -get_system_metrics( SM_CYBORDER ));
    }

    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if ((style & WS_CHILD) && !(ex_style & WS_EX_MDICHILD))
    {
        if (ex_style & WS_EX_CLIENTEDGE)
            InflateRect( rect, -get_system_metrics( SM_CXEDGE ), -get_system_metrics( SM_CYEDGE ));
        if (ex_style & WS_EX_STATICEDGE)
            InflateRect( rect, -get_system_metrics( SM_CXBORDER ), -get_system_metrics( SM_CYBORDER ));
    }
}

void get_sys_popup_pos( HWND hwnd, RECT *rect )
{
    if (is_iconic(hwnd)) get_window_rect( hwnd, rect, get_thread_dpi() );
    else
    {
        DWORD style = get_window_long( hwnd, GWL_STYLE );
        DWORD ex_style = get_window_long( hwnd, GWL_EXSTYLE );

        get_inside_rect( hwnd, COORDS_CLIENT, rect, style, ex_style );
        rect->right = rect->left + get_system_metrics( SM_CYCAPTION ) - 1;
        rect->bottom = rect->top + get_system_metrics( SM_CYCAPTION ) - 1;
        map_window_points( hwnd, 0, (POINT *)rect, 2, get_thread_dpi() );
    }
}

/* Draw a window frame inside the given rectangle, and update the rectangle. */
static void draw_nc_frame( HDC  hdc, RECT  *rect, BOOL  active, DWORD style, DWORD ex_style )
{
    INT width, height;

    if (style & WS_THICKFRAME)
    {
        width  = get_system_metrics( SM_CXFRAME ) - get_system_metrics( SM_CXDLGFRAME );
        height = get_system_metrics( SM_CYFRAME ) - get_system_metrics( SM_CYDLGFRAME );

        NtGdiSelectBrush( hdc, get_sys_color_brush( active ? COLOR_ACTIVEBORDER :
                                                    COLOR_INACTIVEBORDER ));
        /* Draw frame */
        NtGdiPatBlt( hdc, rect->left, rect->top, rect->right - rect->left, height, PATCOPY );
        NtGdiPatBlt( hdc, rect->left, rect->top, width, rect->bottom - rect->top, PATCOPY );
        NtGdiPatBlt( hdc, rect->left, rect->bottom - 1, rect->right - rect->left, -height, PATCOPY );
        NtGdiPatBlt( hdc, rect->right - 1, rect->top, -width, rect->bottom - rect->top, PATCOPY );

        InflateRect( rect, -width, -height );
    }

    /* Now the other bit of the frame */
    if ((style & (WS_BORDER|WS_DLGFRAME)) || (ex_style & WS_EX_DLGMODALFRAME))
    {
        DWORD color;

        width  = get_system_metrics( SM_CXDLGFRAME ) - get_system_metrics( SM_CXEDGE );
        height = get_system_metrics( SM_CYDLGFRAME ) - get_system_metrics( SM_CYEDGE );
        /* This should give a value of 1 that should also work for a border */

        if (ex_style & (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE)) color = COLOR_3DFACE;
        else if (ex_style & WS_EX_STATICEDGE) color = COLOR_WINDOWFRAME;
        else if (style & (WS_DLGFRAME|WS_THICKFRAME)) color = COLOR_3DFACE;
        else color = COLOR_WINDOWFRAME;
        NtGdiSelectBrush( hdc, get_sys_color_brush( color ));

        /* Draw frame */
        NtGdiPatBlt( hdc, rect->left, rect->top,
                     rect->right - rect->left, height, PATCOPY );
        NtGdiPatBlt( hdc, rect->left, rect->top,
                     width, rect->bottom - rect->top, PATCOPY );
        NtGdiPatBlt( hdc, rect->left, rect->bottom - 1,
                     rect->right - rect->left, -height, PATCOPY );
        NtGdiPatBlt( hdc, rect->right - 1, rect->top,
                     -width, rect->bottom - rect->top, PATCOPY );

        InflateRect( rect, -width, -height );
    }
}

static HICON get_nc_icon_for_window( HWND hwnd )
{
    HICON icon = 0;
    WND *win = get_win_ptr( hwnd );

    if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
    {
        icon = win->hIconSmall;
        if (!icon) icon = win->hIcon;
        release_win_ptr( win );
    }
    if (!icon) icon = (HICON) get_class_long_ptr( hwnd, GCLP_HICONSM, FALSE );
    if (!icon) icon = (HICON) get_class_long_ptr( hwnd, GCLP_HICON, FALSE );

    /* If there is no icon specified and this is not a modal dialog, get the default one. */
    if (!icon && !(get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_DLGMODALFRAME))
        icon = LoadImageW( 0, (LPCWSTR)IDI_WINLOGO, IMAGE_ICON, get_system_metrics( SM_CXSMICON ),
                           get_system_metrics( SM_CYSMICON ), LR_DEFAULTCOLOR | LR_SHARED );
    return icon;
}

/* Draws the bar part (ie the big rectangle) of the caption */
static void draw_caption_bar( HDC hdc, const RECT *rect, DWORD style, BOOL active, BOOL gradient )
{
    if (gradient)
    {
        TRIVERTEX vertices[4];
        DWORD left, right;
        int buttons_size = get_system_metrics( SM_CYCAPTION ) - 1;

        static GRADIENT_RECT mesh[] = {{0, 1}, {1, 2}, {2, 3}};

        left  = get_sys_color( active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION );
        right = get_sys_color( active ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION );
        vertices[0].Red   = vertices[1].Red   = GetRValue( left ) << 8;
        vertices[0].Green = vertices[1].Green = GetGValue( left ) << 8;
        vertices[0].Blue  = vertices[1].Blue  = GetBValue( left ) << 8;
        vertices[0].Alpha = vertices[1].Alpha = 0xff00;
        vertices[2].Red   = vertices[3].Red   = GetRValue( right ) << 8;
        vertices[2].Green = vertices[3].Green = GetGValue( right ) << 8;
        vertices[2].Blue  = vertices[3].Blue  = GetBValue( right ) << 8;
        vertices[2].Alpha = vertices[3].Alpha = 0xff00;

        if ((style & WS_SYSMENU) && ((style & WS_MAXIMIZEBOX) || (style & WS_MINIMIZEBOX)))
            buttons_size += 2 * (get_system_metrics( SM_CXSIZE ) + 1);

        /* area behind icon; solid filled with left color */
        vertices[0].x = rect->left;
        vertices[0].y = rect->top;
        if (style & WS_SYSMENU)
            vertices[1].x = min( rect->left + get_system_metrics( SM_CXSMICON ), rect->right );
        else
            vertices[1].x = vertices[0].x;
        vertices[1].y = rect->bottom;

        /* area behind text; gradient */
        vertices[2].x = max( vertices[1].x, rect->right - buttons_size );
        vertices[2].y = rect->top;

        /* area behind buttons; solid filled with right color */
        vertices[3].x = rect->right;
        vertices[3].y = rect->bottom;

        NtGdiGradientFill( hdc, vertices, 4, mesh, 3, GRADIENT_FILL_RECT_H );
    }
    else
    {
        DWORD color = active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION;
        fill_rect( hdc, rect, get_sys_color_brush( color ));
    }
}

/* Draw the system icon */
BOOL draw_nc_sys_button( HWND hwnd, HDC hdc, BOOL down )
{
    HICON icon = get_nc_icon_for_window( hwnd );

    if (icon)
    {
        RECT rect;
        POINT pt;
        DWORD style = get_window_long( hwnd, GWL_STYLE );
        DWORD ex_style = get_window_long( hwnd, GWL_EXSTYLE );

        get_inside_rect( hwnd, COORDS_WINDOW, &rect, style, ex_style );
        pt.x = rect.left + 2;
        pt.y = rect.top + (get_system_metrics( SM_CYCAPTION ) - get_system_metrics( SM_CYSMICON )) / 2;
        NtUserDrawIconEx( hdc, pt.x, pt.y, icon,
                          get_system_metrics( SM_CXSMICON ),
                          get_system_metrics( SM_CYSMICON ), 0, 0, DI_NORMAL );
    }

    return icon != 0;
}

/* Create a square rectangle and return its width */
static int make_square_rect( RECT *src, RECT *dst )
{
    int width  = src->right - src->left;
    int height = src->bottom - src->top;
    int small_diam = width > height ? height : width;

    *dst = *src;

    /* Make it a square box */
    if (width < height)
    {
        dst->top += (height - width) / 2;
        dst->bottom = dst->top + small_diam;
    }
    else if (width > height)
    {
        dst->left += (width - height) / 2;
        dst->right = dst->left + small_diam;
    }

   return small_diam;
}

static void draw_checked_rect( HDC dc, RECT *rect )
{
    if (get_sys_color( COLOR_BTNHIGHLIGHT ) == RGB( 255, 255, 255 ))
    {
      HBRUSH prev_brush;
      DWORD prev_bg;

      fill_rect( dc, rect, get_sys_color_brush( COLOR_BTNFACE ));
      NtGdiGetAndSetDCDword( dc, NtGdiSetBkColor, RGB(255, 255, 255), &prev_bg );
      prev_brush = NtGdiSelectBrush( dc, get_55aa_brush() );
      NtGdiPatBlt( dc, rect->left, rect->top, rect->right-rect->left,
                   rect->bottom-rect->top, 0x00fa0089 );
      NtGdiSelectBrush( dc, prev_brush );
      NtGdiGetAndSetDCDword( dc, NtGdiSetBkColor, prev_bg, NULL );
    }
    else
    {
        fill_rect( dc, rect, get_sys_color_brush( COLOR_BTNHIGHLIGHT ));
    }
}

static BOOL draw_push_button( HDC dc, RECT *r, UINT flags )
{
    RECT rect = *r;
    UINT edge;

    if (flags & (DFCS_PUSHED | DFCS_CHECKED | DFCS_FLAT))
        edge = EDGE_SUNKEN;
    else
        edge = EDGE_RAISED;

    if (flags & DFCS_CHECKED)
    {
        if (flags & DFCS_MONO)
            draw_rect_edge( dc, &rect, edge, BF_MONO|BF_RECT|BF_ADJUST, 1 );
        else
            draw_rect_edge( dc, &rect, edge, (flags & DFCS_FLAT)|BF_RECT|BF_SOFT|BF_ADJUST, 1 );
        if (!(flags & DFCS_TRANSPARENT)) draw_checked_rect( dc, &rect );
    }
    else
    {
        if (flags & DFCS_MONO)
        {
            draw_rect_edge( dc, &rect, edge, BF_MONO|BF_RECT|BF_ADJUST, 1 );
            if (!(flags & DFCS_TRANSPARENT))
                fill_rect( dc, &rect, get_sys_color_brush( COLOR_BTNFACE ));
        }
        else
        {
            UINT edge_flags = BF_RECT | BF_SOFT | (flags & DFCS_FLAT);
            if (!(flags & DFCS_TRANSPARENT)) edge_flags |= BF_MIDDLE;
            draw_rect_edge( dc, r, edge, edge_flags, 1 );
        }
    }

    /* Adjust rectangle if asked */
    if (flags & DFCS_ADJUSTRECT) InflateRect( r, -2, -2 );
    return TRUE;
}

static BOOL draw_frame_caption( HDC dc, RECT *r, UINT flags )
{
    RECT rect;
    int small_diam = make_square_rect( r, &rect ) - 2;
    HFONT prev_font, font;
    int color_idx = flags & DFCS_INACTIVE ? COLOR_BTNSHADOW : COLOR_BTNTEXT;
    int xc = (rect.left + rect.right) / 2;
    int yc = (rect.top + rect.bottom) / 2;
    LOGFONTW lf = { 0 };
    WCHAR str[] = {0, 0};
    DWORD prev_align, prev_bk;
    COLORREF prev_color;
    SIZE size;

    static const WCHAR marlettW[] = {'M','a','r','l','e','t','t',0};

    draw_push_button( dc, r, flags & 0xff00 );

    switch (flags & 0xf)
    {
    case DFCS_CAPTIONCLOSE:    str[0] = 0x72; break;
    case DFCS_CAPTIONHELP:     str[0] = 0x73; break;
    case DFCS_CAPTIONMIN:      str[0] = 0x30; break;
    case DFCS_CAPTIONMAX:      str[0] = 0x31; break;
    case DFCS_CAPTIONRESTORE:  str[0] = 0x32; break;
    default:
        WARN( "Invalid caption; flags=0x%04x\n", flags );
        return FALSE;
    }

    lf.lfHeight = -small_diam;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = SYMBOL_CHARSET;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    memcpy( lf.lfFaceName, marlettW, sizeof(marlettW) );
    font = NtGdiHfontCreate( &lf, sizeof(lf), 0, 0, NULL );
    NtGdiGetAndSetDCDword( dc, NtGdiSetTextAlign, TA_TOP | TA_LEFT, &prev_align );
    NtGdiGetAndSetDCDword( dc, NtGdiSetBkMode, TRANSPARENT, &prev_bk );
    NtGdiGetDCDword( dc, NtGdiGetTextColor, &prev_color );
    prev_font = NtGdiSelectFont( dc, font );
    NtGdiGetTextExtentExW( dc, str, 1, 0, NULL, NULL, &size, 0 );

    if (flags & DFCS_INACTIVE)
    {
        NtGdiGetAndSetDCDword( dc, NtGdiSetTextColor, get_sys_color(COLOR_BTNHIGHLIGHT), NULL );
        NtGdiExtTextOutW( dc, xc-size.cx/2+1, yc-size.cy/2+1, 0, NULL, str, 1, NULL, 0 );
    }
    NtGdiGetAndSetDCDword( dc, NtGdiSetTextColor, get_sys_color( color_idx ), NULL );
    NtGdiExtTextOutW( dc, xc-size.cx/2, yc-size.cy/2, 0, NULL, str, 1, NULL, 0 );

    NtGdiSelectFont(dc, prev_font);
    NtGdiGetAndSetDCDword( dc, NtGdiSetTextColor, prev_color, NULL );
    NtGdiGetAndSetDCDword( dc, NtGdiSetTextAlign, prev_align, NULL );
    NtGdiGetAndSetDCDword( dc, NtGdiSetBkMode, prev_bk, NULL );
    NtGdiDeleteObjectApp( font );

    return TRUE;
}

void draw_menu_button( HWND hwnd, HDC dc, RECT *r, enum NONCLIENT_BUTTON_TYPE type, BOOL down, BOOL grayed )
{
    struct draw_non_client_button_params params;
    void *ret_ptr;
    ULONG ret_len;

    params.hwnd = hwnd;
    params.hdc = dc;
    params.type = type;
    params.rect = *r;
    params.down = down;
    params.grayed = grayed;
    KeUserModeCallback( NtUserDrawNonClientButton, &params, sizeof(params), &ret_ptr, &ret_len );
}

BOOL draw_frame_menu( HDC dc, RECT *r, UINT flags )
{
    RECT rect;
    int dmall_diam = make_square_rect( r, &rect );
    HBRUSH prev_brush;
    HPEN prev_pen;
    POINT points[6];
    int xe, ye;
    int xc, yc;
    BOOL retval = TRUE;
    ULONG count;
    int i;

    fill_rect( dc, r, GetStockObject( WHITE_BRUSH ));

    prev_brush = NtGdiSelectBrush( dc, GetStockObject( BLACK_BRUSH ));
    prev_pen = NtGdiSelectPen( dc, GetStockObject( BLACK_PEN ));

    switch (flags & 0xff)
    {
    case DFCS_MENUARROW:
        i = 187 * dmall_diam / 750;
        points[2].x = rect.left + 468 * dmall_diam/ 750;
        points[2].y = rect.top  + 352 * dmall_diam/ 750 + 1;
        points[0].y = points[2].y - i;
        points[1].y = points[2].y + i;
        points[0].x = points[1].x = points[2].x - i;
        count = 3;
        NtGdiPolyPolyDraw( dc, points, &count, 1, NtGdiPolyPolygon );
        break;

    case DFCS_MENUBULLET:
        xe = rect.left;
        ye = rect.top  + dmall_diam - dmall_diam / 2;
        xc = rect.left + dmall_diam - dmall_diam / 2;
        yc = rect.top  + dmall_diam - dmall_diam / 2;
        i = 234 * dmall_diam / 750;
        i = i < 1 ? 1 : i;
        SetRect( &rect, xc - i + i / 2, yc - i + i / 2, xc + i / 2, yc + i / 2 );
        NtGdiArcInternal( NtGdiPie, dc, rect.left, rect.top, rect.right, rect.bottom,
                          xe, ye, xe, ye );
        break;

    case DFCS_MENUCHECK:
        points[0].x = rect.left + 253 * dmall_diam / 1000;
        points[0].y = rect.top  + 445 * dmall_diam / 1000;
        points[1].x = rect.left + 409 * dmall_diam / 1000;
        points[1].y = points[0].y + (points[1].x - points[0].x);
        points[2].x = rect.left + 690 * dmall_diam / 1000;
        points[2].y = points[1].y - (points[2].x - points[1].x);
        points[3].x = points[2].x;
        points[3].y = points[2].y + 3 * dmall_diam / 16;
        points[4].x = points[1].x;
        points[4].y = points[1].y + 3 * dmall_diam / 16;
        points[5].x = points[0].x;
        points[5].y = points[0].y + 3 * dmall_diam / 16;
        count = 6;
        NtGdiPolyPolyDraw( dc, points, &count, 1, NtGdiPolyPolygon );
        break;

    default:
        WARN( "Invalid menu; flags=0x%04x\n", flags );
        retval = FALSE;
        break;
    }

    NtGdiSelectPen( dc, prev_pen );
    NtGdiSelectBrush( dc, prev_brush );
    return retval;
}

static void draw_close_button( HWND hwnd, HDC hdc, BOOL down, BOOL grayed )
{
    RECT rect;
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    DWORD ex_style = get_window_long( hwnd, GWL_EXSTYLE );
    UINT flags = DFCS_CAPTIONCLOSE;

    get_inside_rect( hwnd, COORDS_WINDOW, &rect, style, ex_style );

    /* A tool window has a smaller Close button */
    if (ex_style & WS_EX_TOOLWINDOW)
    {
        /* Windows does not use SM_CXSMSIZE and SM_CYSMSIZE
         * it uses 11x11 for  the close button in tool window */
        int bmp_height = muldiv( 11, get_dpi_for_window( hwnd ), 96 );
        int bmp_width = bmp_height;
        int caption_height = get_system_metrics( SM_CYSMCAPTION );

        rect.top = rect.top + (caption_height - 1 - bmp_height) / 2;
        rect.left = rect.right - (caption_height + 1 + bmp_width) / 2;
        rect.bottom = rect.top + bmp_height;
        rect.right = rect.left + bmp_width;
    }
    else
    {
        rect.left = rect.right - get_system_metrics( SM_CXSIZE );
        rect.bottom = rect.top + get_system_metrics( SM_CYSIZE ) - 2;
        rect.top += 2;
        rect.right -= 2;
    }

    if (down) flags |= DFCS_PUSHED;
    if (grayed) flags |= DFCS_INACTIVE;
    draw_frame_caption( hdc, &rect, flags );
}

static void draw_max_button( HWND hwnd, HDC hdc, BOOL down, BOOL grayed )
{
    RECT rect;
    UINT flags;
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    DWORD ex_style = get_window_long( hwnd, GWL_EXSTYLE );

    /* never draw maximize box when window has WS_EX_TOOLWINDOW style */
    if (ex_style & WS_EX_TOOLWINDOW) return;

    flags = (style & WS_MAXIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX;

    get_inside_rect( hwnd, COORDS_WINDOW, &rect, style, ex_style );
    if (style & WS_SYSMENU) rect.right -= get_system_metrics( SM_CXSIZE );
    rect.left = rect.right - get_system_metrics( SM_CXSIZE );
    rect.bottom = rect.top + get_system_metrics( SM_CYSIZE ) - 2;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (grayed) flags |= DFCS_INACTIVE;
    draw_frame_caption( hdc, &rect, flags );
}

static void draw_min_button( HWND hwnd, HDC hdc, BOOL down, BOOL grayed )
{
    RECT rect;
    UINT flags;
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    DWORD ex_style = get_window_long( hwnd, GWL_EXSTYLE );

    /* never draw minimize box when window has WS_EX_TOOLWINDOW style */
    if (ex_style & WS_EX_TOOLWINDOW) return;

    flags = (style & WS_MINIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMIN;

    get_inside_rect( hwnd, COORDS_WINDOW, &rect, style, ex_style );
    if (style & WS_SYSMENU)
        rect.right -= get_system_metrics( SM_CXSIZE );
    if (style & (WS_MAXIMIZEBOX|WS_MINIMIZEBOX))
        rect.right -= get_system_metrics( SM_CXSIZE ) - 2;
    rect.left = rect.right - get_system_metrics( SM_CXSIZE );
    rect.bottom = rect.top + get_system_metrics( SM_CYSIZE ) - 2;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (grayed) flags |= DFCS_INACTIVE;
    draw_frame_caption( hdc, &rect, flags );
}

static void draw_nc_caption( HDC hdc, RECT *rect, HWND hwnd, DWORD  style,
                             DWORD  ex_style, BOOL active )
{
    RECT  r = *rect;
    WCHAR buffer[256];
    HPEN prev_pen;
    HMENU sys_menu;
    BOOL gradient = FALSE;
    UINT pen_color = COLOR_3DFACE;
    int len;

    if ((ex_style & (WS_EX_STATICEDGE|WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        pen_color = COLOR_WINDOWFRAME;
    prev_pen = NtGdiSelectPen( hdc, get_sys_color_pen( pen_color ));
    NtGdiMoveTo( hdc, r.left, r.bottom - 1, NULL );
    NtGdiLineTo( hdc, r.right, r.bottom - 1 );
    NtGdiSelectPen( hdc, prev_pen );
    r.bottom--;

    NtUserSystemParametersInfo( SPI_GETGRADIENTCAPTIONS, 0, &gradient, 0 );
    draw_caption_bar( hdc, &r, style, active, gradient );

    if ((style & WS_SYSMENU) && !(ex_style & WS_EX_TOOLWINDOW))
    {
        if (draw_nc_sys_button( hwnd, hdc, FALSE ))
            r.left += get_system_metrics( SM_CXSMICON ) + 2;
    }

    if (style & WS_SYSMENU)
    {
        UINT state;

        /* Go get the sysmenu */
        sys_menu = NtUserGetSystemMenu( hwnd, FALSE );
        state = get_menu_state( sys_menu, SC_CLOSE, MF_BYCOMMAND );

        /* Draw a grayed close button if disabled or if SC_CLOSE is not there */
        draw_close_button( hwnd, hdc, FALSE,
                           (state & (MF_DISABLED | MF_GRAYED)) || (state == 0xFFFFFFFF) );
        r.right -= get_system_metrics( SM_CYCAPTION ) - 1;

        if ((style & WS_MAXIMIZEBOX) || (style & WS_MINIMIZEBOX))
        {
            draw_max_button( hwnd, hdc, FALSE, !(style & WS_MAXIMIZEBOX) );
            r.right -= get_system_metrics( SM_CXSIZE ) + 1;

            draw_min_button( hwnd, hdc, FALSE, !(style & WS_MINIMIZEBOX) );
            r.right -= get_system_metrics( SM_CXSIZE ) + 1;
        }
    }

    len = get_window_text( hwnd, buffer, ARRAY_SIZE( buffer ));
    if (len)
    {
        NONCLIENTMETRICSW nclm;
        HFONT hFont, hOldFont;
        nclm.cbSize = sizeof(nclm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &nclm, 0 );
        if (ex_style & WS_EX_TOOLWINDOW)
            hFont = NtGdiHfontCreate( &nclm.lfSmCaptionFont, sizeof(nclm.lfSmCaptionFont), 0, 0, NULL );
        else
            hFont = NtGdiHfontCreate( &nclm.lfCaptionFont, sizeof(nclm.lfCaptionFont), 0, 0, NULL );
        hOldFont = NtGdiSelectFont( hdc, hFont );
        if (active)
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_CAPTIONTEXT ), NULL );
        else
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_INACTIVECAPTIONTEXT ), NULL );
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkMode, TRANSPARENT, NULL );
        r.left += 2;
        DrawTextW( hdc, buffer, -1, &r,
                     DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT );
        NtGdiDeleteObjectApp( NtGdiSelectFont( hdc, hOldFont ));
    }
}

/***********************************************************************
 *           NtUserDrawCaptionTemp   (win32u.@)
 */
BOOL WINAPI NtUserDrawCaptionTemp( HWND hwnd, HDC hdc, const RECT *rect, HFONT font,
                                   HICON icon, const WCHAR *str, UINT flags )
{
    RECT rc = *rect;

    TRACE( "(%p,%p,%p,%p,%p,%s,%08x)\n", hwnd, hdc, rect, font, icon, debugstr_w(str), flags );

    /* drawing background */
    if (flags & DC_INBUTTON)
    {
        fill_rect( hdc, &rc, get_sys_color_brush( COLOR_3DFACE ));

        if (flags & DC_ACTIVE)
        {
            HBRUSH hbr = NtGdiSelectBrush( hdc, get_55aa_brush() );
            NtGdiPatBlt( hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0xfa0089 );
            NtGdiSelectBrush( hdc, hbr );
        }
    }
    else
    {
        DWORD style = get_window_long( hwnd, GWL_STYLE );
        draw_caption_bar( hdc, &rc, style, flags & DC_ACTIVE, flags & DC_GRADIENT );
    }

    /* drawing icon */
    if ((flags & DC_ICON) && !(flags & DC_SMALLCAP))
    {
        POINT pt;

        pt.x = rc.left + 2;
        pt.y = (rc.bottom + rc.top - get_system_metrics( SM_CYSMICON )) / 2;

        if (!icon) icon = get_nc_icon_for_window( hwnd );
        NtUserDrawIconEx( hdc, pt.x, pt.y, icon, get_system_metrics( SM_CXSMICON ),
                          get_system_metrics( SM_CYSMICON ), 0, 0, DI_NORMAL );
        rc.left = pt.x + get_system_metrics( SM_CXSMICON );
    }

    /* drawing text */
    if (flags & DC_TEXT)
    {
        HFONT prev_font;
        WCHAR text[128];
        DWORD color;

        if (flags & DC_INBUTTON)
            color = COLOR_BTNTEXT;
        else if (flags & DC_ACTIVE)
            color = COLOR_CAPTIONTEXT;
        else
            color = COLOR_INACTIVECAPTIONTEXT;
        NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( color ), NULL );
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkMode, TRANSPARENT, NULL );

        if (font)
            prev_font = NtGdiSelectFont( hdc, font );
        else
        {
            NONCLIENTMETRICSW nclm;
            HFONT new_font;
            LOGFONTW *lf;
            nclm.cbSize = sizeof(NONCLIENTMETRICSW);
            NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &nclm, 0 );
            lf = (flags & DC_SMALLCAP) ? &nclm.lfSmCaptionFont : &nclm.lfCaptionFont;
            new_font = NtGdiHfontCreate( lf, sizeof(*lf), 0, 0, NULL );
            prev_font = NtGdiSelectFont( hdc, new_font );
        }

        if (!str)
        {
            if (!get_window_text( hwnd, text, ARRAY_SIZE( text ))) text[0] = 0;
            str = text;
        }
        rc.left += 2;
        DrawTextW( hdc, str, -1, &rc, ((flags & 0x4000) ? DT_CENTER : DT_LEFT) |
                   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS );

        if (font)
            NtGdiSelectFont( hdc, prev_font );
        else
            NtGdiDeleteObjectApp( NtGdiSelectFont( hdc, prev_font ));
    }

    if (flags & 0x2000) FIXME( "undocumented flag (0x2000)!\n" );
    return FALSE;
}

/* Paint the non-client area for windows */
static void nc_paint( HWND hwnd, HRGN clip )
{
    HDC hdc;
    RECT rfuzz, rect, clip_rect;
    BOOL active;
    WND *win;
    DWORD style, ex_style;
    WORD flags;
    HRGN hrgn;
    RECT rectClient;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS) return;
    style = win->dwStyle;
    ex_style = win->dwExStyle;
    flags = win->flags;
    release_win_ptr( win );

    active = flags & WIN_NCACTIVATED;

    TRACE( "%p %d\n", hwnd, active );

    get_client_rect_rel( hwnd, COORDS_SCREEN, &rectClient, get_thread_dpi() );
    hrgn = NtGdiCreateRectRgn( rectClient.left, rectClient.top,
                               rectClient.right, rectClient.bottom );

    if (clip > (HRGN)1)
    {
        NtGdiCombineRgn( hrgn, clip, hrgn, RGN_DIFF );
        hdc = NtUserGetDCEx( hwnd, hrgn, DCX_USESTYLE | DCX_WINDOW | DCX_INTERSECTRGN );
    }
    else
    {
        hdc = NtUserGetDCEx( hwnd, hrgn, DCX_USESTYLE | DCX_WINDOW | DCX_EXCLUDERGN );
    }

    if (!hdc)
    {
        NtGdiDeleteObjectApp( hrgn );
        return;
    }

    get_window_rect_rel( hwnd, COORDS_WINDOW, &rect, get_thread_dpi() );
    NtGdiGetAppClipBox( hdc, &clip_rect );

    NtGdiSelectPen( hdc, get_sys_color_pen( COLOR_WINDOWFRAME ));

    if (has_static_outer_frame( ex_style ))
        draw_rect_edge( hdc, &rect, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST, 1 );
    else if (has_big_frame( style, ex_style ))
        draw_rect_edge( hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST, 1 );

    draw_nc_frame( hdc, &rect, active, style, ex_style );

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        RECT r = rect;
        if (ex_style & WS_EX_TOOLWINDOW)
        {
            r.bottom = rect.top + get_system_metrics( SM_CYSMCAPTION );
            rect.top += get_system_metrics( SM_CYSMCAPTION );
        }
        else {
            r.bottom = rect.top + get_system_metrics( SM_CYCAPTION );
            rect.top += get_system_metrics( SM_CYCAPTION );
        }

        if (intersect_rect( &rfuzz, &r, &clip_rect ))
            draw_nc_caption( hdc, &r, hwnd, style, ex_style, active );
    }

    if (has_menu( hwnd, style ))
    {
        RECT r = rect;
        HMENU menu;

        r.bottom = rect.top + get_system_metrics( SM_CYMENU );

        TRACE( "drawing menu with rect %s\n", wine_dbgstr_rect( &r ));

        menu = get_menu( hwnd );
        if (!is_menu( menu )) rect.top += get_system_metrics( SM_CYMENU );
        else rect.top += NtUserDrawMenuBarTemp( hwnd, hdc, &r, menu, NULL );
    }

    TRACE( "rect after menu %s\n", wine_dbgstr_rect( &rect ));

    if (ex_style & WS_EX_CLIENTEDGE)
        draw_rect_edge( hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST, 1 );

    /* Draw the scroll-bars */
    draw_nc_scrollbar( hwnd, hdc, style & WS_HSCROLL, style & WS_VSCROLL );

    /* Draw the "size-box" */
    if ((style & WS_VSCROLL) && (style & WS_HSCROLL))
    {
        RECT r = rect;
        if ((ex_style & WS_EX_LEFTSCROLLBAR) != 0)
            r.right = r.left + get_system_metrics( SM_CXVSCROLL ) + 1;
        else
            r.left = r.right - get_system_metrics( SM_CXVSCROLL ) + 1;
        r.top  = r.bottom - get_system_metrics( SM_CYHSCROLL ) + 1;
        fill_rect( hdc, &r, get_sys_color_brush( COLOR_BTNFACE ) );
    }

    NtUserReleaseDC( hwnd, hdc );
}

static LRESULT handle_nc_paint( HWND hwnd , HRGN clip )
{
    HWND parent = NtUserGetAncestor( hwnd, GA_PARENT );
    DWORD style = get_window_long( hwnd, GWL_STYLE );

    if (style & WS_VISIBLE)
    {
        nc_paint( hwnd, clip );

        if (parent == get_desktop_window())
            NtUserPostMessage( parent, WM_PARENTNOTIFY, WM_NCPAINT, (LPARAM)hwnd );
    }
    return 0;
}

static LRESULT handle_nc_activate( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    /* Lotus Notes draws menu descriptions in the caption of its main
     * window. When it wants to restore original "system" view, it just
     * sends WM_NCACTIVATE message to itself. Any optimizations here in
     * attempt to minimize redrawings lead to a not restored caption.
     */
    if (wparam) win_set_flags( hwnd, WIN_NCACTIVATED, 0 );
    else win_set_flags( hwnd, 0, WIN_NCACTIVATED );

    /* This isn't documented but is reproducible in at least XP SP2 and
     * Outlook 2007 depends on it
     */
    if (lparam != -1)
    {
        nc_paint( hwnd, (HRGN)1 );

        if (NtUserGetAncestor( hwnd, GA_PARENT ) == get_desktop_window())
            NtUserPostMessage( get_desktop_window(), WM_PARENTNOTIFY, WM_NCACTIVATE, (LPARAM)hwnd );
    }

    return TRUE;
}

static void handle_nc_calc_size( HWND hwnd, WPARAM wparam, RECT *win_rect )
{
    RECT rect = { 0, 0, 0, 0 };
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    DWORD ex_style = get_window_long( hwnd, GWL_EXSTYLE );

    if (!win_rect) return;

    if (!(style & WS_MINIMIZE))
    {
        adjust_window_rect( &rect, style, FALSE, ex_style & ~WS_EX_CLIENTEDGE, get_system_dpi() );

        win_rect->left   -= rect.left;
        win_rect->top    -= rect.top;
        win_rect->right  -= rect.right;
        win_rect->bottom -= rect.bottom;

        if (((style & (WS_CHILD | WS_POPUP)) != WS_CHILD) && get_menu( hwnd ))
        {
            TRACE( "getting menu bar height with hwnd %p, width %d, at (%d, %d)\n",
                   hwnd, win_rect->right - win_rect->left, -rect.left, -rect.top );

            win_rect->top += get_menu_bar_height( hwnd, win_rect->right - win_rect->left,
                                                  -rect.left, -rect.top );
        }

        if (ex_style & WS_EX_CLIENTEDGE)
            if (win_rect->right - win_rect->left > 2 * get_system_metrics( SM_CXEDGE ) &&
                   win_rect->bottom - win_rect->top > 2 * get_system_metrics( SM_CYEDGE ))
                InflateRect( win_rect, -get_system_metrics( SM_CXEDGE ),
                             -get_system_metrics( SM_CYEDGE ));

        if ((style & WS_VSCROLL) &&
            win_rect->right - win_rect->left >= get_system_metrics( SM_CXVSCROLL ))
        {
            /* rectangle is in screen coords when wparam is false */
            if (!wparam && (ex_style & WS_EX_LAYOUTRTL)) ex_style ^= WS_EX_LEFTSCROLLBAR;

            if (ex_style & WS_EX_LEFTSCROLLBAR)
                win_rect->left  += get_system_metrics( SM_CXVSCROLL );
            else
                win_rect->right -= get_system_metrics( SM_CXVSCROLL );
        }

        if ((style & WS_HSCROLL) &&
            win_rect->bottom - win_rect->top > get_system_metrics( SM_CYHSCROLL ))
        {
            win_rect->bottom -= get_system_metrics( SM_CYHSCROLL );
        }

        if (win_rect->top > win_rect->bottom) win_rect->bottom = win_rect->top;
        if (win_rect->left > win_rect->right) win_rect->right = win_rect->left;
    }
    else
    {
        win_rect->right = win_rect->left;
        win_rect->bottom = win_rect->top;
    }
}

LRESULT handle_nc_hit_test( HWND hwnd, POINT pt )
{
    struct window_rects rects;
    DWORD style, ex_style;

    TRACE( "hwnd %p pt %d,%d\n", hwnd, pt.x, pt.y );

    get_window_rects( hwnd, COORDS_SCREEN, &rects, get_thread_dpi() );
    if (!PtInRect( &rects.window, pt )) return HTNOWHERE;

    style = get_window_long( hwnd, GWL_STYLE );
    ex_style = get_window_long( hwnd, GWL_EXSTYLE );

    if (PtInRect( &rects.client, pt )) return HTCLIENT;

    /* Check borders */
    if (has_thick_frame( style, ex_style ))
    {
        InflateRect( &rects.window, -get_system_metrics( SM_CXFRAME ), -get_system_metrics( SM_CYFRAME ));
        if (!PtInRect( &rects.window, pt ))
        {
            /* Check top sizing border */
            if (pt.y < rects.window.top)
            {
                if (pt.x < rects.window.left + get_system_metrics( SM_CXSIZE )) return HTTOPLEFT;
                if (pt.x >= rects.window.right - get_system_metrics( SM_CXSIZE )) return HTTOPRIGHT;
                return HTTOP;
            }
            /* Check bottom sizing border */
            if (pt.y >= rects.window.bottom)
            {
                if (pt.x < rects.window.left + get_system_metrics( SM_CXSIZE )) return HTBOTTOMLEFT;
                if (pt.x >= rects.window.right - get_system_metrics( SM_CXSIZE )) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            /* Check left sizing border */
            if (pt.x < rects.window.left)
            {
                if (pt.y < rects.window.top + get_system_metrics( SM_CYSIZE )) return HTTOPLEFT;
                if (pt.y >= rects.window.bottom - get_system_metrics( SM_CYSIZE )) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            /* Check right sizing border */
            if (pt.x >= rects.window.right)
            {
                if (pt.y < rects.window.top + get_system_metrics( SM_CYSIZE )) return HTTOPRIGHT;
                if (pt.y >= rects.window.bottom-get_system_metrics( SM_CYSIZE )) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else  /* No thick frame */
    {
        if (has_dialog_frame( style, ex_style ))
            InflateRect( &rects.window, -get_system_metrics( SM_CXDLGFRAME ),
                         -get_system_metrics( SM_CYDLGFRAME ));
        else if (has_thin_frame( style ))
            InflateRect(&rects.window, -get_system_metrics( SM_CXBORDER ),
                        -get_system_metrics( SM_CYBORDER ));
        if (!PtInRect( &rects.window, pt )) return HTBORDER;
    }

    /* Check caption */
    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (ex_style & WS_EX_TOOLWINDOW)
            rects.window.top += get_system_metrics( SM_CYSMCAPTION ) - 1;
        else
            rects.window.top += get_system_metrics( SM_CYCAPTION ) - 1;
        if (!PtInRect( &rects.window, pt ))
        {
            BOOL min_or_max_box = (style & WS_SYSMENU) && (style & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX));
            if (ex_style & WS_EX_LAYOUTRTL)
            {
                /* Check system menu */
                if ((style & WS_SYSMENU) && !(ex_style & WS_EX_TOOLWINDOW) &&
                    get_nc_icon_for_window( hwnd ))
                {
                    rects.window.right -= get_system_metrics( SM_CYCAPTION ) - 1;
                    if (pt.x > rects.window.right) return HTSYSMENU;
                }

                /* Check close button */
                if (style & WS_SYSMENU)
                {
                    rects.window.left += get_system_metrics( SM_CYCAPTION );
                    if (pt.x < rects.window.left) return HTCLOSE;
                }

                if (min_or_max_box && !(ex_style & WS_EX_TOOLWINDOW))
                {
                    /* Check maximize box */
                    rects.window.left += get_system_metrics( SM_CXSIZE );
                    if (pt.x < rects.window.left) return HTMAXBUTTON;

                    /* Check minimize box */
                    rects.window.left += get_system_metrics( SM_CXSIZE );
                    if (pt.x < rects.window.left) return HTMINBUTTON;
                }
            }
            else
            {
                /* Check system menu */
                if ((style & WS_SYSMENU) && !(ex_style & WS_EX_TOOLWINDOW) &&
                    get_nc_icon_for_window( hwnd ))
                {
                    rects.window.left += get_system_metrics( SM_CYCAPTION ) - 1;
                    if (pt.x < rects.window.left) return HTSYSMENU;
                }

                /* Check close button */
                if (style & WS_SYSMENU)
                {
                    rects.window.right -= get_system_metrics( SM_CYCAPTION );
                    if (pt.x > rects.window.right) return HTCLOSE;
                }

                if (min_or_max_box && !(ex_style & WS_EX_TOOLWINDOW))
                {
                    /* Check maximize box */
                    rects.window.right -= get_system_metrics( SM_CXSIZE );
                    if (pt.x > rects.window.right) return HTMAXBUTTON;

                    /* Check minimize box */
                    rects.window.right -= get_system_metrics( SM_CXSIZE );
                    if (pt.x > rects.window.right) return HTMINBUTTON;
                }
            }
            return HTCAPTION;
        }
    }

    /* Check menu bar */
    if (has_menu( hwnd, style ) && (pt.y < rects.client.top) &&
        (pt.x >= rects.client.left) && (pt.x < rects.client.right))
        return HTMENU;

    /* Check vertical scroll bar */
    if (ex_style & WS_EX_LAYOUTRTL) ex_style ^= WS_EX_LEFTSCROLLBAR;
    if (style & WS_VSCROLL)
    {
        if (ex_style & WS_EX_LEFTSCROLLBAR)
            rects.client.left -= get_system_metrics( SM_CXVSCROLL );
        else
            rects.client.right += get_system_metrics( SM_CXVSCROLL );
        if (PtInRect( &rects.client, pt )) return HTVSCROLL;
    }

    /* Check horizontal scroll bar */
    if (style & WS_HSCROLL)
    {
        rects.client.bottom += get_system_metrics( SM_CYHSCROLL );
        if (PtInRect( &rects.client, pt ))
        {
            /* Check size box */
            if ((style & WS_VSCROLL) &&
                ((ex_style & WS_EX_LEFTSCROLLBAR)
                 ? (pt.x <= rects.client.left + get_system_metrics( SM_CXVSCROLL ))
                 : (pt.x >= rects.client.right - get_system_metrics( SM_CXVSCROLL ))))
                return HTSIZE;
            return HTHSCROLL;
        }
    }

    /* Has to return HTNOWHERE if nothing was found
       Could happen when a window has a customized non client area */
    return HTNOWHERE;
}

static void track_min_max_box( HWND hwnd, WORD wparam )
{
    HDC hdc = NtUserGetWindowDC( hwnd );
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    HMENU sys_menu = NtUserGetSystemMenu(hwnd, FALSE);
    void (*paint_button)( HWND, HDC, BOOL, BOOL );
    BOOL pressed = TRUE;
    UINT state;
    MSG msg;

    if (wparam == HTMINBUTTON)
    {
        /* If the style is not present, do nothing */
        if (!(style & WS_MINIMIZEBOX)) return;

        /* Check if the sysmenu item for minimize is there  */
        state = get_menu_state( sys_menu, SC_MINIMIZE, MF_BYCOMMAND );
        paint_button = draw_min_button;
    }
    else
    {
        /* If the style is not present, do nothing */
        if (!(style & WS_MAXIMIZEBOX)) return;

        /* Check if the sysmenu item for maximize is there  */
        state = get_menu_state( sys_menu, SC_MAXIMIZE, MF_BYCOMMAND );
        paint_button = draw_max_button;
    }

    NtUserSetCapture( hwnd );
    paint_button( hwnd, hdc, TRUE, FALSE);

    for (;;)
    {
        BOOL oldstate = pressed;

        if (!NtUserGetMessage( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST )) break;
        if (NtUserCallMsgFilter( &msg, MSGF_MAX )) continue;
        if(msg.message == WM_LBUTTONUP) break;
        if(msg.message != WM_MOUSEMOVE) continue;

        pressed = handle_nc_hit_test( hwnd, msg.pt ) == wparam;
        if (pressed != oldstate) paint_button( hwnd, hdc, pressed, FALSE);
    }

    if (pressed) paint_button( hwnd, hdc, FALSE, FALSE );

    NtUserReleaseCapture();
    NtUserReleaseDC( hwnd, hdc );

    /* If the minimize or maximize items of the sysmenu are not there
     * or if the style is not present, do nothing */
    if (!pressed || state == 0xffffffff) return;

    if (wparam == HTMINBUTTON)
        send_message( hwnd, WM_SYSCOMMAND,
                      is_iconic( hwnd ) ? SC_RESTORE : SC_MINIMIZE, MAKELONG( msg.pt.x, msg.pt.y ));
    else
        send_message( hwnd, WM_SYSCOMMAND,
                      is_zoomed( hwnd ) ? SC_RESTORE : SC_MAXIMIZE, MAKELONG( msg.pt.x, msg.pt.y ));
}

static void track_close_button( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    HMENU sys_menu;
    BOOL pressed = TRUE;
    UINT state;
    MSG msg;
    HDC hdc;

    if (!(sys_menu = NtUserGetSystemMenu( hwnd, FALSE ))) return;

    state = get_menu_state( sys_menu, SC_CLOSE, MF_BYCOMMAND );

    /* If the close item of the sysmenu is disabled or not present do nothing */
    if((state & MF_DISABLED) || (state & MF_GRAYED) || state == 0xFFFFFFFF) return;
    hdc = NtUserGetWindowDC( hwnd );
    NtUserSetCapture( hwnd );
    draw_close_button( hwnd, hdc, TRUE, FALSE );

    for (;;)
    {
        BOOL oldstate = pressed;

        if (!NtUserGetMessage( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST )) break;
        if (NtUserCallMsgFilter( &msg, MSGF_MAX )) continue;
        if (msg.message == WM_LBUTTONUP) break;
        if (msg.message != WM_MOUSEMOVE) continue;

        pressed = handle_nc_hit_test( hwnd, msg.pt ) == wparam;
        if (pressed != oldstate) draw_close_button( hwnd, hdc, pressed, FALSE );
    }

    if (pressed) draw_close_button( hwnd, hdc, FALSE, FALSE );

    NtUserReleaseCapture();
    NtUserReleaseDC( hwnd, hdc );
    if (pressed) send_message( hwnd, WM_SYSCOMMAND, SC_CLOSE, lparam );
}

static LRESULT handle_nc_lbutton_down( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );

    switch (wparam)  /* Hit test */
    {
    case HTCAPTION:
        {
            HWND top = hwnd, parent;
            for (;;)
            {
                if ((get_window_long( top, GWL_STYLE ) & (WS_POPUP | WS_CHILD)) != WS_CHILD)
                    break;
                parent = NtUserGetAncestor( top, GA_PARENT );
                if (!parent || parent == get_desktop_window()) break;
                top = parent;
            }

            if (set_foreground_window( top, TRUE ) || (get_active_window() == top))
                send_message( hwnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lparam );
            break;
        }

    case HTSYSMENU:
        if (style & WS_SYSMENU)
        {
            HDC hdc = NtUserGetWindowDC( hwnd );
            draw_nc_sys_button( hwnd, hdc, TRUE );
            NtUserReleaseDC( hwnd, hdc );
            send_message( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, lparam );
        }
        break;

    case HTMENU:
        send_message( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU, lparam );
        break;

    case HTHSCROLL:
        send_message( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lparam );
        break;

    case HTVSCROLL:
        send_message( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lparam );
        break;

    case HTMINBUTTON:
    case HTMAXBUTTON:
        track_min_max_box( hwnd, wparam );
        break;

    case HTCLOSE:
        track_close_button( hwnd, wparam, lparam );
        break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
        send_message( hwnd, WM_SYSCOMMAND, SC_SIZE + wparam - (HTLEFT - WMSZ_LEFT), lparam );
        break;

    case HTBORDER:
        break;
    }
    return 0;
}

static LRESULT handle_nc_rbutton_down( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    int hittest = wparam;
    MSG msg;

    switch (hittest)
    {
    case HTCAPTION:
    case HTSYSMENU:
        NtUserSetCapture( hwnd );
        for (;;)
        {
            if (!NtUserGetMessage( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST )) break;
            if (NtUserCallMsgFilter( &msg, MSGF_MAX )) continue;
            if (msg.message == WM_RBUTTONUP)
            {
                hittest = handle_nc_hit_test( hwnd, msg.pt );
                break;
            }
        }
        NtUserReleaseCapture();
        if (hittest == HTCAPTION || hittest == HTSYSMENU)
            send_message( hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELPARAM( msg.pt.x, msg.pt.y ));
        break;
    }
    return 0;
}

static LRESULT handle_nc_button_dbl_click( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    /* if this is an icon, send a restore since we are handling a double click */
    if (is_iconic(hwnd))
    {
        send_message( hwnd, WM_SYSCOMMAND, SC_RESTORE, lparam );
        return 0;
    }

    switch (wparam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */
        if (get_window_long( hwnd, GWL_STYLE ) & WS_MAXIMIZEBOX)
            send_message( hwnd, WM_SYSCOMMAND,
                          is_zoomed( hwnd ) ? SC_RESTORE : SC_MAXIMIZE, lparam );
        break;

    case HTSYSMENU:
        {
            HMENU hSysMenu = NtUserGetSystemMenu( hwnd, FALSE );
            UINT state = get_menu_state( hSysMenu, SC_CLOSE, MF_BYCOMMAND );

            /* If the close item of the sysmenu is disabled or not present do nothing */
            if ((state & (MF_DISABLED | MF_GRAYED)) || state == 0xffffffff)
                break;

            send_message( hwnd, WM_SYSCOMMAND, SC_CLOSE, lparam );
            break;
        }

    case HTHSCROLL:
        send_message( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lparam );
        break;

    case HTVSCROLL:
        send_message( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lparam );
        break;
    }
    return 0;
}

static HBRUSH handle_control_color( HDC hdc, UINT type )
{
    if (type == CTLCOLOR_SCROLLBAR)
    {
        HBRUSH hb = get_sys_color_brush( COLOR_SCROLLBAR );
        COLORREF bk = get_sys_color( COLOR_3DHILIGHT );
        NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_3DFACE ), NULL );
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, bk, NULL );

        /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
         * we better use 0x55aa bitmap brush to make scrollbar's background
         * look different from the window background.
         */
        if (bk == get_sys_color( COLOR_WINDOW )) return get_55aa_brush();

        NtGdiUnrealizeObject( hb );
        return hb;
    }

    NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_WINDOWTEXT ), NULL );

    if (type == CTLCOLOR_EDIT || type == CTLCOLOR_LISTBOX)
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, get_sys_color( COLOR_WINDOW ), NULL );
    else
    {
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, get_sys_color( COLOR_3DFACE ), NULL);
        return get_sys_color_brush( COLOR_3DFACE );
    }

    return get_sys_color_brush( COLOR_WINDOW );
}

static LRESULT handle_nc_mouse_move( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    RECT rect;
    POINT pt;

    TRACE( "hwnd=%p wparam=%#lx lparam=%#lx\n", hwnd, (long)wparam, lparam );

    if (wparam != HTHSCROLL && wparam != HTVSCROLL)
        return 0;

    get_window_rect_rel( hwnd, COORDS_CLIENT, &rect, get_thread_dpi() );

    pt.x = (short)LOWORD( lparam );
    pt.y = (short)HIWORD( lparam );
    screen_to_client( hwnd, &pt );
    pt.x -= rect.left;
    pt.y -= rect.top;
    handle_scroll_event( hwnd, wparam == HTHSCROLL ? SB_HORZ : SB_VERT, WM_NCMOUSEMOVE, pt );
    return 0;
}

static LRESULT handle_nc_mouse_leave( HWND hwnd )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    POINT pt = {0, 0};

    TRACE( "hwnd=%p\n", hwnd );

    if (style & WS_HSCROLL)
        handle_scroll_event( hwnd, SB_HORZ, WM_NCMOUSELEAVE, pt );
    if (style & WS_VSCROLL)
        handle_scroll_event( hwnd, SB_VERT, WM_NCMOUSELEAVE, pt );
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

            if (cs->style & (WS_HSCROLL | WS_VSCROLL))
            {
                SCROLLINFO si = { .cbSize = sizeof(si), .fMask = SIF_ALL, .nMax = 100 };
                NtUserSetScrollInfo( hwnd, SB_HORZ, &si, FALSE );
                NtUserSetScrollInfo( hwnd, SB_VERT, &si, FALSE );
            }

            result = 1;
        }
        break;

    case WM_NCDESTROY:
        {
            WND *win = get_win_ptr( hwnd );
            if (!win) return 0;
            free( win->text );
            win->text = NULL;
            free( win->pScroll );
            win->pScroll = NULL;
            release_win_ptr( win );
            break;
        }

    case WM_NCCALCSIZE:
        handle_nc_calc_size( hwnd, wparam, (RECT *)lparam );
        break;

    case WM_NCHITTEST:
        {
            POINT pt;
            pt.x = (short)LOWORD( lparam );
            pt.y = (short)HIWORD( lparam );
            return handle_nc_hit_test( hwnd, pt );
        }

    case WM_NCPAINT:
        return handle_nc_paint( hwnd, (HRGN)wparam );

    case WM_NCACTIVATE:
        return handle_nc_activate( hwnd, wparam, lparam );

    case WM_NCLBUTTONDOWN:
        return handle_nc_lbutton_down( hwnd, wparam, lparam );

    case WM_NCRBUTTONDOWN:
        return handle_nc_rbutton_down( hwnd, wparam, lparam );

    case WM_LBUTTONDBLCLK:
        return handle_nc_button_dbl_click( hwnd, HTCLIENT, lparam );

    case WM_NCLBUTTONDBLCLK:
        return handle_nc_button_dbl_click( hwnd, wparam, lparam );

    case WM_NCMOUSEMOVE:
        result = handle_nc_mouse_move( hwnd, wparam, lparam );
        break;

    case WM_NCMOUSELEAVE:
        result = handle_nc_mouse_leave( hwnd );
        break;

    case WM_RBUTTONUP:
        {
            POINT pt;
            pt.x = (short)LOWORD( lparam );
            pt.y = (short)HIWORD( lparam );
            client_to_screen( hwnd, &pt );
            send_message( hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELPARAM( pt.x, pt.y ));
        }
        break;

    case WM_NCRBUTTONUP:
        break;

    case WM_XBUTTONUP:
    case WM_NCXBUTTONUP:
        if (HIWORD(wparam) == XBUTTON1 || HIWORD(wparam) == XBUTTON2)
        {
            send_message( hwnd, WM_APPCOMMAND, (WPARAM)hwnd,
                          MAKELPARAM( LOWORD( wparam ), FAPPCOMMAND_MOUSE | HIWORD( wparam )));
        }
        break;

    case WM_CONTEXTMENU:
        if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)
            send_message( get_parent( hwnd ), msg, (WPARAM)hwnd, lparam );
        else
        {
            LONG hitcode;
            POINT pt;
            pt.x = (short)LOWORD( lparam );
            pt.y = (short)HIWORD( lparam );
            hitcode = handle_nc_hit_test( hwnd, pt );

            /* Track system popup if click was in the caption area. */
            if (hitcode == HTCAPTION || hitcode == HTSYSMENU)
                NtUserTrackPopupMenuEx( NtUserGetSystemMenu( hwnd, FALSE ),
                                        TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                        pt.x, pt.y, hwnd, NULL );
        }
        break;

    case WM_POPUPSYSTEMMENU:
        /* This is an undocumented message used by the windows taskbar to
         * display the system menu of windows that belong to other processes. */
        NtUserTrackPopupMenuEx( NtUserGetSystemMenu( hwnd, FALSE ), TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                (short)LOWORD(lparam), (short)HIWORD(lparam), hwnd, NULL );
        return 0;

    case WM_WINDOWPOSCHANGING:
        return handle_window_pos_changing( hwnd, (WINDOWPOS *)lparam );

    case WM_WINDOWPOSCHANGED:
        handle_window_pos_changed( hwnd, (const WINDOWPOS *)lparam );
        break;

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

                    get_client_rect( hwnd, &rc, get_thread_dpi() );
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
        if (wparam) set_window_style_bits( hwnd, WS_VISIBLE, 0 );
        else
        {
            NtUserRedrawWindow( hwnd, NULL, 0, RDW_ALLCHILDREN | RDW_VALIDATE );
            set_window_style_bits( hwnd, 0, WS_VISIBLE );
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

        /* Caption clicks are handled by handle_nc_lbutton_down() */
        result = HIWORD(lparam) == WM_LBUTTONDOWN && LOWORD(lparam) == HTCAPTION ?
            MA_NOACTIVATE : MA_ACTIVATE;
        break;

    case WM_ACTIVATE:
        /* The default action in Windows is to set the keyboard focus to
         * the window, if it's being activated and not minimized */
        if (LOWORD(wparam) != WA_INACTIVE && !is_iconic( hwnd )) NtUserSetFocus( hwnd );
        break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)
            result = send_message( get_parent( hwnd ), msg, wparam, lparam );
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
                get_client_rect( hwnd, &rect, get_thread_dpi() );
                NtGdiTransformPoints( hdc, (POINT *)&rect, (POINT *)&rect, 1, NtGdiDPtoLP );
            }
            else NtGdiGetAppClipBox( hdc, &rect );
            fill_rect( hdc, &rect, hbr );
            return 1;
        }

    case WM_GETDLGCODE:
        break;

    case WM_CANCELMODE:
        menu_sys_key = 0;
        end_menu( hwnd );
        if (get_capture() == hwnd) NtUserReleaseCapture();
        break;

    case WM_SETTEXT:
        result = set_window_text( hwnd, (void *)lparam, ansi );
        if (result)
            NtUserNotifyWinEvent( EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_WINDOW, CHILDID_SELF );
        if (result && (get_window_long( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION)
            handle_nc_paint( hwnd , (HRGN)1 );  /* repaint caption */
        break;

    case WM_GETTEXTLENGTH:
        {
            WND *win = get_win_ptr( hwnd );
            if (win && win->text)
            {
                if (ansi)
                    result = win32u_wctomb_size( &ansi_cp, win->text, wcslen( win->text ));
                else
                    result = wcslen( win->text );
            }
            release_win_ptr( win );
        }
        break;

    case WM_GETTEXT:
        if (wparam)
        {
            WND *win;

            if (!(win = get_win_ptr( hwnd ))) break;

            __TRY
            {
                if (ansi)
                {
                    char *dest = (char *)lparam;
                    if (win->text)
                        result = win32u_wctomb( &ansi_cp, dest, wparam - 1,
                                                win->text, wcslen( win->text ));
                    dest[result] = 0;
                }
                else
                {
                    WCHAR *dest = (WCHAR *)lparam;
                    if (win->text) result = min( wcslen( win->text ), wparam - 1 );
                    if (result) memcpy( dest, win->text, result * sizeof(WCHAR) );
                    dest[result] = 0;
                }
            }
            __EXCEPT
            {
            }
            __ENDTRY

            release_win_ptr( win );
        }
        break;

    case WM_SETICON:
        result = (LRESULT)set_window_icon( hwnd, wparam, (HICON)lparam );
        if ((get_window_long( hwnd, GWL_STYLE ) & WS_CAPTION) == WS_CAPTION)
            handle_nc_paint( hwnd , (HRGN)1 );  /* repaint caption */
        break;

    case WM_GETICON:
        result = (LRESULT)get_window_icon( hwnd, wparam );
        break;

    case WM_SETCURSOR:
        if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)
        {
            /* with the exception of the border around a resizable window,
             * give the parent first chance to set the cursor */
            if ((LOWORD( lparam ) < HTSIZEFIRST) || (LOWORD( lparam ) > HTSIZELAST))
            {
                HWND parent = get_parent( hwnd );
                if (parent != get_desktop_window() &&
                    send_message( parent, WM_SETCURSOR, wparam, lparam )) return TRUE;
            }
        }
        handle_set_cursor( hwnd, wparam, lparam );
        break;

    case WM_SHOWWINDOW:
        {
            DWORD style = get_window_long( hwnd, GWL_STYLE );
            WND *win;
            if (!lparam) break; /* sent from ShowWindow */
            if ((style & WS_VISIBLE) && wparam) break;
            if (!(style & WS_VISIBLE) && !wparam) break;
            if (!get_window_relative( hwnd, GW_OWNER )) break;
            if (!(win = get_win_ptr( hwnd ))) break;
            if (win == WND_OTHER_PROCESS) break;
            if (wparam)
            {
                if (!(win->flags & WIN_NEEDS_SHOW_OWNEDPOPUP))
                {
                    release_win_ptr( win );
                    break;
                }
                win->flags &= ~WIN_NEEDS_SHOW_OWNEDPOPUP;
            }
            else win->flags |= WIN_NEEDS_SHOW_OWNEDPOPUP;
            release_win_ptr( win );
            NtUserShowWindow( hwnd, wparam ? SW_SHOWNOACTIVATE : SW_HIDE );
            break;
        }

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORSCROLLBAR:
        return (LRESULT)handle_control_color( (HDC)wparam, msg - WM_CTLCOLORMSGBOX );

    case WM_CTLCOLOR:
        return (LRESULT)handle_control_color( (HDC)wparam, HIWORD( lparam ));

    case WM_SYSCOMMAND:
        result = handle_sys_command( hwnd, wparam, lparam );
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        f10_key = menu_sys_key = 0;
        break;

    case WM_KEYDOWN:
        if (wparam == VK_F10) f10_key = VK_F10;
        break;

    case WM_SYSKEYDOWN:
        if (HIWORD( lparam ) & KEYDATA_ALT)
        {
            if ((wparam == VK_MENU || wparam == VK_LMENU || wparam == VK_RMENU) && !menu_sys_key)
                menu_sys_key = 1;
            else
                menu_sys_key = 0;

            f10_key = 0;

            if (wparam == VK_F4)  /* try to close the window */
            {
                HWND top = NtUserGetAncestor( hwnd, GA_ROOT );
                if (!(get_class_long( top, GCL_STYLE, FALSE ) & CS_NOCLOSE))
                    NtUserPostMessage( top, WM_SYSCOMMAND, SC_CLOSE, 0 );
            }
        }
        else if (wparam == VK_F10)
        {
            if (NtUserGetKeyState(VK_SHIFT) & 0x8000)
                send_message( hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, -1 );
            f10_key = 1;
        }
        else if (wparam == VK_ESCAPE && (NtUserGetKeyState( VK_SHIFT ) & 0x8000))
            send_message( hwnd, WM_SYSCOMMAND, SC_KEYMENU, ' ' );
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        /* Press and release F10 or ALT */
        if (((wparam == VK_MENU || wparam == VK_LMENU || wparam == VK_RMENU) && menu_sys_key) ||
            (wparam == VK_F10 && f10_key))
            send_message( NtUserGetAncestor( hwnd, GA_ROOT ), WM_SYSCOMMAND, SC_KEYMENU, 0 );
        menu_sys_key = f10_key = 0;
        break;

    case WM_SYSCHAR:
        menu_sys_key = 0;
        if (wparam == '\r' && is_iconic( hwnd ))
        {
            NtUserPostMessage( hwnd, WM_SYSCOMMAND, SC_RESTORE, 0 );
            break;
        }
        if ((HIWORD( lparam ) & KEYDATA_ALT) && wparam)
        {
            WCHAR wch;
            if (ansi)
            {
                char ch = wparam;
                win32u_mbtowc( &ansi_cp, &wch, 1, &ch, 1 );
            }
            else wch = wparam;
            if (wch == '\t' || wch == '\x1b') break;
            if (wch == ' ' && (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD))
                send_message( get_parent( hwnd ), msg, wch, lparam );
            else
                send_message( hwnd, WM_SYSCOMMAND, SC_KEYMENU, wch );
        }
        else if (wparam != '\x1b')  /* Ctrl-Esc */
            NtUserMessageBeep(0);
        break;

    case WM_KEYF1:
        {
            HELPINFO hi;

            hi.cbSize = sizeof(HELPINFO);
            NtUserGetCursorPos( &hi.MousePos );
            if (is_menu_active())
            {
                MENUINFO info = { .cbSize = sizeof(info), .fMask = MIM_HELPID };
                hi.iContextType = HELPINFO_MENUITEM;
                hi.hItemHandle = is_menu_active();
                hi.iCtrlId = NtUserMenuItemFromPoint( hwnd, hi.hItemHandle,
                                                      hi.MousePos.x, hi.MousePos.y );
                get_menu_info( hi.hItemHandle, &info );
                hi.dwContextId = info.dwContextHelpID;
            }
            else
            {
                hi.iContextType = HELPINFO_WINDOW;
                hi.hItemHandle = hwnd;
                hi.iCtrlId = get_window_long_ptr( hwnd, GWLP_ID, FALSE );
                hi.dwContextId = NtUserGetWindowContextHelpId( hwnd );
            }
            send_message( hwnd, WM_HELP, 0, (LPARAM)&hi );
            break;
        }

    case WM_PRINT:
        if ((lparam & PRF_CHECKVISIBLE) && !is_window_visible ( hwnd )) break;
        result = 1;

        if (lparam & (PRF_OWNED | PRF_NONCLIENT))
            WARN( "WM_PRINT message with unsupported lparam %lx\n", lparam );

        if (lparam & PRF_ERASEBKGND) send_message( hwnd, WM_ERASEBKGND, wparam, 0 );
        if (lparam & PRF_CLIENT)
        {
            send_message( hwnd, WM_PRINTCLIENT, wparam, lparam );

            if (lparam & PRF_CHILDREN)
            {
                HWND *list, *child;
                POINT org;
                RECT rect;
                UINT dpi;

                if ((list = list_window_children( hwnd )))
                {
                    for (child = list; *child; child++)
                    {
                        if (!(get_window_long( *child, GWL_STYLE ) & WS_VISIBLE))
                            continue;

                        dpi = NtUserGetDpiForWindow( *child );
                        NtUserGetClientRect( *child, &rect, dpi );
                        NtUserMapWindowPoints( *child, hwnd, (POINT *)&rect, 2, dpi );
                        offset_viewport_org( (HDC)wparam, rect.left, rect.top, &org );
                        send_message( *child, WM_PRINT, wparam, PRF_NONCLIENT | PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN );
                        set_viewport_org( (HDC)wparam, org.x, org.y, NULL );
                    }
                    free( list );
                }
            }
        }
        break;

    case WM_APPCOMMAND:
        {
            HWND parent = get_parent( hwnd );
            if (!parent)
                call_hooks( WH_SHELL, HSHELL_APPCOMMAND, wparam, lparam, 0 );
            else
                send_message( parent, msg, wparam, lparam );
            break;
        }

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        result = -1;
        break;

    case WM_DROPOBJECT:
        result = DRAG_FILE;
        break;

    case WM_QUERYDROPOBJECT:
        result = (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES) != 0;
        break;

    case WM_QUERYDRAGICON:
        {
            UINT len;
            HICON icon = (HICON)get_class_long_ptr( hwnd, GCLP_HICON, FALSE );
            HINSTANCE instance = (HINSTANCE)get_window_long_ptr( hwnd, GWLP_HINSTANCE, FALSE );

            if (icon)
            {
                result = (LRESULT)icon;
                break;
            }

            for (len = 1; len < 64; len++)
            {
                if((icon = LoadImageW( instance, MAKEINTRESOURCEW( len ), IMAGE_ICON, 0, 0,
                                       LR_SHARED | LR_DEFAULTSIZE )))
                {
                    result = (LRESULT)icon;
                    break;
                }
            }
            if (!result) result = (LRESULT)LoadImageW( 0, (WCHAR *)IDI_APPLICATION, IMAGE_ICON,
                                                       0, 0, LR_SHARED | LR_DEFAULTSIZE );
            break;
        }

    case WM_ISACTIVEICON:
        result = (win_get_flags( hwnd ) & WIN_NCACTIVATED) != 0;
        break;

    case WM_NOTIFYFORMAT:
        result = is_window_unicode(hwnd) ? NFR_UNICODE : NFR_ANSI;
        break;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
        result = 1;
        break;

    case WM_HELP:
        send_message( get_parent( hwnd ), msg, wparam, lparam );
        break;

    case WM_STYLECHANGED:
        if (wparam == GWL_STYLE && (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED))
        {
            STYLESTRUCT *style = (STYLESTRUCT *)lparam;
            if ((style->styleOld ^ style->styleNew) & (WS_CAPTION|WS_THICKFRAME|WS_VSCROLL|WS_HSCROLL))
                NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER |
                                    SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE );
        }
        break;

    case WM_INPUTLANGCHANGEREQUEST:
        NtUserActivateKeyboardLayout( (HKL)lparam, 0 );
        break;

    case WM_INPUTLANGCHANGE:
        {
            struct user_thread_info *info = get_user_thread_info();
            HWND *win_array = list_window_children( hwnd );
            int count = 0;
            info->kbd_layout = (HKL)lparam;

            if (!win_array)
                break;
            while (win_array[count])
                send_message( win_array[count++], WM_INPUTLANGCHANGE, wparam, lparam );
            free( win_array );
            break;
        }

    case WM_IME_SETCONTEXT:
    case WM_IME_COMPOSITION:
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_NOTIFY:
        {
            HWND ime_hwnd = get_default_ime_window( hwnd );
            if (ime_hwnd && ime_hwnd != NtUserGetParent( hwnd ))
                result = NtUserMessageCall( ime_hwnd, msg, wparam, lparam,
                                            0, NtUserSendMessage, ansi );
        }
        break;
    }

    return result;
}

static void update_children_window_state( HWND hwnd )
{
    HWND *children;
    int i;

    if (!(children = list_window_children( hwnd ))) return;
    for (i = 0; children[i]; i++) update_window_state( children[i] );
    free( children );
}

LRESULT desktop_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
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

            snprintf( buffer, sizeof(buffer), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                      guid->Data1, guid->Data2, guid->Data3,
                      guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                      guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
            NtAddAtom( bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR), &atom );
            NtUserSetProp( hwnd, wine_display_device_guidW, ULongToHandle( atom ) );
        }
        return TRUE;
    }
    case WM_NCCALCSIZE:
        return 0;
    case WM_DISPLAYCHANGE:
    {
        static RECT virtual_rect = {INT_MIN,INT_MIN,INT_MAX,INT_MAX};

        RECT new_rect = get_virtual_screen_rect( 0, MDT_DEFAULT ), old_rect = virtual_rect;
        UINT context, flags = 0;

        if (EqualRect( &new_rect, &old_rect )) return TRUE;
        virtual_rect = new_rect;

        TRACE( "desktop %p change from %s to %s\n", hwnd, wine_dbgstr_rect( &old_rect ), wine_dbgstr_rect( &new_rect ) );

        if (new_rect.right - new_rect.left == old_rect.right - old_rect.left &&
            new_rect.bottom - new_rect.top == old_rect.bottom - old_rect.top)
            flags |= SWP_NOSIZE;
        if (new_rect.left == old_rect.left && new_rect.top == old_rect.top)
            flags |= SWP_NOMOVE;

        context = NtUserSetThreadDpiAwarenessContext( NTUSER_DPI_PER_MONITOR_AWARE );
        NtUserSetWindowPos( hwnd, 0, new_rect.left, new_rect.top,
                            new_rect.right - new_rect.left, new_rect.bottom - new_rect.top,
                            flags | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERERASE );
        NtUserSetThreadDpiAwarenessContext( context );

        update_children_window_state( hwnd );

        return 0;
    }
    default:
        if (msg >= WM_USER && hwnd == get_desktop_window())
            return user_driver->pDesktopWindowProc( hwnd, msg, wparam, lparam );
    }

    return default_window_proc( hwnd, msg, wparam, lparam, ansi );
}

/***********************************************************************
 *           NtUserGetTitleBarInfo   (win32u.@)
 */
BOOL WINAPI NtUserGetTitleBarInfo( HWND hwnd, TITLEBARINFO *info )
{
    DWORD style, ex_style;

    TRACE( "(%p %p)\n", hwnd, info );

    if (!info)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }

    if (info->cbSize != sizeof(TITLEBARINFO))
    {
        TRACE( "Invalid TITLEBARINFO size: %d\n", info->cbSize );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    style = get_window_long( hwnd, GWL_STYLE );
    ex_style = get_window_long( hwnd, GWL_EXSTYLE );
    get_inside_rect( hwnd, COORDS_SCREEN, &info->rcTitleBar, style, ex_style );

    info->rcTitleBar.bottom = info->rcTitleBar.top;
    if (ex_style & WS_EX_TOOLWINDOW)
        info->rcTitleBar.bottom += get_system_metrics( SM_CYSMCAPTION );
    else
    {
        info->rcTitleBar.bottom += get_system_metrics( SM_CYCAPTION );
        info->rcTitleBar.left += get_system_metrics( SM_CXSIZE );
    }

    memset( info->rgstate, 0, sizeof(info->rgstate) );
    info->rgstate[0] = STATE_SYSTEM_FOCUSABLE;

    if (style & WS_CAPTION)
    {
        info->rgstate[1] = STATE_SYSTEM_INVISIBLE;
        if (style & WS_SYSMENU)
        {
            if (!(style & (WS_MINIMIZEBOX|WS_MAXIMIZEBOX)))
            {
                info->rgstate[2] = STATE_SYSTEM_INVISIBLE;
                info->rgstate[3] = STATE_SYSTEM_INVISIBLE;
            }
            else
            {
                if (!(style & WS_MINIMIZEBOX))
                    info->rgstate[2] = STATE_SYSTEM_UNAVAILABLE;
                if (!(style & WS_MAXIMIZEBOX))
                    info->rgstate[3] = STATE_SYSTEM_UNAVAILABLE;
            }
            if (!(ex_style & WS_EX_CONTEXTHELP))
                info->rgstate[4] = STATE_SYSTEM_INVISIBLE;
            if (get_class_long( hwnd, GCL_STYLE, FALSE ) & CS_NOCLOSE )
                info->rgstate[5] = STATE_SYSTEM_UNAVAILABLE;
        }
        else
        {
            info->rgstate[2] = STATE_SYSTEM_INVISIBLE;
            info->rgstate[3] = STATE_SYSTEM_INVISIBLE;
            info->rgstate[4] = STATE_SYSTEM_INVISIBLE;
            info->rgstate[5] = STATE_SYSTEM_INVISIBLE;
        }
    }
    else
        info->rgstate[0] |= STATE_SYSTEM_INVISIBLE;
    return TRUE;
}

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

#if 0
#pragma makedep unix
#endif

#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);


#define SCROLLBAR_MAGIC 0x5c6011ba

/* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4

/* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 17

/* Overlap between arrows and thumb */
#define SCROLL_ARROW_THUMB_OVERLAP 0

/* Delay (in ms) before first repetition when holding the button down */
#define SCROLL_FIRST_DELAY   200

/* Delay (in ms) between scroll repetitions */
#define SCROLL_REPEAT_DELAY  50

/* Scroll timer id */
#define SCROLL_TIMER   0

/* What to do after set_scroll_info() */
#define SA_SSI_HIDE            0x0001
#define SA_SSI_SHOW            0x0002
#define SA_SSI_REFRESH         0x0004
#define SA_SSI_REPAINT_ARROWS  0x0008

/* Scroll Bar tracking information */
static struct SCROLL_TRACKING_INFO g_tracking_info;

/* Is the moving thumb being displayed? */
static BOOL scroll_moving_thumb = FALSE;

/* data for window that has (one or two) scroll bars */
struct win_scroll_bar_info
{
    struct scroll_info horz;
    struct scroll_info vert;
};

#define SCROLLBAR_MAGIC 0x5c6011ba


static struct scroll_info *get_scroll_info_ptr( HWND hwnd, int bar, BOOL alloc )
{
    struct scroll_info *info = NULL;
    WND *win = get_win_ptr( hwnd );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return NULL;

    switch (bar)
    {
    case SB_HORZ:
        if (win->pScroll) info = &win->pScroll->horz;
        break;
    case SB_VERT:
        if (win->pScroll) info = &win->pScroll->vert;
        break;
    case SB_CTL:
        if (win->cbWndExtra >= sizeof(struct scroll_bar_win_data))
        {
            struct scroll_bar_win_data *data = (struct scroll_bar_win_data *)win->wExtra;
            if (data->magic == SCROLLBAR_MAGIC) info = &data->info;
        }
        if (!info) WARN( "window is not a scrollbar control\n" );
        break;
    case SB_BOTH:
        WARN( "with SB_BOTH\n" );
        break;
    }

    if (!info && alloc)
    {
        struct win_scroll_bar_info *win_info;

        if (bar != SB_HORZ && bar != SB_VERT)
            WARN("Cannot initialize bar=%d\n",bar);
        else if ((win_info = malloc( sizeof(struct win_scroll_bar_info) )))
        {
            /* Set default values */
            win_info->horz.minVal = 0;
            win_info->horz.curVal = 0;
            win_info->horz.page = 0;
            /* From MSDN and our own tests:
             * max for a standard scroll bar is 100 by default. */
            win_info->horz.maxVal = 100;
            win_info->horz.flags  = ESB_ENABLE_BOTH;
            win_info->vert = win_info->horz;
            win->pScroll = win_info;
            info = bar == SB_HORZ ? &win_info->horz : &win_info->vert;
        }
    }

    if (info) user_lock();
    release_win_ptr( win );
    return info;
}

static void release_scroll_info_ptr( struct scroll_info *info )
{
    user_unlock();
}

static BOOL show_scroll_bar( HWND hwnd, int bar, BOOL show_horz, BOOL show_vert )
{
    ULONG old_style, set_bits = 0, clear_bits = 0;

    TRACE( "hwnd=%p bar=%d horz=%d, vert=%d\n", hwnd, bar, show_horz, show_vert );

    switch (bar)
    {
    case SB_CTL:
        NtUserShowWindow( hwnd, show_horz ? SW_SHOW : SW_HIDE );
        return TRUE;

    case SB_BOTH:
    case SB_HORZ:
        if (show_horz) set_bits |= WS_HSCROLL;
        else clear_bits |= WS_HSCROLL;
        if (bar == SB_HORZ) break;
        /* fall through */
    case SB_VERT:
        if (show_vert) set_bits |= WS_VSCROLL;
        else clear_bits |= WS_VSCROLL;
        break;

    default:
        return FALSE;  /* Nothing to do! */
    }

    old_style = set_window_style( hwnd, set_bits, clear_bits );
    if ((old_style & clear_bits) != 0 || (old_style & set_bits) != set_bits)
    {
        /* frame has been changed, let the window redraw itself */
        NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0,
                            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
        return TRUE;
    }
    return FALSE; /* no frame changes */
}

/***********************************************************************
 *           get_scroll_bar_rect
 *
 * Compute the scroll bar rectangle, in drawing coordinates (i.e. client
 * coords for SB_CTL, window coords for SB_VERT and SB_HORZ).
 * 'arrow_size' returns the width or height of an arrow (depending on
 * the orientation of the scrollbar), 'thumb_size' returns the size of
 * the thumb, and 'thumb_pos' returns the position of the thumb
 * relative to the left or to the top.
 * Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL get_scroll_bar_rect( HWND hwnd, int bar, RECT *rect, int *arrow_size,
                                 int *thumb_size, int *thumb_pos )
{
    int pixels, min_thumb_size;
    BOOL vertical;
    WND *win = get_win_ptr( hwnd );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;

    switch(bar)
    {
    case SB_HORZ:
        get_window_rects( hwnd, COORDS_WINDOW, NULL, rect, get_thread_dpi() );
        rect->top = rect->bottom;
        rect->bottom += get_system_metrics( SM_CYHSCROLL );
        if (win->dwStyle & WS_VSCROLL) rect->right++;
        vertical = FALSE;
        break;

    case SB_VERT:
        get_window_rects( hwnd, COORDS_WINDOW, NULL, rect, get_thread_dpi() );
        if (win->dwExStyle & WS_EX_LEFTSCROLLBAR)
        {
            rect->right = rect->left;
            rect->left -= get_system_metrics( SM_CXVSCROLL );
        }
        else
        {
            rect->left = rect->right;
            rect->right += get_system_metrics( SM_CXVSCROLL );
        }
        if (win->dwStyle & WS_HSCROLL) rect->bottom++;
        vertical = TRUE;
        break;

    case SB_CTL:
        get_client_rect( hwnd, rect );
        vertical = (win->dwStyle & SBS_VERT) != 0;
        break;

    default:
        release_win_ptr( win );
        return FALSE;
    }

    if (vertical) pixels = rect->bottom - rect->top;
    else pixels = rect->right - rect->left;

    if (pixels <= 2 * get_system_metrics( SM_CXVSCROLL ) + SCROLL_MIN_RECT)
    {
        if (pixels > SCROLL_MIN_RECT)
            *arrow_size = (pixels - SCROLL_MIN_RECT) / 2;
        else
            *arrow_size = 0;
        *thumb_pos = *thumb_size = 0;
    }
    else
    {
        struct scroll_info *info = get_scroll_info_ptr( hwnd, bar, TRUE );
        if (!info)
        {
            WARN( "called for missing scroll bar\n" );
            release_win_ptr( win );
            return FALSE;
        }
        *arrow_size = get_system_metrics( SM_CXVSCROLL );
        pixels -= 2 * (get_system_metrics( SM_CXVSCROLL ) - SCROLL_ARROW_THUMB_OVERLAP);

        if (info->page)
        {
            *thumb_size = muldiv( pixels,info->page, info->maxVal - info->minVal + 1 );
            min_thumb_size = muldiv( SCROLL_MIN_THUMB, get_dpi_for_window( hwnd ), 96 );
            if (*thumb_size < min_thumb_size) *thumb_size = min_thumb_size;
        }
        else *thumb_size = get_system_metrics( SM_CXVSCROLL );

        if ((pixels -= *thumb_size ) < 0 || (info->flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
        {
            /* Rectangle too small or scrollbar disabled -> no thumb */
            *thumb_pos = *thumb_size = 0;
        }
        else
        {
            int max = info->maxVal - max( info->page-1, 0 );
            if (info->minVal >= max)
                *thumb_pos = *arrow_size - SCROLL_ARROW_THUMB_OVERLAP;
            else
                *thumb_pos = *arrow_size - SCROLL_ARROW_THUMB_OVERLAP
                    + muldiv( pixels, info->curVal - info->minVal, max - info->minVal );
        }
        release_scroll_info_ptr( info );
    }
    release_win_ptr( win );
    return vertical;
}

/***********************************************************************
 *           draw_scroll_bar
 *
 * Redraw the whole scrollbar.
 */
static void draw_scroll_bar( HWND hwnd, HDC hdc, int bar, enum SCROLL_HITTEST hit_test,
                      const struct SCROLL_TRACKING_INFO *tracking_info, BOOL draw_arrows,
                      BOOL draw_interior )
{
    struct draw_scroll_bar_params params;
    struct scroll_info *info;
    RECT clip_box, intersect;
    DWORD style;
    void *ret_ptr;
    ULONG ret_len;

    if (!(hwnd = get_full_window_handle( hwnd )))
        return;

    style = get_window_long( hwnd, GWL_STYLE );
    if ((bar == SB_VERT && !(style & WS_VSCROLL)) || (bar == SB_HORZ && !(style & WS_HSCROLL)))
        return;

    if (!is_window_drawable( hwnd, FALSE ))
        return;

    if (!(info = get_scroll_info_ptr( hwnd, bar, TRUE ))) return;
    params.enable_flags = info->flags;
    release_scroll_info_ptr( info );

    if (bar == SB_CTL && get_window_long( hwnd, GWL_STYLE ) & (SBS_SIZEGRIP | SBS_SIZEBOX))
    {
        get_client_rect( hwnd, &params.rect );
        params.arrow_size = 0;
        params.thumb_pos = 0;
        params.thumb_size = 0;
        params.vertical = FALSE;
    }
    else
    {
        int pos, max_size;

        params.vertical = get_scroll_bar_rect( hwnd, bar, &params.rect, &params.arrow_size,
                                               &params.thumb_size, &params.thumb_pos );

        if (scroll_moving_thumb && tracking_info->win == hwnd && tracking_info->bar == bar)
        {
            max_size = params.vertical ?
                params.rect.bottom - params.rect.top : params.rect.right - params.rect.left;
            max_size -= params.arrow_size - SCROLL_ARROW_THUMB_OVERLAP + params.thumb_size;

            pos = tracking_info->thumb_pos;
            if (pos < params.arrow_size - SCROLL_ARROW_THUMB_OVERLAP)
                pos = params.arrow_size - SCROLL_ARROW_THUMB_OVERLAP;
            else if (pos > max_size)
                pos = max_size;

            params.thumb_pos = pos;
        }
    }

    /* do not draw if the scrollbar rectangle is empty */
    if (IsRectEmpty( &params.rect ))
        return;

    TRACE( "hwnd %p, hdc %p, bar %d, hit_test %d, tracking_info(win %p, bar %d, thumb_pos %d, "
           "track_pos %d, vertical %d, hit_test %d), draw_arrows %d, draw_interior %d, rect %s, "
           "arrow_size %d, thumb_pos %d, thumb_val %d, vertical %d, captured window %p\n", hwnd, hdc,
           bar, hit_test, tracking_info->win, tracking_info->bar, tracking_info->thumb_pos,
           tracking_info->thumb_val, tracking_info->vertical, tracking_info->hit_test, draw_arrows,
           draw_interior, wine_dbgstr_rect(&params.rect), params.arrow_size, params.thumb_pos,
           params.thumb_size, params.vertical, get_capture() );

    params.hwnd = hwnd;
    params.hdc = hdc;
    params.bar = bar;
    params.hit_test = hit_test;
    params.tracking_info = *tracking_info;
    params.arrows = draw_arrows;
    params.interior = draw_interior;
    KeUserModeCallback( NtUserDrawScrollBar, &params, sizeof(params), &ret_ptr, &ret_len );

    if (bar == SB_HORZ || bar == SB_VERT)
    {
        NtGdiGetAppClipBox( hdc, &clip_box );
        if (intersect_rect( &intersect, &params.rect, &clip_box ))
            set_standard_scroll_painted( hwnd, bar, TRUE );
    }
}

void draw_nc_scrollbar( HWND hwnd, HDC hdc, BOOL draw_horizontal, BOOL draw_vertical )
{
    if (draw_horizontal)
        draw_scroll_bar( hwnd, hdc, SB_HORZ, g_tracking_info.hit_test, &g_tracking_info, TRUE, TRUE );
    if (draw_vertical)
        draw_scroll_bar( hwnd, hdc, SB_VERT, g_tracking_info.hit_test, &g_tracking_info, TRUE, TRUE );
}

static void refresh_scroll_bar( HWND hwnd, int nBar, BOOL arrows, BOOL interior )
{
    HDC hdc = NtUserGetDCEx( hwnd, 0, DCX_CACHE | (nBar == SB_CTL ? 0 : DCX_WINDOW) );
    if (!hdc) return;

    draw_scroll_bar( hwnd, hdc, nBar, g_tracking_info.hit_test, &g_tracking_info, arrows, interior );
    NtUserReleaseDC( hwnd, hdc );
}

static BOOL point_in_scroll_rect( RECT *r, POINT pt, BOOL vertical )
{
    RECT rect = *r;
    int width;

    /* Pad hit rect to allow mouse to be dragged outside of scrollbar and
     * still be considered in the scrollbar. */
    if (vertical)
    {
        width = r->right - r->left;
        InflateRect( &rect, width * 8, width * 2 );
    }
    else
    {
        width = r->bottom - r->top;
        InflateRect( &rect, width * 2, width * 8 );
    }
    return PtInRect( &rect, pt );
}

/***********************************************************************
 *           scroll_hit_test
 *
 * Scroll-bar hit testing (don't confuse this with WM_NCHITTEST!).
 */
static enum SCROLL_HITTEST scroll_hit_test( HWND hwnd, int bar, POINT pt, BOOL dragging )
{
    int arrow_size, thumb_size, thumb_pos;
    BOOL vertical;
    RECT rect;

    vertical = get_scroll_bar_rect( hwnd, bar, &rect, &arrow_size, &thumb_size, &thumb_pos );

    if ((dragging && !point_in_scroll_rect( &rect, pt, vertical )) || !PtInRect( &rect, pt ))
        return SCROLL_NOWHERE;

    if (vertical)
    {
        if (pt.y < rect.top + arrow_size) return SCROLL_TOP_ARROW;
        if (pt.y >= rect.bottom - arrow_size) return SCROLL_BOTTOM_ARROW;
        if (!thumb_pos) return SCROLL_TOP_RECT;
        pt.y -= rect.top;
        if (pt.y < thumb_pos) return SCROLL_TOP_RECT;
        if (pt.y >= thumb_pos + thumb_size) return SCROLL_BOTTOM_RECT;
    }
    else  /* horizontal */
    {
        if (pt.x < rect.left + arrow_size) return SCROLL_TOP_ARROW;
        if (pt.x >= rect.right - arrow_size) return SCROLL_BOTTOM_ARROW;
        if (!thumb_pos) return SCROLL_TOP_RECT;
        pt.x -= rect.left;
        if (pt.x < thumb_pos) return SCROLL_TOP_RECT;
        if (pt.x >= thumb_pos + thumb_size) return SCROLL_BOTTOM_RECT;
    }
    return SCROLL_THUMB;
}

static BOOL is_standard_scroll_painted( HWND hwnd, int bar )
{
    struct scroll_info *info;
    BOOL ret;

    if (bar != SB_HORZ && bar != SB_VERT) return FALSE;
    if (!(info = get_scroll_info_ptr( hwnd, bar, FALSE ))) return FALSE;
    ret = info->painted;
    release_scroll_info_ptr( info );
    return ret;
}

/***********************************************************************
 *           get_thumb_val
 *
 * Compute the current scroll position based on the thumb position in pixels
 * from the top of the scroll-bar.
 */
static UINT get_thumb_val( HWND hwnd, int bar, RECT *rect, BOOL vertical, int pos )
{
    int thumb_size, min_thumb_size, pixels, range;
    struct scroll_info *info;
    UINT ret;

    pixels = vertical ? rect->bottom-rect->top : rect->right-rect->left;
    pixels -= 2 * (get_system_metrics( SM_CXVSCROLL ) - SCROLL_ARROW_THUMB_OVERLAP);
    if (!(info = get_scroll_info_ptr( hwnd, bar, FALSE ))) return 0;
    ret = info->minVal;
    if (pixels > 0)
    {
        if (info->page)
        {
            thumb_size = muldiv( pixels, info->page, info->maxVal - info->minVal + 1 );
            min_thumb_size = muldiv( SCROLL_MIN_THUMB, get_dpi_for_window( hwnd ), 96 );
            if (thumb_size < min_thumb_size) thumb_size = min_thumb_size;
        }
        else thumb_size = get_system_metrics( SM_CXVSCROLL );

        if ((pixels -= thumb_size) > 0)
        {
            pos = max( 0, pos - (get_system_metrics( SM_CXVSCROLL ) - SCROLL_ARROW_THUMB_OVERLAP ));
            if (pos > pixels) pos = pixels;

            if (!info->page)
                range = info->maxVal - info->minVal;
            else
                range = info->maxVal - info->minVal - info->page + 1;

            ret = info->minVal + muldiv( pos, range, pixels );
        }
    }

    release_scroll_info_ptr( info );
    return ret;
}

static POINT clip_scroll_pos( RECT *rect, POINT pt )
{
    if (pt.x < rect->left)
        pt.x = rect->left;
    else if (pt.x > rect->right)
        pt.x = rect->right;

    if (pt.y < rect->top)
        pt.y = rect->top;
    else if (pt.y > rect->bottom)
        pt.y = rect->bottom;

    return pt;
}

void update_scroll_timer(HWND hwnd, HWND owner_hwnd, HWND ctl_hwnd, enum SCROLL_HITTEST hittest, UINT msg, UINT msg_send, BOOL vertical )
{
    if (hittest == g_tracking_info.hit_test)
    {
        if (msg == WM_LBUTTONDOWN || msg == WM_SYSTIMER)
        {
            send_message( owner_hwnd, vertical ? WM_VSCROLL : WM_HSCROLL, msg_send, (LPARAM)ctl_hwnd );
        }

        NtUserSetSystemTimer( hwnd, SCROLL_TIMER,
                msg == WM_LBUTTONDOWN ? SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY );
    }
    else NtUserKillSystemTimer( hwnd, SCROLL_TIMER );
}

/***********************************************************************
 *           handle_scroll_event
 *
 * Handle a mouse or timer event for the scrollbar.
 * 'pt' is the location of the mouse event in client (for SB_CTL) or
 * windows coordinates.
 */
void handle_scroll_event( HWND hwnd, int bar, UINT msg, POINT pt )
{
    /* Previous mouse position for timer events. */
    static POINT prev_pt;
    /* Thumb position when tracking started. */
    static UINT track_thumb_pos;
    /* Position in the scroll-bar of the last button-down event. */
    static int last_click_pos;
    /* Position in the scroll-bar of the last mouse event. */
    static int last_mouse_pos;

    int arrow_size, thumb_size, thumb_pos;
    enum SCROLL_HITTEST hittest;
    HWND owner_hwnd, ctl_hwnd;
    struct scroll_info *info;
    TRACKMOUSEEVENT tme;
    BOOL vertical;
    RECT rect;
    HDC hdc;

    if (!(info = get_scroll_info_ptr( hwnd, bar, FALSE ))) return;
    release_scroll_info_ptr( info );

    if (g_tracking_info.hit_test == SCROLL_NOWHERE &&
        (msg != WM_LBUTTONDOWN && msg != WM_MOUSEMOVE && msg != WM_MOUSELEAVE &&
         msg != WM_NCMOUSEMOVE && msg != WM_NCMOUSELEAVE))
        return;

    if (bar == SB_CTL && (get_window_long( hwnd, GWL_STYLE ) & (SBS_SIZEGRIP | SBS_SIZEBOX)))
    {
        switch (msg)
        {
        case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
            NtUserHideCaret( hwnd ); /* hide caret while holding down LBUTTON */
            NtUserSetCapture( hwnd );
            prev_pt = pt;
            g_tracking_info.hit_test = hittest = SCROLL_THUMB;
            break;
        case WM_MOUSEMOVE:
            get_client_rect( get_parent( get_parent( hwnd )), &rect );
            prev_pt = pt;
            break;
        case WM_LBUTTONUP:
            release_capture();
            g_tracking_info.hit_test = hittest = SCROLL_NOWHERE;
            if (hwnd == get_focus()) NtUserShowCaret( hwnd );
            break;
        case WM_SYSTIMER:
            pt = prev_pt;
            break;
        }
        return;
    }

    hdc = NtUserGetDCEx( hwnd, 0, DCX_CACHE | (bar == SB_CTL ? 0 : DCX_WINDOW));
    vertical = get_scroll_bar_rect( hwnd, bar, &rect, &arrow_size, &thumb_size, &thumb_pos );
    owner_hwnd = bar == SB_CTL ? get_parent( hwnd ) : hwnd;
    ctl_hwnd   = bar == SB_CTL ? hwnd : 0;

    switch (msg)
    {
    case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
        NtUserHideCaret( hwnd ); /* hide caret while holding down LBUTTON */
        g_tracking_info.vertical = vertical;
        g_tracking_info.hit_test = hittest = scroll_hit_test( hwnd, bar, pt, FALSE );
        last_click_pos  = vertical ? pt.y - rect.top : pt.x - rect.left;
        last_mouse_pos  = last_click_pos;
        track_thumb_pos = thumb_pos;
        prev_pt = pt;
        if (bar == SB_CTL && (get_window_long( hwnd, GWL_STYLE ) & WS_TABSTOP))
            NtUserSetFocus( hwnd );
        NtUserSetCapture( hwnd );
        break;

    case WM_MOUSEMOVE:
        hittest = scroll_hit_test( hwnd, bar, pt,
                                   vertical == g_tracking_info.vertical && get_capture() == hwnd );
        prev_pt = pt;

        if (bar != SB_CTL)
            break;

        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_QUERY;
        NtUserTrackMouseEvent( &tme );
        if (!(tme.dwFlags & TME_LEAVE) || tme.hwndTrack != hwnd)
        {
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            NtUserTrackMouseEvent( &tme );
        }
        break;

    case WM_NCMOUSEMOVE:
        hittest = scroll_hit_test( hwnd, bar, pt,
                                   vertical == g_tracking_info.vertical && get_capture() == hwnd );
        prev_pt = pt;

        if (bar == SB_CTL)
            break;

        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_QUERY;
        NtUserTrackMouseEvent( &tme );
        if (((tme.dwFlags & (TME_NONCLIENT | TME_LEAVE)) != (TME_NONCLIENT | TME_LEAVE)) ||
            tme.hwndTrack != hwnd)
        {
            tme.dwFlags = TME_NONCLIENT | TME_LEAVE;
            tme.hwndTrack = hwnd;
            NtUserTrackMouseEvent( &tme );
        }

        break;

    case WM_NCMOUSELEAVE:
        if (bar == SB_CTL)
            return;

        hittest = SCROLL_NOWHERE;
        break;

    case WM_MOUSELEAVE:
        if (bar != SB_CTL)
            return;

        hittest = SCROLL_NOWHERE;
        break;

    case WM_LBUTTONUP:
        hittest = SCROLL_NOWHERE;
        release_capture();
        /* if scrollbar has focus, show back caret */
        if (hwnd == get_focus()) NtUserShowCaret( hwnd );
        break;

    case WM_SYSTIMER:
        pt = prev_pt;
        hittest = scroll_hit_test( hwnd, bar, pt, FALSE );
        break;

    default:
        return;  /* Should never happen */
    }

    TRACE( "Event: hwnd=%p bar=%d msg=%s pt=%d,%d hit=%d\n",
           hwnd, bar, debugstr_msg_name( msg, hwnd ), (int)pt.x, (int)pt.y, hittest );

    switch (g_tracking_info.hit_test)
    {
    case SCROLL_NOWHERE:  /* No tracking in progress */
        /* For standard scroll bars, hovered state gets painted only when the scroll bar was
         * previously painted by DefWinProc(). If an application handles WM_NCPAINT by itself, then
         * the scrollbar shouldn't be repainted here to avoid overwriting the application painted
         * content */
        if (msg == WM_MOUSEMOVE || msg == WM_MOUSELEAVE ||
            ((msg == WM_NCMOUSEMOVE || msg == WM_NCMOUSELEAVE) &&
             is_standard_scroll_painted( hwnd, bar )))
            draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, TRUE, TRUE );
        break;

    case SCROLL_TOP_ARROW:
        draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, TRUE, FALSE );
        update_scroll_timer( hwnd, owner_hwnd, ctl_hwnd, hittest, msg, SB_LINEUP, vertical );
        break;

    case SCROLL_TOP_RECT:
        draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, FALSE, TRUE );
        update_scroll_timer( hwnd, owner_hwnd, ctl_hwnd, hittest, msg, SB_PAGEUP, vertical );
        break;

    case SCROLL_THUMB:
        if (msg == WM_LBUTTONDOWN)
        {
            g_tracking_info.win = hwnd;
            g_tracking_info.bar = bar;
            g_tracking_info.thumb_pos = track_thumb_pos + last_mouse_pos - last_click_pos;
            g_tracking_info.thumb_val = get_thumb_val( hwnd, bar, &rect, vertical,
                                                       g_tracking_info.thumb_pos );
            if (!scroll_moving_thumb)
            {
                scroll_moving_thumb = TRUE;
                draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, FALSE, TRUE );
            }
        }
        else if (msg == WM_LBUTTONUP)
        {
            draw_scroll_bar( hwnd, hdc, bar, SCROLL_NOWHERE, &g_tracking_info, FALSE, TRUE );
        }
        else if (msg == WM_MOUSEMOVE)
        {
            int pos;

            if (!point_in_scroll_rect( &rect, pt, vertical )) pos = last_click_pos;
            else
            {
                pt = clip_scroll_pos( &rect, pt );
                pos = vertical ? pt.y - rect.top : pt.x - rect.left;
            }
            if (pos != last_mouse_pos || !scroll_moving_thumb)
            {
                last_mouse_pos = pos;
                g_tracking_info.thumb_pos = track_thumb_pos + pos - last_click_pos;
                g_tracking_info.thumb_val = get_thumb_val( hwnd, bar, &rect, vertical,
                                                           g_tracking_info.thumb_pos );
                send_message( owner_hwnd, vertical ? WM_VSCROLL : WM_HSCROLL,
                              MAKEWPARAM( SB_THUMBTRACK, g_tracking_info.thumb_val ),
                              (LPARAM)ctl_hwnd );
                scroll_moving_thumb = TRUE;
                draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, FALSE, TRUE );
            }
        }
        break;

    case SCROLL_BOTTOM_RECT:
        draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, FALSE, TRUE );
        update_scroll_timer( hwnd, owner_hwnd, ctl_hwnd, hittest, msg, SB_PAGEDOWN, vertical );
        break;

    case SCROLL_BOTTOM_ARROW:
        draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, TRUE, FALSE );
        update_scroll_timer( hwnd, owner_hwnd, ctl_hwnd, hittest, msg, SB_LINEDOWN, vertical );
        break;
    }

    if (msg == WM_LBUTTONDOWN)
    {

        if (hittest == SCROLL_THUMB)
        {
            UINT val = get_thumb_val( hwnd, bar, &rect, vertical,
                                      track_thumb_pos + last_mouse_pos - last_click_pos );
            send_message( owner_hwnd, vertical ? WM_VSCROLL : WM_HSCROLL,
                          MAKEWPARAM( SB_THUMBTRACK, val ), (LPARAM)ctl_hwnd );
        }
    }

    if (msg == WM_LBUTTONUP)
    {
        hittest = g_tracking_info.hit_test;
        g_tracking_info.hit_test = SCROLL_NOWHERE; /* Terminate tracking */

        if (hittest == SCROLL_THUMB)
        {
            UINT val = get_thumb_val( hwnd, bar, &rect, vertical,
                                      track_thumb_pos + last_mouse_pos - last_click_pos );
            send_message( owner_hwnd, vertical ? WM_VSCROLL : WM_HSCROLL,
                          MAKEWPARAM( SB_THUMBPOSITION, val ), (LPARAM)ctl_hwnd );
        }
        /* SB_ENDSCROLL doesn't report thumb position */
        send_message( owner_hwnd, vertical ? WM_VSCROLL : WM_HSCROLL,
                      SB_ENDSCROLL, (LPARAM)ctl_hwnd );

        /* Terminate tracking */
        g_tracking_info.win = 0;
        scroll_moving_thumb = FALSE;
        hittest = SCROLL_NOWHERE;
        draw_scroll_bar( hwnd, hdc, bar, hittest, &g_tracking_info, TRUE, TRUE );
    }

    NtUserReleaseDC( hwnd, hdc );
}

/***********************************************************************
 *           track_scroll_bar
 *
 * Track a mouse button press on a scroll-bar.
 * pt is in screen-coordinates for non-client scroll bars.
 */
void track_scroll_bar( HWND hwnd, int scrollbar, POINT pt )
{
    MSG msg;
    RECT rect;

    if (scrollbar != SB_CTL)
    {
        get_window_rects( hwnd, COORDS_CLIENT, &rect, NULL, get_thread_dpi() );
        screen_to_client( hwnd, &pt );
        pt.x -= rect.left;
        pt.y -= rect.top;
    }
    else
        rect.left = rect.top = 0;

    handle_scroll_event( hwnd, scrollbar, WM_LBUTTONDOWN, pt );

    do
    {
        if (!NtUserGetMessage( &msg, 0, 0, 0 )) break;
        if (NtUserCallMsgFilter( &msg, MSGF_SCROLLBAR )) continue;
        if (msg.message == WM_LBUTTONUP ||
            msg.message == WM_MOUSEMOVE ||
            msg.message == WM_MOUSELEAVE ||
            msg.message == WM_NCMOUSEMOVE ||
            msg.message == WM_NCMOUSELEAVE ||
            (msg.message == WM_SYSTIMER && msg.wParam == SCROLL_TIMER))
        {
            pt.x = (short)LOWORD( msg.lParam ) - rect.left;
            pt.y = (short)HIWORD( msg.lParam ) - rect.top;
            handle_scroll_event( hwnd, scrollbar, msg.message, pt );
        }
        else
        {
            NtUserTranslateMessage( &msg, 0 );
            NtUserDispatchMessage( &msg );
        }
        if (!is_window( hwnd ))
        {
            release_capture();
            break;
        }
    } while (msg.message != WM_LBUTTONUP && get_capture() == hwnd);
}

/***********************************************************************
 *           validate_scroll_info
 *
 * Determine if the supplied SCROLLINFO struct is valid.
 */
static inline BOOL validate_scroll_info( const SCROLLINFO *info )
{
    return !(info->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL | SIF_RETURNPREV) ||
             (info->cbSize != sizeof(*info) &&
              info->cbSize != sizeof(*info) - sizeof(info->nTrackPos)));
}


BOOL get_scroll_info( HWND hwnd, int bar, SCROLLINFO *info )
{
    struct scroll_info *scroll;

    /* handle invalid data structure */
    if (!validate_scroll_info( info ) || !(scroll = get_scroll_info_ptr( hwnd, bar, FALSE )))
        return FALSE;

    /* fill in the desired scroll info structure */
    if (info->fMask & SIF_PAGE) info->nPage = scroll->page;
    if (info->fMask & SIF_POS) info->nPos = scroll->curVal;
    if ((info->fMask & SIF_TRACKPOS) && (info->cbSize == sizeof(*info)))
        info->nTrackPos = g_tracking_info.win == get_full_window_handle( hwnd ) ?
            g_tracking_info.thumb_val : scroll->curVal;
    if (info->fMask & SIF_RANGE)
    {
        info->nMin = scroll->minVal;
        info->nMax = scroll->maxVal;
    }
    release_scroll_info_ptr( scroll );

    TRACE( "cbSize %02x fMask %04x nMin %d nMax %d nPage %u nPos %d nTrackPos %d\n",
           info->cbSize, info->fMask, info->nMin, info->nMax, info->nPage,
           info->nPos, info->nTrackPos );

    return (info->fMask & SIF_ALL) != 0;
}

static int set_scroll_info( HWND hwnd, int bar, const SCROLLINFO *info, BOOL redraw )
{
    struct scroll_info *scroll;
    UINT new_flags;
    int action = 0, ret = 0;

    /* handle invalid data structure */
    if (!validate_scroll_info( info ) ||
        !(scroll = get_scroll_info_ptr( hwnd, bar, TRUE )))
        return 0;

    if (TRACE_ON(scroll))
    {
        TRACE( "hwnd=%p bar=%d", hwnd, bar );
        if (info->fMask & SIF_PAGE) TRACE( " page=%d", info->nPage );
        if (info->fMask & SIF_POS) TRACE( " pos=%d", info->nPos );
        if (info->fMask & SIF_RANGE) TRACE( " min=%d max=%d", info->nMin, info->nMax );
        TRACE( "\n" );
    }

    /* undocumented flag, return previous position instead of modified */
    if (info->fMask & SIF_RETURNPREV) ret = scroll->curVal;

    /* Set the page size */
    if ((info->fMask & SIF_PAGE) && scroll->page != info->nPage)
    {
        scroll->page = info->nPage;
        action |= SA_SSI_REFRESH;
    }

    /* Set the scroll pos */
    if ((info->fMask & SIF_POS) && scroll->curVal != info->nPos)
    {
        scroll->curVal = info->nPos;
        action |= SA_SSI_REFRESH;
    }

    /* Set the scroll range */
    if (info->fMask & SIF_RANGE)
    {
        /* Invalid range -> range is set to (0,0) */
        if (info->nMin > info->nMax || (UINT)(info->nMax - info->nMin) >= 0x80000000)
        {
            action |= SA_SSI_REFRESH;
            scroll->minVal = 0;
            scroll->maxVal = 0;
        }
        else
        {
            if (scroll->minVal != info->nMin || scroll->maxVal != info->nMax)
            {
                action |= SA_SSI_REFRESH;
                scroll->minVal = info->nMin;
                scroll->maxVal = info->nMax;
            }
        }
    }

    /* Make sure the page size is valid */
    if (scroll->page < 0) scroll->page = 0;
    else if (scroll->page > scroll->maxVal - scroll->minVal + 1)
        scroll->page = scroll->maxVal - scroll->minVal + 1;

    /* Make sure the pos is inside the range */
    if (scroll->curVal < scroll->minVal)
        scroll->curVal = scroll->minVal;
    else if (scroll->curVal > scroll->maxVal - max( scroll->page - 1, 0 ))
        scroll->curVal = scroll->maxVal - max( scroll->page - 1, 0 );

    TRACE( "    new values: page=%d pos=%d min=%d max=%d\n", scroll->page, scroll->curVal,
           scroll->minVal, scroll->maxVal );

    /* don't change the scrollbar state if SetScrollInfo is just called with SIF_DISABLENOSCROLL */
    if(!(info->fMask & SIF_ALL)) goto done;

    /* Check if the scrollbar should be hidden or disabled */
    if (info->fMask & (SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL))
    {
        new_flags = scroll->flags;
        if (scroll->minVal >= scroll->maxVal - max( scroll->page - 1, 0 ))
        {
            /* Hide or disable scroll-bar */
            if (info->fMask & SIF_DISABLENOSCROLL)
            {
                new_flags = ESB_DISABLE_BOTH;
                action |= SA_SSI_REFRESH;
            }
            else if (bar != SB_CTL && (action & SA_SSI_REFRESH))
            {
                action = SA_SSI_HIDE;
            }
        }
        else if (info->fMask != SIF_PAGE)
        {
            /* Show and enable scroll-bar only if no page only changed. */
            new_flags = ESB_ENABLE_BOTH;
            if (bar != SB_CTL && (action & SA_SSI_REFRESH)) action |= SA_SSI_SHOW;
        }

        if (bar == SB_CTL && redraw && is_window_visible( hwnd ) &&
            (new_flags == ESB_ENABLE_BOTH || new_flags == ESB_DISABLE_BOTH))
        {
            release_scroll_info_ptr( scroll );
            enable_window( hwnd, new_flags == ESB_ENABLE_BOTH );
            if (!(scroll = get_scroll_info_ptr( hwnd, bar, FALSE ))) return 0;
        }

        if (scroll->flags != new_flags) /* check arrow flags */
        {
            scroll->flags = new_flags;
            action |= SA_SSI_REPAINT_ARROWS;
        }
    }

done:
    if (!(info->fMask & SIF_RETURNPREV)) ret = scroll->curVal;
    release_scroll_info_ptr( scroll );
    if (action & SA_SSI_HIDE)
        show_scroll_bar( hwnd, bar, FALSE, FALSE );
    else
    {
        if (action & SA_SSI_SHOW && show_scroll_bar( hwnd, bar, TRUE, TRUE ))
            return ret; /* NtUserSetWindowPos() already did the painting */

        if (redraw)
            refresh_scroll_bar( hwnd, bar, TRUE, TRUE );
        else if (action & SA_SSI_REPAINT_ARROWS)
            refresh_scroll_bar( hwnd, bar, TRUE, FALSE );
    }

    return ret; /* Return current position */
}

static BOOL get_scroll_bar_info( HWND hwnd, LONG id, SCROLLBARINFO *info )
{
    struct scroll_info *scroll;
    int bar, dummy;
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    BOOL pressed;
    RECT rect;

    switch (id)
    {
        case OBJID_CLIENT:  bar = SB_CTL; break;
        case OBJID_HSCROLL: bar = SB_HORZ; break;
        case OBJID_VSCROLL: bar = SB_VERT; break;
        default: return FALSE;
    }

    /* handle invalid data structure */
    if (info->cbSize != sizeof(*info)) return FALSE;

    get_scroll_bar_rect( hwnd, bar, &info->rcScrollBar, &dummy,
                         &info->dxyLineButton, &info->xyThumbTop );
    /* rcScrollBar needs to be in screen coordinates */
    get_window_rect( hwnd, &rect, get_thread_dpi() );
    OffsetRect( &info->rcScrollBar, rect.left, rect.top );

    info->xyThumbBottom = info->xyThumbTop + info->dxyLineButton;

    if (!(scroll = get_scroll_info_ptr( hwnd, bar, TRUE ))) return FALSE;

    /* Scroll bar state */
    info->rgstate[0] = 0;
    if ((bar == SB_HORZ && !(style & WS_HSCROLL)) ||
        (bar == SB_VERT && !(style & WS_VSCROLL)))
        info->rgstate[0] |= STATE_SYSTEM_INVISIBLE;
    if (scroll->minVal >= scroll->maxVal - max( scroll->page - 1, 0 ))
    {
        if (!(info->rgstate[0] & STATE_SYSTEM_INVISIBLE))
            info->rgstate[0] |= STATE_SYSTEM_UNAVAILABLE;
        else
            info->rgstate[0] |= STATE_SYSTEM_OFFSCREEN;
    }
    if (bar == SB_CTL && !is_window_enabled( hwnd ))
        info->rgstate[0] |= STATE_SYSTEM_UNAVAILABLE;

    pressed = (bar == SB_VERT) == g_tracking_info.vertical && get_capture() == hwnd;

    /* Top/left arrow button state. MSDN says top/right, but I don't believe it */
    info->rgstate[1] = 0;
    if (pressed && g_tracking_info.hit_test == SCROLL_TOP_ARROW)
        info->rgstate[1] |= STATE_SYSTEM_PRESSED;
    if (scroll->flags & ESB_DISABLE_LTUP)
        info->rgstate[1] |= STATE_SYSTEM_UNAVAILABLE;

    /* Page up/left region state. MSDN says up/right, but I don't believe it */
    info->rgstate[2] = 0;
    if (scroll->curVal == scroll->minVal)
        info->rgstate[2] |= STATE_SYSTEM_INVISIBLE;
    if (pressed && g_tracking_info.hit_test == SCROLL_TOP_RECT)
        info->rgstate[2] |= STATE_SYSTEM_PRESSED;

    /* Thumb state */
    info->rgstate[3] = 0;
    if (pressed && g_tracking_info.hit_test == SCROLL_THUMB)
        info->rgstate[3] |= STATE_SYSTEM_PRESSED;

    /* Page down/right region state. MSDN says down/left, but I don't believe it */
    info->rgstate[4] = 0;
    if (scroll->curVal >= scroll->maxVal - 1)
        info->rgstate[4] |= STATE_SYSTEM_INVISIBLE;
    if (pressed && g_tracking_info.hit_test == SCROLL_BOTTOM_RECT)
        info->rgstate[4] |= STATE_SYSTEM_PRESSED;

    /* Bottom/right arrow button state. MSDN says bottom/left, but I don't believe it */
    info->rgstate[5] = 0;
    if (pressed && g_tracking_info.hit_test == SCROLL_BOTTOM_ARROW)
        info->rgstate[5] |= STATE_SYSTEM_PRESSED;
    if (scroll->flags & ESB_DISABLE_RTDN)
        info->rgstate[5] |= STATE_SYSTEM_UNAVAILABLE;

    release_scroll_info_ptr( scroll );
    return TRUE;
}

static void create_scroll_bar( HWND hwnd, CREATESTRUCTW *create )
{
    struct scroll_info *info = NULL;
    WND *win;

    TRACE( "hwnd=%p create=%p\n", hwnd, create );

    win = get_win_ptr( hwnd );
    if (win->cbWndExtra >= sizeof(struct scroll_bar_win_data))
    {
        struct scroll_bar_win_data *data = (struct scroll_bar_win_data *)win->wExtra;
        data->magic = SCROLLBAR_MAGIC;
        info = &data->info;
    }
    else WARN( "Not enough extra data\n" );
    release_win_ptr( win );
    if (!info) return;

    if (create->style & WS_DISABLED)
    {
        info->flags = ESB_DISABLE_BOTH;
        TRACE( "Created WS_DISABLED scrollbar\n" );
    }

    if (create->style & (SBS_SIZEGRIP | SBS_SIZEBOX))
    {
        if (create->style & SBS_SIZEBOXTOPLEFTALIGN)
            NtUserMoveWindow( hwnd, create->x, create->y, get_system_metrics( SM_CXVSCROLL ) + 1,
                              get_system_metrics( SM_CYHSCROLL ) + 1, FALSE );
        else if(create->style & SBS_SIZEBOXBOTTOMRIGHTALIGN)
            NtUserMoveWindow( hwnd, create->x + create->cx - get_system_metrics( SM_CXVSCROLL ) - 1,
                              create->y + create->cy-get_system_metrics( SM_CYHSCROLL ) - 1,
                              get_system_metrics( SM_CXVSCROLL ) + 1,
                              get_system_metrics( SM_CYHSCROLL ) + 1, FALSE );
    }
    else if (create->style & SBS_VERT)
    {
        if (create->style & SBS_LEFTALIGN)
            NtUserMoveWindow( hwnd, create->x, create->y, get_system_metrics( SM_CXVSCROLL ) + 1,
                              create->cy, FALSE );
        else if (create->style & SBS_RIGHTALIGN)
            NtUserMoveWindow( hwnd, create->x + create->cx - get_system_metrics( SM_CXVSCROLL ) - 1,
                              create->y, get_system_metrics( SM_CXVSCROLL ) + 1, create->cy, FALSE );
    }
    else  /* SBS_HORZ */
    {
        if (create->style & SBS_TOPALIGN)
            NtUserMoveWindow( hwnd, create->x, create->y, create->cx,
                              get_system_metrics( SM_CYHSCROLL ) + 1, FALSE );
        else if (create->style & SBS_BOTTOMALIGN)
            NtUserMoveWindow( hwnd, create->x,
                              create->y + create->cy - get_system_metrics( SM_CYHSCROLL ) - 1,
                              create->cx, get_system_metrics( SM_CYHSCROLL ) + 1, FALSE );
    }
}

static void handle_kbd_event( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd=%p wparam=%ld lparam=%ld\n", hwnd, (long)wparam, lparam );

    /* hide caret on first KEYDOWN to prevent flicker */
    if ((lparam & PFD_DOUBLEBUFFER_DONTCARE) == 0)
        NtUserHideCaret( hwnd );

    switch (wparam)
    {
    case VK_PRIOR: wparam = SB_PAGEUP; break;
    case VK_NEXT:  wparam = SB_PAGEDOWN; break;
    case VK_HOME:  wparam = SB_TOP; break;
    case VK_END:   wparam = SB_BOTTOM; break;
    case VK_UP:    wparam = SB_LINEUP; break;
    case VK_DOWN:  wparam = SB_LINEDOWN; break;
    case VK_LEFT:  wparam = SB_LINEUP; break;
    case VK_RIGHT: wparam = SB_LINEDOWN; break;
    default: return;
    }

    send_message( get_parent(hwnd),
                  (get_window_long( hwnd, GWL_STYLE ) & SBS_VERT) ? WM_VSCROLL : WM_HSCROLL,
                  wparam, (LPARAM)hwnd);
}

static int get_scroll_pos(HWND hwnd, int bar)
{
    struct scroll_info *scroll = get_scroll_info_ptr( hwnd, bar, FALSE );
    int ret;
    if (!scroll) return 0;
    ret = scroll->curVal;
    release_scroll_info_ptr( scroll );
    return ret;
}

static BOOL set_scroll_range( HWND hwnd, int bar, int min_val, int max_val )
{
    struct scroll_info *scroll = get_scroll_info_ptr( hwnd, bar, FALSE );

    TRACE( "hwnd=%p bar=%d min=%d max=%d\n", hwnd, bar, min_val, max_val );

    if (scroll)
    {
        scroll->minVal = min_val;
        scroll->maxVal = max_val;
        release_scroll_info_ptr( scroll );
    }
    return TRUE;
}

static BOOL get_scroll_range( HWND hwnd, int nBar, int *min, int *max )
{
    struct scroll_info *info;

    if (!(info = get_scroll_info_ptr( hwnd, nBar, FALSE ))) return FALSE;
    if (min) *min = info->minVal;
    if (max) *max = info->maxVal;
    release_scroll_info_ptr( info );
    return TRUE;
}

LRESULT scroll_bar_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    if (!is_window( hwnd )) return 0;

    switch (msg)
    {
    case WM_CREATE:
        create_scroll_bar( hwnd, (CREATESTRUCTW *)lparam );
        return 0;

    case WM_KEYDOWN:
        handle_kbd_event( hwnd, wparam, lparam );
        return 0;

    case WM_KEYUP:
        NtUserShowCaret( hwnd );
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        if (get_window_long( hwnd, GWL_STYLE ) & SBS_SIZEGRIP)
        {
            send_message( get_parent(hwnd), WM_SYSCOMMAND,
                          SC_SIZE + ((get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) ?
                                     WMSZ_BOTTOMLEFT : WMSZ_BOTTOMRIGHT), lparam );
        }
        else
        {
            POINT pt;
            pt.x = (short)LOWORD( lparam );
            pt.y = (short)HIWORD( lparam );
            track_scroll_bar( hwnd, SB_CTL, pt );
        }
        return 0;

    case WM_LBUTTONUP:
    case WM_NCMOUSEMOVE:
    case WM_NCMOUSELEAVE:
    case WM_MOUSEMOVE:
    case WM_MOUSELEAVE:
    case WM_SYSTIMER:
        {
            POINT pt;
            pt.x = (short)LOWORD( lparam );
            pt.y = (short)HIWORD( lparam );
            handle_scroll_event( hwnd, SB_CTL, msg, pt );
        }
        return 0;

    case WM_ENABLE:
        {
            struct scroll_info *scroll;
            if ((scroll = get_scroll_info_ptr( hwnd, SB_CTL, FALSE )))
            {
                scroll->flags = wparam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH;
                release_scroll_info_ptr( scroll );
                refresh_scroll_bar( hwnd, SB_CTL, TRUE, TRUE );
            }
        }
        return 0;

    case WM_SETFOCUS:
        {
            /* Create a caret when a ScrollBar get focus */
            RECT rect;
            int arrow_size, thumb_size, thumb_pos, vertical;
            vertical = get_scroll_bar_rect( hwnd, SB_CTL, &rect, &arrow_size,
                                            &thumb_size, &thumb_pos );
            if (!vertical)
            {
                NtUserCreateCaret( hwnd, (HBITMAP)1, thumb_size - 2, rect.bottom - rect.top - 2 );
                set_caret_pos( thumb_pos + 1, rect.top + 1 );
            }
            else
            {
                NtUserCreateCaret( hwnd, (HBITMAP)1, rect.right - rect.left - 2, thumb_size - 2 );
                set_caret_pos( rect.top + 1, thumb_pos + 1 );
            }
            NtUserShowCaret( hwnd );
        }
        return 0;

    case WM_KILLFOCUS:
        {
            int arrow_size, thumb_size, thumb_pos, vertical;
            RECT rect;
            vertical = get_scroll_bar_rect( hwnd, SB_CTL, &rect,&arrow_size, &thumb_size, &thumb_pos );
            if (!vertical)
            {
                rect.left = thumb_pos + 1;
                rect.right = rect.left + thumb_size;
            }
            else
            {
                rect.top = thumb_pos + 1;
                rect.bottom = rect.top + thumb_size;
            }
            NtUserHideCaret( hwnd );
            NtUserInvalidateRect( hwnd, &rect, 0 );
            destroy_caret();
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS; /* Windows returns this value */

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = wparam ? (HDC)wparam : NtUserBeginPaint( hwnd, &ps );
            draw_scroll_bar( hwnd, hdc, SB_CTL, g_tracking_info.hit_test, &g_tracking_info, TRUE, TRUE );
            if (!wparam) NtUserEndPaint( hwnd, &ps );
        }
        return 0;

    case SBM_SETRANGEREDRAW:
    case SBM_SETRANGE:
        {
            int old_pos = get_scroll_pos( hwnd, SB_CTL );
            set_scroll_range( hwnd, SB_CTL, wparam, lparam );
            if (msg == SBM_SETRANGEREDRAW) refresh_scroll_bar( hwnd, SB_CTL, TRUE, TRUE );
            if (old_pos != get_scroll_pos( hwnd, SB_CTL )) return old_pos;
        }
        return 0;

    case SBM_GETSCROLLINFO:
        return get_scroll_info( hwnd, SB_CTL, (SCROLLINFO *)lparam );

    case SBM_GETSCROLLBARINFO:
        return get_scroll_bar_info( hwnd, OBJID_CLIENT, (SCROLLBARINFO *)lparam );

    case SBM_SETSCROLLINFO:
        return set_scroll_info( hwnd, SB_CTL, (SCROLLINFO *)lparam, wparam );

    case WM_SETCURSOR:
        if (get_window_long( hwnd, GWL_STYLE ) & SBS_SIZEGRIP)
        {
            ULONG_PTR cursor = (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) ?
                IDC_SIZENESW : IDC_SIZENWSE;
            return (LRESULT)NtUserSetCursor( LoadImageW( 0, (const WCHAR *)cursor, IMAGE_CURSOR,
                                                         0, 0, LR_SHARED | LR_DEFAULTSIZE ));
        }
        return default_window_proc( hwnd, msg, wparam, lparam, ansi );

    case SBM_SETPOS:
        {
            SCROLLINFO info;
            info.cbSize = sizeof(info);
            info.nPos   = wparam;
            info.fMask  = SIF_POS | SIF_RETURNPREV;
            return NtUserSetScrollInfo( hwnd, SB_CTL, &info, lparam );
        }

    case SBM_GETPOS:
       return get_scroll_pos( hwnd, SB_CTL );

    case SBM_GETRANGE:
        return get_scroll_range( hwnd, SB_CTL, (int *)wparam, (int *)lparam );

    case SBM_ENABLE_ARROWS:
        return NtUserEnableScrollBar( hwnd, SB_CTL, wparam );

    case 0x00e5:
    case 0x00e7:
    case 0x00e8:
    case 0x00ec:
    case 0x00ed:
    case 0x00ee:
    case 0x00ef:
        ERR( "unknown Win32 msg %04x wp=%08lx lp=%08lx\n", msg, (long)wparam, lparam );
        return 0;

    default:
        if (msg >= WM_USER)
            WARN( "unknown msg %04x wp=%08lx lp=%08lx\n", msg, (long)wparam, lparam );
        return default_window_proc( hwnd, msg, wparam, lparam, ansi );
    }
}

void set_standard_scroll_painted( HWND hwnd, int bar, BOOL painted )
{
    struct scroll_info *info;

    if (bar != SB_HORZ && bar != SB_VERT)
        return;

    if ((info = get_scroll_info_ptr( hwnd, bar, FALSE )))
    {
        info->painted = painted;
        release_scroll_info_ptr( info );
    }
}

/*************************************************************************
 *           NtUserShowScrollBar   (win32u.@)
 */
BOOL WINAPI NtUserShowScrollBar( HWND hwnd, INT bar, BOOL show )
{
    if (!hwnd) return FALSE;

    show_scroll_bar( hwnd, bar, bar == SB_VERT ? 0 : show, bar == SB_HORZ ? 0 : show );
    return TRUE;
}

/*************************************************************************
 *           NtUserGetScrollBarInfo   (win32u.@)
 */
BOOL WINAPI NtUserGetScrollBarInfo( HWND hwnd, LONG id, SCROLLBARINFO *info )
{
    TRACE( "hwnd=%p id=%d info=%p\n", hwnd, (int)id, info );

    /* Refer OBJID_CLIENT requests to the window */
    if (id == OBJID_CLIENT)
        return send_message( hwnd, SBM_GETSCROLLBARINFO, 0, (LPARAM)info );
    return get_scroll_bar_info( hwnd, id, info );
}

/*************************************************************************
 *           NtUserEnableScrollBar   (win32u.@)
 */
BOOL WINAPI NtUserEnableScrollBar( HWND hwnd, UINT bar, UINT flags )
{
    struct scroll_info *scroll;
    BOOL check_flags;

    flags &= ESB_DISABLE_BOTH;

    if (bar == SB_BOTH)
    {
        if (!(scroll = get_scroll_info_ptr( hwnd, SB_VERT, TRUE ))) return FALSE;
        check_flags = scroll->flags == flags;
        scroll->flags = flags;
        release_scroll_info_ptr( scroll );
        if (!check_flags) refresh_scroll_bar( hwnd, SB_VERT, TRUE, TRUE );
        bar = SB_HORZ;
    }
    else
        check_flags = bar != SB_CTL;

    if (!(scroll = get_scroll_info_ptr( hwnd, bar, TRUE ))) return FALSE;
    if (check_flags) check_flags = scroll->flags == flags;
    scroll->flags = flags;
    release_scroll_info_ptr( scroll );
    if (check_flags) return FALSE;

    if (bar == SB_CTL && (flags == ESB_DISABLE_BOTH || flags == ESB_ENABLE_BOTH))
        NtUserEnableWindow( hwnd, flags == ESB_ENABLE_BOTH );

    refresh_scroll_bar( hwnd, bar, TRUE, TRUE );
    return TRUE;
}


/*************************************************************************
 *           NtUserSetScrollInfo   (win32u.@)
 */
INT WINAPI NtUserSetScrollInfo( HWND hwnd, int bar, const SCROLLINFO *info, BOOL redraw )
{
    TRACE( "hwnd=%p bar=%d info=%p, redraw=%d\n", hwnd, bar, info, redraw );

    /* Refer SB_CTL requests to the window */
    if (bar == SB_CTL)
        return send_message( hwnd, SBM_SETSCROLLINFO, redraw, (LPARAM)info );

    return set_scroll_info( hwnd, bar, info, redraw );
}

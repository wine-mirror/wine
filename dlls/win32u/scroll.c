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

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);


#define SCROLLBAR_MAGIC 0x5c6011ba

/* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4

/* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 8

/* Overlap between arrows and thumb */
#define SCROLL_ARROW_THUMB_OVERLAP 0

static struct scroll_info *get_scroll_info_ptr( HWND hwnd, int bar, BOOL alloc )
{
    struct scroll_info *ret = NULL;
    user_lock();
    if (user_callbacks) ret = user_callbacks->get_scroll_info( hwnd, bar, alloc );
    if (!ret) user_unlock();
    return ret;
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

LRESULT scroll_bar_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    switch (msg)
    {
    case WM_CREATE:
        create_scroll_bar( hwnd, (CREATESTRUCTW *)lparam );
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

    default:
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

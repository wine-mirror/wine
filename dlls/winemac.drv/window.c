/*
 * MACDRV windowing driver
 *
 * Copyright 1993, 1994, 1995, 1996, 2001 Alexandre Julliard
 * Copyright 1993 David Metcalfe
 * Copyright 1995, 1996 Alex Korobka
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include "config.h"

#include "macdrv.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(macdrv);


static CRITICAL_SECTION win_data_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &win_data_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": win_data_section") }
};
static CRITICAL_SECTION win_data_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static CFMutableDictionaryRef win_datas;


/***********************************************************************
 *              get_cocoa_window_features
 */
static void get_cocoa_window_features(struct macdrv_win_data *data,
                                      DWORD style, DWORD ex_style,
                                      struct macdrv_window_features* wf)
{
    memset(wf, 0, sizeof(*wf));

    if (IsRectEmpty(&data->window_rect)) return;

    if ((style & WS_CAPTION) == WS_CAPTION && !(ex_style & WS_EX_LAYERED))
    {
        wf->shadow = 1;
        wf->title_bar = 1;
        if (style & WS_SYSMENU) wf->close_button = 1;
        if (style & WS_MINIMIZEBOX) wf->minimize_button = 1;
        if (style & WS_MAXIMIZEBOX) wf->resizable = 1;
        if (ex_style & WS_EX_TOOLWINDOW) wf->utility = 1;
    }
    if (ex_style & WS_EX_DLGMODALFRAME) wf->shadow = 1;
    else if (style & WS_THICKFRAME)
    {
        wf->shadow = 1;
        wf->resizable = 1;
    }
    else if ((style & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME) wf->shadow = 1;
}


/***********************************************************************
 *              get_mac_rect_offset
 *
 * Helper for macdrv_window_to_mac_rect and macdrv_mac_to_window_rect.
 */
static void get_mac_rect_offset(struct macdrv_win_data *data, DWORD style, RECT *rect)
{
    DWORD ex_style, style_mask = 0, ex_style_mask = 0;
    struct macdrv_window_features wf;

    rect->top = rect->bottom = rect->left = rect->right = 0;

    ex_style = GetWindowLongW(data->hwnd, GWL_EXSTYLE);

    get_cocoa_window_features(data, style, ex_style, &wf);

    if (wf.title_bar) style_mask |= WS_CAPTION;
    if (wf.shadow)
    {
        style_mask |= WS_DLGFRAME | WS_THICKFRAME;
        ex_style_mask |= WS_EX_DLGMODALFRAME;
    }

    AdjustWindowRectEx(rect, style & style_mask, FALSE, ex_style & ex_style_mask);

    TRACE("%p/%p style %08x ex_style %08x -> %s\n", data->hwnd, data->cocoa_window,
          style, ex_style, wine_dbgstr_rect(rect));
}


/***********************************************************************
 *              show_window
 */
static void show_window(struct macdrv_win_data *data)
{
    TRACE("win %p/%p\n", data->hwnd, data->cocoa_window);

    data->on_screen = macdrv_order_cocoa_window(data->cocoa_window, NULL, NULL);
}


/***********************************************************************
 *              hide_window
 */
static void hide_window(struct macdrv_win_data *data)
{
    TRACE("win %p/%p\n", data->hwnd, data->cocoa_window);

    macdrv_hide_cocoa_window(data->cocoa_window);
    data->on_screen = FALSE;
}


/***********************************************************************
 *              macdrv_window_to_mac_rect
 *
 * Convert a rect from client to Mac window coordinates
 */
static void macdrv_window_to_mac_rect(struct macdrv_win_data *data, DWORD style, RECT *rect)
{
    RECT rc;

    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return;
    if (IsRectEmpty(rect)) return;

    get_mac_rect_offset(data, style, &rc);

    rect->left   -= rc.left;
    rect->right  -= rc.right;
    rect->top    -= rc.top;
    rect->bottom -= rc.bottom;
    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *              macdrv_mac_to_window_rect
 *
 * Opposite of macdrv_window_to_mac_rect
 */
static void macdrv_mac_to_window_rect(struct macdrv_win_data *data, RECT *rect)
{
    RECT rc;
    DWORD style = GetWindowLongW(data->hwnd, GWL_STYLE);

    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return;
    if (IsRectEmpty(rect)) return;

    get_mac_rect_offset(data, style, &rc);

    rect->left   += rc.left;
    rect->right  += rc.right;
    rect->top    += rc.top;
    rect->bottom += rc.bottom;
    if (rect->top >= rect->bottom) rect->bottom = rect->top + 1;
    if (rect->left >= rect->right) rect->right = rect->left + 1;
}


/***********************************************************************
 *              constrain_window_frame
 *
 * Alter a window frame rectangle to fit within a) Cocoa's documented
 * limits, and b) sane sizes, like twice the desktop rect.
 */
static void constrain_window_frame(CGRect* frame)
{
    CGRect desktop_rect = macdrv_get_desktop_rect();
    int max_width, max_height;

    max_width = min(32000, 2 * CGRectGetWidth(desktop_rect));
    max_height = min(32000, 2 * CGRectGetHeight(desktop_rect));

    if (frame->origin.x < -16000) frame->origin.x = -16000;
    if (frame->origin.y < -16000) frame->origin.y = -16000;
    if (frame->origin.x > 16000) frame->origin.x = 16000;
    if (frame->origin.y > 16000) frame->origin.y = 16000;
    if (frame->size.width > max_width) frame->size.width = max_width;
    if (frame->size.height > max_height) frame->size.height = max_height;
}


/***********************************************************************
 *              alloc_win_data
 */
static struct macdrv_win_data *alloc_win_data(HWND hwnd)
{
    struct macdrv_win_data *data;

    if ((data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data))))
    {
        data->hwnd = hwnd;
        EnterCriticalSection(&win_data_section);
        if (!win_datas)
            win_datas = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
        CFDictionarySetValue(win_datas, hwnd, data);
    }
    return data;
}


/***********************************************************************
 *              get_win_data
 *
 * Lock and return the data structure associated with a window.
 */
static struct macdrv_win_data *get_win_data(HWND hwnd)
{
    struct macdrv_win_data *data;

    if (!hwnd) return NULL;
    EnterCriticalSection(&win_data_section);
    if (win_datas && (data = (struct macdrv_win_data*)CFDictionaryGetValue(win_datas, hwnd)))
        return data;
    LeaveCriticalSection(&win_data_section);
    return NULL;
}


/***********************************************************************
 *              release_win_data
 *
 * Release the data returned by get_win_data.
 */
static void release_win_data(struct macdrv_win_data *data)
{
    if (data) LeaveCriticalSection(&win_data_section);
}


/***********************************************************************
 *              macdrv_get_cocoa_window
 *
 * Return the Mac window associated with the full area of a window
 */
static macdrv_window macdrv_get_cocoa_window(HWND hwnd)
{
    struct macdrv_win_data *data = get_win_data(hwnd);
    macdrv_window ret = data ? data->cocoa_window : NULL;
    release_win_data(data);
    return ret;
}


/***********************************************************************
 *              set_cocoa_window_properties
 *
 * Set the window properties for a Cocoa window based on its Windows
 * properties.
 */
static void set_cocoa_window_properties(struct macdrv_win_data *data)
{
    DWORD style, ex_style;
    struct macdrv_window_features wf;

    style = GetWindowLongW(data->hwnd, GWL_STYLE);
    ex_style = GetWindowLongW(data->hwnd, GWL_EXSTYLE);

    get_cocoa_window_features(data, style, ex_style, &wf);
    macdrv_set_cocoa_window_features(data->cocoa_window, &wf);
}


/**********************************************************************
 *              create_cocoa_window
 *
 * Create the whole Mac window for a given window
 */
static void create_cocoa_window(struct macdrv_win_data *data)
{
    WCHAR text[1024];
    struct macdrv_window_features wf;
    CGRect frame;
    DWORD style, ex_style;

    style = GetWindowLongW(data->hwnd, GWL_STYLE);
    ex_style = GetWindowLongW(data->hwnd, GWL_EXSTYLE);

    data->whole_rect = data->window_rect;
    macdrv_window_to_mac_rect(data, style, &data->whole_rect);

    memset(&wf, 0, sizeof(wf));
    get_cocoa_window_features(data, style, ex_style, &wf);

    frame = cgrect_from_rect(data->whole_rect);
    constrain_window_frame(&frame);

    TRACE("creating %p window %s whole %s client %s\n", data->hwnd, wine_dbgstr_rect(&data->window_rect),
          wine_dbgstr_rect(&data->whole_rect), wine_dbgstr_rect(&data->client_rect));

    data->cocoa_window = macdrv_create_cocoa_window(&wf, frame);
    if (!data->cocoa_window) return;

    set_cocoa_window_properties(data);

    /* set the window text */
    if (!InternalGetWindowText(data->hwnd, text, sizeof(text)/sizeof(WCHAR))) text[0] = 0;
    macdrv_set_cocoa_window_title(data->cocoa_window, text, strlenW(text));
}


/**********************************************************************
 *              destroy_cocoa_window
 *
 * Destroy the whole Mac window for a given window.
 */
static void destroy_cocoa_window(struct macdrv_win_data *data)
{
    if (!data->cocoa_window) return;

    TRACE("win %p Cocoa win %p\n", data->hwnd, data->cocoa_window);

    macdrv_destroy_cocoa_window(data->cocoa_window);
    data->cocoa_window = 0;
    data->on_screen = FALSE;
}


/***********************************************************************
 *              macdrv_create_win_data
 *
 * Create a Mac data window structure for an existing window.
 */
static struct macdrv_win_data *macdrv_create_win_data(HWND hwnd, const RECT *window_rect,
                                                      const RECT *client_rect)
{
    struct macdrv_win_data *data;
    HWND parent;

    if (GetWindowThreadProcessId(hwnd, NULL) != GetCurrentThreadId()) return NULL;

    if (!(parent = GetAncestor(hwnd, GA_PARENT)))  /* desktop */
        return NULL;

    /* don't create win data for HWND_MESSAGE windows */
    if (parent != GetDesktopWindow() && !GetAncestor(parent, GA_PARENT)) return NULL;

    if (!(data = alloc_win_data(hwnd))) return NULL;

    data->whole_rect = data->window_rect = *window_rect;
    data->client_rect = *client_rect;

    if (parent == GetDesktopWindow())
    {
        create_cocoa_window(data);
        TRACE("win %p/%p window %s whole %s client %s\n",
               hwnd, data->cocoa_window, wine_dbgstr_rect(&data->window_rect),
               wine_dbgstr_rect(&data->whole_rect), wine_dbgstr_rect(&data->client_rect));
    }

    return data;
}


/***********************************************************************
 *              sync_window_position
 *
 * Synchronize the Mac window position with the Windows one
 */
static void sync_window_position(struct macdrv_win_data *data, UINT swp_flags)
{
    CGRect frame = cgrect_from_rect(data->whole_rect);
    constrain_window_frame(&frame);

    data->on_screen = macdrv_set_cocoa_window_frame(data->cocoa_window, &frame);

    TRACE("win %p/%p pos %s\n", data->hwnd, data->cocoa_window,
          wine_dbgstr_rect(&data->whole_rect));

    if (data->on_screen && (!(swp_flags & SWP_NOZORDER) || (swp_flags & SWP_SHOWWINDOW)))
    {
        HWND next = NULL;
        macdrv_window prev_window = NULL;
        macdrv_window next_window = NULL;

        /* find window that this one must be after */
        HWND prev = GetWindow(data->hwnd, GW_HWNDPREV);
        while (prev && !((GetWindowLongW(prev, GWL_STYLE) & WS_VISIBLE) &&
                         (prev_window = macdrv_get_cocoa_window(prev))))
            prev = GetWindow(prev, GW_HWNDPREV);
        if (!prev_window)
        {
            /* find window that this one must be before */
            next = GetWindow(data->hwnd, GW_HWNDNEXT);
            while (next && !((GetWindowLongW(next, GWL_STYLE) & WS_VISIBLE) &&
                             (next_window = macdrv_get_cocoa_window(next))))
                next = GetWindow(next, GW_HWNDNEXT);
        }

        data->on_screen = macdrv_order_cocoa_window(data->cocoa_window, prev_window, next_window);

        TRACE("win %p/%p below %p/%p above %p/%p\n",
              data->hwnd, data->cocoa_window, prev, prev_window, next, next_window);
    }
}


/***********************************************************************
 *              move_window_bits
 *
 * Move the window bits when a window is moved.
 */
static void move_window_bits(HWND hwnd, macdrv_window window, const RECT *old_rect, const RECT *new_rect,
                             const RECT *old_client_rect, const RECT *new_client_rect,
                             const RECT *new_window_rect)
{
    RECT src_rect = *old_rect;
    RECT dst_rect = *new_rect;
    HDC hdc_src, hdc_dst;
    HRGN rgn;
    HWND parent = 0;

    if (!window)
    {
        OffsetRect(&dst_rect, -new_window_rect->left, -new_window_rect->top);
        parent = GetAncestor(hwnd, GA_PARENT);
        hdc_src = GetDCEx(parent, 0, DCX_CACHE);
        hdc_dst = GetDCEx(hwnd, 0, DCX_CACHE | DCX_WINDOW);
    }
    else
    {
        OffsetRect(&dst_rect, -new_client_rect->left, -new_client_rect->top);
        /* make src rect relative to the old position of the window */
        OffsetRect(&src_rect, -old_client_rect->left, -old_client_rect->top);
        if (dst_rect.left == src_rect.left && dst_rect.top == src_rect.top) return;
        hdc_src = hdc_dst = GetDCEx(hwnd, 0, DCX_CACHE);
    }

    rgn = CreateRectRgnIndirect(&dst_rect);
    SelectClipRgn(hdc_dst, rgn);
    DeleteObject(rgn);
    ExcludeUpdateRgn(hdc_dst, hwnd);

    TRACE("copying bits for win %p/%p %s -> %s\n", hwnd, window,
          wine_dbgstr_rect(&src_rect), wine_dbgstr_rect(&dst_rect));
    BitBlt(hdc_dst, dst_rect.left, dst_rect.top,
           dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top,
           hdc_src, src_rect.left, src_rect.top, SRCCOPY);

    ReleaseDC(hwnd, hdc_dst);
    if (hdc_src != hdc_dst) ReleaseDC(parent, hdc_src);
}


/**********************************************************************
 *              CreateDesktopWindow   (MACDRV.@)
 */
BOOL CDECL macdrv_CreateDesktopWindow(HWND hwnd)
{
    unsigned int width, height;

    TRACE("%p\n", hwnd);

    /* retrieve the real size of the desktop */
    SERVER_START_REQ(get_window_rectangles)
    {
        req->handle = wine_server_user_handle(hwnd);
        req->relative = COORDS_CLIENT;
        wine_server_call(req);
        width  = reply->window.right;
        height = reply->window.bottom;
    }
    SERVER_END_REQ;

    if (!width && !height)  /* not initialized yet */
    {
        CGRect rect = macdrv_get_desktop_rect();

        SERVER_START_REQ(set_window_pos)
        {
            req->handle        = wine_server_user_handle(hwnd);
            req->previous      = 0;
            req->swp_flags     = SWP_NOZORDER;
            req->window.left   = CGRectGetMinX(rect);
            req->window.top    = CGRectGetMinY(rect);
            req->window.right  = CGRectGetMaxX(rect);
            req->window.bottom = CGRectGetMaxY(rect);
            req->client        = req->window;
            wine_server_call(req);
        }
        SERVER_END_REQ;
    }

    return TRUE;
}


/**********************************************************************
 *              CreateWindow   (MACDRV.@)
 */
BOOL CDECL macdrv_CreateWindow(HWND hwnd)
{
    return TRUE;
}


/***********************************************************************
 *              DestroyWindow   (MACDRV.@)
 */
void CDECL macdrv_DestroyWindow(HWND hwnd)
{
    struct macdrv_win_data *data;

    TRACE("%p\n", hwnd);

    if (!(data = get_win_data(hwnd))) return;

    destroy_cocoa_window(data);

    CFDictionaryRemoveValue(win_datas, hwnd);
    release_win_data(data);
    HeapFree(GetProcessHeap(), 0, data);
}


/*****************************************************************
 *              SetParent   (MACDRV.@)
 */
void CDECL macdrv_SetParent(HWND hwnd, HWND parent, HWND old_parent)
{
    struct macdrv_win_data *data;

    TRACE("%p, %p, %p\n", hwnd, parent, old_parent);

    if (parent == old_parent) return;
    if (!(data = get_win_data(hwnd))) return;

    if (parent != GetDesktopWindow()) /* a child window */
    {
        if (old_parent == GetDesktopWindow())
        {
            /* destroy the old Mac window */
            destroy_cocoa_window(data);
        }
    }
    else  /* new top level window */
        create_cocoa_window(data);
    release_win_data(data);
}


/***********************************************************************
 *              SetWindowStyle   (MACDRV.@)
 *
 * Update the state of the Cocoa window to reflect a style change
 */
void CDECL macdrv_SetWindowStyle(HWND hwnd, INT offset, STYLESTRUCT *style)
{
    struct macdrv_win_data *data;

    TRACE("%p, %d, %p\n", hwnd, offset, style);

    if (hwnd == GetDesktopWindow()) return;
    if (!(data = get_win_data(hwnd))) return;

    if (data->cocoa_window)
        set_cocoa_window_properties(data);

    release_win_data(data);
}


/*****************************************************************
 *              SetWindowText   (MACDRV.@)
 */
void CDECL macdrv_SetWindowText(HWND hwnd, LPCWSTR text)
{
    macdrv_window win;

    TRACE("%p, %s\n", hwnd, debugstr_w(text));

    if ((win = macdrv_get_cocoa_window(hwnd)))
        macdrv_set_cocoa_window_title(win, text, strlenW(text));
}


/***********************************************************************
 *              WindowPosChanging   (MACDRV.@)
 */
void CDECL macdrv_WindowPosChanging(HWND hwnd, HWND insert_after, UINT swp_flags,
                                    const RECT *window_rect, const RECT *client_rect,
                                    RECT *visible_rect, struct window_surface **surface)
{
    struct macdrv_win_data *data = get_win_data(hwnd);
    DWORD style = GetWindowLongW(hwnd, GWL_STYLE);

    TRACE("%p after %p swp %04x window %s client %s visible %s surface %p\n", hwnd, insert_after,
          swp_flags, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
          wine_dbgstr_rect(visible_rect), surface);

    if (!data && !(data = macdrv_create_win_data(hwnd, window_rect, client_rect))) return;

    *visible_rect = *window_rect;
    macdrv_window_to_mac_rect(data, style, visible_rect);
    TRACE("visible_rect %s -> %s\n", wine_dbgstr_rect(window_rect),
          wine_dbgstr_rect(visible_rect));

    /* release the window surface if necessary */
    if (!data->cocoa_window) goto done;
    if (swp_flags & SWP_HIDEWINDOW) goto done;

    if (*surface) window_surface_release(*surface);
    *surface = NULL;

done:
    release_win_data(data);
}


/***********************************************************************
 *              WindowPosChanged   (MACDRV.@)
 */
void CDECL macdrv_WindowPosChanged(HWND hwnd, HWND insert_after, UINT swp_flags,
                                   const RECT *window_rect, const RECT *client_rect,
                                   const RECT *visible_rect, const RECT *valid_rects,
                                   struct window_surface *surface)
{
    struct macdrv_win_data *data;
    DWORD new_style = GetWindowLongW(hwnd, GWL_STYLE);
    RECT old_window_rect, old_whole_rect, old_client_rect;

    if (!(data = get_win_data(hwnd))) return;

    old_window_rect = data->window_rect;
    old_whole_rect  = data->whole_rect;
    old_client_rect = data->client_rect;
    data->window_rect = *window_rect;
    data->whole_rect  = *visible_rect;
    data->client_rect = *client_rect;

    TRACE("win %p/%p window %s whole %s client %s style %08x flags %08x surface %p\n",
           hwnd, data->cocoa_window, wine_dbgstr_rect(window_rect),
           wine_dbgstr_rect(visible_rect), wine_dbgstr_rect(client_rect),
           new_style, swp_flags, surface);

    if (!IsRectEmpty(&valid_rects[0]))
    {
        macdrv_window window = data->cocoa_window;
        int x_offset = old_whole_rect.left - data->whole_rect.left;
        int y_offset = old_whole_rect.top - data->whole_rect.top;

        /* if all that happened is that the whole window moved, copy everything */
        if (!(swp_flags & SWP_FRAMECHANGED) &&
            old_whole_rect.right   - data->whole_rect.right   == x_offset &&
            old_whole_rect.bottom  - data->whole_rect.bottom  == y_offset &&
            old_client_rect.left   - data->client_rect.left   == x_offset &&
            old_client_rect.right  - data->client_rect.right  == x_offset &&
            old_client_rect.top    - data->client_rect.top    == y_offset &&
            old_client_rect.bottom - data->client_rect.bottom == y_offset &&
            !memcmp(&valid_rects[0], &data->client_rect, sizeof(RECT)))
        {
            /* A Cocoa window's bits are moved automatically */
            if (!window && (x_offset != 0 || y_offset != 0))
            {
                release_win_data(data);
                move_window_bits(hwnd, window, &old_whole_rect, visible_rect,
                                 &old_client_rect, client_rect, window_rect);
                if (!(data = get_win_data(hwnd))) return;
            }
        }
        else
        {
            release_win_data(data);
            move_window_bits(hwnd, window, &valid_rects[1], &valid_rects[0],
                             &old_client_rect, client_rect, window_rect);
            if (!(data = get_win_data(hwnd))) return;
        }
    }

    if (!data->cocoa_window) goto done;

    if (data->on_screen)
    {
        if ((swp_flags & SWP_HIDEWINDOW) && !(new_style & WS_VISIBLE))
            hide_window(data);
    }

    if (new_style & WS_VISIBLE)
    {
        if (!data->on_screen || (swp_flags & (SWP_FRAMECHANGED|SWP_STATECHANGED)))
            set_cocoa_window_properties(data);

        if (!data->on_screen)
            show_window(data);
    }

    sync_window_position(data, swp_flags);
    set_cocoa_window_properties(data);

done:
    release_win_data(data);
}

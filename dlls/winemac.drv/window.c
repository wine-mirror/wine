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
        if (!data->shaped)
        {
            wf->title_bar = 1;
            if (style & WS_SYSMENU) wf->close_button = 1;
            if (style & WS_MINIMIZEBOX) wf->minimize_button = 1;
            if (style & WS_MAXIMIZEBOX) wf->resizable = 1;
            if (ex_style & WS_EX_TOOLWINDOW) wf->utility = 1;
        }
    }
    if (ex_style & WS_EX_DLGMODALFRAME) wf->shadow = 1;
    else if (style & WS_THICKFRAME)
    {
        wf->shadow = 1;
        if (!data->shaped) wf->resizable = 1;
    }
    else if ((style & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME) wf->shadow = 1;
}


/*******************************************************************
 *              can_activate_window
 *
 * Check if we can activate the specified window.
 */
static inline BOOL can_activate_window(HWND hwnd)
{
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    RECT rect;

    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    if (style & WS_MINIMIZE) return FALSE;
    if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_NOACTIVATE) return FALSE;
    if (hwnd == GetDesktopWindow()) return FALSE;
    if (GetWindowRect(hwnd, &rect) && IsRectEmpty(&rect)) return FALSE;
    return !(style & WS_DISABLED);
}


/***********************************************************************
 *              get_cocoa_window_state
 */
static void get_cocoa_window_state(struct macdrv_win_data *data,
                                   DWORD style, DWORD ex_style,
                                   struct macdrv_window_state* state)
{
    memset(state, 0, sizeof(*state));
    state->disabled = (style & WS_DISABLED) != 0;
    state->no_activate = !can_activate_window(data->hwnd);
    state->floating = (ex_style & WS_EX_TOPMOST) != 0;
    state->excluded_by_expose = state->excluded_by_cycle =
        !(ex_style & WS_EX_APPWINDOW) &&
        (GetWindow(data->hwnd, GW_OWNER) || (ex_style & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)));
}


/***********************************************************************
 *              get_mac_rect_offset
 *
 * Helper for macdrv_window_to_mac_rect and macdrv_mac_to_window_rect.
 */
static void get_mac_rect_offset(struct macdrv_win_data *data, DWORD style, RECT *rect)
{
    DWORD ex_style, style_mask = 0, ex_style_mask = 0;

    rect->top = rect->bottom = rect->left = rect->right = 0;

    ex_style = GetWindowLongW(data->hwnd, GWL_EXSTYLE);

    if (!data->shaped)
    {
        struct macdrv_window_features wf;
        get_cocoa_window_features(data, style, ex_style, &wf);

        if (wf.title_bar) style_mask |= WS_CAPTION;
        if (wf.shadow)
        {
            style_mask |= WS_DLGFRAME | WS_THICKFRAME;
            ex_style_mask |= WS_EX_DLGMODALFRAME;
        }
    }

    AdjustWindowRectEx(rect, style & style_mask, FALSE, ex_style & ex_style_mask);

    TRACE("%p/%p style %08x ex_style %08x shaped %d -> %s\n", data->hwnd, data->cocoa_window,
          style, ex_style, data->shaped, wine_dbgstr_rect(rect));
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
        data->color_key = CLR_INVALID;
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
    HWND owner;
    macdrv_window owner_win;
    struct macdrv_window_features wf;
    struct macdrv_window_state state;

    style = GetWindowLongW(data->hwnd, GWL_STYLE);
    ex_style = GetWindowLongW(data->hwnd, GWL_EXSTYLE);

    owner = GetWindow(data->hwnd, GW_OWNER);
    owner_win = macdrv_get_cocoa_window(owner);
    macdrv_set_cocoa_parent_window(data->cocoa_window, owner_win);

    get_cocoa_window_features(data, style, ex_style, &wf);
    macdrv_set_cocoa_window_features(data->cocoa_window, &wf);

    get_cocoa_window_state(data, style, ex_style, &state);
    macdrv_set_cocoa_window_state(data->cocoa_window, &state);
}


/***********************************************************************
 *              sync_window_region
 *
 * Update the window region.
 */
static void sync_window_region(struct macdrv_win_data *data, HRGN win_region)
{
    HRGN hrgn = win_region;
    RGNDATA *region_data;
    const CGRect* rects;
    int count;

    if (!data->cocoa_window) return;
    data->shaped = FALSE;

    if (hrgn == (HRGN)1)  /* hack: win_region == 1 means retrieve region from server */
    {
        if (!(hrgn = CreateRectRgn(0, 0, 0, 0))) return;
        if (GetWindowRgn(data->hwnd, hrgn) == ERROR)
        {
            DeleteObject(hrgn);
            hrgn = 0;
        }
    }

    if (hrgn && GetWindowLongW(data->hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
        MirrorRgn(data->hwnd, hrgn);
    if (hrgn)
    {
        OffsetRgn(hrgn, data->window_rect.left - data->whole_rect.left,
                  data->window_rect.top - data->whole_rect.top);
    }
    region_data = get_region_data(hrgn, 0);
    if (region_data)
    {
        rects = (CGRect*)region_data->Buffer;
        count = region_data->rdh.nCount;
        /* Special case optimization.  If the region entirely encloses the Cocoa
           window, it's the same as there being no region.  It's potentially
           hard/slow to test this for arbitrary regions, so we just check for
           very simple regions. */
        if (count == 1 && CGRectContainsRect(rects[0], cgrect_from_rect(data->whole_rect)))
        {
            TRACE("optimizing for simple region that contains Cocoa content rect\n");
            rects = NULL;
            count = 0;
        }
    }
    else
    {
        rects = NULL;
        count = 0;
    }

    TRACE("win %p/%p win_region %p rects %p count %d\n", data->hwnd, data->cocoa_window, win_region, rects, count);
    macdrv_set_window_shape(data->cocoa_window, rects, count);

    HeapFree(GetProcessHeap(), 0, region_data);
    data->shaped = (region_data != NULL);

    if (hrgn && hrgn != win_region) DeleteObject(hrgn);
}


/***********************************************************************
 *              add_bounds_rect
 */
static inline void add_bounds_rect(RECT *bounds, const RECT *rect)
{
    if (rect->left >= rect->right || rect->top >= rect->bottom) return;
    bounds->left   = min(bounds->left, rect->left);
    bounds->top    = min(bounds->top, rect->top);
    bounds->right  = max(bounds->right, rect->right);
    bounds->bottom = max(bounds->bottom, rect->bottom);
}


/***********************************************************************
 *              sync_window_opacity
 */
static void sync_window_opacity(struct macdrv_win_data *data, COLORREF key, BYTE alpha,
                                BOOL per_pixel_alpha, DWORD flags)
{
    CGFloat opacity = 1.0;
    BOOL needs_flush = FALSE;

    if (flags & LWA_ALPHA) opacity = alpha / 255.0;

    TRACE("setting window %p/%p alpha to %g\n", data->hwnd, data->cocoa_window, opacity);
    macdrv_set_window_alpha(data->cocoa_window, opacity);

    if (flags & LWA_COLORKEY)
    {
        /* FIXME: treat PALETTEINDEX and DIBINDEX as black */
        if ((key & (1 << 24)) || key >> 16 == 0x10ff)
            key = RGB(0, 0, 0);
    }
    else
        key = CLR_INVALID;

    if (data->color_key != key)
    {
        if (key == CLR_INVALID)
        {
            TRACE("clearing color-key for window %p/%p\n", data->hwnd, data->cocoa_window);
            macdrv_clear_window_color_key(data->cocoa_window);
        }
        else
        {
            TRACE("setting color-key for window %p/%p to RGB %d,%d,%d\n", data->hwnd, data->cocoa_window,
                  GetRValue(key), GetGValue(key), GetBValue(key));
            macdrv_set_window_color_key(data->cocoa_window, GetRValue(key), GetGValue(key), GetBValue(key));
        }

        data->color_key = key;
        needs_flush = TRUE;
    }

    if (!data->per_pixel_alpha != !per_pixel_alpha)
    {
        macdrv_window_use_per_pixel_alpha(data->cocoa_window, per_pixel_alpha);
        data->per_pixel_alpha = per_pixel_alpha;
        needs_flush = TRUE;
    }

    if (needs_flush && data->surface)
    {
        RECT *bounds;
        RECT rect;

        rect = data->whole_rect;
        OffsetRect(&rect, -data->whole_rect.left, -data->whole_rect.top);
        data->surface->funcs->lock(data->surface);
        bounds = data->surface->funcs->get_bounds(data->surface);
        add_bounds_rect(bounds, &rect);
        data->surface->funcs->unlock(data->surface);
    }
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
    HRGN win_rgn;
    COLORREF key;
    BYTE alpha;
    DWORD layered_flags;

    if ((win_rgn = CreateRectRgn(0, 0, 0, 0)) &&
        GetWindowRgn(data->hwnd, win_rgn) == ERROR)
    {
        DeleteObject(win_rgn);
        win_rgn = 0;
    }
    data->shaped = (win_rgn != 0);

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
    if (!data->cocoa_window) goto done;

    set_cocoa_window_properties(data);

    /* set the window text */
    if (!InternalGetWindowText(data->hwnd, text, sizeof(text)/sizeof(WCHAR))) text[0] = 0;
    macdrv_set_cocoa_window_title(data->cocoa_window, text, strlenW(text));

    /* set the window region */
    if (win_rgn) sync_window_region(data, win_rgn);

    /* set the window opacity */
    if (!GetLayeredWindowAttributes(data->hwnd, &key, &alpha, &layered_flags)) layered_flags = 0;
    sync_window_opacity(data, key, alpha, FALSE, layered_flags);

done:
    if (win_rgn) DeleteObject(win_rgn);
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
    data->color_key = CLR_INVALID;
    if (data->surface) window_surface_release(data->surface);
    data->surface = NULL;
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
 *              get_region_data
 *
 * Calls GetRegionData on the given region and converts the rectangle
 * array to CGRect format. The returned buffer must be freed by
 * caller using HeapFree(GetProcessHeap(),...).
 * If hdc_lptodp is not 0, the rectangles are converted through LPtoDP.
 */
RGNDATA *get_region_data(HRGN hrgn, HDC hdc_lptodp)
{
    RGNDATA *data;
    DWORD size;
    int i;
    RECT *rect;
    CGRect *cgrect;

    if (!hrgn || !(size = GetRegionData(hrgn, 0, NULL))) return NULL;
    if (sizeof(CGRect) > sizeof(RECT))
    {
        /* add extra size for CGRect array */
        int count = (size - sizeof(RGNDATAHEADER)) / sizeof(RECT);
        size += count * (sizeof(CGRect) - sizeof(RECT));
    }
    if (!(data = HeapAlloc(GetProcessHeap(), 0, size))) return NULL;
    if (!GetRegionData(hrgn, size, data))
    {
        HeapFree(GetProcessHeap(), 0, data);
        return NULL;
    }

    rect = (RECT *)data->Buffer;
    cgrect = (CGRect *)data->Buffer;
    if (hdc_lptodp)  /* map to device coordinates */
    {
        LPtoDP(hdc_lptodp, (POINT *)rect, data->rdh.nCount * 2);
        for (i = 0; i < data->rdh.nCount; i++)
        {
            if (rect[i].right < rect[i].left)
            {
                INT tmp = rect[i].right;
                rect[i].right = rect[i].left;
                rect[i].left = tmp;
            }
            if (rect[i].bottom < rect[i].top)
            {
                INT tmp = rect[i].bottom;
                rect[i].bottom = rect[i].top;
                rect[i].top = tmp;
            }
        }
    }

    if (sizeof(CGRect) > sizeof(RECT))
    {
        /* need to start from the end */
        for (i = data->rdh.nCount-1; i >= 0; i--)
            cgrect[i] = cgrect_from_rect(rect[i]);
    }
    else
    {
        for (i = 0; i < data->rdh.nCount; i++)
            cgrect[i] = cgrect_from_rect(rect[i]);
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
    if (data->shaped) sync_window_region(data, (HRGN)1);

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


/***********************************************************************
 *              SetLayeredWindowAttributes  (MACDRV.@)
 *
 * Set transparency attributes for a layered window.
 */
void CDECL macdrv_SetLayeredWindowAttributes(HWND hwnd, COLORREF key, BYTE alpha, DWORD flags)
{
    struct macdrv_win_data *data = get_win_data(hwnd);

    TRACE("hwnd %p key %#08x alpha %#02x flags %x\n", hwnd, key, alpha, flags);

    if (data)
    {
        data->layered = TRUE;
        if (data->cocoa_window)
        {
            sync_window_opacity(data, key, alpha, FALSE, flags);
            /* since layered attributes are now set, can now show the window */
            if ((GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE) && !data->on_screen)
                show_window(data);
        }
        release_win_data(data);
    }
    else
        FIXME("setting layered attributes on window %p of other process not supported\n", hwnd);
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
 *              SetWindowRgn  (MACDRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
int CDECL macdrv_SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL redraw)
{
    struct macdrv_win_data *data;

    TRACE("%p, %p, %d\n", hwnd, hrgn, redraw);

    if ((data = get_win_data(hwnd)))
    {
        sync_window_region(data, hrgn);
        release_win_data(data);
    }
    else
    {
        DWORD procid;

        GetWindowThreadProcessId(hwnd, &procid);
        if (procid != GetCurrentProcessId())
            SendMessageW(hwnd, WM_MACDRV_SET_WIN_REGION, 0, 0);
    }

    return TRUE;
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
    {
        DWORD changed = style->styleNew ^ style->styleOld;

        set_cocoa_window_properties(data);

        if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYERED)) /* changing WS_EX_LAYERED resets attributes */
        {
            data->layered = FALSE;
            sync_window_opacity(data, 0, 0, FALSE, 0);
            if (data->surface) set_surface_use_alpha(data->surface, FALSE);
        }
    }

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
 *              UpdateLayeredWindow   (MACDRV.@)
 */
BOOL CDECL macdrv_UpdateLayeredWindow(HWND hwnd, const UPDATELAYEREDWINDOWINFO *info,
                                      const RECT *window_rect)
{
    struct window_surface *surface;
    struct macdrv_win_data *data;
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, 0 };
    BYTE alpha;
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bmi = (BITMAPINFO *)buffer;
    void *src_bits, *dst_bits;
    RECT rect;
    HDC hdc = 0;
    HBITMAP dib;
    BOOL ret = FALSE;

    if (!(data = get_win_data(hwnd))) return FALSE;

    data->layered = TRUE;

    rect = *window_rect;
    OffsetRect(&rect, -window_rect->left, -window_rect->top);

    surface = data->surface;
    if (!surface || memcmp(&surface->rect, &rect, sizeof(RECT)))
    {
        data->surface = create_surface(data->cocoa_window, &rect, TRUE);
        set_window_surface(data->cocoa_window, data->surface);
        if (surface) window_surface_release(surface);
        surface = data->surface;
    }
    else set_surface_use_alpha(surface, TRUE);

    if (surface) window_surface_add_ref(surface);
    release_win_data(data);

    if (!surface) return FALSE;
    if (!info->hdcSrc)
    {
        window_surface_release(surface);
        return TRUE;
    }

    if (info->dwFlags & ULW_ALPHA)
    {
        /* Apply SourceConstantAlpha via window alpha, not blend. */
        alpha = info->pblend->SourceConstantAlpha;
        blend = *info->pblend;
        blend.SourceConstantAlpha = 0xff;
    }
    else
        alpha = 0xff;

    dst_bits = surface->funcs->get_info(surface, bmi);

    if (!(dib = CreateDIBSection(info->hdcDst, bmi, DIB_RGB_COLORS, &src_bits, NULL, 0))) goto done;
    if (!(hdc = CreateCompatibleDC(0))) goto done;

    SelectObject(hdc, dib);
    if (info->prcDirty)
    {
        IntersectRect(&rect, &rect, info->prcDirty);
        surface->funcs->lock(surface);
        memcpy(src_bits, dst_bits, bmi->bmiHeader.biSizeImage);
        surface->funcs->unlock(surface);
        PatBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, BLACKNESS);
    }
    if (!(ret = GdiAlphaBlend(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                              info->hdcSrc,
                              rect.left + (info->pptSrc ? info->pptSrc->x : 0),
                              rect.top + (info->pptSrc ? info->pptSrc->y : 0),
                              rect.right - rect.left, rect.bottom - rect.top,
                              blend)))
        goto done;

    if ((data = get_win_data(hwnd)))
    {
        if (surface == data->surface)
        {
            surface->funcs->lock(surface);
            memcpy(dst_bits, src_bits, bmi->bmiHeader.biSizeImage);
            add_bounds_rect(surface->funcs->get_bounds(surface), &rect);
            surface->funcs->unlock(surface);
            surface->funcs->flush(surface);
        }

        /* The ULW flags are a superset of the LWA flags. */
        sync_window_opacity(data, info->crKey, alpha, TRUE, info->dwFlags);

        release_win_data(data);
    }

done:
    window_surface_release(surface);
    if (hdc) DeleteDC(hdc);
    if (dib) DeleteObject(dib);
    return ret;
}


/**********************************************************************
 *              WindowMessage   (MACDRV.@)
 */
LRESULT CDECL macdrv_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    struct macdrv_win_data *data;

    TRACE("%p, %u, %u, %lu\n", hwnd, msg, (unsigned)wp, lp);

    switch(msg)
    {
    case WM_MACDRV_SET_WIN_REGION:
        if ((data = get_win_data(hwnd)))
        {
            sync_window_region(data, (HRGN)1);
            release_win_data(data);
        }
        return 0;
    }

    FIXME("unrecognized window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, wp, lp);
    return 0;
}


static inline RECT get_surface_rect(const RECT *visible_rect)
{
    RECT rect;
    RECT desktop_rect = rect_from_cgrect(macdrv_get_desktop_rect());

    IntersectRect(&rect, visible_rect, &desktop_rect);
    OffsetRect(&rect, -visible_rect->left, -visible_rect->top);
    rect.left &= ~127;
    rect.top  &= ~127;
    rect.right  = max(rect.left + 128, (rect.right + 127) & ~127);
    rect.bottom = max(rect.top + 128, (rect.bottom + 127) & ~127);
    return rect;
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
    RECT surface_rect;

    TRACE("%p after %p swp %04x window %s client %s visible %s surface %p\n", hwnd, insert_after,
          swp_flags, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
          wine_dbgstr_rect(visible_rect), surface);

    if (!data && !(data = macdrv_create_win_data(hwnd, window_rect, client_rect))) return;

    *visible_rect = *window_rect;
    macdrv_window_to_mac_rect(data, style, visible_rect);
    TRACE("visible_rect %s -> %s\n", wine_dbgstr_rect(window_rect),
          wine_dbgstr_rect(visible_rect));

    /* create the window surface if necessary */
    if (!data->cocoa_window) goto done;
    if (swp_flags & SWP_HIDEWINDOW) goto done;

    if (*surface) window_surface_release(*surface);
    *surface = NULL;

    surface_rect = get_surface_rect(visible_rect);
    if (data->surface)
    {
        if (!memcmp(&data->surface->rect, &surface_rect, sizeof(surface_rect)))
        {
            /* existing surface is good enough */
            window_surface_add_ref(data->surface);
            *surface = data->surface;
            goto done;
        }
    }
    else if (!(swp_flags & SWP_SHOWWINDOW) && !(style & WS_VISIBLE)) goto done;

    *surface = create_surface(data->cocoa_window, &surface_rect, FALSE);

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
    if (surface)
        window_surface_add_ref(surface);
    set_window_surface(data->cocoa_window, surface);
    if (data->surface) window_surface_release(data->surface);
    data->surface = surface;

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

        /* layered windows are not shown until their attributes are set */
        if (!data->on_screen &&
            (data->layered || !(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED)))
            show_window(data);
    }

    sync_window_position(data, swp_flags);
    set_cocoa_window_properties(data);

done:
    release_win_data(data);
}

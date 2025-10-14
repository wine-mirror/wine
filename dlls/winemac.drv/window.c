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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <IOKit/pwr_mgt/IOPMLib.h>
#define GetCurrentThread Mac_GetCurrentThread
#define LoadResource Mac_LoadResource
#include <CoreServices/CoreServices.h>
#undef GetCurrentThread
#undef LoadResource

#include "macdrv.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(macdrv);


static pthread_mutex_t win_data_mutex;
static CFMutableDictionaryRef win_datas;

static unsigned int activate_on_focus_time;


/* per-monitor DPI aware NtUserSetWindowPos call */
static BOOL set_window_pos(HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags)
{
    UINT context = NtUserSetThreadDpiAwarenessContext(NTUSER_DPI_PER_MONITOR_AWARE_V2);
    BOOL ret = NtUserSetWindowPos(hwnd, after, x, y, cx, cy, flags);
    NtUserSetThreadDpiAwarenessContext(context);
    return ret;
}


static struct macdrv_window_features get_window_features_for_style(DWORD style, DWORD ex_style, BOOL shaped)
{
    struct macdrv_window_features wf = {0};

    if (ex_style & WS_EX_NOACTIVATE) wf.prevents_app_activation = TRUE;

    if ((style & WS_CAPTION) == WS_CAPTION && !(ex_style & WS_EX_LAYERED))
    {
        wf.shadow = TRUE;
        if (!shaped)
        {
            wf.title_bar = TRUE;
            if (style & WS_SYSMENU) wf.close_button = TRUE;
            if (style & WS_MINIMIZEBOX) wf.minimize_button = TRUE;
            if (style & WS_MAXIMIZEBOX) wf.maximize_button = TRUE;
            if (ex_style & WS_EX_TOOLWINDOW) wf.utility = TRUE;
        }
    }
    if (style & WS_THICKFRAME)
    {
        wf.shadow = TRUE;
        if (!shaped) wf.resizable = TRUE;
    }
    else if (ex_style & WS_EX_DLGMODALFRAME) wf.shadow = TRUE;
    else if ((style & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME) wf.shadow = TRUE;

    return wf;
}

/***********************************************************************
 *              get_cocoa_window_features
 */
static struct macdrv_window_features get_cocoa_window_features(struct macdrv_win_data *data, DWORD style, DWORD ex_style)
{
    struct macdrv_window_features wf = {0};

    if (ex_style & WS_EX_NOACTIVATE) wf.prevents_app_activation = TRUE;
    if (EqualRect(&data->rects.window, &data->rects.visible)) return wf;

    return get_window_features_for_style(style, ex_style, data->shaped);
}


/*******************************************************************
 *              can_window_become_foreground
 *
 * Check if the specified window can become the foreground/key
 * window.
 */
static inline BOOL can_window_become_foreground(HWND hwnd)
{
    LONG style = NtUserGetWindowLongW(hwnd, GWL_STYLE);

    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    if (hwnd == NtUserGetDesktopWindow()) return FALSE;
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
    state->no_foreground = !can_window_become_foreground(data->hwnd);
    state->floating = (ex_style & WS_EX_TOPMOST) != 0;
    state->excluded_by_expose = state->excluded_by_cycle =
        (!(ex_style & WS_EX_APPWINDOW) &&
         (NtUserGetWindowRelative(data->hwnd, GW_OWNER) || (ex_style & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE))));
    if (IsRectEmpty(&data->rects.window))
        state->excluded_by_expose = TRUE;
    state->minimized = (style & WS_MINIMIZE) != 0;
    state->minimized_valid = state->minimized != data->minimized;
    state->maximized = (style & WS_MAXIMIZE) != 0;
}


/***********************************************************************
 *              constrain_window_frame
 *
 * Alter a window frame rectangle to fit within a) Cocoa's documented
 * limits, and b) sane sizes, like twice the desktop rect.
 */
static void constrain_window_frame(CGPoint* origin, CGSize* size)
{
    CGRect desktop_rect = macdrv_get_desktop_rect();
    int max_width, max_height;

    max_width = min(32000, 2 * CGRectGetWidth(desktop_rect));
    max_height = min(32000, 2 * CGRectGetHeight(desktop_rect));

    if (origin)
    {
        if (origin->x < -16000) origin->x = -16000;
        if (origin->y < -16000) origin->y = -16000;
        if (origin->x > 16000) origin->x = 16000;
        if (origin->y > 16000) origin->y = 16000;
    }
    if (size)
    {
        if (size->width > max_width) size->width = max_width;
        if (size->height > max_height) size->height = max_height;
    }
}


/***********************************************************************
 *              alloc_win_data
 */
static struct macdrv_win_data *alloc_win_data(HWND hwnd)
{
    struct macdrv_win_data *data;

    if ((data = calloc(1, sizeof(*data))))
    {
        data->hwnd = hwnd;
        pthread_mutex_lock(&win_data_mutex);
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
struct macdrv_win_data *get_win_data(HWND hwnd)
{
    struct macdrv_win_data *data;

    if (!hwnd) return NULL;
    pthread_mutex_lock(&win_data_mutex);
    if (win_datas && (data = (struct macdrv_win_data*)CFDictionaryGetValue(win_datas, hwnd)))
        return data;
    pthread_mutex_unlock(&win_data_mutex);
    return NULL;
}


/***********************************************************************
 *              release_win_data
 *
 * Release the data returned by get_win_data.
 */
void release_win_data(struct macdrv_win_data *data)
{
    if (data) pthread_mutex_unlock(&win_data_mutex);
}


/***********************************************************************
 *              macdrv_get_cocoa_window
 *
 * Return the Mac window associated with the full area of a window
 */
macdrv_window macdrv_get_cocoa_window(HWND hwnd, BOOL require_on_screen)
{
    struct macdrv_win_data *data = get_win_data(hwnd);
    macdrv_window ret = NULL;
    if (data && (data->on_screen || !require_on_screen))
        ret = data->cocoa_window;
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

    style = NtUserGetWindowLongW(data->hwnd, GWL_STYLE);
    ex_style = NtUserGetWindowLongW(data->hwnd, GWL_EXSTYLE);

    owner = NtUserGetWindowRelative(data->hwnd, GW_OWNER);
    if (owner)
        owner = NtUserGetAncestor(owner, GA_ROOT);
    owner_win = macdrv_get_cocoa_window(owner, TRUE);
    macdrv_set_cocoa_parent_window(data->cocoa_window, owner_win);

    wf = get_cocoa_window_features(data, style, ex_style);
    macdrv_set_cocoa_window_features(data->cocoa_window, &wf);

    get_cocoa_window_state(data, style, ex_style, &state);
    macdrv_set_cocoa_window_state(data->cocoa_window, &state);
    if (state.minimized_valid)
        data->minimized = state.minimized;
}


/***********************************************************************
 *              sync_window_region
 *
 * Update the window region.
 */
static void sync_window_region(struct macdrv_win_data *data, HRGN win_region)
{
    if (!data->cocoa_window) return;

    if (IsRectEmpty(&data->rects.window))  /* set an empty shape */
    {
        data->shaped = FALSE;
        TRACE("win %p/%p setting empty shape for zero-sized window\n", data->hwnd, data->cocoa_window);
        macdrv_set_window_shape(data->cocoa_window, &CGRectZero, 1);
        return;
    }

    /* use surface shape instead */
    macdrv_set_window_shape(data->cocoa_window, NULL, 0);
}


/***********************************************************************
 *              sync_window_opacity
 */
static void sync_window_opacity(struct macdrv_win_data *data, BYTE alpha,
                                BOOL per_pixel_alpha, DWORD flags)
{
    CGFloat opacity = 1.0;

    if (flags & LWA_ALPHA) opacity = alpha / 255.0;

    TRACE("setting window %p/%p alpha to %g\n", data->hwnd, data->cocoa_window, opacity);
    macdrv_set_window_alpha(data->cocoa_window, opacity);

    if (!data->per_pixel_alpha != !per_pixel_alpha)
    {
        TRACE("setting window %p/%p per-pixel-alpha to %d\n", data->hwnd, data->cocoa_window, per_pixel_alpha);
        macdrv_window_use_per_pixel_alpha(data->cocoa_window, per_pixel_alpha);
        data->per_pixel_alpha = per_pixel_alpha;
    }
}


/***********************************************************************
 *              sync_window_min_max_info
 */
static void sync_window_min_max_info(HWND hwnd)
{
    LONG style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    LONG exstyle = NtUserGetWindowLongW(hwnd, GWL_EXSTYLE);
    UINT dpi = NtUserGetWinMonitorDpi(hwnd, MDT_RAW_DPI);
    RECT win_rect, primary_monitor_rect;
    MINMAXINFO minmax;
    LONG adjustedStyle;
    INT xinc, yinc;
    WINDOWPLACEMENT wpl;
    HMONITOR monitor;
    struct macdrv_win_data *data;
    BOOL menu;

    TRACE("win %p\n", hwnd);

    if (!macdrv_get_cocoa_window(hwnd, FALSE)) return;

    NtUserGetWindowRect(hwnd, &win_rect, dpi);
    minmax.ptReserved.x = win_rect.left;
    minmax.ptReserved.y = win_rect.top;

    if ((style & WS_CAPTION) == WS_CAPTION)
        adjustedStyle = style & ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
    else
        adjustedStyle = style;

    primary_monitor_rect.left = primary_monitor_rect.top = 0;
    primary_monitor_rect.right = NtUserGetSystemMetrics(SM_CXSCREEN);
    primary_monitor_rect.bottom = NtUserGetSystemMetrics(SM_CYSCREEN);
    menu = ((style & WS_POPUP) && NtUserGetWindowLongPtrW(hwnd, GWLP_ID));
    NtUserAdjustWindowRect(&primary_monitor_rect, adjustedStyle, menu, exstyle, dpi);

    xinc = -primary_monitor_rect.left;
    yinc = -primary_monitor_rect.top;

    minmax.ptMaxSize.x = primary_monitor_rect.right - primary_monitor_rect.left;
    minmax.ptMaxSize.y = primary_monitor_rect.bottom - primary_monitor_rect.top;
    minmax.ptMaxPosition.x = -xinc;
    minmax.ptMaxPosition.y = -yinc;
    if (style & (WS_DLGFRAME | WS_BORDER))
    {
        minmax.ptMinTrackSize.x = NtUserGetSystemMetrics(SM_CXMINTRACK);
        minmax.ptMinTrackSize.y = NtUserGetSystemMetrics(SM_CYMINTRACK);
    }
    else
    {
        minmax.ptMinTrackSize.x = 2 * xinc;
        minmax.ptMinTrackSize.y = 2 * yinc;
    }
    minmax.ptMaxTrackSize.x = NtUserGetSystemMetrics(SM_CXMAXTRACK);
    minmax.ptMaxTrackSize.y = NtUserGetSystemMetrics(SM_CYMAXTRACK);

    wpl.length = sizeof(wpl);
    if (NtUserGetWindowPlacement(hwnd, &wpl) && (wpl.ptMaxPosition.x != -1 || wpl.ptMaxPosition.y != -1))
    {
        minmax.ptMaxPosition = wpl.ptMaxPosition;

        /* Convert from GetWindowPlacement's workspace coordinates to screen coordinates. */
        minmax.ptMaxPosition.x -= wpl.rcNormalPosition.left - win_rect.left;
        minmax.ptMaxPosition.y -= wpl.rcNormalPosition.top - win_rect.top;
    }

    TRACE("initial ptMaxSize %s ptMaxPosition %s ptMinTrackSize %s ptMaxTrackSize %s\n", wine_dbgstr_point(&minmax.ptMaxSize),
          wine_dbgstr_point(&minmax.ptMaxPosition), wine_dbgstr_point(&minmax.ptMinTrackSize), wine_dbgstr_point(&minmax.ptMaxTrackSize));

    send_message(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&minmax);

    TRACE("app's ptMaxSize %s ptMaxPosition %s ptMinTrackSize %s ptMaxTrackSize %s\n", wine_dbgstr_point(&minmax.ptMaxSize),
          wine_dbgstr_point(&minmax.ptMaxPosition), wine_dbgstr_point(&minmax.ptMinTrackSize), wine_dbgstr_point(&minmax.ptMaxTrackSize));

    /* if the app didn't change the values, adapt them for the window's monitor */
    if ((monitor = NtUserMonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY)))
    {
        MONITORINFO mon_info;
        RECT monitor_rect;

        mon_info.cbSize = sizeof(mon_info);
        NtUserGetMonitorInfo(monitor, &mon_info);

        if ((style & WS_MAXIMIZEBOX) && ((style & WS_CAPTION) == WS_CAPTION || !(style & WS_POPUP)))
            monitor_rect = mon_info.rcWork;
        else
            monitor_rect = mon_info.rcMonitor;

        if (minmax.ptMaxSize.x == primary_monitor_rect.right - primary_monitor_rect.left &&
            minmax.ptMaxSize.y == primary_monitor_rect.bottom - primary_monitor_rect.top)
        {
            minmax.ptMaxSize.x = (monitor_rect.right - monitor_rect.left) + 2 * xinc;
            minmax.ptMaxSize.y = (monitor_rect.bottom - monitor_rect.top) + 2 * yinc;
        }
        if (minmax.ptMaxPosition.x == -xinc && minmax.ptMaxPosition.y == -yinc)
        {
            minmax.ptMaxPosition.x = monitor_rect.left - xinc;
            minmax.ptMaxPosition.y = monitor_rect.top - yinc;
        }
    }

    minmax.ptMaxTrackSize.x = max(minmax.ptMaxTrackSize.x, minmax.ptMinTrackSize.x);
    minmax.ptMaxTrackSize.y = max(minmax.ptMaxTrackSize.y, minmax.ptMinTrackSize.y);

    TRACE("adjusted ptMaxSize %s ptMaxPosition %s ptMinTrackSize %s ptMaxTrackSize %s\n", wine_dbgstr_point(&minmax.ptMaxSize),
          wine_dbgstr_point(&minmax.ptMaxPosition), wine_dbgstr_point(&minmax.ptMinTrackSize), wine_dbgstr_point(&minmax.ptMaxTrackSize));

    if ((data = get_win_data(hwnd)) && data->cocoa_window)
    {
        RECT min_rect, max_rect;
        CGSize min_size, max_size;

        SetRect(&min_rect, 0, 0, minmax.ptMinTrackSize.x, minmax.ptMinTrackSize.y);
        min_rect = visible_rect_from_window(&data->rects, min_rect);
        min_size = CGSizeMake(min_rect.right - min_rect.left, min_rect.bottom - min_rect.top);

        if (minmax.ptMaxTrackSize.x == NtUserGetSystemMetrics(SM_CXMAXTRACK) &&
            minmax.ptMaxTrackSize.y == NtUserGetSystemMetrics(SM_CYMAXTRACK))
            max_size = CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX);
        else
        {
            SetRect(&max_rect, 0, 0, minmax.ptMaxTrackSize.x, minmax.ptMaxTrackSize.y);
            max_rect = visible_rect_from_window(&data->rects, max_rect);
            max_size = CGSizeMake(max_rect.right - max_rect.left, max_rect.bottom - max_rect.top);
        }

        constrain_window_frame(NULL, &max_size);

        TRACE("min_size (%g,%g) max_size (%g,%g)\n", min_size.width, min_size.height, max_size.width, max_size.height);
        macdrv_set_window_min_max_sizes(data->cocoa_window, min_size, max_size);
    }

    release_win_data(data);
}


/**********************************************************************
 *              create_cocoa_window
 *
 * Create the whole Mac window for a given window
 */
static void create_cocoa_window(struct macdrv_win_data *data)
{
    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();
    WCHAR text[1024];
    struct macdrv_window_features wf;
    CGRect frame;
    DWORD style, ex_style;
    HRGN win_rgn;
    COLORREF key;
    BYTE alpha;
    DWORD layered_flags;

    if ((win_rgn = NtGdiCreateRectRgn(0, 0, 0, 0)) &&
        NtUserGetWindowRgnEx(data->hwnd, win_rgn, 0) == ERROR)
    {
        NtGdiDeleteObjectApp(win_rgn);
        win_rgn = 0;
    }
    data->shaped = (win_rgn != 0);

    style = NtUserGetWindowLongW(data->hwnd, GWL_STYLE);
    ex_style = NtUserGetWindowLongW(data->hwnd, GWL_EXSTYLE);

    wf = get_cocoa_window_features(data, style, ex_style);

    frame = cgrect_from_rect(data->rects.visible);
    constrain_window_frame(&frame.origin, &frame.size);
    if (frame.size.width < 1 || frame.size.height < 1)
        frame.size.width = frame.size.height = 1;

    TRACE("creating %p window %s whole %s client %s\n", data->hwnd, wine_dbgstr_rect(&data->rects.window),
          wine_dbgstr_rect(&data->rects.visible), wine_dbgstr_rect(&data->rects.client));

    data->cocoa_window = macdrv_create_cocoa_window(&wf, frame, data->hwnd, thread_data->queue);
    if (!data->cocoa_window) goto done;

    set_cocoa_window_properties(data);

    /* set the window text */
    if (!NtUserInternalGetWindowText(data->hwnd, text, ARRAY_SIZE(text))) text[0] = 0;
    macdrv_set_cocoa_window_title(data->cocoa_window, text, wcslen(text));

    /* set the window region */
    if (win_rgn || IsRectEmpty(&data->rects.window)) sync_window_region(data, win_rgn);

    /* set the window opacity */
    if (!NtUserGetLayeredWindowAttributes(data->hwnd, &key, &alpha, &layered_flags)) layered_flags = 0;
    sync_window_opacity(data, alpha, FALSE, layered_flags);

done:
    if (win_rgn) NtGdiDeleteObjectApp(win_rgn);
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
static struct macdrv_win_data *macdrv_create_win_data(HWND hwnd, const struct window_rects *rects)
{
    struct macdrv_win_data *data;
    HWND parent;

    if (NtUserGetWindowThread(hwnd, NULL) != GetCurrentThreadId()) return NULL;

    if (!(parent = NtUserGetAncestor(hwnd, GA_PARENT)))  /* desktop */
    {
        macdrv_init_thread_data();
        return NULL;
    }

    /* don't create win data for HWND_MESSAGE windows */
    if (parent != NtUserGetDesktopWindow() && !NtUserGetAncestor(parent, GA_PARENT)) return NULL;

    if (!(data = alloc_win_data(hwnd))) return NULL;
    data->rects = *rects;

    if (parent == NtUserGetDesktopWindow())
    {
        create_cocoa_window(data);
        TRACE("win %p/%p window %s whole %s client %s\n",
               hwnd, data->cocoa_window, wine_dbgstr_rect(&data->rects.window),
               wine_dbgstr_rect(&data->rects.visible), wine_dbgstr_rect(&data->rects.client));
    }

    TRACE("win %p/%p window %s whole %s client %s\n",
           hwnd, data->cocoa_window, wine_dbgstr_rect(&data->rects.window),
           wine_dbgstr_rect(&data->rects.visible), wine_dbgstr_rect(&data->rects.client));
    return data;
}


/**********************************************************************
 *              is_owned_by
 */
static BOOL is_owned_by(HWND hwnd, HWND maybe_owner)
{
    while (1)
    {
        HWND hwnd2 = NtUserGetWindowRelative(hwnd, GW_OWNER);
        if (!hwnd2)
            hwnd2 = NtUserGetAncestor(hwnd, GA_ROOT);
        if (!hwnd2 || hwnd2 == hwnd)
            break;
        if (hwnd2 == maybe_owner)
            return TRUE;
        hwnd = hwnd2;
    }

    return FALSE;
}


/**********************************************************************
 *              is_all_the_way_front
 */
static BOOL is_all_the_way_front(HWND hwnd)
{
    BOOL topmost = (NtUserGetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    HWND prev = hwnd;

    while ((prev = NtUserGetWindowRelative(prev, GW_HWNDPREV)))
    {
        if (!topmost && (NtUserGetWindowLongW(prev, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0)
            return TRUE;
        if (!is_owned_by(prev, hwnd))
            return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *              set_focus
 */
static void set_focus(HWND hwnd, BOOL raise)
{
    struct macdrv_win_data *data;

    if (!(hwnd = NtUserGetAncestor(hwnd, GA_ROOT))) return;

    if (raise && hwnd == NtUserGetForegroundWindow() && hwnd != NtUserGetDesktopWindow() && !is_all_the_way_front(hwnd))
        NtUserSetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

    if (!(data = get_win_data(hwnd))) return;

    if (data->cocoa_window && data->on_screen)
    {
        BOOL activate = activate_on_focus_time && (NtGetTickCount() - activate_on_focus_time < 2000);
        /* Set Mac focus */
        macdrv_give_cocoa_window_focus(data->cocoa_window, activate);
        activate_on_focus_time = 0;
    }

    release_win_data(data);
}

/***********************************************************************
 *              show_window
 */
static void show_window(struct macdrv_win_data *data)
{
    HWND prev = NULL;
    HWND next = NULL;
    macdrv_window prev_window = NULL;
    macdrv_window next_window = NULL;
    BOOL activate = FALSE;
    GUITHREADINFO info;

    /* find window that this one must be after */
    prev = NtUserGetWindowRelative(data->hwnd, GW_HWNDPREV);
    while (prev && !((NtUserGetWindowLongW(prev, GWL_STYLE) & (WS_VISIBLE | WS_MINIMIZE)) == WS_VISIBLE &&
                     (prev_window = macdrv_get_cocoa_window(prev, TRUE))))
        prev = NtUserGetWindowRelative(prev, GW_HWNDPREV);
    if (!prev_window)
    {
        /* find window that this one must be before */
        next = NtUserGetWindowRelative(data->hwnd, GW_HWNDNEXT);
        while (next && !((NtUserGetWindowLongW(next, GWL_STYLE) & (WS_VISIBLE | WS_MINIMIZE)) == WS_VISIBLE &&
                         (next_window = macdrv_get_cocoa_window(next, TRUE))))
            next = NtUserGetWindowRelative(next, GW_HWNDNEXT);
    }

    TRACE("win %p/%p below %p/%p above %p/%p\n",
          data->hwnd, data->cocoa_window, prev, prev_window, next, next_window);

    if (!prev_window)
        activate = activate_on_focus_time && (NtGetTickCount() - activate_on_focus_time < 2000);
    macdrv_order_cocoa_window(data->cocoa_window, prev_window, next_window, activate);
    data->on_screen = TRUE;

    info.cbSize = sizeof(info);
    if (NtUserGetGUIThreadInfo(NtUserGetWindowThread(data->hwnd, NULL), &info) && info.hwndFocus &&
        (data->hwnd == info.hwndFocus || NtUserIsChild(data->hwnd, info.hwndFocus)))
        set_focus(info.hwndFocus, FALSE);
    if (activate)
        activate_on_focus_time = 0;
}


/***********************************************************************
 *              hide_window
 */
static void hide_window(struct macdrv_win_data *data)
{
    TRACE("win %p/%p\n", data->hwnd, data->cocoa_window);

    if (data->cocoa_window)
        macdrv_hide_cocoa_window(data->cocoa_window);
    data->on_screen = FALSE;
}


/***********************************************************************
 *              get_region_data
 *
 * Calls GetRegionData on the given region and converts the rectangle
 * array to CGRect format. The returned buffer must be freed by caller.
 * If hdc_lptodp is not 0, the rectangles are converted through LPtoDP.
 */
RGNDATA *get_region_data(HRGN hrgn, HDC hdc_lptodp)
{
    RGNDATA *data;
    DWORD size;
    int i;
    RECT *rect;
    CGRect *cgrect;

    if (!hrgn || !(size = NtGdiGetRegionData(hrgn, 0, NULL))) return NULL;
    if (sizeof(CGRect) > sizeof(RECT))
    {
        /* add extra size for CGRect array */
        int count = (size - sizeof(RGNDATAHEADER)) / sizeof(RECT);
        size += count * (sizeof(CGRect) - sizeof(RECT));
    }
    if (!(data = malloc(size))) return NULL;
    if (!NtGdiGetRegionData(hrgn, size, data))
    {
        free(data);
        return NULL;
    }

    rect = (RECT *)data->Buffer;
    cgrect = (CGRect *)data->Buffer;
    if (hdc_lptodp)  /* map to device coordinates */
    {
        NtGdiTransformPoints(hdc_lptodp, (POINT *)rect, (POINT *)rect,
                             data->rdh.nCount * 2, NtGdiLPtoDP);
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
static void sync_window_position(struct macdrv_win_data *data, UINT swp_flags, const struct window_rects *old_rects)
{
    CGRect frame = cgrect_from_rect(data->rects.visible);

    if (data->cocoa_window)
    {
        if (data->minimized) return;

        constrain_window_frame(&frame.origin, &frame.size);
        if (frame.size.width < 1 || frame.size.height < 1)
            frame.size.width = frame.size.height = 1;

        macdrv_set_cocoa_window_frame(data->cocoa_window, &frame);
    }

    if (old_rects &&
        (IsRectEmpty(&old_rects->window) != IsRectEmpty(&data->rects.window) ||
         old_rects->window.left - old_rects->visible.left != data->rects.window.left - data->rects.visible.left ||
         old_rects->window.top - old_rects->visible.top != data->rects.window.top - data->rects.visible.top))
        sync_window_region(data, (HRGN)1);

    TRACE("win %p/%p whole_rect %s frame %s\n", data->hwnd, data->cocoa_window,
          wine_dbgstr_rect(&data->rects.visible), wine_dbgstr_cgrect(frame));

    if (data->cocoa_window && data->on_screen && (!(swp_flags & SWP_NOZORDER) || (swp_flags & SWP_SHOWWINDOW)))
        show_window(data);
}


/**********************************************************************
 *              activate_on_following_focus
 */
void activate_on_following_focus(void)
{
    activate_on_focus_time = NtGetTickCount();
    if (!activate_on_focus_time) activate_on_focus_time = 1;
}


/***********************************************************************
 *              set_app_icon
 */
static void set_app_icon(void)
{
    CFArrayRef images = create_app_icon_images();
    if (images)
    {
        macdrv_set_application_icon(images);
        CFRelease(images);
    }
}


/**********************************************************************
 *		        set_capture_window_for_move
 */
static BOOL set_capture_window_for_move(HWND hwnd)
{
    HWND previous = 0;
    BOOL ret;

    SERVER_START_REQ(set_capture_window)
    {
        req->handle = wine_server_user_handle(hwnd);
        req->flags  = CAPTURE_MOVESIZE;
        if ((ret = !wine_server_call_err(req)))
        {
            previous = wine_server_ptr_handle(reply->previous);
            hwnd = wine_server_ptr_handle(reply->full_handle);
        }
    }
    SERVER_END_REQ;

    if (ret)
    {
        macdrv_SetCapture(hwnd, GUI_INMOVESIZE);

        if (previous && previous != hwnd)
            send_message(previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd);
    }
    return ret;
}


static HMONITOR monitor_from_point(POINT pt, UINT flags)
{
    RECT rect;

    SetRect(&rect, pt.x, pt.y, pt.x + 1, pt.y + 1);
    return NtUserMonitorFromRect(&rect, flags);
}


/***********************************************************************
 *              move_window
 *
 * Based on user32's WINPOS_SysCommandSizeMove() specialized just for
 * moving top-level windows and enforcing Mac-style constraints like
 * keeping the top of the window within the work area.
 */
static LRESULT move_window(HWND hwnd, WPARAM wparam)
{
    UINT dpi = NtUserGetWinMonitorDpi(hwnd, MDT_DEFAULT);
    MSG msg;
    RECT origRect, movedRect, desktopRect;
    int hittest = (int)(wparam & 0x0f);
    POINT capturePoint;
    LONG style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    BOOL moved = FALSE;
    DWORD dwPoint = NtUserGetThreadInfo()->message_pos;
    INT captionHeight;
    HMONITOR mon = 0;
    MONITORINFO info;

    if ((style & (WS_MINIMIZE | WS_MAXIMIZE)) || !IsWindowVisible(hwnd)) return -1;
    if (hittest && hittest != HTCAPTION) return -1;

    capturePoint.x = (short)LOWORD(dwPoint);
    capturePoint.y = (short)HIWORD(dwPoint);
    NtUserClipCursor(NULL);

    TRACE("hwnd %p hittest %d, pos %d,%d\n", hwnd, hittest, capturePoint.x, capturePoint.y);

    origRect.left = origRect.right = origRect.top = origRect.bottom = 0;
    if (NtUserAdjustWindowRect(&origRect, style, FALSE, NtUserGetWindowLongW(hwnd, GWL_EXSTYLE), dpi))
        captionHeight = -origRect.top;
    else
        captionHeight = 0;

    NtUserGetWindowRect(hwnd, &origRect, NtUserGetWinMonitorDpi(hwnd, MDT_DEFAULT));
    movedRect = origRect;

    if (!hittest)
    {
        /* Move pointer to the center of the caption */
        RECT rect = origRect;

        /* Note: to be exactly centered we should take the different types
         * of border into account, but it shouldn't make more than a few pixels
         * of difference so let's not bother with that */
        rect.top += NtUserGetSystemMetrics(SM_CYBORDER);
        if (style & WS_SYSMENU)
            rect.left += NtUserGetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MINIMIZEBOX)
            rect.right -= NtUserGetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MAXIMIZEBOX)
            rect.right -= NtUserGetSystemMetrics(SM_CXSIZE) + 1;
        capturePoint.x = (rect.right + rect.left) / 2;
        capturePoint.y = rect.top + NtUserGetSystemMetrics(SM_CYSIZE)/2;

        NtUserSetCursorPos(capturePoint.x, capturePoint.y);
        send_message(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG(HTCAPTION, WM_MOUSEMOVE));
    }

    desktopRect = rect_from_cgrect(macdrv_get_desktop_rect());
    mon = monitor_from_point(capturePoint, MONITOR_DEFAULTTONEAREST);
    info.cbSize = sizeof(info);
    if (mon && !NtUserGetMonitorInfo(mon, &info))
        mon = 0;

    /* repaint the window before moving it around */
    NtUserRedrawWindow(hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN);

    send_message(hwnd, WM_ENTERSIZEMOVE, 0, 0);
    set_capture_window_for_move(hwnd);

    while(1)
    {
        POINT pt;
        int dx = 0, dy = 0;
        HMONITOR newmon;

        if (!NtUserGetMessage(&msg, 0, 0, 0)) break;
        if (NtUserCallMsgFilter(&msg, MSGF_SIZE)) continue;

        /* Exit on button-up, Return, or Esc */
        if (msg.message == WM_LBUTTONUP ||
            (msg.message == WM_KEYDOWN && (msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE)))
            break;

        if (msg.message != WM_KEYDOWN && msg.message != WM_MOUSEMOVE)
        {
            NtUserTranslateMessage(&msg, 0);
            NtUserDispatchMessage(&msg);
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

        pt.x = max(pt.x, desktopRect.left);
        pt.x = min(pt.x, desktopRect.right - 1);
        pt.y = max(pt.y, desktopRect.top);
        pt.y = min(pt.y, desktopRect.bottom - 1);

        if ((newmon = monitor_from_point(pt, MONITOR_DEFAULTTONULL)) && newmon != mon)
        {
            if (NtUserGetMonitorInfo(newmon, &info))
                mon = newmon;
            else
                mon = 0;
        }

        if (mon)
        {
            /* wineserver clips the cursor position to the virtual desktop rect but,
               if the display configuration is non-rectangular, that could still
               leave the logical cursor position outside of any display.  The window
               could keep moving as you push the cursor against a display edge, even
               though the visible cursor doesn't keep moving. The following keeps
               the window movement in sync with the visible cursor. */
            pt.x = max(pt.x, info.rcMonitor.left);
            pt.x = min(pt.x, info.rcMonitor.right - 1);
            pt.y = max(pt.y, info.rcMonitor.top);
            pt.y = min(pt.y, info.rcMonitor.bottom - 1);

            /* Assuming that dx will be calculated below as pt.x - capturePoint.x,
               dy will be pt.y - capturePoint.y, and movedRect will be offset by those,
               we want to enforce these constraints:
                    movedRect.left + dx < info.rcWork.right
                    movedRect.right + dx > info.rcWork.left
                    movedRect.top + captionHeight + dy < info.rcWork.bottom
                    movedRect.bottom + dy > info.rcWork.top
                    movedRect.top + dy >= info.rcWork.top
               The first four keep at least one edge barely in the work area.
               The last keeps the top (i.e. the title bar) in the work area.
               The fourth is redundant with the last, so can be ignored.

               Substituting for dx and dy and rearranging gives us...
             */
            pt.x = min(pt.x, info.rcWork.right - 1 + capturePoint.x - movedRect.left);
            pt.x = max(pt.x, info.rcWork.left + 1 + capturePoint.x - movedRect.right);
            pt.y = min(pt.y, info.rcWork.bottom - 1 + capturePoint.y - movedRect.top - captionHeight);
            pt.y = max(pt.y, info.rcWork.top + capturePoint.y - movedRect.top);
        }

        dx = pt.x - capturePoint.x;
        dy = pt.y - capturePoint.y;

        if (dx || dy)
        {
            moved = TRUE;

            if (msg.message == WM_KEYDOWN) NtUserSetCursorPos(pt.x, pt.y);
            else
            {
                OffsetRect(&movedRect, dx, dy);
                capturePoint = pt;

                send_message(hwnd, WM_MOVING, 0, (LPARAM)&movedRect);
                set_window_pos(hwnd, 0, movedRect.left, movedRect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }

    set_capture_window_for_move(0);

    send_message(hwnd, WM_EXITSIZEMOVE, 0, 0);
    send_message(hwnd, WM_SETVISIBLE, TRUE, 0L);

    /* if the move is canceled, restore the previous position */
    if (moved && msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
    {
        set_window_pos(hwnd, 0, origRect.left, origRect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }

    return 0;
}


/***********************************************************************
 *              perform_window_command
 */
static void perform_window_command(HWND hwnd, unsigned int style_any, unsigned int style_none, WORD command, WORD hittest)
{
    unsigned int style;

    TRACE("win %p style_any 0x%08x style_none 0x%08x command 0x%04x hittest 0x%04x\n",
          hwnd, style_any, style_none, command, hittest);

    style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    if ((style_any && !(style & style_any)) || (style & (WS_DISABLED | style_none)))
    {
        TRACE("not changing win %p style 0x%08x\n", hwnd, style);
        return;
    }

    if (get_active_window() != hwnd)
    {
        LRESULT ma = send_message(hwnd, WM_MOUSEACTIVATE, (WPARAM)NtUserGetAncestor(hwnd, GA_ROOT),
                                  MAKELPARAM(hittest, WM_NCLBUTTONDOWN));
        switch (ma)
        {
            case MA_NOACTIVATEANDEAT:
            case MA_ACTIVATEANDEAT:
                TRACE("not changing win %p mouse-activate result %ld\n", hwnd, ma);
                return;
            case MA_NOACTIVATE:
                break;
            case MA_ACTIVATE:
            case 0:
                NtUserSetActiveWindow(hwnd);
                break;
            default:
                WARN("unknown WM_MOUSEACTIVATE code %ld\n", ma);
                break;
        }
    }

    TRACE("changing win %p\n", hwnd);
    NtUserPostMessage(hwnd, WM_SYSCOMMAND, command, 0);
}

static void macdrv_client_surface_destroy(struct client_surface *client)
{
    struct macdrv_client_surface *surface = impl_from_client_surface(client);

    TRACE("%s\n", debugstr_client_surface(client));

    if (surface->metal_view) macdrv_view_release_metal_view(surface->metal_view);
    if (surface->metal_device) macdrv_release_metal_device(surface->metal_device);
}

static void macdrv_client_surface_detach(struct client_surface *client)
{
    struct macdrv_client_surface *surface = impl_from_client_surface(client);

    TRACE("%s\n", debugstr_client_surface(client));

    if (surface->cocoa_view)
    {
        struct macdrv_win_data *data;

        if ((data = get_win_data(client->hwnd)))
        {
            if (data->client_view == surface->cocoa_view)
                data->client_view = NULL;
            release_win_data(data);
        }

        macdrv_dispose_view(surface->cocoa_view);
    }
}

static void macdrv_client_surface_update(struct client_surface *client)
{
    struct macdrv_client_surface *surface = impl_from_client_surface(client);
    HWND hwnd = client->hwnd, toplevel = NtUserGetAncestor(hwnd, GA_ROOT);
    struct macdrv_win_data *data;
    RECT rect;

    TRACE("%s\n", debugstr_client_surface(client));

    NtUserGetClientRect(hwnd, &rect, NtUserGetWinMonitorDpi(hwnd, MDT_RAW_DPI));
    NtUserMapWindowPoints(hwnd, toplevel, (POINT *)&rect, 2, NtUserGetWinMonitorDpi(toplevel, MDT_RAW_DPI));

    if (!(data = get_win_data(toplevel))) return;
    OffsetRect(&rect, data->rects.client.left - data->rects.visible.left, data->rects.client.top - data->rects.visible.top);
    macdrv_set_view_frame(surface->cocoa_view, cgrect_from_rect(rect));
    macdrv_set_view_superview(surface->cocoa_view, toplevel == hwnd ? NULL : data->client_view, data->cocoa_window, NULL, NULL);
    release_win_data(data);
}

static void macdrv_client_surface_present(struct client_surface *client, HDC hdc)
{
    struct macdrv_client_surface *surface = impl_from_client_surface(client);
    struct macdrv_win_data *data;

    TRACE("%s\n", debugstr_client_surface(client));

    if (!(data = get_win_data(surface->client.hwnd))) return;
    if (data->client_view != surface->cocoa_view)
    {
        if (data->client_view) macdrv_set_view_hidden(data->client_view, TRUE);
        macdrv_set_view_hidden(surface->cocoa_view, FALSE);
        data->client_view = surface->cocoa_view;
    }
    release_win_data(data);
}

static const struct client_surface_funcs macdrv_client_surface_funcs =
{
    .destroy = macdrv_client_surface_destroy,
    .detach = macdrv_client_surface_detach,
    .update = macdrv_client_surface_update,
    .present = macdrv_client_surface_present,
};

struct macdrv_client_surface *macdrv_client_surface_create(HWND hwnd)
{
    HWND toplevel = NtUserGetAncestor(hwnd, GA_ROOT);
    struct macdrv_client_surface *surface;
    struct macdrv_win_data *data;
    RECT rect;

    NtUserGetClientRect(hwnd, &rect, NtUserGetWinMonitorDpi(hwnd, MDT_RAW_DPI));
    NtUserMapWindowPoints(hwnd, toplevel, (POINT *)&rect, 2, NtUserGetWinMonitorDpi(toplevel, MDT_RAW_DPI));

    if (!(data = get_win_data(toplevel))) return FALSE;
    surface = client_surface_create(sizeof(*surface), &macdrv_client_surface_funcs, hwnd);
    surface->cocoa_view = macdrv_create_view(cgrect_from_rect(rect));
    macdrv_set_view_hidden(surface->cocoa_view, TRUE);
    release_win_data(data);

    if (surface)
    {
        macdrv_client_surface_update(&surface->client);
        macdrv_client_surface_present(&surface->client, 0);
    }

    return surface;
}

/**********************************************************************
 *              SetDesktopWindow   (MACDRV.@)
 */
void macdrv_SetDesktopWindow(HWND hwnd)
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

    set_app_icon();
}

#define WM_WINE_NOTIFY_ACTIVITY WM_USER

LRESULT macdrv_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_WINE_NOTIFY_ACTIVITY:
    {
        /* This wakes from display sleep, but doesn't affect the screen saver. */
        static IOPMAssertionID assertion;
        IOPMAssertionDeclareUserActivity(CFSTR("Wine user input"), kIOPMUserActiveLocal, &assertion);

        /* This prevents the screen saver, but doesn't wake from display sleep. */
        /* It's deprecated, but there's no better alternative. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        UpdateSystemActivity(UsrActivity);
#pragma clang diagnostic pop
        break;
    }
    }
    return NtUserMessageCall(hwnd, msg, wp, lp, 0, NtUserDefWindowProc, FALSE);
}

/***********************************************************************
 *              DestroyWindow   (MACDRV.@)
 */
void macdrv_DestroyWindow(HWND hwnd)
{
    struct macdrv_win_data *data;

    TRACE("%p\n", hwnd);

    if (!(data = get_win_data(hwnd))) return;

    if (hwnd == get_capture()) macdrv_SetCapture(0, 0);
    if (data->drag_event) NtSetEvent(data->drag_event, NULL);

    destroy_cocoa_window(data);

    CFDictionaryRemoveValue(win_datas, hwnd);
    release_win_data(data);
    free(data);
}


/*****************************************************************
 *              ActivateWindow   (MACDRV.@)
 *
 * Set the Mac active window.
 */
void macdrv_ActivateWindow(HWND hwnd, HWND previous)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();

    TRACE("%p\n", hwnd);

    if (!thread_data) return;
    thread_data->dead_key_state = 0;
    set_focus(hwnd, TRUE);
}


/***********************************************************************
 *              SetLayeredWindowAttributes  (MACDRV.@)
 *
 * Set transparency attributes for a layered window.
 */
void macdrv_SetLayeredWindowAttributes(HWND hwnd, COLORREF key, BYTE alpha, DWORD flags)
{
    struct macdrv_win_data *data = get_win_data(hwnd);

    TRACE("hwnd %p key %#08x alpha %#02x flags %x\n", hwnd, key, alpha, flags);

    if (data)
    {
        data->layered = TRUE;
        data->ulw_layered = FALSE;
        if (data->cocoa_window)
        {
            sync_window_opacity(data, alpha, FALSE, flags);
            /* since layered attributes are now set, can now show the window */
            if ((NtUserGetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE) && !data->on_screen)
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
void macdrv_SetParent(HWND hwnd, HWND parent, HWND old_parent)
{
    struct macdrv_win_data *data;

    TRACE("%p, %p, %p\n", hwnd, parent, old_parent);

    if (parent == old_parent) return;
    if (!(data = get_win_data(hwnd))) return;

    if (parent != NtUserGetDesktopWindow()) /* a child window */
    {
        if (old_parent == NtUserGetDesktopWindow())
        {
            /* destroy the old Mac window */
            destroy_cocoa_window(data);
        }
    }
    else  /* new top level window */
    {
        create_cocoa_window(data);
    }
    release_win_data(data);
}


/***********************************************************************
 *              SetWindowRgn  (MACDRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
void macdrv_SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL redraw)
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

        NtUserGetWindowThread(hwnd, &procid);
        if (procid != GetCurrentProcessId())
            send_message(hwnd, WM_MACDRV_SET_WIN_REGION, 0, 0);
    }
}


/***********************************************************************
 *              SetWindowStyle   (MACDRV.@)
 *
 * Update the state of the Cocoa window to reflect a style change
 */
void macdrv_SetWindowStyle(HWND hwnd, INT offset, STYLESTRUCT *style)
{
    struct macdrv_win_data *data;

    TRACE("hwnd %p offset %d styleOld 0x%08x styleNew 0x%08x\n", hwnd, offset, style->styleOld, style->styleNew);

    if (hwnd == NtUserGetDesktopWindow()) return;
    if (!(data = get_win_data(hwnd))) return;

    if (data->cocoa_window)
    {
        DWORD changed = style->styleNew ^ style->styleOld;

        set_cocoa_window_properties(data);

        if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYERED)) /* changing WS_EX_LAYERED resets attributes */
        {
            data->layered = FALSE;
            data->ulw_layered = FALSE;
            sync_window_opacity(data, 0, FALSE, 0);
        }

        if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYOUTRTL))
            sync_window_region(data, (HRGN)1);
    }

    release_win_data(data);
}


/*****************************************************************
 *              SetWindowText   (MACDRV.@)
 */
void macdrv_SetWindowText(HWND hwnd, LPCWSTR text)
{
    macdrv_window win;

    TRACE("%p, %s\n", hwnd, debugstr_w(text));

    if ((win = macdrv_get_cocoa_window(hwnd, FALSE)))
        macdrv_set_cocoa_window_title(win, text, wcslen(text));
}


/***********************************************************************
 *              ShowWindow   (MACDRV.@)
 */
UINT macdrv_ShowWindow(HWND hwnd, INT cmd, RECT *rect, UINT swp)
{
    struct macdrv_thread_data *thread_data = macdrv_thread_data();
    struct macdrv_win_data *data = get_win_data(hwnd);
    CGRect frame;

    TRACE("win %p/%p cmd %d at %s flags %08x\n",
          hwnd, data ? data->cocoa_window : NULL, cmd, wine_dbgstr_rect(rect), swp);

    if (!data || !data->cocoa_window) goto done;
    if (NtUserGetWindowLongW(hwnd, GWL_STYLE) & WS_MINIMIZE)
    {
        if (rect->left != -32000 || rect->top != -32000)
        {
            OffsetRect(rect, -32000 - rect->left, -32000 - rect->top);
            swp &= ~(SWP_NOMOVE | SWP_NOCLIENTMOVE);
        }
        goto done;
    }
    if (!data->on_screen) goto done;

    /* only fetch the new rectangle if the ShowWindow was a result of an external event */

    if (!thread_data->current_event || thread_data->current_event->window != data->cocoa_window)
        goto done;

    if (thread_data->current_event->type != WINDOW_FRAME_CHANGED &&
        thread_data->current_event->type != WINDOW_DID_UNMINIMIZE)
        goto done;

    macdrv_get_cocoa_window_frame(data->cocoa_window, &frame);
    *rect = rect_from_cgrect(frame);
    *rect = window_rect_from_visible(&data->rects, *rect);
    TRACE("rect %s -> %s\n", wine_dbgstr_cgrect(frame), wine_dbgstr_rect(rect));
    swp &= ~(SWP_NOMOVE | SWP_NOCLIENTMOVE | SWP_NOSIZE | SWP_NOCLIENTSIZE);

done:
    release_win_data(data);
    return swp;
}


/***********************************************************************
 *              SysCommand   (MACDRV.@)
 *
 * Perform WM_SYSCOMMAND handling.
 */
LRESULT macdrv_SysCommand(HWND hwnd, WPARAM wparam, LPARAM lparam, const POINT *pos)
{
    struct macdrv_win_data *data;
    LRESULT ret = -1;
    WPARAM command = wparam & 0xfff0;

    TRACE("%p, %x, %lx\n", hwnd, (unsigned)wparam, lparam);

    if (!(data = get_win_data(hwnd))) goto done;
    if (!data->cocoa_window || !data->on_screen) goto done;

    /* prevent a simple ALT press+release from activating the system menu,
       as that can get confusing */
    if (command == SC_KEYMENU && !(WCHAR)lparam &&
        !NtUserGetWindowLongPtrW(hwnd, GWLP_ID) &&
        (NtUserGetWindowLongW(hwnd, GWL_STYLE) & WS_SYSMENU))
    {
        TRACE("ignoring SC_KEYMENU wp %lx lp %lx\n", (unsigned long)wparam, lparam);
        ret = 0;
    }

    if (command == SC_MOVE)
    {
        release_win_data(data);
        return move_window(hwnd, wparam);
    }

done:
    release_win_data(data);
    return ret;
}


/***********************************************************************
 *              UpdateLayeredWindow   (MACDRV.@)
 */
void macdrv_UpdateLayeredWindow(HWND hwnd, BYTE alpha, UINT flags)
{
    struct macdrv_win_data *data;

    if ((data = get_win_data(hwnd)))
    {
        /* Since layered attributes are now set, can now show the window */
        if (data->cocoa_window && !data->on_screen && NtUserGetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE)
            show_window(data);

        /* The ULW flags are a superset of the LWA flags. */
        sync_window_opacity(data, alpha, TRUE, flags);
        release_win_data(data);
    }
}


/**********************************************************************
 *              WindowMessage   (MACDRV.@)
 */
LRESULT macdrv_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
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
    case WM_MACDRV_ACTIVATE_ON_FOLLOWING_FOCUS:
        activate_on_following_focus();
        TRACE("WM_MACDRV_ACTIVATE_ON_FOLLOWING_FOCUS time %u\n", activate_on_focus_time);
        return 0;
    }

    FIXME("unrecognized window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, (unsigned long)wp, lp);
    return 0;
}


/***********************************************************************
 *              WindowPosChanging   (MACDRV.@)
 */
BOOL macdrv_WindowPosChanging(HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects)
{
    struct macdrv_win_data *data = get_win_data(hwnd);
    BOOL ret = FALSE;

    TRACE("hwnd %p, swp_flags %04x, shaped %u, rects %s\n", hwnd, swp_flags, shaped, debugstr_window_rects(rects));

    if (!data && !(data = macdrv_create_win_data(hwnd, rects))) return FALSE; /* use default surface */
    data->shaped = shaped;

    ret = !!data->cocoa_window; /* use default surface if we don't have a window */
    release_win_data(data);

    return ret;
}


/***********************************************************************
 *      GetWindowStyleMasks   (X11DRV.@)
 */
BOOL macdrv_GetWindowStyleMasks(HWND hwnd, UINT style, UINT ex_style, UINT *style_mask, UINT *ex_style_mask)
{
    struct macdrv_window_features wf = get_window_features_for_style(style, ex_style, FALSE);

    *style_mask = ex_style = 0;
    if (wf.title_bar)
    {
        *style_mask |= WS_CAPTION;
        *ex_style_mask |= WS_EX_TOOLWINDOW;
    }
    if (wf.shadow)
    {
        *style_mask |= WS_DLGFRAME | WS_THICKFRAME;
        *ex_style_mask |= WS_EX_DLGMODALFRAME;
    }

    return TRUE;
}


/***********************************************************************
 *              WindowPosChanged   (MACDRV.@)
 */
void macdrv_WindowPosChanged(HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                             const struct window_rects *new_rects, struct window_surface *surface)
{
    struct macdrv_thread_data *thread_data;
    struct macdrv_win_data *data;
    unsigned int new_style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    struct window_rects old_rects;

    if (!(data = get_win_data(hwnd))) return;

    thread_data = macdrv_thread_data();

    old_rects = data->rects;
    data->rects = *new_rects;

    TRACE("win %p/%p new_rects %s style %08x flags %08x fullscreen %u surface %p\n", hwnd, data->cocoa_window,
          debugstr_window_rects(new_rects), new_style, swp_flags, fullscreen, surface);

    if (!data->cocoa_window) goto done;

    if (data->on_screen)
    {
        if ((swp_flags & SWP_HIDEWINDOW) && !(new_style & WS_VISIBLE))
            hide_window(data);
    }

    /* check if we are currently processing an event relevant to this window */
    if (!thread_data || !thread_data->current_event ||
        !data->cocoa_window || thread_data->current_event->window != data->cocoa_window ||
        (thread_data->current_event->type != WINDOW_FRAME_CHANGED &&
         thread_data->current_event->type != WINDOW_DID_UNMINIMIZE))
    {
        sync_window_position(data, swp_flags, &old_rects);
        if (data->cocoa_window)
            set_cocoa_window_properties(data);
    }

    if (new_style & WS_VISIBLE)
    {
        if (data->cocoa_window)
        {
            if (!data->on_screen || (swp_flags & (SWP_FRAMECHANGED|SWP_STATECHANGED)))
                set_cocoa_window_properties(data);

            /* layered windows are not shown until their attributes are set */
            if (!data->on_screen &&
                (data->layered || !(NtUserGetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED)))
                show_window(data);
        }
    }

    if (fullscreen || fullscreen != data->fullscreen)
    {
        CGRect rect = (fullscreen && !EqualRect(&data->rects.window, &data->rects.visible)) ? cgrect_from_rect(data->rects.window) : CGRectZero;
        macdrv_set_window_mask(data->cocoa_window, rect);
        data->fullscreen = fullscreen;
    }

done:
    release_win_data(data);
}


/***********************************************************************
 *              macdrv_window_close_requested
 *
 * Handler for WINDOW_CLOSE_REQUESTED events.
 */
void macdrv_window_close_requested(HWND hwnd)
{
    HMENU sysmenu;

    if (NtUserGetClassLongW(hwnd, GCL_STYLE) & CS_NOCLOSE)
    {
        TRACE("not closing win %p class style CS_NOCLOSE\n", hwnd);
        return;
    }

    sysmenu = NtUserGetSystemMenu(hwnd, FALSE);
    if (sysmenu)
    {
        UINT state = NtUserThunkedMenuItemInfo(sysmenu, SC_CLOSE, MF_BYCOMMAND,
                                               NtUserGetMenuState, NULL, NULL);
        if (state == 0xFFFFFFFF || (state & (MF_DISABLED | MF_GRAYED)))
        {
            TRACE("not closing win %p menu state 0x%08x\n", hwnd, state);
            return;
        }
    }

    perform_window_command(hwnd, 0, 0, SC_CLOSE, HTCLOSE);
}


/***********************************************************************
 *              macdrv_window_frame_changed
 *
 * Handler for WINDOW_FRAME_CHANGED events.
 */
void macdrv_window_frame_changed(HWND hwnd, const macdrv_event *event)
{
    struct macdrv_win_data *data;
    RECT rect;
    UINT flags = SWP_NOACTIVATE | SWP_NOZORDER;
    int width, height;
    BOOL being_dragged;

    if (!hwnd) return;
    if (!(data = get_win_data(hwnd))) return;
    if (!data->on_screen || data->minimized)
    {
        release_win_data(data);
        return;
    }

    /* Get geometry */

    TRACE("win %p/%p new Cocoa frame %s fullscreen %d in_resize %d\n", hwnd, data->cocoa_window,
          wine_dbgstr_cgrect(event->window_frame_changed.frame),
          event->window_frame_changed.fullscreen, event->window_frame_changed.in_resize);

    rect = rect_from_cgrect(event->window_frame_changed.frame);
    rect = window_rect_from_visible(&data->rects, rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    if (data->rects.window.left == rect.left && data->rects.window.top == rect.top)
        flags |= SWP_NOMOVE;
    else
        TRACE("%p moving from (%d,%d) to (%d,%d)\n", hwnd, data->rects.window.left,
              data->rects.window.top, rect.left, rect.top);

    if ((data->rects.window.right - data->rects.window.left == width &&
         data->rects.window.bottom - data->rects.window.top == height) ||
        (IsRectEmpty(&data->rects.window) && width == 1 && height == 1))
        flags |= SWP_NOSIZE;
    else
        TRACE("%p resizing from (%dx%d) to (%dx%d)\n", hwnd, data->rects.window.right - data->rects.window.left,
              data->rects.window.bottom - data->rects.window.top, width, height);

    being_dragged = data->drag_event != NULL;
    release_win_data(data);

    if (event->window_frame_changed.fullscreen)
        flags |= SWP_NOSENDCHANGING;
    if (!(flags & SWP_NOSIZE) || !(flags & SWP_NOMOVE))
    {
        int send_sizemove = !event->window_frame_changed.in_resize && !being_dragged && !event->window_frame_changed.skip_size_move_loop;
        if (send_sizemove)
            send_message(hwnd, WM_ENTERSIZEMOVE, 0, 0);
        NtUserSetRawWindowPos(hwnd, rect, flags, FALSE);
        if (send_sizemove)
            send_message(hwnd, WM_EXITSIZEMOVE, 0, 0);
    }
}


/***********************************************************************
 *              macdrv_window_got_focus
 *
 * Handler for WINDOW_GOT_FOCUS events.
 */
void macdrv_window_got_focus(HWND hwnd, const macdrv_event *event)
{
    unsigned int style = NtUserGetWindowLongW(hwnd, GWL_STYLE);

    if (!hwnd) return;

    TRACE("win %p/%p serial %lu enabled %d visible %d style %08x focus %p active %p fg %p\n",
          hwnd, event->window, event->window_got_focus.serial, NtUserIsWindowEnabled(hwnd),
          NtUserIsWindowVisible(hwnd), style, get_focus(), get_active_window(), NtUserGetForegroundWindow());

    if (can_window_become_foreground(hwnd) && !(style & WS_MINIMIZE))
    {
        TRACE("setting foreground window to %p\n", hwnd);
        NtUserSetForegroundWindow(hwnd);
        return;
    }

    TRACE("win %p/%p rejecting focus\n", hwnd, event->window);
    macdrv_window_rejected_focus(event);
}


/***********************************************************************
 *              macdrv_window_lost_focus
 *
 * Handler for WINDOW_LOST_FOCUS events.
 */
void macdrv_window_lost_focus(HWND hwnd, const macdrv_event *event)
{
    if (!hwnd) return;

    TRACE("win %p/%p fg %p\n", hwnd, event->window, NtUserGetForegroundWindow());

    if (hwnd == NtUserGetForegroundWindow())
    {
        send_message(hwnd, WM_CANCELMODE, 0, 0);
        if (hwnd == NtUserGetForegroundWindow())
            NtUserSetForegroundWindow(NtUserGetDesktopWindow());
    }
}


/***********************************************************************
 *              macdrv_app_activated
 *
 * Handler for APP_ACTIVATED events.
 */
void macdrv_app_activated(void)
{
    TRACE("\n");
    macdrv_UpdateClipboard();
}


/***********************************************************************
 *              macdrv_app_deactivated
 *
 * Handler for APP_DEACTIVATED events.
 */
void macdrv_app_deactivated(void)
{
    NtUserClipCursor(NULL);

    if (get_active_window() == NtUserGetForegroundWindow())
    {
        TRACE("setting fg to desktop\n");
        NtUserSetForegroundWindow(NtUserGetDesktopWindow());
    }
}


/***********************************************************************
 *              macdrv_window_maximize_requested
 *
 * Handler for WINDOW_MAXIMIZE_REQUESTED events.
 */
void macdrv_window_maximize_requested(HWND hwnd)
{
    perform_window_command(hwnd, WS_MAXIMIZEBOX, WS_MAXIMIZE, SC_MAXIMIZE, HTMAXBUTTON);
}


/***********************************************************************
 *              macdrv_window_minimize_requested
 *
 * Handler for WINDOW_MINIMIZE_REQUESTED events.
 */
void macdrv_window_minimize_requested(HWND hwnd)
{
    perform_window_command(hwnd, WS_MINIMIZEBOX, WS_MINIMIZE, SC_MINIMIZE, HTMINBUTTON);
}


/***********************************************************************
 *              macdrv_window_did_minimize
 *
 * Handler for WINDOW_DID_MINIMIZE events.
 */
void macdrv_window_did_minimize(HWND hwnd)
{
    TRACE("win %p\n", hwnd);

    /* If all our windows are minimized, disable cursor clipping. */
    if (!macdrv_is_any_wine_window_visible())
        NtUserClipCursor(NULL);
}


/***********************************************************************
 *              macdrv_window_did_unminimize
 *
 * Handler for WINDOW_DID_UNMINIMIZE events.
 */
void macdrv_window_did_unminimize(HWND hwnd)
{
    struct macdrv_win_data *data;
    unsigned int style;

    TRACE("win %p\n", hwnd);

    if (!(data = get_win_data(hwnd))) return;
    if (!data->minimized) goto done;

    style = NtUserGetWindowLongW(hwnd, GWL_STYLE);

    data->minimized = FALSE;
    if ((style & (WS_MINIMIZE | WS_VISIBLE)) == (WS_MINIMIZE | WS_VISIBLE))
    {
        TRACE("restoring win %p/%p\n", hwnd, data->cocoa_window);
        release_win_data(data);
        NtUserSetActiveWindow(hwnd);
        send_message(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        return;
    }

    TRACE("not restoring win %p/%p style %08x\n", hwnd, data->cocoa_window, style);

done:
    release_win_data(data);
}


/***********************************************************************
 *              macdrv_window_brought_forward
 *
 * Handler for WINDOW_BROUGHT_FORWARD events.
 */
void macdrv_window_brought_forward(HWND hwnd)
{
    TRACE("win %p\n", hwnd);
    NtUserSetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}


/***********************************************************************
 *              macdrv_window_resize_ended
 *
 * Handler for WINDOW_RESIZE_ENDED events.
 */
void macdrv_window_resize_ended(HWND hwnd)
{
    TRACE("hwnd %p\n", hwnd);
    send_message(hwnd, WM_EXITSIZEMOVE, 0, 0);
}


/***********************************************************************
 *              macdrv_window_restore_requested
 *
 * Handler for WINDOW_RESTORE_REQUESTED events.  This is specifically
 * for restoring from maximized, not from minimized.
 */
void macdrv_window_restore_requested(HWND hwnd, const macdrv_event *event)
{
    if (event->window_restore_requested.keep_frame && hwnd)
    {
        DWORD style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
        struct macdrv_win_data *data;

        if ((style & WS_MAXIMIZE) && (style & WS_VISIBLE) && (data = get_win_data(hwnd)))
        {
            RECT rect = rect_from_cgrect(event->window_restore_requested.frame);
            rect = window_rect_from_visible(&data->rects, rect);
            release_win_data(data);

            NtUserSetRawWindowPos(hwnd, rect, 0, TRUE);
        }
    }

    perform_window_command(hwnd, WS_MAXIMIZE, 0, SC_RESTORE, HTMAXBUTTON);
}


/***********************************************************************
 *              macdrv_window_drag_begin
 *
 * Handler for WINDOW_DRAG_BEGIN events.
 */
void macdrv_window_drag_begin(HWND hwnd, const macdrv_event *event)
{
    DWORD style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    struct macdrv_win_data *data;
    HANDLE drag_event = NULL;
    OBJECT_ATTRIBUTES attr;
    BOOL loop = TRUE;
    MSG msg;

    TRACE("win %p\n", hwnd);

    if (style & (WS_DISABLED | WS_MAXIMIZE | WS_MINIMIZE)) return;
    if (!(style & WS_VISIBLE)) return;

    if (!(data = get_win_data(hwnd))) return;
    if (data->drag_event) goto done;

    InitializeObjectAttributes(&attr, NULL, OBJ_OPENIF, NULL, NULL);
    if (NtCreateEvent(&drag_event, EVENT_ALL_ACCESS, &attr, NotificationEvent, FALSE)) goto done;

    data->drag_event = drag_event;
    release_win_data(data);

    if (!event->window_drag_begin.no_activate && can_window_become_foreground(hwnd) &&
        NtUserGetForegroundWindow() != hwnd)
    {
        /* ask whether the window wants to be activated */
        LRESULT ma = send_message(hwnd, WM_MOUSEACTIVATE, (WPARAM)NtUserGetAncestor(hwnd, GA_ROOT),
                                  MAKELONG(HTCAPTION, WM_LBUTTONDOWN));
        if (ma != MA_NOACTIVATEANDEAT && ma != MA_NOACTIVATE)
        {
            TRACE("setting foreground window to %p\n", hwnd);
            NtUserSetForegroundWindow(hwnd);
        }
    }

    NtUserClipCursor(NULL);
    send_message(hwnd, WM_ENTERSIZEMOVE, 0, 0);
    NtUserReleaseCapture();

    while (loop)
    {
        while (!NtUserPeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            DWORD result = NtUserMsgWaitForMultipleObjectsEx(1, &drag_event, INFINITE, QS_ALLINPUT,
                                                             MWMO_INPUTAVAILABLE);
            if (result == WAIT_OBJECT_0)
            {
                loop = FALSE;
                break;
            }
        }
        if (!loop)
            break;

        if (msg.message == WM_QUIT)
            break;

        if (!NtUserCallMsgFilter(&msg, MSGF_SIZE) && msg.message != WM_KEYDOWN &&
            msg.message != WM_MOUSEMOVE && msg.message != WM_LBUTTONDOWN && msg.message != WM_LBUTTONUP)
        {
            NtUserTranslateMessage(&msg, 0);
            NtUserDispatchMessage(&msg);
        }
    }

    send_message(hwnd, WM_EXITSIZEMOVE, 0, 0);

    TRACE("done\n");

    if ((data = get_win_data(hwnd)))
        data->drag_event = NULL;

done:
    release_win_data(data);
    if (drag_event) NtClose(drag_event);
}


/***********************************************************************
 *              macdrv_window_drag_end
 *
 * Handler for WINDOW_DRAG_END events.
 */
void macdrv_window_drag_end(HWND hwnd)
{
    struct macdrv_win_data *data;

    TRACE("win %p\n", hwnd);

    if (!(data = get_win_data(hwnd))) return;
    if (data->drag_event)
        NtSetEvent(data->drag_event, NULL);
    release_win_data(data);
}


/***********************************************************************
 *              macdrv_reassert_window_position
 *
 * Handler for REASSERT_WINDOW_POSITION events.
 */
void macdrv_reassert_window_position(HWND hwnd)
{
    struct macdrv_win_data *data = get_win_data(hwnd);
    if (data)
    {
        if (data->cocoa_window && data->on_screen)
            sync_window_position(data, SWP_NOZORDER | SWP_NOACTIVATE, NULL);
        release_win_data(data);
    }
}


/***********************************************************************
 *              macdrv_app_quit_requested
 *
 * Handler for APP_QUIT_REQUESTED events.
 */
void macdrv_app_quit_requested(const macdrv_event *event)
{
    struct app_quit_request_params params = {.dispatch = {.callback = app_quit_request_callback}};
    void *ret_ptr;
    ULONG ret_len;

    TRACE("reason %d\n", event->app_quit_requested.reason);

    if (event->app_quit_requested.reason == QUIT_REASON_LOGOUT)
        params.flags = ENDSESSION_LOGOFF;

    KeUserDispatchCallback(&params.dispatch, sizeof(params), &ret_ptr, &ret_len);
}


/***********************************************************************
 *              query_resize_size
 *
 * Handler for QUERY_RESIZE_SIZE query.
 */
BOOL query_resize_size(HWND hwnd, macdrv_query *query)
{
    struct macdrv_win_data *data = get_win_data(hwnd);
    RECT rect;
    int corner;
    BOOL ret = FALSE;

    if (!data) return FALSE;

    rect = rect_from_cgrect(query->resize_size.rect);
    rect = window_rect_from_visible(&data->rects, rect);

    if (query->resize_size.from_left)
    {
        if (query->resize_size.from_top)
            corner = WMSZ_TOPLEFT;
        else
            corner = WMSZ_BOTTOMLEFT;
    }
    else if (query->resize_size.from_top)
        corner = WMSZ_TOPRIGHT;
    else
        corner = WMSZ_BOTTOMRIGHT;

    if (send_message(hwnd, WM_SIZING, corner, (LPARAM)&rect))
    {
        rect = visible_rect_from_window(&data->rects, rect);
        query->resize_size.rect = cgrect_from_rect(rect);
        ret = TRUE;
    }

    release_win_data(data);
    return ret;
}


/***********************************************************************
 *              query_resize_start
 *
 * Handler for QUERY_RESIZE_START query.
 */
BOOL query_resize_start(HWND hwnd)
{
    TRACE("hwnd %p\n", hwnd);

    NtUserClipCursor(NULL);

    sync_window_min_max_info(hwnd);
    send_message(hwnd, WM_ENTERSIZEMOVE, 0, 0);

    return TRUE;
}


/***********************************************************************
 *              query_min_max_info
 *
 * Handler for QUERY_MIN_MAX_INFO query.
 */
BOOL query_min_max_info(HWND hwnd)
{
    TRACE("hwnd %p\n", hwnd);
    sync_window_min_max_info(hwnd);
    return TRUE;
}


/***********************************************************************
 *              init_win_context
 */
void init_win_context(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&win_data_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

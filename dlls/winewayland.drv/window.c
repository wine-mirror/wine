/*
 * Wayland window handling
 *
 * Copyright 2020 Alexandros Frantzis for Collabora Ltd
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

#include <assert.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "waylanddrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

/* private window data */
struct wayland_win_data
{
    struct rb_entry entry;
    /* hwnd that this private data belongs to */
    HWND hwnd;
    /* wayland surface (if any) for this window */
    struct wayland_surface *wayland_surface;
    /* wine window_surface backing this window */
    struct window_surface *window_surface;
    /* USER window rectangle relative to win32 parent window client area */
    RECT window_rect;
    /* USER client rectangle relative to win32 parent window client area */
    RECT client_rect;
    BOOL managed;
};

static int wayland_win_data_cmp_rb(const void *key,
                                   const struct rb_entry *entry)
{
    HWND key_hwnd = (HWND)key; /* cast to work around const */
    const struct wayland_win_data *entry_win_data =
        RB_ENTRY_VALUE(entry, const struct wayland_win_data, entry);

    if (key_hwnd < entry_win_data->hwnd) return -1;
    if (key_hwnd > entry_win_data->hwnd) return 1;
    return 0;
}

static pthread_mutex_t win_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct rb_tree win_data_rb = { wayland_win_data_cmp_rb };

/***********************************************************************
 *           wayland_win_data_create
 *
 * Create a data window structure for an existing window.
 */
static struct wayland_win_data *wayland_win_data_create(HWND hwnd,
                                                        const RECT *window_rect,
                                                        const RECT *client_rect)
{
    struct wayland_win_data *data;
    struct rb_entry *rb_entry;
    HWND parent;

    /* Don't create win data for desktop or HWND_MESSAGE windows. */
    if (!(parent = NtUserGetAncestor(hwnd, GA_PARENT))) return NULL;
    if (parent != NtUserGetDesktopWindow() && !NtUserGetAncestor(parent, GA_PARENT))
        return NULL;

    if (!(data = calloc(1, sizeof(*data)))) return NULL;

    data->hwnd = hwnd;
    data->window_rect = *window_rect;
    data->client_rect = *client_rect;

    pthread_mutex_lock(&win_data_mutex);

    /* Check that another thread hasn't already created the wayland_win_data. */
    if ((rb_entry = rb_get(&win_data_rb, hwnd)))
    {
        free(data);
        return RB_ENTRY_VALUE(rb_entry, struct wayland_win_data, entry);
    }

    rb_put(&win_data_rb, hwnd, &data->entry);

    TRACE("hwnd=%p\n", data->hwnd);

    return data;
}

/***********************************************************************
 *           wayland_win_data_destroy
 */
static void wayland_win_data_destroy(struct wayland_win_data *data)
{
    TRACE("hwnd=%p\n", data->hwnd);

    rb_remove(&win_data_rb, &data->entry);

    pthread_mutex_unlock(&win_data_mutex);

    if (data->window_surface)
    {
        wayland_window_surface_update_wayland_surface(data->window_surface, NULL);
        window_surface_release(data->window_surface);
    }
    if (data->wayland_surface) wayland_surface_destroy(data->wayland_surface);
    free(data);
}

/***********************************************************************
 *           wayland_win_data_get
 *
 * Lock and return the data structure associated with a window.
 */
static struct wayland_win_data *wayland_win_data_get(HWND hwnd)
{
    struct rb_entry *rb_entry;

    pthread_mutex_lock(&win_data_mutex);

    if ((rb_entry = rb_get(&win_data_rb, hwnd)))
        return RB_ENTRY_VALUE(rb_entry, struct wayland_win_data, entry);

    pthread_mutex_unlock(&win_data_mutex);

    return NULL;
}

/***********************************************************************
 *           wayland_win_data_release
 *
 * Release the data returned by wayland_win_data_get.
 */
static void wayland_win_data_release(struct wayland_win_data *data)
{
    assert(data);
    pthread_mutex_unlock(&win_data_mutex);
}

static void wayland_win_data_get_config(struct wayland_win_data *data,
                                        struct wayland_window_config *conf)
{
    enum wayland_surface_config_state window_state = 0;
    DWORD style;

    conf->rect = data->window_rect;
    conf->client_rect = data->client_rect;
    style = NtUserGetWindowLongW(data->hwnd, GWL_STYLE);

    TRACE("window=%s style=%#lx\n", wine_dbgstr_rect(&conf->rect), (long)style);

    /* The fullscreen state is implied by the window position and style. */
    if (NtUserIsWindowRectFullScreen(&conf->rect))
    {
        if ((style & WS_MAXIMIZE) && (style & WS_CAPTION) == WS_CAPTION)
            window_state |= WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED;
        else if (!(style & WS_MINIMIZE))
            window_state |= WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN;
    }
    else if (style & WS_MAXIMIZE)
    {
        window_state |= WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED;
    }

    conf->state = window_state;
    conf->scale = NtUserGetDpiForWindow(data->hwnd) / 96.0;
    conf->visible = (style & WS_VISIBLE) == WS_VISIBLE;
    conf->managed = data->managed;
}

static void wayland_win_data_update_wayland_surface(struct wayland_win_data *data)
{
    struct wayland_surface *surface = data->wayland_surface;
    HWND parent = NtUserGetAncestor(data->hwnd, GA_PARENT);
    BOOL visible, xdg_visible;
    RECT clip;

    TRACE("hwnd=%p\n", data->hwnd);

    /* We don't want wayland surfaces for child windows. */
    if (parent != NtUserGetDesktopWindow() && parent != 0)
    {
        if (data->window_surface)
            wayland_window_surface_update_wayland_surface(data->window_surface, NULL);
        if (surface) wayland_surface_destroy(surface);
        surface = NULL;
        goto out;
    }

    /* Otherwise ensure that we have a wayland surface. */
    if (!surface && !(surface = wayland_surface_create(data->hwnd))) return;

    visible = (NtUserGetWindowLongW(data->hwnd, GWL_STYLE) & WS_VISIBLE) == WS_VISIBLE;
    xdg_visible = surface->xdg_toplevel != NULL;

    pthread_mutex_lock(&surface->mutex);

    if (visible != xdg_visible)
    {
        /* If we have a pre-existing surface ensure it has no role. */
        if (data->wayland_surface) wayland_surface_clear_role(surface);
        /* If the window is a visible toplevel make it a wayland
         * xdg_toplevel. Otherwise keep it role-less to avoid polluting the
         * compositor with empty xdg_toplevels. */
        if (visible) wayland_surface_make_toplevel(surface);
    }

    wayland_win_data_get_config(data, &surface->window);

    pthread_mutex_unlock(&surface->mutex);

    if (data->window_surface)
        wayland_window_surface_update_wayland_surface(data->window_surface, surface);

    /* Size/position changes affect the effective pointer constraint, so update
     * it as needed. */
    if (data->hwnd == NtUserGetForegroundWindow() && NtUserGetClipCursor(&clip))
        NtUserClipCursor(&clip);

out:
    TRACE("hwnd=%p surface=%p=>%p\n", data->hwnd, data->wayland_surface, surface);
    data->wayland_surface = surface;
}

static void wayland_win_data_update_wayland_state(struct wayland_win_data *data)
{
    struct wayland_surface *surface = data->wayland_surface;
    BOOL processing_config;

    pthread_mutex_lock(&surface->mutex);

    if (!surface->xdg_toplevel) goto out;

    processing_config = surface->processing.serial &&
                        !surface->processing.processed;

    TRACE("hwnd=%p window_state=%#x %s->state=%#x\n",
          data->hwnd, surface->window.state,
          processing_config ? "processing" : "current",
          processing_config ? surface->processing.state : surface->current.state);

    /* If we are not processing a compositor requested config, use the
     * window state to determine and update the Wayland state. */
    if (!processing_config)
    {
         /* First do all state unsettings, before setting new state. Some
          * Wayland compositors misbehave if the order is reversed. */
        if (!(surface->window.state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED) &&
            (surface->current.state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED))
        {
            xdg_toplevel_unset_maximized(surface->xdg_toplevel);
        }
        if (!(surface->window.state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN) &&
            (surface->current.state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN))
        {
            xdg_toplevel_unset_fullscreen(surface->xdg_toplevel);
        }

        if ((surface->window.state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED) &&
           !(surface->current.state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED))
        {
            xdg_toplevel_set_maximized(surface->xdg_toplevel);
        }
        if ((surface->window.state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN) &&
           !(surface->current.state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN))
        {
            xdg_toplevel_set_fullscreen(surface->xdg_toplevel, NULL);
        }
    }
    else
    {
        surface->processing.processed = TRUE;
    }

out:
    pthread_mutex_unlock(&surface->mutex);
    wl_display_flush(process_wayland.wl_display);
}

static BOOL is_managed(HWND hwnd)
{
    struct wayland_win_data *data = wayland_win_data_get(hwnd);
    BOOL ret = data && data->managed;
    if (data) wayland_win_data_release(data);
    return ret;
}

static HWND *build_hwnd_list(void)
{
    NTSTATUS status;
    HWND *list;
    ULONG count = 128;

    for (;;)
    {
        if (!(list = malloc(count * sizeof(*list)))) return NULL;
        status = NtUserBuildHwndList(0, 0, 0, 0, 0, count, list, &count);
        if (!status) return list;
        free(list);
        if (status != STATUS_BUFFER_TOO_SMALL) return NULL;
    }
}

static BOOL has_owned_popups(HWND hwnd)
{
    HWND *list;
    UINT i;
    BOOL ret = FALSE;

    if (!(list = build_hwnd_list())) return FALSE;

    for (i = 0; list[i] != HWND_BOTTOM; i++)
    {
        if (list[i] == hwnd) break;  /* popups are always above owner */
        if (NtUserGetWindowRelative(list[i], GW_OWNER) != hwnd) continue;
        if ((ret = is_managed(list[i]))) break;
    }

    free(list);
    return ret;
}

static inline HWND get_active_window(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo(GetCurrentThreadId(), &info) ? info.hwndActive : 0;
}

/***********************************************************************
 *		is_window_managed
 *
 * Check if a given window should be managed
 */
static BOOL is_window_managed(HWND hwnd, UINT swp_flags, const RECT *window_rect)
{
    DWORD style, ex_style;

    /* child windows are not managed */
    style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD) return FALSE;
    /* activated windows are managed */
    if (!(swp_flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW))) return TRUE;
    if (hwnd == get_active_window()) return TRUE;
    /* windows with caption are managed */
    if ((style & WS_CAPTION) == WS_CAPTION) return TRUE;
    /* windows with thick frame are managed */
    if (style & WS_THICKFRAME) return TRUE;
    if (style & WS_POPUP)
    {
        HMONITOR hmon;
        MONITORINFO mi;

        /* popup with sysmenu == caption are managed */
        if (style & WS_SYSMENU) return TRUE;
        /* full-screen popup windows are managed */
        hmon = NtUserMonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
        mi.cbSize = sizeof(mi);
        NtUserGetMonitorInfo(hmon, &mi);
        if (window_rect->left <= mi.rcWork.left && window_rect->right >= mi.rcWork.right &&
            window_rect->top <= mi.rcWork.top && window_rect->bottom >= mi.rcWork.bottom)
            return TRUE;
    }
    /* application windows are managed */
    ex_style = NtUserGetWindowLongW(hwnd, GWL_EXSTYLE);
    if (ex_style & WS_EX_APPWINDOW) return TRUE;
    /* windows that own popups are managed */
    if (has_owned_popups(hwnd)) return TRUE;
    /* default: not managed */
    return FALSE;
}

/***********************************************************************
 *           WAYLAND_DestroyWindow
 */
void WAYLAND_DestroyWindow(HWND hwnd)
{
    struct wayland_win_data *data;

    TRACE("%p\n", hwnd);

    if (!(data = wayland_win_data_get(hwnd))) return;
    wayland_win_data_destroy(data);
}

/***********************************************************************
 *           WAYLAND_WindowPosChanging
 */
BOOL WAYLAND_WindowPosChanging(HWND hwnd, HWND insert_after, UINT swp_flags,
                               const RECT *window_rect, const RECT *client_rect,
                               RECT *visible_rect, struct window_surface **surface)
{
    struct wayland_win_data *data = wayland_win_data_get(hwnd);
    HWND parent;
    BOOL visible;
    RECT surface_rect;

    TRACE("hwnd %p window %s client %s visible %s after %p flags %08x\n",
          hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
          wine_dbgstr_rect(visible_rect), insert_after, swp_flags);

    if (!data && !(data = wayland_win_data_create(hwnd, window_rect, client_rect)))
        return TRUE;

    /* Release the dummy surface wine provides for toplevels. */
    if (*surface) window_surface_release(*surface);
    *surface = NULL;

    parent = NtUserGetAncestor(hwnd, GA_PARENT);
    visible = ((NtUserGetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE) ||
               (swp_flags & SWP_SHOWWINDOW)) &&
              !(swp_flags & SWP_HIDEWINDOW);

    /* Check if we don't want a dedicated window surface. */
    if ((parent && parent != NtUserGetDesktopWindow()) || !visible) goto done;

    surface_rect = *window_rect;
    OffsetRect(&surface_rect, -surface_rect.left, -surface_rect.top);

    /* Check if we can reuse our current window surface. */
    if (data->window_surface &&
        EqualRect(&data->window_surface->rect, &surface_rect))
    {
        window_surface_add_ref(data->window_surface);
        *surface = data->window_surface;
        TRACE("reusing surface %p\n", *surface);
        goto done;
    }

    *surface = wayland_window_surface_create(data->hwnd, &surface_rect);

done:
    wayland_win_data_release(data);
    return TRUE;
}

/***********************************************************************
 *           WAYLAND_WindowPosChanged
 */
void WAYLAND_WindowPosChanged(HWND hwnd, HWND insert_after, UINT swp_flags,
                              const RECT *window_rect, const RECT *client_rect,
                              const RECT *visible_rect, const RECT *valid_rects,
                              struct window_surface *surface)
{
    struct wayland_win_data *data;
    BOOL managed;

    TRACE("hwnd %p window %s client %s visible %s after %p flags %08x\n",
          hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
          wine_dbgstr_rect(visible_rect), insert_after, swp_flags);

    /* Get the managed state with win_data unlocked, as is_window_managed
     * may need to query win_data information about other HWNDs and thus
     * acquire the lock itself internally. */
    managed = is_window_managed(hwnd, swp_flags, window_rect);

    if (!(data = wayland_win_data_get(hwnd))) return;

    data->window_rect = *window_rect;
    data->client_rect = *client_rect;
    data->managed = managed;

    if (surface) window_surface_add_ref(surface);
    if (data->window_surface) window_surface_release(data->window_surface);
    data->window_surface = surface;

    wayland_win_data_update_wayland_surface(data);
    if (data->wayland_surface) wayland_win_data_update_wayland_state(data);

    wayland_win_data_release(data);
}

static void wayland_resize_desktop(void)
{
    RECT virtual_rect = NtUserGetVirtualScreenRect();
    NtUserSetWindowPos(NtUserGetDesktopWindow(), 0,
                       virtual_rect.left, virtual_rect.top,
                       virtual_rect.right - virtual_rect.left,
                       virtual_rect.bottom - virtual_rect.top,
                       SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERERASE);
}

static void wayland_configure_window(HWND hwnd)
{
    struct wayland_surface *surface;
    INT width, height, window_width, window_height;
    INT window_surf_width, window_surf_height;
    UINT flags = 0;
    uint32_t state;
    DWORD style;
    BOOL needs_enter_size_move = FALSE;
    BOOL needs_exit_size_move = FALSE;

    if (!(surface = wayland_surface_lock_hwnd(hwnd))) return;

    if (!surface->xdg_toplevel)
    {
        TRACE("missing xdg_toplevel, returning\n");
        pthread_mutex_unlock(&surface->mutex);
        return;
    }

    if (!surface->requested.serial)
    {
        TRACE("requested configure event already handled, returning\n");
        pthread_mutex_unlock(&surface->mutex);
        return;
    }

    surface->processing = surface->requested;
    memset(&surface->requested, 0, sizeof(surface->requested));

    state = surface->processing.state;
    /* Ignore size hints if we don't have a state that requires strict
     * size adherence, in order to avoid spurious resizes. */
    if (state)
    {
        width = surface->processing.width;
        height = surface->processing.height;
    }
    else
    {
        width = height = 0;
    }

    if ((state & WAYLAND_SURFACE_CONFIG_STATE_RESIZING) && !surface->resizing)
    {
        surface->resizing = TRUE;
        needs_enter_size_move = TRUE;
    }

    if (!(state & WAYLAND_SURFACE_CONFIG_STATE_RESIZING) && surface->resizing)
    {
        surface->resizing = FALSE;
        needs_exit_size_move = TRUE;
    }

    /* Transitions between normal/max/fullscreen may entail a frame change. */
    if ((state ^ surface->current.state) &
        (WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED |
         WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN))
    {
        flags |= SWP_FRAMECHANGED;
    }

    wayland_surface_coords_from_window(surface,
                                       surface->window.rect.right -
                                           surface->window.rect.left,
                                       surface->window.rect.bottom -
                                           surface->window.rect.top,
                                       &window_surf_width, &window_surf_height);

    /* If the window is already fullscreen and its size is compatible with what
     * the compositor is requesting, don't force a resize, since some applications
     * are very insistent on a particular fullscreen size (which may not match
     * the monitor size). */
    if ((surface->window.state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN) &&
        wayland_surface_config_is_compatible(&surface->processing,
                                             window_surf_width, window_surf_height,
                                             surface->window.state))
    {
        flags |= SWP_NOSIZE;
    }

    wayland_surface_coords_to_window(surface, width, height,
                                     &window_width, &window_height);

    pthread_mutex_unlock(&surface->mutex);

    TRACE("processing=%dx%d,%#x\n", width, height, state);

    if (needs_enter_size_move) send_message(hwnd, WM_ENTERSIZEMOVE, 0, 0);
    if (needs_exit_size_move) send_message(hwnd, WM_EXITSIZEMOVE, 0, 0);

    flags |= SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE;
    if (window_width == 0 || window_height == 0) flags |= SWP_NOSIZE;

    style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    if (!(state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED) != !(style & WS_MAXIMIZE))
        NtUserSetWindowLong(hwnd, GWL_STYLE, style ^ WS_MAXIMIZE, FALSE);

    /* The Wayland maximized and fullscreen states are very strict about
     * surface size, so don't let the application override it. The tiled state
     * is not as strict, but it indicates a strong size preference, so try to
     * respect it. */
    if (state & (WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED |
                 WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN |
                 WAYLAND_SURFACE_CONFIG_STATE_TILED))
    {
        flags |= SWP_NOSENDCHANGING;
    }

    NtUserSetWindowPos(hwnd, 0, 0, 0, window_width, window_height, flags);
}

/**********************************************************************
 *           WAYLAND_WindowMessage
 */
LRESULT WAYLAND_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_WAYLAND_INIT_DISPLAY_DEVICES:
        wayland_init_display_devices(TRUE);
        wayland_resize_desktop();
        return 0;
    case WM_WAYLAND_CONFIGURE:
        wayland_configure_window(hwnd);
        return 0;
    case WM_WAYLAND_SET_FOREGROUND:
        NtUserSetForegroundWindow(hwnd);
        return 0;
    default:
        FIXME("got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, (long)wp, lp);
        return 0;
    }
}

/**********************************************************************
 *           WAYLAND_DesktopWindowProc
 */
LRESULT WAYLAND_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DISPLAYCHANGE:
        wayland_resize_desktop();
        break;
    }

    return NtUserMessageCall(hwnd, msg, wp, lp, 0, NtUserDefWindowProc, FALSE);
}

static enum xdg_toplevel_resize_edge hittest_to_resize_edge(WPARAM hittest)
{
    switch (hittest)
    {
    case WMSZ_LEFT:        return XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
    case WMSZ_RIGHT:       return XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
    case WMSZ_TOP:         return XDG_TOPLEVEL_RESIZE_EDGE_TOP;
    case WMSZ_TOPLEFT:     return XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
    case WMSZ_TOPRIGHT:    return XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
    case WMSZ_BOTTOM:      return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
    case WMSZ_BOTTOMLEFT:  return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
    case WMSZ_BOTTOMRIGHT: return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
    default:               return XDG_TOPLEVEL_RESIZE_EDGE_NONE;
    }
}

/***********************************************************************
 *          WAYLAND_SysCommand
 */
LRESULT WAYLAND_SysCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    LRESULT ret = -1;
    WPARAM command = wparam & 0xfff0;
    uint32_t button_serial;
    struct wl_seat *wl_seat;
    struct wayland_surface *surface;

    TRACE("cmd=%lx hwnd=%p, %lx, %lx\n",
          (long)command, hwnd, (long)wparam, lparam);

    pthread_mutex_lock(&process_wayland.pointer.mutex);
    if (process_wayland.pointer.focused_hwnd == hwnd)
        button_serial = process_wayland.pointer.button_serial;
    else
        button_serial = 0;
    pthread_mutex_unlock(&process_wayland.pointer.mutex);

    if (command == SC_MOVE || command == SC_SIZE)
    {
        if ((surface = wayland_surface_lock_hwnd(hwnd)))
        {
            pthread_mutex_lock(&process_wayland.seat.mutex);
            wl_seat = process_wayland.seat.wl_seat;
            if (wl_seat && surface->xdg_toplevel && button_serial)
            {
                if (command == SC_MOVE)
                {
                    xdg_toplevel_move(surface->xdg_toplevel, wl_seat, button_serial);
                }
                else if (command == SC_SIZE)
                {
                    xdg_toplevel_resize(surface->xdg_toplevel, wl_seat, button_serial,
                                        hittest_to_resize_edge(wparam & 0x0f));
                }
            }
            pthread_mutex_unlock(&process_wayland.seat.mutex);
            pthread_mutex_unlock(&surface->mutex);
            ret = 0;
        }
    }

    wl_display_flush(process_wayland.wl_display);
    return ret;
}

/**********************************************************************
 *           wayland_window_flush
 *
 *  Flush the window_surface associated with a HWND.
 */
void wayland_window_flush(HWND hwnd)
{
    struct wayland_win_data *data = wayland_win_data_get(hwnd);

    if (!data) return;

    if (data->window_surface)
        data->window_surface->funcs->flush(data->window_surface);

    wayland_win_data_release(data);
}

/**********************************************************************
 *           wayland_surface_lock_hwnd
 */
struct wayland_surface *wayland_surface_lock_hwnd(HWND hwnd)
{
    struct wayland_win_data *data = wayland_win_data_get(hwnd);
    struct wayland_surface *surface;

    if (!data) return NULL;

    if ((surface = data->wayland_surface)) pthread_mutex_lock(&surface->mutex);

    wayland_win_data_release(data);

    return surface;
}

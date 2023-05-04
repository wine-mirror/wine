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
static struct wayland_win_data *wayland_win_data_create(HWND hwnd)
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

static void wayland_win_data_update_wayland_surface(struct wayland_win_data *data)
{
    struct wayland_surface *surface = data->wayland_surface;
    HWND parent = NtUserGetAncestor(data->hwnd, GA_PARENT);
    BOOL visible, xdg_visible;

    TRACE("hwnd=%p\n", data->hwnd);

    /* We don't want wayland surfaces for child windows. */
    if (parent != NtUserGetDesktopWindow() && parent != 0)
    {
        if (surface) wayland_surface_destroy(surface);
        surface = NULL;
        goto out;
    }

    /* Otherwise ensure that we have a wayland surface. */
    if (!surface && !(surface = wayland_surface_create())) return;

    visible = (NtUserGetWindowLongW(data->hwnd, GWL_STYLE) & WS_VISIBLE) == WS_VISIBLE;
    xdg_visible = surface->xdg_toplevel != NULL;

    if (visible != xdg_visible)
    {
        pthread_mutex_lock(&surface->mutex);

        /* If we have a pre-existing surface ensure it has no role. */
        if (data->wayland_surface) wayland_surface_clear_role(surface);
        /* If the window is a visible toplevel make it a wayland
         * xdg_toplevel. Otherwise keep it role-less to avoid polluting the
         * compositor with empty xdg_toplevels. */
        if (visible) wayland_surface_make_toplevel(surface);

        pthread_mutex_unlock(&surface->mutex);
    }

out:
    TRACE("hwnd=%p surface=%p=>%p\n", data->hwnd, data->wayland_surface, surface);
    data->wayland_surface = surface;
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

    TRACE("hwnd %p window %s client %s visible %s after %p flags %08x\n",
          hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
          wine_dbgstr_rect(visible_rect), insert_after, swp_flags);

    if (!data && !(data = wayland_win_data_create(hwnd))) return TRUE;

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
    struct wayland_win_data *data = wayland_win_data_get(hwnd);

    TRACE("hwnd %p window %s client %s visible %s after %p flags %08x\n",
          hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
          wine_dbgstr_rect(visible_rect), insert_after, swp_flags);

    if (!data) return;

    wayland_win_data_update_wayland_surface(data);

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

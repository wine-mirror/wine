/*
 * Wayland surfaces
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
#include <unistd.h>

#include "waylanddrv.h"
#include "wine/debug.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

static void xdg_surface_handle_configure(void *private, struct xdg_surface *xdg_surface,
                                         uint32_t serial)
{
    struct wayland_surface *surface;
    BOOL should_post = FALSE, initial_configure = FALSE;
    struct wayland_win_data *data;
    HWND hwnd = private;

    TRACE("serial=%u\n", serial);

    if (!(data = wayland_win_data_get(hwnd))) return;

    /* Handle this event only if wayland_surface is still associated with
     * the target xdg_surface. */
    if ((surface = data->wayland_surface) && wayland_surface_is_toplevel(surface) &&
        surface->xdg_surface == xdg_surface)
    {
        /* If we have a previously requested config, we have already sent a
         * WM_WAYLAND_CONFIGURE which hasn't been handled yet. In that case,
         * avoid sending another message to reduce message queue traffic. */
        should_post = surface->requested.serial == 0;
        initial_configure = surface->current.serial == 0;
        surface->pending.serial = serial;
        surface->requested = surface->pending;
        memset(&surface->pending, 0, sizeof(surface->pending));
    }

    wayland_win_data_release(data);

    if (should_post) NtUserPostMessage(hwnd, WM_WAYLAND_CONFIGURE, 0, 0);

    /* Flush the window surface in case there is content that we weren't
     * able to flush before due to the lack of the initial configure. */
    if (initial_configure)
    {
        NtUserExposeWindowSurface(hwnd, 0, NULL);
    }
}

static const struct xdg_surface_listener xdg_surface_listener =
{
    xdg_surface_handle_configure
};

static void xdg_toplevel_handle_configure(void *private,
                                          struct xdg_toplevel *xdg_toplevel,
                                          int32_t width, int32_t height,
                                          struct wl_array *states)
{
    struct wayland_surface *surface;
    HWND hwnd = private;
    uint32_t *state;
    enum wayland_surface_config_state config_state = 0;
    struct wayland_win_data *data;

    wl_array_for_each(state, states)
    {
        switch(*state)
        {
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
            config_state |= WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED;
            break;
        case XDG_TOPLEVEL_STATE_RESIZING:
            config_state |= WAYLAND_SURFACE_CONFIG_STATE_RESIZING;
            break;
        case XDG_TOPLEVEL_STATE_TILED_LEFT:
        case XDG_TOPLEVEL_STATE_TILED_RIGHT:
        case XDG_TOPLEVEL_STATE_TILED_TOP:
        case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
            config_state |= WAYLAND_SURFACE_CONFIG_STATE_TILED;
            break;
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
            config_state |= WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN;
            break;
        default:
            break;
        }
    }

    TRACE("hwnd=%p %dx%d,%#x\n", hwnd, width, height, config_state);

    if (!(data = wayland_win_data_get(hwnd))) return;

    if ((surface = data->wayland_surface) && wayland_surface_is_toplevel(surface))
    {
        SetRect(&surface->pending.rect, 0, 0, width, height);
        surface->pending.state = config_state;
    }

    wayland_win_data_release(data);
}

static void xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
    NtUserPostMessage((HWND)data, WM_SYSCOMMAND, SC_CLOSE, 0);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener =
{
    xdg_toplevel_handle_configure,
    xdg_toplevel_handle_close
};

void wp_fractional_scale_handle_scale(void* user_data,
                                      struct wp_fractional_scale_v1 *fractional_scale_v1,
                                      uint32_t scale_fixed)
{
    struct wayland_win_data *data;
    struct wayland_surface *surface;
    double scale = scale_fixed / 120.0;
    HWND hwnd = user_data;

    TRACE("hwnd=%p scale=%lf\n", hwnd, scale);

    if (!(data = wayland_win_data_get(hwnd))) return;
    if (!(surface = data->wayland_surface) || scale == surface->window.scale)
    {
        wayland_win_data_release(data);
        return;
    }

    surface->window.scale = scale;

    /* reattach client surfaces as their rects have changed */
    update_client_surfaces(hwnd);

    /* the subsurface rect has changed */
    if (surface->role == WAYLAND_SURFACE_ROLE_SUBSURFACE)
    {
        surface->processing.serial = 1;
        surface->processing.processed = TRUE;
    }

    wayland_win_data_release(data);

    NtUserExposeWindowSurface(hwnd, 0, NULL);
}

static const struct wp_fractional_scale_v1_listener wp_fractional_scale_listener =
{
    wp_fractional_scale_handle_scale
};

/**********************************************************************
 *          wayland_surface_create
 *
 * Creates a role-less wayland surface.
 */
struct wayland_surface *wayland_surface_create(HWND hwnd)
{
    struct wayland_surface *surface;

    surface = calloc(1, sizeof(*surface));
    if (!surface)
    {
        ERR("Failed to allocate space for Wayland surface\n");
        goto err;
    }

    TRACE("surface=%p\n", surface);

    surface->hwnd = hwnd;
    surface->wl_surface = wl_compositor_create_surface(process_wayland.wl_compositor);
    if (!surface->wl_surface)
    {
        ERR("Failed to create wl_surface Wayland surface\n");
        goto err;
    }
    wl_surface_set_user_data(surface->wl_surface, hwnd);

    surface->wp_viewport =
        wp_viewporter_get_viewport(process_wayland.wp_viewporter,
                                   surface->wl_surface);
    if (!surface->wp_viewport)
    {
        ERR("Failed to create wp_viewport Wayland surface\n");
        goto err;
    }
    if (process_wayland.wp_alpha_modifier_v1)
    {
        COLORREF key;
        DWORD flags;
        BYTE alpha;

        surface->wp_alpha_modifier_surface_v1 =
            wp_alpha_modifier_v1_get_surface(process_wayland.wp_alpha_modifier_v1, surface->wl_surface);

        if (!NtUserGetLayeredWindowAttributes(hwnd, &key, &alpha, &flags)) flags = 0;
        wayland_surface_set_opacity(surface, alpha, flags);
    }

    surface->window.scale = 1.0;

    return surface;

err:
    if (surface) wayland_surface_destroy(surface);
    return NULL;
}

/**********************************************************************
 *          wayland_surface_destroy
 *
 * Destroys a wayland surface.
 */
void wayland_surface_destroy(struct wayland_surface *surface)
{
    pthread_mutex_lock(&process_wayland.pointer.mutex);
    if (process_wayland.pointer.focused_hwnd == surface->hwnd)
    {
        process_wayland.pointer.focused_hwnd = NULL;
        process_wayland.pointer.enter_serial = 0;
    }
    if (process_wayland.pointer.constraint_hwnd == surface->hwnd)
        wayland_pointer_clear_constraint();
    pthread_mutex_unlock(&process_wayland.pointer.mutex);

    pthread_mutex_lock(&process_wayland.keyboard.mutex);
    if (process_wayland.keyboard.focused_hwnd == surface->hwnd)
        process_wayland.keyboard.focused_hwnd = NULL;
    pthread_mutex_unlock(&process_wayland.keyboard.mutex);

    pthread_mutex_lock(&process_wayland.text_input.mutex);
    if (process_wayland.text_input.focused_hwnd == surface->hwnd)
        process_wayland.text_input.focused_hwnd = NULL;
    pthread_mutex_unlock(&process_wayland.text_input.mutex);

    wayland_surface_clear_role(surface);

    if (surface->wp_alpha_modifier_surface_v1)
    {
        wp_alpha_modifier_surface_v1_destroy(surface->wp_alpha_modifier_surface_v1);
        surface->wp_alpha_modifier_surface_v1 = NULL;
    }

    if (surface->wp_viewport)
    {
        wp_viewport_destroy(surface->wp_viewport);
        surface->wp_viewport = NULL;
    }

    if (surface->wl_surface)
    {
        wl_surface_destroy(surface->wl_surface);
        surface->wl_surface = NULL;
    }

    if (surface->big_icon_buffer)
    {
        wayland_shm_buffer_unref(surface->big_icon_buffer);
        surface->big_icon_buffer = NULL;
    }

    if (surface->small_icon_buffer)
    {
        wayland_shm_buffer_unref(surface->small_icon_buffer);
        surface->small_icon_buffer = NULL;
    }

    wl_display_flush(process_wayland.wl_display);

    free(surface);
}

static void wayland_surface_init_fractional_scale(struct wayland_surface *surface,
                                                  double initial_scale)
{
    surface->window.scale = initial_scale;

    if (!process_wayland.wp_fractional_scale_manager_v1) return;

    surface->wp_fractional_scale_v1 =
        wp_fractional_scale_manager_v1_get_fractional_scale(
            process_wayland.wp_fractional_scale_manager_v1,
            surface->wl_surface);
    if (!surface->wp_fractional_scale_v1)
    {
        ERR("Failed to create wp_fractional_scale_v1\n");
        return;
    }
    wp_fractional_scale_v1_add_listener(
        surface->wp_fractional_scale_v1,
        &wp_fractional_scale_listener,
        surface->hwnd);
}

/**********************************************************************
 *          wayland_surface_make_toplevel
 *
 * Gives the toplevel role to a plain wayland surface.
 */
void wayland_surface_make_toplevel(struct wayland_surface *surface)
{
    WCHAR text[1024];

    TRACE("surface=%p\n", surface);

    assert(!surface->role || surface->role == WAYLAND_SURFACE_ROLE_TOPLEVEL);
    if (surface->xdg_surface && surface->xdg_toplevel) return;

    wayland_surface_clear_role(surface);
    surface->role = WAYLAND_SURFACE_ROLE_TOPLEVEL;

    surface->xdg_surface =
        xdg_wm_base_get_xdg_surface(process_wayland.xdg_wm_base, surface->wl_surface);
    if (!surface->xdg_surface) goto err;
    xdg_surface_add_listener(surface->xdg_surface, &xdg_surface_listener, surface->hwnd);

    surface->xdg_toplevel = xdg_surface_get_toplevel(surface->xdg_surface);
    if (!surface->xdg_toplevel) goto err;
    xdg_toplevel_add_listener(surface->xdg_toplevel, &xdg_toplevel_listener, surface->hwnd);

    if (process_name)
        xdg_toplevel_set_app_id(surface->xdg_toplevel, process_name);

    if (!NtUserInternalGetWindowText(surface->hwnd, text, ARRAY_SIZE(text)))
        text[0] = 0;
    wayland_surface_set_title(surface, text);

    wayland_surface_assign_icon(surface);

    wayland_surface_init_fractional_scale(surface, 1.0);

    wl_surface_commit(surface->wl_surface);
    wl_display_flush(process_wayland.wl_display);

    return;

err:
    wayland_surface_clear_role(surface);
    ERR("Failed to assign toplevel role to wayland surface\n");
}

/**********************************************************************
 *          wayland_surface_make_subsurface
 *
 * Gives the subsurface role to a plain wayland surface.
 */
void wayland_surface_make_subsurface(struct wayland_surface *surface,
                                     struct wayland_surface *owner)
{
    assert(!surface->role || surface->role == WAYLAND_SURFACE_ROLE_SUBSURFACE);
    if (surface->wl_subsurface && surface->owner_hwnd == owner->hwnd) return;

    wayland_surface_clear_role(surface);
    surface->role = WAYLAND_SURFACE_ROLE_SUBSURFACE;

    TRACE("surface=%p owner=%p\n", surface, owner);

    surface->wl_subsurface =
        wl_subcompositor_get_subsurface(process_wayland.wl_subcompositor,
                                        surface->wl_surface,
                                        owner->wl_surface);
    if (!surface->wl_subsurface)
    {
        ERR("Failed to create client wl_subsurface\n");
        goto err;
    }

    wayland_surface_init_fractional_scale(surface, owner->window.scale);

    surface->role = WAYLAND_SURFACE_ROLE_SUBSURFACE;
    surface->owner_hwnd = owner->hwnd;

    /* Present contents independently of the owner surface. */
    wl_subsurface_set_desync(surface->wl_subsurface);

    wl_display_flush(process_wayland.wl_display);

    return;

err:
    wayland_surface_clear_role(surface);
    ERR("Failed to assign subsurface role to wayland surface\n");
}

/**********************************************************************
 *          wayland_surface_clear_role
 *
 * Clears the role related Wayland objects of a Wayland surface, making it a
 * plain surface again. We can later assign the same role (but not a
 * different one!) to the surface.
 */
void wayland_surface_clear_role(struct wayland_surface *surface)
{
    TRACE("surface=%p\n", surface);

    /* some objects are shared between several roles */

    if (surface->wp_fractional_scale_v1)
    {
        wp_fractional_scale_v1_destroy(surface->wp_fractional_scale_v1);
        surface->wp_fractional_scale_v1 = NULL;
    }

    switch (surface->role)
    {
    case WAYLAND_SURFACE_ROLE_NONE:
        break;

    case WAYLAND_SURFACE_ROLE_TOPLEVEL:
        if (surface->xdg_toplevel_icon)
        {
            xdg_toplevel_icon_manager_v1_set_icon(
                process_wayland.xdg_toplevel_icon_manager_v1,
                surface->xdg_toplevel, NULL);
            xdg_toplevel_icon_v1_destroy(surface->xdg_toplevel_icon);
            surface->xdg_toplevel_icon = NULL;
        }

        if (surface->xdg_toplevel)
        {
            xdg_toplevel_destroy(surface->xdg_toplevel);
            surface->xdg_toplevel = NULL;
        }

        if (surface->xdg_surface)
        {
            xdg_surface_destroy(surface->xdg_surface);
            surface->xdg_surface = NULL;
        }
        break;

    case WAYLAND_SURFACE_ROLE_SUBSURFACE:
        if (surface->wl_subsurface)
        {
            wl_subsurface_destroy(surface->wl_subsurface);
            surface->wl_subsurface = NULL;
        }

        surface->owner_hwnd = NULL;
        break;
    }

    memset(&surface->pending, 0, sizeof(surface->pending));
    memset(&surface->requested, 0, sizeof(surface->requested));
    memset(&surface->processing, 0, sizeof(surface->processing));
    memset(&surface->current, 0, sizeof(surface->current));

    /* Ensure no buffer is attached, otherwise future role assignments may fail. */
    wl_surface_attach(surface->wl_surface, NULL, 0, 0);
    wl_surface_commit(surface->wl_surface);

    surface->content_width = 0;
    surface->content_height = 0;

    wl_display_flush(process_wayland.wl_display);
}

/**********************************************************************
 *          wayland_surface_attach_shm
 *
 * Attaches a SHM buffer to a wayland surface.
 *
 * The buffer is marked as unavailable until committed and subsequently
 * released by the compositor.
 */
void wayland_surface_attach_shm(struct wayland_surface *surface,
                                struct wayland_shm_buffer *shm_buffer,
                                HRGN surface_damage_region)
{
    RGNDATA *surface_damage;
    int win_width, win_height;

    TRACE("surface=%p shm_buffer=%p (%dx%d)\n",
          surface, shm_buffer, shm_buffer->width, shm_buffer->height);

    shm_buffer->busy = TRUE;
    wayland_shm_buffer_ref(shm_buffer);

    wl_surface_attach(surface->wl_surface, shm_buffer->wl_buffer, 0, 0);

    /* Add surface damage, i.e., which parts of the surface have changed since
     * the last surface commit. Note that this is different from the buffer
     * damage region. */
    surface_damage = get_region_data(surface_damage_region);
    if (surface_damage)
    {
        RECT *rgn_rect = (RECT *)surface_damage->Buffer;
        RECT *rgn_rect_end = rgn_rect + surface_damage->rdh.nCount;

        for (;rgn_rect < rgn_rect_end; rgn_rect++)
        {
            wl_surface_damage_buffer(surface->wl_surface,
                                     rgn_rect->left, rgn_rect->top,
                                     rgn_rect->right - rgn_rect->left,
                                     rgn_rect->bottom - rgn_rect->top);
        }
        free(surface_damage);
    }

    win_width = surface->window.rect.right - surface->window.rect.left;
    win_height = surface->window.rect.bottom - surface->window.rect.top;

    /* It is an error to specify a wp_viewporter source rectangle that
     * is partially or completely outside of the wl_buffe.
     * 0 is also an invalid width / height value so use 1x1 instead.
     */
    win_width = max(1, min(win_width, shm_buffer->width));
    win_height = max(1, min(win_height, shm_buffer->height));

    wp_viewport_set_source(surface->wp_viewport, 0, 0,
                           wl_fixed_from_int(win_width),
                           wl_fixed_from_int(win_height));

    surface->content_width = win_width;
    surface->content_height = win_height;
}

/**********************************************************************
 *          wayland_surface_config_is_compatible
 *
 * Checks whether a wayland_surface_config object is compatible with the
 * the provided arguments.
 */
BOOL wayland_surface_config_is_compatible(struct wayland_surface_config *conf, RECT rect,
                                          enum wayland_surface_config_state state)
{
    static enum wayland_surface_config_state mask =
        WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED;

    /* The fullscreen state requires a size smaller or equal to the configured
     * size. If we have a larger size, we can use surface geometry during
     * surface reconfiguration to provide the smaller size, so we are always
     * compatible with a fullscreen state.
     * NOTE: Fullscreen combined with maximized is the same as fullscreen. */
    if (conf->state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN)
        return TRUE;

    /* We require the same state. */
    if ((state & mask) != (conf->state & mask)) return FALSE;

    /* The maximized state requires the configured size. During surface
     * reconfiguration we can use surface geometry to provide smaller areas
     * from larger sizes, so only smaller sizes are incompatible. */
    if ((conf->state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED) &&
        (rect.right - rect.left < conf->rect.right - conf->rect.left ||
         rect.bottom - rect.top < conf->rect.bottom - conf->rect.top))
    {
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 *          wayland_surface_get_rect_in_monitor
 *
 * Gets the largest rectangle within a surface's window (in window coordinates)
 * that is visible in a monitor.
 */
static void wayland_surface_get_rect_in_monitor(struct wayland_surface *surface,
                                                RECT *rect)
{
    HMONITOR hmonitor;
    MONITORINFO mi;

    mi.cbSize = sizeof(mi);
    if (!(hmonitor = NtUserMonitorFromRect(&surface->window.rect, 0)) ||
        !NtUserGetMonitorInfo(hmonitor, (MONITORINFO *)&mi))
    {
        SetRectEmpty(rect);
        return;
    }

    intersect_rect(rect, &mi.rcMonitor, &surface->window.rect);
    OffsetRect(rect, -surface->window.rect.left, -surface->window.rect.top);
}

/**********************************************************************
 *          wayland_surface_reconfigure_geometry
 *
 * Sets the xdg_surface geometry
 */
static void wayland_surface_reconfigure_geometry(struct wayland_surface *surface, RECT rect)
{
    /* If the window size is bigger than the current state accepts, use the
     * largest visible (from Windows' perspective) subregion of the window. */
    if ((surface->current.state & (WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED |
                                   WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN)) &&
        (rect.right - rect.left > surface->current.rect.right - surface->current.rect.left ||
         rect.bottom - rect.top > surface->current.rect.bottom - surface->current.rect.top))
    {
        wayland_surface_get_rect_in_monitor(surface, &rect);

        rect = map_rect_to_surface(surface, rect);

        /* If the window rect in the monitor is smaller than required,
         * fall back to an appropriately sized rect at the top-left. */
        if ((surface->current.state & WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED) &&
            !(surface->current.state & WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN) &&
            (rect.right - rect.left < surface->current.rect.right - surface->current.rect.left ||
             rect.bottom - rect.top < surface->current.rect.bottom - surface->current.rect.top))
        {
            SetRect(&rect, 0, 0, surface->current.rect.right - surface->current.rect.left,
                    surface->current.rect.bottom - surface->current.rect.top);
        }
        else
        {
            rect.right = min(rect.right, rect.left + surface->current.rect.right - surface->current.rect.left);
            rect.bottom = min(rect.bottom, rect.top + surface->current.rect.bottom - surface->current.rect.top);
        }
        TRACE("Window is too large for Wayland state, using subregion\n");
    }
    else
    {
        OffsetRect(&rect, -rect.left, -rect.top);
    }

    TRACE("hwnd=%p geometry=%s\n", surface->hwnd, wine_dbgstr_rect(&rect));

    if (!IsRectEmpty(&rect))
    {
        int width = rect.right - rect.left, height = rect.bottom - rect.top;
        xdg_surface_set_window_geometry(surface->xdg_surface,
                                        rect.left, rect.top,
                                        width, height);
        if (surface->window.resizeable)
        {
            xdg_toplevel_set_min_size(surface->xdg_toplevel, 0, 0);
            xdg_toplevel_set_max_size(surface->xdg_toplevel, 0, 0);
        }
        else
        {
            xdg_toplevel_set_min_size(surface->xdg_toplevel, width, height);
            xdg_toplevel_set_max_size(surface->xdg_toplevel, width, height);
        }
    }
}

/**********************************************************************
 *          wayland_surface_reconfigure_size
 *
 * Sets the surface size with viewporter
 */
static void wayland_surface_reconfigure_size(struct wayland_surface *surface,
                                             int width, int height)
{
    TRACE("hwnd=%p size=%dx%d\n", surface->hwnd, width, height);

    if (width != 0 && height != 0)
        wp_viewport_set_destination(surface->wp_viewport, width, height);
    else
        wp_viewport_set_destination(surface->wp_viewport, -1, -1);
}

/**********************************************************************
 *          wayland_surface_reconfigure_client
 *
 * Reconfigures the subsurface covering the client area.
 */
static void wayland_surface_reconfigure_client(struct wayland_surface *surface,
                                               struct wayland_client_surface *client,
                                               const RECT *client_rect)
{
    RECT rect = client_rect ? *client_rect : client->rect;

    /* The offset of the client area origin relatively to the window origin. */
    rect = map_rect_to_surface(surface, rect);

    TRACE("hwnd=%p rect=%s\n", surface->hwnd, wine_dbgstr_rect(&rect));

    if (client->wl_subsurface)
    {
        wl_subsurface_set_position(client->wl_subsurface, rect.left, rect.top);
        wl_subsurface_place_above(client->wl_subsurface, surface->wl_surface);
    }

    if (rect.left != rect.right && rect.top != rect.bottom)
        wp_viewport_set_destination(client->wp_viewport, rect.right - rect.left, rect.bottom - rect.top);
    else /* We can't have a 0x0 destination, use 1x1 instead. */
        wp_viewport_set_destination(client->wp_viewport, 1, 1);

    client->rect = *client_rect;
}

/**********************************************************************
 *          wayland_surface_reconfigure_xdg
 *
 * Reconfigures the xdg surface as needed to match the latest requested
 * state.
 */
static BOOL wayland_surface_reconfigure_xdg(struct wayland_surface *surface, RECT rect)
{
    struct wayland_window_config *window = &surface->window;

    /* Acknowledge any compatible processed config. */
    if (surface->processing.serial && surface->processing.processed &&
        wayland_surface_config_is_compatible(&surface->processing, rect,
                                             window->state))
    {
        surface->current = surface->processing;
        memset(&surface->processing, 0, sizeof(surface->processing));
        xdg_surface_ack_configure(surface->xdg_surface, surface->current.serial);
    }
    /* If this is the initial configure, and we have a compatible requested
     * config, use that, in order to draw windows that don't go through the
     * message loop (e.g., some splash screens). */
    else if (!surface->current.serial && surface->requested.serial &&
             wayland_surface_config_is_compatible(&surface->requested, rect,
                                                  window->state))
    {
        surface->current = surface->requested;
        memset(&surface->requested, 0, sizeof(surface->requested));
        xdg_surface_ack_configure(surface->xdg_surface, surface->current.serial);
    }
    else if (!surface->current.serial ||
             !wayland_surface_config_is_compatible(&surface->current, rect,
                                                   window->state))
    {
        return FALSE;
    }

    wayland_surface_reconfigure_geometry(surface, rect);

    return TRUE;
}

/**********************************************************************
 *          wayland_surface_reconfigure_subsurface
 *
 * Reconfigures the subsurface as needed to match the latest requested
 * state.
 */
static void wayland_surface_reconfigure_subsurface(struct wayland_surface *surface)
{
    struct wayland_win_data *owner_data;
    struct wayland_surface *owner_surface;

    if (!surface->processing.serial || !surface->processing.processed) return;
    if (!(owner_data = wayland_win_data_get(surface->owner_hwnd))) return;

    if ((owner_surface = owner_data->wayland_surface))
    {
        RECT rect = surface->window.rect;

        OffsetRect(&rect, -owner_surface->window.rect.left, -owner_surface->window.rect.top);
        rect = map_rect_to_surface(surface, rect);

        TRACE("hwnd=%p rect=%s\n", surface->hwnd, wine_dbgstr_rect(&rect));

        wl_subsurface_set_position(surface->wl_subsurface, rect.left, rect.top);
        if (owner_data->client_surface && owner_data->client_surface->wl_subsurface)
            wl_subsurface_place_above(surface->wl_subsurface, owner_data->client_surface->wl_surface);
        else
            wl_subsurface_place_above(surface->wl_subsurface, owner_surface->wl_surface);
        wl_surface_commit(owner_surface->wl_surface);

        memset(&surface->processing, 0, sizeof(surface->processing));
    }

    wayland_win_data_release(owner_data);
}

/**********************************************************************
 *          wayland_surface_reconfigure
 *
 * Reconfigures the wayland surface as needed to match the latest requested
 * state.
 */
BOOL wayland_surface_reconfigure(struct wayland_surface *surface)
{
    struct wayland_window_config *window = &surface->window;
    RECT rect = map_rect_to_surface(surface, surface->window.rect);

    TRACE("hwnd=%p window=%s,%#x processing=%s,%#x current=%s,%#x\n",
          surface->hwnd, wine_dbgstr_rect(&rect), window->state,
          wine_dbgstr_rect(&surface->processing.rect), surface->processing.state,
          wine_dbgstr_rect(&surface->current.rect), surface->current.state);

    switch (surface->role)
    {
    case WAYLAND_SURFACE_ROLE_NONE:
        break;
    case WAYLAND_SURFACE_ROLE_TOPLEVEL:
        if (!surface->xdg_surface) break; /* surface role has been cleared */
        if (!wayland_surface_reconfigure_xdg(surface, rect)) return FALSE;
        break;
    case WAYLAND_SURFACE_ROLE_SUBSURFACE:
        if (!surface->wl_subsurface) break; /* surface role has been cleared */
        wayland_surface_reconfigure_subsurface(surface);
        break;
    }

    wayland_surface_reconfigure_size(surface, rect.right - rect.left, rect.bottom - rect.top);

    return TRUE;
}

/**********************************************************************
 *          wayland_shm_buffer_ref
 *
 * Increases the reference count of a SHM buffer.
 */
void wayland_shm_buffer_ref(struct wayland_shm_buffer *shm_buffer)
{
    InterlockedIncrement(&shm_buffer->ref);
}

/**********************************************************************
 *          wayland_shm_buffer_unref
 *
 * Decreases the reference count of a SHM buffer (and may destroy it).
 */
void wayland_shm_buffer_unref(struct wayland_shm_buffer *shm_buffer)
{
    if (InterlockedDecrement(&shm_buffer->ref) > 0) return;

    TRACE("destroying %p map=%p\n", shm_buffer, shm_buffer->map_data);

    if (shm_buffer->wl_buffer)
        wl_buffer_destroy(shm_buffer->wl_buffer);
    if (shm_buffer->map_data)
        NtUnmapViewOfSection(GetCurrentProcess(), shm_buffer->map_data);
    if (shm_buffer->damage_region)
        NtGdiDeleteObjectApp(shm_buffer->damage_region);

    free(shm_buffer);
}

/**********************************************************************
 *          wayland_shm_buffer_create
 *
 * Creates a SHM buffer with the specified width, height and format.
 */
struct wayland_shm_buffer *wayland_shm_buffer_create(int width, int height,
                                                     enum wl_shm_format format)
{
    struct wayland_shm_buffer *shm_buffer = NULL;
    HANDLE handle = 0;
    int fd = -1;
    SIZE_T view_size = 0;
    LARGE_INTEGER section_size;
    NTSTATUS status;
    struct wl_shm_pool *pool;
    int stride, size;

    stride = width * WINEWAYLAND_BYTES_PER_PIXEL;
    size = stride * height;
    if (size == 0)
    {
        ERR("Invalid shm_buffer size %dx%d\n", width, height);
        goto err;
    }

    shm_buffer = calloc(1, sizeof(*shm_buffer));
    if (!shm_buffer)
    {
        ERR("Failed to allocate space for SHM buffer\n");
        goto err;
    }

    TRACE("%p %dx%d format=%d size=%d\n", shm_buffer, width, height, format, size);

    shm_buffer->ref = 1;
    shm_buffer->width = width;
    shm_buffer->height = height;
    shm_buffer->format = format;
    shm_buffer->map_size = size;

    shm_buffer->damage_region = NtGdiCreateRectRgn(0, 0, width, height);
    if (!shm_buffer->damage_region)
    {
        ERR("Failed to create buffer damage region\n");
        goto err;
    }

    section_size.QuadPart = size;
    status = NtCreateSection(&handle,
                             GENERIC_READ | SECTION_MAP_READ | SECTION_MAP_WRITE,
                             NULL, &section_size, PAGE_READWRITE, SEC_COMMIT, 0);
    if (status)
    {
        ERR("Failed to create SHM section status=0x%x\n", status);
        goto err;
    }

    status = NtMapViewOfSection(handle, GetCurrentProcess(),
                                (PVOID)&shm_buffer->map_data, 0, 0, NULL,
                                &view_size, ViewUnmap, 0, PAGE_READWRITE);
    if (status)
    {
        shm_buffer->map_data = NULL;
        ERR("Failed to create map SHM handle status=0x%x\n", status);
        goto err;
    }

    status = wine_server_handle_to_fd(handle, FILE_READ_DATA, &fd, NULL);
    if (status)
    {
        ERR("Failed to get fd from SHM handle status=0x%x\n", status);
        goto err;
    }

    pool = wl_shm_create_pool(process_wayland.wl_shm, fd, size);
    if (!pool)
    {
        ERR("Failed to create SHM pool fd=%d size=%d\n", fd, size);
        goto err;
    }
    shm_buffer->wl_buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
                                                      stride, format);
    wl_shm_pool_destroy(pool);
    if (!shm_buffer->wl_buffer)
    {
        ERR("Failed to create SHM buffer %dx%d\n", width, height);
        goto err;
    }

    close(fd);
    NtClose(handle);

    TRACE("=> map=%p\n", shm_buffer->map_data);

    return shm_buffer;

err:
    if (fd >= 0) close(fd);
    if (handle) NtClose(handle);
    if (shm_buffer) wayland_shm_buffer_unref(shm_buffer);
    return NULL;
}

/***********************************************************************
 *           copy_rectangle_into_center_of_square
 *
 * Copies non-square rectangle src to the center of square dest.
 */
static void copy_rectangle_into_center_of_square(const unsigned int *src,
                                                 int src_w, int src_h,
                                                 unsigned int *dest)
{
    int dest_length;

    if (src_w > src_h)
    {
        dest += src_w * (src_w - src_h) / 2;
        dest_length = src_w;
    }
    else
    {
        dest += (src_h - src_w) / 2;
        dest_length = src_h;
    }

    for (int h = 0; h < src_h; h++, dest += dest_length, src += src_w)
        memcpy(dest, src, src_w * 4);
}

/***********************************************************************
 *           wayland_shm_buffer_from_color_bitmaps
 *
 * Create a wayland_shm_buffer for a color bitmap.
 *
 * Adapted from wineandroid.drv code.
 */
struct wayland_shm_buffer *wayland_shm_buffer_from_color_bitmaps(HDC hdc, HBITMAP color,
                                                                 HBITMAP mask,
                                                                 BOOL allow_padding)
{
    struct wayland_shm_buffer *shm_buffer = NULL;
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    BITMAP bm;
    unsigned int *ptr, *bits = NULL;
    unsigned char *mask_bits = NULL;
    int i, j, square_length;
    BOOL has_alpha = FALSE, use_padding = FALSE;

    if (!NtGdiExtGetObjectW(color, sizeof(bm), &bm)) goto failed;

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = bm.bmWidth;
    info->bmiHeader.biHeight = -bm.bmHeight;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;

    use_padding = allow_padding && bm.bmWidth != bm.bmHeight;

    if (use_padding)
    {
        square_length = max(bm.bmWidth, bm.bmHeight);
        shm_buffer = wayland_shm_buffer_create(square_length, square_length,
                                               WL_SHM_FORMAT_ARGB8888);
        if (!shm_buffer) goto failed;
        if (!(bits = malloc(info->bmiHeader.biSizeImage))) goto failed;
    }
    else
    {
        shm_buffer = wayland_shm_buffer_create(bm.bmWidth, bm.bmHeight,
                                               WL_SHM_FORMAT_ARGB8888);
        if (!shm_buffer) goto failed;
        bits = shm_buffer->map_data;
    }

    if (!NtGdiGetDIBitsInternal(hdc, color, 0, bm.bmHeight, bits, info,
                                DIB_RGB_COLORS, 0, 0))
        goto failed;

    for (i = 0; i < bm.bmWidth * bm.bmHeight; i++)
        if ((has_alpha = (bits[i] & 0xff000000) != 0)) break;

    if (!has_alpha)
    {
        unsigned int width_bytes = (bm.bmWidth + 31) / 32 * 4;
        /* generate alpha channel from the mask */
        info->bmiHeader.biBitCount = 1;
        info->bmiHeader.biSizeImage = width_bytes * bm.bmHeight;
        if (!(mask_bits = malloc(info->bmiHeader.biSizeImage))) goto failed;
        if (!NtGdiGetDIBitsInternal(hdc, mask, 0, bm.bmHeight, mask_bits,
                                    info, DIB_RGB_COLORS, 0, 0))
            goto failed;
        ptr = bits;
        for (i = 0; i < bm.bmHeight; i++)
        {
            for (j = 0; j < bm.bmWidth; j++, ptr++)
            {
                if (!((mask_bits[i * width_bytes + j / 8] << (j % 8)) & 0x80))
                    *ptr |= 0xff000000;
            }
        }
        free(mask_bits);
    }

    if (use_padding)
    {
        copy_rectangle_into_center_of_square(bits, bm.bmWidth,
                                             bm.bmHeight, shm_buffer->map_data);
        free(bits);
        bits = shm_buffer->map_data;
    }

    /* Wayland requires pre-multiplied alpha values */
    for (ptr = bits, i = 0; i < shm_buffer->width * shm_buffer->height; ptr++, i++)
    {
        unsigned char alpha = *ptr >> 24;
        if (alpha == 0)
        {
            *ptr = 0;
        }
        else if (alpha != 255)
        {
            *ptr = (alpha << 24) |
                   (((BYTE)(*ptr >> 16) * alpha / 255) << 16) |
                   (((BYTE)(*ptr >> 8) * alpha / 255) << 8) |
                   (((BYTE)*ptr * alpha / 255));
        }
    }

    return shm_buffer;

failed:
    if (shm_buffer) wayland_shm_buffer_unref(shm_buffer);
    if (use_padding) free(bits);
    free(mask_bits);
    return NULL;
}

/**********************************************************************
 *          map_rect_to_surface
 *
 * Converts the window (logical) coordinates to wayland surface-local coordinates.
 */
RECT map_rect_to_surface(struct wayland_surface *surface, RECT rect)
{
    rect.left = round(rect.left / surface->window.scale);
    rect.top  = round(rect.top / surface->window.scale);
    rect.right = round(rect.right / surface->window.scale);
    rect.bottom  = round(rect.bottom / surface->window.scale);
    return rect;
}

/**********************************************************************
 *          map_point_to_surface
 *
 * Converts the window (logical) coordinates to wayland surface-local coordinates.
 */
POINT map_point_to_surface(struct wayland_surface *surface, POINT point)
{
    point.x = round(point.x / surface->window.scale);
    point.y  = round(point.y / surface->window.scale);
    return point;
}

/**********************************************************************
 *          map_rect_from_surface
 *
 * Converts the surface-local coordinates to window (logical) coordinates.
 */
RECT map_rect_from_surface(struct wayland_surface *surface, RECT rect)
{
    rect.left = round(rect.left * surface->window.scale);
    rect.top  = round(rect.top * surface->window.scale);
    rect.right = round(rect.right * surface->window.scale);
    rect.bottom  = round(rect.bottom * surface->window.scale);
    return rect;
}

/**********************************************************************
 *          map_point_from_surface
 *
 * Converts the surface-local coordinates to window (logical) coordinates.
 */
POINT map_point_from_surface(struct wayland_surface *surface, POINT point)
{
    point.x = round(point.x * surface->window.scale);
    point.y = round(point.y * surface->window.scale);
    return point;
}

static void wayland_client_surface_destroy(struct client_surface *client)
{
    struct wayland_client_surface *surface = impl_from_client_surface(client);

    TRACE("%s\n", debugstr_client_surface(client));

    if (surface->wp_viewport)
        wp_viewport_destroy(surface->wp_viewport);
    if (surface->wl_subsurface)
        wl_subsurface_destroy(surface->wl_subsurface);
    if (surface->wl_surface)
        wl_surface_destroy(surface->wl_surface);
}

static void wayland_client_surface_detach(struct client_surface *client)
{
    struct wayland_client_surface *surface = impl_from_client_surface(client);
    struct wayland_win_data *data;

    if ((data = wayland_win_data_get(client->hwnd)))
    {
        if (data->client_surface == surface) data->client_surface = NULL;
        wayland_client_surface_attach(surface, NULL, NULL);
        wayland_win_data_release(data);
    }
}

static void wayland_client_surface_update(struct client_surface *client)
{
    struct wayland_client_surface *surface = impl_from_client_surface(client);
    HWND hwnd = client->hwnd, toplevel = client->toplevel;
    struct wayland_win_data *data;

    TRACE("%s\n", debugstr_client_surface(client));

    if (!(data = wayland_win_data_get(hwnd))) return;

    if (toplevel && NtUserIsWindowVisible(hwnd))
        wayland_client_surface_attach(surface, toplevel, &client->monitor_rect);
    else
        wayland_client_surface_attach(surface, NULL, NULL);

    wayland_win_data_release(data);
}

static void wayland_client_surface_present(struct client_surface *client, HDC hdc)
{
    struct wayland_client_surface *surface = impl_from_client_surface(client);
    HWND hwnd = client->hwnd, toplevel = client->toplevel;
    struct wayland_surface *wayland_surface;
    struct wayland_win_data *data;

    if (!(data = wayland_win_data_get(toplevel))) return;

    if ((wayland_surface = data->wayland_surface))
    {
        wayland_surface_ensure_contents(wayland_surface);

        /* Handle any processed configure request, to ensure the related
         * surface state is applied by the compositor. */
        if (wayland_surface->processing.serial &&
            wayland_surface->processing.processed &&
            wayland_surface_reconfigure(wayland_surface))
        {
            wl_surface_commit(wayland_surface->wl_surface);
        }
    }

    wayland_win_data_release(data);

    set_client_surface(hwnd, surface);
}

static const struct client_surface_funcs wayland_client_surface_funcs =
{
    .destroy = wayland_client_surface_destroy,
    .detach = wayland_client_surface_detach,
    .update = wayland_client_surface_update,
    .present = wayland_client_surface_present,
};

struct wayland_client_surface *impl_from_client_surface(struct client_surface *client)
{
    assert(client->funcs == &wayland_client_surface_funcs);
    return CONTAINING_RECORD(client, struct wayland_client_surface, client);
}

struct client_surface *WAYLAND_CreateClientSurface(HWND hwnd, int pixel_format)
{
    struct wayland_client_surface *client;
    struct wl_region *empty_region;

    if (!(client = client_surface_create(sizeof(*client), &wayland_client_surface_funcs, hwnd))) return NULL;

    client->wl_surface =
        wl_compositor_create_surface(process_wayland.wl_compositor);
    if (!client->wl_surface)
    {
        ERR("Failed to create client wl_surface\n");
        goto err;
    }
    wl_surface_set_user_data(client->wl_surface, hwnd);

    /* Let parent handle all pointer events. */
    empty_region = wl_compositor_create_region(process_wayland.wl_compositor);
    if (!empty_region)
    {
        ERR("Failed to create wl_region\n");
        goto err;
    }
    wl_surface_set_input_region(client->wl_surface, empty_region);
    wl_region_destroy(empty_region);

    client->wp_viewport =
        wp_viewporter_get_viewport(process_wayland.wp_viewporter,
                                    client->wl_surface);
    if (!client->wp_viewport)
    {
        ERR("Failed to create client wp_viewport\n");
        goto err;
    }

    return &client->client;

err:
    client_surface_release(&client->client);
    return NULL;
}

void wayland_client_surface_attach(struct wayland_client_surface *client, HWND toplevel, const RECT *rect)
{
    struct wayland_win_data *toplevel_data;
    struct wayland_surface *surface;

    if (!toplevel)
    {
        if (client->wl_subsurface)
        {
            wl_subsurface_destroy(client->wl_subsurface);
            client->wl_subsurface = NULL;
        }

        client->toplevel = 0;
        return;
    }

    if (!(toplevel_data = wayland_win_data_get(toplevel)) || !(surface = toplevel_data->wayland_surface))
    {
        if (toplevel_data) wayland_win_data_release(toplevel_data);
        return wayland_client_surface_attach(client, NULL, NULL);
    }

    if (client->toplevel != toplevel)
    {
        wayland_client_surface_attach(client, NULL, NULL);

        client->wl_subsurface =
            wl_subcompositor_get_subsurface(process_wayland.wl_subcompositor,
                                            client->wl_surface,
                                            surface->wl_surface);
        if (!client->wl_subsurface) goto done;

        /* Present contents independently of the parent surface. */
        wl_subsurface_set_desync(client->wl_subsurface);

        client->toplevel = toplevel;
    }

    wayland_surface_reconfigure_client(surface, client, rect);
    /* Commit to apply subsurface positioning. */
    wl_surface_commit(surface->wl_surface);

done:
    wayland_win_data_release(toplevel_data);
}

static void dummy_buffer_release(void *data, struct wl_buffer *buffer)
{
    struct wayland_shm_buffer *shm_buffer = data;
    TRACE("shm_buffer=%p\n", shm_buffer);
    wayland_shm_buffer_unref(shm_buffer);
}

static const struct wl_buffer_listener dummy_buffer_listener =
{
    dummy_buffer_release
};

/**********************************************************************
 *          wayland_surface_ensure_contents
 *
 * Ensure that the wayland surface has up-to-date contents, by committing
 * a dummy buffer if necessary.
 */
void wayland_surface_ensure_contents(struct wayland_surface *surface)
{
    struct wayland_shm_buffer *dummy_shm_buffer;
    HRGN damage;
    int width, height;
    BOOL needs_contents;

    width = surface->window.rect.right - surface->window.rect.left;
    height = surface->window.rect.bottom - surface->window.rect.top;
    needs_contents = surface->window.visible &&
                     (surface->content_width != width ||
                      surface->content_height != height);

    TRACE("surface=%p hwnd=%p needs_contents=%d\n",
          surface, surface->hwnd, needs_contents);

    if (!needs_contents) return;

    /* Create a transparent dummy buffer. */
    dummy_shm_buffer = wayland_shm_buffer_create(width, height, WL_SHM_FORMAT_ARGB8888);
    if (!dummy_shm_buffer)
    {
        ERR("Failed to create dummy buffer\n");
        return;
    }
    wl_buffer_add_listener(dummy_shm_buffer->wl_buffer, &dummy_buffer_listener,
                           dummy_shm_buffer);

    if (!(damage = NtGdiCreateRectRgn(0, 0, width, height)))
        WARN("Failed to create damage region for dummy buffer\n");

    if (wayland_surface_reconfigure(surface))
    {
        wayland_surface_attach_shm(surface, dummy_shm_buffer, damage);
        wl_surface_commit(surface->wl_surface);
    }
    else
    {
        wayland_shm_buffer_unref(dummy_shm_buffer);
    }

    if (damage) NtGdiDeleteObjectApp(damage);
}

/**********************************************************************
 *          wayland_surface_set_title
 */
void wayland_surface_set_title(struct wayland_surface *surface, LPCWSTR text)
{
    DWORD text_len;
    DWORD utf8_count;
    char *utf8 = NULL;

    assert(wayland_surface_is_toplevel(surface));

    TRACE("surface=%p hwnd=%p text='%s'\n",
          surface, surface->hwnd, wine_dbgstr_w(text));

    text_len = (lstrlenW(text) + 1) * sizeof(WCHAR);

    if (!RtlUnicodeToUTF8N(NULL, 0, &utf8_count, text, text_len) &&
        (utf8 = malloc(utf8_count)))
    {
        RtlUnicodeToUTF8N(utf8, utf8_count, &utf8_count, text, text_len);
        xdg_toplevel_set_title(surface->xdg_toplevel, utf8);
    }

    free(utf8);
}

/**********************************************************************
 *          wayland_surface_set_icon_buffer
 */
void wayland_surface_set_icon_buffer(struct wayland_surface *surface, UINT type, const ICONINFO *ii)
{
    struct wayland_shm_buffer *icon_buf;
    HDC hDC;

    if (!process_wayland.xdg_toplevel_icon_manager_v1) return;

    assert(ii);

    TRACE("surface=%p type=%x ii=%p\n", surface, type, ii);

    hDC = NtGdiCreateCompatibleDC(0);
    icon_buf = wayland_shm_buffer_from_color_bitmaps(hDC, ii->hbmColor, ii->hbmMask, TRUE);
    NtGdiDeleteObjectApp(hDC);

    if (surface->big_icon_buffer && type == ICON_BIG)
    {
        wayland_shm_buffer_unref(surface->big_icon_buffer);
        surface->big_icon_buffer = NULL;
    }
    else if (surface->small_icon_buffer && type != ICON_BIG)
    {
        wayland_shm_buffer_unref(surface->small_icon_buffer);
        surface->small_icon_buffer = NULL;
    }

    if (icon_buf)
    {
        if (type == ICON_BIG) surface->big_icon_buffer = icon_buf;
        else surface->small_icon_buffer = icon_buf;
    }
}

/**********************************************************************
 *          wayland_surface_assign_icon
 */
void wayland_surface_assign_icon(struct wayland_surface *surface)
{
    if (!process_wayland.xdg_toplevel_icon_manager_v1) return;

    assert(wayland_surface_is_toplevel(surface));

    TRACE("surface=%p\n", surface);

    if (surface->xdg_toplevel_icon)
    {
        xdg_toplevel_icon_manager_v1_set_icon(process_wayland.xdg_toplevel_icon_manager_v1,
                                              surface->xdg_toplevel, NULL);
        xdg_toplevel_icon_v1_destroy(surface->xdg_toplevel_icon);
        surface->xdg_toplevel_icon = NULL;
    }

    if (surface->big_icon_buffer)
    {
        surface->xdg_toplevel_icon =
            xdg_toplevel_icon_manager_v1_create_icon(process_wayland.xdg_toplevel_icon_manager_v1);

        /* FIXME: what to do with scale ? */
        xdg_toplevel_icon_v1_add_buffer(surface->xdg_toplevel_icon,
                                        surface->big_icon_buffer->wl_buffer, 1);
        if (surface->small_icon_buffer)
        {
            xdg_toplevel_icon_v1_add_buffer(surface->xdg_toplevel_icon,
                                            surface->small_icon_buffer->wl_buffer, 1);
        }

        xdg_toplevel_icon_v1_set_name(surface->xdg_toplevel_icon, "");

        xdg_toplevel_icon_manager_v1_set_icon(process_wayland.xdg_toplevel_icon_manager_v1,
                                              surface->xdg_toplevel, surface->xdg_toplevel_icon);
    }
}

void wayland_surface_set_opacity(struct wayland_surface *surface, BYTE alpha, UINT flags)
{
    if (surface->wp_alpha_modifier_surface_v1)
    {
        uint32_t opacity = (flags & LWA_ALPHA) ? (UINT32_MAX / 0xff) * alpha : UINT32_MAX;
        wp_alpha_modifier_surface_v1_set_multiplier(surface->wp_alpha_modifier_surface_v1, opacity);
        wl_surface_commit(surface->wl_surface);
        wl_display_flush(process_wayland.wl_display);
    }
}

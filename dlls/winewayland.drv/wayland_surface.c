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

#include <stdlib.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface,
                                         uint32_t serial)
{
    struct wayland_surface *surface = data;

    TRACE("serial=%u\n", serial);

    pthread_mutex_lock(&surface->mutex);
    /* Handle this event only if wayland_surface is still associated with
     * the target xdg_surface. */
    if (surface->xdg_surface == xdg_surface)
        xdg_surface_ack_configure(xdg_surface, serial);
    pthread_mutex_unlock(&surface->mutex);
}

static const struct xdg_surface_listener xdg_surface_listener =
{
    xdg_surface_handle_configure
};

/**********************************************************************
 *          wayland_surface_create
 *
 * Creates a role-less wayland surface.
 */
struct wayland_surface *wayland_surface_create(void)
{
    struct wayland_surface *surface;

    surface = calloc(1, sizeof(*surface));
    if (!surface)
    {
        ERR("Failed to allocate space for Wayland surface\n");
        goto err;
    }

    TRACE("surface=%p\n", surface);

    pthread_mutex_init(&surface->mutex, NULL);

    surface->wl_surface = wl_compositor_create_surface(process_wayland.wl_compositor);
    if (!surface->wl_surface)
    {
        ERR("Failed to create wl_surface Wayland surface\n");
        goto err;
    }

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
    pthread_mutex_lock(&surface->mutex);

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

    if (surface->wl_surface)
    {
        wl_surface_destroy(surface->wl_surface);
        surface->wl_surface = NULL;
    }

    pthread_mutex_unlock(&surface->mutex);

    wl_display_flush(process_wayland.wl_display);

    pthread_mutex_destroy(&surface->mutex);

    free(surface);
}

/**********************************************************************
 *          wayland_surface_make_toplevel
 *
 * Gives the toplevel role to a plain wayland surface.
 */
void wayland_surface_make_toplevel(struct wayland_surface *surface)
{
    TRACE("surface=%p\n", surface);

    surface->xdg_surface =
        xdg_wm_base_get_xdg_surface(process_wayland.xdg_wm_base, surface->wl_surface);
    if (!surface->xdg_surface) goto err;
    xdg_surface_add_listener(surface->xdg_surface, &xdg_surface_listener, surface);

    surface->xdg_toplevel = xdg_surface_get_toplevel(surface->xdg_surface);
    if (!surface->xdg_toplevel) goto err;

    wl_surface_commit(surface->wl_surface);
    wl_display_flush(process_wayland.wl_display);

    return;

err:
    wayland_surface_clear_role(surface);
    ERR("Failed to assign toplevel role to wayland surface\n");
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

    /* Ensure no buffer is attached, otherwise future role assignments may fail. */
    wl_surface_attach(surface->wl_surface, NULL, 0, 0);
    wl_surface_commit(surface->wl_surface);

    wl_display_flush(process_wayland.wl_display);
}

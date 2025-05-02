/*
 * Wayland window surface implementation
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

#include <limits.h>
#include <stdlib.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

struct wayland_buffer_queue
{
    struct wl_event_queue *wl_event_queue;
    struct wl_list buffer_list;
    int width;
    int height;
    uint32_t format;
};

struct wayland_window_surface
{
    struct window_surface header;
    struct wayland_buffer_queue *wayland_buffer_queue;
};

static struct wayland_window_surface *wayland_window_surface_cast(
    struct window_surface *window_surface)
{
    return (struct wayland_window_surface *)window_surface;
}

static void buffer_release(void *data, struct wl_buffer *buffer)
{
    struct wayland_shm_buffer *shm_buffer = data;
    TRACE("shm_buffer=%p\n", shm_buffer);
    shm_buffer->busy = FALSE;
    wayland_shm_buffer_unref(shm_buffer);
}

static const struct wl_buffer_listener buffer_listener = { buffer_release };

/**********************************************************************
 *          wayland_buffer_queue_destroy
 *
 * Destroys a buffer queue and any contained buffers.
 */
static void wayland_buffer_queue_destroy(struct wayland_buffer_queue *queue)
{
    struct wayland_shm_buffer *shm_buffer, *next;

    wl_list_for_each_safe(shm_buffer, next, &queue->buffer_list, link)
    {
        wl_list_remove(&shm_buffer->link);
        wl_list_init(&shm_buffer->link);
        /* Since this buffer may still be busy, attach it to the per-process
         * wl_event_queue to handle any future buffer release events. */
        wl_proxy_set_queue((struct wl_proxy *)shm_buffer->wl_buffer,
                           process_wayland.wl_event_queue);
        wayland_shm_buffer_unref(shm_buffer);
    }

    if (queue->wl_event_queue)
    {
        /* Dispatch the event queue before destruction to process any
         * pending buffer release events. This is required after changing
         * the buffer proxy event queue in the previous step, to avoid
         * missing any events. */
        wl_display_dispatch_queue_pending(process_wayland.wl_display,
                                          queue->wl_event_queue);
        wl_event_queue_destroy(queue->wl_event_queue);
    }

    free(queue);
}

/**********************************************************************
 *          wayland_buffer_queue_create
 *
 * Creates a buffer queue containing buffers with the specified width and height.
 */
static struct wayland_buffer_queue *wayland_buffer_queue_create(int width, int height,
                                                                uint32_t format)
{
    struct wayland_buffer_queue *queue;

    queue = calloc(1, sizeof(*queue));
    if (!queue) goto err;

    queue->wl_event_queue = wl_display_create_queue(process_wayland.wl_display);
    if (!queue->wl_event_queue) goto err;
    queue->width = width;
    queue->height = height;
    queue->format = format;

    wl_list_init(&queue->buffer_list);

    return queue;

err:
    if (queue) wayland_buffer_queue_destroy(queue);
    return NULL;
}

/**********************************************************************
 *          wayland_buffer_queue_get_free_buffer
 *
 * Gets a free buffer from the buffer queue. If no free buffers
 * are available this function blocks until it can provide one.
 */
static struct wayland_shm_buffer *wayland_buffer_queue_get_free_buffer(struct wayland_buffer_queue *queue)
{
    struct wayland_shm_buffer *shm_buffer;

    TRACE("queue=%p\n", queue);

    while (TRUE)
    {
        int nbuffers = 0;

        /* Dispatch any pending buffer release events. */
        wl_display_dispatch_queue_pending(process_wayland.wl_display,
                                          queue->wl_event_queue);

        /* Search through our buffers to find an available one. */
        wl_list_for_each(shm_buffer, &queue->buffer_list, link)
        {
            if (!shm_buffer->busy) goto out;
            nbuffers++;
        }

        /* Dynamically create up to 3 buffers. */
        if (nbuffers < 3)
        {
            shm_buffer = wayland_shm_buffer_create(queue->width, queue->height,
                                                   queue->format);
            if (shm_buffer)
            {
                /* Buffer events go to their own queue so that we can dispatch
                 * them independently. */
                wl_proxy_set_queue((struct wl_proxy *) shm_buffer->wl_buffer,
                                   queue->wl_event_queue);
                wl_buffer_add_listener(shm_buffer->wl_buffer, &buffer_listener,
                                       shm_buffer);
                wl_list_insert(&queue->buffer_list, &shm_buffer->link);
                goto out;
            }
            else if (nbuffers < 2)
            {
                /* If we failed to allocate a new buffer, but we have at least two
                 * buffers busy, there is a good chance the compositor will
                 * eventually release one of them, so dispatch events and wait
                 * below. Otherwise, give up and return a NULL buffer. */
                ERR(" => failed to acquire buffer\n");
                return NULL;
            }
        }

        /* We don't have any buffers available, so block waiting for a buffer
         * release event. */
        if (wl_display_dispatch_queue(process_wayland.wl_display,
                                      queue->wl_event_queue) == -1)
        {
            return NULL;
        }
    }

out:
    TRACE(" => %p %dx%d map=[%p, %p)\n",
          shm_buffer, shm_buffer->width, shm_buffer->height, shm_buffer->map_data,
          (unsigned char*)shm_buffer->map_data + shm_buffer->map_size);

    return shm_buffer;
}

/**********************************************************************
 *          wayland_buffer_queue_add_damage
 */
static void wayland_buffer_queue_add_damage(struct wayland_buffer_queue *queue, HRGN damage)
{
    struct wayland_shm_buffer *shm_buffer;

    wl_list_for_each(shm_buffer, &queue->buffer_list, link)
    {
        NtGdiCombineRgn(shm_buffer->damage_region, shm_buffer->damage_region,
                        damage, RGN_OR);
    }
}

/***********************************************************************
 *           wayland_window_surface_set_clip
 */
static void wayland_window_surface_set_clip(struct window_surface *window_surface,
                                            const RECT *rects, UINT count)
{
    /* TODO */
}

/**********************************************************************
 *          get_region_data
 */
RGNDATA *get_region_data(HRGN region)
{
    RGNDATA *data;
    DWORD size;

    if (!region) return NULL;
    if (!(size = NtGdiGetRegionData(region, 0, NULL))) return NULL;
    if (!(data = malloc(size))) return NULL;
    if (!NtGdiGetRegionData(region, size, data))
    {
        free(data);
        return NULL;
    }

    return data;
}

/**********************************************************************
 *          copy_pixel_region
 */
static void copy_pixel_region(const char *src_pixels, RECT *src_rect,
                              char *dst_pixels, RECT *dst_rect,
                              HRGN region, BOOL force_opaque)
{
    static const int bpp = WINEWAYLAND_BYTES_PER_PIXEL;
    RGNDATA *rgndata = get_region_data(region);
    RECT *rgn_rect;
    RECT *rgn_rect_end;
    int src_stride, dst_stride;

    if (!rgndata) return;

    src_stride = (src_rect->right - src_rect->left) * bpp;
    dst_stride = (dst_rect->right - dst_rect->left) * bpp;

    rgn_rect = (RECT *)rgndata->Buffer;
    rgn_rect_end = rgn_rect + rgndata->rdh.nCount;

    for (;rgn_rect < rgn_rect_end; rgn_rect++)
    {
        const char *src;
        char *dst;
        int x, y, width, height;
        RECT rc;

        TRACE("rect %s\n", wine_dbgstr_rect(rgn_rect));

        if (!intersect_rect(&rc, rgn_rect, src_rect)) continue;
        if (!intersect_rect(&rc, &rc, dst_rect)) continue;

        src = src_pixels + (rc.top - src_rect->top) * src_stride + (rc.left - src_rect->left) * bpp;
        dst = dst_pixels + (rc.top - dst_rect->top) * dst_stride + (rc.left - dst_rect->left) * bpp;
        width = rc.right - rc.left;
        height = rc.bottom - rc.top;

        /* Fast path for full width rectangles. */
        if (width * bpp == src_stride && src_stride == dst_stride)
        {
            if (force_opaque)
            {
                for (x = 0; x < height * width; ++x)
                    ((UINT32 *)dst)[x] = ((UINT32 *)src)[x] | 0xff000000;
            }
            else memcpy(dst, src, height * width * 4);
            continue;
        }

        if (force_opaque)
        {
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; ++x)
                    ((UINT32 *)dst)[x] = ((UINT32 *)src)[x] | 0xff000000;
                src += src_stride;
                dst += dst_stride;
            }
        }
        else
        {
            for (y = 0; y < height; y++)
            {
                memcpy(dst, src, width * 4);
                src += src_stride;
                dst += dst_stride;
            }
        }
    }

    free(rgndata);
}

/**********************************************************************
 *          wayland_shm_buffer_copy_data
 */
static void wayland_shm_buffer_copy_data(struct wayland_shm_buffer *buffer,
                                         const char *bits, RECT *rect,
                                         HRGN region, BOOL force_opaque)
{
    RECT buffer_rect = {0, 0, buffer->width, buffer->height};
    TRACE("buffer=%p bits=%p rect=%s\n", buffer, bits, wine_dbgstr_rect(rect));
    copy_pixel_region(bits, rect, buffer->map_data, &buffer_rect, region, force_opaque);
}

static void wayland_shm_buffer_copy(struct wayland_shm_buffer *src,
                                    struct wayland_shm_buffer *dst,
                                    HRGN region)
{
    RECT src_rect = {0, 0, src->width, src->height};
    RECT dst_rect = {0, 0, dst->width, dst->height};
    TRACE("src=%p dst=%p\n", src, dst);
    copy_pixel_region(src->map_data, &src_rect, dst->map_data, &dst_rect, region,
                      src->format == WL_SHM_FORMAT_XRGB8888 && dst->format == WL_SHM_FORMAT_ARGB8888);
}

/***********************************************************************
 *           wayland_window_surface_flush
 */
static BOOL wayland_window_surface_flush(struct window_surface *window_surface, const RECT *rect, const RECT *dirty,
                                         const BITMAPINFO *color_info, const void *color_bits, BOOL shape_changed,
                                         const BITMAPINFO *shape_info, const void *shape_bits)
{
    RECT surface_rect = {.right = color_info->bmiHeader.biWidth, .bottom = abs(color_info->bmiHeader.biHeight)};
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);
    struct wayland_shm_buffer *shm_buffer = NULL, *latest_buffer;
    BOOL flushed = FALSE;
    HRGN surface_damage_region = NULL;
    HRGN copy_from_window_region;
    uint32_t buffer_format;

    surface_damage_region = NtGdiCreateRectRgn(rect->left + dirty->left, rect->top + dirty->top,
                                               rect->left + dirty->right, rect->top + dirty->bottom);
    if (!surface_damage_region)
    {
        ERR("failed to create surface damage region\n");
        goto done;
    }

    buffer_format = shape_bits ? WL_SHM_FORMAT_ARGB8888 : WL_SHM_FORMAT_XRGB8888;
    if (wws->wayland_buffer_queue->format != buffer_format)
    {
        int width = wws->wayland_buffer_queue->width;
        int height = wws->wayland_buffer_queue->height;
        TRACE("recreating buffer queue with format %d\n", buffer_format);
        wayland_buffer_queue_destroy(wws->wayland_buffer_queue);
        wws->wayland_buffer_queue = wayland_buffer_queue_create(width, height, buffer_format);
    }

    wayland_buffer_queue_add_damage(wws->wayland_buffer_queue, surface_damage_region);

    shm_buffer = wayland_buffer_queue_get_free_buffer(wws->wayland_buffer_queue);
    if (!shm_buffer)
    {
        ERR("failed to acquire Wayland SHM buffer, returning\n");
        goto done;
    }

    if ((latest_buffer = get_window_surface_contents(window_surface->hwnd)))
    {
        TRACE("latest_window_buffer=%p\n", latest_buffer);
        /* If we have a latest buffer, use it as the source of all pixel
         * data that are not contained in the bounds of the flush... */
        if (latest_buffer != shm_buffer)
        {
            HRGN copy_from_latest_region = NtGdiCreateRectRgn(0, 0, 0, 0);
            if (!copy_from_latest_region)
            {
                ERR("failed to create copy_from_latest region\n");
                goto done;
            }
            NtGdiCombineRgn(copy_from_latest_region, shm_buffer->damage_region,
                            surface_damage_region, RGN_DIFF);
            wayland_shm_buffer_copy(latest_buffer,
                                    shm_buffer, copy_from_latest_region);
            NtGdiDeleteObjectApp(copy_from_latest_region);
        }
        /* ... and use the window_surface as the source of pixel data contained
         * in the flush bounds. */
        copy_from_window_region = surface_damage_region;
        wayland_shm_buffer_unref(latest_buffer);
    }
    else
    {
        TRACE("latest_window_buffer=NULL\n");
        /* If we don't have a latest buffer, use the window_surface as
         * the source of all pixel data. */
        copy_from_window_region = shm_buffer->damage_region;
    }

    wayland_shm_buffer_copy_data(shm_buffer, color_bits, &surface_rect, copy_from_window_region,
                                 !!shape_bits);
    NtGdiSetRectRgn(shm_buffer->damage_region, 0, 0, 0, 0);

    flushed = set_window_surface_contents(window_surface->hwnd, shm_buffer, surface_damage_region);
    wl_display_flush(process_wayland.wl_display);

done:
    if (surface_damage_region) NtGdiDeleteObjectApp(surface_damage_region);
    return flushed;
}

/***********************************************************************
 *           wayland_window_surface_destroy
 */
static void wayland_window_surface_destroy(struct window_surface *window_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);

    TRACE("surface=%p\n", wws);

    wayland_buffer_queue_destroy(wws->wayland_buffer_queue);
}

static const struct window_surface_funcs wayland_window_surface_funcs =
{
    wayland_window_surface_set_clip,
    wayland_window_surface_flush,
    wayland_window_surface_destroy
};

/***********************************************************************
 *           wayland_window_surface_create
 */
static struct window_surface *wayland_window_surface_create(HWND hwnd, const RECT *rect)
{
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct wayland_window_surface *wws;
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;
    struct window_surface *window_surface;

    TRACE("hwnd %p rect %s\n", hwnd, wine_dbgstr_rect(rect));

    memset(info, 0, sizeof(*info));
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = width;
    info->bmiHeader.biHeight      = -height; /* top-down */
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biSizeImage   = width * height * 4;
    info->bmiHeader.biCompression = BI_RGB;

    if ((window_surface = window_surface_create(sizeof(*wws), &wayland_window_surface_funcs, hwnd, rect, info, 0)))
    {
        struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);
        wws->wayland_buffer_queue = wayland_buffer_queue_create(width, height, WL_SHM_FORMAT_XRGB8888);
    }

    return window_surface;
}

/***********************************************************************
 *           WAYLAND_CreateWindowSurface
 */
BOOL WAYLAND_CreateWindowSurface(HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface)
{
    struct window_surface *previous;
    struct wayland_win_data *data;

    TRACE("hwnd %p, layered %u, surface_rect %s, surface %p\n", hwnd, layered, wine_dbgstr_rect(surface_rect), surface);

    if ((previous = *surface) && previous->funcs == &wayland_window_surface_funcs) return TRUE;
    if (!(data = wayland_win_data_get(hwnd))) return TRUE; /* use default surface */
    if (previous) window_surface_release(previous);

    *surface = wayland_window_surface_create(data->hwnd, surface_rect);

    wayland_win_data_release(data);
    return TRUE;
}

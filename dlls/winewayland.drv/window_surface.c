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
};

struct wayland_window_surface
{
    struct window_surface header;
    HWND hwnd;
    struct wayland_surface *wayland_surface;
    struct wayland_buffer_queue *wayland_buffer_queue;
    RECT bounds;
    void *bits;
    pthread_mutex_t mutex;
    BITMAPINFO info;
};

static struct wayland_window_surface *wayland_window_surface_cast(
    struct window_surface *window_surface)
{
    return (struct wayland_window_surface *)window_surface;
}

static inline void reset_bounds(RECT *bounds)
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
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
static struct wayland_buffer_queue *wayland_buffer_queue_create(int width, int height)
{
    struct wayland_buffer_queue *queue;

    queue = calloc(1, sizeof(*queue));
    if (!queue) goto err;

    queue->wl_event_queue = wl_display_create_queue(process_wayland.wl_display);
    if (!queue->wl_event_queue) goto err;
    queue->width = width;
    queue->height = height;

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
                                                   WL_SHM_FORMAT_XRGB8888);
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
 *           wayland_window_surface_lock
 */
static void wayland_window_surface_lock(struct window_surface *window_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);
    pthread_mutex_lock(&wws->mutex);
}

/***********************************************************************
 *           wayland_window_surface_unlock
 */
static void wayland_window_surface_unlock(struct window_surface *window_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);
    pthread_mutex_unlock(&wws->mutex);
}

/***********************************************************************
 *           wayland_window_surface_get_bitmap_info
 */
static void *wayland_window_surface_get_bitmap_info(struct window_surface *window_surface,
                                                    BITMAPINFO *info)
{
    struct wayland_window_surface *surface = wayland_window_surface_cast(window_surface);
    /* We don't store any additional information at the end of our BITMAPINFO, so
     * just copy the structure itself. */
    memcpy(info, &surface->info, sizeof(*info));
    return surface->bits;
}

/***********************************************************************
 *           wayland_window_surface_get_bounds
 */
static RECT *wayland_window_surface_get_bounds(struct window_surface *window_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);
    return &wws->bounds;
}

/***********************************************************************
 *           wayland_window_surface_set_region
 */
static void wayland_window_surface_set_region(struct window_surface *window_surface,
                                              HRGN region)
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
static void copy_pixel_region(char *src_pixels, RECT *src_rect,
                              char *dst_pixels, RECT *dst_rect,
                              HRGN region)
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
        char *src, *dst;
        int y, width_bytes, height;
        RECT rc;

        TRACE("rect %s\n", wine_dbgstr_rect(rgn_rect));

        if (!intersect_rect(&rc, rgn_rect, src_rect)) continue;
        if (!intersect_rect(&rc, &rc, dst_rect)) continue;

        src = src_pixels + rc.top * src_stride + rc.left * bpp;
        dst = dst_pixels + rc.top * dst_stride + rc.left * bpp;
        width_bytes = (rc.right - rc.left) * bpp;
        height = rc.bottom - rc.top;

        /* Fast path for full width rectangles. */
        if (width_bytes == src_stride && width_bytes == dst_stride)
        {
            memcpy(dst, src, height * width_bytes);
            continue;
        }

        for (y = 0; y < height; y++)
        {
            memcpy(dst, src, width_bytes);
            src += src_stride;
            dst += dst_stride;
        }
    }

    free(rgndata);
}

/**********************************************************************
 *          wayland_window_surface_copy_to_buffer
 */
static void wayland_window_surface_copy_to_buffer(struct wayland_window_surface *wws,
                                                  struct wayland_shm_buffer *buffer,
                                                  HRGN region)
{
    RECT wws_rect = {0, 0, wws->info.bmiHeader.biWidth,
                     abs(wws->info.bmiHeader.biHeight)};
    RECT buffer_rect = {0, 0, buffer->width, buffer->height};
    TRACE("wws=%p buffer=%p\n", wws, buffer);
    copy_pixel_region(wws->bits, &wws_rect, buffer->map_data, &buffer_rect, region);
}

static void wayland_shm_buffer_copy(struct wayland_shm_buffer *src,
                                    struct wayland_shm_buffer *dst,
                                    HRGN region)
{
    RECT src_rect = {0, 0, src->width, src->height};
    RECT dst_rect = {0, 0, dst->width, dst->height};
    TRACE("src=%p dst=%p\n", src, dst);
    copy_pixel_region(src->map_data, &src_rect, dst->map_data, &dst_rect, region);
}

/***********************************************************************
 *           wayland_window_surface_flush
 */
static void wayland_window_surface_flush(struct window_surface *window_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);
    struct wayland_shm_buffer *shm_buffer = NULL;
    BOOL flushed = FALSE;
    RECT damage_rect;
    HRGN surface_damage_region = NULL;
    HRGN copy_from_window_region;

    wayland_window_surface_lock(window_surface);

    if (!intersect_rect(&damage_rect, &wws->header.rect, &wws->bounds)) goto done;

    if (!wws->wayland_surface || !wws->wayland_buffer_queue)
    {
        ERR("missing wayland surface=%p or buffer_queue=%p, returning\n",
            wws->wayland_surface, wws->wayland_buffer_queue);
        goto done;
    }

    TRACE("surface=%p hwnd=%p surface_rect=%s bounds=%s\n", wws, wws->hwnd,
          wine_dbgstr_rect(&wws->header.rect), wine_dbgstr_rect(&wws->bounds));

    surface_damage_region = NtGdiCreateRectRgn(damage_rect.left, damage_rect.top,
                                               damage_rect.right, damage_rect.bottom);
    if (!surface_damage_region)
    {
        ERR("failed to create surface damage region\n");
        goto done;
    }

    wayland_buffer_queue_add_damage(wws->wayland_buffer_queue, surface_damage_region);

    shm_buffer = wayland_buffer_queue_get_free_buffer(wws->wayland_buffer_queue);
    if (!shm_buffer)
    {
        ERR("failed to acquire Wayland SHM buffer, returning\n");
        goto done;
    }

    if (wws->wayland_surface->latest_window_buffer)
    {
        TRACE("latest_window_buffer=%p\n", wws->wayland_surface->latest_window_buffer);
        /* If we have a latest buffer, use it as the source of all pixel
         * data that are not contained in the bounds of the flush... */
        if (wws->wayland_surface->latest_window_buffer != shm_buffer)
        {
            HRGN copy_from_latest_region = NtGdiCreateRectRgn(0, 0, 0, 0);
            if (!copy_from_latest_region)
            {
                ERR("failed to create copy_from_latest region\n");
                goto done;
            }
            NtGdiCombineRgn(copy_from_latest_region, shm_buffer->damage_region,
                            surface_damage_region, RGN_DIFF);
            wayland_shm_buffer_copy(wws->wayland_surface->latest_window_buffer,
                                    shm_buffer, copy_from_latest_region);
            NtGdiDeleteObjectApp(copy_from_latest_region);
        }
        /* ... and use the window_surface as the source of pixel data contained
         * in the flush bounds. */
        copy_from_window_region = surface_damage_region;
    }
    else
    {
        TRACE("latest_window_buffer=NULL\n");
        /* If we don't have a latest buffer, use the window_surface as
         * the source of all pixel data. */
        copy_from_window_region = shm_buffer->damage_region;
    }

    wayland_window_surface_copy_to_buffer(wws, shm_buffer, copy_from_window_region);

    pthread_mutex_lock(&wws->wayland_surface->mutex);
    if (wayland_surface_reconfigure(wws->wayland_surface))
    {
        wayland_surface_attach_shm(wws->wayland_surface, shm_buffer,
                                   surface_damage_region);
        wl_surface_commit(wws->wayland_surface->wl_surface);
        flushed = TRUE;
    }
    else
    {
        TRACE("Wayland surface not configured yet, not flushing\n");
    }
    pthread_mutex_unlock(&wws->wayland_surface->mutex);
    wl_display_flush(process_wayland.wl_display);

    NtGdiSetRectRgn(shm_buffer->damage_region, 0, 0, 0, 0);
    /* Update the latest window buffer for the wayland surface. Note that we
     * only care whether the buffer contains the latest window contents,
     * it's irrelevant if it was actually committed or not. */
    if (wws->wayland_surface->latest_window_buffer)
        wayland_shm_buffer_unref(wws->wayland_surface->latest_window_buffer);
    wayland_shm_buffer_ref((wws->wayland_surface->latest_window_buffer = shm_buffer));

done:
    if (flushed) reset_bounds(&wws->bounds);
    if (surface_damage_region) NtGdiDeleteObjectApp(surface_damage_region);
    wayland_window_surface_unlock(window_surface);
}

/***********************************************************************
 *           wayland_window_surface_destroy
 */
static void wayland_window_surface_destroy(struct window_surface *window_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);

    TRACE("surface=%p\n", wws);

    pthread_mutex_destroy(&wws->mutex);
    if (wws->wayland_buffer_queue)
        wayland_buffer_queue_destroy(wws->wayland_buffer_queue);
    free(wws->bits);
    free(wws);
}

static const struct window_surface_funcs wayland_window_surface_funcs =
{
    wayland_window_surface_lock,
    wayland_window_surface_unlock,
    wayland_window_surface_get_bitmap_info,
    wayland_window_surface_get_bounds,
    wayland_window_surface_set_region,
    wayland_window_surface_flush,
    wayland_window_surface_destroy
};

/***********************************************************************
 *           wayland_window_surface_create
 */
struct window_surface *wayland_window_surface_create(HWND hwnd, const RECT *rect)
{
    struct wayland_window_surface *wws;
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;
    pthread_mutexattr_t mutexattr;

    TRACE("hwnd %p rect %s\n", hwnd, wine_dbgstr_rect(rect));

    wws = calloc(1, sizeof(*wws));
    if (!wws) return NULL;
    wws->info.bmiHeader.biSize = sizeof(wws->info.bmiHeader);
    wws->info.bmiHeader.biClrUsed = 0;
    wws->info.bmiHeader.biBitCount = 32;
    wws->info.bmiHeader.biCompression = BI_RGB;
    wws->info.bmiHeader.biWidth = width;
    wws->info.bmiHeader.biHeight = -height; /* top-down */
    wws->info.bmiHeader.biPlanes = 1;
    wws->info.bmiHeader.biSizeImage = width * height * 4;

    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&wws->mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);

    wws->header.funcs = &wayland_window_surface_funcs;
    wws->header.rect = *rect;
    wws->header.ref = 1;
    wws->hwnd = hwnd;
    reset_bounds(&wws->bounds);

    if (!(wws->bits = malloc(wws->info.bmiHeader.biSizeImage)))
        goto failed;

    TRACE("created %p hwnd %p %s bits [%p,%p)\n", wws, hwnd, wine_dbgstr_rect(rect),
          wws->bits, (char *)wws->bits + wws->info.bmiHeader.biSizeImage);

    return &wws->header;

failed:
    wayland_window_surface_destroy(&wws->header);
    return NULL;
}

/***********************************************************************
 *           wayland_window_surface_update_wayland_surface
 */
void wayland_window_surface_update_wayland_surface(struct window_surface *window_surface,
                                                   struct wayland_surface *wayland_surface)
{
    struct wayland_window_surface *wws = wayland_window_surface_cast(window_surface);

    wayland_window_surface_lock(window_surface);

    TRACE("surface=%p hwnd=%p wayland_surface=%p\n", wws, wws->hwnd, wayland_surface);

    wws->wayland_surface = wayland_surface;

    /* We only need a buffer queue if we have a surface to commit to. */
    if (wws->wayland_surface && !wws->wayland_buffer_queue)
    {
        wws->wayland_buffer_queue =
            wayland_buffer_queue_create(wws->info.bmiHeader.biWidth,
                                        abs(wws->info.bmiHeader.biHeight));
    }
    else if (!wws->wayland_surface && wws->wayland_buffer_queue)
    {
        wayland_buffer_queue_destroy(wws->wayland_buffer_queue);
        wws->wayland_buffer_queue = NULL;
    }

    wayland_window_surface_unlock(window_surface);
}

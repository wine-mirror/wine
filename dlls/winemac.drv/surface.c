/*
 * Mac driver window surface implementation
 *
 * Copyright 1993, 1994, 2011 Alexandre Julliard
 * Copyright 2006 Damjan Jovanovic
 * Copyright 2012, 2013 Ken Thomases for CodeWeavers, Inc.
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
#include "macdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitblt);

static inline int get_dib_stride(int width, int bpp)
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size(const BITMAPINFO *info)
{
    return get_dib_stride(info->bmiHeader.biWidth, info->bmiHeader.biBitCount)
        * abs(info->bmiHeader.biHeight);
}


struct macdrv_window_surface
{
    struct window_surface   header;
    WineWindow             *window;
    IOSurfaceRef            front_buffer;
    IOSurfaceRef            back_buffer;
    vImage_Buffer           shape_mask;
};

static struct macdrv_window_surface *get_mac_surface(struct window_surface *surface);

static IOSurfaceRef create_io_surface(int width, int height)
{
    CFStringRef keys[] = { kIOSurfaceWidth, kIOSurfaceHeight, kIOSurfaceBytesPerElement, kIOSurfacePixelFormat };
    CFNumberRef values[4];
    CFDictionaryRef properties;
    IOSurfaceRef surface;
    uint32_t surface_width = width, surface_height = height;
    uint32_t bytes_per_element = 4, pixel_format = 'BGRA';

    values[0] = CFNumberCreate(NULL, kCFNumberSInt32Type, &surface_width);
    values[1] = CFNumberCreate(NULL, kCFNumberSInt32Type, &surface_height);
    values[2] = CFNumberCreate(NULL, kCFNumberSInt32Type, &bytes_per_element);
    values[3] = CFNumberCreate(NULL, kCFNumberSInt32Type, &pixel_format);
    properties = CFDictionaryCreate(NULL, (const void **)keys, (const void **)values, ARRAY_SIZE(keys),
                                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    surface = IOSurfaceCreate(properties);
    CFRelease(properties);
    CFRelease(values[0]);
    CFRelease(values[1]);
    CFRelease(values[2]);
    CFRelease(values[3]);
    return surface;
}

static void fill_io_surface(IOSurfaceRef surface, DWORD color)
{
    BYTE *base;
    size_t row_bytes, y, height;

    IOSurfaceLock(surface, 0, NULL);
    base = IOSurfaceGetBaseAddress(surface);
    row_bytes = IOSurfaceGetBytesPerRow(surface);
    height = IOSurfaceGetHeight(surface);
    for (y = 0; y < height; y++)
        memset_pattern4(base + y * row_bytes, &color, IOSurfaceGetWidth(surface) * 4);
    IOSurfaceUnlock(surface, 0, NULL);
}

/***********************************************************************
 *              macdrv_surface_set_clip
 */
static void macdrv_surface_set_clip(struct window_surface *window_surface, const RECT *rects, UINT count)
{
}

static void update_shape(vImage_Buffer *shape_mask, const BITMAPINFO *shape_info,
                         const void *shape_bits)
{
    const BYTE *src = shape_bits;
    UINT width = shape_mask->width, height = shape_mask->height;
    UINT src_row_bytes = shape_info->bmiHeader.biSizeImage / abs(shape_info->bmiHeader.biHeight);
    UINT x, y;

    for (y = 0; y < height; y++)
    {
        const BYTE *src_row = src + y * src_row_bytes;
        DWORD *dst_row = (DWORD *)((BYTE *)shape_mask->data + y * shape_mask->rowBytes);

        for (x = 0; x < width; x++)
            dst_row[x] = ((src_row[x / 8] >> (7 - x % 8)) & 1) ? 0xffffffff : 0;
    }
}

/***********************************************************************
 *              macdrv_surface_flush
 */
static BOOL macdrv_surface_flush(struct window_surface *window_surface, const RECT *rect, const RECT *dirty,
                                 const BITMAPINFO *color_info, const void *color_bits, BOOL shape_changed,
                                 const BITMAPINFO *shape_info, const void *shape_bits)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    IOSurfaceRef io_surface = surface->back_buffer;
    bool use_alpha = !!window_surface->alpha_mask;
    struct macdrv_win_data *data;
    vImage_Buffer src = {
        .data = (void *)color_bits,
        .height = IOSurfaceGetHeight(io_surface),
        .width = IOSurfaceGetWidth(io_surface),
        .rowBytes = color_info->bmiHeader.biSizeImage / abs(color_info->bmiHeader.biHeight),
    };
    vImage_Buffer dst = {
        .data = IOSurfaceGetBaseAddress(io_surface),
        .height = src.height,
        .width = src.width,
        .rowBytes = IOSurfaceGetBytesPerRow(io_surface),
    };

    if (shape_changed && shape_bits) update_shape(&surface->shape_mask, shape_info, shape_bits);

    IOSurfaceLock(io_surface, 0, NULL);
    if (use_alpha)
    {
        vImageClipToAlpha_BGRA8888(&src, &dst, kvImageNoFlags);
        if (shape_bits &&
            (window_surface->shape_region || window_surface->color_key != CLR_INVALID))
        {
            vImage_Buffer bytes = dst, mask = surface->shape_mask;

            /* The mask repeats 0 or 255 across BGRA, so bytewise clipping clears whole pixels. */
            bytes.width *= 4;
            mask.width *= 4;
            vImageClipToAlpha_Planar8(&bytes, &mask, &bytes, kvImageNoFlags);
        }
    }
    else if (!shape_bits)
    {
        vImageOverwriteChannelsWithScalar_ARGB8888(255, &src, &dst, 0x1, kvImageNoFlags);
    }
    else
    {
        vImageSelectChannels_ARGB8888(&surface->shape_mask, &src, &dst, 0x1, kvImageNoFlags);
        vImageClipToAlpha_BGRA8888(&dst, &dst, kvImageNoFlags);
    }
    IOSurfaceUnlock(io_surface, 0, NULL);

    surface->back_buffer = surface->front_buffer;
    surface->front_buffer = io_surface;
    macdrv_window_set_io_surface(surface->window, io_surface, cgrect_from_rect(*rect), cgrect_from_rect(*dirty));

    if (shape_changed) macdrv_window_shape_changed(surface->window, !!shape_bits);

    /* The window may have been previously drawn with client_surface, for example, when the window
     * had been a target for a D3D swapchain. Hide the client_view so that it doesn't occlude the
     * content in the window_surface */
    if ((data = get_win_data(window_surface->hwnd)))
    {
        if (data->client_view)
        {
            macdrv_set_view_hidden(data->client_view, TRUE);
            data->client_view = NULL;
        }
        release_win_data(data);
    }

    return TRUE;
}

/***********************************************************************
 *              macdrv_surface_destroy
 */
static void macdrv_surface_destroy(struct window_surface *window_surface)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    TRACE("freeing %p\n", surface);
    free(surface->shape_mask.data);
    if (surface->back_buffer) CFRelease(surface->back_buffer);
    if (surface->front_buffer) CFRelease(surface->front_buffer);
}

static const struct window_surface_funcs macdrv_surface_funcs =
{
    .size = sizeof(struct macdrv_window_surface),
    .set_clip = macdrv_surface_set_clip,
    .flush = macdrv_surface_flush,
    .destroy = macdrv_surface_destroy,
};

static struct macdrv_window_surface *get_mac_surface(struct window_surface *surface)
{
    if (!surface || surface->funcs != &macdrv_surface_funcs) return NULL;
    return (struct macdrv_window_surface *)surface;
}

/***********************************************************************
 *              create_surface
 */
static struct window_surface *create_surface(HWND hwnd, WineWindow *window, const RECT *rect)
{
    struct macdrv_window_surface *surface;
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    DWORD window_background;
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct window_surface *window_surface;
    IOSurfaceRef io_surface1 = NULL, io_surface2 = NULL;
    HBITMAP bitmap;
    void *bits;

    memset(info, 0, sizeof(*info));
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = width;
    info->bmiHeader.biHeight      = -height; /* top-down */
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biSizeImage   = get_dib_image_size(info);
    info->bmiHeader.biCompression = BI_RGB;

    if (!(bitmap = NtGdiCreateDIBSection(0, NULL, 0, info, DIB_RGB_COLORS, 0, 0, 0, &bits)))
        return NULL;

    window_background = macdrv_window_background_color();

    if (!(io_surface1 = create_io_surface(width, height)) ||
        !(io_surface2 = create_io_surface(width, height)))
    {
        if (io_surface1) CFRelease(io_surface1);
        NtGdiDeleteObjectApp(bitmap);
        return NULL;
    }

    fill_io_surface(io_surface1, window_background);
    fill_io_surface(io_surface2, window_background);

    if (!(window_surface = window_surface_create(&macdrv_surface_funcs, hwnd, rect, info, bitmap)))
    {
        NtGdiDeleteObjectApp(bitmap);
        CFRelease(io_surface1);
        CFRelease(io_surface2);
    }
    else
    {
        surface = get_mac_surface(window_surface);
        surface->window = window;
        surface->front_buffer = io_surface1;
        surface->back_buffer = io_surface2;
        if (vImageBuffer_Init(&surface->shape_mask, height, width, 32, kvImageNoFlags))
        {
            window_surface_release(window_surface);
            return NULL;
        }
        window_background &= 0x00ffffff;
        memset_pattern4(bits, &window_background, info->bmiHeader.biSizeImage);
    }

    return window_surface;
}


/***********************************************************************
 *              CreateWindowSurface   (MACDRV.@)
 */
BOOL macdrv_CreateWindowSurface(HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface)
{
    struct window_surface *previous;
    struct macdrv_win_data *data;

    TRACE("hwnd %p, layered %u, surface_rect %s, surface %p\n", hwnd, layered, wine_dbgstr_rect(surface_rect), surface);

    if ((previous = *surface) && previous->funcs == &macdrv_surface_funcs) return TRUE;
    if (!(data = get_win_data(hwnd))) return TRUE; /* use default surface */
    if (previous) window_surface_release(previous);

    if (layered)
    {
        data->layered = TRUE;
        data->ulw_layered = TRUE;
    }

    *surface = create_surface(hwnd, data->cocoa_window, surface_rect);

    release_win_data(data);
    return TRUE;
}

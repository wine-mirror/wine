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
#include "winuser.h"

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
    macdrv_window           window;
    CGDataProviderRef       provider;
    BITMAPINFO              info;   /* variable size, must be last */
};

static struct macdrv_window_surface *get_mac_surface(struct window_surface *surface);

static CGDataProviderRef data_provider_create(size_t size, void **bits)
{
    CGDataProviderRef provider;
    CFMutableDataRef data;

    if (!(data = CFDataCreateMutable(kCFAllocatorDefault, size))) return NULL;
    CFDataSetLength(data, size);

    if ((provider = CGDataProviderCreateWithCFData(data)))
        *bits = CFDataGetMutableBytePtr(data);
    CFRelease(data);

    return provider;
}

/***********************************************************************
 *              macdrv_surface_set_clip
 */
static void macdrv_surface_set_clip(struct window_surface *window_surface, const RECT *rects, UINT count)
{
}

/***********************************************************************
 *              macdrv_surface_flush
 */
static BOOL macdrv_surface_flush(struct window_surface *window_surface, const RECT *rect, const RECT *dirty,
                                 const BITMAPINFO *color_info, const void *color_bits)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    CGImageAlphaInfo alpha_info = (window_surface->alpha_mask ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst);
    CGColorSpaceRef colorspace;
    CGImageRef image;

    colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    image = CGImageCreate(color_info->bmiHeader.biWidth, abs(color_info->bmiHeader.biHeight), 8, 32,
                          color_info->bmiHeader.biSizeImage / abs(color_info->bmiHeader.biHeight), colorspace,
                          alpha_info | kCGBitmapByteOrder32Little, surface->provider, NULL, retina_on, kCGRenderingIntentDefault);
    CGColorSpaceRelease(colorspace);

    macdrv_window_set_color_image(surface->window, image, cgrect_from_rect(*rect), cgrect_from_rect(*dirty));
    CGImageRelease(image);

    return TRUE;
}

/***********************************************************************
 *              macdrv_surface_destroy
 */
static void macdrv_surface_destroy(struct window_surface *window_surface)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    TRACE("freeing %p\n", surface);
    CGDataProviderRelease(surface->provider);
    free(surface);
}

static const struct window_surface_funcs macdrv_surface_funcs =
{
    macdrv_surface_set_clip,
    macdrv_surface_flush,
    macdrv_surface_destroy,
};

static struct macdrv_window_surface *get_mac_surface(struct window_surface *surface)
{
    if (!surface || surface->funcs != &macdrv_surface_funcs) return NULL;
    return (struct macdrv_window_surface *)surface;
}

/***********************************************************************
 *              create_surface
 */
static struct window_surface *create_surface(HWND hwnd, macdrv_window window, const RECT *rect,
                                             struct window_surface *old_surface, BOOL use_alpha)
{
    struct macdrv_window_surface *surface = NULL;
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    DWORD window_background;
    D3DKMT_CREATEDCFROMMEMORY desc = {.Format = D3DDDIFMT_A8R8G8B8};
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    CGDataProviderRef provider;
    HBITMAP bitmap = 0;
    UINT status;
    void *bits;

    memset(info, 0, sizeof(*info));
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = width;
    info->bmiHeader.biHeight      = -height; /* top-down */
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biSizeImage   = get_dib_image_size(info);
    info->bmiHeader.biCompression = BI_RGB;

    if (!(provider = data_provider_create(info->bmiHeader.biSizeImage, &bits))) return NULL;

    /* wrap the data in a HBITMAP so we can write to the surface pixels directly */
    desc.Width = info->bmiHeader.biWidth;
    desc.Height = abs(info->bmiHeader.biHeight);
    desc.Pitch = info->bmiHeader.biSizeImage / abs(info->bmiHeader.biHeight);
    desc.pMemory = bits;
    desc.hDeviceDc = NtUserGetDCEx(hwnd, 0, DCX_CACHE | DCX_WINDOW);
    if ((status = NtGdiDdDDICreateDCFromMemory(&desc)))
        ERR("Failed to create HBITMAP, status %#x\n", status);
    else
    {
        bitmap = desc.hBitmap;
        NtGdiDeleteObjectApp(desc.hDc);
    }
    if (desc.hDeviceDc) NtUserReleaseDC(hwnd, desc.hDeviceDc);

    if (!(surface = calloc(1, FIELD_OFFSET(struct macdrv_window_surface, info.bmiColors[3])))) goto failed;
    if (!window_surface_init(&surface->header, &macdrv_surface_funcs, hwnd, rect, info, bitmap)) goto failed;
    memcpy(&surface->info, info, offsetof(BITMAPINFO, bmiColors[3]));

    surface->window = window;
    if (old_surface) surface->header.bounds = old_surface->bounds;
    surface->provider = provider;

    window_background = macdrv_window_background_color();
    memset_pattern4(bits, &window_background, info->bmiHeader.biSizeImage);

    TRACE("created %p for %p %s\n", surface, window, wine_dbgstr_rect(rect));

    if (use_alpha) window_surface_set_layered( &surface->header, CLR_INVALID, -1, 0xff000000 );
    else window_surface_set_layered( &surface->header, CLR_INVALID, -1, 0 );

    return &surface->header;

failed:
    if (surface) window_surface_release(&surface->header);
    if (bitmap) NtGdiDeleteObjectApp(bitmap);
    CGDataProviderRelease(provider);
    return NULL;
}


/***********************************************************************
 *              CreateWindowSurface   (MACDRV.@)
 */
BOOL macdrv_CreateWindowSurface(HWND hwnd, const RECT *surface_rect, struct window_surface **surface)
{
    struct macdrv_win_data *data;

    TRACE("hwnd %p, surface_rect %s, surface %p\n", hwnd, wine_dbgstr_rect(surface_rect), surface);

    if (!(data = get_win_data(hwnd))) return TRUE; /* use default surface */

    if (*surface) window_surface_release(*surface);
    *surface = NULL;

    if (data->surface)
    {
        if (EqualRect(&data->surface->rect, surface_rect))
        {
            /* existing surface is good enough */
            window_surface_add_ref(data->surface);
            *surface = data->surface;
            goto done;
        }
    }

    *surface = create_surface(data->hwnd, data->cocoa_window, surface_rect, data->surface, FALSE);

done:
    release_win_data(data);
    return TRUE;
}


/***********************************************************************
 *              CreateLayeredWindow   (MACDRV.@)
 */
BOOL macdrv_CreateLayeredWindow(HWND hwnd, const RECT *window_rect, COLORREF color_key,
                                struct window_surface **window_surface)
{
    struct window_surface *surface;
    struct macdrv_win_data *data;
    RECT rect;

    if (!(data = get_win_data(hwnd))) return FALSE;

    data->layered = TRUE;
    data->ulw_layered = TRUE;

    rect = *window_rect;
    OffsetRect(&rect, -window_rect->left, -window_rect->top);

    surface = data->surface;
    if (!surface || !EqualRect(&surface->rect, &rect))
    {
        data->surface = create_surface(data->hwnd, data->cocoa_window, &rect, NULL, TRUE);
        if (surface) window_surface_release(surface);
        surface = data->surface;
        if (data->unminimized_surface)
        {
            window_surface_release(data->unminimized_surface);
            data->unminimized_surface = NULL;
        }
    }
    else window_surface_set_layered(surface, color_key, -1, 0xff000000);

    if ((*window_surface = surface)) window_surface_add_ref(surface);

    release_win_data(data);

    return TRUE;
}

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


/* only for use on sanitized BITMAPINFO structures */
static inline int get_dib_info_size(const BITMAPINFO *info, UINT coloruse)
{
    if (info->bmiHeader.biCompression == BI_BITFIELDS)
        return sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
    if (coloruse == DIB_PAL_COLORS)
        return sizeof(BITMAPINFOHEADER) + info->bmiHeader.biClrUsed * sizeof(WORD);
    return FIELD_OFFSET(BITMAPINFO, bmiColors[info->bmiHeader.biClrUsed]);
}

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
    BOOL                    use_alpha;
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
 *              macdrv_surface_get_bitmap_info
 */
static void *macdrv_surface_get_bitmap_info(struct window_surface *window_surface,
                                                  BITMAPINFO *info)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    memcpy(info, &surface->info, get_dib_info_size(&surface->info, DIB_RGB_COLORS));
    return window_surface->color_bits;
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
static BOOL macdrv_surface_flush(struct window_surface *window_surface, const RECT *rect, const RECT *dirty)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    CGImageAlphaInfo alpha_info = (surface->use_alpha ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst);
    BITMAPINFO *color_info = &surface->info;
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
    macdrv_surface_get_bitmap_info,
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
    surface->use_alpha = use_alpha;
    surface->provider = provider;

    window_background = macdrv_window_background_color();
    memset_pattern4(surface->header.color_bits, &window_background, info->bmiHeader.biSizeImage);

    TRACE("created %p for %p %s color_bits %p-%p\n", surface, window, wine_dbgstr_rect(rect),
          surface->header.color_bits, (char *)surface->header.color_bits + info->bmiHeader.biSizeImage);

    return &surface->header;

failed:
    if (surface) window_surface_release(&surface->header);
    if (bitmap) NtGdiDeleteObjectApp(bitmap);
    CGDataProviderRelease(provider);
    return NULL;
}

/***********************************************************************
 *              set_surface_use_alpha
 */
void set_surface_use_alpha(struct window_surface *window_surface, BOOL use_alpha)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    if (surface) surface->use_alpha = use_alpha;
}

/***********************************************************************
 *              surface_clip_to_visible_rect
 *
 * Intersect the accumulated drawn region with a new visible rect,
 * effectively discarding stale drawing in the surface slack area.
 */
static void surface_clip_to_visible_rect(struct window_surface *window_surface, const RECT *visible_rect)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    RECT rect = *visible_rect;
    OffsetRect(&rect, -rect.left, -rect.top);

    if (!surface) return;

    window_surface_lock(window_surface);
    intersect_rect(&window_surface->bounds, &window_surface->bounds, &rect);
    window_surface_unlock(window_surface);
}


static inline RECT get_surface_rect(const RECT *visible_rect)
{
    RECT rect = *visible_rect;

    OffsetRect(&rect, -visible_rect->left, -visible_rect->top);
    rect.left &= ~127;
    rect.top  &= ~127;
    rect.right  = max(rect.left + 128, (rect.right + 127) & ~127);
    rect.bottom = max(rect.top + 128, (rect.bottom + 127) & ~127);
    return rect;
}


/***********************************************************************
 *              CreateWindowSurface   (MACDRV.@)
 */
BOOL macdrv_CreateWindowSurface(HWND hwnd, UINT swp_flags, const RECT *visible_rect, struct window_surface **surface)
{
    struct macdrv_win_data *data;
    DWORD style = NtUserGetWindowLongW(hwnd, GWL_STYLE);
    RECT surface_rect;

    TRACE("hwnd %p, swp_flags %08x, visible %s, surface %p\n", hwnd, swp_flags, wine_dbgstr_rect(visible_rect), surface);

    if (!(data = get_win_data(hwnd))) return TRUE; /* use default surface */

    if (*surface) window_surface_release(*surface);
    *surface = NULL;

    surface_rect = get_surface_rect(visible_rect);
    if (data->surface)
    {
        if (EqualRect(&data->surface->rect, &surface_rect))
        {
            /* existing surface is good enough */
            surface_clip_to_visible_rect(data->surface, visible_rect);
            window_surface_add_ref(data->surface);
            *surface = data->surface;
            goto done;
        }
    }
    else if (!(swp_flags & SWP_SHOWWINDOW) && !(style & WS_VISIBLE)) goto done;

    *surface = create_surface(data->hwnd, data->cocoa_window, &surface_rect, data->surface, FALSE);

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
    else set_surface_use_alpha(surface, TRUE);

    if ((*window_surface = surface)) window_surface_add_ref(surface);

    release_win_data(data);

    return TRUE;
}

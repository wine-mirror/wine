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

static inline void reset_bounds(RECT *bounds)
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}


struct macdrv_window_surface
{
    struct window_surface   header;
    macdrv_window           window;
    HRGN                    region;
    BOOL                    use_alpha;
    BYTE                   *bits;
    BITMAPINFO              info;   /* variable size, must be last */
};

static struct macdrv_window_surface *get_mac_surface(struct window_surface *surface);

/***********************************************************************
 *              macdrv_surface_get_bitmap_info
 */
static void *macdrv_surface_get_bitmap_info(struct window_surface *window_surface,
                                                  BITMAPINFO *info)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    memcpy(info, &surface->info, get_dib_info_size(&surface->info, DIB_RGB_COLORS));
    return surface->bits;
}

/***********************************************************************
 *              macdrv_surface_set_region
 */
static void macdrv_surface_set_region(struct window_surface *window_surface, HRGN region)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    TRACE("updating surface %p with %p\n", surface, region);

    window_surface_lock(window_surface);

    if (region)
    {
        if (!surface->region) surface->region = NtGdiCreateRectRgn(0, 0, 0, 0);
        NtGdiCombineRgn(surface->region, region, 0, RGN_COPY);
    }
    else
    {
        if (surface->region) NtGdiDeleteObjectApp(surface->region);
        surface->region = 0;
    }

    window_surface_unlock(window_surface);
}

/***********************************************************************
 *              macdrv_surface_flush
 */
static BOOL macdrv_surface_flush(struct window_surface *window_surface, const RECT *rect, const RECT *dirty)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    macdrv_window_needs_display(surface->window, cgrect_from_rect(*dirty));
    return FALSE; /* bounds are reset asynchronously, from macdrv_get_surface_display_image */
}

/***********************************************************************
 *              macdrv_surface_destroy
 */
static void macdrv_surface_destroy(struct window_surface *window_surface)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    TRACE("freeing %p bits %p\n", surface, surface->bits);
    if (surface->region) NtGdiDeleteObjectApp(surface->region);
    free(surface->bits);
    free(surface);
}

static const struct window_surface_funcs macdrv_surface_funcs =
{
    macdrv_surface_get_bitmap_info,
    macdrv_surface_set_region,
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
struct window_surface *create_surface(macdrv_window window, const RECT *rect,
                                      struct window_surface *old_surface, BOOL use_alpha)
{
    struct macdrv_window_surface *surface;
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    DWORD *colors;
    DWORD window_background;

    surface = calloc(1, FIELD_OFFSET(struct macdrv_window_surface, info.bmiColors[3]));
    if (!surface) return NULL;
    window_surface_init(&surface->header, &macdrv_surface_funcs, rect);

    surface->info.bmiHeader.biSize        = sizeof(surface->info.bmiHeader);
    surface->info.bmiHeader.biWidth       = width;
    surface->info.bmiHeader.biHeight      = -height; /* top-down */
    surface->info.bmiHeader.biPlanes      = 1;
    surface->info.bmiHeader.biBitCount    = 32;
    surface->info.bmiHeader.biSizeImage   = get_dib_image_size(&surface->info);
    surface->info.bmiHeader.biCompression = BI_RGB;
    surface->info.bmiHeader.biClrUsed     = 0;

    colors = (DWORD *)((char *)&surface->info + surface->info.bmiHeader.biSize);
    colors[0] = 0x00ff0000;
    colors[1] = 0x0000ff00;
    colors[2] = 0x000000ff;

    surface->window = window;
    if (old_surface) surface->header.bounds = old_surface->bounds;
    surface->use_alpha = use_alpha;
    surface->bits = malloc(surface->info.bmiHeader.biSizeImage);
    if (!surface->bits) goto failed;
    window_background = macdrv_window_background_color();
    memset_pattern4(surface->bits, &window_background, surface->info.bmiHeader.biSizeImage);

    TRACE("created %p for %p %s bits %p-%p\n", surface, window, wine_dbgstr_rect(rect),
          surface->bits, surface->bits + surface->info.bmiHeader.biSizeImage);

    return &surface->header;

failed:
    window_surface_release(&surface->header);
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
 *              create_surface_image
 *
 * Caller must hold the surface lock.  On input, *rect is the requested
 * image rect, relative to the window whole_rect, a.k.a. visible_rect.
 * On output, it's been intersected with that part backed by the surface
 * and is the actual size of the returned image.  copy_data indicates if
 * the caller will keep the returned image beyond the point where the
 * surface bits can be guaranteed to remain valid and unchanged.  If so,
 * the bits are copied instead of merely referenced by the image.
 *
 * IMPORTANT: This function is called from non-Wine threads, so it
 *            must not use Win32 or Wine functions, including debug
 *            logging.
 */
CGImageRef macdrv_get_surface_display_image(struct window_surface *window_surface, CGRect *rect, int copy_data, int color_keyed,
        CGFloat key_red, CGFloat key_green, CGFloat key_blue)
{
    CGImageRef cgimage = NULL;
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    RECT surface_rect = window_surface->rect;
    int width, height;

    pthread_mutex_lock(&window_surface->mutex);
    if (IsRectEmpty(&window_surface->bounds)) goto done;

    width  = surface_rect.right - surface_rect.left;
    height = surface_rect.bottom - surface_rect.top;
    *rect = CGRectIntersection(cgrect_from_rect(surface_rect), *rect);
    if (!CGRectIsEmpty(*rect))
    {
        CGRect visrect;
        CGColorSpaceRef colorspace;
        CGDataProviderRef provider;
        int bytes_per_row, offset, size;
        CGImageAlphaInfo alphaInfo;

        visrect = CGRectOffset(*rect, -surface_rect.left, -surface_rect.top);

        colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        bytes_per_row = get_dib_stride(width, 32);
        offset = CGRectGetMinX(visrect) * 4 + CGRectGetMinY(visrect) * bytes_per_row;
        size = min(CGRectGetHeight(visrect) * bytes_per_row,
                   surface->info.bmiHeader.biSizeImage - offset);

        if (copy_data)
        {
            CFDataRef data = CFDataCreate(NULL, (UInt8*)surface->bits + offset, size);
            provider = CGDataProviderCreateWithCFData(data);
            CFRelease(data);
        }
        else
            provider = CGDataProviderCreateWithData(NULL, surface->bits + offset, size, NULL);

        alphaInfo = surface->use_alpha ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst;
        cgimage = CGImageCreate(CGRectGetWidth(visrect), CGRectGetHeight(visrect),
                                8, 32, bytes_per_row, colorspace,
                                alphaInfo | kCGBitmapByteOrder32Little,
                                provider, NULL, retina_on, kCGRenderingIntentDefault);
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(colorspace);

        if (color_keyed)
        {
            CGImageRef maskedImage;
            CGFloat components[] = { key_red   - 0.5, key_red   + 0.5,
                                     key_green - 0.5, key_green + 0.5,
                                     key_blue  - 0.5, key_blue  + 0.5 };
            maskedImage = CGImageCreateWithMaskingColors(cgimage, components);
            if (maskedImage)
            {
                CGImageRelease(cgimage);
                cgimage = maskedImage;
            }
        }
    }

done:
    reset_bounds(&window_surface->bounds);
    pthread_mutex_unlock(&window_surface->mutex);
    return cgimage;
}

/***********************************************************************
 *              surface_clip_to_visible_rect
 *
 * Intersect the accumulated drawn region with a new visible rect,
 * effectively discarding stale drawing in the surface slack area.
 */
void surface_clip_to_visible_rect(struct window_surface *window_surface, const RECT *visible_rect)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    RECT rect = *visible_rect;
    OffsetRect(&rect, -rect.left, -rect.top);

    if (!surface) return;

    window_surface_lock(window_surface);
    intersect_rect(&window_surface->bounds, &window_surface->bounds, &rect);
    window_surface_unlock(window_surface);
}

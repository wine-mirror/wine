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
    BOOL                    use_alpha;
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
    macdrv_window_needs_display(surface->window, cgrect_from_rect(*dirty));
    return FALSE; /* bounds are reset asynchronously, from macdrv_get_surface_display_image */
}

/***********************************************************************
 *              macdrv_surface_destroy
 */
static void macdrv_surface_destroy(struct window_surface *window_surface)
{
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);

    TRACE("freeing %p bits %p\n", surface, window_surface->color_bits);
    free(window_surface->color_bits);
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
struct window_surface *create_surface(HWND hwnd, macdrv_window window, const RECT *rect,
                                      struct window_surface *old_surface, BOOL use_alpha)
{
    struct macdrv_window_surface *surface;
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    DWORD window_background;
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;

    memset(info, 0, sizeof(*info));
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = width;
    info->bmiHeader.biHeight      = -height; /* top-down */
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biSizeImage   = get_dib_image_size(info);
    info->bmiHeader.biCompression = BI_RGB;

    surface = calloc(1, FIELD_OFFSET(struct macdrv_window_surface, info.bmiColors[3]));
    if (!surface) return NULL;
    if (!window_surface_init(&surface->header, &macdrv_surface_funcs, hwnd, info, 0)) goto failed;
    memcpy(&surface->info, info, offsetof(BITMAPINFO, bmiColors[3]));

    surface->window = window;
    if (old_surface) surface->header.bounds = old_surface->bounds;
    surface->use_alpha = use_alpha;
    surface->header.color_bits = malloc(info->bmiHeader.biSizeImage);
    if (!surface->header.color_bits) goto failed;
    window_background = macdrv_window_background_color();
    memset_pattern4(surface->header.color_bits, &window_background, info->bmiHeader.biSizeImage);

    TRACE("created %p for %p %s color_bits %p-%p\n", surface, window, wine_dbgstr_rect(rect),
          surface->header.color_bits, (char *)surface->header.color_bits + info->bmiHeader.biSizeImage);

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
            CFDataRef data = CFDataCreate(NULL, (UInt8 *)window_surface->color_bits + offset, size);
            provider = CGDataProviderCreateWithCFData(data);
            CFRelease(data);
        }
        else
            provider = CGDataProviderCreateWithData(NULL, (UInt8 *)window_surface->color_bits + offset, size, NULL);

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

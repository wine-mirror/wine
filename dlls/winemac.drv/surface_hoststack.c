
#include "config.h"

#include "macdrv.h"

static inline int get_dib_stride(int width, int bpp)
{
    return ((width * bpp + 31) >> 3) & ~3;
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
CGImageRef create_surface_image(void * WIN32PTR window_surface, CGRect *rect, int copy_data)
{
    CGImageRef cgimage = NULL;
    struct macdrv_window_surface *surface = get_mac_surface(window_surface);
    int width, height;

    width  = surface->header.rect.right - surface->header.rect.left;
    height = surface->header.rect.bottom - surface->header.rect.top;
    *rect = CGRectIntersection(cgrect_from_rect(surface->header.rect), *rect);
    if (!CGRectIsEmpty(*rect))
    {
        CGRect visrect;
        CGColorSpaceRef colorspace;
        CGDataProviderRef provider;
        int bytes_per_row, offset, size;
        CGImageAlphaInfo alphaInfo;

        visrect = CGRectOffset(*rect, -surface->header.rect.left, -surface->header.rect.top);

        colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        bytes_per_row = get_dib_stride(width, 32);
        offset = CGRectGetMinX(visrect) * 4 + (height - CGRectGetMaxY(visrect)) * bytes_per_row;
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
    }

    return cgimage;
}

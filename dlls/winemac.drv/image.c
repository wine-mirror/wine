/*
 * MACDRV image functions
 *
 * Copyright 2013 Ken Thomases for CodeWeavers Inc.
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

WINE_DEFAULT_DEBUG_CHANNEL(image);


/***********************************************************************
 *              create_cgimage_from_icon_bitmaps
 */
CGImageRef create_cgimage_from_icon_bitmaps(HDC hdc, HANDLE icon, HBITMAP hbmColor,
                                            unsigned char *color_bits, int color_size, HBITMAP hbmMask,
                                            unsigned char *mask_bits, int mask_size, int width,
                                            int height, int istep)
{
    int i;
    BOOL has_alpha = FALSE;
    DWORD *ptr;
    CGBitmapInfo alpha_format;
    CGColorSpaceRef colorspace;
    CFDataRef data;
    CGDataProviderRef provider;
    CGImageRef cgimage;

    /* draw the cursor frame to a temporary buffer then create a CGImage from that */
    memset(color_bits, 0x00, color_size);
    NtGdiSelectBitmap(hdc, hbmColor);
    if (!NtUserDrawIconEx(hdc, 0, 0, icon, width, height, istep, NULL, DI_NORMAL))
    {
        WARN("Could not draw frame %d (walk past end of frames).\n", istep);
        return NULL;
    }

    /* check if the cursor frame was drawn with an alpha channel */
    for (i = 0, ptr = (DWORD*)color_bits; i < width * height; i++, ptr++)
        if ((has_alpha = (*ptr & 0xff000000) != 0)) break;

    if (has_alpha)
        alpha_format = kCGImageAlphaFirst;
    else
        alpha_format = kCGImageAlphaNoneSkipFirst;

    colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    if (!colorspace)
    {
        WARN("failed to create colorspace\n");
        return NULL;
    }

    data = CFDataCreate(NULL, (UInt8*)color_bits, color_size);
    if (!data)
    {
        WARN("failed to create data\n");
        CGColorSpaceRelease(colorspace);
        return NULL;
    }

    provider = CGDataProviderCreateWithCFData(data);
    CFRelease(data);
    if (!provider)
    {
        WARN("failed to create data provider\n");
        CGColorSpaceRelease(colorspace);
        return NULL;
    }

    cgimage = CGImageCreate(width, height, 8, 32, width * 4, colorspace,
                            alpha_format | kCGBitmapByteOrder32Little,
                            provider, NULL, FALSE, kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorspace);
    if (!cgimage)
    {
        WARN("failed to create image\n");
        return NULL;
    }

    /* if no alpha channel was drawn then generate it from the mask */
    if (!has_alpha)
    {
        unsigned int width_bytes = (width + 31) / 32 * 4;
        CGImageRef cgmask, temp;

        /* draw the cursor mask to a temporary buffer */
        memset(mask_bits, 0xFF, mask_size);
        NtGdiSelectBitmap(hdc, hbmMask);
        if (!NtUserDrawIconEx(hdc, 0, 0, icon, width, height, istep, NULL, DI_MASK))
        {
            WARN("Failed to draw frame mask %d.\n", istep);
            CGImageRelease(cgimage);
            return NULL;
        }

        data = CFDataCreate(NULL, (UInt8*)mask_bits, mask_size);
        if (!data)
        {
            WARN("failed to create data\n");
            CGImageRelease(cgimage);
            return NULL;
        }

        provider = CGDataProviderCreateWithCFData(data);
        CFRelease(data);
        if (!provider)
        {
            WARN("failed to create data provider\n");
            CGImageRelease(cgimage);
            return NULL;
        }

        cgmask = CGImageMaskCreate(width, height, 1, 1, width_bytes, provider, NULL, FALSE);
        CGDataProviderRelease(provider);
        if (!cgmask)
        {
            WARN("failed to create mask\n");
            CGImageRelease(cgimage);
            return NULL;
        }

        temp = CGImageCreateWithMask(cgimage, cgmask);
        CGImageRelease(cgmask);
        CGImageRelease(cgimage);
        if (!temp)
        {
            WARN("failed to create masked image\n");
            return NULL;
        }
        cgimage = temp;
    }

    return cgimage;
}


/***********************************************************************
 *              create_cgimage_from_icon
 *
 * Create a CGImage from a Windows icon.
 */
CGImageRef create_cgimage_from_icon(HANDLE icon, int width, int height)
{
    CGImageRef ret = NULL;
    HDC hdc;
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bitmapinfo = (BITMAPINFO*)buffer;
    unsigned char *color_bits, *mask_bits;
    HBITMAP hbmColor = 0, hbmMask = 0;
    int color_size, mask_size;

    TRACE("icon %p width %d height %d\n", icon, width, height);

    if (!width && !height)
    {
        ICONINFO info;
        BITMAP bm;

        if (!NtUserGetIconInfo(icon, &info, NULL, NULL, NULL, 0))
            return NULL;

        NtGdiExtGetObjectW(info.hbmMask, sizeof(bm), &bm);
        if (!info.hbmColor) bm.bmHeight = max(1, bm.bmHeight / 2);
        width = bm.bmWidth;
        height = bm.bmHeight;
        TRACE("new width %d height %d\n", width, height);

        NtGdiDeleteObjectApp(info.hbmColor);
        NtGdiDeleteObjectApp(info.hbmMask);
    }

    hdc = NtGdiCreateCompatibleDC(0);

    bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo->bmiHeader.biWidth = width;
    bitmapinfo->bmiHeader.biHeight = -height;
    bitmapinfo->bmiHeader.biPlanes = 1;
    bitmapinfo->bmiHeader.biCompression = BI_RGB;
    bitmapinfo->bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo->bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo->bmiHeader.biClrUsed = 0;
    bitmapinfo->bmiHeader.biClrImportant = 0;
    bitmapinfo->bmiHeader.biBitCount = 32;
    color_size = width * height * 4;
    bitmapinfo->bmiHeader.biSizeImage = color_size;
    hbmColor = NtGdiCreateDIBSection(hdc, NULL, 0, bitmapinfo, DIB_RGB_COLORS,
                                     0, 0, 0, (void **)&color_bits);
    if (!hbmColor)
    {
        WARN("failed to create DIB section for cursor color data\n");
        goto cleanup;
    }

    bitmapinfo->bmiHeader.biBitCount = 1;
    bitmapinfo->bmiColors[0].rgbRed      = 0;
    bitmapinfo->bmiColors[0].rgbGreen    = 0;
    bitmapinfo->bmiColors[0].rgbBlue     = 0;
    bitmapinfo->bmiColors[0].rgbReserved = 0;
    bitmapinfo->bmiColors[1].rgbRed      = 0xff;
    bitmapinfo->bmiColors[1].rgbGreen    = 0xff;
    bitmapinfo->bmiColors[1].rgbBlue     = 0xff;
    bitmapinfo->bmiColors[1].rgbReserved = 0;
    mask_size = ((width + 31) / 32 * 4) * height;
    bitmapinfo->bmiHeader.biSizeImage = mask_size;
    hbmMask = NtGdiCreateDIBSection(hdc, NULL, 0, bitmapinfo, DIB_RGB_COLORS,
                                    0, 0, 0, (void **)&mask_bits);
    if (!hbmMask)
    {
        WARN("failed to create DIB section for cursor mask data\n");
        goto cleanup;
    }

    ret = create_cgimage_from_icon_bitmaps(hdc, icon, hbmColor, color_bits, color_size, hbmMask,
                                           mask_bits, mask_size, width, height, 0);

cleanup:
    if (hbmColor) NtGdiDeleteObjectApp(hbmColor);
    if (hbmMask) NtGdiDeleteObjectApp(hbmMask);
    NtGdiDeleteObjectApp(hdc);
    return ret;
}


/***********************************************************************
 *              create_app_icon_images
 */
CFArrayRef create_app_icon_images(void)
{
    struct dispatch_callback_params params = {.callback = app_icon_callback};
    CFMutableArrayRef images = NULL;
    struct app_icon_entry *entries;
    ULONG ret_len;
    unsigned count;
    int i;

    TRACE("()\n");

    if (KeUserDispatchCallback(&params, sizeof(params), (void**)&entries, &ret_len) ||
        (ret_len % sizeof(*entries)))
    {
        WARN("incorrect callback result\n");
        return NULL;
    }
    count = ret_len / sizeof(*entries);
    if (!count || !entries) return NULL;

    images = CFArrayCreateMutable(NULL, count, &kCFTypeArrayCallBacks);
    if (!images)
    {
        WARN("failed to create images array\n");
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        struct app_icon_entry *icon = &entries[i];
        CGImageRef cgimage = NULL;

        if (icon->png)
        {
            CFDataRef data = CFDataCreate(NULL, param_ptr(icon->png), icon->size);
            if (data)
            {
                CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
                CFRelease(data);
                if (provider)
                {
                    cgimage = CGImageCreateWithPNGDataProvider(provider, NULL, FALSE,
                                                               kCGRenderingIntentDefault);
                    CGDataProviderRelease(provider);
                }
            }
        }
        else
        {
            HICON handle = UlongToHandle(icon->icon);
            cgimage = create_cgimage_from_icon(handle, icon->width, icon->height);
            NtUserDestroyCursor(handle, 0);
        }

        if (cgimage)
        {
            CFArrayAppendValue(images, cgimage);
            CGImageRelease(cgimage);
        }
    }

    if (images && !CFArrayGetCount(images))
    {
        CFRelease(images);
        images = NULL;
    }

    return images;
}

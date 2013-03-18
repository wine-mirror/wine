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
    int i, has_alpha = FALSE;
    DWORD *ptr;
    CGBitmapInfo alpha_format;
    CGColorSpaceRef colorspace;
    CFDataRef data;
    CGDataProviderRef provider;
    CGImageRef cgimage;

    /* draw the cursor frame to a temporary buffer then create a CGImage from that */
    memset(color_bits, 0x00, color_size);
    SelectObject(hdc, hbmColor);
    if (!DrawIconEx(hdc, 0, 0, icon, width, height, istep, NULL, DI_NORMAL))
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

    colorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
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
        SelectObject(hdc, hbmMask);
        if (!DrawIconEx(hdc, 0, 0, icon, width, height, istep, NULL, DI_MASK))
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

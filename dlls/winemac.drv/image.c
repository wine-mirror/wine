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

#include "pshpack1.h"

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
} GRPICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY idEntries[1];
} GRPICONDIR;

#include "poppack.h"


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

        if (!GetIconInfo(icon, &info))
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
 *              get_first_resource
 *
 * Helper for create_app_icon_images().  Enum proc for EnumResourceNamesW()
 * which just gets the handle for the first resource and stops further
 * enumeration.
 */
static BOOL CALLBACK get_first_resource(HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR lparam)
{
    HRSRC *res_info = (HRSRC*)lparam;

    *res_info = FindResourceW(module, name, (LPCWSTR)RT_GROUP_ICON);
    return FALSE;
}


/***********************************************************************
 *              create_app_icon_images
 */
CFArrayRef create_app_icon_images(void)
{
    HRSRC res_info;
    HGLOBAL res_data;
    GRPICONDIR *icon_dir;
    CFMutableArrayRef images = NULL;
    int i;

    TRACE("()\n");

    res_info = NULL;
    EnumResourceNamesW(NULL, (LPCWSTR)RT_GROUP_ICON, get_first_resource, (LONG_PTR)&res_info);
    if (!res_info)
    {
        WARN("found no RT_GROUP_ICON resource\n");
        return NULL;
    }

    if (!(res_data = LoadResource(NULL, res_info)))
    {
        WARN("failed to load RT_GROUP_ICON resource\n");
        return NULL;
    }

    if (!(icon_dir = LockResource(res_data)))
    {
        WARN("failed to lock RT_GROUP_ICON resource\n");
        goto cleanup;
    }

    images = CFArrayCreateMutable(NULL, icon_dir->idCount, &kCFTypeArrayCallBacks);
    if (!images)
    {
        WARN("failed to create images array\n");
        goto cleanup;
    }

    for (i = 0; i < icon_dir->idCount; i++)
    {
        int width = icon_dir->idEntries[i].bWidth;
        int height = icon_dir->idEntries[i].bHeight;
        BOOL found_better_bpp = FALSE;
        int j;
        LPCWSTR name;
        HGLOBAL icon_res_data;
        BYTE *icon_bits;

        if (!width) width = 256;
        if (!height) height = 256;

        /* If there's another icon at the same size but with better
           color depth, skip this one.  We end up making CGImages that
           are all 32 bits per pixel, so Cocoa doesn't get the original
           color depth info to pick the best representation itself. */
        for (j = 0; j < icon_dir->idCount; j++)
        {
            int jwidth = icon_dir->idEntries[j].bWidth;
            int jheight = icon_dir->idEntries[j].bHeight;

            if (!jwidth) jwidth = 256;
            if (!jheight) jheight = 256;

            if (j != i && jwidth == width && jheight == height &&
                icon_dir->idEntries[j].wBitCount > icon_dir->idEntries[i].wBitCount)
            {
                found_better_bpp = TRUE;
                break;
            }
        }

        if (found_better_bpp) continue;

        name = MAKEINTRESOURCEW(icon_dir->idEntries[i].nID);
        res_info = FindResourceW(NULL, name, (LPCWSTR)RT_ICON);
        if (!res_info)
        {
            WARN("failed to find RT_ICON resource %d with ID %hd\n", i, icon_dir->idEntries[i].nID);
            continue;
        }

        icon_res_data = LoadResource(NULL, res_info);
        if (!icon_res_data)
        {
            WARN("failed to load icon %d with ID %hd\n", i, icon_dir->idEntries[i].nID);
            continue;
        }

        icon_bits = LockResource(icon_res_data);
        if (icon_bits)
        {
            static const BYTE png_magic[] = { 0x89, 0x50, 0x4e, 0x47 };
            CGImageRef cgimage = NULL;

            if (!memcmp(icon_bits, png_magic, sizeof(png_magic)))
            {
                CFDataRef data = CFDataCreate(NULL, (UInt8*)icon_bits, icon_dir->idEntries[i].dwBytesInRes);
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

            if (!cgimage)
            {
                HICON icon;
                icon = CreateIconFromResourceEx(icon_bits, icon_dir->idEntries[i].dwBytesInRes,
                                                TRUE, 0x00030000, width, height, 0);
                if (icon)
                {
                    cgimage = create_cgimage_from_icon(icon, width, height);
                    DestroyIcon(icon);
                }
                else
                    WARN("failed to create icon %d from resource with ID %hd\n", i, icon_dir->idEntries[i].nID);
            }

            if (cgimage)
            {
                CFArrayAppendValue(images, cgimage);
                CGImageRelease(cgimage);
            }
        }
        else
            WARN("failed to lock RT_ICON resource %d with ID %hd\n", i, icon_dir->idEntries[i].nID);

        FreeResource(icon_res_data);
    }

cleanup:
    if (images && !CFArrayGetCount(images))
    {
        CFRelease(images);
        images = NULL;
    }
    FreeResource(res_data);

    return images;
}

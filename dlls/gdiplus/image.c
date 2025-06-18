/*
 * Copyright (C) 2007 Google (Evan Stade)
 * Copyright (C) 2012,2016 Dmitry Timoshkov
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#define COBJMACROS
#include "objbase.h"
#include "olectl.h"
#include "ole2.h"

#include "initguid.h"
#include "wincodec.h"
#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT, IWICImagingFactory**);

#define WMF_PLACEABLE_KEY 0x9ac6cdd7

static const struct
{
    const WICPixelFormatGUID *wic_format;
    PixelFormat gdip_format;
    /* predefined palette type to use for pixel format conversions */
    WICBitmapPaletteType palette_type;
} pixel_formats[] =
{
    { &GUID_WICPixelFormat1bppIndexed, PixelFormat1bppIndexed, WICBitmapPaletteTypeFixedBW },
    { &GUID_WICPixelFormatBlackWhite, PixelFormat1bppIndexed, WICBitmapPaletteTypeFixedBW },
    { &GUID_WICPixelFormat4bppIndexed, PixelFormat4bppIndexed, WICBitmapPaletteTypeFixedHalftone8 },
    { &GUID_WICPixelFormat8bppIndexed, PixelFormat8bppIndexed, WICBitmapPaletteTypeFixedHalftone256 },
    { &GUID_WICPixelFormat8bppGray, PixelFormat8bppIndexed, WICBitmapPaletteTypeFixedGray256 },
    { &GUID_WICPixelFormat16bppBGR555, PixelFormat16bppRGB555, 0 },
    { &GUID_WICPixelFormat24bppBGR, PixelFormat24bppRGB, 0 },
    { &GUID_WICPixelFormat32bppBGR, PixelFormat32bppRGB, 0 },
    { &GUID_WICPixelFormat32bppBGRA, PixelFormat32bppARGB, 0 },
    { &GUID_WICPixelFormat32bppCMYK, PixelFormat32bppCMYK, 0 },
    { &GUID_WICPixelFormat32bppGrayFloat, PixelFormat32bppARGB, 0 },
    { &GUID_WICPixelFormat32bppPBGRA, PixelFormat32bppPARGB, 0 },
    { &GUID_WICPixelFormat48bppRGB, PixelFormat48bppRGB, 0 },
    { &GUID_WICPixelFormat64bppCMYK, PixelFormat48bppRGB, 0 },
    { &GUID_WICPixelFormat64bppRGBA, PixelFormat48bppRGB, 0 },
    { NULL }
};

static ColorPalette *get_palette(IWICBitmapFrameDecode *frame, WICBitmapPaletteType palette_type)
{
    HRESULT hr;
    IWICImagingFactory *factory;
    IWICPalette *wic_palette;
    ColorPalette *palette = NULL;

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (hr != S_OK) return NULL;

    hr = IWICImagingFactory_CreatePalette(factory, &wic_palette);
    if (hr == S_OK)
    {
        hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
        if (frame)
            hr = IWICBitmapFrameDecode_CopyPalette(frame, wic_palette);
        if (hr != S_OK && palette_type != 0)
        {
            TRACE("using predefined palette %#x\n", palette_type);
            hr = IWICPalette_InitializePredefined(wic_palette, palette_type, FALSE);
        }
        if (hr == S_OK)
        {
            WICBitmapPaletteType type;
            BOOL alpha;
            UINT count;

            IWICPalette_GetColorCount(wic_palette, &count);
            palette = malloc(2 * sizeof(UINT) + count * sizeof(ARGB));
            IWICPalette_GetColors(wic_palette, count, (UINT *)palette->Entries, &palette->Count);

            IWICPalette_GetType(wic_palette, &type);
            switch(type) {
                case WICBitmapPaletteTypeFixedGray4:
                case WICBitmapPaletteTypeFixedGray16:
                case WICBitmapPaletteTypeFixedGray256:
                    palette->Flags = PaletteFlagsGrayScale;
                    break;
                case WICBitmapPaletteTypeFixedHalftone8:
                case WICBitmapPaletteTypeFixedHalftone27:
                case WICBitmapPaletteTypeFixedHalftone64:
                case WICBitmapPaletteTypeFixedHalftone125:
                case WICBitmapPaletteTypeFixedHalftone216:
                case WICBitmapPaletteTypeFixedHalftone252:
                case WICBitmapPaletteTypeFixedHalftone256:
                    palette->Flags = PaletteFlagsHalftone;
                    break;
                default:
                    palette->Flags = 0;
            }
            IWICPalette_HasAlpha(wic_palette, &alpha);
            if(alpha)
                palette->Flags |= PaletteFlagsHasAlpha;
        }
        IWICPalette_Release(wic_palette);
    }
    IWICImagingFactory_Release(factory);
    return palette;
}

static HRESULT set_palette(IWICBitmapFrameEncode *frame, ColorPalette *palette)
{
    HRESULT hr;
    IWICImagingFactory *factory;
    IWICPalette *wic_palette;

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (FAILED(hr))
        return hr;

    hr = IWICImagingFactory_CreatePalette(factory, &wic_palette);
    IWICImagingFactory_Release(factory);
    if (SUCCEEDED(hr))
    {
        hr = IWICPalette_InitializeCustom(wic_palette, (UINT *)palette->Entries, palette->Count);

        if (SUCCEEDED(hr))
            hr = IWICBitmapFrameEncode_SetPalette(frame, wic_palette);

        IWICPalette_Release(wic_palette);
    }

    return hr;
}

GpStatus WINGDIPAPI GdipBitmapApplyEffect(GpBitmap* bitmap, CGpEffect* effect,
    RECT* roi, BOOL useAuxData, VOID** auxData, INT* auxDataSize)
{
    FIXME("(%p %p %p %d %p %p): stub\n", bitmap, effect, roi, useAuxData, auxData, auxDataSize);
    /*
     * Note: According to Jose Roca's GDI+ docs, this function is not
     * implemented in Windows's GDI+.
     */
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipBitmapCreateApplyEffect(GpBitmap** inputBitmaps,
    INT numInputs, CGpEffect* effect, RECT* roi, RECT* outputRect,
    GpBitmap** outputBitmap, BOOL useAuxData, VOID** auxData, INT* auxDataSize)
{
    FIXME("(%p %d %p %p %p %p %d %p %p): stub\n", inputBitmaps, numInputs, effect, roi, outputRect, outputBitmap, useAuxData, auxData, auxDataSize);
    /*
     * Note: According to Jose Roca's GDI+ docs, this function is not
     * implemented in Windows's GDI+.
     */
    return NotImplemented;
}

static inline void getpixel_1bppIndexed(BYTE *index, const BYTE *row, UINT x)
{
    *index = (row[x/8]>>(7-x%8)) & 1;
}

static inline void getpixel_4bppIndexed(BYTE *index, const BYTE *row, UINT x)
{
    if (x & 1)
        *index = row[x/2]&0xf;
    else
        *index = row[x/2]>>4;
}

static inline void getpixel_8bppIndexed(BYTE *index, const BYTE *row, UINT x)
{
    *index = row[x];
}

static inline void getpixel_16bppGrayScale(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *r = *g = *b = row[x*2+1];
    *a = 255;
}

static inline void getpixel_16bppRGB555(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    WORD pixel = *((const WORD*)(row)+x);
    *r = (pixel>>7&0xf8)|(pixel>>12&0x7);
    *g = (pixel>>2&0xf8)|(pixel>>6&0x7);
    *b = (pixel<<3&0xf8)|(pixel>>2&0x7);
    *a = 255;
}

static inline void getpixel_16bppRGB565(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    WORD pixel = *((const WORD*)(row)+x);
    *r = (pixel>>8&0xf8)|(pixel>>13&0x7);
    *g = (pixel>>3&0xfc)|(pixel>>9&0x3);
    *b = (pixel<<3&0xf8)|(pixel>>2&0x7);
    *a = 255;
}

static inline void getpixel_16bppARGB1555(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    WORD pixel = *((const WORD*)(row)+x);
    *r = (pixel>>7&0xf8)|(pixel>>12&0x7);
    *g = (pixel>>2&0xf8)|(pixel>>6&0x7);
    *b = (pixel<<3&0xf8)|(pixel>>2&0x7);
    if ((pixel&0x8000) == 0x8000)
        *a = 255;
    else
        *a = 0;
}

static inline void getpixel_24bppRGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *r = row[x*3+2];
    *g = row[x*3+1];
    *b = row[x*3];
    *a = 255;
}

static inline void getpixel_32bppRGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *r = row[x*4+2];
    *g = row[x*4+1];
    *b = row[x*4];
    *a = 255;
}

static inline void getpixel_32bppARGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *r = row[x*4+2];
    *g = row[x*4+1];
    *b = row[x*4];
    *a = row[x*4+3];
}

static inline void getpixel_32bppPARGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *a = row[x*4+3];
    if (*a == 0)
    {
        *r = row[x*4+2];
        *g = row[x*4+1];
        *b = row[x*4];
    }
    else
    {
        DWORD scaled_q = (255 << 15) / *a;
        *r = (row[x*4+2] > *a) ? 0xff : (row[x*4+2] * scaled_q) >> 15;
        *g = (row[x*4+1] > *a) ? 0xff : (row[x*4+1] * scaled_q) >> 15;
        *b = (row[x*4] > *a) ? 0xff : (row[x*4] * scaled_q) >> 15;
    }
}

static inline void getpixel_48bppRGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *r = row[x*6+5];
    *g = row[x*6+3];
    *b = row[x*6+1];
    *a = 255;
}

static inline void getpixel_64bppARGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *r = row[x*8+5];
    *g = row[x*8+3];
    *b = row[x*8+1];
    *a = row[x*8+7];
}

static inline void getpixel_64bppPARGB(BYTE *r, BYTE *g, BYTE *b, BYTE *a,
    const BYTE *row, UINT x)
{
    *a = row[x*8+7];
    if (*a == 0)
        *r = *g = *b = 0;
    else
    {
        *r = row[x*8+5] * 255 / *a;
        *g = row[x*8+3] * 255 / *a;
        *b = row[x*8+1] * 255 / *a;
    }
}

GpStatus WINGDIPAPI GdipBitmapGetPixel(GpBitmap* bitmap, INT x, INT y,
    ARGB *color)
{
    BYTE r, g, b, a;
    BYTE index;
    BYTE *row;

    if(!bitmap || !color ||
       x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height)
        return InvalidParameter;

    row = bitmap->bits+bitmap->stride*y;

    switch (bitmap->format)
    {
        case PixelFormat1bppIndexed:
            getpixel_1bppIndexed(&index,row,x);
            break;
        case PixelFormat4bppIndexed:
            getpixel_4bppIndexed(&index,row,x);
            break;
        case PixelFormat8bppIndexed:
            getpixel_8bppIndexed(&index,row,x);
            break;
        case PixelFormat16bppGrayScale:
            getpixel_16bppGrayScale(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat16bppRGB555:
            getpixel_16bppRGB555(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat16bppRGB565:
            getpixel_16bppRGB565(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat16bppARGB1555:
            getpixel_16bppARGB1555(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat24bppRGB:
            getpixel_24bppRGB(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat32bppRGB:
            getpixel_32bppRGB(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat32bppARGB:
            getpixel_32bppARGB(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat32bppPARGB:
            getpixel_32bppPARGB(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat48bppRGB:
            getpixel_48bppRGB(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat64bppARGB:
            getpixel_64bppARGB(&r,&g,&b,&a,row,x);
            break;
        case PixelFormat64bppPARGB:
            getpixel_64bppPARGB(&r,&g,&b,&a,row,x);
            break;
        default:
            FIXME("not implemented for format 0x%x\n", bitmap->format);
            return NotImplemented;
    }

    if (bitmap->format & PixelFormatIndexed)
        *color = bitmap->image.palette->Entries[index];
    else
        *color = a<<24|r<<16|g<<8|b;

    return Ok;
}

static unsigned int absdiff(unsigned int x, unsigned int y)
{
    return x > y ? x - y : y - x;
}

static inline UINT get_palette_index(BYTE r, BYTE g, BYTE b, BYTE a, ColorPalette *palette)
{
    BYTE index = 0;
    int best_distance = 0x7fff;
    int distance;
    UINT i;

    if (!palette) return 0;
    /* This algorithm scans entire palette,
       computes difference from desired color (all color components have equal weight)
       and returns the index of color with least difference.

       Note: Maybe it could be replaced with a better algorithm for better image quality
       and performance, though better algorithm would probably need some pre-built lookup
       tables and thus may actually be slower if this method is called only few times per
       every image.
    */
    for(i=0;i<palette->Count;i++) {
        ARGB color=palette->Entries[i];
        distance=absdiff(b, color & 0xff) + absdiff(g, color>>8 & 0xff) + absdiff(r, color>>16 & 0xff) + absdiff(a, color>>24 & 0xff);
        if (distance<best_distance) {
            best_distance=distance;
            index=i;
        }
    }
    return index;
}

static inline void setpixel_8bppIndexed(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x, ColorPalette *palette)
{
     BYTE index = get_palette_index(r,g,b,a,palette);
     row[x]=index;
}

static inline void setpixel_1bppIndexed(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x, ColorPalette *palette)
{
    row[x/8]  = (row[x/8] & ~(1<<(7-x%8))) | (get_palette_index(r,g,b,a,palette)<<(7-x%8));
}

static inline void setpixel_4bppIndexed(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x, ColorPalette *palette)
{
    if (x & 1)
        row[x/2] = (row[x/2] & 0xf0) | get_palette_index(r,g,b,a,palette);
    else
        row[x/2] = (row[x/2] & 0x0f) | get_palette_index(r,g,b,a,palette)<<4;
}

static inline void setpixel_16bppGrayScale(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    *((WORD*)(row)+x) = (r+g+b)*85;
}

static inline void setpixel_16bppRGB555(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    *((WORD*)(row)+x) = (r<<7&0x7c00)|
                        (g<<2&0x03e0)|
                        (b>>3&0x001f);
}

static inline void setpixel_16bppRGB565(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    *((WORD*)(row)+x) = (r<<8&0xf800)|
                         (g<<3&0x07e0)|
                         (b>>3&0x001f);
}

static inline void setpixel_16bppARGB1555(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    *((WORD*)(row)+x) = (a<<8&0x8000)|
                        (r<<7&0x7c00)|
                        (g<<2&0x03e0)|
                        (b>>3&0x001f);
}

static inline void setpixel_24bppRGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    row[x*3+2] = r;
    row[x*3+1] = g;
    row[x*3] = b;
}

static inline void setpixel_32bppRGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    *((DWORD*)(row)+x) = (r<<16)|(g<<8)|b;
}

static inline void setpixel_32bppARGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    *((DWORD*)(row)+x) = (a<<24)|(r<<16)|(g<<8)|b;
}

static inline void setpixel_32bppPARGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    r = (r * a + 127) / 255;
    g = (g * a + 127) / 255;
    b = (b * a + 127) / 255;
    *((DWORD*)(row)+x) = (a<<24)|(r<<16)|(g<<8)|b;
}

static inline void setpixel_48bppRGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    row[x*6+5] = row[x*6+4] = r;
    row[x*6+3] = row[x*6+2] = g;
    row[x*6+1] = row[x*6] = b;
}

static inline void setpixel_64bppARGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    UINT64 a64=a, r64=r, g64=g, b64=b;
    *((UINT64*)(row)+x) = (a64<<56)|(a64<<48)|(r64<<40)|(r64<<32)|(g64<<24)|(g64<<16)|(b64<<8)|b64;
}

static inline void setpixel_64bppPARGB(BYTE r, BYTE g, BYTE b, BYTE a,
    BYTE *row, UINT x)
{
    UINT64 a64, r64, g64, b64;
    a64 = a * 257;
    r64 = r * a / 255;
    g64 = g * a / 255;
    b64 = b * a / 255;
    *((UINT64*)(row)+x) = (a64<<48)|(r64<<32)|(g64<<16)|b64;
}

GpStatus WINGDIPAPI GdipBitmapSetPixel(GpBitmap* bitmap, INT x, INT y,
    ARGB color)
{
    BYTE a, r, g, b;
    BYTE *row;

    if(!bitmap || x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height)
        return InvalidParameter;

    a = color>>24;
    r = color>>16;
    g = color>>8;
    b = color;

    row = bitmap->bits + bitmap->stride * y;

    switch (bitmap->format)
    {
        case PixelFormat16bppGrayScale:
            setpixel_16bppGrayScale(r,g,b,a,row,x);
            break;
        case PixelFormat16bppRGB555:
            setpixel_16bppRGB555(r,g,b,a,row,x);
            break;
        case PixelFormat16bppRGB565:
            setpixel_16bppRGB565(r,g,b,a,row,x);
            break;
        case PixelFormat16bppARGB1555:
            setpixel_16bppARGB1555(r,g,b,a,row,x);
            break;
        case PixelFormat24bppRGB:
            setpixel_24bppRGB(r,g,b,a,row,x);
            break;
        case PixelFormat32bppRGB:
            setpixel_32bppRGB(r,g,b,a,row,x);
            break;
        case PixelFormat32bppARGB:
            setpixel_32bppARGB(r,g,b,a,row,x);
            break;
        case PixelFormat32bppPARGB:
            setpixel_32bppPARGB(r,g,b,a,row,x);
            break;
        case PixelFormat48bppRGB:
            setpixel_48bppRGB(r,g,b,a,row,x);
            break;
        case PixelFormat64bppARGB:
            setpixel_64bppARGB(r,g,b,a,row,x);
            break;
        case PixelFormat64bppPARGB:
            setpixel_64bppPARGB(r,g,b,a,row,x);
            break;
        case PixelFormat8bppIndexed:
            setpixel_8bppIndexed(r,g,b,a,row,x,bitmap->image.palette);
            break;
        case PixelFormat4bppIndexed:
            setpixel_4bppIndexed(r,g,b,a,row,x,bitmap->image.palette);
            break;
        case PixelFormat1bppIndexed:
            setpixel_1bppIndexed(r,g,b,a,row,x,bitmap->image.palette);
            break;
        default:
            FIXME("not implemented for format 0x%x\n", bitmap->format);
            return NotImplemented;
    }

    return Ok;
}

GpStatus convert_pixels(INT width, INT height,
    INT dst_stride, BYTE *dst_bits, PixelFormat dst_format,
    ColorPalette *dst_palette,
    INT src_stride, const BYTE *src_bits, PixelFormat src_format,
    ColorPalette *src_palette)
{
    INT x, y;

    if (src_format == dst_format ||
        (dst_format == PixelFormat32bppRGB && PIXELFORMATBPP(src_format) == 32))
    {
        UINT widthbytes = PIXELFORMATBPP(src_format) * width / 8;
        for (y=0; y<height; y++)
            memcpy(dst_bits+dst_stride*y, src_bits+src_stride*y, widthbytes);
        return Ok;
    }

#define convert_indexed_to_rgb(getpixel_function, setpixel_function) do { \
    for (y=0; y<height; y++) \
        for (x=0; x<width; x++) { \
            BYTE index; \
            ARGB argb; \
            BYTE *color = (BYTE *)&argb; \
            getpixel_function(&index, src_bits+src_stride*y, x); \
            argb = (src_palette && index < src_palette->Count) ? src_palette->Entries[index] : 0; \
            setpixel_function(color[2], color[1], color[0], color[3], dst_bits+dst_stride*y, x); \
        } \
    return Ok; \
} while (0);

#define convert_indexed_to_indexed(getpixel_function, setpixel_function) do { \
    for (y=0; y<height; y++) \
        for (x=0; x<width; x++) { \
            BYTE index; \
            ARGB argb; \
            BYTE *color = (BYTE *)&argb; \
            getpixel_function(&index, src_bits+src_stride*y, x); \
            argb = (src_palette && index < src_palette->Count) ? src_palette->Entries[index] : 0; \
            setpixel_function(color[2], color[1], color[0], color[3], dst_bits+dst_stride*y, x, dst_palette); \
        } \
    return Ok; \
} while (0);

#define convert_rgb_to_rgb(getpixel_function, setpixel_function) do { \
    for (y=0; y<height; y++) \
        for (x=0; x<width; x++) { \
            BYTE r, g, b, a; \
            getpixel_function(&r, &g, &b, &a, src_bits+src_stride*y, x); \
            setpixel_function(r, g, b, a, dst_bits+dst_stride*y, x); \
        } \
    return Ok; \
} while (0);

#define convert_rgb_to_indexed(getpixel_function, setpixel_function) do { \
    for (y=0; y<height; y++) \
        for (x=0; x<width; x++) { \
            BYTE r, g, b, a; \
            getpixel_function(&r, &g, &b, &a, src_bits+src_stride*y, x); \
            setpixel_function(r, g, b, a, dst_bits+dst_stride*y, x, dst_palette); \
        } \
    return Ok; \
} while (0);

    switch (src_format)
    {
    case PixelFormat1bppIndexed:
        switch (dst_format)
        {
        case PixelFormat4bppIndexed:
            convert_indexed_to_indexed(getpixel_1bppIndexed, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_indexed_to_indexed(getpixel_1bppIndexed, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_indexed_to_rgb(getpixel_1bppIndexed, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat4bppIndexed:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_indexed_to_indexed(getpixel_4bppIndexed, setpixel_1bppIndexed);
        case PixelFormat8bppIndexed:
            convert_indexed_to_indexed(getpixel_4bppIndexed, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_indexed_to_rgb(getpixel_4bppIndexed, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat8bppIndexed:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_indexed_to_indexed(getpixel_8bppIndexed, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_indexed_to_indexed(getpixel_8bppIndexed, setpixel_4bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_indexed_to_rgb(getpixel_8bppIndexed, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat16bppGrayScale:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppGrayScale, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppGrayScale, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppGrayScale, setpixel_8bppIndexed);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_16bppGrayScale, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat16bppRGB555:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppRGB555, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppRGB555, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppRGB555, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_16bppRGB555, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat16bppRGB565:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppRGB565, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppRGB565, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppRGB565, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_16bppRGB555);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_16bppRGB565, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat16bppARGB1555:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppARGB1555, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppARGB1555, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_16bppARGB1555, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_16bppRGB565);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_16bppARGB1555, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat24bppRGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_24bppRGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_24bppRGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_24bppRGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_16bppARGB1555);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_24bppRGB, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat32bppRGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppRGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppRGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppRGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_24bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_32bppRGB, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat32bppARGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppARGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppARGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppARGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_24bppRGB);
        case PixelFormat32bppPARGB:
            convert_32bppARGB_to_32bppPARGB(width, height, dst_bits, dst_stride, src_bits, src_stride);
            return Ok;
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_32bppARGB, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat32bppPARGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppPARGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppPARGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_32bppPARGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_32bppARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_32bppPARGB, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat48bppRGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_48bppRGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_48bppRGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_48bppRGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_32bppPARGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_48bppRGB, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    case PixelFormat64bppARGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_64bppARGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_64bppARGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_64bppARGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_64bppARGB, setpixel_48bppRGB);
        default:
            break;
        }
        break;
    case PixelFormat64bppPARGB:
        switch (dst_format)
        {
        case PixelFormat1bppIndexed:
            convert_rgb_to_indexed(getpixel_64bppPARGB, setpixel_1bppIndexed);
        case PixelFormat4bppIndexed:
            convert_rgb_to_indexed(getpixel_64bppPARGB, setpixel_4bppIndexed);
        case PixelFormat8bppIndexed:
            convert_rgb_to_indexed(getpixel_64bppPARGB, setpixel_8bppIndexed);
        case PixelFormat16bppGrayScale:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_16bppGrayScale);
        case PixelFormat16bppRGB555:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_16bppRGB555);
        case PixelFormat16bppRGB565:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_16bppRGB565);
        case PixelFormat16bppARGB1555:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_16bppARGB1555);
        case PixelFormat24bppRGB:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_24bppRGB);
        case PixelFormat32bppRGB:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_32bppRGB);
        case PixelFormat32bppARGB:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_32bppARGB);
        case PixelFormat32bppPARGB:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_32bppPARGB);
        case PixelFormat48bppRGB:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_48bppRGB);
        case PixelFormat64bppARGB:
            convert_rgb_to_rgb(getpixel_64bppPARGB, setpixel_64bppARGB);
        default:
            break;
        }
        break;
    default:
        break;
    }

#undef convert_indexed_to_rgb
#undef convert_indexed_to_indexed
#undef convert_rgb_to_rgb
#undef convert_rgb_to_indexed

    return NotImplemented;
}

/* This function returns a pointer to an array of pixels that represents the
 * bitmap. The *entire* bitmap is locked according to the lock mode specified by
 * flags.  It is correct behavior that a user who calls this function with write
 * privileges can write to the whole bitmap (not just the area in rect).
 *
 * FIXME: only used portion of format is bits per pixel. */
GpStatus WINGDIPAPI GdipBitmapLockBits(GpBitmap* bitmap, GDIPCONST GpRect* rect,
    UINT flags, PixelFormat format, BitmapData* lockeddata)
{
    INT bitspp = PIXELFORMATBPP(format);
    GpRect act_rect; /* actual rect to be used */
    GpStatus stat;

    TRACE("%p %p %d 0x%x %p\n", bitmap, rect, flags, format, lockeddata);

    if(!lockeddata || !bitmap)
        return InvalidParameter;
    if(!image_lock(&bitmap->image))
        return ObjectBusy;

    if(rect){
        if(rect->X < 0 || rect->Y < 0 || (rect->X + rect->Width > bitmap->width) ||
          (rect->Y + rect->Height > bitmap->height) || !flags)
        {
            image_unlock(&bitmap->image);
            return InvalidParameter;
        }

        act_rect = *rect;
    }
    else{
        act_rect.X = act_rect.Y = 0;
        act_rect.Width  = bitmap->width;
        act_rect.Height = bitmap->height;
    }

    if(bitmap->lockmode)
    {
        WARN("bitmap is already locked and cannot be locked again\n");
        image_unlock(&bitmap->image);
        return WrongState;
    }

    if (bitmap->bits && bitmap->format == format && !(flags & ImageLockModeUserInputBuf))
    {
        /* no conversion is necessary; just use the bits directly */
        lockeddata->Width = act_rect.Width;
        lockeddata->Height = act_rect.Height;
        lockeddata->PixelFormat = format;
        lockeddata->Reserved = flags;
        lockeddata->Stride = bitmap->stride;
        lockeddata->Scan0 = bitmap->bits + (bitspp / 8) * act_rect.X +
                            bitmap->stride * act_rect.Y;

        bitmap->lockmode = flags | ImageLockModeRead;

        image_unlock(&bitmap->image);
        return Ok;
    }

    /* Make sure we can convert to the requested format. */
    if (flags & ImageLockModeRead)
    {
        stat = convert_pixels(0, 0, 0, NULL, format, NULL, 0, NULL, bitmap->format, NULL);
        if (stat == NotImplemented)
        {
            FIXME("cannot read bitmap from %x to %x\n", bitmap->format, format);
            image_unlock(&bitmap->image);
            return NotImplemented;
        }
    }

    /* If we're opening for writing, make sure we'll be able to write back in
     * the original format. */
    if (flags & ImageLockModeWrite)
    {
        stat = convert_pixels(0, 0, 0, NULL, bitmap->format, NULL, 0, NULL, format, NULL);
        if (stat == NotImplemented)
        {
            FIXME("cannot write bitmap from %x to %x\n", format, bitmap->format);
            image_unlock(&bitmap->image);
            return NotImplemented;
        }
    }

    lockeddata->Width  = act_rect.Width;
    lockeddata->Height = act_rect.Height;
    lockeddata->PixelFormat = format;
    lockeddata->Reserved = flags;

    if(!(flags & ImageLockModeUserInputBuf))
    {
        lockeddata->Stride = (((act_rect.Width * bitspp + 7) / 8) + 3) & ~3;

        bitmap->bitmapbits = calloc(lockeddata->Stride, act_rect.Height);

        if (!bitmap->bitmapbits)
        {
            image_unlock(&bitmap->image);
            return OutOfMemory;
        }

        lockeddata->Scan0  = bitmap->bitmapbits;
    }

    if (flags & ImageLockModeRead)
    {
        static BOOL fixme = FALSE;

        if (!fixme && (PIXELFORMATBPP(bitmap->format) * act_rect.X) % 8 != 0)
        {
            FIXME("Cannot copy rows that don't start at a whole byte.\n");
            fixme = TRUE;
        }

        stat = convert_pixels(act_rect.Width, act_rect.Height,
            lockeddata->Stride, lockeddata->Scan0, format, bitmap->image.palette,
            bitmap->stride,
            bitmap->bits + bitmap->stride * act_rect.Y + PIXELFORMATBPP(bitmap->format) * act_rect.X / 8,
            bitmap->format, bitmap->image.palette);

        if (stat != Ok)
        {
            free(bitmap->bitmapbits);
            bitmap->bitmapbits = NULL;
            image_unlock(&bitmap->image);
            return stat;
        }
    }

    bitmap->lockmode = flags | ImageLockModeRead;
    bitmap->lockx = act_rect.X;
    bitmap->locky = act_rect.Y;

    image_unlock(&bitmap->image);
    return Ok;
}

GpStatus WINGDIPAPI GdipBitmapSetResolution(GpBitmap* bitmap, REAL xdpi, REAL ydpi)
{
    TRACE("(%p, %.2f, %.2f)\n", bitmap, xdpi, ydpi);

    if (!bitmap || xdpi == 0.0 || ydpi == 0.0)
        return InvalidParameter;

    bitmap->image.xres = xdpi;
    bitmap->image.yres = ydpi;

    return Ok;
}

GpStatus WINGDIPAPI GdipBitmapUnlockBits(GpBitmap* bitmap,
    BitmapData* lockeddata)
{
    GpStatus stat;
    static BOOL fixme = FALSE;

    TRACE("(%p,%p)\n", bitmap, lockeddata);

    if(!bitmap || !lockeddata)
        return InvalidParameter;
    if(!image_lock(&bitmap->image))
        return ObjectBusy;

    if(!bitmap->lockmode)
    {
        image_unlock(&bitmap->image);
        return WrongState;
    }

    if(!(lockeddata->Reserved & ImageLockModeWrite)){
        bitmap->lockmode = 0;
        free(bitmap->bitmapbits);
        bitmap->bitmapbits = NULL;
        image_unlock(&bitmap->image);
        return Ok;
    }

    if (!bitmap->bitmapbits && !(lockeddata->Reserved & ImageLockModeUserInputBuf))
    {
        /* we passed a direct reference; no need to do anything */
        bitmap->lockmode = 0;
        image_unlock(&bitmap->image);
        return Ok;
    }

    if (!fixme && (PIXELFORMATBPP(bitmap->format) * bitmap->lockx) % 8 != 0)
    {
        FIXME("Cannot copy rows that don't start at a whole byte.\n");
        fixme = TRUE;
    }

    /* FIXME: Pass src_palette generated from lockeddata->PixelFormat. */
    stat = convert_pixels(lockeddata->Width, lockeddata->Height,
        bitmap->stride,
        bitmap->bits + bitmap->stride * bitmap->locky + PIXELFORMATBPP(bitmap->format) * bitmap->lockx / 8,
        bitmap->format, bitmap->image.palette,
        lockeddata->Stride, lockeddata->Scan0, lockeddata->PixelFormat, NULL);

    if (stat != Ok)
    {
        ERR("failed to convert pixels; this should never happen\n");
    }

    free(bitmap->bitmapbits);
    bitmap->bitmapbits = NULL;
    bitmap->lockmode = 0;

    image_unlock(&bitmap->image);
    return stat;
}

GpStatus WINGDIPAPI GdipCloneBitmapArea(REAL x, REAL y, REAL width, REAL height,
    PixelFormat format, GpBitmap* srcBitmap, GpBitmap** dstBitmap)
{
    Rect area;
    GpStatus stat;

    TRACE("(%f,%f,%f,%f,0x%x,%p,%p)\n", x, y, width, height, format, srcBitmap, dstBitmap);

    if (!srcBitmap || !dstBitmap || srcBitmap->image.type != ImageTypeBitmap ||
        x < 0 || y < 0 ||
        x + width > srcBitmap->width || y + height > srcBitmap->height)
    {
        TRACE("<-- InvalidParameter\n");
        return InvalidParameter;
    }

    if (format == PixelFormatDontCare)
        format = srcBitmap->format;

    area.X = gdip_round(x);
    area.Y = gdip_round(y);
    area.Width = gdip_round(width);
    area.Height = gdip_round(height);

    stat = GdipCreateBitmapFromScan0(area.Width, area.Height, 0, format, NULL, dstBitmap);
    if (stat == Ok)
    {
        memcpy(&(*dstBitmap)->image.format, &srcBitmap->image.format, sizeof(GUID));
        stat = convert_pixels(area.Width, area.Height, (*dstBitmap)->stride, (*dstBitmap)->bits, (*dstBitmap)->format,
                              (*dstBitmap)->image.palette, srcBitmap->stride,
                              srcBitmap->bits + srcBitmap->stride * area.Y + PIXELFORMATBPP(srcBitmap->format) * area.X / 8,
                              srcBitmap->format, srcBitmap->image.palette);

        if (stat == Ok && srcBitmap->image.palette)
        {
            ColorPalette *src_palette, *dst_palette;

            src_palette = srcBitmap->image.palette;

            dst_palette = calloc(1, sizeof(UINT) * 2 + sizeof(ARGB) * src_palette->Count);

            if (dst_palette)
            {
                dst_palette->Flags = src_palette->Flags;
                dst_palette->Count = src_palette->Count;
                memcpy(dst_palette->Entries, src_palette->Entries, sizeof(ARGB) * src_palette->Count);

                free((*dstBitmap)->image.palette);
                (*dstBitmap)->image.palette = dst_palette;
            }
            else
                stat = OutOfMemory;
        }

        if (stat != Ok)
            GdipDisposeImage(&(*dstBitmap)->image);
    }

    if (stat != Ok)
        *dstBitmap = NULL;

    return stat;
}

GpStatus WINGDIPAPI GdipCloneBitmapAreaI(INT x, INT y, INT width, INT height,
    PixelFormat format, GpBitmap* srcBitmap, GpBitmap** dstBitmap)
{
    TRACE("(%i,%i,%i,%i,0x%x,%p,%p)\n", x, y, width, height, format, srcBitmap, dstBitmap);

    return GdipCloneBitmapArea(x, y, width, height, format, srcBitmap, dstBitmap);
}

GpStatus WINGDIPAPI GdipCloneImage(GpImage *image, GpImage **cloneImage)
{
    TRACE("%p, %p\n", image, cloneImage);

    if (!image || !cloneImage)
        return InvalidParameter;

    if (image->type == ImageTypeBitmap)
    {
        GpBitmap *bitmap = (GpBitmap *)image;

        return GdipCloneBitmapAreaI(0, 0, bitmap->width, bitmap->height,
                                    bitmap->format, bitmap, (GpBitmap **)cloneImage);
    }
    else if (image->type == ImageTypeMetafile && ((GpMetafile*)image)->hemf)
    {
        GpMetafile *result, *metafile;

        metafile = (GpMetafile*)image;

        result = calloc(1, sizeof(*result));
        if (!result)
            return OutOfMemory;

        result->image.type = ImageTypeMetafile;
        result->image.format = image->format;
        result->image.flags = image->flags;
        result->image.frame_count = 1;
        result->image.xres = image->xres;
        result->image.yres = image->yres;
        result->bounds = metafile->bounds;
        result->unit = metafile->unit;
        result->metafile_type = metafile->metafile_type;
        result->hemf = CopyEnhMetaFileW(metafile->hemf, NULL);
        list_init(&result->containers);

        if (!result->hemf)
        {
            free(result);
            return OutOfMemory;
        }

        *cloneImage = &result->image;
        return Ok;
    }
    else
    {
        WARN("GpImage with no image data (metafile in wrong state?)\n");
        return InvalidParameter;
    }
}

GpStatus WINGDIPAPI GdipCreateBitmapFromFile(GDIPCONST WCHAR* filename,
    GpBitmap **bitmap)
{
    GpStatus stat;
    IStream *stream;

    TRACE("(%s) %p\n", debugstr_w(filename), bitmap);

    if(!filename || !bitmap)
        return InvalidParameter;

    *bitmap = NULL;

    stat = GdipCreateStreamOnFile(filename, GENERIC_READ, &stream);

    if(stat != Ok)
        return stat;

    stat = GdipCreateBitmapFromStream(stream, bitmap);

    IStream_Release(stream);

    return stat;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromGdiDib(GDIPCONST BITMAPINFO* info,
                                               VOID *bits, GpBitmap **bitmap)
{
    DWORD height, stride;
    HBITMAP hbm;
    void *bmbits;
    GpStatus status;

    TRACE("(%p, %p, %p)\n", info, bits, bitmap);

    if (!info || !bits || !bitmap)
        return InvalidParameter;

    hbm = CreateDIBSection(0, info, DIB_RGB_COLORS, &bmbits, NULL, 0);
    if (!hbm)
        return InvalidParameter;

    height = abs(info->bmiHeader.biHeight);
    stride = ((info->bmiHeader.biWidth * info->bmiHeader.biBitCount + 31) >> 3) & ~3;
    TRACE("height %lu, stride %lu, image size %lu\n", height, stride, height * stride);

    memcpy(bmbits, bits, height * stride);

    status = GdipCreateBitmapFromHBITMAP(hbm, NULL, bitmap);
    DeleteObject(hbm);

    return status;
}

/* FIXME: no icm */
GpStatus WINGDIPAPI GdipCreateBitmapFromFileICM(GDIPCONST WCHAR* filename,
    GpBitmap **bitmap)
{
    TRACE("(%s) %p\n", debugstr_w(filename), bitmap);

    return GdipCreateBitmapFromFile(filename, bitmap);
}

GpStatus WINGDIPAPI GdipCreateBitmapFromResource(HINSTANCE hInstance,
    GDIPCONST WCHAR* lpBitmapName, GpBitmap** bitmap)
{
    HBITMAP hbm;
    GpStatus stat = InvalidParameter;

    TRACE("%p (%s) %p\n", hInstance, debugstr_w(lpBitmapName), bitmap);

    if(!lpBitmapName || !bitmap)
        return InvalidParameter;

    /* load DIB */
    hbm = LoadImageW(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0,
                     LR_CREATEDIBSECTION);

    if(hbm){
        stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, bitmap);
        DeleteObject(hbm);
    }

    return stat;
}

static inline DWORD blend_argb_no_bkgnd_alpha(DWORD src, DWORD bkgnd)
{
    BYTE b = (BYTE)src;
    BYTE g = (BYTE)(src >> 8);
    BYTE r = (BYTE)(src >> 16);
    DWORD alpha  = (BYTE)(src >> 24);
    return ((b     + ((BYTE)bkgnd         * (255 - alpha) + 127) / 255) |
            (g     + ((BYTE)(bkgnd >> 8)  * (255 - alpha) + 127) / 255) << 8 |
            (r     + ((BYTE)(bkgnd >> 16) * (255 - alpha) + 127) / 255) << 16 |
            (alpha << 24));
}

GpStatus WINGDIPAPI GdipCreateHBITMAPFromBitmap(GpBitmap* bitmap,
    HBITMAP* hbmReturn, ARGB background)
{
    GpStatus stat;
    HBITMAP result;
    UINT width, height;
    BITMAPINFOHEADER bih;
    LPBYTE bits;

    TRACE("(%p,%p,%lx)\n", bitmap, hbmReturn, background);

    if (!bitmap || !hbmReturn) return InvalidParameter;
    if (!image_lock(&bitmap->image)) return ObjectBusy;

    GdipGetImageWidth(&bitmap->image, &width);
    GdipGetImageHeight(&bitmap->image, &height);

    bih.biSize = sizeof(bih);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    result = CreateDIBSection(0, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    if (!result)
    {
        image_unlock(&bitmap->image);
        return GenericError;
    }

    stat = convert_pixels(width, height, -width*4,
            bits + (width * 4 * (height - 1)), PixelFormat32bppPARGB, bitmap->image.palette,
            bitmap->stride, bitmap->bits, bitmap->format, bitmap->image.palette);
    if (stat != Ok)
    {
        DeleteObject(result);
        image_unlock(&bitmap->image);
        return stat;
    }

    if (background & 0xffffff)
    {
        DWORD *ptr;
        UINT i;
        for (ptr = (DWORD*)bits, i = 0; i < width * height; ptr++, i++)
        {
            if ((*ptr & 0xff000000) == 0xff000000) continue;
            *ptr = blend_argb_no_bkgnd_alpha(*ptr, background);
        }
    }

    *hbmReturn = result;
    image_unlock(&bitmap->image);
    return Ok;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromGraphics(INT width, INT height,
    GpGraphics* target, GpBitmap** bitmap)
{
    GpStatus ret;

    TRACE("(%d, %d, %p, %p)\n", width, height, target, bitmap);

    if(!target || !bitmap)
        return InvalidParameter;

    ret = GdipCreateBitmapFromScan0(width, height, 0, PixelFormat32bppPARGB,
                                    NULL, bitmap);

    if (ret == Ok)
    {
        GdipGetDpiX(target, &(*bitmap)->image.xres);
        GdipGetDpiY(target, &(*bitmap)->image.yres);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromHICON(HICON hicon, GpBitmap** bitmap)
{
    GpStatus stat;
    ICONINFO iinfo;
    BITMAP bm;
    int ret;
    UINT width, height, stride;
    GpRect rect;
    BitmapData lockeddata;
    HDC screendc;
    int x, y;
    BITMAPINFOHEADER bih;
    DWORD *src;
    BYTE *dst_row;
    DWORD *dst;
    BYTE *bits;
    int mask_scanlines = 0, color_scanlines = 0;

    TRACE("%p, %p\n", hicon, bitmap);

    if(!bitmap || !GetIconInfo(hicon, &iinfo) || !iinfo.hbmColor || !iinfo.fIcon)
        return InvalidParameter;

    /* get the size of the icon */
    ret = GetObjectA(iinfo.hbmColor, sizeof(bm), &bm);
    if (ret == 0) {
        DeleteObject(iinfo.hbmColor);
        DeleteObject(iinfo.hbmMask);
        return GenericError;
    }

    width = bm.bmWidth;
    height = abs(bm.bmHeight);
    stride = width * 4;

    stat = GdipCreateBitmapFromScan0(width, height, stride, PixelFormat32bppARGB, NULL, bitmap);
    if (stat != Ok) {
        DeleteObject(iinfo.hbmColor);
        DeleteObject(iinfo.hbmMask);
        return stat;
    }

    rect.X = 0;
    rect.Y = 0;
    rect.Width = width;
    rect.Height = height;

    stat = GdipBitmapLockBits(*bitmap, &rect, ImageLockModeWrite, PixelFormat32bppARGB, &lockeddata);
    if (stat != Ok) {
        DeleteObject(iinfo.hbmColor);
        DeleteObject(iinfo.hbmMask);
        GdipDisposeImage(&(*bitmap)->image);
        return stat;
    }

    bih.biSize = sizeof(bih);
    bih.biWidth = width;
    bih.biHeight = -height;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    bits = malloc(height * stride);
    if (!bits)
    {
        DeleteObject(iinfo.hbmColor);
        DeleteObject(iinfo.hbmMask);
        GdipBitmapUnlockBits(*bitmap, &lockeddata);
        GdipDisposeImage(&(*bitmap)->image);
        return OutOfMemory;
    }

    screendc = CreateCompatibleDC(0);
    if (screendc)
    {
        color_scanlines = GetDIBits(screendc, iinfo.hbmColor, 0, height, lockeddata.Scan0,
            (BITMAPINFO*)&bih, DIB_RGB_COLORS);
        mask_scanlines = GetDIBits(screendc, iinfo.hbmMask, 0, height, bits,
            (BITMAPINFO*)&bih, DIB_RGB_COLORS);
        DeleteDC(screendc);
    }

    DeleteObject(iinfo.hbmColor);
    DeleteObject(iinfo.hbmMask);

    if (!screendc || ((color_scanlines == 0 || mask_scanlines == 0) &&
                      GetLastError() == ERROR_INVALID_PARAMETER))
    {
        free(bits);
        GdipBitmapUnlockBits(*bitmap, &lockeddata);
        GdipDisposeImage(&(*bitmap)->image);
        return GenericError;
    }

    src = (DWORD*)bits;
    dst_row = lockeddata.Scan0;
    for (y=0; y<height; y++)
    {
        dst = (DWORD*)dst_row;
        for (x=0; x<width; x++)
        {
            DWORD src_value = *src++;
            if (src_value)
                *dst++ = 0;
            else
                *dst++ |= 0xff000000;
        }
        dst_row += lockeddata.Stride;
    }

    free(bits);

    GdipBitmapUnlockBits(*bitmap, &lockeddata);

    return Ok;
}

static void generate_halftone_palette(ARGB *entries, UINT count)
{
    static const BYTE halftone_values[6]={0x00,0x33,0x66,0x99,0xcc,0xff};
    UINT i;

    for (i=0; i<8 && i<count; i++)
    {
        entries[i] = 0xff000000;
        if (i&1) entries[i] |= 0x800000;
        if (i&2) entries[i] |= 0x8000;
        if (i&4) entries[i] |= 0x80;
    }

    if (8 < count)
        entries[i] = 0xffc0c0c0;

    for (i=9; i<16 && i<count; i++)
    {
        entries[i] = 0xff000000;
        if (i&1) entries[i] |= 0xff0000;
        if (i&2) entries[i] |= 0xff00;
        if (i&4) entries[i] |= 0xff;
    }

    for (i=16; i<40 && i<count; i++)
    {
        entries[i] = 0;
    }

    for (i=40; i<256 && i<count; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[(i-40)%6];
        entries[i] |= halftone_values[((i-40)/6)%6] << 8;
        entries[i] |= halftone_values[((i-40)/36)%6] << 16;
    }
}

static GpStatus get_screen_resolution(REAL *xres, REAL *yres)
{
    HDC screendc = CreateCompatibleDC(0);

    if (!screendc) return GenericError;

    *xres = (REAL)GetDeviceCaps(screendc, LOGPIXELSX);
    *yres = (REAL)GetDeviceCaps(screendc, LOGPIXELSY);

    DeleteDC(screendc);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromScan0(INT width, INT height, INT stride,
    PixelFormat format, BYTE* scan0, GpBitmap** bitmap)
{
    INT row_size;
    BYTE *bits=NULL, *own_bits=NULL;
    REAL xres, yres;
    GpStatus stat;

    TRACE("%d %d %d 0x%x %p %p\n", width, height, stride, format, scan0, bitmap);

    if (!bitmap) return InvalidParameter;

    if(width <= 0 || height <= 0 || (scan0 && (stride % 4))){
        *bitmap = NULL;
        return InvalidParameter;
    }

    if(scan0 && !stride)
        return InvalidParameter;

    stat = get_screen_resolution(&xres, &yres);
    if (stat != Ok) return stat;

    row_size = (width * PIXELFORMATBPP(format)+7) / 8;

    if (scan0)
        bits = scan0;
    else
    {
        stride = (row_size + 3) & ~3;

        own_bits = bits = calloc(1, stride * height);
        if (!own_bits) return OutOfMemory;
    }

    *bitmap = calloc(1, sizeof(GpBitmap));
    if(!*bitmap)
    {
        free(own_bits);
        return OutOfMemory;
    }

    (*bitmap)->image.type = ImageTypeBitmap;
    memcpy(&(*bitmap)->image.format, &ImageFormatMemoryBMP, sizeof(GUID));
    (*bitmap)->image.flags = ImageFlagsNone;
    (*bitmap)->image.frame_count = 1;
    (*bitmap)->image.current_frame = 0;
    (*bitmap)->image.palette = NULL;
    (*bitmap)->image.xres = xres;
    (*bitmap)->image.yres = yres;
    (*bitmap)->width = width;
    (*bitmap)->height = height;
    (*bitmap)->format = format;
    (*bitmap)->image.decoder = NULL;
    (*bitmap)->image.encoder = NULL;
    (*bitmap)->bits = bits;
    (*bitmap)->stride = stride;
    (*bitmap)->own_bits = own_bits;
    (*bitmap)->metadata_reader = NULL;
    (*bitmap)->prop_count = 0;
    (*bitmap)->prop_item = NULL;

    /* set format-related flags */
    if (format & (PixelFormatAlpha|PixelFormatPAlpha|PixelFormatIndexed))
        (*bitmap)->image.flags |= ImageFlagsHasAlpha;

    if (format == PixelFormat1bppIndexed ||
        format == PixelFormat4bppIndexed ||
        format == PixelFormat8bppIndexed)
    {
        (*bitmap)->image.palette = calloc(1, sizeof(UINT) * 2 + sizeof(ARGB) * (1 << PIXELFORMATBPP(format)));

        if (!(*bitmap)->image.palette)
        {
            GdipDisposeImage(&(*bitmap)->image);
            *bitmap = NULL;
            return OutOfMemory;
        }

        (*bitmap)->image.palette->Count = 1 << PIXELFORMATBPP(format);

        if (format == PixelFormat1bppIndexed)
        {
            (*bitmap)->image.palette->Flags = PaletteFlagsGrayScale;
            (*bitmap)->image.palette->Entries[0] = 0xff000000;
            (*bitmap)->image.palette->Entries[1] = 0xffffffff;
        }
        else
        {
            if (format == PixelFormat8bppIndexed)
                (*bitmap)->image.palette->Flags = PaletteFlagsHalftone;

            generate_halftone_palette((*bitmap)->image.palette->Entries,
                (*bitmap)->image.palette->Count);
        }
    }

    TRACE("<-- %p\n", *bitmap);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromStream(IStream* stream,
    GpBitmap **bitmap)
{
    GpStatus stat;

    TRACE("%p %p\n", stream, bitmap);

    stat = GdipLoadImageFromStream(stream, (GpImage**) bitmap);

    if(stat != Ok)
        return stat;

    if((*bitmap)->image.type != ImageTypeBitmap){
        GdipDisposeImage(&(*bitmap)->image);
        *bitmap = NULL;
        return GenericError; /* FIXME: what error to return? */
    }

    return Ok;
}

/* FIXME: no icm */
GpStatus WINGDIPAPI GdipCreateBitmapFromStreamICM(IStream* stream,
    GpBitmap **bitmap)
{
    TRACE("%p %p\n", stream, bitmap);

    return GdipCreateBitmapFromStream(stream, bitmap);
}

GpStatus WINGDIPAPI GdipCreateCachedBitmap(GpBitmap *bitmap, GpGraphics *graphics,
    GpCachedBitmap **cachedbmp)
{
    GpStatus stat;

    TRACE("%p %p %p\n", bitmap, graphics, cachedbmp);

    if(!bitmap || !graphics || !cachedbmp)
        return InvalidParameter;

    *cachedbmp = calloc(1, sizeof(GpCachedBitmap));
    if(!*cachedbmp)
        return OutOfMemory;

    stat = GdipCloneImage(&(bitmap->image), &(*cachedbmp)->image);
    if(stat != Ok){
        free(*cachedbmp);
        return stat;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateHICONFromBitmap(GpBitmap *bitmap, HICON *hicon)
{
    GpStatus stat;
    BitmapData lockeddata;
    ULONG andstride, xorstride, bitssize;
    LPBYTE andbits, xorbits, androw, xorrow, srcrow;
    UINT x, y;

    TRACE("(%p, %p)\n", bitmap, hicon);

    if (!bitmap || !hicon)
        return InvalidParameter;

    stat = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead,
        PixelFormat32bppPARGB, &lockeddata);
    if (stat == Ok)
    {
        andstride = ((lockeddata.Width+31)/32)*4;
        xorstride = lockeddata.Width*4;
        bitssize = (andstride + xorstride) * lockeddata.Height;

        andbits = calloc(1, bitssize);

        if (andbits)
        {
            xorbits = andbits + andstride * lockeddata.Height;

            for (y=0; y<lockeddata.Height; y++)
            {
                srcrow = ((LPBYTE)lockeddata.Scan0) + lockeddata.Stride * y;

                androw = andbits + andstride * y;
                for (x=0; x<lockeddata.Width; x++)
                    if (srcrow[3+4*x] >= 128)
                        androw[x/8] |= 1 << (7-x%8);

                xorrow = xorbits + xorstride * y;
                memcpy(xorrow, srcrow, xorstride);
            }

            *hicon = CreateIcon(NULL, lockeddata.Width, lockeddata.Height, 1, 32,
                andbits, xorbits);

            free(andbits);
        }
        else
            stat = OutOfMemory;

        GdipBitmapUnlockBits(bitmap, &lockeddata);
    }

    return stat;
}

GpStatus WINGDIPAPI GdipDeleteCachedBitmap(GpCachedBitmap *cachedbmp)
{
    TRACE("%p\n", cachedbmp);

    if(!cachedbmp)
        return InvalidParameter;

    GdipDisposeImage(cachedbmp->image);
    free(cachedbmp);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawCachedBitmap(GpGraphics *graphics,
    GpCachedBitmap *cachedbmp, INT x, INT y)
{
    TRACE("%p %p %d %d\n", graphics, cachedbmp, x, y);

    if(!graphics || !cachedbmp)
        return InvalidParameter;

    return GdipDrawImage(graphics, cachedbmp->image, (REAL)x, (REAL)y);
}

/* Internal utility function: Replace the image data of dst with that of src,
 * and free src. */
static void move_bitmap(GpBitmap *dst, GpBitmap *src, BOOL clobber_palette)
{
    assert(src->image.type == ImageTypeBitmap);
    assert(dst->image.type == ImageTypeBitmap);

    free(dst->bitmapbits);
    free(dst->own_bits);

    if (clobber_palette)
    {
        free(dst->image.palette);
        dst->image.palette = src->image.palette;
    }
    else
        free(src->image.palette);

    dst->image.xres = src->image.xres;
    dst->image.yres = src->image.yres;
    dst->width = src->width;
    dst->height = src->height;
    dst->format = src->format;
    dst->bits = src->bits;
    dst->stride = src->stride;
    dst->own_bits = src->own_bits;
    if (dst->metadata_reader)
        IWICMetadataReader_Release(dst->metadata_reader);
    dst->metadata_reader = src->metadata_reader;
    free(dst->prop_item);
    dst->prop_item = src->prop_item;
    dst->prop_count = src->prop_count;
    if (dst->image.decoder)
        IWICBitmapDecoder_Release(dst->image.decoder);
    dst->image.decoder = src->image.decoder;
    terminate_encoder_wic(&dst->image); /* terminate active encoder before overwriting with src */
    dst->image.encoder = src->image.encoder;
    dst->image.frame_count = src->image.frame_count;
    dst->image.current_frame = src->image.current_frame;
    dst->image.format = src->image.format;

    src->image.type = ~0;
    free(src);
}

static GpStatus free_image_data(GpImage *image)
{
    if(!image)
        return InvalidParameter;

    if (image->type == ImageTypeBitmap)
    {
        free(((GpBitmap*)image)->bitmapbits);
        free(((GpBitmap*)image)->own_bits);
        if (((GpBitmap*)image)->metadata_reader)
            IWICMetadataReader_Release(((GpBitmap*)image)->metadata_reader);
        free(((GpBitmap*)image)->prop_item);
    }
    else if (image->type == ImageTypeMetafile)
        METAFILE_Free((GpMetafile *)image);
    else
    {
        WARN("invalid image: %p\n", image);
        return ObjectBusy;
    }
    if (image->decoder)
        IWICBitmapDecoder_Release(image->decoder);
    terminate_encoder_wic(image);
    free(image->palette);

    return Ok;
}

GpStatus WINGDIPAPI GdipDisposeImage(GpImage *image)
{
    GpStatus status;

    TRACE("%p\n", image);

    status = free_image_data(image);
    if (status != Ok) return status;
    image->type = ~0;
    free(image);

    return Ok;
}

GpStatus WINGDIPAPI GdipFindFirstImageItem(GpImage *image, ImageItemData* item)
{
    static int calls;

    TRACE("(%p,%p)\n", image, item);

    if(!image || !item)
        return InvalidParameter;

    if (!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageItemData(GpImage *image, ImageItemData *item)
{
    static int calls;

    TRACE("(%p,%p)\n", image, item);

    if (!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageBounds(GpImage *image, GpRectF *srcRect,
    GpUnit *srcUnit)
{
    TRACE("%p %p %p\n", image, srcRect, srcUnit);

    if(!image || !srcRect || !srcUnit)
        return InvalidParameter;
    if(image->type == ImageTypeMetafile){
        *srcRect = ((GpMetafile*)image)->bounds;
        *srcUnit = ((GpMetafile*)image)->unit;
    }
    else if(image->type == ImageTypeBitmap){
        srcRect->X = srcRect->Y = 0.0;
        srcRect->Width = (REAL) ((GpBitmap*)image)->width;
        srcRect->Height = (REAL) ((GpBitmap*)image)->height;
        *srcUnit = UnitPixel;
    }
    else{
        WARN("GpImage with no image data\n");
        return InvalidParameter;
    }

    TRACE("returning (%f, %f) (%f, %f) unit type %d\n", srcRect->X, srcRect->Y,
          srcRect->Width, srcRect->Height, *srcUnit);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageDimension(GpImage *image, REAL *width,
    REAL *height)
{
    TRACE("%p %p %p\n", image, width, height);

    if(!image || !height || !width)
        return InvalidParameter;

    if(image->type == ImageTypeMetafile){
        *height = units_to_pixels(((GpMetafile*)image)->bounds.Height, ((GpMetafile*)image)->unit,
                                  image->yres, ((GpMetafile*)image)->printer_display);
        *width = units_to_pixels(((GpMetafile*)image)->bounds.Width, ((GpMetafile*)image)->unit,
                                 image->xres, ((GpMetafile*)image)->printer_display);
    }
    else if(image->type == ImageTypeBitmap){
        *height = ((GpBitmap*)image)->height;
        *width = ((GpBitmap*)image)->width;
    }
    else{
        WARN("GpImage with no image data\n");
        return InvalidParameter;
    }

    TRACE("returning (%f, %f)\n", *height, *width);
    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageGraphicsContext(GpImage *image,
    GpGraphics **graphics)
{
    GpStatus stat;

    TRACE("%p %p\n", image, graphics);

    if(!image || !graphics)
        return InvalidParameter;

    if (image->type == ImageTypeMetafile)
        stat = METAFILE_GetGraphicsContext((GpMetafile*)image, graphics);
    else
        stat = graphics_from_image(image, graphics);

    return stat;
}

GpStatus WINGDIPAPI GdipGetImageHeight(GpImage *image, UINT *height)
{
    TRACE("%p %p\n", image, height);

    if(!image || !height)
        return InvalidParameter;

    if(image->type == ImageTypeMetafile)
        *height = units_to_pixels(((GpMetafile*)image)->bounds.Height, ((GpMetafile*)image)->unit,
                                  image->yres, ((GpMetafile*)image)->printer_display);
    else if(image->type == ImageTypeBitmap)
        *height = ((GpBitmap*)image)->height;
    else
    {
        WARN("GpImage with no image data\n");
        return InvalidParameter;
    }

    TRACE("returning %d\n", *height);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageHorizontalResolution(GpImage *image, REAL *res)
{
    if(!image || !res)
        return InvalidParameter;

    *res = image->xres;

    TRACE("(%p) <-- %0.2f\n", image, *res);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImagePaletteSize(GpImage *image, INT *size)
{
    TRACE("%p %p\n", image, size);

    if(!image || !size)
        return InvalidParameter;

    if (image->type == ImageTypeMetafile)
    {
        *size = 0;
        return GenericError;
    }

    if (!image->palette || image->palette->Count == 0)
        *size = sizeof(ColorPalette);
    else
        *size = sizeof(UINT)*2 + sizeof(ARGB)*image->palette->Count;

    TRACE("<-- %u\n", *size);

    return Ok;
}

/* FIXME: test this function for non-bitmap types */
GpStatus WINGDIPAPI GdipGetImagePixelFormat(GpImage *image, PixelFormat *format)
{
    TRACE("%p %p\n", image, format);

    if(!image || !format)
        return InvalidParameter;

    if(image->type != ImageTypeBitmap)
        *format = PixelFormat24bppRGB;
    else
        *format = ((GpBitmap*) image)->format;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageRawFormat(GpImage *image, GUID *format)
{
    TRACE("(%p, %p)\n", image, format);

    if(!image || !format)
        return InvalidParameter;

    memcpy(format, &image->format, sizeof(GUID));

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageType(GpImage *image, ImageType *type)
{
    TRACE("%p %p\n", image, type);

    if(!image || !type)
        return InvalidParameter;

    *type = image->type;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageVerticalResolution(GpImage *image, REAL *res)
{
    if(!image || !res)
        return InvalidParameter;

    *res = image->yres;

    TRACE("(%p) <-- %0.2f\n", image, *res);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageWidth(GpImage *image, UINT *width)
{
    TRACE("%p %p\n", image, width);

    if(!image || !width)
        return InvalidParameter;

    if(image->type == ImageTypeMetafile)
        *width = units_to_pixels(((GpMetafile*)image)->bounds.Width, ((GpMetafile*)image)->unit,
                                 image->xres, ((GpMetafile*)image)->printer_display);
    else if(image->type == ImageTypeBitmap)
        *width = ((GpBitmap*)image)->width;
    else
    {
        WARN("GpImage with no image data\n");
        return InvalidParameter;
    }

    TRACE("returning %d\n", *width);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPropertyCount(GpImage *image, UINT *num)
{
    TRACE("(%p, %p)\n", image, num);

    if (!image || !num) return InvalidParameter;

    *num = 0;

    if (image->type == ImageTypeBitmap)
    {
        if (((GpBitmap *)image)->prop_item)
        {
            *num = ((GpBitmap *)image)->prop_count;
            return Ok;
        }

        if (((GpBitmap *)image)->metadata_reader)
            IWICMetadataReader_GetCount(((GpBitmap *)image)->metadata_reader, num);
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPropertyIdList(GpImage *image, UINT num, PROPID *list)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    UINT prop_count, i;
    ULONG items_returned;

    TRACE("(%p, %u, %p)\n", image, num, list);

    if (!image || !list) return InvalidParameter;

    if (image->type != ImageTypeBitmap)
    {
        FIXME("Not implemented for type %d\n", image->type);
        return NotImplemented;
    }

    if (((GpBitmap *)image)->prop_item)
    {
        if (num != ((GpBitmap *)image)->prop_count) return InvalidParameter;

        for (i = 0; i < num; i++)
        {
            list[i] = ((GpBitmap *)image)->prop_item[i].id;
        }

        return Ok;
    }

    reader = ((GpBitmap *)image)->metadata_reader;
    if (!reader)
    {
        if (num != 0) return InvalidParameter;
        return Ok;
    }

    hr = IWICMetadataReader_GetCount(reader, &prop_count);
    if (FAILED(hr)) return hresult_to_status(hr);

    if (num != prop_count) return InvalidParameter;

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    if (FAILED(hr)) return hresult_to_status(hr);

    IWICEnumMetadataItem_Reset(enumerator);

    for (i = 0; i < num; i++)
    {
        PROPVARIANT id;

        hr = IWICEnumMetadataItem_Next(enumerator, 1, NULL, &id, NULL, &items_returned);
        if (hr != S_OK) break;

        if (id.vt != VT_UI2)
        {
            FIXME("not supported propvariant type for id: %u\n", id.vt);
            list[i] = 0;
            continue;
        }
        list[i] = id.uiVal;
    }

    IWICEnumMetadataItem_Release(enumerator);

    return hr == S_OK ? Ok : hresult_to_status(hr);
}

static UINT propvariant_size(PROPVARIANT *value)
{
    switch (value->vt & ~VT_VECTOR)
    {
    case VT_EMPTY:
        return 0;
    case VT_I1:
    case VT_UI1:
        if (!(value->vt & VT_VECTOR)) return 1;
        return value->caub.cElems;
    case VT_I2:
    case VT_UI2:
        if (!(value->vt & VT_VECTOR)) return 2;
        return value->caui.cElems * 2;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
        if (!(value->vt & VT_VECTOR)) return 4;
        return value->caul.cElems * 4;
    case VT_I8:
    case VT_UI8:
    case VT_R8:
        if (!(value->vt & VT_VECTOR)) return 8;
        return value->cauh.cElems * 8;
    case VT_LPSTR:
        return value->pszVal ? strlen(value->pszVal) + 1 : 0;
    case VT_BLOB:
        return value->blob.cbSize;
    default:
        FIXME("not supported variant type %d\n", value->vt);
        return 0;
    }
}

GpStatus WINGDIPAPI GdipGetPropertyItemSize(GpImage *image, PROPID propid, UINT *size)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT id, value;

    TRACE("(%p,%#lx,%p)\n", image, propid, size);

    if (!size || !image) return InvalidParameter;

    if (image->type != ImageTypeBitmap)
    {
        FIXME("Not implemented for type %d\n", image->type);
        return NotImplemented;
    }

    if (((GpBitmap *)image)->prop_item)
    {
        UINT i;

        for (i = 0; i < ((GpBitmap *)image)->prop_count; i++)
        {
            if (propid == ((GpBitmap *)image)->prop_item[i].id)
            {
                *size = sizeof(PropertyItem) + ((GpBitmap *)image)->prop_item[i].length;
                return Ok;
            }
        }

        return PropertyNotFound;
    }

    reader = ((GpBitmap *)image)->metadata_reader;
    if (!reader) return PropertyNotFound;

    id.vt = VT_UI2;
    id.uiVal = propid;
    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    if (FAILED(hr)) return PropertyNotFound;

    *size = propvariant_size(&value);
    if (*size) *size += sizeof(PropertyItem);
    PropVariantClear(&value);

    return Ok;
}

#ifndef PropertyTagTypeSByte
#define PropertyTagTypeSByte  6
#define PropertyTagTypeSShort 8
#define PropertyTagTypeFloat  11
#define PropertyTagTypeDouble 12
#endif

static UINT vt_to_itemtype(UINT vt)
{
    static const struct
    {
        UINT vt, type;
    } vt2type[] =
    {
        { VT_I1, PropertyTagTypeSByte },
        { VT_UI1, PropertyTagTypeByte },
        { VT_I2, PropertyTagTypeSShort },
        { VT_UI2, PropertyTagTypeShort },
        { VT_I4, PropertyTagTypeSLONG },
        { VT_UI4, PropertyTagTypeLong },
        { VT_I8, PropertyTagTypeSRational },
        { VT_UI8, PropertyTagTypeRational },
        { VT_R4, PropertyTagTypeFloat },
        { VT_R8, PropertyTagTypeDouble },
        { VT_LPSTR, PropertyTagTypeASCII },
        { VT_BLOB, PropertyTagTypeUndefined }
    };
    UINT i;
    for (i = 0; i < ARRAY_SIZE(vt2type); i++)
    {
        if (vt2type[i].vt == vt) return vt2type[i].type;
    }
    FIXME("not supported variant type %u\n", vt);
    return 0;
}

static GpStatus propvariant_to_item(PROPVARIANT *value, PropertyItem *item,
                                    UINT size, PROPID id)
{
    UINT item_size, item_type;

    item_size = propvariant_size(value);
    if (size != item_size + sizeof(PropertyItem)) return InvalidParameter;

    item_type = vt_to_itemtype(value->vt & ~VT_VECTOR);
    if (!item_type) return InvalidParameter;

    item->value = item + 1;

    switch (value->vt & ~VT_VECTOR)
    {
    case VT_I1:
    case VT_UI1:
        if (!(value->vt & VT_VECTOR))
            *(BYTE *)item->value = value->bVal;
        else
            memcpy(item->value, value->caub.pElems, item_size);
        break;
    case VT_I2:
    case VT_UI2:
        if (!(value->vt & VT_VECTOR))
            *(USHORT *)item->value = value->uiVal;
        else
            memcpy(item->value, value->caui.pElems, item_size);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
        if (!(value->vt & VT_VECTOR))
            *(ULONG *)item->value = value->ulVal;
        else
            memcpy(item->value, value->caul.pElems, item_size);
        break;
    case VT_I8:
    case VT_UI8:
    case VT_R8:
        if (!(value->vt & VT_VECTOR))
            *(ULONGLONG *)item->value = value->uhVal.QuadPart;
        else
            memcpy(item->value, value->cauh.pElems, item_size);
        break;
    case VT_LPSTR:
        memcpy(item->value, value->pszVal, item_size);
        break;
    case VT_BLOB:
        memcpy(item->value, value->blob.pBlobData, item_size);
        break;
    default:
        FIXME("not supported variant type %d\n", value->vt);
        return InvalidParameter;
    }

    item->length = item_size;
    item->type = item_type;
    item->id = id;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPropertyItem(GpImage *image, PROPID propid, UINT size,
                                        PropertyItem *buffer)
{
    GpStatus stat;
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT id, value;

    TRACE("(%p,%#lx,%u,%p)\n", image, propid, size, buffer);

    if (!image || !buffer) return InvalidParameter;

    if (image->type != ImageTypeBitmap)
    {
        FIXME("Not implemented for type %d\n", image->type);
        return NotImplemented;
    }

    if (((GpBitmap *)image)->prop_item)
    {
        UINT i;

        for (i = 0; i < ((GpBitmap *)image)->prop_count; i++)
        {
            if (propid == ((GpBitmap *)image)->prop_item[i].id)
            {
                if (size != sizeof(PropertyItem) + ((GpBitmap *)image)->prop_item[i].length)
                    return InvalidParameter;

                *buffer = ((GpBitmap *)image)->prop_item[i];
                buffer->value = buffer + 1;
                memcpy(buffer->value, ((GpBitmap *)image)->prop_item[i].value, buffer->length);
                return Ok;
            }
        }

        return PropertyNotFound;
    }

    reader = ((GpBitmap *)image)->metadata_reader;
    if (!reader) return PropertyNotFound;

    id.vt = VT_UI2;
    id.uiVal = propid;
    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    if (FAILED(hr)) return PropertyNotFound;

    stat = propvariant_to_item(&value, buffer, size, propid);
    PropVariantClear(&value);

    return stat;
}

GpStatus WINGDIPAPI GdipGetPropertySize(GpImage *image, UINT *size, UINT *count)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    UINT prop_count, prop_size, i;
    PROPVARIANT id, value;

    TRACE("(%p,%p,%p)\n", image, size, count);

    if (!image || !size || !count) return InvalidParameter;

    if (image->type != ImageTypeBitmap)
    {
        FIXME("Not implemented for type %d\n", image->type);
        return NotImplemented;
    }

    if (((GpBitmap *)image)->prop_item)
    {
        *count = ((GpBitmap *)image)->prop_count;
        *size = 0;

        for (i = 0; i < ((GpBitmap *)image)->prop_count; i++)
        {
            *size += sizeof(PropertyItem) + ((GpBitmap *)image)->prop_item[i].length;
        }

        return Ok;
    }

    reader = ((GpBitmap *)image)->metadata_reader;
    if (!reader)
    {
        *count = 0;
        *size = 0;
        return Ok;
    }

    hr = IWICMetadataReader_GetCount(reader, &prop_count);
    if (FAILED(hr)) return hresult_to_status(hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    if (FAILED(hr)) return hresult_to_status(hr);

    IWICEnumMetadataItem_Reset(enumerator);

    prop_size = 0;

    PropVariantInit(&id);
    PropVariantInit(&value);

    for (i = 0; i < prop_count; i++)
    {
        ULONG items_returned;
        UINT item_size;

        hr = IWICEnumMetadataItem_Next(enumerator, 1, NULL, &id, &value, &items_returned);
        if (hr != S_OK) break;

        item_size = propvariant_size(&value);
        if (item_size) prop_size += sizeof(PropertyItem) + item_size;

        PropVariantClear(&id);
        PropVariantClear(&value);
    }

    IWICEnumMetadataItem_Release(enumerator);

    if (hr != S_OK) return PropertyNotFound;

    *count = prop_count;
    *size = prop_size;
    return Ok;
}

GpStatus WINGDIPAPI GdipGetAllPropertyItems(GpImage *image, UINT size,
                                            UINT count, PropertyItem *buf)
{
    GpStatus status;
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    UINT prop_count, prop_size, i;
    PROPVARIANT id, value;
    char *item_value;

    TRACE("(%p,%u,%u,%p)\n", image, size, count, buf);

    if (!image || !buf) return InvalidParameter;

    if (image->type != ImageTypeBitmap)
    {
        FIXME("Not implemented for type %d\n", image->type);
        return NotImplemented;
    }

    status = GdipGetPropertySize(image, &prop_size, &prop_count);
    if (status != Ok) return status;

    if (prop_count != count || prop_size != size) return InvalidParameter;

    if (((GpBitmap *)image)->prop_item)
    {
        memcpy(buf, ((GpBitmap *)image)->prop_item, prop_size);

        item_value = (char *)(buf + prop_count);

        for (i = 0; i < prop_count; i++)
        {
            buf[i].value = item_value;
            item_value += buf[i].length;
        }

        return Ok;
    }

    reader = ((GpBitmap *)image)->metadata_reader;
    if (!reader) return GenericError;

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    if (FAILED(hr)) return hresult_to_status(hr);

    IWICEnumMetadataItem_Reset(enumerator);

    item_value = (char *)(buf + prop_count);

    PropVariantInit(&id);
    PropVariantInit(&value);

    for (i = 0; i < prop_count; i++)
    {
        PropertyItem *item;
        ULONG items_returned;
        UINT item_size;

        hr = IWICEnumMetadataItem_Next(enumerator, 1, NULL, &id, &value, &items_returned);
        if (hr != S_OK) break;

        if (id.vt != VT_UI2)
        {
            FIXME("not supported propvariant type for id: %u\n", id.vt);
            continue;
        }

        item_size = propvariant_size(&value);
        if (item_size)
        {
            item = malloc(item_size + sizeof(*item));

            propvariant_to_item(&value, item, item_size + sizeof(*item), id.uiVal);
            buf[i].id = item->id;
            buf[i].type = item->type;
            buf[i].length = item_size;
            buf[i].value = item_value;
            memcpy(item_value, item->value, item_size);
            item_value += item_size;

            free(item);
        }

        PropVariantClear(&id);
        PropVariantClear(&value);
    }

    IWICEnumMetadataItem_Release(enumerator);

    if (hr != S_OK) return PropertyNotFound;

    return Ok;
}

struct image_format_dimension
{
    const GUID *format;
    const GUID *dimension;
};

static const struct image_format_dimension image_format_dimensions[] =
{
    {&ImageFormatGIF, &FrameDimensionTime},
    {&ImageFormatIcon, &FrameDimensionResolution},
    {NULL}
};

GpStatus WINGDIPAPI GdipImageGetFrameCount(GpImage *image,
    GDIPCONST GUID* dimensionID, UINT* count)
{
    TRACE("(%p,%s,%p)\n", image, debugstr_guid(dimensionID), count);

    if(!image || !count)
        return InvalidParameter;

    if (!dimensionID ||
        IsEqualGUID(dimensionID, &image->format) ||
        IsEqualGUID(dimensionID, &FrameDimensionPage) ||
        IsEqualGUID(dimensionID, &FrameDimensionTime))
    {
        *count = image->frame_count;
        return Ok;
    }

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipImageGetFrameDimensionsCount(GpImage *image,
    UINT* count)
{
    TRACE("(%p, %p)\n", image, count);

    /* Native gdiplus 1.1 does not yet support multiple frame dimensions. */

    if(!image || !count)
        return InvalidParameter;

    *count = 1;

    return Ok;
}

GpStatus WINGDIPAPI GdipImageGetFrameDimensionsList(GpImage* image,
    GUID* dimensionIDs, UINT count)
{
    int i;
    const GUID *result=NULL;

    TRACE("(%p,%p,%u)\n", image, dimensionIDs, count);

    if(!image || !dimensionIDs || count != 1)
        return InvalidParameter;

    for (i=0; image_format_dimensions[i].format; i++)
    {
        if (IsEqualGUID(&image->format, image_format_dimensions[i].format))
        {
            result = image_format_dimensions[i].dimension;
            break;
        }
    }

    if (!result)
        result = &FrameDimensionPage;

    memcpy(dimensionIDs, result, sizeof(GUID));

    return Ok;
}

GpStatus WINGDIPAPI GdipLoadImageFromFile(GDIPCONST WCHAR* filename,
                                          GpImage **image)
{
    GpStatus stat;
    IStream *stream;

    TRACE("(%s) %p\n", debugstr_w(filename), image);

    if (!filename || !image)
        return InvalidParameter;

    *image = NULL;

    stat = GdipCreateStreamOnFile(filename, GENERIC_READ, &stream);

    if (stat != Ok)
        return stat;

    stat = GdipLoadImageFromStream(stream, image);

    IStream_Release(stream);

    return stat;
}

/* FIXME: no icm handling */
GpStatus WINGDIPAPI GdipLoadImageFromFileICM(GDIPCONST WCHAR* filename,GpImage **image)
{
    TRACE("(%s) %p\n", debugstr_w(filename), image);

    return GdipLoadImageFromFile(filename, image);
}

static void add_property(GpBitmap *bitmap, PropertyItem *item)
{
    UINT prop_size, prop_count;
    PropertyItem *prop_item;

    if (bitmap->prop_item == NULL)
    {
        prop_size = prop_count = 0;
        prop_item = calloc(1, item->length + sizeof(PropertyItem));
        if (!prop_item) return;
    }
    else
    {
        UINT i;
        char *item_value;

        GdipGetPropertySize(&bitmap->image, &prop_size, &prop_count);

        prop_item = calloc(1, prop_size + item->length + sizeof(PropertyItem));
        if (!prop_item) return;
        memcpy(prop_item, bitmap->prop_item, sizeof(PropertyItem) * bitmap->prop_count);
        prop_size -= sizeof(PropertyItem) * bitmap->prop_count;
        memcpy(prop_item + prop_count + 1, bitmap->prop_item + prop_count, prop_size);

        item_value = (char *)(prop_item + prop_count + 1);

        for (i = 0; i < prop_count; i++)
        {
            prop_item[i].value = item_value;
            item_value += prop_item[i].length;
        }
    }

    prop_item[prop_count].id = item->id;
    prop_item[prop_count].type = item->type;
    prop_item[prop_count].length = item->length;
    prop_item[prop_count].value = (char *)(prop_item + prop_count + 1) + prop_size;
    memcpy(prop_item[prop_count].value, item->value, item->length);

    free(bitmap->prop_item);
    bitmap->prop_item = prop_item;
    bitmap->prop_count++;
}

static BOOL get_bool_property(IWICMetadataReader *reader, const GUID *guid, const WCHAR *prop_name)
{
    HRESULT hr;
    GUID format;
    PROPVARIANT id, value;
    BOOL ret = FALSE;

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    if (FAILED(hr) || !IsEqualGUID(&format, guid)) return FALSE;

    PropVariantInit(&id);
    PropVariantInit(&value);

    id.vt = VT_LPWSTR;
    id.pwszVal = CoTaskMemAlloc((lstrlenW(prop_name) + 1) * sizeof(WCHAR));
    if (!id.pwszVal) return FALSE;
    lstrcpyW(id.pwszVal, prop_name);
    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    if (hr == S_OK && value.vt == VT_BOOL)
        ret = value.boolVal;

    PropVariantClear(&id);
    PropVariantClear(&value);

    return ret;
}

static PropertyItem *get_property(IWICMetadataReader *reader, const GUID *guid, const WCHAR *prop_name)
{
    HRESULT hr;
    GUID format;
    PROPVARIANT id, value;
    PropertyItem *item = NULL;

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    if (FAILED(hr) || !IsEqualGUID(&format, guid)) return NULL;

    PropVariantInit(&id);
    PropVariantInit(&value);

    id.vt = VT_LPWSTR;
    id.pwszVal = CoTaskMemAlloc((lstrlenW(prop_name) + 1) * sizeof(WCHAR));
    if (!id.pwszVal) return NULL;
    lstrcpyW(id.pwszVal, prop_name);
    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    if (hr == S_OK)
    {
        UINT item_size = propvariant_size(&value);
        if (item_size)
        {
            item_size += sizeof(*item);
            item = calloc(1, item_size);
            if (propvariant_to_item(&value, item, item_size, 0) != Ok)
            {
                free(item);
                item = NULL;
            }
        }
    }

    PropVariantClear(&id);
    PropVariantClear(&value);

    return item;
}

static PropertyItem *get_gif_comment(IWICMetadataReader *reader)
{
    PropertyItem *comment;

    comment = get_property(reader, &GUID_MetadataFormatGifComment, L"TextEntry");
    if (comment)
        comment->id = PropertyTagExifUserComment;

    return comment;
}

static PropertyItem *get_gif_loopcount(IWICMetadataReader *reader)
{
    PropertyItem *appext = NULL, *appdata = NULL, *loop = NULL;

    appext = get_property(reader, &GUID_MetadataFormatAPE, L"Application");
    if (appext)
    {
        if (appext->type == PropertyTagTypeByte && appext->length == 11 &&
            (!memcmp(appext->value, "NETSCAPE2.0", 11) || !memcmp(appext->value, "ANIMEXTS1.0", 11)))
        {
            appdata = get_property(reader, &GUID_MetadataFormatAPE, L"Data");
            if (appdata)
            {
                if (appdata->type == PropertyTagTypeByte && appdata->length == 4)
                {
                    BYTE *data = appdata->value;
                    if (data[0] == 3 && data[1] == 1)
                    {
                        loop = calloc(1, sizeof(*loop) + sizeof(SHORT));
                        if (loop)
                        {
                            loop->type = PropertyTagTypeShort;
                            loop->id = PropertyTagLoopCount;
                            loop->length = sizeof(SHORT);
                            loop->value = loop + 1;
                            *(SHORT *)loop->value = data[2] | (data[3] << 8);
                        }
                    }
                }
            }
        }
    }

    free(appext);
    free(appdata);

    return loop;
}

static PropertyItem *get_gif_background(IWICMetadataReader *reader)
{
    PropertyItem *background = NULL;

    if (get_bool_property(reader, &GUID_MetadataFormatLSD, L"GlobalColorTableFlag"))
    {
        background = get_property(reader, &GUID_MetadataFormatLSD, L"BackgroundColorIndex");
        if (background)
            background->id = PropertyTagIndexBackground;
    }

    return background;
}

static PropertyItem *get_gif_palette(IWICBitmapDecoder *decoder, IWICMetadataReader *reader)
{
    HRESULT hr;
    IWICImagingFactory *factory;
    IWICPalette *palette;
    UINT count = 0;
    WICColor colors[256];

    if (!get_bool_property(reader, &GUID_MetadataFormatLSD, L"GlobalColorTableFlag"))
        return NULL;

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (hr != S_OK) return NULL;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    if (hr == S_OK)
    {
        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
        if (hr == S_OK)
            IWICPalette_GetColors(palette, 256, colors, &count);

        IWICPalette_Release(palette);
    }

    IWICImagingFactory_Release(factory);

    if (count)
    {
        PropertyItem *pal;
        UINT i;
        BYTE *rgb;

        pal = calloc(1, sizeof(*pal) + count * 3);
        if (!pal) return NULL;
        pal->type = PropertyTagTypeByte;
        pal->id = PropertyTagGlobalPalette;
        pal->value = pal + 1;
        pal->length = count * 3;

        rgb = pal->value;

        for (i = 0; i < count; i++)
        {
            rgb[i*3] = (colors[i] >> 16) & 0xff;
            rgb[i*3 + 1] = (colors[i] >> 8) & 0xff;
            rgb[i*3 + 2] = colors[i] & 0xff;
        }

        return pal;
    }

    return NULL;
}

static PropertyItem *get_gif_transparent_idx(IWICMetadataReader *reader)
{
    PropertyItem *index = NULL;

    if (get_bool_property(reader, &GUID_MetadataFormatGCE, L"TransparencyFlag"))
    {
        index = get_property(reader, &GUID_MetadataFormatGCE, L"TransparentColorIndex");
        if (index)
            index->id = PropertyTagIndexTransparent;
    }
    return index;
}

static void get_gif_frame_property(IWICBitmapFrameDecode *frame, const GUID *format, const WCHAR *property, LONG *value)
{
    HRESULT hr;
    IWICMetadataBlockReader *block_reader;
    IWICMetadataReader *reader;
    UINT block_count, i;
    PropertyItem *prop;

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&block_reader);
    if (hr == S_OK)
    {
        hr = IWICMetadataBlockReader_GetCount(block_reader, &block_count);
        if (hr == S_OK)
        {
            for (i = 0; i < block_count; i++)
            {
                hr = IWICMetadataBlockReader_GetReaderByIndex(block_reader, i, &reader);
                if (hr == S_OK)
                {
                    prop = get_property(reader, format, property);
                    if (prop)
                    {
                        if (prop->type == PropertyTagTypeByte && prop->length == 1)
                            *value = *(BYTE *)prop->value;
                        else if (prop->type == PropertyTagTypeShort && prop->length == 2)
                            *value = *(SHORT *)prop->value;

                        free(prop);
                    }
                    IWICMetadataReader_Release(reader);
                }
            }
        }
        IWICMetadataBlockReader_Release(block_reader);
    }
}

static void gif_metadata_reader(GpBitmap *bitmap, IWICBitmapDecoder *decoder, UINT active_frame)
{
    HRESULT hr;
    IWICBitmapFrameDecode *frame;
    IWICMetadataBlockReader *block_reader;
    IWICMetadataReader *reader;
    UINT frame_count, block_count, i;
    PropertyItem *delay = NULL, *comment = NULL, *background = NULL;
    PropertyItem *transparent_idx = NULL, *loop = NULL, *palette = NULL;

    IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    delay = calloc(1, sizeof(*delay) + frame_count * sizeof(LONG));
    if (delay)
    {
        LONG *value;
        LONG frame_delay = 0;

        delay->type = PropertyTagTypeLong;
        delay->id = PropertyTagFrameDelay;
        delay->length = frame_count * sizeof(LONG);
        delay->value = delay + 1;

        value = delay->value;

        for (i = 0; i < frame_count; i++)
        {
            hr = IWICBitmapDecoder_GetFrame(decoder, i, &frame);
            if (hr == S_OK)
            {
                get_gif_frame_property(frame, &GUID_MetadataFormatGCE, L"Delay", &frame_delay);
                IWICBitmapFrameDecode_Release(frame);
            }
            value[i] = frame_delay;
        }
    }

    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICMetadataBlockReader, (void **)&block_reader);
    if (hr == S_OK)
    {
        hr = IWICMetadataBlockReader_GetCount(block_reader, &block_count);
        if (hr == S_OK)
        {
            for (i = 0; i < block_count; i++)
            {
                hr = IWICMetadataBlockReader_GetReaderByIndex(block_reader, i, &reader);
                if (hr == S_OK)
                {
                    if (!comment)
                        comment = get_gif_comment(reader);

                    if (!loop)
                        loop = get_gif_loopcount(reader);

                    if (!background)
                        background = get_gif_background(reader);

                    if (!palette)
                        palette = get_gif_palette(decoder, reader);

                    IWICMetadataReader_Release(reader);
                }
            }
        }
        IWICMetadataBlockReader_Release(block_reader);
    }

    if (!loop)
    {
        loop = calloc(1, sizeof(*loop) + sizeof(SHORT));
        if (loop)
        {
            loop->type = PropertyTagTypeShort;
            loop->id = PropertyTagLoopCount;
            loop->length = sizeof(SHORT);
            loop->value = loop + 1;
            *(SHORT *)loop->value = 1;
        }
    }

    if (delay) add_property(bitmap, delay);
    if (comment) add_property(bitmap, comment);
    if (loop) add_property(bitmap, loop);
    if (palette) add_property(bitmap, palette);
    if (background) add_property(bitmap, background);

    free(delay);
    free(comment);
    free(loop);
    free(palette);
    free(background);

    /* Win7 gdiplus always returns transparent color index from frame 0 */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (hr != S_OK) return;

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&block_reader);
    if (hr == S_OK)
    {
        hr = IWICMetadataBlockReader_GetCount(block_reader, &block_count);
        if (hr == S_OK)
        {
            for (i = 0; i < block_count; i++)
            {
                hr = IWICMetadataBlockReader_GetReaderByIndex(block_reader, i, &reader);
                if (hr == S_OK)
                {
                    if (!transparent_idx)
                        transparent_idx = get_gif_transparent_idx(reader);

                    IWICMetadataReader_Release(reader);
                }
            }
        }
        IWICMetadataBlockReader_Release(block_reader);
    }

    if (transparent_idx) add_property(bitmap, transparent_idx);
    free(transparent_idx);

    IWICBitmapFrameDecode_Release(frame);
}

static PropertyItem* create_prop(PROPID propid, PROPVARIANT* value)
{
    PropertyItem *item = NULL;
    UINT item_size = propvariant_size(value);

    if (item_size)
    {
        item_size += sizeof(*item);
        item = calloc(1, item_size);
        if (propvariant_to_item(value, item, item_size, propid) != Ok)
        {
            free(item);
            item = NULL;
        }
    }

    return item;
}

static ULONG get_ulong_by_index(IWICMetadataReader* reader, ULONG index)
{
    PROPVARIANT value;
    HRESULT hr;
    ULONG result=0;

    hr = IWICMetadataReader_GetValueByIndex(reader, index, NULL, NULL, &value);
    if (SUCCEEDED(hr))
    {
        switch (value.vt)
        {
        case VT_UI4:
            result = value.ulVal;
            break;
        default:
            ERR("unhandled case %u\n", value.vt);
            break;
        }
        PropVariantClear(&value);
    }
    return result;
}

static HRESULT png_read_text(IWICMetadataReader *reader, GpBitmap *bitmap, BOOL **seen_text)
{
    HRESULT hr;
    UINT i;
    PROPVARIANT name, value;
    PropertyItem* item;
    static const struct keyword_info {
        const char* name;
        PROPID propid;
    } keywords[] = {
        { "Title", PropertyTagImageTitle },
        { "Author", PropertyTagArtist },
        { "Description", PropertyTagImageDescription },
        { "Copyright", PropertyTagCopyright },
        { "Software", PropertyTagSoftwareUsed },
        { "Source", PropertyTagEquipModel },
        { "Comment", PropertyTagExifUserComment },
    };

    if (*seen_text == NULL)
        *seen_text = calloc(ARRAY_SIZE(keywords), sizeof(BOOL));
    if (*seen_text == NULL)
        return E_OUTOFMEMORY;

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &name, &value);
    if (FAILED(hr))
        return hr;

    if (name.vt == VT_LPSTR)
    {
        for (i = 0; i < ARRAY_SIZE(keywords); i++)
        {
            if (!strcmp(keywords[i].name, name.pszVal))
                break;
        }
        if (i < ARRAY_SIZE(keywords) && !(*seen_text)[i])
        {
            (*seen_text)[i] = TRUE;
            item = create_prop(keywords[i].propid, &value);
            if (item)
                add_property(bitmap, item);
            free(item);
        }
    }

    PropVariantClear(&name);
    PropVariantClear(&value);

    return S_OK;
}

static HRESULT png_read_gamma(IWICMetadataReader *reader, GpBitmap *bitmap)
{
    PropertyItem* item;
    ULONG *rational;

    item = calloc(1, sizeof(*item) + sizeof(ULONG) * 2);
    if (!item)
        return E_OUTOFMEMORY;

    item->length = sizeof(ULONG) * 2;
    item->type = PropertyTagTypeRational;
    item->id = PropertyTagGamma;
    rational = item->value = item + 1;
    rational[0] = 100000;
    rational[1] = get_ulong_by_index(reader, 0);
    add_property(bitmap, item);
    free(item);

    return S_OK;
}

static HRESULT png_read_whitepoint(IWICMetadataReader *reader, GpBitmap *bitmap)
{
    PropertyItem* item;
    ULONG *rational;

    item = calloc(1, sizeof(*item) + sizeof(ULONG) * 4);
    if (!item)
        return E_OUTOFMEMORY;

    item->length = sizeof(ULONG) * 4;
    item->type = PropertyTagTypeRational;
    item->id = PropertyTagWhitePoint;
    rational = item->value = item + 1;
    rational[0] = get_ulong_by_index(reader, 0);
    rational[1] = 100000;
    rational[2] = get_ulong_by_index(reader, 1);
    rational[3] = 100000;
    add_property(bitmap, item);
    free(item);

    return S_OK;
}

static HRESULT png_read_chromaticity(IWICMetadataReader *reader, GpBitmap *bitmap)
{
    PropertyItem* item;
    ULONG *rational;

    item = calloc(1, sizeof(*item) + sizeof(ULONG) * 12);
    if (!item)
        return E_OUTOFMEMORY;

    item->length = sizeof(ULONG) * 12;
    item->type = PropertyTagTypeRational;
    item->id = PropertyTagPrimaryChromaticities;
    rational = item->value = item + 1;
    rational[0] = get_ulong_by_index(reader, 2);
    rational[1] = 100000;
    rational[2] = get_ulong_by_index(reader, 3);
    rational[3] = 100000;
    rational[4] = get_ulong_by_index(reader, 4);
    rational[5] = 100000;
    rational[6] = get_ulong_by_index(reader, 5);
    rational[7] = 100000;
    rational[8] = get_ulong_by_index(reader, 6);
    rational[9] = 100000;
    rational[10] = get_ulong_by_index(reader, 7);
    rational[11] = 100000;
    add_property(bitmap, item);
    free(item);

    return S_OK;
}

static HRESULT png_read_time(IWICMetadataReader *reader, GpBitmap *bitmap)
{
    HRESULT hr;
    UINT item_size, i;
    PropertyItem* item;
    PROPVARIANT value;
    USHORT datetime[6];

    for (i = 0; i < 6; i++)
    {
        hr = IWICMetadataReader_GetValueByIndex(reader, i, NULL, NULL, &value);
        if (FAILED(hr))
            return hr;
        if (i == 0 && value.vt == VT_UI2)
            datetime[0] = value.uiVal;
        else if (i > 0 && value.vt == VT_UI1)
            datetime[i] = value.bVal;
        else
        {
            PropVariantClear(&value);
            return E_FAIL;
        }
        PropVariantClear(&value);
    }

    item_size = 20;
    item = calloc(1, sizeof(*item) + item_size);
    if (!item)
        return E_OUTOFMEMORY;

    item->id = PropertyTagDateTime;
    item->type = PropertyTagTypeASCII;
    item->length = item_size;
    item->value = item + 1;
    snprintf(item->value, item_size, "%04u:%02u:%02u %02u:%02u:%02u",
        datetime[0], datetime[1], datetime[2], datetime[3], datetime[4], datetime[5]);

    add_property(bitmap, item);
    free(item);

    return S_OK;
}

static HRESULT png_read_histogram(IWICMetadataReader *reader, GpBitmap *bitmap)
{
    HRESULT hr;
    PropertyItem* item;
    PROPVARIANT value;

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    if (FAILED(hr))
        return hr;

    item = create_prop(PropertyTagPaletteHistogram, &value);
    if (item)
        add_property(bitmap, item);
    free(item);

    PropVariantClear(&value);

    return S_OK;
}

static HRESULT png_add_unit_properties(IWICBitmapFrameDecode *frame, GpBitmap *bitmap)
{
    HRESULT hr;
    double dpiX, dpiY;
    PropertyItem *unit, *unitX, *unitY;

    hr = IWICBitmapFrameDecode_GetResolution(frame, &dpiX, &dpiY);
    if (FAILED(hr))
        return hr;

    unit = calloc(1, sizeof(*unit) + 1);
    unitX = calloc(1, sizeof(*unitX) + 4);
    unitY = calloc(1, sizeof(*unitY) + 4);

    if (!unit || !unitX || !unitY)
    {
        free(unit);
        free(unitX);
        free(unitY);
        return E_OUTOFMEMORY;
    }

    unit->type = PropertyTagTypeByte;
    unit->id = PropertyTagPixelUnit;
    unit->length = 1;
    unit->value = unit + 1;
    *(BYTE *)unit->value = 1;
    add_property(bitmap, unit);
    free(unit);

    unitX->type = PropertyTagTypeLong;
    unitX->id = PropertyTagPixelPerUnitX;
    unitX->length = 4;
    unitX->value = unitX + 1;
    *(ULONG *)unitX->value = (dpiX == 96.0) ? 0 : gdip_round(dpiX / 0.0254);
    add_property(bitmap, unitX);
    free(unitX);

    unitY->type = PropertyTagTypeLong;
    unitY->id = PropertyTagPixelPerUnitY;
    unitY->length = 4;
    unitY->value = unitY + 1;
    *(ULONG *)unitY->value = (dpiY == 96.0) ? 0 : gdip_round(dpiY / 0.0254);
    add_property(bitmap, unitY);
    free(unitY);

    return S_OK;
}

static void png_metadata_reader(GpBitmap *bitmap, IWICBitmapDecoder *decoder, UINT active_frame)
{
    HRESULT hr;
    IWICBitmapFrameDecode *frame;
    IWICMetadataBlockReader *block_reader;
    UINT block_count, i;
    BOOL seen_gamma=FALSE, seen_whitepoint=FALSE, seen_chrm=FALSE, seen_time=FALSE, seen_histogram=FALSE;
    BOOL *seen_text = NULL;

    hr = IWICBitmapDecoder_GetFrame(decoder, active_frame, &frame);
    if (hr != S_OK)
        return;

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&block_reader);
    if (hr != S_OK)
    {
        IWICBitmapFrameDecode_Release(frame);
        return;
    }

    hr = IWICMetadataBlockReader_GetCount(block_reader, &block_count);
    if (hr != S_OK)
    {
        IWICMetadataBlockReader_Release(block_reader);
        IWICBitmapFrameDecode_Release(frame);
        return;
    }

    for (i = 0; i < block_count; i++)
    {
        IWICMetadataReader *reader;
        GUID format;

        hr = IWICMetadataBlockReader_GetReaderByIndex(block_reader, i, &reader);
        if (hr != S_OK)
            continue;

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        if (FAILED(hr))
        {
            IWICMetadataReader_Release(reader);
            continue;
        }

        if (IsEqualGUID(&GUID_MetadataFormatChunktEXt, &format))
            png_read_text(reader, bitmap, &seen_text);
        else if (IsEqualGUID(&GUID_MetadataFormatChunkgAMA, &format))
        {
            if (!seen_gamma)
            {
                hr = png_read_gamma(reader, bitmap);
                seen_gamma = SUCCEEDED(hr);
            }
        }
        else if (IsEqualGUID(&GUID_MetadataFormatChunkcHRM, &format))
        {
            if (!seen_whitepoint)
            {
                hr = png_read_whitepoint(reader, bitmap);
                seen_whitepoint = SUCCEEDED(hr);
            }
            if (!seen_chrm)
            {
                hr = png_read_chromaticity(reader, bitmap);
                seen_chrm = SUCCEEDED(hr);
            }
        }
        else if (IsEqualGUID(&GUID_MetadataFormatChunktIME, &format))
        {
            if (!seen_time)
            {
                hr = png_read_time(reader, bitmap);
                seen_time = SUCCEEDED(hr);
            }
        }
        else if (IsEqualGUID(&GUID_MetadataFormatChunkhIST, &format))
        {
            if (!seen_histogram)
            {
                hr = png_read_histogram(reader, bitmap);
                seen_histogram = SUCCEEDED(hr);
            }
        }

        IWICMetadataReader_Release(reader);
    }
    free(seen_text);

    png_add_unit_properties(frame, bitmap);

    IWICMetadataBlockReader_Release(block_reader);

    IWICBitmapFrameDecode_Release(frame);
}

static GpStatus initialize_decoder_wic(IStream *stream, REFGUID container, IWICBitmapDecoder **decoder)
{
    IWICImagingFactory *factory;
    HRESULT hr;

    TRACE("%p,%s\n", stream, wine_dbgstr_guid(container));

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (FAILED(hr)) return hresult_to_status(hr);
    hr = IWICImagingFactory_CreateDecoder(factory, container, NULL, decoder);
    IWICImagingFactory_Release(factory);
    if (FAILED(hr)) return hresult_to_status(hr);

    hr = IWICBitmapDecoder_Initialize(*decoder, stream, WICDecodeMetadataCacheOnLoad);
    if (FAILED(hr))
    {
        IWICBitmapDecoder_Release(*decoder);
        return hresult_to_status(hr);
    }
    return Ok;
}

typedef void (*metadata_reader_func)(GpBitmap *bitmap, IWICBitmapDecoder *decoder, UINT frame);

static GpStatus decode_frame_wic(IWICBitmapDecoder *decoder, BOOL force_conversion,
    UINT active_frame, metadata_reader_func metadata_reader, GpImage **image)
{
    GpStatus status=Ok;
    GpBitmap *bitmap;
    HRESULT hr;
    IWICBitmapFrameDecode *frame;
    IWICBitmapSource *source=NULL;
    IWICMetadataBlockReader *block_reader;
    WICPixelFormatGUID wic_format;
    PixelFormat gdip_format=0;
    ColorPalette *palette = NULL;
    WICBitmapPaletteType palette_type = WICBitmapPaletteTypeFixedHalftone256;
    int i;
    UINT width, height, frame_count;
    BitmapData lockeddata;
    WICRect wrc;

    TRACE("%p,%u,%p\n", decoder, active_frame, image);

    IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    hr = IWICBitmapDecoder_GetFrame(decoder, active_frame, &frame);
    if (SUCCEEDED(hr)) /* got frame */
    {
        hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &wic_format);

        if (SUCCEEDED(hr))
        {
            if (!force_conversion)
            {
                for (i=0; pixel_formats[i].wic_format; i++)
                {
                    if (IsEqualGUID(&wic_format, pixel_formats[i].wic_format))
                    {
                        source = (IWICBitmapSource*)frame;
                        IWICBitmapSource_AddRef(source);
                        gdip_format = pixel_formats[i].gdip_format;
                        palette_type = pixel_formats[i].palette_type;
                        break;
                    }
                }
            }
            if (!source)
            {
                /* unknown format; fall back on 32bppARGB */
                hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA, (IWICBitmapSource*)frame, &source);
                gdip_format = PixelFormat32bppARGB;
            }
            TRACE("%s => %#x\n", wine_dbgstr_guid(&wic_format), gdip_format);
        }

        if (SUCCEEDED(hr)) /* got source */
        {
            hr = IWICBitmapSource_GetSize(source, &width, &height);

            if (SUCCEEDED(hr))
                status = GdipCreateBitmapFromScan0(width, height, 0, gdip_format,
                    NULL, &bitmap);

            if (SUCCEEDED(hr) && status == Ok) /* created bitmap */
            {
                status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeWrite,
                    gdip_format, &lockeddata);
                if (status == Ok) /* locked bitmap */
                {
                    wrc.X = 0;
                    wrc.Width = width;
                    wrc.Height = 1;
                    for (i=0; i<height; i++)
                    {
                        wrc.Y = i;
                        hr = IWICBitmapSource_CopyPixels(source, &wrc, abs(lockeddata.Stride),
                            abs(lockeddata.Stride), (BYTE*)lockeddata.Scan0+lockeddata.Stride*i);
                        if (FAILED(hr)) break;
                    }

                    GdipBitmapUnlockBits(bitmap, &lockeddata);
                }

                if (SUCCEEDED(hr) && status == Ok)
                    *image = &bitmap->image;
                else
                {
                    *image = NULL;
                    GdipDisposeImage(&bitmap->image);
                }

                if (SUCCEEDED(hr) && status == Ok)
                {
                    double dpix, dpiy;
                    hr = IWICBitmapSource_GetResolution(source, &dpix, &dpiy);
                    if (SUCCEEDED(hr))
                    {
                        bitmap->image.xres = dpix;
                        bitmap->image.yres = dpiy;
                    }
                    hr = S_OK;
                }
            }

            IWICBitmapSource_Release(source);
        }

        if (SUCCEEDED(hr) && status == Ok) {
            bitmap->metadata_reader = NULL;

            if (metadata_reader)
                metadata_reader(bitmap, decoder, active_frame);
            else if (IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&block_reader) == S_OK)
            {
                UINT block_count = 0;
                if (IWICMetadataBlockReader_GetCount(block_reader, &block_count) == S_OK && block_count)
                    IWICMetadataBlockReader_GetReaderByIndex(block_reader, 0, &bitmap->metadata_reader);
                IWICMetadataBlockReader_Release(block_reader);
            }

            palette = get_palette(frame, palette_type);
        }
        IWICBitmapFrameDecode_Release(frame);
    }

    if (FAILED(hr) && status == Ok) status = hresult_to_status(hr);

    if (status == Ok)
    {
        /* Native GDI+ used to be smarter, but since Win7 it just sets these flags. */
        bitmap->image.flags |= ImageFlagsReadOnly|ImageFlagsHasRealPixelSize|ImageFlagsHasRealDPI;
        if (IsEqualGUID(&wic_format, &GUID_WICPixelFormat2bppGray) ||
            IsEqualGUID(&wic_format, &GUID_WICPixelFormat4bppGray) ||
            IsEqualGUID(&wic_format, &GUID_WICPixelFormat8bppGray) ||
            IsEqualGUID(&wic_format, &GUID_WICPixelFormat16bppGray))
            bitmap->image.flags |= ImageFlagsColorSpaceGRAY;
        else
            bitmap->image.flags |= ImageFlagsColorSpaceRGB;
        bitmap->image.frame_count = frame_count;
        bitmap->image.current_frame = active_frame;
        bitmap->image.decoder = decoder;
        IWICBitmapDecoder_AddRef(decoder);
        if (palette)
        {
            free(bitmap->image.palette);
            bitmap->image.palette = palette;
        }
        else
        {
            if (IsEqualGUID(&wic_format, &GUID_WICPixelFormatBlackWhite))
                bitmap->image.palette->Flags = 0;
        }
        TRACE("=> %p\n", *image);
    }

    return status;
}

static GpStatus decode_image_wic(IStream *stream, REFGUID container,
        metadata_reader_func metadata_reader, GpImage **image)
{
    IWICBitmapDecoder *decoder;
    GpStatus status;

    status = initialize_decoder_wic(stream, container, &decoder);
    if(status != Ok)
        return status;

    status = decode_frame_wic(decoder, FALSE, 0, metadata_reader, image);
    IWICBitmapDecoder_Release(decoder);
    return status;
}

static GpStatus select_frame_wic(GpImage *image, UINT active_frame)
{
    size_t obj_size, body_offset;
    GpImage *new_image;
    GpStatus status;

    status = decode_frame_wic(image->decoder, FALSE, active_frame, NULL, &new_image);
    if(status != Ok)
        return status;

    if (image->type == ImageTypeBitmap)
        obj_size = sizeof(GpBitmap);
    else if (image->type == ImageTypeMetafile)
        obj_size = sizeof(GpMetafile);
    else
    {
        ERR("unknown image type: %d\n", image->type);
        GdipDisposeImage(new_image);
        return GenericError;
    }

    memcpy(&new_image->format, &image->format, sizeof(GUID));
    new_image->encoder = image->encoder;
    image->encoder = NULL;
    free_image_data(image);
    memcpy(image, new_image, FIELD_OFFSET(GpImage, lock));
    body_offset = RTL_SIZEOF_THROUGH_FIELD(GpImage, lock);
    memcpy((char *)image + body_offset, (char *)new_image + body_offset, obj_size - body_offset);
    new_image->type = ~0;
    free(new_image);
    return Ok;
}

static HRESULT get_gif_frame_rect(IWICBitmapFrameDecode *frame,
        UINT *left, UINT *top, UINT *width, UINT *height)
{
    LONG frame_left = 0, frame_top = 0;
    get_gif_frame_property(frame, &GUID_MetadataFormatIMD, L"Left", &frame_left);
    get_gif_frame_property(frame, &GUID_MetadataFormatIMD, L"Top", &frame_top);
    *left = frame_left;
    *top = frame_top;

    return IWICBitmapFrameDecode_GetSize(frame, width, height);
}

static HRESULT blit_gif_frame(GpBitmap *bitmap, IWICBitmapFrameDecode *frame, BOOL first_frame)
{
    UINT i, j, left, top, width, height;
    IWICBitmapSource *source;
    BYTE *new_bits;
    HRESULT hr;

    hr = get_gif_frame_rect(frame, &left, &top, &width, &height);
    if(FAILED(hr))
        return hr;

    hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA, (IWICBitmapSource*)frame, &source);
    if(FAILED(hr))
        return hr;

    new_bits = calloc(width * height, 4);
    if(!new_bits)
        return E_OUTOFMEMORY;

    hr = IWICBitmapSource_CopyPixels(source, NULL, width*4, width*height*4, new_bits);
    IWICBitmapSource_Release(source);
    if(FAILED(hr)) {
        free(new_bits);
        return hr;
    }

    for(i=0; i<height && i+top<bitmap->height; i++) {
        for(j=0; j<width && j+left<bitmap->width; j++) {
            DWORD *src = (DWORD*)(new_bits+i*width*4+j*4);
            DWORD *dst = (DWORD*)(bitmap->bits+(i+top)*bitmap->stride+(j+left)*4);

            if(first_frame || *src>>24 != 0)
                *dst = *src;
        }
    }
    free(new_bits);
    return hr;
}

static DWORD get_gif_background_color(GpBitmap *bitmap)
{
    BYTE bgcolor_idx = 0;
    UINT i;

    for(i=0; i<bitmap->prop_count; i++) {
        if(bitmap->prop_item[i].id == PropertyTagIndexBackground)
            bgcolor_idx = *(BYTE*)bitmap->prop_item[i].value;
        else if(bitmap->prop_item[i].id == PropertyTagIndexTransparent)
            return 0;
    }

    for(i=0; i<bitmap->prop_count; i++) {
        if(bitmap->prop_item[i].id == PropertyTagGlobalPalette) {
            if(bitmap->prop_item[i].length/3 > bgcolor_idx) {
                BYTE *color = ((BYTE*)bitmap->prop_item[i].value)+bgcolor_idx*3;
                return color[2] + (color[1]<<8) + (color[0]<<16) + (0xffu<<24);
            }
            break;
        }
    }

    FIXME("can't get gif background color\n");
    return 0xffffffff;
}

static GpStatus select_frame_gif(GpImage* image, UINT active_frame)
{
    GpBitmap *bitmap = (GpBitmap*)image;
    IWICBitmapFrameDecode *frame;
    int cur_frame=0;
    LONG disposal;
    BOOL bgcolor_set = FALSE;
    DWORD bgcolor = 0;
    HRESULT hr;

    if(active_frame > image->current_frame) {
        hr = IWICBitmapDecoder_GetFrame(bitmap->image.decoder, image->current_frame, &frame);
        if(FAILED(hr))
            return hresult_to_status(hr);
        disposal = 0;
        get_gif_frame_property(frame, &GUID_MetadataFormatGCE, L"Disposal", &disposal);
        IWICBitmapFrameDecode_Release(frame);

        if(disposal == GIF_DISPOSE_RESTORE_TO_BKGND)
            cur_frame = image->current_frame;
        else if(disposal != GIF_DISPOSE_RESTORE_TO_PREV)
            cur_frame = image->current_frame+1;
    }

    while(cur_frame != active_frame) {
        hr = IWICBitmapDecoder_GetFrame(bitmap->image.decoder, cur_frame, &frame);
        if(FAILED(hr))
            return hresult_to_status(hr);
        disposal = 0;
        get_gif_frame_property(frame, &GUID_MetadataFormatGCE, L"Disposal", &disposal);

        if(disposal==GIF_DISPOSE_UNSPECIFIED || disposal==GIF_DISPOSE_DO_NOT_DISPOSE) {
            hr = blit_gif_frame(bitmap, frame, cur_frame==0);
            if(FAILED(hr))
                return hresult_to_status(hr);
        }else if(disposal == GIF_DISPOSE_RESTORE_TO_BKGND) {
            UINT left, top, width, height, i, j;

            if(!bgcolor_set) {
                bgcolor = get_gif_background_color(bitmap);
                bgcolor_set = TRUE;
            }

            hr = get_gif_frame_rect(frame, &left, &top, &width, &height);
            if(FAILED(hr))
                return hresult_to_status(hr);
            for(i=top; i<top+height && i<bitmap->height; i++) {
                DWORD *bits = (DWORD*)(bitmap->bits+i*bitmap->stride);
                for(j=left; j<left+width && j<bitmap->width; j++)
                    bits[j] = bgcolor;
            }
        }

        IWICBitmapFrameDecode_Release(frame);
        cur_frame++;
    }

    hr = IWICBitmapDecoder_GetFrame(bitmap->image.decoder, active_frame, &frame);
    if(FAILED(hr))
        return hresult_to_status(hr);

    hr = blit_gif_frame(bitmap, frame, cur_frame==0);
    IWICBitmapFrameDecode_Release(frame);
    if(FAILED(hr))
        return hresult_to_status(hr);

    image->current_frame = active_frame;
    return Ok;
}

static GpStatus decode_image_icon(IStream* stream, GpImage **image)
{
    return decode_image_wic(stream, &GUID_ContainerFormatIco, NULL, image);
}

static GpStatus decode_image_bmp(IStream* stream, GpImage **image)
{
    GpStatus status;
    GpBitmap* bitmap;

    status = decode_image_wic(stream, &GUID_ContainerFormatBmp, NULL, image);

    bitmap = (GpBitmap*)*image;

    if (status == Ok && bitmap->format == PixelFormat32bppARGB)
    {
        /* WIC supports bmp files with alpha, but gdiplus does not */
        bitmap->format = PixelFormat32bppRGB;
    }

    return status;
}

static GpStatus decode_image_jpeg(IStream* stream, GpImage **image)
{
    return decode_image_wic(stream, &GUID_ContainerFormatJpeg, NULL, image);
}

static BOOL has_png_transparency_chunk(IStream *pIStream)
{
    LARGE_INTEGER seek;
    BOOL has_tRNS = FALSE;
    HRESULT hr;
    BYTE header[8];

    seek.QuadPart = 8;
    do
    {
        ULARGE_INTEGER chunk_start;
        ULONG bytesread, chunk_size;

        hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, &chunk_start);
        if (FAILED(hr)) break;

        hr = IStream_Read(pIStream, header, 8, &bytesread);
        if (FAILED(hr) || bytesread < 8) break;

        chunk_size = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
        if (!memcmp(&header[4], "tRNS", 4))
        {
            has_tRNS = TRUE;
            break;
        }

        seek.QuadPart = chunk_start.QuadPart + chunk_size + 12; /* skip data and CRC */
    } while (memcmp(&header[4], "IDAT", 4) && memcmp(&header[4], "IEND", 4));

    TRACE("has_tRNS = %d\n", has_tRNS);
    return has_tRNS;
}

static GpStatus decode_image_png(IStream* stream, GpImage **image)
{
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    GpStatus status;
    HRESULT hr;
    GUID format;
    BOOL force_conversion = FALSE;

    status = initialize_decoder_wic(stream, &GUID_ContainerFormatPng, &decoder);
    if (status != Ok)
        return status;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (hr == S_OK)
    {
        hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
        if (hr == S_OK)
        {
            if (IsEqualGUID(&format, &GUID_WICPixelFormat8bppGray) ||
                IsEqualGUID(&format, &GUID_WICPixelFormat64bppRGBA))
                force_conversion = TRUE;
            else if ((IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed) ||
                      IsEqualGUID(&format, &GUID_WICPixelFormat4bppIndexed) ||
                      IsEqualGUID(&format, &GUID_WICPixelFormat2bppIndexed) ||
                      IsEqualGUID(&format, &GUID_WICPixelFormat1bppIndexed) ||
                      IsEqualGUID(&format, &GUID_WICPixelFormat24bppBGR)) &&
                     has_png_transparency_chunk(stream))
                force_conversion = TRUE;

            status = decode_frame_wic(decoder, force_conversion, 0, png_metadata_reader, image);
        }
        else
            status = hresult_to_status(hr);

        IWICBitmapFrameDecode_Release(frame);
    }
    else
        status = hresult_to_status(hr);

    IWICBitmapDecoder_Release(decoder);
    return status;
}

static GpStatus decode_image_gif(IStream* stream, GpImage **image)
{
    IWICBitmapDecoder *decoder;
    UINT frame_count;
    GpStatus status;
    HRESULT hr;

    status = initialize_decoder_wic(stream, &GUID_ContainerFormatGif, &decoder);
    if(status != Ok)
        return status;

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    if(FAILED(hr))
        return hresult_to_status(hr);

    status = decode_frame_wic(decoder, frame_count > 1, 0, gif_metadata_reader, image);
    IWICBitmapDecoder_Release(decoder);
    if(status != Ok)
        return status;

    if(frame_count > 1) {
        free((*image)->palette);
        (*image)->palette = NULL;
    }
    return Ok;
}

static GpStatus decode_image_tiff(IStream* stream, GpImage **image)
{
    return decode_image_wic(stream, &GUID_ContainerFormatTiff, NULL, image);
}

C_ASSERT(offsetof(WmfPlaceableFileHeader, Key) == 0);

static GpStatus load_wmf(IStream *stream, GpMetafile **metafile)
{
    WmfPlaceableFileHeader pfh;
    BOOL is_placeable = FALSE;
    LARGE_INTEGER seek;
    GpStatus status;
    METAHEADER mh;
    HMETAFILE hmf;
    HRESULT hr;
    ULONG size;
    void *buf;

    hr = IStream_Read(stream, &mh, sizeof(mh), &size);
    if (hr != S_OK || size != sizeof(mh))
        return GenericError;

    /* detect whether stream starts with a WmfPlaceablefileheader */
    if (*(UINT32 *)&mh == WMF_PLACEABLE_KEY)
    {
        seek.QuadPart = 0;
        hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
        if (FAILED(hr)) return hresult_to_status(hr);

        hr = IStream_Read(stream, &pfh, sizeof(pfh), &size);
        if (hr != S_OK || size != sizeof(pfh))
            return GenericError;

        hr = IStream_Read(stream, &mh, sizeof(mh), &size);
        if (hr != S_OK || size != sizeof(mh))
            return GenericError;

        is_placeable = TRUE;
    }

    seek.QuadPart = is_placeable ? sizeof(pfh) : 0;
    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hresult_to_status(hr);

    buf = malloc(mh.mtSize * 2);
    if (!buf) return OutOfMemory;

    hr = IStream_Read(stream, buf, mh.mtSize * 2, &size);
    if (hr != S_OK || size != mh.mtSize * 2)
    {
        free(buf);
        return GenericError;
    }

    hmf = SetMetaFileBitsEx(mh.mtSize * 2, buf);
    free(buf);
    if (!hmf)
        return GenericError;

    status = GdipCreateMetafileFromWmf(hmf, TRUE, is_placeable ? &pfh : NULL, metafile);
    if (status != Ok)
        DeleteMetaFile(hmf);
    return status;
}

static GpStatus decode_image_wmf(IStream *stream, GpImage **image)
{
    GpMetafile *metafile;
    GpStatus status;

    TRACE("%p %p\n", stream, image);

    if (!stream || !image)
        return InvalidParameter;

    status = load_wmf(stream, &metafile);
    if (status != Ok)
    {
        TRACE("Could not load metafile\n");
        return status;
    }

    *image = (GpImage *)metafile;
    TRACE("<-- %p\n", *image);

    return Ok;
}

static GpStatus load_emf(IStream *stream, GpMetafile **metafile)
{
    LARGE_INTEGER seek;
    ENHMETAHEADER emh;
    HENHMETAFILE hemf;
    GpStatus status;
    HRESULT hr;
    ULONG size;
    void *buf;

    hr = IStream_Read(stream, &emh, sizeof(emh), &size);
    if (hr != S_OK || size != sizeof(emh) || emh.dSignature != ENHMETA_SIGNATURE)
        return GenericError;

    seek.QuadPart = 0;
    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hresult_to_status(hr);

    buf = malloc(emh.nBytes);
    if (!buf) return OutOfMemory;

    hr = IStream_Read(stream, buf, emh.nBytes, &size);
    if (hr != S_OK || size != emh.nBytes)
    {
        free(buf);
        return GenericError;
    }

    hemf = SetEnhMetaFileBits(emh.nBytes, buf);
    free(buf);
    if (!hemf)
        return GenericError;

    status = GdipCreateMetafileFromEmf(hemf, TRUE, metafile);
    if (status != Ok)
        DeleteEnhMetaFile(hemf);
    return status;
}

static GpStatus decode_image_emf(IStream *stream, GpImage **image)
{
    GpMetafile *metafile;
    GpStatus status;

    TRACE("%p %p\n", stream, image);

    if (!stream || !image)
        return InvalidParameter;

    status = load_emf(stream, &metafile);
    if (status != Ok)
    {
        TRACE("Could not load metafile\n");
        return status;
    }

    *image = (GpImage *)metafile;
    TRACE("<-- %p\n", *image);

    return Ok;
}

typedef GpStatus (*encode_image_func)(GpImage *image, IStream* stream,
    GDIPCONST EncoderParameters* params);

typedef GpStatus (*decode_image_func)(IStream *stream, GpImage **image);

typedef GpStatus (*select_image_func)(GpImage *image, UINT active_frame);

typedef struct image_codec {
    ImageCodecInfo info;
    encode_image_func encode_func;
    decode_image_func decode_func;
    select_image_func select_func;
} image_codec;

typedef enum {
    BMP,
    JPEG,
    GIF,
    TIFF,
    EMF,
    WMF,
    PNG,
    ICO,
    NUM_CODECS
} ImageFormat;

static const struct image_codec codecs[NUM_CODECS];

static GpStatus get_decoder_info(IStream* stream, const struct image_codec **result)
{
    BYTE signature[8];
    const BYTE *pattern, *mask;
    LARGE_INTEGER seek;
    HRESULT hr;
    ULONG bytesread;
    int i;
    DWORD j, sig;

    /* seek to the start of the stream */
    seek.QuadPart = 0;
    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hresult_to_status(hr);

    /* read the first 8 bytes */
    /* FIXME: This assumes all codecs have signatures <= 8 bytes in length */
    hr = IStream_Read(stream, signature, 8, &bytesread);
    if (FAILED(hr)) return hresult_to_status(hr);
    if (hr == S_FALSE || bytesread == 0) return GenericError;

    for (i = 0; i < NUM_CODECS; i++) {
        if ((codecs[i].info.Flags & ImageCodecFlagsDecoder) &&
            bytesread >= codecs[i].info.SigSize)
        {
            for (sig=0; sig<codecs[i].info.SigCount; sig++)
            {
                pattern = &codecs[i].info.SigPattern[codecs[i].info.SigSize*sig];
                mask = &codecs[i].info.SigMask[codecs[i].info.SigSize*sig];
                for (j=0; j<codecs[i].info.SigSize; j++)
                    if ((signature[j] & mask[j]) != pattern[j])
                        break;
                if (j == codecs[i].info.SigSize)
                {
                    *result = &codecs[i];
                    return Ok;
                }
            }
        }
    }

    TRACE("no match for %lu byte signature %x %x %x %x %x %x %x %x\n", bytesread,
        signature[0],signature[1],signature[2],signature[3],
        signature[4],signature[5],signature[6],signature[7]);

    return GenericError;
}

static GpStatus get_decoder_info_from_image(GpImage *image, const struct image_codec **result)
{
    int i;

    for (i = 0; i < NUM_CODECS; i++) {
        if ((codecs[i].info.Flags & ImageCodecFlagsDecoder) &&
                IsEqualIID(&codecs[i].info.FormatID, &image->format))
        {
            *result = &codecs[i];
            return Ok;
        }
    }

    TRACE("no match for format: %s\n", wine_dbgstr_guid(&image->format));
    return GenericError;
}

GpStatus WINGDIPAPI GdipImageSelectActiveFrame(GpImage *image, GDIPCONST GUID *dimensionID,
                                               UINT frame)
{
    GpStatus stat;
    const struct image_codec *codec = NULL;

    TRACE("(%p,%s,%u)\n", image, debugstr_guid(dimensionID), frame);

    if (!image || !dimensionID)
        return InvalidParameter;
    if(!image_lock(image))
        return ObjectBusy;

    if (frame >= image->frame_count)
        WARN("requested frame %u, but image has only %u\n", frame, image->frame_count);
        /* rely on codec->select_func() to fail */

    if (image->type != ImageTypeBitmap && image->type != ImageTypeMetafile)
    {
        WARN("invalid image type %d\n", image->type);
        image_unlock(image);
        return InvalidParameter;
    }

    if (image->current_frame == frame)
    {
        image_unlock(image);
        return Ok;
    }

    if (!image->decoder)
    {
        TRACE("image doesn't have an associated decoder\n");
        image_unlock(image);
        return Ok;
    }

    /* choose an appropriate image decoder */
    stat = get_decoder_info_from_image(image, &codec);
    if (stat != Ok)
    {
        WARN("can't find decoder info\n");
        image_unlock(image);
        return stat;
    }

    stat = codec->select_func(image, frame);
    image_unlock(image);
    return stat;
}

GpStatus WINGDIPAPI GdipLoadImageFromStream(IStream *stream, GpImage **image)
{
    GpStatus stat;
    LARGE_INTEGER seek;
    HRESULT hr;
    const struct image_codec *codec=NULL;

    TRACE("%p %p\n", stream, image);

    if (!stream || !image)
        return InvalidParameter;

    /* choose an appropriate image decoder */
    stat = get_decoder_info(stream, &codec);
    if (stat != Ok) return stat;

    /* seek to the start of the stream */
    seek.QuadPart = 0;
    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hresult_to_status(hr);

    /* call on the image decoder to do the real work */
    stat = codec->decode_func(stream, image);

    /* take note of the original data format */
    if (stat == Ok)
    {
        memcpy(&(*image)->format, &codec->info.FormatID, sizeof(GUID));
        return Ok;
    }

    return stat;
}

/* FIXME: no ICM */
GpStatus WINGDIPAPI GdipLoadImageFromStreamICM(IStream* stream, GpImage **image)
{
    TRACE("%p %p\n", stream, image);

    return GdipLoadImageFromStream(stream, image);
}

GpStatus WINGDIPAPI GdipRemovePropertyItem(GpImage *image, PROPID propId)
{
    static int calls;

    TRACE("(%p,%lu)\n", image, propId);

    if(!image)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPropertyItem(GpImage *image, GDIPCONST PropertyItem* item)
{
    static int calls;

    if (!image || !item) return InvalidParameter;

    TRACE("(%p,%p:%#lx,%u,%lu,%p)\n", image, item, item->id, item->type, item->length, item->value);

    if(!(calls++))
        FIXME("not implemented\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipSaveImageToFile(GpImage *image, GDIPCONST WCHAR* filename,
                                        GDIPCONST CLSID *clsidEncoder,
                                        GDIPCONST EncoderParameters *encoderParams)
{
    GpStatus stat;
    IStream *stream;

    TRACE("%p (%s) %p %p\n", image, debugstr_w(filename), clsidEncoder, encoderParams);

    if (!image || !filename|| !clsidEncoder)
        return InvalidParameter;

    /* this might release an old file stream held by the encoder so we can re-create it below */
    terminate_encoder_wic(image);

    stat = GdipCreateStreamOnFile(filename, GENERIC_WRITE, &stream);
    if (stat != Ok)
        return GenericError;

    stat = GdipSaveImageToStream(image, stream, clsidEncoder, encoderParams);

    IStream_Release(stream);
    return stat;
}

/*************************************************************************
 * Encoding functions -
 *   These functions encode an image in different image file formats.
 */

static GpStatus initialize_encoder_wic(IStream *stream, REFGUID container, GpImage *image)
{
    IWICImagingFactory *factory;
    HRESULT hr;

    TRACE("%p,%s\n", stream, wine_dbgstr_guid(container));

    terminate_encoder_wic(image); /* terminate previous encoder if it exists */

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (FAILED(hr)) return hresult_to_status(hr);
    hr = IWICImagingFactory_CreateEncoder(factory, container, NULL, &image->encoder);
    IWICImagingFactory_Release(factory);
    if (FAILED(hr))
    {
        image->encoder = NULL;
        return hresult_to_status(hr);
    }

    hr = IWICBitmapEncoder_Initialize(image->encoder, stream, WICBitmapEncoderNoCache);
    if (FAILED(hr))
    {
        IWICBitmapEncoder_Release(image->encoder);
        image->encoder = NULL;
        return hresult_to_status(hr);
    }
    return Ok;
}

GpStatus terminate_encoder_wic(GpImage *image)
{
    if (!image->encoder)
        return Ok;
    else
    {
        HRESULT hr = IWICBitmapEncoder_Commit(image->encoder);
        IWICBitmapEncoder_Release(image->encoder);
        image->encoder = NULL;
        return hresult_to_status(hr);
    }
}

static GpStatus encode_frame_wic(IWICBitmapEncoder *encoder, GpImage *image)
{
    GpStatus stat;
    GpBitmap *bitmap;
    IWICBitmapFrameEncode *frameencode;
    IPropertyBag2 *encoderoptions;
    GUID container_format;
    HRESULT hr;
    UINT width, height;
    PixelFormat gdipformat=0;
    const WICPixelFormatGUID *desired_wicformat;
    WICPixelFormatGUID wicformat;
    GpRect rc;
    BitmapData lockeddata;
    UINT i;

    if (image->type != ImageTypeBitmap)
        return GenericError;

    bitmap = (GpBitmap*)image;

    GdipGetImageWidth(image, &width);
    GdipGetImageHeight(image, &height);

    rc.X = 0;
    rc.Y = 0;
    rc.Width = width;
    rc.Height = height;

    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frameencode, &encoderoptions);

    if (SUCCEEDED(hr)) /* created frame */
    {
        hr = IWICBitmapEncoder_GetContainerFormat(encoder, &container_format);
        if (SUCCEEDED(hr) && IsEqualGUID(&container_format, &GUID_ContainerFormatPng))
        {
            /* disable PNG filters for faster encoding */
            PROPBAG2 filter_option = { .pstrName = (LPOLESTR) L"FilterOption" };
            VARIANT filter_value;
            VariantInit(&filter_value);
            V_VT(&filter_value) = VT_UI1;
            V_UI1(&filter_value) = WICPngFilterNone;
            hr = IPropertyBag2_Write(encoderoptions, 1, &filter_option, &filter_value);
        }

        if (SUCCEEDED(hr))
            hr = IWICBitmapFrameEncode_Initialize(frameencode, encoderoptions);

        if (SUCCEEDED(hr))
            hr = IWICBitmapFrameEncode_SetSize(frameencode, width, height);

        if (SUCCEEDED(hr))
            hr = IWICBitmapFrameEncode_SetResolution(frameencode, image->xres, image->yres);

        if (SUCCEEDED(hr))
        {
            for (i=0; pixel_formats[i].wic_format; i++)
            {
                if (pixel_formats[i].gdip_format == bitmap->format)
                {
                    desired_wicformat = pixel_formats[i].wic_format;
                    gdipformat = bitmap->format;
                    break;
                }
            }
            if (!gdipformat)
            {
                desired_wicformat = &GUID_WICPixelFormat32bppBGRA;
                gdipformat = PixelFormat32bppARGB;
            }

            memcpy(&wicformat, desired_wicformat, sizeof(GUID));
            hr = IWICBitmapFrameEncode_SetPixelFormat(frameencode, &wicformat);
        }

        if (SUCCEEDED(hr) && !IsEqualGUID(desired_wicformat, &wicformat))
        {
            /* Encoder doesn't support this bitmap's format. */
            gdipformat = 0;
            for (i=0; pixel_formats[i].wic_format; i++)
            {
                if (IsEqualGUID(&wicformat, pixel_formats[i].wic_format))
                {
                    gdipformat = pixel_formats[i].gdip_format;
                    break;
                }
            }
            if (!gdipformat)
            {
                ERR("Cannot support encoder format %s\n", debugstr_guid(&wicformat));
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr) && IsIndexedPixelFormat(gdipformat) && image->palette)
            hr = set_palette(frameencode, image->palette);

        if (SUCCEEDED(hr))
        {
            stat = GdipBitmapLockBits(bitmap, &rc, ImageLockModeRead, gdipformat,
                &lockeddata);

            if (stat == Ok)
            {
                UINT row_size = (lockeddata.Width * PIXELFORMATBPP(gdipformat) + 7)/8;
                BYTE *row;

                /* write one row at a time in case stride is negative */
                row = lockeddata.Scan0;
                for (i=0; i<lockeddata.Height; i++)
                {
                    hr = IWICBitmapFrameEncode_WritePixels(frameencode, 1, row_size, row_size, row);
                    if (FAILED(hr)) break;
                    row += lockeddata.Stride;
                }

                GdipBitmapUnlockBits(bitmap, &lockeddata);
            }
            else
                hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
            hr = IWICBitmapFrameEncode_Commit(frameencode);

        IWICBitmapFrameEncode_Release(frameencode);
        IPropertyBag2_Release(encoderoptions);
    }

    return hresult_to_status(hr);
}

static BOOL has_encoder_param_long(GDIPCONST EncoderParameters *params, GUID param_guid, ULONG val)
{
    int param_idx, value_idx;

    if (!params)
        return FALSE;

    for (param_idx = 0; param_idx < params->Count; param_idx++)
    {
        EncoderParameter param = params->Parameter[param_idx];
        if (param.Type == EncoderParameterValueTypeLong && IsEqualCLSID(&param.Guid, &param_guid))
        {
            ULONG *value_array = (ULONG*) param.Value;
            for (value_idx = 0; value_idx < param.NumberOfValues; value_idx++)
            {
                if (value_array[value_idx] == val)
                    return TRUE;
            }
        }
    }
    return FALSE;
}

static GpStatus encode_image_wic(GpImage *image, IStream *stream,
    REFGUID container, GDIPCONST EncoderParameters *params)
{
    GpStatus status, terminate_status;

    if (image->type != ImageTypeBitmap)
        return GenericError;

    status = initialize_encoder_wic(stream, container, image);

    if (status == Ok)
        status = encode_frame_wic(image->encoder, image);

    if (!has_encoder_param_long(params, EncoderSaveFlag, EncoderValueMultiFrame))
    {
        /* always try to terminate, but if something already failed earlier, keep the old status. */
        terminate_status = terminate_encoder_wic(image);
        if (status == Ok)
            status = terminate_status;
    }

    return status;
}

static GpStatus encode_image_BMP(GpImage *image, IStream* stream,
    GDIPCONST EncoderParameters* params)
{
    return encode_image_wic(image, stream, &GUID_ContainerFormatBmp, params);
}

static GpStatus encode_image_tiff(GpImage *image, IStream* stream,
    GDIPCONST EncoderParameters* params)
{
    return encode_image_wic(image, stream, &GUID_ContainerFormatTiff, params);
}

GpStatus encode_image_png(GpImage *image, IStream* stream,
    GDIPCONST EncoderParameters* params)
{
    return encode_image_wic(image, stream, &GUID_ContainerFormatPng, params);
}

static GpStatus encode_image_jpeg(GpImage *image, IStream* stream,
    GDIPCONST EncoderParameters* params)
{
    return encode_image_wic(image, stream, &GUID_ContainerFormatJpeg, params);
}

static GpStatus encode_image_gif(GpImage *image, IStream* stream,
    GDIPCONST EncoderParameters* params)
{
    return encode_image_wic(image, stream, &GUID_ContainerFormatGif, params);
}

/*****************************************************************************
 * GdipSaveImageToStream [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSaveImageToStream(GpImage *image, IStream* stream,
    GDIPCONST CLSID* clsid, GDIPCONST EncoderParameters* params)
{
    GpStatus stat;
    encode_image_func encode_image;
    int i;

    TRACE("%p, %p, %s, %p\n", image, stream, wine_dbgstr_guid(clsid), params);

    if(!image || !stream)
        return InvalidParameter;

    /* select correct encoder */
    encode_image = NULL;
    for (i = 0; i < NUM_CODECS; i++) {
        if ((codecs[i].info.Flags & ImageCodecFlagsEncoder) &&
            IsEqualCLSID(clsid, &codecs[i].info.Clsid))
            encode_image = codecs[i].encode_func;
    }
    if (encode_image == NULL)
        return UnknownImageFormat;

    stat = encode_image(image, stream, params);

    return stat;
}

/*****************************************************************************
 * GdipSaveAdd [GDIPLUS.@]
 *
 * Like GdipSaveAddImage(), but encode the currently active frame of the given image into the file
 * or stream that is currently being encoded.
 */
GpStatus WINGDIPAPI GdipSaveAdd(GpImage *image, GDIPCONST EncoderParameters *params)
{
    return GdipSaveAddImage(image, image, params);
}

/*****************************************************************************
 * GdipSaveAddImage [GDIPLUS.@]
 *
 * Encode the currently active frame of additional_image into the file or stream that is currently
 * being encoded by the image given in the image parameter. The first frame of a multi-frame image
 * must be encoded using the normal GdipSaveImageToStream() or GdipSaveImageToFile() functions,
 * but with the "MultiFrame" encoding parameter set. The multi-frame encoding process must be
 * finished after adding the last frame by calling GdipSaveAdd() with the "Flush" encoding parameter
 * set.
 */
GpStatus WINGDIPAPI GdipSaveAddImage(GpImage *image, GpImage *additional_image,
    GDIPCONST EncoderParameters *params)
{
    TRACE("%p, %p, %p\n", image, additional_image, params);

    if (!image || !additional_image || !params)
        return InvalidParameter;

    if (!image->encoder)
        return Win32Error;

    if (has_encoder_param_long(params, EncoderSaveFlag, EncoderValueFlush))
        return terminate_encoder_wic(image);
    else if (has_encoder_param_long(params, EncoderSaveFlag, EncoderValueFrameDimensionPage))
        return encode_frame_wic(image->encoder, additional_image);
    else
        return InvalidParameter;
}

/*****************************************************************************
 * GdipGetImagePalette [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImagePalette(GpImage *image, ColorPalette *palette, INT size)
{
    INT count;

    TRACE("(%p,%p,%i)\n", image, palette, size);

    if (!image || !palette)
        return InvalidParameter;

    count = image->palette ? image->palette->Count : 0;

    if (size < (sizeof(UINT)*2+sizeof(ARGB)*count))
    {
        TRACE("<-- InsufficientBuffer\n");
        return InsufficientBuffer;
    }

    if (image->palette)
    {
        palette->Flags = image->palette->Flags;
        palette->Count = image->palette->Count;
        memcpy(palette->Entries, image->palette->Entries, sizeof(ARGB)*image->palette->Count);
    }
    else
    {
        palette->Flags = 0;
        palette->Count = 0;
    }
    return Ok;
}

/*****************************************************************************
 * GdipSetImagePalette [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSetImagePalette(GpImage *image,
    GDIPCONST ColorPalette *palette)
{
    ColorPalette *new_palette;

    TRACE("(%p,%p)\n", image, palette);

    if(!image || !palette || palette->Count > 256)
        return InvalidParameter;

    new_palette = calloc(1, 2 * sizeof(UINT) + palette->Count * sizeof(ARGB));
    if (!new_palette) return OutOfMemory;

    free(image->palette);
    image->palette = new_palette;
    image->palette->Flags = palette->Flags;
    image->palette->Count = palette->Count;
    memcpy(image->palette->Entries, palette->Entries, sizeof(ARGB)*palette->Count);

    return Ok;
}

/*************************************************************************
 * Encoders -
 *   Structures that represent which formats we support for encoding.
 */

/* ImageCodecInfo creation routines taken from libgdiplus */
static const WCHAR bmp_codecname[] = L"Built-in BMP";
static const WCHAR bmp_extension[] = L"*.BMP;*.DIB;*.RLE";
static const WCHAR bmp_mimetype[] = L"image/bmp";
static const WCHAR bmp_format[] = L"BMP";
static const BYTE bmp_sig_pattern[] = { 0x42, 0x4D };
static const BYTE bmp_sig_mask[] = { 0xFF, 0xFF };

static const WCHAR jpeg_codecname[] = L"Built-in JPEG";
static const WCHAR jpeg_extension[] = L"*.JPG;*.JPEG;*.JPE;*.JFIF";
static const WCHAR jpeg_mimetype[] = L"image/jpeg";
static const WCHAR jpeg_format[] = L"JPEG";
static const BYTE jpeg_sig_pattern[] = { 0xFF, 0xD8 };
static const BYTE jpeg_sig_mask[] = { 0xFF, 0xFF };

static const WCHAR gif_codecname[] = L"Built-in GIF";
static const WCHAR gif_extension[] = L"*.GIF";
static const WCHAR gif_mimetype[] = L"image/gif";
static const WCHAR gif_format[] = L"GIF";
static const BYTE gif_sig_pattern[12] = "GIF87aGIF89a";
static const BYTE gif_sig_mask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static const WCHAR tiff_codecname[] = L"Built-in TIFF";
static const WCHAR tiff_extension[] = L"*.TIFF;*.TIF";
static const WCHAR tiff_mimetype[] = L"image/tiff";
static const WCHAR tiff_format[] = L"TIFF";
static const BYTE tiff_sig_pattern[] = {0x49,0x49,42,0,0x4d,0x4d,0,42};
static const BYTE tiff_sig_mask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static const WCHAR emf_codecname[] = L"Built-in EMF";
static const WCHAR emf_extension[] = L"*.EMF";
static const WCHAR emf_mimetype[] = L"image/x-emf";
static const WCHAR emf_format[] = L"EMF";
static const BYTE emf_sig_pattern[] = { 0x01, 0x00, 0x00, 0x00 };
static const BYTE emf_sig_mask[] = { 0xFF, 0xFF, 0xFF, 0xFF };

static const WCHAR wmf_codecname[] = L"Built-in WMF";
static const WCHAR wmf_extension[] = L"*.WMF";
static const WCHAR wmf_mimetype[] = L"image/x-wmf";
static const WCHAR wmf_format[] = L"WMF";
static const BYTE wmf_sig_pattern[] = { 0xd7, 0xcd };
static const BYTE wmf_sig_mask[] = { 0xFF, 0xFF };

static const WCHAR png_codecname[] = L"Built-in PNG";
static const WCHAR png_extension[] = L"*.PNG";
static const WCHAR png_mimetype[] = L"image/png";
static const WCHAR png_format[] = L"PNG";
static const BYTE png_sig_pattern[] = { 137, 80, 78, 71, 13, 10, 26, 10, };
static const BYTE png_sig_mask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static const WCHAR ico_codecname[] = L"Built-in ICO";
static const WCHAR ico_extension[] = L"*.ICO";
static const WCHAR ico_mimetype[] = L"image/x-icon";
static const WCHAR ico_format[] = L"ICO";
static const BYTE ico_sig_pattern[] = { 0x00, 0x00, 0x01, 0x00 };
static const BYTE ico_sig_mask[] = { 0xFF, 0xFF, 0xFF, 0xFF };

static const struct image_codec codecs[NUM_CODECS] = {
    {
        { /* BMP */
            /* Clsid */              { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cabU, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          bmp_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  bmp_format,
            /* FilenameExtension */  bmp_extension,
            /* MimeType */           bmp_mimetype,
            /* Flags */              ImageCodecFlagsEncoder | ImageCodecFlagsDecoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            2,
            /* SigPattern */         bmp_sig_pattern,
            /* SigMask */            bmp_sig_mask,
        },
        encode_image_BMP,
        decode_image_bmp,
        select_frame_wic
    },
    {
        { /* JPEG */
            /* Clsid */              { 0x557cf401, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3caeU, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          jpeg_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  jpeg_format,
            /* FilenameExtension */  jpeg_extension,
            /* MimeType */           jpeg_mimetype,
            /* Flags */              ImageCodecFlagsEncoder | ImageCodecFlagsDecoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            2,
            /* SigPattern */         jpeg_sig_pattern,
            /* SigMask */            jpeg_sig_mask,
        },
        encode_image_jpeg,
        decode_image_jpeg,
        select_frame_wic
    },
    {
        { /* GIF */
            /* Clsid */              { 0x557cf402, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cb0U, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          gif_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  gif_format,
            /* FilenameExtension */  gif_extension,
            /* MimeType */           gif_mimetype,
            /* Flags */              ImageCodecFlagsDecoder | ImageCodecFlagsEncoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           2,
            /* SigSize */            6,
            /* SigPattern */         gif_sig_pattern,
            /* SigMask */            gif_sig_mask,
        },
        encode_image_gif,
        decode_image_gif,
        select_frame_gif
    },
    {
        { /* TIFF */
            /* Clsid */              { 0x557cf405, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cb1U, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          tiff_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  tiff_format,
            /* FilenameExtension */  tiff_extension,
            /* MimeType */           tiff_mimetype,
            /* Flags */              ImageCodecFlagsDecoder | ImageCodecFlagsEncoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           2,
            /* SigSize */            4,
            /* SigPattern */         tiff_sig_pattern,
            /* SigMask */            tiff_sig_mask,
        },
        encode_image_tiff,
        decode_image_tiff,
        select_frame_wic
    },
    {
        { /* EMF */
            /* Clsid */              { 0x557cf403, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cacU, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          emf_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  emf_format,
            /* FilenameExtension */  emf_extension,
            /* MimeType */           emf_mimetype,
            /* Flags */              ImageCodecFlagsDecoder | ImageCodecFlagsSupportVector | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            4,
            /* SigPattern */         emf_sig_pattern,
            /* SigMask */            emf_sig_mask,
        },
        NULL,
        decode_image_emf,
        NULL
    },
    {
        { /* WMF */
            /* Clsid */              { 0x557cf404, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cadU, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          wmf_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  wmf_format,
            /* FilenameExtension */  wmf_extension,
            /* MimeType */           wmf_mimetype,
            /* Flags */              ImageCodecFlagsDecoder | ImageCodecFlagsSupportVector | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            2,
            /* SigPattern */         wmf_sig_pattern,
            /* SigMask */            wmf_sig_mask,
        },
        NULL,
        decode_image_wmf,
        NULL
    },
    {
        { /* PNG */
            /* Clsid */              { 0x557cf406, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cafU, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          png_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  png_format,
            /* FilenameExtension */  png_extension,
            /* MimeType */           png_mimetype,
            /* Flags */              ImageCodecFlagsEncoder | ImageCodecFlagsDecoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            8,
            /* SigPattern */         png_sig_pattern,
            /* SigMask */            png_sig_mask,
        },
        encode_image_png,
        decode_image_png,
        select_frame_wic
    },
    {
        { /* ICO */
            /* Clsid */              { 0x557cf407, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cb5U, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          ico_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  ico_format,
            /* FilenameExtension */  ico_extension,
            /* MimeType */           ico_mimetype,
            /* Flags */              ImageCodecFlagsDecoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            4,
            /* SigPattern */         ico_sig_pattern,
            /* SigMask */            ico_sig_mask,
        },
        NULL,
        decode_image_icon,
        select_frame_wic
    },
};

/*****************************************************************************
 * GdipGetImageDecodersSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageDecodersSize(UINT *numDecoders, UINT *size)
{
    int decoder_count=0;
    int i;
    TRACE("%p %p\n", numDecoders, size);

    if (!numDecoders || !size)
        return InvalidParameter;

    for (i=0; i<NUM_CODECS; i++)
    {
        if (codecs[i].info.Flags & ImageCodecFlagsDecoder)
            decoder_count++;
    }

    *numDecoders = decoder_count;
    *size = decoder_count * sizeof(ImageCodecInfo);

    return Ok;
}

/*****************************************************************************
 * GdipGetImageDecoders [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageDecoders(UINT numDecoders, UINT size, ImageCodecInfo *decoders)
{
    int i, decoder_count=0;
    TRACE("%u %u %p\n", numDecoders, size, decoders);

    if (!decoders ||
        size != numDecoders * sizeof(ImageCodecInfo))
        return GenericError;

    for (i=0; i<NUM_CODECS; i++)
    {
        if (codecs[i].info.Flags & ImageCodecFlagsDecoder)
        {
            if (decoder_count == numDecoders) return GenericError;
            memcpy(&decoders[decoder_count], &codecs[i].info, sizeof(ImageCodecInfo));
            decoder_count++;
        }
    }

    if (decoder_count < numDecoders) return GenericError;

    return Ok;
}

/*****************************************************************************
 * GdipGetImageEncodersSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageEncodersSize(UINT *numEncoders, UINT *size)
{
    int encoder_count=0;
    int i;
    TRACE("%p %p\n", numEncoders, size);

    if (!numEncoders || !size)
        return InvalidParameter;

    for (i=0; i<NUM_CODECS; i++)
    {
        if (codecs[i].info.Flags & ImageCodecFlagsEncoder)
            encoder_count++;
    }

    *numEncoders = encoder_count;
    *size = encoder_count * sizeof(ImageCodecInfo);

    return Ok;
}

/*****************************************************************************
 * GdipGetImageEncoders [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageEncoders(UINT numEncoders, UINT size, ImageCodecInfo *encoders)
{
    int i, encoder_count=0;
    TRACE("%u %u %p\n", numEncoders, size, encoders);

    if (!encoders ||
        size != numEncoders * sizeof(ImageCodecInfo))
        return GenericError;

    for (i=0; i<NUM_CODECS; i++)
    {
        if (codecs[i].info.Flags & ImageCodecFlagsEncoder)
        {
            if (encoder_count == numEncoders) return GenericError;
            memcpy(&encoders[encoder_count], &codecs[i].info, sizeof(ImageCodecInfo));
            encoder_count++;
        }
    }

    if (encoder_count < numEncoders) return GenericError;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetEncoderParameterListSize(GpImage *image,
    GDIPCONST CLSID* clsidEncoder, UINT *size)
{
    static int calls;

    TRACE("(%p,%s,%p)\n", image, debugstr_guid(clsidEncoder), size);

    if(!(calls++))
        FIXME("not implemented\n");

    *size = 0;

    return NotImplemented;
}

static PixelFormat get_16bpp_format(HBITMAP hbm)
{
    BITMAPV4HEADER bmh;
    HDC hdc;
    PixelFormat result;

    hdc = CreateCompatibleDC(NULL);

    memset(&bmh, 0, sizeof(bmh));
    bmh.bV4Size = sizeof(bmh);
    bmh.bV4Width = 1;
    bmh.bV4Height = 1;
    bmh.bV4V4Compression = BI_BITFIELDS;
    bmh.bV4BitCount = 16;

    GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO*)&bmh, DIB_RGB_COLORS);

    if (bmh.bV4RedMask == 0x7c00 &&
        bmh.bV4GreenMask == 0x3e0 &&
        bmh.bV4BlueMask == 0x1f)
    {
        result = PixelFormat16bppRGB555;
    }
    else if (bmh.bV4RedMask == 0xf800 &&
        bmh.bV4GreenMask == 0x7e0 &&
        bmh.bV4BlueMask == 0x1f)
    {
        result = PixelFormat16bppRGB565;
    }
    else
    {
        FIXME("unrecognized bitfields %lx,%lx,%lx\n", bmh.bV4RedMask,
            bmh.bV4GreenMask, bmh.bV4BlueMask);
        result = PixelFormatUndefined;
    }

    DeleteDC(hdc);

    return result;
}

/*****************************************************************************
 * GdipCreateBitmapFromHBITMAP [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateBitmapFromHBITMAP(HBITMAP hbm, HPALETTE hpal, GpBitmap** bitmap)
{
    BITMAP bm;
    GpStatus retval;
    PixelFormat format;
    BitmapData lockeddata;
    char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *pbmi = (BITMAPINFO *)bmibuf;

    TRACE("%p %p %p\n", hbm, hpal, bitmap);

    if(!hbm || !bitmap)
        return InvalidParameter;

    if (GetObjectA(hbm, sizeof(bm), &bm) != sizeof(bm))
            return InvalidParameter;

    /* TODO: Figure out the correct format for 16, 32, 64 bpp */
    switch(bm.bmBitsPixel) {
        case 1:
            format = PixelFormat1bppIndexed;
            break;
        case 4:
            format = PixelFormat4bppIndexed;
            break;
        case 8:
            format = PixelFormat8bppIndexed;
            break;
        case 16:
            format = get_16bpp_format(hbm);
            if (format == PixelFormatUndefined)
                return InvalidParameter;
            break;
        case 24:
            format = PixelFormat24bppRGB;
            break;
        case 32:
            format = PixelFormat32bppRGB;
            break;
        case 48:
            format = PixelFormat48bppRGB;
            break;
        default:
            FIXME("don't know how to handle %d bpp\n", bm.bmBitsPixel);
            return InvalidParameter;
    }

    retval = GdipCreateBitmapFromScan0(bm.bmWidth, bm.bmHeight, 0,
        format, NULL, bitmap);

    if (retval == Ok)
    {
        retval = GdipBitmapLockBits(*bitmap, NULL, ImageLockModeWrite,
            format, &lockeddata);
        if (retval == Ok)
        {
            HDC hdc;
            INT src_height;

            hdc = CreateCompatibleDC(NULL);

            pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            pbmi->bmiHeader.biBitCount = 0;

            GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

            src_height = abs(pbmi->bmiHeader.biHeight);
            pbmi->bmiHeader.biHeight = -src_height;

            GetDIBits(hdc, hbm, 0, src_height, lockeddata.Scan0, pbmi, DIB_RGB_COLORS);

            DeleteDC(hdc);

            GdipBitmapUnlockBits(*bitmap, &lockeddata);
        }

        /* According to the tests hpal is ignored */
        if (retval == Ok && pbmi->bmiHeader.biBitCount <= 8)
        {
            ColorPalette *palette;
            int i, num_palette_entries;

            num_palette_entries = pbmi->bmiHeader.biClrUsed;
            if (!num_palette_entries)
                num_palette_entries = 1 << pbmi->bmiHeader.biBitCount;

            palette = calloc(1, sizeof(ColorPalette) + sizeof(ARGB) * (num_palette_entries - 1));
            if (!palette)
                retval = OutOfMemory;
            else
            {
                palette->Flags = 0;
                palette->Count = num_palette_entries;

                for (i=0; i<num_palette_entries; i++)
                {
                    palette->Entries[i] = 0xff000000 | pbmi->bmiColors[i].rgbRed << 16 |
                                          pbmi->bmiColors[i].rgbGreen << 8 | pbmi->bmiColors[i].rgbBlue;
                }

                retval = GdipSetImagePalette(&(*bitmap)->image, palette);
            }

            free(palette);
        }

        if (retval != Ok)
        {
            GdipDisposeImage(&(*bitmap)->image);
            *bitmap = NULL;
        }
    }

    return retval;
}

static UINT get_effect_parameter_size(EffectType type)
{
    switch (type)
    {
    case BlurEffect:
        return sizeof(struct BlurParams);
    case SharpenEffect:
        return sizeof(struct SharpenParams);
    case TintEffect:
        return sizeof(struct TintParams);
    case RedEyeCorrectionEffect:
        return sizeof(struct RedEyeCorrectionParams);
    case ColorMatrixEffect:
        return sizeof(ColorMatrix);
    case ColorLUTEffect:
        return sizeof(struct ColorLUTParams);
    case BrightnessContrastEffect:
        return sizeof(struct BrightnessContrastParams);
    case HueSaturationLightnessEffect:
        return sizeof(struct HueSaturationLightnessParams);
    case ColorBalanceEffect:
        return sizeof(struct ColorBalanceParams);
    case LevelsEffect:
        return sizeof(struct LevelsParams);
    case ColorCurveEffect:
        return sizeof(struct ColorCurveParams);
    default:
        return 0;
    }
}

/*****************************************************************************
 * GdipCreateEffect [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateEffect(const GUID guid, CGpEffect **effect)
{
    CGpEffect *ef = NULL;
    EffectType type;
    UINT param_size;

    TRACE("(%s, %p)\n", debugstr_guid(&guid), effect);

    if(!effect)
        return InvalidParameter;

    if (IsEqualGUID(&guid, &BlurEffectGuid))
    {
        type = BlurEffect;
    }
    else if (IsEqualGUID(&guid, &SharpenEffectGuid))
    {
        type = SharpenEffect;
    }
    else if (IsEqualGUID(&guid, &TintEffectGuid))
    {
        type = TintEffect;
    }
    else if (IsEqualGUID(&guid, &RedEyeCorrectionEffectGuid))
    {
        type = RedEyeCorrectionEffect;
    }
    else if (IsEqualGUID(&guid, &ColorMatrixEffectGuid))
    {
        type = ColorMatrixEffect;
    }
    else if (IsEqualGUID(&guid, &ColorLUTEffectGuid))
    {
        type = ColorLUTEffect;
    }
    else if (IsEqualGUID(&guid, &BrightnessContrastEffectGuid))
    {
        type = BrightnessContrastEffect;
    }
    else if (IsEqualGUID(&guid, &HueSaturationLightnessEffectGuid))
    {
        type = HueSaturationLightnessEffect;
    }
    else if (IsEqualGUID(&guid, &ColorBalanceEffectGuid))
    {
        type = ColorBalanceEffect;
    }
    else if (IsEqualGUID(&guid, &LevelsEffectGuid))
    {
        type = LevelsEffect;
    }
    else if (IsEqualGUID(&guid, &ColorCurveEffectGuid))
    {
        type = ColorCurveEffect;
    }
    else
    {
        *effect = NULL;
        return Win32Error;
    }

    param_size = get_effect_parameter_size(type);

    ef = calloc(1, FIELD_OFFSET(CGpEffect, params.data[param_size]));
    ef->type = type;
    *effect = ef;

    return Ok;
}

/*****************************************************************************
 * GdipDeleteEffect [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipDeleteEffect(CGpEffect *effect)
{
    TRACE("(%p)\n", effect);

    if (!effect)
        return InvalidParameter;

    free(effect);
    return Ok;
}

/*****************************************************************************
 * GdipGetEffectParameterSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetEffectParameterSize(CGpEffect *effect, UINT *size)
{
    UINT sz = 0;
    GpStatus status = Ok;

    TRACE("(%p,%p)\n", effect, size);

    if (!effect || !size)
        return InvalidParameter;

    sz = get_effect_parameter_size(effect->type);
    if (!sz)
        status = InvalidParameter;

    *size = sz;
    return status;
}

/*****************************************************************************
 * GdipGetEffectParameters [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetEffectParameters(CGpEffect *effect, UINT *size, void *params)
{
    UINT params_size;

    TRACE("(%p,%p,%p)\n", effect, size, params);

    if (!effect || !size || !params)
        return InvalidParameter;

    if (effect->type == RedEyeCorrectionEffect)
    {
        static int calls;
        if (!calls++)
            FIXME("not implemented for RedEyeCorrectionEffect\n");
        return NotImplemented;
    }

    params_size = get_effect_parameter_size(effect->type);

    if (*size < params_size)
        return InvalidParameter;

    *size = params_size;
    memcpy(params, effect->params.data, params_size);

    return Ok;
}

/*****************************************************************************
 * GdipSetEffectParameters [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSetEffectParameters(CGpEffect *effect,
    const VOID *params, const UINT size)
{
    UINT params_size;

    TRACE("(%p,%p,%u)\n", effect, params, size);

    if (!effect || !params)
        return InvalidParameter;

    if (effect->type == RedEyeCorrectionEffect)
    {
        static int calls;
        if (!calls++)
            FIXME("not implemented for RedEyeCorrectionEffect\n");
        return NotImplemented;
    }

    params_size = get_effect_parameter_size(effect->type);

    if (size != params_size)
        return InvalidParameter;

    memcpy(effect->params.data, params, size);

    return Ok;
}

/*****************************************************************************
 * GdipGetImageFlags [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageFlags(GpImage *image, UINT *flags)
{
    TRACE("%p %p\n", image, flags);

    if(!image || !flags)
        return InvalidParameter;

    *flags = image->flags;

    return Ok;
}

GpStatus WINGDIPAPI GdipTestControl(GpTestControlEnum control, void *param)
{
    TRACE("(%d, %p)\n", control, param);

    switch(control){
        case TestControlForceBilinear:
            if(param)
                FIXME("TestControlForceBilinear not handled\n");
            break;
        case TestControlNoICM:
            if(param)
                FIXME("TestControlNoICM not handled\n");
            break;
        case TestControlGetBuildNumber:
            *((DWORD*)param) = 3102;
            break;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipImageForceValidation(GpImage *image)
{
    TRACE("%p\n", image);

    return Ok;
}

/*****************************************************************************
 * GdipGetImageThumbnail [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageThumbnail(GpImage *image, UINT width, UINT height,
                            GpImage **ret_image, GetThumbnailImageAbort cb,
                            VOID * cb_data)
{
    GpStatus stat;
    GpGraphics *graphics;
    UINT srcwidth, srcheight;

    TRACE("(%p %u %u %p %p %p)\n",
        image, width, height, ret_image, cb, cb_data);

    if (!image || !ret_image)
        return InvalidParameter;

    if (!width) width = 120;
    if (!height) height = 120;

    GdipGetImageWidth(image, &srcwidth);
    GdipGetImageHeight(image, &srcheight);

    stat = GdipCreateBitmapFromScan0(width, height, 0, PixelFormat32bppPARGB,
        NULL, (GpBitmap**)ret_image);

    if (stat == Ok)
    {
        stat = GdipGetImageGraphicsContext(*ret_image, &graphics);

        if (stat == Ok)
        {
            stat = GdipDrawImageRectRectI(graphics, image,
                0, 0, width, height, 0, 0, srcwidth, srcheight, UnitPixel,
                NULL, NULL, NULL);

            GdipDeleteGraphics(graphics);
        }

        if (stat != Ok)
        {
            GdipDisposeImage(*ret_image);
            *ret_image = NULL;
        }
    }

    return stat;
}

/*****************************************************************************
 * GdipImageRotateFlip [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipImageRotateFlip(GpImage *image, RotateFlipType type)
{
    GpBitmap *new_bitmap;
    GpBitmap *bitmap;
    int bpp, bytesperpixel;
    BOOL rotate_90, flip_x, flip_y;
    int src_x_offset, src_y_offset;
    LPBYTE src_origin;
    UINT x, y, width, height;
    BitmapData dst_lock;
    GpStatus stat;

    TRACE("(%p, %u)\n", image, type);

    if (!image)
        return InvalidParameter;
    if (!image_lock(image))
        return ObjectBusy;

    rotate_90 = type&1;
    flip_x = (type&6) == 2 || (type&6) == 4;
    flip_y = (type&3) == 1 || (type&3) == 2;

    if (image->type != ImageTypeBitmap)
    {
        FIXME("Not implemented for type %i\n", image->type);
        image_unlock(image);
        return NotImplemented;
    }

    bitmap = (GpBitmap*)image;
    bpp = PIXELFORMATBPP(bitmap->format);

    if (bpp < 8)
    {
        FIXME("Not implemented for %i bit images\n", bpp);
        image_unlock(image);
        return NotImplemented;
    }

    if (rotate_90)
    {
        width = bitmap->height;
        height = bitmap->width;
    }
    else
    {
        width = bitmap->width;
        height = bitmap->height;
    }

    bytesperpixel = bpp/8;

    stat = GdipCreateBitmapFromScan0(width, height, 0, bitmap->format, NULL, &new_bitmap);

    if (stat == Ok)
    {
        stat = GdipBitmapLockBits(new_bitmap, NULL, ImageLockModeWrite, bitmap->format, &dst_lock);

        if (stat == Ok)
        {
            LPBYTE src_row, src_pixel;
            LPBYTE dst_row, dst_pixel;

            src_origin = bitmap->bits;
            if (flip_x) src_origin += bytesperpixel * (bitmap->width - 1);
            if (flip_y) src_origin += bitmap->stride * (bitmap->height - 1);

            if (rotate_90)
            {
                if (flip_y) src_x_offset = -bitmap->stride;
                else src_x_offset = bitmap->stride;
                if (flip_x) src_y_offset = -bytesperpixel;
                else src_y_offset = bytesperpixel;
            }
            else
            {
                if (flip_x) src_x_offset = -bytesperpixel;
                else src_x_offset = bytesperpixel;
                if (flip_y) src_y_offset = -bitmap->stride;
                else src_y_offset = bitmap->stride;
            }

            src_row = src_origin;
            dst_row = dst_lock.Scan0;
            for (y=0; y<height; y++)
            {
                src_pixel = src_row;
                dst_pixel = dst_row;
                for (x=0; x<width; x++)
                {
                    /* FIXME: This could probably be faster without memcpy. */
                    memcpy(dst_pixel, src_pixel, bytesperpixel);
                    dst_pixel += bytesperpixel;
                    src_pixel += src_x_offset;
                }
                src_row += src_y_offset;
                dst_row += dst_lock.Stride;
            }

            GdipBitmapUnlockBits(new_bitmap, &dst_lock);
            move_bitmap(bitmap, new_bitmap, FALSE);
        }
        else GdipDisposeImage(&new_bitmap->image);
    }

    image_unlock(image);
    return stat;
}

/*****************************************************************************
 * GdipImageSetAbort [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipImageSetAbort(GpImage *image, GdiplusAbort *pabort)
{
    TRACE("(%p, %p)\n", image, pabort);

    if (!image)
        return InvalidParameter;

    if (pabort)
        FIXME("Abort callback is not supported.\n");

    return Ok;
}

/*****************************************************************************
 * GdipBitmapConvertFormat [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipBitmapConvertFormat(GpBitmap *bitmap, PixelFormat format, DitherType dithertype,
    PaletteType palettetype, ColorPalette *palette, REAL alphathreshold)
{
    FIXME("(%p, 0x%08x, %d, %d, %p, %f): stub\n", bitmap, format, dithertype, palettetype, palette, alphathreshold);
    return NotImplemented;
}

static void set_histogram_point_argb(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[ color >> 24        ]++;
    ch1[(color >> 16) & 0xff]++;
    ch2[(color >>  8) & 0xff]++;
    ch3[ color        & 0xff]++;
}

static void set_histogram_point_pargb(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    BYTE alpha = color >> 24;

    ch0[alpha]++;
    ch1[(((color >> 16) & 0xff) * alpha) / 0xff]++;
    ch2[(((color >>  8) & 0xff) * alpha) / 0xff]++;
    ch3[(( color        & 0xff) * alpha) / 0xff]++;
}

static void set_histogram_point_rgb(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[(color >> 16) & 0xff]++;
    ch1[(color >>  8) & 0xff]++;
    ch2[ color        & 0xff]++;
}

static void set_histogram_point_gray(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[(76 * ((color >> 16) & 0xff) + 150 * ((color >> 8) & 0xff) + 29 * (color & 0xff)) / 0xff]++;
}

static void set_histogram_point_b(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[color & 0xff]++;
}

static void set_histogram_point_g(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[(color >> 8) & 0xff]++;
}

static void set_histogram_point_r(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[(color >> 16) & 0xff]++;
}

static void set_histogram_point_a(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    ch0[(color >> 24) & 0xff]++;
}

/*****************************************************************************
 * GdipBitmapGetHistogram [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipBitmapGetHistogram(GpBitmap *bitmap, HistogramFormat format, UINT num_of_entries,
    UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3)
{
    static void (* const set_histogram_point[])(ARGB color, UINT *ch0, UINT *ch1, UINT *ch2, UINT *ch3) =
    {
        set_histogram_point_argb,
        set_histogram_point_pargb,
        set_histogram_point_rgb,
        set_histogram_point_gray,
        set_histogram_point_b,
        set_histogram_point_g,
        set_histogram_point_r,
        set_histogram_point_a,
    };
    UINT width, height, x, y;

    TRACE("(%p, %d, %u, %p, %p, %p, %p)\n", bitmap, format, num_of_entries,
        ch0, ch1, ch2, ch3);

    if (!bitmap || num_of_entries != 256)
        return InvalidParameter;

    /* Make sure passed channel pointers match requested format */
    switch (format)
    {
    case HistogramFormatARGB:
    case HistogramFormatPARGB:
        if (!ch0 || !ch1 || !ch2 || !ch3)
            return InvalidParameter;
        memset(ch0, 0, num_of_entries * sizeof(UINT));
        memset(ch1, 0, num_of_entries * sizeof(UINT));
        memset(ch2, 0, num_of_entries * sizeof(UINT));
        memset(ch3, 0, num_of_entries * sizeof(UINT));
        break;
    case HistogramFormatRGB:
        if (!ch0 || !ch1 || !ch2 || ch3)
            return InvalidParameter;
        memset(ch0, 0, num_of_entries * sizeof(UINT));
        memset(ch1, 0, num_of_entries * sizeof(UINT));
        memset(ch2, 0, num_of_entries * sizeof(UINT));
        break;
    case HistogramFormatGray:
    case HistogramFormatB:
    case HistogramFormatG:
    case HistogramFormatR:
    case HistogramFormatA:
        if (!ch0 || ch1 || ch2 || ch3)
            return InvalidParameter;
        memset(ch0, 0, num_of_entries * sizeof(UINT));
        break;
    default:
        WARN("Invalid histogram format requested, %d\n", format);
        return InvalidParameter;
    }

    GdipGetImageWidth(&bitmap->image, &width);
    GdipGetImageHeight(&bitmap->image, &height);

    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
        {
            ARGB color;

            GdipBitmapGetPixel(bitmap, x, y, &color);
            set_histogram_point[format](color, ch0, ch1, ch2, ch3);
        }

    return Ok;
}

/*****************************************************************************
 * GdipBitmapGetHistogramSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipBitmapGetHistogramSize(HistogramFormat format, UINT *num_of_entries)
{
    TRACE("(%d, %p)\n", format, num_of_entries);

    if (!num_of_entries)
        return InvalidParameter;

    *num_of_entries = 256;
    return Ok;
}

static GpStatus create_optimal_palette(ColorPalette *palette, INT desired,
    BOOL transparent, GpBitmap *bitmap)
{
    GpStatus status;
    BitmapData data;
    HRESULT hr;
    IWICImagingFactory *factory;
    IWICPalette *wic_palette;

    if (!bitmap) return InvalidParameter;
    if (palette->Count < desired) return GenericError;

    status = GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead, PixelFormat24bppRGB, &data);
    if (status != Ok) return status;

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (hr != S_OK)
    {
        GdipBitmapUnlockBits(bitmap, &data);
        return hresult_to_status(hr);
    }

    hr = IWICImagingFactory_CreatePalette(factory, &wic_palette);
    if (hr == S_OK)
    {
        IWICBitmap *bitmap;

        /* PixelFormat24bppRGB actually stores the bitmap bits as BGR. */
        hr = IWICImagingFactory_CreateBitmapFromMemory(factory, data.Width, data.Height,
                &GUID_WICPixelFormat24bppBGR, data.Stride, data.Stride * data.Width, data.Scan0, &bitmap);
        if (hr == S_OK)
        {
            hr = IWICPalette_InitializeFromBitmap(wic_palette, (IWICBitmapSource *)bitmap, desired, transparent);
            if (hr == S_OK)
            {
                palette->Flags = 0;
                IWICPalette_GetColorCount(wic_palette, &palette->Count);
                IWICPalette_GetColors(wic_palette, palette->Count, (UINT *)palette->Entries, &palette->Count);
            }

            IWICBitmap_Release(bitmap);
        }

        IWICPalette_Release(wic_palette);
    }

    IWICImagingFactory_Release(factory);
    GdipBitmapUnlockBits(bitmap, &data);

    return hresult_to_status(hr);
}

/*****************************************************************************
 * GdipInitializePalette [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipInitializePalette(ColorPalette *palette,
    PaletteType type, INT desired, BOOL transparent, GpBitmap *bitmap)
{
    TRACE("(%p,%d,%d,%d,%p)\n", palette, type, desired, transparent, bitmap);

    if (!palette) return InvalidParameter;

    switch (type)
    {
    case PaletteTypeCustom:
        return Ok;

    case PaletteTypeOptimal:
        return create_optimal_palette(palette, desired, transparent, bitmap);

    /* WIC palette type enumeration matches these gdiplus enums */
    case PaletteTypeFixedBW:
    case PaletteTypeFixedHalftone8:
    case PaletteTypeFixedHalftone27:
    case PaletteTypeFixedHalftone64:
    case PaletteTypeFixedHalftone125:
    case PaletteTypeFixedHalftone216:
    case PaletteTypeFixedHalftone252:
    case PaletteTypeFixedHalftone256:
    {
        ColorPalette *wic_palette;
        GpStatus status = Ok;

        wic_palette = get_palette(NULL, (WICBitmapPaletteType)type);
        if (!wic_palette) return OutOfMemory;

        if (palette->Count >= wic_palette->Count)
        {
            palette->Flags = wic_palette->Flags;
            palette->Count = wic_palette->Count;
            memcpy(palette->Entries, wic_palette->Entries, wic_palette->Count * sizeof(wic_palette->Entries[0]));
        }
        else
            status = GenericError;

        free(wic_palette);

        return status;
    }

    default:
        FIXME("unknown palette type %d\n", type);
        break;
    }

    return InvalidParameter;
}

/*
 * GDI device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
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

/*
  Important information:
  
  * Current Windows versions support two different DIB structures:

    - BITMAPCOREINFO / BITMAPCOREHEADER (legacy structures; used in OS/2)
    - BITMAPINFO / BITMAPINFOHEADER
  
    Most Windows API functions taking a BITMAPINFO* / BITMAPINFOHEADER* also
    accept the old "core" structures, and so must WINE.
    You can distinguish them by looking at the first member (bcSize/biSize).

    
  * The palettes are stored in different formats:

    - BITMAPCOREINFO: Array of RGBTRIPLE
    - BITMAPINFO:     Array of RGBQUAD

    
  * There are even more DIB headers, but they all extend BITMAPINFOHEADER:
    
    - BITMAPV4HEADER: Introduced in Windows 95 / NT 4.0
    - BITMAPV5HEADER: Introduced in Windows 98 / 2000
    
    If biCompression is BI_BITFIELDS, the color masks are at the same position
    in all the headers (they start at bmiColors of BITMAPINFOHEADER), because
    the new headers have structure members for the masks.


  * You should never access the color table using the bmiColors member,
    because the passed structure may have one of the extended headers
    mentioned above. Use this to calculate the location:
    
    BITMAPINFO* info;
    void* colorPtr = (LPBYTE) info + (WORD) info->bmiHeader.biSize;

    
  * More information:
    Search for "Bitmap Structures" in MSDN
*/

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"

#include "ntgdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


static INT DIB_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL DIB_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs dib_funcs =
{
    DIB_GetObject,     /* pGetObjectW */
    NULL,              /* pUnrealizeObject */
    DIB_DeleteObject   /* pDeleteObject */
};

/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
static int bitmap_info_size( const BITMAPINFO *info, WORD coloruse )
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        if (info->bmiHeader.biClrUsed) colors = min( info->bmiHeader.biClrUsed, 256 );
        else colors = info->bmiHeader.biBitCount > 8 ? 0 : 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

/*******************************************************************************************
 * Verify that the DIB parameters are valid.
 */
static BOOL is_valid_dib_format( const BITMAPINFOHEADER *info, BOOL allow_compression )
{
    if (info->biWidth <= 0) return FALSE;
    if (info->biHeight == 0) return FALSE;

    if (allow_compression && (info->biCompression == BI_RLE4 || info->biCompression == BI_RLE8))
    {
        if (info->biHeight < 0) return FALSE;
        if (!info->biSizeImage) return FALSE;
        return info->biBitCount == (info->biCompression == BI_RLE4 ? 4 : 8);
    }

    if (!info->biPlanes) return FALSE;

    /* check for size overflow */
    if (!info->biBitCount) return FALSE;
    if (UINT_MAX / info->biBitCount < info->biWidth) return FALSE;
    if (UINT_MAX / get_dib_stride( info->biWidth, info->biBitCount ) < abs( info->biHeight )) return FALSE;

    switch (info->biBitCount)
    {
    case 1:
    case 4:
    case 8:
    case 24:
        return (info->biCompression == BI_RGB);
    case 16:
    case 32:
        return (info->biCompression == BI_BITFIELDS || info->biCompression == BI_RGB);
    default:
        return FALSE;
    }
}

/*******************************************************************************************
 *  Fill out a true BITMAPINFOHEADER from a variable sized BITMAPINFOHEADER / BITMAPCOREHEADER.
 */
static BOOL bitmapinfoheader_from_user_bitmapinfo( BITMAPINFOHEADER *dst, const BITMAPINFOHEADER *info )
{
    if (!info) return FALSE;

    if (info->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        dst->biWidth         = core->bcWidth;
        dst->biHeight        = core->bcHeight;
        dst->biPlanes        = core->bcPlanes;
        dst->biBitCount      = core->bcBitCount;
        dst->biCompression   = BI_RGB;
        dst->biXPelsPerMeter = 0;
        dst->biYPelsPerMeter = 0;
        dst->biClrUsed       = 0;
        dst->biClrImportant  = 0;
    }
    else if (info->biSize >= sizeof(BITMAPINFOHEADER)) /* assume BITMAPINFOHEADER */
    {
        *dst = *info;
    }
    else
    {
        WARN( "(%u): unknown/wrong size for header\n", info->biSize );
        return FALSE;
    }

    dst->biSize = sizeof(*dst);
    if (dst->biCompression == BI_RGB || dst->biCompression == BI_BITFIELDS)
        dst->biSizeImage = get_dib_image_size( (BITMAPINFO *)dst );
    return TRUE;
}

/*******************************************************************************************
 *  Fill out a true BITMAPINFO from a variable sized BITMAPINFO / BITMAPCOREINFO.
 *
 * The resulting sanitized BITMAPINFO is guaranteed to have:
 * - biSize set to sizeof(BITMAPINFOHEADER)
 * - biSizeImage set to the actual image size even for non-compressed DIB
 * - biClrUsed set to the size of the color table, and 0 only when there is no color table
 * - color table present only for <= 8 bpp, always starts at info->bmiColors
 */
static BOOL bitmapinfo_from_user_bitmapinfo( BITMAPINFO *dst, const BITMAPINFO *info,
                                             UINT coloruse, BOOL allow_compression )
{
    void *src_colors;

    if (coloruse > DIB_PAL_INDICES) return FALSE;
    if (!bitmapinfoheader_from_user_bitmapinfo( &dst->bmiHeader, &info->bmiHeader )) return FALSE;
    if (!is_valid_dib_format( &dst->bmiHeader, allow_compression )) return FALSE;
    if (coloruse == DIB_PAL_INDICES && (dst->bmiHeader.biBitCount != 1 ||
                dst->bmiHeader.biCompression != BI_RGB)) return FALSE;

    src_colors = (char *)info + info->bmiHeader.biSize;

    if (dst->bmiHeader.biCompression == BI_BITFIELDS)
    {
        /* bitfields are always at bmiColors even in larger structures */
        memcpy( dst->bmiColors, info->bmiColors, 3 * sizeof(DWORD) );
        dst->bmiHeader.biClrUsed = 0;
    }
    else if (dst->bmiHeader.biBitCount <= 8)
    {
        unsigned int colors = dst->bmiHeader.biClrUsed;
        unsigned int max_colors = 1 << dst->bmiHeader.biBitCount;

        if (!colors) colors = max_colors;
        else colors = min( colors, max_colors );

        if (coloruse == DIB_PAL_COLORS)
        {
            memcpy( dst->bmiColors, src_colors, colors * sizeof(WORD) );
            max_colors = colors;
        }
        else if (coloruse == DIB_PAL_INDICES)
        {
            dst->bmiColors[0].rgbRed = 0;
            dst->bmiColors[0].rgbGreen = 0;
            dst->bmiColors[0].rgbBlue = 0;
            dst->bmiColors[0].rgbReserved = 0;
            dst->bmiColors[1].rgbRed = 0xff;
            dst->bmiColors[1].rgbGreen = 0xff;
            dst->bmiColors[1].rgbBlue = 0xff;
            dst->bmiColors[1].rgbReserved = 0;
            colors = max_colors;
        }
        else if (info->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        {
            memcpy( dst->bmiColors, src_colors, colors * sizeof(RGBQUAD) );
        }
        else
        {
            unsigned int i;
            RGBTRIPLE *triple = (RGBTRIPLE *)src_colors;
            for (i = 0; i < colors; i++)
            {
                dst->bmiColors[i].rgbRed      = triple[i].rgbtRed;
                dst->bmiColors[i].rgbGreen    = triple[i].rgbtGreen;
                dst->bmiColors[i].rgbBlue     = triple[i].rgbtBlue;
                dst->bmiColors[i].rgbReserved = 0;
            }
        }
        memset( dst->bmiColors + colors, 0, (max_colors - colors) * sizeof(RGBQUAD) );
        dst->bmiHeader.biClrUsed = max_colors;
    }
    else dst->bmiHeader.biClrUsed = 0;

    return TRUE;
}

static int fill_color_table_from_palette( BITMAPINFO *info, HDC hdc )
{
    PALETTEENTRY palEntry[256];
    HPALETTE palette = NtGdiGetDCObject( hdc, NTGDI_OBJ_PAL );
    int i, colors = 1 << info->bmiHeader.biBitCount;

    info->bmiHeader.biClrUsed = colors;

    if (!palette) return 0;

    memset( palEntry, 0, sizeof(palEntry) );
    if (!get_palette_entries( palette, 0, colors, palEntry ))
        return 0;

    for (i = 0; i < colors; i++)
    {
        info->bmiColors[i].rgbRed      = palEntry[i].peRed;
        info->bmiColors[i].rgbGreen    = palEntry[i].peGreen;
        info->bmiColors[i].rgbBlue     = palEntry[i].peBlue;
        info->bmiColors[i].rgbReserved = 0;
    }

    return colors;
}

BOOL fill_color_table_from_pal_colors( BITMAPINFO *info, HDC hdc )
{
    PALETTEENTRY entries[256];
    RGBQUAD table[256];
    HPALETTE palette;
    const WORD *index = (const WORD *)info->bmiColors;
    int i, count, colors = info->bmiHeader.biClrUsed;

    if (!colors) return TRUE;
    if (!(palette = NtGdiGetDCObject( hdc, NTGDI_OBJ_PAL )))
        return FALSE;
    if (!(count = get_palette_entries( palette, 0, colors, entries ))) return FALSE;

    for (i = 0; i < colors; i++, index++)
    {
        table[i].rgbRed   = entries[*index % count].peRed;
        table[i].rgbGreen = entries[*index % count].peGreen;
        table[i].rgbBlue  = entries[*index % count].peBlue;
        table[i].rgbReserved = 0;
    }
    info->bmiHeader.biClrUsed = 1 << info->bmiHeader.biBitCount;
    memcpy( info->bmiColors, table, colors * sizeof(RGBQUAD) );
    memset( info->bmiColors + colors, 0, (info->bmiHeader.biClrUsed - colors) * sizeof(RGBQUAD) );
    return TRUE;
}

static void *get_pixel_ptr( const BITMAPINFO *info, void *bits, int x, int y )
{
    const int width = info->bmiHeader.biWidth, height = info->bmiHeader.biHeight;
    const int bpp = info->bmiHeader.biBitCount;

    if (height > 0)
        return (char *)bits + (height - y - 1) * get_dib_stride( width, bpp ) + x * bpp / 8;
    else
        return (char *)bits + y * get_dib_stride( width, bpp ) + x * bpp / 8;
}

static BOOL build_rle_bitmap( BITMAPINFO *info, struct gdi_image_bits *bits, HRGN *clip )
{
    DWORD i = 0;
    int left, right;
    int x, y, width = info->bmiHeader.biWidth, height = info->bmiHeader.biHeight;
    HRGN run = NULL;
    BYTE skip, num, data;
    BYTE *out_bits, *in_bits = bits->ptr;

    if (clip) *clip = NULL;

    assert( info->bmiHeader.biBitCount == 4 || info->bmiHeader.biBitCount == 8 );

    out_bits = calloc( 1, get_dib_image_size( info ) );
    if (!out_bits) goto fail;

    if (clip)
    {
        *clip = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        run   = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        if (!*clip || !run) goto fail;
    }

    x = left = right = 0;
    y = height - 1;

    while (i < info->bmiHeader.biSizeImage - 1)
    {
        num = in_bits[i];
        data = in_bits[i + 1];
        i += 2;

        if (num)
        {
            if (x + num > width) num = width - x;
            if (num)
            {
                BYTE s = data, *out_ptr = get_pixel_ptr( info, out_bits, x, y );
                if (info->bmiHeader.biBitCount == 8)
                    memset( out_ptr, s, num );
                else
                {
                    if(x & 1)
                    {
                        s = ((s >> 4) & 0x0f) | ((s << 4) & 0xf0);
                        *out_ptr = (*out_ptr & 0xf0) | (s & 0x0f);
                        out_ptr++;
                        x++;
                        num--;
                    }
                    /* this will write one too many if num is odd, but that doesn't matter */
                    if (num) memset( out_ptr, s, (num + 1) / 2 );
                }
            }
            x += num;
            right = x;
        }
        else
        {
            if (data < 3)
            {
                if(left != right && clip)
                {
                    NtGdiSetRectRgn( run, left, y, right, y + 1 );
                    NtGdiCombineRgn( *clip, run, *clip, RGN_OR );
                }
                switch (data)
                {
                case 0: /* eol */
                    left = right = x = 0;
                    y--;
                    if(y < 0) goto done;
                    break;

                case 1: /* eod */
                    goto done;

                case 2: /* delta */
                    if (i >= info->bmiHeader.biSizeImage - 1) goto done;
                    x += in_bits[i];
                    if (x > width) x = width;
                    left = right = x;
                    y -= in_bits[i + 1];
                    if(y < 0) goto done;
                    i += 2;
                }
            }
            else /* data bytes of data */
            {
                num = data;
                skip = (num * info->bmiHeader.biBitCount + 7) / 8;
                if (skip > info->bmiHeader.biSizeImage - i) goto done;
                skip = (skip + 1) & ~1;
                if (x + num > width) num = width - x;
                if (num)
                {
                    BYTE *out_ptr = get_pixel_ptr( info, out_bits, x, y );
                    if (info->bmiHeader.biBitCount == 8)
                        memcpy( out_ptr, in_bits + i, num );
                    else
                    {
                        if(x & 1)
                        {
                            const BYTE *in_ptr = in_bits + i;
                            for ( ; num; num--, x++)
                            {
                                if (x & 1)
                                {
                                    *out_ptr = (*out_ptr & 0xf0) | ((*in_ptr >> 4) & 0x0f);
                                    out_ptr++;
                                }
                                else
                                    *out_ptr = (*in_ptr++ << 4) & 0xf0;
                            }
                        }
                        else
                            memcpy( out_ptr, in_bits + i, (num + 1) / 2);
                    }
                }
                x += num;
                right = x;
                i += skip;
            }
        }
    }

done:
    if (run) NtGdiDeleteObjectApp( run );
    if (bits->free) bits->free( bits );

    bits->ptr     = out_bits;
    bits->is_copy = TRUE;
    bits->free    = free_heap_bits;
    info->bmiHeader.biSizeImage = get_dib_image_size( info );

    return TRUE;

fail:
    if (run) NtGdiDeleteObjectApp( run );
    if (clip && *clip) NtGdiDeleteObjectApp( *clip );
    free( out_bits );
    return FALSE;
}



INT nulldrv_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst, INT heightDst,
                           INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, const void *bits,
                           BITMAPINFO *src_info, UINT coloruse, DWORD rop )
{
    DC *dc = get_nulldrv_dc( dev );
    char dst_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    struct bitblt_coords src, dst;
    struct gdi_image_bits src_bits;
    DWORD err;
    HRGN clip = NULL;
    INT ret = 0;
    INT height = abs( src_info->bmiHeader.biHeight );
    BOOL top_down = src_info->bmiHeader.biHeight < 0, non_stretch_from_origin = FALSE;
    RECT rect;

    TRACE("%d %d %d %d <- %d %d %d %d rop %08x\n", xDst, yDst, widthDst, heightDst,
          xSrc, ySrc, widthSrc, heightSrc, rop);

    src_bits.ptr = (void*)bits;
    src_bits.is_copy = FALSE;
    src_bits.free = NULL;

    if (coloruse == DIB_PAL_COLORS && !fill_color_table_from_pal_colors( src_info, dev->hdc )) return 0;

    rect.left   = xDst;
    rect.top    = yDst;
    rect.right  = xDst + widthDst;
    rect.bottom = yDst + heightDst;
    lp_to_dp( dc, (POINT *)&rect, 2 );
    dst.x      = rect.left;
    dst.y      = rect.top;
    dst.width  = rect.right - rect.left;
    dst.height = rect.bottom - rect.top;

    if (dc->attr->layout & LAYOUT_RTL && rop & NOMIRRORBITMAP)
    {
        dst.x += dst.width;
        dst.width = -dst.width;
    }
    rop &= ~NOMIRRORBITMAP;

    src.x      = xSrc;
    src.width  = widthSrc;
    src.y      = ySrc;
    src.height = heightSrc;

    if (src.x == 0 && src.y == 0 && src.width == dst.width && src.height == dst.height)
        non_stretch_from_origin = TRUE;

    if (src_info->bmiHeader.biCompression == BI_RLE4 || src_info->bmiHeader.biCompression == BI_RLE8)
    {
        BOOL want_clip = non_stretch_from_origin && (rop == SRCCOPY);
        if (!build_rle_bitmap( src_info, &src_bits, want_clip ? &clip : NULL )) return 0;
    }

    if (rop != SRCCOPY || non_stretch_from_origin)
    {
        if (dst.width == 1 && src.width > 1) src.width--;
        if (dst.height == 1 && src.height > 1) src.height--;
    }

    if (rop != SRCCOPY)
    {
        if (dst.width < 0 && dst.width == src.width)
        {
            /* This is off-by-one, but that's what Windows does */
            dst.x += dst.width;
            src.x += src.width;
            dst.width = -dst.width;
            src.width = -src.width;
        }
        if (dst.height < 0 && dst.height == src.height)
        {
            dst.y += dst.height;
            src.y += src.height;
            dst.height = -dst.height;
            src.height = -src.height;
        }
    }

    if (!top_down || (rop == SRCCOPY && !non_stretch_from_origin)) src.y = height - src.y - src.height;

    if (src.y >= height && src.y + src.height + 1 < height)
        src.y = height - 1;
    else if (src.y > 0 && src.y + src.height + 1 < 0)
        src.y = -src.height - 1;

    get_bounding_rect( &rect, src.x, src.y, src.width, src.height );

    src.visrect.left   = 0;
    src.visrect.right  = src_info->bmiHeader.biWidth;
    src.visrect.top    = 0;
    src.visrect.bottom = height;
    if (!intersect_rect( &src.visrect, &src.visrect, &rect )) goto done;

    if (rop == SRCCOPY) ret = height;
    else ret = src_info->bmiHeader.biHeight;

    get_bounding_rect( &rect, dst.x, dst.y, dst.width, dst.height );

    if (!clip_visrect( dc, &dst.visrect, &rect )) goto done;

    if (!intersect_vis_rectangles( &dst, &src )) goto done;

    if (clip) NtGdiOffsetRgn( clip, dst.x - src.x, dst.y - src.y );

    dev = GET_DC_PHYSDEV( dc, pPutImage );
    copy_bitmapinfo( dst_info, src_info );
    err = dev->funcs->pPutImage( dev, clip, dst_info, &src_bits, &src, &dst, rop );
    if (err == ERROR_BAD_FORMAT)
    {
        DWORD dst_colors = dst_info->bmiHeader.biClrUsed;

        /* 1-bpp destination without a color table requires a fake 1-entry table
         * that contains only the background color. There is no source DC to get
         * it from, so the background is hardcoded to the default color. */
        if (dst_info->bmiHeader.biBitCount == 1 && !dst_colors)
        {
            static const RGBQUAD default_bg = { 255, 255, 255 };
            dst_info->bmiColors[0] = default_bg;
            dst_info->bmiHeader.biClrUsed = 1;
        }

        if (!(err = convert_bits( src_info, &src, dst_info, &src_bits )))
        {
            /* get rid of the fake 1-bpp table */
            dst_info->bmiHeader.biClrUsed = dst_colors;
            err = dev->funcs->pPutImage( dev, clip, dst_info, &src_bits, &src, &dst, rop );
        }
    }

    if (err == ERROR_TRANSFORM_NOT_SUPPORTED)
    {
        copy_bitmapinfo( src_info, dst_info );
        err = stretch_bits( src_info, &src, dst_info, &dst, &src_bits, dc->attr->stretch_blt_mode );
        if (!err) err = dev->funcs->pPutImage( dev, NULL, dst_info, &src_bits, &src, &dst, rop );
    }
    if (err) ret = 0;

done:
    if (src_bits.free) src_bits.free( &src_bits );
    if (clip) NtGdiDeleteObjectApp( clip );
    return ret;
}

/***********************************************************************
 *           NtGdiStretchDIBitsInternal   (win32u.@)
 */
INT WINAPI NtGdiStretchDIBitsInternal( HDC hdc, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                       INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                       const void *bits, const BITMAPINFO *bmi, UINT coloruse,
                                       DWORD rop, UINT max_info, UINT max_bits, HANDLE xform )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    PHYSDEV physdev;
    DC *dc;
    INT ret = 0;

    if (!bits) return 0;
    if (!bitmapinfo_from_user_bitmapinfo( info, bmi, coloruse, TRUE ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if ((dc = get_dc_ptr( hdc )))
    {
        update_dc( dc );
        physdev = GET_DC_PHYSDEV( dc, pStretchDIBits );
        ret = physdev->funcs->pStretchDIBits( physdev, xDst, yDst, widthDst, heightDst,
                                              xSrc, ySrc, widthSrc, heightSrc, bits, info, coloruse, rop );
        release_dc_ptr( dc );
    }
    return ret;
}


/* Sets pixels in a bitmap using colors from DIB, see SetDIBits */
static int set_di_bits( HDC hdc, HBITMAP hbitmap, UINT startscan,
                        UINT lines, LPCVOID bits, const BITMAPINFO *info,
                        UINT coloruse )
{
    BITMAPOBJ *bitmap;
    char src_bmibuf[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *src_info = (BITMAPINFO *)src_bmibuf;
    char dst_bmibuf[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_bmibuf;
    INT result = 0;
    DWORD err;
    struct gdi_image_bits src_bits;
    struct bitblt_coords src, dst;
    INT src_to_dst_offset;
    HRGN clip = 0;

    if (!bitmapinfo_from_user_bitmapinfo( src_info, info, coloruse, TRUE ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (src_info->bmiHeader.biCompression == BI_BITFIELDS)
    {
        DWORD *masks = (DWORD *)src_info->bmiColors;
        if (!masks[0] || !masks[1] || !masks[2])
        {
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    src_bits.ptr = (void *)bits;
    src_bits.is_copy = FALSE;
    src_bits.free = NULL;
    src_bits.param = NULL;

    if (coloruse == DIB_PAL_COLORS && !fill_color_table_from_pal_colors( src_info, hdc )) return 0;

    if (!(bitmap = GDI_GetObjPtr( hbitmap, NTGDI_OBJ_BITMAP ))) return 0;

    if (coloruse == DIB_PAL_INDICES && bitmap->dib.dsBm.bmBitsPixel != 1) return 0;

    if (src_info->bmiHeader.biCompression == BI_RLE4 || src_info->bmiHeader.biCompression == BI_RLE8)
    {
        if (lines == 0) goto done;
        else lines = src_info->bmiHeader.biHeight;
        startscan = 0;

        if (!build_rle_bitmap( src_info, &src_bits, &clip )) goto done;
    }

    dst.visrect.left   = 0;
    dst.visrect.top    = 0;
    dst.visrect.right  = bitmap->dib.dsBm.bmWidth;
    dst.visrect.bottom = bitmap->dib.dsBm.bmHeight;

    src.visrect.left   = 0;
    src.visrect.top    = 0;
    src.visrect.right  = src_info->bmiHeader.biWidth;
    src.visrect.bottom = abs( src_info->bmiHeader.biHeight );

    if (src_info->bmiHeader.biHeight > 0)
    {
        src_to_dst_offset = -startscan;
        lines = min( lines, src.visrect.bottom - startscan );
        if (lines < src.visrect.bottom) src.visrect.top = src.visrect.bottom - lines;
    }
    else
    {
        src_to_dst_offset = src.visrect.bottom - lines - startscan;
        /* Unlike the bottom-up case, Windows doesn't limit lines. */
        if (lines < src.visrect.bottom) src.visrect.bottom = lines;
    }

    result = lines;

    OffsetRect( &src.visrect, 0, src_to_dst_offset );
    if (!intersect_rect( &dst.visrect, &src.visrect, &dst.visrect )) goto done;
    src.visrect = dst.visrect;
    OffsetRect( &src.visrect, 0, -src_to_dst_offset );

    src.x      = src.visrect.left;
    src.y      = src.visrect.top;
    src.width  = src.visrect.right - src.visrect.left;
    src.height = src.visrect.bottom - src.visrect.top;

    dst.x      = dst.visrect.left;
    dst.y      = dst.visrect.top;
    dst.width  = dst.visrect.right - dst.visrect.left;
    dst.height = dst.visrect.bottom - dst.visrect.top;

    copy_bitmapinfo( dst_info, src_info );

    err = put_image_into_bitmap( bitmap, clip, dst_info, &src_bits, &src, &dst );
    if (err == ERROR_BAD_FORMAT)
    {
        err = convert_bits( src_info, &src, dst_info, &src_bits );
        if (!err) err = put_image_into_bitmap( bitmap, clip, dst_info, &src_bits, &src, &dst );
    }
    if(err) result = 0;

done:
    if (src_bits.free) src_bits.free( &src_bits );
    if (clip) NtGdiDeleteObjectApp( clip );
    GDI_ReleaseObj( hbitmap );
    return result;
}


INT nulldrv_SetDIBitsToDevice( PHYSDEV dev, INT x_dst, INT y_dst, DWORD cx, DWORD cy,
                               INT x_src, INT y_src, UINT startscan, UINT lines,
                               const void *bits, BITMAPINFO *src_info, UINT coloruse )
{
    DC *dc = get_nulldrv_dc( dev );
    char dst_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    struct bitblt_coords src, dst;
    struct gdi_image_bits src_bits;
    HRGN clip = 0;
    DWORD err;
    UINT height;
    BOOL top_down;
    POINT pt;
    RECT rect;

    top_down = (src_info->bmiHeader.biHeight < 0);
    height = abs( src_info->bmiHeader.biHeight );

    src_bits.ptr = (void *)bits;
    src_bits.is_copy = FALSE;
    src_bits.free = NULL;

    if (!lines) return 0;
    if (coloruse == DIB_PAL_COLORS && !fill_color_table_from_pal_colors( src_info, dev->hdc )) return 0;

    if (src_info->bmiHeader.biCompression == BI_RLE4 || src_info->bmiHeader.biCompression == BI_RLE8)
    {
        startscan = 0;
        lines = height;
        src_info->bmiHeader.biWidth = x_src + cx;
        src_info->bmiHeader.biHeight = y_src + cy;
        if (src_info->bmiHeader.biWidth <= 0 || src_info->bmiHeader.biHeight <= 0) return 0;
        src.x = x_src;
        src.y = 0;
        src.width = cx;
        src.height = cy;
        if (!build_rle_bitmap( src_info, &src_bits, &clip )) return 0;
    }
    else
    {
        if (startscan >= height) return 0;
        if (!top_down && lines > height - startscan) lines = height - startscan;

        /* map src to top-down coordinates with startscan as origin */
        src.x = x_src;
        src.y = startscan + lines - (y_src + cy);
        src.width = cx;
        src.height = cy;
        if (src.y > 0)
        {
            if (!top_down)
            {
                /* get rid of unnecessary lines */
                if (src.y >= lines) return 0;
                lines -= src.y;
                src.y = 0;
            }
            else if (src.y >= lines) return lines;
        }
        src_info->bmiHeader.biHeight = top_down ? -min( lines, height ) : lines;
        src_info->bmiHeader.biSizeImage = get_dib_image_size( src_info );
    }

    src.visrect.left = src.x;
    src.visrect.top = src.y;
    src.visrect.right = src.x + cx;
    src.visrect.bottom = src.y + cy;
    rect.left = 0;
    rect.top = 0;
    rect.right = src_info->bmiHeader.biWidth;
    rect.bottom = abs( src_info->bmiHeader.biHeight );
    if (!intersect_rect( &src.visrect, &src.visrect, &rect ))
    {
        lines = 0;
        goto done;
    }

    pt.x = x_dst;
    pt.y = y_dst;
    lp_to_dp( dc, &pt, 1 );
    dst.x = pt.x;
    dst.y = pt.y;
    dst.width = cx;
    dst.height = cy;
    if (dc->attr->layout & LAYOUT_RTL) dst.x -= cx - 1;

    rect.left = dst.x;
    rect.top = dst.y;
    rect.right = dst.x + cx;
    rect.bottom = dst.y + cy;
    if (!clip_visrect( dc, &dst.visrect, &rect )) goto done;

    OffsetRect( &src.visrect, dst.x - src.x, dst.y - src.y );
    intersect_rect( &rect, &src.visrect, &dst.visrect );
    src.visrect = dst.visrect = rect;
    OffsetRect( &src.visrect, src.x - dst.x, src.y - dst.y );
    if (IsRectEmpty( &dst.visrect )) goto done;
    if (clip) NtGdiOffsetRgn( clip, dst.x - src.x, dst.y - src.y );

    dev = GET_DC_PHYSDEV( dc, pPutImage );
    copy_bitmapinfo( dst_info, src_info );
    err = dev->funcs->pPutImage( dev, clip, dst_info, &src_bits, &src, &dst, SRCCOPY );
    if (err == ERROR_BAD_FORMAT)
    {
        err = convert_bits( src_info, &src, dst_info, &src_bits );
        if (!err) err = dev->funcs->pPutImage( dev, clip, dst_info, &src_bits, &src, &dst, SRCCOPY );
    }
    if (err) lines = 0;

done:
    if (src_bits.free) src_bits.free( &src_bits );
    if (clip) NtGdiDeleteObjectApp( clip );
    return lines;
}

/***********************************************************************
 *           NtGdiSetDIBitsToDeviceInternal   (win32u.@)
 */
INT WINAPI NtGdiSetDIBitsToDeviceInternal( HDC hdc, INT xDest, INT yDest, DWORD cx,
                                           DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                                           UINT lines, const void *bits, const BITMAPINFO *bmi,
                                           UINT coloruse, UINT max_bits, UINT max_info,
                                           BOOL xform_coords, HANDLE xform )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    PHYSDEV physdev;
    INT ret = 0;
    DC *dc;

    if (xform) return set_di_bits( hdc, xform, startscan, lines, bits, bmi, coloruse );

    if (!bits) return 0;
    if (!bitmapinfo_from_user_bitmapinfo( info, bmi, coloruse, TRUE ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if ((dc = get_dc_ptr( hdc )))
    {
        update_dc( dc );
        physdev = GET_DC_PHYSDEV( dc, pSetDIBitsToDevice );
        ret = physdev->funcs->pSetDIBitsToDevice( physdev, xDest, yDest, cx, cy, xSrc,
                                                  ySrc, startscan, lines, bits, info, coloruse );
        release_dc_ptr( dc );
    }
    return ret;
}

UINT set_dib_dc_color_table( HDC hdc, UINT startpos, UINT entries, const RGBQUAD *colors )
{
    DC * dc;
    UINT i, result = 0;
    BITMAPOBJ * bitmap;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((bitmap = GDI_GetObjPtr( dc->hBitmap, NTGDI_OBJ_BITMAP )))
    {
        if (startpos < bitmap->dib.dsBmih.biClrUsed)
        {
            result = min( entries, bitmap->dib.dsBmih.biClrUsed - startpos );
            for (i = 0; i < result; i++)
            {
                bitmap->color_table[startpos + i].rgbBlue     = colors[i].rgbBlue;
                bitmap->color_table[startpos + i].rgbGreen    = colors[i].rgbGreen;
                bitmap->color_table[startpos + i].rgbRed      = colors[i].rgbRed;
                bitmap->color_table[startpos + i].rgbReserved = 0;
            }
        }
        GDI_ReleaseObj( dc->hBitmap );

        if (result)  /* update colors of selected objects */
        {
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, dc->attr->text_color, NULL );
            NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, dc->attr->background_color, NULL );
            NtGdiSelectPen( hdc, dc->hPen );
            NtGdiSelectBrush( hdc, dc->hBrush );
        }
    }
    release_dc_ptr( dc );
    return result;
}


UINT get_dib_dc_color_table( HDC hdc, UINT startpos, UINT entries, RGBQUAD *colors )
{
    DC * dc;
    BITMAPOBJ *bitmap;
    UINT result = 0;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((bitmap = GDI_GetObjPtr( dc->hBitmap, NTGDI_OBJ_BITMAP )))
    {
        if (startpos < bitmap->dib.dsBmih.biClrUsed)
        {
            result = min( entries, bitmap->dib.dsBmih.biClrUsed - startpos );
            memcpy(colors, bitmap->color_table + startpos, result * sizeof(RGBQUAD));
        }
        GDI_ReleaseObj( dc->hBitmap );
    }
    release_dc_ptr( dc );
    return result;
}

static const DWORD bit_fields_888[3] = {0xff0000, 0x00ff00, 0x0000ff};
static const DWORD bit_fields_555[3] = {0x7c00, 0x03e0, 0x001f};

static int fill_query_info( BITMAPINFO *info, BITMAPOBJ *bmp )
{
    BITMAPINFOHEADER header;

    header.biSize   = info->bmiHeader.biSize; /* Ensure we don't overwrite the original size when we copy back */
    header.biWidth  = bmp->dib.dsBm.bmWidth;
    header.biHeight = bmp->dib.dsBm.bmHeight;
    header.biPlanes = 1;
    header.biBitCount = bmp->dib.dsBm.bmBitsPixel;

    switch (header.biBitCount)
    {
    case 16:
    case 32:
        header.biCompression = BI_BITFIELDS;
        break;
    default:
        header.biCompression = BI_RGB;
        break;
    }

    header.biSizeImage = get_dib_image_size( (BITMAPINFO *)&header );
    header.biXPelsPerMeter = 0;
    header.biYPelsPerMeter = 0;
    header.biClrUsed       = 0;
    header.biClrImportant  = 0;

    if ( info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER) )
    {
        BITMAPCOREHEADER *coreheader = (BITMAPCOREHEADER *)info;

        coreheader->bcWidth    = header.biWidth;
        coreheader->bcHeight   = header.biHeight;
        coreheader->bcPlanes   = header.biPlanes;
        coreheader->bcBitCount = header.biBitCount;
    }
    else
        info->bmiHeader = header;

    return bmp->dib.dsBm.bmHeight;
}

/************************************************************************
 *      copy_color_info
 *
 * Copy BITMAPINFO color information where dst may be a BITMAPCOREINFO.
 */
static void copy_color_info(BITMAPINFO *dst, const BITMAPINFO *src, UINT coloruse)
{
    assert( src->bmiHeader.biSize == sizeof(BITMAPINFOHEADER) );

    if (dst->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREINFO *core = (BITMAPCOREINFO *)dst;
        if (coloruse == DIB_PAL_COLORS)
            memcpy( core->bmciColors, src->bmiColors, src->bmiHeader.biClrUsed * sizeof(WORD) );
        else
        {
            unsigned int i;
            for (i = 0; i < src->bmiHeader.biClrUsed; i++)
            {
                core->bmciColors[i].rgbtRed   = src->bmiColors[i].rgbRed;
                core->bmciColors[i].rgbtGreen = src->bmiColors[i].rgbGreen;
                core->bmciColors[i].rgbtBlue  = src->bmiColors[i].rgbBlue;
            }
        }
    }
    else
    {
        dst->bmiHeader.biClrUsed = src->bmiHeader.biClrUsed;

        if (src->bmiHeader.biCompression == BI_BITFIELDS)
            /* bitfields are always at bmiColors even in larger structures */
            memcpy( dst->bmiColors, src->bmiColors, 3 * sizeof(DWORD) );
        else if (src->bmiHeader.biClrUsed)
        {
            void *colorptr = (char *)dst + dst->bmiHeader.biSize;
            unsigned int size;

            if (coloruse == DIB_PAL_COLORS)
                size = src->bmiHeader.biClrUsed * sizeof(WORD);
            else
                size = src->bmiHeader.biClrUsed * sizeof(RGBQUAD);
            memcpy( colorptr, src->bmiColors, size );
        }
    }
}

const RGBQUAD *get_default_color_table( int bpp )
{
    static const RGBQUAD table_1[2] =
    {
        { 0x00, 0x00, 0x00 }, { 0xff, 0xff, 0xff }
    };
    static const RGBQUAD table_4[16] =
    {
        { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x80 }, { 0x00, 0x80, 0x00 }, { 0x00, 0x80, 0x80 },
        { 0x80, 0x00, 0x00 }, { 0x80, 0x00, 0x80 }, { 0x80, 0x80, 0x00 }, { 0x80, 0x80, 0x80 },
        { 0xc0, 0xc0, 0xc0 }, { 0x00, 0x00, 0xff }, { 0x00, 0xff, 0x00 }, { 0x00, 0xff, 0xff },
        { 0xff, 0x00, 0x00 }, { 0xff, 0x00, 0xff }, { 0xff, 0xff, 0x00 }, { 0xff, 0xff, 0xff },
    };
    static const RGBQUAD table_8[256] =
    {
        /* first and last 10 entries are the default system palette entries */
        { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x80 }, { 0x00, 0x80, 0x00 }, { 0x00, 0x80, 0x80 },
        { 0x80, 0x00, 0x00 }, { 0x80, 0x00, 0x80 }, { 0x80, 0x80, 0x00 }, { 0xc0, 0xc0, 0xc0 },
        { 0xc0, 0xdc, 0xc0 }, { 0xf0, 0xca, 0xa6 }, { 0x00, 0x20, 0x40 }, { 0x00, 0x20, 0x60 },
        { 0x00, 0x20, 0x80 }, { 0x00, 0x20, 0xa0 }, { 0x00, 0x20, 0xc0 }, { 0x00, 0x20, 0xe0 },
        { 0x00, 0x40, 0x00 }, { 0x00, 0x40, 0x20 }, { 0x00, 0x40, 0x40 }, { 0x00, 0x40, 0x60 },
        { 0x00, 0x40, 0x80 }, { 0x00, 0x40, 0xa0 }, { 0x00, 0x40, 0xc0 }, { 0x00, 0x40, 0xe0 },
        { 0x00, 0x60, 0x00 }, { 0x00, 0x60, 0x20 }, { 0x00, 0x60, 0x40 }, { 0x00, 0x60, 0x60 },
        { 0x00, 0x60, 0x80 }, { 0x00, 0x60, 0xa0 }, { 0x00, 0x60, 0xc0 }, { 0x00, 0x60, 0xe0 },
        { 0x00, 0x80, 0x00 }, { 0x00, 0x80, 0x20 }, { 0x00, 0x80, 0x40 }, { 0x00, 0x80, 0x60 },
        { 0x00, 0x80, 0x80 }, { 0x00, 0x80, 0xa0 }, { 0x00, 0x80, 0xc0 }, { 0x00, 0x80, 0xe0 },
        { 0x00, 0xa0, 0x00 }, { 0x00, 0xa0, 0x20 }, { 0x00, 0xa0, 0x40 }, { 0x00, 0xa0, 0x60 },
        { 0x00, 0xa0, 0x80 }, { 0x00, 0xa0, 0xa0 }, { 0x00, 0xa0, 0xc0 }, { 0x00, 0xa0, 0xe0 },
        { 0x00, 0xc0, 0x00 }, { 0x00, 0xc0, 0x20 }, { 0x00, 0xc0, 0x40 }, { 0x00, 0xc0, 0x60 },
        { 0x00, 0xc0, 0x80 }, { 0x00, 0xc0, 0xa0 }, { 0x00, 0xc0, 0xc0 }, { 0x00, 0xc0, 0xe0 },
        { 0x00, 0xe0, 0x00 }, { 0x00, 0xe0, 0x20 }, { 0x00, 0xe0, 0x40 }, { 0x00, 0xe0, 0x60 },
        { 0x00, 0xe0, 0x80 }, { 0x00, 0xe0, 0xa0 }, { 0x00, 0xe0, 0xc0 }, { 0x00, 0xe0, 0xe0 },
        { 0x40, 0x00, 0x00 }, { 0x40, 0x00, 0x20 }, { 0x40, 0x00, 0x40 }, { 0x40, 0x00, 0x60 },
        { 0x40, 0x00, 0x80 }, { 0x40, 0x00, 0xa0 }, { 0x40, 0x00, 0xc0 }, { 0x40, 0x00, 0xe0 },
        { 0x40, 0x20, 0x00 }, { 0x40, 0x20, 0x20 }, { 0x40, 0x20, 0x40 }, { 0x40, 0x20, 0x60 },
        { 0x40, 0x20, 0x80 }, { 0x40, 0x20, 0xa0 }, { 0x40, 0x20, 0xc0 }, { 0x40, 0x20, 0xe0 },
        { 0x40, 0x40, 0x00 }, { 0x40, 0x40, 0x20 }, { 0x40, 0x40, 0x40 }, { 0x40, 0x40, 0x60 },
        { 0x40, 0x40, 0x80 }, { 0x40, 0x40, 0xa0 }, { 0x40, 0x40, 0xc0 }, { 0x40, 0x40, 0xe0 },
        { 0x40, 0x60, 0x00 }, { 0x40, 0x60, 0x20 }, { 0x40, 0x60, 0x40 }, { 0x40, 0x60, 0x60 },
        { 0x40, 0x60, 0x80 }, { 0x40, 0x60, 0xa0 }, { 0x40, 0x60, 0xc0 }, { 0x40, 0x60, 0xe0 },
        { 0x40, 0x80, 0x00 }, { 0x40, 0x80, 0x20 }, { 0x40, 0x80, 0x40 }, { 0x40, 0x80, 0x60 },
        { 0x40, 0x80, 0x80 }, { 0x40, 0x80, 0xa0 }, { 0x40, 0x80, 0xc0 }, { 0x40, 0x80, 0xe0 },
        { 0x40, 0xa0, 0x00 }, { 0x40, 0xa0, 0x20 }, { 0x40, 0xa0, 0x40 }, { 0x40, 0xa0, 0x60 },
        { 0x40, 0xa0, 0x80 }, { 0x40, 0xa0, 0xa0 }, { 0x40, 0xa0, 0xc0 }, { 0x40, 0xa0, 0xe0 },
        { 0x40, 0xc0, 0x00 }, { 0x40, 0xc0, 0x20 }, { 0x40, 0xc0, 0x40 }, { 0x40, 0xc0, 0x60 },
        { 0x40, 0xc0, 0x80 }, { 0x40, 0xc0, 0xa0 }, { 0x40, 0xc0, 0xc0 }, { 0x40, 0xc0, 0xe0 },
        { 0x40, 0xe0, 0x00 }, { 0x40, 0xe0, 0x20 }, { 0x40, 0xe0, 0x40 }, { 0x40, 0xe0, 0x60 },
        { 0x40, 0xe0, 0x80 }, { 0x40, 0xe0, 0xa0 }, { 0x40, 0xe0, 0xc0 }, { 0x40, 0xe0, 0xe0 },
        { 0x80, 0x00, 0x00 }, { 0x80, 0x00, 0x20 }, { 0x80, 0x00, 0x40 }, { 0x80, 0x00, 0x60 },
        { 0x80, 0x00, 0x80 }, { 0x80, 0x00, 0xa0 }, { 0x80, 0x00, 0xc0 }, { 0x80, 0x00, 0xe0 },
        { 0x80, 0x20, 0x00 }, { 0x80, 0x20, 0x20 }, { 0x80, 0x20, 0x40 }, { 0x80, 0x20, 0x60 },
        { 0x80, 0x20, 0x80 }, { 0x80, 0x20, 0xa0 }, { 0x80, 0x20, 0xc0 }, { 0x80, 0x20, 0xe0 },
        { 0x80, 0x40, 0x00 }, { 0x80, 0x40, 0x20 }, { 0x80, 0x40, 0x40 }, { 0x80, 0x40, 0x60 },
        { 0x80, 0x40, 0x80 }, { 0x80, 0x40, 0xa0 }, { 0x80, 0x40, 0xc0 }, { 0x80, 0x40, 0xe0 },
        { 0x80, 0x60, 0x00 }, { 0x80, 0x60, 0x20 }, { 0x80, 0x60, 0x40 }, { 0x80, 0x60, 0x60 },
        { 0x80, 0x60, 0x80 }, { 0x80, 0x60, 0xa0 }, { 0x80, 0x60, 0xc0 }, { 0x80, 0x60, 0xe0 },
        { 0x80, 0x80, 0x00 }, { 0x80, 0x80, 0x20 }, { 0x80, 0x80, 0x40 }, { 0x80, 0x80, 0x60 },
        { 0x80, 0x80, 0x80 }, { 0x80, 0x80, 0xa0 }, { 0x80, 0x80, 0xc0 }, { 0x80, 0x80, 0xe0 },
        { 0x80, 0xa0, 0x00 }, { 0x80, 0xa0, 0x20 }, { 0x80, 0xa0, 0x40 }, { 0x80, 0xa0, 0x60 },
        { 0x80, 0xa0, 0x80 }, { 0x80, 0xa0, 0xa0 }, { 0x80, 0xa0, 0xc0 }, { 0x80, 0xa0, 0xe0 },
        { 0x80, 0xc0, 0x00 }, { 0x80, 0xc0, 0x20 }, { 0x80, 0xc0, 0x40 }, { 0x80, 0xc0, 0x60 },
        { 0x80, 0xc0, 0x80 }, { 0x80, 0xc0, 0xa0 }, { 0x80, 0xc0, 0xc0 }, { 0x80, 0xc0, 0xe0 },
        { 0x80, 0xe0, 0x00 }, { 0x80, 0xe0, 0x20 }, { 0x80, 0xe0, 0x40 }, { 0x80, 0xe0, 0x60 },
        { 0x80, 0xe0, 0x80 }, { 0x80, 0xe0, 0xa0 }, { 0x80, 0xe0, 0xc0 }, { 0x80, 0xe0, 0xe0 },
        { 0xc0, 0x00, 0x00 }, { 0xc0, 0x00, 0x20 }, { 0xc0, 0x00, 0x40 }, { 0xc0, 0x00, 0x60 },
        { 0xc0, 0x00, 0x80 }, { 0xc0, 0x00, 0xa0 }, { 0xc0, 0x00, 0xc0 }, { 0xc0, 0x00, 0xe0 },
        { 0xc0, 0x20, 0x00 }, { 0xc0, 0x20, 0x20 }, { 0xc0, 0x20, 0x40 }, { 0xc0, 0x20, 0x60 },
        { 0xc0, 0x20, 0x80 }, { 0xc0, 0x20, 0xa0 }, { 0xc0, 0x20, 0xc0 }, { 0xc0, 0x20, 0xe0 },
        { 0xc0, 0x40, 0x00 }, { 0xc0, 0x40, 0x20 }, { 0xc0, 0x40, 0x40 }, { 0xc0, 0x40, 0x60 },
        { 0xc0, 0x40, 0x80 }, { 0xc0, 0x40, 0xa0 }, { 0xc0, 0x40, 0xc0 }, { 0xc0, 0x40, 0xe0 },
        { 0xc0, 0x60, 0x00 }, { 0xc0, 0x60, 0x20 }, { 0xc0, 0x60, 0x40 }, { 0xc0, 0x60, 0x60 },
        { 0xc0, 0x60, 0x80 }, { 0xc0, 0x60, 0xa0 }, { 0xc0, 0x60, 0xc0 }, { 0xc0, 0x60, 0xe0 },
        { 0xc0, 0x80, 0x00 }, { 0xc0, 0x80, 0x20 }, { 0xc0, 0x80, 0x40 }, { 0xc0, 0x80, 0x60 },
        { 0xc0, 0x80, 0x80 }, { 0xc0, 0x80, 0xa0 }, { 0xc0, 0x80, 0xc0 }, { 0xc0, 0x80, 0xe0 },
        { 0xc0, 0xa0, 0x00 }, { 0xc0, 0xa0, 0x20 }, { 0xc0, 0xa0, 0x40 }, { 0xc0, 0xa0, 0x60 },
        { 0xc0, 0xa0, 0x80 }, { 0xc0, 0xa0, 0xa0 }, { 0xc0, 0xa0, 0xc0 }, { 0xc0, 0xa0, 0xe0 },
        { 0xc0, 0xc0, 0x00 }, { 0xc0, 0xc0, 0x20 }, { 0xc0, 0xc0, 0x40 }, { 0xc0, 0xc0, 0x60 },
        { 0xc0, 0xc0, 0x80 }, { 0xc0, 0xc0, 0xa0 }, { 0xf0, 0xfb, 0xff }, { 0xa4, 0xa0, 0xa0 },
        { 0x80, 0x80, 0x80 }, { 0x00, 0x00, 0xff }, { 0x00, 0xff, 0x00 }, { 0x00, 0xff, 0xff },
        { 0xff, 0x00, 0x00 }, { 0xff, 0x00, 0xff }, { 0xff, 0xff, 0x00 }, { 0xff, 0xff, 0xff },
    };

    switch (bpp)
    {
    case 1: return table_1;
    case 4: return table_4;
    case 8: return table_8;
    default: return NULL;
    }
}

void fill_default_color_table( BITMAPINFO *info )
{
    info->bmiHeader.biClrUsed = 1 << info->bmiHeader.biBitCount;
    memcpy( info->bmiColors, get_default_color_table( info->bmiHeader.biBitCount ),
            info->bmiHeader.biClrUsed * sizeof(RGBQUAD) );
}

void get_ddb_bitmapinfo( BITMAPOBJ *bmp, BITMAPINFO *info )
{
    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth         = bmp->dib.dsBm.bmWidth;
    info->bmiHeader.biHeight        = -bmp->dib.dsBm.bmHeight;
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = bmp->dib.dsBm.bmBitsPixel;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biSizeImage     = get_dib_image_size( info );
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
}

BITMAPINFO *copy_packed_dib( const BITMAPINFO *src_info, UINT usage )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *ret, *info = (BITMAPINFO *)buffer;
    unsigned int info_size;

    if (!bitmapinfo_from_user_bitmapinfo( info, src_info, usage, FALSE )) return NULL;

    info_size = get_dib_info_size( info, usage );
    if ((ret = malloc( info_size + info->bmiHeader.biSizeImage )))
    {
        memcpy( ret, info, info_size );
        memcpy( (char *)ret + info_size, (char *)src_info + bitmap_info_size( src_info, usage ),
                info->bmiHeader.biSizeImage );
    }
    return ret;
}

/******************************************************************************
 *           NtGdiGetDIBitsInternal    (win32u.@)
 *
 * Retrieves bits of bitmap and copies to buffer.
 */
INT WINAPI NtGdiGetDIBitsInternal( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                   void *bits, BITMAPINFO *info, UINT coloruse,
                                   UINT max_bits, UINT max_info )
{
    DC * dc;
    BITMAPOBJ * bmp;
    int i, dst_to_src_offset, ret = 0;
    DWORD err;
    char dst_bmibuf[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_bmibuf;
    char src_bmibuf[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *src_info = (BITMAPINFO *)src_bmibuf;
    struct gdi_image_bits src_bits;
    struct bitblt_coords src, dst;
    BOOL empty_rect = FALSE;

    /* Since info may be a BITMAPCOREINFO or any of the larger BITMAPINFO structures, we'll use our
       own copy and transfer the colour info back at the end */
    if (!bitmapinfoheader_from_user_bitmapinfo( &dst_info->bmiHeader, &info->bmiHeader )) return 0;
    if (coloruse > DIB_PAL_COLORS) return 0;
    if (bits &&
        (dst_info->bmiHeader.biCompression == BI_JPEG || dst_info->bmiHeader.biCompression == BI_PNG))
        return 0;
    dst_info->bmiHeader.biClrUsed = 0;
    dst_info->bmiHeader.biClrImportant = 0;

    if (!(dc = get_dc_ptr( hdc )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    update_dc( dc );
    if (!(bmp = GDI_GetObjPtr( hbitmap, NTGDI_OBJ_BITMAP )))
    {
        release_dc_ptr( dc );
	return 0;
    }

    src.visrect.left   = 0;
    src.visrect.top    = 0;
    src.visrect.right  = bmp->dib.dsBm.bmWidth;
    src.visrect.bottom = bmp->dib.dsBm.bmHeight;

    dst.visrect.left   = 0;
    dst.visrect.top    = 0;
    dst.visrect.right  = dst_info->bmiHeader.biWidth;
    dst.visrect.bottom = abs( dst_info->bmiHeader.biHeight );

    if (lines == 0 || startscan >= dst.visrect.bottom)
        bits = NULL;

    if (!bits && dst_info->bmiHeader.biBitCount == 0) /* query bitmap info only */
    {
        ret = fill_query_info( info, bmp );
        goto done;
    }

    /* validate parameters */

    if (dst_info->bmiHeader.biWidth <= 0) goto done;
    if (dst_info->bmiHeader.biHeight == 0) goto done;

    switch (dst_info->bmiHeader.biCompression)
    {
    case BI_RLE4:
        if (dst_info->bmiHeader.biBitCount != 4) goto done;
        if (dst_info->bmiHeader.biHeight < 0) goto done;
        if (bits) goto done;  /* can't retrieve compressed bits */
        break;
    case BI_RLE8:
        if (dst_info->bmiHeader.biBitCount != 8) goto done;
        if (dst_info->bmiHeader.biHeight < 0) goto done;
        if (bits) goto done;  /* can't retrieve compressed bits */
        break;
    case BI_BITFIELDS:
        if (dst_info->bmiHeader.biBitCount != 16 && dst_info->bmiHeader.biBitCount != 32) goto done;
        /* fall through */
    case BI_RGB:
        if (lines && !dst_info->bmiHeader.biPlanes) goto done;
        if (dst_info->bmiHeader.biBitCount == 1) break;
        if (dst_info->bmiHeader.biBitCount == 4) break;
        if (dst_info->bmiHeader.biBitCount == 8) break;
        if (dst_info->bmiHeader.biBitCount == 16) break;
        if (dst_info->bmiHeader.biBitCount == 24) break;
        if (dst_info->bmiHeader.biBitCount == 32) break;
        /* fall through */
    default:
        goto done;
    }

    if (bits)
    {
        if (dst_info->bmiHeader.biHeight > 0)
        {
            dst_to_src_offset = -startscan;
            lines = min( lines, dst.visrect.bottom - startscan );
            if (lines < dst.visrect.bottom) dst.visrect.top = dst.visrect.bottom - lines;
        }
        else
        {
            dst_to_src_offset = dst.visrect.bottom - lines - startscan;
            if (dst_to_src_offset < 0)
            {
                dst_to_src_offset = 0;
                lines = dst.visrect.bottom - startscan;
            }
            if (lines < dst.visrect.bottom) dst.visrect.bottom = lines;
        }

        OffsetRect( &dst.visrect, 0, dst_to_src_offset );
        empty_rect = !intersect_rect( &src.visrect, &src.visrect, &dst.visrect );
        dst.visrect = src.visrect;
        OffsetRect( &dst.visrect, 0, -dst_to_src_offset );

        if (dst_info->bmiHeader.biHeight > 0)
        {
            if (dst.visrect.bottom < dst_info->bmiHeader.biHeight)
            {
                int pad_lines = min( dst_info->bmiHeader.biHeight - dst.visrect.bottom, lines );
                int pad_bytes = pad_lines * get_dib_stride( dst_info->bmiHeader.biWidth, dst_info->bmiHeader.biBitCount );
                memset( bits, 0, pad_bytes );
                bits = (char *)bits + pad_bytes;
            }
        }
        else
        {
            if (dst.visrect.bottom < lines)
            {
                int pad_lines = lines - dst.visrect.bottom;
                int stride = get_dib_stride( dst_info->bmiHeader.biWidth, dst_info->bmiHeader.biBitCount );
                int pad_bytes = pad_lines * stride;
                memset( (char *)bits + dst.visrect.bottom * stride, 0, pad_bytes );
            }
        }

        if (empty_rect) bits = NULL;

        src.x      = src.visrect.left;
        src.y      = src.visrect.top;
        src.width  = src.visrect.right - src.visrect.left;
        src.height = src.visrect.bottom - src.visrect.top;

        lines = src.height;
    }

    err = get_image_from_bitmap( bmp, src_info, bits ? &src_bits : NULL, bits ? &src : NULL );

    if (err) goto done;

    /* fill out the src colour table, if it needs one */
    if (src_info->bmiHeader.biBitCount <= 8 && src_info->bmiHeader.biClrUsed == 0)
        fill_default_color_table( src_info );

    /* if the src and dst are the same depth, copy the colour info across */
    if (dst_info->bmiHeader.biBitCount == src_info->bmiHeader.biBitCount && coloruse == DIB_RGB_COLORS )
    {
        switch (src_info->bmiHeader.biBitCount)
        {
        case 16:
            if (src_info->bmiHeader.biCompression == BI_RGB)
            {
                src_info->bmiHeader.biCompression = BI_BITFIELDS;
                memcpy( src_info->bmiColors, bit_fields_555, sizeof(bit_fields_555) );
            }
            break;
        case 32:
            if (src_info->bmiHeader.biCompression == BI_RGB)
            {
                src_info->bmiHeader.biCompression = BI_BITFIELDS;
                memcpy( src_info->bmiColors, bit_fields_888, sizeof(bit_fields_888) );
            }
            break;
        }
        copy_color_info( dst_info, src_info, coloruse );
    }
    else if (dst_info->bmiHeader.biBitCount <= 8) /* otherwise construct a default colour table for the dst, if needed */
    {
        if( coloruse == DIB_PAL_COLORS )
        {
            if (!fill_color_table_from_palette( dst_info, hdc )) goto done;
        }
        else
        {
            fill_default_color_table( dst_info );
        }
    }

    if (bits)
    {
        if(dst_info->bmiHeader.biHeight > 0)
            dst_info->bmiHeader.biHeight = src.height;
        else
            dst_info->bmiHeader.biHeight = -src.height;
        dst_info->bmiHeader.biSizeImage = get_dib_image_size( dst_info );

        convert_bitmapinfo( src_info, src_bits.ptr, &src, dst_info, bits );
        if (src_bits.free) src_bits.free( &src_bits );
        ret = lines;
    }
    else
        ret = !empty_rect;

    if (coloruse == DIB_PAL_COLORS)
    {
        WORD *index = (WORD *)dst_info->bmiColors;
        for (i = 0; i < dst_info->bmiHeader.biClrUsed; i++, index++)
            *index = i;
    }

    copy_color_info( info, dst_info, coloruse );
    if (info->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
    {
        info->bmiHeader.biClrUsed = 0;
        info->bmiHeader.biSizeImage = get_dib_image_size( info );
    }

done:
    release_dc_ptr( dc );
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/***********************************************************************
 *           NtGdiCreateDIBitmapInternal    (win32u.@)
 *
 * Creates a DDB (device dependent bitmap) from a DIB.
 * The DDB will have the same color depth as the reference DC.
 */
HBITMAP WINAPI NtGdiCreateDIBitmapInternal( HDC hdc, INT width, INT height, DWORD init,
                                            const void *bits, const BITMAPINFO *data,
                                            UINT coloruse, UINT max_info, UINT max_bits,
                                            ULONG flags, HANDLE xform )
{
    HBITMAP handle;

    if (coloruse > DIB_PAL_COLORS + 1 || width < 0) return 0;

    /* Top-down DIBs have a negative height */
    height = abs( height );

    TRACE( "hdc=%p, init=%u, bits=%p, data=%p, coloruse=%u (bitmap: width=%d, height=%d)\n",
           hdc, init, bits, data, coloruse, width, height );

    if (hdc == NULL)
        handle = NtGdiCreateBitmap( width, height, 1, 1, NULL );
    else
        handle = NtGdiCreateCompatibleBitmap( hdc, width, height );

    if (handle)
    {
        if (init & CBM_INIT)
        {
            if (set_di_bits( hdc, handle, 0, height, bits, data, coloruse ) == 0)
            {
                NtGdiDeleteObjectApp( handle );
                handle = 0;
            }
        }
    }

    return handle;
}


/***********************************************************************
 *           NtGdiCreateDIBSection    (win32u.@)
 */
HBITMAP WINAPI NtGdiCreateDIBSection( HDC hdc, HANDLE section, DWORD offset, const BITMAPINFO *bmi,
                                      UINT usage, UINT header_size, ULONG flags,
                                      ULONG_PTR color_space, void **bits )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    HBITMAP ret = 0;
    BITMAPOBJ *bmp;
    void *mapBits = NULL;

    if (bits) *bits = NULL;
    if (!bitmapinfo_from_user_bitmapinfo( info, bmi, usage, FALSE )) return 0;
    if (usage > DIB_PAL_COLORS) return 0;
    if (info->bmiHeader.biPlanes != 1)
    {
        if (info->bmiHeader.biPlanes * info->bmiHeader.biBitCount > 16) return 0;
        WARN( "%u planes not properly supported\n", info->bmiHeader.biPlanes );
    }

    if (!(bmp = calloc( 1, sizeof(*bmp) ))) return 0;

    TRACE("format (%d,%d), planes %d, bpp %d, %s, size %d %s\n",
          info->bmiHeader.biWidth, info->bmiHeader.biHeight,
          info->bmiHeader.biPlanes, info->bmiHeader.biBitCount,
          info->bmiHeader.biCompression == BI_BITFIELDS? "BI_BITFIELDS" : "BI_RGB",
          info->bmiHeader.biSizeImage, usage == DIB_PAL_COLORS? "PAL" : "RGB");

    bmp->dib.dsBm.bmType       = 0;
    bmp->dib.dsBm.bmWidth      = info->bmiHeader.biWidth;
    bmp->dib.dsBm.bmHeight     = abs( info->bmiHeader.biHeight );
    bmp->dib.dsBm.bmWidthBytes = get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
    bmp->dib.dsBm.bmPlanes     = info->bmiHeader.biPlanes;
    bmp->dib.dsBm.bmBitsPixel  = info->bmiHeader.biBitCount;
    bmp->dib.dsBmih            = info->bmiHeader;

    if (info->bmiHeader.biBitCount <= 8)  /* build the color table */
    {
        if (usage == DIB_PAL_COLORS && !fill_color_table_from_pal_colors( info, hdc ))
            goto error;
        bmp->dib.dsBmih.biClrUsed = info->bmiHeader.biClrUsed;
        if (!(bmp->color_table = malloc( bmp->dib.dsBmih.biClrUsed * sizeof(RGBQUAD) )))
            goto error;
        memcpy( bmp->color_table, info->bmiColors, bmp->dib.dsBmih.biClrUsed * sizeof(RGBQUAD) );
    }

    /* set dsBitfields values */
    if (info->bmiHeader.biBitCount == 16 && info->bmiHeader.biCompression == BI_RGB)
    {
        bmp->dib.dsBmih.biCompression = BI_BITFIELDS;
        bmp->dib.dsBitfields[0] = 0x7c00;
        bmp->dib.dsBitfields[1] = 0x03e0;
        bmp->dib.dsBitfields[2] = 0x001f;
    }
    else if (info->bmiHeader.biCompression == BI_BITFIELDS)
    {
        if (usage == DIB_PAL_COLORS) goto error;
        bmp->dib.dsBitfields[0] =  *(const DWORD *)info->bmiColors;
        bmp->dib.dsBitfields[1] =  *((const DWORD *)info->bmiColors + 1);
        bmp->dib.dsBitfields[2] =  *((const DWORD *)info->bmiColors + 2);
        if (!bmp->dib.dsBitfields[0] || !bmp->dib.dsBitfields[1] || !bmp->dib.dsBitfields[2]) goto error;
    }
    else bmp->dib.dsBitfields[0] = bmp->dib.dsBitfields[1] = bmp->dib.dsBitfields[2] = 0;

    /* get storage location for DIB bits */

    if (section)
    {
        LARGE_INTEGER map_offset;
        SIZE_T map_size;

        map_offset.QuadPart = offset - (offset % system_info.AllocationGranularity);
        map_size = bmp->dib.dsBmih.biSizeImage + (offset - map_offset.QuadPart);
        if (NtMapViewOfSection( section, GetCurrentProcess(), &mapBits, 0, 0, &map_offset,
                                &map_size, ViewShare, 0, PAGE_READWRITE ))
            goto error;
        bmp->dib.dsBm.bmBits = (char *)mapBits + (offset - map_offset.QuadPart);
    }
    else
    {
        SIZE_T size = bmp->dib.dsBmih.biSizeImage;
        offset = 0;
        if (NtAllocateVirtualMemory( GetCurrentProcess(), &bmp->dib.dsBm.bmBits, zero_bits,
                                     &size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ))
            goto error;
    }
    bmp->dib.dshSection = section;
    bmp->dib.dsOffset = offset;

    if ((ret = alloc_gdi_handle( &bmp->obj, NTGDI_OBJ_BITMAP, &dib_funcs )))
    {
        if (bits) *bits = bmp->dib.dsBm.bmBits;
        return ret;
    }

    if (section) NtUnmapViewOfSection( GetCurrentProcess(), mapBits );
    else
    {
        SIZE_T size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &bmp->dib.dsBm.bmBits, &size, MEM_RELEASE );
    }
error:
    free( bmp->color_table );
    free( bmp );
    return 0;
}


static BOOL memory_dib_DeleteObject( HGDIOBJ handle )
{
    BITMAPOBJ *bmp;

    if (!(bmp = free_gdi_handle( handle ))) return FALSE;

    free( bmp->color_table );
    free( bmp );
    return TRUE;
}


static const struct gdi_obj_funcs memory_dib_funcs =
{
    .pGetObjectW = DIB_GetObject,
    .pDeleteObject = memory_dib_DeleteObject,
};

/***********************************************************************
 *           NtGdiDdDDICreateDCFromMemory    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDICreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    const struct d3dddi_format_info
    {
        D3DDDIFORMAT format;
        unsigned int bit_count;
        DWORD compression;
        unsigned int palette_size;
        DWORD mask_r, mask_g, mask_b;
    } *format = NULL;
    BITMAPOBJ *bmp = NULL;
    HBITMAP bitmap;
    unsigned int i;
    HDC dc;

    static const struct d3dddi_format_info format_info[] =
    {
        { D3DDDIFMT_R8G8B8,   24, BI_RGB,       0,   0x00000000, 0x00000000, 0x00000000 },
        { D3DDDIFMT_A8R8G8B8, 32, BI_RGB,       0,   0x00000000, 0x00000000, 0x00000000 },
        { D3DDDIFMT_X8R8G8B8, 32, BI_RGB,       0,   0x00000000, 0x00000000, 0x00000000 },
        { D3DDDIFMT_R5G6B5,   16, BI_BITFIELDS, 0,   0x0000f800, 0x000007e0, 0x0000001f },
        { D3DDDIFMT_X1R5G5B5, 16, BI_BITFIELDS, 0,   0x00007c00, 0x000003e0, 0x0000001f },
        { D3DDDIFMT_A1R5G5B5, 16, BI_BITFIELDS, 0,   0x00007c00, 0x000003e0, 0x0000001f },
        { D3DDDIFMT_A4R4G4B4, 16, BI_BITFIELDS, 0,   0x00000f00, 0x000000f0, 0x0000000f },
        { D3DDDIFMT_X4R4G4B4, 16, BI_BITFIELDS, 0,   0x00000f00, 0x000000f0, 0x0000000f },
        { D3DDDIFMT_P8,       8,  BI_RGB,       256, 0x00000000, 0x00000000, 0x00000000 },
    };

    if (!desc) return STATUS_INVALID_PARAMETER;

    TRACE("memory %p, format %#x, width %u, height %u, pitch %u, device dc %p, color table %p.\n",
          desc->pMemory, desc->Format, desc->Width, desc->Height,
          desc->Pitch, desc->hDeviceDc, desc->pColorTable);

    if (!desc->pMemory) return STATUS_INVALID_PARAMETER;

    for (i = 0; i < ARRAY_SIZE( format_info ); ++i)
    {
        if (format_info[i].format == desc->Format)
        {
            format = &format_info[i];
            break;
        }
    }
    if (!format) return STATUS_INVALID_PARAMETER;

    if (desc->Width > (UINT_MAX & ~3) / (format->bit_count / 8) ||
        !desc->Pitch || desc->Pitch < get_dib_stride( desc->Width, format->bit_count ) ||
        !desc->Height || desc->Height > UINT_MAX / desc->Pitch) return STATUS_INVALID_PARAMETER;

    if (!desc->hDeviceDc || !(dc = NtGdiCreateCompatibleDC( desc->hDeviceDc )))
        return STATUS_INVALID_PARAMETER;

    if (!(bmp = calloc( 1, sizeof(*bmp) ))) goto error;

    bmp->dib.dsBm.bmWidth      = desc->Width;
    bmp->dib.dsBm.bmHeight     = desc->Height;
    bmp->dib.dsBm.bmWidthBytes = desc->Pitch;
    bmp->dib.dsBm.bmPlanes     = 1;
    bmp->dib.dsBm.bmBitsPixel  = format->bit_count;
    bmp->dib.dsBm.bmBits       = desc->pMemory;

    bmp->dib.dsBmih.biSize         = sizeof(bmp->dib.dsBmih);
    bmp->dib.dsBmih.biWidth        = desc->Width;
    bmp->dib.dsBmih.biHeight       = -(LONG)desc->Height;
    bmp->dib.dsBmih.biPlanes       = 1;
    bmp->dib.dsBmih.biBitCount     = format->bit_count;
    bmp->dib.dsBmih.biCompression  = format->compression;
    bmp->dib.dsBmih.biClrUsed      = format->palette_size;
    bmp->dib.dsBmih.biClrImportant = format->palette_size;

    bmp->dib.dsBitfields[0] = format->mask_r;
    bmp->dib.dsBitfields[1] = format->mask_g;
    bmp->dib.dsBitfields[2] = format->mask_b;

    if (format->palette_size)
    {
        if (!(bmp->color_table = malloc( format->palette_size * sizeof(*bmp->color_table) )))
            goto error;
        if (desc->pColorTable)
        {
            for (i = 0; i < format->palette_size; ++i)
            {
                bmp->color_table[i].rgbRed      = desc->pColorTable[i].peRed;
                bmp->color_table[i].rgbGreen    = desc->pColorTable[i].peGreen;
                bmp->color_table[i].rgbBlue     = desc->pColorTable[i].peBlue;
                bmp->color_table[i].rgbReserved = 0;
            }
        }
        else
        {
            memcpy( bmp->color_table, get_default_color_table( format->bit_count ),
                    format->palette_size * sizeof(*bmp->color_table) );
        }
    }

    if (!(bitmap = alloc_gdi_handle( &bmp->obj, NTGDI_OBJ_BITMAP, &memory_dib_funcs ))) goto error;

    desc->hDc = dc;
    desc->hBitmap = bitmap;
    NtGdiSelectBitmap( dc, bitmap );
    return STATUS_SUCCESS;

error:
    if (bmp) free( bmp->color_table );
    free( bmp );
    NtGdiDeleteObjectApp( dc );
    return STATUS_INVALID_PARAMETER;
}


/***********************************************************************
 *           NtGdiDdDDIDestroyDCFromMemory    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    if (!desc) return STATUS_INVALID_PARAMETER;

    TRACE("dc %p, bitmap %p.\n", desc->hDc, desc->hBitmap);

    if (get_gdi_object_type( desc->hDc ) != NTGDI_OBJ_MEMDC ||
        get_gdi_object_type( desc->hBitmap ) != NTGDI_OBJ_BITMAP) return STATUS_INVALID_PARAMETER;
    NtGdiDeleteObjectApp( desc->hBitmap );
    NtGdiDeleteObjectApp( desc->hDc );

    return STATUS_SUCCESS;
}


/***********************************************************************
 *           DIB_GetObject
 */
static INT DIB_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    INT ret = 0;
    BITMAPOBJ *bmp = GDI_GetObjPtr( handle, NTGDI_OBJ_BITMAP );

    if (!bmp) return 0;

    if (!buffer) ret = sizeof(BITMAP);
    else if (count >= sizeof(DIBSECTION))
    {
        DIBSECTION *dib = buffer;
        *dib = bmp->dib;
        dib->dsBm.bmWidthBytes = get_dib_stride( dib->dsBm.bmWidth, dib->dsBm.bmBitsPixel );
        dib->dsBmih.biHeight = abs( dib->dsBmih.biHeight );
        ret = sizeof(DIBSECTION);
    }
    else if (count >= sizeof(BITMAP))
    {
        BITMAP *bitmap = buffer;
        *bitmap = bmp->dib.dsBm;
        bitmap->bmWidthBytes = get_dib_stride( bitmap->bmWidth, bitmap->bmBitsPixel );
        ret = sizeof(BITMAP);
    }

    GDI_ReleaseObj( handle );
    return ret;
}


/***********************************************************************
 *           DIB_DeleteObject
 */
static BOOL DIB_DeleteObject( HGDIOBJ handle )
{
    BITMAPOBJ *bmp;

    if (!(bmp = free_gdi_handle( handle ))) return FALSE;

    if (bmp->dib.dshSection)
    {
        NtUnmapViewOfSection( GetCurrentProcess(), (char *)bmp->dib.dsBm.bmBits -
                              (bmp->dib.dsOffset % system_info.AllocationGranularity) );
    }
    else
    {
        SIZE_T size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &bmp->dib.dsBm.bmBits, &size, MEM_RELEASE );
    }

    free( bmp->color_table );
    free( bmp );
    return TRUE;
}

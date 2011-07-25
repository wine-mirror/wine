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
    You can distinguish them by looking at the first member (bcSize/biSize),
    or use the internal function DIB_GetBitmapInfo.

    
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
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
        colors = info->bmiHeader.biClrUsed;
        if (colors > 256) colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 0 for COREHEADER, 1 for INFOHEADER, -1 for error.
 */
int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                       LONG *height, WORD *planes, WORD *bpp, DWORD *compr, DWORD *size )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *planes = core->bcPlanes;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        *size   = 0;
        return 0;
    }
    if (header->biSize >= sizeof(BITMAPINFOHEADER)) /* assume BITMAPINFOHEADER */
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *planes = header->biPlanes;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        *size   = header->biSizeImage;
        return 1;
    }
    ERR("(%d): unknown/wrong size for header\n", header->biSize );
    return -1;
}

/*******************************************************************************************
 *  Fill out a true BITMAPINFO from a variable sized BITMAPINFO / BITMAPCOREINFO.
 */
static BOOL bitmapinfo_from_user_bitmapinfo( BITMAPINFO *dst, const BITMAPINFO *info, UINT coloruse )
{
    LONG width, height;
    WORD planes, bpp;
    DWORD compr, size;
    void *src_colors = (char *)info + info->bmiHeader.biSize;
    unsigned int colors;
    int bitmap_type = DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &planes, &bpp, &compr, &size );

    if (bitmap_type == -1) return FALSE;

    colors = (bpp > 8) ? 0 : 1 << bpp;

    if (bitmap_type == 1)
    {
        dst->bmiHeader                 = info->bmiHeader;
        dst->bmiHeader.biSize          = sizeof(dst->bmiHeader);

        if (info->bmiHeader.biClrUsed) colors = info->bmiHeader.biClrUsed;

        if (info->bmiHeader.biCompression == BI_BITFIELDS)
            /* bitfields are always at bmiColors even in larger structures */
            memcpy( dst->bmiColors, info->bmiColors, 3 * sizeof(DWORD) );
        else if (colors)
        {
            unsigned int size;

            if (coloruse == DIB_PAL_COLORS)
                size = colors * sizeof(WORD);
            else
                size = colors * sizeof(RGBQUAD);
            memcpy( dst->bmiColors, src_colors, size );
        }
    }
    else
    {
        dst->bmiHeader.biSize          = sizeof(dst->bmiHeader);
        dst->bmiHeader.biWidth         = width;
        dst->bmiHeader.biHeight        = height;
        dst->bmiHeader.biPlanes        = planes;
        dst->bmiHeader.biBitCount      = bpp;
        dst->bmiHeader.biCompression   = compr;
        dst->bmiHeader.biSizeImage     = size;
        dst->bmiHeader.biXPelsPerMeter = 0;
        dst->bmiHeader.biYPelsPerMeter = 0;
        dst->bmiHeader.biClrUsed       = 0;
        dst->bmiHeader.biClrImportant  = 0;

        if (colors)
        {
            if (coloruse == DIB_PAL_COLORS)
                memcpy( dst->bmiColors, src_colors, colors * sizeof(WORD) );
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
        }
    }

    return TRUE;
}

static int fill_color_table_from_palette( BITMAPINFO *info, DC *dc )
{
    PALETTEENTRY palEntry[256];
    int i, colors = (info->bmiHeader.biBitCount > 8) ? 0 : 1 << info->bmiHeader.biBitCount;

    if (info->bmiHeader.biClrUsed) colors = info->bmiHeader.biClrUsed;

    if (!colors) return 0;

    memset( palEntry, 0, sizeof(palEntry) );
    if (!GetPaletteEntries( dc->hPalette, 0, colors, palEntry ))
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

static void *get_pixel_ptr( const BITMAPINFO *info, void *bits, int x, int y )
{
    const int width = info->bmiHeader.biWidth, height = info->bmiHeader.biHeight;
    const int bpp = info->bmiHeader.biBitCount;

    if (height > 0)
        return (char *)bits + (height - y - 1) * get_dib_stride( width, bpp ) + x * bpp / 8;
    else
        return (char *)bits + y * get_dib_stride( width, bpp ) + x * bpp / 8;
}

static BOOL build_rle_bitmap( const BITMAPINFO *info, struct gdi_image_bits *bits, HRGN *clip )
{
    int i = 0;
    int left, right;
    int x, y, width = info->bmiHeader.biWidth, height = info->bmiHeader.biHeight;
    HRGN run = NULL;
    BYTE skip, num, data;
    BYTE *out_bits, *in_bits = bits->ptr;

    *clip = NULL;

    assert( info->bmiHeader.biBitCount == 4 || info->bmiHeader.biBitCount == 8 );

    out_bits = HeapAlloc( GetProcessHeap(), 0, get_dib_image_size( info ) );
    *clip = CreateRectRgn( 0, 0, 0, 0 );
    run   = CreateRectRgn( 0, 0, 0, 0 );
    if (!out_bits || !*clip || !run) goto fail;

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
                if(left != right)
                {
                    SetRectRgn( run, left, y, right, y + 1 );
                    CombineRgn( *clip, run, *clip, RGN_OR );
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
    DeleteObject( run );
    if (bits->free) bits->free( bits );

    bits->ptr     = out_bits;
    bits->is_copy = TRUE;
    bits->free    = free_heap_bits;

    return TRUE;

fail:
    if (run) DeleteObject( run );
    if (*clip) DeleteObject( *clip );
    HeapFree( GetProcessHeap(), 0, out_bits );
    return FALSE;
}



/* nulldrv fallback implementation using SetDIBits/StretchBlt */
INT nulldrv_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst, INT heightDst,
                           INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, const void *bits,
                           const BITMAPINFO *info, UINT coloruse, DWORD rop )
{
    DC *dc = get_nulldrv_dc( dev );
    INT ret;
    LONG width, height;
    WORD planes, bpp;
    DWORD compr, size;
    HBITMAP hBitmap;
    HDC hdcMem;

    /* make sure we have a real implementation for StretchBlt and PutImage */
    if (GET_DC_PHYSDEV( dc, pStretchBlt ) == dev || GET_DC_PHYSDEV( dc, pPutImage ) == dev)
        return 0;

    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &planes, &bpp, &compr, &size ) == -1)
        return 0;

    if (width < 0) return 0;

    if (xSrc == 0 && ySrc == 0 && widthDst == widthSrc && heightDst == heightSrc &&
        info->bmiHeader.biCompression == BI_RGB)
    {
        /* Windows appears to have a fast case optimization
         * that uses the wrong origin for top-down DIBs */
        if (height < 0 && heightSrc < abs(height)) ySrc = abs(height) - heightSrc;

        if (xDst == 0 && yDst == 0 && info->bmiHeader.biCompression == BI_RGB && rop == SRCCOPY)
        {
            BITMAP bm;
            hBitmap = GetCurrentObject( dev->hdc, OBJ_BITMAP );
            if (GetObjectW( hBitmap, sizeof(bm), &bm ) &&
                bm.bmWidth == widthSrc && bm.bmHeight == heightSrc &&
                bm.bmBitsPixel == bpp && bm.bmPlanes == planes)
            {
                /* fast path */
                return SetDIBits( dev->hdc, hBitmap, 0, abs( height ), bits, info, coloruse );
            }
        }
    }

    hdcMem = CreateCompatibleDC( dev->hdc );
    hBitmap = CreateCompatibleBitmap( dev->hdc, width, height );
    SelectObject( hdcMem, hBitmap );
    if (coloruse == DIB_PAL_COLORS)
        SelectPalette( hdcMem, GetCurrentObject( dev->hdc, OBJ_PAL ), FALSE );

    if (info->bmiHeader.biCompression == BI_RLE4 || info->bmiHeader.biCompression == BI_RLE8)
    {
        /* when RLE compression is used, there may be some gaps (ie the DIB doesn't
         * contain all the rectangle described in bmiHeader, but only part of it.
         * This mean that those undescribed pixels must be left untouched.
         * So, we first copy on a memory bitmap the current content of the
         * destination rectangle, blit the DIB bits on top of it - hence leaving
         * the gaps untouched -, and blitting the rectangle back.
         * This insure that gaps are untouched on the destination rectangle
         */
        StretchBlt( hdcMem, xSrc, abs(height) - heightSrc - ySrc, widthSrc, heightSrc,
                    dev->hdc, xDst, yDst, widthDst, heightDst, rop );
    }
    ret = SetDIBits( hdcMem, hBitmap, 0, abs( height ), bits, info, coloruse );
    if (ret) StretchBlt( dev->hdc, xDst, yDst, widthDst, heightDst,
                         hdcMem, xSrc, abs(height) - heightSrc - ySrc, widthSrc, heightSrc, rop );
    DeleteDC( hdcMem );
    DeleteObject( hBitmap );
    return ret;
}

/***********************************************************************
 *           StretchDIBits   (GDI32.@)
 */
INT WINAPI StretchDIBits(HDC hdc, INT xDst, INT yDst, INT widthDst, INT heightDst,
                         INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, const void *bits,
                         const BITMAPINFO *info, UINT coloruse, DWORD rop )
{
    DC *dc;
    INT ret = 0;

    if (!bits || !info) return 0;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pStretchDIBits );
        update_dc( dc );
        ret = physdev->funcs->pStretchDIBits( physdev, xDst, yDst, widthDst, heightDst,
                                              xSrc, ySrc, widthSrc, heightSrc, bits, info, coloruse, rop );
        release_dc_ptr( dc );
    }
    return ret;
}


/******************************************************************************
 * SetDIBits [GDI32.@]
 *
 * Sets pixels in a bitmap using colors from DIB.
 *
 * PARAMS
 *    hdc       [I] Handle to device context
 *    hbitmap   [I] Handle to bitmap
 *    startscan [I] Starting scan line
 *    lines     [I] Number of scan lines
 *    bits      [I] Array of bitmap bits
 *    info      [I] Address of structure with data
 *    coloruse  [I] Type of color indexes to use
 *
 * RETURNS
 *    Success: Number of scan lines copied
 *    Failure: 0
 */
INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
		      UINT lines, LPCVOID bits, const BITMAPINFO *info,
		      UINT coloruse )
{
    DC *dc = get_dc_ptr( hdc );
    BOOL delete_hdc = FALSE;
    PHYSDEV physdev;
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

    src_bits.ptr = (void *)bits;
    src_bits.is_copy = FALSE;
    src_bits.free = NULL;
    src_bits.param = NULL;

    if (coloruse == DIB_RGB_COLORS && !dc)
    {
        hdc = CreateCompatibleDC(0);
        dc = get_dc_ptr( hdc );
        delete_hdc = TRUE;
    }

    if (!dc) return 0;

    update_dc( dc );

    if (!(bitmap = GDI_GetObjPtr( hbitmap, OBJ_BITMAP )))
    {
        release_dc_ptr( dc );
        if (delete_hdc) DeleteDC(hdc);
        return 0;
    }

    if (!bitmapinfo_from_user_bitmapinfo( src_info, info, coloruse )) goto done;

    if (coloruse == DIB_PAL_COLORS)
        if (!fill_color_table_from_palette( src_info, dc )) goto done;

    if (src_info->bmiHeader.biCompression == BI_RLE4 || src_info->bmiHeader.biCompression == BI_RLE8)
    {
        if ( src_info->bmiHeader.biHeight < 0 ||
            (src_info->bmiHeader.biCompression == BI_RLE4 && src_info->bmiHeader.biBitCount != 4) ||
            (src_info->bmiHeader.biCompression == BI_RLE8 && src_info->bmiHeader.biBitCount != 8) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            goto done;
        }

        if (lines == 0) goto done;
        else lines = src_info->bmiHeader.biHeight;
        startscan = 0;

        if (!build_rle_bitmap( src_info, &src_bits, &clip )) goto done;
    }

    dst.visrect.left   = 0;
    dst.visrect.top    = 0;
    dst.visrect.right  = bitmap->bitmap.bmWidth;
    dst.visrect.bottom = bitmap->bitmap.bmHeight;

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

    physdev = GET_DC_PHYSDEV( dc, pPutImage );
    if (!BITMAP_SetOwnerDC( hbitmap, physdev )) goto done;

    result = lines;

    offset_rect( &src.visrect, 0, src_to_dst_offset );
    if (!intersect_rect( &dst.visrect, &src.visrect, &dst.visrect )) goto done;
    src.visrect = dst.visrect;
    offset_rect( &src.visrect, 0, -src_to_dst_offset );

    src.x      = src.visrect.left;
    src.y      = src.visrect.top;
    src.width  = src.visrect.right - src.visrect.left;
    src.height = src.visrect.bottom - src.visrect.top;

    dst.x      = dst.visrect.left;
    dst.y      = dst.visrect.top;
    dst.width  = dst.visrect.right - dst.visrect.left;
    dst.height = dst.visrect.bottom - dst.visrect.top;

    memcpy( dst_info, src_info, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ));

    err = physdev->funcs->pPutImage( physdev, hbitmap, clip, dst_info, &src_bits, &src, &dst, 0 );
    if (err == ERROR_BAD_FORMAT)
    {
        void *ptr;

        dst_info->bmiHeader.biWidth = dst.width;
        ptr = HeapAlloc( GetProcessHeap(), 0, get_dib_image_size( dst_info ));
        if (ptr)
        {
            err = convert_bitmapinfo( src_info, src_bits.ptr, &src.visrect, dst_info, ptr );
            {
                if (src_bits.free) src_bits.free( &src_bits );
                src_bits.ptr = ptr;
                src_bits.is_copy = TRUE;
                src_bits.free = free_heap_bits;
                if (!err)
                    err = physdev->funcs->pPutImage( physdev, hbitmap, clip, dst_info, &src_bits, &src, &dst, 0 );
            }
        }
        else err = ERROR_OUTOFMEMORY;
    }
    if(err) result = 0;

done:
    if (src_bits.free) src_bits.free( &src_bits );
    if (clip) DeleteObject( clip );
    GDI_ReleaseObj( hbitmap );
    release_dc_ptr( dc );
    if (delete_hdc) DeleteDC(hdc);
    return result;
}


/***********************************************************************
 *           SetDIBitsToDevice   (GDI32.@)
 */
INT WINAPI SetDIBitsToDevice(HDC hdc, INT xDest, INT yDest, DWORD cx,
                           DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                           UINT lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT coloruse )
{
    INT ret = 0;
    DC *dc;

    if (!bits) return 0;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDIBitsToDevice );
        update_dc( dc );
        ret = physdev->funcs->pSetDIBitsToDevice( physdev, xDest, yDest, cx, cy, xSrc,
                                                  ySrc, startscan, lines, bits, info, coloruse );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           SetDIBColorTable    (GDI32.@)
 */
UINT WINAPI SetDIBColorTable( HDC hdc, UINT startpos, UINT entries, CONST RGBQUAD *colors )
{
    DC * dc;
    UINT result = 0;
    BITMAPOBJ * bitmap;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((bitmap = GDI_GetObjPtr( dc->hBitmap, OBJ_BITMAP )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDIBColorTable );

        /* Check if currently selected bitmap is a DIB */
        if (bitmap->color_table)
        {
            if (startpos < bitmap->nb_colors)
            {
                if (startpos + entries > bitmap->nb_colors) entries = bitmap->nb_colors - startpos;
                memcpy(bitmap->color_table + startpos, colors, entries * sizeof(RGBQUAD));
                result = entries;
            }
        }
        GDI_ReleaseObj( dc->hBitmap );
        physdev->funcs->pSetDIBColorTable( physdev, startpos, entries, colors );
    }
    release_dc_ptr( dc );
    return result;
}


/***********************************************************************
 *           GetDIBColorTable    (GDI32.@)
 */
UINT WINAPI GetDIBColorTable( HDC hdc, UINT startpos, UINT entries, RGBQUAD *colors )
{
    DC * dc;
    BITMAPOBJ *bitmap;
    UINT result = 0;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((bitmap = GDI_GetObjPtr( dc->hBitmap, OBJ_BITMAP )))
    {
        /* Check if currently selected bitmap is a DIB */
        if (bitmap->color_table)
        {
            if (startpos < bitmap->nb_colors)
            {
                if (startpos + entries > bitmap->nb_colors) entries = bitmap->nb_colors - startpos;
                memcpy(colors, bitmap->color_table + startpos, entries * sizeof(RGBQUAD));
                result = entries;
            }
        }
        GDI_ReleaseObj( dc->hBitmap );
    }
    release_dc_ptr( dc );
    return result;
}

static const RGBQUAD DefLogPaletteQuads[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const DWORD bit_fields_888[3] = {0xff0000, 0x00ff00, 0x0000ff};
static const DWORD bit_fields_565[3] = {0xf800, 0x07e0, 0x001f};
static const DWORD bit_fields_555[3] = {0x7c00, 0x03e0, 0x001f};

static int fill_query_info( BITMAPINFO *info, BITMAPOBJ *bmp )
{
    BITMAPINFOHEADER header;

    header.biSize   = info->bmiHeader.biSize; /* Ensure we don't overwrite the original size when we copy back */
    header.biWidth  = bmp->bitmap.bmWidth;
    header.biHeight = bmp->bitmap.bmHeight;
    header.biPlanes = 1;

    if (bmp->dib)
    {
        header.biBitCount = bmp->dib->dsBm.bmBitsPixel;
        switch (bmp->dib->dsBm.bmBitsPixel)
        {
        case 16:
        case 32:
            header.biCompression = BI_BITFIELDS;
            break;
        default:
            header.biCompression = BI_RGB;
            break;
        }
    }
    else
    {
        header.biCompression = (bmp->bitmap.bmBitsPixel > 8) ? BI_BITFIELDS : BI_RGB;
        header.biBitCount = bmp->bitmap.bmBitsPixel;
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

    return abs(bmp->bitmap.bmHeight);
}

/************************************************************************
 *      copy_color_info
 *
 * Copy BITMAPINFO color information where dst may be a BITMAPCOREINFO.
 */
static void copy_color_info(BITMAPINFO *dst, const BITMAPINFO *src, UINT coloruse)
{
    unsigned int colors = src->bmiHeader.biBitCount > 8 ? 0 : 1 << src->bmiHeader.biBitCount;
    RGBQUAD *src_colors = (RGBQUAD *)((char *)src + src->bmiHeader.biSize);

    assert( src->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) );

    if (src->bmiHeader.biClrUsed) colors = src->bmiHeader.biClrUsed;

    if (dst->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREINFO *core = (BITMAPCOREINFO *)dst;
        if (coloruse == DIB_PAL_COLORS)
            memcpy( core->bmciColors, src_colors, colors * sizeof(WORD) );
        else
        {
            unsigned int i;
            for (i = 0; i < colors; i++)
            {
                core->bmciColors[i].rgbtRed   = src_colors[i].rgbRed;
                core->bmciColors[i].rgbtGreen = src_colors[i].rgbGreen;
                core->bmciColors[i].rgbtBlue  = src_colors[i].rgbBlue;
            }
        }
    }
    else
    {
        dst->bmiHeader.biClrUsed   = src->bmiHeader.biClrUsed;
        dst->bmiHeader.biSizeImage = src->bmiHeader.biSizeImage;

        if (src->bmiHeader.biCompression == BI_BITFIELDS)
            /* bitfields are always at bmiColors even in larger structures */
            memcpy( dst->bmiColors, src->bmiColors, 3 * sizeof(DWORD) );
        else if (colors)
        {
            void *colorptr = (char *)dst + dst->bmiHeader.biSize;
            unsigned int size;

            if (coloruse == DIB_PAL_COLORS)
                size = colors * sizeof(WORD);
            else
                size = colors * sizeof(RGBQUAD);
            memcpy( colorptr, src_colors, size );
        }
    }
}

static void fill_default_color_table( BITMAPINFO *info )
{
    int i;

    switch (info->bmiHeader.biBitCount)
    {
    case 1:
        info->bmiColors[0].rgbRed = info->bmiColors[0].rgbGreen = info->bmiColors[0].rgbBlue = 0;
        info->bmiColors[0].rgbReserved = 0;
        info->bmiColors[1].rgbRed = info->bmiColors[1].rgbGreen = info->bmiColors[1].rgbBlue = 0xff;
        info->bmiColors[1].rgbReserved = 0;
        break;

    case 4:
        /* The EGA palette is the first and last 8 colours of the default palette
           with the innermost pair swapped */
        memcpy(info->bmiColors,     DefLogPaletteQuads,      7 * sizeof(RGBQUAD));
        memcpy(info->bmiColors + 7, DefLogPaletteQuads + 12, 1 * sizeof(RGBQUAD));
        memcpy(info->bmiColors + 8, DefLogPaletteQuads +  7, 1 * sizeof(RGBQUAD));
        memcpy(info->bmiColors + 9, DefLogPaletteQuads + 13, 7 * sizeof(RGBQUAD));
        break;

    case 8:
        memcpy(info->bmiColors, DefLogPaletteQuads, 10 * sizeof(RGBQUAD));
        memcpy(info->bmiColors + 246, DefLogPaletteQuads + 10, 10 * sizeof(RGBQUAD));
        for (i = 10; i < 246; i++)
        {
            info->bmiColors[i].rgbRed      = (i & 0x07) << 5;
            info->bmiColors[i].rgbGreen    = (i & 0x38) << 2;
            info->bmiColors[i].rgbBlue     =  i & 0xc0;
            info->bmiColors[i].rgbReserved = 0;
        }
        break;

    default:
        ERR("called with bitcount %d\n", info->bmiHeader.biBitCount);
    }
}

/******************************************************************************
 * GetDIBits [GDI32.@]
 *
 * Retrieves bits of bitmap and copies to buffer.
 *
 * RETURNS
 *    Success: Number of scan lines copied from bitmap
 *    Failure: 0
 */
INT WINAPI GetDIBits(
    HDC hdc,         /* [in]  Handle to device context */
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    UINT startscan,  /* [in]  First scan line to set in dest bitmap */
    UINT lines,      /* [in]  Number of scan lines to copy */
    LPVOID bits,       /* [out] Address of array for bitmap bits */
    BITMAPINFO * info, /* [out] Address of structure with bitmap data */
    UINT coloruse)   /* [in]  RGB or palette index */
{
    DC * dc;
    BITMAPOBJ * bmp;
    int i;
    int bitmap_type;
    LONG width;
    LONG height;
    WORD planes, bpp;
    DWORD compr, size;
    char dst_bmibuf[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_bmibuf;
    unsigned int colors = 0;

    if (!info) return 0;

    bitmap_type = DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &planes, &bpp, &compr, &size);
    if (bitmap_type == -1)
    {
        ERR("Invalid bitmap format\n");
        return 0;
    }
    if (!(dc = get_dc_ptr( hdc )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    update_dc( dc );
    if (!(bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP )))
    {
        release_dc_ptr( dc );
	return 0;
    }


    if (bpp == 0) /* query bitmap info only */
    {
        lines = fill_query_info( info, bmp );
        goto done;
    }

    /* Since info may be a BITMAPCOREINFO or any of the larger BITMAPINFO structures, we'll use our
       own copy and transfer the colour info back at the end */

    dst_info->bmiHeader.biSize          = sizeof(dst_info->bmiHeader);
    dst_info->bmiHeader.biWidth         = width;
    dst_info->bmiHeader.biHeight        = height;
    dst_info->bmiHeader.biPlanes        = planes;
    dst_info->bmiHeader.biBitCount      = bpp;
    dst_info->bmiHeader.biCompression   = compr;
    dst_info->bmiHeader.biSizeImage     = get_dib_image_size( dst_info );
    dst_info->bmiHeader.biXPelsPerMeter = 0;
    dst_info->bmiHeader.biYPelsPerMeter = 0;
    dst_info->bmiHeader.biClrUsed       = 0;
    dst_info->bmiHeader.biClrImportant  = 0;

    switch (bpp)
    {
    case 1:
    case 4:
    case 8:
    {
        colors = 1 << bpp;

        /* If the bitmap object is a dib section at the
           same color depth then get the color map from it */
        if (bmp->dib && bpp == bmp->dib->dsBm.bmBitsPixel && coloruse == DIB_RGB_COLORS)
        {
            colors = min( colors, bmp->nb_colors );
            if (colors != 1 << bpp) dst_info->bmiHeader.biClrUsed = colors;
            memcpy( dst_info->bmiColors, bmp->color_table, colors * sizeof(RGBQUAD) );
        }

        /* For color DDBs in native depth (mono DDBs always have a black/white palette):
           Generate the color map from the selected palette.  In the DIB_PAL_COLORS
           case we'll fix up the indices after the format conversion. */
        else if ( (bpp > 1 && bpp == bmp->bitmap.bmBitsPixel) || coloruse == DIB_PAL_COLORS )
        {
            if (!fill_color_table_from_palette( dst_info, dc ))
            {
                lines = 0;
                goto done;
            }
        }
        else
            fill_default_color_table( dst_info );
        break;
    }

    case 15:
        if (dst_info->bmiHeader.biCompression == BI_BITFIELDS)
            memcpy( dst_info->bmiColors, bit_fields_555, sizeof(bit_fields_555) );
        break;

    case 16:
        if (dst_info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (bmp->dib)
            {
                if (bmp->dib->dsBmih.biCompression == BI_BITFIELDS && bmp->dib->dsBmih.biBitCount == bpp)
                    memcpy( dst_info->bmiColors, bmp->dib->dsBitfields, 3 * sizeof(DWORD) );
                else
                    memcpy( dst_info->bmiColors, bit_fields_555, sizeof(bit_fields_555) );
            }
            else
                memcpy( dst_info->bmiColors, bit_fields_565, sizeof(bit_fields_565) );
        }
        break;

    case 24:
    case 32:
        if (dst_info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (bmp->dib && bmp->dib->dsBmih.biCompression == BI_BITFIELDS && bmp->dib->dsBmih.biBitCount == bpp)
                memcpy( dst_info->bmiColors, bmp->dib->dsBitfields, 3 * sizeof(DWORD) );
            else
                memcpy( dst_info->bmiColors, bit_fields_888, sizeof(bit_fields_888) );
        }
        break;
    }

    if (bits && lines)
    {
        PHYSDEV physdev;
        char src_bmibuf[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *src_info = (BITMAPINFO *)src_bmibuf;
        struct gdi_image_bits src_bits;
        struct bitblt_coords src;
        DWORD err;

        /* FIXME: will need updating once the dib driver has pGetImage. */
        physdev = GET_DC_PHYSDEV( dc, pGetImage );

        if (!BITMAP_SetOwnerDC( hbitmap, physdev )) lines = 0;

        src.visrect.left = 0;
        src.visrect.right = min( width, bmp->bitmap.bmWidth );

        if (startscan >= bmp->bitmap.bmHeight)                       /* constrain lines to within src bitmap */
            lines = 0;
        else
            lines = min( lines, bmp->bitmap.bmHeight - startscan );
        lines = min( lines, abs(height) );                           /* and constrain to within dest bitmap */

        if (lines == 0) goto done;

        src.visrect.bottom = bmp->bitmap.bmHeight - startscan;
        src.visrect.top = src.visrect.bottom - lines;
        src.x = src.visrect.left;
        src.y = src.visrect.top;
        src.width = src.visrect.right - src.visrect.left;
        src.height = src.visrect.bottom - src.visrect.top;

        err = physdev->funcs->pGetImage( physdev, hbitmap, src_info, &src_bits, &src );

        if(err)
        {
            lines = 0;
            goto done;
        }

        if (src_info->bmiHeader.biBitCount <= 8 && src_info->bmiHeader.biClrUsed == 0)
        {
            if (bmp->dib)
                memcpy( src_info->bmiColors, bmp->color_table, bmp->nb_colors * sizeof(RGBQUAD) );
            else
                fill_default_color_table( src_info );
        }

        if(dst_info->bmiHeader.biHeight > 0)
            dst_info->bmiHeader.biHeight = lines;
        else
            dst_info->bmiHeader.biHeight = -lines;

        convert_bitmapinfo( src_info, src_bits.ptr, &src.visrect, dst_info, bits );
        if (src_bits.free) src_bits.free( &src_bits );
    }
    else lines = abs(height);

    if (coloruse == DIB_PAL_COLORS)
    {
        WORD *index = (WORD *)dst_info->bmiColors;
        for (i = 0; i < colors; i++, index++)
            *index = i;
    }

    copy_color_info( info, dst_info, coloruse );

done:
    release_dc_ptr( dc );
    GDI_ReleaseObj( hbitmap );
    return lines;
}


/***********************************************************************
 *           CreateDIBitmap    (GDI32.@)
 *
 * Creates a DDB (device dependent bitmap) from a DIB.
 * The DDB will have the same color depth as the reference DC.
 */
HBITMAP WINAPI CreateDIBitmap( HDC hdc, const BITMAPINFOHEADER *header,
                            DWORD init, LPCVOID bits, const BITMAPINFO *data,
                            UINT coloruse )
{
    HBITMAP handle;
    LONG width;
    LONG height;
    WORD planes, bpp;
    DWORD compr, size;

    if (!header) return 0;

    if (DIB_GetBitmapInfo( header, &width, &height, &planes, &bpp, &compr, &size ) == -1) return 0;
    
    if (width < 0)
    {
        TRACE("Bitmap has a negative width\n");
        return 0;
    }
    
    /* Top-down DIBs have a negative height */
    if (height < 0) height = -height;

    TRACE("hdc=%p, header=%p, init=%u, bits=%p, data=%p, coloruse=%u (bitmap: width=%d, height=%d, bpp=%u, compr=%u)\n",
           hdc, header, init, bits, data, coloruse, width, height, bpp, compr);
    
    if (hdc == NULL)
        handle = CreateBitmap( width, height, 1, 1, NULL );
    else
        handle = CreateCompatibleBitmap( hdc, width, height );

    if (handle)
    {
        if (init & CBM_INIT)
        {
            if (SetDIBits( hdc, handle, 0, height, bits, data, coloruse ) == 0)
            {
                DeleteObject( handle );
                handle = 0;
            }
        }
    }

    return handle;
}

/* Copy/synthesize RGB palette from BITMAPINFO. Ripped from dlls/winex11.drv/dib.c */
static void DIB_CopyColorTable( DC *dc, BITMAPOBJ *bmp, WORD coloruse, const BITMAPINFO *info )
{
    RGBQUAD *colorTable;
    unsigned int colors, i;
    BOOL core_info = info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER);

    if (core_info)
    {
        colors = 1 << ((const BITMAPCOREINFO*) info)->bmciHeader.bcBitCount;
    }
    else
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors) colors = 1 << info->bmiHeader.biBitCount;
    }

    if (colors > 256) {
        ERR("called with >256 colors!\n");
        return;
    }

    if (!(colorTable = HeapAlloc(GetProcessHeap(), 0, colors * sizeof(RGBQUAD) ))) return;

    if(coloruse == DIB_RGB_COLORS)
    {
        if (core_info)
        {
           /* Convert RGBTRIPLEs to RGBQUADs */
           for (i=0; i < colors; i++)
           {
               colorTable[i].rgbRed   = ((const BITMAPCOREINFO*) info)->bmciColors[i].rgbtRed;
               colorTable[i].rgbGreen = ((const BITMAPCOREINFO*) info)->bmciColors[i].rgbtGreen;
               colorTable[i].rgbBlue  = ((const BITMAPCOREINFO*) info)->bmciColors[i].rgbtBlue;
               colorTable[i].rgbReserved = 0;
           }
        }
        else
        {
            memcpy(colorTable, (const BYTE*) info + (WORD) info->bmiHeader.biSize, colors * sizeof(RGBQUAD));
        }
    }
    else
    {
        PALETTEENTRY entries[256];
        const WORD *index = (const WORD*) ((const BYTE*) info + (WORD) info->bmiHeader.biSize);
        UINT count = GetPaletteEntries( dc->hPalette, 0, colors, entries );

        for (i = 0; i < colors; i++, index++)
        {
            PALETTEENTRY *entry = &entries[*index % count];
            colorTable[i].rgbRed = entry->peRed;
            colorTable[i].rgbGreen = entry->peGreen;
            colorTable[i].rgbBlue = entry->peBlue;
            colorTable[i].rgbReserved = 0;
        }
    }
    bmp->color_table = colorTable;
    bmp->nb_colors = colors;
}

/***********************************************************************
 *           CreateDIBSection    (GDI32.@)
 */
HBITMAP WINAPI CreateDIBSection(HDC hdc, CONST BITMAPINFO *bmi, UINT usage,
                                VOID **bits, HANDLE section, DWORD offset)
{
    HBITMAP ret = 0;
    DC *dc;
    BOOL bDesktopDC = FALSE;
    DIBSECTION *dib;
    BITMAPOBJ *bmp;
    int bitmap_type;
    LONG width, height;
    WORD planes, bpp;
    DWORD compression, sizeImage;
    void *mapBits = NULL;

    if(!bmi){
        if(bits) *bits = NULL;
        return NULL;
    }

    if (((bitmap_type = DIB_GetBitmapInfo( &bmi->bmiHeader, &width, &height,
                                           &planes, &bpp, &compression, &sizeImage )) == -1))
        return 0;

    switch (bpp)
    {
    case 16:
    case 32:
        if (compression == BI_BITFIELDS) break;
        /* fall through */
    case 1:
    case 4:
    case 8:
    case 24:
        if (compression == BI_RGB) break;
        /* fall through */
    default:
        WARN( "invalid %u bpp compression %u\n", bpp, compression );
        return 0;
    }

    if (!(dib = HeapAlloc( GetProcessHeap(), 0, sizeof(*dib) ))) return 0;

    TRACE("format (%d,%d), planes %d, bpp %d, %s, size %d %s\n",
          width, height, planes, bpp, compression == BI_BITFIELDS? "BI_BITFIELDS" : "BI_RGB",
          sizeImage, usage == DIB_PAL_COLORS? "PAL" : "RGB");

    dib->dsBm.bmType       = 0;
    dib->dsBm.bmWidth      = width;
    dib->dsBm.bmHeight     = height >= 0 ? height : -height;
    dib->dsBm.bmWidthBytes = get_dib_stride( width, bpp );
    dib->dsBm.bmPlanes     = planes;
    dib->dsBm.bmBitsPixel  = bpp;
    dib->dsBm.bmBits       = NULL;

    if (!bitmap_type)  /* core header */
    {
        /* convert the BITMAPCOREHEADER to a BITMAPINFOHEADER */
        dib->dsBmih.biSize = sizeof(BITMAPINFOHEADER);
        dib->dsBmih.biWidth = width;
        dib->dsBmih.biHeight = height;
        dib->dsBmih.biPlanes = planes;
        dib->dsBmih.biBitCount = bpp;
        dib->dsBmih.biCompression = compression;
        dib->dsBmih.biXPelsPerMeter = 0;
        dib->dsBmih.biYPelsPerMeter = 0;
        dib->dsBmih.biClrUsed = 0;
        dib->dsBmih.biClrImportant = 0;
    }
    else
    {
        /* truncate extended bitmap headers (BITMAPV4HEADER etc.) */
        dib->dsBmih = bmi->bmiHeader;
        dib->dsBmih.biSize = sizeof(BITMAPINFOHEADER);
    }

    /* set number of entries in bmi.bmiColors table */
    if( bpp <= 8 )
        dib->dsBmih.biClrUsed = 1 << bpp;

    dib->dsBmih.biSizeImage = dib->dsBm.bmWidthBytes * dib->dsBm.bmHeight;

    /* set dsBitfields values */
    dib->dsBitfields[0] = dib->dsBitfields[1] = dib->dsBitfields[2] = 0;

    if((bpp == 15 || bpp == 16) && compression == BI_RGB)
    {
        /* In this case Windows changes biCompression to BI_BITFIELDS,
           however for now we won't do this, as there are a lot
           of places where BI_BITFIELDS is currently unsupported. */

        /* dib->dsBmih.biCompression = compression = BI_BITFIELDS;*/
        dib->dsBitfields[0] = 0x7c00;
        dib->dsBitfields[1] = 0x03e0;
        dib->dsBitfields[2] = 0x001f;
    }
    else if(compression == BI_BITFIELDS)
    {
        dib->dsBitfields[0] =  *(const DWORD *)bmi->bmiColors;
        dib->dsBitfields[1] =  *((const DWORD *)bmi->bmiColors + 1);
        dib->dsBitfields[2] =  *((const DWORD *)bmi->bmiColors + 2);
    }

    /* get storage location for DIB bits */

    if (section)
    {
        SYSTEM_INFO SystemInfo;
        DWORD mapOffset;
        INT mapSize;

        GetSystemInfo( &SystemInfo );
        mapOffset = offset - (offset % SystemInfo.dwAllocationGranularity);
        mapSize = dib->dsBmih.biSizeImage + (offset - mapOffset);
        mapBits = MapViewOfFile( section, FILE_MAP_ALL_ACCESS, 0, mapOffset, mapSize );
        if (mapBits) dib->dsBm.bmBits = (char *)mapBits + (offset - mapOffset);
    }
    else
    {
        offset = 0;
        dib->dsBm.bmBits = VirtualAlloc( NULL, dib->dsBmih.biSizeImage,
                                         MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );
    }
    dib->dshSection = section;
    dib->dsOffset = offset;

    if (!dib->dsBm.bmBits)
    {
        HeapFree( GetProcessHeap(), 0, dib );
        return 0;
    }

    /* If the reference hdc is null, take the desktop dc */
    if (hdc == 0)
    {
        hdc = CreateCompatibleDC(0);
        bDesktopDC = TRUE;
    }

    if (!(dc = get_dc_ptr( hdc ))) goto error;

    /* create Device Dependent Bitmap and add DIB pointer */
    ret = CreateBitmap( dib->dsBm.bmWidth, dib->dsBm.bmHeight, 1,
                        (bpp == 1) ? 1 : GetDeviceCaps(hdc, BITSPIXEL), NULL );

    if (ret && ((bmp = GDI_GetObjPtr(ret, OBJ_BITMAP))))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pCreateDIBSection );
        bmp->dib = dib;
        bmp->funcs = physdev->funcs;
        /* create local copy of DIB palette */
        if (bpp <= 8) DIB_CopyColorTable( dc, bmp, usage, bmi );
        GDI_ReleaseObj( ret );

        if (!physdev->funcs->pCreateDIBSection( physdev, ret, bmi, usage ))
        {
            DeleteObject( ret );
            ret = 0;
        }
    }

    release_dc_ptr( dc );
    if (bDesktopDC) DeleteDC( hdc );
    if (ret && bits) *bits = dib->dsBm.bmBits;
    return ret;

error:
    if (bDesktopDC) DeleteDC( hdc );
    if (section) UnmapViewOfFile( mapBits );
    else if (!offset) VirtualFree( dib->dsBm.bmBits, 0, MEM_RELEASE );
    HeapFree( GetProcessHeap(), 0, dib );
    return 0;
}

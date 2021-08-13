/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
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

#include <string.h>

#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

/***********************************************************************
 *           METADC_PatBlt
 */
BOOL METADC_PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    return metadc_param6( hdc, META_PATBLT, left, top, width, height,
                          HIWORD(rop), LOWORD(rop) );
}


static BOOL metadc_stretchblt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                               HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                               DWORD rop, WORD type )
{
    BITMAPINFO src_info = {{ sizeof( src_info.bmiHeader ) }};
    UINT bmi_size, size, bpp;
    int i = 0, bitmap_offset;
    BITMAPINFO *bmi;
    METARECORD *mr;
    HBITMAP bitmap;
    BOOL ret;

    if (!(bitmap = GetCurrentObject( hdc_src, OBJ_BITMAP ))) return FALSE;
    if (!GetDIBits( hdc_src, bitmap, 0, INT_MAX, NULL, &src_info, DIB_RGB_COLORS )) return FALSE;

    bpp = src_info.bmiHeader.biBitCount;
    if (bpp <= 8)
        bmi_size = sizeof(BITMAPINFOHEADER) + (1 << bpp) * sizeof(RGBQUAD);
    else if (bpp == 16 || bpp == 32)
        bmi_size = sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD);
    else
        bmi_size = sizeof(BITMAPINFOHEADER);

    bitmap_offset = type == META_DIBBITBLT ? 8 : 10;
    size = FIELD_OFFSET( METARECORD, rdParm[bitmap_offset] ) + bmi_size +
        src_info.bmiHeader.biSizeImage;
    if (!(mr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    mr->rdFunction = type;
    bmi = (BITMAPINFO *)&mr->rdParm[bitmap_offset];
    bmi->bmiHeader = src_info.bmiHeader;
    TRACE( "size = %u  rop=%x\n", size, rop );

    ret = GetDIBits( hdc_src, bitmap, 0, src_info.bmiHeader.biHeight, (BYTE *)bmi + bmi_size,
                     bmi, DIB_RGB_COLORS );
    if (ret)
    {
        mr->rdSize = size / sizeof(WORD);
        mr->rdParm[i++] = LOWORD(rop);
        mr->rdParm[i++] = HIWORD(rop);
        if (bitmap_offset > 8)
        {
            mr->rdParm[i++] = height_src;
            mr->rdParm[i++] = width_src;
        }
        mr->rdParm[i++] = y_src;
        mr->rdParm[i++] = x_src;
        mr->rdParm[i++] = height_dst;
        mr->rdParm[i++] = width_dst;
        mr->rdParm[i++] = y_dst;
        mr->rdParm[i++] = x_dst;
        ret = metadc_record( hdc, mr, size );
    }

    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


BOOL METADC_BitBlt( HDC hdc, INT x_dst, INT y_dst, INT width, INT height,
                    HDC hdc_src, INT x_src, INT y_src, DWORD rop )
{
    return metadc_stretchblt( hdc, x_dst, y_dst, width, height,
                              hdc_src, x_src, y_src, width, height, rop, META_DIBBITBLT );
}

/***********************************************************************
 *           METADC_StretchBlt
 */
BOOL METADC_StretchBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                        HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                        DWORD rop )
{
    return metadc_stretchblt( hdc_dst, x_dst, y_dst, width_dst, height_dst,
                              hdc_src, x_src, y_src, width_src, height_src, rop, META_DIBSTRETCHBLT );
}

/***********************************************************************
 *           METADC_StretchDIBits
 */
INT METADC_StretchDIBits( HDC hdc, INT x_dst, INT y_dst, INT width_dst,
                          INT height_dst, INT x_src, INT y_src, INT width_src,
                          INT height_src, const void *bits,
                          const BITMAPINFO *info, UINT usage, DWORD rop )
{
    DWORD infosize = get_dib_info_size( info, usage );
    DWORD len = sizeof(METARECORD) + 10 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr = HeapAlloc( GetProcessHeap(), 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_STRETCHDIB;
    mr->rdParm[0] = LOWORD(rop);
    mr->rdParm[1] = HIWORD(rop);
    mr->rdParm[2] = usage;
    mr->rdParm[3] = height_src;
    mr->rdParm[4] = width_src;
    mr->rdParm[5] = y_src;
    mr->rdParm[6] = x_src;
    mr->rdParm[7] = height_dst;
    mr->rdParm[8] = width_dst;
    mr->rdParm[9] = y_dst;
    mr->rdParm[10] = x_dst;
    memcpy(mr->rdParm + 11, info, infosize);
    memcpy(mr->rdParm + 11 + infosize / 2, bits, info->bmiHeader.biSizeImage);
    metadc_record( hdc, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return height_src;
}


/***********************************************************************
 *           METADC_SetDIBitsToDevice
 */
INT METADC_SetDIBitsToDevice( HDC hdc, INT x_dst, INT y_dst, DWORD width, DWORD height,
                              INT x_src, INT y_src, UINT startscan, UINT lines,
                              const void *bits, const BITMAPINFO *info, UINT coloruse )

{
    DWORD infosize = get_dib_info_size(info, coloruse);
    DWORD len = sizeof(METARECORD) + 8 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr = HeapAlloc( GetProcessHeap(), 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_SETDIBTODEV;
    mr->rdParm[0] = coloruse;
    mr->rdParm[1] = lines;
    mr->rdParm[2] = startscan;
    mr->rdParm[3] = y_src;
    mr->rdParm[4] = x_src;
    mr->rdParm[5] = height;
    mr->rdParm[6] = width;
    mr->rdParm[7] = y_dst;
    mr->rdParm[8] = x_dst;
    memcpy(mr->rdParm + 9, info, infosize);
    memcpy(mr->rdParm + 9 + infosize / 2, bits, info->bmiHeader.biSizeImage);
    metadc_record( hdc, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return lines;
}

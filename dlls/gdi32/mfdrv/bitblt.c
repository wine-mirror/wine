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
                               DWORD rop )
{
    BITMAPINFO src_info = {{ sizeof( src_info.bmiHeader ) }};
    UINT bmi_size, size, bpp;
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

    size = FIELD_OFFSET( METARECORD, rdParm[10] ) + bmi_size +
        src_info.bmiHeader.biSizeImage;
    if (!(mr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    mr->rdFunction = META_DIBSTRETCHBLT;
    bmi = (BITMAPINFO *)&mr->rdParm[10];
    bmi->bmiHeader = src_info.bmiHeader;
    TRACE( "size = %u  rop=%x\n", size, rop );

    ret = GetDIBits( hdc_src, bitmap, 0, src_info.bmiHeader.biHeight, (BYTE *)bmi + bmi_size,
                     bmi, DIB_RGB_COLORS );
    if (ret)
    {
        mr->rdSize = size / sizeof(WORD);
        mr->rdParm[0] = LOWORD(rop);
        mr->rdParm[1] = HIWORD(rop);
        mr->rdParm[2] = height_src;
        mr->rdParm[3] = width_src;
        mr->rdParm[4] = y_src;
        mr->rdParm[5] = x_src;
        mr->rdParm[6] = height_dst;
        mr->rdParm[7] = width_dst;
        mr->rdParm[8] = y_dst;
        mr->rdParm[9] = x_dst;
        ret = metadc_record( hdc, mr, size );
    }

    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


/***********************************************************************
 *           MFDRV_StretchBlt
 */
BOOL CDECL MFDRV_StretchBlt( PHYSDEV devDst, struct bitblt_coords *dst,
                             PHYSDEV devSrc, struct bitblt_coords *src, DWORD rop )
{
    return metadc_stretchblt( devDst->hdc, dst->log_x, dst->log_y, dst->log_width, dst->log_height,
                              devSrc->hdc, src->log_x, src->log_y, src->log_width, src->log_height,
                              rop );
}

/***********************************************************************
 *           MFDRV_StretchDIBits
 */
INT CDECL MFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                               INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
                               INT heightSrc, const void *bits,
                               BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DWORD infosize = get_dib_info_size(info, wUsage);
    DWORD len = sizeof(METARECORD) + 10 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr = HeapAlloc( GetProcessHeap(), 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_STRETCHDIB;
    mr->rdParm[0] = LOWORD(dwRop);
    mr->rdParm[1] = HIWORD(dwRop);
    mr->rdParm[2] = wUsage;
    mr->rdParm[3] = (INT16)heightSrc;
    mr->rdParm[4] = (INT16)widthSrc;
    mr->rdParm[5] = (INT16)ySrc;
    mr->rdParm[6] = (INT16)xSrc;
    mr->rdParm[7] = (INT16)heightDst;
    mr->rdParm[8] = (INT16)widthDst;
    mr->rdParm[9] = (INT16)yDst;
    mr->rdParm[10] = (INT16)xDst;
    memcpy(mr->rdParm + 11, info, infosize);
    memcpy(mr->rdParm + 11 + infosize / 2, bits, info->bmiHeader.biSizeImage);
    MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return heightSrc;
}


/***********************************************************************
 *           MFDRV_SetDIBitsToDevice
 */
INT CDECL MFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDst, INT yDst, DWORD cx,
                                   DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                                   UINT lines, LPCVOID bits, BITMAPINFO *info, UINT coloruse )

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
    mr->rdParm[3] = (INT16)ySrc;
    mr->rdParm[4] = (INT16)xSrc;
    mr->rdParm[5] = (INT16)cy;
    mr->rdParm[6] = (INT16)cx;
    mr->rdParm[7] = (INT16)yDst;
    mr->rdParm[8] = (INT16)xDst;
    memcpy(mr->rdParm + 9, info, infosize);
    memcpy(mr->rdParm + 9 + infosize / 2, bits, info->bmiHeader.biSizeImage);
    MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return lines;
}

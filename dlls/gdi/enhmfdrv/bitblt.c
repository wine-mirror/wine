/*
 * Enhanced MetaFile driver BitBlt functions
 *
 * Copyright 2002 Huw D M Davies for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi.h"
#include "enhmetafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

BOOL EMFDRV_PatBlt( PHYSDEV dev, INT left, INT top,
                    INT width, INT height, DWORD rop )
{
    EMRBITBLT emr;
    BOOL ret;

    emr.emr.iType = EMR_BITBLT;
    emr.emr.nSize = sizeof(emr);
    emr.rclBounds.left = left;
    emr.rclBounds.top = top;
    emr.rclBounds.right = left + width - 1;
    emr.rclBounds.bottom = top + height - 1;
    emr.xDest = left;
    emr.yDest = top;
    emr.cxDest = width;
    emr.cyDest = height;
    emr.dwRop = rop;
    emr.xSrc = 0;
    emr.ySrc = 0;
    emr.xformSrc.eM11 = 1.0;
    emr.xformSrc.eM12 = 0.0;
    emr.xformSrc.eM21 = 0.0;
    emr.xformSrc.eM22 = 1.0;
    emr.xformSrc.eDx = 0.0;
    emr.xformSrc.eDy = 0.0;
    emr.crBkColorSrc = 0;
    emr.iUsageSrc = 0;
    emr.offBmiSrc = 0;
    emr.cbBmiSrc = 0;
    emr.offBitsSrc = 0;
    emr.cbBitsSrc = 0;

    ret = EMFDRV_WriteRecord( dev, &emr.emr );
    if(ret)
        EMFDRV_UpdateBBox( dev, &emr.rclBounds );
    return ret;
}


INT EMFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                                      INT heightDst, INT xSrc, INT ySrc,
                                      INT widthSrc, INT heightSrc,
                                      const void *bits, const BITMAPINFO *info,
                                      UINT wUsage, DWORD dwRop )
{
    EMRSTRETCHDIBITS *emr;
    BOOL ret;
    UINT bmi_size=0, bits_size, emr_size;
    UINT width_bytes;

    /* calculate the size of the bits we need to store */
    switch(info->bmiHeader.biBitCount)
    {
    case 1:
         width_bytes = (info->bmiHeader.biWidth+7)/8;
         break;
    case 4:
         width_bytes = (info->bmiHeader.biWidth+1)/2;
         break;
    case 8:
         width_bytes = info->bmiHeader.biWidth;
         break;
    case 16:
         width_bytes = info->bmiHeader.biWidth*2;
         break;
    case 24:
         width_bytes = info->bmiHeader.biWidth*3;
         break;
    case 32:
         width_bytes = info->bmiHeader.biWidth*4;
         break;
    default:
         FIXME("bi.biCount has and unknown value (%d)\n", info->bmiHeader.biBitCount);
         return FALSE;
    }
    width_bytes = ((width_bytes+3)/4) << 2;
    bits_size = width_bytes * info->bmiHeader.biHeight;

    /* calculate the size of the colour table */
    bmi_size = sizeof (BITMAPINFO);
    if ( info->bmiHeader.biBitCount <= 8 )
    {
         UINT num_colors = info->bmiHeader.biClrUsed;
         if ( num_colors == 0 )
              num_colors = (1<<info->bmiHeader.biBitCount);
         bmi_size += num_colors*sizeof(RGBQUAD);
    }

    emr_size = sizeof (EMRSTRETCHDIBITS) + bmi_size + bits_size;
    emr = HeapAlloc(GetProcessHeap(), 0, emr_size );

    /* write a bitmap info header (with colours) to the record */
    memcpy( &emr[1], info, bmi_size);

    /* write bitmap bits to the record */
    memcpy ( ( (BYTE *) (&emr[1]) ) + bmi_size, bits, bits_size);

    /* fill in the EMR header at the front of our piece of memory */
    emr->emr.iType = EMR_STRETCHDIBITS;
    emr->emr.nSize = emr_size;

    emr->xDest     = xDst;
    emr->yDest     = yDst;
    emr->cxDest    = widthDst;
    emr->cyDest    = heightDst;
    emr->dwRop     = dwRop;
    emr->xSrc      = xSrc; /* FIXME: only save the piece of the bitmap needed */
    emr->ySrc      = ySrc;

    emr->iUsageSrc    = DIB_RGB_COLORS;
    emr->offBmiSrc    = sizeof (EMRSTRETCHDIBITS);
    emr->cbBmiSrc     = bmi_size;
    emr->offBitsSrc   = emr->offBmiSrc + bmi_size; 
    emr->cbBitsSrc    = bits_size;

    emr->cxSrc = widthSrc;
    emr->cySrc = heightSrc;

    emr->rclBounds.left   = xDst;
    emr->rclBounds.top    = yDst;
    emr->rclBounds.right  = xDst + widthDst;
    emr->rclBounds.bottom = yDst + heightDst;

    /* save the record we just created */
    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dev, &emr->rclBounds );

    HeapFree(GetProcessHeap(), 0, emr);

    return ret;
}

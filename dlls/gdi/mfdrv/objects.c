/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap.h"
#include "wownt32.h"
#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);


/***********************************************************************
 *           MFDRV_SelectBitmap
 */
HBITMAP MFDRV_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    return 0;
}


/******************************************************************
 *         MFDRV_CreateBrushIndirect
 */

INT16 MFDRV_CreateBrushIndirect(PHYSDEV dev, HBRUSH hBrush )
{
    INT16 index = -1;
    DWORD size;
    METARECORD *mr;
    LOGBRUSH logbrush;
    METAFILEDRV_PDEVICE *physDev = (METAFILEDRV_PDEVICE *)dev;

    if (!GetObjectA( hBrush, sizeof(logbrush), &logbrush )) return -1;

    switch(logbrush.lbStyle)
    {
    case BS_SOLID:
    case BS_NULL:
    case BS_HATCHED:
        {
	    LOGBRUSH16 lb16;

	    lb16.lbStyle = logbrush.lbStyle;
	    lb16.lbColor = logbrush.lbColor;
	    lb16.lbHatch = logbrush.lbHatch;
	    size = sizeof(METARECORD) + sizeof(LOGBRUSH16) - 2;
	    mr = HeapAlloc( GetProcessHeap(), 0, size );
	    mr->rdSize = size / 2;
	    mr->rdFunction = META_CREATEBRUSHINDIRECT;
	    memcpy( mr->rdParm, &lb16, sizeof(LOGBRUSH));
	    break;
	}
    case BS_PATTERN:
        {
	    BITMAP bm;
	    BYTE *bits;
	    BITMAPINFO *info;
	    DWORD bmSize;

	    GetObjectA((HANDLE)logbrush.lbHatch, sizeof(bm), &bm);
	    if(bm.bmBitsPixel != 1 || bm.bmPlanes != 1) {
	        FIXME("Trying to store a colour pattern brush\n");
		goto done;
	    }

	    bmSize = DIB_GetDIBImageBytes(bm.bmWidth, bm.bmHeight, 1);

	    size = sizeof(METARECORD) + sizeof(WORD) + sizeof(BITMAPINFO) +
	      sizeof(RGBQUAD) + bmSize;

	    mr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	    if(!mr) goto done;
	    mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	    mr->rdSize = size / 2;
	    mr->rdParm[0] = BS_PATTERN;
	    mr->rdParm[1] = DIB_RGB_COLORS;
	    info = (BITMAPINFO *)(mr->rdParm + 2);

	    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	    info->bmiHeader.biWidth = bm.bmWidth;
	    info->bmiHeader.biHeight = bm.bmHeight;
	    info->bmiHeader.biPlanes = 1;
	    info->bmiHeader.biBitCount = 1;
	    bits = ((BYTE *)info) + sizeof(BITMAPINFO) + sizeof(RGBQUAD);

	    GetDIBits(physDev->hdc, (HANDLE)logbrush.lbHatch, 0, bm.bmHeight,
		      bits, info, DIB_RGB_COLORS);
	    *(DWORD *)info->bmiColors = 0;
	    *(DWORD *)(info->bmiColors + 1) = 0xffffff;
	    break;
	}

    case BS_DIBPATTERN:
        {
	      BITMAPINFO *info;
	      DWORD bmSize, biSize;

	      info = GlobalLock16((HGLOBAL16)logbrush.lbHatch);
	      if (info->bmiHeader.biCompression)
		  bmSize = info->bmiHeader.biSizeImage;
	      else
		  bmSize = DIB_GetDIBImageBytes(info->bmiHeader.biWidth,
						info->bmiHeader.biHeight,
						info->bmiHeader.biBitCount);
	      biSize = DIB_BitmapInfoSize(info, LOWORD(logbrush.lbColor));
	      size = sizeof(METARECORD) + biSize + bmSize + 2;
	      mr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	      if(!mr) goto done;
	      mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	      mr->rdSize = size / 2;
	      *(mr->rdParm) = logbrush.lbStyle;
	      *(mr->rdParm + 1) = LOWORD(logbrush.lbColor);
	      memcpy(mr->rdParm + 2, info, biSize + bmSize);
	      break;
	}
	default:
	    FIXME("Unkonwn brush style %x\n", logbrush.lbStyle);
	    return -1;
    }
    index = MFDRV_AddHandleDC( dev );
    if(!MFDRV_WriteRecord( dev, mr, mr->rdSize * 2))
        index = -1;
    HeapFree(GetProcessHeap(), 0, mr);
done:
    return index;
}


/***********************************************************************
 *           MFDRV_SelectBrush
 */
HBRUSH MFDRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush )
{
    INT16 index;
    METARECORD mr;

    index = MFDRV_CreateBrushIndirect( dev, hbrush );
    if(index == -1) return 0;

    mr.rdSize = sizeof(mr) / 2;
    mr.rdFunction = META_SELECTOBJECT;
    mr.rdParm[0] = index;
    return MFDRV_WriteRecord( dev, &mr, mr.rdSize * 2) ? hbrush : 0;
}

/******************************************************************
 *         MFDRV_CreateFontIndirect
 */

static BOOL MFDRV_CreateFontIndirect(PHYSDEV dev, HFONT hFont, LOGFONT16 *logfont)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGFONT16)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGFONT16) - 2) / 2;
    mr->rdFunction = META_CREATEFONTINDIRECT;
    memcpy(&(mr->rdParm), logfont, sizeof(LOGFONT16));
    if (!(MFDRV_WriteRecord( dev, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MFDRV_AddHandleDC( dev )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/***********************************************************************
 *           MFDRV_SelectFont
 */
HFONT MFDRV_SelectFont( PHYSDEV dev, HFONT hfont )
{
    LOGFONT16 lf16;

    if (!GetObject16( HFONT_16(hfont), sizeof(lf16), &lf16 )) return HGDI_ERROR;
    if (MFDRV_CreateFontIndirect(dev, hfont, &lf16)) return 0;
    return HGDI_ERROR;
}

/******************************************************************
 *         MFDRV_CreatePenIndirect
 */
static BOOL MFDRV_CreatePenIndirect(PHYSDEV dev, HPEN hPen, LOGPEN16 *logpen)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logpen)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logpen) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParm), logpen, sizeof(*logpen));
    if (!(MFDRV_WriteRecord( dev, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MFDRV_AddHandleDC( dev )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
}


/***********************************************************************
 *           MFDRV_SelectPen
 */
HPEN MFDRV_SelectPen( PHYSDEV dev, HPEN hpen )
{
    LOGPEN16 logpen;

    if (!GetObject16( HPEN_16(hpen), sizeof(logpen), &logpen )) return 0;
    if (MFDRV_CreatePenIndirect( dev, hpen, &logpen )) return hpen;
    return 0;
}

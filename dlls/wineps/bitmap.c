/*
 *	PostScript driver bitmap functions
 *
 * Copyright 1998  Huw D M Davies
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

#include <assert.h>

#include "psdrv.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/* Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned. */
inline static int get_dib_width_bytes( int width, int depth )
{
    int words;

    switch(depth)
    {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3)/4; break;
    default:
        WARN("(%d): Unsupported depth\n", depth );
        /* fall through */
    case 32: words = width; break;
    }
    return 4 * words;
}

/* get the bitmap info from either an INFOHEADER or COREHEADER bitmap */
static BOOL get_bitmap_info( const void *ptr, LONG *width, LONG *height, WORD *bpp, WORD *compr )
{
    const BITMAPINFOHEADER *header = ptr;

    switch(header->biSize)
    {
    case sizeof(BITMAPCOREHEADER):
        {
            const BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
            *width  = core->bcWidth;
            *height = core->bcHeight;
            *bpp    = core->bcBitCount;
            *compr  = 0;
        }
        return TRUE;
    case sizeof(BITMAPINFOHEADER):
    case sizeof(BITMAPV4HEADER):
    case sizeof(BITMAPV5HEADER):
        /* V4 and V5 structures are a superset of the INFOHEADER structure */
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        return TRUE;
    default:
        ERR("(%ld): unknown/wrong size for header\n", header->biSize );
        return FALSE;
    }
}


/***************************************************************************
 *                PSDRV_WriteImageHeader
 *
 * Helper for PSDRV_StretchDIBits
 *
 * BUGS
 *  Uses level 2 PostScript
 */

static BOOL PSDRV_WriteImageHeader(PSDRV_PDEVICE *physDev, const BITMAPINFO *info, INT xDst,
				   INT yDst, INT widthDst, INT heightDst,
				   INT widthSrc, INT heightSrc)
{
    COLORREF map[256];
    int i;

    switch(info->bmiHeader.biBitCount) {
    case 8:
        PSDRV_WriteIndexColorSpaceBegin(physDev, 255);
	for(i = 0; i < 256; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(physDev, map, 256);
	PSDRV_WriteIndexColorSpaceEnd(physDev);
	break;

    case 4:
        PSDRV_WriteIndexColorSpaceBegin(physDev, 15);
	for(i = 0; i < 16; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(physDev, map, 16);
	PSDRV_WriteIndexColorSpaceEnd(physDev);
	break;

    case 1:
        PSDRV_WriteIndexColorSpaceBegin(physDev, 1);
	for(i = 0; i < 2; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(physDev, map, 2);
	PSDRV_WriteIndexColorSpaceEnd(physDev);
	break;

    case 15:
    case 16:
    case 24:
    case 32:
      {
	PSCOLOR pscol;
	pscol.type = PSCOLOR_RGB;
	pscol.value.rgb.r = pscol.value.rgb.g = pscol.value.rgb.b = 0.0;
        PSDRV_WriteSetColor(physDev, &pscol);
        break;
      }

    default:
        FIXME("Not implemented yet\n");
	return FALSE;
	break;
    }

    PSDRV_WriteImageDict(physDev, info->bmiHeader.biBitCount, xDst, yDst,
			  widthDst, heightDst, widthSrc, heightSrc, NULL, FALSE);
    return TRUE;
}


/***************************************************************************
 *                PSDRV_WriteImageMaskHeader
 *
 * Helper for PSDRV_StretchDIBits
 *
 * We use the imagemask operator for 1bpp bitmaps since the output
 * takes much less time for the printer to render.
 *
 * BUGS
 *  Uses level 2 PostScript
 */

static BOOL PSDRV_WriteImageMaskHeader(PSDRV_PDEVICE *physDev, const BITMAPINFO *info, INT xDst,
                                       INT yDst, INT widthDst, INT heightDst,
                                       INT widthSrc, INT heightSrc)
{
    COLORREF map[2];
    PSCOLOR bkgnd, foregnd;
    int i;

    assert(info->bmiHeader.biBitCount == 1);

    for(i = 0; i < 2; i++) {
        map[i] =  info->bmiColors[i].rgbRed |
            info->bmiColors[i].rgbGreen << 8 |
            info->bmiColors[i].rgbBlue << 16;
    }

    /* We'll write the mask with -ve polarity so that 
       the foregnd color corresponds to a bit equal to
       0 in the bitmap.
    */
    PSDRV_CreateColor(physDev, &foregnd, map[0]);
    PSDRV_CreateColor(physDev, &bkgnd, map[1]);

    PSDRV_WriteGSave(physDev);
    PSDRV_WriteNewPath(physDev);
    PSDRV_WriteRectangle(physDev, xDst, yDst, widthDst, heightDst);
    PSDRV_WriteSetColor(physDev, &bkgnd);
    PSDRV_WriteFill(physDev);
    PSDRV_WriteGRestore(physDev);

    PSDRV_WriteSetColor(physDev, &foregnd);
    PSDRV_WriteImageDict(physDev, 1, xDst, yDst, widthDst, heightDst,
                         widthSrc, heightSrc, NULL, TRUE);

    return TRUE;
}


/***************************************************************************
 *
 *	PSDRV_StretchDIBits
 *
 * BUGS
 *  Doesn't work correctly if xSrc isn't byte aligned - this affects 1 and 4
 *  bit depths.
 *  Compression not implemented.
 */
INT PSDRV_StretchDIBits( PSDRV_PDEVICE *physDev, INT xDst, INT yDst, INT widthDst,
			 INT heightDst, INT xSrc, INT ySrc,
			 INT widthSrc, INT heightSrc, const void *bits,
			 const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    LONG fullSrcWidth, fullSrcHeight;
    INT widthbytes;
    WORD bpp, compression;
    const char *ptr;
    INT line;
    POINT pt[2];

    TRACE("%p (%d,%d %dx%d) -> (%d,%d %dx%d)\n", physDev->hdc,
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst);

    if (!get_bitmap_info( info, &fullSrcWidth, &fullSrcHeight, &bpp, &compression )) return FALSE;

    widthbytes = get_dib_width_bytes(fullSrcWidth, bpp);

    TRACE("full size=%ldx%ld bpp=%d compression=%d rop=%08lx\n", fullSrcWidth,
	  fullSrcHeight, bpp, compression, dwRop);


    if(compression != BI_RGB) {
        FIXME("Compression not supported\n");
	return FALSE;
    }

    pt[0].x = xDst;
    pt[0].y = yDst;
    pt[1].x = xDst + widthDst;
    pt[1].y = yDst + heightDst;
    LPtoDP( physDev->hdc, pt, 2 );
    xDst = pt[0].x;
    yDst = pt[0].y;
    widthDst = pt[1].x - pt[0].x;
    heightDst = pt[1].y - pt[0].y;

    switch(bpp) {

    case 1:
        PSDRV_SetClip(physDev);
	PSDRV_WriteGSave(physDev);

        /* Use imagemask rather than image */
	PSDRV_WriteImageMaskHeader(physDev, info, xDst, yDst, widthDst, heightDst,
                                   widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
	if(xSrc & 7)
	    FIXME("This won't work...\n");
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(physDev, ptr + xSrc/8, (widthSrc+7)/8);
	break;

    case 4:
        PSDRV_SetClip(physDev);
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteImageHeader(physDev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
	if(xSrc & 1)
	    FIXME("This won't work...\n");
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(physDev, ptr + xSrc/2, (widthSrc+1)/2);
	break;

    case 8:
        PSDRV_SetClip(physDev);
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteImageHeader(physDev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteBytes(physDev, ptr + xSrc, widthSrc);
	break;

    case 15:
    case 16:
        PSDRV_SetClip(physDev);
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteImageHeader(physDev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits16(physDev, (WORD *)ptr + xSrc, widthSrc);
	break;

    case 24:
        PSDRV_SetClip(physDev);
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteImageHeader(physDev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits24(physDev, ptr + xSrc * 3, widthSrc);
	break;

    case 32:
        PSDRV_SetClip(physDev);
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteImageHeader(physDev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

	ptr = bits;
	ptr += (ySrc * widthbytes);
        for(line = 0; line < heightSrc; line++, ptr += widthbytes)
	    PSDRV_WriteDIBits32(physDev, ptr + xSrc * 3, widthSrc);
	break;

    default:
        FIXME("Unsupported depth\n");
	return FALSE;

    }
    PSDRV_WriteSpool(physDev, ">\n", 2);  /* End-of-Data for /HexASCIIDecodeFilter */
    PSDRV_WriteGRestore(physDev);
    PSDRV_ResetClip(physDev);
    return TRUE;
}

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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdlib.h>

#include "psdrv.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/* Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned. */
static inline int get_dib_width_bytes( int width, int depth )
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
            const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
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
        ERR("(%d): unknown/wrong size for header\n", header->biSize );
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

static BOOL PSDRV_WriteImageHeader(PHYSDEV dev, const BITMAPINFO *info, INT xDst,
				   INT yDst, INT widthDst, INT heightDst,
				   INT widthSrc, INT heightSrc)
{
    COLORREF map[256];
    int i;

    switch(info->bmiHeader.biBitCount) {
    case 8:
        PSDRV_WriteIndexColorSpaceBegin(dev, 255);
	for(i = 0; i < 256; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(dev, map, 256);
	PSDRV_WriteIndexColorSpaceEnd(dev);
	break;

    case 4:
        PSDRV_WriteIndexColorSpaceBegin(dev, 15);
	for(i = 0; i < 16; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(dev, map, 16);
	PSDRV_WriteIndexColorSpaceEnd(dev);
	break;

    case 1:
        PSDRV_WriteIndexColorSpaceBegin(dev, 1);
	for(i = 0; i < 2; i++) {
	    map[i] =  info->bmiColors[i].rgbRed |
	      info->bmiColors[i].rgbGreen << 8 |
	      info->bmiColors[i].rgbBlue << 16;
	}
	PSDRV_WriteRGB(dev, map, 2);
	PSDRV_WriteIndexColorSpaceEnd(dev);
	break;

    case 15:
    case 16:
    case 24:
    case 32:
      {
	PSCOLOR pscol;
	pscol.type = PSCOLOR_RGB;
	pscol.value.rgb.r = pscol.value.rgb.g = pscol.value.rgb.b = 0.0;
        PSDRV_WriteSetColor(dev, &pscol);
        break;
      }

    default:
        FIXME("Not implemented yet\n");
	return FALSE;
    }

    PSDRV_WriteImage(dev, info->bmiHeader.biBitCount, xDst, yDst,
		     widthDst, heightDst, widthSrc, heightSrc, FALSE);
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

static BOOL PSDRV_WriteImageMaskHeader(PHYSDEV dev, const BITMAPINFO *info, INT xDst,
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
    PSDRV_CreateColor(dev, &foregnd, map[0]);
    PSDRV_CreateColor(dev, &bkgnd, map[1]);

    PSDRV_WriteGSave(dev);
    PSDRV_WriteNewPath(dev);
    PSDRV_WriteRectangle(dev, xDst, yDst, widthDst, heightDst);
    PSDRV_WriteSetColor(dev, &bkgnd);
    PSDRV_WriteFill(dev);
    PSDRV_WriteGRestore(dev);

    PSDRV_WriteSetColor(dev, &foregnd);
    PSDRV_WriteImage(dev, 1, xDst, yDst, widthDst, heightDst,
		     widthSrc, heightSrc, TRUE);

    return TRUE;
}

static inline DWORD max_rle_size(DWORD size)
{
    return size + (size + 127) / 128 + 1;
}

static inline DWORD max_ascii85_size(DWORD size)
{
    return (size + 3) / 4 * 5;
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
INT PSDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                         INT heightDst, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, const void *bits,
                         const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    LONG fullSrcWidth, fullSrcHeight;
    INT stride;
    WORD bpp, compression;
    INT line;
    POINT pt[2];
    const BYTE *src_ptr;
    BYTE *dst_ptr, *bitmap, *rle, *ascii85;
    DWORD rle_len, ascii85_len, bitmap_size;

    TRACE("%p (%d,%d %dx%d) -> (%d,%d %dx%d)\n", dev->hdc,
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst);

    if (!get_bitmap_info( info, &fullSrcWidth, &fullSrcHeight, &bpp, &compression )) return FALSE;

    stride = get_dib_width_bytes(fullSrcWidth, bpp);
    if(fullSrcHeight < 0) stride = -stride; /* top-down */

    TRACE("full size=%dx%d bpp=%d compression=%d rop=%08x\n", fullSrcWidth,
	  fullSrcHeight, bpp, compression, dwRop);


    if(compression != BI_RGB) {
        FIXME("Compression not supported\n");
	return FALSE;
    }

    pt[0].x = xDst;
    pt[0].y = yDst;
    pt[1].x = xDst + widthDst;
    pt[1].y = yDst + heightDst;
    LPtoDP( dev->hdc, pt, 2 );
    xDst = pt[0].x;
    yDst = pt[0].y;
    widthDst = pt[1].x - pt[0].x;
    heightDst = pt[1].y - pt[0].y;

    switch(bpp) {

    case 1:
        PSDRV_SetClip(dev);
	PSDRV_WriteGSave(dev);

        /* Use imagemask rather than image */
	PSDRV_WriteImageMaskHeader(dev, info, xDst, yDst, widthDst, heightDst,
                                   widthSrc, heightSrc);
	src_ptr = bits;
        if(stride < 0) src_ptr += (fullSrcHeight + 1) * stride;
	src_ptr += (ySrc * stride);
	if(xSrc & 7)
	    FIXME("This won't work...\n");
        bitmap_size = heightSrc * ((widthSrc + 7) / 8);
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride, dst_ptr += ((widthSrc + 7) / 8))
            memcpy(dst_ptr, src_ptr + xSrc / 8, (widthSrc + 7) / 8);
	break;

    case 4:
        PSDRV_SetClip(dev);
	PSDRV_WriteGSave(dev);
	PSDRV_WriteImageHeader(dev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	src_ptr = bits;
        if(stride < 0) src_ptr += (fullSrcHeight + 1) * stride;
	src_ptr += (ySrc * stride);
	if(xSrc & 1)
	    FIXME("This won't work...\n");
        bitmap_size = heightSrc * ((widthSrc + 1) / 2);
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride, dst_ptr += ((widthSrc + 1) / 2))
	    memcpy(dst_ptr, src_ptr + xSrc/2, (widthSrc+1)/2);
	break;

    case 8:
        PSDRV_SetClip(dev);
	PSDRV_WriteGSave(dev);
	PSDRV_WriteImageHeader(dev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);
	src_ptr = bits;
        if(stride < 0) src_ptr += (fullSrcHeight + 1) * stride;
	src_ptr += (ySrc * stride);
        bitmap_size = heightSrc * widthSrc;
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride, dst_ptr += widthSrc)
	    memcpy(dst_ptr, src_ptr + xSrc, widthSrc);
	break;

    case 15:
    case 16:
        PSDRV_SetClip(dev);
	PSDRV_WriteGSave(dev);
	PSDRV_WriteImageHeader(dev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);


        src_ptr = bits;
        if(stride < 0) src_ptr += (fullSrcHeight + 1) * stride;
        src_ptr += (ySrc * stride);
        bitmap_size = heightSrc * widthSrc * 3;
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        
        for(line = 0; line < heightSrc; line++, src_ptr += stride) {
            const WORD *words = (const WORD *)src_ptr + xSrc;
            int i;
            for(i = 0; i < widthSrc; i++) {
                BYTE r, g, b;

                /* We want 0x0 -- 0x1f to map to 0x0 -- 0xff */
                r = words[i] >> 10 & 0x1f;
                r = r << 3 | r >> 2;
                g = words[i] >> 5 & 0x1f;
                g = g << 3 | g >> 2;
                b = words[i] & 0x1f;
                b = b << 3 | b >> 2;
                dst_ptr[0] = r;
                dst_ptr[1] = g;
                dst_ptr[2] = b;
                dst_ptr += 3;
            }
        }
	break;

    case 24:
        PSDRV_SetClip(dev);
	PSDRV_WriteGSave(dev);
	PSDRV_WriteImageHeader(dev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

        src_ptr = bits;
        if(stride < 0) src_ptr += (fullSrcHeight + 1) * stride;
        src_ptr += (ySrc * stride);
        bitmap_size = heightSrc * widthSrc * 3;
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride) {
            const BYTE *byte = src_ptr + xSrc * 3;
            int i;
            for(i = 0; i < widthSrc; i++) {
                dst_ptr[0] = byte[i * 3 + 2];
                dst_ptr[1] = byte[i * 3 + 1];
                dst_ptr[2] = byte[i * 3];
                dst_ptr += 3;
            }
        }
	break;

    case 32:
        PSDRV_SetClip(dev);
	PSDRV_WriteGSave(dev);
	PSDRV_WriteImageHeader(dev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

        src_ptr = bits;
        if(stride < 0) src_ptr += (fullSrcHeight + 1) * stride;
        src_ptr += (ySrc * stride);
        bitmap_size = heightSrc * widthSrc * 3;
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride) {
            const BYTE *byte = src_ptr + xSrc * 4;
            int i;
            for(i = 0; i < widthSrc; i++) {
                dst_ptr[0] = byte[i * 4 + 2];
                dst_ptr[1] = byte[i * 4 + 1];
                dst_ptr[2] = byte[i * 4];
                dst_ptr += 3;
            }
        }
	break;

    default:
        FIXME("Unsupported depth\n");
	return FALSE;

    }

    rle = HeapAlloc(GetProcessHeap(), 0, max_rle_size(bitmap_size));
    rle_len = RLE_encode(bitmap, bitmap_size, rle);
    HeapFree(GetProcessHeap(), 0, bitmap);
    ascii85 = HeapAlloc(GetProcessHeap(), 0, max_ascii85_size(rle_len));
    ascii85_len = ASCII85_encode(rle, rle_len, ascii85);
    HeapFree(GetProcessHeap(), 0, rle);
    PSDRV_WriteData(dev, ascii85, ascii85_len);
    HeapFree(GetProcessHeap(), 0, ascii85);
    PSDRV_WriteSpool(dev, "~>\n", 3);
    PSDRV_WriteGRestore(dev);
    PSDRV_ResetClip(dev);
    return abs(heightSrc);
}

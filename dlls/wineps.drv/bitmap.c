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
    switch(info->bmiHeader.biBitCount)
    {
    case 1:
    case 4:
    case 8:
	PSDRV_WriteIndexColorSpaceBegin(dev, (1 << info->bmiHeader.biBitCount) - 1);
	PSDRV_WriteRGBQUAD(dev, info->bmiColors, 1 << info->bmiHeader.biBitCount);
	PSDRV_WriteIndexColorSpaceEnd(dev);
	break;

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
    }

    PSDRV_WriteImage(dev, info->bmiHeader.biBitCount, xDst, yDst,
		     widthDst, heightDst, widthSrc, heightSrc, FALSE, info->bmiHeader.biHeight < 0);
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
    PSCOLOR bkgnd, foregnd;

    assert(info->bmiHeader.biBitCount == 1);

    /* We'll write the mask with -ve polarity so that 
       the foregnd color corresponds to a bit equal to
       0 in the bitmap.
    */
    PSDRV_CreateColor(dev, &foregnd, RGB(info->bmiColors[0].rgbRed,
                                         info->bmiColors[0].rgbGreen,
                                         info->bmiColors[0].rgbBlue) );
    PSDRV_CreateColor(dev, &bkgnd, RGB(info->bmiColors[1].rgbRed,
                                       info->bmiColors[1].rgbGreen,
                                       info->bmiColors[1].rgbBlue) );

    PSDRV_WriteGSave(dev);
    PSDRV_WriteNewPath(dev);
    PSDRV_WriteRectangle(dev, xDst, yDst, widthDst, heightDst);
    PSDRV_WriteSetColor(dev, &bkgnd);
    PSDRV_WriteFill(dev);
    PSDRV_WriteGRestore(dev);

    PSDRV_WriteSetColor(dev, &foregnd);
    PSDRV_WriteImage(dev, 1, xDst, yDst, widthDst, heightDst,
		     widthSrc, heightSrc, TRUE, info->bmiHeader.biHeight < 0);

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

static void free_heap_bits( struct gdi_image_bits *bits )
{
    HeapFree( GetProcessHeap(), 0, bits->ptr );
}

/***************************************************************************
 *                PSDRV_WriteImageBits
 */
static void PSDRV_WriteImageBits( PHYSDEV dev, const BITMAPINFO *info, INT xDst, INT yDst,
                                  INT widthDst, INT heightDst, INT widthSrc, INT heightSrc,
                                  void *bits, DWORD size )
{
    BYTE *rle, *ascii85;
    DWORD rle_len, ascii85_len;

    if (info->bmiHeader.biBitCount == 1)
        /* Use imagemask rather than image */
	PSDRV_WriteImageMaskHeader(dev, info, xDst, yDst, widthDst, heightDst,
                                   widthSrc, heightSrc);
    else
	PSDRV_WriteImageHeader(dev, info, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

    rle = HeapAlloc(GetProcessHeap(), 0, max_rle_size(size));
    rle_len = RLE_encode(bits, size, rle);
    ascii85 = HeapAlloc(GetProcessHeap(), 0, max_ascii85_size(rle_len));
    ascii85_len = ASCII85_encode(rle, rle_len, ascii85);
    HeapFree(GetProcessHeap(), 0, rle);
    PSDRV_WriteData(dev, ascii85, ascii85_len);
    PSDRV_WriteSpool(dev, "~>\n", 3);
    HeapFree(GetProcessHeap(), 0, ascii85);
}

/***********************************************************************
 *           PSDRV_PutImage
 */
DWORD PSDRV_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                      const struct gdi_image_bits *bits, struct bitblt_coords *src,
                      struct bitblt_coords *dst, DWORD rop )
{
    int src_stride, dst_stride, size, x, y, width, height, bit_offset;
    int dst_x, dst_y, dst_width, dst_height;
    unsigned char *src_ptr, *dst_ptr;
    struct gdi_image_bits dst_bits;

    if (hbitmap) return ERROR_NOT_SUPPORTED;

    if (info->bmiHeader.biPlanes != 1) goto update_format;
    if (info->bmiHeader.biCompression != BI_RGB) goto update_format;
    if (info->bmiHeader.biBitCount == 16 || info->bmiHeader.biBitCount == 32) goto update_format;
    if (!bits) return ERROR_SUCCESS;  /* just querying the format */

    TRACE( "bpp %u %s -> %s\n", info->bmiHeader.biBitCount, wine_dbgstr_rect(&src->visrect),
           wine_dbgstr_rect(&dst->visrect) );

    width = src->visrect.right - src->visrect.left;
    height = src->visrect.bottom - src->visrect.top;
    src_stride = get_dib_width_bytes( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
    dst_stride = (width * info->bmiHeader.biBitCount + 7) / 8;

    src_ptr = bits->ptr;
    if (info->bmiHeader.biHeight > 0)
        src_ptr += (info->bmiHeader.biHeight - src->visrect.bottom) * src_stride;
    else
        src_ptr += src->visrect.top * src_stride;
    bit_offset = src->visrect.left * info->bmiHeader.biBitCount;
    src_ptr += bit_offset / 8;
    bit_offset &= 7;
    if (bit_offset) FIXME( "pos %s not supported\n", wine_dbgstr_rect(&src->visrect) );
    size = height * dst_stride;

    if (src_stride != dst_stride || (info->bmiHeader.biBitCount == 24 && !bits->is_copy))
    {
        if (!(dst_bits.ptr = HeapAlloc( GetProcessHeap(), 0, size ))) return ERROR_OUTOFMEMORY;
        dst_bits.is_copy = TRUE;
        dst_bits.free = free_heap_bits;
    }
    else
    {
        dst_bits.ptr = src_ptr;
        dst_bits.is_copy = bits->is_copy;
        dst_bits.free = NULL;
    }
    dst_ptr = dst_bits.ptr;

    switch (info->bmiHeader.biBitCount)
    {
    case 1:
    case 4:
    case 8:
        if (src_stride != dst_stride)
            for (y = 0; y < height; y++, src_ptr += src_stride, dst_ptr += dst_stride)
                memcpy( dst_ptr, src_ptr, dst_stride );
        break;
    case 24:
        if (dst_ptr != src_ptr)
            for (y = 0; y < height; y++, src_ptr += src_stride, dst_ptr += dst_stride)
                for (x = 0; x < width; x++)
                {
                    dst_ptr[x * 3]     = src_ptr[x * 3 + 2];
                    dst_ptr[x * 3 + 1] = src_ptr[x * 3 + 1];
                    dst_ptr[x * 3 + 2] = src_ptr[x * 3];
                }
        else  /* swap R and B in place */
            for (y = 0; y < height; y++, src_ptr += src_stride, dst_ptr += dst_stride)
                for (x = 0; x < width; x++)
                {
                    unsigned char tmp = dst_ptr[x * 3];
                    dst_ptr[x * 3] = dst_ptr[x * 3 + 2];
                    dst_ptr[x * 3 + 2] = tmp;
                }
        break;
    }

    dst_x = dst->visrect.left;
    dst_y = dst->visrect.top,
    dst_width = dst->visrect.right - dst->visrect.left;
    dst_height = dst->visrect.bottom - dst->visrect.top;
    if (src->width * dst->width < 0)
    {
        dst_x += dst_width;
        dst_width = -dst_width;
    }
    if (src->height * dst->height < 0)
    {
        dst_y += dst_height;
        dst_height = -dst_height;
    }

    PSDRV_SetClip(dev);
    PSDRV_WriteGSave(dev);
    if (clip) PSDRV_AddClip( dev, clip );
    PSDRV_WriteImageBits( dev, info, dst_x, dst_y, dst_width, dst_height,
                          width, height, dst_bits.ptr, size );
    PSDRV_WriteGRestore(dev);
    PSDRV_ResetClip(dev);
    if (dst_bits.free) dst_bits.free( &dst_bits );
    return ERROR_SUCCESS;

update_format:
    info->bmiHeader.biPlanes = 1;
    if (info->bmiHeader.biBitCount > 8) info->bmiHeader.biBitCount = 24;
    info->bmiHeader.biCompression = BI_RGB;
    return ERROR_BAD_FORMAT;
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
    INT stride;
    INT line;
    POINT pt[2];
    const BYTE *src_ptr;
    BYTE *dst_ptr, *bitmap;
    DWORD bitmap_size;

    TRACE("%p (%d,%d %dx%d) -> (%d,%d %dx%d)\n", dev->hdc,
	  xSrc, ySrc, widthSrc, heightSrc, xDst, yDst, widthDst, heightDst);

    stride = get_dib_width_bytes( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );

    TRACE("full size=%dx%d bpp=%d compression=%d rop=%08x\n", info->bmiHeader.biWidth,
	  info->bmiHeader.biHeight, info->bmiHeader.biBitCount, info->bmiHeader.biCompression, dwRop);


    if(info->bmiHeader.biCompression != BI_RGB) {
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

    switch (info->bmiHeader.biBitCount) {

    case 1:
	src_ptr = bits;
	src_ptr += (ySrc * stride);
	if(xSrc & 7)
	    FIXME("This won't work...\n");
        bitmap_size = heightSrc * ((widthSrc + 7) / 8);
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride, dst_ptr += ((widthSrc + 7) / 8))
            memcpy(dst_ptr, src_ptr + xSrc / 8, (widthSrc + 7) / 8);
	break;

    case 4:
	src_ptr = bits;
	src_ptr += (ySrc * stride);
	if(xSrc & 1)
	    FIXME("This won't work...\n");
        bitmap_size = heightSrc * ((widthSrc + 1) / 2);
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride, dst_ptr += ((widthSrc + 1) / 2))
	    memcpy(dst_ptr, src_ptr + xSrc/2, (widthSrc+1)/2);
	break;

    case 8:
	src_ptr = bits;
	src_ptr += (ySrc * stride);
        bitmap_size = heightSrc * widthSrc;
        dst_ptr = bitmap = HeapAlloc(GetProcessHeap(), 0, bitmap_size);
        for(line = 0; line < heightSrc; line++, src_ptr += stride, dst_ptr += widthSrc)
	    memcpy(dst_ptr, src_ptr + xSrc, widthSrc);
	break;

    case 16:
        src_ptr = bits;
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
        src_ptr = bits;
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
        src_ptr = bits;
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

    PSDRV_SetClip(dev);
    PSDRV_WriteGSave(dev);
    PSDRV_WriteImageBits( dev, info, xDst, yDst, widthDst, heightDst,
                          widthSrc, heightSrc, bitmap, bitmap_size );
    HeapFree(GetProcessHeap(), 0, bitmap);
    PSDRV_WriteGRestore(dev);
    PSDRV_ResetClip(dev);
    return abs(heightSrc);
}

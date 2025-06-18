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
    return ((width * depth + 31) / 8) & ~3;
}


/***************************************************************************
 *                PSDRV_WriteImageHeader
 *
 * Helper for PSDRV_PutImage
 *
 * BUGS
 *  Uses level 2 PostScript
 */

static BOOL PSDRV_WriteImageHeader(print_ctx *ctx, const BITMAPINFO *info, BOOL grayscale, INT xDst,
				   INT yDst, INT widthDst, INT heightDst,
				   INT widthSrc, INT heightSrc)
{
    switch(info->bmiHeader.biBitCount)
    {
    case 1:
    case 4:
    case 8:
	PSDRV_WriteIndexColorSpaceBegin(ctx, (1 << info->bmiHeader.biBitCount) - 1);
	PSDRV_WriteRGBQUAD(ctx, info->bmiColors, 1 << info->bmiHeader.biBitCount);
	PSDRV_WriteIndexColorSpaceEnd(ctx);
	break;

    case 16:
    case 24:
    case 32:
      {
	PSCOLOR pscol;
        if (grayscale)
        {
            pscol.type = PSCOLOR_GRAY;
            pscol.value.gray.i = 0;
        }
        else
        {
            pscol.type = PSCOLOR_RGB;
            pscol.value.rgb.r = pscol.value.rgb.g = pscol.value.rgb.b = 0.0;
        }
        PSDRV_WriteSetColor(ctx, &pscol);
        break;
      }
    }

    PSDRV_WriteImage(ctx, info->bmiHeader.biBitCount, grayscale, xDst, yDst,
		     widthDst, heightDst, widthSrc, heightSrc, FALSE, info->bmiHeader.biHeight < 0);
    return TRUE;
}


/***************************************************************************
 *                PSDRV_WriteImageMaskHeader
 *
 * Helper for PSDRV_PutImage
 *
 * We use the imagemask operator for 1bpp bitmaps since the output
 * takes much less time for the printer to render.
 *
 * BUGS
 *  Uses level 2 PostScript
 */

static BOOL PSDRV_WriteImageMaskHeader(print_ctx *ctx, const BITMAPINFO *info, INT xDst,
                                       INT yDst, INT widthDst, INT heightDst,
                                       INT widthSrc, INT heightSrc)
{
    PSCOLOR bkgnd, foregnd;

    assert(info->bmiHeader.biBitCount == 1);

    /* We'll write the mask with -ve polarity so that 
       the foregnd color corresponds to a bit equal to
       0 in the bitmap.
    */
    if (!info->bmiHeader.biClrUsed)
    {
        PSDRV_CreateColor( ctx, &foregnd, GetTextColor( ctx->hdc ) );
        bkgnd = ctx->bkColor;
    }
    else
    {
        PSDRV_CreateColor( ctx, &foregnd, RGB(info->bmiColors[0].rgbRed,
                                              info->bmiColors[0].rgbGreen,
                                              info->bmiColors[0].rgbBlue) );
        PSDRV_CreateColor( ctx, &bkgnd, RGB(info->bmiColors[1].rgbRed,
                                            info->bmiColors[1].rgbGreen,
                                            info->bmiColors[1].rgbBlue) );
    }

    PSDRV_WriteGSave(ctx);
    PSDRV_WriteNewPath(ctx);
    PSDRV_WriteRectangle(ctx, xDst, yDst, widthDst, heightDst);
    PSDRV_WriteSetColor(ctx, &bkgnd);
    PSDRV_WriteFill(ctx);
    PSDRV_WriteGRestore(ctx);

    PSDRV_WriteSetColor(ctx, &foregnd);
    PSDRV_WriteImage(ctx, 1, FALSE, xDst, yDst, widthDst, heightDst,
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

static void free_heap_bits( struct ps_image_bits *bits )
{
    HeapFree( GetProcessHeap(), 0, bits->ptr );
}

/***************************************************************************
 *                PSDRV_WriteImageBits
 */
static void PSDRV_WriteImageBits( print_ctx *ctx, const BITMAPINFO *info, BOOL grayscale, INT xDst, INT yDst,
                                  INT widthDst, INT heightDst, INT widthSrc, INT heightSrc,
                                  void *bits, DWORD size )
{
    BYTE *rle, *ascii85;
    DWORD rle_len, ascii85_len;

    if (info->bmiHeader.biBitCount == 1)
        /* Use imagemask rather than image */
	PSDRV_WriteImageMaskHeader(ctx, info, xDst, yDst, widthDst, heightDst,
                                   widthSrc, heightSrc);
    else
	PSDRV_WriteImageHeader(ctx, info, grayscale, xDst, yDst, widthDst, heightDst,
			       widthSrc, heightSrc);

    rle = HeapAlloc(GetProcessHeap(), 0, max_rle_size(size));
    rle_len = RLE_encode(bits, size, rle);
    ascii85 = HeapAlloc(GetProcessHeap(), 0, max_ascii85_size(rle_len));
    ascii85_len = ASCII85_encode(rle, rle_len, ascii85);
    HeapFree(GetProcessHeap(), 0, rle);
    PSDRV_WriteData(ctx, ascii85, ascii85_len);
    PSDRV_WriteSpool(ctx, "~>\n", 3);
    HeapFree(GetProcessHeap(), 0, ascii85);
}

/***********************************************************************
 *           PSDRV_PutImage
 */
DWORD PSDRV_PutImage( print_ctx *ctx, HRGN clip, BITMAPINFO *info,
                      const struct ps_image_bits *bits, struct ps_bitblt_coords *src,
                      struct ps_bitblt_coords *dst, DWORD rop )
{
    int src_stride, dst_stride, size, x, y, width, height, bit_offset;
    int dst_x, dst_y, dst_width, dst_height;
    unsigned char *src_ptr, *dst_ptr;
    struct ps_image_bits dst_bits;
    BOOL grayscale = info->bmiHeader.biBitCount == 24 && ctx->pi->ppd->ColorDevice == CD_False;

    if (info->bmiHeader.biPlanes != 1) goto update_format;
    if (info->bmiHeader.biCompression != BI_RGB) goto update_format;
    if (info->bmiHeader.biBitCount != 1 && info->bmiHeader.biBitCount != 4 &&
            info->bmiHeader.biBitCount != 8 && info->bmiHeader.biBitCount != 24) goto update_format;
    if (!bits) return ERROR_SUCCESS;  /* just querying the format */

    TRACE( "bpp %u %s -> %s\n", info->bmiHeader.biBitCount, wine_dbgstr_rect(&src->visrect),
           wine_dbgstr_rect(&dst->visrect) );

    width = src->visrect.right - src->visrect.left;
    height = src->visrect.bottom - src->visrect.top;
    src_stride = get_dib_width_bytes( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
    if (grayscale) dst_stride = width;
    else dst_stride = (width * info->bmiHeader.biBitCount + 7) / 8;

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
        if (grayscale)
        {
            PSRGB scale = rgb_to_grayscale_scale();
            for (y = 0; y < height; y++, src_ptr += src_stride, dst_ptr += dst_stride)
                for (x = 0; x < width; x++)
                    dst_ptr[x] = src_ptr[x * 3 + 2] * scale.r + src_ptr[x * 3 + 1] * scale.g + src_ptr[x * 3] * scale.b;
        }
        else if (dst_ptr != src_ptr)
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
    dst_y = dst->visrect.top;
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

    PSDRV_SetClip(ctx);
    PSDRV_WriteGSave(ctx);
    if (clip) PSDRV_AddClip( ctx, clip );
    PSDRV_WriteImageBits( ctx, info, grayscale, dst_x, dst_y, dst_width, dst_height,
                          width, height, dst_bits.ptr, size );
    PSDRV_WriteGRestore(ctx);
    PSDRV_ResetClip(ctx);
    if (dst_bits.free) dst_bits.free( &dst_bits );
    return ERROR_SUCCESS;

update_format:
    info->bmiHeader.biPlanes = 1;
    if (info->bmiHeader.biBitCount != 1 && info->bmiHeader.biBitCount != 4 &&
            info->bmiHeader.biBitCount != 8 && info->bmiHeader.biBitCount != 24)
        info->bmiHeader.biBitCount = 24;
    info->bmiHeader.biCompression = BI_RGB;
    return ERROR_BAD_FORMAT;
}

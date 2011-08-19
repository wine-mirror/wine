/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1998 Huw D M Davies
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


static HGDIOBJ BITMAP_SelectObject( HGDIOBJ handle, HDC hdc );
static INT BITMAP_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL BITMAP_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs bitmap_funcs =
{
    BITMAP_SelectObject,  /* pSelectObject */
    BITMAP_GetObject,     /* pGetObjectA */
    BITMAP_GetObject,     /* pGetObjectW */
    NULL,                 /* pUnrealizeObject */
    BITMAP_DeleteObject   /* pDeleteObject */
};


/***********************************************************************
 *           null driver fallback implementations
 */

DWORD nulldrv_GetImage( PHYSDEV dev, HBITMAP hbitmap, BITMAPINFO *info,
                        struct gdi_image_bits *bits, struct bitblt_coords *src )
{
    BITMAPOBJ *bmp;
    int height, width_bytes;

    if (!hbitmap) return ERROR_NOT_SUPPORTED;
    if (!(bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP ))) return ERROR_INVALID_HANDLE;

    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = bmp->bitmap.bmBitsPixel;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    if (bmp->bitmap.bmBitsPixel == 16)
    {
        DWORD *masks = (DWORD *)info->bmiColors;
        masks[0] = 0x7c00;
        masks[1] = 0x03e0;
        masks[2] = 0x001f;
        info->bmiHeader.biCompression = BI_BITFIELDS;
    }
    if (!bits) goto done;

    height = src->visrect.bottom - src->visrect.top;
    width_bytes = get_dib_stride( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );
    info->bmiHeader.biWidth     = bmp->bitmap.bmWidth;
    info->bmiHeader.biHeight    = -height;
    info->bmiHeader.biSizeImage = height * width_bytes;

    /* make the source rectangle relative to the returned bits */
    src->y -= src->visrect.top;
    offset_rect( &src->visrect, 0, -src->visrect.top );

    if (bmp->bitmap.bmBits && bmp->bitmap.bmWidthBytes == width_bytes)
    {
        bits->ptr = (char *)bmp->bitmap.bmBits + src->visrect.top * width_bytes;
        bits->is_copy = FALSE;
        bits->free = NULL;
    }
    else
    {
        bits->ptr = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
        bits->is_copy = TRUE;
        bits->free = free_heap_bits;
        if (bmp->bitmap.bmBits)
        {
            /* fixup the line alignment */
            char *src = bmp->bitmap.bmBits, *dst = bits->ptr;
            for ( ; height > 0; height--, src += bmp->bitmap.bmWidthBytes, dst += width_bytes)
            {
                memcpy( dst, src, bmp->bitmap.bmWidthBytes );
                memset( dst + bmp->bitmap.bmWidthBytes, 0, width_bytes - bmp->bitmap.bmWidthBytes );
            }
        }
        else memset( bits->ptr, 0, info->bmiHeader.biSizeImage );
    }
done:
    GDI_ReleaseObj( hbitmap );
    return ERROR_SUCCESS;
}

DWORD nulldrv_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                        const struct gdi_image_bits *bits, struct bitblt_coords *src,
                        struct bitblt_coords *dst, DWORD rop )
{
    BITMAPOBJ *bmp;

    if (!hbitmap) return ERROR_SUCCESS;
    if (!(bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP ))) return ERROR_INVALID_HANDLE;

    if (info->bmiHeader.biPlanes != 1) goto update_format;
    if (info->bmiHeader.biBitCount != bmp->bitmap.bmBitsPixel) goto update_format;
    /* FIXME: check color masks */

    if (bits)
    {
        int i, width_bytes = get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
        unsigned char *dst_bits, *src_bits;

        if ((src->width != dst->width) || (src->height != dst->height))
        {
            GDI_ReleaseObj( hbitmap );
            return ERROR_TRANSFORM_NOT_SUPPORTED;
        }
        if (src->visrect.left > 0 || src->visrect.right < bmp->bitmap.bmWidth ||
            dst->visrect.left > 0 || dst->visrect.right < bmp->bitmap.bmWidth)
        {
            FIXME( "setting partial rows not supported\n" );
            GDI_ReleaseObj( hbitmap );
            return ERROR_NOT_SUPPORTED;
        }
        if (clip)
        {
            FIXME( "clip region not supported\n" );
            GDI_ReleaseObj( hbitmap );
            return ERROR_NOT_SUPPORTED;
        }

        if (!bmp->bitmap.bmBits &&
            !(bmp->bitmap.bmBits = HeapAlloc( GetProcessHeap(), 0,
                                              bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes )))
        {
            GDI_ReleaseObj( hbitmap );
            return ERROR_OUTOFMEMORY;
        }
        src_bits = bits->ptr;
        if (info->bmiHeader.biHeight > 0)
        {
            src_bits += (info->bmiHeader.biHeight - 1 - src->visrect.top) * width_bytes;
            width_bytes = -width_bytes;
        }
        else
            src_bits += src->visrect.top * width_bytes;

        dst_bits = (unsigned char *)bmp->bitmap.bmBits + dst->visrect.top * bmp->bitmap.bmWidthBytes;
        if (width_bytes != bmp->bitmap.bmWidthBytes)
        {
            for (i = 0; i < dst->visrect.bottom - dst->visrect.top; i++)
            {
                memcpy( dst_bits, src_bits, min( abs(width_bytes), bmp->bitmap.bmWidthBytes ));
                src_bits += width_bytes;
                dst_bits += bmp->bitmap.bmWidthBytes;
            }
        }
        else memcpy( dst_bits, src_bits, width_bytes * (dst->visrect.bottom - dst->visrect.top) );
    }
    GDI_ReleaseObj( hbitmap );
    return ERROR_SUCCESS;

update_format:
    info->bmiHeader.biPlanes   = 1;
    info->bmiHeader.biBitCount = bmp->bitmap.bmBitsPixel;
    if (info->bmiHeader.biHeight > 0) info->bmiHeader.biHeight = -info->bmiHeader.biHeight;
    GDI_ReleaseObj( hbitmap );
    return ERROR_BAD_FORMAT;
}


/******************************************************************************
 * CreateBitmap [GDI32.@]
 *
 * Creates a bitmap with the specified info.
 *
 * PARAMS
 *    width  [I] bitmap width
 *    height [I] bitmap height
 *    planes [I] Number of color planes
 *    bpp    [I] Number of bits to identify a color
 *    bits   [I] Pointer to array containing color data
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: 0
 */
HBITMAP WINAPI CreateBitmap( INT width, INT height, UINT planes,
                             UINT bpp, LPCVOID bits )
{
    BITMAP bm;

    bm.bmType = 0;
    bm.bmWidth = width;
    bm.bmHeight = height;
    bm.bmWidthBytes = get_bitmap_stride( width, bpp );
    bm.bmPlanes = planes;
    bm.bmBitsPixel = bpp;
    bm.bmBits = (LPVOID)bits;

    return CreateBitmapIndirect( &bm );
}

/******************************************************************************
 * CreateCompatibleBitmap [GDI32.@]
 *
 * Creates a bitmap compatible with the DC.
 *
 * PARAMS
 *    hdc    [I] Handle to device context
 *    width  [I] Width of bitmap
 *    height [I] Height of bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: 0
 */
HBITMAP WINAPI CreateCompatibleBitmap( HDC hdc, INT width, INT height)
{
    HBITMAP hbmpRet = 0;

    TRACE("(%p,%d,%d) =\n", hdc, width, height);

    if (GetObjectType( hdc ) != OBJ_MEMDC)
    {
        hbmpRet = CreateBitmap(width, height,
                               GetDeviceCaps(hdc, PLANES),
                               GetDeviceCaps(hdc, BITSPIXEL),
                               NULL);
    }
    else  /* Memory DC */
    {
        DIBSECTION dib;
        HBITMAP bitmap = GetCurrentObject( hdc, OBJ_BITMAP );
        INT size = GetObjectW( bitmap, sizeof(dib), &dib );

        if (!size) return 0;

        if (size == sizeof(BITMAP))
        {
            /* A device-dependent bitmap is selected in the DC */
            hbmpRet = CreateBitmap(width, height,
                                   dib.dsBm.bmPlanes,
                                   dib.dsBm.bmBitsPixel,
                                   NULL);
        }
        else
        {
            /* A DIB section is selected in the DC */
            BITMAPINFO *bi;
            void *bits;

            /* Allocate memory for a BITMAPINFOHEADER structure and a
               color table. The maximum number of colors in a color table
               is 256 which corresponds to a bitmap with depth 8.
               Bitmaps with higher depths don't have color tables. */
            bi = HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

            if (bi)
            {
                bi->bmiHeader.biSize          = sizeof(bi->bmiHeader);
                bi->bmiHeader.biWidth         = width;
                bi->bmiHeader.biHeight        = height;
                bi->bmiHeader.biPlanes        = dib.dsBmih.biPlanes;
                bi->bmiHeader.biBitCount      = dib.dsBmih.biBitCount;
                bi->bmiHeader.biCompression   = dib.dsBmih.biCompression;
                bi->bmiHeader.biSizeImage     = 0;
                bi->bmiHeader.biXPelsPerMeter = dib.dsBmih.biXPelsPerMeter;
                bi->bmiHeader.biYPelsPerMeter = dib.dsBmih.biYPelsPerMeter;
                bi->bmiHeader.biClrUsed       = dib.dsBmih.biClrUsed;
                bi->bmiHeader.biClrImportant  = dib.dsBmih.biClrImportant;

                if (bi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    /* Copy the color masks */
                    CopyMemory(bi->bmiColors, dib.dsBitfields, 3 * sizeof(DWORD));
                }
                else if (bi->bmiHeader.biBitCount <= 8)
                {
                    /* Copy the color table */
                    GetDIBColorTable(hdc, 0, 256, bi->bmiColors);
                }

                hbmpRet = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
                HeapFree(GetProcessHeap(), 0, bi);
            }
        }
    }

    TRACE("\t\t%p\n", hbmpRet);
    return hbmpRet;
}


/******************************************************************************
 * CreateBitmapIndirect [GDI32.@]
 *
 * Creates a bitmap with the specified info.
 *
 * PARAMS
 *  bmp [I] Pointer to the bitmap info describing the bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  If a width or height of 0 are given, a 1x1 monochrome bitmap is returned.
 */
HBITMAP WINAPI CreateBitmapIndirect( const BITMAP *bmp )
{
    BITMAP bm;
    BITMAPOBJ *bmpobj;
    HBITMAP hbitmap;

    if (!bmp || bmp->bmType)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (bmp->bmWidth > 0x7ffffff || bmp->bmHeight > 0x7ffffff)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    bm = *bmp;

    if (!bm.bmWidth || !bm.bmHeight)
    {
        return GetStockObject( DEFAULT_BITMAP );
    }
    else
    {
        if (bm.bmHeight < 0)
            bm.bmHeight = -bm.bmHeight;
        if (bm.bmWidth < 0)
            bm.bmWidth = -bm.bmWidth;
    }

    if (bm.bmPlanes != 1)
    {
        FIXME("planes = %d\n", bm.bmPlanes);
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    /* Windows only uses 1, 4, 8, 16, 24 and 32 bpp */
    if(bm.bmBitsPixel == 1)         bm.bmBitsPixel = 1;
    else if(bm.bmBitsPixel <= 4)    bm.bmBitsPixel = 4;
    else if(bm.bmBitsPixel <= 8)    bm.bmBitsPixel = 8;
    else if(bm.bmBitsPixel <= 16)   bm.bmBitsPixel = 16;
    else if(bm.bmBitsPixel <= 24)   bm.bmBitsPixel = 24;
    else if(bm.bmBitsPixel <= 32)   bm.bmBitsPixel = 32;
    else {
        WARN("Invalid bmBitsPixel %d, returning ERROR_INVALID_PARAMETER\n", bm.bmBitsPixel);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Windows ignores the provided bm.bmWidthBytes */
    bm.bmWidthBytes = get_bitmap_stride( bm.bmWidth, bm.bmBitsPixel );
    /* XP doesn't allow to create bitmaps larger than 128 Mb */
    if (bm.bmHeight > 128 * 1024 * 1024 / bm.bmWidthBytes)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    /* Create the BITMAPOBJ */
    if (!(bmpobj = HeapAlloc( GetProcessHeap(), 0, sizeof(*bmpobj) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    bmpobj->size.cx = 0;
    bmpobj->size.cy = 0;
    bmpobj->bitmap = bm;
    bmpobj->bitmap.bmBits = NULL;
    bmpobj->funcs = &null_driver;
    bmpobj->dib = NULL;
    bmpobj->color_table = NULL;
    bmpobj->nb_colors = 0;

    if (!(hbitmap = alloc_gdi_handle( &bmpobj->header, OBJ_BITMAP, &bitmap_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, bmpobj );
        return 0;
    }

    if (bm.bmBits)
        SetBitmapBits( hbitmap, bm.bmHeight * bm.bmWidthBytes, bm.bmBits );

    TRACE("%dx%d, %d colors returning %p\n", bm.bmWidth, bm.bmHeight,
          1 << (bm.bmPlanes * bm.bmBitsPixel), hbitmap);

    return hbitmap;
}


/* convenience wrapper for GetImage to retrieve the full contents of a bitmap */
BOOL get_bitmap_image( HBITMAP hbitmap, BITMAPINFO *info, struct gdi_image_bits *bits )
{
    struct bitblt_coords src;
    BOOL ret = FALSE;
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );

    if (bmp)
    {
        src.visrect.left   = src.x = 0;
        src.visrect.top    = src.y = 0;
        src.visrect.right  = src.width = bmp->bitmap.bmWidth;
        src.visrect.bottom = src.height = bmp->bitmap.bmHeight;
        ret = !bmp->funcs->pGetImage( NULL, hbitmap, info, bits, &src );
        GDI_ReleaseObj( hbitmap );
    }
    return ret;
}


/***********************************************************************
 * GetBitmapBits [GDI32.@]
 *
 * Copies bitmap bits of bitmap to buffer.
 *
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
LONG WINAPI GetBitmapBits(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LONG count,        /* [in]  Number of bytes to copy */
    LPVOID bits)       /* [out] Pointer to buffer to receive bits */
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct gdi_image_bits src_bits;
    struct bitblt_coords src;
    int dst_stride, max, ret;
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    const struct gdi_dc_funcs *funcs;

    if (!bmp) return 0;
    funcs = get_bitmap_funcs( bmp );

    if (bmp->dib) dst_stride = get_bitmap_stride( bmp->dib->dsBmih.biWidth, bmp->dib->dsBmih.biBitCount );
    else dst_stride = get_bitmap_stride( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );

    ret = max = dst_stride * bmp->bitmap.bmHeight;
    if (!bits) goto done;
    if (count > max) count = max;
    ret = count;

    src.visrect.left = 0;
    src.visrect.right = bmp->bitmap.bmWidth;
    src.visrect.top = 0;
    src.visrect.bottom = (count + dst_stride - 1) / dst_stride;
    src.x = src.y = 0;
    src.width = src.visrect.right - src.visrect.left;
    src.height = src.visrect.bottom - src.visrect.top;

    if (!funcs->pGetImage( NULL, hbitmap, info, &src_bits, &src ))
    {
        const char *src_ptr = src_bits.ptr;
        int src_stride = get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );

        /* GetBitmapBits returns 16-bit aligned data */

        if (info->bmiHeader.biHeight > 0)
        {
            src_ptr += (info->bmiHeader.biHeight - 1) * src_stride;
            src_stride = -src_stride;
        }
        src_ptr += src.visrect.top * src_stride;

        if (src_stride == dst_stride) memcpy( bits, src_ptr, count );
        else while (count > 0)
        {
            memcpy( bits, src_ptr, min( count, dst_stride ) );
            src_ptr += src_stride;
            bits = (char *)bits + dst_stride;
            count -= dst_stride;
        }
        if (src_bits.free) src_bits.free( &src_bits );
    }
    else ret = 0;

done:
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/******************************************************************************
 * SetBitmapBits [GDI32.@]
 *
 * Sets bits of color data for a bitmap.
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
LONG WINAPI SetBitmapBits(
    HBITMAP hbitmap, /* [in] Handle to bitmap */
    LONG count,        /* [in] Number of bytes in bitmap array */
    LPCVOID bits)      /* [in] Address of array with bitmap bits */
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    BITMAPOBJ *bmp;
    DWORD err;
    int i, src_stride, dst_stride;
    struct bitblt_coords src, dst;
    struct gdi_image_bits src_bits;
    HRGN clip = NULL;
    const struct gdi_dc_funcs *funcs;

    if (!bits) return 0;

    bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    if (!bmp) return 0;

    funcs = get_bitmap_funcs( bmp );

    if (count < 0) {
	WARN("(%d): Negative number of bytes passed???\n", count );
	count = -count;
    }

    if (bmp->dib) src_stride = get_bitmap_stride( bmp->dib->dsBmih.biWidth, bmp->dib->dsBmih.biBitCount );
    else src_stride = get_bitmap_stride( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );
    count = min( count, src_stride * bmp->bitmap.bmHeight );

    dst_stride = get_dib_stride( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );

    src.visrect.left   = src.x = 0;
    src.visrect.top    = src.y = 0;
    src.visrect.right  = src.width = bmp->bitmap.bmWidth;
    src.visrect.bottom = src.height = (count + src_stride - 1 ) / src_stride;
    dst = src;

    if (count % src_stride)
    {
        HRGN last_row;
        int extra_pixels = ((count % src_stride) << 3) / bmp->bitmap.bmBitsPixel;

        if ((count % src_stride << 3) % bmp->bitmap.bmBitsPixel)
            FIXME( "Unhandled partial pixel\n" );
        clip = CreateRectRgn( src.visrect.left, src.visrect.top,
                              src.visrect.right, src.visrect.bottom - 1 );
        last_row = CreateRectRgn( src.visrect.left, src.visrect.bottom - 1,
                                  src.visrect.left + extra_pixels, src.visrect.bottom );
        CombineRgn( clip, clip, last_row, RGN_OR );
        DeleteObject( last_row );
    }

    TRACE("(%p, %d, %p) %dx%d %d bpp fetched height: %d\n",
          hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
          bmp->bitmap.bmBitsPixel, src.height );

    if (src_stride == dst_stride)
    {
        src_bits.ptr = (void *)bits;
        src_bits.is_copy = FALSE;
        src_bits.free = NULL;
    }
    else
    {
        if (!(src_bits.ptr = HeapAlloc( GetProcessHeap(), 0, dst.height * dst_stride )))
        {
            GDI_ReleaseObj( hbitmap );
            return 0;
        }
        src_bits.is_copy = TRUE;
        src_bits.free = free_heap_bits;
        for (i = 0; i < count / src_stride; i++)
            memcpy( (char *)src_bits.ptr + i * dst_stride, (char *)bits + i * src_stride, src_stride );
        if (count % src_stride)
            memcpy( (char *)src_bits.ptr + i * dst_stride, (char *)bits + i * src_stride, count % src_stride );
    }

    /* query the color info */
    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = bmp->bitmap.bmBitsPixel;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    info->bmiHeader.biWidth         = 0;
    info->bmiHeader.biHeight        = 0;
    info->bmiHeader.biSizeImage     = 0;
    err = funcs->pPutImage( NULL, hbitmap, 0, info, NULL, NULL, NULL, SRCCOPY );

    if (!err || err == ERROR_BAD_FORMAT)
    {
        info->bmiHeader.biPlanes        = 1;
        info->bmiHeader.biBitCount      = bmp->bitmap.bmBitsPixel;
        info->bmiHeader.biWidth         = bmp->bitmap.bmWidth;
        info->bmiHeader.biHeight        = -dst.height;
        info->bmiHeader.biSizeImage     = dst.height * dst_stride;
        err = funcs->pPutImage( NULL, hbitmap, clip, info, &src_bits, &src, &dst, SRCCOPY );
    }
    if (err) count = 0;

    if (clip) DeleteObject( clip );
    if (src_bits.free) src_bits.free( &src_bits );
    GDI_ReleaseObj( hbitmap );
    return count;
}

/**********************************************************************
 *		BITMAP_CopyBitmap
 *
 */
HBITMAP BITMAP_CopyBitmap(HBITMAP hbitmap)
{
    HBITMAP res;
    DIBSECTION dib;
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );

    if (!bmp) return 0;
    if (bmp->dib)
    {
        void *bits;
        BITMAPINFO *bi;
        HDC dc;

        dib = *bmp->dib;
        GDI_ReleaseObj( hbitmap );
        dc = CreateCompatibleDC( NULL );

        if (!dc) return 0;
        if (!(bi = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ))))
        {
            DeleteDC( dc );
            return 0;
        }
        bi->bmiHeader = dib.dsBmih;

        /* Get the color table or the color masks */
        GetDIBits( dc, hbitmap, 0, 0, NULL, bi, DIB_RGB_COLORS );
        bi->bmiHeader.biHeight = dib.dsBmih.biHeight;

        res = CreateDIBSection( dc, bi, DIB_RGB_COLORS, &bits, NULL, 0 );
        if (res) SetDIBits( dc, res, 0, dib.dsBm.bmHeight, dib.dsBm.bmBits, bi, DIB_RGB_COLORS );
        HeapFree( GetProcessHeap(), 0, bi );
        DeleteDC( dc );
        return res;
    }
    dib.dsBm = bmp->bitmap;
    dib.dsBm.bmBits = NULL;
    GDI_ReleaseObj( hbitmap );

    res = CreateBitmapIndirect( &dib.dsBm );
    if(res) {
        char *buf = HeapAlloc( GetProcessHeap(), 0, dib.dsBm.bmWidthBytes * dib.dsBm.bmHeight );
        GetBitmapBits (hbitmap, dib.dsBm.bmWidthBytes * dib.dsBm.bmHeight, buf);
        SetBitmapBits (res, dib.dsBm.bmWidthBytes * dib.dsBm.bmHeight, buf);
        HeapFree( GetProcessHeap(), 0, buf );
    }
    return res;
}


static void set_initial_bitmap_bits( HBITMAP hbitmap, BITMAPOBJ *bmp )
{
    if (!bmp->bitmap.bmBits) return;
    SetBitmapBits( hbitmap, bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes, bmp->bitmap.bmBits );
}

/***********************************************************************
 *           BITMAP_SetOwnerDC
 *
 * Set the type of DC that owns the bitmap. This is used when the
 * bitmap is selected into a device to initialize the bitmap function
 * table.
 */
BOOL BITMAP_SetOwnerDC( HBITMAP hbitmap, PHYSDEV physdev )
{
    BITMAPOBJ *bitmap;
    BOOL ret = TRUE;

    /* never set the owner of the stock bitmap since it can be selected in multiple DCs */
    if (hbitmap == GetStockObject(DEFAULT_BITMAP)) return TRUE;

    if (!(bitmap = GDI_GetObjPtr( hbitmap, OBJ_BITMAP ))) return FALSE;

    if (!bitmap->dib && bitmap->funcs != physdev->funcs)
    {
        /* we can only change from the null driver to some other driver */
        if (bitmap->funcs == &null_driver)
        {
            if (physdev->funcs->pCreateBitmap)
            {
                ret = physdev->funcs->pCreateBitmap( physdev, hbitmap );
                if (ret)
                {
                    bitmap->funcs = physdev->funcs;
                    set_initial_bitmap_bits( hbitmap, bitmap );
                }
            }
            else
            {
                WARN( "Trying to select bitmap %p in DC that doesn't support it\n", hbitmap );
                ret = FALSE;
            }
        }
        else
        {
            FIXME( "Trying to select bitmap %p in different DC type\n", hbitmap );
            ret = FALSE;
        }
    }
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/***********************************************************************
 *           BITMAP_SelectObject
 */
static HGDIOBJ BITMAP_SelectObject( HGDIOBJ handle, HDC hdc )
{
    HGDIOBJ ret;
    BITMAPOBJ *bitmap;
    DC *dc;
    PHYSDEV physdev = NULL, old_physdev = NULL;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if (GetObjectType( hdc ) != OBJ_MEMDC)
    {
        ret = 0;
        goto done;
    }
    ret = dc->hBitmap;
    if (handle == dc->hBitmap) goto done;  /* nothing to do */

    if (!(bitmap = GDI_GetObjPtr( handle, OBJ_BITMAP )))
    {
        ret = 0;
        goto done;
    }

    if (bitmap->header.selcount && (handle != GetStockObject(DEFAULT_BITMAP)))
    {
        WARN( "Bitmap already selected in another DC\n" );
        GDI_ReleaseObj( handle );
        ret = 0;
        goto done;
    }

    old_physdev = GET_DC_PHYSDEV( dc, pSelectBitmap );
    if(old_physdev == &dc->dibdrv.dev)
        pop_dc_driver( dc, old_physdev );

    if(bitmap->dib)
    {
        physdev = &dc->dibdrv.dev;
        push_dc_driver( dc, physdev, physdev->funcs );
    }
    else
        physdev = GET_DC_PHYSDEV( dc, pSelectBitmap );

    if (!BITMAP_SetOwnerDC( handle, physdev ))
    {
        GDI_ReleaseObj( handle );
        ret = 0;
        goto done;
    }
    if (!physdev->funcs->pSelectBitmap( physdev, handle ))
    {
        GDI_ReleaseObj( handle );
        ret = 0;
    }
    else
    {
        dc->hBitmap = handle;
        GDI_inc_ref_count( handle );
        dc->dirty = 0;
        dc->vis_rect.left   = 0;
        dc->vis_rect.top    = 0;
        dc->vis_rect.right  = bitmap->bitmap.bmWidth;
        dc->vis_rect.bottom = bitmap->bitmap.bmHeight;
        SetRectRgn( dc->hVisRgn, 0, 0, bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight);
        GDI_ReleaseObj( handle );
        DC_InitDC( dc );
        GDI_dec_ref_count( ret );
    }

 done:
    if(!ret)
    {
        if(physdev == &dc->dibdrv.dev) pop_dc_driver( dc, physdev );
        if(old_physdev == &dc->dibdrv.dev) push_dc_driver( dc, old_physdev, old_physdev->funcs );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           BITMAP_DeleteObject
 */
static BOOL BITMAP_DeleteObject( HGDIOBJ handle )
{
    const DC_FUNCTIONS *funcs;
    BITMAPOBJ *bmp = GDI_GetObjPtr( handle, OBJ_BITMAP );

    if (!bmp) return FALSE;
    funcs = bmp->funcs;
    GDI_ReleaseObj( handle );

    funcs->pDeleteBitmap( handle );

    if (!(bmp = free_gdi_handle( handle ))) return FALSE;

    HeapFree( GetProcessHeap(), 0, bmp->bitmap.bmBits );

    if (bmp->dib)
    {
        DIBSECTION *dib = bmp->dib;

        if (dib->dsBm.bmBits)
        {
            if (dib->dshSection)
            {
                SYSTEM_INFO SystemInfo;
                GetSystemInfo( &SystemInfo );
                UnmapViewOfFile( (char *)dib->dsBm.bmBits -
                                 (dib->dsOffset % SystemInfo.dwAllocationGranularity) );
            }
            else if (!dib->dsOffset)
                VirtualFree(dib->dsBm.bmBits, 0L, MEM_RELEASE );
        }
        HeapFree(GetProcessHeap(), 0, dib);
        HeapFree(GetProcessHeap(), 0, bmp->color_table);
    }
    return HeapFree( GetProcessHeap(), 0, bmp );
}


/***********************************************************************
 *           BITMAP_GetObject
 */
static INT BITMAP_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    INT ret;
    BITMAPOBJ *bmp = GDI_GetObjPtr( handle, OBJ_BITMAP );

    if (!bmp) return 0;

    if (!buffer) ret = sizeof(BITMAP);
    else if (count < sizeof(BITMAP)) ret = 0;
    else if (bmp->dib)
    {
	if (count >= sizeof(DIBSECTION))
	{
            DIBSECTION *dib = buffer;
            *dib = *bmp->dib;
            dib->dsBmih.biHeight = abs( dib->dsBmih.biHeight );
            ret = sizeof(DIBSECTION);
	}
	else /* if (count >= sizeof(BITMAP)) */
        {
            DIBSECTION *dib = bmp->dib;
            memcpy( buffer, &dib->dsBm, sizeof(BITMAP) );
            ret = sizeof(BITMAP);
	}
    }
    else
    {
        memcpy( buffer, &bmp->bitmap, sizeof(BITMAP) );
        ((BITMAP *) buffer)->bmBits = NULL;
        ret = sizeof(BITMAP);
    }
    GDI_ReleaseObj( handle );
    return ret;
}


/******************************************************************************
 * CreateDiscardableBitmap [GDI32.@]
 *
 * Creates a discardable bitmap.
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI CreateDiscardableBitmap(
    HDC hdc,    /* [in] Handle to device context */
    INT width,  /* [in] Bitmap width */
    INT height) /* [in] Bitmap height */
{
    return CreateCompatibleBitmap( hdc, width, height );
}


/******************************************************************************
 * GetBitmapDimensionEx [GDI32.@]
 *
 * Retrieves dimensions of a bitmap.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LPSIZE size)     /* [out] Address of struct receiving dimensions */
{
    BITMAPOBJ * bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    if (!bmp) return FALSE;
    *size = bmp->size;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}


/******************************************************************************
 * SetBitmapDimensionEx [GDI32.@]
 *
 * Assigns dimensions to a bitmap.
 * MSDN says that this function will fail if hbitmap is a handle created by
 * CreateDIBSection, but that's not true on Windows 2000.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    INT x,           /* [in]  Bitmap width */
    INT y,           /* [in]  Bitmap height */
    LPSIZE prevSize) /* [out] Address of structure for orig dims */
{
    BITMAPOBJ * bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}

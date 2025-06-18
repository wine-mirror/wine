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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


static INT BITMAP_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL BITMAP_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs bitmap_funcs =
{
    BITMAP_GetObject,     /* pGetObjectW */
    NULL,                 /* pUnrealizeObject */
    BITMAP_DeleteObject   /* pDeleteObject */
};


/******************************************************************************
 *           NtGdiCreateCompatibleBitmap    (win32u.@)
 *
 * Creates a bitmap compatible with the DC.
 */
HBITMAP WINAPI NtGdiCreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *bi = (BITMAPINFO *)buffer;
    DIBSECTION dib;

    TRACE( "(%p,%d,%d)\n", hdc, width, height );

    if (!width || !height) return 0;

    if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC)
        return NtGdiCreateBitmap( width, height,
                                  NtGdiGetDeviceCaps( hdc, PLANES ),
                                  NtGdiGetDeviceCaps( hdc, BITSPIXEL ), NULL );

    switch (NtGdiExtGetObjectW( NtGdiGetDCObject( hdc, NTGDI_OBJ_SURF ),
                                sizeof(dib), &dib ))
    {
    case sizeof(BITMAP): /* A device-dependent bitmap is selected in the DC */
        return NtGdiCreateBitmap( width, height, dib.dsBm.bmPlanes, dib.dsBm.bmBitsPixel, NULL );

    case sizeof(DIBSECTION): /* A DIB section is selected in the DC */
        bi->bmiHeader = dib.dsBmih;
        bi->bmiHeader.biWidth  = width;
        bi->bmiHeader.biHeight = height;
        if (dib.dsBmih.biCompression == BI_BITFIELDS)  /* copy the color masks */
            memcpy( bi->bmiColors, dib.dsBitfields, sizeof(dib.dsBitfields) );
        else if (dib.dsBmih.biBitCount <= 8)  /* copy the color table */
            NtGdiDoPalette( hdc, 0, 256, bi->bmiColors, NtGdiGetDIBColorTable, FALSE );
        return NtGdiCreateDIBSection( hdc, NULL, 0, bi, DIB_RGB_COLORS, 0, 0, 0, NULL );

    default:
        return 0;
    }
}


/******************************************************************************
 *      NtGdiCreateBitmap (win32u.@)
 *
 * Creates a bitmap with the specified info.
 */
HBITMAP WINAPI NtGdiCreateBitmap( INT width, INT height, UINT planes,
                                  UINT bpp, const void *bits )
{
    BITMAPOBJ *bmpobj;
    HBITMAP hbitmap;
    INT dib_stride;
    SIZE_T size;

    if (width > 0x7ffffff || height > 0x7ffffff)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!width || !height)
        return 0;

    if (height < 0)
        height = -height;
    if (width < 0)
        width = -width;

    if (planes != 1)
    {
        FIXME("planes = %d\n", planes);
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    /* Windows only uses 1, 4, 8, 16, 24 and 32 bpp */
    if(bpp == 1)         bpp = 1;
    else if(bpp <= 4)    bpp = 4;
    else if(bpp <= 8)    bpp = 8;
    else if(bpp <= 16)   bpp = 16;
    else if(bpp <= 24)   bpp = 24;
    else if(bpp <= 32)   bpp = 32;
    else
    {
        WARN("Invalid bmBitsPixel %d, returning ERROR_INVALID_PARAMETER\n", bpp);
        RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    dib_stride = get_dib_stride( width, bpp );
    size = dib_stride * height;
    /* Check for overflow (dib_stride itself must be ok because of the constraint on bm.bmWidth above). */
    if (dib_stride != size / height)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* Create the BITMAPOBJ */
    if (!(bmpobj = calloc( 1, sizeof(*bmpobj) )))
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    bmpobj->dib.dsBm.bmType       = 0;
    bmpobj->dib.dsBm.bmWidth      = width;
    bmpobj->dib.dsBm.bmHeight     = height;
    bmpobj->dib.dsBm.bmWidthBytes = get_bitmap_stride( width, bpp );
    bmpobj->dib.dsBm.bmPlanes     = planes;
    bmpobj->dib.dsBm.bmBitsPixel  = bpp;
    bmpobj->dib.dsBm.bmBits       = calloc( 1, size );
    if (!bmpobj->dib.dsBm.bmBits)
    {
        free( bmpobj );
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    if (!(hbitmap = alloc_gdi_handle( &bmpobj->obj, NTGDI_OBJ_BITMAP, &bitmap_funcs )))
    {
        free( bmpobj->dib.dsBm.bmBits );
        free( bmpobj );
        return 0;
    }

    if (bits)
        NtGdiSetBitmapBits( hbitmap, height * bmpobj->dib.dsBm.bmWidthBytes, bits );

    TRACE("%dx%d, bpp %d planes %d: returning %p\n", width, height, bpp, planes, hbitmap);
    return hbitmap;
}


/***********************************************************************
 *      NtGdiGetBitmapBits (win32u.@)
 *
 * Copies bitmap bits of bitmap to buffer.
 *
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
LONG WINAPI NtGdiGetBitmapBits(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LONG count,        /* [in]  Number of bytes to copy */
    LPVOID bits)       /* [out] Pointer to buffer to receive bits */
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct gdi_image_bits src_bits;
    struct bitblt_coords src;
    int dst_stride, max, ret;
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, NTGDI_OBJ_BITMAP );

    if (!bmp) return 0;

    dst_stride = get_bitmap_stride( bmp->dib.dsBm.bmWidth, bmp->dib.dsBm.bmBitsPixel );
    ret = max = dst_stride * bmp->dib.dsBm.bmHeight;
    if (!bits) goto done;
    if (count < 0 || count > max) count = max;
    ret = count;

    src.visrect.left = 0;
    src.visrect.right = bmp->dib.dsBm.bmWidth;
    src.visrect.top = 0;
    src.visrect.bottom = (count + dst_stride - 1) / dst_stride;
    src.x = src.y = 0;
    src.width = src.visrect.right - src.visrect.left;
    src.height = src.visrect.bottom - src.visrect.top;

    if (!get_image_from_bitmap( bmp, info, &src_bits, &src ))
    {
        const char *src_ptr = src_bits.ptr;
        int src_stride = info->bmiHeader.biSizeImage / abs( info->bmiHeader.biHeight );

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
 *      NtGdiSetBitmapBits (win32u.@)
 *
 * Sets bits of color data for a bitmap.
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
LONG WINAPI NtGdiSetBitmapBits(
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

    if (!bits) return 0;

    bmp = GDI_GetObjPtr( hbitmap, NTGDI_OBJ_BITMAP );
    if (!bmp) return 0;

    if (count < 0) {
	WARN("(%d): Negative number of bytes passed???\n", count );
	count = -count;
    }

    src_stride = get_bitmap_stride( bmp->dib.dsBm.bmWidth, bmp->dib.dsBm.bmBitsPixel );
    count = min( count, src_stride * bmp->dib.dsBm.bmHeight );

    dst_stride = get_dib_stride( bmp->dib.dsBm.bmWidth, bmp->dib.dsBm.bmBitsPixel );

    src.visrect.left   = src.x = 0;
    src.visrect.top    = src.y = 0;
    src.visrect.right  = src.width = bmp->dib.dsBm.bmWidth;
    src.visrect.bottom = src.height = (count + src_stride - 1 ) / src_stride;
    dst = src;

    if (count % src_stride)
    {
        HRGN last_row;
        int extra_pixels = ((count % src_stride) << 3) / bmp->dib.dsBm.bmBitsPixel;

        if ((count % src_stride << 3) % bmp->dib.dsBm.bmBitsPixel)
            FIXME( "Unhandled partial pixel\n" );
        clip = NtGdiCreateRectRgn( src.visrect.left, src.visrect.top,
                                   src.visrect.right, src.visrect.bottom - 1 );
        last_row = NtGdiCreateRectRgn( src.visrect.left, src.visrect.bottom - 1,
                                       src.visrect.left + extra_pixels, src.visrect.bottom );
        NtGdiCombineRgn( clip, clip, last_row, RGN_OR );
        NtGdiDeleteObjectApp( last_row );
    }

    TRACE("(%p, %d, %p) %dx%d %d bpp fetched height: %d\n",
          hbitmap, count, bits, bmp->dib.dsBm.bmWidth, bmp->dib.dsBm.bmHeight,
          bmp->dib.dsBm.bmBitsPixel, src.height );

    if (src_stride == dst_stride)
    {
        src_bits.ptr = (void *)bits;
        src_bits.is_copy = FALSE;
        src_bits.free = NULL;
    }
    else
    {
        if (!(src_bits.ptr = malloc( dst.height * dst_stride )))
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
    info->bmiHeader.biBitCount      = bmp->dib.dsBm.bmBitsPixel;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    info->bmiHeader.biWidth         = 0;
    info->bmiHeader.biHeight        = 0;
    info->bmiHeader.biSizeImage     = 0;
    err = put_image_into_bitmap( bmp, 0, info, NULL, NULL, NULL );

    if (!err || err == ERROR_BAD_FORMAT)
    {
        info->bmiHeader.biWidth     = bmp->dib.dsBm.bmWidth;
        info->bmiHeader.biHeight    = -dst.height;
        info->bmiHeader.biSizeImage = dst.height * dst_stride;
        err = put_image_into_bitmap( bmp, clip, info, &src_bits, &src, &dst );
    }
    if (err) count = 0;

    if (clip) NtGdiDeleteObjectApp( clip );
    if (src_bits.free) src_bits.free( &src_bits );
    GDI_ReleaseObj( hbitmap );
    return count;
}


/***********************************************************************
 *           NtGdiSelectBitmap (win32u.@)
 */
HGDIOBJ WINAPI NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle )
{
    HGDIOBJ ret;
    BITMAPOBJ *bitmap;
    DC *dc;
    PHYSDEV physdev;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC)
    {
        ret = 0;
        goto done;
    }
    ret = dc->hBitmap;
    if (handle == dc->hBitmap) goto done;  /* nothing to do */

    if (!(bitmap = GDI_GetObjPtr( handle, NTGDI_OBJ_BITMAP )))
    {
        ret = 0;
        goto done;
    }

    if (handle != GetStockObject( DEFAULT_BITMAP ) && GDI_get_ref_count( handle ))
    {
        WARN( "Bitmap already selected in another DC\n" );
        GDI_ReleaseObj( handle );
        ret = 0;
        goto done;
    }

    if (!is_bitmapobj_dib( bitmap ) &&
        bitmap->dib.dsBm.bmBitsPixel != 1 &&
        bitmap->dib.dsBm.bmBitsPixel != NtGdiGetDeviceCaps( hdc, BITSPIXEL ) &&
        /* Display compatible DCs should accept 32-bit DDBs because other color depths are emulated */
        !(NtGdiGetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASDISPLAY && bitmap->dib.dsBm.bmBitsPixel == 32))
    {
        WARN( "Wrong format bitmap %u bpp\n", bitmap->dib.dsBm.bmBitsPixel );
        GDI_ReleaseObj( handle );
        ret = 0;
        goto done;
    }

    physdev = GET_DC_PHYSDEV( dc, pSelectBitmap );
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
        dc->attr->vis_rect.left   = 0;
        dc->attr->vis_rect.top    = 0;
        dc->attr->vis_rect.right  = bitmap->dib.dsBm.bmWidth;
        dc->attr->vis_rect.bottom = bitmap->dib.dsBm.bmHeight;
        dc->device_rect = dc->attr->vis_rect;
        GDI_ReleaseObj( handle );
        DC_InitDC( dc );
        GDI_dec_ref_count( ret );
    }

 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           BITMAP_DeleteObject
 */
static BOOL BITMAP_DeleteObject( HGDIOBJ handle )
{
    BITMAPOBJ *bmp = free_gdi_handle( handle );

    if (!bmp) return FALSE;
    free( bmp->dib.dsBm.bmBits );
    free( bmp );
    return TRUE;
}


/***********************************************************************
 *           BITMAP_GetObject
 */
static INT BITMAP_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    INT ret = 0;
    BITMAPOBJ *bmp = GDI_GetObjPtr( handle, NTGDI_OBJ_BITMAP );

    if (!bmp) return 0;

    if (!buffer) ret = sizeof(BITMAP);
    else if (count >= sizeof(BITMAP))
    {
        BITMAP *bitmap = buffer;
        *bitmap = bmp->dib.dsBm;
        bitmap->bmBits = NULL;
        ret = sizeof(BITMAP);
    }
    GDI_ReleaseObj( handle );
    return ret;
}


/******************************************************************************
 *      NtGdiGetBitmapDimension (win32u.@)
 *
 * Retrieves dimensions of a bitmap.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI NtGdiGetBitmapDimension(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LPSIZE size)     /* [out] Address of struct receiving dimensions */
{
    BITMAPOBJ * bmp = GDI_GetObjPtr( hbitmap, NTGDI_OBJ_BITMAP );
    if (!bmp) return FALSE;
    *size = bmp->size;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}


/******************************************************************************
 *      NtGdiSetBitmapDimension (win32u.@)
 *
 * Assigns dimensions to a bitmap.
 * MSDN says that this function will fail if hbitmap is a handle created by
 * CreateDIBSection, but that's not true on Windows 2000.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI NtGdiSetBitmapDimension(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    INT x,           /* [in]  Bitmap width */
    INT y,           /* [in]  Bitmap height */
    LPSIZE prevSize) /* [out] Address of structure for orig dims */
{
    BITMAPOBJ * bmp = GDI_GetObjPtr( hbitmap, NTGDI_OBJ_BITMAP );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}

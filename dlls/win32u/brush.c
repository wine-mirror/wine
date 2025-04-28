/*
 * GDI brush objects
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

/* GDI logical brush object */
typedef struct
{
    struct gdi_obj_header obj;
    LOGBRUSH              logbrush;
    struct brush_pattern  pattern;
} BRUSHOBJ;

#define NB_HATCH_STYLES  6

static INT BRUSH_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL BRUSH_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs brush_funcs =
{
    BRUSH_GetObject,     /* pGetObjectW */
    NULL,                /* pUnrealizeObject */
    BRUSH_DeleteObject   /* pDeleteObject */
};


static BOOL copy_bitmap( struct brush_pattern *brush, HBITMAP bitmap )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct gdi_image_bits bits;
    struct bitblt_coords src;
    BITMAPOBJ *bmp = GDI_GetObjPtr( bitmap, NTGDI_OBJ_BITMAP );

    if (!bmp) return FALSE;

    src.visrect.left   = src.x = 0;
    src.visrect.top    = src.y = 0;
    src.visrect.right  = src.width = bmp->dib.dsBm.bmWidth;
    src.visrect.bottom = src.height = bmp->dib.dsBm.bmHeight;
    if (get_image_from_bitmap( bmp, info, &bits, &src )) goto done;

    brush->bits = bits;
    if (!bits.free)
    {
        if (!(brush->bits.ptr = malloc( info->bmiHeader.biSizeImage ))) goto done;
        memcpy( brush->bits.ptr, bits.ptr, info->bmiHeader.biSizeImage );
        brush->bits.free = free_heap_bits;
    }

    if (!(brush->info = malloc( get_dib_info_size( info, DIB_RGB_COLORS ))))
    {
        if (brush->bits.free) brush->bits.free( &brush->bits );
        goto done;
    }
    memcpy( brush->info, info, get_dib_info_size( info, DIB_RGB_COLORS ));
    brush->bits.is_copy = FALSE;  /* the bits can't be modified */
    brush->usage = DIB_RGB_COLORS;

done:
    GDI_ReleaseObj( bitmap );
    return brush->info != NULL;
}

BOOL store_brush_pattern( LOGBRUSH *brush, struct brush_pattern *pattern )
{
    pattern->info = NULL;
    pattern->bits.free = NULL;

    switch (brush->lbStyle)
    {
    case BS_SOLID:
    case BS_HOLLOW:
        return TRUE;

    case BS_HATCHED:
        if (brush->lbHatch > HS_DIAGCROSS)
        {
            if (brush->lbHatch >= HS_API_MAX) return FALSE;
            brush->lbStyle = BS_SOLID;
            brush->lbHatch = 0;
        }
        return TRUE;

    case BS_PATTERN8X8:
        brush->lbStyle = BS_PATTERN;
        /* fall through */
    case BS_PATTERN:
        brush->lbColor = 0;
        return copy_bitmap( pattern, (HBITMAP)brush->lbHatch );

    case BS_DIBPATTERNPT:
        pattern->usage = brush->lbColor;
        pattern->info = copy_packed_dib( (BITMAPINFO *)brush->lbHatch, pattern->usage );
        if (!pattern->info) return FALSE;
        pattern->bits.ptr = (char *)pattern->info + get_dib_info_size( pattern->info, pattern->usage );
        brush->lbStyle = BS_DIBPATTERN;
        brush->lbColor = 0;
        return TRUE;

    case BS_DIBPATTERN:
    case BS_DIBPATTERN8X8:
    case BS_MONOPATTERN:
    case BS_INDEXED:
    default:
        WARN( "invalid brush style %u\n", brush->lbStyle );
        return FALSE;
    }
}

void free_brush_pattern( struct brush_pattern *pattern )
{
    if (pattern->bits.free) pattern->bits.free( &pattern->bits );
    free( pattern->info );
}

/**********************************************************************
 *           NtGdiIcmBrushInfo    (win32u.@)
 */
BOOL WINAPI NtGdiIcmBrushInfo( HDC hdc, HBRUSH handle, BITMAPINFO *info, void *bits,
                               ULONG *bits_size, UINT *usage, BOOL *unk, UINT mode )
{
    BRUSHOBJ *brush;
    BOOL ret = FALSE;

    if (mode)
    {
        FIXME( "unsupported mode %u\n", mode );
        return FALSE;
    }

    if (!(brush = GDI_GetObjPtr( handle, NTGDI_OBJ_BRUSH ))) return FALSE;

    if (brush->pattern.info)
    {
        if (info)
        {
            memcpy( info, brush->pattern.info,
                    get_dib_info_size( brush->pattern.info, brush->pattern.usage ));
            if (info->bmiHeader.biBitCount <= 8 && !info->bmiHeader.biClrUsed)
                fill_default_color_table( info );
            if (info->bmiHeader.biHeight < 0)
                info->bmiHeader.biHeight = -info->bmiHeader.biHeight;
        }
        if (bits)
        {
            /* always return a bottom-up DIB */
            if (brush->pattern.info->bmiHeader.biHeight < 0)
            {
                unsigned int i, width_bytes, height = -brush->pattern.info->bmiHeader.biHeight;
                char *dst_ptr;

                width_bytes = get_dib_stride( brush->pattern.info->bmiHeader.biWidth,
                                              brush->pattern.info->bmiHeader.biBitCount );
                dst_ptr = (char *)bits + (height - 1) * width_bytes;
                for (i = 0; i < height; i++, dst_ptr -= width_bytes)
                    memcpy( dst_ptr, (char *)brush->pattern.bits.ptr + i * width_bytes,
                            width_bytes );
            }
            else memcpy( bits, brush->pattern.bits.ptr,
                         brush->pattern.info->bmiHeader.biSizeImage );
        }
        if (bits_size) *bits_size = brush->pattern.info->bmiHeader.biSizeImage;
        if (usage) *usage = brush->pattern.usage;
        ret = TRUE;
    }
    GDI_ReleaseObj( handle );
    return ret;
}


HBRUSH create_brush( const LOGBRUSH *brush )
{
    BRUSHOBJ * ptr;
    HBRUSH hbrush;

    if (!(ptr = malloc( sizeof(*ptr) ))) return 0;

    ptr->logbrush = *brush;

    if (store_brush_pattern( &ptr->logbrush, &ptr->pattern ) &&
        (hbrush = alloc_gdi_handle( &ptr->obj, NTGDI_OBJ_BRUSH, &brush_funcs )))
    {
        TRACE("%p\n", hbrush);
        return hbrush;
    }

    free_brush_pattern( &ptr->pattern );
    free( ptr );
    return 0;
}


/***********************************************************************
 *           NtGdiCreateHatchBrushInternal    (win32u.@)
 *
 * Create a logical brush with a hatched pattern.
 */
HBRUSH WINAPI NtGdiCreateHatchBrushInternal( INT style, COLORREF color, BOOL pen )
{
    LOGBRUSH logbrush;

    TRACE( "%d %s\n", style, debugstr_color(color) );

    logbrush.lbStyle = BS_HATCHED;
    logbrush.lbColor = color;
    logbrush.lbHatch = style;

    return create_brush( &logbrush );
}


/***********************************************************************
 *           NtGdiCreatePatternBrushInternal    (win32u.@)
 *
 * Create a logical brush with a pattern from a bitmap.
 */
HBRUSH WINAPI NtGdiCreatePatternBrushInternal( HBITMAP bitmap, BOOL pen, BOOL is_8x8 )
{
    LOGBRUSH logbrush = { BS_PATTERN, 0, 0 };

    TRACE( "%p\n", bitmap );

    logbrush.lbHatch = (ULONG_PTR)bitmap;
    return create_brush( &logbrush );
}


/***********************************************************************
 *           NtGdiCreateDIBBrush    (win32u.@)
 *
 * Create a logical brush with a pattern from a DIB.
 */
HBRUSH WINAPI NtGdiCreateDIBBrush( const void *data, UINT coloruse, UINT size,
                                   BOOL is_8x8, BOOL pen, const void *client )
{
    const BITMAPINFO *info = data;
    LOGBRUSH logbrush;

    if (!data)
        return NULL;

    TRACE( "%p %dx%d %dbpp\n", info, info->bmiHeader.biWidth,
           info->bmiHeader.biHeight,  info->bmiHeader.biBitCount );

    logbrush.lbStyle = BS_DIBPATTERNPT;
    logbrush.lbColor = coloruse;
    logbrush.lbHatch = (ULONG_PTR)data;

    return create_brush( &logbrush );
}


/***********************************************************************
 *           NtGdiCreateSolidBrush    (win32u.@)
 *
 * Create a logical brush consisting of a single colour.
 */
HBRUSH WINAPI NtGdiCreateSolidBrush( COLORREF color, HBRUSH brush )
{
    LOGBRUSH logbrush;

    TRACE("%s\n", debugstr_color(color) );

    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = color;
    logbrush.lbHatch = 0;

    return create_brush( &logbrush );
}


/***********************************************************************
 *           NtGdiSelectBrush (win32u.@)
 */
HGDIOBJ WINAPI NtGdiSelectBrush( HDC hdc, HGDIOBJ handle )
{
    BRUSHOBJ *brush;
    HGDIOBJ ret = 0;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((brush = GDI_GetObjPtr( handle, NTGDI_OBJ_BRUSH )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSelectBrush );
        struct brush_pattern *pattern = &brush->pattern;

        if (!pattern->info) pattern = NULL;

        GDI_inc_ref_count( handle );
        GDI_ReleaseObj( handle );

        if (!physdev->funcs->pSelectBrush( physdev, handle, pattern ))
        {
            GDI_dec_ref_count( handle );
        }
        else
        {
            ret = dc->hBrush;
            dc->hBrush = handle;
            GDI_dec_ref_count( ret );
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           BRUSH_DeleteObject
 */
static BOOL BRUSH_DeleteObject( HGDIOBJ handle )
{
    BRUSHOBJ *brush = free_gdi_handle( handle );

    if (!brush) return FALSE;
    free_brush_pattern( &brush->pattern );
    free( brush );
    return TRUE;
}


/***********************************************************************
 *           BRUSH_GetObject
 */
static INT BRUSH_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    BRUSHOBJ *brush = GDI_GetObjPtr( handle, NTGDI_OBJ_BRUSH );

    if (!brush) return 0;
    if (buffer)
    {
        if (count > sizeof(brush->logbrush)) count = sizeof(brush->logbrush);
        memcpy( buffer, &brush->logbrush, count );
    }
    else count = sizeof(brush->logbrush);
    GDI_ReleaseObj( handle );
    return count;
}

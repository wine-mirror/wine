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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

/* GDI logical brush object */
typedef struct
{
    LOGBRUSH              logbrush;
    struct brush_pattern  pattern;
} BRUSHOBJ;

#define NB_HATCH_STYLES  6

static HGDIOBJ BRUSH_SelectObject( HGDIOBJ handle, HDC hdc );
static INT BRUSH_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL BRUSH_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs brush_funcs =
{
    BRUSH_SelectObject,  /* pSelectObject */
    BRUSH_GetObject,     /* pGetObjectA */
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
    BITMAPOBJ *bmp = GDI_GetObjPtr( bitmap, OBJ_BITMAP );

    if (!bmp) return FALSE;

    src.visrect.left   = src.x = 0;
    src.visrect.top    = src.y = 0;
    src.visrect.right  = src.width = bmp->dib.dsBm.bmWidth;
    src.visrect.bottom = src.height = bmp->dib.dsBm.bmHeight;
    if (get_image_from_bitmap( bmp, info, &bits, &src )) goto done;

    brush->bits = bits;
    if (!bits.free)
    {
        if (!(brush->bits.ptr = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage ))) goto done;
        memcpy( brush->bits.ptr, bits.ptr, info->bmiHeader.biSizeImage );
        brush->bits.free = free_heap_bits;
    }

    if (!(brush->info = HeapAlloc( GetProcessHeap(), 0, get_dib_info_size( info, DIB_RGB_COLORS ))))
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
    HGLOBAL hmem = 0;

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

    case BS_DIBPATTERN:
        hmem = (HGLOBAL)brush->lbHatch;
        if (!(brush->lbHatch = (ULONG_PTR)GlobalLock( hmem ))) return FALSE;
        /* fall through */
    case BS_DIBPATTERNPT:
        pattern->usage = brush->lbColor;
        pattern->info = copy_packed_dib( (BITMAPINFO *)brush->lbHatch, pattern->usage );
        if (hmem) GlobalUnlock( hmem );
        if (!pattern->info) return FALSE;
        pattern->bits.ptr = (char *)pattern->info + get_dib_info_size( pattern->info, pattern->usage );
        brush->lbStyle = BS_DIBPATTERN;
        brush->lbColor = 0;
        return TRUE;

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
    HeapFree( GetProcessHeap(), 0, pattern->info );
}

BOOL get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void **bits, UINT *usage )
{
    BRUSHOBJ *brush;
    BOOL ret = FALSE;

    if (!(brush = GDI_GetObjPtr( handle, OBJ_BRUSH ))) return FALSE;

    if (brush->pattern.info)
    {
        memcpy( info, brush->pattern.info, get_dib_info_size( brush->pattern.info, brush->pattern.usage ));
        if (info->bmiHeader.biBitCount <= 8 && !info->bmiHeader.biClrUsed)
            fill_default_color_table( info );
        *bits = brush->pattern.bits.ptr;
        *usage = brush->pattern.usage;
        ret = TRUE;
    }
    GDI_ReleaseObj( handle );
    return ret;
}


/***********************************************************************
 *           CreateBrushIndirect    (GDI32.@)
 *
 * Create a logical brush with a given style, color or pattern.
 *
 * PARAMS
 *  brush [I] Pointer to a LOGBRUSH structure describing the desired brush.
 *
 * RETURNS
 *  A handle to the created brush, or a NULL handle if the brush cannot be 
 *  created.
 *
 * NOTES
 * - The brush returned should be freed by the caller using DeleteObject()
 *   when it is no longer required.
 * - Windows 95 and earlier cannot create brushes from bitmaps or DIBs larger
 *   than 8x8 pixels. If a larger bitmap is given, only a portion of the bitmap
 *   is used.
 */
HBRUSH WINAPI CreateBrushIndirect( const LOGBRUSH * brush )
{
    BRUSHOBJ * ptr;
    HBRUSH hbrush;

    if (!(ptr = HeapAlloc( GetProcessHeap(), 0, sizeof(*ptr) ))) return 0;

    ptr->logbrush = *brush;

    if (store_brush_pattern( &ptr->logbrush, &ptr->pattern ) &&
        (hbrush = alloc_gdi_handle( ptr, OBJ_BRUSH, &brush_funcs )))
    {
        TRACE("%p\n", hbrush);
        return hbrush;
    }

    free_brush_pattern( &ptr->pattern );
    HeapFree( GetProcessHeap(), 0, ptr );
    return 0;
}


/***********************************************************************
 *           CreateHatchBrush    (GDI32.@)
 *
 * Create a logical brush with a hatched pattern.
 *
 * PARAMS
 *  style [I] Direction of lines for the hatch pattern (HS_* values from "wingdi.h")
 *  color [I] Colour of the hatched pattern
 *
 * RETURNS
 *  A handle to the created brush, or a NULL handle if the brush cannot
 *  be created.
 *
 * NOTES
 * - This function uses CreateBrushIndirect() to create the brush.
 * - The brush returned should be freed by the caller using DeleteObject()
 *   when it is no longer required.
 */
HBRUSH WINAPI CreateHatchBrush( INT style, COLORREF color )
{
    LOGBRUSH logbrush;

    TRACE("%d %06x\n", style, color );

    logbrush.lbStyle = BS_HATCHED;
    logbrush.lbColor = color;
    logbrush.lbHatch = style;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreatePatternBrush    (GDI32.@)
 *
 * Create a logical brush with a pattern from a bitmap.
 *
 * PARAMS
 *  hbitmap  [I] Bitmap containing pattern for the brush
 *
 * RETURNS
 *  A handle to the created brush, or a NULL handle if the brush cannot 
 *  be created.
 *
 * NOTES
 * - This function uses CreateBrushIndirect() to create the brush.
 * - The brush returned should be freed by the caller using DeleteObject()
 *   when it is no longer required.
 */
HBRUSH WINAPI CreatePatternBrush( HBITMAP hbitmap )
{
    LOGBRUSH logbrush = { BS_PATTERN, 0, 0 };
    TRACE("%p\n", hbitmap );

    logbrush.lbHatch = (ULONG_PTR)hbitmap;
    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrush    (GDI32.@)
 *
 * Create a logical brush with a pattern from a DIB.
 *
 * PARAMS
 *  hbitmap  [I] Global object containing BITMAPINFO structure for the pattern
 *  coloruse [I] Specifies color format, if provided
 *
 * RETURNS
 *  A handle to the created brush, or a NULL handle if the brush cannot 
 *  be created.
 *
 * NOTES
 * - This function uses CreateBrushIndirect() to create the brush.
 * - The brush returned should be freed by the caller using DeleteObject()
 *   when it is no longer required.
 * - This function is for compatibility only. CreateDIBPatternBrushPt() should 
 *   be used instead.
 */
HBRUSH WINAPI CreateDIBPatternBrush( HGLOBAL hbitmap, UINT coloruse )
{
    LOGBRUSH logbrush;

    TRACE("%p\n", hbitmap );

    logbrush.lbStyle = BS_DIBPATTERN;
    logbrush.lbColor = coloruse;

    logbrush.lbHatch = (ULONG_PTR)hbitmap;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrushPt    (GDI32.@)
 *
 * Create a logical brush with a pattern from a DIB.
 *
 * PARAMS
 *  data     [I] Pointer to a BITMAPINFO structure and image data  for the pattern
 *  coloruse [I] Specifies color format, if provided
 *
 * RETURNS
 *  A handle to the created brush, or a NULL handle if the brush cannot
 *  be created.
 *
 * NOTES
 * - This function uses CreateBrushIndirect() to create the brush.
 * - The brush returned should be freed by the caller using DeleteObject()
 *   when it is no longer required.
 */
HBRUSH WINAPI CreateDIBPatternBrushPt( const void* data, UINT coloruse )
{
    const BITMAPINFO *info=data;
    LOGBRUSH logbrush;

    if (!data)
        return NULL;

    TRACE("%p %dx%d %dbpp\n", info, info->bmiHeader.biWidth,
	  info->bmiHeader.biHeight,  info->bmiHeader.biBitCount);

    logbrush.lbStyle = BS_DIBPATTERNPT;
    logbrush.lbColor = coloruse;
    logbrush.lbHatch = (ULONG_PTR)data;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateSolidBrush    (GDI32.@)
 *
 * Create a logical brush consisting of a single colour.
 *
 * PARAMS
 *  color [I] Colour to make the solid brush
 *
 * RETURNS
 *  A handle to the newly created brush, or a NULL handle if the brush cannot
 *  be created.
 *
 * NOTES
 * - This function uses CreateBrushIndirect() to create the brush.
 * - The brush returned should be freed by the caller using DeleteObject()
 *   when it is no longer required.
 */
HBRUSH WINAPI CreateSolidBrush( COLORREF color )
{
    LOGBRUSH logbrush;

    TRACE("%06x\n", color );

    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = color;
    logbrush.lbHatch = 0;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           SetBrushOrgEx    (GDI32.@)
 *
 * Set the brush origin for a device context.
 *
 * PARAMS
 *  hdc    [I] Device context to set the brush origin for
 *  x      [I] New x origin
 *  y      [I] New y origin
 *  oldorg [O] If non NULL, destination for previously set brush origin.
 *
 * RETURNS
 *  Success: TRUE. The origin is set to (x,y), and oldorg is updated if given.
 */
BOOL WINAPI SetBrushOrgEx( HDC hdc, INT x, INT y, LPPOINT oldorg )
{
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;
    if (oldorg)
        *oldorg = dc->brush_org;

    dc->brush_org.x = x;
    dc->brush_org.y = y;
    release_dc_ptr( dc );
    return TRUE;
}

/***********************************************************************
 *           FixBrushOrgEx    (GDI32.@)
 *
 * See SetBrushOrgEx.
 *
 * NOTES
 *  This function is no longer documented by MSDN, but in Win95 GDI32 it
 *  is the same as SetBrushOrgEx().
 */
BOOL WINAPI FixBrushOrgEx( HDC hdc, INT x, INT y, LPPOINT oldorg )
{
    return SetBrushOrgEx(hdc,x,y,oldorg);
}


/***********************************************************************
 *           BRUSH_SelectObject
 */
static HGDIOBJ BRUSH_SelectObject( HGDIOBJ handle, HDC hdc )
{
    BRUSHOBJ *brush;
    HGDIOBJ ret = 0;
    DC *dc = get_dc_ptr( hdc );

    if (!dc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if ((brush = GDI_GetObjPtr( handle, OBJ_BRUSH )))
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
    HeapFree( GetProcessHeap(), 0, brush );
    return TRUE;
}


/***********************************************************************
 *           BRUSH_GetObject
 */
static INT BRUSH_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    BRUSHOBJ *brush = GDI_GetObjPtr( handle, OBJ_BRUSH );

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

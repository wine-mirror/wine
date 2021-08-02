/*
 * GDI Device Context functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 *           1999 Huw D M Davies
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

#include "gdi_private.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

static DC_ATTR *get_dc_attr( HDC hdc )
{
    WORD type = gdi_handle_type( hdc );
    DC_ATTR *dc_attr;
    if ((type & 0x1f) != NTGDI_OBJ_DC || !(dc_attr = get_gdi_client_ptr( hdc, 0 )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    return dc_attr->disabled ? NULL : dc_attr;
}

/***********************************************************************
 *           CreateDCA  (GDI32.@)
 */
HDC WINAPI CreateDCA( const char *driver, const char *device, const char *output,
                      const DEVMODEA *init_data )
{
    UNICODE_STRING driverW, deviceW, outputW;
    DEVMODEW *init_dataW = NULL;
    HDC ret;

    if (driver) RtlCreateUnicodeStringFromAsciiz( &driverW, driver );
    else driverW.Buffer = NULL;

    if (device) RtlCreateUnicodeStringFromAsciiz( &deviceW, device );
    else deviceW.Buffer = NULL;

    if (output) RtlCreateUnicodeStringFromAsciiz( &outputW, output );
    else outputW.Buffer = NULL;

    if (init_data)
    {
        /* don't convert init_data for DISPLAY driver, it's not used */
        if (!driverW.Buffer || wcsicmp( driverW.Buffer, L"display" ))
            init_dataW = GdiConvertToDevmodeW( init_data );
    }

    ret = CreateDCW( driverW.Buffer, deviceW.Buffer, outputW.Buffer, init_dataW );

    RtlFreeUnicodeString( &driverW );
    RtlFreeUnicodeString( &deviceW );
    RtlFreeUnicodeString( &outputW );
    HeapFree( GetProcessHeap(), 0, init_dataW );
    return ret;
}

/***********************************************************************
 *           CreateICA    (GDI32.@)
 */
HDC WINAPI CreateICA( const char *driver, const char *device, const char *output,
                      const DEVMODEA *init_data )
{
    /* Nothing special yet for ICs */
    return CreateDCA( driver, device, output, init_data );
}


/***********************************************************************
 *           CreateICW    (GDI32.@)
 */
HDC WINAPI CreateICW( const WCHAR *driver, const WCHAR *device, const WCHAR *output,
                      const DEVMODEW *init_data )
{
    /* Nothing special yet for ICs */
    return CreateDCW( driver, device, output, init_data );
}

/***********************************************************************
 *           ResetDCA    (GDI32.@)
 */
HDC WINAPI ResetDCA( HDC hdc, const DEVMODEA *devmode )
{
    DEVMODEW *devmodeW;
    HDC ret;

    if (devmode) devmodeW = GdiConvertToDevmodeW( devmode );
    else devmodeW = NULL;

    ret = ResetDCW( hdc, devmodeW );

    HeapFree( GetProcessHeap(), 0, devmodeW );
    return ret;
}

/***********************************************************************
 *           SaveDC    (GDI32.@)
 */
INT WINAPI SaveDC( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SaveDC( hdc );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SaveDC( dc_attr )) return FALSE;
    return NtGdiSaveDC( hdc );
}

/***********************************************************************
 *           GetDeviceCaps    (GDI32.@)
 */
INT WINAPI GetDeviceCaps( HDC hdc, INT cap )
{
    if (is_meta_dc( hdc )) return METADC_GetDeviceCaps( hdc, cap );
    if (!get_dc_attr( hdc )) return FALSE;
    return NtGdiGetDeviceCaps( hdc, cap );
}

/***********************************************************************
 *		GetTextAlign (GDI32.@)
 */
UINT WINAPI GetTextAlign( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->text_align : 0;
}

/***********************************************************************
 *           SetTextAlign    (GDI32.@)
 */
UINT WINAPI SetTextAlign( HDC hdc, UINT align )
{
    DC_ATTR *dc_attr;
    UINT ret;

    TRACE("hdc=%p align=%d\n", hdc, align);

    if (is_meta_dc( hdc )) return METADC_SetTextAlign( hdc, align );
    if (!(dc_attr = get_dc_attr( hdc ))) return GDI_ERROR;
    if (dc_attr->emf && !EMFDC_SetTextAlign( dc_attr, align )) return GDI_ERROR;

    ret = dc_attr->text_align;
    dc_attr->text_align = align;
    return ret;
}

/***********************************************************************
 *		GetBkColor (GDI32.@)
 */
COLORREF WINAPI GetBkColor( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->background_color : CLR_INVALID;
}

/***********************************************************************
 *           GetDCBrushColor  (GDI32.@)
 */
COLORREF WINAPI GetDCBrushColor( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->brush_color : CLR_INVALID;
}

/***********************************************************************
 *           GetDCPenColor    (GDI32.@)
 */
COLORREF WINAPI GetDCPenColor(HDC hdc)
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->pen_color : CLR_INVALID;
}

/***********************************************************************
 *		GetTextColor (GDI32.@)
 */
COLORREF WINAPI GetTextColor( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->text_color : 0;
}

/***********************************************************************
 *		GetBkMode (GDI32.@)
 */
INT WINAPI GetBkMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->background_mode : 0;
}

/***********************************************************************
 *		SetBkMode (GDI32.@)
 */
INT WINAPI SetBkMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode <= 0 || mode > BKMODE_LAST)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetBkMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetBkMode( dc_attr, mode )) return 0;

    ret = dc_attr->background_mode;
    dc_attr->background_mode = mode;
    return ret;
}

/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
INT WINAPI GetGraphicsMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->graphics_mode : 0;
}

/***********************************************************************
 *		GetArcDirection (GDI32.@)
 */
INT WINAPI GetArcDirection( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->arc_direction : 0;
}

/***********************************************************************
 *           SetArcDirection    (GDI32.@)
 */
INT WINAPI SetArcDirection( HDC hdc, INT dir )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (dir != AD_COUNTERCLOCKWISE && dir != AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetArcDirection( dc_attr, dir )) return 0;

    ret = dc_attr->arc_direction;
    dc_attr->arc_direction = dir;
    return ret;
}

/***********************************************************************
 *           GetLayout    (GDI32.@)
 */
DWORD WINAPI GetLayout( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->layout : GDI_ERROR;
}

/***********************************************************************
 *           GetMapMode  (GDI32.@)
 */
INT WINAPI GetMapMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->map_mode : 0;
}

/***********************************************************************
 *		GetPolyFillMode  (GDI32.@)
 */
INT WINAPI GetPolyFillMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->poly_fill_mode : 0;
}

/***********************************************************************
 *		SetPolyFillMode (GDI32.@)
 */
INT WINAPI SetPolyFillMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode <= 0 || mode > POLYFILL_LAST)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetPolyFillMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetPolyFillMode( dc_attr, mode )) return 0;

    ret = dc_attr->poly_fill_mode;
    dc_attr->poly_fill_mode = mode;
    return ret;
}

/***********************************************************************
 *		GetStretchBltMode (GDI32.@)
 */
INT WINAPI GetStretchBltMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->stretch_blt_mode : 0;
}

/***********************************************************************
 *		SetStretchBltMode (GDI32.@)
 */
INT WINAPI SetStretchBltMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode <= 0 || mode > MAXSTRETCHBLTMODE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetStretchBltMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetStretchBltMode( dc_attr, mode )) return 0;

    ret = dc_attr->stretch_blt_mode;
    dc_attr->stretch_blt_mode = mode;
    return ret;
}

/***********************************************************************
 *		GetCurrentPositionEx (GDI32.@)
 */
BOOL WINAPI GetCurrentPositionEx( HDC hdc, POINT *point )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *point = dc_attr->cur_pos;
    return TRUE;
}

/***********************************************************************
 *		GetROP2 (GDI32.@)
 */
INT WINAPI GetROP2( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->rop_mode : 0;
}

/***********************************************************************
 *		GetRelAbs  (GDI32.@)
 */
INT WINAPI GetRelAbs( HDC hdc, DWORD ignore )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->rel_abs_mode : 0;
}

/***********************************************************************
 *		SetRelAbs (GDI32.@)
 */
INT WINAPI SetRelAbs( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode != ABSOLUTE && mode != RELATIVE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetRelAbs( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    ret = dc_attr->rel_abs_mode;
    dc_attr->rel_abs_mode = mode;
    return ret;
}

/***********************************************************************
 *		SetROP2 (GDI32.@)
 */
INT WINAPI SetROP2( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if ((mode < R2_BLACK) || (mode > R2_WHITE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetROP2( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetROP2( dc_attr, mode )) return 0;

    ret = dc_attr->rop_mode;
    dc_attr->rop_mode = mode;
    return ret;
}

/***********************************************************************
 *           GetMiterLimit  (GDI32.@)
 */
BOOL WINAPI GetMiterLimit( HDC hdc, FLOAT *limit )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (limit) *limit = dc_attr->miter_limit;
    return TRUE;
}

/*******************************************************************
 *           SetMiterLimit  (GDI32.@)
 */
BOOL WINAPI SetMiterLimit( HDC hdc, FLOAT limit, FLOAT *old_limit )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    /* FIXME: record EMFs */
    if (old_limit) *old_limit = dc_attr->miter_limit;
    dc_attr->miter_limit = limit;
    return TRUE;
}

/***********************************************************************
 *           SetPixel    (GDI32.@)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetPixel( hdc, x, y, color );
    if (!(dc_attr = get_dc_attr( hdc ))) return CLR_INVALID;
    if (dc_attr->emf && !EMFDC_SetPixel( dc_attr, x, y, color )) return CLR_INVALID;
    return NtGdiSetPixel( hdc, x, y, color );
}

/***********************************************************************
 *           SetPixelV    (GDI32.@)
 */
BOOL WINAPI SetPixelV( HDC hdc, INT x, INT y, COLORREF color )
{
    return SetPixel( hdc, x, y, color ) != CLR_INVALID;
}

/***********************************************************************
 *           LineTo    (GDI32.@)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)\n", hdc, x, y );

    if (is_meta_dc( hdc )) return METADC_LineTo( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_LineTo( dc_attr, x, y )) return FALSE;
    return NtGdiLineTo( hdc, x, y );
}

/***********************************************************************
 *           MoveToEx    (GDI32.@)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, POINT *pt )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d), %p\n", hdc, x, y, pt );

    if (is_meta_dc( hdc )) return METADC_MoveTo( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_MoveTo( dc_attr, x, y )) return FALSE;
    return NtGdiMoveTo( hdc, x, y, pt );
}

/***********************************************************************
 *           Arc    (GDI32.@)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (is_meta_dc( hdc ))
        return METADC_Arc( hdc, left, top, right, bottom,
                           xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_ARC ))
        return FALSE;

    return NtGdiArcInternal( NtGdiArc, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *           ArcTo    (GDI32.@)
 */
BOOL WINAPI ArcTo( HDC hdc, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_ARCTO ))
        return FALSE;

    return NtGdiArcInternal( NtGdiArcTo, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *           Chord    (GDI32.@)
 */
BOOL WINAPI Chord( HDC hdc, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (is_meta_dc( hdc ))
        return METADC_Chord( hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_CHORD ))
        return FALSE;

    return NtGdiArcInternal( NtGdiChord, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *           Pie   (GDI32.@)
 */
BOOL WINAPI Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (is_meta_dc( hdc ))
        return METADC_Pie( hdc, left, top, right, bottom,
                           xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_PIE ))
        return FALSE;

    return NtGdiArcInternal( NtGdiPie, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *      AngleArc (GDI32.@)
 */
BOOL WINAPI AngleArc( HDC hdc, INT x, INT y, DWORD radius, FLOAT start_angle, FLOAT sweep_angle )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d), %u, %f, %f\n", hdc, x, y, radius, start_angle, sweep_angle );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_AngleArc( dc_attr, x, y, radius, start_angle, sweep_angle ))
        return FALSE;
    return NtGdiAngleArc( hdc, x, y, radius, start_angle, sweep_angle );
}

/***********************************************************************
 *           Ellipse    (GDI32.@)
 */
BOOL WINAPI Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d)\n", hdc, left, top, right, bottom );

    if (is_meta_dc( hdc )) return METADC_Ellipse( hdc, left, top, right, bottom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_Ellipse( dc_attr, left, top, right, bottom )) return FALSE;
    return NtGdiEllipse( hdc, left, top, right, bottom );
}

/***********************************************************************
 *           Rectangle    (GDI32.@)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d)\n", hdc, left, top, right, bottom );

    if (is_meta_dc( hdc )) return METADC_Rectangle( hdc, left, top, right, bottom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_Rectangle( dc_attr, left, top, right, bottom )) return FALSE;
    return NtGdiRectangle( hdc, left, top, right, bottom );
}

/***********************************************************************
 *           RoundRect    (GDI32.@)
 */
BOOL WINAPI RoundRect( HDC hdc, INT left, INT top, INT right,
                       INT bottom, INT ell_width, INT ell_height )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), %dx%d\n", hdc, left, top, right, bottom,
           ell_width, ell_height );

    if (is_meta_dc( hdc ))
        return METADC_RoundRect( hdc, left, top, right, bottom, ell_width, ell_height );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_RoundRect( dc_attr, left, top, right, bottom,
                                          ell_width, ell_height ))
        return FALSE;

    return NtGdiRoundRect( hdc, left, top, right, bottom, ell_width, ell_height );
}

/**********************************************************************
 *          Polygon  (GDI32.@)
 */
BOOL WINAPI Polygon( HDC hdc, const POINT *points, INT count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %d\n", hdc, points, count );

    if (is_meta_dc( hdc )) return METADC_Polygon( hdc, points, count );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_Polygon( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, (const UINT *)&count, 1, NtGdiPolyPolygon );
}

/**********************************************************************
 *          PolyPolygon  (GDI32.@)
 */
BOOL WINAPI PolyPolygon( HDC hdc, const POINT *points, const INT *counts, UINT polygons )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %u\n", hdc, points, counts, polygons );

    if (is_meta_dc( hdc )) return METADC_PolyPolygon( hdc, points, counts, polygons );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PolyPolygon( dc_attr, points, counts, polygons )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, (const UINT *)counts, polygons, NtGdiPolyPolygon );
}

/**********************************************************************
 *          Polyline   (GDI32.@)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT *points, INT count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %d\n", hdc, points, count );

    if (is_meta_dc( hdc )) return METADC_Polyline( hdc, points, count );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_Polyline( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, (const UINT *)&count, 1, NtGdiPolyPolyline );
}

/**********************************************************************
 *          PolyPolyline  (GDI32.@)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT *points, const DWORD *counts, DWORD polylines )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %u\n", hdc, points, counts, polylines );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PolyPolyline( dc_attr, points, counts, polylines )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, counts, polylines, NtGdiPolyPolyline );
}

/******************************************************************************
 *          PolyBezier  (GDI32.@)
 */
BOOL WINAPI PolyBezier( HDC hdc, const POINT *points, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %u\n", hdc, points, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PolyBezier( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, &count, 1, NtGdiPolyBezier );
}

/******************************************************************************
 *          PolyBezierTo  (GDI32.@)
 */
BOOL WINAPI PolyBezierTo( HDC hdc, const POINT *points, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %u\n", hdc, points, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PolyBezierTo( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, &count, 1, NtGdiPolyBezierTo );
}

/**********************************************************************
 *          PolylineTo   (GDI32.@)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT *points, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %u\n", hdc, points, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PolylineTo( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, &count, 1, NtGdiPolylineTo );
}

/***********************************************************************
 *      PolyDraw (GDI32.@)
 */
BOOL WINAPI PolyDraw( HDC hdc, const POINT *points, const BYTE *types, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %u\n", hdc, points, types, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PolyDraw( dc_attr, points, types, count )) return FALSE;
    return NtGdiPolyDraw( hdc, points, types, count );
}

/***********************************************************************
 *           FillRgn    (GDI32.@)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p\n", hdc, hrgn, hbrush );

    if (is_meta_dc( hdc )) return METADC_FillRgn( hdc, hrgn, hbrush );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_FillRgn( dc_attr, hrgn, hbrush )) return FALSE;
    return NtGdiFillRgn( hdc, hrgn, hbrush );
}

/***********************************************************************
 *           PaintRgn    (GDI32.@)
 */
BOOL WINAPI PaintRgn( HDC hdc, HRGN hrgn )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p\n", hdc, hrgn );

    if (is_meta_dc( hdc )) return METADC_PaintRgn( hdc, hrgn );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_PaintRgn( dc_attr, hrgn )) return FALSE;
    return NtGdiFillRgn( hdc, hrgn, GetCurrentObject( hdc, OBJ_BRUSH ));
}

/***********************************************************************
 *           FrameRgn     (GDI32.@)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %dx%d\n", hdc, hrgn, hbrush, width, height );

    if (is_meta_dc( hdc )) return METADC_FrameRgn( hdc, hrgn, hbrush, width, height );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_FrameRgn( dc_attr, hrgn, hbrush, width, height ))
        return FALSE;
    return NtGdiFrameRgn( hdc, hrgn, hbrush, width, height );
}

/***********************************************************************
 *           InvertRgn    (GDI32.@)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p\n", hdc, hrgn );

    if (is_meta_dc( hdc )) return METADC_InvertRgn( hdc, hrgn );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_InvertRgn( dc_attr, hrgn )) return FALSE;
    return NtGdiInvertRgn( hdc, hrgn );
}

/***********************************************************************
 *          ExtFloodFill   (GDI32.@)
 */
BOOL WINAPI ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d), %08x, %x\n", hdc, x, y, color, fill_type );

    if (is_meta_dc( hdc )) return METADC_ExtFloodFill( hdc, x, y, color, fill_type );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ExtFloodFill( dc_attr, x, y, color, fill_type )) return FALSE;
    return NtGdiExtFloodFill( hdc, x, y, color, fill_type );
}

/***********************************************************************
 *          FloodFill   (GDI32.@)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}

/******************************************************************************
 *           GdiGradientFill   (GDI32.@)
 */
BOOL WINAPI GdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                             void *grad_array, ULONG ngrad, ULONG mode )
{
    DC_ATTR *dc_attr;

    TRACE( "%p vert_array:%p nvert:%d grad_array:%p ngrad:%d\n", hdc, vert_array,
           nvert, grad_array, ngrad );

    if (!(dc_attr = get_dc_attr( hdc )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (dc_attr->emf &&
        !EMFDC_GradientFill( dc_attr, vert_array, nvert, grad_array, ngrad, mode ))
        return FALSE;
    return NtGdiGradientFill( hdc, vert_array, nvert, grad_array, ngrad, mode );
}

/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                         const WCHAR *str, UINT count, const INT *dx )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_ExtTextOut( hdc, x, y, flags, rect, str, count, dx );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ExtTextOut( dc_attr, x, y, flags, rect, str, count, dx ))
        return FALSE;
    return NtGdiExtTextOutW( hdc, x, y, flags, rect, str, count, dx, 0 );
}

/***********************************************************************
 *           BeginPath    (GDI32.@)
 */
BOOL WINAPI BeginPath(HDC hdc)
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_BeginPath( dc_attr )) return FALSE;
    return NtGdiBeginPath( hdc );
}

/***********************************************************************
 *           EndPath    (GDI32.@)
 */
BOOL WINAPI EndPath(HDC hdc)
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_EndPath( dc_attr )) return FALSE;
    return NtGdiEndPath( hdc );
}

/***********************************************************************
 *           AbortPath  (GDI32.@)
 */
BOOL WINAPI AbortPath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_AbortPath( dc_attr )) return FALSE;
    return NtGdiAbortPath( hdc );
}

/***********************************************************************
 *           CloseFigure    (GDI32.@)
 */
BOOL WINAPI CloseFigure( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_CloseFigure( dc_attr )) return FALSE;
    return NtGdiCloseFigure( hdc );
}

/***********************************************************************
 *           GdiSetPixelFormat   (GDI32.@)
 */
BOOL WINAPI GdiSetPixelFormat( HDC hdc, INT format, const PIXELFORMATDESCRIPTOR *descr )
{
    TRACE( "(%p,%d,%p)\n", hdc, format, descr );
    return NtGdiSetPixelFormat( hdc, format );
}

/***********************************************************************
 *           CancelDC    (GDI32.@)
 */
BOOL WINAPI CancelDC(HDC hdc)
{
    FIXME( "stub\n" );
    return TRUE;
}

/***********************************************************************
 *           SetICMMode    (GDI32.@)
 */
INT WINAPI SetICMMode( HDC hdc, INT mode )
{
    /* FIXME: Assume that ICM is always off, and cannot be turned on */
    switch (mode)
    {
    case ICM_OFF:   return ICM_OFF;
    case ICM_ON:    return 0;
    case ICM_QUERY: return ICM_OFF;
    }
    return 0;
}

/***********************************************************************
 *           GdiIsMetaPrintDC  (GDI32.@)
 */
BOOL WINAPI GdiIsMetaPrintDC( HDC hdc )
{
    FIXME( "%p\n", hdc );
    return FALSE;
}

/***********************************************************************
 *           GdiIsMetaFileDC  (GDI32.@)
 */
BOOL WINAPI GdiIsMetaFileDC( HDC hdc )
{
    TRACE( "%p\n", hdc );

    switch (GetObjectType( hdc ))
    {
    case OBJ_METADC:
    case OBJ_ENHMETADC:
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           GdiIsPlayMetafileDC  (GDI32.@)
 */
BOOL WINAPI GdiIsPlayMetafileDC( HDC hdc )
{
    FIXME( "%p\n", hdc );
    return FALSE;
}

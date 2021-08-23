/*
 * Enhanced MetaFile driver graphics functions
 *
 * Copyright 1999 Huw D M Davies
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
#include "enhmfdrv/enhmetafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

static const RECTL empty_bounds = { 0, 0, -1, -1 };

/* determine if we can use 16-bit points to store all the input points */
static BOOL can_use_short_points( const POINT *pts, UINT count )
{
    UINT i;

    for (i = 0; i < count; i++)
        if (((pts[i].x + 0x8000) & ~0xffff) || ((pts[i].y + 0x8000) & ~0xffff))
            return FALSE;
    return TRUE;
}

/* store points in either long or short format; return a pointer to the end of the stored data */
static void *store_points( POINTL *dest, const POINT *pts, UINT count, BOOL short_points )
{
    if (short_points)
    {
        UINT i;
        POINTS *dest_short = (POINTS *)dest;

        for (i = 0; i < count; i++)
        {
            dest_short[i].x = pts[i].x;
            dest_short[i].y = pts[i].y;
        }
        return dest_short + count;
    }
    else
    {
        memcpy( dest, pts, count * sizeof(*dest) );
        return dest + count;
    }
}

/* compute the bounds of an array of points, optionally including the current position */
static void get_points_bounds( RECTL *bounds, const POINT *pts, UINT count, DC_ATTR *dc_attr )
{
    UINT i;

    if (dc_attr)
    {
        bounds->left = bounds->right = dc_attr->cur_pos.x;
        bounds->top = bounds->bottom = dc_attr->cur_pos.y;
    }
    else if (count)
    {
        bounds->left = bounds->right = pts[0].x;
        bounds->top = bounds->bottom = pts[0].y;
    }
    else *bounds = empty_bounds;

    for (i = 0; i < count; i++)
    {
        bounds->left   = min( bounds->left, pts[i].x );
        bounds->right  = max( bounds->right, pts[i].x );
        bounds->top    = min( bounds->top, pts[i].y );
        bounds->bottom = max( bounds->bottom, pts[i].y );
    }
}

/* helper for path stroke and fill functions */
static BOOL emfdrv_stroke_and_fill_path( struct emf *emf, INT type )
{
    DC *dc = get_physdev_dc( &emf->dev );
    EMRSTROKEANDFILLPATH emr;
    struct gdi_path *path;
    POINT *points;
    BYTE *flags;

    emr.emr.iType = type;
    emr.emr.nSize = sizeof(emr);

    if ((path = get_gdi_flat_path( dc, NULL )))
    {
        int count = get_gdi_path_data( path, &points, &flags );
        get_points_bounds( &emr.rclBounds, points, count, 0 );
        free_gdi_path( path );
    }
    else emr.rclBounds = empty_bounds;

    if (!emfdc_record( emf, &emr.emr )) return FALSE;
    if (!path) return FALSE;
    emfdc_update_bounds( emf, &emr.rclBounds );
    return TRUE;
}

/**********************************************************************
 *	     EMFDC_MoveTo
 */
BOOL EMFDC_MoveTo( DC_ATTR *dc_attr, INT x, INT y )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRMOVETOEX emr;

    emr.emr.iType = EMR_MOVETOEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;

    return emfdc_record( emf, &emr.emr );
}

/***********************************************************************
 *           EMFDC_LineTo
 */
BOOL EMFDC_LineTo( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRLINETO emr;

    emr.emr.iType = EMR_LINETO;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;
    return emfdc_record( dc_attr->emf, &emr.emr );
}


/***********************************************************************
 *           EMFDC_ArcChordPie
 */
BOOL EMFDC_ArcChordPie( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend, DWORD type )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRARC emr;
    INT temp;

    if (left == right || top == bottom) return FALSE;

    if (left > right) { temp = left; left = right; right = temp; }
    if (top > bottom) { temp = top; top = bottom; bottom = temp; }

    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        right--;
        bottom--;
    }

    emr.emr.iType     = type;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.ptlStart.x    = xstart;
    emr.ptlStart.y    = ystart;
    emr.ptlEnd.x      = xend;
    emr.ptlEnd.y      = yend;
    return emfdc_record( emf, &emr.emr );
}

/***********************************************************************
 *           EMFDC_AngleArc
 */
BOOL EMFDC_AngleArc( DC_ATTR *dc_attr, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep )
{
    EMRANGLEARC emr;

    emr.emr.iType   = EMR_ANGLEARC;
    emr.emr.nSize   = sizeof( emr );
    emr.ptlCenter.x = x;
    emr.ptlCenter.y = y;
    emr.nRadius     = radius;
    emr.eStartAngle = start;
    emr.eSweepAngle = sweep;

    return emfdc_record( dc_attr->emf, &emr.emr );
}

/***********************************************************************
 *           EMFDC_Ellipse
 */
BOOL EMFDC_Ellipse( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRELLIPSE emr;

    if (left == right || top == bottom) return FALSE;

    emr.emr.iType     = EMR_ELLIPSE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = min( left, right );
    emr.rclBox.top    = min( top, bottom );
    emr.rclBox.right  = max( left, right );
    emr.rclBox.bottom = max( top, bottom );
    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        emr.rclBox.right--;
        emr.rclBox.bottom--;
    }

    return emfdc_record( emf, &emr.emr );
}

/***********************************************************************
 *           EMFDC_Rectangle
 */
BOOL EMFDC_Rectangle( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRRECTANGLE emr;

    if(left == right || top == bottom) return FALSE;

    emr.emr.iType     = EMR_RECTANGLE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = min( left, right );
    emr.rclBox.top    = min( top, bottom );
    emr.rclBox.right  = max( left, right );
    emr.rclBox.bottom = max( top, bottom );
    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        emr.rclBox.right--;
        emr.rclBox.bottom--;
    }

    return emfdc_record( emf, &emr.emr );
}

/***********************************************************************
 *           EMFDC_RoundRect
 */
BOOL EMFDC_RoundRect( DC_ATTR *dc_attr, INT left, INT top, INT right,
                      INT bottom, INT ell_width, INT ell_height )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRROUNDRECT emr;

    if (left == right || top == bottom) return FALSE;

    emr.emr.iType     = EMR_ROUNDRECT;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = min( left, right );
    emr.rclBox.top    = min( top, bottom );
    emr.rclBox.right  = max( left, right );
    emr.rclBox.bottom = max( top, bottom );
    emr.szlCorner.cx  = ell_width;
    emr.szlCorner.cy  = ell_height;
    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        emr.rclBox.right--;
        emr.rclBox.bottom--;
    }

    return emfdc_record( emf, &emr.emr );
}

/***********************************************************************
 *           EMFDC_SetPixel
 */
BOOL EMFDC_SetPixel( DC_ATTR *dc_attr, INT x, INT y, COLORREF color )
{
    EMRSETPIXELV emr;

    emr.emr.iType  = EMR_SETPIXELV;
    emr.emr.nSize  = sizeof(emr);
    emr.ptlPixel.x = x;
    emr.ptlPixel.y = y;
    emr.crColor = color;
    return emfdc_record( dc_attr->emf, &emr.emr );
}

/**********************************************************************
 *          EMFDRV_Polylinegon
 *
 * Helper for EMFDRV_Poly{line|gon}
 */
static BOOL EMFDC_Polylinegon( DC_ATTR *dc_attr, const POINT *points, INT count, DWORD type )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRPOLYLINE *emr;
    DWORD size;
    BOOL ret, use_small_emr = can_use_short_points( points, count );

    size = use_small_emr ? offsetof( EMRPOLYLINE16, apts[count] ) : offsetof( EMRPOLYLINE, aptl[count] );

    emr = HeapAlloc( GetProcessHeap(), 0, size );
    emr->emr.iType = use_small_emr ? type + EMR_POLYLINE16 - EMR_POLYLINE : type;
    emr->emr.nSize = size;
    emr->cptl = count;

    store_points( emr->aptl, points, count, use_small_emr );

    if (!emf->path)
        get_points_bounds( &emr->rclBounds, points, count,
                           (type == EMR_POLYBEZIERTO || type == EMR_POLYLINETO) ? dc_attr : 0 );
    else
        emr->rclBounds = empty_bounds;

    ret = emfdc_record( emf, &emr->emr );
    if (ret && !emf->path)
        emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}


/**********************************************************************
 *          EMFDC_Polyline
 */
BOOL EMFDC_Polyline( DC_ATTR *dc_attr, const POINT *points, INT count )
{
    return EMFDC_Polylinegon( dc_attr, points, count, EMR_POLYLINE );
}

/**********************************************************************
 *          EMFDC_PolylineTo
 */
BOOL EMFDC_PolylineTo( DC_ATTR *dc_attr, const POINT *points, INT count )
{
    return EMFDC_Polylinegon( dc_attr, points, count, EMR_POLYLINETO );
}

/**********************************************************************
 *          EMFDC_Polygon
 */
BOOL EMFDC_Polygon( DC_ATTR *dc_attr, const POINT *pt, INT count )
{
    if(count < 2) return FALSE;
    return EMFDC_Polylinegon( dc_attr, pt, count, EMR_POLYGON );
}

/**********************************************************************
 *          EMFDC_PolyBezier
 */
BOOL EMFDC_PolyBezier( DC_ATTR *dc_attr, const POINT *pts, DWORD count )
{
    return EMFDC_Polylinegon( dc_attr, pts, count, EMR_POLYBEZIER );
}

/**********************************************************************
 *          EMFDC_PolyBezierTo
 */
BOOL EMFDC_PolyBezierTo( DC_ATTR *dc_attr, const POINT *pts, DWORD count )
{
    return EMFDC_Polylinegon( dc_attr, pts, count, EMR_POLYBEZIERTO );
}

/**********************************************************************
 *          EMFDC_PolyPolylinegon
 *
 * Helper for EMFDRV_PolyPoly{line|gon}
 */
static BOOL EMFDC_PolyPolylinegon( EMFDRV_PDEVICE *emf, const POINT *pt, const INT *counts,
                                   UINT polys, DWORD type)
{
    EMRPOLYPOLYLINE *emr;
    DWORD cptl = 0, poly, size;
    BOOL ret, use_small_emr, bounds_valid = TRUE;

    for(poly = 0; poly < polys; poly++) {
        cptl += counts[poly];
        if(counts[poly] < 2) bounds_valid = FALSE;
    }
    if(!cptl) bounds_valid = FALSE;
    use_small_emr = can_use_short_points( pt, cptl );

    size = FIELD_OFFSET(EMRPOLYPOLYLINE, aPolyCounts[polys]);
    if(use_small_emr)
        size += cptl * sizeof(POINTS);
    else
        size += cptl * sizeof(POINTL);

    emr = HeapAlloc( GetProcessHeap(), 0, size );

    emr->emr.iType = type;
    if(use_small_emr) emr->emr.iType += EMR_POLYPOLYLINE16 - EMR_POLYPOLYLINE;

    emr->emr.nSize = size;
    if(bounds_valid && !emf->path)
        get_points_bounds( &emr->rclBounds, pt, cptl, 0 );
    else
        emr->rclBounds = empty_bounds;
    emr->nPolys = polys;
    emr->cptl = cptl;

    if(polys)
    {
        memcpy( emr->aPolyCounts, counts, polys * sizeof(DWORD) );
        store_points( (POINTL *)(emr->aPolyCounts + polys), pt, cptl, use_small_emr );
    }

    ret = emfdc_record( emf, &emr->emr );
    if(ret && !bounds_valid)
    {
        ret = FALSE;
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    if(ret && !emf->path)
        emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/**********************************************************************
 *          EMFDC_PolyPolyline
 */
BOOL EMFDC_PolyPolyline( DC_ATTR *dc_attr, const POINT *pt, const DWORD *counts, DWORD polys)
{
    return EMFDC_PolyPolylinegon( dc_attr->emf, pt, (const INT *)counts, polys, EMR_POLYPOLYLINE );
}

/**********************************************************************
 *          EMFDC_PolyPolygon
 */
BOOL EMFDC_PolyPolygon( DC_ATTR *dc_attr, const POINT *pt, const INT *counts, UINT polys )
{
    return EMFDC_PolyPolylinegon( dc_attr->emf, pt, counts, polys, EMR_POLYPOLYGON );
}

/**********************************************************************
 *          EMFDC_PolyDraw
 */
BOOL EMFDC_PolyDraw( DC_ATTR *dc_attr, const POINT *pts, const BYTE *types, DWORD count )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRPOLYDRAW *emr;
    BOOL ret;
    BYTE *types_dest;
    BOOL use_small_emr = can_use_short_points( pts, count );
    DWORD size;

    size = use_small_emr ? offsetof( EMRPOLYDRAW16, apts[count] ) : offsetof( EMRPOLYDRAW, aptl[count] );
    size += (count + 3) & ~3;

    if (!(emr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;

    emr->emr.iType = use_small_emr ? EMR_POLYDRAW16 : EMR_POLYDRAW;
    emr->emr.nSize = size;
    emr->cptl = count;

    types_dest = store_points( emr->aptl, pts, count, use_small_emr );
    memcpy( types_dest, types, count );
    if (count & 3) memset( types_dest + count, 0, 4 - (count & 3) );

    if (!emf->path)
        get_points_bounds( &emr->rclBounds, pts, count, 0 );
    else
        emr->rclBounds = empty_bounds;

    ret = emfdc_record( emf, &emr->emr );
    if (ret && !emf->path) emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}


/**********************************************************************
 *          EMFDC_ExtFloodFill
 */
BOOL EMFDC_ExtFloodFill( DC_ATTR *dc_attr, INT x, INT y, COLORREF color, UINT fill_type )
{
    EMREXTFLOODFILL emr;

    emr.emr.iType = EMR_EXTFLOODFILL;
    emr.emr.nSize = sizeof(emr);
    emr.ptlStart.x = x;
    emr.ptlStart.y = y;
    emr.crColor = color;
    emr.iMode = fill_type;

    return emfdc_record( dc_attr->emf, &emr.emr );
}


/*********************************************************************
 *          EMFDC_FillRgn
 */
BOOL EMFDC_FillRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRFILLRGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = emfdc_create_brush( emf, hbrush );
    if(!index) return FALSE;

    rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRFILLRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FILLRGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;

    ret = emfdc_record( emf, &emr->emr );
    if(ret)
        emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}


/*********************************************************************
 *          EMFDC_FrameRgn
 */
BOOL EMFDC_FrameRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRFRAMERGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = emfdc_create_brush( emf, hbrush );
    if(!index) return FALSE;

    rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRFRAMERGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FRAMERGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;
    emr->szlStroke.cx = width;
    emr->szlStroke.cy = height;

    ret = emfdc_record( emf, &emr->emr );
    if(ret)
        emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/*********************************************************************
 *          EMF_PaintInvertRgn
 *
 * Helper for EMFDRV_{Paint|Invert}Rgn
 */
static BOOL EMF_PaintInvertRgn( struct emf *emf, HRGN hrgn, DWORD iType )
{
    EMRINVERTRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;

    rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRINVERTRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = iType;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;

    ret = emfdc_record( emf, &emr->emr );
    if(ret)
        emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/**********************************************************************
 *          EMFDC_PaintRgn
 */
BOOL EMFDC_PaintRgn( DC_ATTR *dc_attr, HRGN hrgn )
{
    return EMF_PaintInvertRgn( dc_attr->emf, hrgn, EMR_PAINTRGN );
}

/**********************************************************************
 *          EMF_InvertRgn
 */
BOOL EMFDC_InvertRgn( DC_ATTR *dc_attr, HRGN hrgn )
{
    return EMF_PaintInvertRgn( dc_attr->emf, hrgn, EMR_INVERTRGN );
}

/**********************************************************************
 *          EMFDC_ExtTextOut
 */
BOOL EMFDC_ExtTextOut( DC_ATTR *dc_attr, INT x, INT y, UINT flags, const RECT *lprect,
                       const WCHAR *str, UINT count, const INT *lpDx )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMREXTTEXTOUTW *pemr;
    DWORD nSize;
    BOOL ret;
    int textHeight = 0;
    int textWidth = 0;
    const UINT textAlign = dc_attr->text_align;
    const INT graphicsMode = dc_attr->graphics_mode;
    FLOAT exScale, eyScale;

    nSize = sizeof(*pemr) + ((count+1) & ~1) * sizeof(WCHAR) + count * sizeof(INT);

    TRACE("%s %s count %d nSize = %d\n", debugstr_wn(str, count),
           wine_dbgstr_rect(lprect), count, nSize);
    pemr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nSize);

    if (graphicsMode == GM_COMPATIBLE)
    {
        const INT horzSize = GetDeviceCaps( emf->dev.hdc, HORZSIZE );
        const INT horzRes  = GetDeviceCaps( emf->dev.hdc, HORZRES );
        const INT vertSize = GetDeviceCaps( emf->dev.hdc, VERTSIZE );
        const INT vertRes  = GetDeviceCaps( emf->dev.hdc, VERTRES );
        SIZE wndext, vportext;

        GetViewportExtEx( emf->dev.hdc, &vportext );
        GetWindowExtEx( emf->dev.hdc, &wndext );
        exScale = 100.0 * ((FLOAT)horzSize  / (FLOAT)horzRes) /
                          ((FLOAT)wndext.cx / (FLOAT)vportext.cx);
        eyScale = 100.0 * ((FLOAT)vertSize  / (FLOAT)vertRes) /
                          ((FLOAT)wndext.cy / (FLOAT)vportext.cy);
    }
    else
    {
        exScale = 0.0;
        eyScale = 0.0;
    }

    pemr->emr.iType = EMR_EXTTEXTOUTW;
    pemr->emr.nSize = nSize;
    pemr->iGraphicsMode = graphicsMode;
    pemr->exScale = exScale;
    pemr->eyScale = eyScale;
    pemr->emrtext.ptlReference.x = x;
    pemr->emrtext.ptlReference.y = y;
    pemr->emrtext.nChars = count;
    pemr->emrtext.offString = sizeof(*pemr);
    memcpy((char*)pemr + pemr->emrtext.offString, str, count * sizeof(WCHAR));
    pemr->emrtext.fOptions = flags;
    if(!lprect) {
        pemr->emrtext.rcl.left = pemr->emrtext.rcl.top = 0;
        pemr->emrtext.rcl.right = pemr->emrtext.rcl.bottom = -1;
    } else {
        pemr->emrtext.rcl.left = lprect->left;
        pemr->emrtext.rcl.top = lprect->top;
        pemr->emrtext.rcl.right = lprect->right;
        pemr->emrtext.rcl.bottom = lprect->bottom;
    }

    pemr->emrtext.offDx = pemr->emrtext.offString + ((count+1) & ~1) * sizeof(WCHAR);
    if(lpDx) {
        UINT i;
        SIZE strSize;
        memcpy((char*)pemr + pemr->emrtext.offDx, lpDx, count * sizeof(INT));
        for (i = 0; i < count; i++) {
            textWidth += lpDx[i];
        }
        if (GetTextExtentPoint32W( emf->dev.hdc, str, count, &strSize ))
            textHeight = strSize.cy;
    }
    else {
        UINT i;
        INT *dx = (INT *)((char*)pemr + pemr->emrtext.offDx);
        SIZE charSize;
        for (i = 0; i < count; i++) {
            if (GetTextExtentPoint32W( emf->dev.hdc, str + i, 1, &charSize )) {
                dx[i] = charSize.cx;
                textWidth += charSize.cx;
                textHeight = max(textHeight, charSize.cy);
            }
        }
    }

    if (emf->path)
    {
        pemr->rclBounds.left = pemr->rclBounds.top = 0;
        pemr->rclBounds.right = pemr->rclBounds.bottom = -1;
        goto no_bounds;
    }

    /* FIXME: handle font escapement */
    switch (textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER)) {
    case TA_CENTER: {
        pemr->rclBounds.left  = x - (textWidth / 2) - 1;
        pemr->rclBounds.right = x + (textWidth / 2) + 1;
        break;
    }
    case TA_RIGHT: {
        pemr->rclBounds.left  = x - textWidth - 1;
        pemr->rclBounds.right = x;
        break;
    }
    default: { /* TA_LEFT */
        pemr->rclBounds.left  = x;
        pemr->rclBounds.right = x + textWidth + 1;
    }
    }

    switch (textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE)) {
    case TA_BASELINE: {
        TEXTMETRICW tm;
        if (!GetTextMetricsW( emf->dev.hdc, &tm ))
            tm.tmDescent = 0;
        /* Play safe here... it's better to have a bounding box */
        /* that is too big than too small. */
        pemr->rclBounds.top    = y - textHeight - 1;
        pemr->rclBounds.bottom = y + tm.tmDescent + 1;
        break;
    }
    case TA_BOTTOM: {
        pemr->rclBounds.top    = y - textHeight - 1;
        pemr->rclBounds.bottom = y;
        break;
    }
    default: { /* TA_TOP */
        pemr->rclBounds.top    = y;
        pemr->rclBounds.bottom = y + textHeight + 1;
    }
    }
    emfdc_update_bounds( emf, &pemr->rclBounds );

no_bounds:
    ret = emfdc_record( emf, &pemr->emr );
    HeapFree( GetProcessHeap(), 0, pemr );
    return ret;
}

/**********************************************************************
 *          EMFDC_GradientFill
 */
BOOL EMFDC_GradientFill( DC_ATTR *dc_attr, TRIVERTEX *vert_array, ULONG nvert,
                         void *grad_array, ULONG ngrad, ULONG mode )
{
    EMRGRADIENTFILL *emr;
    ULONG i, pt, size, num_pts = ngrad * (mode == GRADIENT_FILL_TRIANGLE ? 3 : 2);
    const ULONG *pts = (const ULONG *)grad_array;
    BOOL ret;

    size = FIELD_OFFSET(EMRGRADIENTFILL, Ver[nvert]) + num_pts * sizeof(pts[0]);

    emr = HeapAlloc( GetProcessHeap(), 0, size );
    if (!emr) return FALSE;

    for (i = 0; i < num_pts; i++)
    {
        pt = pts[i];

        if (i == 0)
        {
            emr->rclBounds.left = emr->rclBounds.right = vert_array[pt].x;
            emr->rclBounds.top = emr->rclBounds.bottom = vert_array[pt].y;
        }
        else
        {
            if (vert_array[pt].x < emr->rclBounds.left)
                emr->rclBounds.left = vert_array[pt].x;
            else if (vert_array[pt].x > emr->rclBounds.right)
                emr->rclBounds.right = vert_array[pt].x;
            if (vert_array[pt].y < emr->rclBounds.top)
                emr->rclBounds.top = vert_array[pt].y;
            else if (vert_array[pt].y > emr->rclBounds.bottom)
                emr->rclBounds.bottom = vert_array[pt].y;
        }
    }
    emr->rclBounds.right--;
    emr->rclBounds.bottom--;

    emr->emr.iType = EMR_GRADIENTFILL;
    emr->emr.nSize = size;
    emr->nVer = nvert;
    emr->nTri = ngrad;
    emr->ulMode = mode;
    memcpy( emr->Ver, vert_array, nvert * sizeof(vert_array[0]) );
    memcpy( emr->Ver + nvert, pts, num_pts * sizeof(pts[0]) );

    emfdc_update_bounds( dc_attr->emf, &emr->rclBounds );
    ret = emfdc_record( dc_attr->emf, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/**********************************************************************
 *	     EMFDC_FillPath
 */
BOOL EMFDC_FillPath( DC_ATTR *dc_attr )
{
    return emfdrv_stroke_and_fill_path( dc_attr->emf, EMR_FILLPATH );
}

/**********************************************************************
 *	     EMFDC_StrokeAndFillPath
 */
BOOL EMFDC_StrokeAndFillPath( DC_ATTR *dc_attr )
{
    return emfdrv_stroke_and_fill_path( dc_attr->emf, EMR_STROKEANDFILLPATH );
}

/**********************************************************************
 *           EMFDC_StrokePath
 */
BOOL EMFDC_StrokePath( DC_ATTR *dc_attr )
{
    return emfdrv_stroke_and_fill_path( dc_attr->emf, EMR_STROKEPATH );
}

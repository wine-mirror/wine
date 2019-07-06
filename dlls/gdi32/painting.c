/*
 * GDI drawing functions.
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


/***********************************************************************
 *           null driver fallback implementations
 */

BOOL CDECL nulldrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep )
{
    INT x1 = GDI_ROUND( x + cos( start * M_PI / 180 ) * radius );
    INT y1 = GDI_ROUND( y - sin( start * M_PI / 180 ) * radius );
    INT x2 = GDI_ROUND( x + cos( (start + sweep) * M_PI / 180) * radius );
    INT y2 = GDI_ROUND( y - sin( (start + sweep) * M_PI / 180) * radius );
    INT arcdir = SetArcDirection( dev->hdc, sweep >= 0 ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE );
    BOOL ret = ArcTo( dev->hdc, x - radius, y - radius, x + radius, y + radius, x1, y1, x2, y2 );
    SetArcDirection( dev->hdc, arcdir );
    return ret;
}

BOOL CDECL nulldrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend )
{
    INT width = abs( right - left );
    INT height = abs( bottom - top );
    double xradius = width / 2.0;
    double yradius = height / 2.0;
    double xcenter = right > left ? left + xradius : right + xradius;
    double ycenter = bottom > top ? top + yradius : bottom + yradius;
    double angle;

    if (!height || !width) return FALSE;
    /* draw a line from the current position to the starting point of the arc, then draw the arc */
    angle = atan2( (ystart - ycenter) / height, (xstart - xcenter) / width );
    LineTo( dev->hdc, GDI_ROUND( xcenter + cos(angle) * xradius ),
            GDI_ROUND( ycenter + sin(angle) * yradius ));
    return Arc( dev->hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}

BOOL CDECL nulldrv_FillRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush )
{
    BOOL ret = FALSE;
    HBRUSH prev;

    if ((prev = SelectObject( dev->hdc, brush )))
    {
        ret = PaintRgn( dev->hdc, rgn );
        SelectObject( dev->hdc, prev );
    }
    return ret;
}

BOOL CDECL nulldrv_FrameRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush, INT width, INT height )
{
    BOOL ret = FALSE;
    HRGN tmp = CreateRectRgn( 0, 0, 0, 0 );

    if (tmp)
    {
        if (REGION_FrameRgn( tmp, rgn, width, height )) ret = FillRgn( dev->hdc, tmp, brush );
        DeleteObject( tmp );
    }
    return ret;
}

BOOL CDECL nulldrv_InvertRgn( PHYSDEV dev, HRGN rgn )
{
    HBRUSH prev_brush = SelectObject( dev->hdc, GetStockObject(BLACK_BRUSH) );
    INT prev_rop = SetROP2( dev->hdc, R2_NOT );
    BOOL ret = PaintRgn( dev->hdc, rgn );
    SelectObject( dev->hdc, prev_brush );
    SetROP2( dev->hdc, prev_rop );
    return ret;
}

BOOL CDECL nulldrv_PolyBezier( PHYSDEV dev, const POINT *points, DWORD count )
{
    BOOL ret = FALSE;
    POINT *pts;
    INT n;

    if ((pts = GDI_Bezier( points, count, &n )))
    {
        ret = Polyline( dev->hdc, pts, n );
        HeapFree( GetProcessHeap(), 0, pts );
    }
    return ret;
}

BOOL CDECL nulldrv_PolyBezierTo( PHYSDEV dev, const POINT *points, DWORD count )
{
    DC *dc = get_nulldrv_dc( dev );
    BOOL ret = FALSE;
    POINT *pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * (count + 1) );

    if (pts)
    {
        pts[0] = dc->cur_pos;
        memcpy( pts + 1, points, sizeof(POINT) * count );
        ret = PolyBezier( dev->hdc, pts, count + 1 );
        HeapFree( GetProcessHeap(), 0, pts );
    }
    return ret;
}

BOOL CDECL nulldrv_PolyDraw( PHYSDEV dev, const POINT *points, const BYTE *types, DWORD count )
{
    DC *dc = get_nulldrv_dc( dev );
    POINT *line_pts = NULL, *bzr_pts = NULL, bzr[4];
    DWORD i;
    INT num_pts, num_bzr_pts, space, size;

    /* check for valid point types */
    for (i = 0; i < count; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
        case PT_LINETO | PT_CLOSEFIGURE:
        case PT_LINETO:
            break;
        case PT_BEZIERTO:
            if (i + 2 >= count) return FALSE;
            if (types[i + 1] != PT_BEZIERTO) return FALSE;
            if ((types[i + 2] & ~PT_CLOSEFIGURE) != PT_BEZIERTO) return FALSE;
            i += 2;
            break;
        default:
            return FALSE;
        }
    }

    space = count + 300;
    line_pts = HeapAlloc( GetProcessHeap(), 0, space * sizeof(POINT) );
    num_pts = 1;

    line_pts[0] = dc->cur_pos;
    for (i = 0; i < count; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
            if (num_pts >= 2) Polyline( dev->hdc, line_pts, num_pts );
            num_pts = 0;
            line_pts[num_pts++] = points[i];
            break;
        case PT_LINETO:
        case (PT_LINETO | PT_CLOSEFIGURE):
            line_pts[num_pts++] = points[i];
            break;
        case PT_BEZIERTO:
            bzr[0].x = line_pts[num_pts - 1].x;
            bzr[0].y = line_pts[num_pts - 1].y;
            memcpy( &bzr[1], &points[i], 3 * sizeof(POINT) );

            if ((bzr_pts = GDI_Bezier( bzr, 4, &num_bzr_pts )))
            {
                size = num_pts + (count - i) + num_bzr_pts;
                if (space < size)
                {
                    space = size * 2;
                    line_pts = HeapReAlloc( GetProcessHeap(), 0, line_pts, space * sizeof(POINT) );
                }
                memcpy( &line_pts[num_pts], &bzr_pts[1], (num_bzr_pts - 1) * sizeof(POINT) );
                num_pts += num_bzr_pts - 1;
                HeapFree( GetProcessHeap(), 0, bzr_pts );
            }
            i += 2;
            break;
        }
        if (types[i] & PT_CLOSEFIGURE) line_pts[num_pts++] = line_pts[0];
    }

    if (num_pts >= 2) Polyline( dev->hdc, line_pts, num_pts );
    HeapFree( GetProcessHeap(), 0, line_pts );
    return TRUE;
}

BOOL CDECL nulldrv_PolylineTo( PHYSDEV dev, const POINT *points, INT count )
{
    DC *dc = get_nulldrv_dc( dev );
    BOOL ret = FALSE;
    POINT *pts;

    if (!count) return FALSE;
    if ((pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * (count + 1) )))
    {
        pts[0] = dc->cur_pos;
        memcpy( pts + 1, points, sizeof(POINT) * count );
        ret = Polyline( dev->hdc, pts, count + 1 );
        HeapFree( GetProcessHeap(), 0, pts );
    }
    return ret;
}

/***********************************************************************
 *           LineTo    (GDI32.@)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC * dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    BOOL ret;

    TRACE( "%p, (%d, %d)\n", hdc, x, y );

    if(!dc) return FALSE;

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pLineTo );
    ret = physdev->funcs->pLineTo( physdev, x, y );

    if(ret)
    {
        dc->cur_pos.x = x;
        dc->cur_pos.y = y;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           MoveToEx    (GDI32.@)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret;
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d), %p\n", hdc, x, y, pt );

    if(!dc) return FALSE;

    if(pt)
        *pt = dc->cur_pos;

    dc->cur_pos.x = x;
    dc->cur_pos.y = y;

    physdev = GET_DC_PHYSDEV( dc, pMoveTo );
    ret = physdev->funcs->pMoveTo( physdev, x, y );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Arc    (GDI32.@)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right,
                     INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top, right, bottom, xstart, ystart, xend, yend );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pArc );
    ret = physdev->funcs->pArc( physdev, left, top, right, bottom, xstart, ystart, xend, yend );
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           ArcTo    (GDI32.@)
 */
BOOL WINAPI ArcTo( HDC hdc,
                     INT left,   INT top,
                     INT right,  INT bottom,
                     INT xstart, INT ystart,
                     INT xend,   INT yend )
{
    double width = abs( right - left ),
        height = abs( bottom - top ),
        xradius = width/2,
        yradius = height/2,
        xcenter = right > left ? left+xradius : right+xradius,
        ycenter = bottom > top ? top+yradius : bottom+yradius,
        angle;
    PHYSDEV physdev;
    BOOL result;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top, right, bottom, xstart, ystart, xend, yend );

    if(!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pArcTo );
    result = physdev->funcs->pArcTo( physdev, left, top, right, bottom, xstart, ystart, xend, yend );

    if (result)
    {
        angle = atan2(((yend-ycenter)/height),
                      ((xend-xcenter)/width));
        dc->cur_pos.x = GDI_ROUND( xcenter + (cos( angle ) * xradius) );
        dc->cur_pos.y = GDI_ROUND( ycenter + (sin( angle ) * yradius) );
    }
    release_dc_ptr( dc );
    return result;
}


/***********************************************************************
 *           Pie   (GDI32.@)
 */
BOOL WINAPI Pie( HDC hdc, INT left, INT top,
                     INT right, INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    BOOL ret;
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top, right, bottom, xstart, ystart, xend, yend );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPie );
    ret = physdev->funcs->pPie( physdev, left, top, right, bottom, xstart, ystart, xend, yend );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Chord    (GDI32.@)
 */
BOOL WINAPI Chord( HDC hdc, INT left, INT top,
                       INT right, INT bottom, INT xstart, INT ystart,
                       INT xend, INT yend )
{
    BOOL ret;
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top, right, bottom, xstart, ystart, xend, yend );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pChord );
    ret = physdev->funcs->pChord( physdev, left, top, right, bottom, xstart, ystart, xend, yend );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Ellipse    (GDI32.@)
 */
BOOL WINAPI Ellipse( HDC hdc, INT left, INT top,
                         INT right, INT bottom )
{
    BOOL ret;
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d)\n", hdc, left, top, right, bottom );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pEllipse );
    ret = physdev->funcs->pEllipse( physdev, left, top, right, bottom );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Rectangle    (GDI32.@)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top,
                           INT right, INT bottom )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d)\n", hdc, left, top, right, bottom );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pRectangle );
    ret = physdev->funcs->pRectangle( physdev, left, top, right, bottom );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           RoundRect    (GDI32.@)
 */
BOOL WINAPI RoundRect( HDC hdc, INT left, INT top, INT right,
                           INT bottom, INT ell_width, INT ell_height )
{
    PHYSDEV physdev;
    BOOL ret;
    DC *dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d)-(%d, %d), %dx%d\n", hdc, left, top, right, bottom, ell_width, ell_height );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pRoundRect );
    ret = physdev->funcs->pRoundRect( physdev, left, top, right, bottom, ell_width, ell_height );
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           SetPixel    (GDI32.@)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    PHYSDEV physdev;
    COLORREF ret;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return 0;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pSetPixel );
    ret = physdev->funcs->pSetPixel( physdev, x, y, color );
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           SetPixelV    (GDI32.@)
 */
BOOL WINAPI SetPixelV( HDC hdc, INT x, INT y, COLORREF color )
{
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pSetPixel );
    physdev->funcs->pSetPixel( physdev, x, y, color );
    release_dc_ptr( dc );
    return TRUE;
}

/***********************************************************************
 *           GetPixel    (GDI32.@)
 */
COLORREF WINAPI GetPixel( HDC hdc, INT x, INT y )
{
    PHYSDEV physdev;
    COLORREF ret;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return CLR_INVALID;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pGetPixel );
    ret = physdev->funcs->pGetPixel( physdev, x, y );
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * GdiSetPixelFormat [GDI32.@]
 *
 * Probably not the correct semantics, it's supposed to be an internal backend for SetPixelFormat.
 */
BOOL WINAPI GdiSetPixelFormat( HDC hdc, INT format, const PIXELFORMATDESCRIPTOR *descr )
{
    DC *dc;
    BOOL ret = TRUE;

    TRACE("(%p,%d,%p)\n", hdc, format, descr);

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    if (!dc->pixel_format) dc->pixel_format = format;
    else ret = (dc->pixel_format == format);
    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * GdiDescribePixelFormat [GDI32.@]
 *
 * Probably not the correct semantics, it's supposed to be an internal backend for DescribePixelFormat.
 */
INT WINAPI GdiDescribePixelFormat( HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    FIXME( "(%p,%d,%d,%p): stub\n", hdc, format, size, descr );
    return 0;
}


/******************************************************************************
 * GdiSwapBuffers [GDI32.@]
 *
 * Probably not the correct semantics, it's supposed to be an internal backend for SwapBuffers.
 */
BOOL WINAPI GdiSwapBuffers( HDC hdc )
{
    FIXME( "(%p): stub\n", hdc );
    return FALSE;
}


/***********************************************************************
 *           PaintRgn    (GDI32.@)
 */
BOOL WINAPI PaintRgn( HDC hdc, HRGN hrgn )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, %p\n", hdc, hrgn );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPaintRgn );
    ret = physdev->funcs->pPaintRgn( physdev, hrgn );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           FillRgn    (GDI32.@)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    PHYSDEV physdev;
    BOOL retval;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, %p, %p\n", hdc, hrgn, hbrush );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pFillRgn );
    retval = physdev->funcs->pFillRgn( physdev, hrgn, hbrush );
    release_dc_ptr( dc );
    return retval;
}


/***********************************************************************
 *           FrameRgn     (GDI32.@)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush,
                          INT nWidth, INT nHeight )
{
    PHYSDEV physdev;
    BOOL ret;
    DC *dc = get_dc_ptr( hdc );

    TRACE( "%p, %p, %p, %dx%d\n", hdc, hrgn, hbrush, nWidth, nHeight );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pFrameRgn );
    ret = physdev->funcs->pFrameRgn( physdev, hrgn, hbrush, nWidth, nHeight );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           InvertRgn    (GDI32.@)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    PHYSDEV physdev;
    BOOL ret;
    DC *dc = get_dc_ptr( hdc );

    TRACE( "%p, %p\n", hdc, hrgn );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pInvertRgn );
    ret = physdev->funcs->pInvertRgn( physdev, hrgn );
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *          Polyline   (GDI32.@)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT* pt, INT count )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, %p, %d\n", hdc, pt, count );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolyline );
    ret = physdev->funcs->pPolyline( physdev, pt, count );
    release_dc_ptr( dc );
    return ret;
}

/**********************************************************************
 *          PolylineTo   (GDI32.@)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT* pt, DWORD cCount )
{
    DC * dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    BOOL ret;

    TRACE( "%p, %p, %u\n", hdc, pt, cCount );

    if(!dc) return FALSE;

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolylineTo );
    ret = physdev->funcs->pPolylineTo( physdev, pt, cCount );

    if (ret && cCount)
        dc->cur_pos = pt[cCount - 1];

    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *          Polygon  (GDI32.@)
 */
BOOL WINAPI Polygon( HDC hdc, const POINT* pt, INT count )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, %p, %d\n", hdc, pt, count );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolygon );
    ret = physdev->funcs->pPolygon( physdev, pt, count );
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *          PolyPolygon  (GDI32.@)
 */
BOOL WINAPI PolyPolygon( HDC hdc, const POINT* pt, const INT* counts,
                             UINT polygons )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, %p, %p, %u\n", hdc, pt, counts, polygons );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolyPolygon );
    ret = physdev->funcs->pPolyPolygon( physdev, pt, counts, polygons );
    release_dc_ptr( dc );
    return ret;
}

/**********************************************************************
 *          PolyPolyline  (GDI32.@)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT* pt, const DWORD* counts,
                            DWORD polylines )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, %p, %p, %u\n", hdc, pt, counts, polylines );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolyPolyline );
    ret = physdev->funcs->pPolyPolyline( physdev, pt, counts, polylines );
    release_dc_ptr( dc );
    return ret;
}

/**********************************************************************
 *          ExtFloodFill   (GDI32.@)
 */
BOOL WINAPI ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                              UINT fillType )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc = get_dc_ptr( hdc );

    TRACE( "%p, (%d, %d), %08x, %x\n", hdc, x, y, color, fillType );

    if (!dc) return FALSE;
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pExtFloodFill );
    ret = physdev->funcs->pExtFloodFill( physdev, x, y, color, fillType );
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *          FloodFill   (GDI32.@)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}


/******************************************************************************
 * PolyBezier [GDI32.@]
 * Draws one or more Bezier curves
 *
 * PARAMS
 *    hDc     [I] Handle to device context
 *    lppt    [I] Pointer to endpoints and control points
 *    cPoints [I] Count of endpoints and control points
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI PolyBezier( HDC hdc, const POINT* lppt, DWORD cPoints )
{
    PHYSDEV physdev;
    BOOL ret;
    DC * dc;

    TRACE( "%p, %p, %u\n", hdc, lppt, cPoints );

    /* cPoints must be 3 * n + 1 (where n>=1) */
    if (cPoints == 1 || (cPoints % 3) != 1) return FALSE;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolyBezier );
    ret = physdev->funcs->pPolyBezier( physdev, lppt, cPoints );
    release_dc_ptr( dc );
    return ret;
}

/******************************************************************************
 * PolyBezierTo [GDI32.@]
 * Draws one or more Bezier curves
 *
 * PARAMS
 *    hDc     [I] Handle to device context
 *    lppt    [I] Pointer to endpoints and control points
 *    cPoints [I] Count of endpoints and control points
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI PolyBezierTo( HDC hdc, const POINT* lppt, DWORD cPoints )
{
    DC * dc;
    BOOL ret;
    PHYSDEV physdev;

    TRACE( "%p, %p, %u\n", hdc, lppt, cPoints );

    /* cbPoints must be 3 * n (where n>=1) */
    if (!cPoints || (cPoints % 3) != 0) return FALSE;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolyBezierTo );
    ret = physdev->funcs->pPolyBezierTo( physdev, lppt, cPoints );

    if(ret)
        dc->cur_pos = lppt[cPoints - 1];

    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *      AngleArc (GDI32.@)
 */
BOOL WINAPI AngleArc(HDC hdc, INT x, INT y, DWORD dwRadius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
    PHYSDEV physdev;
    BOOL result;
    DC *dc;

    TRACE( "%p, (%d, %d), %u, %f, %f\n", hdc, x, y, dwRadius, eStartAngle, eSweepAngle );

    if( (signed int)dwRadius < 0 )
	return FALSE;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pAngleArc );
    result = physdev->funcs->pAngleArc( physdev, x, y, dwRadius, eStartAngle, eSweepAngle );

    if (result)
    {
        dc->cur_pos.x = GDI_ROUND( x + cos( (eStartAngle + eSweepAngle) * M_PI / 180 ) * dwRadius );
        dc->cur_pos.y = GDI_ROUND( y - sin( (eStartAngle + eSweepAngle) * M_PI / 180 ) * dwRadius );
    }
    release_dc_ptr( dc );
    return result;
}

/***********************************************************************
 *      PolyDraw (GDI32.@)
 */
BOOL WINAPI PolyDraw(HDC hdc, const POINT *lppt, const BYTE *lpbTypes,
                       DWORD cCount)
{
    DC *dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    BOOL result;

    TRACE( "%p, %p, %p, %u\n", hdc, lppt, lpbTypes, cCount );

    if(!dc) return FALSE;

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pPolyDraw );
    result = physdev->funcs->pPolyDraw( physdev, lppt, lpbTypes, cCount );
    if (result && cCount)
        dc->cur_pos = lppt[cCount - 1];

    release_dc_ptr( dc );
    return result;
}


/**********************************************************************
 *           LineDDA   (GDI32.@)
 */
BOOL WINAPI LineDDA(INT nXStart, INT nYStart, INT nXEnd, INT nYEnd,
                    LINEDDAPROC callback, LPARAM lParam )
{
    INT xadd = 1, yadd = 1;
    INT err,erradd;
    INT cnt;
    INT dx = nXEnd - nXStart;
    INT dy = nYEnd - nYStart;

    TRACE( "(%d, %d), (%d, %d), %p, %lx\n", nXStart, nYStart, nXEnd, nYEnd, callback, lParam );

    if (dx < 0)
    {
        dx = -dx;
        xadd = -1;
    }
    if (dy < 0)
    {
        dy = -dy;
        yadd = -1;
    }
    if (dx > dy)  /* line is "more horizontal" */
    {
        err = 2*dy - dx; erradd = 2*dy - 2*dx;
        for(cnt = 0;cnt < dx; cnt++)
        {
            callback(nXStart,nYStart,lParam);
            if (err > 0)
            {
                nYStart += yadd;
                err += erradd;
            }
            else err += 2*dy;
            nXStart += xadd;
        }
    }
    else   /* line is "more vertical" */
    {
        err = 2*dx - dy; erradd = 2*dx - 2*dy;
        for(cnt = 0;cnt < dy; cnt++)
        {
            callback(nXStart,nYStart,lParam);
            if (err > 0)
            {
                nXStart += xadd;
                err += erradd;
            }
            else err += 2*dx;
            nYStart += yadd;
        }
    }
    return TRUE;
}


/******************************************************************
 *
 *   *Very* simple bezier drawing code,
 *
 *   It uses a recursive algorithm to divide the curve in a series
 *   of straight line segments. Not ideal but sufficient for me.
 *   If you are in need for something better look for some incremental
 *   algorithm.
 *
 *   7 July 1998 Rein Klazes
 */

 /*
  * some macro definitions for bezier drawing
  *
  * to avoid truncation errors the coordinates are
  * shifted upwards. When used in drawing they are
  * shifted down again, including correct rounding
  * and avoiding floating point arithmetic
  * 4 bits should allow 27 bits coordinates which I saw
  * somewhere in the win32 doc's
  *
  */

#define BEZIERSHIFTBITS 4
#define BEZIERSHIFTUP(x)    ((x)<<BEZIERSHIFTBITS)
#define BEZIERPIXEL        BEZIERSHIFTUP(1)
#define BEZIERSHIFTDOWN(x)  (((x)+(1<<(BEZIERSHIFTBITS-1)))>>BEZIERSHIFTBITS)
/* maximum depth of recursion */
#define BEZIERMAXDEPTH  8

/* size of array to store points on */
/* enough for one curve */
#define BEZIER_INITBUFSIZE    (150)

/* calculate Bezier average, in this case the middle
 * correctly rounded...
 * */

#define BEZIERMIDDLE(Mid, P1, P2) \
    (Mid).x=((P1).x+(P2).x + 1)/2;\
    (Mid).y=((P1).y+(P2).y + 1)/2;

/**********************************************************
* BezierCheck helper function to check
* that recursion can be terminated
*       Points[0] and Points[3] are begin and endpoint
*       Points[1] and Points[2] are control points
*       level is the recursion depth
*       returns true if the recursion can be terminated
*/
static BOOL BezierCheck( int level, POINT *Points)
{
    INT dx, dy;
    dx=Points[3].x-Points[0].x;
    dy=Points[3].y-Points[0].y;
    if(abs(dy)<=abs(dx)){/* shallow line */
        /* check that control points are between begin and end */
        if(Points[1].x < Points[0].x){
            if(Points[1].x < Points[3].x)
                return FALSE;
        }else
            if(Points[1].x > Points[3].x)
                return FALSE;
        if(Points[2].x < Points[0].x){
            if(Points[2].x < Points[3].x)
                return FALSE;
        }else
            if(Points[2].x > Points[3].x)
                return FALSE;
        dx=BEZIERSHIFTDOWN(dx);
        if(!dx) return TRUE;
        if(abs(Points[1].y-Points[0].y-(dy/dx)*
                BEZIERSHIFTDOWN(Points[1].x-Points[0].x)) > BEZIERPIXEL ||
           abs(Points[2].y-Points[0].y-(dy/dx)*
                   BEZIERSHIFTDOWN(Points[2].x-Points[0].x)) > BEZIERPIXEL )
            return FALSE;
        else
            return TRUE;
    }else{ /* steep line */
        /* check that control points are between begin and end */
        if(Points[1].y < Points[0].y){
            if(Points[1].y < Points[3].y)
                return FALSE;
        }else
            if(Points[1].y > Points[3].y)
                return FALSE;
        if(Points[2].y < Points[0].y){
            if(Points[2].y < Points[3].y)
                return FALSE;
        }else
            if(Points[2].y > Points[3].y)
                return FALSE;
        dy=BEZIERSHIFTDOWN(dy);
        if(!dy) return TRUE;
        if(abs(Points[1].x-Points[0].x-(dx/dy)*
                BEZIERSHIFTDOWN(Points[1].y-Points[0].y)) > BEZIERPIXEL ||
           abs(Points[2].x-Points[0].x-(dx/dy)*
                   BEZIERSHIFTDOWN(Points[2].y-Points[0].y)) > BEZIERPIXEL )
            return FALSE;
        else
            return TRUE;
    }
}

/* Helper for GDI_Bezier.
 * Just handles one Bezier, so Points should point to four POINTs
 */
static void GDI_InternalBezier( POINT *Points, POINT **PtsOut, INT *dwOut,
				INT *nPtsOut, INT level )
{
    if(*nPtsOut == *dwOut) {
        *dwOut *= 2;
	*PtsOut = HeapReAlloc( GetProcessHeap(), 0, *PtsOut,
			       *dwOut * sizeof(POINT) );
    }

    if(!level || BezierCheck(level, Points)) {
        if(*nPtsOut == 0) {
            (*PtsOut)[0].x = BEZIERSHIFTDOWN(Points[0].x);
            (*PtsOut)[0].y = BEZIERSHIFTDOWN(Points[0].y);
            *nPtsOut = 1;
        }
	(*PtsOut)[*nPtsOut].x = BEZIERSHIFTDOWN(Points[3].x);
        (*PtsOut)[*nPtsOut].y = BEZIERSHIFTDOWN(Points[3].y);
        (*nPtsOut) ++;
    } else {
        POINT Points2[4]; /* for the second recursive call */
        Points2[3]=Points[3];
        BEZIERMIDDLE(Points2[2], Points[2], Points[3]);
        BEZIERMIDDLE(Points2[0], Points[1], Points[2]);
        BEZIERMIDDLE(Points2[1],Points2[0],Points2[2]);

        BEZIERMIDDLE(Points[1], Points[0],  Points[1]);
        BEZIERMIDDLE(Points[2], Points[1], Points2[0]);
        BEZIERMIDDLE(Points[3], Points[2], Points2[1]);

        Points2[0]=Points[3];

        /* do the two halves */
        GDI_InternalBezier(Points, PtsOut, dwOut, nPtsOut, level-1);
        GDI_InternalBezier(Points2, PtsOut, dwOut, nPtsOut, level-1);
    }
}



/***********************************************************************
 *           GDI_Bezier   [INTERNAL]
 *   Calculate line segments that approximate -what microsoft calls- a bezier
 *   curve.
 *   The routine recursively divides the curve in two parts until a straight
 *   line can be drawn
 *
 *  PARAMS
 *
 *  Points  [I] Ptr to count POINTs which are the end and control points
 *              of the set of Bezier curves to flatten.
 *  count   [I] Number of Points.  Must be 3n+1.
 *  nPtsOut [O] Will contain no of points that have been produced (i.e. no. of
 *              lines+1).
 *
 *  RETURNS
 *
 *  Ptr to an array of POINTs that contain the lines that approximate the
 *  Beziers.  The array is allocated on the process heap and it is the caller's
 *  responsibility to HeapFree it. [this is not a particularly nice interface
 *  but since we can't know in advance how many points we will generate, the
 *  alternative would be to call the function twice, once to determine the size
 *  and a second time to do the work - I decided this was too much of a pain].
 */
POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut )
{
    POINT *out;
    INT Bezier, dwOut = BEZIER_INITBUFSIZE, i;

    if (count == 1 || (count - 1) % 3 != 0) {
        ERR("Invalid no. of points %d\n", count);
	return NULL;
    }
    *nPtsOut = 0;
    out = HeapAlloc( GetProcessHeap(), 0, dwOut * sizeof(POINT));
    for(Bezier = 0; Bezier < (count-1)/3; Bezier++) {
	POINT ptBuf[4];
	memcpy(ptBuf, Points + Bezier * 3, sizeof(POINT) * 4);
	for(i = 0; i < 4; i++) {
	    ptBuf[i].x = BEZIERSHIFTUP(ptBuf[i].x);
	    ptBuf[i].y = BEZIERSHIFTUP(ptBuf[i].y);
	}
        GDI_InternalBezier( ptBuf, &out, &dwOut, nPtsOut, BEZIERMAXDEPTH );
    }
    TRACE("Produced %d points\n", *nPtsOut);
    return out;
}

/******************************************************************************
 *           GdiGradientFill   (GDI32.@)
 */
BOOL WINAPI GdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                             void *grad_array, ULONG ngrad, ULONG mode )
{
    DC *dc;
    PHYSDEV physdev;
    BOOL ret;
    ULONG i;

    TRACE("%p vert_array:%p nvert:%d grad_array:%p ngrad:%d\n", hdc, vert_array, nvert, grad_array, ngrad);

    if (!vert_array || !nvert || !grad_array || !ngrad || mode > GRADIENT_FILL_TRIANGLE)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    for (i = 0; i < ngrad * (mode == GRADIENT_FILL_TRIANGLE ? 3 : 2); i++)
        if (((ULONG *)grad_array)[i] >= nvert) return FALSE;

    if (!(dc = get_dc_ptr( hdc )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pGradientFill );
    ret = physdev->funcs->pGradientFill( physdev, vert_array, nvert, grad_array, ngrad, mode );
    release_dc_ptr( dc );
    return ret;
}

/******************************************************************************
 *           GdiDrawStream   (GDI32.@)
 *
 */
BOOL WINAPI GdiDrawStream( HDC hdc, ULONG in, void * pvin )
{
    FIXME("stub: %p, %d, %p\n", hdc, in, pvin);
    return FALSE;
}

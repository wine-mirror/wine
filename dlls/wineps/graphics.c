/*
 *	PostScript driver graphics functions
 *
 *	Copyright 1998  Huw D M Davies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#include <math.h>
#if defined(HAVE_FLOAT_H)
 #include <float.h>
#endif
#if !defined(PI)
 #define PI M_PI
#endif
#include "psdrv.h"
#include "wine/debug.h"
#include "winspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/***********************************************************************
 *           PSDRV_LineTo
 */
BOOL PSDRV_LineTo(PSDRV_PDEVICE *physDev, INT x, INT y)
{
    DC *dc = physDev->dc;

    TRACE("%d %d\n", x, y);

    PSDRV_SetPen(physDev);
    PSDRV_WriteMoveTo(physDev, INTERNAL_XWPTODP(dc, dc->CursPosX, dc->CursPosY),
                      INTERNAL_YWPTODP(dc, dc->CursPosX, dc->CursPosY));
    PSDRV_WriteLineTo(physDev, INTERNAL_XWPTODP(dc, x, y),
                      INTERNAL_YWPTODP(dc, x, y));
    PSDRV_DrawLine(physDev);

    return TRUE;
}


/***********************************************************************
 *           PSDRV_Rectangle
 */
BOOL PSDRV_Rectangle( PSDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom )
{
    INT width;
    INT height;
    DC *dc = physDev->dc;

    TRACE("%d %d - %d %d\n", left, top, right, bottom);
    width = INTERNAL_XWSTODS(dc, right - left);
    height = INTERNAL_YWSTODS(dc, bottom - top);
    PSDRV_WriteRectangle(physDev, INTERNAL_XWPTODP(dc, left, top),
                         INTERNAL_YWPTODP(dc, left, top), width, height);
    PSDRV_Brush(physDev,0);
    PSDRV_SetPen(physDev);
    PSDRV_DrawLine(physDev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_RoundRect
 */
BOOL PSDRV_RoundRect( PSDRV_PDEVICE *physDev, INT left, INT top, INT right,
                      INT bottom, INT ell_width, INT ell_height )
{
    DC *dc = physDev->dc;

    left = XLPTODP( dc, left );
    right = XLPTODP( dc, right );
    top = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );
    ell_width = XLSTODS( dc, ell_width );
    ell_height = YLSTODS( dc, ell_height );

    if( left > right ) { INT tmp = left; left = right; right = tmp; }
    if( top > bottom ) { INT tmp = top; top = bottom; bottom = tmp; }

    if(ell_width > right - left) ell_width = right - left;
    if(ell_height > bottom - top) ell_height = bottom - top;

    PSDRV_WriteMoveTo( physDev, left, top + ell_height/2 );
    PSDRV_WriteArc( physDev, left + ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 90.0, 180.0);
    PSDRV_WriteLineTo( physDev, right - ell_width/2, top );
    PSDRV_WriteArc( physDev, right - ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 0.0, 90.0);
    PSDRV_WriteLineTo( physDev, right, bottom - ell_height/2 );
    PSDRV_WriteArc( physDev, right - ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, -90.0, 0.0);
    PSDRV_WriteLineTo( physDev, right - ell_width/2, bottom);
    PSDRV_WriteArc( physDev, left + ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, 180.0, -90.0);
    PSDRV_WriteClosePath( physDev );

    PSDRV_Brush(physDev,0);
    PSDRV_SetPen(physDev);
    PSDRV_DrawLine(physDev);
    return TRUE;
}

/***********************************************************************
 *           PSDRV_DrawArc
 *
 * Does the work of Arc, Chord and Pie. lines is 0, 1 or 2 respectively.
 */
static BOOL PSDRV_DrawArc( PSDRV_PDEVICE *physDev, INT left, INT top,
                           INT right, INT bottom, INT xstart, INT ystart,
                           INT xend, INT yend, int lines )
{
    DC *dc = physDev->dc;
    INT x, y, h, w;
    double start_angle, end_angle, ratio;

    x = XLPTODP(dc, (left + right)/2);
    y = YLPTODP(dc, (top + bottom)/2);

    w = XLSTODS(dc, (right - left));
    h = YLSTODS(dc, (bottom - top));

    if(w < 0) w = -w;
    if(h < 0) h = -h;
    ratio = ((double)w)/h;

    /* angle is the angle after the rectangle is transformed to a square and is
       measured anticlockwise from the +ve x-axis */

    start_angle = atan2((double)(y - ystart) * ratio, (double)(xstart - x));
    end_angle = atan2((double)(y - yend) * ratio, (double)(xend - x));

    start_angle *= 180.0 / PI;
    end_angle *= 180.0 / PI;

    if(lines == 2) /* pie */
        PSDRV_WriteMoveTo(physDev, x, y);
    else
        PSDRV_WriteNewPath( physDev );

    PSDRV_WriteArc(physDev, x, y, w, h, start_angle, end_angle);
    if(lines == 1 || lines == 2) { /* chord or pie */
        PSDRV_WriteClosePath(physDev);
	PSDRV_Brush(physDev,0);
    }
    PSDRV_SetPen(physDev);
    PSDRV_DrawLine(physDev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Arc
 */
BOOL PSDRV_Arc( PSDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
                INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( physDev, left, top, right, bottom, xstart, ystart, xend, yend, 0 );
}

/***********************************************************************
 *           PSDRV_Chord
 */
BOOL PSDRV_Chord( PSDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
                  INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( physDev, left, top, right, bottom, xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           PSDRV_Pie
 */
BOOL PSDRV_Pie( PSDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
                INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( physDev, left, top, right, bottom, xstart, ystart, xend, yend, 2 );
}


/***********************************************************************
 *           PSDRV_Ellipse
 */
BOOL PSDRV_Ellipse( PSDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
    INT x, y, w, h;
    DC *dc = physDev->dc;

    TRACE("%d %d - %d %d\n", left, top, right, bottom);

    x = XLPTODP(dc, (left + right)/2);
    y = YLPTODP(dc, (top + bottom)/2);

    w = XLSTODS(dc, (right - left));
    h = YLSTODS(dc, (bottom - top));

    PSDRV_WriteNewPath(physDev);
    PSDRV_WriteArc(physDev, x, y, w, h, 0.0, 360.0);
    PSDRV_WriteClosePath(physDev);
    PSDRV_Brush(physDev,0);
    PSDRV_SetPen(physDev);
    PSDRV_DrawLine(physDev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_PolyPolyline
 */
BOOL PSDRV_PolyPolyline( PSDRV_PDEVICE *physDev, const POINT* pts, const DWORD* counts,
			   DWORD polylines )
{
    DWORD polyline, line;
    const POINT* pt;
    DC *dc = physDev->dc;

    TRACE("\n");

    pt = pts;
    for(polyline = 0; polyline < polylines; polyline++) {
	PSDRV_WriteMoveTo(physDev, INTERNAL_XWPTODP(dc, pt->x, pt->y), INTERNAL_YWPTODP(dc, pt->x, pt->y));
	pt++;
	for(line = 1; line < counts[polyline]; line++) {
	    PSDRV_WriteLineTo(physDev, INTERNAL_XWPTODP(dc, pt->x, pt->y), INTERNAL_YWPTODP(dc, pt->x, pt->y));
	    pt++;
	}
    }
    PSDRV_SetPen(physDev);
    PSDRV_DrawLine(physDev);
    return TRUE;
}   


/***********************************************************************
 *           PSDRV_Polyline
 */
BOOL PSDRV_Polyline( PSDRV_PDEVICE *physDev, const POINT* pt, INT count )
{
    return PSDRV_PolyPolyline( physDev, pt, (LPDWORD) &count, 1 );
}


/***********************************************************************
 *           PSDRV_PolyPolygon
 */
BOOL PSDRV_PolyPolygon( PSDRV_PDEVICE *physDev, const POINT* pts, const INT* counts,
			  UINT polygons )
{
    DWORD polygon, line;
    const POINT* pt;
    DC *dc = physDev->dc;

    TRACE("\n");

    pt = pts;
    for(polygon = 0; polygon < polygons; polygon++) {
	PSDRV_WriteMoveTo(physDev, INTERNAL_XWPTODP(dc, pt->x, pt->y), INTERNAL_YWPTODP(dc, pt->x, pt->y));
	pt++;
	for(line = 1; line < counts[polygon]; line++) {
	    PSDRV_WriteLineTo(physDev, INTERNAL_XWPTODP(dc, pt->x, pt->y), INTERNAL_YWPTODP(dc, pt->x, pt->y));
	    pt++;
	}
	PSDRV_WriteClosePath(physDev);
    }

    if(GetPolyFillMode( physDev->hdc ) == ALTERNATE)
        PSDRV_Brush(physDev, 1);
    else /* WINDING */
        PSDRV_Brush(physDev, 0);
    PSDRV_SetPen(physDev);
    PSDRV_DrawLine(physDev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Polygon
 */
BOOL PSDRV_Polygon( PSDRV_PDEVICE *physDev, const POINT* pt, INT count )
{
     return PSDRV_PolyPolygon( physDev, pt, &count, 1 );
}


/***********************************************************************
 *           PSDRV_SetPixel
 */
COLORREF PSDRV_SetPixel( PSDRV_PDEVICE *physDev, INT x, INT y, COLORREF color )
{
    PSCOLOR pscolor;
    DC *dc = physDev->dc;

    x = INTERNAL_XWPTODP(dc, x, y);
    y = INTERNAL_YWPTODP(dc, x, y);

    PSDRV_WriteRectangle( physDev, x, y, 0, 0 );
    PSDRV_CreateColor( physDev, &pscolor, color );
    PSDRV_WriteSetColor( physDev, &pscolor );
    PSDRV_WriteFill( physDev );
    return color;
}


/***********************************************************************
 *           PSDRV_DrawLine
 */
VOID PSDRV_DrawLine( PSDRV_PDEVICE *physDev )
{
    if (physDev->pen.style == PS_NULL)
	PSDRV_WriteNewPath(physDev);
    else
	PSDRV_WriteStroke(physDev);
}

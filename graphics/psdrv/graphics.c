/*
 *	PostScript driver graphics functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include <math.h>
#include "config.h"
#if defined(HAVE_FLOAT_H)
 #include <float.h>
#endif
#if !defined(PI)
 #define PI M_PI
#endif
#include "psdrv.h"
#include "debugtools.h"
#include "winspool.h"

DEFAULT_DEBUG_CHANNEL(psdrv)

/**********************************************************************
 *	     PSDRV_MoveToEx
 */
BOOL PSDRV_MoveToEx(DC *dc, INT x, INT y, LPPOINT pt)
{
    TRACE("%d %d\n", x, y);
    if (pt)
    {
	pt->x = dc->w.CursPosX;
	pt->y = dc->w.CursPosY;
    }
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;

    return TRUE;
}


/***********************************************************************
 *           PSDRV_LineTo
 */
BOOL PSDRV_LineTo(DC *dc, INT x, INT y)
{
    TRACE("%d %d\n", x, y);

    PSDRV_SetPen(dc);
    PSDRV_WriteMoveTo(dc, XLPTODP(dc, dc->w.CursPosX),
		          YLPTODP(dc, dc->w.CursPosY));
    PSDRV_WriteLineTo(dc, XLPTODP(dc, x), YLPTODP(dc, y));
    PSDRV_DrawLine(dc);

    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Rectangle
 */
BOOL PSDRV_Rectangle( DC *dc, INT left, INT top, INT right,
		       INT bottom )
{
    INT width = XLSTODS(dc, right - left);
    INT height = YLSTODS(dc, bottom - top);


    TRACE("%d %d - %d %d\n", left, top, right, bottom);

    PSDRV_WriteRectangle(dc, XLPTODP(dc, left), YLPTODP(dc, top),
			     width, height);

    PSDRV_Brush(dc,0);
    PSDRV_SetPen(dc);
    PSDRV_DrawLine(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_RoundRect
 */
BOOL PSDRV_RoundRect( DC *dc, INT left, INT top, INT right,
			INT bottom, INT ell_width, INT ell_height )
{
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

    PSDRV_WriteMoveTo( dc, left, top + ell_height/2 );
    PSDRV_WriteArc( dc, left + ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 90.0, 180.0);
    PSDRV_WriteLineTo( dc, right - ell_width/2, top );
    PSDRV_WriteArc( dc, right - ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 0.0, 90.0);
    PSDRV_WriteLineTo( dc, right, bottom - ell_height/2 );
    PSDRV_WriteArc( dc, right - ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, -90.0, 0.0);
    PSDRV_WriteLineTo( dc, right - ell_width/2, bottom);
    PSDRV_WriteArc( dc, left + ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, 180.0, -90.0);
    PSDRV_WriteClosePath( dc );

    PSDRV_Brush(dc,0);
    PSDRV_SetPen(dc);
    PSDRV_DrawLine(dc);
    return TRUE;
}

/***********************************************************************
 *           PSDRV_DrawArc
 *
 * Does the work of Arc, Chord and Pie. lines is 0, 1 or 2 respectively.
 */
static BOOL PSDRV_DrawArc( DC *dc, INT left, INT top, 
			     INT right, INT bottom,
			     INT xstart, INT ystart,
			     INT xend, INT yend,
			     int lines )
{
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
        PSDRV_WriteMoveTo(dc, x, y);
    else
        PSDRV_WriteNewPath( dc );

    PSDRV_WriteArc(dc, x, y, w, h, start_angle, end_angle);
    if(lines == 1 || lines == 2) { /* chord or pie */
        PSDRV_WriteClosePath(dc);
	PSDRV_Brush(dc,0);
    }
    PSDRV_SetPen(dc);
    PSDRV_DrawLine(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Arc
 */
BOOL PSDRV_Arc( DC *dc, INT left, INT top, INT right, INT bottom,
		  INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( dc, left, top, right, bottom, xstart, ystart,
			 xend, yend, 0 );
}

/***********************************************************************
 *           PSDRV_Chord
 */
BOOL PSDRV_Chord( DC *dc, INT left, INT top, INT right, INT bottom,
		  INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( dc, left, top, right, bottom, xstart, ystart,
			 xend, yend, 1 );
}


/***********************************************************************
 *           PSDRV_Pie
 */
BOOL PSDRV_Pie( DC *dc, INT left, INT top, INT right, INT bottom,
		  INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( dc, left, top, right, bottom, xstart, ystart,
			 xend, yend, 2 );
}


/***********************************************************************
 *           PSDRV_Ellipse
 */
BOOL PSDRV_Ellipse( DC *dc, INT left, INT top, INT right, INT bottom)
{
    INT x, y, w, h;

    TRACE("%d %d - %d %d\n", left, top, right, bottom);

    x = XLPTODP(dc, (left + right)/2);
    y = YLPTODP(dc, (top + bottom)/2);

    w = XLSTODS(dc, (right - left));
    h = YLSTODS(dc, (bottom - top));

    PSDRV_WriteNewPath(dc);
    PSDRV_WriteArc(dc, x, y, w, h, 0.0, 360.0);
    PSDRV_WriteClosePath(dc);
    PSDRV_Brush(dc,0);
    PSDRV_SetPen(dc);
    PSDRV_DrawLine(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_PolyPolyline
 */
BOOL PSDRV_PolyPolyline( DC *dc, const POINT* pts, const DWORD* counts,
			   DWORD polylines )
{
    DWORD polyline, line;
    const POINT* pt;
    TRACE("\n");

    pt = pts;
    for(polyline = 0; polyline < polylines; polyline++) {
        PSDRV_WriteMoveTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	pt++;
	for(line = 1; line < counts[polyline]; line++) {
	    PSDRV_WriteLineTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	    pt++;
	}
    }
    PSDRV_SetPen(dc);
    PSDRV_DrawLine(dc);
    return TRUE;
}   


/***********************************************************************
 *           PSDRV_Polyline
 */
BOOL PSDRV_Polyline( DC *dc, const POINT* pt, INT count )
{
    return PSDRV_PolyPolyline( dc, pt, (LPDWORD) &count, 1 );
}


/***********************************************************************
 *           PSDRV_PolyPolygon
 */
BOOL PSDRV_PolyPolygon( DC *dc, const POINT* pts, const INT* counts,
			  UINT polygons )
{
    DWORD polygon, line;
    const POINT* pt;
    TRACE("\n");

    pt = pts;
    for(polygon = 0; polygon < polygons; polygon++) {
        PSDRV_WriteMoveTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	pt++;
	for(line = 1; line < counts[polygon]; line++) {
	    PSDRV_WriteLineTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	    pt++;
	}
	PSDRV_WriteClosePath(dc);
    }

    if(dc->w.polyFillMode == ALTERNATE)
        PSDRV_Brush(dc, 1);
    else /* WINDING */
        PSDRV_Brush(dc, 0);
    PSDRV_SetPen(dc);
    PSDRV_DrawLine(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Polygon
 */
BOOL PSDRV_Polygon( DC *dc, const POINT* pt, INT count )
{
     return PSDRV_PolyPolygon( dc, pt, &count, 1 );
}


/***********************************************************************
 *           PSDRV_SetPixel
 */
COLORREF PSDRV_SetPixel( DC *dc, INT x, INT y, COLORREF color )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    PSCOLOR pscolor;

    x = XLPTODP(dc, x);
    y = YLPTODP(dc, y);

    PSDRV_WriteRectangle( dc, x, y, 0, 0 );
    PSDRV_CreateColor( physDev, &pscolor, color );
    PSDRV_WriteSetColor( dc, &pscolor );
    PSDRV_WriteFill( dc );
    return color;
}


/***********************************************************************
 *           PSDRV_DrawLine
 */
VOID PSDRV_DrawLine( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if (physDev->pen.style == PS_NULL)
	PSDRV_WriteNewPath(dc);
    else
	PSDRV_WriteStroke(dc);
}

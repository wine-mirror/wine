/*
 * X11 graphics driver graphics functions
 *
 * Copyright 1993,1994 Alexandre Julliard
 */

#include <math.h>
#if defined(__EMX__)
#include <float.h>
#endif
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#ifndef PI
#define PI M_PI
#endif
#include <string.h>

#include "x11drv.h"
#include "bitmap.h"
#include "gdi.h"
#include "graphics.h"
#include "dc.h"
#include "bitmap.h"
#include "callback.h"
#include "metafile.h"
#include "syscolor.h"
#include "stddebug.h"
#include "palette.h"
#include "color.h"
#include "region.h"
#include "struct32.h"
#include "debug.h"
#include "xmalloc.h"


/**********************************************************************
 *	     X11DRV_MoveToEx
 */
BOOL32
X11DRV_MoveToEx(DC *dc,INT32 x,INT32 y,LPPOINT32 pt) {
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
 *           X11DRV_LineTo
 */
BOOL32
X11DRV_LineTo( DC *dc, INT32 x, INT32 y )
{
    if (DC_SetupGCForPen( dc ))
	XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, 
		  dc->w.DCOrgX + XLPTODP( dc, dc->w.CursPosX ),
		  dc->w.DCOrgY + YLPTODP( dc, dc->w.CursPosY ),
		  dc->w.DCOrgX + XLPTODP( dc, x ),
		  dc->w.DCOrgY + YLPTODP( dc, y ) );
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}



/***********************************************************************
 *           GRAPH_DrawArc
 *
 * Helper functions for Arc(), Chord() and Pie().
 * 'lines' is the number of lines to draw: 0 for Arc, 1 for Chord, 2 for Pie.
 */
static BOOL32
X11DRV_DrawArc( DC *dc, INT32 left, INT32 top, INT32 right,
                INT32 bottom, INT32 xstart, INT32 ystart,
                INT32 xend, INT32 yend, INT32 lines )
{
    INT32 xcenter, ycenter, istart_angle, idiff_angle, tmp;
    double start_angle, end_angle;
    XPoint points[3];

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    xstart = XLPTODP( dc, xstart );
    ystart = YLPTODP( dc, ystart );
    xend   = XLPTODP( dc, xend );
    yend   = YLPTODP( dc, yend );
    if ((left == right) || (top == bottom)) return FALSE;

    xcenter = (right + left) / 2;
    ycenter = (bottom + top) / 2;
    start_angle = atan2( (double)(ycenter-ystart)*(right-left),
			 (double)(xstart-xcenter)*(bottom-top) );
    end_angle   = atan2( (double)(ycenter-yend)*(right-left),
			 (double)(xend-xcenter)*(bottom-top) );
    istart_angle = (INT32)(start_angle * 180 * 64 / PI);
    idiff_angle  = (INT32)((end_angle - start_angle) * 180 * 64 / PI );
    if (idiff_angle <= 0) idiff_angle += 360 * 64;
    if (left > right) { tmp=left; left=right; right=tmp; }
    if (top > bottom) { tmp=top; top=bottom; bottom=tmp; }

      /* Fill arc with brush if Chord() or Pie() */

    if ((lines > 0) && DC_SetupGCForBrush( dc ))
    {
        XSetArcMode( display, dc->u.x.gc, (lines==1) ? ArcChord : ArcPieSlice);
        XFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                 dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                 right-left-1, bottom-top-1, istart_angle, idiff_angle );
    }

      /* Draw arc and lines */

    if (!DC_SetupGCForPen( dc )) return TRUE;
    XDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
	      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
	      right-left-1, bottom-top-1, istart_angle, idiff_angle );
    if (!lines) return TRUE;

    points[0].x = dc->w.DCOrgX + xcenter + (int)(cos(start_angle) * (right-left) / 2);
    points[0].y = dc->w.DCOrgY + ycenter - (int)(sin(start_angle) * (bottom-top) / 2);
    points[1].x = dc->w.DCOrgX + xcenter + (int)(cos(end_angle) * (right-left) / 2);
    points[1].y = dc->w.DCOrgY + ycenter - (int)(sin(end_angle) * (bottom-top) / 2);
    if (lines == 2)
    {
	points[2] = points[1];
	points[1].x = dc->w.DCOrgX + xcenter;
	points[1].y = dc->w.DCOrgY + ycenter;
    }
    XDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
	        points, lines+1, CoordModeOrigin );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Arc
 */
BOOL32
X11DRV_Arc( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
            INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return X11DRV_DrawArc( dc, left, top, right, bottom,
			   xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           X11DRV_Pie
 */
BOOL32
X11DRV_Pie( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
            INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return X11DRV_DrawArc( dc, left, top, right, bottom,
			   xstart, ystart, xend, yend, 2 );
}

/***********************************************************************
 *           X11DRV_Chord
 */
BOOL32
X11DRV_Chord( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
              INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return X11DRV_DrawArc( dc, left, top, right, bottom,
		  	   xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           X11DRV_Ellipse
 */
BOOL32
X11DRV_Ellipse( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom )
{
    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    if ((left == right) || (top == bottom)) return FALSE;

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }
    
    if ((dc->u.x.pen.style == PS_INSIDEFRAME) &&
        (dc->u.x.pen.width < right-left-1) &&
        (dc->u.x.pen.width < bottom-top-1))
    {
        left   += dc->u.x.pen.width / 2;
        right  -= (dc->u.x.pen.width + 1) / 2;
        top    += dc->u.x.pen.width / 2;
        bottom -= (dc->u.x.pen.width + 1) / 2;
    }

    if (DC_SetupGCForBrush( dc ))
	XFillArc( display, dc->u.x.drawable, dc->u.x.gc,
		  dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		  right-left-1, bottom-top-1, 0, 360*64 );
    if (DC_SetupGCForPen( dc ))
	XDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
		  dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		  right-left-1, bottom-top-1, 0, 360*64 );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Rectangle
 */
BOOL32
X11DRV_Rectangle(DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    INT32 width;
    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }

    if ((left == right) || (top == bottom))
    {
	if (DC_SetupGCForPen( dc ))
	    XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, 
		  dc->w.DCOrgX + left,
		  dc->w.DCOrgY + top,
		  dc->w.DCOrgX + right,
		  dc->w.DCOrgY + bottom);
	return TRUE;
    }
    width = dc->u.x.pen.width;
    if (!width) width = 1;
    if(dc->u.x.pen.style == PS_NULL) width = 0;

    if ((dc->u.x.pen.style == PS_INSIDEFRAME) &&
        (width < right-left) && (width < bottom-top))
    {
        left   += width / 2;
        right  -= (width + 1) / 2;
        top    += width / 2;
        bottom -= (width + 1) / 2;
    }

    if (DC_SetupGCForBrush( dc ))
	XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left + (width + 1) / 2,
		        dc->w.DCOrgY + top + (width + 1) / 2,
		        right-left-width-1, bottom-top-width-1);
    if (DC_SetupGCForPen( dc ))
	XDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );
    return TRUE;
}

/***********************************************************************
 *           X11DRV_RoundRect
 */
BOOL32
X11DRV_RoundRect( DC *dc, INT32 left, INT32 top, INT32 right,
                  INT32 bottom, INT32 ell_width, INT32 ell_height )
{
    dprintf_graphics(stddeb, "X11DRV_RoundRect(%d %d %d %d  %d %d\n", 
    	left, top, right, bottom, ell_width, ell_height);

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    ell_width  = abs( ell_width * dc->vportExtX / dc->wndExtX );
    ell_height = abs( ell_height * dc->vportExtY / dc->wndExtY );

    /* Fix the coordinates */

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }
    if (ell_width > right - left) ell_width = right - left;
    if (ell_height > bottom - top) ell_height = bottom - top;

    if (DC_SetupGCForBrush( dc ))
    {
        if (ell_width && ell_height)
        {
            XFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width, ell_height, 90 * 64, 90 * 64 );
            XFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + bottom - ell_height,
                      ell_width, ell_height, 180 * 64, 90 * 64 );
            XFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width,
                      dc->w.DCOrgY + bottom - ell_height,
                      ell_width, ell_height, 270 * 64, 90 * 64 );
            XFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width, dc->w.DCOrgY + top,
                      ell_width, ell_height, 0, 90 * 64 );
        }
        if (ell_width < right - left)
        {
            XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left + ell_width / 2,
                            dc->w.DCOrgY + top,
                            right - left - ell_width, ell_height / 2 );
            XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left + ell_width / 2,
                            dc->w.DCOrgY + bottom - (ell_height+1) / 2,
                            right - left - ell_width, (ell_height+1) / 2 );
        }
        if  (ell_height < bottom - top)
        {
            XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left,
                            dc->w.DCOrgY + top + ell_height / 2,
                            right - left, bottom - top - ell_height );
        }
    }
    if (DC_SetupGCForPen(dc))
    {
        if (ell_width && ell_height)
        {
            XDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width, ell_height, 90 * 64, 90 * 64 );
            XDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + bottom - ell_height,
                      ell_width, ell_height, 180 * 64, 90 * 64 );
            XDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width,
                      dc->w.DCOrgY + bottom - ell_height,
                      ell_width, ell_height, 270 * 64, 90 * 64 );
            XDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width, dc->w.DCOrgY + top,
                      ell_width, ell_height, 0, 90 * 64 );
	}
        if (ell_width < right - left)
        {
            XDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
                       dc->w.DCOrgX + left + ell_width / 2,
                       dc->w.DCOrgY + top,
                       dc->w.DCOrgX + right - ell_width / 2,
                       dc->w.DCOrgY + top );
            XDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
                       dc->w.DCOrgX + left + ell_width / 2,
                       dc->w.DCOrgY + bottom,
                       dc->w.DCOrgX + right - ell_width / 2,
                       dc->w.DCOrgY + bottom );
        }
        if (ell_height < bottom - top)
        {
            XDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
                       dc->w.DCOrgX + right,
                       dc->w.DCOrgY + top + ell_height / 2,
                       dc->w.DCOrgX + right,
                       dc->w.DCOrgY + bottom - ell_height / 2 );
            XDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
                       dc->w.DCOrgX + left,
                       dc->w.DCOrgY + top + ell_height / 2,
                       dc->w.DCOrgX + left,
                       dc->w.DCOrgY + bottom - ell_height / 2 );
        }
    }
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetPixel
 */
COLORREF
X11DRV_SetPixel( DC *dc, INT32 x, INT32 y, COLORREF color )
{
    Pixel pixel;
    
    x = dc->w.DCOrgX + XLPTODP( dc, x );
    y = dc->w.DCOrgY + YLPTODP( dc, y );
    pixel = COLOR_ToPhysical( dc, color );
    
    XSetForeground( display, dc->u.x.gc, pixel );
    XSetFunction( display, dc->u.x.gc, GXcopy );
    XDrawPoint( display, dc->u.x.drawable, dc->u.x.gc, x, y );

    /* inefficient but simple... */

    return COLOR_ToLogical(pixel);
}


/***********************************************************************
 *           X11DRV_GetPixel
 */
COLORREF
X11DRV_GetPixel( DC *dc, INT32 x, INT32 y )
{
    static Pixmap pixmap = 0;
    XImage * image;
    int pixel;

    x = dc->w.DCOrgX + XLPTODP( dc, x );
    y = dc->w.DCOrgY + YLPTODP( dc, y );
    if (dc->w.flags & DC_MEMORY)
    {
        image = XGetImage( display, dc->u.x.drawable, x, y, 1, 1,
                           AllPlanes, ZPixmap );
    }
    else
    {
        /* If we are reading from the screen, use a temporary copy */
        /* to avoid a BadMatch error */
        if (!pixmap) pixmap = XCreatePixmap( display, rootWindow,
                                             1, 1, dc->w.bitsPerPixel );
        XCopyArea( display, dc->u.x.drawable, pixmap, BITMAP_colorGC,
                   x, y, 1, 1, 0, 0 );
        image = XGetImage( display, pixmap, 0, 0, 1, 1, AllPlanes, ZPixmap );
    }
    pixel = XGetPixel( image, 0, 0 );
    XDestroyImage( image );
    
    return COLOR_ToLogical(pixel);
}


/***********************************************************************
 *           X11DRV_PaintRgn
 */
BOOL32
X11DRV_PaintRgn( DC *dc, HRGN32 hrgn )
{
    RECT32 box;
    HRGN32 tmpVisRgn, prevVisRgn;
    HDC32  hdc = dc->hSelf; /* FIXME: should not mix dc/hdc this way */

      /* Modify visible region */

    if (!(prevVisRgn = SaveVisRgn( hdc ))) return FALSE;
    if (!(tmpVisRgn = CreateRectRgn32( 0, 0, 0, 0 )))
    {
        RestoreVisRgn( hdc );
        return FALSE;
    }
    CombineRgn32( tmpVisRgn, prevVisRgn, hrgn, RGN_AND );
    SelectVisRgn( hdc, tmpVisRgn );
    DeleteObject32( tmpVisRgn );

      /* Fill the region */

    GetRgnBox32( dc->w.hGCClipRgn, &box );
    if (DC_SetupGCForBrush( dc ))
	XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + box.left, dc->w.DCOrgY + box.top,
		        box.right-box.left, box.bottom-box.top );

      /* Restore the visible region */

    RestoreVisRgn( hdc );
    return TRUE;
}

/**********************************************************************
 *          X11DRV_Polyline
 */
BOOL32
X11DRV_Polyline( DC *dc, const LPPOINT32 pt, INT32 count )
{
    register int i;

    if (DC_SetupGCForPen( dc ))
	for (i = 0; i < count-1; i ++)
	    XDrawLine (display, dc->u.x.drawable, dc->u.x.gc,  
		       dc->w.DCOrgX + XLPTODP(dc, pt [i].x),
		       dc->w.DCOrgY + YLPTODP(dc, pt [i].y),
		       dc->w.DCOrgX + XLPTODP(dc, pt [i+1].x),
		       dc->w.DCOrgY + YLPTODP(dc, pt [i+1].y));
    return TRUE;
}


/**********************************************************************
 *          X11DRV_Polygon
 */
BOOL32
X11DRV_Polygon( DC *dc, LPPOINT32 pt, INT32 count )
{
    register int i;
    XPoint *points;

    points = (XPoint *) xmalloc (sizeof (XPoint) * (count+1));
    for (i = 0; i < count; i++)
    {
	points[i].x = dc->w.DCOrgX + XLPTODP( dc, pt[i].x );
	points[i].y = dc->w.DCOrgY + YLPTODP( dc, pt[i].y );
    }
    points[count] = points[0];

    if (DC_SetupGCForBrush( dc ))
	XFillPolygon( display, dc->u.x.drawable, dc->u.x.gc,
		     points, count+1, Complex, CoordModeOrigin);

    if (DC_SetupGCForPen ( dc ))
	XDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
		   points, count+1, CoordModeOrigin );

    free( points );
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolygon
 */
BOOL32 
X11DRV_PolyPolygon( DC *dc, LPPOINT32 pt, LPINT32 counts, UINT32 polygons)
{
    HRGN32 hrgn;

      /* FIXME: The points should be converted to device coords before */
      /* creating the region. But as CreatePolyPolygonRgn is not */
      /* really correct either, it doesn't matter much... */
      /* At least the outline will be correct :-) */
    hrgn = CreatePolyPolygonRgn32( pt, counts, polygons, dc->w.polyFillMode );
    X11DRV_PaintRgn( dc, hrgn );
    DeleteObject32( hrgn );

      /* Draw the outline of the polygons */

    if (DC_SetupGCForPen ( dc ))
    {
	int i, j, max = 0;
	XPoint *points;

	for (i = 0; i < polygons; i++) if (counts[i] > max) max = counts[i];
	points = (XPoint *) xmalloc( sizeof(XPoint) * (max+1) );

	for (i = 0; i < polygons; i++)
	{
	    for (j = 0; j < counts[i]; j++)
	    {
		points[j].x = dc->w.DCOrgX + XLPTODP( dc, pt->x );
		points[j].y = dc->w.DCOrgY + YLPTODP( dc, pt->y );
		pt++;
	    }
	    points[j] = points[0];
	    XDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
		        points, j + 1, CoordModeOrigin );
	}
	free( points );
    }
    return TRUE;
}


/**********************************************************************
 *          X11DRV_InternalFloodFill
 *
 * Internal helper function for flood fill.
 * (xorg,yorg) is the origin of the X image relative to the drawable.
 * (x,y) is relative to the origin of the X image.
 */
static void X11DRV_InternalFloodFill(XImage *image, DC *dc,
                                     int x, int y,
                                     int xOrg, int yOrg,
                                     Pixel pixel, WORD fillType )
{
    int left, right;

#define TO_FLOOD(x,y)  ((fillType == FLOODFILLBORDER) ? \
                        (XGetPixel(image,x,y) != pixel) : \
                        (XGetPixel(image,x,y) == pixel))

    if (!TO_FLOOD(x,y)) return;

      /* Find left and right boundaries */

    left = right = x;
    while ((left > 0) && TO_FLOOD( left-1, y )) left--;
    while ((right < image->width) && TO_FLOOD( right, y )) right++;
    XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                    xOrg + left, yOrg + y, right-left, 1 );

      /* Set the pixels of this line so we don't fill it again */

    for (x = left; x < right; x++)
    {
        if (fillType == FLOODFILLBORDER) XPutPixel( image, x, y, pixel );
        else XPutPixel( image, x, y, ~pixel );
    }

      /* Fill the line above */

    if (--y >= 0)
    {
        x = left;
        while (x < right)
        {
            while ((x < right) && !TO_FLOOD(x,y)) x++;
            if (x >= right) break;
            while ((x < right) && TO_FLOOD(x,y)) x++;
            X11DRV_InternalFloodFill(image, dc, x-1, y,
                                     xOrg, yOrg, pixel, fillType );
        }
    }

      /* Fill the line below */

    if ((y += 2) < image->height)
    {
        x = left;
        while (x < right)
        {
            while ((x < right) && !TO_FLOOD(x,y)) x++;
            if (x >= right) break;
            while ((x < right) && TO_FLOOD(x,y)) x++;
            X11DRV_InternalFloodFill(image, dc, x-1, y,
                                     xOrg, yOrg, pixel, fillType );
        }
    }
#undef TO_FLOOD    
}


/**********************************************************************
 *          X11DRV_DoFloodFill
 *
 * Main flood-fill routine.
 */

struct FloodFill_params
{
    DC      *dc;
    INT32    x;
    INT32    y;
    COLORREF color;
    UINT32   fillType;
};

static BOOL32 X11DRV_DoFloodFill( const struct FloodFill_params *params )
{
    XImage *image;
    RECT32 rect;
    DC *dc = params->dc;

    if (GetRgnBox32( dc->w.hGCClipRgn, &rect ) == ERROR) return FALSE;

    if (!(image = XGetImage( display, dc->u.x.drawable,
                             dc->w.DCOrgX + rect.left,
                             dc->w.DCOrgY + rect.top,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             AllPlanes, ZPixmap ))) return FALSE;

    if (DC_SetupGCForBrush( dc ))
    {
          /* ROP mode is always GXcopy for flood-fill */
        XSetFunction( display, dc->u.x.gc, GXcopy );
        X11DRV_InternalFloodFill(image, dc,
                                 XLPTODP(dc,params->x) - rect.left,
                                 YLPTODP(dc,params->y) - rect.top,
                                 dc->w.DCOrgX + rect.left,
                                 dc->w.DCOrgY + rect.top,
                                 COLOR_ToPhysical( dc, params->color ),
                                 params->fillType );
    }

    XDestroyImage( image );
    return TRUE;
}


/**********************************************************************
 *          X11DRV_ExtFloodFill
 */
BOOL32
X11DRV_ExtFloodFill( DC *dc, INT32 x, INT32 y, COLORREF color,
                     UINT32 fillType )
{
    struct FloodFill_params params = { dc, x, y, color, fillType };

    dprintf_graphics( stddeb, "X11DRV_ExtFloodFill %d,%d %06lx %d\n",
                      x, y, color, fillType );

    if (!PtVisible32( dc->hSelf, x, y )) return FALSE;
    return CALL_LARGE_STACK( X11DRV_DoFloodFill, &params );
}

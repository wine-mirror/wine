/*
 * X11 graphics driver graphics functions
 *
 * Copyright 1993,1994 Alexandre Julliard
 */


/*
 * FIXME: none of these functions obey the GM_ADVANCED
 * graphics mode
 */

#include <math.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include <stdlib.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
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
#include "palette.h"
#include "color.h"
#include "region.h"
#include "struct32.h"
#include "debug.h"
#include "xmalloc.h"

#define ABS(x)    ((x)<0?(-(x)):(x))
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
	TSXDrawLine(display, dc->u.x.drawable, dc->u.x.gc, 
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
 *
 */
static BOOL32
X11DRV_DrawArc( DC *dc, INT32 left, INT32 top, INT32 right,
                INT32 bottom, INT32 xstart, INT32 ystart,
                INT32 xend, INT32 yend, INT32 lines )
{
    INT32 xcenter, ycenter, istart_angle, idiff_angle;
    INT32 width, oldwidth, oldendcap;
    double start_angle, end_angle;
    XPoint points[4];

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    xstart = XLPTODP( dc, xstart );
    ystart = YLPTODP( dc, ystart );
    xend   = XLPTODP( dc, xend );
    yend   = YLPTODP( dc, yend );

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }
    if ((left == right) || (top == bottom)
            ||(lines && ((right-left==1)||(bottom-top==1)))) return TRUE;

    oldwidth = width = dc->u.x.pen.width;
    oldendcap= dc->u.x.pen.endcap;
    if (!width) width = 1;
    if(dc->u.x.pen.style == PS_NULL) width = 0;

    if ((dc->u.x.pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 0) width=1; /* more accurate */
    dc->u.x.pen.width=width;
    dc->u.x.pen.endcap=PS_ENDCAP_SQUARE;

    xcenter = (right + left) / 2;
    ycenter = (bottom + top) / 2;
    start_angle = atan2( (double)(ycenter-ystart)*(right-left),
			 (double)(xstart-xcenter)*(bottom-top) );
    end_angle   = atan2( (double)(ycenter-yend)*(right-left),
			 (double)(xend-xcenter)*(bottom-top) );
    if ((xstart==xend)&&(ystart==yend))
      { /* A lazy program delivers xstart=xend=ystart=yend=0) */
	start_angle = 0;
	end_angle = 2* PI;
      }
    else /* notorious cases */
      if ((start_angle == PI)&&( end_angle <0))
	start_angle = - PI;
    else
      if ((end_angle == PI)&&( start_angle <0))
	end_angle = - PI;
    istart_angle = (INT32)(start_angle * 180 * 64 / PI + 0.5);
    idiff_angle  = (INT32)((end_angle - start_angle) * 180 * 64 / PI + 0.5);
    if (idiff_angle <= 0) idiff_angle += 360 * 64;

      /* Fill arc with brush if Chord() or Pie() */

    if ((lines > 0) && DC_SetupGCForBrush( dc )) {
        TSXSetArcMode( display, dc->u.x.gc, (lines==1) ? ArcChord : ArcPieSlice);
        TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                 dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                 right-left-1, bottom-top-1, istart_angle, idiff_angle );
    }

      /* Draw arc and lines */

    if (DC_SetupGCForPen( dc )){
    TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
	      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
	      right-left-1, bottom-top-1, istart_angle, idiff_angle );
        if (lines) {
            /* use the truncated values */
            start_angle=(double)istart_angle*PI/64./180.;
            end_angle=(double)(istart_angle+idiff_angle)*PI/64./180.;
            /* calculate the endpoints and round correctly */
            points[0].x = (int) floor(dc->w.DCOrgX + (right+left)/2.0 +
                    cos(start_angle) * (right-left-width*2+2) / 2. + 0.5);
            points[0].y = (int) floor(dc->w.DCOrgY + (top+bottom)/2.0 -
                    sin(start_angle) * (bottom-top-width*2+2) / 2. + 0.5);
            points[1].x = (int) floor(dc->w.DCOrgX + (right+left)/2.0 +
                    cos(end_angle) * (right-left-width*2+2) / 2. + 0.5);
            points[1].y = (int) floor(dc->w.DCOrgY + (top+bottom)/2.0 -
                    sin(end_angle) * (bottom-top-width*2+2) / 2. + 0.5);
                    
            /* OK this stuff is optimized for Xfree86 
             * which is probably the most used server by
             * wine users. Other X servers will not 
             * display correctly. (eXceed for instance)
             * so if you feel you must change make sure that
             * you either use Xfree86 or seperate your changes 
             * from these (compile switch or whatever)
             */
            if (lines == 2) {
                INT32 dx1,dy1;
                points[3] = points[1];
	points[1].x = dc->w.DCOrgX + xcenter;
	points[1].y = dc->w.DCOrgY + ycenter;
                points[2] = points[1];
                dx1=points[1].x-points[0].x;
                dy1=points[1].y-points[0].y;
                if(((top-bottom) | -2) == -2)
                    if(dy1>0) points[1].y--;
                if(dx1<0) {
                    if (((-dx1)*64)<=ABS(dy1)*37) points[0].x--;
                    if(((-dx1*9))<(dy1*16)) points[0].y--;
                    if( dy1<0 && ((dx1*9)) < (dy1*16)) points[0].y--;
                } else {
                    if(dy1 < 0)  points[0].y--;
                    if(((right-left) | -2) == -2) points[1].x--;
                }
                dx1=points[3].x-points[2].x;
                dy1=points[3].y-points[2].y;
                if(((top-bottom) | -2 ) == -2)
                    if(dy1 < 0) points[2].y--;
                if( dx1<0){ 
                    if( dy1>0) points[3].y--;
                    if(((right-left) | -2) == -2 ) points[2].x--;
                }else {
                    points[3].y--;
                    if( dx1 * 64 < dy1 * -37 ) points[3].x--;
                }
                lines++;
    }
    TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
	        points, lines+1, CoordModeOrigin );
        }
    }
    dc->u.x.pen.width=oldwidth;
    dc->u.x.pen.endcap=oldendcap;
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
    INT32 width, oldwidth;
    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    if ((left == right) || (top == bottom)) return TRUE;

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }
    
    oldwidth = width = dc->u.x.pen.width;
    if (!width) width = 1;
    if(dc->u.x.pen.style == PS_NULL) width = 0;

    if ((dc->u.x.pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 0) width=1; /* more accurate */
    dc->u.x.pen.width=width;

    if (DC_SetupGCForBrush( dc ))
	TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
		  dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		  right-left-1, bottom-top-1, 0, 360*64 );
    if (DC_SetupGCForPen( dc ))
	TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
		  dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		  right-left-1, bottom-top-1, 0, 360*64 );
    dc->u.x.pen.width=oldwidth;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Rectangle
 */
BOOL32
X11DRV_Rectangle(DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    INT32 width, oldwidth, oldjoinstyle;

    TRACE(graphics, "(%d %d %d %d)\n", 
    	left, top, right, bottom);

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );

    if ((left == right) || (top == bottom)) return TRUE;

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }

    oldwidth = width = dc->u.x.pen.width;
    if (!width) width = 1;
    if(dc->u.x.pen.style == PS_NULL) width = 0;

    if ((dc->u.x.pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 1) width=0;
    dc->u.x.pen.width=width;
    oldjoinstyle=dc->u.x.pen.linejoin;
    if(dc->u.x.pen.type!=PS_GEOMETRIC)
            dc->u.x.pen.linejoin=PS_JOIN_MITER;

    if ((right > left + width) && (bottom > top + width))
    {
        if (DC_SetupGCForBrush( dc ))
            TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left + (width + 1) / 2,
                            dc->w.DCOrgY + top + (width + 1) / 2,
                            right-left-width-1, bottom-top-width-1);
    }
    if (DC_SetupGCForPen( dc ))
	TSXDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );

    dc->u.x.pen.width=oldwidth;
    dc->u.x.pen.linejoin=oldjoinstyle;
    return TRUE;
}

/***********************************************************************
 *           X11DRV_RoundRect
 */
BOOL32
X11DRV_RoundRect( DC *dc, INT32 left, INT32 top, INT32 right,
                  INT32 bottom, INT32 ell_width, INT32 ell_height )
{
    INT32 width, oldwidth, oldendcap;

    TRACE(graphics, "(%d %d %d %d  %d %d\n", 
    	left, top, right, bottom, ell_width, ell_height);

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );

    if ((left == right) || (top == bottom))
	return TRUE;

    ell_width  = abs( ell_width * dc->vportExtX / dc->wndExtX );
    ell_height = abs( ell_height * dc->vportExtY / dc->wndExtY );

    /* Fix the coordinates */

    if (right < left) { INT32 tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT32 tmp = bottom; bottom = top; top = tmp; }

    oldwidth=width = dc->u.x.pen.width;
    oldendcap = dc->u.x.pen.endcap;
    if (!width) width = 1;
    if(dc->u.x.pen.style == PS_NULL) width = 0;

    if ((dc->u.x.pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 0) width=1;
    dc->u.x.pen.width=width;
    dc->u.x.pen.endcap=PS_ENDCAP_SQUARE;

    if (DC_SetupGCForBrush( dc ))
    {
        if (ell_width > (right-left) )
            if (ell_height > (bottom-top) )
                    TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                              dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                              right - left - 1, bottom - top - 1,
                              0, 360 * 64 );
            else{
                    TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                              dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                              right - left - 1, ell_height, 0, 180 * 64 );
                    TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                              dc->w.DCOrgX + left,
                              dc->w.DCOrgY + bottom - ell_height - 1,
                              right - left - 1, ell_height, 180 * 64, 180 * 64 );
           }
	else if (ell_height > (bottom-top) ){
                TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width, bottom - top - 1, 90 * 64, 180 * 64 );
                TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width -1, dc->w.DCOrgY + top,
                      ell_width, bottom - top - 1, 270 * 64, 180 * 64 );
        }else{
                TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width, ell_height, 90 * 64, 90 * 64 );
                TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left,
                      dc->w.DCOrgY + bottom - ell_height - 1,
                      ell_width, ell_height, 180 * 64, 90 * 64 );
                TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width - 1,
                      dc->w.DCOrgY + bottom - ell_height - 1,
                      ell_width, ell_height, 270 * 64, 90 * 64 );
                TSXFillArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width - 1,
                      dc->w.DCOrgY + top,
                      ell_width, ell_height, 0, 90 * 64 );
        }
        if (ell_width < right - left)
        {
            TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left + (ell_width + 1) / 2,
                            dc->w.DCOrgY + top + 1,
                            right - left - ell_width - 1,
                            (ell_height + 1) / 2 - 1);
            TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left + (ell_width + 1) / 2,
                            dc->w.DCOrgY + bottom - (ell_height) / 2 - 1,
                            right - left - ell_width - 1,
                            (ell_height) / 2 );
        }
        if  (ell_height < bottom - top)
        {
            TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                            dc->w.DCOrgX + left + 1,
                            dc->w.DCOrgY + top + (ell_height + 1) / 2,
                            right - left - 2,
                            bottom - top - ell_height - 1);
        }
    }
    /* FIXME: this could be done with on X call
     * more efficient and probably more correct
     * on any X server: XDrawArcs will draw
     * straight horizontal and vertical lines
     * if width or height are zero.
     *
     * BTW this stuff is optimized for an Xfree86 server
     * read the comments inside the X11DRV_DrawArc function
     */
    if (DC_SetupGCForPen(dc)) {
        if (ell_width > (right-left) )
            if (ell_height > (bottom-top) )
                TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
		      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		      right - left - 1, bottom -top - 1, 0 , 360 * 64 );
            else{
		TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
		      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		      right - left - 1, ell_height - 1, 0 , 180 * 64 );
		TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
		      dc->w.DCOrgX + left, 
                      dc->w.DCOrgY + bottom - ell_height,
		      right - left - 1, ell_height - 1, 180 * 64 , 180 * 64 );
            }
	else if (ell_height > (bottom-top) ){
                TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width - 1 , bottom - top - 1, 90 * 64 , 180 * 64 );
                TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width, 
                      dc->w.DCOrgY + top,
                      ell_width - 1 , bottom - top - 1, 270 * 64 , 180 * 64 );
	}else{
            TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width - 1, ell_height - 1, 90 * 64, 90 * 64 );
            TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + bottom - ell_height,
                      ell_width - 1, ell_height - 1, 180 * 64, 90 * 64 );
            TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width,
                      dc->w.DCOrgY + bottom - ell_height,
                      ell_width - 1, ell_height - 1, 270 * 64, 90 * 64 );
            TSXDrawArc( display, dc->u.x.drawable, dc->u.x.gc,
                      dc->w.DCOrgX + right - ell_width, dc->w.DCOrgY + top,
                      ell_width - 1, ell_height - 1, 0, 90 * 64 );
	}
	if (ell_width < right - left)
	{
	    TSXDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
               dc->w.DCOrgX + left + ell_width / 2,
		       dc->w.DCOrgY + top,
               dc->w.DCOrgX + right - (ell_width+1) / 2,
		       dc->w.DCOrgY + top);
	    TSXDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
               dc->w.DCOrgX + left + ell_width / 2 ,
		       dc->w.DCOrgY + bottom - 1,
               dc->w.DCOrgX + right - (ell_width+1)/ 2,
		       dc->w.DCOrgY + bottom - 1);
	}
	if (ell_height < bottom - top)
	{
	    TSXDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
		       dc->w.DCOrgX + right - 1,
               dc->w.DCOrgY + top + ell_height / 2,
		       dc->w.DCOrgX + right - 1,
               dc->w.DCOrgY + bottom - (ell_height+1) / 2);
	    TSXDrawLine( display, dc->u.x.drawable, dc->u.x.gc, 
		       dc->w.DCOrgX + left,
               dc->w.DCOrgY + top + ell_height / 2,
		       dc->w.DCOrgX + left,
               dc->w.DCOrgY + bottom - (ell_height+1) / 2);
	}
    }
    dc->u.x.pen.width=oldwidth;
    dc->u.x.pen.endcap=oldendcap;
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
    
    TSXSetForeground( display, dc->u.x.gc, pixel );
    TSXSetFunction( display, dc->u.x.gc, GXcopy );
    TSXDrawPoint( display, dc->u.x.drawable, dc->u.x.gc, x, y );

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
    EnterCriticalSection( &X11DRV_CritSection );
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
    LeaveCriticalSection( &X11DRV_CritSection );
    
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

    if (!(tmpVisRgn = CreateRectRgn32( 0, 0, 0, 0 ))) return FALSE;

      /* Transform region into device co-ords */
    if (  !REGION_LPTODP( hdc, tmpVisRgn, hrgn )
        || OffsetRgn32( tmpVisRgn, dc->w.DCOrgX, dc->w.DCOrgY ) == ERROR) {
        DeleteObject32( tmpVisRgn );
	return FALSE;
    }

      /* Modify visible region */
    if (!(prevVisRgn = SaveVisRgn( hdc ))) {
        DeleteObject32( tmpVisRgn );
	return FALSE;
    }
    CombineRgn32( tmpVisRgn, prevVisRgn, tmpVisRgn, RGN_AND );
    SelectVisRgn( hdc, tmpVisRgn );
    DeleteObject32( tmpVisRgn );

      /* Fill the region */

    GetRgnBox32( dc->w.hGCClipRgn, &box );
    if (DC_SetupGCForBrush( dc ))
	TSXFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		          box.left, box.top,
		          box.right-box.left, box.bottom-box.top );

      /* Restore the visible region */

    RestoreVisRgn( hdc );
    return TRUE;
}

/**********************************************************************
 *          X11DRV_Polyline
 */
BOOL32
X11DRV_Polyline( DC *dc, LPPOINT32 pt, INT32 count )
{
    INT32 oldwidth;
    register int i;
    XPoint *points;
    if((oldwidth=dc->u.x.pen.width)==0) dc->u.x.pen.width=1;

    points = (XPoint *) xmalloc (sizeof (XPoint) * (count));
    for (i = 0; i < count; i++)
    {
    points[i].x = dc->w.DCOrgX + XLPTODP( dc, pt[i].x );
    points[i].y = dc->w.DCOrgY + YLPTODP( dc, pt[i].y );
    }

    if (DC_SetupGCForPen ( dc ))
    TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
           points, count, CoordModeOrigin );

    free( points );
    dc->u.x.pen.width=oldwidth;
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
	TSXFillPolygon( display, dc->u.x.drawable, dc->u.x.gc,
		     points, count+1, Complex, CoordModeOrigin);

    if (DC_SetupGCForPen ( dc ))
	TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
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
	    TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
		        points, j + 1, CoordModeOrigin );
	}
	free( points );
    }
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolyline
 */
BOOL32 
X11DRV_PolyPolyline( DC *dc, LPPOINT32 pt, LPDWORD counts, DWORD polylines )
{
    if (DC_SetupGCForPen ( dc ))
    {
        int i, j, max = 0;
        XPoint *points;

        for (i = 0; i < polylines; i++) if (counts[i] > max) max = counts[i];
        points = (XPoint *) xmalloc( sizeof(XPoint) * (max+1) );

        for (i = 0; i < polylines; i++)
        {
            for (j = 0; j < counts[i]; j++)
            {
                points[j].x = dc->w.DCOrgX + XLPTODP( dc, pt->x );
                points[j].y = dc->w.DCOrgY + YLPTODP( dc, pt->y );
                pt++;
            }
            points[j] = points[0];
            TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
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
 *
 * The Xlib critical section must be entered before calling this function.
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
                             rect.left,
                             rect.top,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             AllPlanes, ZPixmap ))) return FALSE;

    if (DC_SetupGCForBrush( dc ))
    {
          /* ROP mode is always GXcopy for flood-fill */
        XSetFunction( display, dc->u.x.gc, GXcopy );
        X11DRV_InternalFloodFill(image, dc,
                                 XLPTODP(dc,params->x) + dc->w.DCOrgX - rect.left,
                                 YLPTODP(dc,params->y) + dc->w.DCOrgY - rect.top,
                                 rect.left,
                                 rect.top,
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
    BOOL32 result;
    struct FloodFill_params params = { dc, x, y, color, fillType };

    TRACE(graphics, "X11DRV_ExtFloodFill %d,%d %06lx %d\n",
                      x, y, color, fillType );

    if (!PtVisible32( dc->hSelf, x, y )) return FALSE;
    EnterCriticalSection( &X11DRV_CritSection );
    result = CALL_LARGE_STACK( X11DRV_DoFloodFill, &params );
    LeaveCriticalSection( &X11DRV_CritSection );
    return result;
}

/******************************************************************
 * 
 *   *Very* simple bezier drawing code, 
 *
 *   It uses a recursive algorithm to divide the curve in a series
 *   of straight line segements. Not ideal but for me sufficient.
 *   If you are in need for something better look for some incremental
 *   algorithm.
 *
 *   7 July 1998 Rein Klazes
 */

 /* 
  * some macro definitions for bezier drawing
  *
  * to avoid trucation errors the coordinates are
  * shifted upwards. When used in drawing they are
  * shifted down again, including correct rounding
  * and avoiding floating point arithmatic
  * 4 bits should allow 27 bits coordinates which I saw
  * somewere in the win32 doc's
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
#define BEZMAXPOINTS    (150)

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
*       returns true if the recusion can be terminated
*/
static BOOL32 BezierCheck( int level, POINT32 *Points)
{ 
    INT32 dx, dy;
    dx=Points[3].x-Points[0].x;
    dy=Points[3].y-Points[0].y;
    if(ABS(dy)<=ABS(dx)){/* shallow line */
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
    
/***********************************************************************
 *           X11DRV_Bezier
 *   Draw a -what microsoft calls- bezier curve
 *   The routine recursively devides the curve
 *   in two parts until a straight line can be drawn
 *
 *   level      recusion depth counted backwards
 *   dc         device context
 *   Points     array of begin(0), end(3) and control points(1 and 2)
 *   XPoints    array with points calculated sofar
 *   *pIx       nr points calculated sofar
 *   
 */
static void X11DRV_Bezier(int level, DC * dc, POINT32 *Points, 
                          XPoint* xpoints, unsigned int* pIx)
{
    if(*pIx == BEZMAXPOINTS){
        TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
                    xpoints, *pIx, CoordModeOrigin );
        *pIx=0;
    }
    if(!level || BezierCheck(level, Points)) {
        if(*pIx == 0){
            xpoints[*pIx].x= dc->w.DCOrgX + BEZIERSHIFTDOWN(Points[0].x);
            xpoints[*pIx].y= dc->w.DCOrgY + BEZIERSHIFTDOWN(Points[0].y);
            *pIx=1;
        }
        xpoints[*pIx].x= dc->w.DCOrgX + BEZIERSHIFTDOWN(Points[3].x);
        xpoints[*pIx].y= dc->w.DCOrgY + BEZIERSHIFTDOWN(Points[3].y);
        (*pIx) ++;
    } else {
        POINT32 Points2[4]; /* for the second recursive call */
        Points2[3]=Points[3];
        BEZIERMIDDLE(Points2[2], Points[2], Points[3]);
        BEZIERMIDDLE(Points2[0], Points[1], Points[2]);
        BEZIERMIDDLE(Points2[1],Points2[0],Points2[2]);

        BEZIERMIDDLE(Points[1], Points[0],  Points[1]);
        BEZIERMIDDLE(Points[2], Points[1], Points2[0]);
        BEZIERMIDDLE(Points[3], Points[2], Points2[1]);

        Points2[0]=Points[3];

        /* do the two halves */
        X11DRV_Bezier(level-1, dc, Points, xpoints, pIx);
        X11DRV_Bezier(level-1, dc, Points2, xpoints, pIx);
    }
}

/***********************************************************************
 *           X11DRV_PolyBezier
 *      Implement functionality for PolyBezier and PolyBezierTo
 *      calls. 
 *      [i] dc pointer to device context
 *      [i] start, first point in curve
 *      [i] BezierPoints , array of point filled with rest of the points
 *      [i] count, number of points in BezierPoints, must be a 
 *          multiple of 3.
 */
BOOL32
X11DRV_PolyBezier(DC *dc, POINT32 start, POINT32 *BezierPoints, DWORD count)
{
    POINT32 Points[4]; 
    int i;
    unsigned int ix=0;
    XPoint* xpoints;
    TRACE(graphics, "dc=%04x count=%ld %d,%d - %d,%d - %d,%d -%d,%d \n", 
            (int)dc, count,
            start.x, start.y,
            (Points+0)->x, (Points+0)->y, 
            (Points+1)->x, (Points+1)->y, 
            (Points+2)->x, (Points+2)->y); 
    if(!count || count % 3){/* paranoid */
        WARN(graphics," bad value for count : %ld\n", count);
        return FALSE; 
    }
    xpoints=(XPoint*) xmalloc( sizeof(XPoint)*BEZMAXPOINTS);
    Points[3].x=BEZIERSHIFTUP(XLPTODP(dc,start.x));
    Points[3].y=BEZIERSHIFTUP(YLPTODP(dc,start.y));
    while(count){
        Points[0]=Points[3];
        for(i=1;i<4;i++) {
            Points[i].x= BEZIERSHIFTUP(XLPTODP(dc,BezierPoints->x));
            Points[i].y= BEZIERSHIFTUP(YLPTODP(dc,BezierPoints->y));
            BezierPoints++;
        }
        X11DRV_Bezier(BEZIERMAXDEPTH , dc, Points, xpoints, &ix );
        count -=3;
    }
    if( ix) TSXDrawLines( display, dc->u.x.drawable, dc->u.x.gc,
                xpoints, ix, CoordModeOrigin );
    free(xpoints);
    return TRUE;
}

/**********************************************************************
 *          X11DRV_SetBkColor
 */
COLORREF
X11DRV_SetBkColor( DC *dc, COLORREF color )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    COLORREF oldColor;

    oldColor = dc->w.backgroundColor;
    dc->w.backgroundColor = color;

    physDev->backgroundPixel = COLOR_ToPhysical( dc, color );

    return oldColor;
}

/**********************************************************************
 *          X11DRV_SetTextColor
 */
COLORREF
X11DRV_SetTextColor( DC *dc, COLORREF color )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    COLORREF oldColor;

    oldColor = dc->w.textColor;
    dc->w.textColor = color;

    physDev->textPixel = COLOR_ToPhysical( dc, color );

    return oldColor;
}

/*
 * X11 graphics driver graphics functions
 *
 * Copyright 1993,1994 Alexandre Julliard
 */

/*
 * FIXME: none of these functions obey the GM_ADVANCED
 * graphics mode
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Intrinsic.h>
#include "ts_xlib.h"
#include "ts_xutil.h"

#include <math.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include <stdlib.h>
#ifndef PI
#define PI M_PI
#endif
#include <string.h>

#include "x11drv.h"
#include "x11font.h"
#include "bitmap.h"
#include "gdi.h"
#include "dc.h"
#include "monitor.h"
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

  /* ROP code to GC function conversion */
const int X11DRV_XROPfunction[16] =
{
    GXclear,        /* R2_BLACK */
    GXnor,          /* R2_NOTMERGEPEN */
    GXandInverted,  /* R2_MASKNOTPEN */
    GXcopyInverted, /* R2_NOTCOPYPEN */
    GXandReverse,   /* R2_MASKPENNOT */
    GXinvert,       /* R2_NOT */
    GXxor,          /* R2_XORPEN */
    GXnand,         /* R2_NOTMASKPEN */
    GXand,          /* R2_MASKPEN */
    GXequiv,        /* R2_NOTXORPEN */
    GXnoop,         /* R2_NOP */
    GXorInverted,   /* R2_MERGENOTPEN */
    GXcopy,         /* R2_COPYPEN */
    GXorReverse,    /* R2_MERGEPENNOT */
    GXor,           /* R2_MERGEPEN */
    GXset           /* R2_WHITE */
};


/***********************************************************************
 *           X11DRV_SetupGCForPatBlt
 *
 * Setup the GC for a PatBlt operation using current brush.
 * If fMapColors is TRUE, X pixels are mapped to Windows colors.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForPatBlt( DC * dc, GC gc, BOOL fMapColors )
{
    XGCValues val;
    unsigned long mask;
    Pixmap pixmap = 0;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (physDev->brush.style == BS_NULL) return FALSE;
    if (physDev->brush.pixel == -1)
    {
	/* Special case used for monochrome pattern brushes.
	 * We need to swap foreground and background because
	 * Windows does it the wrong way...
	 */
	val.foreground = physDev->backgroundPixel;
	val.background = physDev->textPixel;
    }
    else
    {
	val.foreground = physDev->brush.pixel;
	val.background = physDev->backgroundPixel;
    }
    if (fMapColors && COLOR_PixelToPalette)
    {
        val.foreground = COLOR_PixelToPalette[val.foreground];
        val.background = COLOR_PixelToPalette[val.background];
    }

    if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc);

    val.function = X11DRV_XROPfunction[dc->w.ROPmode-1];
    /*
    ** Let's replace GXinvert by GXxor with (black xor white)
    ** This solves the selection color and leak problems in excel
    ** FIXME : Let's do that only if we work with X-pixels, not with Win-pixels
    */
    if (val.function == GXinvert)
	{
	val.foreground = BlackPixelOfScreen(X11DRV_GetXScreen()) ^ WhitePixelOfScreen(X11DRV_GetXScreen());
	val.function = GXxor;
	}
    val.fill_style = physDev->brush.fillStyle;
    switch(val.fill_style)
    {
    case FillStippled:
    case FillOpaqueStippled:
	if (dc->w.backgroundMode==OPAQUE) val.fill_style = FillOpaqueStippled;
	val.stipple = physDev->brush.pixmap;
	mask = GCStipple;
        break;

    case FillTiled:
        if (fMapColors && COLOR_PixelToPalette)
        {
            register int x, y;
            XImage *image;
            EnterCriticalSection( &X11DRV_CritSection );
            pixmap = XCreatePixmap( display, 
				    X11DRV_GetXRootWindow(), 
				    8, 8, 
				    MONITOR_GetDepth(&MONITOR_PrimaryMonitor) );
            image = XGetImage( display, physDev->brush.pixmap, 0, 0, 8, 8,
                               AllPlanes, ZPixmap );
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    XPutPixel( image, x, y,
                               COLOR_PixelToPalette[XGetPixel( image, x, y)] );
            XPutImage( display, pixmap, gc, image, 0, 0, 0, 0, 8, 8 );
            XDestroyImage( image );
            LeaveCriticalSection( &X11DRV_CritSection );
            val.tile = pixmap;
        }
        else val.tile = physDev->brush.pixmap;
	mask = GCTile;
        break;

    default:
        mask = 0;
        break;
    }
    val.ts_x_origin = dc->w.DCOrgX + dc->w.brushOrgX;
    val.ts_y_origin = dc->w.DCOrgY + dc->w.brushOrgY;
    val.fill_rule = (dc->w.polyFillMode==WINDING) ? WindingRule : EvenOddRule;
    TSXChangeGC( display, gc, 
	       GCFunction | GCForeground | GCBackground | GCFillStyle |
	       GCFillRule | GCTileStipXOrigin | GCTileStipYOrigin | mask,
	       &val );
    if (pixmap) TSXFreePixmap( display, pixmap );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetupGCForBrush
 *
 * Setup physDev->gc for drawing operations using current brush.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForBrush( DC * dc )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    return X11DRV_SetupGCForPatBlt( dc, physDev->gc, FALSE );
}


/***********************************************************************
 *           X11DRV_SetupGCForPen
 *
 * Setup physDev->gc for drawing operations using current pen.
 * Return FALSE if pen is PS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForPen( DC * dc )
{
    XGCValues val;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (physDev->pen.style == PS_NULL) return FALSE;

    if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc); 

    switch (dc->w.ROPmode)
    {
    case R2_BLACK :
	val.foreground = BlackPixelOfScreen( X11DRV_GetXScreen() );
	val.function = GXcopy;
	break;
    case R2_WHITE :
	val.foreground = WhitePixelOfScreen( X11DRV_GetXScreen() );
	val.function = GXcopy;
	break;
    case R2_XORPEN :
	val.foreground = physDev->pen.pixel;
	/* It is very unlikely someone wants to XOR with 0 */
	/* This fixes the rubber-drawings in paintbrush */
	if (val.foreground == 0)
	    val.foreground = BlackPixelOfScreen( X11DRV_GetXScreen() )
			    ^ WhitePixelOfScreen( X11DRV_GetXScreen() );
	val.function = GXxor;
	break;
    default :
	val.foreground = physDev->pen.pixel;
	val.function   = X11DRV_XROPfunction[dc->w.ROPmode-1];
    }
    val.background = physDev->backgroundPixel;
    val.fill_style = FillSolid;
    if ((physDev->pen.width <= 1) &&
        (physDev->pen.style != PS_SOLID) &&
        (physDev->pen.style != PS_INSIDEFRAME))
    {
	TSXSetDashes( display, physDev->gc, 0, physDev->pen.dashes,
		      physDev->pen.dash_len );
	val.line_style = (dc->w.backgroundMode == OPAQUE) ?
	                      LineDoubleDash : LineOnOffDash;
    }
    else val.line_style = LineSolid;
    val.line_width = physDev->pen.width;
    if (val.line_width <= 1) {
	val.cap_style = CapNotLast;
    } else {
	switch (physDev->pen.endcap)
	{
	case PS_ENDCAP_SQUARE:
	    val.cap_style = CapProjecting;
	    break;
	case PS_ENDCAP_FLAT:
	    val.cap_style = CapButt;
	    break;
	case PS_ENDCAP_ROUND:
	default:
	    val.cap_style = CapRound;
	}
    }
    switch (physDev->pen.linejoin)
    {
    case PS_JOIN_BEVEL:
	val.join_style = JoinBevel;
        break;
    case PS_JOIN_MITER:
	val.join_style = JoinMiter;
        break;
    case PS_JOIN_ROUND:
    default:
	val.join_style = JoinRound;
    }
    TSXChangeGC( display, physDev->gc, 
	       GCFunction | GCForeground | GCBackground | GCLineWidth |
	       GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle, &val );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetupGCForText
 *
 * Setup physDev->gc for text drawing operations.
 * Return FALSE if the font is null, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForText( DC * dc )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    XFontStruct* xfs = XFONT_GetFontStruct( physDev->font );

    if( xfs )
    {
	XGCValues val;

	if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc);

	val.function   = GXcopy;  /* Text is always GXcopy */
	val.foreground = physDev->textPixel;
	val.background = physDev->backgroundPixel;
	val.fill_style = FillSolid;
	val.font       = xfs->fid;

	TSXChangeGC( display, physDev->gc,
		   GCFunction | GCForeground | GCBackground | GCFillStyle |
		   GCFont, &val );
	return TRUE;
    } 
    WARN(graphics, "Physical font failure\n" );
    return FALSE;
}


/**********************************************************************
 *	     X11DRV_MoveToEx
 */
BOOL
X11DRV_MoveToEx(DC *dc,INT x,INT y,LPPOINT pt) {
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
BOOL
X11DRV_LineTo( DC *dc, INT x, INT y )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (X11DRV_SetupGCForPen( dc ))
	TSXDrawLine(display, physDev->drawable, physDev->gc, 
		  dc->w.DCOrgX + XLPTODP( dc, dc->w.CursPosX ),
		  dc->w.DCOrgY + YLPTODP( dc, dc->w.CursPosY ),
		  dc->w.DCOrgX + XLPTODP( dc, x ),
		  dc->w.DCOrgY + YLPTODP( dc, y ) );
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}



/***********************************************************************
 *           X11DRV_DrawArc
 *
 * Helper functions for Arc(), Chord() and Pie().
 * 'lines' is the number of lines to draw: 0 for Arc, 1 for Chord, 2 for Pie.
 *
 */
static BOOL
X11DRV_DrawArc( DC *dc, INT left, INT top, INT right,
                INT bottom, INT xstart, INT ystart,
                INT xend, INT yend, INT lines )
{
    INT xcenter, ycenter, istart_angle, idiff_angle;
    INT width, oldwidth, oldendcap;
    double start_angle, end_angle;
    XPoint points[4];
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    xstart = XLPTODP( dc, xstart );
    ystart = YLPTODP( dc, ystart );
    xend   = XLPTODP( dc, xend );
    yend   = YLPTODP( dc, yend );

    if (right < left) { INT tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT tmp = bottom; bottom = top; top = tmp; }
    if ((left == right) || (top == bottom)
            ||(lines && ((right-left==1)||(bottom-top==1)))) return TRUE;

    oldwidth = width = physDev->pen.width;
    oldendcap = physDev->pen.endcap;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1; /* more accurate */
    physDev->pen.width = width;
    physDev->pen.endcap = PS_ENDCAP_SQUARE;

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
    istart_angle = (INT)(start_angle * 180 * 64 / PI + 0.5);
    idiff_angle  = (INT)((end_angle - start_angle) * 180 * 64 / PI + 0.5);
    if (idiff_angle <= 0) idiff_angle += 360 * 64;

      /* Fill arc with brush if Chord() or Pie() */

    if ((lines > 0) && X11DRV_SetupGCForBrush( dc )) {
        TSXSetArcMode( display, physDev->gc,
		       (lines==1) ? ArcChord : ArcPieSlice);
        TSXFillArc( display, physDev->drawable, physDev->gc,
                 dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                 right-left-1, bottom-top-1, istart_angle, idiff_angle );
    }

      /* Draw arc and lines */

    if (X11DRV_SetupGCForPen( dc )){
    TSXDrawArc( display, physDev->drawable, physDev->gc,
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
                INT dx1,dy1;
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
    TSXDrawLines( display, physDev->drawable, physDev->gc,
	        points, lines+1, CoordModeOrigin );
        }
    }
    physDev->pen.width = oldwidth;
    physDev->pen.endcap = oldendcap;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Arc
 */
BOOL
X11DRV_Arc( DC *dc, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( dc, left, top, right, bottom,
			   xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           X11DRV_Pie
 */
BOOL
X11DRV_Pie( DC *dc, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( dc, left, top, right, bottom,
			   xstart, ystart, xend, yend, 2 );
}

/***********************************************************************
 *           X11DRV_Chord
 */
BOOL
X11DRV_Chord( DC *dc, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( dc, left, top, right, bottom,
		  	   xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           X11DRV_Ellipse
 */
BOOL
X11DRV_Ellipse( DC *dc, INT left, INT top, INT right, INT bottom )
{
    INT width, oldwidth;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    if ((left == right) || (top == bottom)) return TRUE;

    if (right < left) { INT tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT tmp = bottom; bottom = top; top = tmp; }
    
    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1; /* more accurate */
    physDev->pen.width = width;

    if (X11DRV_SetupGCForBrush( dc ))
	TSXFillArc( display, physDev->drawable, physDev->gc,
		  dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		  right-left-1, bottom-top-1, 0, 360*64 );
    if (X11DRV_SetupGCForPen( dc ))
	TSXDrawArc( display, physDev->drawable, physDev->gc,
		  dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		  right-left-1, bottom-top-1, 0, 360*64 );
    physDev->pen.width = oldwidth;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Rectangle
 */
BOOL
X11DRV_Rectangle(DC *dc, INT left, INT top, INT right, INT bottom)
{
    INT width, oldwidth, oldjoinstyle;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    TRACE(graphics, "(%d %d %d %d)\n", 
    	left, top, right, bottom);

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );

    if ((left == right) || (top == bottom)) return TRUE;

    if (right < left) { INT tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT tmp = bottom; bottom = top; top = tmp; }

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 1) width = 0;
    physDev->pen.width = width;
    oldjoinstyle = physDev->pen.linejoin;
    if(physDev->pen.type != PS_GEOMETRIC)
        physDev->pen.linejoin = PS_JOIN_MITER;

    if ((right > left + width) && (bottom > top + width))
    {
        if (X11DRV_SetupGCForBrush( dc ))
            TSXFillRectangle( display, physDev->drawable, physDev->gc,
                            dc->w.DCOrgX + left + (width + 1) / 2,
                            dc->w.DCOrgY + top + (width + 1) / 2,
                            right-left-width-1, bottom-top-width-1);
    }
    if (X11DRV_SetupGCForPen( dc ))
	TSXDrawRectangle( display, physDev->drawable, physDev->gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );

    physDev->pen.width = oldwidth;
    physDev->pen.linejoin = oldjoinstyle;
    return TRUE;
}

/***********************************************************************
 *           X11DRV_RoundRect
 */
BOOL
X11DRV_RoundRect( DC *dc, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height )
{
    INT width, oldwidth, oldendcap;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    TRACE(graphics, "(%d %d %d %d  %d %d\n", 
    	left, top, right, bottom, ell_width, ell_height);

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );

    if ((left == right) || (top == bottom))
	return TRUE;

    /* Make sure ell_width and ell_height are >= 1 otherwise XDrawArc gets
       called with width/height < 0 */
    ell_width  = MAX(abs( ell_width * dc->vportExtX / dc->wndExtX ), 1);
    ell_height = MAX(abs( ell_height * dc->vportExtY / dc->wndExtY ), 1);

    /* Fix the coordinates */

    if (right < left) { INT tmp = right; right = left; left = tmp; }
    if (bottom < top) { INT tmp = bottom; bottom = top; top = tmp; }

    oldwidth = width = physDev->pen.width;
    oldendcap = physDev->pen.endcap;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (right-left)) width=(right-left + 1)/2;
        if (2*width > (bottom-top)) width=(bottom-top + 1)/2;
        left   += width / 2;
        right  -= (width - 1) / 2;
        top    += width / 2;
        bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1;
    physDev->pen.width = width;
    physDev->pen.endcap = PS_ENDCAP_SQUARE;

    if (X11DRV_SetupGCForBrush( dc ))
    {
        if (ell_width > (right-left) )
            if (ell_height > (bottom-top) )
                    TSXFillArc( display, physDev->drawable, physDev->gc,
				dc->w.DCOrgX + left, dc->w.DCOrgY + top,
				right - left - 1, bottom - top - 1,
				0, 360 * 64 );
            else{
                    TSXFillArc( display, physDev->drawable, physDev->gc,
				dc->w.DCOrgX + left, dc->w.DCOrgY + top,
				right - left - 1, ell_height, 0, 180 * 64 );
                    TSXFillArc( display, physDev->drawable, physDev->gc,
				dc->w.DCOrgX + left,
				dc->w.DCOrgY + bottom - ell_height - 1,
				right - left - 1, ell_height, 180 * 64,
				180 * 64 );
           }
	else if (ell_height > (bottom-top) ){
                TSXFillArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width, bottom - top - 1, 90 * 64, 180 * 64 );
                TSXFillArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + right - ell_width -1, dc->w.DCOrgY + top,
                      ell_width, bottom - top - 1, 270 * 64, 180 * 64 );
        }else{
                TSXFillArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width, ell_height, 90 * 64, 90 * 64 );
                TSXFillArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + left,
                      dc->w.DCOrgY + bottom - ell_height - 1,
                      ell_width, ell_height, 180 * 64, 90 * 64 );
                TSXFillArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + right - ell_width - 1,
                      dc->w.DCOrgY + bottom - ell_height - 1,
                      ell_width, ell_height, 270 * 64, 90 * 64 );
                TSXFillArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + right - ell_width - 1,
                      dc->w.DCOrgY + top,
                      ell_width, ell_height, 0, 90 * 64 );
        }
        if (ell_width < right - left)
        {
            TSXFillRectangle( display, physDev->drawable, physDev->gc,
                            dc->w.DCOrgX + left + (ell_width + 1) / 2,
                            dc->w.DCOrgY + top + 1,
                            right - left - ell_width - 1,
                            (ell_height + 1) / 2 - 1);
            TSXFillRectangle( display, physDev->drawable, physDev->gc,
                            dc->w.DCOrgX + left + (ell_width + 1) / 2,
                            dc->w.DCOrgY + bottom - (ell_height) / 2 - 1,
                            right - left - ell_width - 1,
                            (ell_height) / 2 );
        }
        if  (ell_height < bottom - top)
        {
            TSXFillRectangle( display, physDev->drawable, physDev->gc,
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
    if (X11DRV_SetupGCForPen(dc)) {
        if (ell_width > (right-left) )
            if (ell_height > (bottom-top) )
                TSXDrawArc( display, physDev->drawable, physDev->gc,
		      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		      right - left - 1, bottom -top - 1, 0 , 360 * 64 );
            else{
		TSXDrawArc( display, physDev->drawable, physDev->gc,
		      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		      right - left - 1, ell_height - 1, 0 , 180 * 64 );
		TSXDrawArc( display, physDev->drawable, physDev->gc,
		      dc->w.DCOrgX + left, 
                      dc->w.DCOrgY + bottom - ell_height,
		      right - left - 1, ell_height - 1, 180 * 64 , 180 * 64 );
            }
	else if (ell_height > (bottom-top) ){
                TSXDrawArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width - 1 , bottom - top - 1, 90 * 64 , 180 * 64 );
                TSXDrawArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + right - ell_width, 
                      dc->w.DCOrgY + top,
                      ell_width - 1 , bottom - top - 1, 270 * 64 , 180 * 64 );
	}else{
            TSXDrawArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + top,
                      ell_width - 1, ell_height - 1, 90 * 64, 90 * 64 );
            TSXDrawArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + left, dc->w.DCOrgY + bottom - ell_height,
                      ell_width - 1, ell_height - 1, 180 * 64, 90 * 64 );
            TSXDrawArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + right - ell_width,
                      dc->w.DCOrgY + bottom - ell_height,
                      ell_width - 1, ell_height - 1, 270 * 64, 90 * 64 );
            TSXDrawArc( display, physDev->drawable, physDev->gc,
                      dc->w.DCOrgX + right - ell_width, dc->w.DCOrgY + top,
                      ell_width - 1, ell_height - 1, 0, 90 * 64 );
	}
	if (ell_width < right - left)
	{
	    TSXDrawLine( display, physDev->drawable, physDev->gc, 
               dc->w.DCOrgX + left + ell_width / 2,
		       dc->w.DCOrgY + top,
               dc->w.DCOrgX + right - (ell_width+1) / 2,
		       dc->w.DCOrgY + top);
	    TSXDrawLine( display, physDev->drawable, physDev->gc, 
               dc->w.DCOrgX + left + ell_width / 2 ,
		       dc->w.DCOrgY + bottom - 1,
               dc->w.DCOrgX + right - (ell_width+1)/ 2,
		       dc->w.DCOrgY + bottom - 1);
	}
	if (ell_height < bottom - top)
	{
	    TSXDrawLine( display, physDev->drawable, physDev->gc, 
		       dc->w.DCOrgX + right - 1,
               dc->w.DCOrgY + top + ell_height / 2,
		       dc->w.DCOrgX + right - 1,
               dc->w.DCOrgY + bottom - (ell_height+1) / 2);
	    TSXDrawLine( display, physDev->drawable, physDev->gc, 
		       dc->w.DCOrgX + left,
               dc->w.DCOrgY + top + ell_height / 2,
		       dc->w.DCOrgX + left,
               dc->w.DCOrgY + bottom - (ell_height+1) / 2);
	}
    }
    physDev->pen.width = oldwidth;
    physDev->pen.endcap = oldendcap;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetPixel
 */
COLORREF
X11DRV_SetPixel( DC *dc, INT x, INT y, COLORREF color )
{
    Pixel pixel;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    
    x = dc->w.DCOrgX + XLPTODP( dc, x );
    y = dc->w.DCOrgY + YLPTODP( dc, y );
    pixel = COLOR_ToPhysical( dc, color );
    
    TSXSetForeground( display, physDev->gc, pixel );
    TSXSetFunction( display, physDev->gc, GXcopy );
    TSXDrawPoint( display, physDev->drawable, physDev->gc, x, y );

    /* inefficient but simple... */

    return COLOR_ToLogical(pixel);
}


/***********************************************************************
 *           X11DRV_GetPixel
 */
COLORREF
X11DRV_GetPixel( DC *dc, INT x, INT y )
{
    static Pixmap pixmap = 0;
    XImage * image;
    int pixel;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    x = dc->w.DCOrgX + XLPTODP( dc, x );
    y = dc->w.DCOrgY + YLPTODP( dc, y );
    EnterCriticalSection( &X11DRV_CritSection );
    if (dc->w.flags & DC_MEMORY)
    {
        image = XGetImage( display, physDev->drawable, x, y, 1, 1,
                           AllPlanes, ZPixmap );
    }
    else
    {
        /* If we are reading from the screen, use a temporary copy */
        /* to avoid a BadMatch error */
        if (!pixmap) pixmap = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                             1, 1, dc->w.bitsPerPixel );
        XCopyArea( display, physDev->drawable, pixmap, BITMAP_colorGC,
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
BOOL
X11DRV_PaintRgn( DC *dc, HRGN hrgn )
{
    RECT box;
    HRGN tmpVisRgn, prevVisRgn;
    HDC  hdc = dc->hSelf; /* FIXME: should not mix dc/hdc this way */
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (!(tmpVisRgn = CreateRectRgn( 0, 0, 0, 0 ))) return FALSE;

      /* Transform region into device co-ords */
    if (  !REGION_LPTODP( hdc, tmpVisRgn, hrgn )
        || OffsetRgn( tmpVisRgn, dc->w.DCOrgX, dc->w.DCOrgY ) == ERROR) {
        DeleteObject( tmpVisRgn );
	return FALSE;
    }

      /* Modify visible region */
    if (!(prevVisRgn = SaveVisRgn16( hdc ))) {
        DeleteObject( tmpVisRgn );
	return FALSE;
    }
    CombineRgn( tmpVisRgn, prevVisRgn, tmpVisRgn, RGN_AND );
    SelectVisRgn16( hdc, tmpVisRgn );
    DeleteObject( tmpVisRgn );

      /* Fill the region */

    GetRgnBox( dc->w.hGCClipRgn, &box );
    if (X11DRV_SetupGCForBrush( dc ))
	TSXFillRectangle( display, physDev->drawable, physDev->gc,
		          box.left, box.top,
		          box.right-box.left, box.bottom-box.top );

      /* Restore the visible region */

    RestoreVisRgn16( hdc );
    return TRUE;
}

/**********************************************************************
 *          X11DRV_Polyline
 */
BOOL
X11DRV_Polyline( DC *dc, const POINT* pt, INT count )
{
    INT oldwidth;
    register int i;
    XPoint *points;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if((oldwidth = physDev->pen.width) == 0) physDev->pen.width = 1;

    points = (XPoint *) xmalloc (sizeof (XPoint) * (count));
    for (i = 0; i < count; i++)
    {
    points[i].x = dc->w.DCOrgX + XLPTODP( dc, pt[i].x );
    points[i].y = dc->w.DCOrgY + YLPTODP( dc, pt[i].y );
    }

    if (X11DRV_SetupGCForPen ( dc ))
    TSXDrawLines( display, physDev->drawable, physDev->gc,
           points, count, CoordModeOrigin );

    free( points );
    physDev->pen.width = oldwidth;
    return TRUE;
}


/**********************************************************************
 *          X11DRV_Polygon
 */
BOOL
X11DRV_Polygon( DC *dc, const POINT* pt, INT count )
{
    register int i;
    XPoint *points;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    points = (XPoint *) xmalloc (sizeof (XPoint) * (count+1));
    for (i = 0; i < count; i++)
    {
	points[i].x = dc->w.DCOrgX + XLPTODP( dc, pt[i].x );
	points[i].y = dc->w.DCOrgY + YLPTODP( dc, pt[i].y );
    }
    points[count] = points[0];

    if (X11DRV_SetupGCForBrush( dc ))
	TSXFillPolygon( display, physDev->drawable, physDev->gc,
		     points, count+1, Complex, CoordModeOrigin);

    if (X11DRV_SetupGCForPen ( dc ))
	TSXDrawLines( display, physDev->drawable, physDev->gc,
		   points, count+1, CoordModeOrigin );

    free( points );
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolygon
 */
BOOL 
X11DRV_PolyPolygon( DC *dc, const POINT* pt, const INT* counts, UINT polygons)
{
    HRGN hrgn;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    /* FIXME: The points should be converted to device coords before */
    /* creating the region. */

    hrgn = CreatePolyPolygonRgn( pt, counts, polygons, dc->w.polyFillMode );
    X11DRV_PaintRgn( dc, hrgn );
    DeleteObject( hrgn );

      /* Draw the outline of the polygons */

    if (X11DRV_SetupGCForPen ( dc ))
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
	    TSXDrawLines( display, physDev->drawable, physDev->gc,
		        points, j + 1, CoordModeOrigin );
	}
	free( points );
    }
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolyline
 */
BOOL 
X11DRV_PolyPolyline( DC *dc, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    if (X11DRV_SetupGCForPen ( dc ))
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
            TSXDrawLines( display, physDev->drawable, physDev->gc,
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
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    int left, right;

#define TO_FLOOD(x,y)  ((fillType == FLOODFILLBORDER) ? \
                        (XGetPixel(image,x,y) != pixel) : \
                        (XGetPixel(image,x,y) == pixel))

    if (!TO_FLOOD(x,y)) return;

      /* Find left and right boundaries */

    left = right = x;
    while ((left > 0) && TO_FLOOD( left-1, y )) left--;
    while ((right < image->width) && TO_FLOOD( right, y )) right++;
    XFillRectangle( display, physDev->drawable, physDev->gc,
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
    INT    x;
    INT    y;
    COLORREF color;
    UINT   fillType;
};

static BOOL X11DRV_DoFloodFill( const struct FloodFill_params *params )
{
    XImage *image;
    RECT rect;
    DC *dc = params->dc;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (GetRgnBox( dc->w.hGCClipRgn, &rect ) == ERROR) return FALSE;

    if (!(image = XGetImage( display, physDev->drawable,
                             rect.left,
                             rect.top,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             AllPlanes, ZPixmap ))) return FALSE;

    if (X11DRV_SetupGCForBrush( dc ))
    {
          /* ROP mode is always GXcopy for flood-fill */
        XSetFunction( display, physDev->gc, GXcopy );
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
BOOL
X11DRV_ExtFloodFill( DC *dc, INT x, INT y, COLORREF color,
                     UINT fillType )
{
    BOOL result;
    struct FloodFill_params params = { dc, x, y, color, fillType };

    TRACE(graphics, "X11DRV_ExtFloodFill %d,%d %06lx %d\n",
                      x, y, color, fillType );

    if (!PtVisible( dc->hSelf, x, y )) return FALSE;
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
static BOOL BezierCheck( int level, POINT *Points)
{ 
    INT dx, dy;
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
static void X11DRV_Bezier(int level, DC * dc, POINT *Points, 
                          XPoint* xpoints, unsigned int* pIx)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if(*pIx == BEZMAXPOINTS){
        TSXDrawLines( display, physDev->drawable, physDev->gc,
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
BOOL
X11DRV_PolyBezier(DC *dc, POINT start, const POINT* BezierPoints, DWORD count)
{
    POINT Points[4]; 
    int i;
    unsigned int ix=0;
    XPoint* xpoints;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    TRACE(graphics, "dc=%p count=%ld %ld,%ld - %ld,%ld - %ld,%ld - %ld,%ld\n", 
            dc, count,
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
    if( ix) TSXDrawLines( display, physDev->drawable, physDev->gc,
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

#endif /* !defined(X_DISPLAY_MISSING) */

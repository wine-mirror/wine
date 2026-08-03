/*
 * X11 graphics driver graphics functions
 *
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1998 Huw Davies
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

/*
 * FIXME: only some of these functions obey the GM_ADVANCED
 * graphics mode
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#ifndef PI
#define PI M_PI
#endif
#include <string.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

#define ABS(x)    ((x)<0?(-(x)):(x))

  /* ROP code to GC function conversion */
static const int X11DRV_XROPfunction[16] =
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


/* get the rectangle in device coordinates, with optional mirroring */
static RECT get_device_rect( HDC hdc, int left, int top, int right, int bottom )
{
    DWORD layout;
    RECT rect;

    NtGdiGetDCDword( hdc, NtGdiGetLayout, &layout );

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;
    if (layout & LAYOUT_RTL)
    {
        /* shift the rectangle so that the right border is included after mirroring */
        /* it would be more correct to do this after LPtoDP but that's not what Windows does */
        rect.left--;
        rect.right--;
    }
    lp_to_dp( hdc, (POINT *)&rect, 2 );
    if (rect.left > rect.right)
    {
        int tmp = rect.left;
        rect.left = rect.right;
        rect.right = tmp;
    }
    if (rect.top > rect.bottom)
    {
        int tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }
    return rect;
}

static void add_pen_device_bounds( X11DRV_PDEVICE *dev, const POINT *points, int count )
{
    RECT bounds, rect;
    int width = 0;

    if (!dev->bounds) return;
    reset_bounds( &bounds );

    if (dev->pen.type & PS_GEOMETRIC || dev->pen.width > 1)
    {
        /* Windows uses some heuristics to estimate the distance from the point that will be painted */
        width = dev->pen.width + 2;
        if (dev->pen.linejoin == PS_JOIN_MITER)
        {
            width *= 5;
            if (dev->pen.endcap == PS_ENDCAP_SQUARE) width = (width * 3 + 1) / 2;
        }
        else
        {
            if (dev->pen.endcap == PS_ENDCAP_SQUARE) width -= width / 4;
            else width = (width + 1) / 2;
        }
    }

    while (count-- > 0)
    {
        rect.left   = points->x - width;
        rect.top    = points->y - width;
        rect.right  = points->x + width + 1;
        rect.bottom = points->y + width + 1;
        add_bounds_rect( &bounds, &rect );
        points++;
    }

    add_device_bounds( dev, &bounds );
}

/***********************************************************************
 *           X11DRV_GetRegionData
 *
 * Calls GetRegionData on the given region and converts the rectangle
 * array to XRectangle format. The returned buffer must be freed by caller.
 * If hdc_lptodp is not 0, the rectangles are converted through LPtoDP.
 */
RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp )
{
    RGNDATA *data;
    DWORD size;
    unsigned int i;
    RECT *rect, tmp;
    XRectangle *xrect;

    if (!(size = NtGdiGetRegionData( hrgn, 0, NULL ))) return NULL;
    if (sizeof(XRectangle) > sizeof(RECT))
    {
        /* add extra size for XRectangle array */
        int count = (size - sizeof(RGNDATAHEADER)) / sizeof(RECT);
        size += count * (sizeof(XRectangle) - sizeof(RECT));
    }
    if (!(data = malloc( size ))) return NULL;
    if (!NtGdiGetRegionData( hrgn, size, data ))
    {
        free( data );
        return NULL;
    }

    rect = (RECT *)data->Buffer;
    xrect = (XRectangle *)data->Buffer;
    if (hdc_lptodp)  /* map to device coordinates */
    {
        lp_to_dp( hdc_lptodp, (POINT *)rect, data->rdh.nCount * 2 );
        for (i = 0; i < data->rdh.nCount; i++)
        {
            if (rect[i].right < rect[i].left)
            {
                INT tmp = rect[i].right;
                rect[i].right = rect[i].left;
                rect[i].left = tmp;
            }
            if (rect[i].bottom < rect[i].top)
            {
                INT tmp = rect[i].bottom;
                rect[i].bottom = rect[i].top;
                rect[i].top = tmp;
            }
        }
    }

    if (sizeof(XRectangle) > sizeof(RECT))
    {
        int j;
        /* need to start from the end */
        xrect += data->rdh.nCount;
        for (j = data->rdh.nCount-1; j >= 0; j--)
        {
            tmp = rect[j];
            if (tmp.left > SHRT_MAX) continue;
            if (tmp.top > SHRT_MAX) continue;
            if (tmp.right < SHRT_MIN) continue;
            if (tmp.bottom < SHRT_MIN) continue;
            xrect--;
            xrect->x      = max( min( tmp.left, SHRT_MAX), SHRT_MIN);
            xrect->y      = max( min( tmp.top, SHRT_MAX), SHRT_MIN);
            xrect->width  = max( min( tmp.right, SHRT_MAX ) - xrect->x, 0);
            xrect->height = max( min( tmp.bottom, SHRT_MAX ) - xrect->y, 0);
        }
        if (xrect > (XRectangle *)data->Buffer)
        {
            data->rdh.nCount -= xrect - (XRectangle *)data->Buffer;
            memmove( (XRectangle *)data->Buffer, xrect, data->rdh.nCount * sizeof(*xrect) );
        }
    }
    else
    {
        for (i = 0; i < data->rdh.nCount; i++)
        {
            tmp = rect[i];
            if (tmp.left > SHRT_MAX) continue;
            if (tmp.top > SHRT_MAX) continue;
            if (tmp.right < SHRT_MIN) continue;
            if (tmp.bottom < SHRT_MIN) continue;
            xrect->x      = max( min( tmp.left, SHRT_MAX), SHRT_MIN);
            xrect->y      = max( min( tmp.top, SHRT_MAX), SHRT_MIN);
            xrect->width  = max( min( tmp.right, SHRT_MAX ) - xrect->x, 0);
            xrect->height = max( min( tmp.bottom, SHRT_MAX ) - xrect->y, 0);
            xrect++;
        }
        data->rdh.nCount = xrect - (XRectangle *)data->Buffer;
    }
    return data;
}


/***********************************************************************
 *           update_x11_clipping
 */
static void update_x11_clipping( X11DRV_PDEVICE *physDev, HRGN rgn )
{
    RGNDATA *data;

    if (!rgn)
    {
        XSetClipMask( gdi_display, physDev->gc, None );
    }
    else if ((data = X11DRV_GetRegionData( rgn, 0 )))
    {
        XSetClipRectangles( gdi_display, physDev->gc, physDev->dc_rect.left, physDev->dc_rect.top,
                            (XRectangle *)data->Buffer, data->rdh.nCount, YXBanded );
        free( data );
    }
}


/***********************************************************************
 *           add_extra_clipping_region
 *
 * Temporarily add a region to the current clipping region.
 * The region must be restored with restore_clipping_region.
 */
BOOL add_extra_clipping_region( X11DRV_PDEVICE *dev, HRGN rgn )
{
    HRGN clip;

    if (!rgn) return FALSE;
    if (dev->region)
    {
        if (!(clip = NtGdiCreateRectRgn( 0, 0, 0, 0 ))) return FALSE;
        NtGdiCombineRgn( clip, dev->region, rgn, RGN_AND );
        update_x11_clipping( dev, clip );
        NtGdiDeleteObjectApp( clip );
    }
    else update_x11_clipping( dev, rgn );
    return TRUE;
}


/***********************************************************************
 *           restore_clipping_region
 */
void restore_clipping_region( X11DRV_PDEVICE *dev )
{
    update_x11_clipping( dev, dev->region );
}


/***********************************************************************
 *           X11DRV_SetDeviceClipping
 */
void X11DRV_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );

    physDev->region = rgn;
    update_x11_clipping( physDev, rgn );
}


/***********************************************************************
 *           X11DRV_SetupGCForPatBlt
 *
 * Setup the GC for a PatBlt operation using current brush.
 * If fMapColors is TRUE, X pixels are mapped to Windows colors.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors )
{
    DWORD bk_color, text_color, rop_mode, bk_mode, poly_fill_mode;
    XGCValues val;
    unsigned long mask;
    Pixmap pixmap = 0;
    POINT pt;

    if (physDev->brush.style == BS_NULL) return FALSE;

    NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetBkColor, &bk_color );
    NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetROP2, &rop_mode );

    if (physDev->brush.pixel == -1)
    {
	/* Special case used for monochrome pattern brushes.
	 * We need to swap foreground and background because
	 * Windows does it the wrong way...
	 */
        NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetTextColor, &text_color );
        val.foreground = X11DRV_PALETTE_ToPhysical( physDev, bk_color );
        val.background = X11DRV_PALETTE_ToPhysical( physDev, text_color );
    }
    else
    {
	val.foreground = physDev->brush.pixel;
	val.background = X11DRV_PALETTE_ToPhysical( physDev, bk_color );
    }
    if (fMapColors && X11DRV_PALETTE_XPixelToPalette)
    {
        val.foreground = X11DRV_PALETTE_XPixelToPalette[val.foreground];
        val.background = X11DRV_PALETTE_XPixelToPalette[val.background];
    }

    val.function = X11DRV_XROPfunction[rop_mode - 1];
    /*
    ** Let's replace GXinvert by GXxor with (black xor white)
    ** This solves the selection color and leak problems in excel
    ** FIXME : Let's do that only if we work with X-pixels, not with Win-pixels
    */
    if (val.function == GXinvert)
    {
        val.foreground = (WhitePixel( gdi_display, DefaultScreen(gdi_display) ) ^
                          BlackPixel( gdi_display, DefaultScreen(gdi_display) ));
	val.function = GXxor;
    }
    val.fill_style = physDev->brush.fillStyle;
    switch(val.fill_style)
    {
    case FillStippled:
    case FillOpaqueStippled:
        NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetBkMode, &bk_mode );
        if (bk_mode == OPAQUE) val.fill_style = FillOpaqueStippled;
	val.stipple = physDev->brush.pixmap;
	mask = GCStipple;
        break;

    case FillTiled:
        if (fMapColors && X11DRV_PALETTE_XPixelToPalette)
        {
            register int x, y;
            XImage *image;
            pixmap = XCreatePixmap( gdi_display, root_window, 8, 8, physDev->depth );
            image = XGetImage( gdi_display, physDev->brush.pixmap, 0, 0, 8, 8,
                               AllPlanes, ZPixmap );
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    XPutPixel( image, x, y,
                               X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, x, y)] );
            XPutImage( gdi_display, pixmap, gc, image, 0, 0, 0, 0, 8, 8 );
            XDestroyImage( image );
            val.tile = pixmap;
        }
        else val.tile = physDev->brush.pixmap;
	mask = GCTile;
        break;

    default:
        mask = 0;
        break;
    }

    NtGdiGetDCPoint( physDev->dev.hdc, NtGdiGetBrushOrgEx, &pt );
    NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetPolyFillMode, &poly_fill_mode );

    val.ts_x_origin = physDev->dc_rect.left + pt.x;
    val.ts_y_origin = physDev->dc_rect.top + pt.y;
    val.fill_rule = poly_fill_mode == WINDING ? WindingRule : EvenOddRule;
    XChangeGC( gdi_display, gc,
	       GCFunction | GCForeground | GCBackground | GCFillStyle |
	       GCFillRule | GCTileStipXOrigin | GCTileStipYOrigin | mask,
	       &val );
    if (pixmap) XFreePixmap( gdi_display, pixmap );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetupGCForBrush
 *
 * Setup physDev->gc for drawing operations using current brush.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev )
{
    return X11DRV_SetupGCForPatBlt( physDev, physDev->gc, FALSE );
}


/***********************************************************************
 *           X11DRV_SetupGCForPen
 *
 * Setup physDev->gc for drawing operations using current pen.
 * Return FALSE if pen is PS_NULL, TRUE otherwise.
 */
static BOOL X11DRV_SetupGCForPen( X11DRV_PDEVICE *physDev )
{
    DWORD rop2, bk_color, bk_mode;
    XGCValues val;

    NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetROP2, &rop2 );

    if (physDev->pen.style == PS_NULL) return FALSE;

    switch (rop2)
    {
    case R2_BLACK :
        val.foreground = BlackPixel( gdi_display, DefaultScreen(gdi_display) );
	val.function = GXcopy;
	break;
    case R2_WHITE :
        val.foreground = WhitePixel( gdi_display, DefaultScreen(gdi_display) );
	val.function = GXcopy;
	break;
    case R2_XORPEN :
	val.foreground = physDev->pen.pixel;
	/* It is very unlikely someone wants to XOR with 0 */
	/* This fixes the rubber-drawings in paintbrush */
	if (val.foreground == 0)
            val.foreground = (WhitePixel( gdi_display, DefaultScreen(gdi_display) ) ^
                              BlackPixel( gdi_display, DefaultScreen(gdi_display) ));
	val.function = GXxor;
	break;
    default :
	val.foreground = physDev->pen.pixel;
	val.function   = X11DRV_XROPfunction[rop2-1];
    }
    NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetBkColor, &bk_color );
    val.background = X11DRV_PALETTE_ToPhysical( physDev, bk_color );
    val.fill_style = FillSolid;
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

    if (physDev->pen.dash_len)
        val.line_style = (NtGdiGetDCDword( physDev->dev.hdc, NtGdiGetBkMode, &bk_mode) &&
                          bk_mode == OPAQUE && !physDev->pen.ext)
                         ? LineDoubleDash : LineOnOffDash;
    else
        val.line_style = LineSolid;

    if (physDev->pen.dash_len)
        XSetDashes( gdi_display, physDev->gc, 0, physDev->pen.dashes, physDev->pen.dash_len );
    XChangeGC( gdi_display, physDev->gc,
	       GCFunction | GCForeground | GCBackground | GCLineWidth |
	       GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle, &val );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_XWStoDS
 *
 * Performs a world-to-viewport transformation on the specified width.
 */
INT X11DRV_XWStoDS( HDC hdc, INT width )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    lp_to_dp( hdc, pt, 2 );
    return pt[1].x - pt[0].x;
}

/***********************************************************************
 *           X11DRV_YWStoDS
 *
 * Performs a world-to-viewport transformation on the specified height.
 */
INT X11DRV_YWStoDS( HDC hdc, INT height )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    lp_to_dp( hdc, pt, 2 );
    return pt[1].y - pt[0].y;
}

/***********************************************************************
 *           X11DRV_LineTo
 */
BOOL X11DRV_LineTo( PHYSDEV dev, INT x, INT y )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    POINT pt[2];

    NtGdiGetDCPoint( dev->hdc, NtGdiGetCurrentPosition, &pt[0] );
    pt[1].x = x;
    pt[1].y = y;
    lp_to_dp( dev->hdc, pt, 2 );
    add_pen_device_bounds( physDev, pt, 2 );

    if (X11DRV_SetupGCForPen( physDev ))
        XDrawLine(gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + pt[0].x, physDev->dc_rect.top + pt[0].y,
                  physDev->dc_rect.left + pt[1].x, physDev->dc_rect.top + pt[1].y );
    return TRUE;
}



/***********************************************************************
 *           X11DRV_DrawArc
 *
 * Helper functions for Arc(), Chord() and Pie().
 * 'lines' is the number of lines to draw: 0 for Arc, 1 for Chord, 2 for Pie.
 *
 */
static BOOL X11DRV_DrawArc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                            INT xstart, INT ystart, INT xend, INT yend, INT lines )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    INT xcenter, ycenter, istart_angle, idiff_angle;
    INT width, oldwidth;
    DWORD arc_dir;
    double start_angle, end_angle;
    XPoint points[4];
    POINT start, end;
    RECT rc = get_device_rect( dev->hdc, left, top, right, bottom );

    start.x = xstart;
    start.y = ystart;
    end.x = xend;
    end.y = yend;
    lp_to_dp(dev->hdc, &start, 1);
    lp_to_dp(dev->hdc, &end, 1);

    if ((rc.left == rc.right) || (rc.top == rc.bottom)
            ||(lines && ((rc.right-rc.left==1)||(rc.bottom-rc.top==1)))) return TRUE;

    if (NtGdiGetDCDword( dev->hdc, NtGdiGetArcDirection, &arc_dir ) && arc_dir == AD_CLOCKWISE)
      { POINT tmp = start; start = end; end = tmp; }

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if (physDev->pen.style == PS_INSIDEFRAME)
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1; /* more accurate */
    physDev->pen.width = width;

    xcenter = (rc.right + rc.left) / 2;
    ycenter = (rc.bottom + rc.top) / 2;
    start_angle = atan2( (double)(ycenter-start.y)*(rc.right-rc.left),
			 (double)(start.x-xcenter)*(rc.bottom-rc.top) );
    end_angle   = atan2( (double)(ycenter-end.y)*(rc.right-rc.left),
			 (double)(end.x-xcenter)*(rc.bottom-rc.top) );
    if ((start.x==end.x)&&(start.y==end.y))
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

    if ((lines > 0) && X11DRV_SetupGCForBrush( physDev )) {
        XSetArcMode( gdi_display, physDev->gc, (lines==1) ? ArcChord : ArcPieSlice);
        XFillArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, istart_angle, idiff_angle );
    }

      /* Draw arc and lines */

    if (X11DRV_SetupGCForPen( physDev ))
    {
        XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, istart_angle, idiff_angle );
        if (lines) {
            /* use the truncated values */
            start_angle=(double)istart_angle*PI/64./180.;
            end_angle=(double)(istart_angle+idiff_angle)*PI/64./180.;
            /* calculate the endpoints and round correctly */
            points[0].x = (int) floor(physDev->dc_rect.left + (rc.right+rc.left)/2.0 +
                    cos(start_angle) * (rc.right-rc.left-width*2+2) / 2. + 0.5);
            points[0].y = (int) floor(physDev->dc_rect.top + (rc.top+rc.bottom)/2.0 -
                    sin(start_angle) * (rc.bottom-rc.top-width*2+2) / 2. + 0.5);
            points[1].x = (int) floor(physDev->dc_rect.left + (rc.right+rc.left)/2.0 +
                    cos(end_angle) * (rc.right-rc.left-width*2+2) / 2. + 0.5);
            points[1].y = (int) floor(physDev->dc_rect.top + (rc.top+rc.bottom)/2.0 -
                    sin(end_angle) * (rc.bottom-rc.top-width*2+2) / 2. + 0.5);

            /* OK, this stuff is optimized for Xfree86
             * which is probably the server most used by
             * wine users. Other X servers will not
             * display correctly. (eXceed for instance)
             * so if you feel you must make changes, make sure that
             * you either use Xfree86 or separate your changes
             * from these (compile switch or whatever)
             */
            if (lines == 2) {
                INT dx1,dy1;
                points[3] = points[1];
                points[1].x = physDev->dc_rect.left + xcenter;
                points[1].y = physDev->dc_rect.top + ycenter;
                points[2] = points[1];
                dx1=points[1].x-points[0].x;
                dy1=points[1].y-points[0].y;
                if(((rc.top-rc.bottom) | -2) == -2)
                    if(dy1>0) points[1].y--;
                if(dx1<0) {
                    if (((-dx1)*64)<=ABS(dy1)*37) points[0].x--;
                    if(((-dx1*9))<(dy1*16)) points[0].y--;
                    if( dy1<0 && ((dx1*9)) < (dy1*16)) points[0].y--;
                } else {
                    if(dy1 < 0)  points[0].y--;
                    if(((rc.right-rc.left) | -2) == -2) points[1].x--;
                }
                dx1=points[3].x-points[2].x;
                dy1=points[3].y-points[2].y;
                if(((rc.top-rc.bottom) | -2 ) == -2)
                    if(dy1 < 0) points[2].y--;
                if( dx1<0){
                    if( dy1>0) points[3].y--;
                    if(((rc.right-rc.left) | -2) == -2 ) points[2].x--;
                }else {
                    points[3].y--;
                    if( dx1 * 64 < dy1 * -37 ) points[3].x--;
                }
                lines++;
	    }
            XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                        points, lines+1, CoordModeOrigin );
        }
    }

    physDev->pen.width = oldwidth;
    add_pen_device_bounds( physDev, (POINT *)&rc, 2 );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Arc
 */
BOOL X11DRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           X11DRV_Pie
 */
BOOL X11DRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 2 );
}

/***********************************************************************
 *           X11DRV_Chord
 */
BOOL X11DRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           X11DRV_Ellipse
 */
BOOL X11DRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    INT width, oldwidth;
    RECT rc = get_device_rect( dev->hdc, left, top, right, bottom );

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if (physDev->pen.style == PS_INSIDEFRAME)
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1; /* more accurate */
    physDev->pen.width = width;

    if (X11DRV_SetupGCForBrush( physDev ))
        XFillArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, 0, 360*64 );

    if (X11DRV_SetupGCForPen( physDev ))
        XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, 0, 360*64 );

    physDev->pen.width = oldwidth;
    add_pen_device_bounds( physDev, (POINT *)&rc, 2 );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Rectangle
 */
BOOL X11DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    INT width, oldwidth, oldjoinstyle;
    RECT rc = get_device_rect( dev->hdc, left, top, right, bottom );

    TRACE("(%d %d %d %d)\n", left, top, right, bottom);

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if (physDev->pen.style == PS_INSIDEFRAME)
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 1) width = 0;
    physDev->pen.width = width;
    oldjoinstyle = physDev->pen.linejoin;
    if(physDev->pen.type != PS_GEOMETRIC)
        physDev->pen.linejoin = PS_JOIN_MITER;

    rc.right--;
    rc.bottom--;
    if ((rc.right >= rc.left + width) && (rc.bottom >= rc.top + width))
    {
        if (X11DRV_SetupGCForBrush( physDev ))
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + (width + 1) / 2,
                            physDev->dc_rect.top + rc.top + (width + 1) / 2,
                            rc.right-rc.left-width, rc.bottom-rc.top-width);
    }
    if (X11DRV_SetupGCForPen( physDev ))
        XDrawRectangle( gdi_display, physDev->drawable, physDev->gc,
                        physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                        rc.right-rc.left, rc.bottom-rc.top );

    physDev->pen.width = oldwidth;
    physDev->pen.linejoin = oldjoinstyle;
    add_pen_device_bounds( physDev, (POINT *)&rc, 2 );
    return TRUE;
}

/***********************************************************************
 *           X11DRV_RoundRect
 */
BOOL X11DRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                       INT ell_width, INT ell_height )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    INT width, oldwidth, oldendcap;
    POINT pts[2];
    RECT rc = get_device_rect( dev->hdc, left, top, right, bottom );

    TRACE("(%d %d %d %d  %d %d\n",
    	left, top, right, bottom, ell_width, ell_height);

    if ((rc.left == rc.right) || (rc.top == rc.bottom))
	return TRUE;

    /* Make sure ell_width and ell_height are >= 1 otherwise XDrawArc gets
       called with width/height < 0 */
    pts[0].x = pts[0].y = 0;
    pts[1].x = ell_width;
    pts[1].y = ell_height;
    lp_to_dp(dev->hdc, pts, 2);
    ell_width  = max(abs( pts[1].x - pts[0].x ), 1);
    ell_height = max(abs( pts[1].y - pts[0].y ), 1);

    oldwidth = width = physDev->pen.width;
    oldendcap = physDev->pen.endcap;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if (physDev->pen.style == PS_INSIDEFRAME)
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1;
    physDev->pen.width = width;
    physDev->pen.endcap = PS_ENDCAP_SQUARE;

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        if (ell_width > (rc.right-rc.left) )
            if (ell_height > (rc.bottom-rc.top) )
                XFillArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, rc.bottom - rc.top - 1,
                          0, 360 * 64 );
            else{
                XFillArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, ell_height, 0, 180 * 64 );
                XFillArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left,
                          physDev->dc_rect.top + rc.bottom - ell_height - 1,
                          rc.right - rc.left - 1, ell_height, 180 * 64,
                          180 * 64 );
            }
	else if (ell_height > (rc.bottom-rc.top) ){
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width, rc.bottom - rc.top - 1, 90 * 64, 180 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width - 1, physDev->dc_rect.top + rc.top,
                      ell_width, rc.bottom - rc.top - 1, 270 * 64, 180 * 64 );
        }else{
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width, ell_height, 90 * 64, 90 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left,
                      physDev->dc_rect.top + rc.bottom - ell_height - 1,
                      ell_width, ell_height, 180 * 64, 90 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width - 1,
                      physDev->dc_rect.top + rc.bottom - ell_height - 1,
                      ell_width, ell_height, 270 * 64, 90 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width - 1,
                      physDev->dc_rect.top + rc.top,
                      ell_width, ell_height, 0, 90 * 64 );
        }
        if (ell_width < rc.right - rc.left)
        {
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + (ell_width + 1) / 2,
                            physDev->dc_rect.top + rc.top + 1,
                            rc.right - rc.left - ell_width - 1,
                            (ell_height + 1) / 2 - 1);
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + (ell_width + 1) / 2,
                            physDev->dc_rect.top + rc.bottom - (ell_height) / 2 - 1,
                            rc.right - rc.left - ell_width - 1,
                            (ell_height) / 2 );
        }
        if  (ell_height < rc.bottom - rc.top)
        {
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + 1,
                            physDev->dc_rect.top + rc.top + (ell_height + 1) / 2,
                            rc.right - rc.left - 2,
                            rc.bottom - rc.top - ell_height - 1);
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
    if (X11DRV_SetupGCForPen( physDev ))
    {
        if (ell_width > (rc.right-rc.left) )
            if (ell_height > (rc.bottom-rc.top) )
                XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, rc.bottom - rc.top - 1, 0 , 360 * 64 );
            else{
                XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, ell_height - 1, 0 , 180 * 64 );
                XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left,
                          physDev->dc_rect.top + rc.bottom - ell_height,
                          rc.right - rc.left - 1, ell_height - 1, 180 * 64 , 180 * 64 );
            }
	else if (ell_height > (rc.bottom-rc.top) ){
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width - 1 , rc.bottom - rc.top - 1, 90 * 64 , 180 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width,
                      physDev->dc_rect.top + rc.top,
                      ell_width - 1 , rc.bottom - rc.top - 1, 270 * 64 , 180 * 64 );
	}else{
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width - 1, ell_height - 1, 90 * 64, 90 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.bottom - ell_height,
                      ell_width - 1, ell_height - 1, 180 * 64, 90 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width,
                      physDev->dc_rect.top + rc.bottom - ell_height,
                      ell_width - 1, ell_height - 1, 270 * 64, 90 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width, physDev->dc_rect.top + rc.top,
                      ell_width - 1, ell_height - 1, 0, 90 * 64 );
	}
	if (ell_width < rc.right - rc.left)
	{
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.left + ell_width / 2,
                       physDev->dc_rect.top + rc.top,
                       physDev->dc_rect.left + rc.right - (ell_width+1) / 2,
                       physDev->dc_rect.top + rc.top);
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.left + ell_width / 2 ,
                       physDev->dc_rect.top + rc.bottom - 1,
                       physDev->dc_rect.left + rc.right - (ell_width+1)/ 2,
                       physDev->dc_rect.top + rc.bottom - 1);
	}
	if (ell_height < rc.bottom - rc.top)
	{
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.right - 1,
                       physDev->dc_rect.top + rc.top + ell_height / 2,
                       physDev->dc_rect.left + rc.right - 1,
                       physDev->dc_rect.top + rc.bottom - (ell_height+1) / 2);
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.left,
                       physDev->dc_rect.top + rc.top + ell_height / 2,
                       physDev->dc_rect.left + rc.left,
                       physDev->dc_rect.top + rc.bottom - (ell_height+1) / 2);
	}
    }

    physDev->pen.width = oldwidth;
    physDev->pen.endcap = oldendcap;
    add_pen_device_bounds( physDev, (POINT *)&rc, 2 );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetPixel
 */
COLORREF X11DRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    unsigned long pixel;
    POINT pt;
    RECT rect;

    pt.x = x;
    pt.y = y;
    lp_to_dp( dev->hdc, &pt, 1 );
    pixel = X11DRV_PALETTE_ToPhysical( physDev, color );

    XSetForeground( gdi_display, physDev->gc, pixel );
    XSetFunction( gdi_display, physDev->gc, GXcopy );
    XDrawPoint( gdi_display, physDev->drawable, physDev->gc,
                physDev->dc_rect.left + pt.x, physDev->dc_rect.top + pt.y );

    SetRect( &rect, pt.x, pt.y, pt.x + 1, pt.y + 1 );
    add_device_bounds( physDev, &rect );
    return X11DRV_PALETTE_ToLogical(physDev, pixel);
}


/***********************************************************************
 *           X11DRV_PaintRgn
 */
BOOL X11DRV_PaintRgn( PHYSDEV dev, HRGN hrgn )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    RECT rc;

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        unsigned int i;
        XRectangle *rect;
        RGNDATA *data = X11DRV_GetRegionData( hrgn, dev->hdc );

        if (!data) return FALSE;
        rect = (XRectangle *)data->Buffer;
        for (i = 0; i < data->rdh.nCount; i++)
        {
            rect[i].x += physDev->dc_rect.left;
            rect[i].y += physDev->dc_rect.top;
        }

        XFillRectangles( gdi_display, physDev->drawable, physDev->gc, rect, data->rdh.nCount );
        free( data );
    }
    if (NtGdiGetRgnBox( hrgn, &rc ))
    {
        lp_to_dp( dev->hdc, (POINT *)&rc, 2 );
        add_device_bounds( physDev, &rc );
    }
    return TRUE;
}

/**********************************************************************
 *          X11DRV_Polygon
 */
static BOOL X11DRV_Polygon( PHYSDEV dev, const POINT* pt, INT count )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    int i;
    POINT *points;
    XPoint *xpoints;

    points = malloc( count * sizeof(*pt) );
    if (!points) return FALSE;
    memcpy( points, pt, count * sizeof(*pt) );
    lp_to_dp( dev->hdc, points, count );
    add_pen_device_bounds( physDev, points, count );

    if (!(xpoints = malloc( sizeof(XPoint) * (count+1) )))
    {
        free( points );
        return FALSE;
    }
    for (i = 0; i < count; i++)
    {
        xpoints[i].x = physDev->dc_rect.left + points[i].x;
        xpoints[i].y = physDev->dc_rect.top + points[i].y;
    }
    xpoints[count] = xpoints[0];

    if (X11DRV_SetupGCForBrush( physDev ))
        XFillPolygon( gdi_display, physDev->drawable, physDev->gc,
                      xpoints, count+1, Complex, CoordModeOrigin);

    if (X11DRV_SetupGCForPen ( physDev ))
        XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                    xpoints, count+1, CoordModeOrigin );

    free( xpoints );
    free( points );
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolygon
 */
BOOL X11DRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    DWORD total = 0, max = 0, pos, i;
    POINT *points;
    BOOL ret = FALSE;

    if (polygons == 1) return X11DRV_Polygon( dev, pt, *counts );

    for (i = 0; i < polygons; i++)
    {
        if (counts[i] < 2) return FALSE;
        if (counts[i] > max) max = counts[i];
        total += counts[i];
    }

    points = malloc( total * sizeof(*pt) );
    if (!points) return FALSE;
    memcpy( points, pt, total * sizeof(*pt) );
    lp_to_dp( dev->hdc, points, total );
    add_pen_device_bounds( physDev, points, total );

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        DWORD poly_fill_mode;
        XRectangle *rect;
        HRGN hrgn;
        RGNDATA *data;

        NtGdiGetDCDword( dev->hdc, NtGdiGetPolyFillMode, &poly_fill_mode );
        hrgn = UlongToHandle( NtGdiPolyPolyDraw( UlongToHandle(poly_fill_mode), points,
                                                 (const ULONG *)counts, polygons,
                                                 NtGdiPolyPolygonRgn ));
        data = X11DRV_GetRegionData( hrgn, 0 );
        NtGdiDeleteObjectApp( hrgn );
        if (!data) goto done;
        rect = (XRectangle *)data->Buffer;
        for (i = 0; i < data->rdh.nCount; i++)
        {
            rect[i].x += physDev->dc_rect.left;
            rect[i].y += physDev->dc_rect.top;
        }

        XFillRectangles( gdi_display, physDev->drawable, physDev->gc, rect, data->rdh.nCount );
        free( data );
    }

    if (X11DRV_SetupGCForPen ( physDev ))
    {
        XPoint *xpoints;
        int j;

        if (!(xpoints = malloc( sizeof(XPoint) * (max + 1) ))) goto done;
        for (i = pos = 0; i < polygons; pos += counts[i++])
        {
            for (j = 0; j < counts[i]; j++)
            {
                xpoints[j].x = physDev->dc_rect.left + points[pos + j].x;
                xpoints[j].y = physDev->dc_rect.top + points[pos + j].y;
            }
	    xpoints[j] = xpoints[0];
            XDrawLines( gdi_display, physDev->drawable, physDev->gc, xpoints, j + 1, CoordModeOrigin );
        }
        free( xpoints );
    }
    ret = TRUE;

done:
    free( points );
    return ret;
}


/**********************************************************************
 *          X11DRV_PolyPolyline
 */
BOOL X11DRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    DWORD total = 0, max = 0, pos, i, j;
    POINT *points;

    for (i = 0; i < polylines; i++)
    {
        if (counts[i] < 2) return FALSE;
        if (counts[i] > max) max = counts[i];
        total += counts[i];
    }

    points = malloc( total * sizeof(*pt) );
    if (!points) return FALSE;
    memcpy( points, pt, total * sizeof(*pt) );
    lp_to_dp( dev->hdc, points, total );
    add_pen_device_bounds( physDev, points, total );

    if (X11DRV_SetupGCForPen ( physDev ))
    {
        XPoint *xpoints;

        if (!(xpoints = malloc( sizeof(XPoint) * max )))
        {
            free( points );
            return FALSE;
        }
        for (i = pos = 0; i < polylines; pos += counts[i++])
        {
            for (j = 0; j < counts[i]; j++)
            {
                xpoints[j].x = physDev->dc_rect.left + points[pos + j].x;
                xpoints[j].y = physDev->dc_rect.top + points[pos + j].y;
            }
            XDrawLines( gdi_display, physDev->drawable, physDev->gc, xpoints, j, CoordModeOrigin );
        }
        free( xpoints );
    }
    free( points );
    return TRUE;
}

/* helper for path stroking and filling functions */
static BOOL x11drv_stroke_and_fill_path( PHYSDEV dev, BOOL stroke, BOOL fill )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    POINT *points;
    BYTE *flags;
    BOOL ret = FALSE;
    XPoint *xpoints;
    int i, j, size;

    NtGdiFlattenPath( dev->hdc );
    if ((size = NtGdiGetPath( dev->hdc, NULL, NULL, 0 )) == -1) return FALSE;
    if (!size)
    {
        NtGdiAbortPath( dev->hdc );
        return TRUE;
    }
    xpoints = malloc( (size + 1) * sizeof(*xpoints) );
    points = malloc( size * sizeof(*points) );
    flags = malloc( size * sizeof(*flags) );
    if (!points || !flags || !xpoints) goto done;
    if (NtGdiGetPath( dev->hdc, points, flags, size ) == -1) goto done;
    lp_to_dp( dev->hdc, points, size );

    if (fill && X11DRV_SetupGCForBrush( physDev ))
    {
        XRectangle *rect;
        HRGN hrgn = NtGdiPathToRegion( dev->hdc );
        RGNDATA *data = X11DRV_GetRegionData( hrgn, 0 );

        NtGdiDeleteObjectApp( hrgn );
        if (!data) goto done;
        rect = (XRectangle *)data->Buffer;
        for (i = 0; i < data->rdh.nCount; i++)
        {
            rect[i].x += physDev->dc_rect.left;
            rect[i].y += physDev->dc_rect.top;
        }

        XFillRectangles( gdi_display, physDev->drawable, physDev->gc, rect, data->rdh.nCount );
        free( data );
    }

    if (stroke && X11DRV_SetupGCForPen ( physDev ))
    {
        for (i = j = 0; i < size; i++, j++)
        {
            if (flags[i] == PT_MOVETO)
            {
                if (j > 1)
                {
                    if (fill || (flags[i - 1] & PT_CLOSEFIGURE)) xpoints[j++] = xpoints[0];
                    XDrawLines( gdi_display, physDev->drawable, physDev->gc, xpoints, j, CoordModeOrigin );
                }
                j = 0;
            }
            xpoints[j].x = physDev->dc_rect.left + points[i].x;
            xpoints[j].y = physDev->dc_rect.top + points[i].y;
        }
        if (j > 1)
        {
            if (fill || (flags[i - 1] & PT_CLOSEFIGURE)) xpoints[j++] = xpoints[0];
            XDrawLines( gdi_display, physDev->drawable, physDev->gc, xpoints, j, CoordModeOrigin );
        }
    }

    add_pen_device_bounds( physDev, points, size );
    NtGdiAbortPath( dev->hdc );
    ret = TRUE;

done:
    free( xpoints );
    free( points );
    free( flags );
    return ret;
}

/**********************************************************************
 *          X11DRV_FillPath
 */
BOOL X11DRV_FillPath( PHYSDEV dev )
{
    return x11drv_stroke_and_fill_path( dev, FALSE, TRUE );
}

/**********************************************************************
 *          X11DRV_StrokeAndFillPath
 */
BOOL X11DRV_StrokeAndFillPath( PHYSDEV dev )
{
    return x11drv_stroke_and_fill_path( dev, TRUE, TRUE );
}

/**********************************************************************
 *          X11DRV_StrokePath
 */
BOOL X11DRV_StrokePath( PHYSDEV dev )
{
    return x11drv_stroke_and_fill_path( dev, TRUE, FALSE );
}


/**********************************************************************
 *          X11DRV_InternalFloodFill
 *
 * Internal helper function for flood fill.
 * (xorg,yorg) is the origin of the X image relative to the drawable.
 * (x,y) is relative to the origin of the X image.
 */
static void X11DRV_InternalFloodFill(XImage *image, X11DRV_PDEVICE *physDev,
                                     int x, int y,
                                     int xOrg, int yOrg,
                                     unsigned long pixel, WORD fillType, RECT *bounds )
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
    bounds->left   = min( bounds->left, left );
    bounds->top    = min( bounds->top, y );
    bounds->right  = max( bounds->right, right );
    bounds->bottom = max( bounds->bottom, y + 1 );
    XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
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
            X11DRV_InternalFloodFill(image, physDev, x-1, y,
                                     xOrg, yOrg, pixel, fillType, bounds );
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
            X11DRV_InternalFloodFill(image, physDev, x-1, y,
                                     xOrg, yOrg, pixel, fillType, bounds );
        }
    }
#undef TO_FLOOD
}


static int ExtFloodFillXGetImageErrorHandler( Display *dpy, XErrorEvent *event, void *arg )
{
    return (event->request_code == X_GetImage && event->error_code == BadMatch);
}

/**********************************************************************
 *          X11DRV_ExtFloodFill
 */
BOOL X11DRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    XImage *image;
    RECT rect, bounds;
    POINT pt;

    TRACE("X11DRV_ExtFloodFill %d,%d %s %d\n", x, y, debugstr_color(color), fillType );

    pt.x = x;
    pt.y = y;
    lp_to_dp( dev->hdc, &pt, 1 );

    if (!physDev->region)
    {
        rect.left   = 0;
        rect.top    = 0;
        rect.right  = physDev->dc_rect.right - physDev->dc_rect.left;
        rect.bottom = physDev->dc_rect.bottom - physDev->dc_rect.top;
    }
    else
    {
        if (!NtGdiPtInRegion( physDev->region, pt.x, pt.y )) return FALSE;
        NtGdiGetRgnBox( physDev->region, &rect );
        rect.left   = max( rect.left, 0 );
        rect.top    = max( rect.top, 0 );
        rect.right  = min( rect.right, physDev->dc_rect.right - physDev->dc_rect.left );
        rect.bottom = min( rect.bottom, physDev->dc_rect.bottom - physDev->dc_rect.top );
    }
    if (pt.x < rect.left || pt.x >= rect.right || pt.y < rect.top || pt.y >= rect.bottom) return FALSE;

    X11DRV_expect_error( gdi_display, ExtFloodFillXGetImageErrorHandler, NULL );
    image = XGetImage( gdi_display, physDev->drawable,
                       physDev->dc_rect.left + rect.left, physDev->dc_rect.top + rect.top,
                       rect.right - rect.left, rect.bottom - rect.top,
                       AllPlanes, ZPixmap );
    if(X11DRV_check_error()) image = NULL;
    if (!image) return FALSE;

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        unsigned long pixel = X11DRV_PALETTE_ToPhysical( physDev, color );

        reset_bounds( &bounds );

        X11DRV_InternalFloodFill(image, physDev,
                                 pt.x - rect.left,
                                 pt.y - rect.top,
                                 physDev->dc_rect.left + rect.left,
                                 physDev->dc_rect.top + rect.top,
                                 pixel, fillType, &bounds );

        OffsetRect( &bounds, rect.left, rect.top );
        add_device_bounds( physDev, &bounds );
    }

    XDestroyImage( image );
    return TRUE;
}

/**********************************************************************
 *          X11DRV_GradientFill
 */
BOOL X11DRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                          void *grad_array, ULONG ngrad, ULONG mode )
{
    X11DRV_PDEVICE *physdev = get_x11drv_dev( dev );
    const GRADIENT_RECT *rect = grad_array;
    TRIVERTEX v[2];
    POINT pt[2];
    RECT rc, bounds;
    unsigned int i;
    XGCValues val;

    /* <= 16-bpp use dithering */
    if (physdev->depth <= 16) goto fallback;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        val.function   = GXcopy;
        val.fill_style = FillSolid;
        val.line_width = 1;
        val.cap_style  = CapNotLast;
        val.line_style = LineSolid;
        XChangeGC( gdi_display, physdev->gc,
                   GCFunction | GCLineWidth | GCLineStyle | GCCapStyle | GCFillStyle, &val );
        reset_bounds( &bounds );

        for (i = 0; i < ngrad; i++, rect++)
        {
            int x, dx;

            v[0] = vert_array[rect->UpperLeft];
            v[1] = vert_array[rect->LowerRight];
            pt[0].x = v[0].x;
            pt[0].y = v[0].y;
            pt[1].x = v[1].x;
            pt[1].y = v[1].y;
            lp_to_dp( dev->hdc, pt, 2 );
            dx = pt[1].x - pt[0].x;
            if (!dx) continue;
            if (dx < 0)  /* swap the colors */
            {
                v[0] = vert_array[rect->LowerRight];
                v[1] = vert_array[rect->UpperLeft];
                dx = -dx;
            }
            rc.left   = min( pt[0].x, pt[1].x );
            rc.top    = min( pt[0].y, pt[1].y );
            rc.right  = max( pt[0].x, pt[1].x );
            rc.bottom = max( pt[0].y, pt[1].y );
            add_bounds_rect( &bounds, &rc );
            for (x = 0; x < dx; x++)
            {
                int color = X11DRV_PALETTE_ToPhysical( physdev,
                                 RGB( (v[0].Red   * (dx - x) + v[1].Red   * x) / dx / 256,
                                      (v[0].Green * (dx - x) + v[1].Green * x) / dx / 256,
                                      (v[0].Blue  * (dx - x) + v[1].Blue  * x) / dx / 256) );

                XSetForeground( gdi_display, physdev->gc, color );
                XDrawLine( gdi_display, physdev->drawable, physdev->gc,
                           physdev->dc_rect.left + rc.left + x, physdev->dc_rect.top + rc.top,
                           physdev->dc_rect.left + rc.left + x, physdev->dc_rect.top + rc.bottom );
            }
        }
        add_device_bounds( physdev, &bounds );
        return TRUE;

    case GRADIENT_FILL_RECT_V:
        val.function   = GXcopy;
        val.fill_style = FillSolid;
        val.line_width = 1;
        val.cap_style  = CapNotLast;
        val.line_style = LineSolid;
        XChangeGC( gdi_display, physdev->gc,
                   GCFunction | GCLineWidth | GCLineStyle | GCCapStyle | GCFillStyle, &val );
        reset_bounds( &bounds );

        for (i = 0; i < ngrad; i++, rect++)
        {
            int y, dy;

            v[0] = vert_array[rect->UpperLeft];
            v[1] = vert_array[rect->LowerRight];
            pt[0].x = v[0].x;
            pt[0].y = v[0].y;
            pt[1].x = v[1].x;
            pt[1].y = v[1].y;
            lp_to_dp( dev->hdc, pt, 2 );
            dy = pt[1].y - pt[0].y;
            if (!dy) continue;
            if (dy < 0)  /* swap the colors */
            {
                v[0] = vert_array[rect->LowerRight];
                v[1] = vert_array[rect->UpperLeft];
                dy = -dy;
            }
            rc.left   = min( pt[0].x, pt[1].x );
            rc.top    = min( pt[0].y, pt[1].y );
            rc.right  = max( pt[0].x, pt[1].x );
            rc.bottom = max( pt[0].y, pt[1].y );
            add_bounds_rect( &bounds, &rc );
            for (y = 0; y < dy; y++)
            {
                int color = X11DRV_PALETTE_ToPhysical( physdev,
                                 RGB( (v[0].Red   * (dy - y) + v[1].Red   * y) / dy / 256,
                                      (v[0].Green * (dy - y) + v[1].Green * y) / dy / 256,
                                      (v[0].Blue  * (dy - y) + v[1].Blue  * y) / dy / 256) );

                XSetForeground( gdi_display, physdev->gc, color );
                XDrawLine( gdi_display, physdev->drawable, physdev->gc,
                           physdev->dc_rect.left + rc.left, physdev->dc_rect.top + rc.top + y,
                           physdev->dc_rect.left + rc.right, physdev->dc_rect.top + rc.top + y );
            }
        }
        add_device_bounds( physdev, &bounds );
        return TRUE;
    }

fallback:
    dev = GET_NEXT_PHYSDEV( dev, pGradientFill );
    return dev->funcs->pGradientFill( dev, vert_array, nvert, grad_array, ngrad, mode );
}

static char *get_icm_profile( unsigned long *size )
{
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *profile;
    char *ret = NULL;

    XGetWindowProperty( gdi_display, DefaultRootWindow(gdi_display),
                        x11drv_atom(_ICC_PROFILE), 0, ~0UL, False, AnyPropertyType,
                        &type, &format, &count, &remaining, &profile );
    *size = get_property_size( format, count );
    if (format && count)
    {
        if ((ret = malloc( *size ))) memcpy( ret, profile, *size );
        XFree( profile );
    }
    return ret;
}

static const WCHAR mntr_key[] =
    {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\',
     'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
     'W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t',
     'V','e','r','s','i','o','n','\\','I','C','M','\\','m','n','t','r'};

static const WCHAR color_path[] =
    {'\\','?','?','\\','c',':','\\','w','i','n','d','o','w','s','\\','s','y','s','t','e','m','3','2',
     '\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\','c','o','l','o','r','\\'};

/***********************************************************************
 *              GetICMProfile (X11DRV.@)
 */
BOOL X11DRV_GetICMProfile( PHYSDEV dev, BOOL allow_default, LPDWORD size, LPWSTR filename )
{
    static const WCHAR srgb[] =
        {'s','R','G','B',' ','C','o','l','o','r',' ','S','p','a','c','e',' ',
         'P','r','o','f','i','l','e','.','i','c','m',0};
    HKEY hkey;
    DWORD required;
    char buf[4096];
    KEY_VALUE_FULL_INFORMATION *info = (void *)buf;
    char *buffer;
    unsigned long buflen, i;
    ULONG full_size;
    WCHAR fullname[MAX_PATH + ARRAY_SIZE( color_path )], *p;
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES attr;

    if (!size) return FALSE;

    memcpy( fullname, color_path, sizeof(color_path) );
    p = fullname + ARRAYSIZE(color_path);

    hkey = reg_open_key( NULL, mntr_key, sizeof(mntr_key) );

    if (hkey && !NtEnumerateValueKey( hkey, 0, KeyValueFullInformation,
                                      info, sizeof(buf), &full_size ))
    {
        /* FIXME handle multiple values */
        memcpy( p, info->Name, info->NameLength );
        p[info->NameLength / sizeof(WCHAR)] = 0;
    }
    else if ((buffer = get_icm_profile( &buflen )))
    {
        static const WCHAR icm[] = {'.','i','c','m',0};
        IO_STATUS_BLOCK io = {{0}};
        UINT64 hash = 0;
        HANDLE file;
        int status;

        for (i = 0; i < buflen; i++) hash = (hash << 16) - hash + buffer[i];
        for (i = 0; i < sizeof(hash) * 2; i++)
        {
            int digit = hash & 0xf;
            p[i] = digit < 10 ? '0' + digit : 'a' - 10 + digit;
            hash >>= 4;
        }

        memcpy( p + i, icm, sizeof(icm) );

        RtlInitUnicodeString( &name, fullname );
        InitializeObjectAttributes( &attr, &name, OBJ_CASE_INSENSITIVE, NULL, NULL );
        status = NtCreateFile( &file, GENERIC_WRITE, &attr, &io, NULL, 0, 0, FILE_CREATE,
                               FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL, 0 );
        if (!status)
        {
            status = NtWriteFile( file, NULL, NULL, NULL, &io, buffer, buflen, NULL, NULL );
            if (status) ERR( "Unable to write color profile: %x\n", status );
            NtClose( file );
        }
        free( buffer );
    }
    else if (!allow_default) return FALSE;
    else lstrcpyW( p, srgb );

    NtClose( hkey );
    required = wcslen( fullname ) + 1 - 4 /* skip NT prefix */;
    if (*size < required)
    {
        *size = required;
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (filename)
    {
        FILE_BASIC_INFORMATION info;
        wcscpy( filename, fullname + 4 );
        RtlInitUnicodeString( &name, fullname );
        InitializeObjectAttributes( &attr, &name, OBJ_CASE_INSENSITIVE, NULL, NULL );
        if (NtQueryAttributesFile( &attr, &info ))
            WARN( "color profile not found in %s\n", debugstr_w(fullname) );
    }
    *size = required;
    return TRUE;
}

/*
 * GDI graphics operations
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <math.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#ifndef PI
#define PI M_PI
#endif
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
#include "debug.h"
#include "xmalloc.h"

/***********************************************************************
 *           LineTo16    (GDI.19)
 */
BOOL16 LineTo16( HDC16 hdc, INT16 x, INT16 y )
{
    return LineTo32( hdc, x, y );
}


/***********************************************************************
 *           LineTo32    (GDI32.249)
 */
BOOL32 LineTo32( HDC32 hdc, INT32 x, INT32 y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_LINETO, x, y);
	return TRUE;
    }

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
 *           MoveTo    (GDI.20)
 */
DWORD MoveTo( HDC16 hdc, INT16 x, INT16 y )
{
    DWORD ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_MOVETO, x, y);
	return 0;
    }
    ret = MAKELONG( dc->w.CursPosX, dc->w.CursPosY );
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return ret;
}


/***********************************************************************
 *           MoveToEx16    (GDI.483)
 */
BOOL16 MoveToEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
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
 *           MoveToEx32    (GDI32.254)
 */
BOOL32 MoveToEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    POINT16 pt16;
    if (!MoveToEx16( (HDC16)hdc, (INT16)x, (INT16)y, &pt16 )) return FALSE;
    if (pt) CONV_POINT16TO32( &pt16, pt );
    return TRUE;
}


/***********************************************************************
 *           GRAPH_DrawArc
 *
 * Helper functions for Arc(), Chord() and Pie().
 * 'lines' is the number of lines to draw: 0 for Arc, 1 for Chord, 2 for Pie.
 */
static BOOL32 GRAPH_DrawArc( HDC32 hdc, INT32 left, INT32 top, INT32 right,
                             INT32 bottom, INT32 xstart, INT32 ystart,
                             INT32 xend, INT32 yend, INT32 lines )
{
    INT32 xcenter, ycenter, istart_angle, idiff_angle, tmp;
    double start_angle, end_angle;
    XPoint points[3];
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	switch (lines)
	{
	case 0:
	    MF_MetaParam8(dc, META_ARC, left, top, right, bottom,
			  xstart, ystart, xend, yend);
	    break;

	case 1:
	    MF_MetaParam8(dc, META_CHORD, left, top, right, bottom,
			  xstart, ystart, xend, yend);
	    break;

	case 2:
	    MF_MetaParam8(dc, META_PIE, left, top, right, bottom,
			  xstart, ystart, xend, yend);
	    break;
	}
	return 0;
    }

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
 *           Arc16    (GDI.23)
 */
BOOL16 Arc16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom,
              INT16 xstart, INT16 ystart, INT16 xend, INT16 yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           Arc32    (GDI32.7)
 */
BOOL32 Arc32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom,
              INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           Pie16    (GDI.26)
 */
BOOL16 Pie16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom,
              INT16 xstart, INT16 ystart, INT16 xend, INT16 yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 2 );
}


/***********************************************************************
 *           Pie32   (GDI32.262)
 */
BOOL32 Pie32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom,
              INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 2 );
}


/***********************************************************************
 *           Chord16    (GDI.348)
 */
BOOL16 Chord16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom,
                INT16 xstart, INT16 ystart, INT16 xend, INT16 yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           Chord32    (GDI32.14)
 */
BOOL32 Chord32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom,
                INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           Ellipse16    (GDI.24)
 */
BOOL16 Ellipse16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    return Ellipse32( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Ellipse32    (GDI32.75)
 */
BOOL32 Ellipse32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam4(dc, META_ELLIPSE, left, top, right, bottom);
	return 0;
    }

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
 *           Rectangle16    (GDI.27)
 */
BOOL16 Rectangle16(HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom)
{
    return Rectangle32( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Rectangle32    (GDI32.283)
 */
BOOL32 Rectangle32(HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    INT32 width;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam4(dc, META_RECTANGLE, left, top, right, bottom);
	return TRUE;
    }
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
 *           RoundRect16    (GDI.28)
 */
BOOL16 RoundRect16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                    INT16 bottom, INT16 ell_width, INT16 ell_height )
{
    return RoundRect32( hdc, left, top, right, bottom, ell_width, ell_height );
}


/***********************************************************************
 *           RoundRect32    (GDI32.291)
 */
BOOL32 RoundRect32( HDC32 hdc, INT32 left, INT32 top, INT32 right,
                    INT32 bottom, INT32 ell_width, INT32 ell_height )
{
    DC * dc = (DC *) GDI_GetObjPtr(hdc, DC_MAGIC);
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam6(dc, META_ROUNDRECT, left, top, right, bottom,
		      ell_width, ell_height);
	return TRUE;
    }
    dprintf_graphics(stddeb, "RoundRect(%d %d %d %d  %d %d\n", 
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
 *           FillRect16    (USER.81)
 */
INT16 FillRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH16 prevBrush;

    /* coordinates are logical so we cannot fast-check rectangle
     * - do it in PatBlt() after LPtoDP().
     */

    if (!(prevBrush = SelectObject16( hdc, hbrush ))) return 0;
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject16( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           FillRect32    (USER32.196)
 */
INT32 FillRect32( HDC32 hdc, const RECT32 *rect, HBRUSH32 hbrush )
{
    HBRUSH32 prevBrush;

    if (!(prevBrush = SelectObject32( hdc, hbrush ))) return 0;
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject32( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           InvertRect16    (USER.82)
 */
void InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *           InvertRect32    (USER32.329)
 */
void InvertRect32( HDC32 hdc, const RECT32 *rect )
{
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *           FrameRect16    (USER.83)
 */
INT16 FrameRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH16 prevBrush;
    int left, top, right, bottom;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    left   = XLPTODP( dc, rect->left );
    top    = YLPTODP( dc, rect->top );
    right  = XLPTODP( dc, rect->right );
    bottom = YLPTODP( dc, rect->bottom );

    if ( (right <= left) || (bottom <= top) ) return 0;
    if (!(prevBrush = SelectObject16( hdc, hbrush ))) return 0;
    
    if (DC_SetupGCForBrush( dc ))
    {
   	PatBlt32( hdc, rect->left, rect->top, 1,
                  rect->bottom - rect->top, PATCOPY );
	PatBlt32( hdc, rect->right - 1, rect->top, 1,
                  rect->bottom - rect->top, PATCOPY );
	PatBlt32( hdc, rect->left, rect->top,
                  rect->right - rect->left, 1, PATCOPY );
	PatBlt32( hdc, rect->left, rect->bottom - 1,
                  rect->right - rect->left, 1, PATCOPY );
    }
    SelectObject16( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           FrameRect32    (USER32.202)
 */
INT32 FrameRect32( HDC32 hdc, const RECT32 *rect, HBRUSH32 hbrush )
{
    RECT16 rect16;
    CONV_RECT32TO16( rect, &rect16 );
    return FrameRect16( (HDC16)hdc, &rect16, (HBRUSH16)hbrush );
}


/***********************************************************************
 *           SetPixel16    (GDI.31)
 */
COLORREF SetPixel16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return SetPixel32( hdc, x, y, color );
}


/***********************************************************************
 *           SetPixel32    (GDI32.327)
 */
COLORREF SetPixel32( HDC32 hdc, INT32 x, INT32 y, COLORREF color )
{
    Pixel pixel;
    
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
	MF_MetaParam4(dc, META_SETPIXEL, x, y, HIWORD(color), LOWORD(color)); 
	return 1;
    }

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
 *           GetPixel16    (GDI.83)
 */
COLORREF GetPixel16( HDC16 hdc, INT16 x, INT16 y )
{
    return GetPixel32( hdc, x, y );
}


/***********************************************************************
 *           GetPixel32    (GDI32.211)
 */
COLORREF GetPixel32( HDC32 hdc, INT32 x, INT32 y )
{
    static Pixmap pixmap = 0;
    XImage * image;
    int pixel;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

#ifdef SOLITAIRE_SPEED_HACK
    return 0;
#endif

    if (!PtVisible32( hdc, x, y )) return 0;

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
 *           PaintRgn16    (GDI.43)
 */
BOOL16 PaintRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return PaintRgn32( hdc, hrgn );
}


/***********************************************************************
 *           PaintRgn32    (GDI32.259)
 */
BOOL32 PaintRgn32( HDC32 hdc, HRGN32 hrgn )
{
    RECT32 box;
    HRGN32 tmpVisRgn, prevVisRgn;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

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


/***********************************************************************
 *           FillRgn16    (GDI.40)
 */
BOOL16 FillRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush )
{
    return FillRgn32( hdc, hrgn, hbrush );
}

    
/***********************************************************************
 *           FillRgn32    (GDI32.101)
 */
BOOL32 FillRgn32( HDC32 hdc, HRGN32 hrgn, HBRUSH32 hbrush )
{
    BOOL32 retval;
    HBRUSH32 prevBrush = SelectObject32( hdc, hbrush );
    if (!prevBrush) return FALSE;
    retval = PaintRgn32( hdc, hrgn );
    SelectObject32( hdc, prevBrush );
    return retval;
}


/***********************************************************************
 *           FrameRgn16     (GDI.41)
 */
BOOL16 FrameRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush,
                   INT16 nWidth, INT16 nHeight )
{
    return FrameRgn32( hdc, hrgn, hbrush, nWidth, nHeight );
}


/***********************************************************************
 *           FrameRgn32     (GDI32.105)
 */
BOOL32 FrameRgn32( HDC32 hdc, HRGN32 hrgn, HBRUSH32 hbrush,
                   INT32 nWidth, INT32 nHeight )
{
    HRGN32 tmp = CreateRectRgn32( 0, 0, 0, 0 );
    if(!REGION_FrameRgn( tmp, hrgn, nWidth, nHeight )) return FALSE;
    FillRgn32( hdc, tmp, hbrush );
    DeleteObject32( tmp );
    return TRUE;
}


/***********************************************************************
 *           InvertRgn16    (GDI.42)
 */
BOOL16 InvertRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return InvertRgn32( hdc, hrgn );
}


/***********************************************************************
 *           InvertRgn32    (GDI32.246)
 */
BOOL32 InvertRgn32( HDC32 hdc, HRGN32 hrgn )
{
    HBRUSH32 prevBrush = SelectObject32( hdc, GetStockObject32(BLACK_BRUSH) );
    INT32 prevROP = SetROP232( hdc, R2_NOT );
    BOOL32 retval = PaintRgn32( hdc, hrgn );
    SelectObject32( hdc, prevBrush );
    SetROP232( hdc, prevROP );
    return retval;
}


/***********************************************************************
 *           DrawFocusRect16    (USER.466)
 */
void DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT32 rect32;
    CONV_RECT16TO32( rc, &rect32 );
    DrawFocusRect32( hdc, &rect32 );
}


/***********************************************************************
 *           DrawFocusRect32    (USER32.155)
 */
void DrawFocusRect32( HDC32 hdc, const RECT32* rc )
{
    HPEN32 hOldPen;
    INT32 oldDrawMode, oldBkMode;
    INT32 left, top, right, bottom;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return;

    left   = XLPTODP( dc, rc->left );
    top    = YLPTODP( dc, rc->top );
    right  = XLPTODP( dc, rc->right );
    bottom = YLPTODP( dc, rc->bottom );
    
    hOldPen = SelectObject32( hdc, sysColorObjects.hpenWindowText );
    oldDrawMode = SetROP232(hdc, R2_XORPEN);
    oldBkMode = SetBkMode32(hdc, TRANSPARENT);

    /* Hack: make sure the XORPEN operation has an effect */
    dc->u.x.pen.pixel = (1 << screenDepth) - 1;

    if (DC_SetupGCForPen( dc ))
	XDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );

    SetBkMode32(hdc, oldBkMode);
    SetROP232(hdc, oldDrawMode);
    SelectObject32(hdc, hOldPen);
}


/**********************************************************************
 *          GRAPH_DrawBitmap
 *
 * Short-cut function to blit a bitmap into a device.
 * Faster than CreateCompatibleDC() + SelectBitmap() + BitBlt() + DeleteDC().
 */
BOOL32 GRAPH_DrawBitmap( HDC32 hdc, HBITMAP32 hbitmap, int xdest, int ydest,
                         int xsrc, int ysrc, int width, int height )
{
    BITMAPOBJ *bmp;
    DC *dc;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return FALSE;
    if (!(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return FALSE;
    XSetFunction( display, dc->u.x.gc, GXcopy );
    if (bmp->bitmap.bmBitsPixel == 1)
    {
        XSetForeground( display, dc->u.x.gc, dc->w.backgroundPixel );
        XSetBackground( display, dc->u.x.gc, dc->w.textPixel );
	XCopyPlane( display, bmp->pixmap, dc->u.x.drawable, dc->u.x.gc,
		    xsrc, ysrc, width, height,
		    dc->w.DCOrgX + xdest, dc->w.DCOrgY + ydest, 1 );
	return TRUE;
    }
    else if (bmp->bitmap.bmBitsPixel == dc->w.bitsPerPixel)
    {
	XCopyArea( display, bmp->pixmap, dc->u.x.drawable, dc->u.x.gc,
		   xsrc, ysrc, width, height,
		   dc->w.DCOrgX + xdest, dc->w.DCOrgY + ydest );
	return TRUE;
    }
    else return FALSE;
}


/**********************************************************************
 *          GRAPH_DrawReliefRect  (Not a MSWin Call)
 */
void GRAPH_DrawReliefRect( HDC32 hdc, const RECT32 *rect, INT32 highlight_size,
                           INT32 shadow_size, BOOL32 pressed )
{
    HBRUSH32 hbrushOld;
    INT32 i;

    hbrushOld = SelectObject32(hdc, pressed ? sysColorObjects.hbrushBtnShadow :
			                  sysColorObjects.hbrushBtnHighlight );
    for (i = 0; i < highlight_size; i++)
    {
	PatBlt32( hdc, rect->left + i, rect->top,
                  1, rect->bottom - rect->top - i, PATCOPY );
	PatBlt32( hdc, rect->left, rect->top + i,
                  rect->right - rect->left - i, 1, PATCOPY );
    }

    SelectObject32( hdc, pressed ? sysColorObjects.hbrushBtnHighlight :
		                   sysColorObjects.hbrushBtnShadow );
    for (i = 0; i < shadow_size; i++)
    {
	PatBlt32( hdc, rect->right - i - 1, rect->top + i,
                  1, rect->bottom - rect->top - i, PATCOPY );
	PatBlt32( hdc, rect->left + i, rect->bottom - i - 1,
                  rect->right - rect->left - i, 1, PATCOPY );
    }

    SelectObject32( hdc, hbrushOld );
}


/**********************************************************************
 *          Polyline16  (GDI.37)
 */
BOOL16 Polyline16( HDC16 hdc, LPPOINT16 pt, INT16 count )
{
    register int i;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaPoly(dc, META_POLYLINE, pt, count); 
	return TRUE;
    }

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
 *          Polyline32   (GDI32.276)
 */
BOOL32 Polyline32( HDC32 hdc, const LPPOINT32 pt, INT32 count )
{
    register int i;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
        fprintf( stderr, "Polyline32: Metafile Polyline not yet supported for Win32\n");
/* win 16 code was:
	MF_MetaPoly(dc, META_POLYLINE, pt, count); 
	return TRUE;
*/
	return FALSE;
    }

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
 *          Polygon16  (GDI.36)
 */
BOOL16 Polygon16( HDC16 hdc, LPPOINT16 pt, INT16 count )
{
    register int i;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    XPoint *points;

    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaPoly(dc, META_POLYGON, pt, count); 
	return TRUE;
    }

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
 *          Polygon32  (GDI32.275)
 *
 * This a copy of Polygon16 so that conversion of array of
 * LPPOINT32 to LPPOINT16 is not necessary
 *
 */
BOOL32 Polygon32( HDC32 hdc, LPPOINT32 pt, INT32 count )
{
    register int i;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    XPoint *points;

    if (!dc)
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	/* FIXME: MF_MetaPoly expects LPPOINT16 not 32 */
	/* MF_MetaPoly(dc, META_POLYGON, pt, count); */
	return TRUE;
    }

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
 *          PolyPolygon16  (GDI.450)
 */
BOOL16 PolyPolygon16( HDC16 hdc, LPPOINT16 pt, LPINT16 counts, UINT16 polygons)
{
    HRGN32 hrgn;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	/* MF_MetaPoly(dc, META_POLYGON, pt, count); */
	return TRUE;
    }
      /* FIXME: The points should be converted to device coords before */
      /* creating the region. But as CreatePolyPolygonRgn is not */
      /* really correct either, it doesn't matter much... */
      /* At least the outline will be correct :-) */
    hrgn = CreatePolyPolygonRgn16( pt, counts, polygons, dc->w.polyFillMode );
    PaintRgn32( hdc, hrgn );
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
 *          GRAPH_InternalFloodFill
 *
 * Internal helper function for flood fill.
 * (xorg,yorg) is the origin of the X image relative to the drawable.
 * (x,y) is relative to the origin of the X image.
 */
static void GRAPH_InternalFloodFill( XImage *image, DC *dc,
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
            GRAPH_InternalFloodFill( image, dc, x-1, y,
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
            GRAPH_InternalFloodFill( image, dc, x-1, y,
                                     xOrg, yOrg, pixel, fillType );
        }
    }
#undef TO_FLOOD    
}


/**********************************************************************
 *          GRAPH_DoFloodFill
 *
 * Main flood-fill routine.
 */
static BOOL32 GRAPH_DoFloodFill( DC *dc, RECT32 *rect, INT32 x, INT32 y,
                                 COLORREF color, UINT32 fillType )
{
    XImage *image;

    if (!(image = XGetImage( display, dc->u.x.drawable,
                             dc->w.DCOrgX + rect->left,
                             dc->w.DCOrgY + rect->top,
                             rect->right - rect->left,
                             rect->bottom - rect->top,
                             AllPlanes, ZPixmap ))) return FALSE;

    if (DC_SetupGCForBrush( dc ))
    {
          /* ROP mode is always GXcopy for flood-fill */
        XSetFunction( display, dc->u.x.gc, GXcopy );
        GRAPH_InternalFloodFill( image, dc,
                                 XLPTODP(dc,x) - rect->left,
                                 YLPTODP(dc,y) - rect->top,
                                 dc->w.DCOrgX + rect->left,
                                 dc->w.DCOrgY + rect->top,
                                 COLOR_ToPhysical( dc, color ), fillType );
    }

    XDestroyImage( image );
    return TRUE;
}


/**********************************************************************
 *          ExtFloodFill16   (GDI.372)
 */
BOOL16 ExtFloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color,
                       UINT16 fillType )
{
    return ExtFloodFill32( hdc, x, y, color, fillType );
}


/**********************************************************************
 *          ExtFloodFill32   (GDI32.96)
 */
BOOL32 ExtFloodFill32( HDC32 hdc, INT32 x, INT32 y, COLORREF color,
                       UINT32 fillType )
{
    RECT32 rect;
    DC *dc;

    dprintf_graphics( stddeb, "ExtFloodFill %04x %d,%d %06lx %d\n",
                      hdc, x, y, color, fillType );
    dc = (DC *) GDI_GetObjPtr(hdc, DC_MAGIC);
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam4(dc, META_FLOODFILL, x, y, HIWORD(color), 
		      LOWORD(color)); 
	return TRUE;
    }

    if (!PtVisible32( hdc, x, y )) return FALSE;
    if (GetRgnBox32( dc->w.hGCClipRgn, &rect ) == ERROR) return FALSE;

    return CallTo32_LargeStack( (int(*)())GRAPH_DoFloodFill, 6,
                                dc, &rect, x, y, color, fillType );
}


/**********************************************************************
 *          FloodFill16   (GDI.25)
 */
BOOL16 FloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return ExtFloodFill32( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          FloodFill32   (GDI32.104)
 */
BOOL32 FloodFill32( HDC32 hdc, INT32 x, INT32 y, COLORREF color )
{
    return ExtFloodFill32( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          DrawEdge16   (USER.659)
 */
BOOL16 DrawEdge16( HDC16 hdc, LPRECT16 rc, UINT16 edge, UINT16 flags )
{
    RECT32 rect32;
    BOOL32 ret;

    CONV_RECT16TO32( rc, &rect32 );
    ret = DrawEdge32( hdc, &rect32, edge, flags );
    CONV_RECT32TO16( &rect32, rc );
    return ret;
}


/**********************************************************************
 *          DrawEdge32   (USER32.154)
 */
BOOL32 DrawEdge32( HDC32 hdc, LPRECT32 rc, UINT32 edge, UINT32 flags )
{
    HBRUSH32 hbrushOld;

    if (flags >= BF_DIAGONAL)
        fprintf( stderr, "DrawEdge: unsupported flags %04x\n", flags );

    dprintf_graphics( stddeb, "DrawEdge: %04x %d,%d-%d,%d %04x %04x\n",
                      hdc, rc->left, rc->top, rc->right, rc->bottom,
                      edge, flags );

    /* First do all the raised edges */

    hbrushOld = SelectObject32( hdc, sysColorObjects.hbrushBtnHighlight );
    if (edge & BDR_RAISEDOUTER)
    {
        if (flags & BF_LEFT) PatBlt32( hdc, rc->left, rc->top,
                                       1, rc->bottom - rc->top - 1, PATCOPY );
        if (flags & BF_TOP) PatBlt32( hdc, rc->left, rc->top,
                                      rc->right - rc->left - 1, 1, PATCOPY );
    }
    if (edge & BDR_SUNKENOUTER)
    {
        if (flags & BF_RIGHT) PatBlt32( hdc, rc->right - 1, rc->top,
                                        1, rc->bottom - rc->top, PATCOPY );
        if (flags & BF_BOTTOM) PatBlt32( hdc, rc->left, rc->bottom - 1,
                                         rc->right - rc->left, 1, PATCOPY );
    }
    if (edge & BDR_RAISEDINNER)
    {
        if (flags & BF_LEFT) PatBlt32( hdc, rc->left + 1, rc->top + 1, 
                                       1, rc->bottom - rc->top - 2, PATCOPY );
        if (flags & BF_TOP) PatBlt32( hdc, rc->left + 1, rc->top + 1,
                                      rc->right - rc->left - 2, 1, PATCOPY );
    }
    if (edge & BDR_SUNKENINNER)
    {
        if (flags & BF_RIGHT) PatBlt32( hdc, rc->right - 2, rc->top + 1,
                                        1, rc->bottom - rc->top - 2, PATCOPY );
        if (flags & BF_BOTTOM) PatBlt32( hdc, rc->left + 1, rc->bottom - 2,
                                         rc->right - rc->left - 2, 1, PATCOPY );
    }

    /* Then do all the sunken edges */

    SelectObject32( hdc, sysColorObjects.hbrushBtnShadow );
    if (edge & BDR_SUNKENOUTER)
    {
        if (flags & BF_LEFT) PatBlt32( hdc, rc->left, rc->top,
                                       1, rc->bottom - rc->top - 1, PATCOPY );
        if (flags & BF_TOP) PatBlt32( hdc, rc->left, rc->top,
                                      rc->right - rc->left - 1, 1, PATCOPY );
    }
    if (edge & BDR_RAISEDOUTER)
    {
        if (flags & BF_RIGHT) PatBlt32( hdc, rc->right - 1, rc->top,
                                        1, rc->bottom - rc->top, PATCOPY );
        if (flags & BF_BOTTOM) PatBlt32( hdc, rc->left, rc->bottom - 1,
                                         rc->right - rc->left, 1, PATCOPY );
    }
    if (edge & BDR_SUNKENINNER)
    {
        if (flags & BF_LEFT) PatBlt32( hdc, rc->left + 1, rc->top + 1, 
                                       1, rc->bottom - rc->top - 2, PATCOPY );
        if (flags & BF_TOP) PatBlt32( hdc, rc->left + 1, rc->top + 1,
                                      rc->right - rc->left - 2, 1, PATCOPY );
    }
    if (edge & BDR_RAISEDINNER)
    {
        if (flags & BF_RIGHT) PatBlt32( hdc, rc->right - 2, rc->top + 1,
                                        1, rc->bottom - rc->top - 2, PATCOPY );
        if (flags & BF_BOTTOM) PatBlt32( hdc, rc->left + 1, rc->bottom - 2,
                                         rc->right - rc->left - 2, 1, PATCOPY );
    }

    SelectObject32( hdc, hbrushOld );
    return TRUE;
}


/**********************************************************************
 *          DrawFrameControl16  (USER.656)
 */
BOOL16 DrawFrameControl16( HDC16 hdc, LPRECT16 rc, UINT16 edge, UINT16 flags )
{
    fprintf( stdnimp,"DrawFrameControl16(%x,%p,%d,%x), empty stub!\n",
             hdc,rc,edge,flags );
    return TRUE;
}


/**********************************************************************
 *          DrawFrameControl32  (USER32.157)
 */
BOOL32 DrawFrameControl32( HDC32 hdc, LPRECT32 rc, UINT32 edge, UINT32 flags )
{
    fprintf( stdnimp,"DrawFrameControl32(%x,%p,%d,%x), empty stub!\n",
             hdc,rc,edge,flags );
    return TRUE;
}

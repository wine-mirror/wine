/*
 * GDI graphics operations
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <math.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#ifndef PI
#define PI M_PI
#endif
#include "dc.h"
#include "bitmap.h"
#include "callback.h"
#include "metafile.h"
#include "syscolor.h"
#include "stddebug.h"
#include "color.h"
#include "region.h"
#include "debug.h"

static __inline__ void swap_int(int *a, int *b)
{
	int c;
	
	c = *a;
	*a = *b;
	*b = c;
}

/***********************************************************************
 *           LineTo    (GDI.19)
 */
BOOL LineTo( HDC hdc, short x, short y )
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
DWORD MoveTo( HDC hdc, short x, short y )
{
    short oldx, oldy;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_MOVETO, x, y);
	return 0;
    }

    oldx = dc->w.CursPosX;
    oldy = dc->w.CursPosY;
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return oldx | (oldy << 16);
}


/***********************************************************************
 *           MoveToEx    (GDI.483)
 */
BOOL MoveToEx( HDC hdc, short x, short y, LPPOINT pt )
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
 *           GRAPH_DrawArc
 *
 * Helper functions for Arc(), Chord() and Pie().
 * 'lines' is the number of lines to draw: 0 for Arc, 1 for Chord, 2 for Pie.
 */
static BOOL GRAPH_DrawArc( HDC hdc, int left, int top, int right, int bottom,
		    int xstart, int ystart, int xend, int yend, int lines )
{
    int xcenter, ycenter, istart_angle, idiff_angle;
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
    istart_angle = (int)(start_angle * 180 * 64 / PI);
    idiff_angle  = (int)((end_angle - start_angle) * 180 * 64 / PI );
    if (idiff_angle <= 0) idiff_angle += 360 * 64;
    if (left > right) swap_int( &left, &right );
    if (top > bottom) swap_int( &top, &bottom );

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
 *           Arc    (GDI.23)
 */
BOOL Arc( HDC hdc, int left, int top, int right, int bottom,
	  int xstart, int ystart, int xend, int yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           Pie    (GDI.26)
 */
BOOL Pie( HDC hdc, int left, int top, int right, int bottom,
	  int xstart, int ystart, int xend, int yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 2 );
}


/***********************************************************************
 *           Chord    (GDI.348)
 */
BOOL Chord( HDC hdc, int left, int top, int right, int bottom,
	    int xstart, int ystart, int xend, int yend )
{
    return GRAPH_DrawArc( hdc, left, top, right, bottom,
			  xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           Ellipse    (GDI.24)
 */
BOOL Ellipse( HDC hdc, int left, int top, int right, int bottom )
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

    if (right < left)
    	swap_int(&right, &left);

    if (bottom < top)
    	swap_int(&bottom, &top);
    
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
 *           Rectangle    (GDI.27)
 */
BOOL Rectangle( HDC hdc, int left, int top, int right, int bottom )
{
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

    if (right < left)
    	swap_int(&right, &left);

    if (bottom < top)
    	swap_int(&bottom, &top);

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
    
    if ((dc->u.x.pen.style == PS_INSIDEFRAME) &&
        (dc->u.x.pen.width < right-left) &&
        (dc->u.x.pen.width < bottom-top))
    {
        left   += dc->u.x.pen.width / 2;
        right  -= (dc->u.x.pen.width + 1) / 2;
        top    += dc->u.x.pen.width / 2;
        bottom -= (dc->u.x.pen.width + 1) / 2;
    }

    if (DC_SetupGCForBrush( dc ))
	XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left, bottom-top );
    if (DC_SetupGCForPen( dc ))
	XDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );
    return TRUE;
}


/***********************************************************************
 *           RoundRect    (GDI.28)
 */
BOOL RoundRect( HDC hDC, short left, short top, short right, short bottom,
                short ell_width, short ell_height )
{
    DC * dc = (DC *) GDI_GetObjPtr(hDC, DC_MAGIC);
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hDC, METAFILE_DC_MAGIC);
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
    ell_width  = abs( ell_width * dc->w.VportExtX / dc->w.WndExtX );
    ell_height = abs( ell_height * dc->w.VportExtY / dc->w.WndExtY );

    /* Fix the coordinates */

    if (left > right) { short t = left; left = right; right = t; }
    if (top > bottom) { short t = top; top = bottom; bottom = t; }
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
 *           FillRect    (USER.81)
 */
int FillRect( HDC hdc, LPRECT rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;

    if ((rect->right <= rect->left) || (rect->bottom <= rect->top)) return 0;
    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
	    rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           InvertRect    (USER.82)
 */
void InvertRect( HDC hdc, LPRECT rect )
{
    if ((rect->right <= rect->left) || (rect->bottom <= rect->top)) return;
    PatBlt( hdc, rect->left, rect->top,
	    rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *           FrameRect    (USER.83)
 */
int FrameRect( HDC hdc, LPRECT rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;
    int left, top, right, bottom;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    if ((rect->right <= rect->left) || (rect->bottom <= rect->top)) return 0;
    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    
    left   = XLPTODP( dc, rect->left );
    top    = YLPTODP( dc, rect->top );
    right  = XLPTODP( dc, rect->right );
    bottom = YLPTODP( dc, rect->bottom );
    
    if (DC_SetupGCForBrush( dc ))
    {
   	PatBlt( hdc, rect->left, rect->top, 1,
	    rect->bottom - rect->top, PATCOPY );
	PatBlt( hdc, rect->right - 1, rect->top, 1,
	    rect->bottom - rect->top, PATCOPY );
	PatBlt( hdc, rect->left, rect->top,
	    rect->right - rect->left, 1, PATCOPY );
	PatBlt( hdc, rect->left, rect->bottom - 1,
	    rect->right - rect->left, 1, PATCOPY );
	}    
    SelectObject( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           SetPixel    (GDI.31)
 */
COLORREF SetPixel( HDC hdc, short x, short y, COLORREF color )
{
    Pixel pixel;
    PALETTEENTRY entry;
    
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

    if (screenDepth <= 8)
    {
        GetPaletteEntries( dc->w.hPalette, pixel, 1, &entry );
        return RGB( entry.peRed, entry.peGreen, entry.peBlue );
    }
    else return (COLORREF)pixel;
}


/***********************************************************************
 *           GetPixel    (GDI.83)
 */
COLORREF GetPixel( HDC hdc, short x, short y )
{
    PALETTEENTRY entry;
    XImage * image;
    WORD * mapping;
    int pixel;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

#ifdef SOLITAIRE_SPEED_HACK
    return 0;
#endif

    if (!PtVisible( hdc, x, y )) return 0;

    x = dc->w.DCOrgX + XLPTODP( dc, x );
    y = dc->w.DCOrgY + YLPTODP( dc, y );
    image = XGetImage( display, dc->u.x.drawable, x, y,
		       1, 1, AllPlanes, ZPixmap );
    pixel = XGetPixel( image, 0, 0 );
    XDestroyImage( image );
    
    if (screenDepth > 8) return pixel;
    mapping = (WORD *) GDI_HEAP_LIN_ADDR( dc->u.x.pal.hRevMapping );
    if (mapping) pixel = mapping[pixel];
    GetPaletteEntries( dc->w.hPalette, pixel, 1, &entry );
    return RGB( entry.peRed, entry.peGreen, entry.peBlue );
}


/***********************************************************************
 *           PaintRgn    (GDI.43)
 */
BOOL PaintRgn( HDC hdc, HRGN hrgn )
{
    RECT box;
    HRGN tmpVisRgn, prevVisRgn;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

      /* Modify visible region */

    if (!(prevVisRgn = SaveVisRgn( hdc ))) return FALSE;
    if (!(tmpVisRgn = CreateRectRgn( 0, 0, 0, 0 )))
    {
        RestoreVisRgn( hdc );
        return FALSE;
    }
    CombineRgn( tmpVisRgn, prevVisRgn, hrgn, RGN_AND );
    SelectVisRgn( hdc, tmpVisRgn );
    DeleteObject( tmpVisRgn );

      /* Fill the region */

    GetRgnBox( dc->w.hGCClipRgn, &box );
    if (DC_SetupGCForBrush( dc ))
	XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + box.left, dc->w.DCOrgY + box.top,
		        box.right-box.left, box.bottom-box.top );

      /* Restore the visible region */

    RestoreVisRgn( hdc );
    return TRUE;
}


/***********************************************************************
 *           FillRgn    (GDI.40)
 */
BOOL FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    BOOL retval;
    HBRUSH prevBrush = SelectObject( hdc, hbrush );
    if (!prevBrush) return FALSE;
    retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    return retval;
}

/***********************************************************************
 *           FrameRgn     (GDI.41)
 */
BOOL FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, int nWidth, int nHeight )
{
    HRGN tmp = CreateRectRgn( 0, 0, 0, 0 );
    if(!REGION_FrameRgn( tmp, hrgn, nWidth, nHeight )) return 0;
    FillRgn( hdc, tmp, hbrush );
    DeleteObject( tmp );
    return 1;
}

/***********************************************************************
 *           InvertRgn    (GDI.42)
 */
BOOL InvertRgn( HDC hdc, HRGN hrgn )
{
    HBRUSH prevBrush = SelectObject( hdc, GetStockObject(BLACK_BRUSH) );
    WORD prevROP = SetROP2( hdc, R2_NOT );
    BOOL retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    SetROP2( hdc, prevROP );
    return retval;
}


/***********************************************************************
 *           DrawFocusRect    (USER.466)
 */
void DrawFocusRect( HDC hdc, LPRECT rc )
{
    HPEN hOldPen;
    int oldDrawMode, oldBkMode;
    int left, top, right, bottom;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return;

    left   = XLPTODP( dc, rc->left );
    top    = YLPTODP( dc, rc->top );
    right  = XLPTODP( dc, rc->right );
    bottom = YLPTODP( dc, rc->bottom );
    
    hOldPen = (HPEN)SelectObject(hdc, sysColorObjects.hpenWindowText );
    oldDrawMode = SetROP2(hdc, R2_XORPEN);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);

    if (DC_SetupGCForPen( dc ))
	XDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );

    SetBkMode(hdc, oldBkMode);
    SetROP2(hdc, oldDrawMode);
    SelectObject(hdc, (HANDLE)hOldPen);
}


/**********************************************************************
 *          GRAPH_DrawBitmap
 *
 * Short-cut function to blit a bitmap into a device.
 * Faster than CreateCompatibleDC() + SelectBitmap() + BitBlt() + DeleteDC().
 */
BOOL GRAPH_DrawBitmap( HDC hdc, HBITMAP hbitmap, int xdest, int ydest,
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
void GRAPH_DrawReliefRect( HDC hdc, RECT *rect, int highlight_size,
                           int shadow_size, BOOL pressed )
{
    HBRUSH hbrushOld;
    int i;

    hbrushOld = SelectObject( hdc, pressed ? sysColorObjects.hbrushBtnShadow :
			                  sysColorObjects.hbrushBtnHighlight );
    for (i = 0; i < highlight_size; i++)
    {
	PatBlt( hdc, rect->left + i, rect->top,
	        1, rect->bottom - rect->top - i, PATCOPY );
	PatBlt( hdc, rect->left, rect->top + i,
	        rect->right - rect->left - i, 1, PATCOPY );
    }

    SelectObject( hdc, pressed ? sysColorObjects.hbrushBtnHighlight :
		                 sysColorObjects.hbrushBtnShadow );
    for (i = 0; i < shadow_size; i++)
    {
	PatBlt( hdc, rect->right - i - 1, rect->top + i,
	        1, rect->bottom - rect->top - i, PATCOPY );
	PatBlt( hdc, rect->left + i, rect->bottom - i - 1,
	        rect->right - rect->left - i, 1, PATCOPY );
    }

    SelectObject( hdc, hbrushOld );
}


/**********************************************************************
 *          Polyline  (GDI.37)
 */
BOOL Polyline (HDC hdc, LPPOINT pt, int count)
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
 *          Polygon  (GDI.36)
 */
BOOL Polygon (HDC hdc, LPPOINT pt, int count)
{
    register int i;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    XPoint *points = (XPoint *) malloc (sizeof (XPoint) * (count+1));

    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaPoly(dc, META_POLYGON, pt, count); 
	return TRUE;
    }

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
 *          PolyPolygon  (GDI.450)
 */
BOOL PolyPolygon( HDC hdc, LPPOINT pt, LPINT counts, WORD polygons )
{
    HRGN hrgn;
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
    hrgn = CreatePolyPolygonRgn( pt, counts, polygons, dc->w.polyFillMode );
    PaintRgn( hdc, hrgn );
    DeleteObject( hrgn );

      /* Draw the outline of the polygons */

    if (DC_SetupGCForPen ( dc ))
    {
	int i, j, max = 0;
	XPoint *points;

	for (i = 0; i < polygons; i++) if (counts[i] > max) max = counts[i];
	points = (XPoint *) malloc( sizeof(XPoint) * (max+1) );

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
static BOOL GRAPH_DoFloodFill( DC *dc, RECT *rect, INT x, INT y,
                               COLORREF color, WORD fillType )
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
 *          ExtFloodFill  (GDI.372)
 */
BOOL ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, WORD fillType )
{
    RECT rect;
    DC *dc;

    dprintf_graphics( stddeb, "ExtFloodFill "NPFMT" %d,%d %06lx %d\n",
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

    if (!PtVisible( hdc, x, y )) return FALSE;
    if (GetRgnBox( dc->w.hGCClipRgn, &rect ) == ERROR) return FALSE;

    return CallTo32_LargeStack( (int(*)())GRAPH_DoFloodFill, 6,
                                dc, &rect, x, y, color, fillType );
}


/**********************************************************************
 *          FloodFill  (GDI.25)
 */
BOOL FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}

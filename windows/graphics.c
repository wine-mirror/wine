/*
 * GDI graphics operations
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <math.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#ifndef PI
#define PI M_PI
#endif

#include "gdi.h"
#include "syscolor.h"

extern const int DC_XROPfunction[];

extern int COLOR_ToPhysical( DC *dc, COLORREF color );

static inline swap_int(int *a, int *b)
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
BOOL GRAPH_DrawArc( HDC hdc, int left, int top, int right, int bottom,
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

    if (!DC_SetupGCForPen( dc )) return TRUE;

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

    if ((left == right) || (top == bottom)) {
	if (DC_SetupGCForPen( dc ))
	    XDrawLine(display, dc->u.x.drawable, dc->u.x.gc, 
		  dc->w.DCOrgX + left,
		  dc->w.DCOrgY + top,
		  dc->w.DCOrgX + right,
		  dc->w.DCOrgY + bottom);
	return TRUE;
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
					short ell_width, short ell_height)
{
    int		x1, y1, x2, y2;
    DC * dc = (DC *) GDI_GetObjPtr(hDC, DC_MAGIC);
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hDC, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam6(dc, META_ROUNDRECT, left, top, right, bottom,
		      ell_width, ell_height);
	return TRUE;
    }
/*
    printf("RoundRect(%d %d %d %d  %d %d\n", 
    	left, top, right, bottom, ell_width, ell_height);
*/
    x1 = XLPTODP(dc, left);
    y1 = YLPTODP(dc, top);
    x2 = XLPTODP(dc, right - ell_width);
    y2 = YLPTODP(dc, bottom - ell_height);
    if (DC_SetupGCForBrush(dc)) {
	XFillArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x1, dc->w.DCOrgY + y1,
		        ell_width, ell_height, 90 * 64, 90 * 64);
	XFillArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x1, dc->w.DCOrgY + y2,
		        ell_width, ell_height, 180 * 64, 90 * 64);
	XFillArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x2, dc->w.DCOrgY + y2,
		        ell_width, ell_height, 270 * 64, 90 * 64);
	XFillArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x2, dc->w.DCOrgY + y1,
		        ell_width, ell_height, 0, 90 * 64);
	ell_width /= 2;  ell_height /= 2;
	XFillRectangle(display, dc->u.x.drawable, dc->u.x.gc,
		dc->w.DCOrgX + left + ell_width, dc->w.DCOrgY + top,
		right - left - 2 * ell_width, bottom - top);
	XFillRectangle(display, dc->u.x.drawable, dc->u.x.gc,
		dc->w.DCOrgX + left, dc->w.DCOrgY + top + ell_height,
		ell_width, bottom - top - 2 * ell_height);
	XFillRectangle(display, dc->u.x.drawable, dc->u.x.gc,
		dc->w.DCOrgX + right - ell_width, dc->w.DCOrgY + top + ell_height,
		ell_width, bottom - top - 2 * ell_height);
	ell_width *= 2;  ell_height *= 2;
	}    	
    if (DC_SetupGCForPen(dc)) {
	XDrawArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x1, dc->w.DCOrgY + y1,
		        ell_width, ell_height, 90 * 64, 90 * 64);
	XDrawArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x1, dc->w.DCOrgY + y2,
		        ell_width, ell_height, 180 * 64, 90 * 64);
	XDrawArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x2, dc->w.DCOrgY + y2,
		        ell_width, ell_height, 270 * 64, 90 * 64);
	XDrawArc(display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + x2, dc->w.DCOrgY + y1,
		        ell_width, ell_height, 0, 90 * 64);
	}
    ell_width /= 2;  ell_height /= 2;
    MoveTo(hDC, left, top + ell_height);
    LineTo(hDC, left, bottom - ell_height);
    MoveTo(hDC, left + ell_width, bottom);
    LineTo(hDC, right - ell_width, bottom);
    MoveTo(hDC, right, bottom - ell_height);
    LineTo(hDC, right, top + ell_height);
    MoveTo(hDC, right - ell_width, top);
    LineTo(hDC, left + ell_width, top);
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
    
    if (DC_SetupGCForBrush( dc )) {
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
    int pixel;
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
    GetPaletteEntries( dc->w.hPalette, pixel, 1, &entry );
    
    XSetForeground( display, dc->u.x.gc, pixel );
    XSetFunction( display, dc->u.x.gc, GXcopy );
    XDrawPoint( display, dc->u.x.drawable, dc->u.x.gc, x, y );

    return RGB( entry.peRed, entry.peGreen, entry.peBlue );
}


/***********************************************************************
 *           GetPixel    (GDI.83)
 */
COLORREF GetPixel( HDC hdc, short x, short y )
{
    PALETTEENTRY entry;
    XImage * image;
    
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

#ifdef SOLITAIRE_SPEED_HACK
    return 0;
#endif

    x = dc->w.DCOrgX + XLPTODP( dc, x );
    y = dc->w.DCOrgY + YLPTODP( dc, y );
    if ((x < 0) || (y < 0)) return 0;
    
    if (!(dc->w.flags & DC_MEMORY))
    {
	XWindowAttributes win_attr;
	
	if (!XGetWindowAttributes( display, dc->u.x.drawable, &win_attr ))
	    return 0;
	if (win_attr.map_state != IsViewable) return 0;
	if ((x >= win_attr.width) || (y >= win_attr.height)) return 0;
    }
    
    image = XGetImage( display, dc->u.x.drawable, x, y,
		       1, 1, AllPlanes, ZPixmap );
    GetPaletteEntries( dc->w.hPalette, XGetPixel( image, 0, 0 ), 1, &entry );
    XDestroyImage( image );
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

    prevVisRgn = SaveVisRgn( hdc );
    if (prevVisRgn)
    {
	if (!(tmpVisRgn = CreateRectRgn( 0, 0, 0, 0 )))
	{
	    RestoreVisRgn( hdc );
	    return FALSE;
	}
	CombineRgn( tmpVisRgn, prevVisRgn, hrgn, RGN_AND );
	SelectVisRgn( hdc, tmpVisRgn );
	DeleteObject( tmpVisRgn );
    }
    else SelectVisRgn( hdc, hrgn );

      /* Fill the region */

    GetClipBox( hdc, &box );
    if (DC_SetupGCForBrush( dc ))
	XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + box.left, dc->w.DCOrgY + box.top,
		        box.right-box.left, box.bottom-box.top );

      /* Restore the visible region */

    if (prevVisRgn) RestoreVisRgn( hdc );
    else SelectVisRgn( hdc, 0 );
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
		       int xsrc, int ysrc, int width, int height, int rop )
{
    XGCValues val;
    BITMAPOBJ *bmp;
    DC *dc;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return FALSE;
    if (!(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return FALSE;
    val.function   = DC_XROPfunction[(rop >> 16) & 0x0f];
    val.foreground = dc->w.textPixel;
    val.background = dc->w.backgroundPixel;
    XChangeGC(display, dc->u.x.gc, GCFunction|GCForeground|GCBackground, &val);
    if (bmp->bitmap.bmBitsPixel == 1)
    {
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
 *          DrawReliefRect  (Not a MSWin Call)
 */
void DrawReliefRect( HDC hdc, RECT rect, int thickness, BOOL pressed )
{
    HBRUSH hbrushOld;
    int i;

    hbrushOld = SelectObject( hdc, pressed ? sysColorObjects.hbrushBtnShadow :
			                  sysColorObjects.hbrushBtnHighlight );
    for (i = 0; i < thickness; i++)
    {
	PatBlt( hdc, rect.left + i, rect.top,
	        1, rect.bottom - rect.top - i, PATCOPY );
	PatBlt( hdc, rect.left, rect.top + i,
	        rect.right - rect.left - i, 1, PATCOPY );
    }

    SelectObject( hdc, pressed ? sysColorObjects.hbrushBtnHighlight :
		                 sysColorObjects.hbrushBtnShadow );
    for (i = 0; i < thickness; i++)
    {
	PatBlt( hdc, rect.right - i - 1, rect.top + i,
	        1, rect.bottom - rect.top - i, PATCOPY );
	PatBlt( hdc, rect.left + i, rect.bottom - i - 1,
	        rect.right - rect.left - i, 1, PATCOPY );
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
    {
	for (i = 0; i < count-1; i ++)
	    XDrawLine (display, dc->u.x.drawable, dc->u.x.gc,  
		       dc->w.DCOrgX + XLPTODP(dc, pt [i].x),
		       dc->w.DCOrgY + YLPTODP(dc, pt [i].y),
		       dc->w.DCOrgX + XLPTODP(dc, pt [i+1].x),
		       dc->w.DCOrgY + YLPTODP(dc, pt [i+1].y));
	XDrawLine (display, dc->u.x.drawable, dc->u.x.gc,  
		   dc->w.DCOrgX + XLPTODP(dc, pt [count-1].x),
		   dc->w.DCOrgY + YLPTODP(dc, pt [count-1].y),
		   dc->w.DCOrgX + XLPTODP(dc, pt [0].x),
		   dc->w.DCOrgY + YLPTODP(dc, pt [0].y));
    } 
	
    return (TRUE);
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
    int i;
    HRGN hrgn;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	/* MF_MetaPoly(dc, META_POLYGON, pt, count); */
	return TRUE;
    }
      /* The points should be converted to device coords before */
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
 *          FloodFill_rec -- FloodFill helper function
 *
 * Just does a recursive flood fill:
 * this is /not/ efficent -- a better way would be to draw
 * an entire line at a time, but this will do for now.
 */
static BOOL FloodFill_rec(XImage *image, int x, int y, 
		int orgx, int orgy, int endx, int endy, 
		Pixel borderp, Pixel fillp)
{
	Pixel testp;

	if (x > endx || x < orgx || y > endy || y < orgy)
		return FALSE;
	XPutPixel(image, x, y, fillp);
	
	if ((x+1 <= endx) && (y+1 <= endy)) {
	  testp = XGetPixel(image, x+1, y+1);
	  if (testp != borderp && testp != fillp)
	    FloodFill_rec(image, x+1, y+1, orgx, orgy, 
			  endx, endy, borderp, fillp);
	}
	if ((x+1 <= endx) && (y-1 >= orgy)) {
	  testp = XGetPixel(image, x+1, y-1);
	  if (testp != borderp && testp != fillp)
		FloodFill_rec(image, x+1, y-1, orgx, orgy, 
				endx, endy, borderp, fillp);
	}
	if ((x-1 >= orgx) && (y+1 <= endy)) {
	  testp = XGetPixel(image, x-1, y+1); 
	  if (testp != borderp && testp != fillp)
	    FloodFill_rec(image, x-1, y+1, orgx, orgy,
			  endx, endy, borderp, fillp);
	}
	if ((x-1 >= orgx) && (y-1 >= orgy)) {
	  testp = XGetPixel(image, x-1, y-1);
	  if (testp != borderp && testp != fillp) 
	    FloodFill_rec(image, x-1, y-1, orgx, orgy, 
			  endx, endy, borderp, fillp);
	}
	return TRUE;
}

 
/**********************************************************************
 *          FloodFill (GDI.25)
 */
BOOL FloodFill(HDC hdc, short x, short y, DWORD crColor)
{
    Pixel boundrypixel;
    int imagex, imagey;
    XImage *image;
    DC *dc;

#ifdef DEBUG_GRAPHICS
    printf("FloodFill %x %d,%d %x\n", hdc, x, y, crColor);
#endif
    dc = (DC *) GDI_GetObjPtr(hdc, DC_MAGIC);
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam4(dc, META_FLOODFILL, x, y, HIWORD(crColor), 
		      LOWORD(crColor)); 
	return TRUE;
    }

    x = dc->w.DCOrgX + XLPTODP(dc, x);
    y = dc->w.DCOrgY + YLPTODP(dc, y);

    if (x < dc->w.DCOrgX || x > dc->w.DCOrgX + dc->w.DCSizeX ||
	y < dc->w.DCOrgY || y > dc->w.DCOrgY + dc->w.DCSizeY)
	return FALSE;

    if (!DC_SetupGCForBrush(dc)) 
	return FALSE;

    boundrypixel = GetNearestPaletteIndex( dc->w.hPalette, crColor );	

    image = XGetImage(display, dc->u.x.drawable,  
		      dc->w.DCOrgX, dc->w.DCOrgY,
		      dc->w.DCSizeX, dc->w.DCSizeY, AllPlanes, ZPixmap);
    if (XGetPixel(image, x, y) == boundrypixel) 
	return FALSE;
    if (!FloodFill_rec(image, x, y, 
		       0, 0, 
		       dc->w.DCSizeX-1, 
		       dc->w.DCSizeY-1, 
		       boundrypixel, dc->u.x.brush.pixel)) {
	XDestroyImage(image);
	return FALSE;
    }

    XPutImage(display, dc->u.x.drawable, dc->u.x.gc, image,
	      0, 0,
	      dc->w.DCOrgX, dc->w.DCOrgY,
	      dc->w.DCSizeX, dc->w.DCSizeY);
    XDestroyImage(image);

    return TRUE;
}



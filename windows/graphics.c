/*
 * GDI graphics operations
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <math.h>
#include <X11/Xlib.h>
#ifndef PI
#define PI M_PI
#endif

#include "gdi.h"

/***********************************************************************
 *           LineTo    (GDI.19)
 */
BOOL LineTo( HDC hdc, short x, short y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (DC_SetupGCForPen( dc ))
	XDrawLine(XT_display, dc->u.x.drawable, dc->u.x.gc, 
		  XLPTODP( dc, dc->w.CursPosX ), YLPTODP( dc, dc->w.CursPosY ),
		  XLPTODP( dc, x ), YLPTODP( dc, y ) );
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           MoveTo    (GDI.20)
 */
DWORD MoveTo( HDC hdc, short x, short y )
{
    POINT pt;
    if (MoveToEx( hdc, x, y, &pt )) return pt.x | (pt.y << 16);
    else return 0;
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
    int xcenter, ycenter;
    double start_angle, end_angle, diff_angle;
    XPoint points[3];
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

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
    diff_angle  = end_angle - start_angle;
    if (diff_angle < 0.0) diff_angle += 2*PI;

    XDrawArc( XT_display, dc->u.x.drawable, dc->u.x.gc,
	      left, top, right-left-1, bottom-top-1,
	      (int)(start_angle * 180 * 64 / PI),
	      (int)(diff_angle * 180 * 64 / PI) );
    if (!lines) return TRUE;

    points[0].x = xcenter + (int)(cos(start_angle) * (right-left) / 2);
    points[0].y = ycenter - (int)(sin(start_angle) * (bottom-top) / 2);
    points[1].x = xcenter + (int)(cos(end_angle) * (right-left) / 2);
    points[1].y = ycenter - (int)(sin(end_angle) * (bottom-top) / 2);
    if (lines == 2)
    {
	points[2] = points[1];
	points[1].x = xcenter;
	points[1].y = ycenter;
    }
    XDrawLines( XT_display, dc->u.x.drawable, dc->u.x.gc,
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
    if (!dc) return FALSE;

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    if ((left == right) || (top == bottom)) return FALSE;
    
    if (DC_SetupGCForBrush( dc ))
	XFillArc( XT_display, dc->u.x.drawable, dc->u.x.gc,
		  left, top, right-left-1, bottom-top-1, 0, 360*64 );
    if (DC_SetupGCForPen( dc ))
	XDrawArc( XT_display, dc->u.x.drawable, dc->u.x.gc,
		  left, top, right-left-1, bottom-top-1, 0, 360*64 );
    return TRUE;
}


/***********************************************************************
 *           Rectangle    (GDI.27)
 */
BOOL Rectangle( HDC hdc, int left, int top, int right, int bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    left   = XLPTODP( dc, left );
    top    = YLPTODP( dc, top );
    right  = XLPTODP( dc, right );
    bottom = YLPTODP( dc, bottom );
    
    if (DC_SetupGCForBrush( dc ))
	XFillRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
		        left, top, right-left-1, bottom-top-1 );
    if (DC_SetupGCForPen( dc ))
	XDrawRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
		        left, top, right-left-1, bottom-top-1 );
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
	XDrawRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
		        left, top, right-left-1, bottom-top-1 );
	    
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
    if (!dc) return 0;

    x = XLPTODP( dc, x );
    y = YLPTODP( dc, y );
    pixel = GetNearestPaletteIndex( dc->w.hPalette, color );
    GetPaletteEntries( dc->w.hPalette, pixel, 1, &entry );
    
    XSetForeground( XT_display, dc->u.x.gc, pixel );
    XSetFunction( XT_display, dc->u.x.gc, GXcopy );
    XDrawPoint( XT_display, dc->u.x.drawable, dc->u.x.gc, x, y );

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

    x = XLPTODP( dc, x );
    y = YLPTODP( dc, y );
    if ((x < 0) || (y < 0)) return 0;
    
    if (dc->u.x.widget)
    {
	XWindowAttributes win_attr;
	
	if (!XtIsRealized(dc->u.x.widget)) return 0;
	if (!XGetWindowAttributes( XT_display, dc->u.x.drawable, &win_attr ))
	    return 0;
	if (win_attr.map_state != IsViewable) return 0;
	if ((x >= win_attr.width) || (y >= win_attr.height)) return 0;
    }
    
    image = XGetImage( XT_display, dc->u.x.drawable, x, y,
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
	XFillRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
		        box.left, box.top,
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
 *           DrawFocusRect    (GDI.466)
 */
void DrawFocusRect( HDC hdc, LPRECT rc )
{
    HPEN hPen, hOldPen;
    int oldDrawMode, oldBkMode;
    int left, top, right, bottom;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return;

    left   = XLPTODP( dc, rc->left );
    top    = YLPTODP( dc, rc->top );
    right  = XLPTODP( dc, rc->right );
    bottom = YLPTODP( dc, rc->bottom );
    
    hPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_WINDOWTEXT)); 
    hOldPen = (HPEN)SelectObject(hdc, (HANDLE)hPen);
/*    oldDrawMode = SetROP2(hdc, R2_XORPEN);  */
    oldBkMode = SetBkMode(hdc, TRANSPARENT);

    if (DC_SetupGCForPen( dc ))
	XDrawRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
		        left, top, right-left-1, bottom-top-1 );

    SetBkMode(hdc, oldBkMode);
/*    SetROP2(hdc, oldDrawMode);  */
    SelectObject(hdc, (HANDLE)hOldPen);
    DeleteObject((HANDLE)hPen);
}


/**********************************************************************
 *          Line  (Not a MSWin Call)
 */
void Line(HDC hDC, int X1, int Y1, int X2, int Y2)
{
MoveTo(hDC, X1, Y1);
LineTo(hDC, X2, Y2);
}


/**********************************************************************
 *          DrawReliefRect  (Not a MSWin Call)
 */
 void DrawReliefRect(HDC hDC, RECT rect, int ThickNess, int Mode)
{
HPEN   hWHITEPen;
HPEN   hDKGRAYPen;
HPEN   hOldPen;
int    OldColor;
rect.right--;  rect.bottom--;
hDKGRAYPen = CreatePen(PS_SOLID, 1, 0x00808080L);
hWHITEPen = GetStockObject(WHITE_PEN);
hOldPen = SelectObject(hDC, hWHITEPen);
while(ThickNess > 0) {
    if (Mode == 0)
	SelectObject(hDC, hWHITEPen);
    else
	SelectObject(hDC, hDKGRAYPen);
    Line(hDC, rect.left, rect.top, rect.right, rect.top);
    Line(hDC, rect.left, rect.top, rect.left, rect.bottom + 1);
    if (Mode == 0)
	SelectObject(hDC, hDKGRAYPen);
    else
	SelectObject(hDC, hWHITEPen);
    Line(hDC, rect.right, rect.bottom, rect.left, rect.bottom);
    Line(hDC, rect.right, rect.bottom, rect.right, rect.top - 1);
    InflateRect(&rect, -1, -1);
    ThickNess--;
    }
SelectObject(hDC, hOldPen);
DeleteObject(hDKGRAYPen);
}

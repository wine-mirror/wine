/*
 * Misc. graphics operations
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 */

#include <stdlib.h>
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "monitor.h"
#include "cache.h"
#include "region.h"
#include "path.h"
#include "debug.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/winuser16.h"


/***********************************************************************
 *           LineTo16    (GDI.19)
 */
BOOL16 WINAPI LineTo16( HDC16 hdc, INT16 x, INT16 y )
{
    return LineTo( hdc, x, y );
}


/***********************************************************************
 *           LineTo32    (GDI32.249)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC * dc = DC_GetDCPtr( hdc );

    if(dc && PATH_IsPathOpen(dc->w.path))
        if(!PATH_LineTo(hdc, x, y))
	   return FALSE;
    
    return dc && dc->funcs->pLineTo &&
    	   dc->funcs->pLineTo(dc,x,y);
}


/***********************************************************************
 *           MoveTo    (GDI.20)
 */
DWORD WINAPI MoveTo16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT16	pt;

    if (!MoveToEx16(hdc,x,y,&pt))
    	return 0;
    return MAKELONG(pt.x,pt.y);
}


/***********************************************************************
 *           MoveToEx16    (GDI.483)
 */
BOOL16 WINAPI MoveToEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;

    if (!MoveToEx( (HDC)hdc, (INT)x, (INT)y, &pt32 )) return FALSE;
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return TRUE;

}


/***********************************************************************
 *           MoveToEx32    (GDI32.254)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if(dc && PATH_IsPathOpen(dc->w.path))
        if(!PATH_MoveTo(hdc))
	    return FALSE;

    return dc && dc->funcs->pMoveToEx &&
    	   dc->funcs->pMoveToEx(dc,x,y,pt);
}


/***********************************************************************
 *           Arc16    (GDI.23)
 */
BOOL16 WINAPI Arc16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                     INT16 bottom, INT16 xstart, INT16 ystart,
                     INT16 xend, INT16 yend )
{
    return Arc( (HDC)hdc, (INT)left, (INT)top, (INT)right,
   		  (INT)bottom, (INT)xstart, (INT)ystart, (INT)xend,
		  (INT)yend );
}


/***********************************************************************
 *           Arc32    (GDI32.7)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right,
                     INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if(dc && PATH_IsPathOpen(dc->w.path))
        if(!PATH_Arc(hdc, left, top, right, bottom, xstart, ystart, xend,
	   yend))
	   return FALSE;
    
    return dc && dc->funcs->pArc &&
    	   dc->funcs->pArc(dc,left,top,right,bottom,xstart,ystart,xend,yend);
}

/***********************************************************************
 *           ArcTo    (GDI32.8)
 */
BOOL WINAPI ArcTo( HDC hdc, 
                     INT left,   INT top, 
                     INT right,  INT bottom,
                     INT xstart, INT ystart,
                     INT xend,   INT yend )
{
    BOOL result;

    /*
     * According to the documentation, a line is drawn from the current
     * position to the starting point of the arc.
     */
    LineTo(hdc, xstart, ystart);

    /*
     * Then the arc is drawn.
     */
    result = Arc(hdc, 
                  left, top,
                  right, bottom,
                  xstart, ystart,
                  xend, yend);

    /*
     * If no error occured, the current position is moved to the ending
     * point of the arc.
     */
    if (result)
    {
        MoveToEx(hdc, xend, yend, NULL);
    }

    return result;
}

/***********************************************************************
 *           Pie16    (GDI.26)
 */
BOOL16 WINAPI Pie16( HDC16 hdc, INT16 left, INT16 top,
                     INT16 right, INT16 bottom, INT16 xstart, INT16 ystart,
                     INT16 xend, INT16 yend )
{
    return Pie( (HDC)hdc, (INT)left, (INT)top, (INT)right,
   		  (INT)bottom, (INT)xstart, (INT)ystart, (INT)xend,
		  (INT)yend );
}


/***********************************************************************
 *           Pie32   (GDI32.262)
 */
BOOL WINAPI Pie( HDC hdc, INT left, INT top,
                     INT right, INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pPie &&
    	   dc->funcs->pPie(dc,left,top,right,bottom,xstart,ystart,xend,yend);
}


/***********************************************************************
 *           Chord16    (GDI.348)
 */
BOOL16 WINAPI Chord16( HDC16 hdc, INT16 left, INT16 top,
                       INT16 right, INT16 bottom, INT16 xstart, INT16 ystart,
                       INT16 xend, INT16 yend )
{
    return Chord( hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           Chord32    (GDI32.14)
 */
BOOL WINAPI Chord( HDC hdc, INT left, INT top,
                       INT right, INT bottom, INT xstart, INT ystart,
                       INT xend, INT yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pChord &&
    	   dc->funcs->pChord(dc,left,top,right,bottom,xstart,ystart,xend,yend);
}


/***********************************************************************
 *           Ellipse16    (GDI.24)
 */
BOOL16 WINAPI Ellipse16( HDC16 hdc, INT16 left, INT16 top,
                         INT16 right, INT16 bottom )
{
    return Ellipse( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Ellipse32    (GDI32.75)
 */
BOOL WINAPI Ellipse( HDC hdc, INT left, INT top,
                         INT right, INT bottom )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pEllipse &&
    	   dc->funcs->pEllipse(dc,left,top,right,bottom);
}


/***********************************************************************
 *           Rectangle16    (GDI.27)
 */
BOOL16 WINAPI Rectangle16( HDC16 hdc, INT16 left, INT16 top,
                           INT16 right, INT16 bottom )
{
    return Rectangle( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Rectangle32    (GDI32.283)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top,
                           INT right, INT bottom )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if(dc && PATH_IsPathOpen(dc->w.path))
        if(!PATH_Rectangle(hdc, left, top, right, bottom))
           return FALSE;

    return dc && dc->funcs->pRectangle &&
    	   dc->funcs->pRectangle(dc,left,top,right,bottom);
}


/***********************************************************************
 *           RoundRect16    (GDI.28)
 */
BOOL16 WINAPI RoundRect16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                           INT16 bottom, INT16 ell_width, INT16 ell_height )
{
    return RoundRect( hdc, left, top, right, bottom, ell_width, ell_height );
}


/***********************************************************************
 *           RoundRect32    (GDI32.291)
 */
BOOL WINAPI RoundRect( HDC hdc, INT left, INT top, INT right,
                           INT bottom, INT ell_width, INT ell_height )
{
  
    if(ell_width == 0 || ell_height == 0) /* Just an optimization */
        return Rectangle(hdc, left, top, right, bottom);

    else {
        DC * dc = DC_GetDCPtr( hdc );

	return dc && dc->funcs->pRoundRect &&
    	   dc->funcs->pRoundRect(dc,left,top,right,bottom,ell_width,ell_height);
    }
}


/***********************************************************************
 *           FillRect16    (USER.81)
 */
INT16 WINAPI FillRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH16 prevBrush;

    /* coordinates are logical so we cannot fast-check 'rect',
     * it will be done later in the PatBlt().
     */

    if (!(prevBrush = SelectObject16( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject16( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           FillRect32    (USER32.197)
 */
INT WINAPI FillRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;

    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           InvertRect16    (USER.82)
 */
void WINAPI InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *           InvertRect32    (USER32.330)
 */
BOOL WINAPI InvertRect( HDC hdc, const RECT *rect )
{
    return PatBlt( hdc, rect->left, rect->top,
		     rect->right - rect->left, rect->bottom - rect->top, 
		     DSTINVERT );
}


/***********************************************************************
 *           FrameRect16    (USER.83)
 */
INT16 WINAPI FrameRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
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
    
    PatBlt( hdc, rect->left, rect->top, 1,
	      rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->right - 1, rect->top, 1,
	      rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
	      rect->right - rect->left, 1, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom - 1,
	      rect->right - rect->left, 1, PATCOPY );

    SelectObject16( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           FrameRect32    (USER32.203)
 */
INT WINAPI FrameRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    RECT16 rect16;
    CONV_RECT32TO16( rect, &rect16 );
    return FrameRect16( (HDC16)hdc, &rect16, (HBRUSH16)hbrush );
}


/***********************************************************************
 *           SetPixel16    (GDI.31)
 */
COLORREF WINAPI SetPixel16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return SetPixel( hdc, x, y, color );
}


/***********************************************************************
 *           SetPixel32    (GDI32.327)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc || !dc->funcs->pSetPixel) return 0;
    return dc->funcs->pSetPixel(dc,x,y,color);
}

/***********************************************************************
 *           SetPixelV32    (GDI32.329)
 */
BOOL WINAPI SetPixelV( HDC hdc, INT x, INT y, COLORREF color )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc || !dc->funcs->pSetPixel) return FALSE;
    dc->funcs->pSetPixel(dc,x,y,color);
    return TRUE;
}

/***********************************************************************
 *           GetPixel16    (GDI.83)
 */
COLORREF WINAPI GetPixel16( HDC16 hdc, INT16 x, INT16 y )
{
    return GetPixel( hdc, x, y );
}


/***********************************************************************
 *           GetPixel32    (GDI32.211)
 */
COLORREF WINAPI GetPixel( HDC hdc, INT x, INT y )
{
    DC * dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;
#ifdef SOLITAIRE_SPEED_HACK
    return 0;
#endif

    /* FIXME: should this be in the graphics driver? */
    if (!PtVisible( hdc, x, y )) return 0;
    if (!dc || !dc->funcs->pGetPixel) return 0;
    return dc->funcs->pGetPixel(dc,x,y);
}


/******************************************************************************
 * ChoosePixelFormat [GDI32.13]
 * Matches a pixel format to given format
 *
 * PARAMS
 *    hdc  [I] Device context to search for best pixel match
 *    ppfd [I] Pixel format for which a match is sought
 *
 * RETURNS
 *    Success: Pixel format index closest to given format
 *    Failure: 0
 */
INT WINAPI ChoosePixelFormat( HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd )
{
    FIXME(gdi, "(%d,%p): stub\n",hdc,ppfd);
    return 1;
}


/******************************************************************************
 * SetPixelFormat [GDI32.328]
 * Sets pixel format of device context
 *
 * PARAMS
 *    hdc          [I] Device context to search for best pixel match
 *    iPixelFormat [I] Pixel format index
 *    ppfd         [I] Pixel format for which a match is sought
 *
 * RETURNS STD
 */
BOOL WINAPI SetPixelFormat( HDC hdc, int iPixelFormat, 
                              const PIXELFORMATDESCRIPTOR* ppfd)
{
    FIXME(gdi, "(%d,%d,%p): stub\n",hdc,iPixelFormat,ppfd);
    return TRUE;
}


/******************************************************************************
 * GetPixelFormat [GDI32.212]
 * Gets index of pixel format of DC
 *
 * PARAMETERS
 *    hdc [I] Device context whose pixel format index is sought
 *
 * RETURNS
 *    Success: Currently selected pixel format
 *    Failure: 0
 */
int WINAPI GetPixelFormat( HDC hdc )
{
    FIXME(gdi, "(%d): stub\n",hdc);
    return 1;
}


/******************************************************************************
 * DescribePixelFormat [GDI32.71]
 * Gets info about pixel format from DC
 *
 * PARAMS
 *    hdc          [I] Device context
 *    iPixelFormat [I] Pixel format selector
 *    nBytes       [I] Size of buffer
 *    ppfd         [O] Pointer to structure to receive pixel format data
 *
 * RETURNS
 *    Success: Maximum pixel format index of the device context
 *    Failure: 0
 */
int WINAPI DescribePixelFormat( HDC hdc, int iPixelFormat, UINT nBytes,
                                LPPIXELFORMATDESCRIPTOR ppfd )
{
    FIXME(gdi, "(%d,%d,%d,%p): stub\n",hdc,iPixelFormat,nBytes,ppfd);
    ppfd->nSize = nBytes;
    ppfd->nVersion = 1;
    return 3;
}


/******************************************************************************
 * SwapBuffers [GDI32.354]
 * Exchanges front and back buffers of window
 *
 * PARAMS
 *    hdc [I] Device context whose buffers get swapped
 *
 * RETURNS STD
 */
BOOL WINAPI SwapBuffers( HDC hdc )
{
    FIXME(gdi, "(%d): stub\n",hdc);
    return TRUE;
}


/***********************************************************************
 *           PaintRgn16    (GDI.43)
 */
BOOL16 WINAPI PaintRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return PaintRgn( hdc, hrgn );
}


/***********************************************************************
 *           PaintRgn32    (GDI32.259)
 */
BOOL WINAPI PaintRgn( HDC hdc, HRGN hrgn )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPaintRgn &&
	   dc->funcs->pPaintRgn(dc,hrgn);
}


/***********************************************************************
 *           FillRgn16    (GDI.40)
 */
BOOL16 WINAPI FillRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush )
{
    return FillRgn( hdc, hrgn, hbrush );
}

    
/***********************************************************************
 *           FillRgn32    (GDI32.101)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    BOOL retval;
    HBRUSH prevBrush = SelectObject( hdc, hbrush );
    if (!prevBrush) return FALSE;
    retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    return retval;
}


/***********************************************************************
 *           FrameRgn16     (GDI.41)
 */
BOOL16 WINAPI FrameRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush,
                          INT16 nWidth, INT16 nHeight )
{
    return FrameRgn( hdc, hrgn, hbrush, nWidth, nHeight );
}


/***********************************************************************
 *           FrameRgn32     (GDI32.105)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush,
                          INT nWidth, INT nHeight )
{
    HRGN tmp = CreateRectRgn( 0, 0, 0, 0 );
    if(!REGION_FrameRgn( tmp, hrgn, nWidth, nHeight )) return FALSE;
    FillRgn( hdc, tmp, hbrush );
    DeleteObject( tmp );
    return TRUE;
}


/***********************************************************************
 *           InvertRgn16    (GDI.42)
 */
BOOL16 WINAPI InvertRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return InvertRgn( hdc, hrgn );
}


/***********************************************************************
 *           InvertRgn32    (GDI32.246)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    HBRUSH prevBrush = SelectObject( hdc, GetStockObject(BLACK_BRUSH) );
    INT prevROP = SetROP2( hdc, R2_NOT );
    BOOL retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    SetROP2( hdc, prevROP );
    return retval;
}


/***********************************************************************
 *           DrawFocusRect16    (USER.466)
 */
void WINAPI DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT rect32;
    CONV_RECT16TO32( rc, &rect32 );
    DrawFocusRect( hdc, &rect32 );
}


/***********************************************************************
 *           DrawFocusRect32    (USER32.156)
 *
 * FIXME: PatBlt(PATINVERT) with background brush.
 */
BOOL WINAPI DrawFocusRect( HDC hdc, const RECT* rc )
{
    HBRUSH hOldBrush;
    HPEN hOldPen, hNewPen;
    INT oldDrawMode, oldBkMode;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
       SetLastError( ERROR_INVALID_HANDLE );
       return FALSE;
    }

    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    hNewPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_WINDOWTEXT));
    hOldPen = SelectObject(hdc, hNewPen);
    oldDrawMode = SetROP2(hdc, R2_XORPEN);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);

    Rectangle(hdc, rc->left, rc->top, rc->right, rc->bottom);

    SetBkMode(hdc, oldBkMode);
    SetROP2(hdc, oldDrawMode);
    SelectObject(hdc, hOldPen);
    DeleteObject(hNewPen);
    SelectObject(hdc, hOldBrush);

    return TRUE;
}


/**********************************************************************
 *          Polyline16  (GDI.37)
 */
BOOL16 WINAPI Polyline16( HDC16 hdc, const POINT16* pt, INT16 count )
{
    register int i;
    BOOL16 ret;
    LPPOINT pt32 = (LPPOINT)HeapAlloc( GetProcessHeap(), 0,
                                           count*sizeof(POINT) );

    if (!pt32) return FALSE;
    for (i=count;i--;) CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    ret = Polyline(hdc,pt32,count);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/**********************************************************************
 *          Polyline32   (GDI32.276)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT* pt, INT count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyline &&
    	   dc->funcs->pPolyline(dc,pt,count);
}

/**********************************************************************
 *          PolylineTo32   (GDI32.277)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT* pt, DWORD cCount )
{
    POINT *pts = HeapAlloc( GetProcessHeap(), 0,
			      sizeof(POINT) * (cCount + 1) );
    if(!pts) return FALSE;

    /* Get the current point */
    MoveToEx( hdc, 0, 0, pts);

    /* Add in the other points */
    memcpy( pts + 1, pt, sizeof(POINT) * cCount );

    /* Draw the lines */
    Polyline( hdc, pts, cCount + 1 );

    /* Move to last point */
    MoveToEx( hdc, (pts + cCount)->x, (pts + cCount)->y, NULL );

    HeapFree( GetProcessHeap(), 0, pts );
    return TRUE;
}

/**********************************************************************
 *          Polygon16  (GDI.36)
 */
BOOL16 WINAPI Polygon16( HDC16 hdc, const POINT16* pt, INT16 count )
{
    register int i;
    BOOL ret;
    LPPOINT pt32 = (LPPOINT)HeapAlloc( GetProcessHeap(), 0,
                                           count*sizeof(POINT) );

    if (!pt32) return FALSE;
    for (i=count;i--;) CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    ret = Polygon(hdc,pt32,count);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/**********************************************************************
 *          Polygon32  (GDI32.275)
 */
BOOL WINAPI Polygon( HDC hdc, const POINT* pt, INT count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolygon &&
    	   dc->funcs->pPolygon(dc,pt,count);
}


/**********************************************************************
 *          PolyPolygon16  (GDI.450)
 */
BOOL16 WINAPI PolyPolygon16( HDC16 hdc, const POINT16* pt, const INT16* counts,
                             UINT16 polygons )
{
    int		i,nrpts;
    LPPOINT	pt32;
    LPINT	counts32;
    BOOL16	ret;

    nrpts=0;
    for (i=polygons;i--;)
    	nrpts+=counts[i];
    pt32 = (LPPOINT)HEAP_xalloc( GetProcessHeap(), 0, sizeof(POINT)*nrpts);
    for (i=nrpts;i--;)
    	CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    counts32 = (LPINT)HEAP_xalloc( GetProcessHeap(), 0,
                                     polygons*sizeof(INT) );
    for (i=polygons;i--;) counts32[i]=counts[i];
   
    ret = PolyPolygon(hdc,pt32,counts32,polygons);
    HeapFree( GetProcessHeap(), 0, counts32 );
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}

/**********************************************************************
 *          PolyPolygon32  (GDI.450)
 */
BOOL WINAPI PolyPolygon( HDC hdc, const POINT* pt, const INT* counts,
                             UINT polygons )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyPolygon &&
    	   dc->funcs->pPolyPolygon(dc,pt,counts,polygons);
}

/**********************************************************************
 *          PolyPolyline  (GDI32.272)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT* pt, const DWORD* counts,
                            DWORD polylines )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyPolyline &&
    	   dc->funcs->pPolyPolyline(dc,pt,counts,polylines);
}

/**********************************************************************
 *          ExtFloodFill16   (GDI.372)
 */
BOOL16 WINAPI ExtFloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color,
                              UINT16 fillType )
{
    return ExtFloodFill( hdc, x, y, color, fillType );
}


/**********************************************************************
 *          ExtFloodFill32   (GDI32.96)
 */
BOOL WINAPI ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                              UINT fillType )
{
    DC *dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pExtFloodFill &&
	   dc->funcs->pExtFloodFill(dc,x,y,color,fillType);
}


/**********************************************************************
 *          FloodFill16   (GDI.25)
 */
BOOL16 WINAPI FloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          FloodFill32   (GDI32.104)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          DrawAnimatedRects16  (USER.448)
 */
BOOL16 WINAPI DrawAnimatedRects16( HWND16 hwnd, INT16 idAni,
                                   const RECT16* lprcFrom,
                                   const RECT16* lprcTo )
{
    RECT rcFrom32, rcTo32;

    rcFrom32.left	= (INT)lprcFrom->left;
    rcFrom32.top	= (INT)lprcFrom->top;
    rcFrom32.right	= (INT)lprcFrom->right;
    rcFrom32.bottom	= (INT)lprcFrom->bottom;

    rcTo32.left		= (INT)lprcTo->left;
    rcTo32.top		= (INT)lprcTo->top;
    rcTo32.right	= (INT)lprcTo->right;
    rcTo32.bottom	= (INT)lprcTo->bottom;

    return DrawAnimatedRects((HWND)hwnd, (INT)idAni, &rcFrom32, &rcTo32);
}


/**********************************************************************
 *          DrawAnimatedRects32  (USER32.153)
 */
BOOL WINAPI DrawAnimatedRects( HWND hwnd, int idAni,
                                   const RECT* lprcFrom,
                                   const RECT* lprcTo )
{
    FIXME(gdi,"(0x%x,%d,%p,%p): stub\n",hwnd,idAni,lprcFrom,lprcTo);
    return TRUE;
}


/**********************************************************************
 *          PAINTING_DrawStateJam
 *
 * Jams in the requested type in the dc
 */
static BOOL PAINTING_DrawStateJam(HDC hdc, UINT opcode,
                                    DRAWSTATEPROC func, LPARAM lp, WPARAM wp, 
                                    LPRECT rc, UINT dtflags,
                                    BOOL unicode, BOOL _32bit)
{
    HDC memdc;
    HBITMAP hbmsave;
    BOOL retval;
    INT cx = rc->right - rc->left;
    INT cy = rc->bottom - rc->top;
    
    switch(opcode)
    {
    case DST_TEXT:
    case DST_PREFIXTEXT:
        if(unicode)
            return DrawTextW(hdc, (LPWSTR)lp, (INT)wp, rc, dtflags);
        else if(_32bit)
            return DrawTextA(hdc, (LPSTR)lp, (INT)wp, rc, dtflags);
        else
            return DrawTextA(hdc, (LPSTR)PTR_SEG_TO_LIN(lp), (INT)wp, rc, dtflags);

    case DST_ICON:
        return DrawIcon(hdc, rc->left, rc->top, (HICON)lp);

    case DST_BITMAP:
        memdc = CreateCompatibleDC(hdc);
        if(!memdc) return FALSE;
        hbmsave = (HBITMAP)SelectObject(memdc, (HBITMAP)lp);
        if(!hbmsave) 
        {
            DeleteDC(memdc);
            return FALSE;
        }
        retval = BitBlt(hdc, rc->left, rc->top, cx, cy, memdc, 0, 0, SRCCOPY);
        SelectObject(memdc, hbmsave);
        DeleteDC(memdc);
        return retval;
            
    case DST_COMPLEX:
        if(func)
            if(_32bit)
                return func(hdc, lp, wp, cx, cy);
            else
                return (BOOL)((DRAWSTATEPROC16)func)((HDC16)hdc, (LPARAM)lp, (WPARAM16)wp, (INT16)cx, (INT16)cy);
        else
            return FALSE;
    }
    return FALSE;
}

/**********************************************************************
 *      PAINTING_DrawState32()
 */
static BOOL PAINTING_DrawState(HDC hdc, HBRUSH hbr, 
                                   DRAWSTATEPROC func, LPARAM lp, WPARAM wp,
                                   INT x, INT y, INT cx, INT cy, 
                                   UINT flags, BOOL unicode, BOOL _32bit)
{
    HBITMAP hbm, hbmsave;
    HFONT hfsave;
    HBRUSH hbsave;
    HDC memdc;
    RECT rc;
    UINT dtflags = DT_NOCLIP;
    COLORREF fg, bg;
    UINT opcode = flags & 0xf;
    INT len = wp;
    BOOL retval, tmp;

    if((opcode == DST_TEXT || opcode == DST_PREFIXTEXT) && !len)    /* The string is '\0' terminated */
    {
        if(unicode)
            len = lstrlenW((LPWSTR)lp);
        else if(_32bit)
            len = lstrlenA((LPSTR)lp);
        else
            len = lstrlenA((LPSTR)PTR_SEG_TO_LIN(lp));
    }

    /* Find out what size the image has if not given by caller */
    if(!cx || !cy)
    {
        SIZE s;
        CURSORICONINFO *ici;
        BITMAPOBJ *bmp;

        switch(opcode)
        {
        case DST_TEXT:
        case DST_PREFIXTEXT:
            if(unicode)
                retval = GetTextExtentPoint32W(hdc, (LPWSTR)lp, len, &s);
            else if(_32bit)
                retval = GetTextExtentPoint32A(hdc, (LPSTR)lp, len, &s);
            else
                retval = GetTextExtentPoint32A(hdc, PTR_SEG_TO_LIN(lp), len, &s);
            if(!retval) return FALSE;
            break;
            
        case DST_ICON:
            ici = (CURSORICONINFO *)GlobalLock16((HGLOBAL16)lp);
            if(!ici) return FALSE;
            s.cx = ici->nWidth;
            s.cy = ici->nHeight;
            GlobalUnlock16((HGLOBAL16)lp);
            break;            

        case DST_BITMAP:
            bmp = (BITMAPOBJ *)GDI_GetObjPtr((HBITMAP16)lp, BITMAP_MAGIC);
            if(!bmp) return FALSE;
            s.cx = bmp->bitmap.bmWidth;
            s.cy = bmp->bitmap.bmHeight;
            break;
            
        case DST_COMPLEX: /* cx and cy must be set in this mode */
            return FALSE;
	}
	            
        if(!cx) cx = s.cx;
        if(!cy) cy = s.cy;
    }

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + cx;
    rc.bottom = y + cy;

    if(flags & DSS_RIGHT)    /* This one is not documented in the win32.hlp file */
        dtflags |= DT_RIGHT;
    if(opcode == DST_TEXT)
        dtflags |= DT_NOPREFIX;

    /* For DSS_NORMAL we just jam in the image and return */
    if((flags & 0x7ff0) == DSS_NORMAL)
    {
        return PAINTING_DrawStateJam(hdc, opcode, func, lp, len, &rc, dtflags, unicode, _32bit);
    }

    /* For all other states we need to convert the image to B/W in a local bitmap */
    /* before it is displayed */
    fg = SetTextColor(hdc, RGB(0, 0, 0));
    bg = SetBkColor(hdc, RGB(255, 255, 255));
    hbm = (HBITMAP)NULL; hbmsave = (HBITMAP)NULL;
    memdc = (HDC)NULL; hbsave = (HBRUSH)NULL;
    retval = FALSE; /* assume failure */
    
    /* From here on we must use "goto cleanup" when something goes wrong */
    hbm     = CreateBitmap(cx, cy, 1, 1, NULL);
    if(!hbm) goto cleanup;
    memdc   = CreateCompatibleDC(hdc);
    if(!memdc) goto cleanup;
    hbmsave = (HBITMAP)SelectObject(memdc, hbm);
    if(!hbmsave) goto cleanup;
    rc.left = rc.top = 0;
    rc.right = cx;
    rc.bottom = cy;
    if(!FillRect(memdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH))) goto cleanup;
    SetBkColor(memdc, RGB(255, 255, 255));
    SetTextColor(memdc, RGB(0, 0, 0));
    hfsave  = (HFONT)SelectObject(memdc, GetCurrentObject(hdc, OBJ_FONT));
    if(!hfsave && (opcode == DST_TEXT || opcode == DST_PREFIXTEXT)) goto cleanup;
    tmp = PAINTING_DrawStateJam(memdc, opcode, func, lp, len, &rc, dtflags, unicode, _32bit);
    if(hfsave) SelectObject(memdc, hfsave);
    if(!tmp) goto cleanup;
    
    /* These states cause the image to be dithered */
    if(flags & (DSS_UNION|DSS_DISABLED))
    {
        hbsave = (HBRUSH)SelectObject(memdc, CACHE_GetPattern55AABrush());
        if(!hbsave) goto cleanup;
        tmp = PatBlt(memdc, 0, 0, cx, cy, 0x00FA0089);
        if(hbsave) SelectObject(memdc, hbsave);
        if(!tmp) goto cleanup;
    }

    hbsave = (HBRUSH)SelectObject(hdc, hbr ? hbr : GetStockObject(WHITE_BRUSH));
    if(!hbsave) goto cleanup;
    
    if(!BitBlt(hdc, x, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
    
    /* DSS_DEFAULT makes the image boldface */
    if(flags & DSS_DEFAULT)
    {
        if(!BitBlt(hdc, x+1, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
    }

    retval = TRUE; /* We succeeded */
    
cleanup:    
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);

    if(hbsave)  SelectObject(hdc, hbsave);
    if(hbmsave) SelectObject(memdc, hbmsave);
    if(hbm)     DeleteObject(hbm);
    if(memdc)   DeleteDC(memdc);

    return retval;
}

/**********************************************************************
 *      DrawState32A()   (USER32.162)
 */
BOOL WINAPI DrawStateA(HDC hdc, HBRUSH hbr,
                   DRAWSTATEPROC func, LPARAM ldata, WPARAM wdata,
                   INT x, INT y, INT cx, INT cy, UINT flags)
{
    return PAINTING_DrawState(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, FALSE, TRUE);
}

/**********************************************************************
 *      DrawState32W()   (USER32.163)
 */
BOOL WINAPI DrawStateW(HDC hdc, HBRUSH hbr,
                   DRAWSTATEPROC func, LPARAM ldata, WPARAM wdata,
                   INT x, INT y, INT cx, INT cy, UINT flags)
{
    return PAINTING_DrawState(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, TRUE, TRUE);
}

/**********************************************************************
 *      DrawState16()   (USER.449)
 */
BOOL16 WINAPI DrawState16(HDC16 hdc, HBRUSH16 hbr,
                   DRAWSTATEPROC16 func, LPARAM ldata, WPARAM16 wdata,
                   INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags)
{
    return PAINTING_DrawState(hdc, hbr, (DRAWSTATEPROC)func, ldata, wdata, x, y, cx, cy, flags, FALSE, FALSE);
}


/******************************************************************************
 * PolyBezier16 [GDI.502]
 */
BOOL16 WINAPI PolyBezier16( HDC16 hDc, const POINT16* lppt, INT16 cPoints )
{
    int i;
    BOOL16 ret;
    LPPOINT pt32 = (LPPOINT)HeapAlloc( GetProcessHeap(), 0,
                                           cPoints*sizeof(POINT) );
    if(!pt32) return FALSE;
    for (i=cPoints;i--;) CONV_POINT16TO32(&(lppt[i]),&(pt32[i]));
    ret= PolyBezier(hDc, pt32, cPoints);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}

/******************************************************************************
 * PolyBezierTo16 [GDI.503]
 */
BOOL16 WINAPI PolyBezierTo16( HDC16 hDc, const POINT16* lppt, INT16 cPoints )
{
    int i;
    BOOL16 ret;
    LPPOINT pt32 = (LPPOINT)HeapAlloc( GetProcessHeap(), 0,
                                           cPoints*sizeof(POINT) );
    if(!pt32) return FALSE;
    for (i=cPoints;i--;) CONV_POINT16TO32(&(lppt[i]),&(pt32[i]));
    ret= PolyBezierTo(hDc, pt32, cPoints);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}

/******************************************************************************
 * PolyBezier32 [GDI32.268]
 * Draws one or more Bezier curves
 *
 * PARAMS
 *    hDc     [I] Handle to device context
 *    lppt    [I] Pointer to endpoints and control points
 *    cPoints [I] Count of endpoints and control points
 *
 * RETURNS STD
 */
BOOL WINAPI PolyBezier( HDC hdc, const POINT* lppt, DWORD cPoints )
{
    DC * dc = DC_GetDCPtr( hdc );
    if(!dc) return FALSE;
    if(dc && PATH_IsPathOpen(dc->w.path))
        FIXME(gdi, "PATH_PolyBezier is not implemented!\n");
/*        if(!PATH_PolyBezier(hdc, x, y))
	   return FALSE; */
    return dc->funcs->pPolyBezier&&
    	   dc->funcs->pPolyBezier(dc, lppt[0], lppt+1, cPoints-1);
}

/******************************************************************************
 * PolyBezierTo32 [GDI32.269]
 * Draws one or more Bezier curves
 *
 * PARAMS
 *    hDc     [I] Handle to device context
 *    lppt    [I] Pointer to endpoints and control points
 *    cPoints [I] Count of endpoints and control points
 *
 * RETURNS STD
 */
BOOL WINAPI PolyBezierTo( HDC hdc, const POINT* lppt, DWORD cPoints )
{
    DC * dc = DC_GetDCPtr( hdc );
    POINT pt;
    BOOL ret;
    if(!dc) return FALSE;
    pt.x=dc->w.CursPosX;
    pt.y=dc->w.CursPosY;
    if(dc && PATH_IsPathOpen(dc->w.path))
        FIXME(gdi, "PATH_PolyBezierTo is not implemented!\n");
/*        if(!PATH_PolyBezier(hdc, x, y))
	   return FALSE; */
    ret= dc->funcs->pPolyBezier &&
    	   dc->funcs->pPolyBezier(dc, pt, lppt, cPoints);
    if( dc->funcs->pMoveToEx)
    	   dc->funcs->pMoveToEx(dc,lppt[cPoints].x,lppt[cPoints].y,&pt);
    return ret;
}

/***************************************************************
 *      AngleArc (GDI32.5)
 *
 */
BOOL WINAPI AngleArc(HDC hdc, INT x, INT y, DWORD dwRadius,
                       FLOAT eStartAngle, FLOAT eSweepAngle)
{
        FIXME(gdi,"AngleArc, stub\n");
        return 0;
}

/***************************************************************
 *      PolyDraw (GDI32.270)
 *
 */
BOOL WINAPI PolyDraw(HDC hdc, const POINT *lppt, const BYTE *lpbTypes,
                       DWORD cCount)
{
        FIXME(gdi,"PolyDraw, stub\n");
        return 0;
}

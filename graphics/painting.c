/*
 * Misc. graphics operations
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 */

#include <string.h>
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "monitor.h"
#include "cache.h"
#include "region.h"
#include "path.h"
#include "debugtools.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(gdi)


/***********************************************************************
 *           LineTo16    (GDI.19)
 */
BOOL16 WINAPI LineTo16( HDC16 hdc, INT16 x, INT16 y )
{
    return LineTo( hdc, x, y );
}


/***********************************************************************
 *           LineTo    (GDI32.249)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC * dc = DC_GetDCPtr( hdc );
    BOOL ret;

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        ret = PATH_LineTo(hdc, x, y);
    else
        ret = dc->funcs->pLineTo && dc->funcs->pLineTo(dc,x,y);
    if(ret) {
        dc->w.CursPosX = x;
        dc->w.CursPosY = y;
    }
    return ret;
}


/***********************************************************************
 *           MoveTo16    (GDI.20)
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
 *           MoveToEx    (GDI32.254)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    DC * dc = DC_GetDCPtr( hdc );

    if(!dc) return FALSE;

    if(pt) {
        pt->x = dc->w.CursPosX;
        pt->y = dc->w.CursPosY;
    }
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;

    if(PATH_IsPathOpen(dc->w.path))
        return PATH_MoveTo(hdc);

    if(dc->funcs->pMoveToEx)
        return dc->funcs->pMoveToEx(dc,x,y,pt);
    return TRUE;
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
 *           Arc    (GDI32.7)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right,
                     INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if(dc && PATH_IsPathOpen(dc->w.path))
        return PATH_Arc(hdc, left, top, right, bottom, xstart, ystart, xend,
	   yend);
    
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
    DC * dc = DC_GetDCPtr( hdc );

    if(!dc) return FALSE;

    if(dc->funcs->pArcTo)
        return dc->funcs->pArcTo( dc, left, top, right, bottom,
				  xstart, ystart, xend, yend );
    /* 
     * Else emulate it.
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
 *           Pie   (GDI32.262)
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
 *           Chord    (GDI32.14)
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
 *           Ellipse    (GDI32.75)
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
 *           Rectangle    (GDI32.283)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top,
                           INT right, INT bottom )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if(dc && PATH_IsPathOpen(dc->w.path))
        return PATH_Rectangle(hdc, left, top, right, bottom);

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
 *           RoundRect    (GDI32.291)
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
 *           SetPixel16    (GDI.31)
 */
COLORREF WINAPI SetPixel16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return SetPixel( hdc, x, y, color );
}


/***********************************************************************
 *           SetPixel    (GDI32.327)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc || !dc->funcs->pSetPixel) return 0;
    return dc->funcs->pSetPixel(dc,x,y,color);
}

/***********************************************************************
 *           SetPixelV    (GDI32.329)
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
 *           GetPixel    (GDI32.211)
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
    FIXME("(%d,%p): stub\n",hdc,ppfd);
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
    FIXME("(%d,%d,%p): stub\n",hdc,iPixelFormat,ppfd);
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
    FIXME("(%d): stub\n",hdc);
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
    FIXME("(%d,%d,%d,%p): stub\n",hdc,iPixelFormat,nBytes,ppfd);
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
    FIXME("(%d): stub\n",hdc);
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
 *           PaintRgn    (GDI32.259)
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
 *           FillRgn    (GDI32.101)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    BOOL retval;
    HBRUSH prevBrush;
    DC * dc = DC_GetDCPtr( hdc );

    if (!dc) return FALSE;
    if(dc->funcs->pFillRgn)
        return dc->funcs->pFillRgn(dc, hrgn, hbrush);

    prevBrush = SelectObject( hdc, hbrush );
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
 *           FrameRgn     (GDI32.105)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush,
                          INT nWidth, INT nHeight )
{
    HRGN tmp;
    DC *dc = DC_GetDCPtr( hdc );

    if(dc->funcs->pFrameRgn)
        return dc->funcs->pFrameRgn( dc, hrgn, hbrush, nWidth, nHeight );

    tmp = CreateRectRgn( 0, 0, 0, 0 );
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
 *           InvertRgn    (GDI32.246)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    HBRUSH prevBrush;
    INT prevROP;
    BOOL retval;
    DC *dc = DC_GetDCPtr( hdc );

    if(dc->funcs->pInvertRgn)
        return dc->funcs->pInvertRgn( dc, hrgn );

    prevBrush = SelectObject( hdc, GetStockObject(BLACK_BRUSH) );
    prevROP = SetROP2( hdc, R2_NOT );
    retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    SetROP2( hdc, prevROP );
    return retval;
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
 *          Polyline   (GDI32.276)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT* pt, INT count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyline &&
    	   dc->funcs->pPolyline(dc,pt,count);
}

/**********************************************************************
 *          PolylineTo   (GDI32.277)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT* pt, DWORD cCount )
{
    DC * dc = DC_GetDCPtr( hdc );
    BOOL ret;

    if(!dc) return FALSE;

    if(dc->funcs->pPolylineTo)
        ret = dc->funcs->pPolylineTo(dc, pt, cCount);

    else { /* do it using Polyline */
        POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				sizeof(POINT) * (cCount + 1) );
	if(!pts) return FALSE;

	pts[0].x = dc->w.CursPosX;
	pts[0].y = dc->w.CursPosY;
	memcpy( pts + 1, pt, sizeof(POINT) * cCount );
	ret = Polyline( hdc, pts, cCount + 1 );
	HeapFree( GetProcessHeap(), 0, pts );
    }
    if(ret) {
        dc->w.CursPosX = pt[cCount-1].x;
	dc->w.CursPosY = pt[cCount-1].y;
    }
    return ret;
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
 *          Polygon  (GDI32.275)
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
 *          PolyPolygon  (GDI.450)
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
 *          ExtFloodFill   (GDI32.96)
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
 *          FloodFill   (GDI32.104)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
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
 * PolyBezier [GDI32.268]
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

    return dc && dc->funcs->pPolyBezier &&
    	   dc->funcs->pPolyBezier(dc, lppt, cPoints);
}

/******************************************************************************
 * PolyBezierTo [GDI32.269]
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
    BOOL ret;

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        ret = PATH_PolyBezierTo(hdc, lppt, cPoints);
    else 
        ret = dc->funcs->pPolyBezierTo &&
	    dc->funcs->pPolyBezierTo(dc, lppt, cPoints);

    if(ret) {
        dc->w.CursPosX = lppt[cPoints-1].x;
        dc->w.CursPosY = lppt[cPoints-1].y;
    }
    return ret;
}

/***************************************************************
 *      AngleArc (GDI32.5)
 *
 */
BOOL WINAPI AngleArc(HDC hdc, INT x, INT y, DWORD dwRadius,
                       FLOAT eStartAngle, FLOAT eSweepAngle)
{
        FIXME("AngleArc, stub\n");
        return 0;
}

/***************************************************************
 *      PolyDraw (GDI32.270)
 *
 */
BOOL WINAPI PolyDraw(HDC hdc, const POINT *lppt, const BYTE *lpbTypes,
                       DWORD cCount)
{
        FIXME("PolyDraw, stub\n");
        return 0;
}

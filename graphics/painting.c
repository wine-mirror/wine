/*
 * GDI drawing functions.
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 *           1999 Huw D M Davies
 */

#include <string.h>
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "cache.h"
#include "region.h"
#include "path.h"
#include "debugtools.h"
#include "winerror.h"
#include "windef.h"
#include "wingdi.h"

DEFAULT_DEBUG_CHANNEL(gdi);


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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        return PATH_Arc(hdc, left, top, right, bottom, xstart, ystart, xend,
	   yend);
    
    return dc->funcs->pArc &&
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path)) {
        FIXME("-> Path: stub\n");
	return FALSE;
    }

    return dc->funcs->pPie &&
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path)) {
        FIXME("-> Path: stub\n");
	return FALSE;
    }
  
    return dc->funcs->pChord &&
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path)) {
        FIXME("-> Path: stub\n");
	return FALSE;
    }
  
    return dc->funcs->pEllipse &&
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        return PATH_Rectangle(hdc, left, top, right, bottom);

    return dc->funcs->pRectangle &&
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
    DC * dc = DC_GetDCPtr( hdc );
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path)) {
        FIXME("-> Path: stub\n");
	return FALSE;
    }

    return dc->funcs->pRoundRect &&
      dc->funcs->pRoundRect(dc,left,top,right,bottom,ell_width,ell_height);
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
INT WINAPI ChoosePixelFormat( HDC hdc, const LPPIXELFORMATDESCRIPTOR ppfd )
{
  DC * dc = DC_GetDCPtr( hdc );

  TRACE("(%08x,%p)\n",hdc,ppfd);
  
  if (dc == NULL) return 0;
  if (dc->funcs->pChoosePixelFormat == NULL) {
    FIXME(" :stub\n");
    return 0;
  }
  
  return dc->funcs->pChoosePixelFormat(dc,ppfd);
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
BOOL WINAPI SetPixelFormat( HDC hdc, INT iPixelFormat,
                            const PIXELFORMATDESCRIPTOR *ppfd)
{
  DC * dc = DC_GetDCPtr( hdc );

  TRACE("(%d,%d,%p)\n",hdc,iPixelFormat,ppfd);

  if (dc == NULL) return 0;
  if (dc->funcs->pSetPixelFormat == NULL) {
    FIXME(" :stub\n");
    return 0;
  }
  return dc->funcs->pSetPixelFormat(dc,iPixelFormat,ppfd);
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
INT WINAPI GetPixelFormat( HDC hdc )
{
  DC * dc = DC_GetDCPtr( hdc );

  TRACE("(%08x)\n",hdc);

  if (dc == NULL) return 0;
  if (dc->funcs->pGetPixelFormat == NULL) {
    FIXME(" :stub\n");
    return 0;
  }
  return dc->funcs->pGetPixelFormat(dc);
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
INT WINAPI DescribePixelFormat( HDC hdc, INT iPixelFormat, UINT nBytes,
                                LPPIXELFORMATDESCRIPTOR ppfd )
{
  DC * dc = DC_GetDCPtr( hdc );

  TRACE("(%08x,%d,%d,%p): stub\n",hdc,iPixelFormat,nBytes,ppfd);

  if (dc == NULL) return 0;
  if (dc->funcs->pDescribePixelFormat == NULL) {
    FIXME(" :stub\n");
    ppfd->nSize = nBytes;
    ppfd->nVersion = 1;
    return 3;
  }
  return dc->funcs->pDescribePixelFormat(dc,iPixelFormat,nBytes,ppfd);
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
  DC * dc = DC_GetDCPtr( hdc );

  TRACE("(%08x)\n",hdc);

  if (dc == NULL) return 0;
  if (dc->funcs->pSwapBuffers == NULL) {
    FIXME(" :stub\n");
    return TRUE;
  }
  return dc->funcs->pSwapBuffers(dc);
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        return PATH_Polyline(hdc, pt, count);

    return dc->funcs->pPolyline &&
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

    if(PATH_IsPathOpen(dc->w.path))
        ret = PATH_PolylineTo(hdc, pt, cCount);

    else if(dc->funcs->pPolylineTo)
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
	return PATH_Polygon(hdc, pt, count);

    return dc->funcs->pPolygon &&
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
    pt32 = (LPPOINT)HeapAlloc( GetProcessHeap(), 0, sizeof(POINT)*nrpts);
    if(pt32 == NULL) return FALSE;
    for (i=nrpts;i--;)
    	CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    counts32 = (LPINT)HeapAlloc( GetProcessHeap(), 0, polygons*sizeof(INT) );
    if(counts32 == NULL) {
	HeapFree( GetProcessHeap(), 0, pt32 );
	return FALSE;
    }
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        return PATH_PolyPolygon(hdc, pt, counts, polygons);

    return dc->funcs->pPolyPolygon &&
    	   dc->funcs->pPolyPolygon(dc,pt,counts,polygons);
}

/**********************************************************************
 *          PolyPolyline  (GDI32.272)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT* pt, const DWORD* counts,
                            DWORD polylines )
{
    DC * dc = DC_GetDCPtr( hdc );
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
        return PATH_PolyPolyline(hdc, pt, counts, polylines);

    return dc->funcs->pPolyPolyline &&
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
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->w.path))
	return PATH_PolyBezier(hdc, lppt, cPoints);

    if(dc->funcs->pPolyBezier)
        return dc->funcs->pPolyBezier(dc, lppt, cPoints);

    /* We'll convert it into line segments and draw them using Polyline */
    {
        POINT *Pts;
	INT nOut;
	BOOL ret;

	Pts = GDI_Bezier( lppt, cPoints, &nOut );
	if(!Pts) return FALSE;
	TRACE("Pts = %p, no = %d\n", Pts, nOut);
	ret = Polyline( dc->hSelf, Pts, nOut );
	HeapFree( GetProcessHeap(), 0, Pts );
	return ret;
    }
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
    else if(dc->funcs->pPolyBezierTo)
        ret = dc->funcs->pPolyBezierTo(dc, lppt, cPoints);
    else { /* We'll do it using PolyBezier */
        POINT *pt;
	pt = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * (cPoints + 1) );
	if(!pt) return FALSE;
	pt[0].x = dc->w.CursPosX;
	pt[0].y = dc->w.CursPosY;
	memcpy(pt + 1, lppt, sizeof(POINT) * cPoints);
	ret = PolyBezier(dc->hSelf, pt, cPoints+1);
	HeapFree( GetProcessHeap(), 0, pt );
    }
    if(ret) {
        dc->w.CursPosX = lppt[cPoints-1].x;
        dc->w.CursPosY = lppt[cPoints-1].y;
    }
    return ret;
}

/***********************************************************************
 *      AngleArc (GDI32.5)
 *
 */
BOOL WINAPI AngleArc(HDC hdc, INT x, INT y, DWORD dwRadius,
                       FLOAT eStartAngle, FLOAT eSweepAngle)
{
        FIXME("AngleArc, stub\n");
        return 0;
}

/***********************************************************************
 *      PolyDraw (GDI32.270)
 *
 */
BOOL WINAPI PolyDraw(HDC hdc, const POINT *lppt, const BYTE *lpbTypes,
                       DWORD cCount)
{
        FIXME("PolyDraw, stub\n");
        return 0;
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
#define BEZIER_INITBUFSIZE    (150)

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
    if(abs(dy)<=abs(dx)){/* shallow line */
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
    
/* Helper for GDI_Bezier.
 * Just handles one Bezier, so Points should point to four POINTs
 */
static void GDI_InternalBezier( POINT *Points, POINT **PtsOut, INT *dwOut,
				INT *nPtsOut, INT level )
{
    if(*nPtsOut == *dwOut) {
        *dwOut *= 2;
	*PtsOut = HeapReAlloc( GetProcessHeap(), 0, *PtsOut,
			       *dwOut * sizeof(POINT) );
    }

    if(!level || BezierCheck(level, Points)) {
        if(*nPtsOut == 0) {
            (*PtsOut)[0].x = BEZIERSHIFTDOWN(Points[0].x);
            (*PtsOut)[0].y = BEZIERSHIFTDOWN(Points[0].y);
            *nPtsOut = 1;
        }
	(*PtsOut)[*nPtsOut].x = BEZIERSHIFTDOWN(Points[3].x);
        (*PtsOut)[*nPtsOut].y = BEZIERSHIFTDOWN(Points[3].y);
        (*nPtsOut) ++;
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
        GDI_InternalBezier(Points, PtsOut, dwOut, nPtsOut, level-1);
        GDI_InternalBezier(Points2, PtsOut, dwOut, nPtsOut, level-1);
    }
}


    
/***********************************************************************
 *           GDI_Bezier   [INTERNAL]
 *   Calculate line segments that approximate -what microsoft calls- a bezier
 *   curve.
 *   The routine recursively divides the curve in two parts until a straight
 *   line can be drawn
 *
 *  PARAMS
 *
 *  Points  [I] Ptr to count POINTs which are the end and control points
 *              of the set of Bezier curves to flatten.
 *  count   [I] Number of Points.  Must be 3n+1.
 *  nPtsOut [O] Will contain no of points that have been produced (i.e. no. of
 *              lines+1).
 *   
 *  RETURNS
 *
 *  Ptr to an array of POINTs that contain the lines that approximinate the
 *  Beziers.  The array is allocated on the process heap and it is the caller's
 *  responsibility to HeapFree it. [this is not a particularly nice interface
 *  but since we can't know in advance how many points will generate, the
 *  alternative would be to call the function twice, once to determine the size
 *  and a second time to do the work - I decided this was too much of a pain].
 */
POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut )
{
    POINT *out;
    INT Bezier, dwOut = BEZIER_INITBUFSIZE, i;

    if((count - 1) % 3 != 0) {
        ERR("Invalid no. of points\n");
	return NULL;
    }
    *nPtsOut = 0;
    out = HeapAlloc( GetProcessHeap(), 0, dwOut * sizeof(POINT));
    for(Bezier = 0; Bezier < (count-1)/3; Bezier++) {
	POINT ptBuf[4];
	memcpy(ptBuf, Points + Bezier * 3, sizeof(POINT) * 4);
	for(i = 0; i < 4; i++) {
	    ptBuf[i].x = BEZIERSHIFTUP(ptBuf[i].x);
	    ptBuf[i].y = BEZIERSHIFTUP(ptBuf[i].y);
	}
        GDI_InternalBezier( ptBuf, &out, &dwOut, nPtsOut, BEZIERMAXDEPTH );
    }
    TRACE("Produced %d points\n", *nPtsOut);
    return out;
}

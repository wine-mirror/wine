/*
 * GDI drawing functions.
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 *           1999 Huw D M Davies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "wingdi.h"
#include "winerror.h"
#include "gdi.h"
#include "bitmap.h"
#include "region.h"
#include "path.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


/***********************************************************************
 *           LineTo    (GDI.19)
 */
BOOL16 WINAPI LineTo16( HDC16 hdc, INT16 x, INT16 y )
{
    return LineTo( hdc, x, y );
}


/***********************************************************************
 *           LineTo    (GDI32.@)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC * dc = DC_GetDCUpdate( hdc );
    BOOL ret;

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
        ret = PATH_LineTo(dc, x, y);
    else
        ret = dc->funcs->pLineTo && dc->funcs->pLineTo(dc,x,y);
    if(ret) {
        dc->CursPosX = x;
        dc->CursPosY = y;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           MoveTo    (GDI.20)
 */
DWORD WINAPI MoveTo16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;

    if (!MoveToEx( (HDC)hdc, x, y, &pt )) return 0;
    return MAKELONG(pt.x,pt.y);
}


/***********************************************************************
 *           MoveToEx    (GDI.483)
 */
BOOL16 WINAPI MoveToEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;

    if (!MoveToEx( (HDC)hdc, (INT)x, (INT)y, &pt32 )) return FALSE;
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return TRUE;
}


/***********************************************************************
 *           MoveToEx    (GDI32.@)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );

    if(!dc) return FALSE;

    if(pt) {
        pt->x = dc->CursPosX;
        pt->y = dc->CursPosY;
    }
    dc->CursPosX = x;
    dc->CursPosY = y;

    if(PATH_IsPathOpen(dc->path)) ret = PATH_MoveTo(dc);
    else if (dc->funcs->pMoveTo) ret = dc->funcs->pMoveTo(dc,x,y);
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           Arc    (GDI.23)
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
 *           Arc    (GDI32.@)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right,
                     INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
    if(PATH_IsPathOpen(dc->path))
            ret = PATH_Arc(dc, left, top, right, bottom, xstart, ystart, xend, yend,0);
        else if (dc->funcs->pArc)
            ret = dc->funcs->pArc(dc,left,top,right,bottom,xstart,ystart,xend,yend);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *           ArcTo    (GDI32.@)
 */
BOOL WINAPI ArcTo( HDC hdc, 
                     INT left,   INT top, 
                     INT right,  INT bottom,
                     INT xstart, INT ystart,
                     INT xend,   INT yend )
{
    BOOL result;
    DC * dc = DC_GetDCUpdate( hdc );
    if(!dc) return FALSE;

    if(dc->funcs->pArcTo)
    {
        result = dc->funcs->pArcTo( dc, left, top, right, bottom,
				  xstart, ystart, xend, yend );
        GDI_ReleaseObj( hdc );
        return result;
    }
    GDI_ReleaseObj( hdc );
    /* 
     * Else emulate it.
     * According to the documentation, a line is drawn from the current
     * position to the starting point of the arc.
     */
    LineTo(hdc, xstart, ystart);
    /*
     * Then the arc is drawn.
     */
    result = Arc(hdc, left, top, right, bottom, xstart, ystart, xend, yend);
    /*
     * If no error occurred, the current position is moved to the ending
     * point of the arc.
     */
    if (result) MoveToEx(hdc, xend, yend, NULL);
    return result;
}

/***********************************************************************
 *           Pie    (GDI.26)
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
 *           Pie   (GDI32.@)
 */
BOOL WINAPI Pie( HDC hdc, INT left, INT top,
                     INT right, INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
        ret = PATH_Arc(dc,left,top,right,bottom,xstart,ystart,xend,yend,2); 
    else if(dc->funcs->pPie)
        ret = dc->funcs->pPie(dc,left,top,right,bottom,xstart,ystart,xend,yend);

    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           Chord    (GDI.348)
 */
BOOL16 WINAPI Chord16( HDC16 hdc, INT16 left, INT16 top,
                       INT16 right, INT16 bottom, INT16 xstart, INT16 ystart,
                       INT16 xend, INT16 yend )
{
    return Chord( hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           Chord    (GDI32.@)
 */
BOOL WINAPI Chord( HDC hdc, INT left, INT top,
                       INT right, INT bottom, INT xstart, INT ystart,
                       INT xend, INT yend )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
	ret = PATH_Arc(dc,left,top,right,bottom,xstart,ystart,xend,yend,1);
    else if(dc->funcs->pChord)
        ret = dc->funcs->pChord(dc,left,top,right,bottom,xstart,ystart,xend,yend);

    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           Ellipse    (GDI.24)
 */
BOOL16 WINAPI Ellipse16( HDC16 hdc, INT16 left, INT16 top,
                         INT16 right, INT16 bottom )
{
    return Ellipse( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Ellipse    (GDI32.@)
 */
BOOL WINAPI Ellipse( HDC hdc, INT left, INT top,
                         INT right, INT bottom )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
	ret = PATH_Ellipse(dc,left,top,right,bottom);
    else if (dc->funcs->pEllipse)
        ret = dc->funcs->pEllipse(dc,left,top,right,bottom);

    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           Rectangle    (GDI.27)
 */
BOOL16 WINAPI Rectangle16( HDC16 hdc, INT16 left, INT16 top,
                           INT16 right, INT16 bottom )
{
    return Rectangle( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Rectangle    (GDI32.@)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top,
                           INT right, INT bottom )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {  
    if(PATH_IsPathOpen(dc->path))
            ret = PATH_Rectangle(dc, left, top, right, bottom);
        else if (dc->funcs->pRectangle)
            ret = dc->funcs->pRectangle(dc,left,top,right,bottom);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           RoundRect    (GDI.28)
 */
BOOL16 WINAPI RoundRect16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                           INT16 bottom, INT16 ell_width, INT16 ell_height )
{
    return RoundRect( hdc, left, top, right, bottom, ell_width, ell_height );
}


/***********************************************************************
 *           RoundRect    (GDI32.@)
 */
BOOL WINAPI RoundRect( HDC hdc, INT left, INT top, INT right,
                           INT bottom, INT ell_width, INT ell_height )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCUpdate( hdc );

    if (dc)
    {
        if(PATH_IsPathOpen(dc->path))
	    ret = PATH_RoundRect(dc,left,top,right,bottom,ell_width,ell_height);
        else if (dc->funcs->pRoundRect)
            ret = dc->funcs->pRoundRect(dc,left,top,right,bottom,ell_width,ell_height);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *           SetPixel    (GDI.31)
 */
COLORREF WINAPI SetPixel16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return SetPixel( hdc, x, y, color );
}


/***********************************************************************
 *           SetPixel    (GDI32.@)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    COLORREF ret = 0;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (dc->funcs->pSetPixel) ret = dc->funcs->pSetPixel(dc,x,y,color);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *           SetPixelV    (GDI32.@)
 */
BOOL WINAPI SetPixelV( HDC hdc, INT x, INT y, COLORREF color )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (dc->funcs->pSetPixel)
        {
            dc->funcs->pSetPixel(dc,x,y,color);
            ret = TRUE;
        }
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *           GetPixel    (GDI.83)
 */
COLORREF WINAPI GetPixel16( HDC16 hdc, INT16 x, INT16 y )
{
    return GetPixel( hdc, x, y );
}


/***********************************************************************
 *           GetPixel    (GDI32.@)
 */
COLORREF WINAPI GetPixel( HDC hdc, INT x, INT y )
{
    COLORREF ret = CLR_INVALID;
    DC * dc = DC_GetDCUpdate( hdc );

    if (dc)
    {
    /* FIXME: should this be in the graphics driver? */
        if (PtVisible( hdc, x, y ))
        {
            if (dc->funcs->pGetPixel) ret = dc->funcs->pGetPixel(dc,x,y);
        }
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/******************************************************************************
 * ChoosePixelFormat [GDI32.@]
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
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%08x,%p)\n",hdc,ppfd);
  
    if (!dc) return 0;

    if (!dc->funcs->pChoosePixelFormat) FIXME(" :stub\n");
    else ret = dc->funcs->pChoosePixelFormat(dc,ppfd);

    GDI_ReleaseObj( hdc );  
    return ret;
}


/******************************************************************************
 * SetPixelFormat [GDI32.@]
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
    INT bRet = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%d,%d,%p)\n",hdc,iPixelFormat,ppfd);

    if (!dc) return 0;

    if (!dc->funcs->pSetPixelFormat) FIXME(" :stub\n");
    else bRet = dc->funcs->pSetPixelFormat(dc,iPixelFormat,ppfd);

    GDI_ReleaseObj( hdc );  
    return bRet;
}


/******************************************************************************
 * GetPixelFormat [GDI32.@]
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
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%08x)\n",hdc);

    if (!dc) return 0;

    if (!dc->funcs->pGetPixelFormat) FIXME(" :stub\n");
    else ret = dc->funcs->pGetPixelFormat(dc);

    GDI_ReleaseObj( hdc );  
    return ret;
}


/******************************************************************************
 * DescribePixelFormat [GDI32.@]
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
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%08x,%d,%d,%p): stub\n",hdc,iPixelFormat,nBytes,ppfd);

    if (!dc) return 0;

    if (!dc->funcs->pDescribePixelFormat)
    {
        FIXME(" :stub\n");
        ppfd->nSize = nBytes;
        ppfd->nVersion = 1;
	ret = 3;
    }
    else ret = dc->funcs->pDescribePixelFormat(dc,iPixelFormat,nBytes,ppfd);

    GDI_ReleaseObj( hdc );  
    return ret;
}


/******************************************************************************
 * SwapBuffers [GDI32.@]
 * Exchanges front and back buffers of window
 *
 * PARAMS
 *    hdc [I] Device context whose buffers get swapped
 *
 * RETURNS STD
 */
BOOL WINAPI SwapBuffers( HDC hdc )
{
    INT bRet = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%08x)\n",hdc);

    if (!dc) return TRUE;

    if (!dc->funcs->pSwapBuffers)
    {
        FIXME(" :stub\n");
	bRet = TRUE;
    }
    else bRet = dc->funcs->pSwapBuffers(dc);

    GDI_ReleaseObj( hdc );  
    return bRet;
}


/***********************************************************************
 *           PaintRgn    (GDI.43)
 */
BOOL16 WINAPI PaintRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return PaintRgn( hdc, hrgn );
}


/***********************************************************************
 *           PaintRgn    (GDI32.@)
 */
BOOL WINAPI PaintRgn( HDC hdc, HRGN hrgn )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (dc->funcs->pPaintRgn) ret = dc->funcs->pPaintRgn(dc,hrgn);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           FillRgn    (GDI.40)
 */
BOOL16 WINAPI FillRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush )
{
    return FillRgn( hdc, hrgn, hbrush );
}

    
/***********************************************************************
 *           FillRgn    (GDI32.@)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    BOOL retval = FALSE;
    HBRUSH prevBrush;
    DC * dc = DC_GetDCUpdate( hdc );

    if (!dc) return FALSE;
    if(dc->funcs->pFillRgn)
        retval = dc->funcs->pFillRgn(dc, hrgn, hbrush);
    else if ((prevBrush = SelectObject( hdc, hbrush )))
    {
    retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    }
    GDI_ReleaseObj( hdc );
    return retval;
}


/***********************************************************************
 *           FrameRgn     (GDI.41)
 */
BOOL16 WINAPI FrameRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush,
                          INT16 nWidth, INT16 nHeight )
{
    return FrameRgn( hdc, hrgn, hbrush, nWidth, nHeight );
}


/***********************************************************************
 *           FrameRgn     (GDI32.@)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush,
                          INT nWidth, INT nHeight )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCUpdate( hdc );

    if (!dc) return FALSE;
    if(dc->funcs->pFrameRgn)
        ret = dc->funcs->pFrameRgn( dc, hrgn, hbrush, nWidth, nHeight );
    else
    {
        HRGN tmp = CreateRectRgn( 0, 0, 0, 0 );
        if (tmp)
        {
            if (REGION_FrameRgn( tmp, hrgn, nWidth, nHeight ))
            {
                FillRgn( hdc, tmp, hbrush );
                ret = TRUE;
            }
            DeleteObject( tmp );
        }
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           InvertRgn    (GDI.42)
 */
BOOL16 WINAPI InvertRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return InvertRgn( hdc, hrgn );
}


/***********************************************************************
 *           InvertRgn    (GDI32.@)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    HBRUSH prevBrush;
    INT prevROP;
    BOOL retval;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;

    if(dc->funcs->pInvertRgn)
        retval = dc->funcs->pInvertRgn( dc, hrgn );
    else
    {
    prevBrush = SelectObject( hdc, GetStockObject(BLACK_BRUSH) );
    prevROP = SetROP2( hdc, R2_NOT );
    retval = PaintRgn( hdc, hrgn );
    SelectObject( hdc, prevBrush );
    SetROP2( hdc, prevROP );
    }
    GDI_ReleaseObj( hdc );
    return retval;
}

/**********************************************************************
 *          Polyline  (GDI.37)
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
 *          Polyline   (GDI32.@)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT* pt, INT count )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (PATH_IsPathOpen(dc->path)) ret = PATH_Polyline(dc, pt, count);
        else if (dc->funcs->pPolyline) ret = dc->funcs->pPolyline(dc,pt,count);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/**********************************************************************
 *          PolylineTo   (GDI32.@)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT* pt, DWORD cCount )
{
    DC * dc = DC_GetDCUpdate( hdc );
    BOOL ret = FALSE;

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
        ret = PATH_PolylineTo(dc, pt, cCount);

    else if(dc->funcs->pPolylineTo)
        ret = dc->funcs->pPolylineTo(dc, pt, cCount);

    else { /* do it using Polyline */
        POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				sizeof(POINT) * (cCount + 1) );
	if (pts)
        {
	pts[0].x = dc->CursPosX;
	pts[0].y = dc->CursPosY;
	memcpy( pts + 1, pt, sizeof(POINT) * cCount );
	ret = Polyline( hdc, pts, cCount + 1 );
	HeapFree( GetProcessHeap(), 0, pts );
    }
    }
    if(ret) {
        dc->CursPosX = pt[cCount-1].x;
	dc->CursPosY = pt[cCount-1].y;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}

/**********************************************************************
 *          Polygon  (GDI.36)
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
 *          Polygon  (GDI32.@)
 */
BOOL WINAPI Polygon( HDC hdc, const POINT* pt, INT count )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (PATH_IsPathOpen(dc->path)) ret = PATH_Polygon(dc, pt, count);
        else if (dc->funcs->pPolygon) ret = dc->funcs->pPolygon(dc,pt,count);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/**********************************************************************
 *          PolyPolygon (GDI.450)
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
 *          PolyPolygon  (GDI32.@)
 */
BOOL WINAPI PolyPolygon( HDC hdc, const POINT* pt, const INT* counts,
                             UINT polygons )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (PATH_IsPathOpen(dc->path)) ret = PATH_PolyPolygon(dc, pt, counts, polygons);
        else if (dc->funcs->pPolyPolygon) ret = dc->funcs->pPolyPolygon(dc,pt,counts,polygons);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/**********************************************************************
 *          PolyPolyline  (GDI32.@)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT* pt, const DWORD* counts,
                            DWORD polylines )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (PATH_IsPathOpen(dc->path)) ret = PATH_PolyPolyline(dc, pt, counts, polylines);
        else if (dc->funcs->pPolyPolyline) ret = dc->funcs->pPolyPolyline(dc,pt,counts,polylines);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/**********************************************************************
 *          ExtFloodFill   (GDI.372)
 */
BOOL16 WINAPI ExtFloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color,
                              UINT16 fillType )
{
    return ExtFloodFill( hdc, x, y, color, fillType );
}


/**********************************************************************
 *          ExtFloodFill   (GDI32.@)
 */
BOOL WINAPI ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                              UINT fillType )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );
    if (dc)
    {
        if (dc->funcs->pExtFloodFill) ret = dc->funcs->pExtFloodFill(dc,x,y,color,fillType);
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/**********************************************************************
 *          FloodFill   (GDI.25)
 */
BOOL16 WINAPI FloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          FloodFill   (GDI32.@)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}


/******************************************************************************
 * PolyBezier [GDI.502]
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
 * PolyBezierTo [GDI.503]
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
 * PolyBezier [GDI32.@]
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
    BOOL ret = FALSE;
    DC * dc = DC_GetDCUpdate( hdc );

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
	ret = PATH_PolyBezier(dc, lppt, cPoints);
    else if (dc->funcs->pPolyBezier)
        ret = dc->funcs->pPolyBezier(dc, lppt, cPoints);
    else  /* We'll convert it into line segments and draw them using Polyline */
    {
        POINT *Pts;
	INT nOut;

	if ((Pts = GDI_Bezier( lppt, cPoints, &nOut )))
        {
	    TRACE("Pts = %p, no = %d\n", Pts, nOut);
	    ret = Polyline( dc->hSelf, Pts, nOut );
	    HeapFree( GetProcessHeap(), 0, Pts );
	}
    }
 
    GDI_ReleaseObj( hdc );
    return ret;
}

/******************************************************************************
 * PolyBezierTo [GDI32.@]
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
    DC * dc = DC_GetDCUpdate( hdc );
    BOOL ret;

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
        ret = PATH_PolyBezierTo(dc, lppt, cPoints);
    else if(dc->funcs->pPolyBezierTo)
        ret = dc->funcs->pPolyBezierTo(dc, lppt, cPoints);
    else { /* We'll do it using PolyBezier */
        POINT *pt;
	pt = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * (cPoints + 1) );
	if(!pt) return FALSE;
	pt[0].x = dc->CursPosX;
	pt[0].y = dc->CursPosY;
	memcpy(pt + 1, lppt, sizeof(POINT) * cPoints);
	ret = PolyBezier(dc->hSelf, pt, cPoints+1);
	HeapFree( GetProcessHeap(), 0, pt );
    }
    if(ret) {
        dc->CursPosX = lppt[cPoints-1].x;
        dc->CursPosY = lppt[cPoints-1].y;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}

/***********************************************************************
 *      AngleArc (GDI32.@)
 */
BOOL WINAPI AngleArc(HDC hdc, INT x, INT y, DWORD dwRadius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
    INT x1,y1,x2,y2, arcdir;
    BOOL result;
    DC *dc;

    if( (signed int)dwRadius < 0 ) 
	return FALSE;

    dc = DC_GetDCUpdate( hdc );
    if(!dc) return FALSE; 
    
    if(dc->funcs->pAngleArc)
    {
        result = dc->funcs->pAngleArc( dc, x, y, dwRadius, eStartAngle, eSweepAngle );

        GDI_ReleaseObj( hdc );
        return result;
    }
    GDI_ReleaseObj( hdc );
 
    /* AngleArc always works counterclockwise */
    arcdir = GetArcDirection( hdc );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE ); 

    x1 = x + cos(eStartAngle*M_PI/180) * dwRadius;
    y1 = y - sin(eStartAngle*M_PI/180) * dwRadius;
    x2 = x + cos((eStartAngle+eSweepAngle)*M_PI/180) * dwRadius;
    y2 = x - sin((eStartAngle+eSweepAngle)*M_PI/180) * dwRadius;
 
    LineTo( hdc, x1, y1 );
    if( eSweepAngle >= 0 )
        result = Arc( hdc, x-dwRadius, y-dwRadius, x+dwRadius, y+dwRadius, 
		      x1, y1, x2, y2 );
    else
	result = Arc( hdc, x-dwRadius, y-dwRadius, x+dwRadius, y+dwRadius, 
		      x2, y2, x1, y1 );
 
    if( result ) MoveToEx( hdc, x2, y2, NULL );
    SetArcDirection( hdc, arcdir );
    return result; 
}

/***********************************************************************
 *      PolyDraw (GDI32.@)
 */
BOOL WINAPI PolyDraw(HDC hdc, const POINT *lppt, const BYTE *lpbTypes,
                       DWORD cCount)
{
    DC *dc;
    BOOL result;
    POINT lastmove;
    int i;

    dc = DC_GetDCUpdate( hdc );
    if(!dc) return FALSE; 

    if(dc->funcs->pPolyDraw)
    {
        result = dc->funcs->pPolyDraw( dc, lppt, lpbTypes, cCount );
        GDI_ReleaseObj( hdc );
        return result;
    }
    GDI_ReleaseObj( hdc );

    /* check for each bezierto if there are two more points */
    for( i = 0; i < cCount; i++ )
	if( lpbTypes[i] != PT_MOVETO &&
	    lpbTypes[i] & PT_BEZIERTO )
	{
	    if( cCount < i+3 )
		return FALSE;
	    else
		i += 2;
	}

    /* if no moveto occurs, we will close the figure here */
    lastmove.x = dc->CursPosX;
    lastmove.y = dc->CursPosY;

    /* now let's draw */
    for( i = 0; i < cCount; i++ )
    {
	if( lpbTypes[i] == PT_MOVETO )
	{
	    MoveToEx( hdc, lppt[i].x, lppt[i].y, NULL ); 				
	    lastmove.x = dc->CursPosX;
	    lastmove.y = dc->CursPosY;
	}
	else if( lpbTypes[i] & PT_LINETO )
	    LineTo( hdc, lppt[i].x, lppt[i].y );
	else if( lpbTypes[i] & PT_BEZIERTO )
	{
	    PolyBezierTo( hdc, &lppt[i], 3 );
	    i += 2;
	}
	else 
	    return FALSE; 

	if( lpbTypes[i] & PT_CLOSEFIGURE )
	{
	    if( PATH_IsPathOpen( dc->path ) )
		CloseFigure( hdc );
	    else 
		LineTo( hdc, lastmove.x, lastmove.y );
	}
    }

    return TRUE; 
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

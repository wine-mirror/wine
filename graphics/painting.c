/*
 * Misc. graphics operations
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
#include "gdi.h"
#include "dc.h"
#include "bitmap.h"
#include "callback.h"
#include "heap.h"
#include "metafile.h"
#include "syscolor.h"
#include "palette.h"
#include "color.h"
#include "region.h"
#include "stddebug.h"
#include "debug.h"

BOOL32 DrawDiagEdge32(HDC32 hdc, RECT32 *rect, UINT32 edge, UINT32 flags);
BOOL32 DrawRectEdge32(HDC32 hdc, RECT32 *rect, UINT32 edge, UINT32 flags);
BOOL32 DrawFrameButton32(HDC32 hdc, LPRECT32 rc, UINT32 uState);
BOOL32 DrawFrameCaption32(HDC32 hdc, LPRECT32 rc, UINT32 uState);
BOOL32 DrawFrameMenu32(HDC32 hdc, LPRECT32 rc, UINT32 uState);
BOOL32 DrawFrameScroll32(HDC32 hdc, LPRECT32 rc, UINT32 uState);

/***********************************************************************
 *           LineTo16    (GDI.19)
 */
BOOL16 WINAPI LineTo16( HDC16 hdc, INT16 x, INT16 y )
{
    return LineTo32( hdc, x, y );
}


/***********************************************************************
 *           LineTo32    (GDI32.249)
 */
BOOL32 WINAPI LineTo32( HDC32 hdc, INT32 x, INT32 y )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pLineTo &&
    	   dc->funcs->pLineTo(dc,x,y);
}


/***********************************************************************
 *           MoveTo    (GDI.20)
 */
DWORD WINAPI MoveTo( HDC16 hdc, INT16 x, INT16 y )
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
    POINT32 pt32;

    if (!MoveToEx32( (HDC32)hdc, (INT32)x, (INT32)y, &pt32 )) return FALSE;
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return TRUE;

}


/***********************************************************************
 *           MoveToEx32    (GDI32.254)
 */
BOOL32 WINAPI MoveToEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    DC * dc = DC_GetDCPtr( hdc );
  
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
    return Arc32( (HDC32)hdc, (INT32)left, (INT32)top, (INT32)right,
   		  (INT32)bottom, (INT32)xstart, (INT32)ystart, (INT32)xend,
		  (INT32)yend );
}


/***********************************************************************
 *           Arc32    (GDI32.7)
 */
BOOL32 WINAPI Arc32( HDC32 hdc, INT32 left, INT32 top, INT32 right,
                     INT32 bottom, INT32 xstart, INT32 ystart,
                     INT32 xend, INT32 yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pArc &&
    	   dc->funcs->pArc(dc,left,top,right,bottom,xstart,ystart,xend,yend);
}


/***********************************************************************
 *           Pie16    (GDI.26)
 */
BOOL16 WINAPI Pie16( HDC16 hdc, INT16 left, INT16 top,
                     INT16 right, INT16 bottom, INT16 xstart, INT16 ystart,
                     INT16 xend, INT16 yend )
{
    return Pie32( (HDC32)hdc, (INT32)left, (INT32)top, (INT32)right,
   		  (INT32)bottom, (INT32)xstart, (INT32)ystart, (INT32)xend,
		  (INT32)yend );
}


/***********************************************************************
 *           Pie32   (GDI32.262)
 */
BOOL32 WINAPI Pie32( HDC32 hdc, INT32 left, INT32 top,
                     INT32 right, INT32 bottom, INT32 xstart, INT32 ystart,
                     INT32 xend, INT32 yend )
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
    return Chord32( hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           Chord32    (GDI32.14)
 */
BOOL32 WINAPI Chord32( HDC32 hdc, INT32 left, INT32 top,
                       INT32 right, INT32 bottom, INT32 xstart, INT32 ystart,
                       INT32 xend, INT32 yend )
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
    return Ellipse32( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Ellipse32    (GDI32.75)
 */
BOOL32 WINAPI Ellipse32( HDC32 hdc, INT32 left, INT32 top,
                         INT32 right, INT32 bottom )
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
    return Rectangle32( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           Rectangle32    (GDI32.283)
 */
BOOL32 WINAPI Rectangle32( HDC32 hdc, INT32 left, INT32 top,
                           INT32 right, INT32 bottom )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pRectangle &&
    	   dc->funcs->pRectangle(dc,left,top,right,bottom);
}


/***********************************************************************
 *           RoundRect16    (GDI.28)
 */
BOOL16 WINAPI RoundRect16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                           INT16 bottom, INT16 ell_width, INT16 ell_height )
{
    return RoundRect32( hdc, left, top, right, bottom, ell_width, ell_height );
}


/***********************************************************************
 *           RoundRect32    (GDI32.291)
 */
BOOL32 WINAPI RoundRect32( HDC32 hdc, INT32 left, INT32 top, INT32 right,
                           INT32 bottom, INT32 ell_width, INT32 ell_height )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pRoundRect &&
    	   dc->funcs->pRoundRect(dc,left,top,right,bottom,ell_width,ell_height);
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
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject16( hdc, prevBrush );
    return 1;
}


/***********************************************************************
 *           FillRect32    (USER32.196)
 */
INT32 WINAPI FillRect32( HDC32 hdc, const RECT32 *rect, HBRUSH32 hbrush )
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
void WINAPI InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *           InvertRect32    (USER32.329)
 */
void WINAPI InvertRect32( HDC32 hdc, const RECT32 *rect )
{
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
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
INT32 WINAPI FrameRect32( HDC32 hdc, const RECT32 *rect, HBRUSH32 hbrush )
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
    return SetPixel32( hdc, x, y, color );
}


/***********************************************************************
 *           SetPixel32    (GDI32.327)
 */
COLORREF WINAPI SetPixel32( HDC32 hdc, INT32 x, INT32 y, COLORREF color )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc || !dc->funcs->pSetPixel) return 0;
    return dc->funcs->pSetPixel(dc,x,y,color);
}

/***********************************************************************
 *           SetPixel32    (GDI32.329)
 */
BOOL32 WINAPI SetPixelV32( HDC32 hdc, INT32 x, INT32 y, COLORREF color )
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
    return GetPixel32( hdc, x, y );
}


/***********************************************************************
 *           GetPixel32    (GDI32.211)
 */
COLORREF WINAPI GetPixel32( HDC32 hdc, INT32 x, INT32 y )
{
    DC * dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;
#ifdef SOLITAIRE_SPEED_HACK
    return 0;
#endif

    /* FIXME: should this be in the graphics driver? */
    if (!PtVisible32( hdc, x, y )) return 0;
    if (!dc || !dc->funcs->pGetPixel) return 0;
    return dc->funcs->pGetPixel(dc,x,y);
}


/***********************************************************************
 *           PaintRgn16    (GDI.43)
 */
BOOL16 WINAPI PaintRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return PaintRgn32( hdc, hrgn );
}


/***********************************************************************
 *           PaintRgn32    (GDI32.259)
 */
BOOL32 WINAPI PaintRgn32( HDC32 hdc, HRGN32 hrgn )
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
    return FillRgn32( hdc, hrgn, hbrush );
}

    
/***********************************************************************
 *           FillRgn32    (GDI32.101)
 */
BOOL32 WINAPI FillRgn32( HDC32 hdc, HRGN32 hrgn, HBRUSH32 hbrush )
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
BOOL16 WINAPI FrameRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush,
                          INT16 nWidth, INT16 nHeight )
{
    return FrameRgn32( hdc, hrgn, hbrush, nWidth, nHeight );
}


/***********************************************************************
 *           FrameRgn32     (GDI32.105)
 */
BOOL32 WINAPI FrameRgn32( HDC32 hdc, HRGN32 hrgn, HBRUSH32 hbrush,
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
BOOL16 WINAPI InvertRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return InvertRgn32( hdc, hrgn );
}


/***********************************************************************
 *           InvertRgn32    (GDI32.246)
 */
BOOL32 WINAPI InvertRgn32( HDC32 hdc, HRGN32 hrgn )
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
void WINAPI DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT32 rect32;
    CONV_RECT16TO32( rc, &rect32 );
    DrawFocusRect32( hdc, &rect32 );
}


/***********************************************************************
 *           DrawFocusRect32    (USER32.155)
 *
 * FIXME: PatBlt(PATINVERT) with background brush.
 */
void WINAPI DrawFocusRect32( HDC32 hdc, const RECT32* rc )
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
 *          Polyline16  (GDI.37)
 */
BOOL16 WINAPI Polyline16( HDC16 hdc, LPPOINT16 pt, INT16 count )
{
    register int i;
    BOOL16 ret;
    LPPOINT32 pt32 = (LPPOINT32)HeapAlloc( GetProcessHeap(), 0,
                                           count*sizeof(POINT32) );

    if (!pt32) return FALSE;
    for (i=count;i--;) CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    ret = Polyline32(hdc,pt32,count);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/**********************************************************************
 *          Polyline32   (GDI32.276)
 */
BOOL32 WINAPI Polyline32( HDC32 hdc, const LPPOINT32 pt, INT32 count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyline &&
    	   dc->funcs->pPolyline(dc,pt,count);
}


/**********************************************************************
 *          Polygon16  (GDI.36)
 */
BOOL16 WINAPI Polygon16( HDC16 hdc, LPPOINT16 pt, INT16 count )
{
    register int i;
    BOOL32 ret;
    LPPOINT32 pt32 = (LPPOINT32)HeapAlloc( GetProcessHeap(), 0,
                                           count*sizeof(POINT32) );

    if (!pt32) return FALSE;
    for (i=count;i--;) CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    ret = Polygon32(hdc,pt32,count);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/**********************************************************************
 *          Polygon32  (GDI32.275)
 */
BOOL32 WINAPI Polygon32( HDC32 hdc, LPPOINT32 pt, INT32 count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolygon &&
    	   dc->funcs->pPolygon(dc,pt,count);
}


/**********************************************************************
 *          PolyPolygon16  (GDI.450)
 */
BOOL16 WINAPI PolyPolygon16( HDC16 hdc, LPPOINT16 pt, LPINT16 counts,
                             UINT16 polygons )
{
    int		i,nrpts;
    LPPOINT32	pt32;
    LPINT32	counts32;
    BOOL16	ret;

    nrpts=0;
    for (i=polygons;i--;)
    	nrpts+=counts[i];
    pt32 = (LPPOINT32)HEAP_xalloc( GetProcessHeap(), 0, sizeof(POINT32)*nrpts);
    for (i=nrpts;i--;)
    	CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    counts32 = (LPINT32)HEAP_xalloc( GetProcessHeap(), 0,
                                     polygons*sizeof(INT32) );
    for (i=polygons;i--;) counts32[i]=counts[i];
   
    ret = PolyPolygon32(hdc,pt32,counts32,polygons);
    HeapFree( GetProcessHeap(), 0, counts32 );
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}

/**********************************************************************
 *          PolyPolygon32  (GDI.450)
 */
BOOL32 WINAPI PolyPolygon32( HDC32 hdc, LPPOINT32 pt, LPINT32 counts,
                             UINT32 polygons )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyPolygon &&
    	   dc->funcs->pPolyPolygon(dc,pt,counts,polygons);
}

/**********************************************************************
 *          ExtFloodFill16   (GDI.372)
 */
BOOL16 WINAPI ExtFloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color,
                              UINT16 fillType )
{
    return ExtFloodFill32( hdc, x, y, color, fillType );
}


/**********************************************************************
 *          ExtFloodFill32   (GDI32.96)
 */
BOOL32 WINAPI ExtFloodFill32( HDC32 hdc, INT32 x, INT32 y, COLORREF color,
                              UINT32 fillType )
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
    return ExtFloodFill32( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          FloodFill32   (GDI32.104)
 */
BOOL32 WINAPI FloodFill32( HDC32 hdc, INT32 x, INT32 y, COLORREF color )
{
    return ExtFloodFill32( hdc, x, y, color, FLOODFILLBORDER );
}


/**********************************************************************
 *          DrawFrameControl32  (USER32.152)
 */
BOOL32 WINAPI DrawAnimatedRects32( HWND32 hwnd, int idAni,
                                   const LPRECT32 lprcFrom,
                                   const LPRECT32 lprcTo )
{
    fprintf( stdnimp,"DrawAnimatedRects32(%x,%d,%p,%p), empty stub!\n",
             hwnd, idAni, lprcFrom, lprcTo );
    return TRUE;
}

BOOL32 WINAPI DrawState32A(
	HDC32 hdc,HBRUSH32 hbrush,DRAWSTATEPROC drawstateproc,
	LPARAM lparam,WPARAM32 wparam,INT32 x,INT32 y,INT32 z,INT32 a,UINT32 b
) {
	fprintf(stderr,"DrawStateA(%x,%x,%p,0x%08lx,0x%08x,%d,%d,%d,%d,%d),stub\n",
		hdc,hbrush,drawstateproc,lparam,wparam,x,y,z,a,b
	);
	return TRUE;
}


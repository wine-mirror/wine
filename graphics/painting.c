/*
 * Misc. graphics operations
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 */

#include <math.h>
#include <stdlib.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
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
#include "palette.h"
#include "cache.h"
#include "color.h"
#include "region.h"
#include "path.h"
#include "debug.h"

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

    if(dc && PATH_IsPathOpen(dc->w.path))
        if(!PATH_LineTo(hdc, x, y))
	   return FALSE;
    
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
  
    if(dc && PATH_IsPathOpen(dc->w.path))
        if(!PATH_Arc(hdc, left, top, right, bottom, xstart, ystart, xend,
	   yend))
	   return FALSE;
    
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
    HPEN32 hOldPen, hnewPen;
    INT32 oldDrawMode, oldBkMode;
    INT32 left, top, right, bottom;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return;

    left   = XLPTODP( dc, rc->left );
    top    = YLPTODP( dc, rc->top );
    right  = XLPTODP( dc, rc->right );
    bottom = YLPTODP( dc, rc->bottom );
    
    hnewPen = CreatePen32(PS_DOT, 1, GetSysColor32(COLOR_WINDOWTEXT) );
    hOldPen = SelectObject32( hdc, hnewPen );
    oldDrawMode = SetROP232(hdc, R2_XORPEN);
    oldBkMode = SetBkMode32(hdc, TRANSPARENT);

    /* Hack: make sure the XORPEN operation has an effect */
    dc->u.x.pen.pixel = (1 << screenDepth) - 1;

    if (DC_SetupGCForPen( dc ))
	TSXDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc,
		        dc->w.DCOrgX + left, dc->w.DCOrgY + top,
		        right-left-1, bottom-top-1 );

    SetBkMode32(hdc, oldBkMode);
    SetROP232(hdc, oldDrawMode);
    SelectObject32(hdc, hOldPen);
    DeleteObject32(hnewPen);
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
 *          DrawAnimatedRects32  (USER32.153)
 */
BOOL32 WINAPI DrawAnimatedRects32( HWND32 hwnd, int idAni,
                                   const LPRECT32 lprcFrom,
                                   const LPRECT32 lprcTo )
{
    FIXME(gdi,"(%x,%d,%p,%p): empty stub!\n",
	  hwnd, idAni, lprcFrom, lprcTo );
    return TRUE;
}


/**********************************************************************
 *          PAINTING_DrawStateJam
 *
 * Jams in the requested type in the dc
 */
static BOOL32 PAINTING_DrawStateJam(HDC32 hdc, UINT32 opcode,
                                    DRAWSTATEPROC32 func, LPARAM lp, WPARAM32 wp, 
                                    LPRECT32 rc, UINT32 dtflags,
                                    BOOL32 unicode, BOOL32 _32bit)
{
    HDC32 memdc;
    HBITMAP32 hbmsave;
    BOOL32 retval;
    INT32 cx = rc->right - rc->left;
    INT32 cy = rc->bottom - rc->top;
    
    switch(opcode)
    {
    case DST_TEXT:
    case DST_PREFIXTEXT:
        if(unicode)
            return DrawText32W(hdc, (LPWSTR)lp, (INT32)wp, rc, dtflags);
        else if(_32bit)
            return DrawText32A(hdc, (LPSTR)lp, (INT32)wp, rc, dtflags);
        else
            return DrawText32A(hdc, (LPSTR)PTR_SEG_TO_LIN(lp), (INT32)wp, rc, dtflags);

    case DST_ICON:
        return DrawIcon32(hdc, rc->left, rc->top, (HICON32)lp);

    case DST_BITMAP:
        memdc = CreateCompatibleDC32(hdc);
        if(!memdc) return FALSE;
        hbmsave = (HBITMAP32)SelectObject32(memdc, (HBITMAP32)lp);
        if(!hbmsave) 
        {
            DeleteDC32(memdc);
            return FALSE;
        }
        retval = BitBlt32(hdc, rc->left, rc->top, cx, cy, memdc, 0, 0, SRCCOPY);
        SelectObject32(memdc, hbmsave);
        DeleteDC32(memdc);
        return retval;
            
    case DST_COMPLEX:
        if(func)
            if(_32bit)
                return func(hdc, lp, wp, cx, cy);
            else
                return (BOOL32)((DRAWSTATEPROC16)func)((HDC16)hdc, (LPARAM)lp, (WPARAM16)wp, (INT16)cx, (INT16)cy);
        else
            return FALSE;
    }
    return FALSE;
}

/**********************************************************************
 *      PAINTING_DrawState32()
 */
static BOOL32 PAINTING_DrawState32(HDC32 hdc, HBRUSH32 hbr, 
                                   DRAWSTATEPROC32 func, LPARAM lp, WPARAM32 wp,
                                   INT32 x, INT32 y, INT32 cx, INT32 cy, 
                                   UINT32 flags, BOOL32 unicode, BOOL32 _32bit)
{
    HBITMAP32 hbm, hbmsave;
    HFONT32 hfsave;
    HBRUSH32 hbsave;
    HDC32 memdc;
    RECT32 rc;
    UINT32 dtflags = DT_NOCLIP;
    COLORREF fg, bg;
    UINT32 opcode = flags & 0xf;
    INT32 len = wp;
    BOOL32 retval, tmp;

    if((opcode == DST_TEXT || opcode == DST_PREFIXTEXT) && !len)    /* The string is '\0' terminated */
    {
        if(unicode)
            len = lstrlen32W((LPWSTR)lp);
        else if(_32bit)
            len = lstrlen32A((LPSTR)lp);
        else
            len = lstrlen32A((LPSTR)PTR_SEG_TO_LIN(lp));
    }

    /* Find out what size the image has if not given by caller */
    if(!cx || !cy)
    {
        SIZE32 s;
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
    fg = SetTextColor32(hdc, RGB(0, 0, 0));
    bg = SetBkColor32(hdc, RGB(255, 255, 255));
    hbm = NULL; hbmsave = NULL; memdc = NULL; memdc = NULL; hbsave = NULL;
    retval = FALSE; /* assume failure */
    
    /* From here on we must use "goto cleanup" when something goes wrong */
    hbm     = CreateBitmap32(cx, cy, 1, 1, NULL);
    if(!hbm) goto cleanup;
    memdc   = CreateCompatibleDC32(hdc);
    if(!memdc) goto cleanup;
    hbmsave = (HBITMAP32)SelectObject32(memdc, hbm);
    if(!hbmsave) goto cleanup;
    rc.left = rc.top = 0;
    rc.right = cx;
    rc.bottom = cy;
    if(!FillRect32(memdc, &rc, (HBRUSH32)GetStockObject32(WHITE_BRUSH))) goto cleanup;
    SetBkColor32(memdc, RGB(255, 255, 255));
    SetTextColor32(memdc, RGB(0, 0, 0));
    hfsave  = (HFONT32)SelectObject32(memdc, GetCurrentObject(hdc, OBJ_FONT));
    if(!hfsave && (opcode == DST_TEXT || opcode == DST_PREFIXTEXT)) goto cleanup;
    tmp = PAINTING_DrawStateJam(memdc, opcode, func, lp, len, &rc, dtflags, unicode, _32bit);
    if(hfsave) SelectObject32(memdc, hfsave);
    if(!tmp) goto cleanup;
    
    /* These states cause the image to be dithered */
    if(flags & (DSS_UNION|DSS_DISABLED))
    {
        hbsave = (HBRUSH32)SelectObject32(memdc, CACHE_GetPattern55AABrush());
        if(!hbsave) goto cleanup;
        tmp = PatBlt32(memdc, 0, 0, cx, cy, 0x00FA0089);
        if(hbsave) SelectObject32(memdc, hbsave);
        if(!tmp) goto cleanup;
    }

    hbsave = (HBRUSH32)SelectObject32(hdc, hbr ? hbr : GetStockObject32(WHITE_BRUSH));
    if(!hbsave) goto cleanup;
    
    if(!BitBlt32(hdc, x, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
    
    /* DSS_DEFAULT makes the image boldface */
    if(flags & DSS_DEFAULT)
    {
        if(!BitBlt32(hdc, x+1, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
    }

    retval = TRUE; /* We succeeded */
    
cleanup:    
    SetTextColor32(hdc, fg);
    SetBkColor32(hdc, bg);

    if(hbsave)  SelectObject32(hdc, hbsave);
    if(hbmsave) SelectObject32(memdc, hbmsave);
    if(hbm)     DeleteObject32(hbm);
    if(memdc)   DeleteDC32(memdc);

    return retval;
}

/**********************************************************************
 *      DrawState32A()   (USER32.162)
 */
BOOL32 WINAPI DrawState32A(HDC32 hdc, HBRUSH32 hbr,
                   DRAWSTATEPROC32 func, LPARAM ldata, WPARAM32 wdata,
                   INT32 x, INT32 y, INT32 cx, INT32 cy, UINT32 flags)
{
    return PAINTING_DrawState32(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, FALSE, TRUE);
}

/**********************************************************************
 *      DrawState32W()   (USER32.163)
 */
BOOL32 WINAPI DrawState32W(HDC32 hdc, HBRUSH32 hbr,
                   DRAWSTATEPROC32 func, LPARAM ldata, WPARAM32 wdata,
                   INT32 x, INT32 y, INT32 cx, INT32 cy, UINT32 flags)
{
    return PAINTING_DrawState32(hdc, hbr, func, ldata, wdata, x, y, cx, cy, flags, TRUE, TRUE);
}

/**********************************************************************
 *      DrawState16()   (USER.449)
 */
BOOL16 WINAPI DrawState16(HDC16 hdc, HBRUSH16 hbr,
                   DRAWSTATEPROC16 func, LPARAM ldata, WPARAM16 wdata,
                   INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags)
{
    return PAINTING_DrawState32(hdc, hbr, (DRAWSTATEPROC32)func, ldata, wdata, x, y, cx, cy, flags, FALSE, FALSE);
}

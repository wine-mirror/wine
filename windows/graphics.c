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
#include "gdi.h"
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
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pLineTo &&
    	   dc->funcs->pLineTo(dc,x,y);
}


/***********************************************************************
 *           MoveTo    (GDI.20)
 */
DWORD MoveTo( HDC16 hdc, INT16 x, INT16 y )
{
    POINT16	pt;

    if (!MoveToEx16(hdc,x,y,&pt))
    	return 0;
    return MAKELONG(pt.x,pt.y);
}


/***********************************************************************
 *           MoveToEx16    (GDI.483)
 */
BOOL16 MoveToEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT32 pt32;

    if (!MoveToEx32( (HDC32)hdc, (INT32)x, (INT32)y, &pt32 )) return FALSE;
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return TRUE;

}


/***********************************************************************
 *           MoveToEx32    (GDI32.254)
 */
BOOL32 MoveToEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pMoveToEx &&
    	   dc->funcs->pMoveToEx(dc,x,y,pt);
}


/***********************************************************************
 *           Arc16    (GDI.23)
 */
BOOL16 Arc16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom,
              INT16 xstart, INT16 ystart, INT16 xend, INT16 yend )
{
    return Arc32( (HDC32)hdc, (INT32)left, (INT32)top, (INT32)right,
   		  (INT32)bottom, (INT32)xstart, (INT32)ystart, (INT32)xend,
		  (INT32)yend );
}


/***********************************************************************
 *           Arc32    (GDI32.7)
 */
BOOL32 Arc32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom,
              INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pArc &&
    	   dc->funcs->pArc(dc,left,top,right,bottom,xstart,ystart,xend,yend);
}


/***********************************************************************
 *           Pie16    (GDI.26)
 */
BOOL16 Pie16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom,
              INT16 xstart, INT16 ystart, INT16 xend, INT16 yend )
{
    return Pie32( (HDC32)hdc, (INT32)left, (INT32)top, (INT32)right,
   		  (INT32)bottom, (INT32)xstart, (INT32)ystart, (INT32)xend,
		  (INT32)yend );
}


/***********************************************************************
 *           Pie32   (GDI32.262)
 */
BOOL32 Pie32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom,
              INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pPie &&
    	   dc->funcs->pPie(dc,left,top,right,bottom,xstart,ystart,xend,yend);
}


/***********************************************************************
 *           Chord16    (GDI.348)
 */
BOOL16 Chord16( HDC16 hdc, INT16 left, INT16 top, INT16 right, INT16 bottom,
                INT16 xstart, INT16 ystart, INT16 xend, INT16 yend )
{
    return Chord32( hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           Chord32    (GDI32.14)
 */
BOOL32 Chord32( HDC32 hdc, INT32 left, INT32 top, INT32 right, INT32 bottom,
                INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pChord &&
    	   dc->funcs->pChord(dc,left,top,right,bottom,xstart,ystart,xend,yend);
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
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pEllipse &&
    	   dc->funcs->pEllipse(dc,left,top,right,bottom);
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
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pRectangle &&
    	   dc->funcs->pRectangle(dc,left,top,right,bottom);
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
    DC * dc = DC_GetDCPtr( hdc );
  
    return dc && dc->funcs->pRoundRect &&
    	   dc->funcs->pRoundRect(dc,left,top,right,bottom,ell_width,ell_height);
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
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc || !dc->funcs->pSetPixel) return 0;
    return dc->funcs->pSetPixel(dc,x,y,color);
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
BOOL16 PaintRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return PaintRgn32( hdc, hrgn );
}


/***********************************************************************
 *           PaintRgn32    (GDI32.259)
 */
BOOL32 PaintRgn32( HDC32 hdc, HRGN32 hrgn )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPaintRgn &&
	   dc->funcs->pPaintRgn(dc,hrgn);
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
 *
 * FIXME: should use Rectangle32!
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
    LPPOINT32 pt32 = (LPPOINT32)xmalloc(count*sizeof(POINT32));
    BOOL16 ret;

    for (i=count;i--;) CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    ret = Polyline32(hdc,pt32,count);
    free(pt32);
    return ret;
}


/**********************************************************************
 *          Polyline32   (GDI32.276)
 */
BOOL32 Polyline32( HDC32 hdc, const LPPOINT32 pt, INT32 count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyline &&
    	   dc->funcs->pPolyline(dc,pt,count);
}


/**********************************************************************
 *          Polygon16  (GDI.36)
 */
BOOL16 Polygon16( HDC16 hdc, LPPOINT16 pt, INT16 count )
{
    register int i;
    LPPOINT32 pt32 = (LPPOINT32)xmalloc(count*sizeof(POINT32));
    BOOL32 ret;


    for (i=count;i--;) CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    ret = Polygon32(hdc,pt32,count);
    free(pt32);
    return ret;
}


/**********************************************************************
 *          Polygon32  (GDI32.275)
 */
BOOL32 Polygon32( HDC32 hdc, LPPOINT32 pt, INT32 count )
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolygon &&
    	   dc->funcs->pPolygon(dc,pt,count);
}


/**********************************************************************
 *          PolyPolygon16  (GDI.450)
 */
BOOL16 PolyPolygon16( HDC16 hdc, LPPOINT16 pt, LPINT16 counts, UINT16 polygons)
{
    int		i,nrpts;
    LPPOINT32	pt32;
    LPINT32	counts32;
    BOOL16	ret;

    nrpts=0;
    for (i=polygons;i--;)
    	nrpts+=counts[i];
    pt32 = (LPPOINT32)xmalloc(sizeof(POINT32)*nrpts);
    for (i=nrpts;i--;)
    	CONV_POINT16TO32(&(pt[i]),&(pt32[i]));
    counts32 = (LPINT32)xmalloc(polygons*sizeof(INT32));
    for (i=polygons;i--;) counts32[i]=counts[i];
   
    ret = PolyPolygon32(hdc,pt32,counts32,polygons);
    free(counts32);
    free(pt32);
    return ret;
}

/**********************************************************************
 *          PolyPolygon32  (GDI.450)
 */
BOOL32 PolyPolygon32( HDC32 hdc, LPPOINT32 pt, LPINT32 counts, UINT32 polygons)
{
    DC * dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pPolyPolygon &&
    	   dc->funcs->pPolyPolygon(dc,pt,counts,polygons);
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
    DC *dc = DC_GetDCPtr( hdc );

    return dc && dc->funcs->pExtFloodFill &&
	   dc->funcs->pExtFloodFill(dc,x,y,color,fillType);
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

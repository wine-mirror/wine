/*
 * GDI region objects
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <stdlib.h>
#include <stdio.h>
#include "region.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           REGION_DeleteObject
 */
BOOL32 REGION_DeleteObject( HRGN32 hrgn, RGNOBJ * obj )
{
    dprintf_region(stddeb, "DeleteRegion: %04x\n", hrgn );
    if (obj->xrgn) XDestroyRegion( obj->xrgn );
    return GDI_FreeObject( hrgn );
}


/***********************************************************************
 *           OffsetRgn16    (GDI.101)
 */
INT16 OffsetRgn16( HRGN16 hrgn, INT16 x, INT16 y )
{
    return OffsetRgn32( hrgn, x, y );
}


/***********************************************************************
 *           OffsetRgn32   (GDI32.256)
 */
INT32 OffsetRgn32( HRGN32 hrgn, INT32 x, INT32 y )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (!obj) return ERROR;
    dprintf_region(stddeb, "OffsetRgn: %04x %d,%d\n", hrgn, x, y );
    if (!obj->xrgn) return NULLREGION;
    XOffsetRegion( obj->xrgn, x, y );
    return COMPLEXREGION;
}


/***********************************************************************
 *           GetRgnBox16    (GDI.134)
 */
INT16 GetRgnBox16( HRGN16 hrgn, LPRECT16 rect )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (!obj) return ERROR;
    dprintf_region(stddeb, "GetRgnBox: %04x\n", hrgn );
    if (!obj->xrgn)
    {
        SetRectEmpty16( rect );
        return NULLREGION;
    }
    else
    {
        XRectangle xrect;
        XClipBox( obj->xrgn, &xrect );
        SetRect16( rect, xrect.x, xrect.y,
                   xrect.x + xrect.width, xrect.y + xrect.height);
        return COMPLEXREGION;
    }
}


/***********************************************************************
 *           GetRgnBox32    (GDI32.219)
 */
INT32 GetRgnBox32( HRGN32 hrgn, LPRECT32 rect )
{
    RECT16 r;
    INT16 ret = GetRgnBox16( hrgn, &r );
    CONV_RECT16TO32( &r, rect );
    return ret;
}


/***********************************************************************
 *           CreateRectRgn16    (GDI.64)
 */
HRGN16 CreateRectRgn16( INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    return (HRGN16)CreateRectRgn32( left, top, right, bottom );
}


/***********************************************************************
 *           CreateRectRgn32   (GDI32.59)
 */
HRGN32 CreateRectRgn32( INT32 left, INT32 top, INT32 right, INT32 bottom )
{
    HRGN32 hrgn;
    RGNOBJ *obj;

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    obj = (RGNOBJ *) GDI_HEAP_LIN_ADDR( hrgn );
    if ((right > left) && (bottom > top))
    {
	XRectangle rect = { left, top, right - left, bottom - top };
	if (!(obj->xrgn = XCreateRegion()))
        {
            GDI_FreeObject( hrgn );
            return 0;
        }
        XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
    }
    else obj->xrgn = 0;
    dprintf_region( stddeb, "CreateRectRgn(%d,%d-%d,%d): returning %04x\n",
                    left, top, right, bottom, hrgn );
    return hrgn;
}


/***********************************************************************
 *           CreateRectRgnIndirect16    (GDI.65)
 */
HRGN16 CreateRectRgnIndirect16( const RECT16* rect )
{
    return CreateRectRgn32( rect->left, rect->top, rect->right, rect->bottom );
}


/***********************************************************************
 *           CreateRectRgnIndirect32    (GDI32.60)
 */
HRGN32 CreateRectRgnIndirect32( const RECT32* rect )
{
    return CreateRectRgn32( rect->left, rect->top, rect->right, rect->bottom );
}


/***********************************************************************
 *           SetRectRgn    (GDI.172) (GDI32.332)
 */
VOID SetRectRgn( HRGN32 hrgn, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    RGNOBJ * obj;

    dprintf_region(stddeb, "SetRectRgn: %04x %d,%d-%d,%d\n", 
		   hrgn, left, top, right, bottom );
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return;
    if (obj->xrgn) XDestroyRegion( obj->xrgn );
    if ((right > left) && (bottom > top))
    {
	XRectangle rect = { left, top, right - left, bottom - top };
	if ((obj->xrgn = XCreateRegion()) != 0)
            XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
    }
    else obj->xrgn = 0;
}


/***********************************************************************
 *           CreateRoundRectRgn16    (GDI.444)
 */
HRGN16 CreateRoundRectRgn16( INT16 left, INT16 top, INT16 right, INT16 bottom,
                             INT16 ellipse_width, INT16 ellipse_height )
{
    return (HRGN16)CreateRoundRectRgn32( left, top, right, bottom,
                                         ellipse_width, ellipse_height );
}


/***********************************************************************
 *           CreateRoundRectRgn32    (GDI32.61)
 */
HRGN32 CreateRoundRectRgn32( INT32 left, INT32 top, INT32 right, INT32 bottom,
                             INT32 ellipse_width, INT32 ellipse_height )
{
    RGNOBJ * obj;
    HRGN32 hrgn;
    XRectangle rect;
    int asq, bsq, d, xd, yd;

      /* Check if we can do a normal rectangle instead */

    if ((right <= left) || (bottom <= top) ||
        (ellipse_width <= 0) || (ellipse_height <= 0))
        return CreateRectRgn32( left, top, right, bottom );

      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    obj = (RGNOBJ *) GDI_HEAP_LIN_ADDR( hrgn );
    obj->xrgn = XCreateRegion();
    dprintf_region(stddeb,"CreateRoundRectRgn(%d,%d-%d,%d %dx%d): return=%04x\n",
               left, top, right, bottom, ellipse_width, ellipse_height, hrgn );

      /* Check parameters */

    if (ellipse_width > right-left) ellipse_width = right-left;
    if (ellipse_height > bottom-top) ellipse_height = bottom-top;

      /* Ellipse algorithm, based on an article by K. Porter */
      /* in DDJ Graphics Programming Column, 8/89 */

    asq = ellipse_width * ellipse_width / 4;        /* a^2 */
    bsq = ellipse_height * ellipse_height / 4;      /* b^2 */
    d = bsq - asq * ellipse_height / 2 + asq / 4;   /* b^2 - a^2b + a^2/4 */
    xd = 0;
    yd = asq * ellipse_height;                      /* 2a^2b */

    rect.x      = left + ellipse_width / 2;
    rect.width  = right - left - ellipse_width;
    rect.height = 1;

      /* Loop to draw first half of quadrant */

    while (xd < yd)
    {
        if (d > 0)  /* if nearest pixel is toward the center */
        {
              /* move toward center */
            rect.y = top++;
            XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
            rect.y = --bottom;
            XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
            yd -= 2*asq;
            d  -= yd;
        }
        rect.x--;        /* next horiz point */
        rect.width += 2;
        xd += 2*bsq;
        d  += bsq + xd;
    }

      /* Loop to draw second half of quadrant */

    d += (3 * (asq-bsq) / 2 - (xd+yd)) / 2;
    while (yd >= 0)
    {
          /* next vertical point */
        rect.y = top++;
        XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
        rect.y = --bottom;
        XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
        if (d < 0)   /* if nearest pixel is outside ellipse */
        {
            rect.x--;     /* move away from center */
            rect.width += 2;
            xd += 2*bsq;
            d  += xd;
        }
        yd -= 2*asq;
        d  += asq - yd;
    }

      /* Add the inside rectangle */

    if (top <= bottom)
    {
        rect.y = top;
        rect.height = bottom - top + 1;
        XUnionRectWithRegion( &rect, obj->xrgn, obj->xrgn );
    }
    return hrgn;
}


/***********************************************************************
 *           CreateEllipticRgn16    (GDI.54)
 */
HRGN16 CreateEllipticRgn16( INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    return (HRGN16)CreateRoundRectRgn32( left, top, right, bottom,
                                         right-left, bottom-top );
}


/***********************************************************************
 *           CreateEllipticRgn32    (GDI32.39)
 */
HRGN32 CreateEllipticRgn32( INT32 left, INT32 top, INT32 right, INT32 bottom )
{
    return CreateRoundRectRgn32( left, top, right, bottom,
                                 right-left, bottom-top );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect16    (GDI.55)
 */
HRGN16 CreateEllipticRgnIndirect16( const RECT16 *rect )
{
    return CreateRoundRectRgn32( rect->left, rect->top, rect->right,
                                 rect->bottom, rect->right - rect->left,
                                 rect->bottom - rect->top );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect32    (GDI32.40)
 */
HRGN32 CreateEllipticRgnIndirect32( const RECT32 *rect )
{
    return CreateRoundRectRgn32( rect->left, rect->top, rect->right,
                                 rect->bottom, rect->right - rect->left,
                                 rect->bottom - rect->top );
}


/***********************************************************************
 *           CreatePolygonRgn16    (GDI.63)
 */
HRGN16 CreatePolygonRgn16( const POINT16 * points, INT16 count, INT16 mode )
{
    return CreatePolyPolygonRgn16( points, &count, 1, mode );
}


/***********************************************************************
 *           CreatePolyPolygonRgn16    (GDI.451)
 */
HRGN16 CreatePolyPolygonRgn16( const POINT16 * points, const INT16 * count,
                               INT16 nbpolygons, INT16 mode )
{
    int		i,nrofpts;
    LPINT32	count32;
    LPPOINT32	points32;
    HRGN32	ret;

    nrofpts=0;
    for (i=nbpolygons;i--;)
	nrofpts+=count[i];
    points32 = (LPPOINT32)HeapAlloc( GetProcessHeap(), 0,
                                     nrofpts*sizeof(POINT32) );
    for (i=nrofpts;i--;)
    	CONV_POINT16TO32( &(points[i]), &(points32[i]) );
    count32 = (LPINT32)HeapAlloc( GetProcessHeap(), 0, 
                                  sizeof(INT32)*nbpolygons );
    for (i=nbpolygons;i--;)
    	count32[i]=count[i];
    ret = CreatePolyPolygonRgn32(points32,count32,nbpolygons,mode);
    HeapFree( GetProcessHeap(), 0, count32 );
    HeapFree( GetProcessHeap(), 0, points32 );
    return ret;
}


/***********************************************************************
 *           CreatePolygonRgn32    (GDI32.58)
 */
HRGN32 CreatePolygonRgn32( const POINT32 *points, INT32 count, INT32 mode )
{
    return CreatePolyPolygonRgn32( points, &count, 1, mode );
}


/***********************************************************************
 *           CreatePolyPolygonRgn32    (GDI32.57)
 */
HRGN32 CreatePolyPolygonRgn32( const POINT32 * points, const INT32 * count,
                               INT32 nbpolygons, INT32 mode )
{
    RGNOBJ * obj;
    HRGN32 hrgn;
    int i, j, maxPoints;
    XPoint *xpoints, *pt;
    Region xrgn;

      /* Allocate points array */

    if (!nbpolygons) return 0;
    for (i = maxPoints = 0; i < nbpolygons; i++)
	if (maxPoints < count[i]) maxPoints = count[i];
    if (!maxPoints) return 0;
    if (!(xpoints = (XPoint *)HeapAlloc( GetProcessHeap(), 0,
                                         sizeof(XPoint) * maxPoints )))
        return 0;

      /* Allocate region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC )))
    {
	HeapFree( GetProcessHeap(), 0, xpoints );
	return 0;
    }
    obj = (RGNOBJ *) GDI_HEAP_LIN_ADDR( hrgn );
    obj->xrgn = 0;
    dprintf_region(stddeb, "CreatePolyPolygonRgn: %d polygons, returning %04x\n",
                   nbpolygons, hrgn );

      /* Create X region */

    for (i = 0; i < nbpolygons; i++, count++)
    {
	for (j = *count, pt = xpoints; j > 0; j--, points++, pt++)
	{
	    pt->x = points->x;
	    pt->y = points->y;
	}
	xrgn = XPolygonRegion( xpoints, *count,
			       (mode == WINDING) ? WindingRule : EvenOddRule );
	if (!xrgn)
        {
            if (obj->xrgn) XDestroyRegion( obj->xrgn );
            HeapFree( GetProcessHeap(), 0, xpoints );
            GDI_FreeObject( hrgn );
            return 0;
        }
	if (i > 0)
	{
	    Region tmprgn = XCreateRegion();
	    if (mode == WINDING) XUnionRegion( xrgn, obj->xrgn, tmprgn );
	    else XXorRegion( xrgn, obj->xrgn, tmprgn );
	    XDestroyRegion( obj->xrgn );
	    obj->xrgn = tmprgn;
	}
	else obj->xrgn = xrgn;
    }

    HeapFree( GetProcessHeap(), 0, xpoints );
    return hrgn;
}


/***********************************************************************
 *           PtInRegion16    (GDI.161)
 */
BOOL16 PtInRegion16( HRGN16 hrgn, INT16 x, INT16 y )
{
    return PtInRegion32( hrgn, x, y );
}


/***********************************************************************
 *           PtInRegion32    (GDI32.278)
 */
BOOL32 PtInRegion32( HRGN32 hrgn, INT32 x, INT32 y )
{
    RGNOBJ * obj;
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;
    if (!obj->xrgn) return FALSE;
    return XPointInRegion( obj->xrgn, x, y );
}


/***********************************************************************
 *           RectInRegion16    (GDI.181)
 */
BOOL16 RectInRegion16( HRGN16 hrgn, const RECT16 *rect )
{
    RGNOBJ * obj;
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;
    if (!obj->xrgn) return FALSE;
    return (XRectInRegion( obj->xrgn, rect->left, rect->top,
                           rect->right-rect->left,
                           rect->bottom-rect->top ) != RectangleOut);
}


/***********************************************************************
 *           RectInRegion32    (GDI32.281)
 */
BOOL32 RectInRegion32( HRGN32 hrgn, const RECT32 *rect )
{
    RGNOBJ * obj;
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;
    if (!obj->xrgn) return FALSE;
    return (XRectInRegion( obj->xrgn, rect->left, rect->top,
                           rect->right-rect->left,
                           rect->bottom-rect->top ) != RectangleOut);
}


/***********************************************************************
 *           EqualRgn16    (GDI.72)
 */
BOOL16 EqualRgn16( HRGN16 rgn1, HRGN16 rgn2 )
{
    return EqualRgn32( rgn1, rgn2 );
}


/***********************************************************************
 *           EqualRgn32    (GDI32.90)
 */
BOOL32 EqualRgn32( HRGN32 rgn1, HRGN32 rgn2 )
{
    RGNOBJ *obj1, *obj2;
    if (!(obj1 = (RGNOBJ *) GDI_GetObjPtr( rgn1, REGION_MAGIC ))) return FALSE;
    if (!(obj2 = (RGNOBJ *) GDI_GetObjPtr( rgn2, REGION_MAGIC ))) return FALSE;
    if (!obj1->xrgn || !obj2->xrgn) return (!obj1->xrgn && !obj2->xrgn);
    return XEqualRegion( obj1->xrgn, obj2->xrgn );
}


/***********************************************************************
 *           REGION_CopyRegion
 *
 * Copy region src into dest.
 */
static INT32 REGION_CopyRegion( RGNOBJ *src, RGNOBJ *dest )
{
    Region tmprgn;
    if (src->xrgn)
    {
        if (src->xrgn == dest->xrgn) return COMPLEXREGION;
        tmprgn = XCreateRegion();
        if (!dest->xrgn) dest->xrgn = XCreateRegion();
        XUnionRegion( tmprgn, src->xrgn, dest->xrgn );
        XDestroyRegion( tmprgn );
        return COMPLEXREGION;
    }
    else
    {
        if (dest->xrgn) XDestroyRegion( dest->xrgn );
        dest->xrgn = 0;
        return NULLREGION;
    }
}

/***********************************************************************
 *           REGION_UnionRectWithRgn
 *
 * Add rectangle to region
 */
BOOL32 REGION_UnionRectWithRgn( HRGN32 hRgn, const RECT32 *rc )
{
  RGNOBJ 	*rgnObj = (RGNOBJ*) GDI_GetObjPtr( hRgn, REGION_MAGIC );
  XRectangle     rect = { rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top };
  BOOL32 ret = FALSE;

  if( rgnObj )
  {
    if( !rgnObj->xrgn )
    {
      if (!(rgnObj->xrgn = XCreateRegion()))
      {
         GDI_FreeObject( hRgn );
         return 0;
      }
      ret = SIMPLEREGION; 
    }
    else
      ret = COMPLEXREGION;
    XUnionRectWithRegion( &rect, rgnObj->xrgn, rgnObj->xrgn );
  }
  return ret;
}


/***********************************************************************
 *           REGION_CreateFrameRgn
 *
 * Create a region that is a frame around another region
 */
BOOL32 REGION_FrameRgn( HRGN32 hDest, HRGN32 hSrc, INT32 x, INT32 y )
{
    RGNOBJ *destObj,*srcObj;
    Region result;

    destObj = (RGNOBJ*) GDI_GetObjPtr( hDest, REGION_MAGIC );
    srcObj  = (RGNOBJ*) GDI_GetObjPtr( hSrc, REGION_MAGIC );
    if (!srcObj->xrgn) return FALSE;
    REGION_CopyRegion( srcObj, destObj );
    XShrinkRegion( destObj->xrgn, -x, -y );
    result = XCreateRegion();
    XSubtractRegion( destObj->xrgn, srcObj->xrgn, result );
    XDestroyRegion( destObj->xrgn );
    destObj->xrgn = result;
    return TRUE;
}


/***********************************************************************
 *           CombineRgn16    (GDI.451)
 */
INT16 CombineRgn16( HRGN16 hDest, HRGN16 hSrc1, HRGN16 hSrc2, INT16 mode )
{
    return (INT16)CombineRgn32( hDest, hSrc1, hSrc2, mode );
}


/***********************************************************************
 *           CombineRgn32   (GDI32.19)
 *
 * Note: The behavior is correct even if src and dest regions are the same.
 */
INT32 CombineRgn32( HRGN32 hDest, HRGN32 hSrc1, HRGN32 hSrc2, INT32 mode )
{
    RGNOBJ *destObj, *src1Obj, *src2Obj;
    Region destrgn;

    dprintf_region(stddeb, "CombineRgn: %04x,%04x -> %04x mode=%x\n", 
		   hSrc1, hSrc2, hDest, mode );
    
    if (!(destObj = (RGNOBJ *) GDI_GetObjPtr( hDest, REGION_MAGIC )))
	return ERROR;
    if (!(src1Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc1, REGION_MAGIC )))
	return ERROR;
    if (mode == RGN_COPY) return REGION_CopyRegion( src1Obj, destObj );

    if (!(src2Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc2, REGION_MAGIC )))
        return ERROR;

      /* Some optimizations for null regions */

    if (!src1Obj->xrgn || !src2Obj->xrgn)
    {
        switch(mode)
        {
        case RGN_DIFF:
            if (src1Obj->xrgn)
                return REGION_CopyRegion( src1Obj, destObj );
            /* else fall through */
        case RGN_AND:
            if (destObj->xrgn) XDestroyRegion( destObj->xrgn );
	    destObj->xrgn = 0;
	    return NULLREGION;
        case RGN_OR:
        case RGN_XOR:
            if (src1Obj->xrgn)
                return REGION_CopyRegion( src1Obj, destObj );
            else
                return REGION_CopyRegion( src2Obj, destObj );
	default:
	    return ERROR;
        }
    }

      /* Perform the operation with the two X regions */

    if (!(destrgn = XCreateRegion())) return ERROR;
    switch(mode)
    {
    case RGN_AND:
        XIntersectRegion( src1Obj->xrgn, src2Obj->xrgn, destrgn );
        break;
    case RGN_OR:
        XUnionRegion( src1Obj->xrgn, src2Obj->xrgn, destrgn );
        break;
    case RGN_XOR:
        XXorRegion( src1Obj->xrgn, src2Obj->xrgn, destrgn );
        break;
    case RGN_DIFF:
        XSubtractRegion( src1Obj->xrgn, src2Obj->xrgn, destrgn );
        break;
    default:
        XDestroyRegion( destrgn );
        return ERROR;
    }
    if (destObj->xrgn) XDestroyRegion( destObj->xrgn );
    destObj->xrgn = destrgn;
    if (XEmptyRegion(destObj->xrgn))
    {
        XDestroyRegion( destObj->xrgn );
        destObj->xrgn = 0;
        return NULLREGION;
    }
    return COMPLEXREGION;
}

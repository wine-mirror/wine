/*
 * GDI region objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdi.h"

  /* GC used for region operations */
static GC regionGC = 0;

/***********************************************************************
 *           REGION_Init
 */
BOOL REGION_Init()
{
    Pixmap tmpPixmap;

      /* CreateGC needs a drawable */
    tmpPixmap = XCreatePixmap( display, rootWindow, 1, 1, 1 );
    if (tmpPixmap)
    {
	regionGC = XCreateGC( XT_display, tmpPixmap, 0, NULL );
	XFreePixmap( XT_display, tmpPixmap );
	if (!regionGC) return FALSE;
	XSetForeground( XT_display, regionGC, 1 );
	XSetGraphicsExposures( XT_display, regionGC, False );
	return TRUE;
    }
    else return FALSE;
}


/***********************************************************************
 *           REGION_SetRect
 *
 * Set the bounding box of the region and create the pixmap.
 * The hrgn must be valid.
 */
static BOOL REGION_SetRect( HRGN hrgn, LPRECT rect )
{
    int width, height;

      /* Fill region */

    REGION * region = &((RGNOBJ *)GDI_HEAP_ADDR( hrgn ))->region;
    width  = rect->right - rect->left;
    height = rect->bottom - rect->top;
    if ((width <= 0) || (height <= 0))
    {
	region->type       = NULLREGION;
	region->box.left   = 0;
	region->box.right  = 0;
	region->box.top    = 0;
	region->box.bottom = 0;
	region->pixmap     = 0;
	return TRUE;
    }
    region->type = SIMPLEREGION;
    region->box  = *rect;
    
      /* Create pixmap */

    region->pixmap = XCreatePixmap( display, rootWindow, width, height, 1 );
    if (!region->pixmap) return FALSE;

      /* Fill pixmap */

    XSetFunction( XT_display, regionGC, GXclear );
    XFillRectangle( XT_display, region->pixmap, regionGC,
		    0, 0, width, height );
    return TRUE;
}


/***********************************************************************
 *           REGION_DeleteObject
 */
BOOL REGION_DeleteObject( HRGN hrgn, RGNOBJ * obj )
{
    if (obj->region.pixmap) XFreePixmap( XT_display, obj->region.pixmap );
    return GDI_FreeObject( hrgn );
}


/***********************************************************************
 *           OffsetRgn    (GDI.101)
 */
int OffsetRgn( HRGN hrgn, short x, short y )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (!obj) return ERROR;
#ifdef DEBUG_REGION
    printf( "OffsetRgn: %d %d,%d\n", hrgn, x, y );
#endif
    OffsetRect( &obj->region.box, x, y );
    return obj->region.type;
}


/***********************************************************************
 *           GetRgnBox    (GDI.134)
 */
int GetRgnBox( HRGN hrgn, LPRECT rect )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (!obj) return ERROR;
#ifdef DEBUG_REGION
    printf( "GetRgnBox: %d\n", hrgn );
#endif
    *rect = obj->region.box;
    return obj->region.type;
}


/***********************************************************************
 *           CreateRectRgn    (GDI.64)
 */
HRGN CreateRectRgn( short left, short top, short right, short bottom )
{
    RECT rect = { left, top, right, bottom };    
    return CreateRectRgnIndirect( &rect );
}


/***********************************************************************
 *           CreateRectRgnIndirect    (GDI.65)
 */
HRGN CreateRectRgnIndirect( LPRECT rect )
{
    RGNOBJ * rgnObj;
    HRGN hrgn;

#ifdef DEBUG_REGION
    printf( "CreateRectRgnIndirect: %d,%d-%d,%d\n",
	    rect->left, rect->top, rect->right, rect->bottom );
#endif
    
      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    if (!REGION_SetRect( hrgn, rect ))
    {
	GDI_FreeObject( hrgn );
	return 0;
    }
    rgnObj = (RGNOBJ *) GDI_HEAP_ADDR( hrgn );

      /* Fill pixmap */
    
    if (rgnObj->region.type != NULLREGION)
    {
	int width  = rgnObj->region.box.right - rgnObj->region.box.left;
	int height = rgnObj->region.box.bottom - rgnObj->region.box.top;
	XSetFunction( XT_display, regionGC, GXcopy );
	XFillRectangle( XT_display, rgnObj->region.pixmap, regionGC,
		        0, 0, width, height );
    }
    
    return hrgn;
}


/***********************************************************************
 *           CreateRoundRectRgn    (GDI.444)
 */
HRGN CreateRoundRectRgn( short left, short top, short right, short bottom,
			 short ellipse_width, short ellipse_height )
{
    RECT rect = { left, top, right, bottom };    
    RGNOBJ * rgnObj;
    HRGN hrgn;

#ifdef DEBUG_REGION
    printf( "CreateRoundRectRgn: %d,%d-%d,%d %dx%d\n",
	    left, top, right, bottom, ellipse_width, ellipse_height );
#endif
    
      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    if (!REGION_SetRect( hrgn, &rect ))
    {
	GDI_FreeObject( hrgn );
	return 0;
    }
    rgnObj = (RGNOBJ *) GDI_HEAP_ADDR( hrgn );

      /* Fill pixmap */
    
    if (rgnObj->region.type != NULLREGION)
    {
	int width  = rgnObj->region.box.right - rgnObj->region.box.left;
	int height = rgnObj->region.box.bottom - rgnObj->region.box.top;
	XSetFunction( XT_display, regionGC, GXcopy );
	XFillRectangle( XT_display, rgnObj->region.pixmap, regionGC,
		        0, ellipse_height / 2,
		        width, height - ellipse_height );
	XFillRectangle( XT_display, rgnObj->region.pixmap, regionGC,
		        ellipse_width / 2, 0,
		        width - ellipse_width, height );
	XFillArc( XT_display, rgnObj->region.pixmap, regionGC,
		  0, 0,
		  ellipse_width, ellipse_height, 0, 360*64 );
	XFillArc( XT_display, rgnObj->region.pixmap, regionGC,
		  width - ellipse_width, 0,
		  ellipse_width, ellipse_height, 0, 360*64 );
	XFillArc( XT_display, rgnObj->region.pixmap, regionGC,
		  0, height - ellipse_height,
		  ellipse_width, ellipse_height, 0, 360*64 );
	XFillArc( XT_display, rgnObj->region.pixmap, regionGC,
		  width - ellipse_width, height - ellipse_height,
		  ellipse_width, ellipse_height, 0, 360*64 );
    }
    
    return hrgn;
}


/***********************************************************************
 *           SetRectRgn    (GDI.172)
 */
void SetRectRgn( HRGN hrgn, short left, short top, short right, short bottom )
{
    RECT rect = { left, top, right, bottom };    
    RGNOBJ * rgnObj;

#ifdef DEBUG_REGION
    printf( "SetRectRgn: %d %d,%d-%d,%d\n", hrgn, left, top, right, bottom );
#endif
    
      /* Free previous pixmap */

    if (!(rgnObj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return;
    if (rgnObj->region.pixmap) 
	XFreePixmap( XT_display, rgnObj->region.pixmap );

    if (!REGION_SetRect( hrgn, &rect )) return;

      /* Fill pixmap */
    
    if (rgnObj->region.type != NULLREGION)
    {
	int width  = rgnObj->region.box.right - rgnObj->region.box.left;
	int height = rgnObj->region.box.bottom - rgnObj->region.box.top;
	XSetFunction( XT_display, regionGC, GXcopy );
	XFillRectangle( XT_display, rgnObj->region.pixmap, regionGC,
		        0, 0, width, height );
    }
}


/***********************************************************************
 *           CreateEllipticRgn    (GDI.54)
 */
HRGN CreateEllipticRgn( short left, short top, short right, short bottom )
{
    RECT rect = { left, top, right, bottom };    
    return CreateEllipticRgnIndirect( &rect );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect    (GDI.55)
 */
HRGN CreateEllipticRgnIndirect( LPRECT rect )
{
    RGNOBJ * rgnObj;
    HRGN hrgn;

#ifdef DEBUG_REGION
    printf( "CreateEllipticRgnIndirect: %d,%d-%d,%d\n",
	    rect->left, rect->top, rect->right, rect->bottom );
#endif
    
      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    if (!REGION_SetRect( hrgn, rect ))
    {
	GDI_FreeObject( hrgn );
	return 0;
    }
    rgnObj = (RGNOBJ *) GDI_HEAP_ADDR( hrgn );

      /* Fill pixmap */
    
    if (rgnObj->region.type != NULLREGION)
    {
	int width  = rgnObj->region.box.right - rgnObj->region.box.left;
	int height = rgnObj->region.box.bottom - rgnObj->region.box.top;
	XSetFunction( XT_display, regionGC, GXcopy );
	XFillArc( XT_display, rgnObj->region.pixmap, regionGC,
		  0, 0, width, height, 0, 360*64 );
    }
    
    return hrgn;
}


/***********************************************************************
 *           CreatePolygonRgn    (GDI.63)
 */
HRGN CreatePolygonRgn( POINT * points, short count, short mode )
{
    return CreatePolyPolygonRgn( points, &count, 1, mode );
}


/***********************************************************************
 *           CreatePolyPolygonRgn    (GDI.451)
 */
HRGN CreatePolyPolygonRgn( POINT * points, short * count,
			   short nbpolygons, short mode )
{
    RGNOBJ * rgnObj;
    HRGN hrgn;
    RECT box;
    int i, j, totalPoints;
    POINT * pt;
    XPoint * xpoints;
    
    if (!nbpolygons) return 0;
#ifdef DEBUG_REGION
    printf( "CreatePolyPolygonRgn: %d polygons\n", nbpolygons );
#endif
    
      /* Find bounding box */

    box.top   = box.left   = 32767;
    box.right = box.bottom = 0;
    for (i = totalPoints = 0, pt = points; i < nbpolygons; i++)
    {
	totalPoints += count[i];
	for (j = 0; j < count[i]; j++, pt++)
	{
	    if (pt->x < box.left) box.left = pt->x;
	    if (pt->x > box.right) box.right = pt->x;
	    if (pt->y < box.top) box.top = pt->y;
	    if (pt->y > box.bottom) box.bottom = pt->y;
	}
    }        
    if (!totalPoints) return 0;
    
      /* Build points array */

    xpoints = (XPoint *) malloc( sizeof(XPoint) * totalPoints );
    if (!xpoints) return 0;
    for (i = 0, pt = points; i < totalPoints; i++, pt++)
    {
	xpoints[i].x = pt->x - box.left;
	xpoints[i].y = pt->y - box.top;
    }

      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC )) ||
	!REGION_SetRect( hrgn, &box ))
    {
	if (hrgn) GDI_FreeObject( hrgn );
	free( xpoints );
	return 0;
    }
    rgnObj = (RGNOBJ *) GDI_HEAP_ADDR( hrgn );

      /* Fill pixmap */

    if (rgnObj->region.type != NULLREGION)
    {
	XSetFunction( XT_display, regionGC, GXcopy );
	if (mode == WINDING) XSetFillRule( XT_display, regionGC, WindingRule );
	else XSetFillRule( XT_display, regionGC, EvenOddRule );
	for (i = j = 0; i < nbpolygons; i++)
	{
	    XFillPolygon( XT_display, rgnObj->region.pixmap, regionGC,
			  &xpoints[j], count[i], Complex, CoordModeOrigin );
	    j += count[i];
	}
    }
    
    free( xpoints );
    return hrgn;
}


/***********************************************************************
 *           PtInRegion    (GDI.161)
 */
BOOL PtInRegion( HRGN hrgn, short x, short y )
{
    XImage * image;
    BOOL res;
    RGNOBJ * obj;
    POINT pt = { x, y };
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;
    if (!PtInRect( &obj->region.box, pt )) return FALSE;
    image = XGetImage( XT_display, obj->region.pixmap,
		       x - obj->region.box.left, y - obj->region.box.top,
		       1, 1, AllPlanes, ZPixmap );
    if (!image) return FALSE;
    res = (XGetPixel( image, 0, 0 ) != 0);
    XDestroyImage( image );
    return res;
}


/***********************************************************************
 *           RectInRegion    (GDI.181)
 */
BOOL RectInRegion( HRGN hrgn, LPRECT rect )
{
    XImage * image;
    RGNOBJ * obj;
    RECT intersect;
    int x, y;
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;
    if (!IntersectRect( &intersect, &obj->region.box, rect )) return FALSE;
    
    image = XGetImage( XT_display, obj->region.pixmap,
		       intersect.left - obj->region.box.left,
		       intersect.top - obj->region.box.top,
		       intersect.right - intersect.left,
		       intersect.bottom - intersect.top,
		       AllPlanes, ZPixmap );
    if (!image) return FALSE;
    for (y = 0; y < image->height; y++)
	for (x = 0; x < image->width; x++)
	    if (XGetPixel( image, x, y ) != 0)
	    {
		XDestroyImage( image );
		return TRUE;
	    }
    
    XDestroyImage( image );
    return FALSE;
}


/***********************************************************************
 *           EqualRgn    (GDI.72)
 */
BOOL EqualRgn( HRGN rgn1, HRGN rgn2 )
{
    RGNOBJ *obj1, *obj2;
    XImage *image1, *image2;
    int width, height, x, y;

      /* Compare bounding boxes */

    if (!(obj1 = (RGNOBJ *) GDI_GetObjPtr( rgn1, REGION_MAGIC ))) return FALSE;
    if (!(obj2 = (RGNOBJ *) GDI_GetObjPtr( rgn2, REGION_MAGIC ))) return FALSE;
    if (obj1->region.type == NULLREGION)
	return (obj2->region.type == NULLREGION);
    else if (obj2->region.type == NULLREGION) return FALSE;
    if (!EqualRect( &obj1->region.box, &obj2->region.box )) return FALSE;

      /* Get pixmap contents */

    width  = obj1->region.box.right - obj1->region.box.left;
    height = obj1->region.box.bottom - obj1->region.box.top;
    image1 = XGetImage( XT_display, obj1->region.pixmap,
		        0, 0, width, height, AllPlanes, ZPixmap );
    if (!image1) return FALSE;
    image2 = XGetImage( XT_display, obj2->region.pixmap,
		        0, 0, width, height, AllPlanes, ZPixmap );
    if (!image2)
    {
	XDestroyImage( image1 );
	return FALSE;
    }
    
      /* Compare pixmaps */
    for (y = 0; y < height; y++)
	for (x = 0; x < width; x++)
	    if (XGetPixel( image1, x, y ) != XGetPixel( image2, x, y))
	    {
		XDestroyImage( image1 );
		XDestroyImage( image2 );
		return FALSE;
	    }
    
    XDestroyImage( image1 );
    XDestroyImage( image2 );
    return TRUE;
}


/***********************************************************************
 *           REGION_CopyIntersection
 *
 * Copy to dest->pixmap the area of src->pixmap delimited by
 * the intersection of dest and src regions, using the current GC function.
 */
void REGION_CopyIntersection( REGION * dest, REGION * src )
{
    RECT inter;
    if (!IntersectRect( &inter, &dest->box, &src->box )) return;
    XCopyArea( XT_display, src->pixmap, dest->pixmap, regionGC,
	       inter.left - src->box.left, inter.top - src->box.top,
	       inter.right - inter.left, inter.bottom - inter.top,
	       inter.left - dest->box.left, inter.top - dest->box.top );
}


/***********************************************************************
 *           CombineRgn    (GDI.451)
 */
int CombineRgn( HRGN hDest, HRGN hSrc1, HRGN hSrc2, short mode )
{
    RGNOBJ *destObj, *src1Obj, *src2Obj;
    REGION * region;
    int width, height;
    BOOL res;
    
#ifdef DEBUG_REGION
    printf( "CombineRgn: %d %d %d %d\n", hDest, hSrc1, hSrc2, mode );
#endif
    
    if (!(destObj = (RGNOBJ *) GDI_GetObjPtr( hDest, REGION_MAGIC )))
	return ERROR;
    if (!(src1Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc1, REGION_MAGIC )))
	return ERROR;
    if (mode != RGN_COPY)
	if (!(src2Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc2, REGION_MAGIC )))
	    return ERROR;
    region = &destObj->region;

    switch(mode)
    {
      case RGN_AND:
	res = IntersectRect( &region->box, &src1Obj->region.box,
			     &src2Obj->region.box );
	region->type = COMPLEXREGION;
	break;

      case RGN_OR:
      case RGN_XOR:
	res = UnionRect( &region->box, &src1Obj->region.box,
			 &src2Obj->region.box );
	region->type = COMPLEXREGION;
	break;

      case RGN_DIFF:
	res = SubtractRect( &region->box, &src1Obj->region.box,
			    &src2Obj->region.box );
	region->type = COMPLEXREGION;
	break;

      case RGN_COPY:
	region->box  = src1Obj->region.box;
	region->type = src1Obj->region.type;
	res = (region->type != NULLREGION);
	break;

      default:
	return ERROR;
    }

    if (region->pixmap) XFreePixmap( XT_display, region->pixmap );    
    if (!res)
    {
	region->type   = NULLREGION;
	region->pixmap = 0;
	return NULLREGION;
    }
    
    width  = region->box.right - region->box.left;
    height = region->box.bottom - region->box.top;
    if (!width || !height)
    {
	printf( "CombineRgn: width or height is 0. Please report this.\n" );
	printf( "src1=%d,%d-%d,%d  src2=%d,%d-%d,%d  dst=%d,%d-%d,%d  op=%d\n",
	        src1Obj->region.box.left, src1Obj->region.box.top,
	        src1Obj->region.box.right, src1Obj->region.box.bottom,
	        src2Obj->region.box.left, src2Obj->region.box.top,
	        src2Obj->region.box.right, src2Obj->region.box.bottom,
	        region->box.left, region->box.top,
	        region->box.right, region->box.bottom, mode );
	exit(1);
    }
    region->pixmap = XCreatePixmap( display, rootWindow, width, height, 1 );

    switch(mode)
    {
      case RGN_AND:
	  XSetFunction( XT_display, regionGC, GXcopy );
	  REGION_CopyIntersection( region, &src1Obj->region );
	  XSetFunction( XT_display, regionGC, GXand );
	  REGION_CopyIntersection( region, &src2Obj->region );
	  break;

      case RGN_OR:
      case RGN_XOR:
	  XSetFunction( XT_display, regionGC, GXclear );
	  XFillRectangle( XT_display, region->pixmap, regionGC,
			  0, 0, width, height );
	  XSetFunction( XT_display, regionGC, (mode == RGN_OR) ? GXor : GXxor);
	  REGION_CopyIntersection( region, &src1Obj->region );
	  REGION_CopyIntersection( region, &src2Obj->region );
	  break;
	  
      case RGN_DIFF:
	  XSetFunction( XT_display, regionGC, GXclear );
	  XFillRectangle( XT_display, region->pixmap, regionGC,
			  0, 0, width, height );
	  XSetFunction( XT_display, regionGC, GXcopy );
	  REGION_CopyIntersection( region, &src1Obj->region );
	  XSetFunction( XT_display, regionGC, GXandInverted );
	  REGION_CopyIntersection( region, &src2Obj->region );
	  break;
	  
      case RGN_COPY:
	  XSetFunction( XT_display, regionGC, GXcopy );
	  XCopyArea( XT_display, src1Obj->region.pixmap, region->pixmap,
		     regionGC, 0, 0, width, height, 0, 0 );
	  break;
    }
    return region->type;
}

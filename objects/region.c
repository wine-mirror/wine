/*
 * GDI region objects
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993, 1994";
*/
#include <stdlib.h>
#include <stdio.h>

#include "gdi.h"
#include "stddebug.h"
/* #define DEBUG_REGION */
#include "debug.h"

  /* GC used for region operations */
static GC regionGC = 0;

/***********************************************************************
 *           REGION_Init
 */
BOOL REGION_Init(void)
{
    Pixmap tmpPixmap;

      /* CreateGC needs a drawable */
    tmpPixmap = XCreatePixmap( display, rootWindow, 1, 1, 1 );
    if (tmpPixmap)
    {
	regionGC = XCreateGC( display, tmpPixmap, 0, NULL );
	XFreePixmap( display, tmpPixmap );
	if (!regionGC) return FALSE;
	XSetForeground( display, regionGC, 1 );
	XSetGraphicsExposures( display, regionGC, False );
	return TRUE;
    }
    else return FALSE;
}


/***********************************************************************
 *           REGION_MakePixmap
 *
 * Make a pixmap of an X region.
 */
static BOOL REGION_MakePixmap( REGION *region )
{
    int width = region->box.right - region->box.left;
    int height = region->box.bottom - region->box.top;

    if (!region->xrgn) return TRUE;  /* Null region */
    region->pixmap = XCreatePixmap( display, rootWindow, width, height, 1 );
    if (!region->pixmap) return FALSE;
    XSetRegion( display, regionGC, region->xrgn );
    XSetClipOrigin( display, regionGC, -region->box.left, -region->box.top );
    XSetFunction( display, regionGC, GXset );
    XFillRectangle( display, region->pixmap, regionGC, 0, 0, width, height );
    XSetClipMask( display, regionGC, None );  /* Clear clip region */
    return TRUE;
}


/***********************************************************************
 *           REGION_SetRect
 *
 * Set the bounding box of the region and create the pixmap (or the X rgn).
 * The hrgn must be valid.
 */
static BOOL REGION_SetRect( HRGN hrgn, LPRECT rect, BOOL createXrgn )
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
	region->xrgn       = 0;
	return TRUE;
    }
    region->type   = SIMPLEREGION;
    region->box    = *rect;
    region->xrgn   = 0;
    region->pixmap = 0;

    if (createXrgn)  /* Create and set the X region */
    {
	XRectangle xrect = { region->box.left, region->box.top, width, height};
	if (!(region->xrgn = XCreateRegion())) return FALSE;
        XUnionRectWithRegion( &xrect, region->xrgn, region->xrgn );
    }
    else  /* Create the pixmap */
    {
	region->pixmap = XCreatePixmap( display, rootWindow, width, height, 1);
	if (!region->pixmap) return FALSE;
	  /* Fill the pixmap */
	XSetFunction( display, regionGC, GXclear );
	XFillRectangle(display, region->pixmap, regionGC, 0, 0, width, height);
    }
    return TRUE;
}


/***********************************************************************
 *           REGION_DeleteObject
 */
BOOL REGION_DeleteObject( HRGN hrgn, RGNOBJ * obj )
{
    if (obj->region.pixmap) XFreePixmap( display, obj->region.pixmap );
    if (obj->region.xrgn) XDestroyRegion( obj->region.xrgn );
    return GDI_FreeObject( hrgn );
}


/***********************************************************************
 *           OffsetRgn    (GDI.101)
 */
int OffsetRgn( HRGN hrgn, short x, short y )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (!obj) return ERROR;
    dprintf_region(stddeb, "OffsetRgn: %d %d,%d\n", hrgn, x, y );
    OffsetRect( &obj->region.box, x, y );
    if (obj->region.xrgn) XOffsetRegion( obj->region.xrgn, x, y );
    return obj->region.type;
}


/***********************************************************************
 *           GetRgnBox    (GDI.134)
 */
int GetRgnBox( HRGN hrgn, LPRECT rect )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (!obj) return ERROR;
    dprintf_region(stddeb, "GetRgnBox: %d\n", hrgn );
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
    HRGN hrgn;

    dprintf_region(stddeb, "CreateRectRgnIndirect: %d,%d-%d,%d\n",
	    rect->left, rect->top, rect->right, rect->bottom );
    
      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    if (!REGION_SetRect( hrgn, rect, TRUE ))
    {
	GDI_FreeObject( hrgn );
	return 0;
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

    dprintf_region(stddeb, "CreateRoundRectRgn: %d,%d-%d,%d %dx%d\n",
	    left, top, right, bottom, ellipse_width, ellipse_height );
    
      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    if (!REGION_SetRect( hrgn, &rect, FALSE ))
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
	XSetFunction( display, regionGC, GXcopy );
	XFillRectangle( display, rgnObj->region.pixmap, regionGC,
		        0, ellipse_height / 2,
		        width, height - ellipse_height );
	XFillRectangle( display, rgnObj->region.pixmap, regionGC,
		        ellipse_width / 2, 0,
		        width - ellipse_width, height );
	XFillArc( display, rgnObj->region.pixmap, regionGC,
		  0, 0,
		  ellipse_width, ellipse_height, 0, 360*64 );
	XFillArc( display, rgnObj->region.pixmap, regionGC,
		  width - ellipse_width, 0,
		  ellipse_width, ellipse_height, 0, 360*64 );
	XFillArc( display, rgnObj->region.pixmap, regionGC,
		  0, height - ellipse_height,
		  ellipse_width, ellipse_height, 0, 360*64 );
	XFillArc( display, rgnObj->region.pixmap, regionGC,
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

    dprintf_region(stddeb, "SetRectRgn: %d %d,%d-%d,%d\n", 
		   hrgn, left, top, right, bottom );
    
      /* Free previous pixmap */

    if (!(rgnObj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return;
    if (rgnObj->region.pixmap) XFreePixmap( display, rgnObj->region.pixmap );
    if (rgnObj->region.xrgn) XDestroyRegion( rgnObj->region.xrgn );
    REGION_SetRect( hrgn, &rect, TRUE );
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

    dprintf_region(stddeb, "CreateEllipticRgnIndirect: %d,%d-%d,%d\n",
	    rect->left, rect->top, rect->right, rect->bottom );
    
      /* Create region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC ))) return 0;
    if (!REGION_SetRect( hrgn, rect, FALSE ))
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
	XSetFunction( display, regionGC, GXcopy );
	XFillArc( display, rgnObj->region.pixmap, regionGC,
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
    int i, j, maxPoints;
    XPoint *xpoints, *pt;
    XRectangle rect;
    Region xrgn;

    dprintf_region(stddeb, "CreatePolyPolygonRgn: %d polygons\n", nbpolygons );

      /* Allocate points array */

    if (!nbpolygons) return 0;
    for (i = maxPoints = 0; i < nbpolygons; i++)
	if (maxPoints < count[i]) maxPoints = count[i];
    if (!maxPoints) return 0;
    if (!(xpoints = (XPoint *) malloc( sizeof(XPoint) * maxPoints )))
	return 0;

      /* Allocate region */

    if (!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC )))
    {
	free( xpoints );
	return 0;
    }
    rgnObj = (RGNOBJ *) GDI_HEAP_ADDR( hrgn );
    rgnObj->region.type   = SIMPLEREGION;
    rgnObj->region.pixmap = 0;

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
	if (!xrgn) break;
	if (i > 0)
	{
	    Region tmprgn = XCreateRegion();
	    if (mode == WINDING) XUnionRegion(xrgn,rgnObj->region.xrgn,tmprgn);
	    else XXorRegion( xrgn, rgnObj->region.xrgn, tmprgn );
	    XDestroyRegion( rgnObj->region.xrgn );
	    rgnObj->region.xrgn = tmprgn;
	}
	else rgnObj->region.xrgn = xrgn;
    }

    free( xpoints );
    if (!xrgn)
    {
	GDI_FreeObject( hrgn );
	return 0;
    }
    XClipBox( rgnObj->region.xrgn, &rect );
    SetRect( &rgnObj->region.box, rect.x, rect.y,
	     rect.x + rect.width, rect.y + rect.height);
    return hrgn;
}


/***********************************************************************
 *           PtInRegion    (GDI.161)
 */
BOOL PtInRegion( HRGN hrgn, short x, short y )
{
    BOOL res;
    RGNOBJ * obj;
    POINT pt = { x, y };
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;
    if (!PtInRect( &obj->region.box, pt )) return FALSE;
    if (obj->region.xrgn)
    {
	return XPointInRegion( obj->region.xrgn, x, y );
    }
    else
    {
	XImage *image = XGetImage( display, obj->region.pixmap,
			    x - obj->region.box.left, y - obj->region.box.top,
			    1, 1, AllPlanes, ZPixmap );
	if (!image) return FALSE;
	res = (XGetPixel( image, 0, 0 ) != 0);
	XDestroyImage( image );
    }
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
    if (obj->region.xrgn)
    {
	return (XRectInRegion( obj->region.xrgn, rect->left, rect->top,
			       rect->right-rect->left,
			       rect->bottom-rect->top ) != RectangleOut);
    }
    else
    {
	if (!IntersectRect( &intersect, &obj->region.box, rect )) return FALSE;
    
	image = XGetImage( display, obj->region.pixmap,
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
    }
    return FALSE;
}


/***********************************************************************
 *           EqualRgn    (GDI.72)
 */
BOOL EqualRgn( HRGN rgn1, HRGN rgn2 )
{
    RGNOBJ *obj1, *obj2;
    XImage *image1, *image2;
    Pixmap pixmap1, pixmap2;
    int width, height, x, y;

      /* Compare bounding boxes */

    if (!(obj1 = (RGNOBJ *) GDI_GetObjPtr( rgn1, REGION_MAGIC ))) return FALSE;
    if (!(obj2 = (RGNOBJ *) GDI_GetObjPtr( rgn2, REGION_MAGIC ))) return FALSE;
    if (obj1->region.type == NULLREGION)
	return (obj2->region.type == NULLREGION);
    else if (obj2->region.type == NULLREGION) return FALSE;
    if (!EqualRect( &obj1->region.box, &obj2->region.box )) return FALSE;
    if (obj1->region.xrgn && obj2->region.xrgn)
    {
	return XEqualRegion( obj1->region.xrgn, obj2->region.xrgn );
    }
    
      /* Get pixmap contents */

    if (!(pixmap1 = obj1->region.pixmap) &&
	!REGION_MakePixmap( &obj1->region )) return FALSE;
    if (!(pixmap2 = obj2->region.pixmap) &&
	!REGION_MakePixmap( &obj2->region )) return FALSE;
    width  = obj1->region.box.right - obj1->region.box.left;
    height = obj1->region.box.bottom - obj1->region.box.top;
    image1 = XGetImage( display, obj1->region.pixmap,
			0, 0, width, height, AllPlanes, ZPixmap );
    image2 = XGetImage( display, obj2->region.pixmap,
		        0, 0, width, height, AllPlanes, ZPixmap );
    if (!image1 || !image2)
    {
	if (image1) XDestroyImage( image1 );
	if (image2) XDestroyImage( image2 );
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
    XCopyArea( display, src->pixmap, dest->pixmap, regionGC,
	       inter.left - src->box.left, inter.top - src->box.top,
	       inter.right - inter.left, inter.bottom - inter.top,
	       inter.left - dest->box.left, inter.top - dest->box.top );
}


/***********************************************************************
 *           REGION_CopyRegion
 *
 * Copy region src into dest.
 */
static int REGION_CopyRegion( RGNOBJ *src, RGNOBJ *dest )
{
    if (dest->region.pixmap) XFreePixmap( display, dest->region.pixmap );
    dest->region.type   = src->region.type;
    dest->region.box    = src->region.box;
    dest->region.pixmap = 0;
    if (src->region.xrgn)  /* Copy only the X region */
    {
        Region tmprgn = XCreateRegion();
        if (!dest->region.xrgn) dest->region.xrgn = XCreateRegion();
        XUnionRegion( tmprgn, src->region.xrgn, dest->region.xrgn );
        XDestroyRegion( tmprgn );
    }
    else  /* Copy the pixmap (if any) */
    {
        if (dest->region.xrgn)
        {
            XDestroyRegion( dest->region.xrgn );
            dest->region.xrgn = 0;
        }
        if (src->region.pixmap)
        {
            int width = src->region.box.right - src->region.box.left;
            int height = src->region.box.bottom - src->region.box.top;
            
            dest->region.pixmap = XCreatePixmap( display, rootWindow,
                                                 width, height, 1 );
            XSetFunction( display, regionGC, GXcopy );
            XCopyArea( display, src->region.pixmap, dest->region.pixmap,
                       regionGC, 0, 0, width, height, 0, 0 );
        }
    }
    return dest->region.type;
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
    
    dprintf_region(stddeb, "CombineRgn: %d %d %d %d\n", 
		   hDest, hSrc1, hSrc2, mode );
    
    if (!(destObj = (RGNOBJ *) GDI_GetObjPtr( hDest, REGION_MAGIC )))
	return ERROR;
    if (!(src1Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc1, REGION_MAGIC )))
	return ERROR;
    if (mode == RGN_COPY) return REGION_CopyRegion( src1Obj, destObj );

    if (!(src2Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc2, REGION_MAGIC )))
        return ERROR;
    region = &destObj->region;

    if (src1Obj->region.xrgn && src2Obj->region.xrgn)
    {
	/* Perform the operation with X regions */

	if (region->pixmap) XFreePixmap( display, region->pixmap );
	region->pixmap = 0;
	if (!region->xrgn) region->xrgn = XCreateRegion();
	switch(mode)
	{
	case RGN_AND:
	    XIntersectRegion( src1Obj->region.xrgn, src2Obj->region.xrgn,
			      region->xrgn );
	    break;
	case RGN_OR:
	    XUnionRegion( src1Obj->region.xrgn, src2Obj->region.xrgn,
			  region->xrgn );
	    break;
	case RGN_XOR:
	    XXorRegion( src1Obj->region.xrgn, src2Obj->region.xrgn,
			region->xrgn );
	    break;
	case RGN_DIFF:
	    XSubtractRegion( src1Obj->region.xrgn, src2Obj->region.xrgn,
			     region->xrgn );
	    break;
	default:
	    return ERROR;
	}
	if (XEmptyRegion(region->xrgn))
	{
            XDestroyRegion( region->xrgn );
	    region->type = NULLREGION;
	    region->xrgn = 0;
	    return NULLREGION;
	}
	else
	{
	    XRectangle rect;
	    XClipBox( region->xrgn, &rect );
	    region->type       = COMPLEXREGION;
	    region->box.left   = rect.x;
	    region->box.top    = rect.y;
	    region->box.right  = rect.x + rect.width;
	    region->box.bottom = rect.y + rect.height;
	    return COMPLEXREGION;
	}
    }
    else  /* Create pixmaps if needed */
    {
	if (!src1Obj->region.pixmap)
	    if (!REGION_MakePixmap( &src1Obj->region )) return ERROR;
	if (!src2Obj->region.pixmap)
	    if (!REGION_MakePixmap( &src2Obj->region )) return ERROR;
    }
    

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

      default:
	return ERROR;
    }

    if (region->pixmap) XFreePixmap( display, region->pixmap );
    if (region->xrgn) XDestroyRegion( region->xrgn );
    if (!res)
    {
	region->type   = NULLREGION;
	region->pixmap = 0;
	region->xrgn   = 0;
	return NULLREGION;
    }
    
    width  = region->box.right - region->box.left;
    height = region->box.bottom - region->box.top;
    if (!width || !height)
    {
	fprintf(stderr, "CombineRgn: width or height is 0. Please report this.\n" );
	fprintf(stderr, "src1=%d,%d-%d,%d  src2=%d,%d-%d,%d  dst=%d,%d-%d,%d  op=%d\n",
	        src1Obj->region.box.left, src1Obj->region.box.top,
	        src1Obj->region.box.right, src1Obj->region.box.bottom,
	        src2Obj->region.box.left, src2Obj->region.box.top,
	        src2Obj->region.box.right, src2Obj->region.box.bottom,
	        region->box.left, region->box.top,
	        region->box.right, region->box.bottom, mode );
	exit(1);
    }
    region->pixmap = XCreatePixmap( display, rootWindow, width, height, 1 );
    region->xrgn   = 0;

    switch(mode)
    {
      case RGN_AND:
	  XSetFunction( display, regionGC, GXcopy );
	  REGION_CopyIntersection( region, &src1Obj->region );
	  XSetFunction( display, regionGC, GXand );
	  REGION_CopyIntersection( region, &src2Obj->region );
	  break;

      case RGN_OR:
      case RGN_XOR:
	  XSetFunction( display, regionGC, GXclear );
	  XFillRectangle( display, region->pixmap, regionGC,
			  0, 0, width, height );
	  XSetFunction( display, regionGC, (mode == RGN_OR) ? GXor : GXxor);
	  REGION_CopyIntersection( region, &src1Obj->region );
	  REGION_CopyIntersection( region, &src2Obj->region );
	  break;
	  
      case RGN_DIFF:
	  XSetFunction( display, regionGC, GXclear );
	  XFillRectangle( display, region->pixmap, regionGC,
			  0, 0, width, height );
	  XSetFunction( display, regionGC, GXcopy );
	  REGION_CopyIntersection( region, &src1Obj->region );
	  XSetFunction( display, regionGC, GXandInverted );
	  REGION_CopyIntersection( region, &src2Obj->region );
	  break;
    }
    return region->type;
}

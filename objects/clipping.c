/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include "gdi.h"


/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn of VisRgn have changed.
 */
static void CLIPPING_UpdateGCRegion( DC * dc )
{
    if (!dc->w.hGCClipRgn) dc->w.hGCClipRgn = CreateRectRgn(0,0,0,0);

    if (!dc->w.hVisRgn)
    {
	if (!dc->w.hClipRgn)
	{
	    DeleteObject( dc->w.hGCClipRgn );
	    dc->w.hGCClipRgn = 0;
	}
	else
	    CombineRgn( dc->w.hGCClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
    else
    {
	if (!dc->w.hClipRgn)
	    CombineRgn( dc->w.hGCClipRgn, dc->w.hVisRgn, 0, RGN_COPY );
	else
	    CombineRgn( dc->w.hGCClipRgn, dc->w.hClipRgn, dc->w.hVisRgn, RGN_AND );
    }
    
    if (dc->w.hGCClipRgn)
    {
	RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr( dc->w.hGCClipRgn, REGION_MAGIC );
	XSetClipMask( XT_display, dc->u.x.gc, obj->region.pixmap );
	XSetClipOrigin( XT_display, dc->u.x.gc,
		        obj->region.box.left, obj->region.box.top );
    }
    else
    {
	XSetClipMask( XT_display, dc->u.x.gc, None );
	XSetClipOrigin( XT_display, dc->u.x.gc, 0, 0 );
    }	
}


/***********************************************************************
 *           CLIPPING_SelectRgn
 *
 * Helper function for SelectClipRgn() and SelectVisRgn().
 */
static int CLIPPING_SelectRgn( DC * dc, HRGN * hrgnPrev, HRGN hrgn )
{
    int retval;
    
    if (hrgn)
    {
	if (!*hrgnPrev) *hrgnPrev = CreateRectRgn(0,0,0,0);
	retval = CombineRgn( *hrgnPrev, hrgn, 0, RGN_COPY );
    }
    else
    {
	if (*hrgnPrev) DeleteObject( *hrgnPrev );
	*hrgnPrev = 0;
	retval = SIMPLEREGION; /* Clip region == client area */
    }
    CLIPPING_UpdateGCRegion( dc );
    return retval;
}


/***********************************************************************
 *           SelectClipRgn    (GDI.44)
 */
int SelectClipRgn( HDC hdc, HRGN hrgn )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;
    
#ifdef DEBUG_CLIPPING
    printf( "SelectClipRgn: %d %d\n", hdc, hrgn );
#endif
    return CLIPPING_SelectRgn( dc, &dc->w.hClipRgn, hrgn );
}


/***********************************************************************
 *           SelectVisRgn    (GDI.105)
 */
int SelectVisRgn( HDC hdc, HRGN hrgn )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;
    
#ifdef DEBUG_CLIPPING
    printf( "SelectVisRgn: %d %d\n", hdc, hrgn );
#endif
    return CLIPPING_SelectRgn( dc, &dc->w.hVisRgn, hrgn );
}


/***********************************************************************
 *           OffsetClipRgn    (GDI.32)
 */
int OffsetClipRgn( HDC hdc, short x, short y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "OffsetClipRgn: %d %d,%d\n", hdc, x, y );
#endif

    if (dc->w.hClipRgn)
    {
	int retval = OffsetRgn( dc->w.hClipRgn, x, y );
	CLIPPING_UpdateGCRegion( dc );
	return retval;
    }
    else return SIMPLEREGION; /* Clip region == client area */
}


/***********************************************************************
 *           OffsetVisRgn    (GDI.102)
 */
int OffsetVisRgn( HDC hdc, short x, short y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "OffsetVisRgn: %d %d,%d\n", hdc, x, y );
#endif

    if (dc->w.hVisRgn)
    {
	int retval = OffsetRgn( dc->w.hVisRgn, x, y );
	CLIPPING_UpdateGCRegion( dc );
	return retval;
    }
    else return SIMPLEREGION; /* Clip region == client area */
}


/***********************************************************************
 *           CLIPPING_IntersectRect
 *
 * Helper function for {Intersect,Exclude}{Clip,Vis}Rect
 */
int CLIPPING_IntersectRect( DC * dc, HRGN * hrgn, short left, short top,
			    short right, short bottom, int exclude )
{
    HRGN tempRgn, newRgn;
    RGNOBJ *newObj, *prevObj;
    int retval;

    if (!*hrgn) return NULLREGION;
    if (!(newRgn = CreateRectRgn( 0, 0, 0, 0))) return ERROR;
    if (!(tempRgn = CreateRectRgn( left, top, right, bottom )))
    {
	DeleteObject( newRgn );
	return ERROR;
    }
    retval = CombineRgn( newRgn, *hrgn, tempRgn, exclude ? RGN_DIFF : RGN_AND);
    newObj = (RGNOBJ *) GDI_GetObjPtr( newRgn, REGION_MAGIC );
    prevObj = (RGNOBJ *) GDI_GetObjPtr( *hrgn, REGION_MAGIC );
    if (newObj && prevObj) newObj->header.hNext = prevObj->header.hNext;
    DeleteObject( tempRgn );
    DeleteObject( *hrgn );
    *hrgn = newRgn;    
    CLIPPING_UpdateGCRegion( dc );
    return retval;
}


/***********************************************************************
 *           ExcludeClipRect    (GDI.21)
 */
int ExcludeClipRect( HDC hdc, short left, short top,
		     short right, short bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "ExcludeClipRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
#endif
    return CLIPPING_IntersectRect( dc, &dc->w.hClipRgn, left, top,
				   right, bottom, 1 );
}


/***********************************************************************
 *           IntersectClipRect    (GDI.22)
 */
int IntersectClipRect( HDC hdc, short left, short top,
		       short right, short bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "IntersectClipRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
#endif
    return CLIPPING_IntersectRect( dc, &dc->w.hClipRgn, left, top,
				   right, bottom, 0 );
}


/***********************************************************************
 *           ExcludeVisRect    (GDI.73)
 */
int ExcludeVisRect( HDC hdc, short left, short top,
		    short right, short bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "ExcludeVisRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
#endif
    return CLIPPING_IntersectRect( dc, &dc->w.hVisRgn, left, top,
				   right, bottom, 1 );
}


/***********************************************************************
 *           IntersectVisRect    (GDI.98)
 */
int IntersectVisRect( HDC hdc, short left, short top,
		      short right, short bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "IntersectVisRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
#endif
    return CLIPPING_IntersectRect( dc, &dc->w.hVisRgn, left, top,
				   right, bottom, 0 );
}


/***********************************************************************
 *           PtVisible    (GDI.103)
 */
BOOL PtVisible( HDC hdc, short x, short y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "PtVisible: %d %d,%d\n", hdc, x, y );
#endif
    if (!dc->w.hClipRgn) return FALSE;
    return PtInRegion( dc->w.hClipRgn, x, y );
}


/***********************************************************************
 *           RectVisible    (GDI.104)
 */
BOOL RectVisible( HDC hdc, LPRECT rect )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "RectVisible: %d %p\n", hdc, rect );
#endif
    if (!dc->w.hClipRgn) return FALSE;
    return RectInRegion( dc->w.hClipRgn, rect );
}


/***********************************************************************
 *           GetClipBox    (GDI.77)
 */
int GetClipBox( HDC hdc, LPRECT rect )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "GetClipBox: %d %p\n", hdc, rect );
#endif

    if (dc->w.hGCClipRgn) return GetRgnBox( dc->w.hGCClipRgn, rect );
    else
    {
	Window root;
	int width, height, x, y, border, depth;
	XGetGeometry( XT_display, dc->u.x.drawable, &root, &x, &y, 
		      &width, &height, &border, &depth );
	rect->top = rect->left = 0;
	rect->right  = width & 0xffff;
	rect->bottom = height & 0xffff;
	return SIMPLEREGION;
    }
}


/***********************************************************************
 *           SaveVisRgn    (GDI.129)
 */
HRGN SaveVisRgn( HDC hdc )
{
    HRGN copy;
    RGNOBJ *obj, *copyObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
#ifdef DEBUG_CLIPPING
    printf( "SaveVisRgn: %d\n", hdc );
#endif
    if (!dc->w.hVisRgn) return 0;
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC )))
	return 0;
    if (!(copy = CreateRectRgn( 0, 0, 0, 0 ))) return 0;
    CombineRgn( copy, dc->w.hVisRgn, 0, RGN_COPY );
    if (!(copyObj = (RGNOBJ *) GDI_GetObjPtr( copy, REGION_MAGIC )))
	return 0;
    copyObj->header.hNext = obj->header.hNext;
    obj->header.hNext = copy;
    return copy;
}


/***********************************************************************
 *           RestoreVisRgn    (GDI.130)
 */
int RestoreVisRgn( HDC hdc )
{
    HRGN saved;
    RGNOBJ *obj, *savedObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
#ifdef DEBUG_CLIPPING
    printf( "RestoreVisRgn: %d\n", hdc );
#endif
    if (!dc->w.hVisRgn) return ERROR;
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC )))
	return ERROR;
    if (!(saved = obj->header.hNext)) return ERROR;
    if (!(savedObj = (RGNOBJ *) GDI_GetObjPtr( saved, REGION_MAGIC )))
	return ERROR;
    DeleteObject( dc->w.hVisRgn );
    dc->w.hVisRgn = saved;
    CLIPPING_UpdateGCRegion( dc );
    return savedObj->region.type;
}

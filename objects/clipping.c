/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include "gdi.h"
#include "metafile.h"
#include "stddebug.h"
/* #define DEBUG_CLIPPING /* */
/* #undef  DEBUG_CLIPPING /* */
#include "debug.h"

/***********************************************************************
 *           CLIPPING_SetDeviceClipping
 *
 * Set the clip region of the physical device.
 */
void CLIPPING_SetDeviceClipping( DC * dc )
{
    if (dc->w.hGCClipRgn)
    {
	RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
	if (obj->region.xrgn)
	{
	    XSetRegion( display, dc->u.x.gc, obj->region.xrgn );
	    XSetClipOrigin( display, dc->u.x.gc, dc->w.DCOrgX, dc->w.DCOrgY );
	}
	else if (obj->region.pixmap)
	{
	    XSetClipMask( display, dc->u.x.gc, obj->region.pixmap );
	    XSetClipOrigin( display, dc->u.x.gc,
			    dc->w.DCOrgX + obj->region.box.left,
			    dc->w.DCOrgY + obj->region.box.top );
	}
	else  /* Clip everything */
	{
	    XSetClipRectangles( display, dc->u.x.gc, 0, 0, NULL, 0, 0 );
	}
    }
    else
    {
	XSetClipMask( display, dc->u.x.gc, None );
	XSetClipOrigin( display, dc->u.x.gc, dc->w.DCOrgX, dc->w.DCOrgY );
    }
}


/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn or VisRgn have changed.
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
    CLIPPING_SetDeviceClipping( dc );
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
    
    dprintf_clipping(stddeb, "SelectClipRgn: %d %d\n", hdc, hrgn );
    return CLIPPING_SelectRgn( dc, &dc->w.hClipRgn, hrgn );
}


/***********************************************************************
 *           SelectVisRgn    (GDI.105)
 */
int SelectVisRgn( HDC hdc, HRGN hrgn )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;
    
    dprintf_clipping(stddeb, "SelectVisRgn: %d %d\n", hdc, hrgn );
    return CLIPPING_SelectRgn( dc, &dc->w.hVisRgn, hrgn );
}


/***********************************************************************
 *           OffsetClipRgn    (GDI.32)
 */
int OffsetClipRgn( HDC hdc, short x, short y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return ERROR;
	MF_MetaParam2(dc, META_OFFSETCLIPRGN, x, y);
	return NULLREGION;   /* ?? */
    }

    dprintf_clipping(stddeb, "OffsetClipRgn: %d %d,%d\n", hdc, x, y );

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
    dprintf_clipping(stddeb, "OffsetVisRgn: %d %d,%d\n", hdc, x, y );

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
    HRGN tempRgn = 0, prevRgn = 0, newRgn = 0;
    RGNOBJ *newObj, *prevObj;
    int retval;

    if (!*hrgn)
    {
	if (!(*hrgn = CreateRectRgn( 0, 0, dc->w.DCSizeX, dc->w.DCSizeY )))
	    goto Error;
	prevRgn = *hrgn;
    }
    if (!(newRgn = CreateRectRgn( 0, 0, 0, 0))) goto Error;
    if (!(tempRgn = CreateRectRgn( left, top, right, bottom ))) goto Error;

    retval = CombineRgn( newRgn, *hrgn, tempRgn, exclude ? RGN_DIFF : RGN_AND);
    if (retval == ERROR) goto Error;

    newObj = (RGNOBJ *) GDI_GetObjPtr( newRgn, REGION_MAGIC );
    prevObj = (RGNOBJ *) GDI_GetObjPtr( *hrgn, REGION_MAGIC );
    if (newObj && prevObj) newObj->header.hNext = prevObj->header.hNext;
    DeleteObject( tempRgn );
    if (*hrgn) DeleteObject( *hrgn );
    *hrgn = newRgn;    
    CLIPPING_UpdateGCRegion( dc );
    return retval;

 Error:
    if (tempRgn) DeleteObject( tempRgn );
    if (newRgn) DeleteObject( newRgn );
    if (prevRgn)
    {
	DeleteObject( prevRgn );
	*hrgn = 0;
    }
    return ERROR;
}


/***********************************************************************
 *           ExcludeClipRect    (GDI.21)
 */
int ExcludeClipRect( HDC hdc, short left, short top,
		     short right, short bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return ERROR;
	MF_MetaParam4(dc, META_EXCLUDECLIPRECT, left, top, right, bottom);
	return NULLREGION;   /* ?? */
    }

    dprintf_clipping(stddeb, "ExcludeClipRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
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
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return ERROR;
	MF_MetaParam4(dc, META_INTERSECTCLIPRECT, left, top, right, bottom);
	return NULLREGION;   /* ?? */
    }

    dprintf_clipping(stddeb, "IntersectClipRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
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
    dprintf_clipping(stddeb, "ExcludeVisRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
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
    dprintf_clipping(stddeb, "IntersectVisRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
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

    dprintf_clipping(stddeb, "PtVisible: %d %d,%d\n", hdc, x, y );
    if (!dc->w.hGCClipRgn) return FALSE;
    return PtInRegion( dc->w.hGCClipRgn, XLPTODP(dc,x), YLPTODP(dc,y) );
}


/***********************************************************************
 *           RectVisible    (GDI.104)
 */
BOOL RectVisible( HDC hdc, LPRECT rect )
{
    RECT tmpRect;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    dprintf_clipping(stddeb,"RectVisible: %d %p\n", hdc, rect );
    if (!dc->w.hGCClipRgn) return FALSE;
    tmpRect.left   = XLPTODP(dc, rect->left);
    tmpRect.top    = YLPTODP(dc, rect->top);
    tmpRect.right  = XLPTODP(dc, rect->right);
    tmpRect.bottom = YLPTODP(dc, rect->bottom);
    return RectInRegion( dc->w.hGCClipRgn, &tmpRect );
}


/***********************************************************************
 *           GetClipBox    (GDI.77)
 */
int GetClipBox( HDC hdc, LPRECT rect )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    dprintf_clipping(stddeb, "GetClipBox: %d %p\n", hdc, rect );

    if (dc->w.hGCClipRgn) return GetRgnBox( dc->w.hGCClipRgn, rect );
    else
    {
	rect->top = rect->left = 0;
	rect->right = dc->w.DCSizeX;
	rect->bottom = dc->w.DCSizeY;
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
    dprintf_clipping(stddeb, "SaveVisRgn: %d\n", hdc );
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
    dprintf_clipping(stddeb, "RestoreVisRgn: %d\n", hdc );
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

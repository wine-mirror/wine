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
/* #define DEBUG_CLIPPING */
/* #undef  DEBUG_CLIPPING */
#include "debug.h"

/***********************************************************************
 *           CLIPPING_SetDeviceClipping
 *
 * Set the clip region of the physical device.
 */
static void CLIPPING_SetDeviceClipping( DC * dc )
{
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
    if (!obj)
    {
        fprintf( stderr, "SetDeviceClipping: Rgn is 0. Please report this.\n");
        exit(1);
    }
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


/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn or VisRgn have changed.
 */
void CLIPPING_UpdateGCRegion( DC * dc )
{
    if (!dc->w.hGCClipRgn) dc->w.hGCClipRgn = CreateRectRgn( 0, 0, 0, 0 );

    if (!dc->w.hVisRgn)
    {
        fprintf( stderr, "UpdateGCRegion: hVisRgn is zero. Please report this.\n" );
        exit(1);
    }
    if (!dc->w.hClipRgn)
        CombineRgn( dc->w.hGCClipRgn, dc->w.hVisRgn, 0, RGN_COPY );
    else
        CombineRgn( dc->w.hGCClipRgn, dc->w.hClipRgn, dc->w.hVisRgn, RGN_AND );
    CLIPPING_SetDeviceClipping( dc );
}


/***********************************************************************
 *           SelectClipRgn    (GDI.44)
 */
int SelectClipRgn( HDC hdc, HRGN hrgn )
{
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;

    dprintf_clipping(stddeb, "SelectClipRgn: %d %d\n", hdc, hrgn );

    if (hrgn)
    {
	if (!dc->w.hClipRgn) dc->w.hClipRgn = CreateRectRgn(0,0,0,0);
	retval = CombineRgn( dc->w.hClipRgn, hrgn, 0, RGN_COPY );
    }
    else
    {
	if (dc->w.hClipRgn) DeleteObject( dc->w.hClipRgn );
	dc->w.hClipRgn = 0;
	retval = SIMPLEREGION; /* Clip region == whole DC */
    }
    CLIPPING_UpdateGCRegion( dc );
    return retval;
}


/***********************************************************************
 *           SelectVisRgn    (GDI.105)
 */
int SelectVisRgn( HDC hdc, HRGN hrgn )
{
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc || !hrgn) return ERROR;

    dprintf_clipping(stddeb, "SelectVisRgn: %d %d\n", hdc, hrgn );

    retval = CombineRgn( dc->w.hVisRgn, hrgn, 0, RGN_COPY );
    CLIPPING_UpdateGCRegion( dc );
    return retval;
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
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    dprintf_clipping(stddeb, "OffsetVisRgn: %d %d,%d\n", hdc, x, y );
    retval = OffsetRgn( dc->w.hVisRgn, x, y );
    CLIPPING_UpdateGCRegion( dc );
    return retval;
}


/***********************************************************************
 *           CLIPPING_IntersectClipRect
 *
 * Helper function for {Intersect,Exclude}ClipRect
 */
static int CLIPPING_IntersectClipRect( DC * dc, short left, short top,
                                       short right, short bottom, BOOL exclude)
{
    HRGN tempRgn, newRgn;
    int ret;

    if (!(newRgn = CreateRectRgn( 0, 0, 0, 0 ))) return ERROR;
    if (!(tempRgn = CreateRectRgn( left, top, right, bottom )))
    {
        DeleteObject( newRgn );
        return ERROR;
    }
    ret = CombineRgn( newRgn, dc->w.hClipRgn ? dc->w.hClipRgn : dc->w.hVisRgn,
                      tempRgn, exclude ? RGN_DIFF : RGN_AND);
    DeleteObject( tempRgn );

    if (ret != ERROR)
    {
        if (dc->w.hClipRgn) DeleteObject( dc->w.hClipRgn );
        dc->w.hClipRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
    }
    else DeleteObject( newRgn );
    return ret;
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
    return CLIPPING_IntersectClipRect( dc, left, top, right, bottom, TRUE );
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
    return CLIPPING_IntersectClipRect( dc, left, top, right, bottom, FALSE );
}


/***********************************************************************
 *           CLIPPING_IntersectVisRect
 *
 * Helper function for {Intersect,Exclude}VisRect
 */
static int CLIPPING_IntersectVisRect( DC * dc, short left, short top,
                                      short right, short bottom, BOOL exclude )
{
    HRGN tempRgn, newRgn;
    int ret;

    if (!(newRgn = CreateRectRgn( 0, 0, 0, 0 ))) return ERROR;
    if (!(tempRgn = CreateRectRgn( left, top, right, bottom )))
    {
        DeleteObject( newRgn );
        return ERROR;
    }
    ret = CombineRgn( newRgn, dc->w.hVisRgn, tempRgn,
                      exclude ? RGN_DIFF : RGN_AND);
    DeleteObject( tempRgn );

    if (ret != ERROR)
    {
        RGNOBJ *newObj  = (RGNOBJ*)GDI_GetObjPtr( newRgn, REGION_MAGIC);
        RGNOBJ *prevObj = (RGNOBJ*)GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC);
        if (newObj && prevObj) newObj->header.hNext = prevObj->header.hNext;
        DeleteObject( dc->w.hVisRgn );
        dc->w.hVisRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
    }
    else DeleteObject( newRgn );
    return ret;
}


/***********************************************************************
 *           ExcludeVisRect    (GDI.73)
 */
int ExcludeVisRect( HDC hdc, short left, short top, short right, short bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    dprintf_clipping(stddeb, "ExcludeVisRect: %d %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
    return CLIPPING_IntersectVisRect( dc, left, top, right, bottom, TRUE );
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
    return CLIPPING_IntersectVisRect( dc, left, top, right, bottom, FALSE );
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
    return GetRgnBox( dc->w.hGCClipRgn, rect );
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
    if (!dc->w.hVisRgn)
    {
        fprintf( stderr, "SaveVisRgn: hVisRgn is zero. Please report this.\n" );
        exit(1);
    }
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
    if (!dc || !dc->w.hVisRgn) return ERROR;    
    dprintf_clipping(stddeb, "RestoreVisRgn: %d\n", hdc );
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

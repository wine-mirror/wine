/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdio.h>
#include "region.h"
#include "metafile.h"
#include "stddebug.h"
/* #define DEBUG_CLIPPING */
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
    if (obj->xrgn)
    {
        XSetRegion( display, dc->u.x.gc, obj->xrgn );
        XSetClipOrigin( display, dc->u.x.gc, dc->w.DCOrgX, dc->w.DCOrgY );
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

    dprintf_clipping(stddeb, "SelectClipRgn: "NPFMT" "NPFMT"\n", hdc, hrgn );

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

    dprintf_clipping(stddeb, "SelectVisRgn: "NPFMT" "NPFMT"\n", hdc, hrgn );

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

    dprintf_clipping(stddeb, "OffsetClipRgn: "NPFMT" %d,%d\n", hdc, x, y );

    if (dc->w.hClipRgn)
    {
	int retval = OffsetRgn( dc->w.hClipRgn, XLPTODP(dc,x), YLPTODP(dc,y) );
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
    dprintf_clipping(stddeb, "OffsetVisRgn: "NPFMT" %d,%d\n", hdc, x, y );
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

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

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

    dprintf_clipping(stddeb, "ExcludeClipRect: "NPFMT" %dx%d,%dx%d\n",
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

    dprintf_clipping(stddeb, "IntersectClipRect: "NPFMT" %dx%d,%dx%d\n",
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

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

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
    dprintf_clipping(stddeb, "ExcludeVisRect: "NPFMT" %dx%d,%dx%d\n",
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
    dprintf_clipping(stddeb, "IntersectVisRect: "NPFMT" %dx%d,%dx%d\n",
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

    dprintf_clipping(stddeb, "PtVisible: "NPFMT" %d,%d\n", hdc, x, y );
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
    dprintf_clipping(stddeb,"RectVisible: "NPFMT" %ld,%ldx%ld,%ld\n",
                     hdc, (LONG)rect->left, (LONG)rect->top, (LONG)rect->right,
		     (LONG)rect->bottom );
    if (!dc->w.hGCClipRgn) return FALSE;
    /* copy rectangle to avoid overwriting by LPtoDP */
    tmpRect = *rect;
    LPtoDP( hdc, (LPPOINT)&tmpRect, 2 );
    return RectInRegion( dc->w.hGCClipRgn, &tmpRect );
}


/***********************************************************************
 *           GetClipBox    (GDI.77)
 */
int GetClipBox( HDC hdc, LPRECT rect )
{
    int ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    dprintf_clipping(stddeb, "GetClipBox: "NPFMT" %p\n", hdc, rect );
    ret = GetRgnBox( dc->w.hGCClipRgn, rect );
    DPtoLP( hdc, (LPPOINT)rect, 2 );
    return ret;
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
    dprintf_clipping(stddeb, "SaveVisRgn: "NPFMT"\n", hdc );
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
    dprintf_clipping(stddeb, "RestoreVisRgn: "NPFMT"\n", hdc );
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC )))
	return ERROR;
    if (!(saved = obj->header.hNext)) return ERROR;
    if (!(savedObj = (RGNOBJ *) GDI_GetObjPtr( saved, REGION_MAGIC )))
	return ERROR;
    DeleteObject( dc->w.hVisRgn );
    dc->w.hVisRgn = saved;
    CLIPPING_UpdateGCRegion( dc );
    return savedObj->xrgn ? COMPLEXREGION : NULLREGION;
}

/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdio.h>
#include "dc.h"
#include "metafile.h"
#include "region.h"
#include "stddebug.h"
/* #define DEBUG_CLIPPING */
#include "debug.h"

#define UPDATE_DIRTY_DC(dc) \
 do { \
   if ((dc)->hookProc && !((dc)->w.flags & (DC_SAVED | DC_MEMORY))) \
     (dc)->hookProc( (dc)->hSelf, DCHC_INVALIDVISRGN, (dc)->dwHookData, 0 ); \
 } while(0)

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

    if (dc->w.flags & DC_DIRTY)
    {
        UPDATE_DIRTY_DC(dc);
        dc->w.flags &= ~DC_DIRTY;
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
int SelectClipRgn( HDC hdc, HRGN32 hrgn )
{
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;

    dprintf_clipping(stddeb, "SelectClipRgn: %04x %04x\n", hdc, hrgn );

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
int SelectVisRgn( HDC hdc, HRGN32 hrgn )
{
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc || !hrgn) return ERROR;

    dprintf_clipping(stddeb, "SelectVisRgn: %04x %04x\n", hdc, hrgn );

    dc->w.flags &= ~DC_DIRTY;

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

    dprintf_clipping(stddeb, "OffsetClipRgn: %04x %d,%d\n", hdc, x, y );

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
    dprintf_clipping(stddeb, "OffsetVisRgn: %04x %d,%d\n", hdc, x, y );
    retval = OffsetRgn( dc->w.hVisRgn, x, y );
    CLIPPING_UpdateGCRegion( dc );
    return retval;
}


/***********************************************************************
 *           CLIPPING_IntersectClipRect
 *
 * Helper function for {Intersect,Exclude}ClipRect, can be called from
 * elsewhere (like ExtTextOut()) to skip redundant metafile update and
 * coordinate conversion.
 */
int CLIPPING_IntersectClipRect( DC * dc, short left, short top,
                                         short right, short bottom, UINT16 flags)
{
    HRGN32 newRgn;
    int 	ret;

    if ( !(newRgn = CreateRectRgn( left, top, right, bottom )) ) return ERROR;
    if ( !dc->w.hClipRgn )
    {
       if( flags & CLIP_INTERSECT )
       {
	   dc->w.hClipRgn = newRgn;
	   CLIPPING_UpdateGCRegion( dc );
       }
       return SIMPLEREGION;
    }

    ret = CombineRgn( newRgn, dc->w.hClipRgn, newRgn, 
			     (flags & CLIP_EXCLUDE)? RGN_DIFF : RGN_AND);
    if (ret != ERROR)
    {
        if ( !(flags & CLIP_KEEPRGN) ) DeleteObject( dc->w.hClipRgn );
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

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    dprintf_clipping(stddeb, "ExcludeClipRect: %04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
    return CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_EXCLUDE );
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

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    dprintf_clipping(stddeb, "IntersectClipRect: %04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
    return CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_INTERSECT );
}


/***********************************************************************
 *           CLIPPING_IntersectVisRect
 *
 * Helper function for {Intersect,Exclude}VisRect
 */
static int CLIPPING_IntersectVisRect( DC * dc, short left, short top,
                                      short right, short bottom, BOOL exclude )
{
    HRGN32 tempRgn, newRgn;
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
    dprintf_clipping(stddeb, "ExcludeVisRect: %04x %dx%d,%dx%d\n",
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
    dprintf_clipping(stddeb, "IntersectVisRect: %04x %dx%d,%dx%d\n",
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

    dprintf_clipping(stddeb, "PtVisible: %04x %d,%d\n", hdc, x, y );
    if (!dc->w.hGCClipRgn) return FALSE;

    if( dc->w.flags & DC_DIRTY ) UPDATE_DIRTY_DC(dc);
    dc->w.flags &= ~DC_DIRTY;

    return PtInRegion( dc->w.hGCClipRgn, XLPTODP(dc,x), YLPTODP(dc,y) );
}


/***********************************************************************
 *           RectVisible16    (GDI.104)
 */
BOOL16 RectVisible16( HDC16 hdc, LPRECT16 rect )
{
    RECT16 tmpRect;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    dprintf_clipping(stddeb,"RectVisible: %04x %d,%dx%d,%d\n",
                     hdc, rect->left, rect->top, rect->right, rect->bottom );
    if (!dc->w.hGCClipRgn) return FALSE;
    /* copy rectangle to avoid overwriting by LPtoDP */
    tmpRect = *rect;
    LPtoDP16( hdc, (LPPOINT16)&tmpRect, 2 );
    return RectInRegion16( dc->w.hGCClipRgn, &tmpRect );
}


/***********************************************************************
 *           RectVisible32    (GDI32.282)
 */
BOOL32 RectVisible32( HDC32 hdc, LPRECT32 rect )
{
    RECT16 rect16;
    CONV_RECT32TO16( rect, &rect16 );
    return RectVisible16( (HDC16)hdc, &rect16 );
}


/***********************************************************************
 *           GetClipBox16    (GDI.77)
 */
INT16 GetClipBox16( HDC16 hdc, LPRECT16 rect )
{
    int ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    ret = GetRgnBox16( dc->w.hGCClipRgn, rect );
    DPtoLP16( hdc, (LPPOINT16)rect, 2 );
    return ret;
}


/***********************************************************************
 *           GetClipBox32    (GDI32.162)
 */
INT32 GetClipBox32( HDC32 hdc, LPRECT32 rect )
{
    INT32 ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    ret = GetRgnBox32( dc->w.hGCClipRgn, rect );
    DPtoLP32( hdc, (LPPOINT32)rect, 2 );
    return ret;
}


/***********************************************************************
 *           SaveVisRgn    (GDI.129)
 */
HRGN32 SaveVisRgn( HDC hdc )
{
    HRGN32 copy;
    RGNOBJ *obj, *copyObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    dprintf_clipping(stddeb, "SaveVisRgn: %04x\n", hdc );
    if (!dc->w.hVisRgn)
    {
        fprintf( stderr, "SaveVisRgn: hVisRgn is zero. Please report this.\n" );
        exit(1);
    }
    if( dc->w.flags & DC_DIRTY ) UPDATE_DIRTY_DC(dc);
    dc->w.flags &= ~DC_DIRTY;

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
    HRGN32 saved;
    RGNOBJ *obj, *savedObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc || !dc->w.hVisRgn) return ERROR;    
    dprintf_clipping(stddeb, "RestoreVisRgn: %04x\n", hdc );
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

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
#include "debug.h"

#define UPDATE_DIRTY_DC(dc) \
 do { \
   if ((dc)->hookProc && !((dc)->w.flags & (DC_SAVED | DC_MEMORY))) \
     (dc)->hookProc( (dc)->hSelf, DCHC_INVALIDVISRGN, (dc)->dwHookData, 0 ); \
 } while(0)



/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn or VisRgn have changed.
 */
void CLIPPING_UpdateGCRegion( DC * dc )
{
    if (!dc->w.hGCClipRgn) dc->w.hGCClipRgn = CreateRectRgn32( 0, 0, 0, 0 );

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
        CombineRgn32( dc->w.hGCClipRgn, dc->w.hVisRgn, 0, RGN_COPY );
    else
        CombineRgn32(dc->w.hGCClipRgn, dc->w.hClipRgn, dc->w.hVisRgn, RGN_AND);
    if (dc->funcs->pSetDeviceClipping) dc->funcs->pSetDeviceClipping( dc );
}


/***********************************************************************
 *           SelectClipRgn16    (GDI.44)
 */
INT16 WINAPI SelectClipRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return (INT16)SelectClipRgn32( hdc, hrgn );
}


/***********************************************************************
 *           SelectClipRgn32    (GDI32.297)
 */
INT32 WINAPI SelectClipRgn32( HDC32 hdc, HRGN32 hrgn )
{
    INT32 retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;

    dprintf_clipping(stddeb, "SelectClipRgn: %04x %04x\n", hdc, hrgn );

    if (hrgn)
    {
	if (!dc->w.hClipRgn) dc->w.hClipRgn = CreateRectRgn32(0,0,0,0);
	retval = CombineRgn32( dc->w.hClipRgn, hrgn, 0, RGN_COPY );
    }
    else
    {
	if (dc->w.hClipRgn) DeleteObject16( dc->w.hClipRgn );
	dc->w.hClipRgn = 0;
	retval = SIMPLEREGION; /* Clip region == whole DC */
    }

    CLIPPING_UpdateGCRegion( dc );
    GDI_HEAP_UNLOCK( hdc );
    return retval;
}


/***********************************************************************
 *           SelectVisRgn    (GDI.105)
 */
INT16 WINAPI SelectVisRgn( HDC16 hdc, HRGN16 hrgn )
{
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc || !hrgn) return ERROR;

    dprintf_clipping(stddeb, "SelectVisRgn: %04x %04x\n", hdc, hrgn );

    dc->w.flags &= ~DC_DIRTY;

    retval = CombineRgn16( dc->w.hVisRgn, hrgn, 0, RGN_COPY );
    CLIPPING_UpdateGCRegion( dc );
    GDI_HEAP_UNLOCK( hdc );
    return retval;
}


/***********************************************************************
 *           OffsetClipRgn16    (GDI.32)
 */
INT16 WINAPI OffsetClipRgn16( HDC16 hdc, INT16 x, INT16 y )
{
    return (INT16)OffsetClipRgn32( hdc, x, y );
}


/***********************************************************************
 *           OffsetClipRgn32    (GDI32.255)
 */
INT32 WINAPI OffsetClipRgn32( HDC32 hdc, INT32 x, INT32 y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return ERROR;
	MF_MetaParam2(dc, META_OFFSETCLIPRGN, x, y);
	GDI_HEAP_UNLOCK( hdc );
	return NULLREGION;   /* ?? */
    }

    dprintf_clipping(stddeb, "OffsetClipRgn: %04x %d,%d\n", hdc, x, y );

    if (dc->w.hClipRgn)
    {
	INT32 ret = OffsetRgn32( dc->w.hClipRgn, XLPTODP(dc,x), YLPTODP(dc,y));
	CLIPPING_UpdateGCRegion( dc );
	GDI_HEAP_UNLOCK( hdc );
	return ret;
    }
    GDI_HEAP_UNLOCK( hdc );
    return SIMPLEREGION; /* Clip region == client area */
}


/***********************************************************************
 *           OffsetVisRgn    (GDI.102)
 */
INT16 WINAPI OffsetVisRgn( HDC16 hdc, INT16 x, INT16 y )
{
    INT16 retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    dprintf_clipping(stddeb, "OffsetVisRgn: %04x %d,%d\n", hdc, x, y );
    retval = OffsetRgn32( dc->w.hVisRgn, x, y );
    CLIPPING_UpdateGCRegion( dc );
    GDI_HEAP_UNLOCK( hdc );
    return retval;
}


/***********************************************************************
 *           CLIPPING_IntersectClipRect
 *
 * Helper function for {Intersect,Exclude}ClipRect, can be called from
 * elsewhere (like ExtTextOut()) to skip redundant metafile update and
 * coordinate conversion.
 */
INT32 CLIPPING_IntersectClipRect( DC * dc, INT32 left, INT32 top,
                                  INT32 right, INT32 bottom, UINT32 flags )
{
    HRGN32 newRgn;
    INT32 ret;

    if (!(newRgn = CreateRectRgn32( left, top, right, bottom ))) return ERROR;
    if (!dc->w.hClipRgn)
    {
       if( flags & CLIP_INTERSECT )
       {
	   dc->w.hClipRgn = newRgn;
	   CLIPPING_UpdateGCRegion( dc );
       }
       return SIMPLEREGION;
    }

    ret = CombineRgn32( newRgn, dc->w.hClipRgn, newRgn, 
                        (flags & CLIP_EXCLUDE) ? RGN_DIFF : RGN_AND );
    if (ret != ERROR)
    {
        if (!(flags & CLIP_KEEPRGN)) DeleteObject32( dc->w.hClipRgn );
        dc->w.hClipRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
    }
    else DeleteObject32( newRgn );
    return ret;
}


/***********************************************************************
 *           ExcludeClipRect16    (GDI.21)
 */
INT16 WINAPI ExcludeClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                INT16 right, INT16 bottom )
{
    return (INT16)ExcludeClipRect32( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           ExcludeClipRect32    (GDI32.92)
 */
INT32 WINAPI ExcludeClipRect32( HDC32 hdc, INT32 left, INT32 top,
                                INT32 right, INT32 bottom )
{
    INT32 ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return ERROR;
	MF_MetaParam4(dc, META_EXCLUDECLIPRECT, left, top, right, bottom);
	GDI_HEAP_UNLOCK( hdc );
	return NULLREGION;   /* ?? */
    }

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    dprintf_clipping(stddeb, "ExcludeClipRect: %04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
    ret = CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_EXCLUDE );
    GDI_HEAP_UNLOCK( hdc );
    return ret;
}


/***********************************************************************
 *           IntersectClipRect16    (GDI.22)
 */
INT16 WINAPI IntersectClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                  INT16 right, INT16 bottom )
{
    return (INT16)IntersectClipRect32( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           IntersectClipRect32    (GDI32.245)
 */
INT32 WINAPI IntersectClipRect32( HDC32 hdc, INT32 left, INT32 top,
                                  INT32 right, INT32 bottom )
{
    INT32 ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return ERROR;
	MF_MetaParam4(dc, META_INTERSECTCLIPRECT, left, top, right, bottom);
	GDI_HEAP_UNLOCK( hdc );
	return NULLREGION;   /* ?? */
    }

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    dprintf_clipping(stddeb, "IntersectClipRect: %04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
    ret = CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_INTERSECT );
    GDI_HEAP_UNLOCK( hdc );
    return ret;
}


/***********************************************************************
 *           CLIPPING_IntersectVisRect
 *
 * Helper function for {Intersect,Exclude}VisRect
 */
static INT32 CLIPPING_IntersectVisRect( DC * dc, INT32 left, INT32 top,
                                        INT32 right, INT32 bottom,
                                        BOOL32 exclude )
{
    HRGN32 tempRgn, newRgn;
    INT32 ret;

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    if (!(newRgn = CreateRectRgn32( 0, 0, 0, 0 ))) return ERROR;
    if (!(tempRgn = CreateRectRgn32( left, top, right, bottom )))
    {
        DeleteObject32( newRgn );
        return ERROR;
    }
    ret = CombineRgn32( newRgn, dc->w.hVisRgn, tempRgn,
                        exclude ? RGN_DIFF : RGN_AND );
    DeleteObject32( tempRgn );

    if (ret != ERROR)
    {
        RGNOBJ *newObj  = (RGNOBJ*)GDI_GetObjPtr( newRgn, REGION_MAGIC);
        RGNOBJ *prevObj = (RGNOBJ*)GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC);
        if (newObj && prevObj) newObj->header.hNext = prevObj->header.hNext;
        DeleteObject32( dc->w.hVisRgn );
        dc->w.hVisRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
	GDI_HEAP_UNLOCK( newRgn );
    }
    else DeleteObject32( newRgn );
    return ret;
}


/***********************************************************************
 *           ExcludeVisRect    (GDI.73)
 */
INT16 WINAPI ExcludeVisRect( HDC16 hdc, INT16 left, INT16 top,
                             INT16 right, INT16 bottom )
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
INT16 WINAPI IntersectVisRect( HDC16 hdc, INT16 left, INT16 top,
                               INT16 right, INT16 bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    dprintf_clipping(stddeb, "IntersectVisRect: %04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );

    return CLIPPING_IntersectVisRect( dc, left, top, right, bottom, FALSE );
}


/***********************************************************************
 *           PtVisible16    (GDI.103)
 */
BOOL16 WINAPI PtVisible16( HDC16 hdc, INT16 x, INT16 y )
{
    return PtVisible32( hdc, x, y );
}


/***********************************************************************
 *           PtVisible32    (GDI32.279)
 */
BOOL32 WINAPI PtVisible32( HDC32 hdc, INT32 x, INT32 y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    

    dprintf_clipping(stddeb, "PtVisible: %04x %d,%d\n", hdc, x, y );
    if (!dc->w.hGCClipRgn) return FALSE;

    if( dc->w.flags & DC_DIRTY ) UPDATE_DIRTY_DC(dc);
    dc->w.flags &= ~DC_DIRTY;

    return PtInRegion32( dc->w.hGCClipRgn, XLPTODP(dc,x), YLPTODP(dc,y) );
}


/***********************************************************************
 *           RectVisible16    (GDI.104)
 */
BOOL16 WINAPI RectVisible16( HDC16 hdc, LPRECT16 rect )
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
BOOL32 WINAPI RectVisible32( HDC32 hdc, LPRECT32 rect )
{
    RECT16 rect16;
    CONV_RECT32TO16( rect, &rect16 );
    return RectVisible16( (HDC16)hdc, &rect16 );
}


/***********************************************************************
 *           GetClipBox16    (GDI.77)
 */
INT16 WINAPI GetClipBox16( HDC16 hdc, LPRECT16 rect )
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
INT32 WINAPI GetClipBox32( HDC32 hdc, LPRECT32 rect )
{
    INT32 ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    ret = GetRgnBox32( dc->w.hGCClipRgn, rect );
    DPtoLP32( hdc, (LPPOINT32)rect, 2 );
    return ret;
}


/***********************************************************************
 *           GetClipRgn32  (GDI32.163)
 */
INT32 WINAPI GetClipRgn32( HDC32 hdc, HRGN32 hRgn )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if( dc && hRgn )
      if( dc->w.hClipRgn )
      { 
	/* this assumes that dc->w.hClipRgn is in coordinates
	   relative to the DC origin (not device) */

	if( CombineRgn32(hRgn, dc->w.hClipRgn, 0, RGN_COPY) != ERROR )
	    return 1;
      }
      else return 0;
    return -1;
}

/***********************************************************************
 *           SaveVisRgn    (GDI.129)
 */
HRGN16 WINAPI SaveVisRgn( HDC16 hdc )
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
    {
        GDI_HEAP_UNLOCK( hdc );
	return 0;
    }
    if (!(copy = CreateRectRgn32( 0, 0, 0, 0 )))
    {
        GDI_HEAP_UNLOCK( dc->w.hVisRgn );
        GDI_HEAP_UNLOCK( hdc );
        return 0;
    }  
    CombineRgn32( copy, dc->w.hVisRgn, 0, RGN_COPY );
    if (!(copyObj = (RGNOBJ *) GDI_GetObjPtr( copy, REGION_MAGIC )))
    {
        GDI_HEAP_UNLOCK( dc->w.hVisRgn );
        GDI_HEAP_UNLOCK( hdc );
	return 0;
    }
    copyObj->header.hNext = obj->header.hNext;
    obj->header.hNext = copy;
    GDI_HEAP_UNLOCK( dc->w.hVisRgn );
    GDI_HEAP_UNLOCK( hdc );
    GDI_HEAP_UNLOCK( copy );
    return copy;
}


/***********************************************************************
 *           RestoreVisRgn    (GDI.130)
 */
INT16 WINAPI RestoreVisRgn( HDC16 hdc )
{
    HRGN32 saved;
    RGNOBJ *obj, *savedObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    if (!dc->w.hVisRgn)
    {
        GDI_HEAP_UNLOCK( hdc );
        return ERROR;    
    }
    dprintf_clipping(stddeb, "RestoreVisRgn: %04x\n", hdc );
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hdc );
	return ERROR;
    }
    if (!(saved = obj->header.hNext)) 
    {
        GDI_HEAP_UNLOCK( dc->w.hVisRgn );
        GDI_HEAP_UNLOCK( hdc );
        return ERROR;
    }
    if (!(savedObj = (RGNOBJ *) GDI_GetObjPtr( saved, REGION_MAGIC )))
    {
        GDI_HEAP_UNLOCK( dc->w.hVisRgn );
        GDI_HEAP_UNLOCK( hdc );
        return ERROR;
    }
    DeleteObject32( dc->w.hVisRgn );
    dc->w.hVisRgn = saved;
    CLIPPING_UpdateGCRegion( dc );
    GDI_HEAP_UNLOCK( hdc );
    GDI_HEAP_UNLOCK( saved );
    return savedObj->xrgn ? COMPLEXREGION : NULLREGION;
}

/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include "dc.h"
#include "metafile.h"
#include "region.h"
#include "debug.h"
#include "wine/winuser16.h"

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
    if (!dc->w.hGCClipRgn) dc->w.hGCClipRgn = CreateRectRgn( 0, 0, 0, 0 );

    if (!dc->w.hVisRgn)
    {
        ERR(region, "hVisRgn is zero. Please report this.\n" );
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
        CombineRgn(dc->w.hGCClipRgn, dc->w.hClipRgn, dc->w.hVisRgn, RGN_AND);
    if (dc->funcs->pSetDeviceClipping) dc->funcs->pSetDeviceClipping( dc );
}


/***********************************************************************
 *           SelectClipRgn16    (GDI.44)
 */
INT16 WINAPI SelectClipRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return (INT16)SelectClipRgn( hdc, hrgn );
}


/***********************************************************************
 *           SelectClipRgn32    (GDI32.297)
 */
INT WINAPI SelectClipRgn( HDC hdc, HRGN hrgn )
{
    return ExtSelectClipRgn( hdc, hrgn, RGN_COPY );
}

/******************************************************************************
 *		ExtSelectClipRgn	[GDI32.97]
 */
INT WINAPI ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT fnMode )
{
    INT retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;

    TRACE( clipping, "%04x %04x %d\n", hdc, hrgn, fnMode );

    if (!hrgn)
    {
        if (fnMode == RGN_COPY)
        {
            if (dc->w.hClipRgn) DeleteObject16( dc->w.hClipRgn );
            dc->w.hClipRgn = 0;
            retval = SIMPLEREGION; /* Clip region == whole DC */
        }
        else
        {
            FIXME(clipping, "Unimplemented: hrgn NULL in mode: %d\n", fnMode); 
            return ERROR;
        }
    }
    else 
    {
        if (!dc->w.hClipRgn)
        {
            RECT rect;
            GetRgnBox( dc->w.hVisRgn, &rect );
            dc->w.hClipRgn = CreateRectRgnIndirect( &rect );
        }

        OffsetRgn( dc->w.hClipRgn, -dc->w.DCOrgX, -dc->w.DCOrgY );
        if(fnMode == RGN_COPY)
            retval = CombineRgn( dc->w.hClipRgn, hrgn, 0, fnMode );
        else
            retval = CombineRgn( dc->w.hClipRgn, dc->w.hClipRgn, hrgn, fnMode);
        OffsetRgn( dc->w.hClipRgn, dc->w.DCOrgX, dc->w.DCOrgY );
    }


    CLIPPING_UpdateGCRegion( dc );
    GDI_HEAP_UNLOCK( hdc );
    return retval;
}

/***********************************************************************
 *           SelectVisRgn    (GDI.105)
 */
INT16 WINAPI SelectVisRgn16( HDC16 hdc, HRGN16 hrgn )
{
    int retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc || !hrgn) return ERROR;

    TRACE(clipping, "%04x %04x\n", hdc, hrgn );

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
    return (INT16)OffsetClipRgn( hdc, x, y );
}


/***********************************************************************
 *           OffsetClipRgn32    (GDI32.255)
 */
INT WINAPI OffsetClipRgn( HDC hdc, INT x, INT y )
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

    TRACE(clipping, "%04x %d,%d\n", hdc, x, y );

    if (dc->w.hClipRgn)
    {
	INT ret = OffsetRgn( dc->w.hClipRgn, XLPTODP(dc,x), YLPTODP(dc,y));
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
INT16 WINAPI OffsetVisRgn16( HDC16 hdc, INT16 x, INT16 y )
{
    INT16 retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    TRACE(clipping, "%04x %d,%d\n", hdc, x, y );
    retval = OffsetRgn( dc->w.hVisRgn, x, y );
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
INT CLIPPING_IntersectClipRect( DC * dc, INT left, INT top,
                                  INT right, INT bottom, UINT flags )
{
    HRGN newRgn;
    INT ret;

    left   += dc->w.DCOrgX;
    right  += dc->w.DCOrgX;
    top    += dc->w.DCOrgY;
    bottom += dc->w.DCOrgY;

    if (!(newRgn = CreateRectRgn( left, top, right, bottom ))) return ERROR;
    if (!dc->w.hClipRgn)
    {
       if( flags & CLIP_INTERSECT )
       {
	   dc->w.hClipRgn = newRgn;
	   CLIPPING_UpdateGCRegion( dc );
           return SIMPLEREGION;
       }
       else if( flags & CLIP_EXCLUDE )
       {
           dc->w.hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
     	   CombineRgn( dc->w.hClipRgn, dc->w.hVisRgn, 0, RGN_COPY );
       }
       else WARN(clipping,"No hClipRgn and flags are %x\n",flags);
    }

    ret = CombineRgn( newRgn, dc->w.hClipRgn, newRgn, 
                        (flags & CLIP_EXCLUDE) ? RGN_DIFF : RGN_AND );
    if (ret != ERROR)
    {
        if (!(flags & CLIP_KEEPRGN)) DeleteObject( dc->w.hClipRgn );
        dc->w.hClipRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
    }
    else DeleteObject( newRgn );
    return ret;
}


/***********************************************************************
 *           ExcludeClipRect16    (GDI.21)
 */
INT16 WINAPI ExcludeClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                INT16 right, INT16 bottom )
{
    return (INT16)ExcludeClipRect( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           ExcludeClipRect32    (GDI32.92)
 */
INT WINAPI ExcludeClipRect( HDC hdc, INT left, INT top,
                                INT right, INT bottom )
{
    INT ret;
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

    TRACE(clipping, "%04x %dx%d,%dx%d\n",
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
    return (INT16)IntersectClipRect( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           IntersectClipRect32    (GDI32.245)
 */
INT WINAPI IntersectClipRect( HDC hdc, INT left, INT top,
                                  INT right, INT bottom )
{
    INT ret;
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

    TRACE(clipping, "%04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );
    ret = CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_INTERSECT );
    GDI_HEAP_UNLOCK( hdc );
    return ret;
}


/***********************************************************************
 *           CLIPPING_IntersectVisRect
 *
 * Helper function for {Intersect,Exclude}VisRect, can be called from
 * elsewhere (like ExtTextOut()) to skip redundant metafile update and
 * coordinate conversion.
 */
INT CLIPPING_IntersectVisRect( DC * dc, INT left, INT top,
                                 INT right, INT bottom,
                                 BOOL exclude )
{
    HRGN tempRgn, newRgn;
    INT ret;

    left   += dc->w.DCOrgX;
    right  += dc->w.DCOrgX;
    top    += dc->w.DCOrgY;
    bottom += dc->w.DCOrgY;

    if (!(newRgn = CreateRectRgn( 0, 0, 0, 0 ))) return ERROR;
    if (!(tempRgn = CreateRectRgn( left, top, right, bottom )))
    {
        DeleteObject( newRgn );
        return ERROR;
    }
    ret = CombineRgn( newRgn, dc->w.hVisRgn, tempRgn,
                        exclude ? RGN_DIFF : RGN_AND );
    DeleteObject( tempRgn );

    if (ret != ERROR)
    {
        RGNOBJ *newObj  = (RGNOBJ*)GDI_GetObjPtr( newRgn, REGION_MAGIC);
        RGNOBJ *prevObj = (RGNOBJ*)GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC);
        if (newObj && prevObj) newObj->header.hNext = prevObj->header.hNext;
        DeleteObject( dc->w.hVisRgn );
        dc->w.hVisRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
	GDI_HEAP_UNLOCK( newRgn );
    }
    else DeleteObject( newRgn );
    return ret;
}


/***********************************************************************
 *           ExcludeVisRect    (GDI.73)
 */
INT16 WINAPI ExcludeVisRect16( HDC16 hdc, INT16 left, INT16 top,
                             INT16 right, INT16 bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    TRACE(clipping, "%04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );

    return CLIPPING_IntersectVisRect( dc, left, top, right, bottom, TRUE );
}


/***********************************************************************
 *           IntersectVisRect    (GDI.98)
 */
INT16 WINAPI IntersectVisRect16( HDC16 hdc, INT16 left, INT16 top,
                               INT16 right, INT16 bottom )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    TRACE(clipping, "%04x %dx%d,%dx%d\n",
	    hdc, left, top, right, bottom );

    return CLIPPING_IntersectVisRect( dc, left, top, right, bottom, FALSE );
}


/***********************************************************************
 *           PtVisible16    (GDI.103)
 */
BOOL16 WINAPI PtVisible16( HDC16 hdc, INT16 x, INT16 y )
{
    return PtVisible( hdc, x, y );
}


/***********************************************************************
 *           PtVisible32    (GDI32.279)
 */
BOOL WINAPI PtVisible( HDC hdc, INT x, INT y )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    

    TRACE(clipping, "%04x %d,%d\n", hdc, x, y );
    if (!dc->w.hGCClipRgn) return FALSE;

    if( dc->w.flags & DC_DIRTY ) UPDATE_DIRTY_DC(dc);
    dc->w.flags &= ~DC_DIRTY;

    return PtInRegion( dc->w.hGCClipRgn, XLPTODP(dc,x) + dc->w.DCOrgX, 
                                           YLPTODP(dc,y) + dc->w.DCOrgY );
}


/***********************************************************************
 *           RectVisible16    (GDI.104)
 */
BOOL16 WINAPI RectVisible16( HDC16 hdc, const RECT16* rect )
{
    RECT16 tmpRect;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    TRACE(clipping,"%04x %d,%dx%d,%d\n",
                     hdc, rect->left, rect->top, rect->right, rect->bottom );
    if (!dc->w.hGCClipRgn) return FALSE;
    /* copy rectangle to avoid overwriting by LPtoDP */
    tmpRect = *rect;
    LPtoDP16( hdc, (LPPOINT16)&tmpRect, 2 );
    OffsetRect16( &tmpRect, dc->w.DCOrgX, dc->w.DCOrgY );
    return RectInRegion16( dc->w.hGCClipRgn, &tmpRect );
}


/***********************************************************************
 *           RectVisible32    (GDI32.282)
 */
BOOL WINAPI RectVisible( HDC hdc, const RECT* rect )
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
    OffsetRect16( rect, -dc->w.DCOrgX, -dc->w.DCOrgY );
    DPtoLP16( hdc, (LPPOINT16)rect, 2 );
    TRACE(clipping, "%d,%d-%d,%d\n", 
            rect->left,rect->top,rect->right,rect->bottom );
    return ret;
}


/***********************************************************************
 *           GetClipBox32    (GDI32.162)
 */
INT WINAPI GetClipBox( HDC hdc, LPRECT rect )
{
    INT ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return ERROR;    
    ret = GetRgnBox( dc->w.hGCClipRgn, rect );
    OffsetRect( rect, -dc->w.DCOrgX, -dc->w.DCOrgY );
    DPtoLP( hdc, (LPPOINT)rect, 2 );
    return ret;
}


/***********************************************************************
 *           GetClipRgn32  (GDI32.163)
 */
INT WINAPI GetClipRgn( HDC hdc, HRGN hRgn )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if( dc && hRgn )
    {
      if( dc->w.hClipRgn )
      { 
	/* this assumes that dc->w.hClipRgn is in coordinates
	   relative to the device (not DC origin) */

	if( CombineRgn(hRgn, dc->w.hClipRgn, 0, RGN_COPY) != ERROR )
        {
            OffsetRgn( hRgn, -dc->w.DCOrgX, -dc->w.DCOrgY );
	    return 1;
        }
      }
      else return 0;
    }
    return -1;
}

/***********************************************************************
 *           SaveVisRgn    (GDI.129)
 */
HRGN16 WINAPI SaveVisRgn16( HDC16 hdc )
{
    HRGN copy;
    RGNOBJ *obj, *copyObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    TRACE(clipping, "%04x\n", hdc );
    if (!dc->w.hVisRgn)
    {
        ERR(region, "hVisRgn is zero. Please report this.\n" );
        exit(1);
    }
    if( dc->w.flags & DC_DIRTY ) UPDATE_DIRTY_DC(dc);
    dc->w.flags &= ~DC_DIRTY;

    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->w.hVisRgn, REGION_MAGIC )))
    {
        GDI_HEAP_UNLOCK( hdc );
	return 0;
    }
    if (!(copy = CreateRectRgn( 0, 0, 0, 0 )))
    {
        GDI_HEAP_UNLOCK( dc->w.hVisRgn );
        GDI_HEAP_UNLOCK( hdc );
        return 0;
    }  
    CombineRgn( copy, dc->w.hVisRgn, 0, RGN_COPY );
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
INT16 WINAPI RestoreVisRgn16( HDC16 hdc )
{
    HRGN saved;
    RGNOBJ *obj, *savedObj;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    INT16 ret;

    if (!dc) return ERROR;    
    if (!dc->w.hVisRgn)
    {
        GDI_HEAP_UNLOCK( hdc );
        return ERROR;    
    }
    TRACE(clipping, "%04x\n", hdc );
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
    DeleteObject( dc->w.hVisRgn );
    dc->w.hVisRgn = saved;
    CLIPPING_UpdateGCRegion( dc );
    GDI_HEAP_UNLOCK( hdc );
    ret = savedObj->rgn->type; /* FIXME */
    GDI_HEAP_UNLOCK( saved );
    return ret;
}

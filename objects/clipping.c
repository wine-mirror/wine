/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "gdi.h"
#include "region.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(clipping);


/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn or VisRgn have changed.
 */
void CLIPPING_UpdateGCRegion( DC * dc )
{
    if (!dc->hGCClipRgn) dc->hGCClipRgn = CreateRectRgn( 0, 0, 0, 0 );

    if (!dc->hVisRgn)
    {
        ERR("hVisRgn is zero. Please report this.\n" );
        exit(1);
    }

    if (dc->flags & DC_DIRTY) ERR( "DC is dirty. Please report this.\n" );

    if (!dc->hClipRgn)
        CombineRgn( dc->hGCClipRgn, dc->hVisRgn, 0, RGN_COPY );
    else
        CombineRgn(dc->hGCClipRgn, dc->hClipRgn, dc->hVisRgn, RGN_AND);
    if (dc->funcs->pSetDeviceClipping) dc->funcs->pSetDeviceClipping( dc );
}


/***********************************************************************
 *           SelectClipRgn    (GDI.44)
 */
INT16 WINAPI SelectClipRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return (INT16)SelectClipRgn( hdc, hrgn );
}


/***********************************************************************
 *           SelectClipRgn    (GDI32.@)
 */
INT WINAPI SelectClipRgn( HDC hdc, HRGN hrgn )
{
    return ExtSelectClipRgn( hdc, hrgn, RGN_COPY );
}

/******************************************************************************
 *		ExtSelectClipRgn	[GDI.508]
 */
INT16 WINAPI ExtSelectClipRgn16( HDC16 hdc, HRGN16 hrgn, INT16 fnMode )
{
  return (INT16) ExtSelectClipRgn((HDC) hdc, (HRGN) hrgn, fnMode);
}

/******************************************************************************
 *		ExtSelectClipRgn	[GDI32.@]
 */
INT WINAPI ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT fnMode )
{
    INT retval;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%04x %04x %d\n", hdc, hrgn, fnMode );

    if (!hrgn)
    {
        if (fnMode == RGN_COPY)
        {
            if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
            dc->hClipRgn = 0;
            retval = SIMPLEREGION; /* Clip region == whole DC */
        }
        else
        {
            FIXME("Unimplemented: hrgn NULL in mode: %d\n", fnMode); 
            GDI_ReleaseObj( hdc );
            return ERROR;
        }
    }
    else 
    {
        if (!dc->hClipRgn)
        {
            RECT rect;
            GetRgnBox( dc->hVisRgn, &rect );
            dc->hClipRgn = CreateRectRgnIndirect( &rect );
        }

        OffsetRgn( dc->hClipRgn, -dc->DCOrgX, -dc->DCOrgY );
        if(fnMode == RGN_COPY)
            retval = CombineRgn( dc->hClipRgn, hrgn, 0, fnMode );
        else
            retval = CombineRgn( dc->hClipRgn, dc->hClipRgn, hrgn, fnMode);
        OffsetRgn( dc->hClipRgn, dc->DCOrgX, dc->DCOrgY );
    }

    CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
    return retval;
}

/***********************************************************************
 *           SelectVisRgn    (GDI.105)
 */
INT16 WINAPI SelectVisRgn16( HDC16 hdc, HRGN16 hrgn )
{
    int retval;
    DC * dc;

    if (!hrgn) return ERROR;
    if (!(dc = DC_GetDCPtr( hdc ))) return ERROR;

    TRACE("%04x %04x\n", hdc, hrgn );

    dc->flags &= ~DC_DIRTY;

    retval = CombineRgn16( dc->hVisRgn, hrgn, 0, RGN_COPY );
    CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
    return retval;
}


/***********************************************************************
 *           OffsetClipRgn    (GDI.32)
 */
INT16 WINAPI OffsetClipRgn16( HDC16 hdc, INT16 x, INT16 y )
{
    return (INT16)OffsetClipRgn( hdc, x, y );
}


/***********************************************************************
 *           OffsetClipRgn    (GDI32.@)
 */
INT WINAPI OffsetClipRgn( HDC hdc, INT x, INT y )
{
    INT ret = SIMPLEREGION;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%04x %d,%d\n", hdc, x, y );

    if(dc->funcs->pOffsetClipRgn)
        ret = dc->funcs->pOffsetClipRgn( dc, x, y );
    else if (dc->hClipRgn) {
        ret = OffsetRgn( dc->hClipRgn, XLSTODS(dc,x), YLSTODS(dc,y));
	CLIPPING_UpdateGCRegion( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           OffsetVisRgn    (GDI.102)
 */
INT16 WINAPI OffsetVisRgn16( HDC16 hdc, INT16 x, INT16 y )
{
    INT16 retval;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;    
    TRACE("%04x %d,%d\n", hdc, x, y );
    retval = OffsetRgn( dc->hVisRgn, x, y );
    CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
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

    left   += dc->DCOrgX;
    right  += dc->DCOrgX;
    top    += dc->DCOrgY;
    bottom += dc->DCOrgY;

    if (!(newRgn = CreateRectRgn( left, top, right, bottom ))) return ERROR;
    if (!dc->hClipRgn)
    {
       if( flags & CLIP_INTERSECT )
       {
	   dc->hClipRgn = newRgn;
	   CLIPPING_UpdateGCRegion( dc );
           return SIMPLEREGION;
       }
       else if( flags & CLIP_EXCLUDE )
       {
           dc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
     	   CombineRgn( dc->hClipRgn, dc->hVisRgn, 0, RGN_COPY );
       }
       else WARN("No hClipRgn and flags are %x\n",flags);
    }

    ret = CombineRgn( newRgn, dc->hClipRgn, newRgn, 
                        (flags & CLIP_EXCLUDE) ? RGN_DIFF : RGN_AND );
    if (ret != ERROR)
    {
        if (!(flags & CLIP_KEEPRGN)) DeleteObject( dc->hClipRgn );
        dc->hClipRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
    }
    else DeleteObject( newRgn );
    return ret;
}


/***********************************************************************
 *           ExcludeClipRect    (GDI.21)
 */
INT16 WINAPI ExcludeClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                INT16 right, INT16 bottom )
{
    return (INT16)ExcludeClipRect( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           ExcludeClipRect    (GDI32.@)
 */
INT WINAPI ExcludeClipRect( HDC hdc, INT left, INT top,
                                INT right, INT bottom )
{
    INT ret;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%04x %dx%d,%dx%d\n", hdc, left, top, right, bottom );

    if(dc->funcs->pExcludeClipRect)
        ret = dc->funcs->pExcludeClipRect( dc, left, top, right, bottom );
    else {
        left   = XLPTODP( dc, left );
	right  = XLPTODP( dc, right );
	top    = YLPTODP( dc, top );
	bottom = YLPTODP( dc, bottom );

	ret = CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_EXCLUDE );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           IntersectClipRect    (GDI.22)
 */
INT16 WINAPI IntersectClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                  INT16 right, INT16 bottom )
{
    return (INT16)IntersectClipRect( hdc, left, top, right, bottom );
}


/***********************************************************************
 *           IntersectClipRect    (GDI32.@)
 */
INT WINAPI IntersectClipRect( HDC hdc, INT left, INT top,
                                  INT right, INT bottom )
{
    INT ret;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%04x %dx%d,%dx%d\n", hdc, left, top, right, bottom );

    if(dc->funcs->pIntersectClipRect)
        ret = dc->funcs->pIntersectClipRect( dc, left, top, right, bottom );
    else {
        left   = XLPTODP( dc, left );
	right  = XLPTODP( dc, right );
	top    = YLPTODP( dc, top );
	bottom = YLPTODP( dc, bottom );

	ret = CLIPPING_IntersectClipRect( dc, left, top, right, bottom, CLIP_INTERSECT );
    }
    GDI_ReleaseObj( hdc );
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

    left   += dc->DCOrgX;
    right  += dc->DCOrgX;
    top    += dc->DCOrgY;
    bottom += dc->DCOrgY;

    if (!(newRgn = CreateRectRgn( 0, 0, 0, 0 ))) return ERROR;
    if (!(tempRgn = CreateRectRgn( left, top, right, bottom )))
    {
        DeleteObject( newRgn );
        return ERROR;
    }
    ret = CombineRgn( newRgn, dc->hVisRgn, tempRgn,
                        exclude ? RGN_DIFF : RGN_AND );
    DeleteObject( tempRgn );

    if (ret != ERROR)
    {
        RGNOBJ *newObj  = (RGNOBJ*)GDI_GetObjPtr( newRgn, REGION_MAGIC);
        if (newObj)
        {
            RGNOBJ *prevObj = (RGNOBJ*)GDI_GetObjPtr( dc->hVisRgn, REGION_MAGIC);
            if (prevObj)
            {
                newObj->header.hNext = prevObj->header.hNext;
                GDI_ReleaseObj( dc->hVisRgn );
            }
            GDI_ReleaseObj( newRgn );
        }
        DeleteObject( dc->hVisRgn );
        dc->hVisRgn = newRgn;    
        CLIPPING_UpdateGCRegion( dc );
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
    INT16 ret;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;    

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    TRACE("%04x %dx%d,%dx%d\n", hdc, left, top, right, bottom );

    ret = CLIPPING_IntersectVisRect( dc, left, top, right, bottom, TRUE );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           IntersectVisRect    (GDI.98)
 */
INT16 WINAPI IntersectVisRect16( HDC16 hdc, INT16 left, INT16 top,
                               INT16 right, INT16 bottom )
{
    INT16 ret;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;    

    left   = XLPTODP( dc, left );
    right  = XLPTODP( dc, right );
    top    = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );

    TRACE("%04x %dx%d,%dx%d\n", hdc, left, top, right, bottom );

    ret = CLIPPING_IntersectVisRect( dc, left, top, right, bottom, FALSE );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           PtVisible    (GDI.103)
 */
BOOL16 WINAPI PtVisible16( HDC16 hdc, INT16 x, INT16 y )
{
    return PtVisible( hdc, x, y );
}


/***********************************************************************
 *           PtVisible    (GDI32.@)
 */
BOOL WINAPI PtVisible( HDC hdc, INT x, INT y )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCUpdate( hdc );

    TRACE("%04x %d,%d\n", hdc, x, y );
    if (!dc) return FALSE;
    if (dc->hGCClipRgn)
    {
        ret = PtInRegion( dc->hGCClipRgn, XLPTODP(dc,x) + dc->DCOrgX, 
                                           YLPTODP(dc,y) + dc->DCOrgY );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           RectVisible    (GDI.465)
 *           RectVisibleOld (GDI.104)
 */
BOOL16 WINAPI RectVisible16( HDC16 hdc, const RECT16* rect16 )
{
    RECT rect;
    CONV_RECT16TO32( rect16, &rect );
    return RectVisible( hdc, &rect );
}


/***********************************************************************
 *           RectVisible    (GDI32.@)
 */
BOOL WINAPI RectVisible( HDC hdc, const RECT* rect )
{
    BOOL ret = FALSE;
    RECT tmpRect;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;
    TRACE("%04x %d,%dx%d,%d\n",
          hdc, rect->left, rect->top, rect->right, rect->bottom );
    if (dc->hGCClipRgn)
    {
        /* copy rectangle to avoid overwriting by LPtoDP */
        tmpRect = *rect;
        LPtoDP( hdc, (LPPOINT)&tmpRect, 2 );
        tmpRect.left   += dc->DCOrgX;
        tmpRect.right  += dc->DCOrgX;
        tmpRect.top    += dc->DCOrgY;
        tmpRect.bottom += dc->DCOrgY;
        ret = RectInRegion( dc->hGCClipRgn, &tmpRect );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GetClipBox    (GDI.77)
 */
INT16 WINAPI GetClipBox16( HDC16 hdc, LPRECT16 rect )
{
    int ret;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;
    ret = GetRgnBox16( dc->hGCClipRgn, rect );
    rect->left   -= dc->DCOrgX;
    rect->right  -= dc->DCOrgX;
    rect->top    -= dc->DCOrgY;
    rect->bottom -= dc->DCOrgY;
    DPtoLP16( hdc, (LPPOINT16)rect, 2 );
    TRACE("%d,%d-%d,%d\n", rect->left,rect->top,rect->right,rect->bottom );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GetClipBox    (GDI32.@)
 */
INT WINAPI GetClipBox( HDC hdc, LPRECT rect )
{
    INT ret;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;
    ret = GetRgnBox( dc->hGCClipRgn, rect );
    rect->left   -= dc->DCOrgX;
    rect->right  -= dc->DCOrgX;
    rect->top    -= dc->DCOrgY;
    rect->bottom -= dc->DCOrgY;
    DPtoLP( hdc, (LPPOINT)rect, 2 );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GetClipRgn  (GDI32.@)
 */
INT WINAPI GetClipRgn( HDC hdc, HRGN hRgn )
{
    INT ret = -1;
    DC * dc;
    if (hRgn && (dc = DC_GetDCPtr( hdc )))
    {
      if( dc->hClipRgn )
      { 
	/* this assumes that dc->hClipRgn is in coordinates
	   relative to the device (not DC origin) */

	if( CombineRgn(hRgn, dc->hClipRgn, 0, RGN_COPY) != ERROR )
        {
            OffsetRgn( hRgn, -dc->DCOrgX, -dc->DCOrgY );
            ret = 1;
        }
      }
      else ret = 0;
      GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *           SaveVisRgn    (GDI.129)
 */
HRGN16 WINAPI SaveVisRgn16( HDC16 hdc )
{
    HRGN copy;
    RGNOBJ *obj, *copyObj;
    DC *dc = DC_GetDCUpdate( hdc );

    if (!dc) return 0;
    TRACE("%04x\n", hdc );

    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->hVisRgn, REGION_MAGIC )))
    {
        GDI_ReleaseObj( hdc );
	return 0;
    }
    if (!(copy = CreateRectRgn( 0, 0, 0, 0 )))
    {
        GDI_ReleaseObj( dc->hVisRgn );
        GDI_ReleaseObj( hdc );
        return 0;
    }  
    CombineRgn( copy, dc->hVisRgn, 0, RGN_COPY );
    if (!(copyObj = (RGNOBJ *) GDI_GetObjPtr( copy, REGION_MAGIC )))
    {
        DeleteObject( copy );
        GDI_ReleaseObj( dc->hVisRgn );
        GDI_ReleaseObj( hdc );
	return 0;
    }
    copyObj->header.hNext = obj->header.hNext;
    obj->header.hNext = copy;
    GDI_ReleaseObj( copy );
    GDI_ReleaseObj( dc->hVisRgn );
    GDI_ReleaseObj( hdc );
    return copy;
}


/***********************************************************************
 *           RestoreVisRgn    (GDI.130)
 */
INT16 WINAPI RestoreVisRgn16( HDC16 hdc )
{
    HRGN saved;
    RGNOBJ *obj, *savedObj;
    DC *dc = DC_GetDCPtr( hdc );
    INT16 ret = ERROR;

    if (!dc) return ERROR;    
    if (!dc->hVisRgn) goto done;
    TRACE("%04x\n", hdc );
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( dc->hVisRgn, REGION_MAGIC ))) goto done;

    saved = obj->header.hNext;
    GDI_ReleaseObj( dc->hVisRgn );
    if (!saved || !(savedObj = (RGNOBJ *) GDI_GetObjPtr( saved, REGION_MAGIC ))) goto done;

    DeleteObject( dc->hVisRgn );
    dc->hVisRgn = saved;
    dc->flags &= ~DC_DIRTY;
    CLIPPING_UpdateGCRegion( dc );
    ret = savedObj->rgn->type; /* FIXME */
    GDI_ReleaseObj( saved );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}

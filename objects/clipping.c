/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "gdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipping);


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
    if (dc->funcs->pSetDeviceClipping)
        dc->funcs->pSetDeviceClipping( dc->physDev, dc->hGCClipRgn );
}


/***********************************************************************
 *           SelectClipRgn    (GDI32.@)
 */
INT WINAPI SelectClipRgn( HDC hdc, HRGN hrgn )
{
    return ExtSelectClipRgn( hdc, hrgn, RGN_COPY );
}


/******************************************************************************
 *		ExtSelectClipRgn	[GDI32.@]
 */
INT WINAPI ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT fnMode )
{
    INT retval;
    RECT rect;
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%p %p %d\n", hdc, hrgn, fnMode );

    if (dc->funcs->pExtSelectClipRgn)
    {
        retval = dc->funcs->pExtSelectClipRgn( dc->physDev, hrgn, fnMode );
        GDI_ReleaseObj( hdc );
        return retval;
    }

    if (!hrgn)
    {
        if (fnMode == RGN_COPY)
        {
            if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
            dc->hClipRgn = 0;
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

        if(fnMode == RGN_COPY)
            CombineRgn( dc->hClipRgn, hrgn, 0, fnMode );
        else
            CombineRgn( dc->hClipRgn, dc->hClipRgn, hrgn, fnMode);
    }

    CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );

    return GetClipBox(hdc, &rect);
}

/***********************************************************************
 *           SelectVisRgn   (GDI.105)
 */
INT16 WINAPI SelectVisRgn16( HDC16 hdc16, HRGN16 hrgn )
{
    int retval;
    HDC hdc = HDC_32( hdc16 );
    DC * dc;

    if (!hrgn) return ERROR;
    if (!(dc = DC_GetDCPtr( hdc ))) return ERROR;

    TRACE("%p %04x\n", hdc, hrgn );

    dc->flags &= ~DC_DIRTY;

    retval = CombineRgn( dc->hVisRgn, HRGN_32(hrgn), 0, RGN_COPY );
    CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
    return retval;
}


/***********************************************************************
 *           OffsetClipRgn    (GDI32.@)
 */
INT WINAPI OffsetClipRgn( HDC hdc, INT x, INT y )
{
    INT ret = SIMPLEREGION;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%p %d,%d\n", hdc, x, y );

    if(dc->funcs->pOffsetClipRgn)
        ret = dc->funcs->pOffsetClipRgn( dc->physDev, x, y );
    else if (dc->hClipRgn) {
        ret = OffsetRgn( dc->hClipRgn, MulDiv( x, dc->vportExtX, dc->wndExtX ),
                         MulDiv( y, dc->vportExtY, dc->wndExtY ) );
	CLIPPING_UpdateGCRegion( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           OffsetVisRgn    (GDI.102)
 */
INT16 WINAPI OffsetVisRgn16( HDC16 hdc16, INT16 x, INT16 y )
{
    INT16 retval;
    HDC hdc = HDC_32( hdc16 );
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;
    TRACE("%p %d,%d\n", hdc, x, y );
    retval = OffsetRgn( dc->hVisRgn, x, y );
    CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
    return retval;
}


/***********************************************************************
 *           ExcludeClipRect    (GDI32.@)
 */
INT WINAPI ExcludeClipRect( HDC hdc, INT left, INT top,
                                INT right, INT bottom )
{
    HRGN newRgn;
    INT ret;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%p %dx%d,%dx%d\n", hdc, left, top, right, bottom );

    if(dc->funcs->pExcludeClipRect)
        ret = dc->funcs->pExcludeClipRect( dc->physDev, left, top, right, bottom );
    else
    {
        POINT pt[2];

        pt[0].x = left;
        pt[0].y = top;
        pt[1].x = right;
        pt[1].y = bottom;
        LPtoDP( hdc, pt, 2 );
        if (!(newRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
        else
        {
            if (!dc->hClipRgn)
            {
                dc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
                CombineRgn( dc->hClipRgn, dc->hVisRgn, 0, RGN_COPY );
            }
            ret = CombineRgn( dc->hClipRgn, dc->hClipRgn, newRgn, RGN_DIFF );
            DeleteObject( newRgn );
        }
        if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           IntersectClipRect    (GDI32.@)
 */
INT WINAPI IntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    INT ret;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    TRACE("%p %d,%d - %d,%d\n", hdc, left, top, right, bottom );

    if(dc->funcs->pIntersectClipRect)
        ret = dc->funcs->pIntersectClipRect( dc->physDev, left, top, right, bottom );
    else
    {
        POINT pt[2];

        pt[0].x = left;
        pt[0].y = top;
        pt[1].x = right;
        pt[1].y = bottom;

        LPtoDP( hdc, pt, 2 );

        if (!dc->hClipRgn)
        {
            dc->hClipRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y );
            ret = SIMPLEREGION;
        }
        else
        {
            HRGN newRgn;

            if (!(newRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
            else
            {
                ret = CombineRgn( dc->hClipRgn, dc->hClipRgn, newRgn, RGN_AND );
                DeleteObject( newRgn );
            }
        }
        if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           ExcludeVisRect   (GDI.73)
 */
INT16 WINAPI ExcludeVisRect16( HDC16 hdc16, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    HRGN tempRgn;
    INT16 ret;
    POINT pt[2];
    HDC hdc = HDC_32( hdc16 );
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    pt[0].x = left;
    pt[0].y = top;
    pt[1].x = right;
    pt[1].y = bottom;

    LPtoDP( hdc, pt, 2 );

    TRACE("%p %ld,%ld - %ld,%ld\n", hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

    if (!(tempRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
    else
    {
        ret = CombineRgn( dc->hVisRgn, dc->hVisRgn, tempRgn, RGN_DIFF );
        DeleteObject( tempRgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           IntersectVisRect   (GDI.98)
 */
INT16 WINAPI IntersectVisRect16( HDC16 hdc16, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    HRGN tempRgn;
    INT16 ret;
    POINT pt[2];
    HDC hdc = HDC_32( hdc16 );
    DC * dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;

    pt[0].x = left;
    pt[0].y = top;
    pt[1].x = right;
    pt[1].y = bottom;

    LPtoDP( hdc, pt, 2 );

    TRACE("%p %ld,%ld - %ld,%ld\n", hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);


    if (!(tempRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
    else
    {
        ret = CombineRgn( dc->hVisRgn, dc->hVisRgn, tempRgn, RGN_AND );
        DeleteObject( tempRgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           PtVisible    (GDI32.@)
 */
BOOL WINAPI PtVisible( HDC hdc, INT x, INT y )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCUpdate( hdc );

    TRACE("%p %d,%d\n", hdc, x, y );
    if (!dc) return FALSE;
    if (dc->hGCClipRgn)
    {
        POINT pt;

        pt.x = x;
        pt.y = y;
        LPtoDP( hdc, &pt, 1 );
        ret = PtInRegion( dc->hGCClipRgn, pt.x, pt.y );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           RectVisible    (GDI32.@)
 */
BOOL WINAPI RectVisible( HDC hdc, const RECT* rect )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;
    TRACE("%p %ld,%ldx%ld,%ld\n", hdc, rect->left, rect->top, rect->right, rect->bottom );
    if (dc->hGCClipRgn)
    {
        POINT pt[2];
	RECT tmpRect;

        pt[0].x = rect->left;
        pt[0].y = rect->top;
        pt[1].x = rect->right;
        pt[1].y = rect->bottom;
        LPtoDP( hdc, pt, 2 );
	tmpRect.left	= pt[0].x;
	tmpRect.top	= pt[0].y;
	tmpRect.right	= pt[1].x;
	tmpRect.bottom	= pt[1].y;
        ret = RectInRegion( dc->hGCClipRgn, &tmpRect );
    }
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
          if( CombineRgn(hRgn, dc->hClipRgn, 0, RGN_COPY) != ERROR ) ret = 1;
      }
      else ret = 0;
      GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *           SaveVisRgn   (GDI.129)
 */
HRGN16 WINAPI SaveVisRgn16( HDC16 hdc16 )
{
    HRGN copy;
    GDIOBJHDR *obj, *copyObj;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCUpdate( hdc );

    if (!dc) return 0;
    TRACE("%p\n", hdc );

    if (!(obj = GDI_GetObjPtr( dc->hVisRgn, REGION_MAGIC )))
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
    if (!(copyObj = GDI_GetObjPtr( copy, REGION_MAGIC )))
    {
        DeleteObject( copy );
        GDI_ReleaseObj( dc->hVisRgn );
        GDI_ReleaseObj( hdc );
	return 0;
    }
    copyObj->hNext = obj->hNext;
    obj->hNext = HRGN_16(copy);
    GDI_ReleaseObj( copy );
    GDI_ReleaseObj( dc->hVisRgn );
    GDI_ReleaseObj( hdc );
    return HRGN_16(copy);
}


/***********************************************************************
 *           RestoreVisRgn   (GDI.130)
 */
INT16 WINAPI RestoreVisRgn16( HDC16 hdc16 )
{
    HRGN saved;
    GDIOBJHDR *obj, *savedObj;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );
    INT16 ret = ERROR;

    if (!dc) return ERROR;

    TRACE("%p\n", hdc );

    if (!(obj = GDI_GetObjPtr( dc->hVisRgn, REGION_MAGIC ))) goto done;
    saved = HRGN_32(obj->hNext);

    if ((savedObj = GDI_GetObjPtr( saved, REGION_MAGIC )))
    {
        ret = CombineRgn( dc->hVisRgn, saved, 0, RGN_COPY );
        obj->hNext = savedObj->hNext;
        GDI_ReleaseObj( saved );
        DeleteObject( saved );
        dc->flags &= ~DC_DIRTY;
        CLIPPING_UpdateGCRegion( dc );
    }
    GDI_ReleaseObj( dc->hVisRgn );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 * GetRandomRgn [GDI32.@]
 *
 * NOTES
 *     This function is documented in MSDN online for the case of
 *     dwCode == SYSRGN (4).
 *
 *     For dwCode == 1 it should return the clip region
 *                   2 "    "       "   the meta region
 *                   3 "    "       "   the intersection of the clip with
 *                                      the meta region (== 'Rao' region).
 *
 *     See http://www.codeproject.com/gdi/cliprgnguide.asp
 */
INT WINAPI GetRandomRgn(HDC hDC, HRGN hRgn, DWORD dwCode)
{
    switch (dwCode)
    {
    case SYSRGN: /* == 4 */
	{
	    DC *dc = DC_GetDCPtr (hDC);
	    if (!dc) return -1;

	    CombineRgn (hRgn, dc->hVisRgn, 0, RGN_COPY);
            GDI_ReleaseObj( hDC );
	    /*
	     *     On Windows NT/2000,
	     *           the region returned is in screen coordinates.
	     *     On Windows 95/98,
	     *           the region returned is in window coordinates
	     */
            if (!(GetVersion() & 0x80000000))
            {
                POINT org;
                GetDCOrgEx(hDC, &org);
                OffsetRgn(hRgn, org.x, org.y);
            }
	    return 1;
	}

    case 1: /* clip region */
            return GetClipRgn (hDC, hRgn);

    default:
        WARN("Unknown dwCode %ld\n", dwCode);
        return -1;
    }

    return -1;
}


/***********************************************************************
 *           GetMetaRgn    (GDI32.@)
 */
INT WINAPI GetMetaRgn( HDC hdc, HRGN hRgn )
{
    FIXME( "stub\n" );

    return 0;
}


/***********************************************************************
 *           SetMetaRgn    (GDI32.@)
 */
INT WINAPI SetMetaRgn( HDC hdc )
{
    FIXME( "stub\n" );

    return ERROR;
}

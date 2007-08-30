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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipping);


/***********************************************************************
 *           get_clip_region
 *
 * Return the total clip region (if any).
 */
static inline HRGN get_clip_region( DC * dc )
{
    if (dc->hMetaClipRgn) return dc->hMetaClipRgn;
    if (dc->hMetaRgn) return dc->hMetaRgn;
    return dc->hClipRgn;
}


/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn or VisRgn have changed.
 */
void CLIPPING_UpdateGCRegion( DC * dc )
{
    HRGN clip_rgn;

    if (!dc->hVisRgn)
    {
        ERR("hVisRgn is zero. Please report this.\n" );
        exit(1);
    }

    if (dc->flags & DC_DIRTY) ERR( "DC is dirty. Please report this.\n" );

    /* update the intersection of meta and clip regions */
    if (dc->hMetaRgn && dc->hClipRgn)
    {
        if (!dc->hMetaClipRgn) dc->hMetaClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->hMetaClipRgn, dc->hClipRgn, dc->hMetaRgn, RGN_AND );
        clip_rgn = dc->hMetaClipRgn;
    }
    else  /* only one is set, no need for an intersection */
    {
        if (dc->hMetaClipRgn) DeleteObject( dc->hMetaClipRgn );
        dc->hMetaClipRgn = 0;
        clip_rgn = dc->hMetaRgn ? dc->hMetaRgn : dc->hClipRgn;
    }

    if (dc->funcs->pSetDeviceClipping)
        dc->funcs->pSetDeviceClipping( dc->physDev, dc->hVisRgn, clip_rgn );
}

/***********************************************************************
 *           create_default_clip_region
 *
 * Create a default clipping region when none already exists.
 */
static inline void create_default_clip_region( DC * dc )
{
    UINT width, height;

    if (GDIMAGIC( dc->header.wMagic ) == MEMORY_DC_MAGIC)
    {
        BITMAP bitmap;

        GetObjectW( dc->hBitmap, sizeof(bitmap), &bitmap );
        width = bitmap.bmWidth;
        height = bitmap.bmHeight;
    }
    else
    {
        width = GetDeviceCaps( dc->hSelf, DESKTOPHORZRES );
        height = GetDeviceCaps( dc->hSelf, DESKTOPVERTRES );
    }
    dc->hClipRgn = CreateRectRgn( 0, 0, width, height );
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
        DC_ReleaseDCPtr( dc );
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
            DC_ReleaseDCPtr( dc );
            return ERROR;
        }
    }
    else
    {
        if (!dc->hClipRgn)
            create_default_clip_region( dc );

        if(fnMode == RGN_COPY)
            CombineRgn( dc->hClipRgn, hrgn, 0, fnMode );
        else
            CombineRgn( dc->hClipRgn, dc->hClipRgn, hrgn, fnMode);
    }

    CLIPPING_UpdateGCRegion( dc );
    DC_ReleaseDCPtr( dc );

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
    DC_ReleaseDCPtr( dc );
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
    {
        ret = dc->funcs->pOffsetClipRgn( dc->physDev, x, y );
        /* FIXME: ret is just a success flag, we should return a proper value */
    }
    else if (dc->hClipRgn) {
        ret = OffsetRgn( dc->hClipRgn, MulDiv( x, dc->vportExtX, dc->wndExtX ),
                         MulDiv( y, dc->vportExtY, dc->wndExtY ) );
	CLIPPING_UpdateGCRegion( dc );
    }
    DC_ReleaseDCPtr( dc );
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
    DC_ReleaseDCPtr( dc );
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
    {
        ret = dc->funcs->pExcludeClipRect( dc->physDev, left, top, right, bottom );
        /* FIXME: ret is just a success flag, we should return a proper value */
    }
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
                create_default_clip_region( dc );
            ret = CombineRgn( dc->hClipRgn, dc->hClipRgn, newRgn, RGN_DIFF );
            DeleteObject( newRgn );
        }
        if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    }
    DC_ReleaseDCPtr( dc );
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
    {
        ret = dc->funcs->pIntersectClipRect( dc->physDev, left, top, right, bottom );
        /* FIXME: ret is just a success flag, we should return a proper value */
    }
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
    DC_ReleaseDCPtr( dc );
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

    TRACE("%p %d,%d - %d,%d\n", hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

    if (!(tempRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
    else
    {
        ret = CombineRgn( dc->hVisRgn, dc->hVisRgn, tempRgn, RGN_DIFF );
        DeleteObject( tempRgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    DC_ReleaseDCPtr( dc );
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

    TRACE("%p %d,%d - %d,%d\n", hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);


    if (!(tempRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
    else
    {
        ret = CombineRgn( dc->hVisRgn, dc->hVisRgn, tempRgn, RGN_AND );
        DeleteObject( tempRgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    DC_ReleaseDCPtr( dc );
    return ret;
}


/***********************************************************************
 *           PtVisible    (GDI32.@)
 */
BOOL WINAPI PtVisible( HDC hdc, INT x, INT y )
{
    POINT pt;
    BOOL ret;
    HRGN clip;
    DC *dc = DC_GetDCUpdate( hdc );

    TRACE("%p %d,%d\n", hdc, x, y );
    if (!dc) return FALSE;

    pt.x = x;
    pt.y = y;
    LPtoDP( hdc, &pt, 1 );
    ret = PtInRegion( dc->hVisRgn, pt.x, pt.y );
    if (ret && (clip = get_clip_region(dc))) ret = PtInRegion( clip, pt.x, pt.y );
    DC_ReleaseDCPtr( dc );
    return ret;
}


/***********************************************************************
 *           RectVisible    (GDI32.@)
 */
BOOL WINAPI RectVisible( HDC hdc, const RECT* rect )
{
    RECT tmpRect;
    BOOL ret;
    HRGN clip;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return FALSE;
    TRACE("%p %d,%dx%d,%d\n", hdc, rect->left, rect->top, rect->right, rect->bottom );

    tmpRect = *rect;
    LPtoDP( hdc, (POINT *)&tmpRect, 2 );

    if ((clip = get_clip_region(dc)))
    {
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( hrgn, dc->hVisRgn, clip, RGN_AND );
        ret = RectInRegion( hrgn, &tmpRect );
        DeleteObject( hrgn );
    }
    else ret = RectInRegion( dc->hVisRgn, &tmpRect );
    DC_ReleaseDCPtr( dc );
    return ret;
}


/***********************************************************************
 *           GetClipBox    (GDI32.@)
 */
INT WINAPI GetClipBox( HDC hdc, LPRECT rect )
{
    INT ret;
    HRGN clip;
    DC *dc = DC_GetDCUpdate( hdc );
    if (!dc) return ERROR;
    if ((clip = get_clip_region(dc)))
    {
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( hrgn, dc->hVisRgn, clip, RGN_AND );
        ret = GetRgnBox( hrgn, rect );
        DeleteObject( hrgn );
    }
    else ret = GetRgnBox( dc->hVisRgn, rect );
    DPtoLP( hdc, (LPPOINT)rect, 2 );
    DC_ReleaseDCPtr( dc );
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
      DC_ReleaseDCPtr( dc );
    }
    return ret;
}


/***********************************************************************
 *           GetMetaRgn    (GDI32.@)
 */
INT WINAPI GetMetaRgn( HDC hdc, HRGN hRgn )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );

    if (dc)
    {
        if (dc->hMetaRgn && CombineRgn( hRgn, dc->hMetaRgn, 0, RGN_COPY ) != ERROR)
            ret = 1;
        DC_ReleaseDCPtr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SaveVisRgn   (GDI.129)
 */
HRGN16 WINAPI SaveVisRgn16( HDC16 hdc16 )
{
    struct saved_visrgn *saved;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCUpdate( hdc );

    if (!dc) return 0;
    TRACE("%p\n", hdc );

    if (!(saved = HeapAlloc( GetProcessHeap(), 0, sizeof(*saved) ))) goto error;
    if (!(saved->hrgn = CreateRectRgn( 0, 0, 0, 0 ))) goto error;
    CombineRgn( saved->hrgn, dc->hVisRgn, 0, RGN_COPY );
    saved->next = dc->saved_visrgn;
    dc->saved_visrgn = saved;
    DC_ReleaseDCPtr( dc );
    return HRGN_16(saved->hrgn);

error:
    DC_ReleaseDCPtr( dc );
    HeapFree( GetProcessHeap(), 0, saved );
    return 0;
}


/***********************************************************************
 *           RestoreVisRgn   (GDI.130)
 */
INT16 WINAPI RestoreVisRgn16( HDC16 hdc16 )
{
    struct saved_visrgn *saved;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );
    INT16 ret = ERROR;

    if (!dc) return ERROR;

    TRACE("%p\n", hdc );

    if (!(saved = dc->saved_visrgn)) goto done;

    ret = CombineRgn( dc->hVisRgn, saved->hrgn, 0, RGN_COPY );
    dc->saved_visrgn = saved->next;
    DeleteObject( saved->hrgn );
    HeapFree( GetProcessHeap(), 0, saved );
    dc->flags &= ~DC_DIRTY;
    CLIPPING_UpdateGCRegion( dc );
 done:
    DC_ReleaseDCPtr( dc );
    return ret;
}


/***********************************************************************
 * GetRandomRgn [GDI32.@]
 *
 * NOTES
 *     This function is documented in MSDN online for the case of
 *     iCode == SYSRGN (4).
 *
 *     For iCode == 1 it should return the clip region
 *                  2 "    "       "   the meta region
 *                  3 "    "       "   the intersection of the clip with
 *                                     the meta region (== 'Rao' region).
 *
 *     See http://www.codeproject.com/gdi/cliprgnguide.asp
 */
INT WINAPI GetRandomRgn(HDC hDC, HRGN hRgn, INT iCode)
{
    HRGN rgn;
    DC *dc = DC_GetDCPtr( hDC );

    if (!dc) return -1;

    switch (iCode)
    {
    case 1:
        rgn = dc->hClipRgn;
        break;
    case 2:
        rgn = dc->hMetaRgn;
        break;
    case 3:
        rgn = dc->hMetaClipRgn;
        if(!rgn) rgn = dc->hClipRgn;
        if(!rgn) rgn = dc->hMetaRgn;
        break;
    case SYSRGN: /* == 4 */
        rgn = dc->hVisRgn;
        break;
    default:
        WARN("Unknown code %d\n", iCode);
        DC_ReleaseDCPtr( dc );
        return -1;
    }
    if (rgn) CombineRgn( hRgn, rgn, 0, RGN_COPY );
    DC_ReleaseDCPtr( dc );

    /* On Windows NT/2000, the SYSRGN returned is in screen coordinates */
    if (iCode == SYSRGN && !(GetVersion() & 0x80000000))
    {
        POINT org;
        GetDCOrgEx( hDC, &org );
        OffsetRgn( hRgn, org.x, org.y );
    }
    return (rgn != 0);
}


/***********************************************************************
 *           SetMetaRgn    (GDI32.@)
 */
INT WINAPI SetMetaRgn( HDC hdc )
{
    INT ret;
    RECT dummy;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return ERROR;

    if (dc->hMetaClipRgn)
    {
        /* the intersection becomes the new meta region */
        DeleteObject( dc->hMetaRgn );
        DeleteObject( dc->hClipRgn );
        dc->hMetaRgn = dc->hMetaClipRgn;
        dc->hClipRgn = 0;
        dc->hMetaClipRgn = 0;
    }
    else if (dc->hClipRgn)
    {
        dc->hMetaRgn = dc->hClipRgn;
        dc->hClipRgn = 0;
    }
    /* else nothing to do */

    /* Note: no need to call CLIPPING_UpdateGCRegion, the overall clip region hasn't changed */

    ret = GetRgnBox( dc->hMetaRgn, &dummy );
    DC_ReleaseDCPtr( dc );
    return ret;
}

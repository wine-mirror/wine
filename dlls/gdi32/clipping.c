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
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipping);


/***********************************************************************
 *           get_clip_rect
 *
 * Compute a clip rectangle from its logical coordinates.
 */
static inline RECT get_clip_rect( DC * dc, int left, int top, int right, int bottom )
{
    RECT rect;

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;
    LPtoDP( dc->hSelf, (POINT *)&rect, 2 );
    if (dc->layout & LAYOUT_RTL)
    {
        int tmp = rect.left;
        rect.left = rect.right + 1;
        rect.right = tmp + 1;
    }
    return rect;
}

/***********************************************************************
 *           get_clip_box
 *
 * Get the clipping rectangle in device coordinates.
 */
int get_clip_box( DC *dc, RECT *rect )
{
    int ret = ERROR;
    HRGN rgn, clip = get_clip_region( dc );

    if (!clip) return GetRgnBox( dc->hVisRgn, rect );

    if ((rgn = CreateRectRgn( 0, 0, 0, 0 )))
    {
        CombineRgn( rgn, dc->hVisRgn, clip, RGN_AND );
        ret = GetRgnBox( rgn, rect );
        DeleteObject( rgn );
    }
    return ret;
}

/***********************************************************************
 *           CLIPPING_UpdateGCRegion
 *
 * Update the GC clip region when the ClipRgn or VisRgn have changed.
 */
void CLIPPING_UpdateGCRegion( DC * dc )
{
    HRGN clip_rgn;
    PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDeviceClipping );

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
    physdev->funcs->pSetDeviceClipping( physdev, dc->hVisRgn, clip_rgn );
}

/***********************************************************************
 *           create_default_clip_region
 *
 * Create a default clipping region when none already exists.
 */
static inline void create_default_clip_region( DC * dc )
{
    UINT width, height;

    if (dc->header.type == OBJ_MEMDC)
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
 *           null driver fallback implementations
 */

INT nulldrv_ExtSelectClipRgn( PHYSDEV dev, HRGN rgn, INT mode )
{
    DC *dc = get_nulldrv_dc( dev );
    INT ret;

    if (!rgn)
    {
        if (mode != RGN_COPY)
        {
            FIXME("Unimplemented: hrgn NULL in mode: %d\n", mode);
            return ERROR;
        }
        if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
        dc->hClipRgn = 0;
        ret = SIMPLEREGION;
    }
    else
    {
        HRGN mirrored = 0;

        if (dc->layout & LAYOUT_RTL)
        {
            if (!(mirrored = CreateRectRgn( 0, 0, 0, 0 ))) return ERROR;
            mirror_region( mirrored, rgn, dc->vis_rect.right - dc->vis_rect.left );
            rgn = mirrored;
        }

        if (!dc->hClipRgn)
            create_default_clip_region( dc );

        if (mode == RGN_COPY)
            ret = CombineRgn( dc->hClipRgn, rgn, 0, mode );
        else
            ret = CombineRgn( dc->hClipRgn, dc->hClipRgn, rgn, mode);

        if (mirrored) DeleteObject( mirrored );
    }
    CLIPPING_UpdateGCRegion( dc );
    return ret;
}

INT nulldrv_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    DC *dc = get_nulldrv_dc( dev );
    RECT rect = get_clip_rect( dc, left, top, right, bottom );
    INT ret;
    HRGN rgn;

    if (!(rgn = CreateRectRgnIndirect( &rect ))) return ERROR;
    if (!dc->hClipRgn) create_default_clip_region( dc );
    ret = CombineRgn( dc->hClipRgn, dc->hClipRgn, rgn, RGN_DIFF );
    DeleteObject( rgn );
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    return ret;
}

INT nulldrv_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    DC *dc = get_nulldrv_dc( dev );
    RECT rect = get_clip_rect( dc, left, top, right, bottom );
    INT ret;
    HRGN rgn;

    if (!dc->hClipRgn)
    {
        dc->hClipRgn = CreateRectRgnIndirect( &rect );
        ret = SIMPLEREGION;
    }
    else
    {
        if (!(rgn = CreateRectRgnIndirect( &rect ))) return ERROR;
        ret = CombineRgn( dc->hClipRgn, dc->hClipRgn, rgn, RGN_AND );
        DeleteObject( rgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    return ret;
}

INT nulldrv_OffsetClipRgn( PHYSDEV dev, INT x, INT y )
{
    DC *dc = get_nulldrv_dc( dev );
    INT ret = NULLREGION;

    if (dc->hClipRgn)
    {
        x = MulDiv( x, dc->vportExtX, dc->wndExtX );
        y = MulDiv( y, dc->vportExtY, dc->wndExtY );
        if (dc->layout & LAYOUT_RTL) x = -x;
        ret = OffsetRgn( dc->hClipRgn, x, y );
	CLIPPING_UpdateGCRegion( dc );
    }
    return ret;
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
    INT retval = ERROR;
    DC * dc = get_dc_ptr( hdc );

    TRACE("%p %p %d\n", hdc, hrgn, fnMode );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pExtSelectClipRgn );
        update_dc( dc );
        retval = physdev->funcs->pExtSelectClipRgn( physdev, hrgn, fnMode );
        release_dc_ptr( dc );
    }
    return retval;
}

/***********************************************************************
 *           __wine_set_visible_region   (GDI32.@)
 */
void CDECL __wine_set_visible_region( HDC hdc, HRGN hrgn, const RECT *vis_rect )
{
    DC * dc;

    if (!(dc = get_dc_ptr( hdc ))) return;

    TRACE( "%p %p %s\n", hdc, hrgn, wine_dbgstr_rect(vis_rect) );

    /* map region to DC coordinates */
    OffsetRgn( hrgn, -vis_rect->left, -vis_rect->top );

    DeleteObject( dc->hVisRgn );
    dc->dirty = 0;
    dc->vis_rect = *vis_rect;
    dc->hVisRgn = hrgn;
    DC_UpdateXforms( dc );
    CLIPPING_UpdateGCRegion( dc );
    release_dc_ptr( dc );
}


/***********************************************************************
 *           OffsetClipRgn    (GDI32.@)
 */
INT WINAPI OffsetClipRgn( HDC hdc, INT x, INT y )
{
    INT ret = ERROR;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p %d,%d\n", hdc, x, y );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pOffsetClipRgn );
        update_dc( dc );
        ret = physdev->funcs->pOffsetClipRgn( physdev, x, y );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           ExcludeClipRect    (GDI32.@)
 */
INT WINAPI ExcludeClipRect( HDC hdc, INT left, INT top,
                                INT right, INT bottom )
{
    INT ret = ERROR;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p %d,%d-%d,%d\n", hdc, left, top, right, bottom );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pExcludeClipRect );
        update_dc( dc );
        ret = physdev->funcs->pExcludeClipRect( physdev, left, top, right, bottom );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           IntersectClipRect    (GDI32.@)
 */
INT WINAPI IntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    INT ret = ERROR;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p %d,%d - %d,%d\n", hdc, left, top, right, bottom );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pIntersectClipRect );
        update_dc( dc );
        ret = physdev->funcs->pIntersectClipRect( physdev, left, top, right, bottom );
        release_dc_ptr( dc );
    }
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
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p %d,%d\n", hdc, x, y );
    if (!dc) return FALSE;

    pt.x = x;
    pt.y = y;
    LPtoDP( hdc, &pt, 1 );
    update_dc( dc );
    ret = PtInRegion( dc->hVisRgn, pt.x, pt.y );
    if (ret && (clip = get_clip_region(dc))) ret = PtInRegion( clip, pt.x, pt.y );
    release_dc_ptr( dc );
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
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    TRACE("%p %s\n", hdc, wine_dbgstr_rect( rect ));

    tmpRect = *rect;
    LPtoDP( hdc, (POINT *)&tmpRect, 2 );

    update_dc( dc );
    if ((clip = get_clip_region(dc)))
    {
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( hrgn, dc->hVisRgn, clip, RGN_AND );
        ret = RectInRegion( hrgn, &tmpRect );
        DeleteObject( hrgn );
    }
    else ret = RectInRegion( dc->hVisRgn, &tmpRect );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetClipBox    (GDI32.@)
 */
INT WINAPI GetClipBox( HDC hdc, LPRECT rect )
{
    INT ret;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return ERROR;

    update_dc( dc );
    ret = get_clip_box( dc, rect );
    if (dc->layout & LAYOUT_RTL)
    {
        int tmp = rect->left;
        rect->left = rect->right - 1;
        rect->right = tmp - 1;
    }
    DPtoLP( hdc, (LPPOINT)rect, 2 );
    release_dc_ptr( dc );
    TRACE("%p => %d %s\n", hdc, ret, wine_dbgstr_rect( rect ));
    return ret;
}


/***********************************************************************
 *           GetClipRgn  (GDI32.@)
 */
INT WINAPI GetClipRgn( HDC hdc, HRGN hRgn )
{
    INT ret = -1;
    DC * dc;
    if ((dc = get_dc_ptr( hdc )))
    {
      if( dc->hClipRgn )
      {
          if( CombineRgn(hRgn, dc->hClipRgn, 0, RGN_COPY) != ERROR )
          {
              ret = 1;
              if (dc->layout & LAYOUT_RTL)
                  mirror_region( hRgn, hRgn, dc->vis_rect.right - dc->vis_rect.left );
          }
      }
      else ret = 0;
      release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           GetMetaRgn    (GDI32.@)
 */
INT WINAPI GetMetaRgn( HDC hdc, HRGN hRgn )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        if (dc->hMetaRgn && CombineRgn( hRgn, dc->hMetaRgn, 0, RGN_COPY ) != ERROR)
        {
            ret = 1;
            if (dc->layout & LAYOUT_RTL)
                mirror_region( hRgn, hRgn, dc->vis_rect.right - dc->vis_rect.left );
        }
        release_dc_ptr( dc );
    }
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
    DC *dc = get_dc_ptr( hDC );

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
        update_dc( dc );
        rgn = dc->hVisRgn;
        break;
    default:
        WARN("Unknown code %d\n", iCode);
        release_dc_ptr( dc );
        return -1;
    }
    if (rgn) CombineRgn( hRgn, rgn, 0, RGN_COPY );
    release_dc_ptr( dc );

    /* On Windows NT/2000, the SYSRGN returned is in screen coordinates */
    if (iCode == SYSRGN && !(GetVersion() & 0x80000000))
        OffsetRgn( hRgn, dc->vis_rect.left, dc->vis_rect.top );

    return (rgn != 0);
}


/***********************************************************************
 *           SetMetaRgn    (GDI32.@)
 */
INT WINAPI SetMetaRgn( HDC hdc )
{
    INT ret;
    RECT dummy;
    DC *dc = get_dc_ptr( hdc );

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
    release_dc_ptr( dc );
    return ret;
}

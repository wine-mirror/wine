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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipping);


/* return the DC device rectangle if not empty */
static inline BOOL get_dc_device_rect( DC *dc, RECT *rect )
{
    *rect = dc->device_rect;
    OffsetRect( rect, -dc->attr->vis_rect.left, -dc->attr->vis_rect.top );
    return !IsRectEmpty( rect );
}

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
    lp_to_dp( dc, (POINT *)&rect, 2 );
    if (dc->attr->layout & LAYOUT_RTL)
    {
        int tmp = rect.left;
        rect.left = rect.right + 1;
        rect.right = tmp + 1;
    }
    return rect;
}

/***********************************************************************
 *           clip_device_rect
 *
 * Clip a rectangle to the whole DC surface.
 */
BOOL clip_device_rect( DC *dc, RECT *dst, const RECT *src )
{
    RECT clip;

    if (get_dc_device_rect( dc, &clip )) return intersect_rect( dst, src, &clip );
    *dst = *src;
    return TRUE;
}

/***********************************************************************
 *           clip_visrect
 *
 * Clip a rectangle to the DC visible rect.
 */
BOOL clip_visrect( DC *dc, RECT *dst, const RECT *src )
{
    RECT clip;

    if (!clip_device_rect( dc, dst, src )) return FALSE;
    if (NtGdiGetRgnBox( get_dc_region(dc), &clip )) return intersect_rect( dst, dst, &clip );
    return TRUE;
}

/***********************************************************************
 *           update_dc_clipping
 *
 * Update the DC and device clip regions when the ClipRgn or VisRgn have changed.
 */
void update_dc_clipping( DC * dc )
{
    PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDeviceClipping );
    HRGN regions[3];
    int count = 0;

    if (dc->hVisRgn)  regions[count++] = dc->hVisRgn;
    if (dc->hClipRgn) regions[count++] = dc->hClipRgn;
    if (dc->hMetaRgn) regions[count++] = dc->hMetaRgn;

    if (count > 1)
    {
        if (!dc->region) dc->region = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( dc->region, regions[0], regions[1], RGN_AND );
        if (count > 2) NtGdiCombineRgn( dc->region, dc->region, regions[2], RGN_AND );
    }
    else  /* only one region, we don't need the total region */
    {
        if (dc->region) NtGdiDeleteObjectApp( dc->region );
        dc->region = 0;
    }
    physdev->funcs->pSetDeviceClipping( physdev, get_dc_region( dc ));
}

/***********************************************************************
 *           create_default_clip_region
 *
 * Create a default clipping region when none already exists.
 */
static inline void create_default_clip_region( DC * dc )
{
    RECT rect;

    if (!get_dc_device_rect( dc, &rect ))
    {
        rect.left = 0;
        rect.top = 0;
        rect.right = NtGdiGetDeviceCaps( dc->hSelf, DESKTOPHORZRES );
        rect.bottom = NtGdiGetDeviceCaps( dc->hSelf, DESKTOPVERTRES );
    }
    dc->hClipRgn = NtGdiCreateRectRgn( rect.left, rect.top, rect.right, rect.bottom );
}


/******************************************************************************
 *		NtGdiExtSelectClipRgn  (win32u.@)
 */
INT WINAPI NtGdiExtSelectClipRgn( HDC hdc, HRGN rgn, INT mode )
{
    INT ret = ERROR;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return ERROR;
    update_dc( dc );

    if (!rgn)
    {
        switch (mode)
        {
        case RGN_COPY:
            if (dc->hClipRgn) NtGdiDeleteObjectApp( dc->hClipRgn );
            dc->hClipRgn = 0;
            ret = SIMPLEREGION;
            break;

        case RGN_DIFF:
            break;

        default:
            FIXME("Unimplemented: hrgn NULL in mode: %d\n", mode);
            break;
        }
    }
    else
    {
        HRGN mirrored = 0;

        if (dc->attr->layout & LAYOUT_RTL)
        {
            if (!(mirrored = NtGdiCreateRectRgn( 0, 0, 0, 0 )))
            {
                release_dc_ptr( dc );
                return ERROR;
            }
            mirror_region( mirrored, rgn, dc->attr->vis_rect.right - dc->attr->vis_rect.left );
            rgn = mirrored;
        }

        if (!dc->hClipRgn)
            create_default_clip_region( dc );

        if (mode == RGN_COPY)
            ret = NtGdiCombineRgn( dc->hClipRgn, rgn, 0, mode );
        else
            ret = NtGdiCombineRgn( dc->hClipRgn, dc->hClipRgn, rgn, mode );

        if (mirrored) NtGdiDeleteObjectApp( mirrored );
    }
    if (ret != ERROR) update_dc_clipping( dc );
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           set_visible_region
 */
void set_visible_region( HDC hdc, HRGN hrgn, const RECT *vis_rect, const RECT *device_rect,
                         struct window_surface *surface )
{
    DC * dc;

    if (!(dc = get_dc_ptr( hdc ))) return;

    TRACE( "%p %p %s %s %p\n", hdc, hrgn,
           wine_dbgstr_rect(vis_rect), wine_dbgstr_rect(device_rect), surface );

    /* map region to DC coordinates */
    NtGdiOffsetRgn( hrgn, -vis_rect->left, -vis_rect->top );

    if (dc->hVisRgn) NtGdiDeleteObjectApp( dc->hVisRgn );
    dc->dirty = 0;
    dc->attr->vis_rect = *vis_rect;
    dc->device_rect = *device_rect;
    dc->hVisRgn = hrgn;
    dibdrv_set_window_surface( dc, surface );
    DC_UpdateXforms( dc );
    update_dc_clipping( dc );
    release_dc_ptr( dc );
}


/***********************************************************************
 *           NtGdiOffsetClipRgn    (win32u.@)
 */
INT WINAPI NtGdiOffsetClipRgn( HDC hdc, INT x, INT y )
{
    INT ret = NULLREGION;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return ERROR;
    update_dc( dc );

    if (dc->hClipRgn)
    {
        x = muldiv( x, dc->attr->vport_ext.cx, dc->attr->wnd_ext.cx );
        y = muldiv( y, dc->attr->vport_ext.cy, dc->attr->wnd_ext.cy );
        if (dc->attr->layout & LAYOUT_RTL) x = -x;
        ret = NtGdiOffsetRgn( dc->hClipRgn, x, y );
        update_dc_clipping( dc );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiExcludeClipRect    (win32u.@)
 */
INT WINAPI NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    INT ret = ERROR;
    RECT rect;
    HRGN rgn;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p %d,%d-%d,%d\n", hdc, left, top, right, bottom );

    if (!dc) return ERROR;
    update_dc( dc );

    rect = get_clip_rect( dc, left, top, right, bottom );

    if ((rgn = NtGdiCreateRectRgn( rect.left, rect.top, rect.right, rect.bottom )))
    {
        if (!dc->hClipRgn) create_default_clip_region( dc );
        ret = NtGdiCombineRgn( dc->hClipRgn, dc->hClipRgn, rgn, RGN_DIFF );
        NtGdiDeleteObjectApp( rgn );
        if (ret != ERROR) update_dc_clipping( dc );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiIntersectClipRect    (win32u.@)
 */
INT WINAPI NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    INT ret = ERROR;
    RECT rect;
    HRGN rgn;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return ERROR;
    update_dc( dc );

    rect = get_clip_rect( dc, left, top, right, bottom );
    if (!dc->hClipRgn)
    {
        if ((dc->hClipRgn = NtGdiCreateRectRgn( rect.left, rect.top, rect.right, rect.bottom )))
            ret = SIMPLEREGION;
    }
    else if ((rgn = NtGdiCreateRectRgn( rect.left, rect.top, rect.right, rect.bottom )))
    {
        ret = NtGdiCombineRgn( dc->hClipRgn, dc->hClipRgn, rgn, RGN_AND );
        NtGdiDeleteObjectApp( rgn );
    }
    if (ret != ERROR) update_dc_clipping( dc );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiPtVisible    (win32u.@)
 */
BOOL WINAPI NtGdiPtVisible( HDC hdc, INT x, INT y )
{
    POINT pt;
    RECT visrect;
    BOOL ret;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p %d,%d\n", hdc, x, y );
    if (!dc) return FALSE;

    pt.x = x;
    pt.y = y;
    lp_to_dp( dc, &pt, 1 );
    update_dc( dc );
    ret = (!get_dc_device_rect( dc, &visrect ) ||
           (pt.x >= visrect.left && pt.x < visrect.right &&
            pt.y >= visrect.top && pt.y < visrect.bottom));
    if (ret && get_dc_region( dc )) ret = NtGdiPtInRegion( get_dc_region( dc ), pt.x, pt.y );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiRectVisible    (win32u.@)
 */
BOOL WINAPI NtGdiRectVisible( HDC hdc, const RECT *rect )
{
    RECT tmpRect, visrect;
    BOOL ret;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    TRACE("%p %s\n", hdc, wine_dbgstr_rect( rect ));

    tmpRect = *rect;
    lp_to_dp( dc, (POINT *)&tmpRect, 2 );
    order_rect( &tmpRect );

    update_dc( dc );
    ret = (!get_dc_device_rect( dc, &visrect ) || intersect_rect( &visrect, &visrect, &tmpRect ));
    if (ret && get_dc_region( dc )) ret = NtGdiRectInRegion( get_dc_region( dc ), &tmpRect );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiGetAppClipBox    (win32u.@)
 */
INT WINAPI NtGdiGetAppClipBox( HDC hdc, RECT *rect )
{
    RECT visrect;
    INT ret;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return ERROR;

    update_dc( dc );
    if (get_dc_region( dc ))
    {
        ret = NtGdiGetRgnBox( get_dc_region( dc ), rect );
    }
    else
    {
        ret = IsRectEmpty( &dc->attr->vis_rect ) ? ERROR : SIMPLEREGION;
        *rect = dc->attr->vis_rect;
    }

    if (get_dc_device_rect( dc, &visrect ) && !intersect_rect( rect, rect, &visrect )) ret = NULLREGION;

    if (dc->attr->layout & LAYOUT_RTL)
    {
        int tmp = rect->left;
        rect->left = rect->right - 1;
        rect->right = tmp - 1;
    }
    dp_to_lp( dc, (LPPOINT)rect, 2 );
    release_dc_ptr( dc );
    TRACE("%p => %d %s\n", hdc, ret, wine_dbgstr_rect( rect ));
    return ret;
}


/***********************************************************************
 *           NtGdiGetRandomRgn    (win32u.@)
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
INT WINAPI NtGdiGetRandomRgn( HDC hDC, HRGN hRgn, INT iCode )
{
    INT ret = 1;
    DC *dc = get_dc_ptr( hDC );

    if (!dc) return -1;

    switch (iCode & ~NTGDI_RGN_MIRROR_RTL)
    {
    case 1:
        if (!dc->hClipRgn) ret = 0;
        else if (!NtGdiCombineRgn( hRgn, dc->hClipRgn, 0, RGN_COPY )) ret = -1;
        break;
    case 2:
        if (!dc->hMetaRgn) ret = 0;
        else if (!NtGdiCombineRgn( hRgn, dc->hMetaRgn, 0, RGN_COPY )) ret = -1;
        break;
    case 3:
        if (dc->hClipRgn && dc->hMetaRgn) NtGdiCombineRgn( hRgn, dc->hClipRgn, dc->hMetaRgn, RGN_AND );
        else if (dc->hClipRgn) NtGdiCombineRgn( hRgn, dc->hClipRgn, 0, RGN_COPY );
        else if (dc->hMetaRgn) NtGdiCombineRgn( hRgn, dc->hMetaRgn, 0, RGN_COPY );
        else ret = 0;
        break;
    case SYSRGN: /* == 4 */
        update_dc( dc );
        if (dc->hVisRgn)
        {
            NtGdiCombineRgn( hRgn, dc->hVisRgn, 0, RGN_COPY );
            /* On Windows NT/2000, the SYSRGN returned is in screen coordinates */
            if (NtCurrentTeb()->Peb->OSPlatformId != VER_PLATFORM_WIN32s)
                NtGdiOffsetRgn( hRgn, dc->attr->vis_rect.left, dc->attr->vis_rect.top );
        }
        else if (!IsRectEmpty( &dc->device_rect ))
            NtGdiSetRectRgn( hRgn, dc->device_rect.left, dc->device_rect.top,
                             dc->device_rect.right, dc->device_rect.bottom );
        else
            ret = 0;
        break;
    default:
        WARN("Unknown code %d\n", iCode);
        ret = -1;
        break;
    }

    /* Wine extension */
    if (ret > 0 && (iCode & NTGDI_RGN_MIRROR_RTL) && (dc->attr->layout & LAYOUT_RTL))
        mirror_region( hRgn, hRgn, dc->attr->vis_rect.right - dc->attr->vis_rect.left );

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiSetMetaRgn    (win32u.@)
 */
INT WINAPI NtGdiSetMetaRgn( HDC hdc )
{
    INT ret;
    RECT dummy;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return ERROR;

    if (dc->hClipRgn)
    {
        if (dc->hMetaRgn)
        {
            /* the intersection becomes the new meta region */
            NtGdiCombineRgn( dc->hMetaRgn, dc->hMetaRgn, dc->hClipRgn, RGN_AND );
            NtGdiDeleteObjectApp( dc->hClipRgn );
            dc->hClipRgn = 0;
        }
        else
        {
            dc->hMetaRgn = dc->hClipRgn;
            dc->hClipRgn = 0;
        }
    }
    /* else nothing to do */

    /* Note: no need to call update_dc_clipping, the overall clip region hasn't changed */

    ret = NtGdiGetRgnBox( dc->hMetaRgn, &dummy );
    release_dc_ptr( dc );
    return ret;
}

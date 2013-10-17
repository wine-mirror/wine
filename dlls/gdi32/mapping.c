/*
 * GDI mapping mode functions
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);


static SIZE get_dc_virtual_size( DC *dc )
{
    SIZE ret = dc->virtual_size;

    if (!ret.cx)
    {
        ret.cx = GetDeviceCaps( dc->hSelf, HORZSIZE );
        ret.cy = GetDeviceCaps( dc->hSelf, VERTSIZE );
    }
    return ret;
}

static SIZE get_dc_virtual_res( DC *dc )
{
    SIZE ret = dc->virtual_res;

    if (!ret.cx)
    {
        ret.cx = GetDeviceCaps( dc->hSelf, HORZRES );
        ret.cy = GetDeviceCaps( dc->hSelf, VERTRES );
    }
    return ret;
}

/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
static void MAPPING_FixIsotropic( DC * dc )
{
    SIZE virtual_size = get_dc_virtual_size( dc );
    SIZE virtual_res = get_dc_virtual_res( dc );
    double xdim = fabs((double)dc->vportExtX * virtual_size.cx / (virtual_res.cx * dc->wndExtX));
    double ydim = fabs((double)dc->vportExtY * virtual_size.cy / (virtual_res.cy * dc->wndExtY));

    if (xdim > ydim)
    {
        INT mincx = (dc->vportExtX >= 0) ? 1 : -1;
        dc->vportExtX = floor(dc->vportExtX * ydim / xdim + 0.5);
        if (!dc->vportExtX) dc->vportExtX = mincx;
    }
    else
    {
        INT mincy = (dc->vportExtY >= 0) ? 1 : -1;
        dc->vportExtY = floor(dc->vportExtY * xdim / ydim + 0.5);
        if (!dc->vportExtY) dc->vportExtY = mincy;
    }
}


/***********************************************************************
 *           null driver fallback implementations
 */

BOOL nulldrv_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    DC *dc = get_nulldrv_dc( dev );

    if (pt)
    {
        pt->x = dc->vportOrgX;
        pt->y = dc->vportOrgY;
    }
    dc->vportOrgX += x;
    dc->vportOrgY += y;
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    DC *dc = get_nulldrv_dc( dev );

    if (pt)
    {
        pt->x = dc->wndOrgX;
        pt->y = dc->wndOrgY;
    }
    dc->wndOrgX += x;
    dc->wndOrgY += y;
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_ScaleViewportExtEx( PHYSDEV dev, INT x_num, INT x_denom, INT y_num, INT y_denom, SIZE *size )
{
    DC *dc = get_nulldrv_dc( dev );

    if (size)
    {
        size->cx = dc->vportExtX;
        size->cy = dc->vportExtY;
    }
    if (dc->MapMode != MM_ISOTROPIC && dc->MapMode != MM_ANISOTROPIC) return TRUE;
    if (!x_num || !x_denom || !y_num || !y_denom) return FALSE;

    dc->vportExtX = (dc->vportExtX * x_num) / x_denom;
    dc->vportExtY = (dc->vportExtY * y_num) / y_denom;
    if (dc->vportExtX == 0) dc->vportExtX = 1;
    if (dc->vportExtY == 0) dc->vportExtY = 1;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_ScaleWindowExtEx( PHYSDEV dev, INT x_num, INT x_denom, INT y_num, INT y_denom, SIZE *size )
{
    DC *dc = get_nulldrv_dc( dev );

    if (size)
    {
        size->cx = dc->wndExtX;
        size->cy = dc->wndExtY;
    }
    if (dc->MapMode != MM_ISOTROPIC && dc->MapMode != MM_ANISOTROPIC) return TRUE;
    if (!x_num || !x_denom || !y_num || !y_denom) return FALSE;

    dc->wndExtX = (dc->wndExtX * x_num) / x_denom;
    dc->wndExtY = (dc->wndExtY * y_num) / y_denom;
    if (dc->wndExtX == 0) dc->wndExtX = 1;
    if (dc->wndExtY == 0) dc->wndExtY = 1;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
    return TRUE;
}

INT nulldrv_SetMapMode( PHYSDEV dev, INT mode )
{
    DC *dc = get_nulldrv_dc( dev );
    INT ret = dc->MapMode;
    SIZE virtual_size, virtual_res;

    if (mode == dc->MapMode && (mode == MM_ISOTROPIC || mode == MM_ANISOTROPIC)) return ret;

    virtual_size = get_dc_virtual_size( dc );
    virtual_res = get_dc_virtual_res( dc );
    switch (mode)
    {
    case MM_TEXT:
        dc->wndExtX   = 1;
        dc->wndExtY   = 1;
        dc->vportExtX = 1;
        dc->vportExtY = 1;
        break;
    case MM_LOMETRIC:
    case MM_ISOTROPIC:
        dc->wndExtX   = virtual_size.cx * 10;
        dc->wndExtY   = virtual_size.cy * 10;
        dc->vportExtX = virtual_res.cx;
        dc->vportExtY = -virtual_res.cy;
        break;
    case MM_HIMETRIC:
        dc->wndExtX   = virtual_size.cx * 100;
        dc->wndExtY   = virtual_size.cy * 100;
        dc->vportExtX = virtual_res.cx;
        dc->vportExtY = -virtual_res.cy;
        break;
    case MM_LOENGLISH:
        dc->wndExtX   = MulDiv(1000, virtual_size.cx, 254);
        dc->wndExtY   = MulDiv(1000, virtual_size.cy, 254);
        dc->vportExtX = virtual_res.cx;
        dc->vportExtY = -virtual_res.cy;
        break;
    case MM_HIENGLISH:
        dc->wndExtX   = MulDiv(10000, virtual_size.cx, 254);
        dc->wndExtY   = MulDiv(10000, virtual_size.cy, 254);
        dc->vportExtX = virtual_res.cx;
        dc->vportExtY = -virtual_res.cy;
        break;
    case MM_TWIPS:
        dc->wndExtX   = MulDiv(14400, virtual_size.cx, 254);
        dc->wndExtY   = MulDiv(14400, virtual_size.cy, 254);
        dc->vportExtX = virtual_res.cx;
        dc->vportExtY = -virtual_res.cy;
        break;
    case MM_ANISOTROPIC:
        break;
    default:
        return 0;
    }
    /* RTL layout is always MM_ANISOTROPIC */
    if (!(dc->layout & LAYOUT_RTL)) dc->MapMode = mode;
    DC_UpdateXforms( dc );
    return ret;
}

BOOL nulldrv_SetViewportExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    DC *dc = get_nulldrv_dc( dev );

    if (size)
    {
        size->cx = dc->vportExtX;
        size->cy = dc->vportExtY;
    }
    if (dc->MapMode != MM_ISOTROPIC && dc->MapMode != MM_ANISOTROPIC) return TRUE;
    if (!cx || !cy) return FALSE;
    dc->vportExtX = cx;
    dc->vportExtY = cy;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    DC *dc = get_nulldrv_dc( dev );

    if (pt)
    {
        pt->x = dc->vportOrgX;
        pt->y = dc->vportOrgY;
    }
    dc->vportOrgX = x;
    dc->vportOrgY = y;
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_SetWindowExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    DC *dc = get_nulldrv_dc( dev );

    if (size)
    {
        size->cx = dc->wndExtX;
        size->cy = dc->wndExtY;
    }
    if (dc->MapMode != MM_ISOTROPIC && dc->MapMode != MM_ANISOTROPIC) return TRUE;
    if (!cx || !cy) return FALSE;
    dc->wndExtX = cx;
    dc->wndExtY = cy;
    /* The API docs say that you should call SetWindowExtEx before
       SetViewportExtEx. This advice does not imply that Windows
       doesn't ensure the isotropic mapping after SetWindowExtEx! */
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    DC *dc = get_nulldrv_dc( dev );

    if (pt)
    {
        pt->x = dc->wndOrgX;
        pt->y = dc->wndOrgY;
    }
    dc->wndOrgX = x;
    dc->wndOrgY = y;
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode )
{
    DC *dc = get_nulldrv_dc( dev );

    switch (mode)
    {
    case MWT_IDENTITY:
        dc->xformWorld2Wnd.eM11 = 1.0f;
        dc->xformWorld2Wnd.eM12 = 0.0f;
        dc->xformWorld2Wnd.eM21 = 0.0f;
        dc->xformWorld2Wnd.eM22 = 1.0f;
        dc->xformWorld2Wnd.eDx  = 0.0f;
        dc->xformWorld2Wnd.eDy  = 0.0f;
        break;
    case MWT_LEFTMULTIPLY:
        CombineTransform( &dc->xformWorld2Wnd, xform, &dc->xformWorld2Wnd );
        break;
    case MWT_RIGHTMULTIPLY:
        CombineTransform( &dc->xformWorld2Wnd, &dc->xformWorld2Wnd, xform );
        break;
    default:
        return FALSE;
    }
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL nulldrv_SetWorldTransform( PHYSDEV dev, const XFORM *xform )
{
    DC *dc = get_nulldrv_dc( dev );

    dc->xformWorld2Wnd = *xform;
    DC_UpdateXforms( dc );
    return TRUE;
}

/***********************************************************************
 *           DPtoLP    (GDI32.@)
 */
BOOL WINAPI DPtoLP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if (dc->vport2WorldValid)
    {
        while (count--)
        {
            double x = points->x;
            double y = points->y;
            points->x = floor( x * dc->xformVport2World.eM11 +
                               y * dc->xformVport2World.eM21 +
                               dc->xformVport2World.eDx + 0.5 );
            points->y = floor( x * dc->xformVport2World.eM12 +
                               y * dc->xformVport2World.eM22 +
                               dc->xformVport2World.eDy + 0.5 );
            points++;
        }
    }
    release_dc_ptr( dc );
    return (count < 0);
}


/***********************************************************************
 *           LPtoDP    (GDI32.@)
 */
BOOL WINAPI LPtoDP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    while (count--)
    {
        double x = points->x;
        double y = points->y;
        points->x = floor( x * dc->xformWorld2Vport.eM11 +
                           y * dc->xformWorld2Vport.eM21 +
                           dc->xformWorld2Vport.eDx + 0.5 );
        points->y = floor( x * dc->xformWorld2Vport.eM12 +
                           y * dc->xformWorld2Vport.eM22 +
                           dc->xformWorld2Vport.eDy + 0.5 );
        points++;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           SetMapMode    (GDI32.@)
 */
INT WINAPI SetMapMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );

    TRACE("%p %d\n", hdc, mode );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetMapMode );
        ret = physdev->funcs->pSetMapMode( physdev, mode );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetViewportExtEx    (GDI32.@)
 */
BOOL WINAPI SetViewportExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetViewportExtEx );
        ret = physdev->funcs->pSetViewportExtEx( physdev, x, y, size );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI SetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetViewportOrgEx );
        ret = physdev->funcs->pSetViewportOrgEx( physdev, x, y, pt );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetWindowExtEx    (GDI32.@)
 */
BOOL WINAPI SetWindowExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetWindowExtEx );
        ret = physdev->funcs->pSetWindowExtEx( physdev, x, y, size );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI SetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetWindowOrgEx );
        ret = physdev->funcs->pSetWindowOrgEx( physdev, x, y, pt );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt)
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pOffsetViewportOrgEx );
        ret = physdev->funcs->pOffsetViewportOrgEx( physdev, x, y, pt );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pOffsetWindowOrgEx );
        ret = physdev->funcs->pOffsetWindowOrgEx( physdev, x, y, pt );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           ScaleViewportExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleViewportExtEx( HDC hdc, INT xNum, INT xDenom,
                                    INT yNum, INT yDenom, LPSIZE size )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pScaleViewportExtEx );
        ret = physdev->funcs->pScaleViewportExtEx( physdev, xNum, xDenom, yNum, yDenom, size );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           ScaleWindowExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleWindowExtEx( HDC hdc, INT xNum, INT xDenom,
                                  INT yNum, INT yDenom, LPSIZE size )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pScaleWindowExtEx );
        ret = physdev->funcs->pScaleWindowExtEx( physdev, xNum, xDenom, yNum, yDenom, size );
        release_dc_ptr( dc );
    }
    return ret;
}


/****************************************************************************
 *           ModifyWorldTransform   (GDI32.@)
 */
BOOL WINAPI ModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    BOOL ret = FALSE;
    DC *dc;

    if (!xform && mode != MWT_IDENTITY) return FALSE;
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pModifyWorldTransform );
        if (dc->GraphicsMode == GM_ADVANCED)
            ret = physdev->funcs->pModifyWorldTransform( physdev, xform, mode );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetWorldTransform    (GDI32.@)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    BOOL ret = FALSE;
    DC *dc;

    if (!xform) return FALSE;
    /* The transform must conform to (eM11 * eM22 != eM12 * eM21) requirement */
    if (xform->eM11 * xform->eM22 == xform->eM12 * xform->eM21) return FALSE;

    TRACE("eM11 %f eM12 %f eM21 %f eM22 %f eDx %f eDy %f\n",
        xform->eM11, xform->eM12, xform->eM21, xform->eM22, xform->eDx, xform->eDy);

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetWorldTransform );
        if (dc->GraphicsMode == GM_ADVANCED)
            ret = physdev->funcs->pSetWorldTransform( physdev, xform );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetVirtualResolution   (GDI32.@)
 *
 * Undocumented on msdn.
 *
 * Changes the values of screen size in pixels and millimeters used by
 * the mapping mode functions.
 *
 * PARAMS
 *     hdc       [I] Device context
 *     horz_res  [I] Width in pixels  (equivalent to HORZRES device cap).
 *     vert_res  [I] Height in pixels (equivalent to VERTRES device cap).
 *     horz_size [I] Width in mm      (equivalent to HORZSIZE device cap).
 *     vert_size [I] Height in mm     (equivalent to VERTSIZE device cap).
 *
 * RETURNS
 *    TRUE if successful.
 *    FALSE if any (but not all) of the last four params are zero.
 *
 * NOTES
 *    This doesn't change the values returned by GetDeviceCaps, just the
 *    scaling of the mapping modes.
 *
 *    Calling with the last four params equal to zero sets the values
 *    back to their defaults obtained by calls to GetDeviceCaps.
 */
BOOL WINAPI SetVirtualResolution(HDC hdc, DWORD horz_res, DWORD vert_res,
                                 DWORD horz_size, DWORD vert_size)
{
    DC * dc;
    TRACE("(%p %d %d %d %d)\n", hdc, horz_res, vert_res, horz_size, vert_size);

    if (!horz_res || !vert_res || !horz_size || !vert_size)
    {
        /* they must be all zero */
        if (horz_res || vert_res || horz_size || vert_size) return FALSE;
    }

    dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    dc->virtual_res.cx  = horz_res;
    dc->virtual_res.cy  = vert_res;
    dc->virtual_size.cx = horz_size;
    dc->virtual_size.cy = vert_size;

    release_dc_ptr( dc );
    return TRUE;
}

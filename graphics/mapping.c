/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "gdi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi);


/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
void MAPPING_FixIsotropic( DC * dc )
{
    double xdim = (double)dc->vportExtX * GetDeviceCaps( dc->hSelf, HORZSIZE ) /
                  (GetDeviceCaps( dc->hSelf, HORZRES ) * dc->wndExtX);
    double ydim = (double)dc->vportExtY * GetDeviceCaps( dc->hSelf, VERTSIZE ) /
                  (GetDeviceCaps( dc->hSelf, VERTRES ) * dc->wndExtY);
    if (xdim > ydim)
    {
	dc->vportExtX = dc->vportExtX * fabs( ydim / xdim );
	if (!dc->vportExtX) dc->vportExtX = 1;
    }
    else
    {
	dc->vportExtY = dc->vportExtY * fabs( xdim / ydim );
	if (!dc->vportExtY) dc->vportExtY = 1;
    }
}


/***********************************************************************
 *           DPtoLP    (GDI.67)
 */
BOOL16 WINAPI DPtoLP16( HDC16 hdc, LPPOINT16 points, INT16 count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	points->x = XDPTOLP( dc, points->x );
	points->y = YDPTOLP( dc, points->y );
        points++;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           DPtoLP    (GDI32.@)
 */
BOOL WINAPI DPtoLP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
        if (!INTERNAL_DPTOLP( dc, points ))
	    break;
        points++;
    }
    GDI_ReleaseObj( hdc );
    return (count < 0);
}


/***********************************************************************
 *           LPtoDP    (GDI.99)
 */
BOOL16 WINAPI LPtoDP16( HDC16 hdc, LPPOINT16 points, INT16 count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	points->x = XLPTODP( dc, points->x );
	points->y = YLPTODP( dc, points->y );
        points++;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           LPtoDP    (GDI32.@)
 */
BOOL WINAPI LPtoDP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	INTERNAL_LPTODP( dc, points );
        points++;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           SetMapMode    (GDI.3)
 */
INT16 WINAPI SetMapMode16( HDC16 hdc, INT16 mode )
{
    return SetMapMode( hdc, mode );
}


/***********************************************************************
 *           SetMapMode    (GDI32.@)
 */
INT WINAPI SetMapMode( HDC hdc, INT mode )
{
    INT prevMode;
    INT horzSize, vertSize, horzRes, vertRes;

    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetMapMode)
    {
        prevMode = dc->funcs->pSetMapMode( dc, mode );
        goto done;
    }

    TRACE("%04x %d\n", hdc, mode );

    prevMode = dc->MapMode;
    horzSize = GetDeviceCaps( hdc, HORZSIZE );
    vertSize = GetDeviceCaps( hdc, VERTSIZE );
    horzRes  = GetDeviceCaps( hdc, HORZRES );
    vertRes  = GetDeviceCaps( hdc, VERTRES );
    switch(mode)
    {
    case MM_TEXT:
        dc->wndExtX   = 1;
        dc->wndExtY   = 1;
        dc->vportExtX = 1;
        dc->vportExtY = 1;
        break;
    case MM_LOMETRIC:
    case MM_ISOTROPIC:
        dc->wndExtX   = horzSize;
        dc->wndExtY   = vertSize;
        dc->vportExtX = horzRes / 10;
        dc->vportExtY = vertRes / -10;
        break;
    case MM_HIMETRIC:
        dc->wndExtX   = horzSize * 10;
        dc->wndExtY   = vertSize * 10;
        dc->vportExtX = horzRes / 10;
        dc->vportExtY = vertRes / -10;
        break;
    case MM_LOENGLISH:
        dc->wndExtX   = horzSize;
        dc->wndExtY   = vertSize;
        dc->vportExtX = 254L * horzRes / 1000;
        dc->vportExtY = -254L * vertRes / 1000;
        break;
    case MM_HIENGLISH:
        dc->wndExtX   = horzSize * 10;
        dc->wndExtY   = vertSize * 10;
        dc->vportExtX = 254L * horzRes / 1000;
        dc->vportExtY = -254L * vertRes / 1000;
        break;
    case MM_TWIPS:
        dc->wndExtX   = 144L * horzSize / 10;
        dc->wndExtY   = 144L * vertSize / 10;
        dc->vportExtX = 254L * horzRes / 1000;
        dc->vportExtY = -254L * vertRes / 1000;
        break;
    case MM_ANISOTROPIC:
        break;
    default:
        goto done;
    }
    dc->MapMode = mode;
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return prevMode;
}


/***********************************************************************
 *           SetViewportExt    (GDI.14)
 */
DWORD WINAPI SetViewportExt16( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE size;
    if (!SetViewportExtEx( hdc, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetViewportExtEx    (GDI.479)
 */
BOOL16 WINAPI SetViewportExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = SetViewportExtEx( hdc, x, y, &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           SetViewportExtEx    (GDI32.@)
 */
BOOL WINAPI SetViewportExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportExt)
    {
        ret = dc->funcs->pSetViewportExt( dc, x, y );
        goto done;
    }
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!x || !y)
    {
        ret = FALSE;
        goto done;
    }
    dc->vportExtX = x;
    dc->vportExtY = y;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetViewportOrg    (GDI.13)
 */
DWORD WINAPI SetViewportOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!SetViewportOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetViewportOrgEx    (GDI.480)
 */
BOOL16 WINAPI SetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = SetViewportOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           SetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI SetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportOrg)
        ret = dc->funcs->pSetViewportOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->vportOrgX;
            pt->y = dc->vportOrgY;
        }
        dc->vportOrgX = x;
        dc->vportOrgY = y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetWindowExt    (GDI.12)
 */
DWORD WINAPI SetWindowExt16( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE size;
    if (!SetWindowExtEx( hdc, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetWindowExtEx    (GDI.481)
 */
BOOL16 WINAPI SetWindowExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = SetWindowExtEx( hdc, x, y, &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           SetWindowExtEx    (GDI32.@)
 */
BOOL WINAPI SetWindowExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowExt)
    {
        ret = dc->funcs->pSetWindowExt( dc, x, y );
        goto done;
    }
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!x || !y)
    {
        ret = FALSE;
        goto done;
    }
    dc->wndExtX = x;
    dc->wndExtY = y;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetWindowOrg    (GDI.11)
 */
DWORD WINAPI SetWindowOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!SetWindowOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI.482)
 */
BOOL16 WINAPI SetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = SetWindowOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI SetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowOrg) ret = dc->funcs->pSetWindowOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->wndOrgX;
            pt->y = dc->wndOrgY;
        }
        dc->wndOrgX = x;
        dc->wndOrgY = y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrg    (GDI.17)
 */
DWORD WINAPI OffsetViewportOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!OffsetViewportOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           OffsetViewportOrgEx    (GDI.476)
 */
BOOL16 WINAPI OffsetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt)
{
    POINT pt32;
    BOOL16 ret = OffsetViewportOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt)
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetViewportOrg)
        ret = dc->funcs->pOffsetViewportOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->vportOrgX;
            pt->y = dc->vportOrgY;
        }
        dc->vportOrgX += x;
        dc->vportOrgY += y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrg    (GDI.15)
 */
DWORD WINAPI OffsetWindowOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!OffsetWindowOrgEx( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           OffsetWindowOrgEx    (GDI.477)
 */
BOOL16 WINAPI OffsetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = OffsetWindowOrgEx( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetWindowOrg)
        ret = dc->funcs->pOffsetWindowOrg( dc, x, y );
    else
    {
        if (pt)
        {
            pt->x = dc->wndOrgX;
            pt->y = dc->wndOrgY;
        }
        dc->wndOrgX += x;
        dc->wndOrgY += y;
        DC_UpdateXforms( dc );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           ScaleViewportExt    (GDI.18)
 */
DWORD WINAPI ScaleViewportExt16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                               INT16 yNum, INT16 yDenom )
{
    SIZE size;
    if (!ScaleViewportExtEx( hdc, xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           ScaleViewportExtEx    (GDI.484)
 */
BOOL16 WINAPI ScaleViewportExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                    INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = ScaleViewportExtEx( hdc, xNum, xDenom, yNum, yDenom,
                                       &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           ScaleViewportExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleViewportExtEx( HDC hdc, INT xNum, INT xDenom,
                                    INT yNum, INT yDenom, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleViewportExt)
    {
        ret = dc->funcs->pScaleViewportExt( dc, xNum, xDenom, yNum, yDenom );
        goto done;
    }
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!xNum || !xDenom || !xNum || !yDenom)
    {
        ret = FALSE;
        goto done;
    }
    dc->vportExtX = (dc->vportExtX * xNum) / xDenom;
    dc->vportExtY = (dc->vportExtY * yNum) / yDenom;
    if (dc->vportExtX == 0) dc->vportExtX = 1;
    if (dc->vportExtY == 0) dc->vportExtY = 1;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           ScaleWindowExt    (GDI.16)
 */
DWORD WINAPI ScaleWindowExt16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                             INT16 yNum, INT16 yDenom )
{
    SIZE size;
    if (!ScaleWindowExtEx( hdc, xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           ScaleWindowExtEx    (GDI.485)
 */
BOOL16 WINAPI ScaleWindowExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                  INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = ScaleWindowExtEx( hdc, xNum, xDenom, yNum, yDenom,
                                     &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           ScaleWindowExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleWindowExtEx( HDC hdc, INT xNum, INT xDenom,
                                  INT yNum, INT yDenom, LPSIZE size )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleWindowExt)
    {
        ret = dc->funcs->pScaleWindowExt( dc, xNum, xDenom, yNum, yDenom );
        goto done;
    }
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!xNum || !xDenom || !xNum || !yDenom)
    {
        ret = FALSE;
        goto done;
    }
    dc->wndExtX = (dc->wndExtX * xNum) / xDenom;
    dc->wndExtY = (dc->wndExtY * yNum) / yDenom;
    if (dc->wndExtX == 0) dc->wndExtX = 1;
    if (dc->wndExtY == 0) dc->wndExtY = 1;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}

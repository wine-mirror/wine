/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <math.h>
#include "dc.h"
#include "debug.h"


/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
void MAPPING_FixIsotropic( DC * dc )
{
    double xdim = (double)dc->vportExtX * dc->w.devCaps->horzSize /
	          (dc->w.devCaps->horzRes * dc->wndExtX);
    double ydim = (double)dc->vportExtY * dc->w.devCaps->vertSize /
	          (dc->w.devCaps->vertRes * dc->wndExtY);
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
 *           DPtoLP16    (GDI.67)
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
    return TRUE;
}


/***********************************************************************
 *           DPtoLP32    (GDI32.65)
 */
BOOL32 WINAPI DPtoLP32( HDC32 hdc, LPPOINT32 points, INT32 count )
{
    FLOAT determinant=1.0, x, y;
    
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    if (dc->w.UseWorldXform)
    {
        determinant = dc->w.WorldXform.eM11*dc->w.WorldXform.eM22 -
            dc->w.WorldXform.eM12*dc->w.WorldXform.eM21;
        if (determinant > -1e-12 && determinant < 1e-12)
            return FALSE;
    }

    while (count--)
    {
	if (dc->w.UseWorldXform)
	{
            x = (FLOAT)XDPTOLP( dc, points->x ) - dc->w.WorldXform.eDx;
	    y = (FLOAT)YDPTOLP( dc, points->y ) - dc->w.WorldXform.eDy;
	    points->x = (INT32)( (x*dc->w.WorldXform.eM22 -
	       y*dc->w.WorldXform.eM21) / determinant );
	    points->y = (INT32)( (-x*dc->w.WorldXform.eM12 +
	       y*dc->w.WorldXform.eM11) / determinant );
	}
	else
	{
	    points->x = XDPTOLP( dc, points->x );
	    points->y = YDPTOLP( dc, points->y );
	}
        points++;
    }
    return TRUE;
}


/***********************************************************************
 *           LPtoDP16    (GDI.99)
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
    return TRUE;
}


/***********************************************************************
 *           LPtoDP32    (GDI32.247)
 */
BOOL32 WINAPI LPtoDP32( HDC32 hdc, LPPOINT32 points, INT32 count )
{
    FLOAT x, y;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    while (count--)
    {
	if (dc->w.UseWorldXform)
	{
	    x = (FLOAT)points->x * dc->w.WorldXform.eM11 +
	    	(FLOAT)points->y * dc->w.WorldXform.eM21 +
		dc->w.WorldXform.eDx;
	    y = (FLOAT)points->x * dc->w.WorldXform.eM12 +
	        (FLOAT)points->y * dc->w.WorldXform.eM22 +
		dc->w.WorldXform.eDy;
	    points->x = XLPTODP( dc, (INT32)x );
	    points->y = YLPTODP( dc, (INT32)y );
	    
	}
	else
	{
	    points->x = XLPTODP( dc, points->x );
	    points->y = YLPTODP( dc, points->y );
	}
        points++;
    }
    return TRUE;
}


/***********************************************************************
 *           SetMapMode16    (GDI.3)
 */
INT16 WINAPI SetMapMode16( HDC16 hdc, INT16 mode )
{
    return SetMapMode32( hdc, mode );
}


/***********************************************************************
 *           SetMapMode32    (GDI32.321)
 */
INT32 WINAPI SetMapMode32( HDC32 hdc, INT32 mode )
{
    INT32 prevMode;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetMapMode) return dc->funcs->pSetMapMode( dc, mode );

    TRACE(gdi, "%04x %d\n", hdc, mode );
    
    prevMode = dc->w.MapMode;
    switch(mode)
    {
      case MM_TEXT:
	  dc->wndOrgX   = dc->wndOrgY   = 0;
	  dc->vportOrgX = dc->vportOrgY = 0;
	  dc->wndExtX   = 1;
	  dc->wndExtY   = 1;
	  dc->vportExtX = 1;
	  dc->vportExtY = 1;
	  break;
	  
      case MM_LOMETRIC:
      case MM_ISOTROPIC:
	  dc->wndOrgX   = dc->wndOrgY   = 0;
	  dc->vportOrgX = dc->vportOrgY = 0;
	  dc->wndExtX   = dc->w.devCaps->horzSize;
	  dc->wndExtY   = dc->w.devCaps->vertSize;
	  dc->vportExtX = dc->w.devCaps->horzRes / 10;
	  dc->vportExtY = dc->w.devCaps->vertRes / -10;
	  break;
	  
      case MM_HIMETRIC:
	  dc->wndOrgX   = dc->wndOrgY   = 0;
	  dc->vportOrgX = dc->vportOrgY = 0;
	  dc->wndExtX   = dc->w.devCaps->horzSize * 10;
	  dc->wndExtY   = dc->w.devCaps->vertSize * 10;
	  dc->vportExtX = dc->w.devCaps->horzRes / 10;
	  dc->vportExtY = dc->w.devCaps->vertRes / -10;
	  break;
	  
      case MM_LOENGLISH:
	  dc->wndOrgX   = dc->wndOrgY   = 0;
	  dc->vportOrgX = dc->vportOrgY = 0;
	  dc->wndExtX   = dc->w.devCaps->horzSize;
	  dc->wndExtY   = dc->w.devCaps->vertSize;
	  dc->vportExtX = 254L * dc->w.devCaps->horzRes / 1000;
	  dc->vportExtY = -254L * dc->w.devCaps->vertRes / 1000;
	  break;	  
	  
      case MM_HIENGLISH:
	  dc->wndOrgX   = dc->wndOrgY   = 0;
	  dc->vportOrgX = dc->vportOrgY = 0;
	  dc->wndExtX   = dc->w.devCaps->horzSize * 10;
	  dc->wndExtY   = dc->w.devCaps->vertSize * 10;
	  dc->vportExtX = 254L * dc->w.devCaps->horzRes / 1000;
	  dc->vportExtY = -254L * dc->w.devCaps->vertRes / 1000;
	  break;
	  
      case MM_TWIPS:
	  dc->wndOrgX   = dc->wndOrgY   = 0;
	  dc->vportOrgX = dc->vportOrgY = 0;
	  dc->wndExtX   = 144L * dc->w.devCaps->horzSize / 10;
	  dc->wndExtY   = 144L * dc->w.devCaps->vertSize / 10;
	  dc->vportExtX = 254L * dc->w.devCaps->horzRes / 1000;
	  dc->vportExtY = -254L * dc->w.devCaps->vertRes / 1000;
	  break;
	  
      case MM_ANISOTROPIC:
	  break;

      default:
	  return prevMode;
    }
    dc->w.MapMode = mode;
    return prevMode;
}


/***********************************************************************
 *           SetViewportExt    (GDI.14)
 */
DWORD WINAPI SetViewportExt( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE32 size;
    if (!SetViewportExtEx32( hdc, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetViewportExtEx16    (GDI.479)
 */
BOOL16 WINAPI SetViewportExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE32 size32;
    BOOL16 ret = SetViewportExtEx32( hdc, x, y, &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           SetViewportExtEx32    (GDI32.340)
 */
BOOL32 WINAPI SetViewportExtEx32( HDC32 hdc, INT32 x, INT32 y, LPSIZE32 size )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportExt)
        return dc->funcs->pSetViewportExt( dc, x, y );
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!x || !y) return FALSE;
    dc->vportExtX = x;
    dc->vportExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           SetViewportOrg    (GDI.13)
 */
DWORD WINAPI SetViewportOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT32 pt;
    if (!SetViewportOrgEx32( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetViewportOrgEx16    (GDI.480)
 */
BOOL16 WINAPI SetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT32 pt32;
    BOOL16 ret = SetViewportOrgEx32( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           SetViewportOrgEx32    (GDI32.341)
 */
BOOL32 WINAPI SetViewportOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportOrg)
        return dc->funcs->pSetViewportOrg( dc, x, y );
    if (pt)
    {
	pt->x = dc->vportOrgX;
	pt->y = dc->vportOrgY;
    }
    dc->vportOrgX = x;
    dc->vportOrgY = y;
    return TRUE;
}


/***********************************************************************
 *           SetWindowExt    (GDI.12)
 */
DWORD WINAPI SetWindowExt( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE32 size;
    if (!SetWindowExtEx32( hdc, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetWindowExtEx16    (GDI.481)
 */
BOOL16 WINAPI SetWindowExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE32 size32;
    BOOL16 ret = SetWindowExtEx32( hdc, x, y, &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           SetWindowExtEx32    (GDI32.344)
 */
BOOL32 WINAPI SetWindowExtEx32( HDC32 hdc, INT32 x, INT32 y, LPSIZE32 size )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowExt) return dc->funcs->pSetWindowExt( dc, x, y );
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!x || !y) return FALSE;
    dc->wndExtX = x;
    dc->wndExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           SetWindowOrg    (GDI.11)
 */
DWORD WINAPI SetWindowOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT32 pt;
    if (!SetWindowOrgEx32( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetWindowOrgEx16    (GDI.482)
 */
BOOL16 WINAPI SetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT32 pt32;
    BOOL16 ret = SetWindowOrgEx32( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           SetWindowOrgEx32    (GDI32.345)
 */
BOOL32 WINAPI SetWindowOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowOrg) return dc->funcs->pSetWindowOrg( dc, x, y );
    if (pt)
    {
	pt->x = dc->wndOrgX;
	pt->y = dc->wndOrgY;
    }
    dc->wndOrgX = x;
    dc->wndOrgY = y;
    return TRUE;
}


/***********************************************************************
 *           OffsetViewportOrg    (GDI.17)
 */
DWORD WINAPI OffsetViewportOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT32 pt;
    if (!OffsetViewportOrgEx32( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           OffsetViewportOrgEx16    (GDI.476)
 */
BOOL16 WINAPI OffsetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt)
{
    POINT32 pt32;
    BOOL16 ret = OffsetViewportOrgEx32( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrgEx32    (GDI32.257)
 */
BOOL32 WINAPI OffsetViewportOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt)
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetViewportOrg)
        return dc->funcs->pOffsetViewportOrg( dc, x, y );
    if (pt)
    {
	pt->x = dc->vportOrgX;
	pt->y = dc->vportOrgY;
    }
    dc->vportOrgX += x;
    dc->vportOrgY += y;
    return TRUE;
}


/***********************************************************************
 *           OffsetWindowOrg    (GDI.15)
 */
DWORD WINAPI OffsetWindowOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT32 pt;
    if (!OffsetWindowOrgEx32( hdc, x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           OffsetWindowOrgEx16    (GDI.477)
 */
BOOL16 WINAPI OffsetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT32 pt32;
    BOOL16 ret = OffsetWindowOrgEx32( hdc, x, y, &pt32 );
    if (pt) CONV_POINT32TO16( &pt32, pt );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrgEx32    (GDI32.258)
 */
BOOL32 WINAPI OffsetWindowOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetWindowOrg)
        return dc->funcs->pOffsetWindowOrg( dc, x, y );
    if (pt)
    {
	pt->x = dc->wndOrgX;
	pt->y = dc->wndOrgY;
    }
    dc->wndOrgX += x;
    dc->wndOrgY += y;
    return TRUE;
}


/***********************************************************************
 *           ScaleViewportExt    (GDI.18)
 */
DWORD WINAPI ScaleViewportExt( HDC16 hdc, INT16 xNum, INT16 xDenom,
                               INT16 yNum, INT16 yDenom )
{
    SIZE32 size;
    if (!ScaleViewportExtEx32( hdc, xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           ScaleViewportExtEx16    (GDI.484)
 */
BOOL16 WINAPI ScaleViewportExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                    INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE32 size32;
    BOOL16 ret = ScaleViewportExtEx32( hdc, xNum, xDenom, yNum, yDenom,
                                       &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           ScaleViewportExtEx32    (GDI32.293)
 */
BOOL32 WINAPI ScaleViewportExtEx32( HDC32 hdc, INT32 xNum, INT32 xDenom,
                                    INT32 yNum, INT32 yDenom, LPSIZE32 size )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleViewportExt)
        return dc->funcs->pScaleViewportExt( dc, xNum, xDenom, yNum, yDenom );
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!xNum || !xDenom || !xNum || !yDenom) return FALSE;
    dc->vportExtX = (dc->vportExtX * xNum) / xDenom;
    dc->vportExtY = (dc->vportExtY * yNum) / yDenom;
    if (dc->vportExtX == 0) dc->vportExtX = 1;
    if (dc->vportExtY == 0) dc->vportExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           ScaleWindowExt    (GDI.16)
 */
DWORD WINAPI ScaleWindowExt( HDC16 hdc, INT16 xNum, INT16 xDenom,
                             INT16 yNum, INT16 yDenom )
{
    SIZE32 size;
    if (!ScaleWindowExtEx32( hdc, xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           ScaleWindowExtEx16    (GDI.485)
 */
BOOL16 WINAPI ScaleWindowExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                  INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE32 size32;
    BOOL16 ret = ScaleWindowExtEx32( hdc, xNum, xDenom, yNum, yDenom,
                                     &size32 );
    if (size) CONV_SIZE32TO16( &size32, size );
    return ret;
}


/***********************************************************************
 *           ScaleWindowExtEx32    (GDI32.294)
 */
BOOL32 WINAPI ScaleWindowExtEx32( HDC32 hdc, INT32 xNum, INT32 xDenom,
                                  INT32 yNum, INT32 yDenom, LPSIZE32 size )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleWindowExt)
        return dc->funcs->pScaleWindowExt( dc, xNum, xDenom, yNum, yDenom );
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!xNum || !xDenom || !xNum || !yDenom) return FALSE;
    dc->wndExtX = (dc->wndExtX * xNum) / xDenom;
    dc->wndExtY = (dc->wndExtY * yNum) / yDenom;
    if (dc->wndExtX == 0) dc->wndExtX = 1;
    if (dc->wndExtY == 0) dc->wndExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}

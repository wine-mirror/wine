/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <math.h>
#include "gdi.h"
#include "metafile.h"
#include "stddebug.h"
/* #define DEBUG_GDI */
#include "debug.h"


/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
void MAPPING_FixIsotropic( DC * dc )
{
    double xdim = (double)dc->w.VportExtX * dc->w.devCaps->horzSize /
	          (dc->w.devCaps->horzRes * dc->w.WndExtX);
    double ydim = (double)dc->w.VportExtY * dc->w.devCaps->vertSize /
	          (dc->w.devCaps->vertRes * dc->w.WndExtY);
    if (xdim > ydim)
    {
	dc->w.VportExtX = dc->w.VportExtX * fabs( ydim / xdim );
	if (!dc->w.VportExtX) dc->w.VportExtX = 1;
    }
    else
    {
	dc->w.VportExtY = dc->w.VportExtY * fabs( xdim / ydim );
	if (!dc->w.VportExtY) dc->w.VportExtY = 1;
    }	
}


/***********************************************************************
 *           DPtoLP16    (GDI.67)
 */
BOOL16 DPtoLP16( HDC16 hdc, LPPOINT16 points, INT16 count )
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
BOOL32 DPtoLP32( HDC32 hdc, LPPOINT32 points, INT32 count )
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
 *           LPtoDP16    (GDI.99)
 */
BOOL16 LPtoDP16( HDC16 hdc, LPPOINT16 points, INT16 count )
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
BOOL32 LPtoDP32( HDC32 hdc, LPPOINT32 points, INT32 count )
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
 *           SetMapMode    (GDI.3)
 */
WORD SetMapMode( HDC16 hdc, WORD mode )
{
    WORD prevMode;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
	MF_MetaParam1(dc, META_SETMAPMODE, mode);
	return 1;
    }

    dprintf_gdi(stddeb, "SetMapMode: %04x %d\n", hdc, mode );
    
    prevMode = dc->w.MapMode;
    switch(mode)
    {
      case MM_TEXT:
	  dc->w.WndOrgX   = dc->w.WndOrgY   = 0;
	  dc->w.VportOrgX = dc->w.VportOrgY = 0;
	  dc->w.WndExtX   = 1;
	  dc->w.WndExtY   = 1;
	  dc->w.VportExtX = 1;
	  dc->w.VportExtY = 1;
	  break;
	  
      case MM_LOMETRIC:
      case MM_ISOTROPIC:
	  dc->w.WndOrgX   = dc->w.WndOrgY   = 0;
	  dc->w.VportOrgX = dc->w.VportOrgY = 0;
	  dc->w.WndExtX   = dc->w.devCaps->horzSize;
	  dc->w.WndExtY   = dc->w.devCaps->vertSize;
	  dc->w.VportExtX = dc->w.devCaps->horzRes / 10;
	  dc->w.VportExtY = dc->w.devCaps->vertRes / -10;
	  break;
	  
      case MM_HIMETRIC:
	  dc->w.WndOrgX   = dc->w.WndOrgY   = 0;
	  dc->w.VportOrgX = dc->w.VportOrgY = 0;
	  dc->w.WndExtX   = dc->w.devCaps->horzSize * 10;
	  dc->w.WndExtY   = dc->w.devCaps->vertSize * 10;
	  dc->w.VportExtX = dc->w.devCaps->horzRes / 10;
	  dc->w.VportExtY = dc->w.devCaps->vertRes / -10;
	  break;
	  
      case MM_LOENGLISH:
	  dc->w.WndOrgX   = dc->w.WndOrgY   = 0;
	  dc->w.VportOrgX = dc->w.VportOrgY = 0;
	  dc->w.WndExtX   = dc->w.devCaps->horzSize;
	  dc->w.WndExtY   = dc->w.devCaps->vertSize;
	  dc->w.VportExtX = (short)(254L * dc->w.devCaps->horzRes / 1000);
	  dc->w.VportExtY = (short)(-254L * dc->w.devCaps->vertRes / 1000);
	  break;	  
	  
      case MM_HIENGLISH:
	  dc->w.WndOrgX   = dc->w.WndOrgY   = 0;
	  dc->w.VportOrgX = dc->w.VportOrgY = 0;
	  dc->w.WndExtX   = dc->w.devCaps->horzSize * 10;
	  dc->w.WndExtY   = dc->w.devCaps->vertSize * 10;
	  dc->w.VportExtX = (short)(254L * dc->w.devCaps->horzRes / 1000);
	  dc->w.VportExtY = (short)(-254L * dc->w.devCaps->vertRes / 1000);
	  break;
	  
      case MM_TWIPS:
	  dc->w.WndOrgX   = dc->w.WndOrgY   = 0;
	  dc->w.VportOrgX = dc->w.VportOrgY = 0;
	  dc->w.WndExtX   = (short)(144L * dc->w.devCaps->horzSize / 10);
	  dc->w.WndExtY   = (short)(144L * dc->w.devCaps->vertSize / 10);
	  dc->w.VportExtX = (short)(254L * dc->w.devCaps->horzRes / 1000);
	  dc->w.VportExtY = (short)(-254L * dc->w.devCaps->vertRes / 1000);
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
DWORD SetViewportExt( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE16 size;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_SETVIEWPORTEXT, x, y);
	return 0;
    }

    size.cx = dc->w.VportExtX;
    size.cy = dc->w.VportExtY;
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return MAKELONG( size.cx, size.cy );
    if (!x || !y) return 0;
    dc->w.VportExtX = x;
    dc->w.VportExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetViewportExtEx16    (GDI.479)
 */
BOOL16 SetViewportExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (size)
    {
	size->cx = dc->w.VportExtX;
	size->cy = dc->w.VportExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!x || !y) return FALSE;
    dc->w.VportExtX = x;
    dc->w.VportExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           SetViewportExtEx32    (GDI32.340)
 */
BOOL32 SetViewportExtEx32( HDC32 hdc, INT32 x, INT32 y, LPSIZE32 size )
{
    SIZE16 size16;
    BOOL16 ret = SetViewportExtEx16( (HDC16)hdc, (INT16)x, (INT16)y, &size16 );
    if (size) CONV_SIZE16TO32( &size16, size );
    return ret;
}


/***********************************************************************
 *           SetViewportOrg    (GDI.13)
 */
DWORD SetViewportOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT16 pt;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_SETVIEWPORTORG, x, y);
	return 0;
    }

    pt.x = dc->w.VportOrgX;
    pt.y = dc->w.VportOrgY;
    dc->w.VportOrgX = x;
    dc->w.VportOrgY = y;
    return pt.x | (pt.y << 16);
}


/***********************************************************************
 *           SetViewportOrgEx16    (GDI.480)
 */
BOOL16 SetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (pt)
    {
	pt->x = dc->w.VportOrgX;
	pt->y = dc->w.VportOrgY;
    }
    dc->w.VportOrgX = x;
    dc->w.VportOrgY = y;
    return TRUE;
}


/***********************************************************************
 *           SetViewportOrgEx32    (GDI32.341)
 */
BOOL32 SetViewportOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    POINT16 pt16;
    BOOL16 ret = SetViewportOrgEx16( (HDC16)hdc, (INT16)x, (INT16)y, &pt16 );
    if (pt) CONV_POINT16TO32( &pt16, pt );
    return ret;
}


/***********************************************************************
 *           SetWindowExt    (GDI.12)
 */
DWORD SetWindowExt( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE16 size;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_SETWINDOWEXT, x, y);
	return 0;
    }

    size.cx = dc->w.WndExtX;
    size.cy = dc->w.WndExtY;
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return MAKELONG( size.cx, size.cy );
    if (!x || !y) return 0;
    dc->w.WndExtX = x;
    dc->w.WndExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetWindowExtEx16    (GDI.481)
 */
BOOL16 SetWindowExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (size)
    {
	size->cx = dc->w.WndExtX;
	size->cy = dc->w.WndExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!x || !y) return FALSE;
    dc->w.WndExtX = x;
    dc->w.WndExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           SetWindowExtEx32    (GDI32.344)
 */
BOOL32 SetWindowExtEx32( HDC32 hdc, INT32 x, INT32 y, LPSIZE32 size )
{
    SIZE16 size16;
    BOOL16 ret = SetWindowExtEx16( (HDC16)hdc, (INT16)x, (INT16)y, &size16 );
    if (size) CONV_SIZE16TO32( &size16, size );
    return ret;
}


/***********************************************************************
 *           SetWindowOrg    (GDI.11)
 */
DWORD SetWindowOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT16 pt;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_SETWINDOWORG, x, y);
	return 0;
    }

    pt.x = dc->w.WndOrgX;
    pt.y = dc->w.WndOrgY;
    dc->w.WndOrgX = x;
    dc->w.WndOrgY = y;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetWindowOrgEx16    (GDI.482)
 */
BOOL16 SetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (pt)
    {
	pt->x = dc->w.WndOrgX;
	pt->y = dc->w.WndOrgY;
    }
    dc->w.WndOrgX = x;
    dc->w.WndOrgY = y;
    return TRUE;
}


/***********************************************************************
 *           SetWindowOrgEx32    (GDI32.345)
 */
BOOL32 SetWindowOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    POINT16 pt16;
    BOOL16 ret = SetWindowOrgEx16( (HDC16)hdc, (INT16)x, (INT16)y, &pt16 );
    if (pt) CONV_POINT16TO32( &pt16, pt );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrg    (GDI.17)
 */
DWORD OffsetViewportOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT16 pt;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_OFFSETVIEWPORTORG, x, y);
	return 0;
    }

    pt.x = dc->w.VportOrgX;
    pt.y = dc->w.VportOrgY;
    dc->w.VportOrgX += x;
    dc->w.VportOrgY += y;
    return pt.x | (pt.y << 16);
}


/***********************************************************************
 *           OffsetViewportOrgEx16    (GDI.476)
 */
BOOL16 OffsetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (pt)
    {
	pt->x = dc->w.VportOrgX;
	pt->y = dc->w.VportOrgY;
    }
    dc->w.VportOrgX += x;
    dc->w.VportOrgY += y;
    return TRUE;
}


/***********************************************************************
 *           OffsetViewportOrgEx32    (GDI32.257)
 */
BOOL32 OffsetViewportOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    POINT16 pt16;
    BOOL16 ret = OffsetViewportOrgEx16( (HDC16)hdc, (INT16)x, (INT16)y, &pt16);
    if (pt) CONV_POINT16TO32( &pt16, pt );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrg    (GDI.15)
 */
DWORD OffsetWindowOrg( HDC16 hdc, INT16 x, INT16 y )
{
    POINT16 pt;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam2(dc, META_OFFSETWINDOWORG, x, y);
	return 0;
    }

    pt.x = dc->w.WndOrgX;
    pt.y = dc->w.WndOrgY;
    dc->w.WndOrgX += x;
    dc->w.WndOrgY += y;
    return pt.x | (pt.y << 16);
}


/***********************************************************************
 *           OffsetWindowOrgEx16    (GDI.477)
 */
BOOL16 OffsetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (pt)
    {
	pt->x = dc->w.WndOrgX;
	pt->y = dc->w.WndOrgY;
    }
    dc->w.WndOrgX += x;
    dc->w.WndOrgY += y;
    return TRUE;
}


/***********************************************************************
 *           OffsetWindowOrgEx32    (GDI32.258)
 */
BOOL32 OffsetWindowOrgEx32( HDC32 hdc, INT32 x, INT32 y, LPPOINT32 pt )
{
    POINT16 pt16;
    BOOL16 ret = OffsetWindowOrgEx16( (HDC16)hdc, (INT16)x, (INT16)y, &pt16 );
    if (pt) CONV_POINT16TO32( &pt16, pt );
    return ret;
}


/***********************************************************************
 *           ScaleViewportExt    (GDI.18)
 */
DWORD ScaleViewportExt( HDC16 hdc, INT16 xNum, INT16 xDenom,
                        INT16 yNum, INT16 yDenom )
{
    SIZE16 size;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam4(dc, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom);
	return 0;
    }

    size.cx = dc->w.VportExtX;
    size.cy = dc->w.VportExtY;
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return size.cx | (size.cy << 16);
    if (!xNum || !xDenom || !xNum || !yDenom) return 0;
    dc->w.VportExtX = (dc->w.VportExtX * xNum) / xDenom;
    dc->w.VportExtY = (dc->w.VportExtY * yNum) / yDenom;
    if (dc->w.VportExtX == 0) dc->w.VportExtX = 1;
    if (dc->w.VportExtY == 0) dc->w.VportExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return size.cx | (size.cy << 16);
}


/***********************************************************************
 *           ScaleViewportExtEx16    (GDI.484)
 */
BOOL16 ScaleViewportExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                             INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (size)
    {
	size->cx = dc->w.VportExtX;
	size->cy = dc->w.VportExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!xNum || !xDenom || !xNum || !yDenom) return FALSE;
    dc->w.VportExtX = (dc->w.VportExtX * xNum) / xDenom;
    dc->w.VportExtY = (dc->w.VportExtY * yNum) / yDenom;
    if (dc->w.VportExtX == 0) dc->w.VportExtX = 1;
    if (dc->w.VportExtY == 0) dc->w.VportExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           ScaleViewportExtEx32    (GDI32.293)
 */
BOOL32 ScaleViewportExtEx32( HDC32 hdc, INT32 xNum, INT32 xDenom,
                             INT32 yNum, INT32 yDenom, LPSIZE32 size )
{
    SIZE16 size16;
    BOOL16 ret = ScaleViewportExtEx16( (HDC16)hdc, (INT16)xNum, (INT16)xDenom,
                                       (INT16)yNum, (INT16)yDenom, &size16 );
    if (size) CONV_SIZE16TO32( &size16, size );
    return ret;
}


/***********************************************************************
 *           ScaleWindowExt    (GDI.16)
 */
DWORD ScaleWindowExt( HDC16 hdc, INT16 xNum, INT16 xDenom,
		      INT16 yNum, INT16 yDenom )
{
    SIZE16 size;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam4(dc, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom);
	return 0;
    }

    size.cx = dc->w.WndExtX;
    size.cy = dc->w.WndExtY;
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return size.cx | (size.cy << 16);
    if (!xNum || !xDenom || !xNum || !yDenom) return FALSE;
    dc->w.WndExtX = (dc->w.WndExtX * xNum) / xDenom;
    dc->w.WndExtY = (dc->w.WndExtY * yNum) / yDenom;
    if (dc->w.WndExtX == 0) dc->w.WndExtX = 1;
    if (dc->w.WndExtY == 0) dc->w.WndExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return size.cx | (size.cy << 16);
}


/***********************************************************************
 *           ScaleWindowExtEx16    (GDI.485)
 */
BOOL16 ScaleWindowExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                           INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    if (size)
    {
	size->cx = dc->w.WndExtX;
	size->cy = dc->w.WndExtY;
    }
    if ((dc->w.MapMode != MM_ISOTROPIC) && (dc->w.MapMode != MM_ANISOTROPIC))
	return TRUE;
    if (!xNum || !xDenom || !xNum || !yDenom) return FALSE;
    dc->w.WndExtX = (dc->w.WndExtX * xNum) / xDenom;
    dc->w.WndExtY = (dc->w.WndExtY * yNum) / yDenom;
    if (dc->w.WndExtX == 0) dc->w.WndExtX = 1;
    if (dc->w.WndExtY == 0) dc->w.WndExtY = 1;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return TRUE;
}


/***********************************************************************
 *           ScaleWindowExtEx32    (GDI32.294)
 */
BOOL32 ScaleWindowExtEx32( HDC32 hdc, INT32 xNum, INT32 xDenom,
                           INT32 yNum, INT32 yDenom, LPSIZE32 size )
{
    SIZE16 size16;
    BOOL16 ret = ScaleWindowExtEx16( (HDC16)hdc, (INT16)xNum, (INT16)xDenom,
                                     (INT16)yNum, (INT16)yDenom, &size16 );
    if (size) CONV_SIZE16TO32( &size16, size );
    return ret;
}

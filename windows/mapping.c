/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "gdi.h"
#include "metafile.h"


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
	dc->w.VportExtX = dc->w.VportExtX * ydim / xdim;
	if (!dc->w.VportExtX) dc->w.VportExtX = 1;
    }
    else
    {
	dc->w.VportExtY = dc->w.VportExtY * xdim / ydim;
	if (!dc->w.VportExtY) dc->w.VportExtY = 1;
    }	
}

/***********************************************************************
 *           DPtoLP    (GDI.67)
 */
BOOL DPtoLP( HDC hdc, LPPOINT points, int count )
{
    POINT * pt;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    
    for (pt = points; count > 0; pt++, count--)
    {
	pt->x = XDPTOLP( dc, pt->x );
	pt->y = YDPTOLP( dc, pt->y );
    }
    return TRUE;
}


/***********************************************************************
 *           LPtoDP    (GDI.99)
 */
BOOL LPtoDP( HDC hdc, LPPOINT points, int count )
{
    POINT * pt;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    
    for (pt = points; count > 0; pt++, count--)
    {
	pt->x = XLPTODP( dc, pt->x );
	pt->y = YLPTODP( dc, pt->y );
    }
    return TRUE;
}


/***********************************************************************
 *           SetMapMode    (GDI.3)
 */
WORD SetMapMode( HDC hdc, WORD mode )
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

#ifdef DEBUG_GDI
    printf( "SetMapMode: %d %d\n", hdc, mode );
#endif
    
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
DWORD SetViewportExt( HDC hdc, short x, short y )
{
    SIZE size;
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
	return size.cx | (size.cy << 16);
    if (!x || !y) return 0;
    dc->w.VportExtX = x;
    dc->w.VportExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return size.cx | (size.cy << 16);
}


/***********************************************************************
 *           SetViewportExtEx    (GDI.479)
 */
BOOL SetViewportExtEx( HDC hdc, short x, short y, LPSIZE size )
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
 *           SetViewportOrg    (GDI.13)
 */
DWORD SetViewportOrg( HDC hdc, short x, short y )
{
    POINT pt;
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
 *           SetViewportOrgEx    (GDI.480)
 */
BOOL SetViewportOrgEx( HDC hdc, short x, short y, LPPOINT pt )
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
 *           SetWindowExt    (GDI.12)
 */
DWORD SetWindowExt( HDC hdc, short x, short y )
{
    SIZE size;
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
	return size.cx | (size.cy << 16);
    if (!x || !y) return 0;
    dc->w.WndExtX = x;
    dc->w.WndExtY = y;
    if (dc->w.MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    return size.cx | (size.cy << 16);
}


/***********************************************************************
 *           SetWindowExtEx    (GDI.481)
 */
BOOL SetWindowExtEx( HDC hdc, short x, short y, LPSIZE size )
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
 *           SetWindowOrg    (GDI.11)
 */
DWORD SetWindowOrg( HDC hdc, short x, short y )
{
    POINT pt;
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
    return pt.x | (pt.y << 16);
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI.482)
 */
BOOL SetWindowOrgEx( HDC hdc, short x, short y, LPPOINT pt )
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
 *           OffsetViewportOrg    (GDI.17)
 */
DWORD OffsetViewportOrg( HDC hdc, short x, short y )
{
    POINT pt;
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
 *           OffsetViewportOrgEx    (GDI.476)
 */
BOOL OffsetViewportOrgEx( HDC hdc, short x, short y, LPPOINT pt )
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
 *           OffsetWindowOrg    (GDI.15)
 */
DWORD OffsetWindowOrg( HDC hdc, short x, short y )
{
    POINT pt;
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
 *           OffsetWindowOrgEx    (GDI.477)
 */
BOOL OffsetWindowOrgEx( HDC hdc, short x, short y, LPPOINT pt )
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
 *           ScaleViewportExt    (GDI.18)
 */
DWORD ScaleViewportExt( HDC hdc, short xNum, short xDenom,
		      short yNum, short yDenom )
{
    SIZE size;
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
 *           ScaleViewportExtEx    (GDI.484)
 */
BOOL ScaleViewportExtEx( HDC hdc, short xNum, short xDenom,
			 short yNum, short yDenom, LPSIZE size )
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
 *           ScaleWindowExt    (GDI.16)
 */
DWORD ScaleWindowExt( HDC hdc, short xNum, short xDenom,
		      short yNum, short yDenom )
{
    SIZE size;
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
 *           ScaleWindowExtEx    (GDI.485)
 */
BOOL ScaleWindowExtEx( HDC hdc, short xNum, short xDenom,
		       short yNum, short yDenom, LPSIZE size )
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

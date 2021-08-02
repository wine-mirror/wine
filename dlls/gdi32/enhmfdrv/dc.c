/*
 * Enhanced MetaFile driver dc value functions
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 2016 Alexandre Julliard
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

#include <assert.h>
#include "enhmfdrv/enhmetafiledrv.h"

BOOL EMFDC_SaveDC( DC_ATTR *dc_attr )
{
    EMRSAVEDC emr;
    emr.emr.iType = EMR_SAVEDC;
    emr.emr.nSize = sizeof(emr);
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL CDECL EMFDRV_RestoreDC( PHYSDEV dev, INT level )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pRestoreDC );
    EMFDRV_PDEVICE* physDev = get_emf_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    EMRRESTOREDC emr;
    BOOL ret;

    emr.emr.iType = EMR_RESTOREDC;
    emr.emr.nSize = sizeof(emr);

    if (level < 0)
        emr.iRelative = level;
    else
        emr.iRelative = level - dc->saveLevel - 1;

    physDev->restoring++;
    ret = next->funcs->pRestoreDC( next, level );
    physDev->restoring--;

    if (ret) EMFDRV_WriteRecord( dev, &emr.emr );
    return ret;
}

BOOL EMFDC_SetTextAlign( DC_ATTR *dc_attr, UINT align )
{
    EMRSETTEXTALIGN emr;
    emr.emr.iType = EMR_SETTEXTALIGN;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = align;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL CDECL EMFDRV_SetTextJustification(PHYSDEV dev, INT nBreakExtra, INT nBreakCount)
{
    EMRSETTEXTJUSTIFICATION emr;
    emr.emr.iType = EMR_SETTEXTJUSTIFICATION;
    emr.emr.nSize = sizeof(emr);
    emr.nBreakExtra = nBreakExtra;
    emr.nBreakCount = nBreakCount;
    return EMFDRV_WriteRecord(dev, &emr.emr);
}

BOOL EMFDC_SetBkMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETBKMODE emr;
    emr.emr.iType = EMR_SETBKMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

COLORREF CDECL EMFDRV_SetBkColor( PHYSDEV dev, COLORREF color )
{
    EMRSETBKCOLOR emr;
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    if (physDev->restoring) return color;  /* don't output records during RestoreDC */

    emr.emr.iType = EMR_SETBKCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? color : CLR_INVALID;
}


COLORREF CDECL EMFDRV_SetTextColor( PHYSDEV dev, COLORREF color )
{
    EMRSETTEXTCOLOR emr;
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    if (physDev->restoring) return color;  /* don't output records during RestoreDC */

    emr.emr.iType = EMR_SETTEXTCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? color : CLR_INVALID;
}

BOOL EMFDC_SetROP2( DC_ATTR *dc_attr, INT rop )
{
    EMRSETROP2 emr;
    emr.emr.iType = EMR_SETROP2;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = rop;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetPolyFillMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETPOLYFILLMODE emr;
    emr.emr.iType = EMR_SETPOLYFILLMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetStretchBltMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETSTRETCHBLTMODE emr;
    emr.emr.iType = EMR_SETSTRETCHBLTMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetArcDirection( DC_ATTR *dc_attr, INT dir )
{
    EMRSETARCDIRECTION emr;

    emr.emr.iType = EMR_SETARCDIRECTION;
    emr.emr.nSize = sizeof(emr);
    emr.iArcDirection = dir;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

INT CDECL EMFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pExcludeClipRect );
    EMREXCLUDECLIPRECT emr;

    emr.emr.iType      = EMR_EXCLUDECLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return ERROR;
    return next->funcs->pExcludeClipRect( next, left, top, right, bottom );
}

INT CDECL EMFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pIntersectClipRect );
    EMRINTERSECTCLIPRECT emr;

    emr.emr.iType      = EMR_INTERSECTCLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return ERROR;
    return next->funcs->pIntersectClipRect( next, left, top, right, bottom );
}

INT CDECL EMFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetClipRgn );
    EMROFFSETCLIPRGN emr;

    emr.emr.iType   = EMR_OFFSETCLIPRGN;
    emr.emr.nSize   = sizeof(emr);
    emr.ptlOffset.x = x;
    emr.ptlOffset.y = y;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return ERROR;
    return next->funcs->pOffsetClipRgn( next, x, y );
}

INT CDECL EMFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pExtSelectClipRgn );
    EMREXTSELECTCLIPRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;

    if (!hrgn)
    {
        if (mode != RGN_COPY) return ERROR;
        rgnsize = 0;
    }
    else rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );

    size = rgnsize + offsetof(EMREXTSELECTCLIPRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );
    if (rgnsize) NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_EXTSELECTCLIPRGN;
    emr->emr.nSize = size;
    emr->cbRgnData = rgnsize;
    emr->iMode     = mode;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret ? next->funcs->pExtSelectClipRgn( next, hrgn, mode ) : ERROR;
}

INT CDECL EMFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetMapMode );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETMAPMODE emr;
    INT ret;

    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    physDev->modifying_transform++;
    ret = next->funcs->pSetMapMode( next, mode );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_SetViewportExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetViewportExtEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETVIEWPORTEXTEX emr;
    BOOL ret;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pSetViewportExtEx( next, cx, cy, size );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_SetWindowExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWindowExtEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETWINDOWEXTEX emr;
    BOOL ret;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pSetWindowExtEx( next, cx, cy, size );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetViewportOrgEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETVIEWPORTORGEX emr;
    BOOL ret;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pSetViewportOrgEx( next, x, y, pt );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWindowOrgEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETWINDOWORGEX emr;
    BOOL ret;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pSetWindowOrgEx( next, x, y, pt );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pScaleViewportExtEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSCALEVIEWPORTEXTEX emr;
    BOOL ret;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pScaleViewportExtEx( next, xNum, xDenom, yNum, yDenom, size );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pScaleWindowExtEx );
    EMRSCALEWINDOWEXTEX emr;

    emr.emr.iType = EMR_SCALEWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pScaleWindowExtEx( next, xNum, xDenom, yNum, yDenom, size );
}

DWORD CDECL EMFDRV_SetLayout( PHYSDEV dev, DWORD layout )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetLayout );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETLAYOUT emr;
    DWORD ret;

    emr.emr.iType = EMR_SETLAYOUT;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = layout;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return GDI_ERROR;
    physDev->modifying_transform++;
    ret = next->funcs->pSetLayout( next, layout );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_SetWorldTransform( PHYSDEV dev, const XFORM *xform)
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWorldTransform );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETWORLDTRANSFORM emr;
    BOOL ret;

    emr.emr.iType = EMR_SETWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pSetWorldTransform( next, xform );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode)
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pModifyWorldTransform );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRMODIFYWORLDTRANSFORM emr;
    BOOL ret;

    emr.emr.iType = EMR_MODIFYWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    if (mode == MWT_IDENTITY)
    {
        emr.xform.eM11 = 1.0f;
        emr.xform.eM12 = 0.0f;
        emr.xform.eM21 = 0.0f;
        emr.xform.eM22 = 1.0f;
        emr.xform.eDx = 0.0f;
        emr.xform.eDy = 0.0f;
    }
    else
    {
        emr.xform = *xform;
    }
    emr.iMode = mode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pModifyWorldTransform( next, xform, mode );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetViewportOrgEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETVIEWPORTORGEX emr;
    POINT prev;
    BOOL ret;

    GetViewportOrgEx( dev->hdc, &prev );

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = prev.x + x;
    emr.ptlOrigin.y = prev.y + y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pOffsetViewportOrgEx( next, x, y, pt );
    physDev->modifying_transform--;
    return ret;
}

BOOL CDECL EMFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetWindowOrgEx );
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRSETWINDOWORGEX emr;
    POINT prev;
    BOOL ret;

    GetWindowOrgEx( dev->hdc, &prev );

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = prev.x + x;
    emr.ptlOrigin.y = prev.y + y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    physDev->modifying_transform++;
    ret = next->funcs->pOffsetWindowOrgEx( next, x, y, pt );
    physDev->modifying_transform--;
    return ret;
}

DWORD CDECL EMFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags )
{
    EMRSETMAPPERFLAGS emr;

    emr.emr.iType = EMR_SETMAPPERFLAGS;
    emr.emr.nSize = sizeof(emr);
    emr.dwFlags   = flags;

    return EMFDRV_WriteRecord( dev, &emr.emr ) ? flags : GDI_ERROR;
}

BOOL EMFDC_AbortPath( DC_ATTR *dc_attr )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRABORTPATH emr;

    emr.emr.iType = EMR_ABORTPATH;
    emr.emr.nSize = sizeof(emr);

    emf->path = FALSE;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_BeginPath( DC_ATTR *dc_attr )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRBEGINPATH emr;

    emr.emr.iType = EMR_BEGINPATH;
    emr.emr.nSize = sizeof(emr);

    if (!EMFDRV_WriteRecord( &emf->dev, &emr.emr )) return FALSE;
    emf->path = TRUE;
    return TRUE;
}

BOOL EMFDC_CloseFigure( DC_ATTR *dc_attr )
{
    EMRCLOSEFIGURE emr;

    emr.emr.iType = EMR_CLOSEFIGURE;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_EndPath( DC_ATTR *dc_attr )
{
    EMFDRV_PDEVICE *emf = dc_attr->emf;
    EMRENDPATH emr;

    emr.emr.iType = EMR_ENDPATH;
    emr.emr.nSize = sizeof(emr);

    emf->path = FALSE;
    return EMFDRV_WriteRecord( &emf->dev, &emr.emr );
}

BOOL CDECL EMFDRV_FlattenPath( PHYSDEV dev )
{
    EMRFLATTENPATH emr;

    emr.emr.iType = EMR_FLATTENPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL CDECL EMFDRV_SelectClipPath( PHYSDEV dev, INT iMode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectClipPath );
    EMRSELECTCLIPPATH emr;
    BOOL ret = FALSE;
    HRGN hrgn;

    emr.emr.iType = EMR_SELECTCLIPPATH;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = iMode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    hrgn = PathToRegion( dev->hdc );
    if (hrgn)
    {
        ret = next->funcs->pExtSelectClipRgn( next, hrgn, iMode );
        DeleteObject( hrgn );
    }
    return ret;
}

BOOL CDECL EMFDRV_WidenPath( PHYSDEV dev )
{
    EMRWIDENPATH emr;

    emr.emr.iType = EMR_WIDENPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT CDECL EMFDRV_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    if (cap >= 0 && cap < ARRAY_SIZE( physDev->dev_caps ))
        return physDev->dev_caps[cap];
    return 0;
}

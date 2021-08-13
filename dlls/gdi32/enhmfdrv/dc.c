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

BOOL EMFDC_SetTextJustification( DC_ATTR *dc_attr, INT extra, INT breaks )
{
    EMRSETTEXTJUSTIFICATION emr;
    emr.emr.iType = EMR_SETTEXTJUSTIFICATION;
    emr.emr.nSize = sizeof(emr);
    emr.nBreakExtra = extra;
    emr.nBreakCount = breaks;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
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

INT EMFDC_ExcludeClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom )
{
    EMREXCLUDECLIPRECT emr;

    emr.emr.iType      = EMR_EXCLUDECLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_IntersectClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom)
{
    EMRINTERSECTCLIPRECT emr;

    emr.emr.iType      = EMR_INTERSECTCLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_OffsetClipRgn( DC_ATTR *dc_attr, INT x, INT y )
{
    EMROFFSETCLIPRGN emr;

    emr.emr.iType   = EMR_OFFSETCLIPRGN;
    emr.emr.nSize   = sizeof(emr);
    emr.ptlOffset.x = x;
    emr.ptlOffset.y = y;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_ExtSelectClipRgn( DC_ATTR *dc_attr, HRGN hrgn, INT mode )
{
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

    ret = EMFDRV_WriteRecord( dc_attr->emf, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_SetMapMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETMAPMODE emr;

    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetViewportExtEx( DC_ATTR *dc_attr, INT cx, INT cy )
{
    EMRSETVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetWindowExtEx( DC_ATTR *dc_attr, INT cx, INT cy )
{
    EMRSETWINDOWEXTEX emr;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetViewportOrgEx( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRSETVIEWPORTORGEX emr;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetWindowOrgEx( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRSETWINDOWORGEX emr;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_ScaleViewportExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    EMRSCALEVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = x_num;
    emr.xDenom    = x_denom;
    emr.yNum      = y_num;
    emr.yDenom    = y_denom;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_ScaleWindowExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    EMRSCALEWINDOWEXTEX emr;

    emr.emr.iType = EMR_SCALEWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = x_num;
    emr.xDenom    = x_denom;
    emr.yNum      = y_num;
    emr.yDenom    = y_denom;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetLayout( DC_ATTR *dc_attr, DWORD layout )
{
    EMRSETLAYOUT emr;

    emr.emr.iType = EMR_SETLAYOUT;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = layout;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetWorldTransform( DC_ATTR *dc_attr, const XFORM *xform )
{
    EMRSETWORLDTRANSFORM emr;

    emr.emr.iType = EMR_SETWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_ModifyWorldTransform( DC_ATTR *dc_attr, const XFORM *xform, DWORD mode )
{
    EMRMODIFYWORLDTRANSFORM emr;

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
    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
}

BOOL EMFDC_SetMapperFlags( DC_ATTR *dc_attr, DWORD flags )
{
    EMRSETMAPPERFLAGS emr;

    emr.emr.iType = EMR_SETMAPPERFLAGS;
    emr.emr.nSize = sizeof(emr);
    emr.dwFlags   = flags;

    return EMFDRV_WriteRecord( dc_attr->emf, &emr.emr );
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
        ret = NtGdiExtSelectClipRgn( dev->hdc, hrgn, iMode );
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

/*
 * Enhanced MetaFile driver mapping functions
 *
 * Copyright 1999 Huw D M Davies
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

#include "enhmfdrv/enhmetafiledrv.h"

INT CDECL EMFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetMapMode );
    EMRSETMAPMODE emr;
    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pSetMapMode( next, mode );
}

BOOL CDECL EMFDRV_SetViewportExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetViewportExtEx );
    EMRSETVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pSetViewportExtEx( next, cx, cy, size );
}

BOOL CDECL EMFDRV_SetWindowExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWindowExtEx );
    EMRSETWINDOWEXTEX emr;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pSetWindowExtEx( next, cx, cy, size );
}

BOOL CDECL EMFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetViewportOrgEx );
    EMRSETVIEWPORTORGEX emr;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pSetViewportOrgEx( next, x, y, pt );
}

BOOL CDECL EMFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWindowOrgEx );
    EMRSETWINDOWORGEX emr;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pSetWindowOrgEx( next, x, y, pt );
}

BOOL CDECL EMFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pScaleViewportExtEx );
    EMRSCALEVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pScaleViewportExtEx( next, xNum, xDenom, yNum, yDenom, size );
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

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pScaleWindowExtEx( next, xNum, xDenom, yNum, yDenom, size );
}

BOOL CDECL EMFDRV_SetWorldTransform( PHYSDEV dev, const XFORM *xform)
{
    EMRSETWORLDTRANSFORM emr;

    emr.emr.iType = EMR_SETWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL CDECL EMFDRV_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode)
{
    EMRMODIFYWORLDTRANSFORM emr;

    emr.emr.iType = EMR_MODIFYWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;
    emr.iMode = mode;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL CDECL EMFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetViewportOrgEx );
    EMRSETVIEWPORTORGEX emr;
    EMFDRV_PDEVICE* physDev = (EMFDRV_PDEVICE*)dev;

    GetViewportOrgEx(physDev->hdc, pt);

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = pt->x + x;
    emr.ptlOrigin.y = pt->y + y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pOffsetViewportOrgEx( next, x, y, pt );
}

BOOL CDECL EMFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetWindowOrgEx );
    EMRSETWINDOWORGEX emr;
    EMFDRV_PDEVICE* physDev = (EMFDRV_PDEVICE*)dev;

    GetWindowOrgEx(physDev->hdc, pt);

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = pt->x + x;
    emr.ptlOrigin.y = pt->y + y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pOffsetWindowOrgEx( next, x, y, pt );
}

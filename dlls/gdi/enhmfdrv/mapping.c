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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "enhmfdrv/enhmetafiledrv.h"

INT EMFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    EMRSETMAPMODE emr;
    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_SetViewportExt( PHYSDEV dev, INT cx, INT cy )
{
    EMRSETVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_SetWindowExt( PHYSDEV dev, INT cx, INT cy )
{
    EMRSETWINDOWEXTEX emr;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_SetViewportOrg( PHYSDEV dev, INT x, INT y )
{
    EMRSETVIEWPORTORGEX emr;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_SetWindowOrg( PHYSDEV dev, INT x, INT y )
{
    EMRSETWINDOWORGEX emr;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_ScaleViewportExt( PHYSDEV dev, INT xNum, INT xDenom, INT yNum,
			     INT yDenom )
{
    EMRSCALEVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_ScaleWindowExt( PHYSDEV dev, INT xNum, INT xDenom, INT yNum,
			   INT yDenom )
{
    EMRSCALEWINDOWEXTEX emr;

    emr.emr.iType = EMR_SCALEWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL EMFDRV_SetWorldTransform( PHYSDEV dev, const XFORM *xform)
{
    EMRSETWORLDTRANSFORM emr;

    emr.emr.iType = EMR_SETWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL EMFDRV_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, INT mode)
{
    EMRMODIFYWORLDTRANSFORM emr;

    emr.emr.iType = EMR_MODIFYWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;
    emr.iMode = mode;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

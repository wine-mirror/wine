/*
 * Enhanced MetaFile driver mapping functions
 *
 * Copyright 1999 Huw D M Davies
 */
#include "enhmetafiledrv.h"

BOOL EMFDRV_SetViewportExt( DC *dc, INT cx, INT cy )
{
    EMRSETVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_SetWindowExt( DC *dc, INT cx, INT cy )
{
    EMRSETWINDOWEXTEX emr;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_SetViewportOrg( DC *dc, INT x, INT y )
{
    EMRSETVIEWPORTORGEX emr;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_SetWindowOrg( DC *dc, INT x, INT y )
{
    EMRSETWINDOWORGEX emr;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_ScaleViewportExt( DC *dc, INT xNum, INT xDenom, INT yNum,
			      INT yDenom )
{
    EMRSCALEVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_ScaleWindowExt( DC *dc, INT xNum, INT xDenom, INT yNum,
			    INT yDenom )
{
    EMRSCALEWINDOWEXTEX emr;

    emr.emr.iType = EMR_SCALEWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}



/*
 * Metafile GDI mapping mode functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "gdi.h"
#include "metafile.h"
#include "metafiledrv.h"


/***********************************************************************
 *           MFDRV_SetMapMode
 */
INT MFDRV_SetMapMode( DC *dc, INT mode )
{
    INT prevMode = dc->w.MapMode;
    MF_MetaParam1( dc, META_SETMAPMODE, mode );
    return prevMode;
}


/***********************************************************************
 *           MFDRV_SetViewportExt
 */
BOOL MFDRV_SetViewportExt( DC *dc, INT x, INT y )
{
    MF_MetaParam2( dc, META_SETVIEWPORTEXT, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetViewportOrg
 */
BOOL MFDRV_SetViewportOrg( DC *dc, INT x, INT y )
{
    MF_MetaParam2( dc, META_SETVIEWPORTORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetWindowExt
 */
BOOL MFDRV_SetWindowExt( DC *dc, INT x, INT y )
{
    MF_MetaParam2( dc, META_SETWINDOWEXT, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetWindowOrg
 */
BOOL MFDRV_SetWindowOrg( DC *dc, INT x, INT y )
{
    MF_MetaParam2( dc, META_SETWINDOWORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_OffsetViewportOrg
 */
BOOL MFDRV_OffsetViewportOrg( DC *dc, INT x, INT y )
{
    MF_MetaParam2( dc, META_OFFSETVIEWPORTORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_OffsetWindowOrg
 */
BOOL MFDRV_OffsetWindowOrg( DC *dc, INT x, INT y )
{
    MF_MetaParam2( dc, META_OFFSETWINDOWORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_ScaleViewportExt
 */
BOOL MFDRV_ScaleViewportExt( DC *dc, INT xNum, INT xDenom,
                               INT yNum, INT yDenom )
{
    MF_MetaParam4( dc, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_ScaleWindowExt
 */
BOOL MFDRV_ScaleWindowExt( DC *dc, INT xNum, INT xDenom,
                             INT yNum, INT yDenom )
{
    MF_MetaParam4( dc, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom );
    return TRUE;
}

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
INT32 MFDRV_SetMapMode( DC *dc, INT32 mode )
{
    INT32 prevMode = dc->w.MapMode;
    MF_MetaParam1( dc, META_SETMAPMODE, mode );
    return prevMode;
}


/***********************************************************************
 *           MFDRV_SetViewportExt
 */
BOOL32 MFDRV_SetViewportExt( DC *dc, INT32 x, INT32 y )
{
    MF_MetaParam2( dc, META_SETVIEWPORTEXT, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetViewportOrg
 */
BOOL32 MFDRV_SetViewportOrg( DC *dc, INT32 x, INT32 y )
{
    MF_MetaParam2( dc, META_SETVIEWPORTORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetWindowExt
 */
BOOL32 MFDRV_SetWindowExt( DC *dc, INT32 x, INT32 y )
{
    MF_MetaParam2( dc, META_SETWINDOWEXT, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetWindowOrg
 */
BOOL32 MFDRV_SetWindowOrg( DC *dc, INT32 x, INT32 y )
{
    MF_MetaParam2( dc, META_SETWINDOWORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_OffsetViewportOrg
 */
BOOL32 MFDRV_OffsetViewportOrg( DC *dc, INT32 x, INT32 y )
{
    MF_MetaParam2( dc, META_OFFSETVIEWPORTORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_OffsetWindowOrg
 */
BOOL32 MFDRV_OffsetWindowOrg( DC *dc, INT32 x, INT32 y )
{
    MF_MetaParam2( dc, META_OFFSETWINDOWORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_ScaleViewportExt
 */
BOOL32 MFDRV_ScaleViewportExt( DC *dc, INT32 xNum, INT32 xDenom,
                               INT32 yNum, INT32 yDenom )
{
    MF_MetaParam4( dc, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_ScaleWindowExt
 */
BOOL32 MFDRV_ScaleWindowExt( DC *dc, INT32 xNum, INT32 xDenom,
                             INT32 yNum, INT32 yDenom )
{
    MF_MetaParam4( dc, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom );
    return TRUE;
}

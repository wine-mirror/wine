/*
 * Metafile GDI mapping mode functions
 *
 * Copyright 1996 Alexandre Julliard
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

#include "gdi.h"
#include "metafiledrv.h"


/***********************************************************************
 *           MFDRV_SetMapMode
 */
INT MFDRV_SetMapMode( DC *dc, INT mode )
{
    INT prevMode = dc->MapMode;
    MFDRV_MetaParam1( dc, META_SETMAPMODE, mode );
    return prevMode;
}


/***********************************************************************
 *           MFDRV_SetViewportExt
 */
BOOL MFDRV_SetViewportExt( DC *dc, INT x, INT y )
{
    MFDRV_MetaParam2( dc, META_SETVIEWPORTEXT, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetViewportOrg
 */
BOOL MFDRV_SetViewportOrg( DC *dc, INT x, INT y )
{
    MFDRV_MetaParam2( dc, META_SETVIEWPORTORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetWindowExt
 */
BOOL MFDRV_SetWindowExt( DC *dc, INT x, INT y )
{
    MFDRV_MetaParam2( dc, META_SETWINDOWEXT, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_SetWindowOrg
 */
BOOL MFDRV_SetWindowOrg( DC *dc, INT x, INT y )
{
    MFDRV_MetaParam2( dc, META_SETWINDOWORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_OffsetViewportOrg
 */
BOOL MFDRV_OffsetViewportOrg( DC *dc, INT x, INT y )
{
    MFDRV_MetaParam2( dc, META_OFFSETVIEWPORTORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_OffsetWindowOrg
 */
BOOL MFDRV_OffsetWindowOrg( DC *dc, INT x, INT y )
{
    MFDRV_MetaParam2( dc, META_OFFSETWINDOWORG, x, y );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_ScaleViewportExt
 */
BOOL MFDRV_ScaleViewportExt( DC *dc, INT xNum, INT xDenom,
                               INT yNum, INT yDenom )
{
    MFDRV_MetaParam4( dc, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_ScaleWindowExt
 */
BOOL MFDRV_ScaleWindowExt( DC *dc, INT xNum, INT xDenom,
                             INT yNum, INT yDenom )
{
    MFDRV_MetaParam4( dc, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom );
    return TRUE;
}


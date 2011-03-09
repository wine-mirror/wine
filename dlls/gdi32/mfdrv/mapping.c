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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "mfdrv/metafiledrv.h"


/***********************************************************************
 *           MFDRV_SetMapMode
 */
INT CDECL MFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETMAPMODE, mode );
}


/***********************************************************************
 *           MFDRV_SetViewportExtEx
 */
BOOL CDECL MFDRV_SetViewportExtEx( PHYSDEV dev, INT x, INT y, SIZE *size )
{
    return MFDRV_MetaParam2( dev, META_SETVIEWPORTEXT, x, y );
}


/***********************************************************************
 *           MFDRV_SetViewportOrgEx
 */
BOOL CDECL MFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_SETVIEWPORTORG, x, y );
}


/***********************************************************************
 *           MFDRV_SetWindowExtEx
 */
BOOL CDECL MFDRV_SetWindowExtEx( PHYSDEV dev, INT x, INT y, SIZE *size )
{
    return MFDRV_MetaParam2( dev, META_SETWINDOWEXT, x, y );
}


/***********************************************************************
 *           MFDRV_SetWindowOrgEx
 */
BOOL CDECL MFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_SETWINDOWORG, x, y );
}


/***********************************************************************
 *           MFDRV_OffsetViewportOrgEx
 */
BOOL CDECL MFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_OFFSETVIEWPORTORG, x, y );
}


/***********************************************************************
 *           MFDRV_OffsetWindowOrgEx
 */
BOOL CDECL MFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_OFFSETWINDOWORG, x, y );
}


/***********************************************************************
 *           MFDRV_ScaleViewportExtEx
 */
BOOL CDECL MFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    return MFDRV_MetaParam4( dev, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom );
}


/***********************************************************************
 *           MFDRV_ScaleWindowExtEx
 */
BOOL CDECL MFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    return MFDRV_MetaParam4( dev, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom );
}

/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "win16drv/win16drv.h"
#include "wownt32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


/***********************************************************************
 *           WIN16DRV_SelectBitmap
 */
HBITMAP WIN16DRV_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    FIXME("BITMAP not implemented\n");
    return (HBITMAP)1;
}


/***********************************************************************
 *           WIN16DRV_SelectBrush
 */
HBRUSH WIN16DRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    int          nSize;
    LOGBRUSH16 lBrush16;

    if (!GetObject16( HBRUSH_16(hbrush), sizeof(lBrush16), &lBrush16 )) return 0;

    if ( physDev->BrushInfo )
    {
        TRACE("UnRealizing BrushInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_BRUSH,
                                      physDev->BrushInfo,
                                      physDev->BrushInfo, 0);
    }
    else
    {
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_BRUSH, &lBrush16, 0, 0);
        physDev->BrushInfo = HeapAlloc( GetProcessHeap(), 0, nSize );
    }

    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                 &lBrush16, physDev->BrushInfo, win16drv_SegPtr_TextXForm);
    return hbrush;
}


/***********************************************************************
 *           WIN16DRV_SelectPen
 */
HPEN WIN16DRV_SelectPen( PHYSDEV dev, HPEN hpen )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    int          nSize;
    LOGPEN16     lPen16;

    if (!GetObject16( HPEN_16(hpen), sizeof(lPen16), &lPen16 )) return 0;

    if ( physDev->PenInfo )
    {
        TRACE("UnRealizing PenInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_PEN,
                                      physDev->PenInfo,
                                      physDev->PenInfo, 0);
    }
    else
    {
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_PEN, &lPen16, 0, 0);
        physDev->PenInfo = HeapAlloc( GetProcessHeap(), 0, nSize );
    }

    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_PEN,
                                 &lPen16, physDev->PenInfo, 0);
    return hpen;
}

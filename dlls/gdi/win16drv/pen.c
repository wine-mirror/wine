/*
 * GDI pen objects
 *
 * Copyright 1997 John Harvey
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

#include "win16drv/win16drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16drv);

/***********************************************************************
 *           PEN_SelectObject
 */
HPEN WIN16DRV_PEN_SelectObject( DC * dc, HPEN hpen )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HPEN prevHandle = dc->hPen;
    int		 nSize;
    LOGPEN16 	 lPen16;

    if (!GetObject16( hpen, sizeof(lPen16), &lPen16 )) return 0;

    dc->hPen = hpen;

    if ( physDev->PenInfo )
    {
        TRACE("UnRealizing PenInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_PEN,
				      physDev->PenInfo,
				      physDev->PenInfo, 0);
    }
    else 
    {
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_PEN,
                                  &lPen16, 0, 0); 
	physDev->PenInfo = HeapAlloc( GetProcessHeap(), 0, nSize );
    }

    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_PEN,
                                 &lPen16, 
                                 physDev->PenInfo, 
                                 0); 

    return prevHandle;
}


/*
 * GDI brush objects - win16drv
 *
 * Copyright 1997  John Harvey
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

#include <stdlib.h>
#include "win16drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16drv);

HBRUSH WIN16DRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HBRUSH16	 prevHandle = dc->hBrush;
    int		 nSize;
    LOGBRUSH16 lBrush16;

    if (!GetObject16( hbrush, sizeof(lBrush16), &lBrush16 )) return 0;

    dc->hBrush = hbrush;
    if ( physDev->BrushInfo )
    {
        TRACE("UnRealizing BrushInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_BRUSH,
				      physDev->BrushInfo,
				      physDev->BrushInfo, 0);
    }
    else 
    {
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                  &lBrush16, 0, 0); 
	physDev->BrushInfo = HeapAlloc( GetProcessHeap(), 0, nSize );
    }


    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                 &lBrush16, 
                                 physDev->BrushInfo, 
                                 win16drv_SegPtr_TextXForm); 
                         
    return prevHandle;
}

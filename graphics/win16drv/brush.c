/*
 * GDI brush objects - win16drv
 *
 * Copyright 1997  John Harvey
 */

#include <stdlib.h>
#include "win16drv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win16drv);

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

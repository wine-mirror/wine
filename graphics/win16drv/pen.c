/*
 * GDI pen objects
 *
 * Copyright 1997 John Harvey
 */

#include "win16drv.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win16drv);

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
	physDev->PenInfo = SEGPTR_ALLOC( nSize );
    }

    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_PEN,
                                 &lPen16, 
                                 physDev->PenInfo, 
                                 0); 

    return prevHandle;
}


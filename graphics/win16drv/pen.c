/*
 * GDI pen objects
 *
 * Copyright 1997 John Harvey
 */

#include "pen.h"
#include "win16drv.h"
#include "heap.h"
#include "debug.h"

/***********************************************************************
 *           PEN_SelectObject
 */
HPEN WIN16DRV_PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HPEN prevHandle = dc->w.hPen;
    int		 nSize;
    LOGPEN16 	 lPen16;
    dc->w.hPen = hpen;
    TRACE(win16drv, "In WIN16DRV_PEN_SelectObject\n");
    lPen16.lopnStyle   = pen->logpen.lopnStyle;
    lPen16.lopnWidth.x = pen->logpen.lopnWidth.x;
    lPen16.lopnWidth.y = pen->logpen.lopnWidth.y;
    lPen16.lopnColor   = pen->logpen.lopnColor;

    if ( physDev->PenInfo )
    {
        TRACE(win16drv, "UnRealizing PenInfo\n");
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


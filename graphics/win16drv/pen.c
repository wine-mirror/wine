/*
 * GDI pen objects
 *
 * Copyright 1997 John Harvey
 */

#include "pen.h"
#include "color.h"
#include "win16drv.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           PEN_SelectObject
 */
HPEN32 WIN16DRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HPEN32 prevHandle = dc->w.hPen;
    int		 nSize;
    LOGPEN16 	 lPen16;
    dc->w.hPen = hpen;
    printf("In WIN16DRV_PEN_SelectObject\n");
    lPen16.lopnStyle   = pen->logpen.lopnStyle;
    lPen16.lopnWidth.x = pen->logpen.lopnWidth.x;
    lPen16.lopnWidth.y = pen->logpen.lopnWidth.y;
    lPen16.lopnColor   = pen->logpen.lopnColor;
    nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_PEN,
                                  &lPen16, NULL, 
                                  0); 
    /*  may need to realloc segptrFOntInfo*/
    physDev->segptrPenInfo = WIN16_GlobalLock16(GlobalAlloc16(GHND, nSize));
    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_PEN,
                                 &lPen16, 
                                 (LPVOID)physDev->segptrPenInfo, 
                                 0); 
                         

    return prevHandle;
}


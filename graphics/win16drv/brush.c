/*
 * GDI brush objects - win16drv
 *
 * Copyright 1997  John Harvey
 */

#include <stdlib.h>
#include "brush.h"
#include "win16drv.h"
#include "stddebug.h"
#include "debug.h"

HBRUSH32 WIN16DRV_BRUSH_SelectObject( DC * dc, HBRUSH32 hbrush,
                                      BRUSHOBJ * brush )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HBRUSH16	 prevHandle = dc->w.hBrush;
    int		 nSize;
    LOGBRUSH16 	 lBrush16;
    dc->w.hBrush = hbrush;
    lBrush16.lbStyle = brush->logbrush.lbStyle;
    lBrush16.lbColor = brush->logbrush.lbColor;
    lBrush16.lbHatch = brush->logbrush.lbHatch;
    nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                  &lBrush16, NULL, 
                                  0); 
    /*  may need to realloc segptrFOntInfo*/
    physDev->segptrBrushInfo = WIN16_GlobalLock16(GlobalAlloc16(GHND, nSize));
    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                 &lBrush16, 
                                 (LPVOID)physDev->segptrBrushInfo, 
                                 win16drv_SegPtr_TextXForm); 
                         
    return prevHandle;
}

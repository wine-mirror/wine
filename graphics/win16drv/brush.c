/*
 * GDI brush objects - win16drv
 *
 * Copyright 1997  John Harvey
 */

#include <stdlib.h>
#include "brush.h"
#include "win16drv.h"
#include "heap.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(win16drv)

HBRUSH WIN16DRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush,
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


    if ( physDev->BrushInfo )
    {
        TRACE(win16drv, "UnRealizing BrushInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_BRUSH,
				      physDev->BrushInfo,
				      physDev->BrushInfo, 0);
    }
    else 
    {
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                  &lBrush16, 0, 0); 
	physDev->BrushInfo = SEGPTR_ALLOC( nSize );
    }


    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_BRUSH,
                                 &lBrush16, 
                                 physDev->BrushInfo, 
                                 win16drv_SegPtr_TextXForm); 
                         
    return prevHandle;
}

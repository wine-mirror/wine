/*
 * X11DRV clipping functions
 *
 * Copyright 1998 Huw Davies
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"

#include <stdio.h>
#include "dc.h"
#include "x11drv.h"
#include "region.h"
#include "debugtools.h"
#include "heap.h"
#include "local.h"

DEFAULT_DEBUG_CHANNEL(x11drv)

/***********************************************************************
 *           X11DRV_SetDeviceClipping
 *           Copy RECT32s to a temporary buffer of XRectangles and call
 *           TSXSetClipRectangles().
 *
 *           Could write using GetRegionData but this would be slower.
 */
void X11DRV_SetDeviceClipping( DC * dc )
{
    XRectangle *pXrect;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
    if (!obj)
    {
        ERR("Rgn is 0. Please report this.\n");
	return;
    }
    
    if (obj->rgn->numRects > 0)
    {
        XRectangle *pXr;
        RECT *pRect = obj->rgn->rects;
        RECT *pEndRect = obj->rgn->rects + obj->rgn->numRects;

        pXrect = HeapAlloc( GetProcessHeap(), 0, 
			    sizeof(*pXrect) * obj->rgn->numRects );
	if(!pXrect)
	{
	    WARN("Can't alloc buffer\n");
	    GDI_HEAP_UNLOCK( dc->w.hGCClipRgn );
	    return;
	}

        for(pXr = pXrect; pRect < pEndRect; pRect++, pXr++)
        {
            pXr->x = pRect->left;
            pXr->y = pRect->top;
            pXr->width = pRect->right - pRect->left;
            pXr->height = pRect->bottom - pRect->top;
        }
    }
    else
        pXrect = NULL;

    TSXSetClipRectangles( display, physDev->gc, 0, 0,
                          pXrect, obj->rgn->numRects, YXBanded );

    if(pXrect)
        HeapFree( GetProcessHeap(), 0, pXrect );

    GDI_HEAP_UNLOCK( dc->w.hGCClipRgn );
}

#endif /* !defined(X_DISPLAY_MISSING) */


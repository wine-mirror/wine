/*
 * X11DRV clipping functions
 *
 * Copyright 1998 Huw Davies
 */

#include <stdio.h>
#include "dc.h"
#include "x11drv.h"
#include "region.h"
#include "stddebug.h"
#include "debug.h"
#include "heap.h"

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
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
    if (!obj)
    {
        fprintf( stderr, "X11DRV_SetDeviceClipping: Rgn is 0. Please report this.\n");
        exit(1);
    }
    
    if (obj->rgn->numRects > 0)
    {
        XRectangle *pXr;
        RECT32 *pRect = obj->rgn->rects;
        RECT32 *pEndRect = obj->rgn->rects + obj->rgn->numRects;

        pXrect = HeapAlloc( GetProcessHeap(), 0, 
			    sizeof(*pXrect) * obj->rgn->numRects );
	if(!pXrect)
	{
	    fprintf(stderr, "X11DRV_SetDeviceClipping() can't alloc buffer\n");
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

    TSXSetClipRectangles( display, dc->u.x.gc, dc->w.DCOrgX, dc->w.DCOrgY, 
                pXrect, obj->rgn->numRects, YXBanded );

    if(pXrect)
        HeapFree( GetProcessHeap(), 0, pXrect );

    GDI_HEAP_UNLOCK( dc->w.hGCClipRgn );
}

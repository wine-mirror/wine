/*
 * DC clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdio.h>
#include "dc.h"
#include "x11drv.h"
#include "region.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           X11DRV_SetDeviceClipping
 */
void X11DRV_SetDeviceClipping( DC * dc )
{
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
    if (!obj)
    {
        fprintf( stderr, "X11DRV_SetDeviceClipping: Rgn is 0. Please report this.\n");
        exit(1);
    }
    if (obj->xrgn)
    {
        XSetRegion( display, dc->u.x.gc, obj->xrgn );
        XSetClipOrigin( display, dc->u.x.gc, dc->w.DCOrgX, dc->w.DCOrgY );
    }
    else  /* Clip everything */
    {
        XSetClipRectangles( display, dc->u.x.gc, 0, 0, NULL, 0, 0 );
    }
    GDI_HEAP_UNLOCK( dc->w.hGCClipRgn );
}

/*
 * X11DRV clipping functions
 *
 * Copyright 1998 Huw Davies
 */

#include "config.h"

#include "ts_xlib.h"

#include <stdio.h>

#include "gdi.h"
#include "x11drv.h"
#include "region.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

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
    
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->hGCClipRgn, REGION_MAGIC);
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
	    GDI_ReleaseObj( dc->hGCClipRgn );
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

    TSXSetClipRectangles( gdi_display, physDev->gc, 0, 0,
                          pXrect, obj->rgn->numRects, YXBanded );

    if(pXrect)
        HeapFree( GetProcessHeap(), 0, pXrect );

    GDI_ReleaseObj( dc->hGCClipRgn );
}


/***********************************************************************
 *           X11DRV_SetDrawable
 *
 * Set the drawable, clipping mode and origin for a DC.
 */
void X11DRV_SetDrawable( HDC hdc, Drawable drawable, int mode, int org_x, int org_y )
{
    DC *dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        X11DRV_PDEVICE *physDev = dc->physDev;
        /*
         * This function change the coordinate system (DCOrgX,DCOrgY)
         * values. When it moves the origin, other data like the current clipping
         * region will not be moved to that new origin. In the case of DCs that are class
         * or window DCs that clipping region might be a valid value from a previous use
         * of the DC and changing the origin of the DC without moving the clip region
         * results in a clip region that is not placed properly in the DC.
         * This code will save the dc origin, let the SetDrawable
         * modify the origin and reset the clipping. When the clipping is set,
         * it is moved according to the new DC origin.
         */
        if (dc->hClipRgn) OffsetRgn( dc->hClipRgn, org_x - dc->DCOrgX, org_y - dc->DCOrgY );
        dc->DCOrgX = org_x;
        dc->DCOrgY = org_y;
        physDev->drawable = drawable;
        TSXSetSubwindowMode( gdi_display, physDev->gc, mode );
	if(physDev->xrender)
	  X11DRV_XRender_UpdateDrawable(dc);
        GDI_ReleaseObj( hdc );
    }
}


/***********************************************************************
 *           X11DRV_StartGraphicsExposures
 *
 * Set the DC in graphics exposures mode
 */
void X11DRV_StartGraphicsExposures( HDC hdc )
{
    DC *dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        X11DRV_PDEVICE *physDev = dc->physDev;
        TSXSetGraphicsExposures( gdi_display, physDev->gc, True );
        physDev->exposures = 0;
        GDI_ReleaseObj( hdc );
    }
}


/***********************************************************************
 *           X11DRV_EndGraphicsExposures
 *
 * End the graphics exposures mode and process the events
 */
void X11DRV_EndGraphicsExposures( HDC hdc, HRGN hrgn )
{
    HRGN tmp = 0;
    DC *dc = DC_GetDCPtr( hdc );

    if (dc)
    {
        XEvent event;
        X11DRV_PDEVICE *physDev = dc->physDev;

        SetRectRgn( hrgn, 0, 0, 0, 0 );
        wine_tsx11_lock();
        XSetGraphicsExposures( gdi_display, physDev->gc, False );
        if (physDev->exposures)
        {
            XSync( gdi_display, False );
            for (;;)
            {
                XWindowEvent( gdi_display, physDev->drawable, ~0, &event );
                if (event.type == NoExpose) break;
                if (event.type == GraphicsExpose)
                {
                    int x = event.xgraphicsexpose.x - dc->DCOrgX;
                    int y = event.xgraphicsexpose.y - dc->DCOrgY;

                    TRACE( "got %d,%d %dx%d count %d\n",
                           x, y, event.xgraphicsexpose.width, event.xgraphicsexpose.height,
                           event.xgraphicsexpose.count );

                    if (!tmp) tmp = CreateRectRgn( 0, 0, 0, 0 );
                    SetRectRgn( tmp, x, y,
                                x + event.xgraphicsexpose.width,
                                y + event.xgraphicsexpose.height );
                    CombineRgn( hrgn, hrgn, tmp, RGN_OR );
                    if (!event.xgraphicsexpose.count) break;
                }
                else
                {
                    ERR( "got unexpected event %d\n", event.type );
                    break;
                }
                if (tmp) DeleteObject( tmp );
            }
        }
        wine_tsx11_unlock();
        GDI_ReleaseObj( hdc );
    }
}


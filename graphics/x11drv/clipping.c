/*
 * X11DRV clipping functions
 *
 * Copyright 1998 Huw Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "ts_xlib.h"

#include <stdio.h>

#include "gdi.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

/***********************************************************************
 *           X11DRV_GetRegionData
 *
 * Calls GetRegionData on the given region and converts the rectangle
 * array to XRectangle format. The returned buffer must be freed by
 * caller using HeapFree(GetProcessHeap(),...).
 * If hdc_lptodp is not 0, the rectangles are converted through LPtoDP.
 */
RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp )
{
    RGNDATA *data;
    DWORD size;
    int i;
    RECT *rect, tmp;
    XRectangle *xrect;

    if (!(size = GetRegionData( hrgn, 0, NULL ))) return NULL;
    if (sizeof(XRectangle) > sizeof(RECT))
    {
        /* add extra size for XRectangle array */
        int count = (size - sizeof(RGNDATAHEADER)) / sizeof(RECT);
        size += count * (sizeof(XRectangle) - sizeof(RECT));
    }
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!GetRegionData( hrgn, size, data ))
    {
        HeapFree( GetProcessHeap(), 0, data );
        return NULL;
    }

    rect = (RECT *)data->Buffer;
    xrect = (XRectangle *)data->Buffer;
    if (hdc_lptodp)  /* map to device coordinates */
    {
        LPtoDP( hdc_lptodp, (POINT *)rect, data->rdh.nCount * 2 );
        for (i = 0; i < data->rdh.nCount; i++)
        {
            if (rect[i].right < rect[i].left)
            {
                INT tmp = rect[i].right;
                rect[i].right = rect[i].left;
                rect[i].left = tmp;
            }
            if (rect[i].bottom < rect[i].top)
            {
                INT tmp = rect[i].bottom;
                rect[i].bottom = rect[i].top;
                rect[i].top = tmp;
            }
        }
    }

    if (sizeof(XRectangle) > sizeof(RECT))
    {
        /* need to start from the end */
        for (i = data->rdh.nCount-1; i >=0; i--)
        {
            tmp = rect[i];
            xrect[i].x      = tmp.left;
            xrect[i].y      = tmp.top;
            xrect[i].width  = tmp.right - tmp.left;
            xrect[i].height = tmp.bottom - tmp.top;
        }
    }
    else
    {
        for (i = 0; i < data->rdh.nCount; i++)
        {
            tmp = rect[i];
            xrect[i].x      = tmp.left;
            xrect[i].y      = tmp.top;
            xrect[i].width  = tmp.right - tmp.left;
            xrect[i].height = tmp.bottom - tmp.top;
        }
    }
    return data;
}


/***********************************************************************
 *           X11DRV_SetDeviceClipping
 */
void X11DRV_SetDeviceClipping( X11DRV_PDEVICE *physDev, HRGN hrgn )
{
    RGNDATA *data;

    if (!(data = X11DRV_GetRegionData( hrgn, 0 ))) return;
    TSXSetClipRectangles( gdi_display, physDev->gc, 0, 0,
                          (XRectangle *)data->Buffer, data->rdh.nCount, YXBanded );
    HeapFree( GetProcessHeap(), 0, data );
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
        X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
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
	  X11DRV_XRender_UpdateDrawable( physDev );
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
        X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
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
        X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

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


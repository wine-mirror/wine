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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>

#include "x11drv.h"

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
    unsigned int i;
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
        int j;
        /* need to start from the end */
        for (j = data->rdh.nCount-1; j >= 0; j--)
        {
            tmp = rect[j];
            xrect[j].x      = tmp.left;
            xrect[j].y      = tmp.top;
            xrect[j].width  = tmp.right - tmp.left;
            xrect[j].height = tmp.bottom - tmp.top;
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
void X11DRV_SetDeviceClipping( X11DRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn )
{
    RGNDATA *data;

    CombineRgn( physDev->region, vis_rgn, clip_rgn, clip_rgn ? RGN_AND : RGN_COPY );
    if (!(data = X11DRV_GetRegionData( physDev->region, 0 ))) return;

    wine_tsx11_lock();
    XSetClipRectangles( gdi_display, physDev->gc, physDev->org.x, physDev->org.y,
                        (XRectangle *)data->Buffer, data->rdh.nCount, YXBanded );
    wine_tsx11_unlock();
    HeapFree( GetProcessHeap(), 0, data );
}

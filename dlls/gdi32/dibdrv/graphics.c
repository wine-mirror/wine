/*
 * DIB driver graphics operations.
 *
 * Copyright 2011 Huw Davies
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

#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

/***********************************************************************
 *           dibdrv_LineTo
 */
BOOL CDECL dibdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pLineTo );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    POINT pts[2];

    GetCurrentPositionEx(dev->hdc, pts);
    pts[1].x = x;
    pts[1].y = y;

    LPtoDP(dev->hdc, pts, 2);

    if(defer_pen(pdev) || !pdev->pen_line(pdev, pts, pts + 1))
        return next->funcs->pLineTo( next, x, y );

    return TRUE;
}

/*
 * Windows 16 bit device driver graphics functions
 *
 * Copyright 1997 John Harvey
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

#include <stdio.h>

#include "win16drv/win16drv.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16drv);

/***********************************************************************
 *           WIN16DRV_LineTo
 */
BOOL
WIN16DRV_LineTo( PHYSDEV dev, INT x, INT y )
{
    BOOL bRet ;
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    POINT16 points[2];

    points[0].x = dc->CursPosX;
    points[0].y = dc->CursPosY;
    points[1].x = x;
    points[1].y = y;
    LPtoDP16( HDC_16(physDev->hdc), points, 2 );

    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_POLYLINE, 2, points,
                         physDev->PenInfo,
                         NULL,
                         win16drv_SegPtr_DrawMode, dc->hClipRgn);

    dc->CursPosX = x;
    dc->CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_Rectangle
 */
BOOL
WIN16DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    BOOL bRet = 0;
    POINT16 points[2];

    TRACE("In WIN16DRV_Rectangle, x %d y %d\n", left, top );
    TRACE("In WIN16DRV_Rectangle, VPortOrgX %d y %d\n",
           dc->vportOrgX, dc->vportOrgY);
    points[0].x = left;
    points[0].y = top;
    points[1].x = right;
    points[1].y = bottom;
    LPtoDP16( HDC_16(physDev->hdc), points, 2 );

    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                           OS_RECTANGLE, 2, points,
                           physDev->PenInfo,
			   physDev->BrushInfo,
			   win16drv_SegPtr_DrawMode, dc->hClipRgn);
    return bRet;
}




/***********************************************************************
 *           WIN16DRV_Polygon
 */
BOOL
WIN16DRV_Polygon(PHYSDEV dev, const POINT* pt, INT count )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    BOOL bRet = 0;
    LPPOINT16 points;
    int i;

    if(count < 2) return TRUE;
    if(pt[0].x != pt[count-1].x || pt[0].y != pt[count-1].y)
        count++; /* Ensure polygon is closed */

    points = HeapAlloc( GetProcessHeap(), 0, count * sizeof(POINT16) );
    if(points == NULL) return FALSE;

    for (i = 0; i < count - 1; i++)
    {
      points[i].x = pt[i].x;
      points[i].y = pt[i].y;
    }
    LPtoDP16( HDC_16(physDev->hdc), points, count-1 );
    points[count-1].x = points[0].x;
    points[count-1].y = points[0].y;
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_WINDPOLYGON, count, points,
                         physDev->PenInfo,
                         physDev->BrushInfo,
                         win16drv_SegPtr_DrawMode, dc->hClipRgn);
    HeapFree( GetProcessHeap(), 0, points );
    return bRet;
}


/***********************************************************************
 *           WIN16DRV_Polyline
 */
BOOL
WIN16DRV_Polyline(PHYSDEV dev, const POINT* pt, INT count )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    BOOL bRet = 0;
    LPPOINT16 points;
    int i;

    if(count < 2) return TRUE;

    points = HeapAlloc( GetProcessHeap(), 0, count * sizeof(POINT16) );
    if(points == NULL) return FALSE;

    for (i = 0; i < count; i++)
    {
      points[i].x = pt[i].x;
      points[i].y = pt[i].y;
    }
    LPtoDP16( HDC_16(physDev->hdc), points, count );
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_POLYLINE, count, points,
                         physDev->PenInfo,
                         NULL,
                         win16drv_SegPtr_DrawMode, dc->hClipRgn);
    HeapFree( GetProcessHeap(), 0, points );
    return bRet;
}



/***********************************************************************
 *           WIN16DRV_Ellipse
 */
BOOL
WIN16DRV_Ellipse(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    BOOL bRet = 0;
    POINT16 points[2];

    TRACE("In WIN16DRV_Ellipse, x %d y %d\n", left, top );
    TRACE("In WIN16DRV_Ellipse, VPortOrgX %d y %d\n", dc->vportOrgX, dc->vportOrgY);

    points[0].x = left;
    points[0].y = top;
    points[1].x = right;
    points[1].y = bottom;
    LPtoDP16( HDC_16(physDev->hdc), points, 2 );

    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_ELLIPSE, 2, points,
                         physDev->PenInfo,
                         physDev->BrushInfo,
                         win16drv_SegPtr_DrawMode, dc->hClipRgn);
    return bRet;
}

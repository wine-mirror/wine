/*
 * Windows 16 bit device driver graphics functions
 *
 * Copyright 1997 John Harvey
 */

#include <stdio.h>
#include "heap.h"
#include "win16drv.h"
#include "debug.h"

/**********************************************************************
 *	     WIN16DRV_MoveToEx
 */
BOOL
WIN16DRV_MoveToEx(DC *dc,INT x,INT y,LPPOINT pt) 
{
    if (pt)
    {
	pt->x = dc->w.CursPosX;
	pt->y = dc->w.CursPosY;
    }
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}

/***********************************************************************
 *           WIN16DRV_LineTo
 */
BOOL
WIN16DRV_LineTo( DC *dc, INT x, INT y )
{
    BOOL bRet ;
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    POINT16 points[2];
    points[0].x = dc->w.DCOrgX + XLPTODP( dc, dc->w.CursPosX );
    points[0].y = dc->w.DCOrgY + YLPTODP( dc, dc->w.CursPosY );
    points[1].x = dc->w.DCOrgX + XLPTODP( dc, x );
    points[1].y = dc->w.DCOrgY + YLPTODP( dc, y );
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_POLYLINE, 2, points, 
                         physDev->PenInfo,
                         NULL,
                         win16drv_SegPtr_DrawMode, dc->w.hClipRgn);

    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_Rectangle
 */
BOOL
WIN16DRV_Rectangle(DC *dc, INT left, INT top, INT right, INT bottom)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL bRet = 0;
    POINT16 points[2];

    TRACE(win16drv, "In WIN16DRV_Rectangle, x %d y %d DCOrgX %d y %d\n",
           left, top, dc->w.DCOrgX, dc->w.DCOrgY);
    TRACE(win16drv, "In WIN16DRV_Rectangle, VPortOrgX %d y %d\n",
           dc->vportOrgX, dc->vportOrgY);
    points[0].x = XLPTODP(dc, left);
    points[0].y = YLPTODP(dc, top);

    points[1].x = XLPTODP(dc, right);
    points[1].y = YLPTODP(dc, bottom);
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                           OS_RECTANGLE, 2, points, 
                           physDev->PenInfo,
			   physDev->BrushInfo,
			   win16drv_SegPtr_DrawMode, dc->w.hClipRgn);
    return bRet;
}




/***********************************************************************
 *           WIN16DRV_Polygon
 */
BOOL
WIN16DRV_Polygon(DC *dc, const POINT* pt, INT count )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL bRet = 0;
    LPPOINT16 points;
    int i;

    if(count < 2) return TRUE;
    if(pt[0].x != pt[count-1].x || pt[0].y != pt[count-1].y)
        count++; /* Ensure polygon is closed */

    points = HEAP_xalloc( GetProcessHeap(), 0, count * sizeof(POINT16) );
    for (i = 0; i < count - 1; i++)
    {
      points[i].x = XLPTODP( dc, pt[i].x );
      points[i].y = YLPTODP( dc, pt[i].y );
    }
    points[count-1].x = points[0].x;
    points[count-1].y = points[0].y;
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_WINDPOLYGON, count, points, 
                         physDev->PenInfo,
                         physDev->BrushInfo,
                         win16drv_SegPtr_DrawMode, dc->w.hClipRgn);
    HeapFree( GetProcessHeap(), 0, points );
    return bRet;
}


/***********************************************************************
 *           WIN16DRV_Polyline
 */
BOOL
WIN16DRV_Polyline(DC *dc, const POINT* pt, INT count )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL bRet = 0;
    LPPOINT16 points;
    int i;

    if(count < 2) return TRUE;

    points = HEAP_xalloc( GetProcessHeap(), 0, count * sizeof(POINT16) );
    for (i = 0; i < count; i++)
    {
      points[i].x = XLPTODP( dc, pt[i].x );
      points[i].y = YLPTODP( dc, pt[i].y );
    }
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_POLYLINE, count, points, 
                         physDev->PenInfo,
                         NULL,
                         win16drv_SegPtr_DrawMode, dc->w.hClipRgn);
    HeapFree( GetProcessHeap(), 0, points );
    return bRet;
}



/***********************************************************************
 *           WIN16DRV_Ellipse
 */
BOOL
WIN16DRV_Ellipse(DC *dc, INT left, INT top, INT right, INT bottom)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL bRet = 0;
    POINT16 points[2];
    TRACE(win16drv, "In WIN16DRV_Ellipse, x %d y %d DCOrgX %d y %d\n",
           left, top, dc->w.DCOrgX, dc->w.DCOrgY);
    TRACE(win16drv, "In WIN16DRV_Ellipse, VPortOrgX %d y %d\n",
           dc->vportOrgX, dc->vportOrgY);
    points[0].x = XLPTODP(dc, left);
    points[0].y = YLPTODP(dc, top);

    points[1].x = XLPTODP(dc, right);
    points[1].y = YLPTODP(dc, bottom);

    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_ELLIPSE, 2, points, 
                         physDev->PenInfo,
                         physDev->BrushInfo,
                         win16drv_SegPtr_DrawMode, dc->w.hClipRgn);
    return bRet;
}









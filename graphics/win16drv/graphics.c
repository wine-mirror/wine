/*
 * Windows 16 bit device driver graphics functions
 *
 * Copyright 1997 John Harvey
 */

#include "win16drv.h"

/**********************************************************************
 *	     WIN16DRV_MoveToEx
 */
BOOL32
WIN16DRV_MoveToEx(DC *dc,INT32 x,INT32 y,LPPOINT32 pt) 
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
BOOL32
WIN16DRV_LineTo( DC *dc, INT32 x, INT32 y )
{
    BOOL32 bRet ;
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    POINT16 points[2];
    points[0].x = dc->w.DCOrgX + XLPTODP( dc, dc->w.CursPosX );
    points[0].y = dc->w.DCOrgY + YLPTODP( dc, dc->w.CursPosY );
    points[1].x = dc->w.DCOrgX + XLPTODP( dc, x );
    points[1].y = dc->w.DCOrgY + YLPTODP( dc, y );
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_POLYLINE, 2, points, 
                         physDev->segptrPenInfo,
                         NULL,
                         win16drv_SegPtr_DrawMode, NULL);

    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_Rectangle
 */
BOOL32
WIN16DRV_Rectangle(DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL32 bRet = 0;
    POINT16 points[2];
    printf("In WIN16drv_Rectangle, x %d y %d DCOrgX %d y %d\n",
           left, top, dc->w.DCOrgX, dc->w.DCOrgY);
    printf("In WIN16drv_Rectangle, VPortOrgX %d y %d\n",
           dc->vportOrgX, dc->vportOrgY);
    points[0].x = XLPTODP(dc, left);
    points[0].y = YLPTODP(dc, top);

    points[1].x = XLPTODP(dc, right);
    points[1].y = XLPTODP(dc, bottom);
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_RECTANGLE, 2, points, 
                         physDev->segptrPenInfo,
                         physDev->segptrBrushInfo,
                         win16drv_SegPtr_DrawMode, NULL);
    return bRet;
}




/***********************************************************************
 *           WIN16DRV_Polygon
 */
BOOL32
WIN16DRV_Polygon(DC *dc, LPPOINT32 pt, INT32 count )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL32 bRet = 0;
    LPPOINT16 points;
    int i;
    points = malloc(count * sizeof(POINT16));
    for (i = 0; i<count ; i++)
    {
      points[i].x = ((pt[i].x - dc->wndOrgX) * dc->vportExtX/ dc->wndExtX) + dc->vportOrgX;
      points[i].y = ((pt[i].y - dc->wndOrgY) * dc->vportExtY/ dc->wndExtY) + dc->vportOrgY;
    }
    bRet = PRTDRV_Output(physDev->segptrPDEVICE,
                         OS_WINDPOLYGON, 2, points, 
                         physDev->segptrPenInfo,
                         physDev->segptrBrushInfo,
                         win16drv_SegPtr_DrawMode, NULL);
    return bRet;
}





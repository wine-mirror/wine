/*
 *	PostScript driver graphics functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include "windows.h"
#include "psdrv.h"
#include "debug.h"
#include "print.h"

/**********************************************************************
 *	     PSDRV_MoveToEx
 */
BOOL32 PSDRV_MoveToEx(DC *dc, INT32 x, INT32 y, LPPOINT32 pt)
{
    TRACE(psdrv, "%d %d\n", x, y);
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
 *           PSDRV_LineTo
 */
BOOL32 PSDRV_LineTo(DC *dc, INT32 x, INT32 y)
{
    TRACE(psdrv, "%d %d\n", x, y);

    PSDRV_SetPen(dc);
    PSDRV_WriteMoveTo(dc, XLPTODP(dc, dc->w.CursPosX),
		          YLPTODP(dc, dc->w.CursPosY));
    PSDRV_WriteLineTo(dc, XLPTODP(dc, x), YLPTODP(dc, y));
    PSDRV_WriteStroke(dc);

    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}

/***********************************************************************
 *           PSDRV_Rectangle
 */
BOOL32 PSDRV_Rectangle(DC *dc, INT32 left, INT32 top, INT32 right,
		       INT32 bottom)
{
    INT32 width = XLSTODS(dc, right - left);
    INT32 height = YLSTODS(dc, bottom - top);


    TRACE(psdrv, "%d %d - %d %d\n", left, top, right, bottom);

    PSDRV_WriteRectangle(dc, XLPTODP(dc, left), YLPTODP(dc, top),
			     width, height);

    PSDRV_SetBrush(dc);
    PSDRV_Writegsave(dc);
    PSDRV_WriteFill(dc);
    PSDRV_Writegrestore(dc);
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Ellipse
 */
BOOL32 PSDRV_Ellipse( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    INT32 x, y, a, b;

    TRACE(psdrv, "%d %d - %d %d\n", left, top, right, bottom);

    x = XLPTODP(dc, (left + right)/2);
    y = YLPTODP(dc, (top + bottom)/2);

    a = XLSTODS(dc, (right - left)/2);
    b = YLSTODS(dc, (bottom - top)/2);

    PSDRV_WriteEllispe(dc, x, y, a, b);
    PSDRV_SetBrush(dc);
    PSDRV_Writegsave(dc);
    PSDRV_WriteFill(dc);
    PSDRV_Writegrestore(dc);
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Polyline
 */
BOOL32 PSDRV_Polyline( DC *dc, const LPPOINT32 pt, INT32 count )
{
    INT32 i;
    TRACE(psdrv, "count = %d\n", count);
    
    PSDRV_SetPen(dc);
    PSDRV_WriteMoveTo(dc, XLPTODP(dc, pt[0].x), YLPTODP(dc, pt[0].y));
    for(i = 1; i < count; i++)
        PSDRV_WriteLineTo(dc, XLPTODP(dc, pt[i].x), YLPTODP(dc, pt[i].y));
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Polygon
 */
BOOL32 PSDRV_Polygon( DC *dc, LPPOINT32 pt, INT32 count )
{
    INT32 i;
    TRACE(psdrv, "count = %d\n", count);
    FIXME(psdrv, "Hack!\n");
    
    PSDRV_SetPen(dc);
    PSDRV_WriteMoveTo(dc, XLPTODP(dc, pt[0].x), YLPTODP(dc, pt[0].y));
    for(i = 1; i < count; i++)
        PSDRV_WriteLineTo(dc, XLPTODP(dc, pt[i].x), YLPTODP(dc, pt[i].y));

    if(pt[0].x != pt[count-1].x || pt[0].y != pt[count-1].y)
        PSDRV_WriteLineTo(dc, XLPTODP(dc, pt[0].x), YLPTODP(dc, pt[0].y));

    PSDRV_WriteStroke(dc);
    return TRUE;
}


/*
 *	Postscript driver graphics functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 *	Not much here yet...
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

    return PSDRV_WriteMoveTo(dc, XLPTODP(dc, x), YLPTODP(dc, y));
}

/***********************************************************************
 *           PSDRV_LineTo
 */
BOOL32 PSDRV_LineTo(DC *dc, INT32 x, INT32 y)
{
    TRACE(psdrv, "%d %d\n", x, y);

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
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Ellipse
 */
BOOL32 PSDRV_Ellipse( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom )
{
    TRACE(psdrv, "%d %d - %d %d\n", left, top, right, bottom);
    
    return TRUE;
}

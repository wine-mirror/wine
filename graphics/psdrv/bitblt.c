/*
 *	PostScript driver BitBlt, StretchBlt and PatBlt
 *
 * Copyright 1999  Huw D M Davies
 *
 */

#include "gdi.h"
#include "psdrv.h"
#include "debugtools.h"
#include "winbase.h"

DEFAULT_DEBUG_CHANNEL(psdrv)


/***********************************************************************
 *
 *                    PSDRV_PatBlt
 */
BOOL PSDRV_PatBlt(DC *dc, INT x, INT y, INT width, INT height, DWORD dwRop)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    switch(dwRop) {
    case PATCOPY:
        PSDRV_WriteGSave(dc);
	PSDRV_WriteRectangle(dc, XLPTODP(dc, x), YLPTODP(dc, y),
			     XLSTODS(dc, width), YLSTODS(dc, height));
	PSDRV_Brush(dc, FALSE);
	PSDRV_WriteGRestore(dc);
	return TRUE;

    case BLACKNESS:
    case WHITENESS:
      {
	PSCOLOR pscol;

        PSDRV_WriteGSave(dc);
	PSDRV_WriteRectangle(dc, XLPTODP(dc, x), YLPTODP(dc, y),
			     XLSTODS(dc, width), YLSTODS(dc, height));
	PSDRV_CreateColor( physDev, &pscol, (dwRop == BLACKNESS) ?
			   RGB(0,0,0) : RGB(0xff,0xff,0xff) );
	PSDRV_WriteSetColor(dc, &pscol);
	PSDRV_WriteFill(dc);
	PSDRV_WriteGRestore(dc);
	return TRUE;
      }
    default:
        FIXME("Unsupported rop %ld\n", dwRop);
	return FALSE;
    }
}

/*
 *	PostScript driver BitBlt, StretchBlt and PatBlt
 *
 * Copyright 1999  Huw D M Davies
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

#include "gdi.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


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

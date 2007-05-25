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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/***********************************************************************
 *
 *                    PSDRV_PatBlt
 */
BOOL PSDRV_PatBlt(PSDRV_PDEVICE *physDev, INT x, INT y, INT width, INT height, DWORD dwRop)
{
    POINT pt[2];

    pt[0].x = x;
    pt[0].y = y;
    pt[1].x = x + width;
    pt[1].y = y + height;
    LPtoDP( physDev->hdc, pt, 2 );

    switch(dwRop) {
    case PATCOPY:
        PSDRV_SetClip(physDev);
        PSDRV_WriteGSave(physDev);
        PSDRV_WriteRectangle(physDev, pt[0].x, pt[0].y, pt[1].x - pt[0].x, pt[1].y - pt[0].y );
	PSDRV_Brush(physDev, FALSE);
	PSDRV_WriteGRestore(physDev);
        PSDRV_ResetClip(physDev);
	return TRUE;

    case BLACKNESS:
    case WHITENESS:
      {
	PSCOLOR pscol;

        PSDRV_SetClip(physDev);
        PSDRV_WriteGSave(physDev);
        PSDRV_WriteRectangle(physDev, pt[0].x, pt[0].y, pt[1].x - pt[0].x, pt[1].y - pt[0].y );
	PSDRV_CreateColor( physDev, &pscol, (dwRop == BLACKNESS) ?
			   RGB(0,0,0) : RGB(0xff,0xff,0xff) );
	PSDRV_WriteSetColor(physDev, &pscol);
	PSDRV_WriteFill(physDev);
	PSDRV_WriteGRestore(physDev);
        PSDRV_ResetClip(physDev);
	return TRUE;
      }
    default:
        FIXME("Unsupported rop %d\n", dwRop);
	return FALSE;
    }
}

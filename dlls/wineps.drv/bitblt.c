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
BOOL CDECL PSDRV_PatBlt(PHYSDEV dev, struct bitblt_coords *dst, DWORD dwRop)
{
    switch(dwRop) {
    case PATCOPY:
        PSDRV_SetClip(dev);
        PSDRV_WriteGSave(dev);
        PSDRV_WriteRectangle(dev, dst->visrect.left, dst->visrect.top,
                             dst->visrect.right - dst->visrect.left,
                             dst->visrect.bottom - dst->visrect.top );
	PSDRV_Brush(dev, FALSE);
	PSDRV_WriteGRestore(dev);
        PSDRV_ResetClip(dev);
	return TRUE;

    case BLACKNESS:
    case WHITENESS:
      {
	PSCOLOR pscol;

        PSDRV_SetClip(dev);
        PSDRV_WriteGSave(dev);
        PSDRV_WriteRectangle(dev, dst->visrect.left, dst->visrect.top,
                             dst->visrect.right - dst->visrect.left,
                             dst->visrect.bottom - dst->visrect.top );
	PSDRV_CreateColor( dev, &pscol, (dwRop == BLACKNESS) ? RGB(0,0,0) : RGB(0xff,0xff,0xff) );
	PSDRV_WriteSetColor(dev, &pscol);
	PSDRV_WriteFill(dev);
	PSDRV_WriteGRestore(dev);
        PSDRV_ResetClip(dev);
	return TRUE;
      }
    default:
        FIXME("Unsupported rop %06lx\n", dwRop);
	return FALSE;
    }
}

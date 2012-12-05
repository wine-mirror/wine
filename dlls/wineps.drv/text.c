/*
 *	PostScript driver text functions
 *
 *	Copyright 1998  Huw D M Davies
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
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "windef.h"
#include "wingdi.h"
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static BOOL PSDRV_Text(PHYSDEV dev, INT x, INT y, UINT flags,
		       LPCWSTR str, UINT count,
		       BOOL bDrawBackground, const INT *lpDx);

/***********************************************************************
 *           PSDRV_ExtTextOut
 */
BOOL PSDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect, LPCWSTR str, UINT count,
                       const INT *lpDx )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    BOOL bResult = TRUE;
    BOOL bClipped = FALSE;
    BOOL bOpaque = FALSE;

    TRACE("(x=%d, y=%d, flags=0x%08x, str=%s, count=%d, lpDx=%p)\n", x, y,
	  flags, debugstr_wn(str, count), count, lpDx);

    if(physDev->job.id == 0) return FALSE;

    /* write font if not already written */
    PSDRV_SetFont(dev);

    PSDRV_SetClip(dev);

    /* set clipping and/or draw background */
    if ((flags & (ETO_CLIPPED | ETO_OPAQUE)) && (lprect != NULL))
    {
	PSDRV_WriteGSave(dev);
	PSDRV_WriteRectangle(dev, lprect->left, lprect->top, lprect->right - lprect->left,
			     lprect->bottom - lprect->top);

	if (flags & ETO_OPAQUE)
	{
	    bOpaque = TRUE;
	    PSDRV_WriteGSave(dev);
	    PSDRV_WriteSetColor(dev, &physDev->bkColor);
	    PSDRV_WriteFill(dev);
	    PSDRV_WriteGRestore(dev);
	}

	if (flags & ETO_CLIPPED)
	{
	    bClipped = TRUE;
	    PSDRV_WriteClip(dev);
	}

	bResult = PSDRV_Text(dev, x, y, flags, str, count, !(bClipped && bOpaque), lpDx);
	PSDRV_WriteGRestore(dev);
    }
    else
    {
	bResult = PSDRV_Text(dev, x, y, flags, str, count, TRUE, lpDx);
    }

    PSDRV_ResetClip(dev);
    return bResult;
}

/***********************************************************************
 *           PSDRV_Text
 */
static BOOL PSDRV_Text(PHYSDEV dev, INT x, INT y, UINT flags, LPCWSTR str,
		       UINT count, BOOL bDrawBackground, const INT *lpDx)
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    WORD *glyphs = NULL;

    if (!count)
	return TRUE;

    if(physDev->font.fontloc == Download && !(flags & ETO_GLYPH_INDEX))
    {
        glyphs = HeapAlloc( GetProcessHeap(), 0, count * sizeof(WORD) );
        GetGlyphIndicesW( dev->hdc, str, count, glyphs, 0 );
        str = glyphs;
    }

    PSDRV_WriteMoveTo(dev, x, y);

    if(!lpDx) {
        if(physDev->font.fontloc == Download)
	    PSDRV_WriteDownloadGlyphShow(dev, str, count);
	else
	    PSDRV_WriteBuiltinGlyphShow(dev, str, count);
    }
    else {
        UINT i;
	POINT offset = {0, 0};

        for(i = 0; i < count-1; i++) {
	    if(physDev->font.fontloc == Download)
	        PSDRV_WriteDownloadGlyphShow(dev, str + i, 1);
	    else
	        PSDRV_WriteBuiltinGlyphShow(dev, str + i, 1);
            if(flags & ETO_PDY)
            {
                offset.x += lpDx[i * 2];
                offset.y += lpDx[i * 2 + 1];
            }
            else
                offset.x += lpDx[i];
	    PSDRV_WriteMoveTo(dev, x + offset.x, y + offset.y);
	}
	if(physDev->font.fontloc == Download)
	    PSDRV_WriteDownloadGlyphShow(dev, str + i, 1);
	else
	    PSDRV_WriteBuiltinGlyphShow(dev, str + i, 1);
    }

    HeapFree( GetProcessHeap(), 0, glyphs );
    return TRUE;
}

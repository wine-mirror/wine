/*
 *	PostScript driver font functions
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
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winspool.h"
#include "winternl.h"

#include "psdrv.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           SelectFont   (WINEPS.@)
 */
HFONT CDECL PSDRV_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    struct font_info font_info;

    if (ExtEscape(dev->hdc, PSDRV_GET_BUILTIN_FONT_INFO, 0, NULL,
                sizeof(font_info), (char *)&font_info))
    {
        physDev->font.fontloc = Builtin;
    }
    else
    {
        physDev->font.fontloc = Download;
        physDev->font.fontinfo.Download = NULL;
    }
    return hfont;
}

/***********************************************************************
 *           PSDRV_SetFont
 */
BOOL PSDRV_SetFont( PHYSDEV dev, BOOL vertical )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    PSDRV_WriteSetColor(dev, &physDev->font.color);
    if (vertical && (physDev->font.set == VERTICAL_SET)) return TRUE;
    if (!vertical && (physDev->font.set == HORIZONTAL_SET)) return TRUE;

    switch(physDev->font.fontloc) {
    case Builtin:
        PSDRV_WriteSetBuiltinFont(dev);
	break;
    case Download:
        PSDRV_WriteSetDownloadFont(dev, vertical);
	break;
    default:
        ERR("fontloc = %d\n", physDev->font.fontloc);
        assert(1);
	break;
    }
    if (vertical)
        physDev->font.set = VERTICAL_SET;
    else
        physDev->font.set = HORIZONTAL_SET;
    return TRUE;
}

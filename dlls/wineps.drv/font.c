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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           SelectFont   (WINEPS.@)
 */
HFONT CDECL PSDRV_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectFont );
    HFONT ret;
    LOGFONTW lf;
    BOOL subst = FALSE;
    char FaceName[LF_FACESIZE];

    if (!GetObjectW( hfont, sizeof(lf), &lf )) return 0;

    *aa_flags = GGO_BITMAP; /* no anti-aliasing on printer devices */

    TRACE("FaceName = %s Height = %ld Italic = %d Weight = %ld\n",
	  debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
	  lf.lfWeight);

    WideCharToMultiByte(CP_ACP, 0, lf.lfFaceName, -1,
			FaceName, sizeof(FaceName), NULL, NULL);

    if(FaceName[0] == '\0') {
        switch(lf.lfPitchAndFamily & 0xf0) {
	case FF_DONTCARE:
	    break;
	case FF_ROMAN:
	case FF_SCRIPT:
	    strcpy(FaceName, "Times");
	    break;
	case FF_SWISS:
	    strcpy(FaceName, "Helvetica");
	    break;
	case FF_MODERN:
	    strcpy(FaceName, "Courier");
	    break;
	case FF_DECORATIVE:
	    strcpy(FaceName, "Symbol");
	    break;
	}
    }

    if(FaceName[0] == '\0') {
        switch(lf.lfPitchAndFamily & 0x0f) {
	case VARIABLE_PITCH:
	    strcpy(FaceName, "Times");
	    break;
	default:
	    strcpy(FaceName, "Courier");
	    break;
	}
    }

    if (physDev->pi->FontSubTableSize != 0)
    {
	DWORD i;

	for (i = 0; i < physDev->pi->FontSubTableSize; ++i)
	{
	    if (!stricmp (FaceName,
		    physDev->pi->FontSubTable[i].pValueName))
	    {
		TRACE ("substituting facename '%s' for '%s'\n",
			(LPSTR) physDev->pi->FontSubTable[i].pData, FaceName);
		if (strlen ((LPSTR) physDev->pi->FontSubTable[i].pData) <
			LF_FACESIZE)
		{
		    strcpy (FaceName,
			    (LPSTR) physDev->pi->FontSubTable[i].pData);
		    subst = TRUE;
		}
		else
		    WARN ("Facename '%s' is too long; ignoring substitution\n",
			    (LPSTR) physDev->pi->FontSubTable[i].pData);
		break;
	    }
	}
    }

    physDev->font.escapement = lf.lfEscapement;
    physDev->font.set = UNSET;

    if (!subst && ((ret = next->funcs->pSelectFont( next, hfont, aa_flags ))))
    {
        PSDRV_SelectDownloadFont(dev);
        return ret;
    }

    PSDRV_SelectBuiltinFont(dev, hfont, &lf, FaceName);
    next->funcs->pSelectFont( next, 0, aa_flags );  /* tell next driver that we selected a device font */
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

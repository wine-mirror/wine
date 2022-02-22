/*
 *	PostScript pen handling
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static const DWORD PEN_dash[]       = { 50, 30 };                 /* -----   -----   -----  */
static const DWORD PEN_dot[]        = { 20 };                     /* --  --  --  --  --  -- */
static const DWORD PEN_dashdot[]    = { 40, 30, 20, 30 };         /* ----   --   ----   --  */
static const DWORD PEN_dashdotdot[] = { 40, 20, 20, 20, 20, 20 }; /* ----  --  --  ----  */
static const DWORD PEN_alternate[]  = { 1 };

/***********************************************************************
 *           SelectPen   (WINEPS.@)
 */
HPEN CDECL PSDRV_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    LOGPEN logpen;
    EXTLOGPEN *elp = NULL;

    if (!GetObjectW( hpen, sizeof(logpen), &logpen ))
    {
        /* must be an extended pen */
        INT size = GetObjectW( hpen, 0, NULL );

        if (!size) return 0;

        elp = HeapAlloc( GetProcessHeap(), 0, size );

        GetObjectW( hpen, size, elp );
        /* FIXME: add support for user style pens */
        logpen.lopnStyle = elp->elpPenStyle;
        logpen.lopnWidth.x = elp->elpWidth;
        logpen.lopnWidth.y = 0;
        logpen.lopnColor = elp->elpColor;
    }

    TRACE("hpen = %p colour = %08lx\n", hpen, logpen.lopnColor);

    physDev->pen.width = logpen.lopnWidth.x;
    if ((logpen.lopnStyle & PS_GEOMETRIC) || (physDev->pen.width > 1))
    {
        physDev->pen.width = PSDRV_XWStoDS( dev, physDev->pen.width );
        if(physDev->pen.width < 0) physDev->pen.width = -physDev->pen.width;
    }
    if (hpen == GetStockObject( DC_PEN ))
        logpen.lopnColor = GetDCPenColor( dev->hdc );

    switch (logpen.lopnStyle & PS_JOIN_MASK)
    {
    default:
    case PS_JOIN_ROUND: physDev->pen.join = 1; break;
    case PS_JOIN_BEVEL: physDev->pen.join = 2; break;
    case PS_JOIN_MITER: physDev->pen.join = 0; break;
    }

    switch (logpen.lopnStyle & PS_ENDCAP_MASK)
    {
    default:
    case PS_ENDCAP_ROUND:  physDev->pen.endcap = 1; break;
    case PS_ENDCAP_SQUARE: physDev->pen.endcap = 2; break;
    case PS_ENDCAP_FLAT:   physDev->pen.endcap = 0; break;
    }

    PSDRV_CreateColor(dev, &physDev->pen.color, logpen.lopnColor);
    physDev->pen.style = logpen.lopnStyle & PS_STYLE_MASK;

    switch(physDev->pen.style) {
    case PS_DASH:
        memcpy( physDev->pen.dash, PEN_dash, sizeof(PEN_dash) );
        physDev->pen.dash_len = ARRAY_SIZE( PEN_dash );
	break;

    case PS_DOT:
        memcpy( physDev->pen.dash, PEN_dot, sizeof(PEN_dot) );
        physDev->pen.dash_len = ARRAY_SIZE( PEN_dot );
	break;

    case PS_DASHDOT:
        memcpy( physDev->pen.dash, PEN_dashdot, sizeof(PEN_dashdot) );
        physDev->pen.dash_len = ARRAY_SIZE( PEN_dashdot );
	break;

    case PS_DASHDOTDOT:
        memcpy( physDev->pen.dash, PEN_dashdotdot, sizeof(PEN_dashdotdot) );
        physDev->pen.dash_len = ARRAY_SIZE( PEN_dashdotdot );
	break;

    case PS_ALTERNATE:
        memcpy( physDev->pen.dash, PEN_alternate, sizeof(PEN_alternate) );
        physDev->pen.dash_len = ARRAY_SIZE( PEN_alternate );
	break;

    case PS_USERSTYLE:
        physDev->pen.dash_len = min( elp->elpNumEntries, MAX_DASHLEN );
        memcpy( physDev->pen.dash, elp->elpStyleEntry, physDev->pen.dash_len * sizeof(DWORD) );
	break;

    default:
	physDev->pen.dash_len = 0;
    }

    if ((physDev->pen.width > 1) && physDev->pen.dash_len &&
        physDev->pen.style != PS_USERSTYLE && physDev->pen.style != PS_ALTERNATE)
    {
        physDev->pen.style = PS_SOLID;
        physDev->pen.dash_len = 0;
    }

    HeapFree( GetProcessHeap(), 0, elp );
    physDev->pen.set = FALSE;
    return hpen;
}


/***********************************************************************
 *           SetDCPenColor (WINEPS.@)
 */
COLORREF CDECL PSDRV_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    if (GetCurrentObject( dev->hdc, OBJ_PEN ) == GetStockObject( DC_PEN ))
        PSDRV_CreateColor( dev, &physDev->pen.color, color );
    return color;
}


/**********************************************************************
 *
 *	PSDRV_SetPen
 *
 */
BOOL PSDRV_SetPen( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    if (physDev->pen.style != PS_NULL) {
	PSDRV_WriteSetColor(dev, &physDev->pen.color);

	if(!physDev->pen.set) {
	    PSDRV_WriteSetPen(dev);
	    physDev->pen.set = TRUE;
	}
    }

    return TRUE;
}

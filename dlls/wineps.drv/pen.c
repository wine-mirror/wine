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
 *           SelectPen
 */
HPEN PSDRV_SelectPen( print_ctx *ctx, HPEN hpen, const struct ps_brush_pattern *pattern )
{
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

    ctx->pen.width = logpen.lopnWidth.x;
    if ((logpen.lopnStyle & PS_GEOMETRIC) || (ctx->pen.width > 1))
    {
        ctx->pen.width = PSDRV_XWStoDS( ctx, ctx->pen.width );
        if(ctx->pen.width < 0) ctx->pen.width = -ctx->pen.width;
    }
    if (hpen == GetStockObject( DC_PEN ))
        logpen.lopnColor = GetDCPenColor( ctx->hdc );

    switch (logpen.lopnStyle & PS_JOIN_MASK)
    {
    default:
    case PS_JOIN_ROUND: ctx->pen.join = 1; break;
    case PS_JOIN_BEVEL: ctx->pen.join = 2; break;
    case PS_JOIN_MITER: ctx->pen.join = 0; break;
    }

    switch (logpen.lopnStyle & PS_ENDCAP_MASK)
    {
    default:
    case PS_ENDCAP_ROUND:  ctx->pen.endcap = 1; break;
    case PS_ENDCAP_SQUARE: ctx->pen.endcap = 2; break;
    case PS_ENDCAP_FLAT:   ctx->pen.endcap = 0; break;
    }

    PSDRV_CreateColor(ctx, &ctx->pen.color, logpen.lopnColor);
    ctx->pen.style = logpen.lopnStyle & PS_STYLE_MASK;

    switch(ctx->pen.style) {
    case PS_DASH:
        memcpy( ctx->pen.dash, PEN_dash, sizeof(PEN_dash) );
        ctx->pen.dash_len = ARRAY_SIZE( PEN_dash );
	break;

    case PS_DOT:
        memcpy( ctx->pen.dash, PEN_dot, sizeof(PEN_dot) );
        ctx->pen.dash_len = ARRAY_SIZE( PEN_dot );
	break;

    case PS_DASHDOT:
        memcpy( ctx->pen.dash, PEN_dashdot, sizeof(PEN_dashdot) );
        ctx->pen.dash_len = ARRAY_SIZE( PEN_dashdot );
	break;

    case PS_DASHDOTDOT:
        memcpy( ctx->pen.dash, PEN_dashdotdot, sizeof(PEN_dashdotdot) );
        ctx->pen.dash_len = ARRAY_SIZE( PEN_dashdotdot );
	break;

    case PS_ALTERNATE:
        memcpy( ctx->pen.dash, PEN_alternate, sizeof(PEN_alternate) );
        ctx->pen.dash_len = ARRAY_SIZE( PEN_alternate );
	break;

    case PS_USERSTYLE:
        ctx->pen.dash_len = min( elp->elpNumEntries, MAX_DASHLEN );
        memcpy( ctx->pen.dash, elp->elpStyleEntry, ctx->pen.dash_len * sizeof(DWORD) );
	break;

    default:
	ctx->pen.dash_len = 0;
    }

    if ((ctx->pen.width > 1) && ctx->pen.dash_len &&
        ctx->pen.style != PS_USERSTYLE && ctx->pen.style != PS_ALTERNATE)
    {
        ctx->pen.style = PS_SOLID;
        ctx->pen.dash_len = 0;
    }

    HeapFree( GetProcessHeap(), 0, elp );
    ctx->pen.set = FALSE;
    return hpen;
}


/***********************************************************************
 *           SetDCPenColor
 */
COLORREF PSDRV_SetDCPenColor( print_ctx *ctx, COLORREF color )
{
    if (GetCurrentObject( ctx->hdc, OBJ_PEN ) == GetStockObject( DC_PEN ))
        PSDRV_CreateColor( ctx, &ctx->pen.color, color );
    return color;
}


/**********************************************************************
 *
 *	PSDRV_SetPen
 *
 */
BOOL PSDRV_SetPen( print_ctx *ctx )
{
    if (ctx->pen.style != PS_NULL) {
	PSDRV_WriteSetColor(ctx, &ctx->pen.color);

	if(!ctx->pen.set) {
	    PSDRV_WriteSetPen(ctx);
	    ctx->pen.set = TRUE;
	}
    }

    return TRUE;
}

/*
 *	PostScript brush handling
 *
 * Copyright 1998  Huw D M Davies
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
#include "winbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           SelectBrush
 */
HBRUSH PSDRV_SelectBrush( print_ctx *ctx, HBRUSH hbrush, const struct ps_brush_pattern *pattern )
{
    LOGBRUSH logbrush;

    if (!GetObjectA( hbrush, sizeof(logbrush), &logbrush )) return 0;

    TRACE("hbrush = %p\n", hbrush);

    if (hbrush == GetStockObject( DC_BRUSH ))
        logbrush.lbColor = GetDCBrushColor( ctx->hdc );

    switch(logbrush.lbStyle) {

    case BS_SOLID:
        PSDRV_CreateColor(ctx, &ctx->brush.color, logbrush.lbColor);
	break;

    case BS_NULL:
        break;

    case BS_HATCHED:
        PSDRV_CreateColor(ctx, &ctx->brush.color, logbrush.lbColor);
        break;

    case BS_PATTERN:
    case BS_DIBPATTERN:
        ctx->brush.pattern = *pattern;
	break;

    default:
        FIXME("Unrecognized brush style %d\n", logbrush.lbStyle);
	break;
    }

    ctx->brush.set = FALSE;
    return hbrush;
}


/***********************************************************************
 *           SetDCBrushColor
 */
COLORREF PSDRV_SetDCBrushColor( print_ctx *ctx, COLORREF color )
{
    if (GetCurrentObject( ctx->hdc, OBJ_BRUSH ) == GetStockObject( DC_BRUSH ))
    {
        PSDRV_CreateColor( ctx, &ctx->brush.color, color );
        ctx->brush.set = FALSE;
    }
    return color;
}


/**********************************************************************
 *
 *	PSDRV_SetBrush
 *
 */
static BOOL PSDRV_SetBrush( print_ctx *ctx )
{
    LOGBRUSH logbrush;
    BOOL ret = TRUE;

    if (!GetObjectA( GetCurrentObject(ctx->hdc,OBJ_BRUSH), sizeof(logbrush), &logbrush ))
    {
        ERR("Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
        PSDRV_WriteSetColor(ctx, &ctx->brush.color);
	break;

    case BS_NULL:
        break;

    default:
        ret = FALSE;
        break;

    }
    ctx->brush.set = TRUE;
    return ret;
}


/**********************************************************************
 *
 *	PSDRV_Fill
 *
 */
static BOOL PSDRV_Fill(print_ctx *ctx, BOOL EO)
{
    if(!EO)
        return PSDRV_WriteFill(ctx);
    else
        return PSDRV_WriteEOFill(ctx);
}


/**********************************************************************
 *
 *	PSDRV_Clip
 *
 */
static BOOL PSDRV_Clip(print_ctx *ctx, BOOL EO)
{
    if(!EO)
        return PSDRV_WriteClip(ctx);
    else
        return PSDRV_WriteEOClip(ctx);
}

/**********************************************************************
 *
 *	PSDRV_Brush
 *
 */
BOOL PSDRV_Brush(print_ctx *ctx, BOOL EO)
{
    LOGBRUSH logbrush;
    BOOL ret = TRUE;

    if(ctx->pathdepth)
        return FALSE;

    if (!GetObjectA( GetCurrentObject(ctx->hdc,OBJ_BRUSH), sizeof(logbrush), &logbrush ))
    {
        ERR("Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (logbrush.lbStyle) {
    case BS_SOLID:
	PSDRV_WriteGSave(ctx);
        PSDRV_SetBrush(ctx);
        PSDRV_Fill(ctx, EO);
	PSDRV_WriteGRestore(ctx);
	break;

    case BS_HATCHED:
        PSDRV_WriteGSave(ctx);
        PSDRV_SetBrush(ctx);

	switch(logbrush.lbHatch) {
	case HS_VERTICAL:
	case HS_CROSS:
            PSDRV_WriteGSave(ctx);
	    PSDRV_Clip(ctx, EO);
	    PSDRV_WriteHatch(ctx);
	    PSDRV_WriteStroke(ctx);
	    PSDRV_WriteGRestore(ctx);
	    if(logbrush.lbHatch == HS_VERTICAL)
	        break;
	    /* else fallthrough for HS_CROSS */

	case HS_HORIZONTAL:
            PSDRV_WriteGSave(ctx);
	    PSDRV_Clip(ctx, EO);
	    PSDRV_WriteRotate(ctx, 90.0);
	    PSDRV_WriteHatch(ctx);
	    PSDRV_WriteStroke(ctx);
	    PSDRV_WriteGRestore(ctx);
	    break;

	case HS_FDIAGONAL:
	case HS_DIAGCROSS:
	    PSDRV_WriteGSave(ctx);
	    PSDRV_Clip(ctx, EO);
	    PSDRV_WriteRotate(ctx, -45.0);
	    PSDRV_WriteHatch(ctx);
	    PSDRV_WriteStroke(ctx);
	    PSDRV_WriteGRestore(ctx);
	    if(logbrush.lbHatch == HS_FDIAGONAL)
	        break;
	    /* else fallthrough for HS_DIAGCROSS */

	case HS_BDIAGONAL:
	    PSDRV_WriteGSave(ctx);
	    PSDRV_Clip(ctx, EO);
	    PSDRV_WriteRotate(ctx, 45.0);
	    PSDRV_WriteHatch(ctx);
	    PSDRV_WriteStroke(ctx);
	    PSDRV_WriteGRestore(ctx);
	    break;

	default:
	    ERR("Unknown hatch style\n");
	    ret = FALSE;
            break;
	}
        PSDRV_WriteGRestore(ctx);
	break;

    case BS_NULL:
	break;

    case BS_PATTERN:
    case BS_DIBPATTERN:
        if(ctx->pi->ppd->LanguageLevel > 1) {
            PSDRV_WriteGSave(ctx);
            ret = PSDRV_WriteDIBPatternDict(ctx, ctx->brush.pattern.info,
                                            ctx->brush.pattern.bits.ptr, ctx->brush.pattern.usage );
            PSDRV_Fill(ctx, EO);
            PSDRV_WriteGRestore(ctx);
        } else {
            FIXME("Trying to set a pattern brush on a level 1 printer\n");
            ret = FALSE;
	}
	break;

    default:
        ret = FALSE;
	break;
    }
    return ret;
}

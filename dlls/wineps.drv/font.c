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
 *           SelectFont
 */
HFONT PSDRV_SelectFont( print_ctx *ctx, HFONT hfont, UINT *aa_flags )
{
    struct font_info font_info;

    ctx->font.set = UNSET;

    if (ExtEscape(ctx->hdc, PSDRV_GET_BUILTIN_FONT_INFO, 0, NULL,
                sizeof(font_info), (char *)&font_info))
    {
        ctx->font.fontloc = Builtin;
    }
    else
    {
        ctx->font.fontloc = Download;
        ctx->font.fontinfo.Download = NULL;
    }
    return hfont;
}

/***********************************************************************
 *           PSDRV_SetFont
 */
BOOL PSDRV_SetFont( print_ctx *ctx, BOOL vertical )
{
    PSDRV_WriteSetColor(ctx, &ctx->font.color);
    if (vertical && (ctx->font.set == VERTICAL_SET)) return TRUE;
    if (!vertical && (ctx->font.set == HORIZONTAL_SET)) return TRUE;

    switch(ctx->font.fontloc) {
    case Builtin:
        PSDRV_WriteSetBuiltinFont(ctx);
	break;
    case Download:
        PSDRV_WriteSetDownloadFont(ctx, vertical);
	break;
    default:
        ERR("fontloc = %d\n", ctx->font.fontloc);
        assert(1);
	break;
    }
    if (vertical)
        ctx->font.set = VERTICAL_SET;
    else
        ctx->font.set = HORIZONTAL_SET;
    return TRUE;
}

/*
 *	PostScript driver builtin font functions
 *
 *	Copyright 2002  Huw D M Davies for CodeWeavers
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
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winnls.h"
#include "winternl.h"

#include "psdrv.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

BOOL PSDRV_WriteSetBuiltinFont(print_ctx *ctx)
{
    struct font_info font_info;
    matrix size;

    ExtEscape(ctx->hdc, PSDRV_GET_BUILTIN_FONT_INFO, 0, NULL,
            sizeof(font_info), (char *)&font_info);
    size.xx = font_info.size.cx;
    size.yy = font_info.size.cy;
    size.xy = size.yx = 0;
    return PSDRV_WriteSetFont(ctx, font_info.font_name, size,
            font_info.escapement, FALSE);
}

BOOL PSDRV_WriteBuiltinGlyphShow(print_ctx *ctx, LPCWSTR str, INT count)
{
    char name[32];
    int i;

    for (i = 0; i < count; ++i)
    {
        ExtEscape(ctx->hdc, PSDRV_GET_GLYPH_NAME, sizeof(str[i]), (const char *)&str[i], sizeof(name), name);
	PSDRV_WriteGlyphShow(ctx, name);
    }

    return TRUE;
}

/******************************************************************************
 *  	PSDRV_UVMetrics
 *
 *  Find the AFMMETRICS for a given UV.  Returns first glyph in the font
 *  (space?) if the font does not have a glyph for the given UV.
 */
static int __cdecl MetricsByUV(const void *a, const void *b)
{
    return (int)(((const AFMMETRICS *)a)->UV - ((const AFMMETRICS *)b)->UV);
}

const AFMMETRICS *PSDRV_UVMetrics(LONG UV, const AFM *afm)
{
    AFMMETRICS	    	key;
    const AFMMETRICS	*needle;

    /*
     *	Ugly work-around for symbol fonts.  Wine is sending characters which
     *	belong in the Unicode private use range (U+F020 - U+F0FF) as ASCII
     *	characters (U+0020 - U+00FF).
     */

    if ((afm->Metrics->UV & 0xff00) == 0xf000 && UV < 0x100)
    	UV |= 0xf000;

    key.UV = UV;

    needle = bsearch(&key, afm->Metrics, afm->NumofMetrics, sizeof(AFMMETRICS),
	    MetricsByUV);

    if (needle == NULL)
    {
	WARN("No glyph for U+%.4lX in %s\n", UV, afm->FontName);
    	needle = afm->Metrics;
    }

    return needle;
}

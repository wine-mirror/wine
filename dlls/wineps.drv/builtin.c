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

static int __cdecl agl_by_uv(const void *a, const void *b)
{
    return (int)(((const UNICODEGLYPH *)a)->UV - ((const UNICODEGLYPH *)b)->UV);
}

static const char *find_ag_name(WCHAR wch)
{
    UNICODEGLYPH key = { .UV = wch };
    UNICODEGLYPH *needle;

    needle = bsearch(&key, PSDRV_AGLbyUV, PSDRV_AGLbyUVSize, sizeof(key), agl_by_uv);
    return needle ? needle->name->sz : NULL;
}

BOOL PSDRV_WriteBuiltinGlyphShow(print_ctx *ctx, LPCWSTR str, INT count)
{
    const char *name;
    WCHAR wch;
    int i;

    for (i = 0; i < count; ++i)
    {
        ExtEscape(ctx->hdc, PSDRV_CHECK_WCHAR, sizeof(str[i]),
                (const char *)&str[i], sizeof(wch), (char *)&wch);
        name = find_ag_name(wch);
        if (!name)
        {
            ERR("can't find glyph name for %x\n", wch);
            continue;
        }
	PSDRV_WriteGlyphShow(ctx, name);
    }

    return TRUE;
}

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

typedef struct tagRun {
    INT start;
    BOOL vertical;
    INT x;
    INT y;
}Run;

static BOOL PSDRV_Text(print_ctx *ctx, INT x, INT y, UINT flags,
		       LPCWSTR str, UINT count,
		       BOOL bDrawBackground, const INT *lpDx);

extern const unsigned short vertical_orientation_table[];

static BOOL check_unicode_tategaki(WCHAR uchar)
{
    unsigned short orientation = vertical_orientation_table[vertical_orientation_table[vertical_orientation_table[uchar >> 8]+((uchar >> 4) & 0x0f)]+ (uchar & 0xf)];

    /* Type: U or Type: Tu */
    /* TODO Type: Tr,  Normally the logic for Tr would be that if
       Typographical substitution occurs, then do not rotate. However
       we have no facility at present to determine if GetGlyphIndices is
       successfully performing substitutions (well formed font) or not.
       Thus we are erroring on the side of the font being well formed,
       doing typographical substitution,  and so we are not doing rotation */
    return (orientation ==  1 || orientation == 2 || orientation == 3);
}

static Run* build_vertical_runs(print_ctx *ctx, UINT flags, LPCWSTR str, UINT count, INT *run_count)
{
    BOOL last_vert;
    INT start, end;
    INT array_size = 5;
    Run *run = HeapAlloc(GetProcessHeap(),0,sizeof(Run)*array_size);
    int index = 0;
    LOGFONTW lf;

    if (count && str && (!(flags & ETO_GLYPH_INDEX)) && GetObjectW( GetCurrentObject(ctx->hdc, OBJ_FONT), sizeof(lf), &lf ) && (lf.lfFaceName[0] == '@'))
    {
        last_vert = check_unicode_tategaki(str[0]);
        start = end = 0;
        while (start < count)
        {
            int offset = 0;

            while (end < count && check_unicode_tategaki(str[end]) == last_vert)
                end++;

            run[index].start = start;
            run[index].vertical = last_vert;
            run[index].x = 0;
            run[index].y = 0;

            if (run[index].vertical)
            {
                TEXTMETRICW tm;
                GetTextMetricsW(ctx->hdc, &tm);
                offset += PSDRV_XWStoDS(ctx, tm.tmAscent - tm.tmInternalLeading);
            }

            if (start > 0)
            {
                SIZE size;
                GetTextExtentPointW(ctx->hdc, str, start, &size);
                offset += PSDRV_XWStoDS(ctx, size.cx);
            }

            if (offset)
            {
                double angle;
                angle = (lf.lfEscapement / 10.0) * M_PI / 180.0;
                run[index].y = -offset * sin(angle);
                run[index].x = -offset * cos(angle);
            }

            index ++;
            if (index >= array_size)
            {
                array_size *=2;
                run = HeapReAlloc(GetProcessHeap(), 0, run, sizeof(Run)*array_size);
            }
            start = end;
            if (start < count)
                last_vert = check_unicode_tategaki(str[end]);
        }
    }
    else
    {
        run[0].start = 0;
        run[0].vertical = 0;
        run[0].x = 0;
        run[0].y = 0;
        index = 1;
    }
    *run_count = index;
    return run;
}

/***********************************************************************
 *           PSDRV_ExtTextOut
 */
BOOL PSDRV_ExtTextOut( print_ctx *ctx, INT x, INT y, UINT flags, const RECT *lprect, LPCWSTR str, UINT count,
                       const INT *lpDx )
{
    BOOL bResult = TRUE;
    BOOL bClipped = FALSE;
    BOOL bOpaque = FALSE;
    Run *runs = NULL;
    int run_count = 0;
    int i = 0;

    TRACE("(x=%d, y=%d, flags=0x%08x, str=%s, count=%d, lpDx=%p)\n", x, y,
	  flags, debugstr_wn(str, count), count, lpDx);

    if(ctx->job.id == 0) return FALSE;

    runs = build_vertical_runs(ctx, flags, str, count, &run_count);

    /* set draw background */
    if ((flags & ETO_OPAQUE) && (lprect != NULL))
    {
        PSDRV_SetClip(ctx);
        PSDRV_WriteGSave(ctx);
        PSDRV_WriteRectangle(ctx, lprect->left, lprect->top, lprect->right - lprect->left,
                     lprect->bottom - lprect->top);

        bOpaque = TRUE;
        PSDRV_WriteSetColor(ctx, &ctx->bkColor);
        PSDRV_WriteFill(ctx);

        PSDRV_WriteGRestore(ctx);
        PSDRV_ResetClip(ctx);
    }

    while (i < run_count)
    {
        int cnt;

        if (i != run_count - 1)
            cnt = runs[i+1].start- runs[i].start;
        else
            cnt = count - runs[i].start;

        PSDRV_SetFont(ctx, runs[i].vertical);

        PSDRV_SetClip(ctx);

        /* set clipping */
        if ((flags & ETO_CLIPPED) && (lprect != NULL))
        {
            PSDRV_WriteGSave(ctx);

            PSDRV_WriteRectangle(ctx, lprect->left, lprect->top, lprect->right - lprect->left,
                     lprect->bottom - lprect->top);

            bClipped = TRUE;
            PSDRV_WriteClip(ctx);

            bResult = PSDRV_Text(ctx, runs[i].x+x, runs[i].y+y, flags, &str[runs[i].start], cnt, !(bClipped && bOpaque), (lpDx)?&lpDx[runs[i].start]:NULL);

            PSDRV_WriteGRestore(ctx);
        }
        else
            bResult = PSDRV_Text(ctx, runs[i].x+x, runs[i].y+y, flags, &str[runs[i].start], cnt, TRUE, (lpDx)?&lpDx[runs[i].start]:NULL);

        i++;
        PSDRV_ResetClip(ctx);
    }

    HeapFree(GetProcessHeap(),0,runs);
    return bResult;
}

/***********************************************************************
 *           PSDRV_Text
 */
static BOOL PSDRV_Text(print_ctx *ctx, INT x, INT y, UINT flags, LPCWSTR str,
		       UINT count, BOOL bDrawBackground, const INT *lpDx)
{
    WORD *glyphs = NULL;

    if (!count)
	return TRUE;

    if(ctx->font.fontloc == Download && !(flags & ETO_GLYPH_INDEX))
    {
        glyphs = HeapAlloc( GetProcessHeap(), 0, count * sizeof(WORD) );
        GetGlyphIndicesW( ctx->hdc, str, count, glyphs, 0 );
        str = glyphs;
    }

    PSDRV_WriteMoveTo(ctx, x, y);

    if(!lpDx) {
        if(ctx->font.fontloc == Download)
	    PSDRV_WriteDownloadGlyphShow(ctx, str, count);
	else
	    PSDRV_WriteBuiltinGlyphShow(ctx, str, count);
    }
    else {
        UINT i;
	POINT offset = {0, 0};

        for(i = 0; i < count-1; i++) {
	    if(ctx->font.fontloc == Download)
	        PSDRV_WriteDownloadGlyphShow(ctx, str + i, 1);
	    else
	        PSDRV_WriteBuiltinGlyphShow(ctx, str + i, 1);
            if(flags & ETO_PDY)
            {
                offset.x += lpDx[i * 2];
                offset.y += lpDx[i * 2 + 1];
            }
            else
                offset.x += lpDx[i];
	    PSDRV_WriteMoveTo(ctx, x + offset.x, y + offset.y);
	}
	if(ctx->font.fontloc == Download)
	    PSDRV_WriteDownloadGlyphShow(ctx, str + i, 1);
	else
	    PSDRV_WriteBuiltinGlyphShow(ctx, str + i, 1);
    }

    HeapFree( GetProcessHeap(), 0, glyphs );
    return TRUE;
}

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

static BOOL PSDRV_Text(PHYSDEV dev, INT x, INT y, UINT flags,
		       LPCWSTR str, UINT count,
		       BOOL bDrawBackground, const INT *lpDx);

static const struct { WCHAR lower; WCHAR upper;} unrotate_ranges[] =
    {
        {0x0000, 0x10FF},
        /* Hangul Jamo */
        {0x1200, 0x17FF},
        /* Mongolian  */
        {0x18B0, 0x1FFF},
        /* General Punctuation */
        {0x2070, 0x209F},
        /* Currency Symbols */
        /* Combining Diacritical Marks for Symbols */
        /* Letterlike Symbols */
        {0x2150, 0x245F},
        /* Enclosed Alphanumerics */
        {0x2500, 0x259F},
        /* Geometric Shapes */
        /* Miscellaneous Symbols */
        /* Dingbats */
        /* Miscellaneous Mathematical Symbols-A */
        /* Supplemental Arrows-A */
        {0x2800, 0x2E7F},
        /* East Asian scripts and symbols */
        {0xA000, 0xABFF},
        /* Hangul Syllables */
        /* Hangul Jamo Extended-B */
        {0xD800, 0xF8FF},
        /* CJK Compatibility Ideographs */
        {0xFB00, 0xFE0F},
        /* Vertical Forms */
        /* Combining Half Marks */
        /* CJK Compatibility Forms */
        {0xFE50, 0xFEFF},
        /* Halfwidth and Fullwidth Forms  */
        {0xFFEF, 0xFFFF},
    };

static BOOL check_unicode_tategaki(WCHAR uchar)
{
    int i;
    for (i = 0 ;; i++)
    {
        if (uchar < unrotate_ranges[i].lower)
            return TRUE;

        if (uchar >= unrotate_ranges[i].lower && uchar  <= unrotate_ranges[i].upper)
            return FALSE;
    }
}

static Run* build_vertical_runs(PHYSDEV dev, UINT flags, LPCWSTR str, UINT count, INT *run_count)
{
    BOOL last_vert = check_unicode_tategaki(str[0]);
    INT start, end;
    INT array_size = 5;
    Run *run = HeapAlloc(GetProcessHeap(),0,sizeof(Run)*array_size);
    int index = 0;
    LOGFONTW lf;

    if ((!(flags & ETO_GLYPH_INDEX)) && GetObjectW( GetCurrentObject(dev->hdc, OBJ_FONT), sizeof(lf), &lf ) && (lf.lfFaceName[0] == '@'))
    {
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
                GetTextMetricsW(dev->hdc, &tm);
                offset += PSDRV_XWStoDS(dev, tm.tmAscent - tm.tmInternalLeading);
            }

            if (start > 0)
            {
                SIZE size;
                GetTextExtentPointW(dev->hdc, str, start, &size);
                offset += PSDRV_XWStoDS(dev, size.cx);
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
BOOL PSDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect, LPCWSTR str, UINT count,
                       const INT *lpDx )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    BOOL bResult = TRUE;
    BOOL bClipped = FALSE;
    BOOL bOpaque = FALSE;
    Run *runs = NULL;
    int run_count = 0;
    int i = 0;

    TRACE("(x=%d, y=%d, flags=0x%08x, str=%s, count=%d, lpDx=%p)\n", x, y,
	  flags, debugstr_wn(str, count), count, lpDx);

    if(physDev->job.id == 0) return FALSE;

    runs = build_vertical_runs(dev, flags, str, count, &run_count);

    /* set draw background */
    if ((flags & ETO_OPAQUE) && (lprect != NULL))
    {
        PSDRV_SetClip(dev);
        PSDRV_WriteGSave(dev);
        PSDRV_WriteRectangle(dev, lprect->left, lprect->top, lprect->right - lprect->left,
                     lprect->bottom - lprect->top);

        bOpaque = TRUE;
        PSDRV_WriteSetColor(dev, &physDev->bkColor);
        PSDRV_WriteFill(dev);

        PSDRV_WriteGRestore(dev);
        PSDRV_ResetClip(dev);
    }

    while (i < run_count)
    {
        int cnt;

        if (i != run_count - 1)
            cnt = runs[i+1].start- runs[i].start;
        else
            cnt = count - runs[i].start;

        PSDRV_SetFont(dev, runs[i].vertical);

        PSDRV_SetClip(dev);

        /* set clipping */
        if ((flags & ETO_CLIPPED) && (lprect != NULL))
        {
            PSDRV_WriteGSave(dev);

            PSDRV_WriteRectangle(dev, lprect->left, lprect->top, lprect->right - lprect->left,
                     lprect->bottom - lprect->top);

            bClipped = TRUE;
            PSDRV_WriteClip(dev);

            bResult = PSDRV_Text(dev, runs[i].x+x, runs[i].y+y, flags, &str[runs[i].start], cnt, !(bClipped && bOpaque), (lpDx)?&lpDx[runs[i].start]:NULL);

            PSDRV_WriteGRestore(dev);
        }
        else
            bResult = PSDRV_Text(dev, runs[i].x+x, runs[i].y+y, flags, &str[runs[i].start], cnt, TRUE, (lpDx)?&lpDx[runs[i].start]:NULL);

        i++;
        PSDRV_ResetClip(dev);
    }

    HeapFree(GetProcessHeap(),0,runs);
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

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <string.h>
#include "gdi.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static BOOL PSDRV_Text(PSDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
		       LPCWSTR str, UINT count,
		       BOOL bDrawBackground, const INT *lpDx);

/***********************************************************************
 *           PSDRV_ExtTextOut
 */
BOOL PSDRV_ExtTextOut( PSDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
		       const RECT *lprect, LPCWSTR str, UINT count,
		       const INT *lpDx )
{
    BOOL bResult = TRUE;
    BOOL bClipped = FALSE;
    BOOL bOpaque = FALSE;
    RECT rect;

    TRACE("(x=%d, y=%d, flags=0x%08x, str=%s, count=%d, lpDx=%p)\n", x, y,
	  flags, debugstr_wn(str, count), count, lpDx);

    /* write font if not already written */
    PSDRV_SetFont(physDev);

    PSDRV_SetClip(physDev);

    /* set clipping and/or draw background */
    if ((flags & (ETO_CLIPPED | ETO_OPAQUE)) && (lprect != NULL))
    {
        rect = *lprect;
        LPtoDP( physDev->hdc, (POINT *)&rect, 2 );
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteRectangle(physDev, rect.left, rect.top, rect.right - rect.left,
			     rect.bottom - rect.top);

	if (flags & ETO_OPAQUE)
	{
	    bOpaque = TRUE;
	    PSDRV_WriteGSave(physDev);
	    PSDRV_WriteSetColor(physDev, &physDev->bkColor);
	    PSDRV_WriteFill(physDev);
	    PSDRV_WriteGRestore(physDev);
	}

	if (flags & ETO_CLIPPED)
	{
	    bClipped = TRUE;
	    PSDRV_WriteClip(physDev);
	}

	bResult = PSDRV_Text(physDev, x, y, flags, str, count, !(bClipped && bOpaque), lpDx);
	PSDRV_WriteGRestore(physDev);
    }
    else
    {
	bResult = PSDRV_Text(physDev, x, y, flags, str, count, TRUE, lpDx);
    }

    PSDRV_ResetClip(physDev);
    return bResult;
}

/***********************************************************************
 *           PSDRV_Text
 */
static BOOL PSDRV_Text(PSDRV_PDEVICE *physDev, INT x, INT y, UINT flags, LPCWSTR str,
		       UINT count, BOOL bDrawBackground, const INT *lpDx)
{
    SIZE sz;
    TEXTMETRICW tm;
    POINT pt;
    INT ascent, descent;
    WORD *glyphs = NULL;
    DC *dc = physDev->dc;
    UINT align = GetTextAlign( physDev->hdc );
    INT char_extra;
    INT *deltas = NULL;
    double cosEsc, sinEsc;
    LOGFONTW lf;

    if (!count)
	return TRUE;

    GetObjectW(GetCurrentObject(physDev->hdc, OBJ_FONT), sizeof(lf), &lf);
    if(lf.lfEscapement != 0) {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    } else {
        cosEsc = 1;
        sinEsc = 0;
    }

    if(physDev->font.fontloc == Download) {
        if(flags & ETO_GLYPH_INDEX)
	    glyphs = (LPWORD)str;
	else {
	    glyphs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WORD));
	    GetGlyphIndicesW(physDev->hdc, str, count, glyphs, 0);
	}
    }

    if(align & TA_UPDATECP) {
	x = dc->CursPosX;
	y = dc->CursPosY;
    }

    pt.x = x;
    pt.y = y;
    LPtoDP(physDev->hdc, &pt, 1);
    x = pt.x;
    y = pt.y;

    if(physDev->font.fontloc == Download)
        GetTextExtentPointI(physDev->hdc, glyphs, count, &sz);
    else
        GetTextExtentPoint32W(physDev->hdc, str, count, &sz);

    if((char_extra = GetTextCharacterExtra(physDev->hdc)) != 0) {
        INT i;
	SIZE tmpsz;

        deltas = HeapAlloc(GetProcessHeap(), 0, count * sizeof(INT));
	for(i = 0; i < count; i++) {
	    deltas[i] = char_extra;
	    if(lpDx)
	        deltas[i] += lpDx[i];
	    else {
	        if(physDev->font.fontloc == Download)
		    GetTextExtentPointI(physDev->hdc, glyphs + i, 1, &tmpsz);
		else
		    GetTextExtentPoint32W(physDev->hdc, str + i, 1, &tmpsz);
	        deltas[i] += tmpsz.cx;
	    }
	}
    } else if(lpDx)
        deltas = (INT*)lpDx;

    if(deltas) {
        SIZE tmpsz;
	INT i;
	/* Get the width of the last char and add on all the offsets */
	if(physDev->font.fontloc == Download)
	    GetTextExtentPointI(physDev->hdc, glyphs + count - 1, 1, &tmpsz);
	else
	    GetTextExtentPoint32W(physDev->hdc, str + count - 1, 1, &tmpsz);
	for(i = 0; i < count-1; i++)
	    tmpsz.cx += deltas[i];
	sz.cx = tmpsz.cx; /* sz.cy remains untouched */
    }

    sz.cx = INTERNAL_XWSTODS(dc, sz.cx);
    sz.cy = INTERNAL_YWSTODS(dc, sz.cy);

    GetTextMetricsW(physDev->hdc, &tm);
    ascent = INTERNAL_YWSTODS(dc, tm.tmAscent);
    descent = INTERNAL_YWSTODS(dc, tm.tmDescent);

    TRACE("textAlign = %x\n", align);
    switch(align & (TA_LEFT | TA_CENTER | TA_RIGHT) ) {
    case TA_LEFT:
        if(align & TA_UPDATECP)
        {
            POINT pt;
            pt.x = x + sz.cx * cosEsc;
            pt.y = y - sz.cx * sinEsc;
            DPtoLP( physDev->hdc, &pt, 1 );
            dc->CursPosX = pt.x;
            dc->CursPosY = pt.y;
	}
	break;

    case TA_CENTER:
        x -= sz.cx * cosEsc / 2;
        y += sz.cx * sinEsc / 2;
	break;

    case TA_RIGHT:
        x -= sz.cx * cosEsc;
        y += sz.cx * sinEsc;
	if(align & TA_UPDATECP)
        {
            POINT pt;
            pt.x = x;
            pt.y = y;
            DPtoLP( physDev->hdc, &pt, 1 );
            dc->CursPosX = pt.x;
            dc->CursPosY = pt.y;
	}
	break;
    }

    switch(align & (TA_TOP | TA_BASELINE | TA_BOTTOM) ) {
    case TA_TOP:
        y += ascent * cosEsc;
        x += ascent * sinEsc;
	break;

    case TA_BASELINE:
	break;

    case TA_BOTTOM:
        y -= descent * cosEsc;
        x -= descent * sinEsc;
	break;
    }

    if ((GetBkMode( physDev->hdc ) != TRANSPARENT) && bDrawBackground)
    {
	PSDRV_WriteGSave(physDev);
	PSDRV_WriteNewPath(physDev);
	PSDRV_WriteRectangle(physDev, x, y - ascent, sz.cx,
			     ascent + descent);
	PSDRV_WriteSetColor(physDev, &physDev->bkColor);
	PSDRV_WriteFill(physDev);
	PSDRV_WriteGRestore(physDev);
    }

    PSDRV_WriteMoveTo(physDev, x, y);

    if(!deltas) {
        if(physDev->font.fontloc == Download)
	    PSDRV_WriteDownloadGlyphShow(physDev, glyphs, count);
	else
	    PSDRV_WriteBuiltinGlyphShow(physDev, str, count);
    }
    else {
        INT i;
	float dx = 0.0, dy = 0.0;
	float cos_theta = cos(physDev->font.escapement * M_PI / 1800.0);
	float sin_theta = sin(physDev->font.escapement * M_PI / 1800.0);
        for(i = 0; i < count-1; i++) {
	    TRACE("lpDx[%d] = %d\n", i, deltas[i]);
	    if(physDev->font.fontloc == Download)
	        PSDRV_WriteDownloadGlyphShow(physDev, glyphs + i, 1);
	    else
	        PSDRV_WriteBuiltinGlyphShow(physDev, str + i, 1);
	    dx += deltas[i] * cos_theta;
	    dy -= deltas[i] * sin_theta;
	    PSDRV_WriteMoveTo(physDev, x + INTERNAL_XWSTODS(dc, dx),
			      y + INTERNAL_YWSTODS(dc, dy));
	}
	if(physDev->font.fontloc == Download)
	    PSDRV_WriteDownloadGlyphShow(physDev, glyphs + i, 1);
	else
	    PSDRV_WriteBuiltinGlyphShow(physDev, str + i, 1);
	if(deltas != lpDx)
	    HeapFree(GetProcessHeap(), 0, deltas);
    }

    /*
     * Underline and strikeout attributes.
     */
    if ((tm.tmUnderlined) || (tm.tmStruckOut)) {

        /* Get the thickness and the position for the underline attribute */
        /* We'll use the same thickness for the strikeout attribute       */

        INT escapement =  physDev->font.escapement;

        /* Do the underline */

        if (tm.tmUnderlined) {
            PSDRV_WriteNewPath(physDev); /* will be closed by WriteRectangle */
            if (escapement != 0)  /* rotated text */
            {
                PSDRV_WriteGSave(physDev);  /* save the graphics state */
                PSDRV_WriteMoveTo(physDev, x, y); /* move to the start */

                /* temporarily rotate the coord system */
                PSDRV_WriteRotate(physDev, -escapement/10);

                /* draw the underline relative to the starting point */
                PSDRV_WriteRRectangle(physDev, 0, -physDev->font.underlinePosition,
				      sz.cx, physDev->font.underlineThickness);
            }
            else
                PSDRV_WriteRectangle(physDev, x, y - physDev->font.underlinePosition,
				     sz.cx, physDev->font.underlineThickness);

            PSDRV_WriteFill(physDev);

            if (escapement != 0)  /* rotated text */
                PSDRV_WriteGRestore(physDev);  /* restore the graphics state */
        }

        /* Do the strikeout */

        if (tm.tmStruckOut) {
            PSDRV_WriteNewPath(physDev); /* will be closed by WriteRectangle */
            if (escapement != 0)  /* rotated text */
            {
                PSDRV_WriteGSave(physDev);  /* save the graphics state */
                PSDRV_WriteMoveTo(physDev, x, y); /* move to the start */

                /* temporarily rotate the coord system */
                PSDRV_WriteRotate(physDev, -escapement/10);

                /* draw the line relative to the starting point */
                PSDRV_WriteRRectangle(physDev, 0, -physDev->font.strikeoutPosition,
				      sz.cx, physDev->font.strikeoutThickness);
            }
            else
                PSDRV_WriteRectangle(physDev, x, y - physDev->font.strikeoutPosition,
				     sz.cx, physDev->font.strikeoutThickness);

            PSDRV_WriteFill(physDev);

            if (escapement != 0)  /* rotated text */
                PSDRV_WriteGRestore(physDev);  /* restore the graphics state */
        }
    }

    if(glyphs && glyphs != str) HeapFree(GetProcessHeap(), 0, glyphs);
    return TRUE;
}

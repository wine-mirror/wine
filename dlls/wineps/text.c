/*
 *	PostScript driver text functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include "psdrv.h"
#include "debugtools.h"
#include "winspool.h"

DEFAULT_DEBUG_CHANNEL(psdrv);

static BOOL PSDRV_Text(DC *dc, INT x, INT y, LPCWSTR str, UINT count,
		       BOOL bDrawBackground, const INT *lpDx);

/***********************************************************************
 *           PSDRV_ExtTextOut
 */
BOOL PSDRV_ExtTextOut( DC *dc, INT x, INT y, UINT flags,
		       const RECT *lprect, LPCWSTR str, UINT count,
		       const INT *lpDx )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    BOOL bResult = TRUE;
    BOOL bClipped = FALSE;
    BOOL bOpaque = FALSE;
    RECT rect;

    TRACE("(x=%d, y=%d, flags=0x%08x, str=%s, count=%d, lpDx=%p)\n", x, y,
	  flags, debugstr_wn(str, count), count, lpDx);

    /* write font if not already written */
    PSDRV_SetFont(dc);

    /* set clipping and/or draw background */
    if ((flags & (ETO_CLIPPED | ETO_OPAQUE)) && (lprect != NULL))
    {
	rect.left = INTERNAL_XWPTODP(dc, lprect->left, lprect->top);
	rect.right = INTERNAL_XWPTODP(dc, lprect->right, lprect->bottom);
	rect.top = INTERNAL_YWPTODP(dc, lprect->left, lprect->top);
	rect.bottom = INTERNAL_YWPTODP(dc, lprect->right, lprect->bottom);

	PSDRV_WriteGSave(dc);
	PSDRV_WriteRectangle(dc, rect.left, rect.top, rect.right - rect.left, 
			     rect.bottom - rect.top);

	if (flags & ETO_OPAQUE)
	{
	    bOpaque = TRUE;
	    PSDRV_WriteGSave(dc);
	    PSDRV_WriteSetColor(dc, &physDev->bkColor);
	    PSDRV_WriteFill(dc);
	    PSDRV_WriteGRestore(dc);
	}

	if (flags & ETO_CLIPPED)
	{
	    bClipped = TRUE;
	    PSDRV_WriteClip(dc);
	}

	bResult = PSDRV_Text(dc, x, y, str, count, !(bClipped && bOpaque), lpDx); 
	PSDRV_WriteGRestore(dc);
    }
    else
    {
	bResult = PSDRV_Text(dc, x, y, str, count, TRUE, lpDx); 
    }

    return bResult;
}

/***********************************************************************
 *           PSDRV_Text
 */
static BOOL PSDRV_Text(DC *dc, INT x, INT y, LPCWSTR str, UINT count,
		       BOOL bDrawBackground, const INT *lpDx)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    LPWSTR strbuf;
    SIZE sz;

    strbuf = HeapAlloc( PSDRV_Heap, 0, (count + 1) * sizeof(WCHAR));
    if(!strbuf) {
        WARN("HeapAlloc failed\n");
        return FALSE;
    }

    if(dc->textAlign & TA_UPDATECP) {
	x = dc->CursPosX;
	y = dc->CursPosY;
    }

    x = INTERNAL_XWPTODP(dc, x, y);
    y = INTERNAL_YWPTODP(dc, x, y);

    GetTextExtentPoint32W(dc->hSelf, str, count, &sz);
    if(lpDx) {
        SIZE tmpsz;
	INT i;
	/* Get the width of the last char and add on all the offsets */
	GetTextExtentPoint32W(dc->hSelf, str + count - 1, 1, &tmpsz);
	for(i = 0; i < count-1; i++)
	    tmpsz.cx += lpDx[i];
	sz.cx = tmpsz.cx; /* sz.cy remains untouched */
    }

    sz.cx = INTERNAL_XWSTODS(dc, sz.cx);
    sz.cy = INTERNAL_YWSTODS(dc, sz.cy);
    TRACE("textAlign = %x\n", dc->textAlign);
    switch(dc->textAlign & (TA_LEFT | TA_CENTER | TA_RIGHT) ) {
    case TA_LEFT:
        if(dc->textAlign & TA_UPDATECP) {
	    dc->CursPosX = INTERNAL_XDPTOWP(dc, x + sz.cx, y);
	}
	break;

    case TA_CENTER:
	x -= sz.cx/2;
	break;

    case TA_RIGHT:
	x -= sz.cx;
	if(dc->textAlign & TA_UPDATECP) {
	    dc->CursPosX = INTERNAL_XDPTOWP(dc, x, y);
	}
	break;
    }

    switch(dc->textAlign & (TA_TOP | TA_BASELINE | TA_BOTTOM) ) {
    case TA_TOP:
        y += physDev->font.tm.tmAscent;
	break;

    case TA_BASELINE:
	break;

    case TA_BOTTOM:
        y -= physDev->font.tm.tmDescent;
	break;
    }

    memcpy(strbuf, str, count * sizeof(WCHAR));
    *(strbuf + count) = '\0';
    
    if ((dc->backgroundMode != TRANSPARENT) && (bDrawBackground != FALSE))
    {
	PSDRV_WriteGSave(dc);
	PSDRV_WriteNewPath(dc);
	PSDRV_WriteRectangle(dc, x, y - physDev->font.tm.tmAscent, sz.cx, 
			     physDev->font.tm.tmAscent + 
			     physDev->font.tm.tmDescent);
	PSDRV_WriteSetColor(dc, &physDev->bkColor);
	PSDRV_WriteFill(dc);
	PSDRV_WriteGRestore(dc);
    }

    PSDRV_WriteMoveTo(dc, x, y);
    
    if(!lpDx)
        PSDRV_WriteShow(dc, strbuf, lstrlenW(strbuf));
    else {
        INT i;
	float dx = 0.0, dy = 0.0;
	float cos_theta = cos(physDev->font.escapement * M_PI / 1800.0);
	float sin_theta = sin(physDev->font.escapement * M_PI / 1800.0);
        for(i = 0; i < count-1; i++) {
	    TRACE("lpDx[%d] = %d\n", i, lpDx[i]);
	    PSDRV_WriteShow(dc, &strbuf[i], 1);
	    dx += lpDx[i] * cos_theta;
	    dy -= lpDx[i] * sin_theta;
	    PSDRV_WriteMoveTo(dc, x + INTERNAL_XWSTODS(dc, dx),
			      y + INTERNAL_YWSTODS(dc, dy));
	}
	PSDRV_WriteShow(dc, &strbuf[i], 1);
    }

    /*
     * Underline and strikeout attributes.
     */
    if ((physDev->font.tm.tmUnderlined) || (physDev->font.tm.tmStruckOut)) {

        /* Get the thickness and the position for the underline attribute */
        /* We'll use the same thickness for the strikeout attribute       */

        float thick = physDev->font.afm->UnderlineThickness * physDev->font.scale;
        float pos   = -physDev->font.afm->UnderlinePosition * physDev->font.scale;
        SIZE size;
        INT escapement =  physDev->font.escapement;

        TRACE("Position = %f Thickness %f Escapement %d\n",
              pos, thick, escapement);

        /* Get the width of the text */

        PSDRV_GetTextExtentPoint(dc, strbuf, lstrlenW(strbuf), &size);
        size.cx = INTERNAL_XWSTODS(dc, size.cx);

        /* Do the underline */

        if (physDev->font.tm.tmUnderlined) {
            if (escapement != 0)  /* rotated text */
            {
                PSDRV_WriteGSave(dc);  /* save the graphics state */
                PSDRV_WriteMoveTo(dc, x, y); /* move to the start */

                /* temporarily rotate the coord system */
                PSDRV_WriteRotate(dc, -escapement/10); 
                
                /* draw the underline relative to the starting point */
                PSDRV_WriteRRectangle(dc, 0, (INT)pos, size.cx, (INT)thick);
            }
            else
                PSDRV_WriteRectangle(dc, x, y + (INT)pos, size.cx, (INT)thick);

            PSDRV_WriteFill(dc);

            if (escapement != 0)  /* rotated text */
                PSDRV_WriteGRestore(dc);  /* restore the graphics state */
        }

        /* Do the strikeout */

        if (physDev->font.tm.tmStruckOut) {
            pos = -physDev->font.tm.tmAscent / 2;

            if (escapement != 0)  /* rotated text */
            {
                PSDRV_WriteGSave(dc);  /* save the graphics state */
                PSDRV_WriteMoveTo(dc, x, y); /* move to the start */

                /* temporarily rotate the coord system */
                PSDRV_WriteRotate(dc, -escapement/10);

                /* draw the underline relative to the starting point */
                PSDRV_WriteRRectangle(dc, 0, (INT)pos, size.cx, (INT)thick);
            }
            else
                PSDRV_WriteRectangle(dc, x, y + (INT)pos, size.cx, (INT)thick);

            PSDRV_WriteFill(dc);

            if (escapement != 0)  /* rotated text */
                PSDRV_WriteGRestore(dc);  /* restore the graphics state */
        }
    }

    HeapFree(PSDRV_Heap, 0, strbuf);
    return TRUE;
}

/*
 *	PostScript driver text functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include "windows.h"
#include "psdrv.h"
#include "debug.h"
#include "print.h"

/***********************************************************************
 *           PSDRV_ExtTextOut
 */
BOOL32 PSDRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
                   const RECT32 *lprect, LPCSTR str, UINT32 count,
                   const INT32 *lpDx )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *strbuf;
    SIZE32 sz;

    TRACE(psdrv, "(x=%d, y=%d, flags=0x%08x, str='%.*s', count=%d)\n", x, y,
	  flags, count, str, count);

    strbuf = (char *)HeapAlloc( PSDRV_Heap, 0, count + 1);
    if(!strbuf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return FALSE;
    }

    if(dc->w.textAlign & TA_UPDATECP) {
	x = dc->w.CursPosX;
	y = dc->w.CursPosY;
    }

    x = XLPTODP(dc, x);
    y = YLPTODP(dc, y);

    GetTextExtentPoint32A(dc->hSelf, str, count, &sz);
    sz.cx = XLSTODS(dc, sz.cx);
    sz.cy = YLSTODS(dc, sz.cy);

    switch(dc->w.textAlign & (TA_LEFT | TA_CENTER | TA_RIGHT) ) {
    case TA_LEFT:
        if(dc->w.textAlign & TA_UPDATECP)
	    dc->w.CursPosX = XDPTOLP(dc, x + sz.cx);
	break;

    case TA_CENTER:
	x -= sz.cx/2;
	break;

    case TA_RIGHT:
	x -= sz.cx;
	if(dc->w.textAlign & TA_UPDATECP)
	    dc->w.CursPosX = XDPTOLP(dc, x);
	break;
    }

    switch(dc->w.textAlign & (TA_TOP | TA_BASELINE | TA_BOTTOM) ) {
    case TA_TOP:
        y += physDev->font.tm.tmAscent;
	break;

    case TA_BASELINE:
	break;

    case TA_BOTTOM:
        y -= physDev->font.tm.tmDescent;
	break;
    }

    memcpy(strbuf, str, count);
    *(strbuf + count) = '\0';
    
    PSDRV_SetFont(dc);

    PSDRV_WriteMoveTo(dc, x, y);
    PSDRV_WriteShow(dc, strbuf, strlen(strbuf));

    HeapFree(PSDRV_Heap, 0, strbuf);
    return TRUE;
}

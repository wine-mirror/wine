/*
 * win16 driver text functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
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

#include <stdlib.h>
#include "win16drv/win16drv.h"
#include "gdi.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16drv);

/***********************************************************************
 *           WIN16DRV_ExtTextOut
 */
BOOL WIN16DRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                           const RECT *lprect, LPCWSTR wstr, UINT count,
                           const INT *lpDx )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    BOOL bRet = 1;
    RECT16	 clipRect;
    RECT16 	 opaqueRect;
    RECT16 	*lpOpaqueRect = NULL;
    WORD wOptions = 0;
    DWORD len;
    POINT pt;
    INT16 width;
    char *str;
    DWORD dwRet;

    if (count == 0)
        return FALSE;

    TRACE("%p %d %d %x %p %s %p\n",
	  dc->hSelf, x, y, flags, lprect, debugstr_wn(wstr, count), lpDx);

    len = WideCharToMultiByte( CP_ACP, 0, wstr, count, NULL, 0, NULL, NULL );
    str = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, wstr, count, str, len, NULL, NULL );

    clipRect.left = 0;
    clipRect.top = 0;

    clipRect.right = physDev->DevCaps.horzRes;
    clipRect.bottom = physDev->DevCaps.vertRes;
    if (lprect) {
	opaqueRect.left = lprect->left;
	opaqueRect.top = lprect->top;
	opaqueRect.right = lprect->right;
	opaqueRect.bottom = lprect->bottom;
	lpOpaqueRect = &opaqueRect;
    }

    TRACE("textalign = %d\n", dc->textAlign);

    if (dc->textAlign & TA_UPDATECP) {
        x = dc->CursPosX;
	y = dc->CursPosY;
    }

    pt.x = x;
    pt.y = y;
    LPtoDP( physDev->hdc, &pt, 1 );
    x = pt.x;
    y = pt.y;

    dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0,
			      NULL, str, -len,  physDev->FontInfo,
			      win16drv_SegPtr_DrawMode,
			      win16drv_SegPtr_TextXForm,
			      NULL, NULL, 0);

    width = LOWORD(dwRet);

    switch( dc->textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) ) {
    case TA_LEFT:
        if (dc->textAlign & TA_UPDATECP)
        {
            pt.x = x + width;
            pt.y = y;
            DPtoLP( physDev->hdc, &pt, 1 );
            dc->CursPosX = pt.x;
        }
	break;
    case TA_RIGHT:
        x -= width;
        if (dc->textAlign & TA_UPDATECP)
        {
            pt.x = x;
            pt.y = y;
            DPtoLP( physDev->hdc, &pt, 1 );
            dc->CursPosX = pt.x;
        }
	break;
    case TA_CENTER:
        x -= width / 2;
	break;
    }

    switch( dc->textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE) ) {
    case TA_TOP:
        break;
    case TA_BOTTOM:
        y -= physDev->FontInfo->dfPixHeight;
	break;
    case TA_BASELINE:
        y -= physDev->FontInfo->dfAscent;
	break;
    }

    dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE,
			      x, y, &clipRect, str, (WORD)len,
			      physDev->FontInfo, win16drv_SegPtr_DrawMode,
			      win16drv_SegPtr_TextXForm, NULL, lpOpaqueRect,
			      wOptions);

    HeapFree( GetProcessHeap(), 0, str );
    return bRet;
}

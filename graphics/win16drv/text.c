/*
 * win16 driver text functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <stdlib.h>
#include "windows.h"
#include "win16drv.h"
#include "dc.h"
#include "gdi.h"
#include "debug.h"

/***********************************************************************
 *           WIN16DRV_ExtTextOut
 */
BOOL32 WIN16DRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
                           const RECT32 *lprect, LPCSTR str, UINT32 count,
                           const INT32 *lpDx )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL32 bRet = 1;
    RECT16	 clipRect;
    RECT16 	 opaqueRect;
    RECT16 	*lpOpaqueRect = NULL; 
    WORD wOptions = 0;
    WORD wCount = count;
    INT16 width;

    if (count == 0)
      return FALSE;

    dprintf_info(win16drv, "WIN16DRV_ExtTextOut: %04x %d %d %x %p %*s %p\n",
	   dc->hSelf, x, y, flags,  lprect, count > 0 ? count : 8, str, lpDx);


    if (dc != NULL)   
    {
	DWORD dwRet;

	clipRect.left = 0;
	clipRect.top = 0;
        
	clipRect.right = dc->w.devCaps->horzRes;
	clipRect.bottom = dc->w.devCaps->vertRes;
        if (lprect)
        {
            opaqueRect.left = lprect->left;
            opaqueRect.top = lprect->top;
            opaqueRect.right = lprect->right;
            opaqueRect.bottom = lprect->bottom;
            lpOpaqueRect = &opaqueRect;
            
        }
        
	dprintf_info(win16drv, "textalign = %d\n", dc->w.textAlign);

	if (dc->w.textAlign & TA_UPDATECP)
	{
	    x = dc->w.CursPosX;
	    y = dc->w.CursPosY;
	}

	x = XLPTODP( dc, x );
	y = YLPTODP( dc, y );

	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
		 NULL, str, -count,  physDev->FontInfo, 
		 win16drv_SegPtr_DrawMode, win16drv_SegPtr_TextXForm,
		 NULL, NULL, 0);

	width = LOWORD(dwRet);

	switch( dc->w.textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER) )
	{
	case TA_LEFT:
 	    if (dc->w.textAlign & TA_UPDATECP)
	        dc->w.CursPosX = XDPTOLP( dc, x + width );
	    break;
	case TA_RIGHT:
	     x -= width;
	     if (dc->w.textAlign & TA_UPDATECP)
	         dc->w.CursPosX = XDPTOLP( dc, x );
	     break;
	case TA_CENTER:
	    x -= width / 2;
	    break;
	}

	switch( dc->w.textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
	{
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
	      x, y, &clipRect, str, wCount,
	      physDev->FontInfo, win16drv_SegPtr_DrawMode, 
	      win16drv_SegPtr_TextXForm, NULL, lpOpaqueRect, wOptions);
    }
    return bRet;
}










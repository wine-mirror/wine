/*
 * win16 driver text functions
 *
 * Copyright 1996 John Harvey
 */

#include <stdlib.h>
#include "windows.h"
#include "win16drv.h"
#include "dc.h"
#include "gdi.h"
#include "stddebug.h"
/* #define DEBUG_WIN16DRV */
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

    static BOOL32 bInit = FALSE;
    


    if (count == 0)
      return FALSE;

    dprintf_win16drv(stddeb, "WIN16DRV_ExtTextOut: %04x %d %d %x %p %*s %p\n", dc->hSelf, x, y, 
	   flags,  lprect, count > 0 ? count : 8, str, lpDx);


    if (bInit == FALSE)
    {
	DWORD dwRet;

	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
				  NULL, " ", 
				  -1,  physDev->segptrFontInfo, win16drv_SegPtr_DrawMode, 
				  win16drv_SegPtr_TextXForm, NULL, NULL, 0);
	bInit = TRUE;
    }

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
        
#ifdef NOTDEF
    {
        RECT16 rcPageSize;
	FONTINFO16 *p = (FONTINFO16 *)PTR_SEG_TO_LIN(physDev->segptrFontInfo);
        rcPageSize.left = 0;
        rcPageSize.right = 0x3c0;
        
        rcPageSize.top = 0;
        rcPageSize.bottom = 0x630;
        



        if(y < rcPageSize.top  ||  y + p->dfPixHeight > rcPageSize.bottom)
        {
            printf("Failed 1 y %d top %d y +height %d bottom %d\n",
                   y, rcPageSize.top  ,  y + p->dfPixHeight , rcPageSize.bottom);
        }
        

        if(x >= rcPageSize.right  ||
            x + wCount * p->dfPixWidth < rcPageSize.left)
        {
            printf("Faile 2\n");
        }
        
    }
#endif        

	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, XLPTODP(dc,x), YLPTODP(dc,y), 
				  &clipRect, str, 
				  wCount,  physDev->segptrFontInfo, win16drv_SegPtr_DrawMode, 
				  win16drv_SegPtr_TextXForm, NULL, lpOpaqueRect, wOptions);
    }
    return bRet;
}

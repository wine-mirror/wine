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
    extern DRAWMODE DrawMode;
    LPDRAWMODE lpDrawMode = &DrawMode;
    TEXTXFORM16 TextXForm;
    LPTEXTXFORM16 lpTextXForm = &TextXForm;
    RECT16 rcClipRect;
    RECT16 * lpClipRect = &rcClipRect;
    RECT16 rcOpaqueRect;
    RECT16 *lpOpaqueRect = &rcOpaqueRect;
    WORD wOptions = 0;
    WORD wCount = count;

    static BOOL32 bInit = FALSE;
    


    if (count == 0)
      return FALSE;

    dprintf_win16drv(stddeb, "WIN16DRV_ExtTextOut: %04x %d %d %x %p %*s %p\n", dc->hSelf, x, y, 
	   flags,  lprect, count > 0 ? count : 8, str, lpDx);

    InitTextXForm(lpTextXForm);

    if (bInit == FALSE)
    {
	DWORD dwRet;

	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
				  NULL, " ", 
				  -1,  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, NULL, NULL, 0);
	bInit = TRUE;
    }

    if (dc != NULL)   
    {
	DWORD dwRet;
/*
	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
				  NULL, "0", 
				  -1,  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, NULL, NULL, 0);

	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
				  NULL, str, -wCount,
				  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, NULL, NULL, 0);
*/
	lpClipRect->left = lprect->left ;
	lpClipRect->top = lprect->top;
	lpClipRect->right = lprect->right;
	lpClipRect->bottom = lprect->bottom;
        dprintf_win16drv(stddeb, "WIN16DRV_ExtTextOut Clip rect left %d top %d rigt %d bottom %d\n",
                         lpClipRect->left,lpClipRect->top,lpClipRect->right,lpClipRect->bottom);
        
	lpClipRect->left = 0;
	lpClipRect->top = 0;
	lpClipRect->right = 0x3fc;
	lpClipRect->bottom = 0x630;
        dprintf_win16drv(stddeb, "WIN16DRV_ExtTextOut Clip rect left %d top %d rigt %d bottom %d\n",
                         lpClipRect->left,lpClipRect->top,lpClipRect->right,lpClipRect->bottom);
	lpOpaqueRect->left = x;
	lpOpaqueRect->top = y;
	lpOpaqueRect->right = 0x3a1;
	lpOpaqueRect->bottom = 0x01;
        printf("drawmode ropt 0x%x bkMode 0x%x bkColor 0x%lx textColor 0x%lx tbbreakExtra 0x%x breakextra 0x%x\n",
               lpDrawMode->Rop2,    lpDrawMode->bkMode,    lpDrawMode->bkColor,
               lpDrawMode->TextColor,    lpDrawMode->TBreakExtra,    lpDrawMode->BreakExtra);
        printf("breakerr 0x%x breakrem 0x%x breakcount 0x%x chextra 0x%x lbkcolor 0x%lx ltextcolor 0x%lx\n",
               lpDrawMode->BreakErr,    lpDrawMode->BreakRem,    lpDrawMode->BreakCount,
               lpDrawMode->CharExtra,	   lpDrawMode->LbkColor,    lpDrawMode->LTextColor);

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
        

	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, x, y, 
				  lpClipRect, str, 
				  wCount,  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, NULL, NULL, wOptions);
/*
	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, x, y, 
				  lpClipRect, str, 
				  wCount,  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, lpDx, NULL, flags);
*/
    }
    return bRet;
}

/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996,1997 John Harvey
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include "wine/winbase16.h"
#include "winuser.h"
#include "wownt32.h"
#include "win16drv/win16drv.h"
#include "wine/debug.h"
#include "bitmap.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16drv);

/* ### start build ### */
extern WORD CALLBACK PRTDRV_CallTo16_word_lwll (FARPROC16,LONG,WORD,LONG,LONG);
extern WORD CALLBACK PRTDRV_CallTo16_word_lwlll (FARPROC16,LONG,WORD,LONG,LONG,
						 LONG);
extern WORD CALLBACK PRTDRV_CallTo16_word_llll (FARPROC16,LONG,LONG,LONG,LONG);
extern WORD CALLBACK PRTDRV_CallTo16_word_lwwlllll (FARPROC16,LONG,WORD,WORD,
						    LONG,LONG,LONG,LONG,LONG);
extern LONG CALLBACK PRTDRV_CallTo16_long_lwlll (FARPROC16,LONG,WORD,LONG,LONG,
						 LONG);
extern WORD CALLBACK PRTDRV_CallTo16_word_lwwwwlwwwwllll (FARPROC16,LONG,WORD,
							  WORD,WORD,WORD,LONG,
							  WORD,WORD,WORD,WORD,
							  LONG,LONG,LONG,LONG);
extern LONG CALLBACK PRTDRV_CallTo16_long_lwwllwlllllw (FARPROC16,LONG,WORD,
							WORD,LONG,LONG,WORD,
							LONG,LONG,LONG,LONG,
							LONG,WORD);
extern WORD CALLBACK PRTDRV_CallTo16_word_llwwlll (FARPROC16,LONG,LONG,WORD,
						   WORD,LONG,LONG,LONG);
extern WORD CALLBACK PRTDRV_CallTo16_word_wwlllllw (FARPROC16,WORD,WORD,LONG,
						    LONG,LONG,LONG,LONG,WORD);
extern LONG CALLBACK PRTDRV_CallTo16_long_llwll (FARPROC16,LONG,LONG,WORD,LONG,
						 LONG);

/* ### stop build ### */


#define MAX_PRINTER_DRIVERS 	16
static LOADED_PRINTER_DRIVER *gapLoadedPrinterDrivers[MAX_PRINTER_DRIVERS];


static void GetPrinterDriverFunctions(HINSTANCE16 hInst, LOADED_PRINTER_DRIVER *pLPD)
{
#define LoadPrinterDrvFunc(A) pLPD->fn[FUNC_##A] = \
      GetProcAddress16(hInst, MAKEINTRESOURCEA(ORD_##A))

      LoadPrinterDrvFunc(BITBLT);
      LoadPrinterDrvFunc(COLORINFO);
      LoadPrinterDrvFunc(CONTROL);
      LoadPrinterDrvFunc(DISABLE);
      LoadPrinterDrvFunc(ENABLE);
      LoadPrinterDrvFunc(ENUMDFONTS);
      LoadPrinterDrvFunc(ENUMOBJ);
      LoadPrinterDrvFunc(OUTPUT);
      LoadPrinterDrvFunc(PIXEL);
      LoadPrinterDrvFunc(REALIZEOBJECT);
      LoadPrinterDrvFunc(STRBLT);
      LoadPrinterDrvFunc(SCANLR);
      LoadPrinterDrvFunc(DEVICEMODE);
      LoadPrinterDrvFunc(EXTTEXTOUT);
      LoadPrinterDrvFunc(GETCHARWIDTH);
      LoadPrinterDrvFunc(DEVICEBITMAP);
      LoadPrinterDrvFunc(FASTBORDER);
      LoadPrinterDrvFunc(SETATTRIBUTE);
      LoadPrinterDrvFunc(STRETCHBLT);
      LoadPrinterDrvFunc(STRETCHDIBITS);
      LoadPrinterDrvFunc(SELECTBITMAP);
      LoadPrinterDrvFunc(BITMAPBITS);
      LoadPrinterDrvFunc(EXTDEVICEMODE);
      LoadPrinterDrvFunc(DEVICECAPABILITIES);
      LoadPrinterDrvFunc(ADVANCEDSETUPDIALOG);
      LoadPrinterDrvFunc(DIALOGFN);
      LoadPrinterDrvFunc(PSEUDOEDIT);
      TRACE("got func CONTROL %p enable %p enumDfonts %p realizeobject %p extextout %p\n",
              pLPD->fn[FUNC_CONTROL],
              pLPD->fn[FUNC_ENABLE],
              pLPD->fn[FUNC_ENUMDFONTS],
              pLPD->fn[FUNC_REALIZEOBJECT],
              pLPD->fn[FUNC_EXTTEXTOUT]);
}


static LOADED_PRINTER_DRIVER *FindPrinterDriverFromName(const char *pszDriver)
{
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    int		nDriverSlot = 0;

    /* Look to see if the printer driver is already loaded */
    while (pLPD == NULL && nDriverSlot < MAX_PRINTER_DRIVERS)
    {
	LOADED_PRINTER_DRIVER *ptmpLPD;
	ptmpLPD = gapLoadedPrinterDrivers[nDriverSlot++];
	if (ptmpLPD != NULL)
	{
	    TRACE("Comparing %s,%s\n",ptmpLPD->szDriver,pszDriver);
	    /* Found driver store info, exit loop */
	    if (strcasecmp(ptmpLPD->szDriver, pszDriver) == 0)
	      pLPD = ptmpLPD;
	}
    }
    return pLPD;
}

static LOADED_PRINTER_DRIVER *FindPrinterDriverFromPDEVICE(SEGPTR segptrPDEVICE)
{
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    /* Find the printer driver associated with this PDEVICE */
    /* Each of the PDEVICE structures has a PDEVICE_HEADER structure */
    /* just before it */
    if (segptrPDEVICE != 0)
    {
	PDEVICE_HEADER *pPDH = ((PDEVICE_HEADER *)MapSL(segptrPDEVICE)) - 1;
        pLPD = pPDH->pLPD;
    }
    return pLPD;
}

/*
 * Load a printer driver, adding it self to the list of loaded drivers.
 */

LOADED_PRINTER_DRIVER *LoadPrinterDriver(const char *pszDriver)
{
    HINSTANCE16	hInst;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    int		nDriverSlot = 0;
    BOOL	bSlotFound = FALSE;

    /* First look to see if driver is loaded */
    pLPD = FindPrinterDriverFromName(pszDriver);
    if (pLPD != NULL)
    {
	/* Already loaded so increase usage count */
	pLPD->nUsageCount++;
	return pLPD;
    }

    /* Not loaded so try and find an empty slot */
    while (!bSlotFound && nDriverSlot < MAX_PRINTER_DRIVERS)
    {
	if (gapLoadedPrinterDrivers[nDriverSlot] == NULL)
	  bSlotFound = TRUE;
	else
	  nDriverSlot++;
    }
    if (!bSlotFound)
    {
	WARN("Too many printers drivers loaded\n");
	return NULL;
    }

    {
        char *p, *drvName = HeapAlloc(GetProcessHeap(), 0, strlen(pszDriver) + 5);
        strcpy(drvName, pszDriver);

	/* Append .DRV to name if no extension present */
	if (!(p = strrchr(drvName, '.')) || strchr(p, '/') || strchr(p, '\\'))
	    strcat(drvName, ".DRV");

        hInst = LoadLibrary16(drvName);
	HeapFree(GetProcessHeap(), 0, drvName);
    }


    if (hInst <= 32)
    {
	/* Failed to load driver */
	WARN("Failed to load printer driver %s\n", pszDriver);
    } else {
        TRACE("Loaded the library\n");
	/* Allocate some memory for printer driver info */
	pLPD = malloc(sizeof(LOADED_PRINTER_DRIVER));
	memset(pLPD, 0 , sizeof(LOADED_PRINTER_DRIVER));

	pLPD->hInst	= hInst;
	pLPD->szDriver = HeapAlloc(GetProcessHeap(),0,strlen(pszDriver)+1);
	strcpy( pLPD->szDriver, pszDriver );

	/* Get DS for the printer module */
	pLPD->ds_reg	= hInst;

	TRACE("DS for %s is %x\n", pszDriver, pLPD->ds_reg);

	/* Get address of printer driver functions */
	GetPrinterDriverFunctions(hInst, pLPD);

	/* Set initial usage count */
	pLPD->nUsageCount = 1;

	/* Update table of loaded printer drivers */
	pLPD->nIndex = nDriverSlot;
	gapLoadedPrinterDrivers[nDriverSlot] = pLPD;
    }

    return pLPD;
}

/*
 *  Control (ordinal 3)
 */
INT16 PRTDRV_Control(LPPDEVICE lpDestDev, WORD wfunction, SEGPTR lpInData, SEGPTR lpOutData)
{
    /* wfunction == Escape code */
    /* lpInData, lpOutData depend on code */

    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("%08x 0x%x %08lx %08lx\n", (unsigned int)lpDestDev, wfunction, lpInData, lpOutData);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	if (pLPD->fn[FUNC_CONTROL] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}
	wRet = PRTDRV_CallTo16_word_lwll( pLPD->fn[FUNC_CONTROL],
					  (SEGPTR)lpDestDev, wfunction,
					  lpInData, lpOutData );
    }
    TRACE("return %x\n", wRet);
    return wRet;
}

/*
 *	Enable (ordinal 5)
 */
WORD PRTDRV_Enable(LPVOID lpDevInfo, WORD wStyle, LPCSTR  lpDestDevType,
                          LPCSTR lpDeviceName, LPCSTR lpOutputFile, LPVOID lpData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("%s %s\n",lpDestDevType, lpOutputFile);

    /* Get the printer driver info */
    if (wStyle == INITPDEVICE)
	pLPD = FindPrinterDriverFromPDEVICE((SEGPTR)lpDevInfo);
    else
	pLPD = FindPrinterDriverFromName((char *)lpDeviceName);
    if (pLPD != NULL) {
	LONG		lP5;
	DeviceCaps devcaps;
	SEGPTR lP1, lP3,lP4;
	WORD		wP2;

	if (!pLPD->fn[FUNC_ENABLE]) {
	    WARN("Not supported by driver\n");
	    return 0;
	}

	if (wStyle == INITPDEVICE)
	    lP1 = (SEGPTR)lpDevInfo;/* 16 bit segmented ptr already */
	else
	    lP1 = MapLS(&devcaps);

	wP2 = wStyle;

	/* MapLS handles NULL like a charm ... */
	lP3 = MapLS(lpDestDevType);
	lP4 = MapLS(lpOutputFile);
	lP5 = (LONG)lpData;

	wRet = PRTDRV_CallTo16_word_lwlll(pLPD->fn[FUNC_ENABLE], lP1, wP2, lP3, lP4, lP5);
	UnMapLS(lP3);
	UnMapLS(lP4);

	/* Get the data back */
	if (lP1 != 0 && wStyle != INITPDEVICE) {
	    memcpy(lpDevInfo,&devcaps,sizeof(DeviceCaps));
            UnMapLS(lP1);
	}
    }
    TRACE("return %x\n", wRet);
    return wRet;
}


/*
 *	EnumDFonts (ordinal 6)
 */
WORD PRTDRV_EnumDFonts(LPPDEVICE lpDestDev, LPSTR lpFaceName,
		       FARPROC16 lpCallbackFunc, LPVOID lpClientData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("%08lx %s %p %p\n",
		     lpDestDev, lpFaceName, lpCallbackFunc, lpClientData);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	SEGPTR lP1, lP2, lP4;

	if (pLPD->fn[FUNC_ENUMDFONTS] == NULL) {
	    WARN("Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
        lP2 = MapLS(lpFaceName);
	lP4 = (LONG)lpClientData;
        wRet = PRTDRV_CallTo16_word_llll( pLPD->fn[FUNC_ENUMDFONTS], lP1, lP2,
                                          (LONG)lpCallbackFunc,lP4);
        UnMapLS(lP2);
    } else
        WARN("Failed to find device\n");

    TRACE("return %x\n", wRet);
    return wRet;
}
/*
 *	EnumObj (ordinal 7)
 */
BOOL16 PRTDRV_EnumObj(LPPDEVICE lpDestDev, WORD iStyle,
		       FARPROC16 lpCallbackFunc, LPVOID lpClientData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("(some params - FIXME)\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP3, lP4;
	WORD wP2;

	if (pLPD->fn[FUNC_ENUMOBJ] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;

	wP2 = iStyle;

	/*
	 * Need to pass addres of function conversion function that will switch back to 32 bit code if necessary
	 */
	lP3 = (LONG)lpCallbackFunc;

	lP4 = (LONG)lpClientData;

        wRet = PRTDRV_CallTo16_word_lwll( pLPD->fn[FUNC_ENUMOBJ],
					  lP1, wP2, lP3, lP4 );
    }
    else
        WARN("Failed to find device\n");

    TRACE("return %x\n", wRet);
    return wRet;
}

/*
 *	Output (ordinal 8)
 */
WORD PRTDRV_Output(LPPDEVICE 	 lpDestDev,
                   WORD 	 wStyle,
                   WORD 	 wCount,
                   POINT16      *points,
                   LPLOGPEN16 	 lpPen,
                   LPLOGBRUSH16	 lpBrush,
                   SEGPTR	 lpDrawMode,
                   HRGN 	 hClipRgn)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("PRTDRV_OUTPUT %d\n", wStyle );

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
        LONG lP1, lP4, lP5, lP6, lP7, lP8;
        WORD wP2, wP3;
	if (pLPD->fn[FUNC_OUTPUT] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}

        lP1 = lpDestDev;
        wP2 = wStyle;
        wP3 = wCount;
        lP4 = MapLS( points );
        lP5 = MapLS( lpPen );
        lP6 = MapLS( lpBrush );
        lP7 = lpDrawMode;

	if (hClipRgn)
	{
	    DWORD size;
	    RGNDATA *clip;

	    size = GetRegionData( hClipRgn, 0, NULL );
	    clip = HeapAlloc( GetProcessHeap(), 0, size );
	    if(!clip)
	    {
	        WARN("Can't alloc clip array in PRTDRV_Output\n");
		return FALSE;
	    }
	    GetRegionData( hClipRgn, size, clip );
	    if( clip->rdh.nCount == 0 )
	    {
		wRet = PRTDRV_CallTo16_word_lwwlllll(pLPD->fn[FUNC_OUTPUT],
						     lP1, wP2, wP3, lP4,
						     lP5, lP6, lP7,
						     (SEGPTR) NULL);
	    }
	    else
	    {
	        RECT *pRect;
                RECT16 r16;
	        lP8 = MapLS(&r16);

		for(pRect = (RECT *)clip->Buffer;
		    pRect < (RECT *)clip->Buffer + clip->rdh.nCount; pRect++)
	        {
		    CONV_RECT32TO16( pRect, &r16 );

		    TRACE("rect = %d,%d - %d,%d\n", r16.left, r16.top, r16.right, r16.bottom );
		    wRet = PRTDRV_CallTo16_word_lwwlllll(pLPD->fn[FUNC_OUTPUT],
							 lP1, wP2, wP3, lP4,
							 lP5, lP6, lP7, lP8);
		}
		UnMapLS(lP8);
	    }
	    HeapFree( GetProcessHeap(), 0, clip );
	}
	else
	{
	    wRet = PRTDRV_CallTo16_word_lwwlllll(pLPD->fn[FUNC_OUTPUT],
						 lP1, wP2, wP3, lP4,
						 lP5, lP6, lP7, (SEGPTR) NULL);
	}
        UnMapLS( lP4 );
        UnMapLS( lP5 );
        UnMapLS( lP6 );
    }
    TRACE("PRTDRV_Output return %d\n", wRet);
    return wRet;
}

/*
 * RealizeObject (ordinal 10)
 */
DWORD PRTDRV_RealizeObject(LPPDEVICE lpDestDev, WORD wStyle,
                           LPVOID lpInObj, LPVOID lpOutObj,
                           SEGPTR lpTextXForm)
{
    WORD dwRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("%08lx %04x %p %p %08lx\n",
		 lpDestDev, wStyle, lpInObj, lpOutObj, lpTextXForm);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
        LONG lP1, lP3, lP4, lP5;
	WORD wP2;

	if (pLPD->fn[FUNC_REALIZEOBJECT] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wStyle;
        lP3 = MapLS( lpInObj );
        lP4 = MapLS( lpOutObj );
        lP5 = lpTextXForm;
	TRACE("Calling Realize %08lx %04x %08lx %08lx %08lx\n",
		     lP1, wP2, lP3, lP4, lP5);
	dwRet = PRTDRV_CallTo16_long_lwlll(pLPD->fn[FUNC_REALIZEOBJECT],
					   lP1, wP2, lP3, lP4, lP5);
        UnMapLS( lP3 );
        UnMapLS( lP4 );
    }
    TRACE("return %x\n", dwRet);
    return dwRet;
}

/*
 * StretchBlt (ordinal 27)
 */
DWORD PRTDRV_StretchBlt(LPPDEVICE lpDestDev,
                        WORD wDestX, WORD wDestY,
                        WORD wDestXext, WORD wDestYext,
                        LPPDEVICE lpSrcDev,
                        WORD wSrcX, WORD wSrcY,
                        WORD wSrcXext, WORD wSrcYext,
                        DWORD Rop3,
                        LPLOGBRUSH16 lpBrush,
                        SEGPTR lpDrawMode,
                        RECT16 *lpClipRect)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("(lots of params - FIXME)\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
        LONG lP1,lP6, lP11, lP12, lP13;
        SEGPTR lP14;
        WORD wP2, wP3, wP4, wP5, wP7, wP8, wP9, wP10;

	if (pLPD->fn[FUNC_STRETCHBLT] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}
	lP1  = lpDestDev;
	wP2  = wDestX;
	wP3  = wDestY;
	wP4  = wDestXext;
	wP5  = wDestYext;
        lP6  = lpSrcDev;
        wP7  = wSrcX;
        wP8  = wSrcY;
        wP9  = wSrcXext;
        wP10 = wSrcYext;
        lP11 = Rop3;
        lP12 = MapLS( lpBrush );
        lP13 = lpDrawMode;
        lP14 = MapLS(lpClipRect);
	wRet = PRTDRV_CallTo16_word_lwwwwlwwwwllll(pLPD->fn[FUNC_STRETCHBLT],
						   lP1, wP2, wP3, wP4, wP5,
						   lP6, wP7, wP8, wP9, wP10,
						   lP11, lP12, lP13, lP14);
        UnMapLS(lP12);
        UnMapLS(lP14);
        TRACE("Called StretchBlt ret %d\n",wRet);
    }
    return wRet;
}

DWORD PRTDRV_ExtTextOut(LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg,
                        RECT16 *lpClipRect, LPCSTR lpString, WORD wCount,
                        LPFONTINFO16 lpFontInfo, SEGPTR lpDrawMode,
                        SEGPTR lpTextXForm, SHORT *lpCharWidths,
                        RECT16 *     lpOpaqueRect, WORD wOptions)
{
    DWORD dwRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("(lots of params - FIXME)\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG		lP1, lP4, lP5, lP7, lP8, lP9, lP10, lP11;
	WORD		wP2, wP3, wP12;
        INT16		iP6;

	if (pLPD->fn[FUNC_EXTTEXTOUT] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wDestXOrg;
	wP3 = wDestYOrg;
        lP4 = MapLS( lpClipRect );
        lP5 = MapLS( lpString );
	iP6 = wCount;

	/* This should be realized by the driver, so in 16bit data area */
	lP7 = MapLS( lpFontInfo );
        lP8 = lpDrawMode;
        lP9 = lpTextXForm;

	if (lpCharWidths != NULL)
	  FIXME("Char widths not supported\n");
	lP10 = 0;
	lP11 = MapLS( lpOpaqueRect );
	wP12 = wOptions;
	TRACE("Calling ExtTextOut 0x%lx 0x%x 0x%x 0x%lx\n", lP1, wP2, wP3, lP4);
        TRACE("%s 0x%x 0x%lx 0x%lx\n", lpString, iP6, lP7, lP8);
        TRACE("0x%lx 0x%lx 0x%lx 0x%x\n",
		     lP9, lP10, lP11, wP12);
        dwRet = PRTDRV_CallTo16_long_lwwllwlllllw(pLPD->fn[FUNC_EXTTEXTOUT], lP1, wP2, wP3,
                                                  lP4, lP5, iP6, lP7, lP8, lP9, lP10, lP11, wP12);
        UnMapLS( lP4 );
        UnMapLS( lP5 );
        UnMapLS( lP7 );
        UnMapLS( lP11 );
    }
    TRACE("return %lx\n", dwRet);
    return dwRet;
}

/***********************************************************************
 *		dmEnumDFonts (GDI.206)
 */
int WINAPI dmEnumDFonts16(LPPDEVICE lpDestDev, LPSTR lpFaceName, FARPROC16 lpCallbackFunc, LPVOID lpClientData)
{
        /* Windows 3.1 just returns 1 */
        return 1;
}

/***********************************************************************
 *		dmRealizeObject (GDI.210)
 */
int WINAPI dmRealizeObject16(LPPDEVICE lpDestDev, INT16 wStyle, LPSTR lpInObj, LPSTR lpOutObj, SEGPTR lpTextXForm)
{
    FIXME("(lpDestDev=%08x,wStyle=%04x,lpInObj=%08x,lpOutObj=%08x,lpTextXForm=%08x): stub\n",
        (UINT)lpDestDev, wStyle, (UINT)lpInObj, (UINT)lpOutObj, (UINT)lpTextXForm);
    if (wStyle < 0) { /* Free extra memory of given object's structure */
	switch ( -wStyle ) {
            case DRVOBJ_PEN:    {
                                /* LPLOGPEN16 DeletePen = (LPLOGPEN16)lpInObj; */

                                TRACE("DRVOBJ_PEN_delete\n");
				break;
                                }
            case DRVOBJ_BRUSH:	{
				TRACE("DRVOBJ_BRUSH_delete\n");
                                break;
				}
            case DRVOBJ_FONT:	{
				/* LPTEXTXFORM16 TextXForm
					= (LPTEXTXFORM16)lpTextXForm; */
				TRACE("DRVOBJ_FONT_delete\n");
                                break;
				}
            case DRVOBJ_PBITMAP:        TRACE("DRVOBJ_PBITMAP_delete\n");
                                break;
	}
    }
    else { /* Realize given object */

	switch (wStyle) {
	    case DRVOBJ_PEN: {
				LPLOGPEN16 InPen  = (LPLOGPEN16)lpInObj;

				TRACE("DRVOBJ_PEN\n");
				if (lpOutObj) {
				    if (InPen->lopnStyle == PS_NULL) {
					*(DWORD *)lpOutObj = 0;
					*(WORD *)(lpOutObj+4) = InPen->lopnStyle;
				    }
				    else
				    if ((InPen->lopnWidth.x > 1) || (InPen->lopnStyle > PS_NULL) ) {
					*(DWORD *)lpOutObj = InPen->lopnColor;
					*(WORD *)(lpOutObj+4) = 0;
				    }
				    else {
					*(DWORD *)lpOutObj = InPen->lopnColor & 0xffff0000;
					*(WORD *)(lpOutObj+4) = InPen->lopnStyle;
				    }
				}
				return sizeof(LOGPEN16);
			     }
	    case DRVOBJ_BRUSH: {
				LPLOGBRUSH16 InBrush  = (LPLOGBRUSH16)lpInObj;
				LPLOGBRUSH16 OutBrush = (LPLOGBRUSH16)lpOutObj;
                                /* LPPOINT16 Point = (LPPOINT16)lpTextXForm; */

				TRACE("DRVOBJ_BRUSH\n");
				if (!lpOutObj) return sizeof(LOGBRUSH16);
				else {
				    OutBrush->lbStyle = InBrush->lbStyle;
				    OutBrush->lbColor = InBrush->lbColor;
				    OutBrush->lbHatch = InBrush->lbHatch;
				    if (InBrush->lbStyle == BS_SOLID)
				    return 0x8002; /* FIXME: diff mono-color */
				    else return 0x8000;
				}
		               }
	    case DRVOBJ_FONT: {
                                /* LPTEXTXFORM16 TextXForm
                                        = (LPTEXTXFORM16)lpTextXForm; */
                                TRACE("DRVOBJ_FONT\n");
				return 0;/* DISPLAY.DRV doesn't realize fonts */
			      }
	    case DRVOBJ_PBITMAP:	TRACE("DRVOBJ_PBITMAP\n");
					return 0; /* create memory bitmap */
	}
    }
    return 1;
}


WORD PRTDRV_GetCharWidth(LPPDEVICE lpDestDev, LPINT lpBuffer,
		      WORD wFirstChar, WORD wLastChar, LPFONTINFO16 lpFontInfo,
		      SEGPTR lpDrawMode, SEGPTR lpTextXForm )
{

    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    TRACE("(lots of params - FIXME)\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG		lP1, lP2, lP5, lP6, lP7;
	WORD		wP3, wP4;

	if (pLPD->fn[FUNC_GETCHARWIDTH] == NULL)
	{
	    WARN("Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	lP2 = MapLS( lpBuffer );
	wP3 = wFirstChar;
	wP4 = wLastChar;
	lP5 = MapLS( lpFontInfo );
        lP6 = lpDrawMode;
        lP7 = lpTextXForm;

	wRet = PRTDRV_CallTo16_word_llwwlll(pLPD->fn[FUNC_GETCHARWIDTH],
                                            lP1, lP2, wP3, wP4, lP5, lP6, lP7 );

        UnMapLS( lP2 );
        UnMapLS( lP5 );
    }
    return wRet;
}

/**************************************************************
 *
 *       WIN16DRV_ExtDeviceMode
 */
INT WIN16DRV_ExtDeviceMode(LPSTR lpszDriver, HWND hwnd, LPDEVMODEA lpdmOutput,
			   LPSTR lpszDevice, LPSTR lpszPort,
			   LPDEVMODEA lpdmInput, LPSTR lpszProfile,
			   DWORD dwMode)
{
    LOADED_PRINTER_DRIVER *pLPD = LoadPrinterDriver(lpszDriver);
    SEGPTR lpSegOut, lpSegIn, lpSegDevice, lpSegPort, lpSegProfile;
    INT16 wRet;

    if(!pLPD) return -1;

    if(pLPD->fn[FUNC_EXTDEVICEMODE] == NULL) {
        WARN("No EXTDEVICEMODE\n");
	return -1;
    }
    lpSegDevice = MapLS(lpszDevice);
    lpSegPort = MapLS(lpszPort);
    lpSegProfile = MapLS(lpszProfile);
    lpSegOut = MapLS(lpdmOutput);
    lpSegIn = MapLS(lpdmInput);
    wRet = PRTDRV_CallTo16_word_wwlllllw( pLPD->fn[FUNC_EXTDEVICEMODE],
					  HWND_16(hwnd), pLPD->hInst,
                                          lpSegOut, lpSegDevice, lpSegPort, lpSegIn,
                                          lpSegProfile, dwMode );
    UnMapLS(lpSegOut);
    UnMapLS(lpSegIn);
    UnMapLS(lpSegDevice);
    UnMapLS(lpSegPort);
    UnMapLS(lpSegProfile);
    return wRet;
}

/**************************************************************
 *
 *       WIN16DRV_DeviceCapabilities
 *
 * This is a bit of a pain since we don't know the size of lpszOutput we have
 * call the driver twice.
 */
DWORD WIN16DRV_DeviceCapabilities(LPSTR lpszDriver, LPCSTR lpszDevice,
				  LPCSTR lpszPort, WORD fwCapability,
				  LPSTR lpszOutput, LPDEVMODEA lpDevMode)
{
    LOADED_PRINTER_DRIVER *pLPD = LoadPrinterDriver(lpszDriver);
    SEGPTR lpSegdm, lpSegOut, lpSegDevice, lpSegPort;
    DWORD dwRet;

    TRACE("%s,%s,%s,%d,%p,%p\n", lpszDriver, lpszDevice, lpszPort,
	  fwCapability, lpszOutput, lpDevMode);

    if(!pLPD) return -1;

    if(pLPD->fn[FUNC_DEVICECAPABILITIES] == NULL) {
        WARN("No DEVICECAPABILITES\n");
	return -1;
    }
    lpSegDevice = MapLS(lpszDevice);
    lpSegPort = MapLS(lpszPort);
    lpSegdm = MapLS(lpDevMode);
    lpSegOut = MapLS(lpszOutput);
    dwRet = PRTDRV_CallTo16_long_llwll( pLPD->fn[FUNC_DEVICECAPABILITIES],
                                        lpSegDevice, lpSegPort, fwCapability, lpSegOut, lpSegdm );
    UnMapLS(lpSegOut);
    UnMapLS(lpSegdm);
    UnMapLS(lpSegDevice);
    UnMapLS(lpSegPort);
    return dwRet;
}

/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996,1997 John Harvey
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "windows.h"
#include "win16drv.h"
#include "heap.h"
#include "brush.h"
#include "callback.h"
#include "debug.h"
#include "bitmap.h"
#include "pen.h"

#define MAX_PRINTER_DRIVERS 	16
static LOADED_PRINTER_DRIVER *gapLoadedPrinterDrivers[MAX_PRINTER_DRIVERS];


static void GetPrinterDriverFunctions(HINSTANCE16 hInst, LOADED_PRINTER_DRIVER *pLPD)
{
#define LoadPrinterDrvFunc(A) pLPD->fn[FUNC_##A] = \
      GetProcAddress16(hInst, MAKEINTRESOURCE16(ORD_##A))

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
      TRACE(win16drv,"got func CONTROL 0x%p enable 0x%p enumDfonts 0x%p realizeobject 0x%p extextout 0x%p\n",
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
	    TRACE(win16drv, "Comparing %s,%s\n",ptmpLPD->szDriver,pszDriver);
	    /* Found driver store info, exit loop */
	    if (lstrcmpi32A(ptmpLPD->szDriver, pszDriver) == 0)
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
    if (segptrPDEVICE != (SEGPTR)NULL)
    {
	PDEVICE_HEADER *pPDH = (PDEVICE_HEADER *)
	  (PTR_SEG_TO_LIN(segptrPDEVICE) - sizeof(PDEVICE_HEADER)); 
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
    BOOL32	bSlotFound = FALSE;

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
	fprintf(stderr,"Too many printers drivers loaded\n");
	return NULL;
    }

    {
        char *drvName = malloc(strlen(pszDriver)+5);
        strcpy(drvName, pszDriver);
        strcat(drvName, ".DRV");
        hInst = LoadLibrary16(drvName);
    }

    
    if (hInst <= 32)
    {
	/* Failed to load driver */
	fprintf(stderr, "Failed to load printer driver %s\n", pszDriver);
    } else {
        TRACE(win16drv, "Loaded the library\n");
	/* Allocate some memory for printer driver info */
	pLPD = malloc(sizeof(LOADED_PRINTER_DRIVER));
	memset(pLPD, 0 , sizeof(LOADED_PRINTER_DRIVER));
	
	pLPD->hInst	= hInst;
	pLPD->szDriver	= HEAP_strdupA(SystemHeap,0,pszDriver);

	/* Get DS for the printer module */
	pLPD->ds_reg	= hInst;

	TRACE(win16drv, "DS for %s is %x\n", pszDriver, pLPD->ds_reg);

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

    TRACE(win16drv, "%08x 0x%x %08lx %08lx\n", (unsigned int)lpDestDev, wfunction, lpInData, lpOutData);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	if (pLPD->fn[FUNC_CONTROL] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}
	wRet = Callbacks->CallDrvControlProc( pLPD->fn[FUNC_CONTROL], 
                                              (SEGPTR)lpDestDev, wfunction,
                                              lpInData, lpOutData );
    }
    TRACE(win16drv, "return %x\n", wRet);
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

    TRACE(win16drv, "%s %s\n",lpDestDevType, lpOutputFile);

    /* Get the printer driver info */
    if (wStyle == INITPDEVICE)
	pLPD = FindPrinterDriverFromPDEVICE((SEGPTR)lpDevInfo);
    else
	pLPD = FindPrinterDriverFromName((char *)lpDeviceName);
    if (pLPD != NULL) {
	LONG		lP5;
	DeviceCaps	*lP1;
	LPSTR		lP3,lP4;
	WORD		wP2;

	if (!pLPD->fn[FUNC_ENABLE]) {
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

	if (wStyle == INITPDEVICE)
	    lP1 = (DeviceCaps*)lpDevInfo;/* 16 bit segmented ptr already */
	else
	    lP1 = SEGPTR_NEW(DeviceCaps);
	
	wP2 = wStyle;
	
	/* SEGPTR_STRDUP handles NULL like a charm ... */
	lP3 = SEGPTR_STRDUP(lpDestDevType);
	lP4 = SEGPTR_STRDUP(lpOutputFile);
	lP5 = (LONG)lpData;
        
	wRet = Callbacks->CallDrvEnableProc(pLPD->fn[FUNC_ENABLE], 
			     (wStyle==INITPDEVICE)?(SEGPTR)lP1:SEGPTR_GET(lP1),
			     wP2,
			     SEGPTR_GET(lP3),
			     SEGPTR_GET(lP4),
			     lP5);
	SEGPTR_FREE(lP3);
	SEGPTR_FREE(lP4);

	/* Get the data back */
	if (lP1 != 0 && wStyle != INITPDEVICE) {
	    memcpy(lpDevInfo,lP1,sizeof(DeviceCaps));
	    SEGPTR_FREE(lP1);
	}
    }
    TRACE(win16drv, "return %x\n", wRet);
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

    TRACE(win16drv, "%08lx %s %p %p\n",
		     lpDestDev, lpFaceName, lpCallbackFunc, lpClientData);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP4;
	LPBYTE lP2;

	if (pLPD->fn[FUNC_ENUMDFONTS] == NULL) {
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	if(lpFaceName)
	    lP2 = SEGPTR_STRDUP(lpFaceName);
	else
	    lP2 = NULL;
	lP4 = (LONG)lpClientData;
        wRet = Callbacks->CallDrvEnumDFontsProc( pLPD->fn[FUNC_ENUMDFONTS], 
                                lP1, SEGPTR_GET(lP2), lpCallbackFunc, lP4);
	if(lpFaceName)
	    SEGPTR_FREE(lP2);
    } else 
        fprintf(stderr,"Failed to find device\n");
    
    TRACE(win16drv, "return %x\n", wRet);
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

    TRACE(win16drv, "(some params - fixme)\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP4;
        FARPROC16 lP3;
	WORD wP2;

	if (pLPD->fn[FUNC_ENUMOBJ] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	
	wP2 = iStyle;

	/* 
	 * Need to pass addres of function conversion function that will switch back to 32 bit code if necessary
	 */
	lP3 = (FARPROC16)lpCallbackFunc; 

	lP4 = (LONG)lpClientData;
        
        wRet = Callbacks->CallDrvEnumObjProc( pLPD->fn[FUNC_ENUMOBJ], 
                                              lP1, wP2, lP3, lP4 );
    }
    else 
        fprintf(stderr,"Failed to find device\n");
    
    TRACE(win16drv, "return %x\n", wRet);
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
                   HRGN32 	 hClipRgn)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    
    TRACE(win16drv, "PRTDRV_OUTPUT %d\n", wStyle );
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
        LONG lP1, lP5, lP6, lP7;
        LPPOINT16 lP4;
        LPRECT16 lP8;
        WORD wP2, wP3;
	int   nSize;
	if (pLPD->fn[FUNC_OUTPUT] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

        lP1 = lpDestDev;
        wP2 = wStyle;
        wP3 = wCount;
        nSize = sizeof(POINT16) * wCount;
 	lP4 = (LPPOINT16 )SEGPTR_ALLOC(nSize);
 	memcpy(lP4,points,nSize);
        lP5 = SEGPTR_GET( lpPen );
        lP6 = SEGPTR_GET( lpBrush );
        lP7 = lpDrawMode;
        
	if (hClipRgn)
	{
	    DWORD size;
	    RGNDATA *clip;

	    size = GetRegionData( hClipRgn, 0, NULL );
	    clip = HeapAlloc( SystemHeap, 0, size );
	    if(!clip) 
	    {
	        fprintf(stderr, "Can't alloc clip array in PRTDRV_Output\n");
		return FALSE;
	    }
	    GetRegionData( hClipRgn, size, clip );
	    if( clip->rdh.nCount == 0 )
	    {
		wRet = Callbacks->CallDrvOutputProc(pLPD->fn[FUNC_OUTPUT], 
                                            lP1, wP2, wP3, SEGPTR_GET(lP4),
                                            lP5, lP6, lP7, (SEGPTR) NULL);
	    }
	    else
	    {
	        RECT32 *pRect;
	        lP8 = SEGPTR_NEW(RECT16);
	    
		for(pRect = (RECT32 *)clip->Buffer; 
		    pRect < (RECT32 *)clip->Buffer + clip->rdh.nCount; pRect++)
	        {
		    CONV_RECT32TO16( pRect, lP8 );

		    TRACE(win16drv, "rect = %d,%d - %d,%d\n",
			    lP8->left, lP8->top, lP8->right, lP8->bottom );
		    wRet = Callbacks->CallDrvOutputProc(pLPD->fn[FUNC_OUTPUT], 
                                            lP1, wP2, wP3, SEGPTR_GET(lP4),
                                            lP5, lP6, lP7, SEGPTR_GET(lP8));
		}
		SEGPTR_FREE(lP8);
	    }
	    HeapFree( SystemHeap, 0, clip );
	}
	else
	{
	    wRet = Callbacks->CallDrvOutputProc(pLPD->fn[FUNC_OUTPUT], 
                                            lP1, wP2, wP3, SEGPTR_GET(lP4),
                                            lP5, lP6, lP7, (SEGPTR) NULL);
	}
        SEGPTR_FREE(lP4);
    }
    TRACE(win16drv, "PRTDRV_Output return %d\n", wRet);
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
    
    TRACE(win16drv, "%08lx %04x %p %p %08lx\n",
		 lpDestDev, wStyle, lpInObj, lpOutObj, lpTextXForm);
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
        LONG lP1, lP3, lP4, lP5;  
	WORD wP2;
	LPBYTE lpBuf = NULL;
	unsigned int	nSize;

	if (pLPD->fn[FUNC_REALIZEOBJECT] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wStyle;
	
	switch ((INT16)wStyle)
	{
        case DRVOBJ_BRUSH:
            nSize = sizeof (LOGBRUSH16);
            break;
        case DRVOBJ_FONT: 
            nSize = sizeof(LOGFONT16); 
            break;
        case DRVOBJ_PEN:
            nSize = sizeof(LOGPEN16); 
            break;

        case -DRVOBJ_BRUSH:
        case -DRVOBJ_FONT: 
        case -DRVOBJ_PEN:
	    nSize = -1;
            break;

        case DRVOBJ_PBITMAP:
        default:
	    fprintf(stderr, 
	       "PRTDRV_RealizeObject: Object type %d not supported\n", wStyle);
            nSize = 0;
            
	}

	if(nSize != -1)
	{
	    lpBuf = SEGPTR_ALLOC(nSize);
	    memcpy(lpBuf, lpInObj, nSize);
	    lP3 = SEGPTR_GET(lpBuf);
	} 
	else
	    lP3 = SEGPTR_GET( lpInObj );

	lP4 = SEGPTR_GET( lpOutObj );

        lP5 = lpTextXForm;
	TRACE(win16drv, "Calling Realize %08lx %04x %08lx %08lx %08lx\n",
		     lP1, wP2, lP3, lP4, lP5);
	dwRet = Callbacks->CallDrvRealizeProc(pLPD->fn[FUNC_REALIZEOBJECT], 
                                              lP1, wP2, lP3, lP4, lP5);
	if(lpBuf)
	    SEGPTR_FREE(lpBuf);

    }
    TRACE(win16drv, "return %x\n", dwRet);
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
    
    TRACE(win16drv, "(lots of params - fixme)\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
        LONG lP1,lP6, lP11, lP12, lP13;
        LPRECT16 lP14;
        WORD wP2, wP3, wP4, wP5, wP7, wP8, wP9, wP10;

	if (pLPD->fn[FUNC_STRETCHBLT] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
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
        lP12 = SEGPTR_GET( lpBrush );
        lP13 = lpDrawMode;
	if (lpClipRect != NULL)
	{
	    lP14 = SEGPTR_NEW(RECT16);
	    memcpy(lP14,lpClipRect,sizeof(RECT16));

	}
	else
	  lP14 = 0L;
	wRet = Callbacks->CallDrvStretchBltProc(pLPD->fn[FUNC_STRETCHBLT], 
                                                lP1, wP2, wP3, wP4, wP5,
                                                lP6, wP7, wP8, wP9, wP10, 
                                                lP11, lP12, lP13,
                                                SEGPTR_GET(lP14));
        SEGPTR_FREE(lP14);
        TRACE(win16drv, "Called StretchBlt ret %d\n",wRet);
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
    
    TRACE(win16drv, "(lots of params - fixme)\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG		lP1, lP7, lP8, lP9, lP10;  
	LPSTR		lP5;
	LPRECT16	lP4,lP11;
	WORD		wP2, wP3, wP12;
        INT16		iP6;
	unsigned int	nSize = -1;

	if (pLPD->fn[FUNC_EXTTEXTOUT] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wDestXOrg;
	wP3 = wDestYOrg;
	
	if (lpClipRect != NULL) {
	    lP4 = SEGPTR_NEW(RECT16);
            TRACE(win16drv, "Adding lpClipRect\n");
	    memcpy(lP4,lpClipRect,sizeof(RECT16));
	} else
	  lP4 = 0L;

	if (lpString != NULL) {
	    nSize = strlen(lpString);
	    if (nSize>abs(wCount))
	    	nSize = abs(wCount);
	    lP5 = SEGPTR_ALLOC(nSize+1);
            TRACE(win16drv, "Adding lpString (nSize is %d)\n",nSize);
	    memcpy(lP5,lpString,nSize);
	    *((char *)lP5 + nSize) = '\0';
	} else
	  lP5 = 0L;
	
	iP6 = wCount;
	
	/* This should be realized by the driver, so in 16bit data area */
	lP7 = SEGPTR_GET( lpFontInfo );
        lP8 = lpDrawMode;
        lP9 = lpTextXForm;
	
	if (lpCharWidths != NULL) 
	  FIXME(win16drv, "Char widths not supported\n");
	lP10 = 0;
	
	if (lpOpaqueRect != NULL) {
	    lP11 = SEGPTR_NEW(RECT16);
            TRACE(win16drv, "Adding lpOpaqueRect\n");
	    memcpy(lP11,lpOpaqueRect,sizeof(RECT16));	
	} else
	  lP11 = 0L;
	
	wP12 = wOptions;
	TRACE(win16drv, "Calling ExtTextOut 0x%lx 0x%x 0x%x %p\n",
		     lP1, wP2, wP3, lP4);
        TRACE(win16drv, "%*s 0x%x 0x%lx 0x%lx\n",
		     nSize,lP5, iP6, lP7, lP8);
        TRACE(win16drv, "0x%lx 0x%lx %p 0x%x\n",
		     lP9, lP10, lP11, wP12);
	dwRet = Callbacks->CallDrvExtTextOutProc(pLPD->fn[FUNC_EXTTEXTOUT], 
                                                 lP1, wP2, wP3,
                                                 SEGPTR_GET(lP4),
                                                 SEGPTR_GET(lP5), iP6, lP7,
                                                 lP8, lP9, lP10,
                                                 SEGPTR_GET(lP11), wP12);
    }
    TRACE(win16drv, "return %lx\n", dwRet);
    return dwRet;
}

int WINAPI dmEnumDFonts(LPPDEVICE lpDestDev, LPSTR lpFaceName, FARPROC16 lpCallbackFunc, LPVOID lpClientData)
{
        /* Windows 3.1 just returns 1 */
        return 1;
}

int WINAPI dmRealizeObject(LPPDEVICE lpDestDev, INT16 wStyle, LPSTR lpInObj, LPSTR lpOutObj, SEGPTR lpTextXForm)
{
    fprintf(stderr, "dmRealizeObject(lpDestDev: %08x, wStyle: %04x, lpInObj: %08x, lpOutObj: %08x, lpTextXForm: %08x): stub: ", (UINT32)lpDestDev, wStyle, (UINT32)lpInObj, (UINT32)lpOutObj, (UINT32)lpTextXForm);
    if (wStyle < 0) { /* Free extra memory of given object's structure */
	switch ( -wStyle ) {
            case DRVOBJ_PEN:    {
                                /* LPLOGPEN16 DeletePen = (LPLOGPEN16)lpInObj; */

                                fprintf(stderr, "DRVOBJ_PEN_delete\n");
				break;
                                }
            case DRVOBJ_BRUSH:	{
				fprintf(stderr, "DRVOBJ_BRUSH_delete\n");
                                break;
				}
            case DRVOBJ_FONT:	{
				/* LPTEXTXFORM16 TextXForm
					= (LPTEXTXFORM16)lpTextXForm; */
				fprintf(stderr, "DRVOBJ_FONT_delete\n");
                                break;
				}
            case DRVOBJ_PBITMAP:        fprintf(stderr, "DRVOBJ_PBITMAP_delete\n");
                                break;
	}
    }
    else { /* Realize given object */

	switch (wStyle) {
	    case DRVOBJ_PEN: {
				LPLOGPEN16 InPen  = (LPLOGPEN16)lpInObj;

				fprintf(stderr, "DRVOBJ_PEN\n");
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

				fprintf(stderr, "DRVOBJ_BRUSH\n");
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
                                fprintf(stderr, "DRVOBJ_FONT\n");
				return 0;/* DISPLAY.DRV doesn't realize fonts */
			      }
	    case DRVOBJ_PBITMAP:	fprintf(stderr, "DRVOBJ_PBITMAP\n");
					return 0; /* create memory bitmap */
	}
    }
    return 1;
}


WORD PRTDRV_GetCharWidth(LPPDEVICE lpDestDev, LPINT32 lpBuffer, 
		      WORD wFirstChar, WORD wLastChar, LPFONTINFO16 lpFontInfo,
		      SEGPTR lpDrawMode, SEGPTR lpTextXForm )
{

    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    
    TRACE(win16drv, "(lots of params - fixme)\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG		lP1, lP5, lP6, lP7;  
	LPWORD		lP2;
	WORD		wP3, wP4, i;
	
	if (pLPD->fn[FUNC_GETCHARWIDTH] == NULL)
	{
	    WARN(win16drv, "Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	lP2 = SEGPTR_ALLOC( (wLastChar - wFirstChar + 1) * sizeof(WORD) );
	wP3 = wFirstChar;
	wP4 = wLastChar;
	lP5 = SEGPTR_GET( lpFontInfo );
        lP6 = lpDrawMode;
        lP7 = lpTextXForm;
	
	wRet = Callbacks->CallDrvGetCharWidthProc(pLPD->fn[FUNC_GETCHARWIDTH], 
                                                 lP1, SEGPTR_GET(lP2), wP3,
                                                 wP4, lP5, lP6, lP7 );

	for(i = 0; i <= wLastChar - wFirstChar; i++)
	    lpBuffer[i] = (INT32) lP2[i];

	SEGPTR_FREE(lP2);
    }
    return wRet;
}

/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996 John Harvey
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "windows.h"
#include "module.h"
#include "win16drv.h"
#include "gdi.h"
#include "bitmap.h"
#include "heap.h"
#include "color.h"
#include "font.h"
#include "callback.h"
#include "stddebug.h"
#include "debug.h"

#define SUPPORT_REALIZED_FONTS 1 	
typedef SEGPTR LPPDEVICE;


#if 0
static BOOL16 windrvExtTextOut16( DC *dc, INT16 x, INT16 y, UINT16 flags, const RECT16 * lprect,
                                 LPCSTR str, UINT16 count, const INT16 *lpDx);
#endif

static BOOL32 WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                                 LPCSTR output, const DEVMODE16* initData );
static INT32 WIN16DRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
                              SEGPTR lpInData, SEGPTR lpOutData );

static const DC_FUNCTIONS WIN16DRV_Funcs =
{
    NULL,                            /* pArc */
    NULL,                            /* pBitBlt */
    NULL,                            /* pChord */
    WIN16DRV_CreateDC,               /* pCreateDC */
    NULL,                            /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    NULL,                            /* pEllipse */
    WIN16DRV_Escape,                 /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    NULL,                            /* pExtFloodFill */
    NULL,                            /* pExtTextOut */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFloodFill */
    NULL,                            /* pFrameRgn */
    WIN16DRV_GetTextExtentPoint,     /* pGetTextExtentPoint */
    WIN16DRV_GetTextMetrics,         /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    NULL,                            /* pInvertRgn */
    NULL,                            /* pLineTo */
    NULL,                            /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    NULL,                            /* pPaintRgn */
    NULL,                            /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolygon */
    NULL,                            /* pPolyline */
    NULL,                            /* pRealizePalette */
    NULL,                            /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipRgn */
    NULL,                            /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExt (optional) */
    NULL,                            /* pSetViewportOrg (optional) */
    NULL,                            /* pSetWindowExt (optional) */
    NULL,                            /* pSetWindowOrg (optional) */
    NULL,                            /* pStretchBlt */
    NULL,                            /* pStretchDIBits */
    NULL                             /* pTextOut */
};


#define MAX_PRINTER_DRIVERS 	16

static LOADED_PRINTER_DRIVER *gapLoadedPrinterDrivers[MAX_PRINTER_DRIVERS];


/**********************************************************************
 *	     WIN16DRV_Init
 */
BOOL32 WIN16DRV_Init(void)
{
    return DRIVER_RegisterDriver( NULL /* generic driver */, &WIN16DRV_Funcs );
        
}

/* Tempory functions, for initialising structures */
/* These values should be calculated, not hardcoded */
void InitTextXForm(LPTEXTXFORM lpTextXForm)
{
    lpTextXForm->txfHeight 	= 0x0001;
    lpTextXForm->txfWidth 	= 0x000c;
    lpTextXForm->txfEscapement 	= 0x0000;
    lpTextXForm->txfOrientation = 0x0000;
    lpTextXForm->txfWeight 	= 0x0190;
    lpTextXForm->txfItalic 	= 0x00;
    lpTextXForm->txfUnderline 	= 0x00;
    lpTextXForm->txfStrikeOut 	= 0x00; 
    lpTextXForm->txfOutPrecision = 0x02;
    lpTextXForm->txfClipPrecision = 0x01;
    lpTextXForm->txfAccelerator = 0x0001;
    lpTextXForm->txfOverhang 	= 0x0000;
}


void InitDrawMode(LPDRAWMODE lpDrawMode)
{
    lpDrawMode->Rop2		= 0x000d;       
    lpDrawMode->bkMode		= 0x0001;     
    lpDrawMode->bkColor		= 0x3fffffff;    
    lpDrawMode->TextColor	= 0x20000000;  
    lpDrawMode->TBreakExtra	= 0x0000;
    lpDrawMode->BreakExtra	= 0x0000; 
    lpDrawMode->BreakErr	= 0x0000;   
    lpDrawMode->BreakRem	= 0x0000;   
    lpDrawMode->BreakCount	= 0x0000; 
    lpDrawMode->CharExtra	= 0x0000;  
    lpDrawMode->LbkColor	= 0x00ffffff;   
    lpDrawMode->LTextColor	= 0x00000000;     
}
/* 
 * Thunking utility functions
 */

static BOOL32 AddData(SEGPTR *pSegPtr, const void *pData, int nSize, SEGPTR Limit)
{
    BOOL32 bRet = FALSE;
    char *pBuffer = PTR_SEG_TO_LIN((*pSegPtr));
    char *pLimit = PTR_SEG_TO_LIN(Limit);


    if ((pBuffer + nSize) < pLimit)
    {
	DWORD *pdw = (DWORD *)pSegPtr;
	SEGPTR SegPtrOld = *pSegPtr;
	SEGPTR SegPtrNew;

	dprintf_win16drv(stddeb, "AddData: Copying %d from %p to %p(0x%x)\n", nSize, pData, pBuffer, (UINT32)*pSegPtr);
	memcpy(pBuffer, pData, nSize); 
	SegPtrNew = (SegPtrOld + nSize + 1);
	*pdw = (DWORD)SegPtrNew;
    }
    return bRet;
}


static BOOL32 GetParamData(SEGPTR SegPtrSrc,void *pDataDest, int nSize)
{
    char *pSrc =  PTR_SEG_TO_LIN(SegPtrSrc);
    char *pDest = pDataDest;

    dprintf_win16drv(stddeb, "GetParamData: Copying %d from %lx(%lx) to %lx\n", nSize, (DWORD)pSrc, (DWORD)SegPtrSrc, (DWORD)pDataDest);
    memcpy(pDest, pSrc, nSize);
    return TRUE;
}


static void GetPrinterDriverFunctions(HINSTANCE16 hInst, LOADED_PRINTER_DRIVER *pLPD)
{
#define LoadPrinterDrvFunc(A,B) pLPD->fn[A] = \
      GetProcAddress16(hInst, MAKEINTRESOURCE(B))

      LoadPrinterDrvFunc(FUNC_CONTROL, ORD_CONTROL); 	       	/* 3 */
      LoadPrinterDrvFunc(FUNC_ENABLE, ORD_ENABLE);		/* 5 */
      LoadPrinterDrvFunc(FUNC_ENUMDFONTS, ORD_ENUMDFONTS);	/* 6 */
      LoadPrinterDrvFunc(FUNC_REALIZEOBJECT, ORD_REALIZEOBJECT);/* 10 */
      LoadPrinterDrvFunc(FUNC_EXTTEXTOUT, ORD_EXTTEXTOUT);	/* 14 */
      dprintf_win16drv (stddeb,"got func CONTROL 0x%p enable 0x%p enumDfonts 0x%p realizeobject 0x%p extextout 0x%p\n",
              pLPD->fn[FUNC_CONTROL],
              pLPD->fn[FUNC_ENABLE],
              pLPD->fn[FUNC_ENUMDFONTS],
              pLPD->fn[FUNC_REALIZEOBJECT],
              pLPD->fn[FUNC_EXTTEXTOUT]);
      

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
	    dprintf_win16drv(stddeb, "Comparing %s,%s\n",ptmpLPD->szDriver,pszDriver);
	    /* Found driver store info, exit loop */
	    if (lstrcmpi32A(ptmpLPD->szDriver, pszDriver) == 0)
	      pLPD = ptmpLPD;
	}
    }
    if (pLPD == NULL) printf("Couldn't find driver %s\n", pszDriver);
    return pLPD;
}
/* 
 * Load a printer driver, adding it self to the list of loaded drivers.
 */

static LOADED_PRINTER_DRIVER *LoadPrinterDriver(const char *pszDriver)
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
	printf("Too many printers drivers loaded\n");
	return NULL;
    }

    {
        char *drvName = malloc(strlen(pszDriver)+5);
        strcpy(drvName, pszDriver);
        strcat(drvName, ".DRV");
        hInst = LoadLibrary16(drvName);
    }
    dprintf_win16drv(stddeb, "Loaded the library\n");

    
    if (hInst <= 32)
    {
	/* Failed to load driver */
	fprintf(stderr, "Failed to load printer driver %s\n", pszDriver);
    }
    else
    {
	HANDLE16 hHandle;

	/* Allocate some memory for printer driver info */
	pLPD = malloc(sizeof(LOADED_PRINTER_DRIVER));
	memset(pLPD, 0 , sizeof(LOADED_PRINTER_DRIVER));
	
	pLPD->hInst = hInst;
	strcpy(pLPD->szDriver,pszDriver);

	/* Get DS for the printer module */
	pLPD->ds_reg = hInst;

	dprintf_win16drv(stddeb, "DS for %s is %x\n", pszDriver, pLPD->ds_reg);

	/* Get address of printer driver functions */
	GetPrinterDriverFunctions(hInst, pLPD);

	/* Set initial usage count */
	pLPD->nUsageCount = 1;

	/* Create a thunking buffer */
	hHandle = GlobalAlloc16(GHND, (1024 * 8));
	pLPD->hThunk = hHandle;
	pLPD->ThunkBufSegPtr = WIN16_GlobalLock16(hHandle);
	pLPD->ThunkBufLimit = pLPD->ThunkBufSegPtr + (1024*8);

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

    dprintf_win16drv(stddeb, "PRTDRV_Control: %08x 0x%x %08lx %08lx\n", (unsigned int)lpDestDev, wfunction, lpInData, lpOutData);

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP3, lP4;
	WORD wP2;

	if (pLPD->fn[FUNC_CONTROL] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_Control: Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	wP2 = wfunction;
	lP3 = (SEGPTR)lpInData;
	lP4 = (SEGPTR)lpOutData;

	wRet = CallTo16_word_lwll(pLPD->fn[FUNC_CONTROL], 
                                  lP1, wP2, lP3, lP4);
    }
    dprintf_win16drv(stddeb, "PRTDRV_Control: return %x\n", wRet);
    return wRet;

    return 0;
}

/*
 *	Enable (ordinal 5)
 */
static WORD PRTDRV_Enable(LPVOID lpDevInfo, WORD wStyle, LPCSTR  lpDestDevType, 
                          LPCSTR lpDeviceName, LPCSTR lpOutputFile, LPVOID lpData)
{
    WORD wRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;

    dprintf_win16drv(stddeb, "PRTDRV_Enable: %s %s\n",lpDestDevType, lpOutputFile);

    /* Get the printer driver info */
    if (wStyle == INITPDEVICE)
    {
	pLPD = FindPrinterDriverFromPDEVICE((SEGPTR)lpDevInfo);
    }
    else
    {
	pLPD = FindPrinterDriverFromName((char *)lpDeviceName);
    }
    if (pLPD != NULL)
    {
	LONG lP1, lP3, lP4, lP5;
	WORD wP2;
	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_ENABLE] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_Enable: Not supported by driver\n");
	    return 0;
	}

	if (wStyle == INITPDEVICE)
	{
	    /* All ready a 16 address */
	    lP1 = (SEGPTR)lpDevInfo;
	}
	else
	{
	    /* 32 bit data */
	    lP1 = SegPtr;
	    nSize = sizeof(DeviceCaps);
	    AddData(&SegPtr, lpDevInfo, nSize, Limit);	
	}
	
	wP2 = wStyle;
	
	lP3 = SegPtr;
	nSize = strlen(lpDestDevType) + 1;
	AddData(&SegPtr, lpDestDevType, nSize, Limit);	

	lP4 = SegPtr; 
	nSize = strlen(lpOutputFile) + 1;
	AddData(&SegPtr, lpOutputFile, nSize, Limit);	

	lP5 = (LONG)lpData;
        

	wRet = CallTo16_word_lwlll(pLPD->fn[FUNC_ENABLE], 
				   lP1, wP2, lP3, lP4, lP5);

	/* Get the data back */
	if (lP1 != 0 && wStyle != INITPDEVICE)
	{
	    nSize = sizeof(DeviceCaps);
	    GetParamData(lP1, lpDevInfo, nSize);
	}
    }
    dprintf_win16drv(stddeb, "PRTDRV_Enable: return %x\n", wRet);
    return wRet;
}


/*
 * EnumCallback (GDI.158)
 * 
 * This is the callback function used when EnumDFonts is called. 
 * (The printer drivers uses it to pass info on available fonts).
 *
 * lpvClientData is the pointer passed to EnumDFonts, which points to a WEPFC
 * structure (WEPFC = WINE_ENUM_PRINTER_FONT_CALLBACK).  This structure 
 * contains infomation on how to store the data passed .
 *
 * There are two modes:
 * 	1) Just count the number of fonts available.
 * 	2) Store all font data passed.
 */
WORD WineEnumDFontCallback(LPLOGFONT16 lpLogFont, LPTEXTMETRIC16 lpTextMetrics, 
			   WORD wFontType, LONG lpvClientData) 
{
    int wRet = 0;
    WEPFC *pWEPFC = (WEPFC *)lpvClientData; 
    
    /* Make sure we have the right structure */
    if (pWEPFC != NULL )
    {
        dprintf_win16drv(stddeb, "mode is 0x%x\n",pWEPFC->nMode);
        
	switch (pWEPFC->nMode)
	{
	    /* Count how many fonts */
	  case 1:
	    pWEPFC->nCount++;
	    break;

	    /* Store the fonts in the printer driver structure */
	  case 2:
	  {
	      PRINTER_FONTS_INFO *pPFI;
                  
	      dprintf_win16drv(stddeb, "WineEnumDFontCallback: Found %s %x\n", 
		     lpLogFont->lfFaceName, wFontType);
	      
	      pPFI = &pWEPFC->pLPD->paPrinterFonts[pWEPFC->nCount];
	      memcpy(&(pPFI->lf), lpLogFont, sizeof(LOGFONT16));
	      memcpy(&(pPFI->tm), lpTextMetrics, sizeof(TEXTMETRIC16));	      
	      pWEPFC->nCount++;
              
	  }
	    break;
	}
	wRet = 1;
    }
    dprintf_win16drv(stddeb, "WineEnumDFontCallback: returnd %d\n", wRet);
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

    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts:\n");

    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP2, lP3, lP4;

	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_ENUMDFONTS] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts: Not supported by driver\n");
	    return 0;
	}

	lP1 = (SEGPTR)lpDestDev;
	
	if (lpFaceName == NULL)
	{
	    lP2 = 0;
	}
	else
	{
	    lP2 = SegPtr;
	    nSize = strlen(lpFaceName) + 1;
	    AddData(&SegPtr, lpFaceName, nSize, Limit);	
	}

	lP3 = (LONG)lpCallbackFunc; 

	lP4 = (LONG)lpClientData;
        
	wRet = CallTo16_word_llll(pLPD->fn[FUNC_ENUMDFONTS], 
                                  lP1, lP2, lP3, lP4);
    }
    else 
        printf("Failed to find device\n");
    
    dprintf_win16drv(stddeb, "PRTDRV_EnumDFonts: return %x\n", wRet);
    return wRet;
}

/* 
 * RealizeObject (ordinal 10)
 */
DWORD PRTDRV_RealizeObject(LPPDEVICE lpDestDev, WORD wStyle, 
		    LPVOID lpInObj, LPVOID lpOutObj,
                    LPTEXTXFORM lpTextXForm)
{
    WORD dwRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    
    dprintf_win16drv(stddeb, "PRTDRV_RealizeObject:\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP3, lP4, lP5;  
	WORD wP2;
	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_REALIZEOBJECT] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_RealizeObject: Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wStyle;
	
	lP3 = SegPtr;
	switch (wStyle)
	{
        case 3: 
            nSize = sizeof(LOGFONT16); 
            break;
        default:
	    printf("PRTDRV_RealizeObject: Object type %d not supported\n", wStyle);
            nSize = 0;
            
	}
	AddData(&SegPtr, lpInObj, nSize, Limit);	
	
	lP4 = (LONG)lpOutObj;

	if (lpTextXForm != NULL)
	{
	    lP5 = SegPtr;
	    nSize = sizeof(TEXTXFORM);
	    AddData(&SegPtr, lpTextXForm, nSize, Limit);	
	}
	else
	  lP5 = 0L;

	dwRet = CallTo16_long_lwlll(pLPD->fn[FUNC_REALIZEOBJECT], 
                                    lP1, wP2, lP3, lP4, lP5);

    }
    dprintf_win16drv(stddeb, "PRTDRV_RealizeObject: return %x\n", dwRet);
    return dwRet;
}


BOOL32 WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODE16* initData )
{
    LOADED_PRINTER_DRIVER *pLPD;
    WORD wRet;
    DeviceCaps *printerDevCaps;
    FARPROC16 pfnCallback;
    int nPDEVICEsize;
    PDEVICE_HEADER *pPDH;
    WIN16DRV_PDEVICE *physDev;

    /* Realizing fonts */
    TEXTXFORM TextXForm;
    int nSize;
    char printerEnabled[20];
    PROFILE_GetWineIniString( "wine", "printer", "off",
                             printerEnabled, sizeof(printerEnabled) );
    if (strcmp(printerEnabled,"on"))
    {
        printf("WIN16DRV_CreateDC disabled in wine.conf file\n");
        return FALSE;
    }

    dprintf_win16drv(stddeb, "In creatdc for (%s,%s,%s) initData 0x%p\n",driver, device, output, initData);

    physDev = (WIN16DRV_PDEVICE *)HeapAlloc( SystemHeap, 0, sizeof(*physDev) );
    if (!physDev) return FALSE;
    dc->physDev = physDev;

    pLPD = LoadPrinterDriver(driver);
    if (pLPD == NULL)
    {
	dprintf_win16drv(stddeb, "LPGDI_CreateDC: Failed to find printer driver\n");
        HeapFree( SystemHeap, 0, physDev );
        return FALSE;
    }
    dprintf_win16drv(stddeb, "windevCreateDC pLPD 0x%p\n", pLPD);

    /* Now Get the device capabilities from the printer driver */
    
    printerDevCaps = (DeviceCaps *) malloc(sizeof(DeviceCaps));
    memset(printerDevCaps, 0, sizeof(DeviceCaps));

    /* Get GDIINFO which is the same as a DeviceCaps structure */
    wRet = PRTDRV_Enable(printerDevCaps, GETGDIINFO, device, driver, output,NULL); 

    /* Add this to the DC */
    dc->w.devCaps = printerDevCaps;
    dc->w.hVisRgn = CreateRectRgn32(0, 0, dc->w.devCaps->horzRes, dc->w.devCaps->vertRes);
    dc->w.bitsPerPixel = dc->w.devCaps->bitsPixel;
    
    printf("Got devcaps width %d height %d bits %d planes %d\n",
           dc->w.devCaps->horzRes,
           dc->w.devCaps->vertRes, 
           dc->w.devCaps->bitsPixel,
           dc->w.devCaps->planes);

    /* Now we allocate enough memory for the PDEVICE structure */
    /* The size of this varies between printer drivers */
    /* This PDEVICE is used by the printer DRIVER not by the GDI so must */
    /* be accessable from 16 bit code */
    nPDEVICEsize = dc->w.devCaps->pdeviceSize + sizeof(PDEVICE_HEADER);

    /* TTD Shouldn't really do pointer arithmetic on segment points */
    physDev->segptrPDEVICE = WIN16_GlobalLock16(GlobalAlloc16(GHND, nPDEVICEsize))+sizeof(PDEVICE_HEADER);
    *(BYTE *)(PTR_SEG_TO_LIN(physDev->segptrPDEVICE)+0) = 'N'; 
    *(BYTE *)(PTR_SEG_TO_LIN(physDev->segptrPDEVICE)+1) = 'B'; 

    /* Set up the header */
    pPDH = (PDEVICE_HEADER *)(PTR_SEG_TO_LIN(physDev->segptrPDEVICE) - sizeof(PDEVICE_HEADER)); 
    pPDH->pLPD = pLPD;
    
    dprintf_win16drv(stddeb, "PRTDRV_Enable: PDEVICE allocated %08lx\n",(DWORD)(physDev->segptrPDEVICE));
    
    /* Now get the printer driver to initialise this data */
    wRet = PRTDRV_Enable((LPVOID)physDev->segptrPDEVICE, INITPDEVICE, device, driver, output, NULL); 

    /* Now enumerate the fonts supported by the printer driver*/
    /* GDI.158 is EnumCallback, which is called by the 16bit printer driver */
    /* passing information on the available fonts */
    if (pLPD->paPrinterFonts == NULL)
    {
	pfnCallback = GetProcAddress16(GetModuleHandle("GDI"), 
				     (MAKEINTRESOURCE(158)));
        
	if (pfnCallback != NULL)
	{
	    WEPFC wepfc;
	    
	    wepfc.nMode = 1;
	    wepfc.nCount = 0;
	    wepfc.pLPD = pLPD;
	    
	    /* First count the number of fonts */
            
	    PRTDRV_EnumDFonts(physDev->segptrPDEVICE, NULL, pfnCallback, 
			      (void *)&wepfc);
	    
	    /* Allocate a buffer to store all of the fonts */
	    pLPD->nPrinterFonts = wepfc.nCount;
            dprintf_win16drv(stddeb, "Got %d fonts\n",wepfc.nCount);
            
	    if (wepfc.nCount > 0)
	    {

		pLPD->paPrinterFonts = malloc(sizeof(PRINTER_FONTS_INFO) * wepfc.nCount);
		
		/* Now get all of the fonts */
		wepfc.nMode = 2;
		wepfc.nCount = 0;
		PRTDRV_EnumDFonts(physDev->segptrPDEVICE, NULL, pfnCallback, 
				  (void *)&wepfc);
	    }
	}
    }
		
    /* Select the first font into the DC */
    /* Set up the logfont */
    memcpy(&physDev->lf, 
	   &pLPD->paPrinterFonts[0].lf, 
	   sizeof(LOGFONT16));

    /* Set up the textmetrics */
    memcpy(&physDev->tm, 
	   &pLPD->paPrinterFonts[0].tm, 
	   sizeof(TEXTMETRIC16));

#ifdef SUPPORT_REALIZED_FONTS
    /* TTD should calculate this */
    InitTextXForm(&TextXForm);
    
    /* First get the size of the realized font */
    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, OBJ_FONT,
				 &pLPD->paPrinterFonts[0], NULL, 
				 NULL);
    
    physDev->segptrFontInfo = WIN16_GlobalLock16(GlobalAlloc16(GHND, nSize));
    /* Realize the font */
    PRTDRV_RealizeObject(physDev->segptrPDEVICE, OBJ_FONT,
			 &pLPD->paPrinterFonts[0], 
			 (LPVOID)physDev->segptrFontInfo, 
			 &TextXForm);
    /* Quick look at structure */
    if (physDev->segptrFontInfo)
    {  
	FONTINFO *p = (FONTINFO *)PTR_SEG_TO_LIN(physDev->segptrFontInfo);

	dprintf_win16drv(stddeb, "T:%d VR:%d HR:%d, F:%d L:%d\n",
	       p->dfType,
	       p->dfVertRes, p->dfHorizRes,
	       p->dfFirstCHAR, p->dfLastCHAR
	       );
    }

#endif
    /* TTD Lots more to do here */

    return TRUE;
}


/* 
 * Escape (GDI.38)
 */
static INT32 WIN16DRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
                              SEGPTR lpInData, SEGPTR lpOutData )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    int nRet = 0;

    /* We should really process the nEscape parameter, but for now just
       pass it all to the driver */
    if (dc != NULL && physDev->segptrPDEVICE != 0)
    {
	switch(nEscape)
	{
	  case 0x9:
	    printf("Escape: SetAbortProc ignored\n");
	    break;

	  default:
	    nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape, 
				  lpInData, lpOutData);
	}
    }
    else
	fprintf(stderr, "Escape(nEscape = %04x)\n", nEscape);      
    return nRet;
}

DWORD PRTDRV_ExtTextOut(LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg,
                        RECT16 *lpClipRect, LPCSTR lpString, WORD wCount, 
                        SEGPTR lpFontInfo, LPDRAWMODE lpDrawMode, 
                        LPTEXTXFORM lpTextXForm, SHORT *lpCharWidths,
                        RECT16 *     lpOpaqueRect, WORD wOptions)
{
    DWORD dwRet = 0;
    LOADED_PRINTER_DRIVER *pLPD = NULL;
    
    dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut:\n");
    
    if ((pLPD = FindPrinterDriverFromPDEVICE(lpDestDev)) != NULL)
    {
	LONG lP1, lP4, lP5, lP7, lP8, lP9, lP10, lP11;  
	WORD wP2, wP3, wP6, wP12;
	SEGPTR SegPtr = pLPD->ThunkBufSegPtr;
	SEGPTR Limit = pLPD->ThunkBufLimit;
	int   nSize;

	if (pLPD->fn[FUNC_EXTTEXTOUT] == NULL)
	{
	    dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut: Not supported by driver\n");
	    return 0;
	}

	lP1 = lpDestDev;
	wP2 = wDestXOrg;
	wP3 = wDestYOrg;
	
	if (lpClipRect != NULL)
	{
	    lP4 = SegPtr;
	    nSize = sizeof(RECT16);
            dprintf_win16drv(stddeb, "Adding lpClipRect\n");
            
	    AddData(&SegPtr, lpClipRect, nSize, Limit);	
	}
	else
	  lP4 = 0L;

	if (lpString != NULL)
	{
	    /* TTD WARNING THIS STRING ISNT NULL TERMINATED */
	    lP5 = SegPtr;
	    nSize = strlen(lpString);
            dprintf_win16drv(stddeb, "Adding string size %d\n",nSize);
            
	    AddData(&SegPtr, lpString, nSize, Limit);	
	}
	else
	  lP5 = 0L;
	
	wP6 = wCount;
	
	/* This should be realized by the driver, so in 16bit data area */
	lP7 = lpFontInfo;
	
	if (lpDrawMode != NULL)
	{
	    lP8 = SegPtr;
	    nSize = sizeof(DRAWMODE);
            dprintf_win16drv(stddeb, "adding lpDrawMode\n");
            
	    AddData(&SegPtr, lpDrawMode, nSize, Limit);	
	}
	else
	  lP8 = 0L;
	
	if (lpTextXForm != NULL)
	{
	    lP9 = SegPtr;
	    nSize = sizeof(TEXTXFORM);
            dprintf_win16drv(stddeb, "Adding TextXForm\n");
	    AddData(&SegPtr, lpTextXForm, nSize, Limit);	
	}
	else
	  lP9 = 0L;
	
	if (lpCharWidths != NULL) 
	  dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut: Char widths not supported\n");
	lP10 = 0;
	
	if (lpOpaqueRect != NULL)
	{
	    lP11 = SegPtr;
	    nSize = sizeof(RECT16);
            dprintf_win16drv(stddeb, "Adding opaqueRect\n");
	    AddData(&SegPtr, lpOpaqueRect, nSize, Limit);	
	}
	else
	  lP11 = 0L;
	
	wP12 = wOptions;
	dprintf_win16drv(stddeb, "Calling exttextout 0x%lx 0x%x 0x%x 0x%lx\n0x%lx 0x%x 0x%lx 0x%lx\n"
               "0x%lx 0x%lx 0x%lx 0x%x\n",lP1, wP2, wP3, lP4, 
					   lP5, wP6, lP7, lP8, lP9, lP10,
					   lP11, wP12);
	dwRet = CallTo16_long_lwwllwlllllw(pLPD->fn[FUNC_EXTTEXTOUT], 
                                           lP1, wP2, wP3, lP4, 
					   lP5, wP6, lP7, lP8, lP9, lP10,
					   lP11, wP12);
    }
    dprintf_win16drv(stddeb, "PRTDRV_ExtTextOut: return %lx\n", dwRet);
    return dwRet;
}


/*
 * ExtTextOut (GDI.351)
 */
static BOOL16 windrvExtTextOut16( DC *dc, INT16 x, INT16 y, UINT16 flags, const RECT16 * lprect,
                                 LPCSTR str, UINT16 count, const INT16 *lpDx)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL32 bRet = 1;
    DRAWMODE DrawMode;
    LPDRAWMODE lpDrawMode = &DrawMode;
    TEXTXFORM TextXForm;
    LPTEXTXFORM lpTextXForm = &TextXForm;
    RECT16 rcClipRect;
    RECT16 * lpClipRect = &rcClipRect;
    RECT16 rcOpaqueRect;
    RECT16 *lpOpaqueRect = &rcOpaqueRect;
    WORD wOptions = 0;
    WORD wCount = count;

    static BOOL32 bInit = FALSE;
    


    if (count == 0)
      return FALSE;

    dprintf_win16drv(stddeb, "LPGDI_ExtTextOut: %04x %d %d %x %p %*s %p\n", dc->hSelf, x, y, 
	   flags,  lprect, count > 0 ? count : 8, str, lpDx);

    InitTextXForm(lpTextXForm);
    InitDrawMode(lpDrawMode);

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
	lpClipRect->left = 0;
	lpClipRect->top = 0;
	lpClipRect->right = 0x3fc;
	lpClipRect->bottom = 0x42;
	lpOpaqueRect->left = x;
	lpOpaqueRect->top = y;
	lpOpaqueRect->right = 0x3a1;
	lpOpaqueRect->bottom = 0x01;
/*
	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, x, y, 
				  lpClipRect, str, 
				  wCount,  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, lpDx, lpOpaqueRect, wOptions);
*/
	dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, x, y, 
				  NULL, str, 
				  wCount,  physDev->segptrFontInfo, lpDrawMode, 
				  lpTextXForm, NULL, NULL, wOptions);
    }
    return bRet;
}

/****************** misc. printer releated functions */

/*
 * The following function should implement a queing system
 */
#ifndef HPQ 
#define HPQ WORD
#endif
HPQ CreatePQ(int size) { printf("CreatePQ: %d\n",size); return 1; }
int DeletePQ(HPQ hPQ) { printf("DeletePQ: %x\n", hPQ); return 0; }
int ExtractPQ(HPQ hPQ) { printf("ExtractPQ: %x\n", hPQ); return 0; }
int InsertPQ(HPQ hPQ, int tag, int key) 
{ printf("ExtractPQ: %x %d %d\n", hPQ, tag, key);  return 0; }
int MinPQ(HPQ hPQ) { printf("MinPQ: %x\n", hPQ); return 0; }
int SizePQ(HPQ hPQ, int sizechange) 
{  printf("SizePQ: %x %d\n", hPQ, sizechange); return -1; }

/* 
 * The following functions implement part of the spooling process to 
 * print manager.  I would like to see wine have a version of print managers
 * that used LPR/LPD.  For simplicity print jobs will be sent to a file for
 * now.
 */
typedef struct PRINTJOB
{
    char	*pszOutput;
    char 	*pszTitle;
    HDC16  	hDC;
    HANDLE16 	hHandle;
    int		nIndex;
    int		fd;
} PRINTJOB, *PPRINTJOB;

#define MAX_PRINT_JOBS 1
#define SP_ERROR -1
#define SP_OUTOFDISK -4
#define SP_OK 1

PPRINTJOB gPrintJobsTable[MAX_PRINT_JOBS];


static PPRINTJOB FindPrintJobFromHandle(HANDLE16 hHandle)
{
    return gPrintJobsTable[0];
}

/* TTD Need to do some DOS->UNIX file conversion here */
static int CreateSpoolFile(LPSTR pszOutput)
{
    int fd;
    char szSpoolFile[32];

    /* TTD convert the 'output device' into a spool file name */

    if (pszOutput == NULL || *pszOutput == '\0')
	strcpy(szSpoolFile,"lp.out");
    else
	strcpy(szSpoolFile, pszOutput);

    if ((fd = open(szSpoolFile, O_CREAT | O_TRUNC | O_WRONLY , 0600)) < 0)
    {
	printf("Failed to create spool file %s, errno = %d\n", szSpoolFile, errno);
    }
    return fd;
}

static int FreePrintJob(HANDLE16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob;

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL)
    {
	gPrintJobsTable[pPrintJob->nIndex] = NULL;
	free(pPrintJob->pszOutput);
	free(pPrintJob->pszTitle);
	if (pPrintJob->fd >= 0) close(pPrintJob->fd);
	free(pPrintJob);
	nRet = SP_OK;
    }
    return nRet;
}

HANDLE16 OpenJob(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC)
{
    HANDLE16 hHandle = SP_ERROR;
    PPRINTJOB pPrintJob;

    dprintf_win16drv(stddeb, "OpenJob: \"%s\" \"%s\" %04x\n", lpOutput, lpTitle, hDC);

    pPrintJob = gPrintJobsTable[0];
    if (pPrintJob == NULL)
    {
	int fd;

	/* Try an create a spool file */
	fd = CreateSpoolFile(lpOutput);
	if (fd >= 0)
	{
	    hHandle = 1;

	    pPrintJob = malloc(sizeof(PRINTJOB));
	    memset(pPrintJob, 0, sizeof(PRINTJOB));

	    pPrintJob->pszOutput = strdup(lpOutput);
	    pPrintJob->pszTitle = strdup(lpTitle);
	    pPrintJob->hDC = hDC;
	    pPrintJob->fd = fd;
	    pPrintJob->nIndex = 0;
	    pPrintJob->hHandle = hHandle; 
	    gPrintJobsTable[pPrintJob->nIndex] = pPrintJob; 
	}
    }
    dprintf_win16drv(stddeb, "OpenJob: return %04x\n", hHandle);
    return hHandle;
}

int CloseJob(HANDLE16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    dprintf_win16drv(stddeb, "CloseJob: %04x\n", hJob);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL)
    {
	/* Close the spool file */
	close(pPrintJob->fd);
	FreePrintJob(hJob);
	nRet  = 1;
    }
    return nRet;
}

int WriteSpool(HANDLE16 hJob, LPSTR lpData, WORD cch)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    dprintf_win16drv(stddeb, "WriteSpool: %04x %08lx %04x\n", hJob, (DWORD)lpData, cch);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL && pPrintJob->fd >= 0 && cch)
    {
	if (write(pPrintJob->fd, lpData, cch) != cch)
	  nRet = SP_OUTOFDISK;
	else
	  nRet = cch;
    }
    return nRet;
}

int WriteDialog(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg)
{
    int nRet = 0;

    dprintf_win16drv(stddeb, "WriteDialog: %04x %04x \"%s\"\n", hJob,  cchMsg, lpMsg);

    nRet = MessageBox16(0, lpMsg, "Printing Error", MB_OKCANCEL);
    return nRet;
}

int DeleteJob(HANDLE16 hJob, WORD wNotUsed)
{
    int nRet;

    dprintf_win16drv(stddeb, "DeleteJob: %04x\n", hJob);

    nRet = FreePrintJob(hJob);
    return nRet;
}

/* 
 * The following two function would allow a page to be sent to the printer
 * when it has been processed.  For simplicity they havn't been implemented.
 * This means a whole job has to be processed before it is sent to the printer.
 */
int StartSpoolPage(HANDLE16 hJob)
{
    dprintf_win16drv(stddeb, "StartSpoolPage GDI.246 unimplemented\n");
    return 1;

}
int EndSpoolPage(HANDLE16 hJob)
{
    dprintf_win16drv(stddeb, "EndSpoolPage GDI.247 unimplemented\n");
    return 1;
}


DWORD GetSpoolJob(int nOption, LONG param)
{
    DWORD retval = 0;
    dprintf_win16drv(stddeb, "In GetSpoolJob param 0x%lx noption %d\n",param, nOption);
    return retval;
}

/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <string.h>
#include <ctype.h>
#include "win16drv.h"
#include "gdi.h"
#include "bitmap.h"
#include "heap.h"
#include "font.h"
#include "options.h"
#include "xmalloc.h"
#include "debugtools.h"
#include "dc.h"

DEFAULT_DEBUG_CHANNEL(win16drv)

#define SUPPORT_REALIZED_FONTS 1
#include "pshpack1.h"
typedef struct
{
  SHORT nSize;
  SEGPTR lpindata;
  SEGPTR lpFont;
  SEGPTR lpXForm;
  SEGPTR lpDrawMode;
} EXTTEXTDATA, *LPEXTTEXTDATA;
#include "poppack.h"

SEGPTR		win16drv_SegPtr_TextXForm;
LPTEXTXFORM16 	win16drv_TextXFormP;
SEGPTR		win16drv_SegPtr_DrawMode;
LPDRAWMODE 	win16drv_DrawModeP;


static BOOL WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                                 LPCSTR output, const DEVMODEA* initData );
static INT WIN16DRV_Escape( DC *dc, INT nEscape, INT cbInput, 
                              SEGPTR lpInData, SEGPTR lpOutData );

static const DC_FUNCTIONS WIN16DRV_Funcs =
{
    NULL,                            /* pAbortDoc */
    NULL,                            /* pArc */
    NULL,                            /* pBitBlt */
    NULL,                            /* pBitmapBits */
    NULL,                            /* pChord */
    NULL,                            /* pCreateBitmap */
    WIN16DRV_CreateDC,               /* pCreateDC */
    NULL,                            /* pCreateDIBSection */
    NULL,                            /* pCreateDIBSection16 */
    NULL,                            /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    WIN16DRV_DeviceCapabilities,     /* pDeviceCapabilities */
    WIN16DRV_Ellipse,                /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    WIN16DRV_EnumDeviceFonts,        /* pEnumDeviceFonts */
    WIN16DRV_Escape,                 /* pEscape */
    NULL,                            /* pExcludeClipRect */
    WIN16DRV_ExtDeviceMode,          /* pExtDeviceMode */
    NULL,                            /* pExtFloodFill */
    WIN16DRV_ExtTextOut,             /* pExtTextOut */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFrameRgn */
    WIN16DRV_GetCharWidth,           /* pGetCharWidth */
    NULL,                            /* pGetPixel */
    WIN16DRV_GetTextExtentPoint,     /* pGetTextExtentPoint */
    WIN16DRV_GetTextMetrics,         /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pInvertRgn */
    WIN16DRV_LineTo,                 /* pLineTo */
    NULL,                            /* pLoadOEMResource */
    WIN16DRV_MoveToEx,               /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrgEx */
    NULL,                            /* pOffsetWindowOrgEx */
    NULL,                            /* pPaintRgn */
    WIN16DRV_PatBlt,                 /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolyPolyline */
    WIN16DRV_Polygon,                /* pPolygon */
    WIN16DRV_Polyline,               /* pPolyline */
    NULL,                            /* pPolyBezier */
    NULL,                            /* pRealizePalette */
    WIN16DRV_Rectangle,              /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExtEx */
    NULL,                            /* pScaleWindowExtEx */
    NULL,                            /* pSelectClipRgn */
    WIN16DRV_SelectObject,           /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode */
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
    NULL,                            /* pSetViewportExtEx */
    NULL,                            /* pSetViewportOrgEx */
    NULL,                            /* pSetWindowExtEx */
    NULL,                            /* pSetWindowOrgEx */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    NULL,                            /* pStretchBlt */
    NULL                             /* pStretchDIBits */
};





/**********************************************************************
 *	     WIN16DRV_Init
 */
BOOL WIN16DRV_Init(void)
{
    return DRIVER_RegisterDriver( NULL /* generic driver */, &WIN16DRV_Funcs );
        
}

/* Tempory functions, for initialising structures */
/* These values should be calculated, not hardcoded */
void InitTextXForm(LPTEXTXFORM16 lpTextXForm)
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

BOOL WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA* initData )
{
    LOADED_PRINTER_DRIVER *pLPD;
    WORD wRet;
    DeviceCaps *printerDevCaps;
    int nPDEVICEsize;
    PDEVICE_HEADER *pPDH;
    WIN16DRV_PDEVICE *physDev;
    char printerEnabled[20];
    PROFILE_GetWineIniString( "wine", "printer", "off",
                             printerEnabled, sizeof(printerEnabled) );
    if (lstrcmpiA(printerEnabled,"on"))
    {
        MESSAGE("Printing disabled in wine.conf or .winerc file\n");
        MESSAGE("Use \"printer=on\" in the \"[wine]\" section to enable it.\n");
        return FALSE;
    }

    TRACE("In creatdc for (%s,%s,%s) initData 0x%p\n",
	  driver, device, output, initData);

    physDev = (WIN16DRV_PDEVICE *)HeapAlloc( SystemHeap, 0, sizeof(*physDev) );
    if (!physDev) return FALSE;
    dc->physDev = physDev;

    pLPD = LoadPrinterDriver(driver);
    if (pLPD == NULL)
    {
	WARN("Failed to find printer driver\n");
        HeapFree( SystemHeap, 0, physDev );
        return FALSE;
    }
    TRACE("windevCreateDC pLPD 0x%p\n", pLPD);

    /* Now Get the device capabilities from the printer driver */
    
    printerDevCaps = (DeviceCaps *) xmalloc(sizeof(DeviceCaps));
    memset(printerDevCaps, 0, sizeof(DeviceCaps));

    if(!output) output = "LPT1:";
    /* Get GDIINFO which is the same as a DeviceCaps structure */
    wRet = PRTDRV_Enable(printerDevCaps, GETGDIINFO, device, driver, output,NULL); 

    /* Add this to the DC */
    dc->w.devCaps = printerDevCaps;
    dc->w.hVisRgn = CreateRectRgn(0, 0, dc->w.devCaps->horzRes, dc->w.devCaps->vertRes);
    dc->w.bitsPerPixel = dc->w.devCaps->bitsPixel;
    
    TRACE("Got devcaps width %d height %d bits %d planes %d\n",
	  dc->w.devCaps->horzRes, dc->w.devCaps->vertRes, 
	  dc->w.devCaps->bitsPixel, dc->w.devCaps->planes);

    /* Now we allocate enough memory for the PDEVICE structure */
    /* The size of this varies between printer drivers */
    /* This PDEVICE is used by the printer DRIVER not by the GDI so must */
    /* be accessable from 16 bit code */
    nPDEVICEsize = dc->w.devCaps->pdeviceSize + sizeof(PDEVICE_HEADER);

    /* TTD Shouldn't really do pointer arithmetic on segment points */
    physDev->segptrPDEVICE = WIN16_GlobalLock16(GlobalAlloc16(GHND, nPDEVICEsize))+sizeof(PDEVICE_HEADER);
    *((BYTE *)PTR_SEG_TO_LIN(physDev->segptrPDEVICE)+0) = 'N'; 
    *((BYTE *)PTR_SEG_TO_LIN(physDev->segptrPDEVICE)+1) = 'B'; 

    /* Set up the header */
    pPDH = (PDEVICE_HEADER *)((BYTE*)PTR_SEG_TO_LIN(physDev->segptrPDEVICE) - sizeof(PDEVICE_HEADER)); 
    pPDH->pLPD = pLPD;
    
    TRACE("PDEVICE allocated %08lx\n",(DWORD)(physDev->segptrPDEVICE));
    
    /* Now get the printer driver to initialise this data */
    wRet = PRTDRV_Enable((LPVOID)physDev->segptrPDEVICE, INITPDEVICE, device, driver, output, NULL); 

    physDev->FontInfo = NULL;
    physDev->BrushInfo = NULL;
    physDev->PenInfo = NULL;
    win16drv_SegPtr_TextXForm = WIN16_GlobalLock16(GlobalAlloc16(GHND, sizeof(TEXTXFORM16)));
    win16drv_TextXFormP = PTR_SEG_TO_LIN(win16drv_SegPtr_TextXForm);
    
    InitTextXForm(win16drv_TextXFormP);

    /* TTD Lots more to do here */
    win16drv_SegPtr_DrawMode = WIN16_GlobalLock16(GlobalAlloc16(GHND, sizeof(DRAWMODE)));
    win16drv_DrawModeP = PTR_SEG_TO_LIN(win16drv_SegPtr_DrawMode);
    
    InitDrawMode(win16drv_DrawModeP);

    return TRUE;
}

BOOL WIN16DRV_PatBlt( struct tagDC *dc, INT left, INT top,
			INT width, INT height, DWORD rop )
{
  
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL bRet = 0;

    bRet = PRTDRV_StretchBlt( physDev->segptrPDEVICE, left, top, width, height, (SEGPTR)NULL, 0, 0, width, height,
                       PATCOPY, physDev->BrushInfo, win16drv_SegPtr_DrawMode, NULL);

    return bRet;
}
/* 
 * Escape (GDI.38)
 */
static INT WIN16DRV_Escape( DC *dc, INT nEscape, INT cbInput, 
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
	  case ENABLEPAIRKERNING:
	    FIXME("Escape: ENABLEPAIRKERNING ignored.\n");
            nRet = 1;
	    break;
	  case GETPAIRKERNTABLE:
	    FIXME("Escape: GETPAIRKERNTABLE ignored.\n");
            nRet = 0;
	    break;
          case SETABORTPROC: {
		/* FIXME: The AbortProc should be called:
		- After every write to printer port or spool file
		- Several times when no more disk space
		- Before every metafile record when GDI does banding
		*/ 

       /* save the callback address and call Control with hdc as lpInData */
	    HDC16 *seghdc = SEGPTR_NEW(HDC16);
	    *seghdc = dc->hSelf;
	    dc->w.spfnPrint = (FARPROC16)lpInData;
            nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
				  SEGPTR_GET(seghdc), lpOutData);
	    SEGPTR_FREE(seghdc);
	    break;
	  }

          case NEXTBAND:
            {
              LPPOINT16 newInData =  SEGPTR_NEW(POINT16);

              nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
                                    SEGPTR_GET(newInData), lpOutData);
              SEGPTR_FREE(newInData);
              break;
            }

	  case GETEXTENDEDTEXTMETRICS:
            {
	      EXTTEXTDATA *textData = SEGPTR_NEW(EXTTEXTDATA);

              textData->nSize = cbInput;
              textData->lpindata = lpInData;
              textData->lpFont = SEGPTR_GET( physDev->FontInfo );
              textData->lpXForm = win16drv_SegPtr_TextXForm;
              textData->lpDrawMode = win16drv_SegPtr_DrawMode;
              nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
                                    SEGPTR_GET(textData), lpOutData);
              SEGPTR_FREE(textData);
            }
          break;
          case STARTDOC:
	    nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
				  lpInData, lpOutData);
            if (nRet != -1)
            {
              HDC *tmpHdc = SEGPTR_NEW(HDC);

#define SETPRINTERDC SETABORTPROC

              *tmpHdc = dc->hSelf;
              PRTDRV_Control(physDev->segptrPDEVICE, SETPRINTERDC,
                             SEGPTR_GET(tmpHdc), (SEGPTR)NULL);
              SEGPTR_FREE(tmpHdc);
            }
            break;
	  default:
	    nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
				  lpInData, lpOutData);
            break;
	}
    }
    else
	WARN("Escape(nEscape = %04x) - ???\n", nEscape);      
    return nRet;
}


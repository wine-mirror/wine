/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "winreg.h"
#include "win16drv.h"
#include "gdi.h"
#include "bitmap.h"
#include "heap.h"
#include "font.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win16drv);

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
static INT WIN16DRV_GetDeviceCaps( DC *dc, INT cap );
static INT WIN16DRV_ExtEscape( DC *dc, INT escape, INT in_count, LPCVOID in_data,
                               INT out_count, LPVOID out_data );

static const DC_FUNCTIONS WIN16DRV_Funcs =
{
    NULL,                            /* pAbortDoc */
    NULL,                            /* pAbortPath */
    NULL,                            /* pAngleArc */
    NULL,                            /* pArc */
    NULL,                            /* pArcTo */
    NULL,                            /* pBeginPath */
    NULL,                            /* pBitBlt */
    NULL,                            /* pBitmapBits */
    NULL,                            /* pChoosePixelFormat */
    NULL,                            /* pChord */
    NULL,                            /* pCloseFigure */
    NULL,                            /* pCreateBitmap */
    WIN16DRV_CreateDC,               /* pCreateDC */
    NULL,                            /* pCreateDIBSection */
    NULL,                            /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    NULL,                            /* pDescribePixelFormat */
    WIN16DRV_DeviceCapabilities,     /* pDeviceCapabilities */
    WIN16DRV_Ellipse,                /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    NULL,                            /* pEndPath */
    WIN16DRV_EnumDeviceFonts,        /* pEnumDeviceFonts */
    NULL,                            /* pExcludeClipRect */
    WIN16DRV_ExtDeviceMode,          /* pExtDeviceMode */
    WIN16DRV_ExtEscape,              /* pExtEscape */
    NULL,                            /* pExtFloodFill */
    WIN16DRV_ExtTextOut,             /* pExtTextOut */
    NULL,                            /* pFillPath */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFlattenPath */
    NULL,                            /* pFrameRgn */
    WIN16DRV_GetCharWidth,           /* pGetCharWidth */
    NULL,                            /* pGetDCOrgEx */
    WIN16DRV_GetDeviceCaps,          /* pGetDeviceCaps */
    NULL,                            /* pGetDeviceGammaRamp */
    NULL,                            /* pGetPixel */
    NULL,                            /* pGetPixelFormat */
    WIN16DRV_GetTextExtentPoint,     /* pGetTextExtentPoint */
    WIN16DRV_GetTextMetrics,         /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pInvertRgn */
    WIN16DRV_LineTo,                 /* pLineTo */
    NULL,                            /* pMoveTo */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrgEx */
    NULL,                            /* pOffsetWindowOrgEx */
    NULL,                            /* pPaintRgn */
    WIN16DRV_PatBlt,                 /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyBezier */
    NULL,                            /* pPolyBezierTo */
    NULL,                            /* pPolyDraw */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolyPolyline */
    WIN16DRV_Polygon,                /* pPolygon */
    WIN16DRV_Polyline,               /* pPolyline */
    NULL,                            /* pPolylineTo */
    NULL,                            /* pRealizePalette */
    WIN16DRV_Rectangle,              /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExtEx */
    NULL,                            /* pScaleWindowExtEx */
    NULL,                            /* pSelectClipPath */
    NULL,                            /* pSelectClipRgn */
    WIN16DRV_SelectObject,           /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDeviceGammaRamp */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode */
    NULL,                            /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPixelFormat */
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
    NULL,                            /* pStretchDIBits */
    NULL,                            /* pStrokeAndFillPath */
    NULL,                            /* pStrokePath */
    NULL,                            /* pSwapBuffers */
    NULL                             /* pWidenPath */
};


/* FIXME: this no longer works */
#if 0
/**********************************************************************
 *	     WIN16DRV_Init
 */
BOOL WIN16DRV_Init(void)
{
    return DRIVER_RegisterDriver( NULL /* generic driver */, &WIN16DRV_Funcs );
}
#endif

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
    lpDrawMode->ICMCXform       = 0; /* ? */
    lpDrawMode->StretchBltMode  = STRETCH_ANDSCANS;
    lpDrawMode->eMiterLimit     = 1;
}

BOOL WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA* initData )
{
    LOADED_PRINTER_DRIVER *pLPD;
    WORD wRet;
    int nPDEVICEsize;
    PDEVICE_HEADER *pPDH;
    WIN16DRV_PDEVICE *physDev;
    char printerEnabled[20];
    HKEY hkey;

    /* default value */
    strcpy(printerEnabled, "off");
    if(!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\wine", &hkey))
    {
	DWORD type, count = sizeof(printerEnabled);
	RegQueryValueExA(hkey, "printer", 0, &type, printerEnabled, &count);
	RegCloseKey(hkey);
    }

    if (strcasecmp(printerEnabled,"on"))
    {
        MESSAGE("Printing disabled in wine.conf or .winerc file\n");
        MESSAGE("Use \"printer=on\" in the \"[wine]\" section to enable it.\n");
        return FALSE;
    }

    TRACE("In creatdc for (%s,%s,%s) initData 0x%p\n",
	  driver, device, output, initData);

    physDev = (WIN16DRV_PDEVICE *)HeapAlloc( GetProcessHeap(), 0, sizeof(*physDev) );
    if (!physDev) return FALSE;
    dc->physDev = physDev;

    pLPD = LoadPrinterDriver(driver);
    if (pLPD == NULL)
    {
	WARN("Failed to find printer driver\n");
        HeapFree( GetProcessHeap(), 0, physDev );
        return FALSE;
    }
    TRACE("windevCreateDC pLPD 0x%p\n", pLPD);

    /* Now Get the device capabilities from the printer driver */
    memset( &physDev->DevCaps, 0, sizeof(physDev->DevCaps) );
    if(!output) output = "LPT1:";

    /* Get GDIINFO which is the same as a DeviceCaps structure */
    wRet = PRTDRV_Enable(&physDev->DevCaps, GETGDIINFO, device, driver, output,NULL); 

    /* Add this to the DC */
    dc->hVisRgn = CreateRectRgn(0, 0, physDev->DevCaps.horzRes, physDev->DevCaps.vertRes);
    dc->bitsPerPixel = physDev->DevCaps.bitsPixel;
    
    TRACE("Got devcaps width %d height %d bits %d planes %d\n",
	  physDev->DevCaps.horzRes, physDev->DevCaps.vertRes, 
	  physDev->DevCaps.bitsPixel, physDev->DevCaps.planes);

    /* Now we allocate enough memory for the PDEVICE structure */
    /* The size of this varies between printer drivers */
    /* This PDEVICE is used by the printer DRIVER not by the GDI so must */
    /* be accessable from 16 bit code */
    nPDEVICEsize = physDev->DevCaps.pdeviceSize + sizeof(PDEVICE_HEADER);

    /* TTD Shouldn't really do pointer arithmetic on segment points */
    physDev->segptrPDEVICE = K32WOWGlobalLock16(GlobalAlloc16(GHND, nPDEVICEsize))+sizeof(PDEVICE_HEADER);
    *((BYTE *)MapSL(physDev->segptrPDEVICE)+0) = 'N'; 
    *((BYTE *)MapSL(physDev->segptrPDEVICE)+1) = 'B'; 

    /* Set up the header */
    pPDH = (PDEVICE_HEADER *)((BYTE*)MapSL(physDev->segptrPDEVICE) - sizeof(PDEVICE_HEADER)); 
    pPDH->pLPD = pLPD;
    
    TRACE("PDEVICE allocated %08lx\n",(DWORD)(physDev->segptrPDEVICE));
    
    /* Now get the printer driver to initialise this data */
    wRet = PRTDRV_Enable((LPVOID)physDev->segptrPDEVICE, INITPDEVICE, device, driver, output, NULL); 

    physDev->FontInfo = NULL;
    physDev->BrushInfo = NULL;
    physDev->PenInfo = NULL;
    win16drv_SegPtr_TextXForm = K32WOWGlobalLock16(GlobalAlloc16(GHND, sizeof(TEXTXFORM16)));
    win16drv_TextXFormP = MapSL(win16drv_SegPtr_TextXForm);
    
    InitTextXForm(win16drv_TextXFormP);

    /* TTD Lots more to do here */
    win16drv_SegPtr_DrawMode = K32WOWGlobalLock16(GlobalAlloc16(GHND, sizeof(DRAWMODE)));
    win16drv_DrawModeP = MapSL(win16drv_SegPtr_DrawMode);
    
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


/***********************************************************************
 *           WIN16DRV_GetDeviceCaps
 */
static INT WIN16DRV_GetDeviceCaps( DC *dc, INT cap )
{
    WIN16DRV_PDEVICE *physDev = dc->physDev;
    if (cap >= PHYSICALWIDTH || (cap % 2))
    {
        FIXME("(%04x): unsupported capability %d, will return 0\n", dc->hSelf, cap );
        return 0;
    }
    return *((WORD *)&physDev->DevCaps + (cap / 2));
}


/***********************************************************************
 *           WIN16DRV_ExtEscape
 */
static INT WIN16DRV_ExtEscape( DC *dc, INT escape, INT in_count, LPCVOID in_data,
                               INT out_count, LPVOID out_data )
{
#if 0
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

       /* Call Control with hdc as lpInData */
	    HDC16 *seghdc = SEGPTR_NEW(HDC16);
	    *seghdc = dc->hSelf;
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
	    {
	      /* lpInData is not necessarily \0 terminated so make it so */
	      char *cp = SEGPTR_ALLOC(cbInput + 1);
	      memcpy(cp, MapSL(lpInData), cbInput);
	      cp[cbInput] = '\0';
	    
	      nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
				    SEGPTR_GET(cp), lpOutData);
	      SEGPTR_FREE(cp);
	      if (nRet != -1)
		{
		  HDC *tmpHdc = SEGPTR_NEW(HDC);

#define SETPRINTERDC SETABORTPROC

		  *tmpHdc = dc->hSelf;
		  PRTDRV_Control(physDev->segptrPDEVICE, SETPRINTERDC,
				 SEGPTR_GET(tmpHdc), (SEGPTR)NULL);
		  SEGPTR_FREE(tmpHdc);
		}
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
#endif
    /* FIXME: should convert args to SEGPTR and redo all the above */
    FIXME("temporarily broken, please fix\n");
    return 0;
}



/*
 * Exported functions from the Postscript driver.
 *
 * [Ext]DeviceMode, DeviceCapabilities. 
 *
 * Will need ExtTextOut for winword6 (urgh!)
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "windows.h"
#include "psdrv.h"
#include "debug.h"


static DEVMODE16 DefaultDevMode = 
{
/* dmDeviceName */	"Wine Postscript Driver",
/* dmSpecVersion */	0x30a,
/* dmDriverVersion */	0x001,
/* dmSize */		sizeof(DEVMODE16),
/* dmDriverExtra */	0,
/* dmFields */		DM_ORIENTATION | DM_PAPERSIZE | DM_PAPERLENGTH | 
			  DM_PAPERWIDTH,
/* dmOrientation */	DMORIENT_PORTRAIT,
/* dmPaperSize */	DMPAPER_A4,
/* dmPaperLength */	2930,
/* dmPaperWidth */      2000,
/* dmScale */		0,
/* dmCopies */		0,
/* dmDefaultSource */	0,
/* dmPrintQuality */	0,
/* dmColor */		0,
/* dmDuplex */		0,
/* dmYResolution */	0,
/* dmTTOption */	0,
/* dmCollate */		0,
/* dmFormName */	"",
/* dmUnusedPadding */   0,
/* dmBitsPerPel */	0,
/* dmPelsWidth */	0,
/* dmPelsHeight */	0,
/* dmDisplayFlags */	0,
/* dmDisplayFrequency */ 0
};


static char PaperNames[][64] = {"My A4"};
static WORD Papers[] = {DMPAPER_A4};
static POINT16 PaperSizes[] = {{2110, 2975}};
static char BinNames[][24] = {"My Bin"};
static WORD Bins[] = {DMBIN_AUTO};
static LONG Resolutions[][2] = { {600,600} };


/***************************************************************
 *
 *	PSDRV_ExtDeviceMode16	[WINEPS.90]
 *
 * Just returns default devmode at the moment
 */
INT16 WINAPI PSDRV_ExtDeviceMode16(HWND16 hwnd, HANDLE16 hDriver,
LPDEVMODE16 lpdmOutput, LPSTR lpszDevice, LPSTR lpszPort,
LPDEVMODE16 lpdmInput, LPSTR lpszProfile, WORD fwMode)
{

  TRACE(psdrv,
"(hwnd=%04x, hDriver=%04x, devOut=%p, Device='%s', Port='%s', devIn=%p, Profile='%s', Mode=%04x)\n",
hwnd, hDriver, lpdmOutput, lpszDevice, lpszPort, lpdmInput, lpszProfile,
fwMode);

  if(!fwMode)
    return sizeof(DefaultDevMode);

  if(fwMode & DM_COPY)
    memcpy(lpdmOutput, &DefaultDevMode, sizeof(DefaultDevMode));
  
  return IDOK;
}

/***************************************************************
 *
 *	PSDRV_DeviceCapabilities16	[WINEPS.91]
 *
 */
DWORD WINAPI PSDRV_DeviceCapabilities16(LPSTR lpszDevice, LPSTR lpszPort,
  WORD fwCapability, LPSTR lpszOutput, LPDEVMODE16 lpdm)
{
  TRACE(psdrv, "Cap=%d\n", fwCapability);

  switch(fwCapability) {

  case DC_PAPERS:
    if(lpszOutput != NULL)
      memcpy(lpszOutput, Papers, sizeof(Papers));
    return sizeof(Papers) / sizeof(WORD);

  case DC_PAPERSIZE:
    if(lpszOutput != NULL)
      memcpy(lpszOutput, PaperSizes, sizeof(PaperSizes));
    return sizeof(PaperSizes) / sizeof(POINT16);

  case DC_BINS:
    if(lpszOutput != NULL)
      memcpy(lpszOutput, Bins, sizeof(Bins));
    return sizeof(Bins) / sizeof(WORD);
    
  case DC_BINNAMES:
    if(lpszOutput != NULL)
      memcpy(lpszOutput, BinNames, sizeof(BinNames));
    return sizeof(BinNames) / sizeof(BinNames[0]);

  case DC_PAPERNAMES:
    if(lpszOutput != NULL)
      memcpy(lpszOutput, PaperNames, sizeof(PaperNames));
    return sizeof(PaperNames) / sizeof(PaperNames[0]);

  case DC_ORIENTATION:
    return 90;

  case DC_ENUMRESOLUTIONS:
    if(lpszOutput != NULL)
      memcpy(lpszOutput, Resolutions, sizeof(Resolutions));
    return sizeof(Resolutions) / sizeof(Resolutions[0]);


  default:
    FIXME(psdrv, "Unsupported capability %d\n", fwCapability);
  }
  return -1;
}

/***************************************************************
 *
 *	PSDRV_DeviceMode16	[WINEPS.13]
 *
 */
void WINAPI PSDRV_DeviceMode16(HWND16 hwnd, HANDLE16 hDriver,
LPSTR lpszDevice, LPSTR lpszPort)
{
    PSDRV_ExtDeviceMode16( hwnd, hDriver, NULL, lpszDevice, lpszPort, NULL, 
			   NULL, DM_PROMPT );
    return;
}

/*
 * Exported functions from the PostScript driver.
 *
 * [Ext]DeviceMode, DeviceCapabilities, AdvancedSetupDialog. 
 *
 * Will need ExtTextOut for winword6 (urgh!)
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include <windows.h>
#include <psdrv.h>
#include <debug.h>
#include <resource.h>
#include <string.h>
#include <win.h>
#include <print.h>

static LONG Resolutions[][2] = { {600,600} };


/************************************************************************
 *
 *		PSDRV_MergeDevmodes
 *
 * Updates dm1 with some fields from dm2
 *
 */
void PSDRV_MergeDevmodes(PSDRV_DEVMODE16 *dm1, PSDRV_DEVMODE16 *dm2,
			 PRINTERINFO *pi)
{
    /* some sanity checks here on dm2 */
    if(dm2->dmPublic.dmFields & DM_ORIENTATION)
        dm1->dmPublic.dmOrientation = dm2->dmPublic.dmOrientation;
	    /* NB PaperWidth is always < PaperLength */
        
    if(dm2->dmPublic.dmFields & DM_PAPERSIZE) {
        PAGESIZE *page;

	for(page = pi->ppd->PageSizes; page; page = page->next) {
	    if(page->WinPage == dm2->dmPublic.dmPaperSize)
	        break;
	}
	if(page) {
	    dm1->dmPublic.dmPaperSize = dm2->dmPublic.dmPaperSize;
	    dm1->dmPublic.dmPaperWidth = page->PaperDimension->x * 25.4 / 72.0;
	    dm1->dmPublic.dmPaperLength = page->PaperDimension->y * 25.4 / 72.0;
	    TRACE(psdrv, "Changing page to %s %d x %d\n", page->FullName,
		     dm1->dmPublic.dmPaperWidth, dm1->dmPublic.dmPaperLength );
	} else {
	    TRACE(psdrv, "Trying to change to unsupported pagesize %d\n",
		      dm2->dmPublic.dmPaperSize);
	}
    }

    if(dm2->dmPublic.dmFields & DM_DEFAULTSOURCE) {
        INPUTSLOT *slot;
	
	for(slot = pi->ppd->InputSlots; slot; slot = slot->next) {
	    if(slot->WinBin == dm2->dmPublic.dmDefaultSource)
	        break;
	}
	if(slot) {
	    dm1->dmPublic.dmDefaultSource = dm2->dmPublic.dmDefaultSource;
	    TRACE(psdrv, "Changing bin to '%s'\n", slot->FullName);
	} else {
	  TRACE(psdrv, "Trying to change to unsupported bin %d\n",
		dm2->dmPublic.dmDefaultSource);
	}
    }

    /* etc */

    return;
}


#if 0
/*******************************************************************
 *
 *		PSDRV_NewPrinterDlgProc32
 *
 *
 */
LRESULT WINAPI PSDRV_NewPrinterDlgProc32(HWND32 hWnd, UINT32 wMsg,
					    WPARAM32 wParam, LPARAM lParam)
{
  switch (wMsg) {
  case WM_INITDIALOG:
    TRACE(psdrv,"WM_INITDIALOG lParam=%08lX\n", lParam);
    ShowWindow32(hWnd, SW_SHOWNORMAL);
    return TRUE;
 
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      EndDialog32(hWnd, TRUE);
      return TRUE;
  
    case IDCANCEL:
      EndDialog32(hWnd, FALSE);
      return TRUE;
    
    default:
      return FALSE;
    }
  
  default:
    return FALSE;
  }
}

LRESULT WINAPI PSDRV_AdvancedSetupDlgProc32(HWND32 hWnd, UINT32 wMsg,
					    WPARAM32 wParam, LPARAM lParam)
{
  switch (wMsg) {
  case WM_INITDIALOG:
    TRACE(psdrv,"WM_INITDIALOG lParam=%08lX\n", lParam);
    SendDlgItemMessage32A(hWnd, 99, CB_ADDSTRING32, 0, 
			  (LPARAM)"Default Tray");
    ShowWindow32(hWnd, SW_SHOWNORMAL);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      EndDialog32(hWnd, TRUE);
      return TRUE;

    case IDCANCEL:
      EndDialog32(hWnd, FALSE);
      return TRUE;

    case 200:
      DialogBoxIndirectParam32A( WIN_GetWindowInstance( hWnd ),
			  SYSRES_GetResPtr( SYSRES_DIALOG_PSDRV_NEWPRINTER ),
			  hWnd, PSDRV_NewPrinterDlgProc32, (LPARAM) NULL );
      return TRUE;

    default:
      return FALSE;
    }

  default:    
    return FALSE;
  }
}
#endif /* 0 */

/**************************************************************
 *
 *	PSDRV_AdvancedSetupDialog16	[WINEPS.93]
 *
 * Presumably dev1 and dev2 are in and out, don't know which is which yet.
 * Not sure about return value either. (I haven't got any docs on this at all)
 *
 */
INT16 WINAPI PSDRV_AdvancedSetupDialog16(HWND16 hwnd, HANDLE16 hDriver,
					 LPDEVMODE16 dev1, LPDEVMODE16 dev2 )
{

  TRACE(psdrv, "hwnd = %04x, hDriver = %04x dev1=%p dev2=%p\n", hwnd,
	hDriver, dev1, dev2);
  return IDCANCEL;


#if 0
  return DialogBoxIndirectParam32A( WIN_GetWindowInstance( hwnd ),
	SYSRES_GetResPtr( SYSRES_DIALOG_PSDRV_ADVANCEDSETUP ),
	hwnd, PSDRV_AdvancedSetupDlgProc32, (LPARAM) NULL );
#endif


}

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
  PRINTERINFO *pi = PSDRV_FindPrinterInfo(lpszDevice);

  TRACE(psdrv,
"(hwnd=%04x, hDriver=%04x, devOut=%p, Device='%s', Port='%s', devIn=%p, Profile='%s', Mode=%04x)\n",
hwnd, hDriver, lpdmOutput, lpszDevice, lpszPort, lpdmInput, lpszProfile,
fwMode);

  if(!fwMode)
    return sizeof(DEVMODE16); /* Just copy dmPublic bit of PSDRV_DEVMODE */

  if((fwMode & DM_PROMPT) || (fwMode & DM_UPDATE))
    FIXME(psdrv, "Mode %d not implemented\n", fwMode);

  if(fwMode & DM_MODIFY) {
    TRACE(psdrv, "DM_MODIFY set. devIn->dmFields = %08lx\n", lpdmInput->dmFields);
    PSDRV_MergeDevmodes(pi->Devmode, (PSDRV_DEVMODE16 *)lpdmInput, pi);
  }

  if(fwMode & DM_COPY) {
    memcpy(lpdmOutput, pi->Devmode, sizeof(DEVMODE16));
  }
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
  PRINTERINFO *pi;

  pi = PSDRV_FindPrinterInfo(lpszDevice);
  TRACE(psdrv, "Cap=%d. Got PrinterInfo = %p\n", fwCapability, pi);

  switch(fwCapability) {

  case DC_PAPERS:
    {
      PAGESIZE *ps;
      WORD *wp = (WORD *)lpszOutput;
      int i = 0;

      for(ps = pi->ppd->PageSizes; ps; ps = ps->next, i++)
	if(lpszOutput != NULL)
	  *wp++ = ps->WinPage;
      return i;
    }

  case DC_PAPERSIZE:
    {
      PAGESIZE *ps;
      POINT16 *pt = (POINT16 *)lpszOutput;
      int i = 0;

      for(ps = pi->ppd->PageSizes; ps; ps = ps->next, i++)
	if(lpszOutput != NULL) {
	  pt->x = ps->PaperDimension->x * 254.0 / 72.0;
	  pt->y = ps->PaperDimension->y * 254.0 / 72.0;
	  pt++;
	}
      return i;
    }

  case DC_PAPERNAMES:
    {
      PAGESIZE *ps;
      char *cp = lpszOutput;
      int i = 0;

      for(ps = pi->ppd->PageSizes; ps; ps = ps->next, i++)
	if(lpszOutput != NULL) {
	  strncpy(cp, ps->FullName, 64);
	  *(cp + 63) = '\0';
	  cp += 64;
	}
      return i;
    }

  case DC_ORIENTATION:
    return pi->ppd->LandscapeOrientation ? pi->ppd->LandscapeOrientation : 90;

  case DC_BINS:
    {
      INPUTSLOT *slot;
      WORD *wp = (WORD *)lpszOutput;
      int i = 0;

      for(slot = pi->ppd->InputSlots; slot; slot = slot->next, i++)
	if(lpszOutput != NULL)
	  *wp++ = slot->WinBin;
      return i;
    }

  case DC_BINNAMES:
    {
      INPUTSLOT *slot;
      char *cp = lpszOutput;
      int i = 0;
      
      for(slot = pi->ppd->InputSlots; slot; slot = slot->next, i++)
	if(lpszOutput != NULL) {
	  strncpy(cp, slot->FullName, 24);
	  *(cp + 23) = '\0';
	  cp += 24;
	}
      return i;
    }

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

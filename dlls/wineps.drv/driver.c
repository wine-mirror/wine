/*
 * Exported functions from the PostScript driver.
 *
 * [Ext]DeviceMode, DeviceCapabilities, AdvancedSetupDialog.
 *
 * Will need ExtTextOut for winword6 (urgh!)
 *
 * Copyright 1998  Huw D M Davies
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"

#include <string.h>

#include "wine/debug.h"
#include "psdlg.h"
#include "psdrv.h"

#include "winuser.h"
#include "wownt32.h"
#include "prsht.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/************************************************************************
 *
 *		PSDRV_MergeDevmodes
 *
 * Updates dm1 with some fields from dm2
 *
 */
void PSDRV_MergeDevmodes(PSDRV_DEVMODEA *dm1, PSDRV_DEVMODEA *dm2,
			 PRINTERINFO *pi)
{
    /* some sanity checks here on dm2 */

    if(dm2->dmPublic.dmFields & DM_ORIENTATION) {
        dm1->dmPublic.u1.s1.dmOrientation = dm2->dmPublic.u1.s1.dmOrientation;
	TRACE("Changing orientation to %d (%s)\n",
	      dm1->dmPublic.u1.s1.dmOrientation,
	      dm1->dmPublic.u1.s1.dmOrientation == DMORIENT_PORTRAIT ?
	      "Portrait" :
	      (dm1->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE ?
	       "Landscape" : "unknown"));
    }

    /* NB PaperWidth is always < PaperLength */
    if(dm2->dmPublic.dmFields & DM_PAPERSIZE) {
        PAGESIZE *page;

	LIST_FOR_EACH_ENTRY(page, &pi->ppd->PageSizes, PAGESIZE, entry) {
	    if(page->WinPage == dm2->dmPublic.u1.s1.dmPaperSize)
	        break;
	}
	if(&page->entry != &pi->ppd->PageSizes ) {
	    dm1->dmPublic.u1.s1.dmPaperSize = dm2->dmPublic.u1.s1.dmPaperSize;
	    dm1->dmPublic.u1.s1.dmPaperWidth = page->PaperDimension->x *
								254.0 / 72.0;
	    dm1->dmPublic.u1.s1.dmPaperLength = page->PaperDimension->y *
								254.0 / 72.0;
	    dm1->dmPublic.dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH);
	    dm1->dmPublic.dmFields |= DM_PAPERSIZE;
	    TRACE("Changing page to %s %d x %d\n", page->FullName,
		  dm1->dmPublic.u1.s1.dmPaperWidth,
		  dm1->dmPublic.u1.s1.dmPaperLength );
	} else {
	    TRACE("Trying to change to unsupported pagesize %d\n",
		      dm2->dmPublic.u1.s1.dmPaperSize);
	}
    } else if((dm2->dmPublic.dmFields & DM_PAPERLENGTH) &&
       (dm2->dmPublic.dmFields & DM_PAPERWIDTH)) {
        dm1->dmPublic.u1.s1.dmPaperLength = dm2->dmPublic.u1.s1.dmPaperLength;
        dm1->dmPublic.u1.s1.dmPaperWidth = dm2->dmPublic.u1.s1.dmPaperWidth;
	TRACE("Changing PaperLength|Width to %dx%d\n",
	      dm2->dmPublic.u1.s1.dmPaperLength,
	      dm2->dmPublic.u1.s1.dmPaperWidth);
	dm1->dmPublic.dmFields &= ~DM_PAPERSIZE;
	dm1->dmPublic.dmFields |= (DM_PAPERLENGTH | DM_PAPERWIDTH);
    } else if(dm2->dmPublic.dmFields & (DM_PAPERLENGTH | DM_PAPERWIDTH)) {
      /* You might think that this would be allowed if dm1 is in custom size
	 mode, but apparently Windows reverts to standard paper mode even in
	 this case */
        FIXME("Trying to change only paperlength or paperwidth\n");
	dm1->dmPublic.dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH);
	dm1->dmPublic.dmFields |= DM_PAPERSIZE;
    }

    if(dm2->dmPublic.dmFields & DM_SCALE) {
        dm1->dmPublic.dmScale = dm2->dmPublic.dmScale;
	TRACE("Changing Scale to %d\n", dm2->dmPublic.dmScale);
    }

    if(dm2->dmPublic.dmFields & DM_COPIES) {
        dm1->dmPublic.dmCopies = dm2->dmPublic.dmCopies;
	TRACE("Changing Copies to %d\n", dm2->dmPublic.dmCopies);
    }

    if(dm2->dmPublic.dmFields & DM_DEFAULTSOURCE) {
        INPUTSLOT *slot;

	for(slot = pi->ppd->InputSlots; slot; slot = slot->next) {
	    if(slot->WinBin == dm2->dmPublic.dmDefaultSource)
	        break;
	}
	if(slot) {
	    dm1->dmPublic.dmDefaultSource = dm2->dmPublic.dmDefaultSource;
	    TRACE("Changing bin to '%s'\n", slot->FullName);
	} else {
	  TRACE("Trying to change to unsupported bin %d\n",
		dm2->dmPublic.dmDefaultSource);
	}
    }

   if (dm2->dmPublic.dmFields & DM_DEFAULTSOURCE )
       dm1->dmPublic.dmDefaultSource = dm2->dmPublic.dmDefaultSource;
   if (dm2->dmPublic.dmFields & DM_PRINTQUALITY )
       dm1->dmPublic.dmPrintQuality = dm2->dmPublic.dmPrintQuality;
   if (dm2->dmPublic.dmFields & DM_COLOR )
       dm1->dmPublic.dmColor = dm2->dmPublic.dmColor;
   if (dm2->dmPublic.dmFields & DM_DUPLEX && pi->ppd->DefaultDuplex && pi->ppd->DefaultDuplex->WinDuplex != 0)
       dm1->dmPublic.dmDuplex = dm2->dmPublic.dmDuplex;
   if (dm2->dmPublic.dmFields & DM_YRESOLUTION )
       dm1->dmPublic.dmYResolution = dm2->dmPublic.dmYResolution;
   if (dm2->dmPublic.dmFields & DM_TTOPTION )
       dm1->dmPublic.dmTTOption = dm2->dmPublic.dmTTOption;
   if (dm2->dmPublic.dmFields & DM_COLLATE )
       dm1->dmPublic.dmCollate = dm2->dmPublic.dmCollate;
   if (dm2->dmPublic.dmFields & DM_FORMNAME )
       lstrcpynA((LPSTR)dm1->dmPublic.dmFormName, (LPCSTR)dm2->dmPublic.dmFormName, CCHFORMNAME);
   if (dm2->dmPublic.dmFields & DM_BITSPERPEL )
       dm1->dmPublic.dmBitsPerPel = dm2->dmPublic.dmBitsPerPel;
   if (dm2->dmPublic.dmFields & DM_PELSWIDTH )
       dm1->dmPublic.dmPelsWidth = dm2->dmPublic.dmPelsWidth;
   if (dm2->dmPublic.dmFields & DM_PELSHEIGHT )
       dm1->dmPublic.dmPelsHeight = dm2->dmPublic.dmPelsHeight;
   if (dm2->dmPublic.dmFields & DM_DISPLAYFLAGS )
       dm1->dmPublic.dmDisplayFlags = dm2->dmPublic.dmDisplayFlags;
   if (dm2->dmPublic.dmFields & DM_DISPLAYFREQUENCY )
       dm1->dmPublic.dmDisplayFrequency = dm2->dmPublic.dmDisplayFrequency;
   if (dm2->dmPublic.dmFields & DM_POSITION )
       dm1->dmPublic.u1.dmPosition = dm2->dmPublic.u1.dmPosition;
   if (dm2->dmPublic.dmFields & DM_LOGPIXELS )
       dm1->dmPublic.dmLogPixels = dm2->dmPublic.dmLogPixels;
   if (dm2->dmPublic.dmFields & DM_ICMMETHOD )
       dm1->dmPublic.dmICMMethod = dm2->dmPublic.dmICMMethod;
   if (dm2->dmPublic.dmFields & DM_ICMINTENT )
       dm1->dmPublic.dmICMIntent = dm2->dmPublic.dmICMIntent;
   if (dm2->dmPublic.dmFields & DM_MEDIATYPE )
       dm1->dmPublic.dmMediaType = dm2->dmPublic.dmMediaType;
   if (dm2->dmPublic.dmFields & DM_DITHERTYPE )
       dm1->dmPublic.dmDitherType = dm2->dmPublic.dmDitherType;
   if (dm2->dmPublic.dmFields & DM_PANNINGWIDTH )
       dm1->dmPublic.dmPanningWidth = dm2->dmPublic.dmPanningWidth;
   if (dm2->dmPublic.dmFields & DM_PANNINGHEIGHT )
       dm1->dmPublic.dmPanningHeight = dm2->dmPublic.dmPanningHeight;

    return;
}


/**************************************************************
 *	AdvancedSetupDialog	[WINEPS16.93]
 *
 */
WORD WINAPI PSDRV_AdvancedSetupDialog16(HWND16 hwnd, HANDLE16 hDriver,
					 LPDEVMODEA devin, LPDEVMODEA devout)
{

  TRACE("hwnd = %04x, hDriver = %04x devin=%p devout=%p\n", hwnd,
	hDriver, devin, devout);
  return IDCANCEL;
}

/****************************************************************
 *       PSDRV_PaperDlgProc
 *
 * Dialog proc for 'Paper' propsheet
 */
static INT_PTR CALLBACK PSDRV_PaperDlgProc(HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam)
{
  PSDRV_DLGINFO *di;
  int i, Cursel = 0;
  PAGESIZE *ps;
  DUPLEX *duplex;

  switch(msg) {
  case WM_INITDIALOG:
    di = (PSDRV_DLGINFO*)((PROPSHEETPAGEA*)lParam)->lParam;
    SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)di);

    i = 0;
    LIST_FOR_EACH_ENTRY(ps, &di->pi->ppd->PageSizes, PAGESIZE, entry) {
      SendDlgItemMessageA(hwnd, IDD_PAPERS, LB_INSERTSTRING, i,
			  (LPARAM)ps->FullName);
      if(di->pi->Devmode->dmPublic.u1.s1.dmPaperSize == ps->WinPage)
	Cursel = i;
      i++;
    }
    SendDlgItemMessageA(hwnd, IDD_PAPERS, LB_SETCURSEL, Cursel, 0);
    
    CheckRadioButton(hwnd, IDD_ORIENT_PORTRAIT, IDD_ORIENT_LANDSCAPE,
		     di->pi->Devmode->dmPublic.u1.s1.dmOrientation ==
		     DMORIENT_PORTRAIT ? IDD_ORIENT_PORTRAIT :
		     IDD_ORIENT_LANDSCAPE);

    if(!di->pi->ppd->Duplexes) {
        ShowWindow(GetDlgItem(hwnd, IDD_DUPLEX), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDD_DUPLEX_NAME), SW_HIDE);        
    } else {
        Cursel = 0;
        for(duplex = di->pi->ppd->Duplexes, i = 0; duplex; duplex = duplex->next, i++) {
            SendDlgItemMessageA(hwnd, IDD_DUPLEX, CB_INSERTSTRING, i,
                                (LPARAM)(duplex->FullName ? duplex->FullName : duplex->Name));
            if(di->pi->Devmode->dmPublic.dmDuplex == duplex->WinDuplex)
                Cursel = i;
        }
        SendDlgItemMessageA(hwnd, IDD_DUPLEX, CB_SETCURSEL, Cursel, 0);
    }
    break;

  case WM_COMMAND:
    di = (PSDRV_DLGINFO *)GetWindowLongPtrW(hwnd, DWLP_USER);
    switch(LOWORD(wParam)) {
    case IDD_PAPERS:
      if(HIWORD(wParam) == LBN_SELCHANGE) {
	Cursel = SendDlgItemMessageA(hwnd, LOWORD(wParam), LB_GETCURSEL, 0, 0);
        i = 0;
	LIST_FOR_EACH_ENTRY(ps, &di->pi->ppd->PageSizes, PAGESIZE, entry) {
            if(i >= Cursel) break;
            i++;
        }
	TRACE("Setting pagesize to item %d Winpage = %d\n", Cursel,
	      ps->WinPage);
	di->dlgdm->dmPublic.u1.s1.dmPaperSize = ps->WinPage;
      }
      break;
    case IDD_ORIENT_PORTRAIT:
    case IDD_ORIENT_LANDSCAPE:
      TRACE("Setting orientation to %s\n", wParam == IDD_ORIENT_PORTRAIT ?
	    "portrait" : "landscape");
      di->dlgdm->dmPublic.u1.s1.dmOrientation = wParam == IDD_ORIENT_PORTRAIT ?
	DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE;
      break;
    case IDD_DUPLEX:
      if(HIWORD(wParam) == CBN_SELCHANGE) {
	Cursel = SendDlgItemMessageA(hwnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
	for(i = 0, duplex = di->pi->ppd->Duplexes; i < Cursel; i++, duplex = duplex->next)
	  ;
	TRACE("Setting duplex to item %d Winduplex = %d\n", Cursel,
	      duplex->WinDuplex);
	di->dlgdm->dmPublic.dmDuplex = duplex->WinDuplex;
      }
      break;
    }
    break;

  case WM_NOTIFY:
   {
    NMHDR *nmhdr = (NMHDR *)lParam;
    di = (PSDRV_DLGINFO *)GetWindowLongPtrW(hwnd, DWLP_USER);
    switch(nmhdr->code) {
    case PSN_APPLY:
      memcpy(di->pi->Devmode, di->dlgdm, sizeof(PSDRV_DEVMODEA));
      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
      return TRUE;

    default:
      return FALSE;
    }
    break;
   }

  default:
    return FALSE;
  }
  return TRUE;
}


static void (WINAPI *pInitCommonControls) (void);
static HPROPSHEETPAGE (WINAPI *pCreatePropertySheetPage) (LPCPROPSHEETPAGEW);
static int (WINAPI *pPropertySheet) (LPCPROPSHEETHEADERW);


 /******************************************************************
 *         PSDRV_ExtDeviceMode
 *
 *  Retrieves or modifies device-initialization information for the PostScript
 *  driver, or displays a driver-supplied dialog box for configuring the driver.
 *
 * PARAMETERS
 *  lpszDriver  -- Driver name
 *  hwnd        -- Parent window for the dialog box
 *  lpdmOutput  -- Address of a DEVMODE structure for writing initialization information
 *  lpszDevice  -- Device name
 *  lpszPort    -- Port name
 *  lpdmInput   -- Address of a DEVMODE structure for reading initialization information
 *  lpProfile   -- Name of initialization file, defaults to WIN.INI if NULL
 *  wMode      -- Operation to perform.  Can be a combination if > 0.
 *      (0)             -- Returns number of bytes required by DEVMODE structure
 *      DM_UPDATE (1)   -- Write current settings to environment and initialization file
 *      DM_COPY (2)     -- Write current settings to lpdmOutput
 *      DM_PROMPT (4)   -- Presents the driver's modal dialog box (USER.240)
 *      DM_MODIFY (8)   -- Changes current settings according to lpdmInput before any other operation
 *
 * RETURNS
 *  Returns size of DEVMODE structure if wMode is 0.  Otherwise, IDOK is returned for success
 *  for both dialog and non-dialog operations.  IDCANCEL is returned if the dialog box was cancelled.
 *  A return value less than zero is returned if a non-dialog operation fails.
 *  
 * BUGS
 *
 * Just returns default devmode at the moment.  No use of initialization file.
 */
INT PSDRV_ExtDeviceMode(LPSTR lpszDriver, HWND hwnd, LPDEVMODEA lpdmOutput,
			LPSTR lpszDevice, LPSTR lpszPort, LPDEVMODEA lpdmInput,
			LPSTR lpszProfile, DWORD dwMode)
{
  PRINTERINFO *pi = PSDRV_FindPrinterInfo(lpszDevice);
  if(!pi) return -1;

  TRACE("(Driver=%s, hwnd=%p, devOut=%p, Device='%s', Port='%s', devIn=%p, Profile='%s', Mode=%04x)\n",
  lpszDriver, hwnd, lpdmOutput, lpszDevice, lpszPort, lpdmInput, lpszProfile, dwMode);

  /* If dwMode == 0, return size of DEVMODE structure */
  if(!dwMode)
    return pi->Devmode->dmPublic.dmSize + pi->Devmode->dmPublic.dmDriverExtra;

  /* If DM_MODIFY is set, change settings in accordance with lpdmInput */
  if((dwMode & DM_MODIFY) && lpdmInput) {
    TRACE("DM_MODIFY set. devIn->dmFields = %08x\n", lpdmInput->dmFields);
    PSDRV_MergeDevmodes(pi->Devmode, (PSDRV_DEVMODEA *)lpdmInput, pi);
  }

  /* If DM_PROMPT is set, present modal dialog box */
  if(dwMode & DM_PROMPT) {
    HINSTANCE hinstComctl32;
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEW psp;
    PROPSHEETHEADERW psh;
    PSDRV_DLGINFO *di;
    PSDRV_DEVMODEA *dlgdm;
    static const WCHAR PAPERW[] = {'P','A','P','E','R','\0'};
    static const WCHAR SetupW[] = {'S','e','t','u','p','\0'};

    hinstComctl32 = LoadLibraryA("comctl32.dll");
    pInitCommonControls = (void*)GetProcAddress(hinstComctl32,
						"InitCommonControls");
    pCreatePropertySheetPage = (void*)GetProcAddress(hinstComctl32,
						    "CreatePropertySheetPageW");
    pPropertySheet = (void*)GetProcAddress(hinstComctl32, "PropertySheetW");
    memset(&psp,0,sizeof(psp));
    dlgdm = HeapAlloc( PSDRV_Heap, 0, sizeof(*dlgdm) );
    memcpy(dlgdm, pi->Devmode, sizeof(*dlgdm));
    di = HeapAlloc( PSDRV_Heap, 0, sizeof(*di) );
    di->pi = pi;
    di->dlgdm = dlgdm;
    psp.dwSize = sizeof(psp);
    psp.hInstance = PSDRV_hInstance;
    psp.u.pszTemplate = PAPERW;
    psp.u2.pszIcon = NULL;
    psp.pfnDlgProc = PSDRV_PaperDlgProc;
    psp.lParam = (LPARAM)di;
    hpsp[0] = pCreatePropertySheetPage(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.pszCaption = SetupW;
    psh.nPages = 1;
    psh.hwndParent = HWND_32(hwnd);
    psh.u3.phpage = hpsp;

    pPropertySheet(&psh);

  }
  
  /* If DM_UPDATE is set, should write settings to environment and initialization file */
  if(dwMode & DM_UPDATE)
    FIXME("Mode DM_UPDATE.  Just do the same as DM_COPY\n");

  /* If DM_COPY is set, should write settings to lpdmOutput */
  if((dwMode & DM_COPY) || (dwMode & DM_UPDATE)) {
    if (lpdmOutput)
        memcpy(lpdmOutput, pi->Devmode, pi->Devmode->dmPublic.dmSize + pi->Devmode->dmPublic.dmDriverExtra );
    else
        FIXME("lpdmOutput is NULL what should we do??\n");
  }
  return IDOK;
}
/***************************************************************
 *	ExtDeviceMode	[WINEPS16.90]
 *
 */

INT16 WINAPI PSDRV_ExtDeviceMode16(HWND16 hwnd, HANDLE16 hDriver,
				   LPDEVMODEA lpdmOutput, LPSTR lpszDevice,
				   LPSTR lpszPort, LPDEVMODEA lpdmInput,
				   LPSTR lpszProfile, WORD fwMode)

{
    return PSDRV_ExtDeviceMode(NULL, HWND_32(hwnd), lpdmOutput, lpszDevice,
				 lpszPort, lpdmInput, lpszProfile, (DWORD) fwMode);
}

/***********************************************************************
 *	PSDRV_DeviceCapabilities	
 *
 *      Retrieves the capabilities of a printer device driver.
 *
 * Parameters
 *      lpszDriver -- printer driver name
 *      lpszDevice -- printer name
 *      lpszPort -- port name
 *      fwCapability -- device capability
 *      lpszOutput -- output buffer
 *      lpDevMode -- device data buffer
 *
 * Returns
 *      Result depends on the setting of fwCapability.  -1 indicates failure.
 */
DWORD PSDRV_DeviceCapabilities(LPSTR lpszDriver, LPCSTR lpszDevice, LPCSTR lpszPort,
					WORD fwCapability, LPSTR lpszOutput,
					LPDEVMODEA lpDevMode)
{
  PRINTERINFO *pi;
  DEVMODEA *lpdm;
  DWORD ret;
  pi = PSDRV_FindPrinterInfo(lpszDevice);

  TRACE("%s %s %s, %u, %p, %p\n", debugstr_a(lpszDriver), debugstr_a(lpszDevice),
        debugstr_a(lpszPort), fwCapability, lpszOutput, lpDevMode);

  if (!pi) {
      ERR("no printer info for %s %s, return 0!\n",
          debugstr_a(lpszDriver), debugstr_a(lpszDevice));
      return 0;
  }

  lpdm = lpDevMode ? lpDevMode : (DEVMODEA *)pi->Devmode;

  switch(fwCapability) {

  case DC_PAPERS:
    {
      PAGESIZE *ps;
      WORD *wp = (WORD *)lpszOutput;
      int i = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
        TRACE("DC_PAPERS: %u\n", ps->WinPage);
        i++;
	if(lpszOutput != NULL)
	  *wp++ = ps->WinPage;
      }
      return i;
    }

  case DC_PAPERSIZE:
    {
      PAGESIZE *ps;
      POINT16 *pt = (POINT16 *)lpszOutput;
      int i = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
        TRACE("DC_PAPERSIZE: %f x %f\n", ps->PaperDimension->x, ps->PaperDimension->y);
        i++;
	if(lpszOutput != NULL) {
	  pt->x = ps->PaperDimension->x * 254.0 / 72.0;
	  pt->y = ps->PaperDimension->y * 254.0 / 72.0;
	  pt++;
	}
      }
      return i;
    }

  case DC_PAPERNAMES:
    {
      PAGESIZE *ps;
      char *cp = lpszOutput;
      int i = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
        TRACE("DC_PAPERNAMES: %s\n", debugstr_a(ps->FullName));
        i++;
	if(lpszOutput != NULL) {
	  lstrcpynA(cp, ps->FullName, 64);
	  cp += 64;
	}
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
	  lstrcpynA(cp, slot->FullName, 24);
	  cp += 24;
	}
      return i;
    }

  case DC_BINADJUST:
    FIXME("DC_BINADJUST: stub.\n");
    return DCBA_FACEUPNONE;

  case DC_ENUMRESOLUTIONS:
    {
      LONG *lp = (LONG*)lpszOutput;

      if(lpszOutput != NULL) {
	lp[0] = (LONG)pi->ppd->DefaultResolution;
	lp[1] = (LONG)pi->ppd->DefaultResolution;
      }
      return 1;
    }

  /* Windows returns 9999 too */
  case DC_COPIES:
    TRACE("DC_COPIES: returning 9999\n");
    return 9999;

  case DC_DRIVER:
    return lpdm->dmDriverVersion;

  case DC_DATATYPE_PRODUCED:
    FIXME("DATA_TYPE_PRODUCED: stub.\n");
    return -1; /* simulate that the driver supports 'RAW' */

  case DC_DUPLEX:
    ret = 0;
    if(pi->ppd->DefaultDuplex && pi->ppd->DefaultDuplex->WinDuplex != 0)
      ret = 1;
    TRACE("DC_DUPLEX: returning %d\n", ret);
    return ret;

  case DC_EMF_COMPLIANT:
    FIXME("DC_EMF_COMPLIANT: stub.\n");
    return -1; /* simulate that the driver do not support EMF */

  case DC_EXTRA:
    return lpdm->dmDriverExtra;

  case DC_FIELDS:
    return lpdm->dmFields;

  case DC_FILEDEPENDENCIES:
    FIXME("DC_FILEDEPENDENCIES: stub.\n");
    return 0;

  case DC_MAXEXTENT:
    {
      PAGESIZE *ps;
      POINT ptMax;
      ptMax.x = ptMax.y = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
	if(ps->PaperDimension->x > ptMax.x)
	  ptMax.x = ps->PaperDimension->x;
	if(ps->PaperDimension->y > ptMax.y)
	  ptMax.y = ps->PaperDimension->y;
      }
      return MAKELONG(ptMax.x * 254.0 / 72.0, ptMax.y * 254.0 / 72.0 );
    }

  case DC_MINEXTENT:
    {
      PAGESIZE *ps;
      POINT ptMin;
      ptMin.x = ptMin.y = -1;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
	if(ptMin.x == -1 || ps->PaperDimension->x < ptMin.x)
	  ptMin.x = ps->PaperDimension->x;
	if(ptMin.y == -1 || ps->PaperDimension->y < ptMin.y)
	  ptMin.y = ps->PaperDimension->y;
      }
      return MAKELONG(ptMin.x * 254.0 / 72.0, ptMin.y * 254.0 / 72.0);
    }

  case DC_SIZE:
    return lpdm->dmSize;

  case DC_TRUETYPE:
    FIXME("DC_TRUETYPE: stub\n");
    return DCTT_SUBDEV;

  case DC_VERSION:
    return lpdm->dmSpecVersion;

  /* We'll just return false here, very few printers can collate anyway */
  case DC_COLLATE:
    TRACE("DC_COLLATE: returning FALSE\n");
    return FALSE;

  /* Printer supports colour printing - 1 if yes, 0 if no (Win2k/XP only) */
  case DC_COLORDEVICE:
    return (pi->ppd->ColorDevice != CD_False) ? TRUE : FALSE;

  /* Identification number of the printer manufacturer for use with ICM (Win9x only) */
  case DC_MANUFACTURER:
    FIXME("DC_MANUFACTURER: stub\n");
    return -1;
    
  /* Identification number of the printer model for use with ICM (Win9x only) */
  case DC_MODEL:
    FIXME("DC_MODEL: stub\n");
    return -1;
    
  /* Nonzero if the printer supports stapling, zero otherwise (Win2k/XP only) */
  case DC_STAPLE: /* WINVER >= 0x0500 */
    FIXME("DC_STAPLE: stub\n");
    return -1;
    
  /* Returns an array of 64-character string buffers containing the names of the paper forms
   * available for use, unless pOutput is NULL.  The return value is the number of paper forms.
   * (Win2k/XP only)
   */
  case DC_MEDIAREADY: /* WINVER >= 0x0500 */
    FIXME("DC_MEDIAREADY: stub\n");
    return -1;

  /* Returns an array of 64-character string buffers containing the names of the supported
   * media types, unless pOutput is NULL.  The return value is the number of supported.
   * media types (XP only)
   */
  case DC_MEDIATYPENAMES: /* WINVER >= 0x0501 */
    FIXME("DC_MEDIATYPENAMES: stub\n");
    return -1;
    
  /* Returns an array of DWORD values which represent the supported media types, unless
   * pOutput is NULL.  The return value is the number of supported media types. (XP only)
   */
  case DC_MEDIATYPES: /* WINVER >= 0x0501 */
    FIXME("DC_MEDIATYPES: stub\n");
    return -1;

  /* Returns an array of DWORD values, each representing a supported number of document
   * pages per printed page, unless pOutput is NULL.  The return value is the number of
   * array entries. (Win2k/XP only)
   */
  case DC_NUP:
    FIXME("DC_NUP: stub\n");
    return -1;
    
  /* Returns an array of 32-character string buffers containing a list of printer description
   * languages supported by the printer, unless pOutput is NULL.  The return value is
   * number of array entries. (Win2k/XP only)
   */
   
  case DC_PERSONALITY: /* WINVER >= 0x0500 */
    FIXME("DC_PERSONALITY: stub\n");
    return -1;
    
  /* Returns the amount of printer memory in kilobytes. (Win2k/XP only) */
  case DC_PRINTERMEM: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTERMEM: stub\n");
    return -1;
    
  /* Returns the printer's print rate in PRINTRATEUNIT units. (Win2k/XP only) */
  case DC_PRINTRATE: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTRATE: stub\n");
    return -1;
    
  /* Returns the printer's print rate in pages per minute. (Win2k/XP only) */
  case DC_PRINTRATEPPM: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTRATEPPM: stub\n");
    return -1;
   
  /* Returns the printer rate unit used for DC_PRINTRATE, which is one of
   * PRINTRATEUNIT_{CPS,IPM,LPM,PPM} (Win2k/XP only)
   */  
  case DC_PRINTRATEUNIT: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTRATEUNIT: stub\n");
    return -1;
    
  default:
    FIXME("Unsupported capability %d\n", fwCapability);
  }
  return -1;
}

/**************************************************************
 *     DeviceCapabilities [WINEPS16.91]
 */
DWORD WINAPI PSDRV_DeviceCapabilities16(LPCSTR lpszDevice,
			       LPCSTR lpszPort, WORD fwCapability,
			       LPSTR lpszOutput, LPDEVMODEA lpdm)
{
    return PSDRV_DeviceCapabilities(NULL, lpszDevice, lpszPort, fwCapability,
				      lpszOutput, lpdm);
}

/***************************************************************
 *	DeviceMode	[WINEPS16.13]
 *
 */
void WINAPI PSDRV_DeviceMode16(HWND16 hwnd, HANDLE16 hDriver,
LPSTR lpszDevice, LPSTR lpszPort)
{
    PSDRV_ExtDeviceMode16( hwnd, hDriver, NULL, lpszDevice, lpszPort, NULL,
			   NULL, DM_PROMPT );
    return;
}

#if 0
typedef struct {
  DWORD nPages;
  DWORD Unknown;
  HPROPSHEETPAGE hPages[10];
} EDMPS;

INT PSDRV_ExtDeviceModePropSheet(HWND hwnd, LPSTR lpszDevice, LPSTR lpszPort,
				 LPVOID pPropSheet)
{
    EDMPS *ps = pPropSheet;
    PROPSHEETPAGE psp;

    psp->dwSize = sizeof(psp);
    psp->hInstance = 0x1234;

    ps->nPages = 1;

}

#endif

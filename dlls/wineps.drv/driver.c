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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "prsht.h"
#include "ddk/winddiui.h"

#include "wine/debug.h"

#include "psdrv.h"
#include "psdlg.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/* convert points to paper size units (10th of a millimeter) */
static inline int paper_size_from_points( float size )
{
    return size * 254 / 72;
}

INPUTSLOT *find_slot( PPD *ppd, const PSDRV_DEVMODE *dm )
{
    INPUTSLOT *slot;

    LIST_FOR_EACH_ENTRY( slot, &ppd->InputSlots, INPUTSLOT, entry )
        if (slot->WinBin == dm->dmPublic.dmDefaultSource)
            return slot;

    return NULL;
}

PAGESIZE *find_pagesize( PPD *ppd, const PSDRV_DEVMODE *dm )
{
    PAGESIZE *page;

    LIST_FOR_EACH_ENTRY( page, &ppd->PageSizes, PAGESIZE, entry )
        if (page->WinPage == dm->dmPublic.dmPaperSize)
            return page;

    return NULL;
}

DUPLEX *find_duplex( PPD *ppd, const PSDRV_DEVMODE *dm )
{
    DUPLEX *duplex;
    WORD win_duplex = dm->dmPublic.dmFields & DM_DUPLEX ? dm->dmPublic.dmDuplex : 0;

    if (win_duplex == 0) return NULL; /* Not capable */

    LIST_FOR_EACH_ENTRY( duplex, &ppd->Duplexes, DUPLEX, entry )
        if (duplex->WinDuplex == win_duplex)
            return duplex;

    return NULL;
}

/************************************************************************
 *
 *		PSDRV_MergeDevmodes
 *
 * Updates dm1 with some fields from dm2
 *
 */
void PSDRV_MergeDevmodes( PSDRV_DEVMODE *dm1, const PSDRV_DEVMODE *dm2, PRINTERINFO *pi )
{
    /* some sanity checks here on dm2 */

    if(dm2->dmPublic.dmFields & DM_ORIENTATION) {
        dm1->dmPublic.dmOrientation = dm2->dmPublic.dmOrientation;
	TRACE("Changing orientation to %d (%s)\n",
	      dm1->dmPublic.dmOrientation,
	      dm1->dmPublic.dmOrientation == DMORIENT_PORTRAIT ?
	      "Portrait" :
	      (dm1->dmPublic.dmOrientation == DMORIENT_LANDSCAPE ?
	       "Landscape" : "unknown"));
    }

    /* NB PaperWidth is always < PaperLength */
    if (dm2->dmPublic.dmFields & DM_PAPERSIZE)
    {
        PAGESIZE *page = find_pagesize( pi->ppd, dm2 );

        if (page)
        {
	    dm1->dmPublic.dmPaperSize = dm2->dmPublic.dmPaperSize;
	    dm1->dmPublic.dmPaperWidth  = paper_size_from_points( page->PaperDimension->x );
	    dm1->dmPublic.dmPaperLength = paper_size_from_points( page->PaperDimension->y );
	    dm1->dmPublic.dmFields |= DM_PAPERSIZE | DM_PAPERWIDTH | DM_PAPERLENGTH;
	    TRACE("Changing page to %s %d x %d\n", page->FullName,
		  dm1->dmPublic.dmPaperWidth,
		  dm1->dmPublic.dmPaperLength );

            if (dm1->dmPublic.dmSize >= FIELD_OFFSET(DEVMODEW, dmFormName) + CCHFORMNAME * sizeof(WCHAR))
            {
                MultiByteToWideChar(CP_ACP, 0, page->FullName, -1, dm1->dmPublic.dmFormName, CCHFORMNAME);
                dm1->dmPublic.dmFields |= DM_FORMNAME;
            }
	}
        else
            TRACE("Trying to change to unsupported pagesize %d\n", dm2->dmPublic.dmPaperSize);
    }

    else if((dm2->dmPublic.dmFields & DM_PAPERLENGTH) &&
       (dm2->dmPublic.dmFields & DM_PAPERWIDTH)) {
        dm1->dmPublic.dmPaperLength = dm2->dmPublic.dmPaperLength;
        dm1->dmPublic.dmPaperWidth = dm2->dmPublic.dmPaperWidth;
	TRACE("Changing PaperLength|Width to %dx%d\n",
	      dm2->dmPublic.dmPaperLength,
	      dm2->dmPublic.dmPaperWidth);
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

    if (dm2->dmPublic.dmFields & DM_DEFAULTSOURCE)
    {
        INPUTSLOT *slot = find_slot( pi->ppd, dm2 );

        if (slot)
        {
	    dm1->dmPublic.dmDefaultSource = dm2->dmPublic.dmDefaultSource;
	    TRACE("Changing bin to '%s'\n", slot->FullName);
	}
        else
            TRACE("Trying to change to unsupported bin %d\n", dm2->dmPublic.dmDefaultSource);
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
       lstrcpynW(dm1->dmPublic.dmFormName, dm2->dmPublic.dmFormName, CCHFORMNAME);
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
       dm1->dmPublic.dmPosition = dm2->dmPublic.dmPosition;
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


typedef struct
{
    PRINTERINFO *pi;
    PSDRV_DEVMODE *dlgdm;
} PSDRV_DLGINFO;

/****************************************************************
 *       PSDRV_PaperDlgProc
 *
 * Dialog proc for 'Paper' propsheet
 */
static INT_PTR CALLBACK PSDRV_PaperDlgProc(HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam)
{
  PSDRV_DLGINFO *di;
  int i, Cursel;
  PAGESIZE *ps;
  DUPLEX *duplex;
  RESOLUTION *res;

  switch(msg) {
  case WM_INITDIALOG:
    di = (PSDRV_DLGINFO*)((PROPSHEETPAGEA*)lParam)->lParam;
    SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)di);

    i = Cursel = 0;
    LIST_FOR_EACH_ENTRY(ps, &di->pi->ppd->PageSizes, PAGESIZE, entry) {
      SendDlgItemMessageA(hwnd, IDD_PAPERS, LB_INSERTSTRING, i,
			  (LPARAM)ps->FullName);
      if(di->pi->Devmode->dmPublic.dmPaperSize == ps->WinPage)
	Cursel = i;
      i++;
    }
    SendDlgItemMessageA(hwnd, IDD_PAPERS, LB_SETCURSEL, Cursel, 0);

    CheckRadioButton(hwnd, IDD_ORIENT_PORTRAIT, IDD_ORIENT_LANDSCAPE,
		     di->pi->Devmode->dmPublic.dmOrientation ==
		     DMORIENT_PORTRAIT ? IDD_ORIENT_PORTRAIT :
		     IDD_ORIENT_LANDSCAPE);

    if (list_empty( &di->pi->ppd->Duplexes ))
    {
        ShowWindow(GetDlgItem(hwnd, IDD_DUPLEX), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDD_DUPLEX_NAME), SW_HIDE);
    }
    else
    {
        i = Cursel = 0;
        LIST_FOR_EACH_ENTRY( duplex, &di->pi->ppd->Duplexes, DUPLEX, entry )
        {
            SendDlgItemMessageA(hwnd, IDD_DUPLEX, CB_INSERTSTRING, i,
                                (LPARAM)(duplex->FullName ? duplex->FullName : duplex->Name));
            if(di->pi->Devmode->dmPublic.dmDuplex == duplex->WinDuplex)
                Cursel = i;
            i++;
        }
        SendDlgItemMessageA(hwnd, IDD_DUPLEX, CB_SETCURSEL, Cursel, 0);
    }

    if (list_empty( &di->pi->ppd->Resolutions ))
    {
        int len, res;
        WCHAR buf[256];

        res = di->pi->ppd->DefaultResolution;
        len = swprintf(buf, ARRAY_SIZE(buf), L"%d", res);
        buf[len++] = ' ';
        LoadStringW(PSDRV_hInstance, IDS_DPI, buf + len, ARRAY_SIZE(buf) - len);
        SendDlgItemMessageW(hwnd, IDD_QUALITY, CB_ADDSTRING, 0, (LPARAM)buf);
        SendDlgItemMessageW(hwnd, IDD_QUALITY, CB_SETITEMDATA, 0, MAKELONG(res, res));
        Cursel = 0;
    }
    else
    {
        int resx, resy;

        Cursel = 0;
        resx = resy = di->pi->ppd->DefaultResolution;

        if (di->pi->Devmode->dmPublic.dmFields & DM_PRINTQUALITY)
            resx = resy = di->pi->Devmode->dmPublic.dmPrintQuality;

        if (di->pi->Devmode->dmPublic.dmFields & DM_YRESOLUTION)
            resy = di->pi->Devmode->dmPublic.dmYResolution;

        if (di->pi->Devmode->dmPublic.dmFields & DM_LOGPIXELS)
            resx = resy = di->pi->Devmode->dmPublic.dmLogPixels;

        LIST_FOR_EACH_ENTRY(res, &di->pi->ppd->Resolutions, RESOLUTION, entry)
        {
            int len;
            WCHAR buf[256];
            DWORD idx;

            if (res->resx == res->resy)
                len = swprintf(buf, ARRAY_SIZE(buf), L"%d", res->resx);
            else
                len = swprintf(buf, ARRAY_SIZE(buf), L"%dx%d", res->resx, res->resy);
            buf[len++] = ' ';
            LoadStringW(PSDRV_hInstance, IDS_DPI, buf + len, ARRAY_SIZE(buf) - len);
            idx = SendDlgItemMessageW(hwnd, IDD_QUALITY, CB_ADDSTRING, 0, (LPARAM)buf);
            SendDlgItemMessageW(hwnd, IDD_QUALITY, CB_SETITEMDATA, idx, MAKELONG(res->resx, res->resy));

            if (res->resx == resx && res->resy == resy)
                Cursel = idx;
        }
    }
    SendDlgItemMessageW(hwnd, IDD_QUALITY, CB_SETCURSEL, Cursel, 0);

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
        TRACE("Setting pagesize to item %d, WinPage %d (%s), PaperSize %.2fx%.2f\n", Cursel,
              ps->WinPage, ps->FullName, ps->PaperDimension->x, ps->PaperDimension->y);
        di->dlgdm->dmPublic.dmPaperSize = ps->WinPage;
        di->dlgdm->dmPublic.dmFields |= DM_PAPERSIZE;

        di->dlgdm->dmPublic.dmPaperWidth  = paper_size_from_points(ps->PaperDimension->x);
        di->dlgdm->dmPublic.dmPaperLength = paper_size_from_points(ps->PaperDimension->y);
        di->dlgdm->dmPublic.dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH;

        if (di->dlgdm->dmPublic.dmSize >= FIELD_OFFSET(DEVMODEW, dmFormName) + CCHFORMNAME * sizeof(WCHAR))
        {
            MultiByteToWideChar(CP_ACP, 0, ps->FullName, -1, di->dlgdm->dmPublic.dmFormName, CCHFORMNAME);
            di->dlgdm->dmPublic.dmFields |= DM_FORMNAME;
        }
        SendMessageW(GetParent(hwnd), PSM_CHANGED, 0, 0);
      }
      break;
    case IDD_ORIENT_PORTRAIT:
    case IDD_ORIENT_LANDSCAPE:
      TRACE("Setting orientation to %s\n", wParam == IDD_ORIENT_PORTRAIT ?
            "portrait" : "landscape");
      di->dlgdm->dmPublic.dmOrientation = wParam == IDD_ORIENT_PORTRAIT ?
        DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE;
      di->dlgdm->dmPublic.dmFields |= DM_ORIENTATION;
      SendMessageW(GetParent(hwnd), PSM_CHANGED, 0, 0);
      break;
    case IDD_DUPLEX:
      if(HIWORD(wParam) == CBN_SELCHANGE) {
	Cursel = SendDlgItemMessageA(hwnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
        i = 0;
        LIST_FOR_EACH_ENTRY( duplex, &di->pi->ppd->Duplexes, DUPLEX, entry )
        {
            if (i >= Cursel) break;
            i++;
        }
        TRACE("Setting duplex to item %d Winduplex = %d\n", Cursel, duplex->WinDuplex);
        di->dlgdm->dmPublic.dmDuplex = duplex->WinDuplex;
        di->dlgdm->dmPublic.dmFields |= DM_DUPLEX;
        SendMessageW(GetParent(hwnd), PSM_CHANGED, 0, 0);
      }
      break;

    case IDD_QUALITY:
      if (HIWORD(wParam) == CBN_SELCHANGE)
      {
        LPARAM data;
        int resx, resy;

        Cursel = SendDlgItemMessageW(hwnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
        data = SendDlgItemMessageW(hwnd, IDD_QUALITY, CB_GETITEMDATA, Cursel, 0);

        resx = LOWORD(data);
        resy = HIWORD(data);
        TRACE("Setting resolution to %dx%d\n", resx, resy);

        di->dlgdm->dmPublic.dmPrintQuality = resx;
        di->dlgdm->dmPublic.dmFields |= DM_PRINTQUALITY;

        di->dlgdm->dmPublic.dmYResolution = resy;
        di->dlgdm->dmPublic.dmFields |= DM_YRESOLUTION;

        if (di->pi->Devmode->dmPublic.dmFields & DM_LOGPIXELS)
        {
            di->dlgdm->dmPublic.dmLogPixels = resx;
            di->dlgdm->dmPublic.dmFields |= DM_LOGPIXELS;
        }

        SendMessageW(GetParent(hwnd), PSM_CHANGED, 0, 0);
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
      *di->pi->Devmode = *di->dlgdm;
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


static HPROPSHEETPAGE (WINAPI *pCreatePropertySheetPage) (LPCPROPSHEETPAGEW);
static int (WINAPI *pPropertySheet) (LPCPROPSHEETHEADERW);

/******************************************************************************
 *           DrvDocumentProperties    (wineps.drv.@)
 *
 *  Retrieves or modifies device-initialization information for the PostScript
 *  driver, or displays a driver-supplied dialog box for configuring the driver.
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
INT WINAPI DrvDocumentProperties(HWND hwnd, const WCHAR *device, DEVMODEW *output,
                                 DEVMODEW *input, DWORD mode)
{
  PRINTERINFO *pi;

  TRACE("(hwnd=%p, Device='%s', devOut=%p, devIn=%p, Mode=%04x)\n",
        hwnd, debugstr_w(device), output, input, mode);

  if (!(pi = PSDRV_FindPrinterInfo(device))) return -1;

  /* If mode == 0, return size of DEVMODE structure */
  if (!mode)
      return pi->Devmode->dmPublic.dmSize + pi->Devmode->dmPublic.dmDriverExtra;

  /* If DM_MODIFY is set, change settings in accordance with lpdmInput */
  if ((mode & DM_MODIFY) && input)
  {
    TRACE("DM_MODIFY set. devIn->dmFields = %08x\n", input->dmFields);
    PSDRV_MergeDevmodes(pi->Devmode, (PSDRV_DEVMODE *)input, pi);
  }

  /* If DM_PROMPT is set, present modal dialog box */
  if (mode & DM_PROMPT) {
    HINSTANCE hinstComctl32;
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEW psp;
    PROPSHEETHEADERW psh;
    PSDRV_DLGINFO di;
    PSDRV_DEVMODE dlgdm;
    WCHAR SetupW[64];

    LoadStringW(PSDRV_hInstance, IDS_SETUP, SetupW, ARRAY_SIZE(SetupW));
    hinstComctl32 = LoadLibraryA("comctl32.dll");
    pCreatePropertySheetPage = (void*)GetProcAddress(hinstComctl32,
						    "CreatePropertySheetPageW");
    pPropertySheet = (void*)GetProcAddress(hinstComctl32, "PropertySheetW");
    memset(&psp,0,sizeof(psp));
    dlgdm = *pi->Devmode;
    di.pi = pi;
    di.dlgdm = &dlgdm;
    psp.dwSize = sizeof(psp);
    psp.hInstance = PSDRV_hInstance;
    psp.pszTemplate = L"PAPER";
    psp.pszIcon = NULL;
    psp.pfnDlgProc = PSDRV_PaperDlgProc;
    psp.lParam = (LPARAM)&di;
    hpsp[0] = pCreatePropertySheetPage(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.pszCaption = SetupW;
    psh.nPages = 1;
    psh.hwndParent = hwnd;
    psh.phpage = hpsp;

    pPropertySheet(&psh);

  }

  /* If DM_UPDATE is set, should write settings to environment and initialization file */
  if (mode & DM_UPDATE)
    FIXME("Mode DM_UPDATE.  Just do the same as DM_COPY\n");

  /* If DM_COPY is set, should write settings to lpdmOutput */
  if (output && (mode & (DM_COPY | DM_UPDATE)))
    memcpy( output, &pi->Devmode->dmPublic,
            pi->Devmode->dmPublic.dmSize + pi->Devmode->dmPublic.dmDriverExtra );
  return IDOK;
}

/******************************************************************************
 *           DrvDeviceCapabilities    (wineps.drv.@)
 */
DWORD WINAPI DrvDeviceCapabilities(HANDLE printer, WCHAR *device_name, WORD capability,
                                   void *output, DEVMODEW *devmode)
{
  PRINTERINFO *pi;
  DEVMODEW *lpdm;
  DWORD ret;

  TRACE("%s %u, %p, %p\n", debugstr_w(device_name), capability, output, devmode);

  if (!(pi = PSDRV_FindPrinterInfo(device_name))) {
      ERR("no printer info for %s, return 0!\n", debugstr_w(device_name));
      return 0;
  }

  lpdm = &pi->Devmode->dmPublic;
  if (devmode) lpdm = devmode;

  switch(capability) {

  case DC_PAPERS:
    {
      PAGESIZE *ps;
      WORD *wp = output;
      int i = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
        TRACE("DC_PAPERS: %u\n", ps->WinPage);
        i++;
        if (output != NULL) *wp++ = ps->WinPage;
      }
      ret = i;
      break;
    }

  case DC_PAPERSIZE:
    {
      PAGESIZE *ps;
      POINT *pt = output;
      int i = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
        TRACE("DC_PAPERSIZE: %f x %f\n", ps->PaperDimension->x, ps->PaperDimension->y);
        i++;
        if (output != NULL) {
          pt->x = paper_size_from_points( ps->PaperDimension->x );
          pt->y = paper_size_from_points( ps->PaperDimension->y );
	  pt++;
	}
      }
      ret = i;
      break;
    }

  case DC_PAPERNAMES:
    {
      PAGESIZE *ps;
      WCHAR *cp = output;
      int i = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
        TRACE("DC_PAPERNAMES: %s\n", debugstr_a(ps->FullName));
        i++;
        if (output != NULL) {
          MultiByteToWideChar(CP_ACP, 0, ps->FullName, -1, cp, 64);
	  cp += 64;
	}
      }
      ret = i;
      break;
    }

  case DC_ORIENTATION:
    ret = pi->ppd->LandscapeOrientation ? pi->ppd->LandscapeOrientation : 90;
    break;

  case DC_BINS:
    {
      INPUTSLOT *slot;
      WORD *wp = output;
      int i = 0;

      LIST_FOR_EACH_ENTRY( slot, &pi->ppd->InputSlots, INPUTSLOT, entry )
      {
          i++;
          if (output != NULL)
              *wp++ = slot->WinBin;
      }
      ret = i;
      break;
    }

  case DC_BINNAMES:
    {
      INPUTSLOT *slot;
      WCHAR *cp = output;
      int i = 0;

      LIST_FOR_EACH_ENTRY( slot, &pi->ppd->InputSlots, INPUTSLOT, entry )
      {
          i++;
          if (output != NULL)
          {
              MultiByteToWideChar(CP_ACP, 0, slot->FullName, -1, cp, 24);
              cp += 24;
          }
      }
      ret = i;
      break;
    }

  case DC_BINADJUST:
    FIXME("DC_BINADJUST: stub.\n");
    ret = DCBA_FACEUPNONE;
    break;

  case DC_ENUMRESOLUTIONS:
    {
        RESOLUTION *res;
        LONG *lp = output;
        int i = 0;

        LIST_FOR_EACH_ENTRY(res, &pi->ppd->Resolutions, RESOLUTION, entry)
        {
            i++;
            if (output != NULL)
            {
                lp[0] = res->resx;
                lp[1] = res->resy;
                lp += 2;
            }
        }
        ret = i;
        break;
    }

  /* Windows returns 9999 too */
  case DC_COPIES:
    TRACE("DC_COPIES: returning 9999\n");
    ret = 9999;
    break;

  case DC_DRIVER:
    ret = lpdm->dmDriverVersion;
    break;

  case DC_DATATYPE_PRODUCED:
    FIXME("DATA_TYPE_PRODUCED: stub.\n");
    ret = -1; /* simulate that the driver supports 'RAW' */
    break;

  case DC_DUPLEX:
    ret = 0;
    if(pi->ppd->DefaultDuplex && pi->ppd->DefaultDuplex->WinDuplex != 0)
      ret = 1;
    TRACE("DC_DUPLEX: returning %d\n", ret);
    break;

  case DC_EMF_COMPLIANT:
    FIXME("DC_EMF_COMPLIANT: stub.\n");
    ret = -1; /* simulate that the driver do not support EMF */
    break;

  case DC_EXTRA:
    ret = lpdm->dmDriverExtra;
    break;

  case DC_FIELDS:
    ret = lpdm->dmFields;
    break;

  case DC_FILEDEPENDENCIES:
    FIXME("DC_FILEDEPENDENCIES: stub.\n");
    ret = 0;
    break;

  case DC_MAXEXTENT:
    {
      PAGESIZE *ps;
      float x = 0, y = 0;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
          if (ps->PaperDimension->x > x) x = ps->PaperDimension->x;
          if (ps->PaperDimension->y > y) y = ps->PaperDimension->y;
      }
      ret = MAKELONG( paper_size_from_points(x), paper_size_from_points(y) );
      break;
    }

  case DC_MINEXTENT:
    {
      PAGESIZE *ps;
      float x = 1e6, y = 1e6;

      LIST_FOR_EACH_ENTRY(ps, &pi->ppd->PageSizes, PAGESIZE, entry)
      {
          if (ps->PaperDimension->x < x) x = ps->PaperDimension->x;
          if (ps->PaperDimension->y < y) y = ps->PaperDimension->y;
      }
      ret = MAKELONG( paper_size_from_points(x), paper_size_from_points(y) );
      break;
    }

  case DC_SIZE:
    ret = lpdm->dmSize;
    break;

  case DC_TRUETYPE:
    FIXME("DC_TRUETYPE: stub\n");
    ret = DCTT_SUBDEV;
    break;

  case DC_VERSION:
    ret = lpdm->dmSpecVersion;
    break;

  /* We'll just return false here, very few printers can collate anyway */
  case DC_COLLATE:
    TRACE("DC_COLLATE: returning FALSE\n");
    ret = FALSE;
    break;

  /* Printer supports colour printing - 1 if yes, 0 if no (Win2k/XP only) */
  case DC_COLORDEVICE:
    ret = pi->ppd->ColorDevice != CD_False;
    break;

  /* Identification number of the printer manufacturer for use with ICM (Win9x only) */
  case DC_MANUFACTURER:
    FIXME("DC_MANUFACTURER: stub\n");
    ret = -1;
    break;

  /* Identification number of the printer model for use with ICM (Win9x only) */
  case DC_MODEL:
    FIXME("DC_MODEL: stub\n");
    ret = -1;
    break;

  /* Nonzero if the printer supports stapling, zero otherwise (Win2k/XP only) */
  case DC_STAPLE: /* WINVER >= 0x0500 */
    FIXME("DC_STAPLE: stub\n");
    ret = -1;
    break;

  /* Returns an array of 64-character string buffers containing the names of the paper forms
   * available for use, unless pOutput is NULL.  The return value is the number of paper forms.
   * (Win2k/XP only)
   */
  case DC_MEDIAREADY: /* WINVER >= 0x0500 */
    FIXME("DC_MEDIAREADY: stub\n");
    ret = -1;
    break;

  /* Returns an array of 64-character string buffers containing the names of the supported
   * media types, unless pOutput is NULL.  The return value is the number of supported.
   * media types (XP only)
   */
  case DC_MEDIATYPENAMES: /* WINVER >= 0x0501 */
    FIXME("DC_MEDIATYPENAMES: stub\n");
    ret = -1;
    break;

  /* Returns an array of DWORD values which represent the supported media types, unless
   * pOutput is NULL.  The return value is the number of supported media types. (XP only)
   */
  case DC_MEDIATYPES: /* WINVER >= 0x0501 */
    FIXME("DC_MEDIATYPES: stub\n");
    ret = -1;
    break;

  /* Returns an array of DWORD values, each representing a supported number of document
   * pages per printed page, unless pOutput is NULL.  The return value is the number of
   * array entries. (Win2k/XP only)
   */
  case DC_NUP:
    FIXME("DC_NUP: stub\n");
    ret = -1;
    break;

  /* Returns an array of 32-character string buffers containing a list of printer description
   * languages supported by the printer, unless pOutput is NULL.  The return value is
   * number of array entries. (Win2k/XP only)
   */
  case DC_PERSONALITY: /* WINVER >= 0x0500 */
    FIXME("DC_PERSONALITY: stub\n");
    ret = -1;
    break;

  /* Returns the amount of printer memory in kilobytes. (Win2k/XP only) */
  case DC_PRINTERMEM: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTERMEM: stub\n");
    ret = -1;
    break;

  /* Returns the printer's print rate in PRINTRATEUNIT units. (Win2k/XP only) */
  case DC_PRINTRATE: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTRATE: stub\n");
    ret = -1;
    break;

  /* Returns the printer's print rate in pages per minute. (Win2k/XP only) */
  case DC_PRINTRATEPPM: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTRATEPPM: stub\n");
    ret = -1;
    break;

  /* Returns the printer rate unit used for DC_PRINTRATE, which is one of
   * PRINTRATEUNIT_{CPS,IPM,LPM,PPM} (Win2k/XP only)
   */
  case DC_PRINTRATEUNIT: /* WINVER >= 0x0500 */
    FIXME("DC_PRINTRATEUNIT: stub\n");
    ret = -1;
    break;

  default:
    FIXME("Unsupported capability %d\n", capability);
    ret = -1;
  }

  return ret;
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

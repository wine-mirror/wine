/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 * Copyright 2000 Huw D M Davies
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "commdlg.h"
#include "dlgs.h"
#include "module.h"
#include "debugtools.h"
#include "winproc.h"
#include "cderr.h"
#include "winspool.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"


/* This PRINTDLGA internal structure stores
 * pointers to several throughout useful structures.
 * 
 */
typedef struct  
{
  LPDEVMODEA        lpDevMode;
  LPPRINTDLGA       lpPrintDlg;
  LPPRINTER_INFO_2A lpPrinterInfo;
  UINT              HelpMessageID;
  HICON             hCollateIcon;    /* PrintDlg only */
  HICON             hNoCollateIcon;  /* PrintDlg only */
  HICON             hPortraitIcon;   /* PrintSetupDlg only */
  HICON             hLandscapeIcon;  /* PrintSetupDlg only */
} PRINT_PTRA;

/* Debugiging info */
static struct pd_flags {
  DWORD flag;
  LPSTR name;
} pd_flags[] = {
  {PD_SELECTION, "PD_SELECTION "},
  {PD_PAGENUMS, "PD_PAGENUMS "},
  {PD_NOSELECTION, "PD_NOSELECTION "},
  {PD_NOPAGENUMS, "PD_NOPAGENUMS "},
  {PD_COLLATE, "PD_COLLATE "},
  {PD_PRINTTOFILE, "PD_PRINTTOFILE "},
  {PD_PRINTSETUP, "PD_PRINTSETUP "},
  {PD_NOWARNING, "PD_NOWARNING "},
  {PD_RETURNDC, "PD_RETURNDC "},
  {PD_RETURNIC, "PD_RETURNIC "},
  {PD_RETURNDEFAULT, "PD_RETURNDEFAULT "},
  {PD_SHOWHELP, "PD_SHOWHELP "},
  {PD_ENABLEPRINTHOOK, "PD_ENABLEPRINTHOOK "},
  {PD_ENABLESETUPHOOK, "PD_ENABLESETUPHOOK "},
  {PD_ENABLEPRINTTEMPLATE, "PD_ENABLEPRINTTEMPLATE "},
  {PD_ENABLESETUPTEMPLATE, "PD_ENABLESETUPTEMPLATE "},
  {PD_ENABLEPRINTTEMPLATEHANDLE, "PD_ENABLEPRINTTEMPLATEHANDLE "},
  {PD_ENABLESETUPTEMPLATEHANDLE, "PD_ENABLESETUPTEMPLATEHANDLE "},
  {PD_USEDEVMODECOPIES, "PD_USEDEVMODECOPIES[ANDCOLLATE] "},
  {PD_DISABLEPRINTTOFILE, "PD_DISABLEPRINTTOFILE "},
  {PD_HIDEPRINTTOFILE, "PD_HIDEPRINTTOFILE "},
  {PD_NONETWORKBUTTON, "PD_NONETWORKBUTTON "},
  {-1, NULL}
};


/***********************************************************************
 *    PRINTDLG_GetDefaultPrinterName
 *
 * Returns the default printer name in buf.
 * Even under WinNT/2000 default printer is retrieved via GetProfileString - 
 * these entries are mapped somewhere in the registry rather than win.ini.
 *
 * Returns TRUE on success else FALSE
 */
static BOOL PRINTDLG_GetDefaultPrinterName(LPSTR buf, DWORD len)
{
    char *ptr;

    if(!GetProfileStringA("windows", "device", "", buf, len))
	return FALSE;
    if((ptr = strchr(buf, ',')) == NULL)
	return FALSE;
    *ptr = '\0';
    return TRUE;
}

/***********************************************************************
 *    PRINTDLG_OpenDefaultPrinter
 *
 * Returns a winspool printer handle to the default printer in *hprn
 * Caller must call ClosePrinter on the handle
 *
 * Returns TRUE on success else FALSE
 */
static BOOL PRINTDLG_OpenDefaultPrinter(HANDLE *hprn)
{
    char buf[260];
    if(!PRINTDLG_GetDefaultPrinterName(buf, sizeof(buf)))
        return FALSE;
    return OpenPrinterA(buf, hprn, NULL);
}

/***********************************************************************
 *    PRINTDLG_SetUpPrinterListCombo
 *
 * Initializes printer list combox.
 * hDlg:  HWND of dialog
 * id:    Control id of combo
 * name:  Name of printer to select
 *
 * Initializes combo with list of available printers.  Selects printer 'name'
 * If name is NULL or does not exist select the default printer.
 *
 * Returns number of printers added to list.
 */
static INT PRINTDLG_SetUpPrinterListCombo(HWND hDlg, UINT id, LPCSTR name)
{
    DWORD needed, num;
    INT i;
    LPPRINTER_INFO_2A pi;
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &needed, &num);
    pi = HeapAlloc(GetProcessHeap(), 0, needed);
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)pi, needed, &needed, 
		  &num);

    for(i = 0; i < num; i++) {
        SendDlgItemMessageA(hDlg, id, CB_ADDSTRING, 0,
			    (LPARAM)pi[i].pPrinterName );
    }
    HeapFree(GetProcessHeap(), 0, pi);
    if(!name ||
       (i = SendDlgItemMessageA(hDlg, id, CB_FINDSTRINGEXACT, -1,
				(LPARAM)name)) == CB_ERR) {

        char buf[260];
        TRACE("Can't find '%s' in printer list so trying to find default\n",
	      name);
	if(!PRINTDLG_GetDefaultPrinterName(buf, sizeof(buf)))
	    return num;
	i = SendDlgItemMessageA(hDlg, id, CB_FINDSTRINGEXACT, -1, (LPARAM)buf);
	if(i == CB_ERR)
	    TRACE("Can't find default printer in printer list\n");
    }
    SendDlgItemMessageA(hDlg, id, CB_SETCURSEL, i, 0);
    return num;
}

/***********************************************************************
 *             PRINTDLG_CreateDevNames          [internal]
 *
 *
 *   creates a DevNames structure.
 *
 *  (NB. when we handle unicode the offsets will be in wchars).
 */
static BOOL PRINTDLG_CreateDevNames(HGLOBAL *hmem, char* DeviceDriverName, 
				    char* DeviceName, char* OutputPort)
{
    long size;
    char*   pDevNamesSpace;
    char*   pTempPtr;
    LPDEVNAMES lpDevNames;
    char buf[260];

    size = strlen(DeviceDriverName) + 1
            + strlen(DeviceName) + 1
            + strlen(OutputPort) + 1
            + sizeof(DEVNAMES);
            
    if(*hmem)
        *hmem = GlobalReAlloc(*hmem, size, GMEM_MOVEABLE);
    else
        *hmem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (*hmem == 0)
        return FALSE;

    pDevNamesSpace = GlobalLock(*hmem);
    lpDevNames = (LPDEVNAMES) pDevNamesSpace;

    pTempPtr = pDevNamesSpace + sizeof(DEVNAMES);
    strcpy(pTempPtr, DeviceDriverName);
    lpDevNames->wDriverOffset = pTempPtr - pDevNamesSpace;

    pTempPtr += strlen(DeviceDriverName) + 1;
    strcpy(pTempPtr, DeviceName);
    lpDevNames->wDeviceOffset = pTempPtr - pDevNamesSpace;
        
    pTempPtr += strlen(DeviceName) + 1;
    strcpy(pTempPtr, OutputPort);
    lpDevNames->wOutputOffset = pTempPtr - pDevNamesSpace;

    PRINTDLG_GetDefaultPrinterName(buf, sizeof(buf));
    lpDevNames->wDefault = (strcmp(buf, DeviceName) == 0) ? 1 : 0;
    GlobalUnlock(*hmem);
    return TRUE;
}

/***********************************************************************
 *             PRINTDLG_UpdatePrintDlg          [internal]
 *
 *
 *   updates the PrintDlg structure for returnvalues.
 *      
 * RETURNS
 *   FALSE if user is not allowed to close (i.e. wrong nTo or nFrom values)
 *   TRUE  if succesful.
 */
static BOOL PRINTDLG_UpdatePrintDlg(HWND hDlg, 
				    PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA       lppd = PrintStructures->lpPrintDlg;
    PDEVMODEA         lpdm = PrintStructures->lpDevMode;
    LPPRINTER_INFO_2A pi = PrintStructures->lpPrinterInfo;


    if(!lpdm) return FALSE;


    if(!(lppd->Flags & PD_PRINTSETUP)) {
        /* check whether nFromPage and nToPage are within range defined by
	 * nMinPage and nMaxPage
	 */
        if (IsDlgButtonChecked(hDlg, rad3) == BST_CHECKED) { /* Pages */
	    WORD nToPage;
	    WORD nFromPage;
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
	    if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
		nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage) {
	        char resourcestr[256];
		char resultstr[256];
		LoadStringA(COMDLG32_hInstance, PD32_INVALID_PAGE_RANGE, 
			    resourcestr, 255);
		sprintf(resultstr,resourcestr, lppd->nMinPage, lppd->nMaxPage);
		LoadStringA(COMDLG32_hInstance, PD32_PRINT_TITLE, 
			    resourcestr, 255);
		MessageBoxA(hDlg, resultstr, resourcestr,
			    MB_OK | MB_ICONWARNING);
		return FALSE;
	    }
	    lppd->nFromPage = nFromPage;
	    lppd->nToPage   = nToPage;
	}

	if (IsDlgButtonChecked(hDlg, chx1) == BST_CHECKED) {/* Print to file */
	    lppd->Flags |= PD_PRINTTOFILE;
	    pi->pPortName = "FILE:";
	}

	if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED) { /* Collate */
	    FIXME("Collate lppd not yet implemented as output\n");
	}

	/* set PD_Collate and nCopies */
	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /*  The application doesn't support multiple copies or collate...
	   */
	    lppd->Flags &= ~PD_COLLATE;
	    lppd->nCopies = 1;
	  /* if the printer driver supports it... store info there
	   * otherwise no collate & multiple copies !
	   */
	    if (lpdm->dmFields & DM_COLLATE)
	        lpdm->dmCollate = 
		  (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED);
	    if (lpdm->dmFields & DM_COPIES)
	        lpdm->dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	} else {
	    if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
	        lppd->Flags |= PD_COLLATE;
            else
               lppd->Flags &= ~PD_COLLATE;
            lppd->nCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	}
    }
    return TRUE;
}


/************************************************************************
 * PRINTDLG_SetUpPaperComboBox
 *
 * Initialize either the papersize or inputslot combos of the Printer Setup
 * dialog.  We store the associated word (eg DMPAPER_A4) as the item data.
 * We also try to re-select the old selection.
 */
static BOOL PRINTDLG_SetUpPaperComboBox(HWND hDlg,
					int   nIDComboBox,
					char* PrinterName, 
					char* PortName,
					LPDEVMODEA dm)
{
    int     i;
    DWORD   NrOfEntries;
    char*   Names;
    WORD*   Words;
    DWORD   Sel;
    WORD    oldWord = 0;
    int     NamesSize;
    int     fwCapability_Names;
    int     fwCapability_Words;
    
    TRACE(" Printer: %s, ComboID: %d\n",PrinterName,nIDComboBox);
    
    /* query the dialog box for the current selected value */
    Sel = SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETCURSEL, 0, 0);
    if(Sel != CB_ERR) {
        oldWord = SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA, Sel,
				      0);
    }

    if (nIDComboBox == cmb2) {
         NamesSize          = 64;
         fwCapability_Names = DC_PAPERNAMES;
         fwCapability_Words = DC_PAPERS;
    } else {
         nIDComboBox        = cmb3;
         NamesSize          = 24;
         fwCapability_Names = DC_BINNAMES;
         fwCapability_Words = DC_BINS;
    }
    
    /* for some printer drivers, DeviceCapabilities calls a VXD to obtain the 
     * paper settings. As Wine doesn't allow VXDs, this results in a crash.
     */
    WARN(" if your printer driver uses VXDs, expect a crash now!\n");
    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
                                      fwCapability_Names, NULL, dm);
    if (NrOfEntries == 0)
         WARN("no Name Entries found!\n");

    if(DeviceCapabilitiesA(PrinterName, PortName, fwCapability_Words, NULL, dm)
       != NrOfEntries) {
        ERR("Number of caps is different\n");
	NrOfEntries = 0;
    }

    Names = HeapAlloc(GetProcessHeap(),0, NrOfEntries*NamesSize);
    Words = HeapAlloc(GetProcessHeap(),0, NrOfEntries*sizeof(WORD));
    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
                                      fwCapability_Names, Names, dm);
    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
				      fwCapability_Words, (LPSTR)Words, dm);

    /* reset any current content in the combobox */
    SendDlgItemMessageA(hDlg, nIDComboBox, CB_RESETCONTENT, 0, 0);
    
    /* store new content */
    for (i = 0; i < NrOfEntries; i++) {
        DWORD pos = SendDlgItemMessageA(hDlg, nIDComboBox, CB_ADDSTRING, 0,
					(LPARAM)(&Names[i*NamesSize]) );
	SendDlgItemMessageA(hDlg, nIDComboBox, CB_SETITEMDATA, pos, 
			    Words[i]);
    }

    /* Look for old selection - can't do this is previous loop since
       item order will change as more items are added */
    Sel = 0;
    for (i = 0; i < NrOfEntries; i++) {
        if(SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) ==
	   oldWord) {
	    Sel = i;
	    break;
	}
    }
    SendDlgItemMessageA(hDlg, nIDComboBox, CB_SETCURSEL, Sel, 0);

    HeapFree(GetProcessHeap(),0,Words);
    HeapFree(GetProcessHeap(),0,Names);
    return TRUE;
}

/***********************************************************************
 *               PRINTDLG_UpdatePrinterInfoTexts               [internal]
 */
static void PRINTDLG_UpdatePrinterInfoTexts(HWND hDlg, LPPRINTER_INFO_2A pi)
{
    char   StatusMsg[256];
    char   ResourceString[256];
    int    i;

    /* Status Message */
    StatusMsg[0]='\0';

    /* add all status messages */
    for (i = 0; i < 25; i++) {
        if (pi->Status & (1<<i)) {
	    LoadStringA(COMDLG32_hInstance, PD32_PRINTER_STATUS_PAUSED+i, 
			ResourceString, 255);
	    strcat(StatusMsg,ResourceString);
        }
    }
    /* append "ready" */
    /* FIXME: status==ready must only be appended if really so. 
              but how to detect? */
    LoadStringA(COMDLG32_hInstance, PD32_PRINTER_STATUS_READY, 
		ResourceString, 255);
    strcat(StatusMsg,ResourceString);
  
    SendDlgItemMessageA(hDlg, stc12, WM_SETTEXT, 0, (LPARAM)StatusMsg);

    /* set all other printer info texts */
    SendDlgItemMessageA(hDlg, stc11, WM_SETTEXT, 0, (LPARAM)pi->pDriverName);
    if (pi->pLocation != NULL && pi->pLocation[0] != '\0')
        SendDlgItemMessageA(hDlg, stc14, WM_SETTEXT, 0,(LPARAM)pi->pLocation);
    else
        SendDlgItemMessageA(hDlg, stc14, WM_SETTEXT, 0,(LPARAM)pi->pPortName);
    SendDlgItemMessageA(hDlg, stc13, WM_SETTEXT, 0, (LPARAM)pi->pComment);
    return;
}


/*******************************************************************
 *
 *                 PRINTDLG_ChangePrinter
 *
 */
static BOOL PRINTDLG_ChangePrinter(HWND hDlg, char *name,
				   PRINT_PTRA *PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    LPDEVMODEA lpdm = NULL;
    LONG dmSize;
    DWORD needed;
    HANDLE hprn;

    if(PrintStructures->lpPrinterInfo)
        HeapFree(GetProcessHeap(),0, PrintStructures->lpPrinterInfo);
    if(!OpenPrinterA(name, &hprn, NULL)) {
        ERR("Can't open printer %s\n", name);
	return FALSE;
    }
    GetPrinterA(hprn, 2, NULL, 0, &needed);
    PrintStructures->lpPrinterInfo = HeapAlloc(GetProcessHeap(),0,needed);
    GetPrinterA(hprn, 2, (LPBYTE)PrintStructures->lpPrinterInfo, needed,
		&needed);
    ClosePrinter(hprn);

    PRINTDLG_UpdatePrinterInfoTexts(hDlg, PrintStructures->lpPrinterInfo);

    if(PrintStructures->lpDevMode) {
        HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	PrintStructures->lpDevMode = NULL;
    }

    dmSize = DocumentPropertiesA(0, 0, name, NULL, NULL, 0);
    if(dmSize == -1) {
        ERR("DocumentProperties fails on %s\n", debugstr_a(name));
	return FALSE;
    }
    PrintStructures->lpDevMode = HeapAlloc(GetProcessHeap(), 0, dmSize);
    dmSize = DocumentPropertiesA(0, 0, name, PrintStructures->lpDevMode, NULL,
				 DM_OUT_BUFFER);
    if(lppd->hDevMode && (lpdm = GlobalLock(lppd->hDevMode)) &&
			  !strcmp(lpdm->dmDeviceName,
				  PrintStructures->lpDevMode->dmDeviceName)) {
      /* Supplied devicemode matches current printer so try to use it */
        DocumentPropertiesA(0, 0, name, PrintStructures->lpDevMode, lpdm,
			    DM_OUT_BUFFER | DM_IN_BUFFER);
    }
    if(lpdm)
        GlobalUnlock(lppd->hDevMode);

    lpdm = PrintStructures->lpDevMode;  /* use this as a shortcut */

    if(!(lppd->Flags & PD_PRINTSETUP)) {
      /* Print range (All/Range/Selection) */
        SetDlgItemInt(hDlg, edt1, lppd->nFromPage, FALSE);
	SetDlgItemInt(hDlg, edt2, lppd->nToPage, FALSE);
	CheckRadioButton(hDlg, rad1, rad3, rad1);		/* default */
	if (lppd->Flags & PD_NOSELECTION)
	    EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
	else
	    if (lppd->Flags & PD_SELECTION)
	        CheckRadioButton(hDlg, rad1, rad3, rad2);
	if (lppd->Flags & PD_NOPAGENUMS) {
	    EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
	    EnableWindow(GetDlgItem(hDlg, stc2),FALSE);
	    EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
	    EnableWindow(GetDlgItem(hDlg, stc3),FALSE);
	    EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
	} else {
	    if (lppd->Flags & PD_PAGENUMS)
	        CheckRadioButton(hDlg, rad1, rad3, rad3);
	}
	/* "All xxx pages"... */
	{
	    char        resourcestr[64];
	    char        result[64];
	    LoadStringA(COMDLG32_hInstance, PD32_PRINT_ALL_X_PAGES, 
			resourcestr, 49);
	    sprintf(result,resourcestr,lppd->nMaxPage - lppd->nMinPage + 1);
	    SendDlgItemMessageA(hDlg, rad1, WM_SETTEXT, 0, (LPARAM) result);
	}
        
	/* Collate pages 
	 *
	 * FIXME: The ico3 is not displayed for some reason. I don't know why.
	 */
	if (lppd->Flags & PD_COLLATE) {
	    SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON, 
				(LPARAM)PrintStructures->hCollateIcon);
	    CheckDlgButton(hDlg, chx2, 1);
	} else {
	    SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON, 
				(LPARAM)PrintStructures->hNoCollateIcon);
	    CheckDlgButton(hDlg, chx2, 0);
	}

	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /* if printer doesn't support it: no Collate */
	    if (!(lpdm->dmFields & DM_COLLATE)) {
	        EnableWindow(GetDlgItem(hDlg, chx2), FALSE);    
		EnableWindow(GetDlgItem(hDlg, ico3), FALSE);    
	    }
	}

	/* nCopies */
	if (lppd->hDevMode == 0)
	    SetDlgItemInt(hDlg, edt3, lppd->nCopies, FALSE);
	else
	    SetDlgItemInt(hDlg, edt3, lpdm->dmCopies, FALSE);

	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /* if printer doesn't support it: no nCopies */
	    if (!(lpdm->dmFields & DM_COPIES)) {
	        EnableWindow(GetDlgItem(hDlg, edt3), FALSE);    
		EnableWindow(GetDlgItem(hDlg, stc5), FALSE);    
	    }
	}

	/* print to file */
	CheckDlgButton(hDlg, chx1, (lppd->Flags & PD_PRINTTOFILE) ? 1 : 0);
	if (lppd->Flags & PD_DISABLEPRINTTOFILE)
            EnableWindow(GetDlgItem(hDlg, chx1), FALSE);    
	if (lppd->Flags & PD_HIDEPRINTTOFILE)
            ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);

    } else { /* PD_PRINTSETUP */
      PRINTDLG_SetUpPaperComboBox(hDlg, cmb2,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      PRINTDLG_SetUpPaperComboBox(hDlg, cmb3,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      CheckRadioButton(hDlg, rad1, rad2,
		       (lpdm->u1.s1.dmOrientation == DMORIENT_PORTRAIT) ?
		       rad1: rad2);
    }

    /* help button */
    if ((lppd->Flags & PD_SHOWHELP)==0) {
        /* hide if PD_SHOWHELP not specified */
        ShowWindow(GetDlgItem(hDlg, pshHelp), SW_HIDE);         
    }

    return TRUE;
}

/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam,
				     PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    DEVNAMES *pdn;
    DEVMODEA *pdm;
    char *name = NULL;
    UINT comboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;

    /* load Collate ICONs */
    PrintStructures->hCollateIcon =
      LoadIconA(COMDLG32_hInstance, "PD32_COLLATE");
    PrintStructures->hNoCollateIcon = 
      LoadIconA(COMDLG32_hInstance, "PD32_NOCOLLATE");
    if(PrintStructures->hCollateIcon == 0 ||
       PrintStructures->hNoCollateIcon == 0) {
        ERR("no icon in resourcefile\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	EndDialog(hDlg, FALSE);
    }

    /* load Paper Orientation ICON */
    /* FIXME: not implemented yet */

    /*
     * if lppd->Flags PD_SHOWHELP is specified, a HELPMESGSTRING message
     * must be registered and the Help button must be shown.
     */
    if (lppd->Flags & PD_SHOWHELP) {
        if((PrintStructures->HelpMessageID = 
	    RegisterWindowMessageA(HELPMSGSTRING)) == 0) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
	    return FALSE;
	}
    } else
        PrintStructures->HelpMessageID = 0;

    /* FIXME: I allow more freedom than either Win95 or WinNT,
     *        which do not agree to what errors should be thrown or not
     *        in case nToPage or nFromPage is out-of-range.
     */
    if (lppd->nMaxPage < lppd->nMinPage)
    	lppd->nMaxPage = lppd->nMinPage;
    if (lppd->nMinPage == lppd->nMaxPage) 
    	lppd->Flags |= PD_NOPAGENUMS;        
    if (lppd->nToPage < lppd->nMinPage)
        lppd->nToPage = lppd->nMinPage;
    if (lppd->nToPage > lppd->nMaxPage)
        lppd->nToPage = lppd->nMaxPage;
    if (lppd->nFromPage < lppd->nMinPage)
        lppd->nFromPage = lppd->nMinPage;
    if (lppd->nFromPage > lppd->nMaxPage)
        lppd->nFromPage = lppd->nMaxPage;

    /* Fill Combobox 
     */
    pdn = GlobalLock(lppd->hDevNames);
    pdm = GlobalLock(lppd->hDevMode);
    if(pdn)
        name = (char*)pdn + pdn->wDeviceOffset;
    else if(pdm)
        name = pdm->dmDeviceName;
    PRINTDLG_SetUpPrinterListCombo(hDlg, comboID, name);
    if(pdm) GlobalUnlock(lppd->hDevMode);
    if(pdn) GlobalUnlock(lppd->hDevNames);

    /* Now find selected printer and update rest of dlg */
    name = HeapAlloc(GetProcessHeap(),0,256);
    GetDlgItemTextA(hDlg, comboID, name, 255);
    PRINTDLG_ChangePrinter(hDlg, name, PrintStructures);
    HeapFree(GetProcessHeap(),0,name);

    return TRUE;
}

/***********************************************************************
 *                              PRINTDLG_WMCommand               [internal]
 */
static LRESULT PRINTDLG_WMCommand(HWND hDlg, WPARAM wParam, 
			LPARAM lParam, PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    UINT PrinterComboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;
    LPDEVMODEA lpdm = PrintStructures->lpDevMode;

    switch (LOWORD(wParam))  {
    case IDOK:
        TRACE(" OK button was hit\n");
        if (PRINTDLG_UpdatePrintDlg(hDlg, PrintStructures)!=TRUE)
	    return(FALSE);
	EndDialog(hDlg, TRUE);
	return(TRUE);

    case IDCANCEL:
        TRACE(" CANCEL button was hit\n");
        EndDialog(hDlg, FALSE);
	return(FALSE);

     case pshHelp:
        TRACE(" HELP button was hit\n");
        SendMessageA(lppd->hwndOwner, PrintStructures->HelpMessageID, 
        			(WPARAM) hDlg, (LPARAM) lppd);
        break;

     case chx2:                         /* collate pages checkbox */
        if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON, 
                                    (LPARAM)PrintStructures->hCollateIcon);
        else
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON, 
                                    (LPARAM)PrintStructures->hNoCollateIcon);
        break;        
     case edt1:                         /* from page nr editbox */
     case edt2:                         /* to page nr editbox */
        if (HIWORD(wParam)==EN_CHANGE) {
	    WORD nToPage;
	    WORD nFromPage;
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
            if (nFromPage != lppd->nFromPage || nToPage != lppd->nToPage)
	        CheckRadioButton(hDlg, rad1, rad3, rad3);
	}
        break;
     case psh2:                       /* Properties button */
       {
         HANDLE hPrinter;
         char   PrinterName[256];

         GetDlgItemTextA(hDlg, PrinterComboID, PrinterName, 255);
         if (!OpenPrinterA(PrinterName, &hPrinter, NULL)) {
	     WARN(" Call to OpenPrinter did not succeed!\n");
	     break;
	 }
	 DocumentPropertiesA(hDlg, hPrinter, PrinterName, 
			     PrintStructures->lpDevMode,
			     PrintStructures->lpDevMode,
			     DM_IN_BUFFER | DM_OUT_BUFFER | DM_IN_PROMPT);
	 ClosePrinter(hPrinter);
         break;
       }

    case cmb1:
    case cmb4:                         /* Printer combobox */
         if (HIWORD(wParam)==CBN_SELCHANGE) {
	     char   PrinterName[256];
	     GetDlgItemTextA(hDlg, LOWORD(wParam), PrinterName, 255);
	     PRINTDLG_ChangePrinter(hDlg, PrinterName, PrintStructures);
	 }
	 break;

    case cmb2: /* Papersize */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u1.s1.dmPaperSize = SendDlgItemMessageA(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->dmDefaultSource = SendDlgItemMessageA(hDlg, cmb3,
							  CB_GETITEMDATA, Sel,
							  0);
      }
      break; 
    }
    return FALSE;
}    

/***********************************************************************
 *           PrintDlgProcA			[internal]
 */
BOOL WINAPI PrintDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
			  LPARAM lParam)
{
    PRINT_PTRA* PrintStructures;
    LRESULT res=FALSE;

    if (uMsg!=WM_INITDIALOG) {
        PrintStructures = (PRINT_PTRA*) GetWindowLongA(hDlg, DWL_USER);   
	if (!PrintStructures)
	    return FALSE;
    } else {
        PrintStructures = (PRINT_PTRA*) lParam;
	SetWindowLongA(hDlg, DWL_USER, lParam); 
	res = PRINTDLG_WMInitDialog(hDlg, wParam, PrintStructures);

	if(PrintStructures->lpPrintDlg->Flags & PD_ENABLEPRINTHOOK) {
	    res = PrintStructures->lpPrintDlg->lpfnPrintHook(hDlg, uMsg,
						 wParam,
				       (LPARAM)PrintStructures->lpPrintDlg); 
	}
	return res;
    }
  
    if(PrintStructures->lpPrintDlg->Flags & PD_ENABLEPRINTHOOK) {
        res = PrintStructures->lpPrintDlg->lpfnPrintHook(hDlg, uMsg, wParam,
							 lParam);
	if(res) return res;
    }

    switch (uMsg) {
    case WM_COMMAND:
        return PRINTDLG_WMCommand(hDlg, wParam, lParam, PrintStructures);

    case WM_DESTROY:
	DestroyIcon(PrintStructures->hCollateIcon);
	DestroyIcon(PrintStructures->hNoCollateIcon);
    /* FIXME: don't forget to delete the paper orientation icons here! */

        return FALSE;
    }    
    return res;
}

/************************************************************
 *
 *      PRINTDLG_GetDlgTemplate
 *
 */
static HGLOBAL PRINTDLG_GetDlgTemplate(PRINTDLGA *lppd)
{
    HGLOBAL hDlgTmpl, hResInfo;

    if (lppd->Flags & PD_PRINTSETUP) {
        if(lppd->Flags & PD_ENABLESETUPTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hSetupTemplate;
	} else if(lppd->Flags & PD_ENABLESETUPTEMPLATE) {	
	    hResInfo = FindResourceA(lppd->hInstance,
				     lppd->lpSetupTemplateName, RT_DIALOGA);
	    hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
	} else {
	    hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32_SETUP",
				     RT_DIALOGA);
	    hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo);
	}
    } else {
        if(lppd->Flags & PD_ENABLEPRINTTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hPrintTemplate;
	} else if(lppd->Flags & PD_ENABLEPRINTTEMPLATE) {
	    hResInfo = FindResourceA(lppd->hInstance,
				     lppd->lpPrintTemplateName,
				     RT_DIALOGA);
	    hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
	} else {
	    hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32",
				     RT_DIALOGA);
	    hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo);
	}
    }
    return hDlgTmpl;
}

/***********************************************************************
 *
 *      PRINTDLG_CreateDC
 *
 */
static BOOL PRINTDLG_CreateDC(LPPRINTDLGA lppd)
{
    DEVNAMES *pdn = GlobalLock(lppd->hDevNames);
    DEVMODEA *pdm = GlobalLock(lppd->hDevMode);

    if(lppd->Flags & PD_RETURNDC) {
        lppd->hDC = CreateDCA((char*)pdn + pdn->wDriverOffset,
			      (char*)pdn + pdn->wDeviceOffset,
			      (char*)pdn + pdn->wOutputOffset,
			      pdm );
    } else if(lppd->Flags & PD_RETURNIC) {
        lppd->hDC = CreateICA((char*)pdn + pdn->wDriverOffset,
			      (char*)pdn + pdn->wDeviceOffset,
			      (char*)pdn + pdn->wOutputOffset,
			      pdm );
    }
    GlobalUnlock(lppd->hDevNames);
    GlobalUnlock(lppd->hDevMode);
    return lppd->hDC ? TRUE : FALSE;
}

/***********************************************************************
 *           PrintDlgA   (COMDLG32.17)
 *
 *  Displays the the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  PrintDlg:
 *  * The Collate Icons do not display, even though they are in the code.
 *  * The Properties Button(s) should call DocumentPropertiesA().
 *  PrintSetupDlg:
 *  * The Paper Orientation Icons are not implemented yet.
 *  * The Properties Button(s) should call DocumentPropertiesA().
 *  * Settings are not yet taken from a provided DevMode or 
 *    default printer settings.
 */
BOOL WINAPI PrintDlgA(
		      LPPRINTDLGA lppd /* ptr to PRINTDLG32 struct */
		      )
{
    BOOL      bRet = FALSE;
    LPVOID   ptr;
    HINSTANCE hInst = GetWindowLongA( lppd->hwndOwner, GWL_HINSTANCE );

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	struct pd_flags *pflag = pd_flags;
	for( ; pflag->name; pflag++) {
	    if(lppd->Flags & pflag->flag)
	        strcat(flagstr, pflag->name);
	}
	TRACE("(%p): hwndOwner = %08x, hDevMode = %08x, hDevNames = %08x\n"
	      "pp. %d-%d, min p %d, max p %d, copies %d, hinst %08x\n"
	      "flags %08lx (%s)\n",
	      lppd, lppd->hwndOwner, lppd->hDevMode, lppd->hDevNames,
	      lppd->nFromPage, lppd->nToPage, lppd->nMinPage, lppd->nMaxPage,
	      lppd->nCopies, lppd->hInstance, lppd->Flags, flagstr);
    }

    if(lppd->lStructSize != sizeof(PRINTDLGA)) {
        WARN("structure size failure !!!\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_STRUCTSIZE);
	return FALSE; 
    }

    if(lppd->Flags & PD_RETURNDEFAULT) {
        PRINTER_INFO_2A *pbuf;
	HANDLE hprn;
	DWORD needed;

	if(lppd->hDevMode || lppd->hDevNames) {
	    WARN("hDevMode or hDevNames non-zero for PD_RETURNDEFAULT\n");
	    COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE); 
	    return FALSE;
	}
        if(!PRINTDLG_OpenDefaultPrinter(&hprn)) {
	    WARN("Can't find default printer\n");
	    COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN); 
	    return FALSE;
	}

	GetPrinterA(hprn, 2, NULL, 0, &needed);
	pbuf = HeapAlloc(GetProcessHeap(), 0, needed);
	GetPrinterA(hprn, 2, (LPBYTE)pbuf, needed, &needed);
	ClosePrinter(hprn);
	PRINTDLG_CreateDevNames(&(lppd->hDevNames), "winspool",
				  pbuf->pDevMode->dmDeviceName,
				  pbuf->pPortName);
	lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE, pbuf->pDevMode->dmSize +
				     pbuf->pDevMode->dmDriverExtra);
	ptr = GlobalLock(lppd->hDevMode);
	memcpy(ptr, pbuf->pDevMode, pbuf->pDevMode->dmSize +
	       pbuf->pDevMode->dmDriverExtra);
	GlobalUnlock(lppd->hDevMode);
	HeapFree(GetProcessHeap(), 0, pbuf);
	bRet = TRUE;
    } else {
	HGLOBAL hDlgTmpl;
	PRINT_PTRA *PrintStructures;

    /* load Dialog resources, 
     * depending on Flags indicates Print32 or Print32_setup dialog 
     */
	hDlgTmpl = PRINTDLG_GetDlgTemplate(lppd);
	if (!(hDlgTmpl) || !(ptr = LockResource( hDlgTmpl ))) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        PrintStructures = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				    sizeof(PRINT_PTRA));
	PrintStructures->lpPrintDlg = lppd;

	/* and create & process the dialog 
	 */
	bRet = DialogBoxIndirectParamA(hInst, ptr, lppd->hwndOwner,
				       PrintDlgProcA,
				       (LPARAM)PrintStructures);

	if(bRet) {
	    DEVMODEA *lpdm = PrintStructures->lpDevMode, *lpdmReturn;
	    PRINTER_INFO_2A *pi = PrintStructures->lpPrinterInfo;

	    if (lppd->hDevMode == 0) {
	        TRACE(" No hDevMode yet... Need to create my own\n");
		lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE,
					lpdm->dmSize + lpdm->dmDriverExtra);
	    } else {
	        WORD locks;
		if((locks = (GlobalFlags(lppd->hDevMode) & GMEM_LOCKCOUNT))) {
		    WARN("hDevMode has %d locks on it. Unlocking it now\n", locks);
		    while(locks--) {
		        GlobalUnlock(lppd->hDevMode);
			TRACE("Now got %d locks\n", locks);
		    }
		}
		lppd->hDevMode = GlobalReAlloc(lppd->hDevMode,
					       lpdm->dmSize + lpdm->dmDriverExtra,
					       GMEM_MOVEABLE);
	    }
	    lpdmReturn = GlobalLock(lppd->hDevMode);
	    memcpy(lpdmReturn, lpdm, lpdm->dmSize + lpdm->dmDriverExtra);

	    if (lppd->hDevNames != 0) {
	        WORD locks;
		if((locks = (GlobalFlags(lppd->hDevNames) & GMEM_LOCKCOUNT))) {
		    WARN("hDevNames has %d locks on it. Unlocking it now\n", locks);
		    while(locks--)
		        GlobalUnlock(lppd->hDevNames);
		}
	    }
	    PRINTDLG_CreateDevNames(&(lppd->hDevNames), "winspool",
				    lpdmReturn->dmDeviceName, pi->pPortName);
	    GlobalUnlock(lppd->hDevMode);
	}
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpPrinterInfo);
	HeapFree(GetProcessHeap(), 0, PrintStructures);
    }
    if(bRet && (lppd->Flags & PD_RETURNDC || lppd->Flags & PD_RETURNIC))
        bRet = PRINTDLG_CreateDC(lppd);

    TRACE("exit! (%d)\n", bRet);        
    return bRet;    
}



/***********************************************************************
 *           PrintDlgW   (COMDLG32.18)
 */
BOOL WINAPI PrintDlgW( LPPRINTDLGW printdlg )
{
    FIXME("A really empty stub\n" );
    return FALSE;
}

/***********************************************************************
 *
 *          PageSetupDlg
 */

/***********************************************************************
 *            PageSetupDlgA  (COMDLG32.15)
 */
BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg) {
	FIXME("(%p), stub!\n",setupdlg);
	return FALSE;
}
/***********************************************************************
 *            PageSetupDlgW  (COMDLG32.16)
 */
BOOL WINAPI PageSetupDlgW(LPPAGESETUPDLGW setupdlg) {
	FIXME("(%p), stub!\n",setupdlg);
	return FALSE;
}

/**********************************************************************
 *
 *      16 bit commdlg
 */

/***********************************************************************
 *           PrintDlgProc16   (COMMDLG.21)
 */
LRESULT WINAPI PrintDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                            LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow16(hWnd, SW_SHOWNORMAL);
      return (TRUE);
    case WM_COMMAND:
      switch (wParam)
	{
	case IDOK:
	  EndDialog(hWnd, TRUE);
	  return(TRUE);
	case IDCANCEL:
	  EndDialog(hWnd, FALSE);
	  return(TRUE);
	}
      return(FALSE);
    }
  return FALSE;
}


/***********************************************************************
 *           PrintSetupDlgProc16   (COMMDLG.22)
 */
LRESULT WINAPI PrintSetupDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
				   LPARAM lParam)
{
  switch (wMsg)
    {
    case WM_INITDIALOG:
      TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow16(hWnd, SW_SHOWNORMAL);
      return (TRUE);
    case WM_COMMAND:
      switch (wParam) {
      case IDOK:
	EndDialog(hWnd, TRUE);
	return(TRUE);
      case IDCANCEL:
	EndDialog(hWnd, FALSE);
	return(TRUE);
      }
      return(FALSE);
    }
  return FALSE;
}


/***********************************************************************
 *           PrintDlg16   (COMMDLG.20)
 * 
 *  Displays the the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  * calls up to the 32-bit versions of the Dialogs, which look different
 *  * Customizing is *not* implemented.
 */
BOOL16 WINAPI PrintDlg16( LPPRINTDLG16 lpPrint )
{
    PRINTDLGA Print32;
    BOOL16 ret;
    char *ptr, *ptr16;
    DWORD size;

    memset(&Print32, 0, sizeof(Print32));
    Print32.lStructSize = sizeof(Print32);
    Print32.hwndOwner   = lpPrint->hwndOwner;
    if(lpPrint->hDevMode) {
        size = GlobalSize16(lpPrint->hDevMode);
        Print32.hDevMode = GlobalAlloc(GMEM_MOVEABLE, size);
	ptr = GlobalLock(Print32.hDevMode);
	ptr16 = GlobalLock16(lpPrint->hDevMode);
	memcpy(ptr, ptr16, size);
	GlobalFree16(lpPrint->hDevMode);
	GlobalUnlock(Print32.hDevMode);
    } else
        Print32.hDevMode = 0;
    if(lpPrint->hDevNames) {
        size = GlobalSize16(lpPrint->hDevNames);
        Print32.hDevNames = GlobalAlloc(GMEM_MOVEABLE, size);
	ptr = GlobalLock(Print32.hDevNames);
	ptr16 = GlobalLock16(lpPrint->hDevNames);
	memcpy(ptr, ptr16, size);
	GlobalFree16(lpPrint->hDevNames);
	GlobalUnlock(Print32.hDevNames);
    } else
        Print32.hDevNames = 0;
    Print32.Flags       = lpPrint->Flags;
    Print32.nFromPage   = lpPrint->nFromPage;
    Print32.nToPage     = lpPrint->nToPage;
    Print32.nMinPage    = lpPrint->nMinPage;
    Print32.nMaxPage    = lpPrint->nMaxPage;
    Print32.nCopies     = lpPrint->nCopies;
    Print32.hInstance   = lpPrint->hInstance;
    Print32.lCustData   = lpPrint->lCustData;
    if(lpPrint->lpfnPrintHook) {
        FIXME("Need to allocate thunk\n");
/*        Print32.lpfnPrintHook = lpPrint->lpfnPrintHook;*/
    }
    if(lpPrint->lpfnSetupHook) {
        FIXME("Need to allocate thunk\n");
/*	Print32.lpfnSetupHook = lpPrint->lpfnSetupHook;*/
    }
    Print32.lpPrintTemplateName = PTR_SEG_TO_LIN(lpPrint->lpPrintTemplateName);
    Print32.lpSetupTemplateName = PTR_SEG_TO_LIN(lpPrint->lpSetupTemplateName);
    Print32.hPrintTemplate = lpPrint->hPrintTemplate;
    Print32.hSetupTemplate = lpPrint->hSetupTemplate;

    ret = PrintDlgA(&Print32);

    if(Print32.hDevMode) {
        size = GlobalSize(Print32.hDevMode);
	lpPrint->hDevMode = GlobalAlloc16(GMEM_MOVEABLE, size);
	ptr16 = GlobalLock16(lpPrint->hDevMode);
	ptr = GlobalLock(Print32.hDevMode);
	memcpy(ptr16, ptr, size);
	GlobalFree(Print32.hDevMode);
	GlobalUnlock16(lpPrint->hDevMode);
    } else
        lpPrint->hDevMode = 0;
    if(Print32.hDevNames) {
        size = GlobalSize(Print32.hDevNames);
	lpPrint->hDevNames = GlobalAlloc16(GMEM_MOVEABLE, size);
	ptr16 = GlobalLock16(lpPrint->hDevNames);
	ptr = GlobalLock(Print32.hDevNames);
	memcpy(ptr16, ptr, size);
	GlobalFree(Print32.hDevNames);
	GlobalUnlock16(lpPrint->hDevNames);
    } else
        lpPrint->hDevNames = 0;
    lpPrint->hDC       = Print32.hDC;
    lpPrint->Flags     = Print32.Flags;
    lpPrint->nFromPage = Print32.nFromPage;
    lpPrint->nToPage   = Print32.nToPage;
    lpPrint->nCopies   = Print32.nCopies;

    return ret;
}

/***********************************************************************
 *	PrintDlgExA
 */
HRESULT WINAPI PrintDlgExA(/*LPPRINTDLGEXA*/ LPVOID lpPrintDlgExA)
{
	FIXME("stub\n");
	return E_NOTIMPL;
}
/***********************************************************************
 *	PrintDlgExW
 */
HRESULT WINAPI PrintDlgExW(/*LPPRINTDLGEXW*/ LPVOID lpPrintDlgExW)
{
	FIXME("stub\n");
	return E_NOTIMPL;
}

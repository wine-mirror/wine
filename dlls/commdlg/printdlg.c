/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "commdlg.h"
#include "dialog.h"
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
  LPPRINTER_INFO_2A lpPrinterInfo;
  UINT              CurrentPrinter;  /* used as lpPrinterInfo[CurrentPrinter] */
  UINT              DefaultPrinter;  /* used as lpPrinterInfo[DefaultPrinter] */
  DWORD             NrOfPrinterInfoEntries;
  LPPRINTDLGA       lpPrintDlg;
  UINT              HelpMessageID;
  HICON             hCollateIcon;    /* PrintDlg only */
  HICON             hNoCollateIcon;  /* PrintDlg only */
  HICON             hPortraitIcon;   /* PrintSetupDlg only */
  HICON             hLandscapeIcon;  /* PrintSetupDlg only */
} PRINT_PTRA;


/* prototypes */
static BOOL PRINTDLG_ValidateAndDuplicateSettings(HWND hDlg, 
						  PRINT_PTRA* PrintStructures);

LRESULT WINAPI PrintSetupDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);


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

    memset(&Print32, 0, sizeof(Print32));
    Print32.lStructSize = sizeof(Print32);
    Print32.hwndOwner   = lpPrint->hwndOwner;
    Print32.hDevMode    = lpPrint->hDevMode;
    Print32.hDevNames   = lpPrint->hDevNames;
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

    lpPrint->hDevMode  = Print32.hDevMode;
    lpPrint->hDevNames = Print32.hDevNames;
    lpPrint->hDC       = Print32.hDC;
    lpPrint->Flags     = Print32.Flags;
    lpPrint->nFromPage = Print32.nFromPage;
    lpPrint->nToPage   = Print32.nToPage;
    lpPrint->nCopies   = Print32.nCopies;

    return ret;
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
/* My implementing strategy:
 * 
 * step 1: display the dialog and implement the layout-flags
 * step 2: enter valid information in the fields (e.g. real printers)
 * step 3: fix the RETURN-TRUE-ALWAYS Fixme by checking lppd->Flags for
 *         PD_RETURNDEFAULT
 * step 4: implement all other specs
 * step 5: allow customisation of the dialog box
 *
 * current implementation is in step 4.
 */ 

    HWND      hwndDialog;
    BOOL      bRet = FALSE;
    LPCVOID   ptr;
    HANDLE    hResInfo, hDlgTmpl;
    HINSTANCE hInst = GetWindowLongA( lppd->hwndOwner, GWL_HINSTANCE );
    DWORD     EnumBytesNeeded;
    DWORD     CopyOfEnumBytesNeeded;
    PRINT_PTRA PrintStructures;

    TRACE("(lppd: %p)\n", lppd);
    PrintStructures.lpPrintDlg      = lppd;
    
    /* load Dialog resources, 
     * depending on Flags indicates Print32 or Print32_setup dialog 
     */
    if (lppd->Flags & PD_PRINTSETUP)
        hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32_SETUP", RT_DIALOGA);
    else
        hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32", RT_DIALOGA);
    if (!hResInfo)
      {
	   COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	   return FALSE;
      }
    
    if (!(hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo )) ||
        !(ptr = LockResource( hDlgTmpl )))
      {
       COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
       return FALSE;
      }

    /* load Collate ICONs */
    PrintStructures.hCollateIcon = 
                              LoadIconA(COMDLG32_hInstance, "PD32_COLLATE");
    PrintStructures.hNoCollateIcon = 
                                LoadIconA(COMDLG32_hInstance, "PD32_NOCOLLATE");
    if (PrintStructures.hCollateIcon==0 || PrintStructures.hNoCollateIcon==0)
    {
    ERR("no icon in resourcefile???");
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }

    /* load Paper Orientation ICON */
        /* FIXME: not implemented yet */

    /*
     * if lppd->Flags PD_SHOWHELP is specified, a HELPMESGSTRING message
     * must be registered and the Help button must be shown.
     */
    if (lppd->Flags & PD_SHOWHELP)
       {
        if((PrintStructures.HelpMessageID = 
                       RegisterWindowMessageA(HELPMSGSTRING)) == 0)
            {
             COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
             return FALSE;
            }
       }
    else
    	PrintStructures.HelpMessageID=0;
	
    /* Use EnumPrinters to obtain a list of PRINTER_INFO_2A's
     * and store a pointer to this list in our "global structure"
     * as reference for the rest of the PrintDlg routines
     */
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 
        0, &EnumBytesNeeded, &PrintStructures.NrOfPrinterInfoEntries);
    CopyOfEnumBytesNeeded=EnumBytesNeeded+16;
    PrintStructures.lpPrinterInfo = malloc(CopyOfEnumBytesNeeded*sizeof(char));
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, 
        (LPBYTE)PrintStructures.lpPrinterInfo, 
        CopyOfEnumBytesNeeded, &EnumBytesNeeded, 
        &PrintStructures.NrOfPrinterInfoEntries);

    /* Find the default printer.
     * If not: display a warning message (unless PD_NOWARNING specified)
     * and return PDERR_NODEFAULTPRN
     */
    /* FIXME: not implemented yet!!! */
    if (!PrintStructures.NrOfPrinterInfoEntries)
    {
        COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN);
        return FALSE;
    }
    PrintStructures.CurrentPrinter=0; 
    PrintStructures.DefaultPrinter=0; 
     
    /* FIXME: Currently Unimplemented */
    if (lppd->Flags & PD_NOWARNING)	
	   {
	    COMDLG32_SetCommDlgExtendedError(PDERR_INITFAILURE); 
	    WARN(": PD_NOWARNING Flag is not yet implemented.\n");
	   }
    	
    /*
     * FIXME : Should respond to TEMPLATE and HOOK flags here
     * For now, only the standard dialog works.
     */
    if (lppd->Flags & (PD_ENABLEPRINTHOOK | PD_ENABLEPRINTTEMPLATE |
			  PD_ENABLEPRINTTEMPLATEHANDLE | PD_ENABLESETUPHOOK | 
			  PD_ENABLESETUPTEMPLATE|PD_ENABLESETUPTEMPLATEHANDLE)) 
    	FIXME(": unimplemented flag (ignored)\n");     
	
    /*
     * if lppd->Flags PD_RETURNDEFAULT is specified, the PrintDlg function
     * does not display the dialog box, but returns with valid entries
     * for hDevMode and hDevNames .
     *
     * According to MSDN, it is required that hDevMode and hDevNames equal
     * zero if this flag is set.
     */
    if (lppd->Flags & PD_RETURNDEFAULT)
       {
	TRACE(" PD_RETURNDEFAULT: was requested to return printer info only.\n");
        if (lppd->hDevMode!=0 || lppd->hDevNames !=0)
            {
     	     COMDLG32_SetCommDlgExtendedError(PDERR_INITFAILURE); 
             return(FALSE);
            }
        return(PRINTDLG_ValidateAndDuplicateSettings(0, &PrintStructures));
       }
	
    /* and create & process the dialog 
     */
	if (lppd->Flags & PD_PRINTSETUP)
		{
         hwndDialog= DIALOG_CreateIndirect(hInst, ptr, TRUE, lppd->hwndOwner,
            (DLGPROC16)PrintSetupDlgProcA, (LPARAM)&PrintStructures, WIN_PROC_32A );
        }
    else
		{
         hwndDialog= DIALOG_CreateIndirect(hInst, ptr, TRUE, lppd->hwndOwner,
            (DLGPROC16)PrintDlgProcA, (LPARAM)&PrintStructures, WIN_PROC_32A );
        }
    if (hwndDialog) 
        bRet = DIALOG_DoDialogBox(hwndDialog, lppd->hwndOwner);  
     
    /* free memory & resources
     */   
    free(PrintStructures.lpPrinterInfo);
    DeleteObject(PrintStructures.hCollateIcon);
    DeleteObject(PrintStructures.hNoCollateIcon);
    /* FIXME: don't forget to delete the paper orientation icons here! */

  TRACE(" exit! (%d)", bRet);        
  return bRet;            
}



/***********************************************************************
 *           PrintDlg32W   (COMDLG32.18)
 */
BOOL WINAPI PrintDlgW( LPPRINTDLGW printdlg )
{
    FIXME("A really empty stub\n" );
    return FALSE;
}


/***********************************************************************
 *               PRINTDLG_UpdatePrinterInfoTexts               [internal]
 */
void PRINTDLG_UpdatePrinterInfoTexts(HWND hDlg, PRINT_PTRA* PrintStructures)
{
    char   StatusMsg[256];
    char   ResourceString[256];
    int    i;
    LPPRINTER_INFO_2A lpPi = &(PrintStructures->lpPrinterInfo
                                             [PrintStructures->CurrentPrinter]);
    
    /* Status Message */
    StatusMsg[0]='\0';
    /* FIXME: if default printer, add this first */
    ;
    /* add all status messages */
    for (i=0; i< 25; i++)
    {
        if (lpPi->Status & (1<<i))
        {
         LoadStringA(COMDLG32_hInstance, PD32_PRINTER_STATUS_PAUSED+i, 
                        ResourceString, 255);
         strcat(StatusMsg,ResourceString);
        }
    }
    /* append "ready" */
    /* FIXME: status==ready must only be appended if really so. 
              but how to detect??? */
    LoadStringA(COMDLG32_hInstance, PD32_PRINTER_STATUS_READY, 
                        ResourceString, 255);
    strcat(StatusMsg,ResourceString);
  
    SendDlgItemMessageA(hDlg, stc12, WM_SETTEXT, 0, (LPARAM)StatusMsg);

    /* set all other printer info texts */
    SendDlgItemMessageA(hDlg, stc11, WM_SETTEXT, 0, (LPARAM)lpPi->pDriverName);
    if (lpPi->pLocation != NULL && lpPi->pLocation[0]!='\0')
        SendDlgItemMessageA(hDlg, stc14, WM_SETTEXT, 0,(LPARAM)lpPi->pLocation);
    else                                        
        SendDlgItemMessageA(hDlg, stc14, WM_SETTEXT, 0,(LPARAM)lpPi->pPortName);
    SendDlgItemMessageA(hDlg, stc13, WM_SETTEXT, 0, (LPARAM)lpPi->pComment);
}

/***********************************************************************
 *        PRINTSETUP32DLG_ComboBox               [internal]
 *
 * Queries the DeviceCapabilities for a list of paper sizes / bin names
 * and stores these in combobox cmb2 / cmb3.
 * If there was already an item selected in the listbox,
 * this item is looked up in the new list and reselected, 
 * the accompanying ID (for BinNames, this is the dmDefaultSource value)
 * is returned
 *
 * If any entries in the listbox existed, these are deleted
 *
 * RETURNS:
 *   If an entry was selected and also exists in the new list,
 *   its corresponding ID is returned.
 * 
 *   returns zero on not found, error or SelectedName==NULL.
 *
 *
 * BUGS:
 * * the lookup of a new entry shouldn't be done on stringname,
 *   but on ID value, as some drivers name the same paper format 
 *   differently (language differences, added paper size)
 */
short PRINTSETUP32DLG_UpdateComboBox(HWND hDlg,
                                      int   nIDComboBox,
                                      char* PrinterName, 
                                      char* PortName)
{
    int     i;
    DWORD   NrOfEntries;
    char*   Names;
    WORD*   Sizes;
    HGLOBAL hTempMem;
    short   returnvalue = 0;
    char    SelectedName[256];
    int     NamesSize;
    int     fwCapability_Names;
    int     fwCapability_Words;
    
    TRACE(" Printer: %s, ComboID: %d\n",PrinterName,nIDComboBox);
    
    /* query the dialog box for the current selected value */
    GetDlgItemTextA(hDlg, nIDComboBox, SelectedName, 255);

    if (nIDComboBox == cmb2)
        {
         NamesSize          = 64;
         fwCapability_Names = DC_PAPERNAMES;
         fwCapability_Words = DC_PAPERS;
        }
    else
        {
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
                                      fwCapability_Names, NULL, NULL);
    if (NrOfEntries == 0)
        {
         WARN(" no Name Entries found!\n");
        }
    hTempMem = GlobalAlloc(GMEM_MOVEABLE, NrOfEntries*NamesSize);
    if (hTempMem == 0)
        {
         ERR(" Not enough memory to store Paper Size Names!\n");
         return(0);
        }
    Names = GlobalLock(hTempMem);
    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
                                      fwCapability_Names, Names, NULL);
                                      
    /* reset any current content in the combobox */
    SendDlgItemMessageA(hDlg, nIDComboBox, CB_RESETCONTENT, 0, 0);
    
    /* store new content */
    for (i=0; i<NrOfEntries; i++)
      {
       SendDlgItemMessageA(hDlg, nIDComboBox, CB_ADDSTRING, 0,
                        (LPARAM)(&Names[i*NamesSize]) );
      }
                        
    /* select first entry */                        
    SendDlgItemMessageA(hDlg, nIDComboBox, CB_SELECTSTRING, 0, 
                        (LPARAM)(&Names[0]) );

    /* lookup SelectedName and select it, if found */
    if (SelectedName[0] != '\0')
       {
        for (i=0; i<NrOfEntries; i++)
            {
             if (strcmp(&Names[i*NamesSize], SelectedName)==0)
                {
                 SendDlgItemMessageA(hDlg, nIDComboBox, CB_SELECTSTRING, 0, 
                                     (LPARAM)(SelectedName));
                 
                 /* now, we need the i-th entry from the list of paper sizes */
                 /* let's recycle the memory */
                 DeviceCapabilitiesA(PrinterName, PortName, fwCapability_Words,
                                     Names, NULL);
                 Sizes = (WORD*) Names;
                 returnvalue = Sizes[i];                 
                 break; /* quit for loop */
                }
            }
       }
                        
    GlobalUnlock(hTempMem);
    GlobalFree(hTempMem);
 return(returnvalue);
}



/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
				     PRINT_PTRA* PrintStructures)
{
 int               i;
 LPPRINTDLGA       lppd     = PrintStructures->lpPrintDlg;
 LPPRINTER_INFO_2A lppi     = PrintStructures->lpPrinterInfo;
 PDEVMODEA	   pDevMode = lppi[PrintStructures->CurrentPrinter].pDevMode; 
 
	SetWindowLongA(hDlg, DWL_USER, lParam); 
	TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);

	if (lppd->lStructSize != sizeof(PRINTDLGA))
	{
		FIXME("structure size failure !!!\n");
/*		EndDialog (hDlg, 0); 
		return FALSE; 
*/	}

/* Fill Combobox according to info from PRINTER_INFO2A
 * structure inside PrintStructures,
 * select the default printer and generate an
 * update-message to have the rest of the dialog box updated.
 */ 
    for (i=0; i < PrintStructures->NrOfPrinterInfoEntries; i++)
	   SendDlgItemMessageA(hDlg, cmb4, CB_ADDSTRING, 0,
                        (LPARAM)lppi[i].pPrinterName );
    i=SendDlgItemMessageA(hDlg, cmb4, CB_SELECTSTRING, 
        (WPARAM) -1,
        (LPARAM) lppi[PrintStructures->CurrentPrinter].pPrinterName);
    SendDlgItemMessageA(hDlg, cmb4, CB_SETCURSEL, 
        (WPARAM)i, (LPARAM)0);
    PRINTDLG_UpdatePrinterInfoTexts(hDlg, PrintStructures);

/* Flag processing to set the according buttons on/off and
 * Initialise the various values
 */

    /* Print range (All/Range/Selection) */
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
    SetDlgItemInt(hDlg, edt1, lppd->nFromPage, FALSE);
    SetDlgItemInt(hDlg, edt2, lppd->nToPage, FALSE);
    CheckRadioButton(hDlg, rad1, rad3, rad1);		/* default */
    if (lppd->Flags & PD_NOSELECTION)
		EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
    else
		if (lppd->Flags & PD_SELECTION)
	    	CheckRadioButton(hDlg, rad1, rad3, rad2);
    if (lppd->Flags & PD_NOPAGENUMS)
       {
		EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc2),FALSE);
		EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc3),FALSE);
		EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
       }
    else
       {
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
    if (lppd->Flags & PD_COLLATE)
       {
        SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON, 
                                    (LPARAM)PrintStructures->hCollateIcon);
   	    CheckDlgButton(hDlg, chx2, 1);
       }
    else
       {
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON, 
                                    (LPARAM)PrintStructures->hNoCollateIcon);
   	    CheckDlgButton(hDlg, chx2, 0);
       }
       
    if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE ||
        lppd->Flags & PD_USEDEVMODECOPIES)
	{
	 /* if printer doesn't support it: no Collate */
	 if (!(pDevMode->dmFields & DM_COLLATE))
	    {
		EnableWindow(GetDlgItem(hDlg, chx2), FALSE);    
		EnableWindow(GetDlgItem(hDlg, ico3), FALSE);    
	    }
	}
        
    /* nCopies */
    if (lppd->hDevMode == 0)
    	{
         SetDlgItemInt(hDlg, edt3, lppd->nCopies, FALSE);
	}
    else
        {
         SetDlgItemInt(hDlg, edt1, pDevMode->dmCopies, FALSE);
	}
    if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE ||
        lppd->Flags & PD_USEDEVMODECOPIES)
	{
	 /* if printer doesn't support it: no nCopies */
	 if (!(pDevMode->dmFields & DM_COPIES))
	    {
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

    /* help button */
	if ((lppd->Flags & PD_SHOWHELP)==0)
    	{	/* hide if PD_SHOWHELP not specified */
		 ShowWindow(GetDlgItem(hDlg, pshHelp), SW_HIDE);         
        }

  GlobalUnlock(lppd->hDevMode);
  return TRUE;
}




/***********************************************************************
 *           PRINTSETUP32DLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTSETUP32DLG_WMInitDialog(HWND hDlg, WPARAM wParam, 
                     LPARAM lParam,
				     PRINT_PTRA* PrintStructures)
{
 int               i;
 LPPRINTDLGA       lppd     = PrintStructures->lpPrintDlg;
 LPPRINTER_INFO_2A lppi     = PrintStructures->lpPrinterInfo;
 
	SetWindowLongA(hDlg, DWL_USER, lParam); 
	TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);

	if (lppd->lStructSize != sizeof(PRINTDLGA))
	{
		FIXME("structure size failure !!!\n");
/*		EndDialog (hDlg, 0); 
		return FALSE; 
*/	}

/* Fill Combobox 1 according to info from PRINTER_INFO2A
 * structure inside PrintStructures,
 * select the default printer and generate an
 * update-message to have the rest of the dialog box updated.
 */ 
    for (i=0; i < PrintStructures->NrOfPrinterInfoEntries; i++)
	   SendDlgItemMessageA(hDlg, cmb1, CB_ADDSTRING, 0,
                        (LPARAM)lppi[i].pPrinterName );
    i=SendDlgItemMessageA(hDlg, cmb1, CB_SELECTSTRING, 
        (WPARAM) -1,
        (LPARAM) lppi[PrintStructures->CurrentPrinter].pPrinterName);
    SendDlgItemMessageA(hDlg, cmb1, CB_SETCURSEL, 
        (WPARAM)i, (LPARAM)0);
    PRINTDLG_UpdatePrinterInfoTexts(hDlg, PrintStructures);
    
/* 
 * fill both ComboBoxes with their info
 */
  PRINTSETUP32DLG_UpdateComboBox(hDlg, cmb2,
                lppi[PrintStructures->CurrentPrinter].pPrinterName,
                lppi[PrintStructures->CurrentPrinter].pPortName);
  PRINTSETUP32DLG_UpdateComboBox(hDlg, cmb3,
                lppi[PrintStructures->CurrentPrinter].pPrinterName,
                lppi[PrintStructures->CurrentPrinter].pPortName);
                
/* 
 * set the correct radiobutton & icon for print orientation
 */
  /* this should be dependent on a incoming DevMode 
   * (FIXME: not implemented yet) */
  CheckRadioButton(hDlg, rad1, rad2, rad1);
  /* also set the correct icon (FIXME: not implemented yet) */
                
  return TRUE;
}


/***********************************************************************
 *             PRINTDLG_CreateDevNames          [internal]
 *
 *
 *   creates a DevNames structure.
 * RETURNS
 *   HGLOBAL to DevNames memory object on success or
 *   zero on faillure
 */
HGLOBAL PRINTDLG_CreateDevNames(
                    char* DeviceDriverName, 
                    char* DeviceName, 
                    char* OutputPort, 
                    WORD  Flags)
{
    long size;
    HGLOBAL hDevNames;
    char*   pDevNamesSpace;
    char*   pTempPtr;
    LPDEVNAMES lpDevNames;
    
    size = strlen(DeviceDriverName) +1
            + strlen(DeviceName) + 1
            + strlen(OutputPort) + 1
            + sizeof(DEVNAMES);
            
    hDevNames = GlobalAlloc(GMEM_MOVEABLE, size*sizeof(char));
    if (hDevNames != 0)
    {
        pDevNamesSpace = GlobalLock(hDevNames);
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
        
        lpDevNames->wDefault = Flags;
        
        GlobalUnlock(hDevNames);
    }
    return(hDevNames);
}
            
    
           
    
/***********************************************************************
 *             PRINTDLG_ValidateAndDuplicateSettings          [internal]
 *
 *
 *   updates the PrintDlg structure for returnvalues.
 *   (yep, the name was chosen a bit stupid...)
 *
 *   if hDlg equals zero, only hDevModes and hDevNames are adapted.
 *      
 * RETURNS
 *   FALSE if user is not allowed to close (i.e. wrong nTo or nFrom values)
 *   TRUE  if succesful.
 */
static BOOL PRINTDLG_ValidateAndDuplicateSettings(HWND hDlg, 
						  PRINT_PTRA* PrintStructures)
{
 LPPRINTER_INFO_2A lpPi = &(PrintStructures->lpPrinterInfo
                                             [PrintStructures->CurrentPrinter]);
 LPPRINTDLGA       lppd = PrintStructures->lpPrintDlg;
 PDEVMODEA         pDevMode;
 
 if (hDlg!=0)
 {
    /* check whether nFromPage and nToPage are within range defined by
     * nMinPage and nMaxPage
     */
    if (IsDlgButtonChecked(hDlg, rad3) == BST_CHECKED)
    {
        WORD nToPage;
        WORD nFromPage;
     	nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
        nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
        if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
            nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage)
        {
	    char resourcestr[256];
            char resultstr[256];
            LoadStringA(COMDLG32_hInstance, PD32_INVALID_PAGE_RANGE, 
                resourcestr, 255);
            sprintf(resultstr,resourcestr, lppd->nMinPage, lppd->nMaxPage);
            LoadStringA(COMDLG32_hInstance, PD32_PRINT_TITLE, 
                resourcestr, 255);
            MessageBoxA(hDlg, resultstr, resourcestr, MB_OK | MB_ICONWARNING);
            return(FALSE);
        }
        lppd->nFromPage = nFromPage;
        lppd->nToPage   = nToPage;
    }
     
    
    if (IsDlgButtonChecked(hDlg, chx1) == BST_CHECKED)
       {
        lppd->Flags |= PD_PRINTTOFILE;
        lpPi->pPortName = "FILE:";
       }

    if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
       {
        FIXME("Collate lppd not yet implemented as output\n");
       }
 } /* end-of-if(hDlg!=0) */

    /* 
     * create or modify hDevMode 
     */     
    if (lppd->hDevMode == 0)
       {
        TRACE(" No hDevMode yet... Need to create my own\n");
        /* FIXME: possible memory leak? Memory never freed again! */
        lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE, lpPi->pDevMode->dmSize);
        pDevMode    = GlobalLock(lppd->hDevMode);
        memcpy(pDevMode, lpPi->pDevMode, lpPi->pDevMode->dmSize);
       }
    else
       {
        FIXME(" already hDevMode... must adjust it... Not implemented yet\n");
        pDevMode    = GlobalLock(lppd->hDevMode);
       }
              
    /* If hDevNames already exists, trash it.
     * But create a new one anyway
     */
    if (lppd->hDevNames != 0)
       {
        if ( (GlobalFlags(lppd->hDevNames)&0xFF) != 0)
	    ERR(" Tried to free hDevNames, but your application still has a"
	    	" lock on hDevNames. Possible program crash...");
        GlobalFree(lppd->hDevNames);
       }
    /* FIXME: The first entry  of DevNames is fixed to "winspool", 
     * because I don't know of any printerdriver which doesn't return 
     * winspool there. But I guess they do exist... 
     */
    lppd->hDevNames = PRINTDLG_CreateDevNames("winspool",
                      lpPi->pDriverName, lpPi->pPortName, 
		      (PrintStructures->DefaultPrinter ==
		                PrintStructures->CurrentPrinter)?1:0);

    /* set PD_Collate and nCopies */
    if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE ||
        lppd->Flags & PD_USEDEVMODECOPIES)
        {
	 /* if one of the above flags was set, the application doesn't
	   * (want to) support multiple copies or collate...
	   */
         lppd->Flags &= ~PD_COLLATE;
         lppd->nCopies = 1;
	  /* if the printer driver supports it... store info there
	   * otherwise no collate & multiple copies !
	   */
         if (pDevMode->dmFields & DM_COLLATE)
          {
           pDevMode->dmCollate = 0;
           if (hDlg!=0)
             if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
	    	   pDevMode->dmCollate = 1;	 
          }
         if (pDevMode->dmFields & DM_COPIES)
          {
           pDevMode->dmCopies = 1;
           if (hDlg!=0)
                pDevMode->dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	      }
	    }
    else
        {
         if (hDlg!=0)
           {
	        /* set Collate & nCopies according to dialog */
            if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
               lppd->Flags |= PD_COLLATE;
            else
               lppd->Flags &= ~PD_COLLATE;
            lppd->nCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
           }
        else
           {
            /* stick to defaults */
            lppd->Flags &= ~PD_COLLATE;
            lppd->nCopies = 1;
           }
       }


    GlobalUnlock(lppd->hDevMode);

 return(TRUE);   
}


    
/***********************************************************************
 *      PRINTSETUP32DLG_ValidateAndDuplicateSettings          [internal]
 *
 *
 *   updates the PrintDlg structure for returnvalues.
 *   (yep, the name was chosen a bit stupid...)
 *
 *   if hDlg equals zero, only hDevModes and hDevNames are adapted.
 *      
 * RETURNS
 *   FALSE if user is not allowed to close (i.e. wrong nTo or nFrom values)
 *   TRUE  if succesful.
 */
static BOOL PRINTSETUP32DLG_ValidateAndDuplicateSettings(HWND hDlg, 
						  PRINT_PTRA* PrintStructures)
{
 LPPRINTDLGA       lppd = PrintStructures->lpPrintDlg;
 LPPRINTER_INFO_2A lppi = &(PrintStructures->lpPrinterInfo
                                             [PrintStructures->CurrentPrinter]);
 PDEVMODEA         pDevMode;

   if (PRINTDLG_ValidateAndDuplicateSettings(0, PrintStructures)==FALSE)
      return(FALSE);

   pDevMode    = GlobalLock(lppd->hDevMode);

   /* set bin type and paper size to DevMode */
   if (pDevMode->dmFields & DM_PAPERSIZE)
     {
      pDevMode->u1.s1.dmPaperSize = PRINTSETUP32DLG_UpdateComboBox(hDlg, cmb2,
                            lppi->pPrinterName,
                            lppi->pPortName);
      /* FIXME: should set dmPaperLength and dmPaperWidth also??? */
     }                      
   if (pDevMode->dmFields & DM_DEFAULTSOURCE)
      pDevMode->dmDefaultSource = PRINTSETUP32DLG_UpdateComboBox(hDlg, cmb3,
                            lppi->pPrinterName,
                            lppi->pPortName);
 
   /* set paper orientation to DevMode */
   if (pDevMode->dmFields & DM_ORIENTATION)
      {
       if (IsDlgButtonChecked(hDlg, rad1) == BST_CHECKED)
           pDevMode->u1.s1.dmOrientation = DMORIENT_PORTRAIT;
       else
           pDevMode->u1.s1.dmOrientation = DMORIENT_LANDSCAPE;
      }
      
   GlobalUnlock(lppd->hDevMode);
   
 return(TRUE);
}



/***********************************************************************
 *                              PRINTDLG_WMCommand               [internal]
 */
static LRESULT PRINTDLG_WMCommand(HWND hDlg, WPARAM wParam, 
			LPARAM lParam, PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    LPPRINTER_INFO_2A lppi = &(PrintStructures->lpPrinterInfo
                                        [PrintStructures->CurrentPrinter]);
    

    switch (LOWORD(wParam)) 
    {
	 case IDOK:
        TRACE(" OK button was hit\n");
        if (PRINTDLG_ValidateAndDuplicateSettings(hDlg, PrintStructures)!=TRUE)
        	return(FALSE);
	    DestroyWindow(hDlg);
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
        if (HIWORD(wParam)==EN_CHANGE)
           {
            WORD nToPage;
	        WORD nFromPage;
	    	nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	        nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
            if (nFromPage != lppd->nFromPage || nToPage != lppd->nToPage)
			    CheckRadioButton(hDlg, rad1, rad3, rad3);
    }
        break;
     case psh2:                         /* Properties button */
        {
/*         HANDLE hPrinter;
         char   PrinterName[256];
         GetDlgItemTextA(hDlg, cmb4, PrinterName, 255);
         if (OpenPrinterA(PrinterName, &hPrinter, NULL))
            {
             PrinterProperties(hDlg, hPrinter);
             ClosePrinter(hPrinter);
            }
         else
            WARN(" Call to OpenPrinter did not succeed!\n");
         break;
*/       MessageBoxA(hDlg, "Not implemented yet!", "PRINT", MB_OK);
        }
     case cmb4:                         /* Printer combobox */
        if (HIWORD(wParam)==CBN_SELCHANGE)
           {
            int i;
        	char   PrinterName[256];

            /* look the newly selected Printer up in 
             * our array Printer_Info2As
             */
        	GetDlgItemTextA(hDlg, cmb4, PrinterName, 255);
            for (i=0; i < PrintStructures->NrOfPrinterInfoEntries; i++)
           	   {
                if (strcmp(PrintStructures->lpPrinterInfo[i].pPrinterName, 
                           PrinterName)==0)
                     break;
               }
            PrintStructures->CurrentPrinter = i;   
	    PRINTDLG_UpdatePrinterInfoTexts(hDlg, PrintStructures);
	    lppi = &(PrintStructures->lpPrinterInfo
                                       [PrintStructures->CurrentPrinter]);
            if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE ||
                lppd->Flags & PD_USEDEVMODECOPIES)
        	{
	         /* if printer doesn't support it: no nCopies */
	 	 if (!(lppi->pDevMode->dmFields & DM_COPIES))
		    {
			EnableWindow(GetDlgItem(hDlg, edt3), FALSE);    
			EnableWindow(GetDlgItem(hDlg, stc5), FALSE);    
		    }
		 else
		    {
			EnableWindow(GetDlgItem(hDlg, edt3), TRUE);    
			EnableWindow(GetDlgItem(hDlg, stc5), TRUE);    
		    }
	         /* if printer doesn't support it: no Collate */
	 	 if (!(lppi->pDevMode->dmFields & DM_COPIES))
		    {
			EnableWindow(GetDlgItem(hDlg, ico3), FALSE);    
			EnableWindow(GetDlgItem(hDlg, chx2), FALSE);    
		    }
		 else
		    {
			EnableWindow(GetDlgItem(hDlg, ico3), TRUE);
			EnableWindow(GetDlgItem(hDlg, chx2), TRUE);
		    }
		}

           }
        break;
    }
    return FALSE;
}    


/***********************************************************************
 *                  PRINTSETUP32DLG_WMCommand               [internal]
 */
static LRESULT PRINTSETUP32DLG_WMCommand(HWND hDlg, WPARAM wParam, 
			LPARAM lParam, PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    LPPRINTER_INFO_2A lppi = &(PrintStructures->lpPrinterInfo
                                        [PrintStructures->CurrentPrinter]);
    

    switch (LOWORD(wParam)) 
    {
     case IDOK:
        TRACE(" OK button was hit\n");
        if (PRINTSETUP32DLG_ValidateAndDuplicateSettings(hDlg, PrintStructures) != TRUE)
        	return(FALSE);
        DestroyWindow(hDlg);
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
     case psh2:                         /* Properties button */
       MessageBoxA(hDlg, "Not implemented yet!", "PRINT SETUP", MB_OK);
       break;
     case cmb1:                         /* Printer combobox */
        if (HIWORD(wParam)==CBN_SELCHANGE)
           {
            int i;
        	char   Name[256];

            /* look the newly selected Printer up in 
             * our array Printer_Info2As
             */
        	GetDlgItemTextA(hDlg, cmb1, Name, 255);
            for (i=0; i < PrintStructures->NrOfPrinterInfoEntries; i++)
           	   {
                if (strcmp(PrintStructures->lpPrinterInfo[i].pPrinterName, 
                           Name)==0)
                     break;
               }
            PrintStructures->CurrentPrinter = i;   
            PRINTDLG_UpdatePrinterInfoTexts(hDlg, PrintStructures);
            lppi = &(PrintStructures->
                        lpPrinterInfo[PrintStructures->CurrentPrinter]);
                    
            /* Update both ComboBoxes to the items available for the new
             * printer. Keep the same entry selected, if possible
             */
            PRINTSETUP32DLG_UpdateComboBox(hDlg, cmb2, lppi->pPrinterName,
                                             lppi->pPortName);
            PRINTSETUP32DLG_UpdateComboBox(hDlg, cmb3, lppi->pPrinterName,
                                             lppi->pPortName);
           }
        break;
    }
    return FALSE;
}    


/***********************************************************************
 *           PrintDlgProcA			[internal]
 */
LRESULT WINAPI PrintDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
  PRINT_PTRA* PrintStructures;
  LRESULT res=FALSE;
  if (uMsg!=WM_INITDIALOG)
  {
   PrintStructures = (PRINT_PTRA*) GetWindowLongA(hDlg, DWL_USER);   
   if (!PrintStructures)
    return FALSE;
}
  else
  {
    PrintStructures=(PRINT_PTRA*) lParam;
    if (!PRINTDLG_WMInitDialog(hDlg, wParam, lParam, PrintStructures)) 
    {
      TRACE("PRINTDLG_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
  }
  switch (uMsg)
  {
  	case WM_COMMAND:
       return PRINTDLG_WMCommand(hDlg, wParam, lParam, PrintStructures);
    case WM_DESTROY:
	    return FALSE;
  }
  
 return res;
}




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
 *           PrintSetupDlgProc   (COMMDLG.22)
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
 *           PrintSetupDlgProcA			[???]
 *
 *   FIXME:
 *   note: I don't know whether this function actually is allowed
 *         to exist (i.e. is exported/overrideable from the DLL)
 *         For now, this function is local only.
 *         If necessary, this call can be merged with PrintDlgProcA,
 *         as it is very similar.
 */
LRESULT WINAPI PrintSetupDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
  PRINT_PTRA* PrintStructures;
  LRESULT res=FALSE;
  if (uMsg!=WM_INITDIALOG)
  {
   PrintStructures = (PRINT_PTRA*) GetWindowLongA(hDlg, DWL_USER);   
   if (!PrintStructures)
    return FALSE;
}
  else
  {
    PrintStructures=(PRINT_PTRA*) lParam;
    if (!PRINTSETUP32DLG_WMInitDialog(hDlg, wParam, lParam, PrintStructures)) 
    {
      TRACE("PRINTSETUP32DLG_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
  }
  switch (uMsg)
  {
  	case WM_COMMAND:
       return PRINTSETUP32DLG_WMCommand(hDlg, wParam, lParam, PrintStructures);
    case WM_DESTROY:
	    return FALSE;
  }
  
 return res;
}





/***********************************************************************
 *            PageSetupDlgA  (COMDLG32.15)
 */
BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg) {
	FIXME("(%p), stub!\n",setupdlg);
	return FALSE;
}

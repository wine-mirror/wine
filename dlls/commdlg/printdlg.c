/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "ldt.h"
#include "commdlg.h"
#include "resource.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "debugtools.h"
#include "winproc.h"
#include "cderr.h"
#include "winspool.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"

/***********************************************************************
 *           PrintDlg16   (COMMDLG.20)
 */
BOOL16 WINAPI PrintDlg16( SEGPTR printdlg )
{
    HANDLE16 hInst;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    HWND hwndDialog;
    HANDLE hResInfo, hDlgTmpl;
    LPSTR rscname;
    LPPRINTDLG16 lpPrint = (LPPRINTDLG16)PTR_SEG_TO_LIN(printdlg);

    TRACE("(%p) -- Flags=%08lX\n", lpPrint, lpPrint->Flags );

    if (lpPrint->Flags & PD_RETURNDEFAULT)
        /* FIXME: should fill lpPrint->hDevMode and lpPrint->hDevNames here */
        return TRUE;

    if (lpPrint->Flags & PD_PRINTSETUP)
        rscname = "PRINT_SETUP";
    else
        rscname = "PRINT";

    if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, rscname, RT_DIALOGA)))
    {
	COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	return FALSE;
    }
    if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
        !(template = LockResource( hDlgTmpl )))
    {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }

    hInst = GetWindowLongA( lpPrint->hwndOwner, GWL_HINSTANCE );
    hwndDialog = DIALOG_CreateIndirect( hInst, template, TRUE,
                                        lpPrint->hwndOwner,
                               (DLGPROC16)((lpPrint->Flags & PD_PRINTSETUP) ?
                                MODULE_GetWndProcEntry16("PrintSetupDlgProc") :
                                MODULE_GetWndProcEntry16("PrintDlgProc")),
                                printdlg, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpPrint->hwndOwner);
    return bRet;
}


/* This PRINTDLGA internal structure stores
 * pointers to several throughout useful structures.
 * 
 */
typedef struct  
{
  LPPRINTER_INFO_2A lpPrinterInfo;
  DWORD             NrOfPrinterInfoEntries;
  LPPRINTDLGA       lpPrintDlg;
  UINT              HelpMessageID;
} PRINT_PTRA;

/***********************************************************************
 *           PrintDlgA   (COMDLG32.17)
 *
 *  Displays the the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *
 *  (Note: according to the MS Platform SDK, this call was in the past 
 *   also used to display some PRINT SETUP dialog. As this is superseded 
 *   by PageSetupDlg, this now results in an error!)
 *
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  The function is a stub only, returning TRUE to allow more programs
 *  to function.
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
 * current implementation is in step 2, nearly going to 3.
     */ 

    HWND      hwndDialog;
    BOOL      bRet = FALSE;
    LPCVOID   ptr;
    HANDLE    hResInfo, hDlgTmpl;
    HINSTANCE hInst = GetWindowLongA( lppd->hwndOwner, GWL_HINSTANCE );
    DWORD     EnumBytesNeeded;
    DWORD     CopyOfEnumBytesNeeded;
    PRINT_PTRA PrintStructures;

    FIXME("KVG (%p): stub\n", lppd);
    PrintStructures.lpPrintDlg = lppd;

    if (!(hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32", RT_DIALOGA)))
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

    /*
     * if lppd->Flags PD_SHOWHELP is specified, a HELPMESGSTRING message
     * must be registered and the Help button must be shown.
     */
    if (lppd->Flags & PD_SHOWHELP)
       {
        if((PrintStructures.HelpMessageID = RegisterWindowMessageA(HELPMSGSTRING)) 
    			== 0)
            {
             COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
             return FALSE;
            }
       }
    else
    	PrintStructures.HelpMessageID=0;
	
	/*
	 * if lppd->Flags PD_RETURNDEFAULT is specified, the PrintDlg function
	 * does not display the dialog box, but returns with valid entries
	 * for hDevMode and hDevNames .
	 * 
	 * Currently the flag is recognised, but we return empty hDevMode and
	 * and hDevNames. This will be fixed when I am in step 3.
	 */
	if (lppd->Flags & PD_RETURNDEFAULT)
	   {
	    WARN(": PrintDlg was requested to return printer info only."
					  "\n The return value currently does NOT provide these.\n");
		COMDLG32_SetCommDlgExtendedError(PDERR_NODEVICES); 
        								/* return TRUE, thus never checked! */
	    return(TRUE);
	   }
	   
	if (lppd->Flags & PD_PRINTSETUP)
		{
		 FIXME(": PrintDlg was requested to display PrintSetup box.\n");
		 COMDLG32_SetCommDlgExtendedError(PDERR_INITFAILURE); 
		 return(FALSE);
		}
		
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
     */
    /* FIXME: Currently Unimplemented */
    if (lppd->Flags & PD_NOWARNING)	
	   {
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
	
		
    hwndDialog= DIALOG_CreateIndirect(hInst, ptr, TRUE, lppd->hwndOwner,
            (DLGPROC16)PrintDlgProcA, (LPARAM)&PrintStructures, WIN_PROC_32A );
    if (hwndDialog) 
        bRet = DIALOG_DoDialogBox(hwndDialog, lppd->hwndOwner);  
        
    free(PrintStructures.lpPrinterInfo);
        
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
	char   PrinterName[256];
    char   StatusMsg[256];
    int    i;
    LPPRINTER_INFO_2A lpPi = NULL;
	GetDlgItemTextA(hDlg, cmb4, PrinterName, 255);
             
    /* look the selected PrinterName up in our array Printer_Info2As*/
    for (i=0; i < PrintStructures->NrOfPrinterInfoEntries; i++)
    	{
         lpPi = &PrintStructures->lpPrinterInfo[i];
         if (strcmp(lpPi->pPrinterName, PrinterName)==0)
         break;
        }
    /* FIXME: the status byte must be converted to user-understandable text...*/
    sprintf(StatusMsg,"%ld = 0x%08lx", lpPi->Status, lpPi->Status);
    SendDlgItemMessageA(hDlg, stc12, WM_SETTEXT, 0, (LPARAM)StatusMsg);
    SendDlgItemMessageA(hDlg, stc11, WM_SETTEXT, 0, (LPARAM)lpPi->pDriverName);
    if (lpPi->pLocation != NULL && lpPi->pLocation[0]!='\0')
        SendDlgItemMessageA(hDlg, stc14, WM_SETTEXT, 0,(LPARAM)lpPi->pLocation);
    else                                        
        SendDlgItemMessageA(hDlg, stc14, WM_SETTEXT, 0, (LPARAM)lpPi->pPortName);
    SendDlgItemMessageA(hDlg, stc13, WM_SETTEXT, 0, (LPARAM)lpPi->pComment);
}


/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         PRINT_PTRA* PrintStructures)
{
 int         i;
 LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
 LPPRINTER_INFO_2A lppi = PrintStructures->lpPrinterInfo;
 
	SetWindowLongA(hDlg, DWL_USER, lParam); 
	TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);

	if (lppd->lStructSize != sizeof(PRINTDLGA))
	{
		FIXME("structure size failure !!!\n");
/*		EndDialog (hDlg, 0); 
		return FALSE; 
*/	}

/* Fill Combobox according to info from PRINTER_INFO2A
 * structure inside PrintStructures and generate an
 * update-message to have the rest of the dialog box updated.
 */ 
    for (i=0; i < PrintStructures->NrOfPrinterInfoEntries; i++)
	   SendDlgItemMessageA(hDlg, cmb4, CB_ADDSTRING, 0,
                        (LPARAM)lppi[i].pPrinterName );  
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
	/* FIXME: in Win95, the radiobutton "All" is displayed as
	 * "Print all xxx pages"... This is not done here (yet?)
	 */
        
	/* Collate pages */
    if (lppd->Flags & PD_COLLATE)
    	FIXME("PD_COLLATE not implemented yet\n");
        
    /* print to file */
   	CheckDlgButton(hDlg, chx1, (lppd->Flags & PD_PRINTTOFILE) ? 1 : 0);
    if (lppd->Flags & PD_DISABLEPRINTTOFILE)
		EnableWindow(GetDlgItem(hDlg, chx1), FALSE);    
    if (lppd->Flags & PD_HIDEPRINTTOFILE)
		ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);
    
    /* status */

    /* help button */
	if ((lppd->Flags & PD_SHOWHELP)==0)
    	{	/* hide if PD_SHOWHELP not specified */
		 ShowWindow(GetDlgItem(hDlg, pshHelp), SW_HIDE);         
        }

TRACE("succesful!\n");
  return TRUE;
}


/***********************************************************************
 *             PRINTDLG_ValidateAndDuplicateSettings          [internal]
 *
 *
 *   updates the PrintDlg structure for returnvalues.
 * RETURNS
 *   FALSE if user is not allowed to close (i.e. wrong nTo or nFrom values)
 *   TRUE  if succesful.
 */
BOOL PRINTDLG_ValidateAndDuplicateSettings(HWND hDlg, 
                                           PRINT_PTRA* PrintStructures)
{
 LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
 
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
			char TempBuffer[256];
            FIXME("This MessageBox is not internationalised.");
         sprintf(TempBuffer, "This value lies not within Page range\n"
                             "Please enter a value between %d and %d",
                             lppd->nMinPage, lppd->nMaxPage);
         MessageBoxA(hDlg, TempBuffer, "Print", MB_OK | MB_ICONWARNING);
         return(FALSE);
        }
        lppd->nFromPage = nFromPage;
        lppd->nToPage   = nToPage;
    }
     
    if (IsDlgButtonChecked(hDlg, chx1) == BST_CHECKED)
       {
        lppd->Flags |= PD_PRINTTOFILE;
        /* FIXME: insert code to set "FILE:" in DEVNAMES structure */
}

 return(TRUE);   
}


/***********************************************************************
 *                              PRINTDLG_WMCommand               [internal]
 */
static LRESULT PRINTDLG_WMCommand(HWND hDlg, WPARAM wParam, 
			LPARAM lParam, PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;

    switch (LOWORD(wParam)) 
    {
	 case IDOK:
        TRACE(" OK button was hit\n");
        if (PRINTDLG_ValidateAndDuplicateSettings(hDlg, PrintStructures) != TRUE)
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
     case edt1:							/* from page nr editbox */
     case edt2:							/* to page nr editbox */
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
     case psh2:							/* Properties button */
        {
         HANDLE hPrinter;
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
        }
     case cmb4:							/* Printer combobox */
        if (HIWORD(wParam)==CBN_SELCHANGE)
			PRINTDLG_UpdatePrinterInfoTexts(hDlg, PrintStructures);
        break;
/*     default:
        printf("wParam: 0x%x  ",wParam);
        break;
*/    }
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
    MessageBoxA(hDlg,"Warning: this dialog has no functionality yet!", 
    			NULL, MB_OK);
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
 *            PageSetupDlgA  (COMDLG32.15)
 */
BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg) {
	FIXME("(%p), stub!\n",setupdlg);
	return FALSE;
}

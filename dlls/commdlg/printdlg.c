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
#include "win.h"
#include "message.h"
#include "commdlg.h"
#include "resource.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "debug.h"
#include "winproc.h"
#include "cderr.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

/***********************************************************************
 *           PrintDlg16   (COMMDLG.20)
 */
BOOL16 WINAPI PrintDlg16( SEGPTR printdlg )
{
    HANDLE16 hInst;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    HWND hwndDialog;
    LPPRINTDLG16 lpPrint = (LPPRINTDLG16)PTR_SEG_TO_LIN(printdlg);

    TRACE(commdlg,"(%p) -- Flags=%08lX\n", lpPrint, lpPrint->Flags );

    if (lpPrint->Flags & PD_RETURNDEFAULT)
        /* FIXME: should fill lpPrint->hDevMode and lpPrint->hDevNames here */
        return TRUE;

    if (lpPrint->Flags & PD_PRINTSETUP)
	template = SYSRES_GetResPtr( SYSRES_DIALOG_PRINT_SETUP );
    else
	template = SYSRES_GetResPtr( SYSRES_DIALOG_PRINT );

    hInst = WIN_GetWindowInstance( lpPrint->hwndOwner );
    hwndDialog = DIALOG_CreateIndirect( hInst, template, TRUE,
                                        lpPrint->hwndOwner,
                               (DLGPROC16)((lpPrint->Flags & PD_PRINTSETUP) ?
                                MODULE_GetWndProcEntry16("PrintSetupDlgProc") :
                                MODULE_GetWndProcEntry16("PrintDlgProc")),
                                printdlg, WIN_PROC_16 );
    if (hwndDialog) bRet = DIALOG_DoDialogBox( hwndDialog, lpPrint->hwndOwner);
    return bRet;
}



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
 * current implementation is in step 1.
     */ 

    HWND      hwndDialog;
    BOOL      bRet = FALSE;
    LPCVOID   ptr = SYSRES_GetResPtr( SYSRES_DIALOG_PRINT32 );  
  	HINSTANCE hInst = WIN_GetWindowInstance( lppd->hwndOwner );

    FIXME(commdlg, "KVG (%p): stub\n", lppd);

    /*
     * FIXME : Should respond to TEMPLATE and HOOK flags here
     * For now, only the standard dialog works.
     */
    if (lppd->Flags & (PD_ENABLEPRINTHOOK | PD_ENABLEPRINTTEMPLATE |
			  PD_ENABLEPRINTTEMPLATEHANDLE | PD_ENABLESETUPHOOK | 
			  PD_ENABLESETUPTEMPLATE|PD_ENABLESETUPTEMPLATEHANDLE)) 
    	FIXME(commdlg, ": unimplemented flag (ignored)\n");     
	
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
	    WARN(commdlg, ": PrintDlg was requested to return printer info only."
					  "\n The return value currently does NOT provide these.\n");
		COMDLG32_SetCommDlgExtendedError(PDERR_NODEVICES); 
        								/* return TRUE, thus never checked! */
	    return(TRUE);
	   }
	   
	if (lppd->Flags & PD_PRINTSETUP)
		{
		 FIXME(commdlg, ": PrintDlg was requested to display PrintSetup box.\n");
		 COMDLG32_SetCommDlgExtendedError(PDERR_INITFAILURE); 
		 return(FALSE);
		}
		
    hwndDialog= DIALOG_CreateIndirect(hInst, ptr, TRUE, lppd->hwndOwner,
            (DLGPROC16)PrintDlgProcA, (LPARAM)lppd, WIN_PROC_32A );
    if (hwndDialog) 
        bRet = DIALOG_DoDialogBox(hwndDialog, lppd->hwndOwner);  
  return bRet;            
}





/***********************************************************************
 *           PrintDlg32W   (COMDLG32.18)
 */
BOOL WINAPI PrintDlgW( LPPRINTDLGW printdlg )
{
    FIXME(commdlg, "A really empty stub\n" );
    return FALSE;
}



/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPPRINTDLGA lppd)
{
	SetWindowLongA(hDlg, DWL_USER, lParam); 
	TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);

	if (lppd->lStructSize != sizeof(PRINTDLGA))
	{
		FIXME(commdlg,"structure size failure !!!\n");
/*		EndDialog (hDlg, 0); 
		return FALSE; 
*/	}

/* Flag processing to set the according buttons on/off and
 * Initialise the various values
 */

    /* Print range (All/Range/Selection) */
    if (lppd->nMinPage == lppd->nMaxPage) 
    	lppd->Flags &= ~PD_NOPAGENUMS;        
    /* FIXME: I allow more freedom than either Win95 or WinNT,
     *        as officially we should return error if
     *        ToPage or FromPage is out-of-range
     */
    if (lppd->nToPage < lppd->nMinPage)
        lppd->nToPage = lppd->nMinPage;
    if (lppd->nToPage > lppd->nMaxPage)
        lppd->nToPage = lppd->nMaxPage;
    if (lppd->nFromPage < lppd->nMinPage)
        lppd->nFromPage = lppd->nMinPage;
    if (lppd->nFromPage > lppd->nMaxPage)
        lppd->nFromPage = lppd->nMaxPage;
    SetDlgItemInt(hDlg, edt2, lppd->nFromPage, FALSE);
    SetDlgItemInt(hDlg, edt3, lppd->nToPage, FALSE);
    CheckRadioButton(hDlg, rad1, rad3, rad1);
    if (lppd->Flags & PD_NOSELECTION)
		EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
    else
		if (lppd->Flags & PD_SELECTION)
	    	CheckRadioButton(hDlg, rad1, rad3, rad3);
    if (lppd->Flags & PD_NOPAGENUMS)
       {
		EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc10),FALSE);
		EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc11),FALSE);
		EnableWindow(GetDlgItem(hDlg, edt3), FALSE);
       }
    else
       {
		if (lppd->Flags & PD_PAGENUMS)
	    	CheckRadioButton(hDlg, rad1, rad3, rad2);
       }
	/* FIXME: in Win95, the radiobutton "All" is displayed as
	 * "Print all xxx pages"... This is not done here (yet?)
	 */
        
	/* Collate pages */
    if (lppd->Flags & PD_COLLATE)
    	FIXME(commdlg, "PD_COLLATE not implemented yet\n");
        
    /* print to file */
   	CheckDlgButton(hDlg, chx1, (lppd->Flags & PD_PRINTTOFILE) ? 1 : 0);
    if (lppd->Flags & PD_DISABLEPRINTTOFILE)
		EnableWindow(GetDlgItem(hDlg, chx1), FALSE);    
    if (lppd->Flags & PD_HIDEPRINTTOFILE)
		ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);
    
    /* status */

TRACE(commdlg, "succesful!\n");
  return TRUE;
}


/***********************************************************************
 *             PRINTDLG_ValidateAndDuplicateSettings          [internal]
 */
BOOL PRINTDLG_ValidateAndDuplicateSettings(HWND hDlg, LPPRINTDLGA lppd)
{
 WORD nToPage;
 WORD nFromPage;
 char TempBuffer[256];
 
    /* check whether nFromPage and nToPage are within range defined by
     * nMinPage and nMaxPage
     */
    /* FIXMNo checking on rad2 is performed now, because IsDlgButtonCheck
     * currently doesn't seem to know a state BST_CHECKED
     */
/*	if (IsDlgButtonChecked(hDlg, rad2) == BST_CHECKED) */
    {
    	nFromPage = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
        nToPage   = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
        if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
            nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage)
        {
         FIXME(commdlg, "The MessageBox is not internationalised.");
         sprintf(TempBuffer, "This value lies not within Page range\n"
                             "Please enter a value between %d and %d",
                             lppd->nMinPage, lppd->nMaxPage);
         MessageBoxA(hDlg, TempBuffer, "Print", MB_OK | MB_ICONWARNING);
         return(FALSE);
        }
        lppd->nFromPage = nFromPage;
        lppd->nToPage   = nToPage;
    }
     
 return(TRUE);   
}



/***********************************************************************
 *                              PRINTDLG_WMCommand               [internal]
 */
static LRESULT PRINTDLG_WMCommand(HWND hDlg, WPARAM wParam, 
			LPARAM lParam, LPPRINTDLGA lppd)
{
    switch (wParam) 
    {
	 case IDOK:
        if (PRINTDLG_ValidateAndDuplicateSettings(hDlg, lppd) != TRUE)
        	return(FALSE);
        MessageBoxA(hDlg, "OK was hit!", NULL, MB_OK);
	    DestroyWindow(hDlg);
	    return(TRUE);
	 case IDCANCEL:
        MessageBoxA(hDlg, "CANCEL was hit!", NULL, MB_OK);
        EndDialog(hDlg, FALSE);
/*	    DestroyWindow(hDlg); */
	    return(FALSE);
    }
    return FALSE;
}    



/***********************************************************************
 *           PrintDlgProcA			[internal]
 */
LRESULT WINAPI PrintDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
  LPPRINTDLGA lppd;
  LRESULT res=FALSE;
  if (uMsg!=WM_INITDIALOG)
  {
   lppd=(LPPRINTDLGA)GetWindowLongA(hDlg, DWL_USER);   
   if (!lppd)
    return FALSE;
}
  else
  {
    lppd=(LPPRINTDLGA)lParam;
    if (!PRINTDLG_WMInitDialog(hDlg, wParam, lParam, lppd)) 
    {
      TRACE(commdlg, "PRINTDLG_WMInitDialog returned FALSE\n");
      return FALSE;
    }  
    MessageBoxA(hDlg,"Warning: this dialog has no functionality yet!", 
    			NULL, MB_OK);
  }
  switch (uMsg)
  {
  	case WM_COMMAND:
		return PRINTDLG_WMCommand(hDlg, wParam, lParam, lppd);
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
      TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);
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
      TRACE(commdlg,"WM_INITDIALOG lParam=%08lX\n", lParam);
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
	FIXME(commdlg,"(%p), stub!\n",setupdlg);
	return FALSE;
}

/*
 * COMMDLG - Find & Replace Text Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
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

#include "cdlg.h"

/***********************************************************************
 *           FindText16   (COMMDLG.11)
 */
HWND16 WINAPI FindText16( SEGPTR find )
{
    HANDLE16 hInst;
    LPCVOID ptr;
    HANDLE hResInfo, hDlgTmpl;
    LPFINDREPLACE16 lpFind = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, MAKEINTRESOURCEA(FINDDLGORD), RT_DIALOGA)))
    {
	COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	return FALSE;
    }
    if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
        !(ptr = LockResource( hDlgTmpl )))
    {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                        (DLGPROC16)MODULE_GetWndProcEntry16("FindTextDlgProc"),
                                  find, WIN_PROC_16 );
}


/***********************************************************************
 *           ReplaceText16   (COMMDLG.12)
 */
HWND16 WINAPI ReplaceText16( SEGPTR find )
{
    HANDLE16 hInst;
    LPCVOID ptr;
    HANDLE hResInfo, hDlgTmpl;
    LPFINDREPLACE16 lpFind = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);

    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, MAKEINTRESOURCEA(REPLACEDLGORD), RT_DIALOGA)))
    {
	COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	return FALSE;
    }
    if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
        !(ptr = LockResource( hDlgTmpl )))
    {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                     (DLGPROC16)MODULE_GetWndProcEntry16("ReplaceTextDlgProc"),
                                  find, WIN_PROC_16 );
}


/***********************************************************************
 *                              FINDDLG_WMInitDialog            [internal]
 */
static LRESULT FINDDLG_WMInitDialog(HWND hWnd, LPARAM lParam, LPDWORD lpFlags,
                                    LPSTR lpstrFindWhat, BOOL fUnicode)
{
    SetWindowLongA(hWnd, DWL_USER, lParam);
    *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the
     * FindNext (IDOK) button.  Only after typing some text, the button should be
     * enabled.
     */
    if (fUnicode) SetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat);
	else SetDlgItemTextA(hWnd, edt1, lpstrFindWhat);
    CheckRadioButton(hWnd, rad1, rad2, (*lpFlags & FR_DOWN) ? rad2 : rad1);
    if (*lpFlags & (FR_HIDEUPDOWN | FR_NOUPDOWN)) {
	EnableWindow(GetDlgItem(hWnd, rad1), FALSE);
	EnableWindow(GetDlgItem(hWnd, rad2), FALSE);
    }
    if (*lpFlags & FR_HIDEUPDOWN) {
	ShowWindow(GetDlgItem(hWnd, rad1), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, rad2), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, grp1), SW_HIDE);
    }
    CheckDlgButton(hWnd, chx1, (*lpFlags & FR_WHOLEWORD) ? 1 : 0);
    if (*lpFlags & (FR_HIDEWHOLEWORD | FR_NOWHOLEWORD))
	EnableWindow(GetDlgItem(hWnd, chx1), FALSE);
    if (*lpFlags & FR_HIDEWHOLEWORD)
	ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
    CheckDlgButton(hWnd, chx2, (*lpFlags & FR_MATCHCASE) ? 1 : 0);
    if (*lpFlags & (FR_HIDEMATCHCASE | FR_NOMATCHCASE))
	EnableWindow(GetDlgItem(hWnd, chx2), FALSE);
    if (*lpFlags & FR_HIDEMATCHCASE)
	ShowWindow(GetDlgItem(hWnd, chx2), SW_HIDE);
    if (!(*lpFlags & FR_SHOWHELP)) {
	EnableWindow(GetDlgItem(hWnd, pshHelp), FALSE);
	ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
}    


/***********************************************************************
 *                              FINDDLG_WMCommand               [internal]
 */
static LRESULT FINDDLG_WMCommand(HWND hWnd, WPARAM wParam, 
			HWND hwndOwner, LPDWORD lpFlags,
			LPSTR lpstrFindWhat, WORD wFindWhatLen, 
			BOOL fUnicode)
{
    int uFindReplaceMessage = RegisterWindowMessageA( FINDMSGSTRING );
    int uHelpMessage = RegisterWindowMessageA( HELPMSGSTRING );

    switch (wParam) {
	case IDOK:
	    if (fUnicode) 
	      GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
	      else GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
	    if (IsDlgButtonChecked(hWnd, rad2))
		*lpFlags |= FR_DOWN;
		else *lpFlags &= ~FR_DOWN;
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_FINDNEXT;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case IDCANCEL:
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    *lpFlags |= FR_DIALOGTERM;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    DestroyWindow(hWnd);
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessageA(hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}    


/***********************************************************************
 *           FindTextDlgProc16   (COMMDLG.13)
 */
LRESULT WINAPI FindTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                                 LPARAM lParam)
{
    LPFINDREPLACE16 lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(lParam);
	    return FINDDLG_WMInitDialog(hWnd, lParam, &(lpfr->Flags),
		PTR_SEG_TO_LIN(lpfr->lpstrFindWhat), FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
	    return FINDDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner,
		&lpfr->Flags, PTR_SEG_TO_LIN(lpfr->lpstrFindWhat),
		lpfr->wFindWhatLen, FALSE);
    }
    return FALSE;
}


/***********************************************************************
 *                              REPLACEDLG_WMInitDialog         [internal]
 */
static LRESULT REPLACEDLG_WMInitDialog(HWND hWnd, LPARAM lParam,
		    LPDWORD lpFlags, LPSTR lpstrFindWhat, 
		    LPSTR lpstrReplaceWith, BOOL fUnicode)
{
    SetWindowLongA(hWnd, DWL_USER, lParam);
    *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the FinNext /
     * Replace / ReplaceAll buttons.  Only after typing some text, the buttons should be
     * enabled.
     */
    if (fUnicode)     
    {
	SetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat);
	SetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith);
    } else
    {
	SetDlgItemTextA(hWnd, edt1, lpstrFindWhat);
	SetDlgItemTextA(hWnd, edt2, lpstrReplaceWith);
    }
    CheckDlgButton(hWnd, chx1, (*lpFlags & FR_WHOLEWORD) ? 1 : 0);
    if (*lpFlags & (FR_HIDEWHOLEWORD | FR_NOWHOLEWORD))
	EnableWindow(GetDlgItem(hWnd, chx1), FALSE);
    if (*lpFlags & FR_HIDEWHOLEWORD)
	ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
    CheckDlgButton(hWnd, chx2, (*lpFlags & FR_MATCHCASE) ? 1 : 0);
    if (*lpFlags & (FR_HIDEMATCHCASE | FR_NOMATCHCASE))
	EnableWindow(GetDlgItem(hWnd, chx2), FALSE);
    if (*lpFlags & FR_HIDEMATCHCASE)
	ShowWindow(GetDlgItem(hWnd, chx2), SW_HIDE);
    if (!(*lpFlags & FR_SHOWHELP)) {
	EnableWindow(GetDlgItem(hWnd, pshHelp), FALSE);
	ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
    }
    ShowWindow(hWnd, SW_SHOWNORMAL);
    return TRUE;
}    


/***********************************************************************
 *                              REPLACEDLG_WMCommand            [internal]
 */
static LRESULT REPLACEDLG_WMCommand(HWND hWnd, WPARAM16 wParam,
		    HWND hwndOwner, LPDWORD lpFlags,
		    LPSTR lpstrFindWhat, WORD wFindWhatLen,
		    LPSTR lpstrReplaceWith, WORD wReplaceWithLen,
		    BOOL fUnicode)
{
    int uFindReplaceMessage = RegisterWindowMessageA( FINDMSGSTRING );
    int uHelpMessage = RegisterWindowMessageA( HELPMSGSTRING );

    switch (wParam) {
	case IDOK:
	    if (fUnicode)
	    {
		GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
		GetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith, wReplaceWithLen/2);
	    }  else
	    {
		GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
		GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    }
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_FINDNEXT;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case IDCANCEL:
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    *lpFlags |= FR_DIALOGTERM;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    DestroyWindow(hWnd);
	    return TRUE;
	case psh1:
	    if (fUnicode)
	    {
		GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
		GetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith, wReplaceWithLen/2);
	    }  else
	    {	
		GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
		GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    }
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_REPLACE;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case psh2:
	    if (fUnicode)
	    {
		GetDlgItemTextW(hWnd, edt1, (LPWSTR)lpstrFindWhat, wFindWhatLen/2);
		GetDlgItemTextW(hWnd, edt2, (LPWSTR)lpstrReplaceWith, wReplaceWithLen/2);
	    }  else
	    {
		GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
		GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    }
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD; 
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE; 
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_DIALOGTERM);
	    *lpFlags |= FR_REPLACEALL;
	    SendMessageA(hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongA(hWnd, DWL_USER) );
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessageA(hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}    


/***********************************************************************
 *           ReplaceTextDlgProc16   (COMMDLG.14)
 */
LRESULT WINAPI ReplaceTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                                    LPARAM lParam)
{
    LPFINDREPLACE16 lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(lParam);
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    PTR_SEG_TO_LIN(lpfr->lpstrFindWhat),
		    PTR_SEG_TO_LIN(lpfr->lpstrReplaceWith), FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACE16)PTR_SEG_TO_LIN(GetWindowLongA(hWnd, DWL_USER));
	    return REPLACEDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner, 
		    &lpfr->Flags, PTR_SEG_TO_LIN(lpfr->lpstrFindWhat),
		    lpfr->wFindWhatLen, PTR_SEG_TO_LIN(lpfr->lpstrReplaceWith),
		    lpfr->wReplaceWithLen, FALSE);
    }
    return FALSE;
}


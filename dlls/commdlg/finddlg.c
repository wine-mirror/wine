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


/***********************************************************************
 *           FindText16   (COMMDLG.11)
 */
HWND16 WINAPI FindText16( SEGPTR find )
{
    HANDLE16 hInst;
    LPCVOID ptr;
    LPFINDREPLACE16 lpFind = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                        (DLGPROC16)MODULE_GetWndProcEntry16("FindTextDlgProc"),
                                  find, WIN_PROC_16 );
}

/***********************************************************************
 *           FindText32A   (COMMDLG.6)
 */
HWND WINAPI FindTextA( LPFINDREPLACEA lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                (DLGPROC16)FindTextDlgProcA, (LPARAM)lpFind, WIN_PROC_32A );
}

/***********************************************************************
 *           FindText32W   (COMMDLG.7)
 */
HWND WINAPI FindTextW( LPFINDREPLACEW lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_FIND_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                (DLGPROC16)FindTextDlgProcW, (LPARAM)lpFind, WIN_PROC_32W );
}

/***********************************************************************
 *           ReplaceText16   (COMMDLG.12)
 */
HWND16 WINAPI ReplaceText16( SEGPTR find )
{
    HANDLE16 hInst;
    LPCVOID ptr;
    LPFINDREPLACE16 lpFind = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);

    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
                     (DLGPROC16)MODULE_GetWndProcEntry16("ReplaceTextDlgProc"),
                                  find, WIN_PROC_16 );
}

/***********************************************************************
 *           ReplaceText32A   (COMDLG32.19)
 */
HWND WINAPI ReplaceTextA( LPFINDREPLACEA lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : Should respond to FR_ENABLETEMPLATE and FR_ENABLEHOOK here
     * For now, only the standard dialog works.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE |
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");     
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
		(DLGPROC16)ReplaceTextDlgProcA, (LPARAM)lpFind, WIN_PROC_32A );
}

/***********************************************************************
 *           ReplaceText32W   (COMDLG32.20)
 */
HWND WINAPI ReplaceTextW( LPFINDREPLACEW lpFind )
{
    HANDLE16 hInst;
    LPCVOID ptr;

    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    if (lpFind->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | 
	FR_ENABLEHOOK)) FIXME(commdlg, ": unimplemented flag (ignored)\n");
    ptr = SYSRES_GetResPtr( SYSRES_DIALOG_REPLACE_TEXT );
    hInst = WIN_GetWindowInstance( lpFind->hwndOwner );
    return DIALOG_CreateIndirect( hInst, ptr, TRUE, lpFind->hwndOwner,
		(DLGPROC16)ReplaceTextDlgProcW, (LPARAM)lpFind, WIN_PROC_32W );
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
 *           FindTextDlgProc32A
 */
LRESULT WINAPI FindTextDlgProcA(HWND hWnd, UINT wMsg, WPARAM wParam,
                                 LPARAM lParam)
{
    LPFINDREPLACEA lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
	    lpfr=(LPFINDREPLACEA)lParam;
	    return FINDDLG_WMInitDialog(hWnd, lParam, &(lpfr->Flags),
	      lpfr->lpstrFindWhat, FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEA)GetWindowLongA(hWnd, DWL_USER);
	    return FINDDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner,
		&lpfr->Flags, lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		FALSE);
    }
    return FALSE;
}

/***********************************************************************
 *           FindTextDlgProc32W
 */
LRESULT WINAPI FindTextDlgProcW(HWND hWnd, UINT wMsg, WPARAM wParam,
                                 LPARAM lParam)
{
    LPFINDREPLACEW lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
	    lpfr=(LPFINDREPLACEW)lParam;
	    return FINDDLG_WMInitDialog(hWnd, lParam, &(lpfr->Flags),
	      (LPSTR)lpfr->lpstrFindWhat, TRUE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEW)GetWindowLongA(hWnd, DWL_USER);
	    return FINDDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner,
		&lpfr->Flags, (LPSTR)lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		TRUE);
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

/***********************************************************************
 *           ReplaceTextDlgProc32A
 */ 
LRESULT WINAPI ReplaceTextDlgProcA(HWND hWnd, UINT wMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    LPFINDREPLACEA lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACEA)lParam;
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    lpfr->lpstrFindWhat, lpfr->lpstrReplaceWith, FALSE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEA)GetWindowLongA(hWnd, DWL_USER);
	    return REPLACEDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner, 
		    &lpfr->Flags, lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		    lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen, FALSE);
    }
    return FALSE;
}

/***********************************************************************
 *           ReplaceTextDlgProc32W
 */ 
LRESULT WINAPI ReplaceTextDlgProcW(HWND hWnd, UINT wMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    LPFINDREPLACEW lpfr;
    switch (wMsg) {
	case WM_INITDIALOG:
            lpfr=(LPFINDREPLACEW)lParam;
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    (LPSTR)lpfr->lpstrFindWhat, (LPSTR)lpfr->lpstrReplaceWith,
		    TRUE);
	case WM_COMMAND:
	    lpfr=(LPFINDREPLACEW)GetWindowLongA(hWnd, DWL_USER);
	    return REPLACEDLG_WMCommand(hWnd, wParam, lpfr->hwndOwner, 
		    &lpfr->Flags, (LPSTR)lpfr->lpstrFindWhat, lpfr->wFindWhatLen,
		    (LPSTR)lpfr->lpstrReplaceWith, lpfr->wReplaceWithLen, TRUE);
    }
    return FALSE;
}



/*
 * COMMDLG - 16 bits Find & Replace Text Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "cderr.h"
#include "cdlg16.h"



/***********************************************************************
 *                              FINDDLG_WMInitDialog            [internal]
 */
static LRESULT FINDDLG_WMInitDialog(HWND hWnd, LPARAM lParam, LPDWORD lpFlags,
                                    LPCSTR lpstrFindWhat)
{
    SetWindowLongPtrW(hWnd, DWLP_USER, lParam);
    *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the
     * FindNext (IDOK) button.  Only after typing some text, the button should be
     * enabled.
     */
    SetDlgItemTextA(hWnd, edt1, lpstrFindWhat);
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
                                 LPSTR lpstrFindWhat, WORD wFindWhatLen)
{
    int uFindReplaceMessage = RegisterWindowMessageA( FINDMSGSTRINGA );
    int uHelpMessage = RegisterWindowMessageA( HELPMSGSTRINGA );

    switch (LOWORD(wParam)) {
	case IDOK:
            GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
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
	    SendMessageW( hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongPtrW(hWnd, DWLP_USER) );
	    return TRUE;
	case IDCANCEL:
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    *lpFlags |= FR_DIALOGTERM;
	    SendMessageW( hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongPtrW(hWnd, DWLP_USER) );
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
 *           find_text_dlgproc   (internal)
 */
static INT_PTR CALLBACK find_text_dlgproc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    LPFINDREPLACE16 lpfr;
    switch (wMsg)
    {
	case WM_INITDIALOG:
            lpfr=MapSL(lParam);
	    return FINDDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags, MapSL(lpfr->lpstrFindWhat));
	case WM_COMMAND:
	    lpfr=MapSL(GetWindowLongPtrW(hWnd, DWLP_USER));
	    return FINDDLG_WMCommand(hWnd, wParam, HWND_32(lpfr->hwndOwner),
		&lpfr->Flags, MapSL(lpfr->lpstrFindWhat),
		lpfr->wFindWhatLen);
    }
    return FALSE;
}


/***********************************************************************
 *           FindText   (COMMDLG.11)
 */
HWND16 WINAPI FindText16( SEGPTR find )
{
    FINDREPLACE16 *fr16 = MapSL( find );

    return HWND_16( CreateDialogParamA( GetModuleHandleA("comdlg32.dll"), MAKEINTRESOURCEA(FINDDLGORD),
                                        HWND_32(fr16->hwndOwner), find_text_dlgproc, find ));
}


/***********************************************************************
 *           FindTextDlgProc   (COMMDLG.13)
 */
BOOL16 CALLBACK FindTextDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
                                 LPARAM lParam)
{
    return find_text_dlgproc( HWND_32(hWnd16), wMsg, wParam, lParam );
}


/***********************************************************************
 *                              REPLACEDLG_WMInitDialog         [internal]
 */
static LRESULT REPLACEDLG_WMInitDialog(HWND hWnd, LPARAM lParam,
		    LPDWORD lpFlags, LPCSTR lpstrFindWhat,
		    LPCSTR lpstrReplaceWith)
{
    SetWindowLongPtrW(hWnd, DWLP_USER, lParam);
    *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
    /*
     * FIXME : If the initial FindWhat string is empty, we should disable the FindNext /
     * Replace / ReplaceAll buttons.  Only after typing some text, the buttons should be
     * enabled.
     */
    SetDlgItemTextA(hWnd, edt1, lpstrFindWhat);
    SetDlgItemTextA(hWnd, edt2, lpstrReplaceWith);
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
static LRESULT REPLACEDLG_WMCommand(HWND hWnd, WPARAM wParam,
                                    HWND hwndOwner, LPDWORD lpFlags,
                                    LPSTR lpstrFindWhat, WORD wFindWhatLen,
                                    LPSTR lpstrReplaceWith, WORD wReplaceWithLen)
{
    int uFindReplaceMessage = RegisterWindowMessageA( FINDMSGSTRINGA );
    int uHelpMessage = RegisterWindowMessageA( HELPMSGSTRINGA );

    switch (LOWORD(wParam)) {
	case IDOK:
            GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
            GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD;
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE;
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_REPLACE | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_FINDNEXT;
	    SendMessageW( hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongPtrW(hWnd, DWLP_USER) );
	    return TRUE;
	case IDCANCEL:
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL);
	    *lpFlags |= FR_DIALOGTERM;
	    SendMessageW( hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongPtrW(hWnd, DWLP_USER) );
	    DestroyWindow(hWnd);
	    return TRUE;
	case psh1:
            GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
            GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD;
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE;
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACEALL | FR_DIALOGTERM);
	    *lpFlags |= FR_REPLACE;
	    SendMessageW( hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongPtrW(hWnd, DWLP_USER) );
	    return TRUE;
	case psh2:
            GetDlgItemTextA(hWnd, edt1, lpstrFindWhat, wFindWhatLen);
            GetDlgItemTextA(hWnd, edt2, lpstrReplaceWith, wReplaceWithLen);
	    if (IsDlgButtonChecked(hWnd, chx1))
		*lpFlags |= FR_WHOLEWORD;
		else *lpFlags &= ~FR_WHOLEWORD;
	    if (IsDlgButtonChecked(hWnd, chx2))
		*lpFlags |= FR_MATCHCASE;
		else *lpFlags &= ~FR_MATCHCASE;
            *lpFlags &= ~(FR_FINDNEXT | FR_REPLACE | FR_DIALOGTERM);
	    *lpFlags |= FR_REPLACEALL;
	    SendMessageW( hwndOwner, uFindReplaceMessage, 0,
                          GetWindowLongPtrW(hWnd, DWLP_USER) );
	    return TRUE;
	case pshHelp:
	    /* FIXME : should lpfr structure be passed as an argument ??? */
	    SendMessageA(hwndOwner, uHelpMessage, 0, 0);
	    return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           replace_text_dlgproc
 */
static INT_PTR CALLBACK replace_text_dlgproc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    LPFINDREPLACE16 lpfr;
    switch (wMsg)
    {
	case WM_INITDIALOG:
            lpfr=MapSL(lParam);
	    return REPLACEDLG_WMInitDialog(hWnd, lParam, &lpfr->Flags,
		    MapSL(lpfr->lpstrFindWhat),
		    MapSL(lpfr->lpstrReplaceWith));
	case WM_COMMAND:
	    lpfr=MapSL(GetWindowLongPtrW(hWnd, DWLP_USER));
	    return REPLACEDLG_WMCommand(hWnd, wParam, HWND_32(lpfr->hwndOwner),
		    &lpfr->Flags, MapSL(lpfr->lpstrFindWhat),
		    lpfr->wFindWhatLen, MapSL(lpfr->lpstrReplaceWith),
		    lpfr->wReplaceWithLen);
    }
    return FALSE;
}


/***********************************************************************
 *           ReplaceText   (COMMDLG.12)
 */
HWND16 WINAPI ReplaceText16( SEGPTR find )
{
    FINDREPLACE16 *fr16 = MapSL( find );

    return HWND_16( CreateDialogParamA( GetModuleHandleA("comdlg32.dll"), MAKEINTRESOURCEA(REPLACEDLGORD),
                                        HWND_32(fr16->hwndOwner), replace_text_dlgproc, find ));
}


/***********************************************************************
 *           ReplaceTextDlgProc   (COMMDLG.14)
 */
BOOL16 CALLBACK ReplaceTextDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
                                    LPARAM lParam)
{
    return replace_text_dlgproc( HWND_32(hWnd16), wMsg, wParam, lParam );
}


/***********************************************************************
 *	CommDlgExtendedError			(COMMDLG.26)
 */
DWORD WINAPI CommDlgExtendedError16(void)
{
    return CommDlgExtendedError();
}

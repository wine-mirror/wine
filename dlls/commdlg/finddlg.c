/*
 * COMMDLG - 16 bits Find & Replace Text Dialogs
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
#include "commdlg.h"
#include "ldt.h"
#include "debugtools.h"
#include "cderr.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"

struct FRPRIVATE 
{
    HANDLE16 hDlgTmpl16; /* handle for resource 16 */
    HANDLE16 hResource16; /* handle for allocated resource 16 */
    HANDLE16 hGlobal16; /* 16 bits mem block (resources) */
    LPCVOID template; /* template for 32 bits resource */
    BOOL find; /* TRUE if find dialog, FALSE if replace dialog */
    FINDREPLACE16 *fr16;
};

#define LFRPRIVATE struct FRPRIVATE *

LRESULT WINAPI FindTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
                                 LPARAM lParam);
LRESULT WINAPI ReplaceTextDlgProc16(HWND16 hWnd, UINT16 wMsg, WPARAM16 wParam,
				    LPARAM lParam);

/***********************************************************************
 *           FINDDLG_Get16BitsTemplate                                [internal]
 *
 * Get a template (FALSE if failure) when 16 bits dialogs are used
 * by a 16 bits application
 * FIXME : no test was done for the user-provided template cases
 */
BOOL FINDDLG_Get16BitsTemplate(LFRPRIVATE lfr)
{
    LPFINDREPLACE16 fr16 = lfr->fr16;

    if (fr16->Flags & FR_ENABLETEMPLATEHANDLE)
    {
        lfr->template = GlobalLock16(fr16->hInstance);
        if (!lfr->template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            return FALSE;
        }
    }
    else if (fr16->Flags & FR_ENABLETEMPLATE)
    {
	HANDLE16 hResInfo;
	if (!(hResInfo = FindResource16(fr16->hInstance,
					PTR_SEG_TO_LIN(fr16->lpTemplateName),
                                        RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(lfr->hDlgTmpl16 = LoadResource16( fr16->hInstance, hResInfo )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        lfr->hResource16 = lfr->hDlgTmpl16;
        lfr->template = LockResource16(lfr->hResource16);
        if (!lfr->template)
        {
            FreeResource16(lfr->hResource16);
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            return FALSE;
        }
    }
    else
    { /* get resource from (32 bits) own Wine resource; convert it to 16 */
	HANDLE hResInfo, hDlgTmpl32;
        LPCVOID template32;
        DWORD size;
        HGLOBAL16 hGlobal16;

	if (!(hResInfo = FindResourceA(COMMDLG_hInstance32, 
               lfr->find ?
               MAKEINTRESOURCEA(FINDDLGORD):MAKEINTRESOURCEA(REPLACEDLGORD),
               RT_DIALOGA)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl32 = LoadResource(COMMDLG_hInstance32, hResInfo )) ||
	    !(template32 = LockResource( hDlgTmpl32 )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        size = SizeofResource(GetModuleHandleA("COMDLG32"), hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %ld bytes\n", size);
            return FALSE;
        }
        lfr->template = GlobalLock16(hGlobal16);
        if (!lfr->template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hGlobal16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)lfr->template);
        lfr->hDlgTmpl16 = hGlobal16;
        lfr->hGlobal16 = hGlobal16;
    }
    return TRUE;
}


/***********************************************************************
 *           FINDDLG_FreeResources                                [internal]
 *
 * Free resources allocated 
 */
void FINDDLG_FreeResources(LFRPRIVATE lfr)
{
    /* free resources */
    if (lfr->fr16->Flags & FR_ENABLETEMPLATEHANDLE)
        GlobalUnlock16(lfr->fr16->hInstance);
    if (lfr->hResource16)
    {
        GlobalUnlock16(lfr->hResource16);
        FreeResource16(lfr->hResource16);
    }
    if (lfr->hGlobal16)
    {
        GlobalUnlock16(lfr->hGlobal16);
        GlobalFree16(lfr->hGlobal16);
    }
}

/***********************************************************************
 *           FindText16   (COMMDLG.11)
 */
HWND16 WINAPI FindText16( SEGPTR find )
{
    HANDLE16 hInst;
    HWND16 ret = 0;
    FARPROC16 ptr;
    LFRPRIVATE lfr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct FRPRIVATE));

    if (!lfr) return 0;
    lfr->fr16 = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);
    lfr->find = TRUE;
    if (FINDDLG_Get16BitsTemplate(lfr))
    {
        hInst = GetWindowLongA( lfr->fr16->hwndOwner , GWL_HINSTANCE);
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 13);
        ret = CreateDialogIndirectParam16( hInst, lfr->template,
                    lfr->fr16->hwndOwner, (DLGPROC16) ptr, find);
        FINDDLG_FreeResources(lfr);
    }
    HeapFree(GetProcessHeap(), 0, lfr);
    return ret;
}


/***********************************************************************
 *           ReplaceText16   (COMMDLG.12)
 */
HWND16 WINAPI ReplaceText16( SEGPTR find )
{
    HANDLE16 hInst;
    HWND16 ret = 0;
    FARPROC16 ptr;
    LFRPRIVATE lfr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct FRPRIVATE));

    if (!lfr) return 0;
    /*
     * FIXME : We should do error checking on the lpFind structure here
     * and make CommDlgExtendedError() return the error condition.
     */
    lfr->fr16 = (LPFINDREPLACE16)PTR_SEG_TO_LIN(find);
    lfr->find = FALSE;
    if (FINDDLG_Get16BitsTemplate(lfr))
    {
        hInst = GetWindowLongA( lfr->fr16->hwndOwner , GWL_HINSTANCE);
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 14);
        ret = CreateDialogIndirectParam16( hInst, lfr->template,
                    lfr->fr16->hwndOwner, (DLGPROC16) ptr, find);

        FINDDLG_FreeResources(lfr);
    }
    HeapFree(GetProcessHeap(), 0, lfr);
    return ret;
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


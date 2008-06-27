/*
 *	common shell dialogs
 *
 * Copyright 2000 Juergen Schmied
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

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "wine/debug.h"

#include "shellapi.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "undocshell.h"

typedef struct
    {
	HWND hwndOwner ;
	HICON hIcon ;
	LPCWSTR lpstrDirectory ;
	LPCWSTR lpstrTitle ;
	LPCWSTR lpstrDescription ;
	UINT uFlags ;
    } RUNFILEDLGPARAMS ;

typedef BOOL (*WINAPI LPFNOFN) (OPENFILENAMEW *) ;

WINE_DEFAULT_DEBUG_CHANNEL(shell);
static INT_PTR CALLBACK RunDlgProc (HWND, UINT, WPARAM, LPARAM) ;
static void FillList (HWND, char *) ;


/*************************************************************************
 * PickIconDlg					[SHELL32.62]
 *
 */
BOOL WINAPI PickIconDlg(
	HWND hwndOwner,
	LPSTR lpstrFile,
	DWORD nMaxFile,
	LPDWORD lpdwIconIndex)
{
	FIXME("(%p,%s,%08x,%p):stub.\n",
	  hwndOwner, lpstrFile, nMaxFile,lpdwIconIndex);
	return 0xffffffff;
}

/*************************************************************************
 * RunFileDlgW					[internal]
 *
 * The Unicode function that is available as ordinal 61 on Windows NT/2000/XP/...
 *
 * SEE ALSO
 *   RunFileDlgAW
 */
void WINAPI RunFileDlgW(
	HWND hwndOwner,
	HICON hIcon,
	LPCWSTR lpstrDirectory,
	LPCWSTR lpstrTitle,
	LPCWSTR lpstrDescription,
	UINT uFlags)
{
    static const WCHAR resnameW[] = {'S','H','E','L','L','_','R','U','N','_','D','L','G',0};
    RUNFILEDLGPARAMS rfdp;
    HRSRC hRes;
    LPVOID template;
    TRACE("\n");

    rfdp.hwndOwner        = hwndOwner;
    rfdp.hIcon            = hIcon;
    rfdp.lpstrDirectory   = lpstrDirectory;
    rfdp.lpstrTitle       = lpstrTitle;
    rfdp.lpstrDescription = lpstrDescription;
    rfdp.uFlags           = uFlags;

    if(!(hRes = FindResourceW(shell32_hInstance, resnameW, (LPWSTR)RT_DIALOG)))
        {
        MessageBoxA (hwndOwner, "Couldn't find dialog.", "Nix", MB_OK) ;
        return;
        }
    if(!(template = LoadResource(shell32_hInstance, hRes)))
        {
        MessageBoxA (hwndOwner, "Couldn't load dialog.", "Nix", MB_OK) ;
        return;
        }

    DialogBoxIndirectParamW(shell32_hInstance,
			    template, hwndOwner, RunDlgProc, (LPARAM)&rfdp);

}

/* Dialog procedure for RunFileDlg */
static INT_PTR CALLBACK RunDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
    static RUNFILEDLGPARAMS *prfdp = NULL ;

    switch (message)
        {
        case WM_INITDIALOG :
            prfdp = (RUNFILEDLGPARAMS *)lParam ;
            SetWindowTextW (hwnd, prfdp->lpstrTitle) ;
            SetClassLongPtrW (hwnd, GCLP_HICON, (LPARAM)prfdp->hIcon) ;
            SendMessageW (GetDlgItem (hwnd, 12297), STM_SETICON,
                          (WPARAM)LoadIconW (NULL, (LPCWSTR)IDI_WINLOGO), 0);
            FillList (GetDlgItem (hwnd, IDC_RUNDLG_EDITPATH), NULL) ;
            SetFocus (GetDlgItem (hwnd, IDC_RUNDLG_EDITPATH)) ;
            return TRUE ;

        case WM_COMMAND :
            switch (LOWORD (wParam))
                {
                case IDOK :
                    {
                    int ic ;
                    HWND htxt = GetDlgItem (hwnd, IDC_RUNDLG_EDITPATH);
                    if ((ic = GetWindowTextLengthW (htxt)))
                        {
                        WCHAR *psz ;
                        psz = HeapAlloc( GetProcessHeap(), 0, (ic + 1)*sizeof(WCHAR) );
                        GetWindowTextW (htxt, psz, ic + 1) ;

                        if (ShellExecuteW(NULL, NULL, psz, NULL, NULL, SW_SHOWNORMAL) < (HINSTANCE)33)
                            {
                            char *pszSysMsg = NULL ;
                            char szMsg[256];
                            FormatMessageA (
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, GetLastError (),
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPSTR)&pszSysMsg, 0, NULL
                                ) ;
                            sprintf (szMsg, "Error: %s", pszSysMsg) ;
                            LocalFree ((HLOCAL)pszSysMsg) ;
                            MessageBoxA (hwnd, szMsg, "Nix", MB_OK | MB_ICONEXCLAMATION) ;

                            HeapFree(GetProcessHeap(), 0, psz);
                            SendMessageA (htxt, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                            return TRUE ;
                            }

                        /* FillList is still ANSI */
                        GetWindowTextA (htxt, (LPSTR)psz, ic + 1) ;
                        FillList (htxt, (LPSTR)psz) ;

                        HeapFree(GetProcessHeap(), 0, psz);
                        EndDialog (hwnd, 0) ;
                        }
                    }

                case IDCANCEL :
                    EndDialog (hwnd, 0) ;
                    return TRUE ;

                case IDC_RUNDLG_BROWSE :
                    {
                    HMODULE hComdlg = NULL ;
                    LPFNOFN ofnProc = NULL ;
                    WCHAR szFName[1024] = {0};
                    WCHAR szFilter[MAX_PATH], szCaption[MAX_PATH];
                    static const char ansiFilter[] = "Executable Files\0*.exe\0All Files\0*.*\0\0\0\0";
                    OPENFILENAMEW ofn;

                    MultiByteToWideChar(CP_UTF8, 0, ansiFilter, sizeof(ansiFilter), szFilter, MAX_PATH);
                    MultiByteToWideChar(CP_UTF8, 0, "Browse", -1, szCaption, MAX_PATH);
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(OPENFILENAMEW);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = szFilter;
                    ofn.lpstrFile = szFName;
                    ofn.nMaxFile = 1023;
                    ofn.lpstrTitle = szCaption;
                    ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

                    if (NULL == (hComdlg = LoadLibraryExA ("comdlg32", NULL, 0)))
                        {
                        MessageBoxA (hwnd, "Unable to display dialog box (LoadLibraryEx) !", "Nix", MB_OK | MB_ICONEXCLAMATION) ;
                        return TRUE ;
                        }

                    if (NULL == (ofnProc = (LPFNOFN)GetProcAddress (hComdlg, "GetOpenFileNameW")))
                        {
                        MessageBoxA (hwnd, "Unable to display dialog box (GetProcAddress) !", "Nix", MB_OK | MB_ICONEXCLAMATION) ;
                        return TRUE ;
                        }

                    if (ofnProc(&ofn))
                    {

                        SetFocus (GetDlgItem (hwnd, IDOK)) ;
                        SetWindowTextW (GetDlgItem (hwnd, 12298), szFName) ;
                        SendMessageW (GetDlgItem (hwnd, 12298), CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                        SetFocus (GetDlgItem (hwnd, IDOK)) ;
                    }

                    FreeLibrary (hComdlg) ;

                    return TRUE ;
                    }
                }
            return TRUE ;
        }
    return FALSE ;
    }

/* This grabs the MRU list from the registry and fills the combo for the "Run" dialog above */
static void FillList (HWND hCb, char *pszLatest)
    {
    HKEY hkey ;
/*    char szDbgMsg[256] = "" ; */
    char *pszList = NULL, *pszCmd = NULL, cMatch = 0, cMax = 0x60, szIndex[2] = "-" ;
    DWORD icList = 0, icCmd = 0 ;
    UINT Nix ;

    SendMessageA (hCb, CB_RESETCONTENT, 0, 0) ;

    if (ERROR_SUCCESS != RegCreateKeyExA (
        HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
        MessageBoxA (hCb, "Unable to open registry key !", "Nix", MB_OK) ;

    RegQueryValueExA (hkey, "MRUList", NULL, NULL, NULL, &icList) ;

    if (icList > 0)
        {
        pszList = HeapAlloc( GetProcessHeap(), 0, icList) ;
        if (ERROR_SUCCESS != RegQueryValueExA (hkey, "MRUList", NULL, NULL, (LPBYTE)pszList, &icList))
            MessageBoxA (hCb, "Unable to grab MRUList !", "Nix", MB_OK) ;
        }
    else
        {
        icList = 1 ;
        pszList = HeapAlloc( GetProcessHeap(), 0, icList) ;
        pszList[0] = 0 ;
        }

    for (Nix = 0 ; Nix < icList - 1 ; Nix++)
        {
        if (pszList[Nix] > cMax)
            cMax = pszList[Nix] ;

        szIndex[0] = pszList[Nix] ;

        if (ERROR_SUCCESS != RegQueryValueExA (hkey, szIndex, NULL, NULL, NULL, &icCmd))
            MessageBoxA (hCb, "Unable to grab size of index", "Nix", MB_OK) ;
        if( pszCmd )
            pszCmd = HeapReAlloc(GetProcessHeap(), 0, pszCmd, icCmd) ;
        else
            pszCmd = HeapAlloc(GetProcessHeap(), 0, icCmd) ;
        if (ERROR_SUCCESS != RegQueryValueExA (hkey, szIndex, NULL, NULL, (LPBYTE)pszCmd, &icCmd))
            MessageBoxA (hCb, "Unable to grab index", "Nix", MB_OK) ;

        if (NULL != pszLatest)
            {
            if (!lstrcmpiA(pszCmd, pszLatest))
                {
                /*
                sprintf (szDbgMsg, "Found existing (%d).\n", Nix) ;
                MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
                */
                SendMessageA (hCb, CB_INSERTSTRING, 0, (LPARAM)pszCmd) ;
                SetWindowTextA (hCb, pszCmd) ;
                SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;

                cMatch = pszList[Nix] ;
                memmove (&pszList[1], pszList, Nix) ;
                pszList[0] = cMatch ;
                continue ;
                }
            }

        if (26 != icList - 1 || icList - 2 != Nix || cMatch || NULL == pszLatest)
            {
            /*
            sprintf (szDbgMsg, "Happily appending (%d).\n", Nix) ;
            MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
            */
            SendMessageA (hCb, CB_ADDSTRING, 0, (LPARAM)pszCmd) ;
            if (!Nix)
                {
                SetWindowTextA (hCb, pszCmd) ;
                SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                }

            }
        else
            {
            /*
            sprintf (szDbgMsg, "Doing loop thing.\n") ;
            MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
            */
            SendMessageA (hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest) ;
            SetWindowTextA (hCb, pszLatest) ;
            SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;

            cMatch = pszList[Nix] ;
            memmove (&pszList[1], pszList, Nix) ;
            pszList[0] = cMatch ;
            szIndex[0] = cMatch ;
            RegSetValueExA (hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, strlen (pszLatest) + 1) ;
            }
        }

    if (!cMatch && NULL != pszLatest)
        {
        /*
        sprintf (szDbgMsg, "Simply inserting (increasing list).\n") ;
        MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
        */
        SendMessageA (hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest) ;
        SetWindowTextA (hCb, pszLatest) ;
        SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;

        cMatch = ++cMax ;
        if( pszList )
            pszList = HeapReAlloc(GetProcessHeap(), 0, pszList, ++icList) ;
        else
            pszList = HeapAlloc(GetProcessHeap(), 0, ++icList) ;
        memmove (&pszList[1], pszList, icList - 1) ;
        pszList[0] = cMatch ;
        szIndex[0] = cMatch ;
        RegSetValueExA (hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, strlen (pszLatest) + 1) ;
        }

    RegSetValueExA (hkey, "MRUList", 0, REG_SZ, (LPBYTE)pszList, strlen (pszList) + 1) ;

    HeapFree( GetProcessHeap(), 0, pszCmd) ;
    HeapFree( GetProcessHeap(), 0, pszList) ;
    }

/*************************************************************************
 * RunFileDlgA					[internal]
 *
 * The ANSI function that is available as ordinal 61 on Windows 9x/Me
 *
 * SEE ALSO
 *   RunFileDlgAW
 */
void WINAPI RunFileDlgA(
	HWND hwndOwner,
	HICON hIcon,
	LPCSTR lpstrDirectory,
	LPCSTR lpstrTitle,
	LPCSTR lpstrDescription,
	UINT uFlags)
{
    WCHAR title[MAX_PATH];       /* longer string wouldn't be visible in the dialog anyway */
    WCHAR description[MAX_PATH];
    WCHAR directory[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, lpstrTitle, -1, title, MAX_PATH);
    title[MAX_PATH - 1] = 0;
    MultiByteToWideChar(CP_ACP, 0, lpstrDescription, -1, description, MAX_PATH);
    description[MAX_PATH - 1] = 0;
    if (!MultiByteToWideChar(CP_ACP, 0, lpstrDirectory, -1, directory, MAX_PATH))
        directory[0] = 0;

    RunFileDlgW(hwndOwner, hIcon, directory, title, description, uFlags);
}

/*************************************************************************
 * RunFileDlgAW					[SHELL32.61]
 *
 * An undocumented way to open the Run File dialog. A documented way is to use
 * CLSID_Shell, IID_IShellDispatch (as of Wine 1.0, not implemented under Wine)
 *
 * Exported by ordinal. ANSI on Windows 9x and Unicode on Windows NT/2000/XP/etc
 *
 */
void WINAPI RunFileDlgAW(
	HWND hwndOwner,
	HICON hIcon,
	LPCVOID lpstrDirectory,
	LPCVOID lpstrTitle,
	LPCVOID lpstrDescription,
	UINT uFlags)
{
    if (SHELL_OsIsUnicode())
        RunFileDlgW(hwndOwner, hIcon, lpstrDirectory, lpstrTitle, lpstrDescription, uFlags);
    else
        RunFileDlgA(hwndOwner, hIcon, lpstrDirectory, lpstrTitle, lpstrDescription, uFlags);
}


/*************************************************************************
 * ConfirmDialog				[internal]
 *
 * Put up a confirm box, return TRUE if the user confirmed
 */
static BOOL ConfirmDialog(HWND hWndOwner, UINT PromptId, UINT TitleId)
{
  WCHAR Prompt[256];
  WCHAR Title[256];

  LoadStringW(shell32_hInstance, PromptId, Prompt, sizeof(Prompt) / sizeof(WCHAR));
  LoadStringW(shell32_hInstance, TitleId, Title, sizeof(Title) / sizeof(WCHAR));
  return MessageBoxW(hWndOwner, Prompt, Title, MB_YESNO|MB_ICONQUESTION) == IDYES;
}


/*************************************************************************
 * RestartDialogEx				[SHELL32.730]
 */

int WINAPI RestartDialogEx(HWND hWndOwner, LPCWSTR lpwstrReason, DWORD uFlags, DWORD uReason)
{
    TRACE("(%p)\n", hWndOwner);

    /* FIXME: use lpwstrReason */
    if (ConfirmDialog(hWndOwner, IDS_RESTART_PROMPT, IDS_RESTART_TITLE))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES npr;

        /* enable the shutdown privilege for the current process */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);
            npr.PrivilegeCount = 1;
            npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_REBOOT, uReason);
    }

    return 0;
}


/*************************************************************************
 * RestartDialog				[SHELL32.59]
 */

int WINAPI RestartDialog(HWND hWndOwner, LPCWSTR lpstrReason, DWORD uFlags)
{
    return RestartDialogEx(hWndOwner, lpstrReason, uFlags, 0);
}


/*************************************************************************
 * ExitWindowsDialog				[SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
void WINAPI ExitWindowsDialog (HWND hWndOwner)
{
    TRACE("(%p)\n", hWndOwner);

    if (ConfirmDialog(hWndOwner, IDS_SHUTDOWN_PROMPT, IDS_SHUTDOWN_TITLE))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES npr;

        /* enable shutdown privilege for current process */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);
            npr.PrivilegeCount = 1;
            npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_SHUTDOWN, 0);
    }
}

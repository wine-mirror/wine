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

/* RunFileDlg flags */
#define RFF_NOBROWSE        0x01
#define RFF_NODEFAULT       0x02
#define RFF_CALCDIRECTORY   0x04
#define RFF_NOLABEL         0x08
#define RFF_NOSEPARATEMEM   0x20  /* NT only */

/* RunFileFlg notification structure */
typedef struct
{
    NMHDR hdr;
    const char *lpFile;
    const char *lpDirectory;
    int nShow;
} NM_RUNFILEDLG;

/* RunFileDlg notification return values */
#define RF_OK      0x00
#define RF_CANCEL  0x01
#define RF_RETRY   0x02

typedef struct
    {
	HWND hwndOwner ;
	HICON hIcon ;
	LPCWSTR lpstrDirectory ;
	LPCWSTR lpstrTitle ;
	LPCWSTR lpstrDescription ;
	UINT uFlags ;
    } RUNFILEDLGPARAMS ;

typedef BOOL (WINAPI * LPFNOFN) (OPENFILENAMEW *) ;

WINE_DEFAULT_DEBUG_CHANNEL(shell);
static INT_PTR CALLBACK RunDlgProc (HWND, UINT, WPARAM, LPARAM) ;
static void FillList (HWND, char *, BOOL) ;


/*************************************************************************
 * PickIconDlg					[SHELL32.62]
 *
 */
INT WINAPI PickIconDlg(HWND hwndOwner, WCHAR *path, UINT path_len, INT *index)
{
    FIXME("(%p,%s,%u,%p):stub.\n", hwndOwner, debugstr_w(path), path_len, index);
    return 0xffffffff;
}

HRESULT WINAPI SHOpenWithDialog(HWND parent, const OPENASINFO *info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

/*************************************************************************
 * RunFileDlgW					[internal]
 *
 * The Unicode function that is available as ordinal 61 on Windows NT/2000/XP/...
 *
 * SEE ALSO
 *   RunFileDlgAW
 */
static void RunFileDlgW(
	HWND hwndOwner,
	HICON hIcon,
	LPCWSTR lpstrDirectory,
	LPCWSTR lpstrTitle,
	LPCWSTR lpstrDescription,
	UINT uFlags)
{
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

    if (!(hRes = FindResourceW(shell32_hInstance, L"SHELL_RUN_DLG", (LPWSTR)RT_DIALOG)) ||
        !(template = LoadResource(shell32_hInstance, hRes)))
    {
        ERR("Couldn't load SHELL_RUN_DLG resource\n");
        ShellMessageBoxW(shell32_hInstance, hwndOwner, MAKEINTRESOURCEW(IDS_RUNDLG_ERROR), NULL, MB_OK | MB_ICONERROR);
        return;
    }

    DialogBoxIndirectParamW(shell32_hInstance,
			    template, hwndOwner, RunDlgProc, (LPARAM)&rfdp);

}

/* find the directory that contains the file being run */
static LPWSTR RunDlg_GetParentDir(LPCWSTR cmdline)
{
    const WCHAR *src;
    WCHAR *dest, *result, *result_end=NULL;

    result = malloc(sizeof(WCHAR) * (wcslen(cmdline) + 5));

    src = cmdline;
    dest = result;

    if (*src == '"')
    {
        src++;
        while (*src && *src != '"')
        {
            if (*src == '\\')
                result_end = dest;
            *dest++ = *src++;
        }
    }
    else {
        while (*src)
        {
            if (iswspace(*src))
            {
                *dest = 0;
                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(result))
                    break;
                lstrcatW(dest, L".exe");
                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(result))
                    break;
            }
            else if (*src == '\\')
                result_end = dest;
            *dest++ = *src++;
        }
    }

    if (result_end)
    {
        *result_end = 0;
        return result;
    }
    else
    {
        free(result);
        return NULL;
    }
}

/* Dialog procedure for RunFileDlg */
static INT_PTR CALLBACK RunDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
    RUNFILEDLGPARAMS *prfdp = (RUNFILEDLGPARAMS *)GetWindowLongPtrW(hwnd, DWLP_USER);

    switch (message)
        {
        case WM_INITDIALOG :
            prfdp = (RUNFILEDLGPARAMS *)lParam ;
            SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)prfdp);

            if (prfdp->lpstrTitle)
                SetWindowTextW(hwnd, prfdp->lpstrTitle);
            if (prfdp->lpstrDescription)
                SetWindowTextW(GetDlgItem(hwnd, IDC_RUNDLG_DESCRIPTION), prfdp->lpstrDescription);
            if (prfdp->uFlags & RFF_NOBROWSE)
            {
                HWND browse = GetDlgItem(hwnd, IDC_RUNDLG_BROWSE);
                ShowWindow(browse, SW_HIDE);
                EnableWindow(browse, FALSE);
            }
            if (prfdp->uFlags & RFF_NOLABEL)
                ShowWindow(GetDlgItem(hwnd, IDC_RUNDLG_LABEL), SW_HIDE);
            if (prfdp->uFlags & RFF_CALCDIRECTORY)
                FIXME("RFF_CALCDIRECTORY not supported\n");

            if (prfdp->hIcon == NULL)
                prfdp->hIcon = LoadIconW(NULL, (LPCWSTR)IDI_WINLOGO);
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)prfdp->hIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)prfdp->hIcon);
            SendMessageW(GetDlgItem(hwnd, IDC_RUNDLG_ICON), STM_SETICON, (WPARAM)prfdp->hIcon, 0);

            FillList (GetDlgItem (hwnd, IDC_RUNDLG_EDITPATH), NULL, (prfdp->uFlags & RFF_NODEFAULT) == 0) ;
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
                        WCHAR *psz, *parent=NULL ;
                        SHELLEXECUTEINFOW sei ;

                        ZeroMemory (&sei, sizeof(sei)) ;
                        sei.cbSize = sizeof(sei) ;
                        psz = malloc( (ic + 1) * sizeof(WCHAR) );
                        GetWindowTextW (htxt, psz, ic + 1) ;

                        /* according to http://www.codeproject.com/KB/shell/runfiledlg.aspx we should send a
                         * WM_NOTIFY before execution */

                        sei.hwnd = hwnd;
                        sei.nShow = SW_SHOWNORMAL;
                        sei.lpFile = psz;

                        if (prfdp->lpstrDirectory)
                            sei.lpDirectory = prfdp->lpstrDirectory;
                        else
                            sei.lpDirectory = parent = RunDlg_GetParentDir(sei.lpFile);

                        if (!ShellExecuteExW( &sei ))
                        {
                            free(psz);
                            free(parent);
                            SendMessageA (htxt, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                            return TRUE ;
                        }

                        /* FillList is still ANSI */
                        GetWindowTextA (htxt, (LPSTR)psz, ic + 1) ;
                        FillList (htxt, (LPSTR)psz, FALSE) ;

                        free(psz);
                        free(parent);
                        EndDialog (hwnd, 0);
                        }
                    }
                    return TRUE;

                case IDCANCEL :
                    EndDialog (hwnd, 0) ;
                    return TRUE ;

                case IDC_RUNDLG_BROWSE :
                    {
                    WCHAR szFName[1024] = {0};
                    WCHAR filter_exe[256], filter_all[256], filter[MAX_PATH], szCaption[MAX_PATH];
                    OPENFILENAMEW ofn;

                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_FILTER_EXE, filter_exe, 256);
                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_FILTER_ALL, filter_all, 256);
                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_CAPTION, szCaption, MAX_PATH);
                    swprintf( filter, MAX_PATH, L"%s%c*.exe%c%s%c*.*%c", filter_exe, 0, 0, filter_all, 0, 0 );

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(OPENFILENAMEW);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = filter;
                    ofn.lpstrFile = szFName;
                    ofn.nMaxFile = 1023;
                    ofn.lpstrTitle = szCaption;
                    ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                    ofn.lpstrInitialDir = prfdp->lpstrDirectory;

                    if (GetOpenFileNameW(&ofn))
                    {
                        SetFocus (GetDlgItem (hwnd, IDOK)) ;
                        SetWindowTextW (GetDlgItem (hwnd, IDC_RUNDLG_EDITPATH), szFName) ;
                        SendMessageW (GetDlgItem (hwnd, IDC_RUNDLG_EDITPATH), CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                        SetFocus (GetDlgItem (hwnd, IDOK)) ;
                    }

                    return TRUE ;
                    }
                }
            return TRUE ;
        }
    return FALSE ;
    }

/* This grabs the MRU list from the registry and fills the combo for the "Run" dialog above */
/* fShowDefault ignored if pszLatest != NULL */
static void FillList (HWND hCb, char *pszLatest, BOOL fShowDefault)
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
        pszList = malloc(icList) ;
        if (ERROR_SUCCESS != RegQueryValueExA (hkey, "MRUList", NULL, NULL, (LPBYTE)pszList, &icList))
            MessageBoxA (hCb, "Unable to grab MRUList !", "Nix", MB_OK) ;
        }
    else
        {
        icList = 1 ;
        pszList = malloc(icList) ;
        pszList[0] = 0 ;
        }

    for (Nix = 0 ; Nix < icList - 1 ; Nix++)
        {
        if (pszList[Nix] > cMax)
            cMax = pszList[Nix] ;

        szIndex[0] = pszList[Nix] ;

        if (ERROR_SUCCESS != RegQueryValueExA (hkey, szIndex, NULL, NULL, NULL, &icCmd))
            MessageBoxA (hCb, "Unable to grab size of index", "Nix", MB_OK) ;
        pszCmd = realloc(pszCmd, icCmd) ;
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
            if (!Nix && fShowDefault)
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
        pszList = realloc(pszList, ++icList) ;
        memmove (&pszList[1], pszList, icList - 1) ;
        pszList[0] = cMatch ;
        szIndex[0] = cMatch ;
        RegSetValueExA (hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, strlen (pszLatest) + 1) ;
        }

    RegSetValueExA (hkey, "MRUList", 0, REG_SZ, (LPBYTE)pszList, strlen (pszList) + 1) ;

    free(pszCmd) ;
    free(pszList) ;
}

/*************************************************************************
 * RunFileDlgA					[internal]
 *
 * The ANSI function that is available as ordinal 61 on Windows 9x/Me
 *
 * SEE ALSO
 *   RunFileDlgAW
 */
static void RunFileDlgA(
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

    RunFileDlgW(hwndOwner, hIcon,
        lpstrDirectory ? directory : NULL,
        lpstrTitle ? title : NULL,
        lpstrDescription ? description : NULL,
        uFlags);
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

  LoadStringW(shell32_hInstance, PromptId, Prompt, ARRAY_SIZE(Prompt));
  LoadStringW(shell32_hInstance, TitleId, Title, ARRAY_SIZE(Title));
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

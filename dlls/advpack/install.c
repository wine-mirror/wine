/*
 * Advpack install functions
 *
 * Copyright 2006 James Hawkins
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winver.h"
#include "winternl.h"
#include "winnls.h"
#include "setupapi.h"
#include "advpub.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

#define SPAPI_ERROR     0xE0000000L
#define SPAPI_PREFIX    0x800F0000L
#define SPAPI_MASK      0xFFFFL
#define HRESULT_FROM_SPAPI(x)   ((x & SPAPI_MASK) | SPAPI_PREFIX)

#define ADV_HRESULT(x)  ((x & SPAPI_ERROR) ? HRESULT_FROM_SPAPI(x) : HRESULT_FROM_WIN32(x))

/* sequentially returns pointers to parameters in a parameter list
 * returns NULL if the parameter is empty, e.g. one,,three  */
LPWSTR get_parameter(LPWSTR *params, WCHAR separator)
{
    LPWSTR token = *params;

    if (!*params)
        return NULL;

    *params = strchrW(*params, separator);
    if (*params)
        *(*params)++ = '\0';

    if (!*token)
        return NULL;

    return token;
}

static BOOL is_full_path(LPWSTR path)
{
    const int MIN_PATH_LEN = 3;

    if (!path || lstrlenW(path) < MIN_PATH_LEN)
        return FALSE;

    if (path[1] == ':' || (path[0] == '\\' && path[1] == '\\'))
        return TRUE;

    return FALSE;
}

/* this structure very closely resembles parameters of RunSetupCommand() */
typedef struct
{
    HWND hwnd;
    LPCSTR title;
    LPCSTR inf_name;
    LPCSTR dir;
    LPCSTR section_name;
} SETUPCOMMAND_PARAMS;

/***********************************************************************
 *      DoInfInstall  (ADVPACK.@)
 *
 * Install an INF section.
 *
 * PARAMS
 *  setup [I] Structure containing install information.
 *
 * RETURNS
 *   S_OK                                Everything OK
 *   HRESULT_FROM_WIN32(GetLastError())  Some other error
 */
HRESULT WINAPI DoInfInstall(const SETUPCOMMAND_PARAMS *setup)
{
    BOOL ret;
    HINF hinf;
    void *callback_context;

    TRACE("%p, %s, %s, %s, %s\n", setup->hwnd, debugstr_a(setup->title),
          debugstr_a(setup->inf_name), debugstr_a(setup->dir),
          debugstr_a(setup->section_name));

    hinf = SetupOpenInfFileA(setup->inf_name, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

    callback_context = SetupInitDefaultQueueCallback(setup->hwnd);

    ret = SetupInstallFromInfSectionA(NULL, hinf, setup->section_name, SPINST_ALL,
                                      NULL, NULL, 0, SetupDefaultQueueCallbackA,
                                      callback_context, NULL, NULL);
    SetupTermDefaultQueueCallback(callback_context);
    SetupCloseInfFile(hinf);

    return ret ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

/***********************************************************************
 *             ExecuteCabA    (ADVPACK.@)
 *
 * See ExecuteCabW.
 */
HRESULT WINAPI ExecuteCabA(HWND hwnd, CABINFOA* pCab, LPVOID pReserved)
{
    UNICODE_STRING cab, inf, section;
    CABINFOW cabinfo;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", hwnd, pCab, pReserved);

    if (!pCab)
        return E_INVALIDARG;

    if (pCab->pszCab)
    {
        RtlCreateUnicodeStringFromAsciiz(&cab, pCab->pszCab);
        cabinfo.pszCab = cab.Buffer;
    }
    else
        cabinfo.pszCab = NULL;

    RtlCreateUnicodeStringFromAsciiz(&inf, pCab->pszInf);
    RtlCreateUnicodeStringFromAsciiz(&section, pCab->pszSection);
    
    MultiByteToWideChar(CP_ACP, 0, pCab->szSrcPath, -1, cabinfo.szSrcPath,
                        sizeof(cabinfo.szSrcPath) / sizeof(WCHAR));

    cabinfo.pszInf = inf.Buffer;
    cabinfo.pszSection = section.Buffer;
    cabinfo.dwFlags = pCab->dwFlags;

    hr = ExecuteCabW(hwnd, &cabinfo, pReserved);

    if (pCab->pszCab)
        RtlFreeUnicodeString(&cab);

    RtlFreeUnicodeString(&inf);
    RtlFreeUnicodeString(&section);

    return hr;
}

/***********************************************************************
 *             ExecuteCabW    (ADVPACK.@)
 * 
 * Installs the INF file extracted from a specified cabinet file.
 * 
 * PARAMS
 *   hwnd      [I] Handle to the window used for the display.
 *   pCab      [I] Information about the cabinet file.
 *   pReserved [I] Reserved.  Must be NULL.
 * 
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented
 */
HRESULT WINAPI ExecuteCabW(HWND hwnd, CABINFOW* pCab, LPVOID pReserved)
{
    FIXME("(%p, %p, %p): stub\n", hwnd, pCab, pReserved);
    return E_FAIL;
}

/***********************************************************************
 *      LaunchINFSectionA   (ADVPACK.@)
 *
 * See LaunchINFSectionW.
 */
INT WINAPI LaunchINFSectionA(HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show)
{
    UNICODE_STRING cmd;
    HRESULT hr;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInst, debugstr_a(cmdline), show);

    RtlCreateUnicodeStringFromAsciiz(&cmd, cmdline);

    hr = LaunchINFSectionW(hWnd, hInst, cmd.Buffer, show);

    RtlFreeUnicodeString(&cmd);

    return hr;
}

/***********************************************************************
 *      LaunchINFSectionW   (ADVPACK.@)
 *
 * Installs an INF section without BACKUP/ROLLBACK capabilities.
 *
 * PARAMS
 *   hWnd    [I] Handle to parent window, NULL for desktop.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order INF,section,flags,reboot.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: S_FALSE
 *
 * NOTES
 *  INF - Filename of the INF to launch.
 *  section - INF section to install.
 *  flags - see advpub.h.
 *  reboot - smart reboot behavior
 *    'A' Always reboot.
 *    'I' Reboot if needed (default).
 *    'N' No reboot.
 * 
 * BUGS
 *  Unimplemented.
 */
INT WINAPI LaunchINFSectionW(HWND hWnd, HINSTANCE hInst, LPWSTR cmdline, INT show)
{
    FIXME("(%p, %p, %s, %i): stub\n", hWnd, hInst, debugstr_w(cmdline), show);
    return 0;
}

/***********************************************************************
 *      LaunchINFSectionExA (ADVPACK.@)
 *
 * See LaunchINFSectionExW.
 */
HRESULT WINAPI LaunchINFSectionExA(HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show)
{
    UNICODE_STRING cmd;
    HRESULT hr;

    TRACE("(%p, %p, %s, %i): stub\n", hWnd, hInst, debugstr_a(cmdline), show);

    RtlCreateUnicodeStringFromAsciiz(&cmd, cmdline);

    hr = LaunchINFSectionExW(hWnd, hInst, cmd.Buffer, show);

    RtlFreeUnicodeString(&cmd);

    return hr;
}

/***********************************************************************
 *      LaunchINFSectionExW (ADVPACK.@)
 *
 * Installs an INF section with BACKUP/ROLLBACK capabilities.
 *
 * PARAMS
 *   hWnd    [I] Handle to parent window, NULL for desktop.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order INF,section,CAB,flags,reboot.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL.
 *
 * NOTES
 *  INF - Filename of the INF to launch.
 *  section - INF section to install.
 *  flags - see advpub.h.
 *  reboot - smart reboot behavior
 *    'A' Always reboot.
 *    'I' Reboot if needed (default).
 *    'N' No reboot.
 *
 * BUGS
 *  Doesn't handle the reboot flag.
 */
HRESULT WINAPI LaunchINFSectionExW(HWND hWnd, HINSTANCE hInst, LPWSTR cmdline, INT show)
{
    LPWSTR cmdline_copy, cmdline_ptr;
    LPWSTR flags, ptr;
    CABINFOW cabinfo;
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %s, %d)\n", hWnd, hInst, debugstr_w(cmdline), show);

    if (!cmdline)
        return E_INVALIDARG;

    cmdline_copy = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    cmdline_ptr = cmdline_copy;
    lstrcpyW(cmdline_copy, cmdline);

    cabinfo.pszInf = get_parameter(&cmdline_ptr, ',');
    cabinfo.pszSection = get_parameter(&cmdline_ptr, ',');
    cabinfo.pszCab = get_parameter(&cmdline_ptr, ',');

    flags = get_parameter(&cmdline_ptr, ',');
    if (flags)
        cabinfo.dwFlags = atolW(flags);

    /* get the source path from the cab filename */
    if (cabinfo.pszCab && *cabinfo.pszCab)
    {
        if (!is_full_path(cabinfo.pszCab))
            goto done;

        lstrcpyW(cabinfo.szSrcPath, cabinfo.pszCab);
        ptr = strrchrW(cabinfo.szSrcPath, '\\');
        *(++ptr) = '\0';
    }

    hr = ExecuteCabW(hWnd, &cabinfo, NULL);

done:
    HeapFree(GetProcessHeap(), 0, cmdline_copy);

    return hr;
}

static HRESULT launch_exe(LPCWSTR cmd, LPCWSTR dir, HANDLE *phEXE)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    if (phEXE) *phEXE = NULL;

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcessW(NULL, (LPWSTR)cmd, NULL, NULL, FALSE,
                        CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP,
                        NULL, dir, &si, &pi))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(pi.hThread);

    if (phEXE)
    {
        *phEXE = pi.hProcess;
        return S_ASYNCHRONOUS;
    }

    /* wait for the child process to finish */
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    return S_OK;
}

/***********************************************************************
 *      RunSetupCommandA  (ADVPACK.@)
 *
 * See RunSetupCommandW.
 */
HRESULT WINAPI RunSetupCommandA(HWND hWnd, LPCSTR szCmdName,
                                LPCSTR szInfSection, LPCSTR szDir,
                                LPCSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved)
{
    UNICODE_STRING cmdname, infsec;
    UNICODE_STRING dir, title;
    HRESULT hr;

    TRACE("(%p, %s, %s, %s, %s, %p, %ld, %p)\n",
          hWnd, debugstr_a(szCmdName), debugstr_a(szInfSection),
          debugstr_a(szDir), debugstr_a(lpszTitle),
          phEXE, dwFlags, pvReserved);

    if (!szCmdName || !szDir)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&cmdname, szCmdName);
    RtlCreateUnicodeStringFromAsciiz(&infsec, szInfSection);
    RtlCreateUnicodeStringFromAsciiz(&dir, szDir);
    RtlCreateUnicodeStringFromAsciiz(&title, lpszTitle);

    hr = RunSetupCommandW(hWnd, cmdname.Buffer, infsec.Buffer, dir.Buffer,
                          title.Buffer, phEXE, dwFlags, pvReserved);

    RtlFreeUnicodeString(&cmdname);
    RtlFreeUnicodeString(&infsec);
    RtlFreeUnicodeString(&dir);
    RtlFreeUnicodeString(&title);

    return hr;
}

/***********************************************************************
 *      RunSetupCommandW  (ADVPACK.@)
 *
 * Executes an install section in an INF file or a program.
 *
 * PARAMS
 *   hWnd          [I] Handle to parent window, NULL for quiet mode
 *   szCmdName     [I] Inf or EXE filename to execute
 *   szInfSection  [I] Inf section to install, NULL for DefaultInstall
 *   szDir         [I] Path to extracted files
 *   szTitle       [I] Title of all dialogs
 *   phEXE         [O] Handle of EXE to wait for
 *   dwFlags       [I] Flags; see include/advpub.h
 *   pvReserved    [I] Reserved
 *
 * RETURNS
 *   S_OK                                 Everything OK
 *   S_ASYNCHRONOUS                       OK, required to wait on phEXE
 *   ERROR_SUCCESS_REBOOT_REQUIRED        Reboot required
 *   E_INVALIDARG                         Invalid argument given
 *   HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION)
 *                                        Not supported on this Windows version
 *   E_UNEXPECTED                         Unexpected error
 *   HRESULT_FROM_WIN32(GetLastError())   Some other error
 *
 * BUGS
 *   INF install unimplemented.
 */
HRESULT WINAPI RunSetupCommandW(HWND hWnd, LPCWSTR szCmdName,
                                LPCWSTR szInfSection, LPCWSTR szDir,
                                LPCWSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved)
{
    HINF hinf;

    TRACE("(%p, %s, %s, %s, %s, %p, %ld, %p)\n",
          hWnd, debugstr_w(szCmdName), debugstr_w(szInfSection),
          debugstr_w(szDir), debugstr_w(lpszTitle),
          phEXE, dwFlags, pvReserved);

    if (dwFlags)
        FIXME("Unhandled flags: 0x%08lx\n", dwFlags);

    if (!szCmdName || !szDir)
        return E_INVALIDARG;

    if (!(dwFlags & RSC_FLAG_INF))
        return launch_exe(szCmdName, szDir, phEXE);

    hinf = SetupOpenInfFileW(szCmdName, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE)
        return ADV_HRESULT(GetLastError());

    SetupCloseInfFile(hinf);
    return E_UNEXPECTED;
}

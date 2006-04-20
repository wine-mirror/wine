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
#include "advpack_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

#define SPAPI_ERROR     0xE0000000L
#define SPAPI_PREFIX    0x800F0000L
#define SPAPI_MASK      0xFFFFL
#define HRESULT_FROM_SPAPI(x)   ((x & SPAPI_MASK) | SPAPI_PREFIX)

#define ADV_HRESULT(x)  ((x & SPAPI_ERROR) ? HRESULT_FROM_SPAPI(x) : HRESULT_FROM_WIN32(x))

/* contains information about a specific install instance */
typedef struct _ADVInfo
{
    HINF hinf;
    LPWSTR inf_filename;
    LPWSTR install_sec;
    LPWSTR working_dir;
    DWORD flags;
    BOOL need_reboot;
} ADVInfo;

typedef HRESULT (*iterate_fields_func)(HINF hinf, PCWSTR field, void *arg);

/* Advanced INF commands */
static const WCHAR RegisterOCXs[] = {'R','e','g','i','s','t','e','r','O','C','X','s',0};
static const WCHAR RunPostSetupCommands[] = {
    'R','u','n','P','o','s','t','S','e','t','u','p','C','o','m','m','a','n','d','s',0
};

/* Advanced INF callbacks */
static HRESULT register_ocxs_callback(HINF hinf, PCWSTR field, void *arg)
{
    HMODULE hm;
    INFCONTEXT context;
    HRESULT hr = S_OK;

    BOOL ok = SetupFindFirstLineW(hinf, field, NULL, &context);
    
    for (; ok; ok = SetupFindNextLine(&context, &context))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        /* get OCX filename */
        if (!SetupGetStringFieldW(&context, 1, buffer,
                                  sizeof(buffer) / sizeof(WCHAR), NULL))
            continue;

        hm = LoadLibraryExW(buffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!hm)
        {
            hr = E_FAIL;
            continue;
        }

        if (do_ocx_reg(hm, TRUE))
            hr = E_FAIL;

        FreeLibrary(hm);
    }

    return hr;
}

static HRESULT run_post_setup_commands_callback(HINF hinf, PCWSTR field, void *arg)
{
    ADVInfo *info = (ADVInfo *)arg;
    INFCONTEXT context;
    HRESULT hr = S_OK;
    DWORD size;

    BOOL ok = SetupFindFirstLineW(hinf, field, NULL, &context);

    for (; ok; ok = SetupFindNextLine(&context, &context))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        if (!SetupGetLineTextW(&context, NULL, NULL, NULL, buffer,
                               MAX_INF_STRING_LENGTH, &size))
            continue;

        if (launch_exe(buffer, info->working_dir, NULL))
            hr = E_FAIL;
    }

    return hr;
}

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

/* retrieves the contents of a field, dynamically growing the buffer if necessary */
static WCHAR *get_field_string(INFCONTEXT *context, DWORD index, WCHAR *buffer,
                               WCHAR *static_buffer, DWORD *size)
{
    DWORD required;

    if (SetupGetStringFieldW(context, index, buffer, *size, &required)) return buffer;

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        /* now grow the buffer */
        if (buffer != static_buffer) HeapFree(GetProcessHeap(), 0, buffer);
        if (!(buffer = HeapAlloc(GetProcessHeap(), 0, required*sizeof(WCHAR)))) return NULL;
        *size = required;
        if (SetupGetStringFieldW(context, index, buffer, *size, &required)) return buffer;
    }

    if (buffer != static_buffer) HeapFree(GetProcessHeap(), 0, buffer);
    return NULL;
}

/* iterates over all fields of a certain key of a certain section */
static HRESULT iterate_section_fields(HINF hinf, PCWSTR section, PCWSTR key,
                                      iterate_fields_func callback, void *arg)
{
    WCHAR static_buffer[200];
    WCHAR *buffer = static_buffer;
    DWORD size = sizeof(static_buffer) / sizeof(WCHAR);
    INFCONTEXT context;
    HRESULT hr = E_FAIL;

    BOOL ok = SetupFindFirstLineW(hinf, section, key, &context);
    while (ok)
    {
        UINT i, count = SetupGetFieldCount(&context);

        for (i = 1; i <= count; i++)
        {
            if (!(buffer = get_field_string(&context, i, buffer, static_buffer, &size)))
                goto done;

            if ((hr = callback(hinf, buffer, arg)) != S_OK)
                goto done;
        }

        ok = SetupFindNextMatchLineW(&context, key, &context);
    }

    hr = S_OK;

 done:
    if (buffer && buffer != static_buffer) HeapFree(GetProcessHeap(), 0, buffer);
    return hr;
}

/* performs a setupapi-level install of the INF file */
static HRESULT spapi_install(ADVInfo *info)
{
    BOOL ret;
    HRESULT res;
    PVOID context;

    context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, NULL);
    if (!context)
        return ADV_HRESULT(GetLastError());

    ret = SetupInstallFromInfSectionW(NULL, info->hinf, info->install_sec,
                                      SPINST_FILES, NULL, info->working_dir,
                                      SP_COPY_NEWER, SetupDefaultQueueCallbackW,
                                      context, NULL, NULL);
    if (!ret)
    {
        res = ADV_HRESULT(GetLastError());
        SetupTermDefaultQueueCallback(context);

        return res;
    }

    SetupTermDefaultQueueCallback(context);

    ret = SetupInstallFromInfSectionW(NULL, info->hinf, info->install_sec,
                                      SPINST_INIFILES | SPINST_REGISTRY,
                                      HKEY_LOCAL_MACHINE, NULL, 0,
                                      NULL, NULL, NULL, NULL);
    if (!ret)
        return ADV_HRESULT(GetLastError());

    return S_OK;
}

/* processes the Advanced INF commands */
static HRESULT adv_install(ADVInfo *info)
{
    HRESULT hr;

    hr = iterate_section_fields(info->hinf, info->install_sec,
                                RegisterOCXs, register_ocxs_callback, NULL);
    if (hr != S_OK)
        return hr;

    hr = iterate_section_fields(info->hinf, info->install_sec, RunPostSetupCommands,
                                run_post_setup_commands_callback, info);
    if (hr != S_OK)
        return hr;

    return hr;
}

/* loads the INF file and performs checks on it */
HRESULT install_init(LPCWSTR inf_filename, LPCWSTR install_sec,
                     LPCWSTR working_dir, DWORD flags, ADVInfo *info)
{
    DWORD len;
    LPCWSTR ptr;

    static const WCHAR default_install[] = {
        'D','e','f','a','u','l','t','I','n','s','t','a','l','l',0
    };

    len = lstrlenW(inf_filename);

    info->inf_filename = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!info->inf_filename)
        return E_OUTOFMEMORY;

    lstrcpyW(info->inf_filename, inf_filename);

    /* FIXME: determine the proper platform to install (NTx86, etc) */
    if (!install_sec || !*install_sec)
    {
        len = sizeof(default_install) - 1;
        ptr = default_install;
    }
    else
    {
        len = lstrlenW(install_sec);
        ptr = install_sec;
    }

    info->install_sec = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!info->install_sec)
        return E_OUTOFMEMORY;

    lstrcpyW(info->install_sec, ptr);

    /* FIXME: need to get the real working directory */
    if (!working_dir || !*working_dir)
    {
        ptr = strrchrW(info->inf_filename, '\\');
        len = ptr - info->inf_filename + 1;
        ptr = info->inf_filename;
    }
    else
    {
        len = lstrlenW(working_dir);
        ptr = working_dir;
    }

    info->working_dir = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!info->working_dir)
        return E_OUTOFMEMORY;

    lstrcpynW(info->working_dir, ptr, len);

    info->hinf = SetupOpenInfFileW(info->inf_filename, NULL, INF_STYLE_WIN4, NULL);
    if (info->hinf == INVALID_HANDLE_VALUE)
        return ADV_HRESULT(GetLastError());

    set_ldids(info->hinf, info->install_sec, info->working_dir);

    /* FIXME: check that the INF is advanced */

    info->flags = flags;
    info->need_reboot = FALSE;

    return S_OK;
}

/* release the install instance information */
void install_release(ADVInfo *info)
{
    if (info->hinf && info->hinf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(info->hinf);

    HeapFree(GetProcessHeap(), 0, info->inf_filename);
    HeapFree(GetProcessHeap(), 0, info->install_sec);
    HeapFree(GetProcessHeap(), 0, info->working_dir);
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

    TRACE("(%p)\n", setup);

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

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInst, debugstr_a(cmdline), show);

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

HRESULT launch_exe(LPCWSTR cmd, LPCWSTR dir, HANDLE *phEXE)
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
    ADVInfo info;
    HRESULT hr;

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

    ZeroMemory(&info, sizeof(ADVInfo));

    hr = install_init(szCmdName, szInfSection, szDir, dwFlags, &info);
    if (hr != S_OK)
        goto done;

    hr = spapi_install(&info);
    if (hr != S_OK)
        goto done;

    hr = adv_install(&info);

done:
    install_release(&info);

    return hr;
}

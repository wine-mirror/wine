/*
 * Advpack main
 *
 * Copyright 2004 Huw D M Davies
 * Copyright 2005 Sami Aario
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winver.h"
#include "winnls.h"
#include "setupapi.h"
#include "advpub.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

typedef HRESULT (WINAPI *DLLREGISTER) (void);

#define MAX_FIELD_LENGTH    512
#define PREFIX_LEN          5

/* parses the destination directory parameters from pszSection
 * the parameters are of the form: root,key,value,unknown,fallback
 * we first read the reg value root\\key\\value and if that fails,
 * use fallback as the destination directory
 */
static void get_dest_dir(HINF hInf, PCSTR pszSection, PSTR pszBuffer, DWORD dwSize)
{
    INFCONTEXT context;
    CHAR key[MAX_PATH], value[MAX_PATH];
    CHAR prefix[PREFIX_LEN];
    HKEY root, subkey;
    DWORD size;

    /* load the destination parameters */
    SetupFindFirstLineA(hInf, pszSection, NULL, &context);
    SetupGetStringFieldA(&context, 1, prefix, PREFIX_LEN, &size);
    SetupGetStringFieldA(&context, 2, key, MAX_PATH, &size);
    SetupGetStringFieldA(&context, 3, value, MAX_PATH, &size);

    if (!lstrcmpA(prefix, "HKLM"))
        root = HKEY_LOCAL_MACHINE;
    else if (!lstrcmpA(prefix, "HKCU"))
        root = HKEY_CURRENT_USER;
    else
        root = NULL;

    /* preserve the buffer size */
    size = dwSize;

    /* fallback to the default destination dir if reg fails */
    if (RegOpenKeyA(root, key, &subkey) ||
        RegQueryValueExA(subkey, value, NULL, NULL, (LPBYTE)pszBuffer, &size))
    {
        SetupGetStringFieldA(&context, 6, pszBuffer, dwSize, &size);
    }

    RegCloseKey(subkey);
}

/* loads the LDIDs specified in the install section of an INF */
static void set_ldids(HINF hInf, PCSTR pszInstallSection)
{
    CHAR field[MAX_FIELD_LENGTH];
    CHAR key[MAX_FIELD_LENGTH];
    CHAR dest[MAX_PATH];
    INFCONTEXT context;
    DWORD size;
    int ldid;

    if (!SetupGetLineTextA(NULL, hInf, pszInstallSection, "CustomDestination",
                           field, MAX_FIELD_LENGTH, &size))
        return;

    if (!SetupFindFirstLineA(hInf, field, NULL, &context))
        return;

    do
    {
        SetupGetIntField(&context, 0, &ldid);
        SetupGetLineTextA(&context, NULL, NULL, NULL,
                          key, MAX_FIELD_LENGTH, &size);

        get_dest_dir(hInf, key, dest, MAX_PATH);

        SetupSetDirectoryIdA(hInf, ldid, dest);
    } while (SetupFindNextLine(&context, &context));
}

/***********************************************************************
 *           CloseINFEngine (ADVPACK.@)
 *
 * Closes a handle to an INF file opened with OpenINFEngine.
 *
 * PARAMS
 *   hInf [I] Handle to the INF file to close.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI CloseINFEngine(HINF hInf)
{
    FIXME("(%p) stub\n", hInf);

    return E_FAIL;
}

/***********************************************************************
 *           DllMain (ADVPACK.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %ld, %p)\n",hinstDLL, fdwReason, lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);

    return TRUE;
}

/***********************************************************************
 *		RunSetupCommand  (ADVPACK.@)
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
 *   Unimplemented
 */
HRESULT WINAPI RunSetupCommand( HWND hWnd, LPCSTR szCmdName,
                                LPCSTR szInfSection, LPCSTR szDir,
                                LPCSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved )
{
    FIXME("(%p, %s, %s, %s, %s, %p, 0x%08lx, %p): stub\n",
           hWnd, debugstr_a(szCmdName), debugstr_a(szInfSection),
           debugstr_a(szDir), debugstr_a(lpszTitle),
           phEXE, dwFlags, pvReserved);
    return E_UNEXPECTED;
}

/***********************************************************************
 *		LaunchINFSection  (ADVPACK.@)
 *
 * Installs an INF section without BACKUP/ROLLBACK capabilities.
 *
 * PARAMS
 *   hWnd    [I] Handle to parent window, NULL for desktop.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order INF,section,flags.
 *   show    [I] Reboot behaviour:
 *              'A' reboot always
 *              'I' default, reboot if needed
 *              'N' no reboot
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: S_FALSE
 *
 * BUGS
 *  Unimplemented.
 */
INT WINAPI LaunchINFSection( HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show )
{
    FIXME("(%p %p %s %d): stub\n", hWnd, hInst, debugstr_a(cmdline), show );
    return 0;
}

/***********************************************************************
 *		LaunchINFSectionEx  (ADVPACK.@)
 *
 * Installs an INF section with BACKUP/ROLLBACK capabilities.
 *
 * PARAMS
 *   hWnd    [I] Handle to parent window, NULL for desktop.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order INF,section,CAB,flags.
 *   show    [I] Reboot behaviour:
 *              'A' reboot always
 *              'I' default, reboot if needed
 *              'N' no reboot
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL.
 *
 * BUGS
 *  Unimplemented.
 */
HRESULT WINAPI LaunchINFSectionEx( HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show )
{
    FIXME("(%p %p %s %d): stub\n", hWnd, hInst, debugstr_a(cmdline), show );
    return E_FAIL;
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
 *		DoInfInstall  (ADVPACK.@)
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

    TRACE("%p %s %s %s %s\n", setup->hwnd, debugstr_a(setup->title),
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
 *              IsNTAdmin	(ADVPACK.@)
 *
 * Checks if the user has admin privileges.
 *
 * PARAMS
 *   reserved  [I] Reserved.  Must be 0.
 *   pReserved [I] Reserved.  Must be NULL.
 *
 * RETURNS
 *   TRUE if user has admin rights, FALSE otherwise.
 */
BOOL WINAPI IsNTAdmin( DWORD reserved, LPDWORD pReserved )
{
    SID_IDENTIFIER_AUTHORITY SidAuthority = {SECURITY_NT_AUTHORITY};
    PTOKEN_GROUPS pTokenGroups;
    BOOL bSidFound = FALSE;
    DWORD dwSize, i;
    HANDLE hToken;
    PSID pSid;

    TRACE("(0x%08lx, %p)\n", reserved, pReserved);

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    if (!GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    pTokenGroups = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!pTokenGroups)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenGroups, pTokenGroups, dwSize, &dwSize))
    {
        HeapFree(GetProcessHeap(), 0, pTokenGroups);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    if (!AllocateAndInitializeSid(&SidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSid))
    {
        HeapFree(GetProcessHeap(), 0, pTokenGroups);
        return FALSE;
    }

    for (i = 0; i < pTokenGroups->GroupCount; i++)
    {
        if (EqualSid(pSid, pTokenGroups->Groups[i].Sid))
        {
            bSidFound = TRUE;
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, pTokenGroups);
    FreeSid(pSid);

    return bSidFound;
}

/***********************************************************************
 *             NeedRebootInit  (ADVPACK.@)
 *
 * Sets up conditions for reboot checking.
 *
 * RETURNS
 *   Value required by NeedReboot.
 */
DWORD WINAPI NeedRebootInit(VOID)
{
    FIXME("(): stub\n");
    return 0;
}

/***********************************************************************
 *             NeedReboot      (ADVPACK.@)
 *
 * Determines whether a reboot is required.
 *
 * PARAMS
 *   dwRebootCheck [I] Value from NeedRebootInit.
 *
 * RETURNS
 *   TRUE if a reboot is needed, FALSE otherwise.
 *
 * BUGS
 *   Unimplemented.
 */
BOOL WINAPI NeedReboot(DWORD dwRebootCheck)
{
    FIXME("(0x%08lx): stub\n", dwRebootCheck);
    return FALSE;
}

/***********************************************************************
 *             OpenINFEngine    (ADVPACK.@)
 *
 * Opens and returns a handle to an INF file to be used by
 * TranslateInfStringEx to continuously translate the INF file.
 *
 * PARAMS
 *   pszInfFilename    [I] Filename of the INF to open.
 *   pszInstallSection [I] Name of the Install section in the INF.
 *   dwFlags           [I] See advpub.h.
 *   phInf             [O] Handle to the loaded INF file.
 *   pvReserved        [I] Reserved.  Must be NULL.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI OpenINFEngine(PCSTR pszInfFilename, PCSTR pszInstallSection,
                             DWORD dwFlags, HINF *phInf, PVOID pvReserved)
{
    FIXME("(%p, %p, %ld, %p, %p) stub\n", pszInfFilename, pszInstallSection,
          dwFlags, phInf, pvReserved);

    return E_FAIL;
}

/***********************************************************************
 *             RebootCheckOnInstall    (ADVPACK.@)
 *
 * Checks if a reboot is required for an installed INF section.
 *
 * PARAMS
 *   hWnd       [I] Handle to the window used for messages.
 *   pszINF     [I] Filename of the INF file.
 *   pszSec     [I] INF section to check.
 *   dwReserved [I] Reserved.  Must be 0.
 *
 * RETURNS
 *   Success: S_OK - Reboot is needed if the INF section is installed.
 *            S_FALSE - Reboot is not needed.
 *   Failure: HRESULT of GetLastError().
 *
 * NOTES
 *   if pszSec is NULL, RebootCheckOnInstall checks the DefaultInstall
 *   or DefaultInstall.NT section.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RebootCheckOnInstall(HWND hWnd, LPCSTR pszINF,
                                    LPCSTR pszSec, DWORD dwReserved)
{
    FIXME("(%p, %p, %p, %ld) stub\n", hWnd, pszINF, pszSec, dwReserved);

    return E_FAIL;
}

/***********************************************************************
 *             RegisterOCX    (ADVPACK.@)
 */
void WINAPI RegisterOCX( HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show )
{
    WCHAR wszBuff[MAX_PATH];
    WCHAR* pwcComma;
    HMODULE hm;
    DLLREGISTER pfnRegister;
    HRESULT hr;

    TRACE("(%s)\n", cmdline);

    MultiByteToWideChar(CP_ACP, 0, cmdline, strlen(cmdline), wszBuff, MAX_PATH);
    if ((pwcComma = strchrW( wszBuff, ',' ))) *pwcComma = 0;

    TRACE("Parsed DLL name (%s)\n", debugstr_w(wszBuff));

    hm = LoadLibraryExW(wszBuff, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hm)
    {
        ERR("Couldn't load DLL: %s\n", debugstr_w(wszBuff));
        return;
    }

    pfnRegister = (DLLREGISTER)GetProcAddress(hm, "DllRegisterServer");
    if (pfnRegister == NULL)
    {
        ERR("DllRegisterServer entry point not found\n");
    }
    else
    {
        hr = pfnRegister();
        if (hr != S_OK)
        {
            ERR("DllRegisterServer entry point returned %08lx\n", hr);
        }
    }

    TRACE("Successfully registered OCX\n");

    FreeLibrary(hm);
}

/***********************************************************************
 *             ExecuteCab    (ADVPACK.@)
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
HRESULT WINAPI ExecuteCab( HWND hwnd, PCABINFO pCab, LPVOID pReserved )
{
    FIXME("(%p %p %p): stub\n", hwnd, pCab, pReserved);
    return E_FAIL;
}

/***********************************************************************
 *             SetPerUserSecValues    (ADVPACK.@)
 *
 * Prepares the per-user stub values under IsInstalled\{GUID} that
 * control the per-user installation.
 *
 * PARAMS
 *   pPerUser [I] Per-user stub values.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI SetPerUserSecValues(PPERUSERSECTION pPerUser)
{
    FIXME("(%p) stub\n", pPerUser);

    return E_FAIL;
}

/***********************************************************************
 *             TranslateInfString    (ADVPACK.@)
 *
 * Translates the value of a specified key in an inf file into the
 * current locale by expanding string macros.
 *
 * PARAMS
 *   pszInfFilename      [I] Filename of the inf file.
 *   pszInstallSection   [I]
 *   pszTranslateSection [I] Inf section where the key exists.
 *   pszTranslateKey     [I] Key to translate.
 *   pszBuffer           [O] Contains the translated string on exit.
 *   dwBufferSize        [I] Size on input of pszBuffer.
 *   pdwRequiredSize     [O] Length of the translated key.
 *   pvReserved          [I] Reserved, must be NULL.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: An hresult error code.
 */
HRESULT WINAPI TranslateInfString(PCSTR pszInfFilename, PCSTR pszInstallSection,
                PCSTR pszTranslateSection, PCSTR pszTranslateKey, PSTR pszBuffer,
                DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved)
{
    HINF hInf;

    TRACE("(%s %s %s %s %p %ld %p %p)\n",
          debugstr_a(pszInfFilename), debugstr_a(pszInstallSection),
          debugstr_a(pszTranslateSection), debugstr_a(pszTranslateKey),
          pszBuffer, dwBufferSize,pdwRequiredSize, pvReserved);

    if (!pszInfFilename || !pszTranslateSection ||
        !pszTranslateKey || !pdwRequiredSize)
        return E_INVALIDARG;

    hInf = SetupOpenInfFileA(pszInfFilename, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    set_ldids(hInf, pszInstallSection);

    if (!SetupGetLineTextA(NULL, hInf, pszTranslateSection, pszTranslateKey,
                           pszBuffer, dwBufferSize, pdwRequiredSize))
    {
        if (dwBufferSize < *pdwRequiredSize)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        return SPAPI_E_LINE_NOT_FOUND;
    }

    SetupCloseInfFile(hInf);
    return S_OK;
}

/***********************************************************************
 *             TranslateInfStringEx    (ADVPACK.@)
 *
 * Using a handle to an INF file opened with OpenINFEngine, translates
 * the value of a specified key in an inf file into the current locale
 * by expanding string macros.
 *
 * PARAMS
 *   hInf                [I] Handle to the INF file.
 *   pszInfFilename      [I] Filename of the INF file.
 *   pszTranslateSection [I] Inf section where the key exists.
 *   pszTranslateKey     [I] Key to translate.
 *   pszBuffer           [O] Contains the translated string on exit.
 *   dwBufferSize        [I] Size on input of pszBuffer.
 *   pdwRequiredSize     [O] Length of the translated key.
 *   pvReserved          [I] Reserved.  Must be NULL.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   To use TranslateInfStringEx to translate an INF file continuously,
 *   open the INF file with OpenINFEngine, call TranslateInfStringEx as
 *   many times as needed, then release the handle with CloseINFEngine.
 *   When translating more than one keys, this method is more efficient
 *   than calling TranslateInfString, because the INF file is only
 *   opened once.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI TranslateInfStringEx(HINF hInf, PCSTR pszInfFilename,
                                    PCSTR pszTranslateSection, PCSTR pszTranslateKey,
                                    PSTR pszBuffer, DWORD dwBufferSize,
                                    PDWORD pdwRequiredSize, PVOID pvReserved)
{
    FIXME("(%p, %p, %p, %p, %p, %ld, %p, %p) stub\n", hInf, pszInfFilename,
          pszTranslateSection, pszTranslateKey, pszBuffer, dwBufferSize,
          pdwRequiredSize, pvReserved);

    return E_FAIL;   
}

/***********************************************************************
 *             UserInstStubWrapper    (ADVPACK.@)
 */
HRESULT WINAPI UserInstStubWrapper(HWND hWnd, HINSTANCE hInstance,
                                   PSTR pszParms, INT nShow)
{
    FIXME("(%p, %p, %p, %i) stub\n", hWnd, hInstance, pszParms, nShow);

    return E_FAIL;
}

/***********************************************************************
 *             UserUnInstStubWrapper    (ADVPACK.@)
 */
HRESULT WINAPI UserUnInstStubWrapper(HWND hWnd, HINSTANCE hInstance,
                                     PSTR pszParms, INT nShow)
{
    FIXME("(%p, %p, %p, %i) stub\n", hWnd, hInstance, pszParms, nShow);

    return E_FAIL;
}

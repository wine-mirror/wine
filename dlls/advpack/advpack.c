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
 */
void WINAPI LaunchINFSection( HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show )
{
    FIXME("(%p %p %s %d): stub\n", hWnd, hInst, debugstr_a(cmdline), show );
}

/***********************************************************************
 *		LaunchINFSectionEx  (ADVPACK.@)
 */
void WINAPI LaunchINFSectionEx( HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show )
{
    FIXME("(%p %p %s %d): stub\n", hWnd, hInst, debugstr_a(cmdline), show );
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
 */
BOOL WINAPI DoInfInstall(const SETUPCOMMAND_PARAMS *setup)
{
    BOOL ret;
    HINF hinf;
    void *callback_context;

    TRACE("%p %s %s %s %s\n", setup->hwnd, debugstr_a(setup->title),
          debugstr_a(setup->inf_name), debugstr_a(setup->dir),
          debugstr_a(setup->section_name));

    hinf = SetupOpenInfFileA(setup->inf_name, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE) return FALSE;

    callback_context = SetupInitDefaultQueueCallback(setup->hwnd);

    ret = SetupInstallFromInfSectionA(NULL, hinf, setup->section_name, SPINST_ALL,
                                      NULL, NULL, 0, SetupDefaultQueueCallbackA,
                                      callback_context, NULL, NULL);
    SetupTermDefaultQueueCallback(callback_context);
    SetupCloseInfFile(hinf);

    return ret;
}

/***********************************************************************
 *              IsNTAdmin	(ADVPACK.@)
 */
BOOL WINAPI IsNTAdmin( DWORD reserved, PDWORD pReserved )
{
    FIXME("(0x%08lx, %p): stub\n", reserved, pReserved);
    return TRUE;
}

/***********************************************************************
 *             NeedRebootInit  (ADVPACK.@)
 */
DWORD WINAPI NeedRebootInit(VOID)
{
    FIXME("(): stub\n");
    return 0;
}

/***********************************************************************
 *             NeedReboot      (ADVPACK.@)
 */
BOOL WINAPI NeedReboot(DWORD dwRebootCheck)
{
    FIXME("(0x%08lx): stub\n", dwRebootCheck);
    return FALSE;
}

/***********************************************************************
 *             GetVersionFromFile      (ADVPACK.@)
 */
HRESULT WINAPI GetVersionFromFile( LPSTR Filename, LPDWORD MajorVer,
                                   LPDWORD MinorVer, BOOL Version )
{
    TRACE("(%s, %p, %p, %d)\n", Filename, MajorVer, MinorVer, Version);
    return GetVersionFromFileEx(Filename, MajorVer, MinorVer, Version);
}

/***********************************************************************
 *             GetVersionFromFileEx    (ADVPACK.@)
 */
HRESULT WINAPI GetVersionFromFileEx( LPSTR lpszFilename, LPDWORD pdwMSVer,
                                     LPDWORD pdwLSVer, BOOL bVersion )
{
    DWORD hdl, retval;
    LPVOID pVersionInfo;
    BOOL boolret;
    VS_FIXEDFILEINFO *pFixedVersionInfo;
    UINT uiLength;
    TRACE("(%s, %p, %p, %d)\n", lpszFilename, pdwMSVer, pdwLSVer, bVersion);

    if (bVersion)
    {
        retval = GetFileVersionInfoSizeA(lpszFilename, &hdl);
        if (retval == 0 || hdl != 0)
            return E_FAIL;

        pVersionInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retval);
        if (pVersionInfo == NULL)
             return E_FAIL;
        GetFileVersionInfoA( lpszFilename, 0, retval, pVersionInfo);

        boolret = VerQueryValueA(pVersionInfo, "\\",
                                 (LPVOID) &pFixedVersionInfo, &uiLength);

        HeapFree(GetProcessHeap(), 0, pVersionInfo);

        if (boolret)
        {
            *pdwMSVer = pFixedVersionInfo->dwFileVersionMS;
            *pdwLSVer = pFixedVersionInfo->dwFileVersionLS;
        }
        else
            return E_FAIL;
    }
    else
    {
        *pdwMSVer = GetUserDefaultUILanguage();
        *pdwLSVer = GetACP();
    }

    return S_OK;
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

static HRESULT DELNODE_recurse_dirtree(LPSTR fname, DWORD flags)
{
    DWORD fattrs = GetFileAttributesA(fname);
    HRESULT ret = E_FAIL;

    if (fattrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        HANDLE hFindFile;
        WIN32_FIND_DATAA w32fd;
        BOOL done = TRUE;
        int fname_len = lstrlenA(fname);

        /* Generate a path with wildcard suitable for iterating */
        if (CharPrevA(fname, fname + fname_len) != "\\")
        {
            lstrcpyA(fname + fname_len, "\\");
            ++fname_len;
        }
        lstrcpyA(fname + fname_len, "*");

        if ((hFindFile = FindFirstFileA(fname, &w32fd)) != INVALID_HANDLE_VALUE)
        {
            /* Iterate through the files in the directory */
            for (done = FALSE; !done; done = !FindNextFileA(hFindFile, &w32fd))
            {
                TRACE("%s\n", w32fd.cFileName);
                if (lstrcmpA(".", w32fd.cFileName) != 0 &&
                    lstrcmpA("..", w32fd.cFileName) != 0)
                {
                    lstrcpyA(fname + fname_len, w32fd.cFileName);
                    if (DELNODE_recurse_dirtree(fname, flags) != S_OK)
                    {
                        break; /* Failure */
                    }
                }
            }
            FindClose(hFindFile);
        }

        /* We're done with this directory, so restore the old path without wildcard */
        *(fname + fname_len) = '\0';

        if (done)
        {
            TRACE("%s: directory\n", fname);
            if (SetFileAttributesA(fname, FILE_ATTRIBUTE_NORMAL) && RemoveDirectoryA(fname))
            {
                ret = S_OK;
            }
        }
    }
    else
    {
        TRACE("%s: file\n", fname);
        if (SetFileAttributesA(fname, FILE_ATTRIBUTE_NORMAL) && DeleteFileA(fname))
        {
            ret = S_OK;
        }
    }
    
    return ret;
}

/***********************************************************************
 *              DelNode    (ADVPACK.@)
 *
 * Deletes a file or directory
 *
 * PARAMS
 *   pszFileOrDirName   [I] Name of file or directory to delete
 *   dwFlags            [I] Flags; see include/advpub.h
 *
 * RETURNS 
 *   Success: S_OK
 *   Failure: E_FAIL
 *
 * BUGS
 *   - Ignores flags
 *   - Native version apparently does a lot of checking to make sure
 *     we're not trying to delete a system directory etc.
 */
HRESULT WINAPI DelNode( LPCSTR pszFileOrDirName, DWORD dwFlags )
{
    CHAR fname[MAX_PATH];
    HRESULT ret = E_FAIL;

    FIXME("(%s, 0x%08lx): flags ignored\n", debugstr_a(pszFileOrDirName), dwFlags);
    if (pszFileOrDirName && *pszFileOrDirName)
    {
        lstrcpyA(fname, pszFileOrDirName);

        /* TODO: Should check for system directory deletion etc. here */

        ret = DELNODE_recurse_dirtree(fname, dwFlags);
    }

    return ret;
}

/***********************************************************************
 *             DelNodeRunDLL32    (ADVPACK.@)
 *
 * BUGS
 *   Unimplemented
 */
void WINAPI DelNodeRunDLL32( HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show )
{
    FIXME("(%s): stub\n", debugstr_a(cmdline));
}

/***********************************************************************
 *             ExecuteCab    (ADVPACK.@)
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
 *             ExtractFiles    (ADVPACK.@)
 *
 * BUGS
 *   Unimplemented
 */

HRESULT WINAPI ExtractFiles ( LPCSTR CabName, LPCSTR ExpandDir, DWORD Flags,
                              LPCSTR FileList, LPVOID LReserved, DWORD Reserved)
{
    FIXME("(%p %p 0x%08lx %p %p 0x%08lx): stub\n", CabName, ExpandDir, Flags, 
          FileList, LReserved, Reserved);
    return E_FAIL;
}

/***********************************************************************
 *             TranslateInfString    (ADVPACK.@)
 *
 * BUGS
 *   Unimplemented
 */
HRESULT WINAPI TranslateInfString(PCSTR pszInfFilename, PCSTR pszInstallSection,
                PCSTR pszTranslateSection, PCSTR pszTranslateKey, PSTR pszBuffer,
                DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved)
{
    FIXME("(%s %s %s %s %p %ld %p %p): stub\n",
        debugstr_a(pszInfFilename), debugstr_a(pszInstallSection),
        debugstr_a(pszTranslateSection), debugstr_a(pszTranslateKey),
        pszBuffer, dwBufferSize,pdwRequiredSize, pvReserved);
    return E_FAIL;
}

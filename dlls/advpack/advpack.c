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
 *      AdvInstallFile (ADVPACK.@)
 *
 * Copies a file from the source to a destination.
 *
 * PARAMS
 *   hwnd           [I] Handle to the window used for messages.
 *   lpszSourceDir  [I] Source directory.
 *   lpszSourceFile [I] Source filename.
 *   lpszDestDir    [I] Destination directory.
 *   lpszDestFile   [I] Optional destination filename.
 *   dwFlags        [I] See advpub.h.
 *   dwReserved     [I] Reserved.  Must be 0.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   If lpszDestFile is NULL, the destination filename is the same as
 *   lpszSourceFIle.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI AdvInstallFile(HWND hwnd, LPCSTR lpszSourceDir, LPCSTR lpszSourceFile,
                              LPCSTR lpszDestDir, LPCSTR lpszDestFile,
                              DWORD dwFlags, DWORD dwReserved)
{
    FIXME("(%p,%p,%p,%p,%p,%ld,%ld) stub\n", hwnd, debugstr_a(lpszSourceDir),
          debugstr_a(lpszSourceFile), debugstr_a(lpszDestDir),
          debugstr_a(lpszDestFile), dwFlags, dwReserved);
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
 *
 * BUGS
 *   Unimplemented.
 */
BOOL WINAPI IsNTAdmin( DWORD reserved, LPDWORD pReserved )
{
    FIXME("(0x%08lx, %p): stub\n", reserved, pReserved);
    return TRUE;
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
 *             GetVersionFromFile      (ADVPACK.@)
 *
 * See GetVersionFromFileEx.
 */
HRESULT WINAPI GetVersionFromFile( LPSTR Filename, LPDWORD MajorVer,
                                   LPDWORD MinorVer, BOOL Version )
{
    TRACE("(%s, %p, %p, %d)\n", Filename, MajorVer, MinorVer, Version);
    return GetVersionFromFileEx(Filename, MajorVer, MinorVer, Version);
}

/* data for GetVersionFromFileEx */
typedef struct tagLANGANDCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE;

/***********************************************************************
 *             GetVersionFromFileEx    (ADVPACK.@)
 *
 * Gets the files version or language information.
 *
 * PARAMS
 *   lpszFilename [I] The file to get the info from.
 *   pdwMSVer     [O] Major version.
 *   pdwLSVer     [O] Minor version.
 *   bVersion     [I] Whether to retrieve version or language info.
 *
 * RETURNS
 *   Always returns S_OK.
 *
 * NOTES
 *   If bVersion is TRUE, version information is retrieved, else
 *   pdwMSVer gets the language ID and pdwLSVer gets the codepage ID.
 */
HRESULT WINAPI GetVersionFromFileEx( LPSTR lpszFilename, LPDWORD pdwMSVer,
                                     LPDWORD pdwLSVer, BOOL bVersion )
{
    VS_FIXEDFILEINFO *pFixedVersionInfo;
    LANGANDCODEPAGE *pLangAndCodePage;
    DWORD dwHandle, dwInfoSize;
    CHAR szWinDir[MAX_PATH];
    CHAR szFile[MAX_PATH];
    LPVOID pVersionInfo = NULL;
    BOOL bFileCopied = FALSE;
    UINT uValueLen;

    TRACE("(%s, %p, %p, %d)\n", lpszFilename, pdwMSVer, pdwLSVer, bVersion);

    *pdwLSVer = 0;
    *pdwMSVer = 0;

    lstrcpynA(szFile, lpszFilename, MAX_PATH);

    dwInfoSize = GetFileVersionInfoSizeA(szFile, &dwHandle);
    if (!dwInfoSize)
    {
        /* check that the file exists */
        if (GetFileAttributesA(szFile) == INVALID_FILE_ATTRIBUTES)
            return S_OK;

        /* file exists, but won't be found by GetFileVersionInfoSize,
        * so copy it to the temp dir where it will be found.
        */
        GetWindowsDirectoryA(szWinDir, MAX_PATH);
        GetTempFileNameA(szWinDir, NULL, 0, szFile);
        CopyFileA(lpszFilename, szFile, FALSE);
        bFileCopied = TRUE;

        dwInfoSize = GetFileVersionInfoSizeA(szFile, &dwHandle);
        if (!dwInfoSize)
            goto done;
    }

    pVersionInfo = HeapAlloc(GetProcessHeap(), 0, dwInfoSize);
    if (!pVersionInfo)
        goto done;

    if (!GetFileVersionInfoA(szFile, dwHandle, dwInfoSize, pVersionInfo))
        goto done;

    if (bVersion)
    {
        if (!VerQueryValueA(pVersionInfo, "\\",
            (LPVOID *)&pFixedVersionInfo, &uValueLen))
            goto done;

        if (!uValueLen)
            goto done;

        *pdwMSVer = pFixedVersionInfo->dwFileVersionMS;
        *pdwLSVer = pFixedVersionInfo->dwFileVersionLS;
    }
    else
    {
        if (!VerQueryValueA(pVersionInfo, "\\VarFileInfo\\Translation",
             (LPVOID *)&pLangAndCodePage, &uValueLen))
            goto done;

        if (!uValueLen)
            goto done;

        *pdwMSVer = pLangAndCodePage->wLanguage;
        *pdwLSVer = pLangAndCodePage->wCodePage;
    }

done:
    HeapFree(GetProcessHeap(), 0, pVersionInfo);

    if (bFileCopied)
        DeleteFileA(szFile);

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
 * Deletes a file or directory, WinMain style.
 *
 * PARAMS
 *   hWnd    [I] Handle to the window used for the display.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order FileOrDirName,Flags.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented
 */
HRESULT WINAPI DelNodeRunDLL32( HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show )
{
    FIXME("(%s): stub\n", debugstr_a(cmdline));
    return E_FAIL;
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

    if (!SetupGetLineTextA(NULL, hInf, pszTranslateSection, pszTranslateKey,
                           pszBuffer, dwBufferSize, pdwRequiredSize))
    {
        if (dwBufferSize < *pdwRequiredSize)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        return SPAPI_E_LINE_NOT_FOUND;
    }

    return S_OK;
}

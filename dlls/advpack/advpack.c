/*
 * Advpack main
 *
 * Copyright 2004 Huw D M Davies
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);


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
 *		LaunchINFSection  (ADVPACK.@)
 */
void WINAPI LaunchINFSection( HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show )
{
    FIXME("%p %p %s %d\n", hWnd, hInst, debugstr_a(cmdline), show );
}

/***********************************************************************
 *		LaunchINFSectionEx  (ADVPACK.@)
 */
void WINAPI LaunchINFSectionEx( HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show )
{
    FIXME("%p %p %s %d\n", hWnd, hInst, debugstr_a(cmdline), show );
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
 *             NeedRebootInit  (ADVPACK.@)
 */
DWORD WINAPI NeedRebootInit(VOID)
{
    FIXME("() stub!\n");
    return 0;
}

/***********************************************************************
 *             NeedReboot      (ADVPACK.@)
 */
BOOL WINAPI NeedReboot(DWORD dwRebootCheck)
{
    FIXME("(0x%08lx) stub!\n", dwRebootCheck);
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

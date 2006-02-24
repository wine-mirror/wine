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
#include "setupapi.h"
#include "advpub.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

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
 *             ExecuteCabA    (ADVPACK.@)
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
HRESULT WINAPI ExecuteCabA( HWND hwnd, CABINFOA* pCab, LPVOID pReserved )
{
    FIXME("(%p %p %p): stub\n", hwnd, pCab, pReserved);
    return E_FAIL;
}

/***********************************************************************
 *      LaunchINFSectionA   (ADVPACK.@)
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
INT WINAPI LaunchINFSectionA( HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show )
{
    FIXME("(%p %p %s %d): stub\n", hWnd, hInst, debugstr_a(cmdline), show );
    return 0;
}

/***********************************************************************
 *      LaunchINFSectionExA (ADVPACK.@)
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
HRESULT WINAPI LaunchINFSectionExA( HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show )
{
    FIXME("(%p %p %s %d): stub\n", hWnd, hInst, debugstr_a(cmdline), show );
    return E_FAIL;
}

/***********************************************************************
 *      RunSetupCommandA  (ADVPACK.@)
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
HRESULT WINAPI RunSetupCommandA(HWND hWnd, LPCSTR szCmdName,
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

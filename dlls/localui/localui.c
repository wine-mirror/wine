/*
 * Implementation of the Local Printmonitor User Interface
 *
 * Copyright 2007 Detlef Riekenberg
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

#include <stdarg.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "localui.h"

WINE_DEFAULT_DEBUG_CHANNEL(localui);

/*****************************************************/

static HINSTANCE LOCALUI_hInstance;

static const WCHAR cmd_DeletePortW[] = {'D','e','l','e','t','e','P','o','r','t',0};
static const WCHAR cmd_GetDefaultCommConfigW[] = {'G','e','t',
                                    'D','e','f','a','u','l','t',
                                    'C','o','m','m','C','o','n','f','i','g',0};
static const WCHAR cmd_SetDefaultCommConfigW[] = {'S','e','t',
                                    'D','e','f','a','u','l','t',
                                    'C','o','m','m','C','o','n','f','i','g',0};

static const WCHAR portname_LPT[]  = {'L','P','T',0};
static const WCHAR portname_COM[]  = {'C','O','M',0};
static const WCHAR portname_FILE[] = {'F','I','L','E',':',0};
static const WCHAR portname_CUPS[] = {'C','U','P','S',':',0};
static const WCHAR portname_LPR[]  = {'L','P','R',':',0};

static const WCHAR XcvPortW[] = {',','X','c','v','P','o','r','t',' ',0};


/*****************************************************
 *   strdupWW [internal]
 */

static LPWSTR strdupWW(LPCWSTR pPrefix, LPCWSTR pSuffix)
{
    LPWSTR  ptr;
    DWORD   len;

    len = lstrlenW(pPrefix) + (pSuffix ? lstrlenW(pSuffix) : 0) + 1;
    ptr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (ptr) {
        lstrcpyW(ptr, pPrefix);
        if (pSuffix) lstrcatW(ptr, pSuffix);
    }
    return ptr;
}

/*****************************************************
 *   dlg_configure_com [internal]
 *
 */

static BOOL dlg_configure_com(HANDLE hXcv, HWND hWnd, PCWSTR pPortName)
{
    COMMCONFIG cfg;
    LPWSTR shortname;
    DWORD status;
    DWORD dummy;
    DWORD len;
    BOOL  res;

    /* strip the colon (pPortName is never empty here) */
    len = lstrlenW(pPortName);
    shortname = HeapAlloc(GetProcessHeap(), 0, len  * sizeof(WCHAR));
    if (shortname) {
        memcpy(shortname, pPortName, (len -1) * sizeof(WCHAR));
        shortname[len-1] = '\0';

        /* get current settings */
        len = sizeof(cfg);
        status = ERROR_SUCCESS;
        res = XcvDataW( hXcv, cmd_GetDefaultCommConfigW,
                        (PBYTE) shortname,
                        (lstrlenW(shortname) +1) * sizeof(WCHAR),
                        (PBYTE) &cfg, len, &len, &status);

        if (res && (status == ERROR_SUCCESS)) {
            /* display the Dialog */
            res = CommConfigDialogW(pPortName, hWnd, &cfg);
            if (res) {
                status = ERROR_SUCCESS;
                /* set new settings */
                res = XcvDataW(hXcv, cmd_SetDefaultCommConfigW,
                               (PBYTE) &cfg, len,
                               (PBYTE) &dummy, 0, &len, &status);
            }
        }
        HeapFree(GetProcessHeap(), 0, shortname);
        return res;
    }
    return FALSE;
}

/******************************************************************
 * display the Dialog "Nothing to configure"
 *
 */

static void dlg_nothingtoconfig(HWND hWnd)
{
    WCHAR res_PortW[IDS_LOCALPORT_MAXLEN];
    WCHAR res_nothingW[IDS_NOTHINGTOCONFIG_MAXLEN];

    res_PortW[0] = '\0';
    res_nothingW[0] = '\0';
    LoadStringW(LOCALUI_hInstance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN);
    LoadStringW(LOCALUI_hInstance, IDS_NOTHINGTOCONFIG, res_nothingW, IDS_NOTHINGTOCONFIG_MAXLEN);

    MessageBoxW(hWnd, res_nothingW, res_PortW, MB_OK | MB_ICONINFORMATION);
}

/*****************************************************
 * get_type_from_name (internal)
 *
 */

static DWORD get_type_from_name(LPCWSTR name)
{
    HANDLE  hfile;

    if (!strncmpiW(name, portname_LPT, sizeof(portname_LPT) / sizeof(WCHAR) -1))
        return PORT_IS_LPT;

    if (!strncmpiW(name, portname_COM, sizeof(portname_COM) / sizeof(WCHAR) -1))
        return PORT_IS_COM;

    if (!strcmpiW(name, portname_FILE))
        return PORT_IS_FILE;

    if (name[0] == '/')
        return PORT_IS_UNIXNAME;

    if (name[0] == '|')
        return PORT_IS_PIPE;

    if (!strncmpW(name, portname_CUPS, sizeof(portname_CUPS) / sizeof(WCHAR) -1))
        return PORT_IS_CUPS;

    if (!strncmpW(name, portname_LPR, sizeof(portname_LPR) / sizeof(WCHAR) -1))
        return PORT_IS_LPR;

    /* Must be a file or a directory. Does the file exist ? */
    hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    TRACE("%p for OPEN_EXISTING on %s\n", hfile, debugstr_w(name));
    if (hfile == INVALID_HANDLE_VALUE) {
        /* Can we create the file? */
        hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
        TRACE("%p for OPEN_ALWAYS\n", hfile);
    }
    if (hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(hfile);
        return PORT_IS_FILENAME;
    }
    /* We can't use the name. use GetLastError() for the reason */
    return PORT_IS_UNKNOWN;
}

/*****************************************************
 *   open_monitor_by_name [internal]
 *
 */
static BOOL open_monitor_by_name(LPCWSTR pPrefix, LPCWSTR pPort, HANDLE * phandle)
{
    PRINTER_DEFAULTSW pd;
    LPWSTR  fullname;
    BOOL    res;

    * phandle = 0;
    TRACE("(%s,%s)\n", debugstr_w(pPrefix),debugstr_w(pPort) );

    fullname = strdupWW(pPrefix, pPort);
    pd.pDatatype = NULL;
    pd.pDevMode  = NULL;
    pd.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    res = OpenPrinterW(fullname, phandle, &pd);
    HeapFree(GetProcessHeap(), 0, fullname);
    return res;
}

/*****************************************************
 *   localui_AddPortUI [exported through MONITORUI]
 *
 * Display a Dialog to add a local Port
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  hWnd        [I] Handle to parent Window for the Dialog-Box or NULL
 *  pMonitorName[I] Name of the Monitor, that should be used to add a Port or NULL
 *  ppPortName  [O] PTR to PTR of a buffer, that receive the Name of the new Port or NULL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localui_AddPortUI(PCWSTR pName, HWND hWnd, PCWSTR pMonitorName, PWSTR *ppPortName)
{
    FIXME("(%s, %p, %s, %p) stub\n", debugstr_w(pName), hWnd, debugstr_w(pMonitorName), ppPortName);
    return TRUE;
}


/*****************************************************
 *   localui_ConfigurePortUI [exported through MONITORUI]
 *
 * Display the Configuration-Dialog for a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box or NULL
 *  pPortName [I] Name of the Port, that should be configured
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localui_ConfigurePortUI(PCWSTR pName, HWND hWnd, PCWSTR pPortName)
{
    HANDLE  hXcv;
    DWORD   res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));
    if (open_monitor_by_name(XcvPortW, pPortName, &hXcv)) {

        res = get_type_from_name(pPortName);
        switch(res)
        {

        case PORT_IS_COM:
            res = dlg_configure_com(hXcv, hWnd, pPortName);
            break;

        default:
            dlg_nothingtoconfig(hWnd);
            SetLastError(ERROR_CANCELLED);
            res = FALSE;
        }

        ClosePrinter(hXcv);
        return res;
    }
    return FALSE;

}

/*****************************************************
 *   localui_DeletePortUI [exported through MONITORUI]
 *
 * Delete a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window
 *  pPortName [I] Name of the Port, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Native localui does not allow to delete a COM / LPT - Port (ERROR_NOT_SUPPORTED)
 *
 */
static BOOL WINAPI localui_DeletePortUI(PCWSTR pName, HWND hWnd, PCWSTR pPortName)
{
    HANDLE  hXcv;
    DWORD   dummy;
    DWORD   needed;
    DWORD   status;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    if ((!pPortName) || (!pPortName[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (open_monitor_by_name(XcvPortW, pPortName, &hXcv)) {
        /* native localui tests here for LPT / COM - Ports and failed with
           ERROR_NOT_SUPPORTED. */
        if (XcvDataW(hXcv, cmd_DeletePortW, (LPBYTE) pPortName,
            (lstrlenW(pPortName)+1) * sizeof(WCHAR), (LPBYTE) &dummy, 0, &needed, &status)) {

            ClosePrinter(hXcv);
            if (status != ERROR_SUCCESS) SetLastError(status);
            return (status == ERROR_SUCCESS);
        }
        ClosePrinter(hXcv);
        return FALSE;
    }
    SetLastError(ERROR_UNKNOWN_PORT);
    return FALSE;
}

/*****************************************************
 *      InitializePrintMonitorUI  (LOCALUI.@)
 *
 * Initialize the User-Interface for the Local Ports
 *
 * RETURNS
 *  Success: Pointer to a MONITORUI Structure
 *  Failure: NULL
 *
 */

PMONITORUI WINAPI InitializePrintMonitorUI(void)
{
    static MONITORUI mymonitorui =
    {
        sizeof(MONITORUI),
        localui_AddPortUI,
        localui_ConfigurePortUI,
        localui_DeletePortUI
    };

    TRACE("=> %p\n", &mymonitorui);
    return &mymonitorui;
}

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;           /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            LOCALUI_hInstance = hinstDLL;
            break;
    }
    return TRUE;
}

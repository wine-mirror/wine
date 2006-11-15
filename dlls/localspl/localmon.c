/*
 * Implementation of the Local Printmonitor
 *
 * Copyright 2006 Detlef Riekenberg
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winver.h"
#include "winnls.h"

#include "winspool.h"
#include "ddk/winsplp.h"
#include "localspl_private.h"

#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(localspl);

/*****************************************************/

static const WCHAR WinNT_CV_PortsW[] = {'S','o','f','t','w','a','r','e','\\',
                                        'M','i','c','r','o','s','o','f','t','\\',
                                        'W','i','n','d','o','w','s',' ','N','T','\\',
                                        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                        'P','o','r','t','s',0};

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
    LoadStringW(LOCALSPL_hInstance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN);  
    LoadStringW(LOCALSPL_hInstance, IDS_NOTHINGTOCONFIG, res_nothingW, IDS_NOTHINGTOCONFIG_MAXLEN);  

    MessageBoxW(hWnd, res_nothingW, res_PortW, MB_OK | MB_ICONINFORMATION);
}

/******************************************************************
 * enumerate the local Ports from the Registry (internal)  
 *
 * See localmon_EnumPortsW.
 *
 * NOTES
 *  returns the needed size (in bytes) for pPorts
 *  and *lpreturned is set to number of entries returned in pPorts
 *
 */

static DWORD get_ports_from_reg(DWORD level, LPBYTE pPorts, DWORD cbBuf, LPDWORD lpreturned)
{
    HKEY    hroot = 0;
    LPWSTR  ptr;
    LPPORT_INFO_2W out;
    WCHAR   portname[MAX_PATH];
    WCHAR   res_PortW[32];
    WCHAR   res_MonitorW[32];
    INT     reslen_PortW;
    INT     reslen_MonitorW;
    DWORD   len;
    DWORD   res;
    DWORD   needed = 0;
    DWORD   numentries;
    DWORD   entrysize;
    DWORD   id = 0;

    TRACE("(%d, %p, %d, %p)\n", level, pPorts, cbBuf, lpreturned);

    entrysize = (level == 1) ? sizeof(PORT_INFO_1W) : sizeof(PORT_INFO_2W);

    numentries = *lpreturned;           /* this is 0, when we scan the registry */
    needed = entrysize * numentries;
    ptr = (LPWSTR) &pPorts[needed];

    if (needed > cbBuf) pPorts = NULL;  /* No buffer for the structs */

    numentries = 0;
    needed = 0;

    /* we do not check more parameters as done in windows */
    if ((level < 1) || (level > 2)) {
        goto getports_cleanup;
    }

    /* "+1" for '\0' */
    reslen_MonitorW = LoadStringW(LOCALSPL_hInstance, IDS_LOCALMONITOR, res_MonitorW, 32) + 1;  
    reslen_PortW = LoadStringW(LOCALSPL_hInstance, IDS_LOCALPORT, res_PortW, 32) + 1;  

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_PortsW, &hroot);
    if (res == ERROR_SUCCESS) {

        /* Scan all Port-Names */
        while (res == ERROR_SUCCESS) {
            len = MAX_PATH;
            portname[0] = '\0';
            res = RegEnumValueW(hroot, id, portname, &len, NULL, NULL, NULL, NULL);

            if ((res == ERROR_SUCCESS) && (portname[0])) {
                numentries++;
                /* calsulate the required size */
                needed += entrysize;
                needed += (len + 1) * sizeof(WCHAR);
                if (level > 1) {
                    needed += (reslen_MonitorW + reslen_PortW) * sizeof(WCHAR);
                }

                /* Now fill the user-buffer, if available */
                if (pPorts && (cbBuf >= needed)){
                    out = (LPPORT_INFO_2W) pPorts;
                    pPorts += entrysize;
                    TRACE("%p: writing PORT_INFO_%dW #%d (%s)\n", out, level, numentries, debugstr_w(portname));
                    out->pPortName = ptr;
                    lstrcpyW(ptr, portname);            /* Name of the Port */
                    ptr += (len + 1);
                    if (level > 1) {
                        out->pMonitorName = ptr;
                        lstrcpyW(ptr, res_MonitorW);    /* Name of the Monitor */
                        ptr += reslen_MonitorW;

                        out->pDescription = ptr;
                        lstrcpyW(ptr, res_PortW);       /* Port Description */
                        ptr += reslen_PortW;

                        out->fPortType = PORT_TYPE_WRITE;
                        out->Reserved = 0;
                    }
                }
                id++;
            }
        }
        RegCloseKey(hroot);
    }
    else
    {
        ERR("failed with %d for %s\n", res, debugstr_w(WinNT_CV_PortsW));
        SetLastError(res);
    }

getports_cleanup:
    *lpreturned = numentries;
    TRACE("need %d byte for %d entries (%d)\n", needed, numentries, GetLastError());
    return needed;
}

/*****************************************************
 *   localmon_ConfigurePortW [exported through MONITOREX]
 *
 * Display the Configuration-Dialog for a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box
 *  pPortName [I] Name of the Port, that should be configured
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
BOOL WINAPI localmon_ConfigurePortW(LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));
    /* ToDo: Dialogs by Portname ("LPTx:", "COMx:") */

    dlg_nothingtoconfig(hWnd);
    return ROUTER_SUCCESS;
}

/*****************************************************
 *   localmon_EnumPortsW [exported through MONITOREX]
 *
 * Enumerate all local Ports
 *
 * PARAMS
 *  pName       [I] Servername (ignored)
 *  level       [I] Structure-Level (1 or 2)
 *  pPorts      [O] PTR to Buffer that receives the Result
 *  cbBuf       [I] Size of Buffer at pPorts
 *  pcbNeeded   [O] PTR to DWORD that receives the size in Bytes used / required for pPorts
 *  pcReturned  [O] PTR to DWORD that receives the number of Ports in pPorts
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPorts, if cbBuf is too small
 *
 * NOTES
 *|  Windows ignores pName
 *|  Windows crash the app, when pPorts, pcbNeeded or pcReturned are NULL
 *|  Windows >NT4.0 does not check for illegal levels (TRUE is returned)
 *
 * ToDo
 *   "HCU\Software\Wine\Spooler\<portname>" - redirection
 *
 */
BOOL WINAPI localmon_EnumPortsW(LPWSTR pName, DWORD level, LPBYTE pPorts,
                            DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL    res = FALSE;
    DWORD   needed;
    DWORD   numentries;

    TRACE("(%s, %d, %p, %d, %p, %p)\n",
          debugstr_w(pName), level, pPorts, cbBuf, pcbNeeded, pcReturned);

    numentries = 0;
    needed = get_ports_from_reg(level, NULL, 0, &numentries);
    /* we calculated the needed buffersize. now do the error-checks */
    if (cbBuf < needed) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto cleanup;
    }

    /* fill the buffer with the Port-Names */
    needed = get_ports_from_reg(level, pPorts, cbBuf, &numentries);
    res = TRUE;

    if (pcReturned) *pcReturned = numentries;

cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;

    TRACE("returning %d with %d (%d byte for %d entries)\n", 
            res, GetLastError(), needed, numentries);

    return (res);
}

/*****************************************************
 *      InitializePrintMonitor  (LOCALSPL.@)
 *
 * Initialize the Monitor for the Local Ports
 *
 * PARAMS
 *  regroot [I] Registry-Path, where the settings are stored
 *
 * RETURNS
 *  Success: Pointer to a MONITOREX Structure
 *  Failure: NULL
 *
 * NOTES
 *  The fixed location "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports"
 *  is used to store the Ports (IniFileMapping from "win.ini", Section "Ports").
 *  Native localspl.dll fails, when no valid Port-Entry is present.
 *
 */

LPMONITOREX WINAPI InitializePrintMonitor(LPWSTR regroot)
{
    static MONITOREX mymonitorex =
    {
        sizeof(MONITOREX) - sizeof(DWORD),
        {
            localmon_EnumPortsW,
            NULL,       /* localmon_OpenPortW */ 
            NULL,       /* localmon_OpenPortExW */ 
            NULL,       /* localmon_StartDocPortW */
            NULL,       /* localmon_WritePortW */
            NULL,       /* localmon_ReadPortW */
            NULL,       /* localmon_EndDocPortW */
            NULL,       /* localmon_ClosePortW */
            NULL,       /* localmon_AddPortW */
            NULL,       /* localmon_AddPortExW */
            localmon_ConfigurePortW
        }
    };

    TRACE("(%s)\n", debugstr_w(regroot));
    /* Parameter "regroot" is ignored on NT4.0 (localmon.dll) */
    if (!regroot || !regroot[0]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    TRACE("=> %p\n", &mymonitorex);
    /* Native windows returns always the same pointer on success */
    return &mymonitorex;
}

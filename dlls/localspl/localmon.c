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
#include <stdlib.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"

#include "winspool.h"
#include "ddk/winsplp.h"
#include "localspl_private.h"

#include "wine/debug.h"
#include "wine/list.h"


WINE_DEFAULT_DEBUG_CHANNEL(localspl);

/*****************************************************/

static CRITICAL_SECTION port_handles_cs;
static CRITICAL_SECTION_DEBUG port_handles_cs_debug =
{
    0, 0, &port_handles_cs,
    { &port_handles_cs_debug.ProcessLocksList, &port_handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": port_handles_cs") }
};
static CRITICAL_SECTION port_handles_cs = { &port_handles_cs_debug, -1, 0, 0, 0, 0 };


static CRITICAL_SECTION xcv_handles_cs;
static CRITICAL_SECTION_DEBUG xcv_handles_cs_debug =
{
    0, 0, &xcv_handles_cs,
    { &xcv_handles_cs_debug.ProcessLocksList, &xcv_handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": xcv_handles_cs") }
};
static CRITICAL_SECTION xcv_handles_cs = { &xcv_handles_cs_debug, -1, 0, 0, 0, 0 };

/* ############################### */

typedef struct {
    struct list entry;
    DWORD   type;
    HANDLE  hfile;
    DWORD   thread_id;
    INT64   doc_handle;
    WCHAR   nameW[1];
} port_t;

typedef struct {
    struct list entry;
    ACCESS_MASK GrantedAccess;
    WCHAR       nameW[1];
} xcv_t;

static struct list port_handles = LIST_INIT( port_handles );
static struct list xcv_handles = LIST_INIT( xcv_handles );

static const WCHAR WinNT_CV_PortsW[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Ports";
static const WCHAR WinNT_CV_WindowsW[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";

HINSTANCE localspl_instance;

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %ld, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            localspl_instance = hinstDLL;
            if (__wine_init_unix_call())
                return FALSE;
            UNIX_CALL(process_attach, NULL);
            break;
    }
    return TRUE;
}

/******************************************************************
 * does_port_exist (internal)
 *
 * returns TRUE, when the Port already exists
 *
 */
static BOOL does_port_exist(LPCWSTR myname)
{

    LPPORT_INFO_1W  pi;
    DWORD   needed = 0;
    DWORD   returned;
    DWORD   id;

    TRACE("(%s)\n", debugstr_w(myname));

    id = EnumPortsW(NULL, 1, NULL, 0, &needed, &returned);
    pi = malloc(needed);
    returned = 0;
    if (pi)
        id = EnumPortsW(NULL, 1, (LPBYTE) pi, needed, &needed, &returned);

    if (id && returned > 0) {
        /* we got a number of valid names. */
        for (id = 0; id < returned; id++)
        {
            if (lstrcmpiW(myname, pi[id].pName) == 0) {
                TRACE("(%lu) found %s\n", id, debugstr_w(pi[id].pName));
                free(pi);
                return TRUE;
            }
        }
    }

    free(pi);
    return FALSE;
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
    WCHAR   res_PortW[IDS_LOCALPORT_MAXLEN];
    WCHAR   res_MonitorW[IDS_LOCALMONITOR_MAXLEN];
    INT     reslen_PortW;
    INT     reslen_MonitorW;
    DWORD   len;
    DWORD   res;
    DWORD   needed = 0;
    DWORD   numentries;
    DWORD   entrysize;
    DWORD   id = 0;

    TRACE("(%ld, %p, %ld, %p)\n", level, pPorts, cbBuf, lpreturned);

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
    reslen_MonitorW = LoadStringW(localspl_instance, IDS_LOCALMONITOR, res_MonitorW, IDS_LOCALMONITOR_MAXLEN) + 1;
    reslen_PortW = LoadStringW(localspl_instance, IDS_LOCALPORT, res_PortW, IDS_LOCALPORT_MAXLEN) + 1;

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_PortsW, &hroot);
    if (res == ERROR_SUCCESS) {

        /* Scan all Port-Names */
        while (res == ERROR_SUCCESS) {
            len = MAX_PATH;
            portname[0] = '\0';
            res = RegEnumValueW(hroot, id, portname, &len, NULL, NULL, NULL, NULL);

            if ((res == ERROR_SUCCESS) && (portname[0])) {
                numentries++;
                /* calculate the required size */
                needed += entrysize;
                needed += (len + 1) * sizeof(WCHAR);
                if (level > 1) {
                    needed += (reslen_MonitorW + reslen_PortW) * sizeof(WCHAR);
                }

                /* Now fill the user-buffer, if available */
                if (pPorts && (cbBuf >= needed)){
                    out = (LPPORT_INFO_2W) pPorts;
                    pPorts += entrysize;
                    TRACE("%p: writing PORT_INFO_%ldW #%ld (%s)\n", out, level, numentries, debugstr_w(portname));
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
        ERR("failed with %ld for %s\n", res, debugstr_w(WinNT_CV_PortsW));
        SetLastError(res);
    }

getports_cleanup:
    *lpreturned = numentries;
    TRACE("need %ld byte for %ld entries (%ld)\n", needed, numentries, GetLastError());
    return needed;
}

/*****************************************************
 * get_type_from_name (internal)
 * 
 */

static DWORD get_type_from_name(LPCWSTR name, BOOL check_filename)
{
    HANDLE  hfile;

    if (!wcsncmp(name, L"LPT", ARRAY_SIZE(L"LPT") - 1))
        return PORT_IS_LPT;

    if (!wcsncmp(name, L"COM", ARRAY_SIZE(L"COM") - 1))
        return PORT_IS_COM;

    if (!lstrcmpW(name, L"FILE:"))
        return PORT_IS_FILE;

    if (name[0] == '/')
        return PORT_IS_UNIXNAME;

    if (name[0] == '|')
        return PORT_IS_PIPE;

    if (!wcsncmp(name, L"CUPS:", ARRAY_SIZE(L"CUPS:") - 1))
        return PORT_IS_CUPS;

    if (!wcsncmp(name, L"LPR:", ARRAY_SIZE(L"LPR:") - 1))
        return PORT_IS_LPR;

    if (!check_filename)
        return PORT_IS_UNKNOWN;

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
 * get_type_from_local_name (internal)
 *
 */

static DWORD get_type_from_local_name(LPCWSTR nameW)
{
    LPPORT_INFO_1W  pi;
    LPWSTR  myname = NULL;
    DWORD   needed = 0;
    DWORD   numentries = 0;
    DWORD   id;

    TRACE("(%s)\n", debugstr_w(nameW));

    if ((id = get_type_from_name(nameW, FALSE)) >= PORT_IS_WINE)
        return id;

    needed = get_ports_from_reg(1, NULL, 0, &numentries);
    pi = malloc(needed);
    if (pi)
        needed = get_ports_from_reg(1, (LPBYTE) pi, needed, &numentries);

    if (pi && needed && numentries > 0) {
        /* we got a number of valid ports. */

        for (id = 0; id < numentries; id++)
        {
            if (lstrcmpiW(nameW, pi[id].pName) == 0) {
                TRACE("(%lu) found %s\n", id, debugstr_w(pi[id].pName));
                myname = pi[id].pName;
                break;
            }
        }
    }

    id = myname ? get_type_from_name(myname, TRUE) : PORT_IS_UNKNOWN;

    free(pi);
    return id;

}
/******************************************************************************
 *   localmon_AddPortExW [exported through MONITOREX]
 *
 * Add a Port, without presenting a user interface
 *
 * PARAMS
 *  pName         [I] Servername or NULL (local Computer)
 *  level         [I] Structure-Level (1) for pBuffer
 *  pBuffer       [I] PTR to the Input-Data (PORT_INFO_1)
 *  pMonitorName  [I] Name of the Monitor that manage the Port
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Level 2 is documented on MSDN for Portmonitors, but not supported by the
 *  "Local Port" Portmonitor (localspl.dll / localmon.dll)
 */
static BOOL WINAPI localmon_AddPortExW(LPWSTR pName, DWORD level, LPBYTE pBuffer, LPWSTR pMonitorName)
{
    PORT_INFO_1W * pi;
    HKEY  hroot;
    DWORD res;

    pi = (PORT_INFO_1W *) pBuffer;
    TRACE("(%s, %ld, %p, %s) => %s\n", debugstr_w(pName), level, pBuffer,
            debugstr_w(pMonitorName), debugstr_w(pi ? pi->pName : NULL));


    if ((pMonitorName == NULL) || (lstrcmpiW(pMonitorName, L"Local Port") != 0 ) ||
        (pi == NULL) || (pi->pName == NULL) || (pi->pName[0] == '\0') ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_PortsW, &hroot);
    if (res == ERROR_SUCCESS) {
        if (does_port_exist(pi->pName)) {
            RegCloseKey(hroot);
            TRACE("=> FALSE with %u\n", ERROR_INVALID_PARAMETER);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        res = RegSetValueExW(hroot, pi->pName, 0, REG_SZ, (const BYTE *) L"", sizeof(L""));
        RegCloseKey(hroot);
    }
    if (res != ERROR_SUCCESS) SetLastError(ERROR_INVALID_PARAMETER);
    TRACE("=> %u with %lu\n", (res == ERROR_SUCCESS), GetLastError());
    return (res == ERROR_SUCCESS);
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
static BOOL WINAPI localmon_EnumPortsW(LPWSTR pName, DWORD level, LPBYTE pPorts,
                                       DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL    res = FALSE;
    DWORD   needed;
    DWORD   numentries;

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n",
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

    TRACE("returning %d with %ld (%ld byte for %ld entries)\n",
            res, GetLastError(), needed, numentries);

    return (res);
}

/*****************************************************
 * localmon_OpenPort [exported through MONITOREX]
 *
 * Open a Data-Channel for a Port
 *
 * PARAMS
 *  pName     [i] Name of selected Object
 *  phPort    [o] The resulting Handle is stored here
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localmon_OpenPortW(LPWSTR pName, PHANDLE phPort)
{
    port_t * port;
    DWORD   type;

    TRACE("%s, %p)\n", debugstr_w(pName), phPort);

    /* an empty name is invalid */
    if (!pName[0]) return FALSE;

    /* does the port exist? */
    type = get_type_from_local_name(pName);
    if (!type) return FALSE;

    port = malloc(FIELD_OFFSET(port_t, nameW[wcslen(pName) + 1]));
    if (!port) return FALSE;

    port->type = type;
    port->hfile = INVALID_HANDLE_VALUE;
    port->doc_handle = 0;
    lstrcpyW(port->nameW, pName);
    *phPort = port;

    EnterCriticalSection(&port_handles_cs);
    list_add_tail(&port_handles, &port->entry);
    LeaveCriticalSection(&port_handles_cs);

    TRACE("=> %p\n", port);
    return TRUE;
}

static BOOL WINAPI localmon_StartDocPort(HANDLE hport, WCHAR *printer_name,
        DWORD job_id, DWORD level, BYTE *info)
{
    DOC_INFO_1W *doc_info = (DOC_INFO_1W *)info;
    port_t *port = hport;

    TRACE("(%p %s %ld %ld %p)\n", hport, debugstr_w(printer_name),
            job_id, level, doc_info);

    if (port->type >= PORT_IS_WINE)
    {
        struct start_doc_params params;

        if (port->doc_handle)
            return TRUE;

        port->thread_id = GetCurrentThreadId();

        params.type = port->type;
        params.port = port->nameW;
        params.document_title = doc_info ? doc_info->pDocName : NULL;
        params.doc = &port->doc_handle;
        return UNIX_CALL(start_doc, &params);
    }

    if (port->type != PORT_IS_FILE)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (port->hfile != INVALID_HANDLE_VALUE)
        return TRUE;

    if (!doc_info || !doc_info->pOutputFile)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    port->hfile = CreateFileW(doc_info->pOutputFile, GENERIC_WRITE,
            FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    return port->hfile != INVALID_HANDLE_VALUE;
}

static BOOL WINAPI localmon_WritePort(HANDLE hport, BYTE *buf, DWORD size,
        DWORD *written)
{
    port_t *port = hport;

    TRACE("(%p %p %lu %p)\n", hport, buf, size, written);

    if (port->type >= PORT_IS_WINE)
    {
        struct write_doc_params params;
        BOOL ret;

        if (!port->doc_handle)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        if (port->type == PORT_IS_CUPS && port->thread_id != GetCurrentThreadId())
        {
            FIXME("used from other thread\n");
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return FALSE;
        }

        params.doc = port->doc_handle;
        params.buf = buf;
        params.size = size;
        ret = UNIX_CALL(write_doc, &params);
        *written = ret ? size : 0;
        return ret;
    }

    return WriteFile(port->hfile, buf, size, written, NULL);
}

static BOOL WINAPI localmon_EndDocPort(HANDLE hport)
{
    port_t *port = hport;

    TRACE("(%p)\n", hport);

    if (port->type >= PORT_IS_WINE)
    {
        struct end_doc_params params;

        if (!port->doc_handle)
            return TRUE;

        if (port->type == PORT_IS_CUPS && port->thread_id != GetCurrentThreadId())
        {
            FIXME("used from other thread\n");
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return FALSE;
        }

        params.doc = port->doc_handle;
        port->doc_handle = 0;
        return UNIX_CALL(end_doc, &params);
    }

    CloseHandle(port->hfile);
    port->hfile = INVALID_HANDLE_VALUE;
    return TRUE;
}

/*****************************************************
 * localmon_ClosePort [exported through MONITOREX]
 *
 * Close a Port
 *
 * PARAMS
 *  hport  [i] The Handle to close
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localmon_ClosePort(HANDLE hport)
{
    port_t *port = hport;

    TRACE("(%p)\n", port);

    localmon_EndDocPort(hport);

    EnterCriticalSection(&port_handles_cs);
    list_remove(&port->entry);
    LeaveCriticalSection(&port_handles_cs);
    free(port);
    return TRUE;
}

/*****************************************************
 * localmon_XcvClosePort [exported through MONITOREX]
 *
 * Close a Communication-Channel
 *
 * PARAMS
 *  hXcv  [i] The Handle to close
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localmon_XcvClosePort(HANDLE hXcv)
{
    xcv_t * xcv = hXcv;

    TRACE("(%p)\n", xcv);
    /* No checks are done in Windows */
    EnterCriticalSection(&xcv_handles_cs);
    list_remove(&xcv->entry);
    LeaveCriticalSection(&xcv_handles_cs);
    free(xcv);
    return TRUE;
}

/*****************************************************
 * localmon_XcvDataPort [exported through MONITOREX]
 *
 * Execute command through a Communication-Channel
 *
 * PARAMS
 *  hXcv            [i] The Handle to work with
 *  pszDataName     [i] Name of the command to execute
 *  pInputData      [i] Buffer for extra Input Data (needed only for some commands)
 *  cbInputData     [i] Size in Bytes of Buffer at pInputData
 *  pOutputData     [o] Buffer to receive additional Data (needed only for some commands)
 *  cbOutputData    [i] Size in Bytes of Buffer at pOutputData
 *  pcbOutputNeeded [o] PTR to receive the minimal Size in Bytes of the Buffer at pOutputData
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: win32 error code
 *
 * NOTES
 *
 *  Minimal List of commands, that every Printmonitor DLL should support:
 *
 *| "MonitorUI" : Return the Name of the Userinterface-DLL as WSTR in pOutputData
 *| "AddPort"   : Add a Port (Name as WSTR in pInputData)
 *| "DeletePort": Delete a Port (Name as WSTR in pInputData)
 *
 *
 */
static DWORD WINAPI localmon_XcvDataPort(HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData,
                                         PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    WCHAR   buffer[16];     /* buffer for a decimal number */
    LPWSTR  ptr;
    DWORD   res;
    DWORD   needed;
    HKEY    hroot;

    TRACE("(%p, %s, %p, %ld, %p, %ld, %p)\n", hXcv, debugstr_w(pszDataName),
          pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    if (!lstrcmpW(pszDataName, L"AddPort")) {
        TRACE("InputData (%ld): %s\n", cbInputData, debugstr_w( (LPWSTR) pInputData));
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_PortsW, &hroot);
        if (res == ERROR_SUCCESS) {
            if (does_port_exist((LPWSTR) pInputData)) {
                RegCloseKey(hroot);
                TRACE("=> %u\n", ERROR_ALREADY_EXISTS);
                return ERROR_ALREADY_EXISTS;
            }
            res = RegSetValueExW(hroot, (LPWSTR)pInputData, 0, REG_SZ, (const BYTE*)L"", sizeof(L""));
            RegCloseKey(hroot);
        }
        TRACE("=> %lu\n", res);
        return res;
    }


    if (!lstrcmpW(pszDataName, L"ConfigureLPTPortCommandOK")) {
        TRACE("InputData (%ld): %s\n", cbInputData, debugstr_w( (LPWSTR) pInputData));
        res = RegCreateKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_WindowsW, &hroot);
        if (res == ERROR_SUCCESS) {
            res = RegSetValueExW(hroot, L"TransmissionRetryTimeout", 0, REG_SZ, pInputData, cbInputData);
            RegCloseKey(hroot);
        }
        return res;
    }

    if (!lstrcmpW(pszDataName, L"DeletePort")) {
        TRACE("InputData (%ld): %s\n", cbInputData, debugstr_w( (LPWSTR) pInputData));
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_PortsW, &hroot);
        if (res == ERROR_SUCCESS) {
            res = RegDeleteValueW(hroot, (LPWSTR) pInputData);
            RegCloseKey(hroot);
            TRACE("=> %lu with %lu\n", res, GetLastError() );
            return res;
        }
        return ERROR_FILE_NOT_FOUND;
    }

    if (!lstrcmpW(pszDataName, L"GetDefaultCommConfig")) {
        TRACE("InputData (%ld): %s\n", cbInputData, debugstr_w( (LPWSTR) pInputData));
        *pcbOutputNeeded = cbOutputData;
        res = GetDefaultCommConfigW((LPWSTR) pInputData, (LPCOMMCONFIG) pOutputData, pcbOutputNeeded);
        TRACE("got %lu with %lu\n", res, GetLastError() );
        return res ? ERROR_SUCCESS : GetLastError();
    }

    if (!lstrcmpW(pszDataName, L"GetTransmissionRetryTimeout")) {
        * pcbOutputNeeded = sizeof(DWORD);
        if (cbOutputData >= sizeof(DWORD)) {
            /* the w2k resource kit documented a default of 90, but that's wrong */
            *((LPDWORD) pOutputData) = 45;

            res = RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_WindowsW, &hroot);
            if (res == ERROR_SUCCESS) {
                needed = sizeof(buffer) - sizeof(WCHAR);
                res = RegQueryValueExW(hroot, L"TransmissionRetryTimeout", NULL, NULL, (BYTE*)buffer, &needed);
                if ((res == ERROR_SUCCESS) && (buffer[0])) {
                    *((LPDWORD) pOutputData) = wcstoul(buffer, NULL, 0);
                }
                RegCloseKey(hroot);
            }
            return ERROR_SUCCESS;
        }
        return ERROR_INSUFFICIENT_BUFFER;
    }


    if (!lstrcmpW(pszDataName, L"MonitorUI")) {
        * pcbOutputNeeded = sizeof(L"localui.dll");
        if (cbOutputData >= sizeof(L"localui.dll")) {
            memcpy(pOutputData, L"localui.dll", sizeof(L"localui.dll"));
            return ERROR_SUCCESS;
        }
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if (!lstrcmpW(pszDataName, L"PortIsValid")) {
        TRACE("InputData (%ld): %s\n", cbInputData, debugstr_w( (LPWSTR) pInputData));
        res = get_type_from_name((LPCWSTR) pInputData, TRUE);
        TRACE("detected as %lu\n",  res);
        /* names, that we have recognized, are valid */
        if (res) return ERROR_SUCCESS;

        /* ERROR_ACCESS_DENIED, ERROR_PATH_NOT_FOUND or something else */
        TRACE("=> %lu\n", GetLastError());
        return GetLastError();
    }

    if (!lstrcmpW(pszDataName, L"SetDefaultCommConfig")) {
        /* get the portname from the Handle */
        ptr =  wcschr(((xcv_t *)hXcv)->nameW, ' ');
        if (ptr) {
            ptr++;  /* skip the space */
        }
        else
        {
            ptr =  ((xcv_t *)hXcv)->nameW;
        }
        lstrcpynW(buffer, ptr, ARRAY_SIZE(buffer));
        if (buffer[0]) buffer[wcslen(buffer)-1] = '\0';  /* remove the ':' */
        res = SetDefaultCommConfigW(buffer, (LPCOMMCONFIG) pInputData, cbInputData);
        TRACE("got %lu with %lu\n", res, GetLastError() );
        return res ? ERROR_SUCCESS : GetLastError();
    }

    FIXME("command not supported: %s\n", debugstr_w(pszDataName));
    return ERROR_INVALID_PARAMETER;
}

/*****************************************************
 * localmon_XcvOpenPort [exported through MONITOREX]
 *
 * Open a Communication-Channel
 *
 * PARAMS
 *  pName         [i] Name of selected Object
 *  GrantedAccess [i] Access-Rights to use
 *  phXcv         [o] The resulting Handle is stored here
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localmon_XcvOpenPort(LPCWSTR pName, ACCESS_MASK GrantedAccess, PHANDLE phXcv)
{
    xcv_t * xcv;

    TRACE("%s, 0x%lx, %p)\n", debugstr_w(pName), GrantedAccess, phXcv);
    /* No checks for any field is done in Windows */
    xcv = malloc(FIELD_OFFSET(xcv_t, nameW[wcslen(pName) + 1]));
    if (xcv) {
        xcv->GrantedAccess = GrantedAccess;
        lstrcpyW(xcv->nameW, pName);
        *phXcv = xcv;
        EnterCriticalSection(&xcv_handles_cs);
        list_add_tail(&xcv_handles, &xcv->entry);
        LeaveCriticalSection(&xcv_handles_cs);
        TRACE("=> %p\n", xcv);
        return TRUE;
    }
    else
    {
        *phXcv = NULL;
        return FALSE;
    }
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
        sizeof(MONITOR),
        {
            localmon_EnumPortsW,
            localmon_OpenPortW,
            NULL,       /* localmon_OpenPortExW */ 
            localmon_StartDocPort,
            localmon_WritePort,
            NULL,       /* localmon_ReadPortW */
            localmon_EndDocPort,
            localmon_ClosePort,
            NULL,       /* Use AddPortUI in localui.dll */
            localmon_AddPortExW,
            NULL,       /* Use ConfigurePortUI in localui.dll */
            NULL,       /* Use DeletePortUI in localui.dll */
            NULL,       /* localmon_GetPrinterDataFromPort */
            NULL,       /* localmon_SetPortTimeOuts */
            localmon_XcvOpenPort,
            localmon_XcvDataPort,
            localmon_XcvClosePort
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

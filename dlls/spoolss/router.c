/*
 * Routing for Spooler-Service helper DLL
 *
 * Copyright 2006-2009 Detlef Riekenberg
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "wingdi.h"
#include "winspool.h"
#include "ddk/winsplp.h"
#include "spoolss.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(spoolss);

/* ################################ */

#define MAX_BACKEND 3

typedef struct {
    /* PRINTPROVIDOR functions */
    DWORD  (WINAPI *fpOpenPrinter)(LPWSTR, HANDLE *, LPPRINTER_DEFAULTSW);
    DWORD  (WINAPI *fpSetJob)(HANDLE, DWORD, DWORD, LPBYTE, DWORD);
    DWORD  (WINAPI *fpGetJob)(HANDLE, DWORD, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpEnumJobs)(HANDLE, DWORD, DWORD, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    HANDLE (WINAPI *fpAddPrinter)(LPWSTR, DWORD, LPBYTE);
    DWORD  (WINAPI *fpDeletePrinter)(HANDLE);
    DWORD  (WINAPI *fpSetPrinter)(HANDLE, DWORD, LPBYTE, DWORD);
    DWORD  (WINAPI *fpGetPrinter)(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpEnumPrinters)(DWORD, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpAddPrinterDriver)(LPWSTR, DWORD, LPBYTE);
    DWORD  (WINAPI *fpEnumPrinterDrivers)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpGetPrinterDriver)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpGetPrinterDriverDirectory)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpDeletePrinterDriver)(LPWSTR, LPWSTR, LPWSTR);
    DWORD  (WINAPI *fpAddPrintProcessor)(LPWSTR, LPWSTR, LPWSTR, LPWSTR);
    DWORD  (WINAPI *fpEnumPrintProcessors)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpGetPrintProcessorDirectory)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpDeletePrintProcessor)(LPWSTR, LPWSTR, LPWSTR);
    DWORD  (WINAPI *fpEnumPrintProcessorDatatypes)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpStartDocPrinter)(HANDLE, DWORD, LPBYTE);
    DWORD  (WINAPI *fpStartPagePrinter)(HANDLE);
    DWORD  (WINAPI *fpWritePrinter)(HANDLE, LPVOID, DWORD, LPDWORD);
    DWORD  (WINAPI *fpEndPagePrinter)(HANDLE);
    DWORD  (WINAPI *fpAbortPrinter)(HANDLE);
    DWORD  (WINAPI *fpReadPrinter)(HANDLE, LPVOID, DWORD, LPDWORD);
    DWORD  (WINAPI *fpEndDocPrinter)(HANDLE);
    DWORD  (WINAPI *fpAddJob)(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpScheduleJob)(HANDLE, DWORD);
    DWORD  (WINAPI *fpGetPrinterData)(HANDLE, LPWSTR, LPDWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpSetPrinterData)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD);
    DWORD  (WINAPI *fpWaitForPrinterChange)(HANDLE, DWORD);
    DWORD  (WINAPI *fpClosePrinter)(HANDLE);
    DWORD  (WINAPI *fpAddForm)(HANDLE, DWORD, LPBYTE);
    DWORD  (WINAPI *fpDeleteForm)(HANDLE, LPWSTR);
    DWORD  (WINAPI *fpGetForm)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpSetForm)(HANDLE, LPWSTR, DWORD, LPBYTE);
    DWORD  (WINAPI *fpEnumForms)(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpEnumMonitors)(LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpEnumPorts)(LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpAddPort)(LPWSTR, HWND, LPWSTR);
    DWORD  (WINAPI *fpConfigurePort)(LPWSTR, HWND, LPWSTR);
    DWORD  (WINAPI *fpDeletePort)(LPWSTR, HWND, LPWSTR);
    HANDLE (WINAPI *fpCreatePrinterIC)(HANDLE, LPDEVMODEW);
    DWORD  (WINAPI *fpPlayGdiScriptOnPrinterIC)(HANDLE, LPBYTE, DWORD, LPBYTE, DWORD, DWORD);
    DWORD  (WINAPI *fpDeletePrinterIC)(HANDLE);
    DWORD  (WINAPI *fpAddPrinterConnection)(LPWSTR);
    DWORD  (WINAPI *fpDeletePrinterConnection)(LPWSTR);
    DWORD  (WINAPI *fpPrinterMessageBox)(HANDLE, DWORD, HWND, LPWSTR, LPWSTR, DWORD);
    DWORD  (WINAPI *fpAddMonitor)(LPWSTR, DWORD, LPBYTE);
    DWORD  (WINAPI *fpDeleteMonitor)(LPWSTR, LPWSTR, LPWSTR);
    DWORD  (WINAPI *fpResetPrinter)(HANDLE, LPPRINTER_DEFAULTSW);
    DWORD  (WINAPI *fpGetPrinterDriverEx)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, DWORD, DWORD, PDWORD, PDWORD);
    HANDLE (WINAPI *fpFindFirstPrinterChangeNotification)(HANDLE, DWORD, DWORD, LPVOID);
    DWORD  (WINAPI *fpFindClosePrinterChangeNotification)(HANDLE);
    DWORD  (WINAPI *fpAddPortEx)(HANDLE, LPWSTR, DWORD, LPBYTE, LPWSTR);
    DWORD  (WINAPI *fpShutDown)(LPVOID);
    DWORD  (WINAPI *fpRefreshPrinterChangeNotification)(HANDLE, DWORD, PVOID, PVOID);
    DWORD  (WINAPI *fpOpenPrinterEx)(LPWSTR, LPHANDLE, LPPRINTER_DEFAULTSW, LPBYTE, DWORD);
    HANDLE (WINAPI *fpAddPrinterEx)(LPWSTR, DWORD, LPBYTE, LPBYTE, DWORD);
    DWORD  (WINAPI *fpSetPort)(LPWSTR, LPWSTR, DWORD, LPBYTE);
    DWORD  (WINAPI *fpEnumPrinterData)(HANDLE, DWORD, LPWSTR, DWORD, LPDWORD, LPDWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpDeletePrinterData)(HANDLE, LPWSTR);
    DWORD  (WINAPI *fpClusterSplOpen)(LPCWSTR, LPCWSTR, PHANDLE, LPCWSTR, LPCWSTR);
    DWORD  (WINAPI *fpClusterSplClose)(HANDLE);
    DWORD  (WINAPI *fpClusterSplIsAlive)(HANDLE);
    DWORD  (WINAPI *fpSetPrinterDataEx)(HANDLE, LPCWSTR, LPCWSTR, DWORD, LPBYTE, DWORD);
    DWORD  (WINAPI *fpGetPrinterDataEx)(HANDLE, LPCWSTR, LPCWSTR, LPDWORD, LPBYTE, DWORD, LPDWORD);
    DWORD  (WINAPI *fpEnumPrinterDataEx)(HANDLE, LPCWSTR, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpEnumPrinterKey)(HANDLE, LPCWSTR, LPWSTR, DWORD, LPDWORD);
    DWORD  (WINAPI *fpDeletePrinterDataEx)(HANDLE, LPCWSTR, LPCWSTR);
    DWORD  (WINAPI *fpDeletePrinterKey)(HANDLE hPrinter, LPCWSTR pKeyName);
    DWORD  (WINAPI *fpSeekPrinter)(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD, BOOL);
    DWORD  (WINAPI *fpDeletePrinterDriverEx)(LPWSTR, LPWSTR, LPWSTR, DWORD, DWORD);
    DWORD  (WINAPI *fpAddPerMachineConnection)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
    DWORD  (WINAPI *fpDeletePerMachineConnection)(LPCWSTR, LPCWSTR);
    DWORD  (WINAPI *fpEnumPerMachineConnections)(LPCWSTR, LPBYTE, DWORD, LPDWORD, LPDWORD);
    DWORD  (WINAPI *fpXcvData)(HANDLE, LPCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD, PDWORD);
    DWORD  (WINAPI *fpAddPrinterDriverEx)(LPWSTR, DWORD, LPBYTE, DWORD);
    DWORD  (WINAPI *fpSplReadPrinter)(HANDLE, LPBYTE *, DWORD);
    DWORD  (WINAPI *fpDriverUnloadComplete)(LPWSTR);
    DWORD  (WINAPI *fpGetSpoolFileInfo)(HANDLE, LPWSTR *, LPHANDLE, HANDLE, HANDLE);
    DWORD  (WINAPI *fpCommitSpoolData)(HANDLE, DWORD);
    DWORD  (WINAPI *fpCloseSpoolFileHandle)(HANDLE);
    DWORD  (WINAPI *fpFlushPrinter)(HANDLE, LPBYTE, DWORD, LPDWORD, DWORD);
    DWORD  (WINAPI *fpSendRecvBidiData)(HANDLE, LPCWSTR, LPBIDI_REQUEST_CONTAINER, LPBIDI_RESPONSE_CONTAINER *);
    DWORD  (WINAPI *fpAddDriverCatalog)(HANDLE, DWORD, VOID *, DWORD);
    /* Private Data */
    HMODULE dll;
    LPWSTR  dllname;
    LPWSTR  name;
    LPWSTR  regroot;
    DWORD   index;
} backend_t;

/* ################################ */

static backend_t *backend[MAX_BACKEND];
static DWORD used_backends = 0;

static CRITICAL_SECTION backend_cs;
static CRITICAL_SECTION_DEBUG backend_cs_debug =
{
    0, 0, &backend_cs,
    { &backend_cs_debug.ProcessLocksList, &backend_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": backend_cs") }
};
static CRITICAL_SECTION backend_cs = { &backend_cs_debug, -1, 0, 0, 0, 0 };

/* ################################ */

static WCHAR localsplW[] = L"localspl.dll";

/******************************************************************
 * backend_unload_all [internal]
 *
 * unload all backends
 */
void backend_unload_all(void)
{
    EnterCriticalSection(&backend_cs);
    while (used_backends > 0) {
        used_backends--;
        FreeLibrary(backend[used_backends]->dll);
        free(backend[used_backends]->dllname);
        free(backend[used_backends]->name);
        free(backend[used_backends]->regroot);
        free(backend[used_backends]);
        backend[used_backends] = NULL;
    }
    LeaveCriticalSection(&backend_cs);
}

/******************************************************************************
 * backend_load [internal]
 *
 * load and init a backend
 *
 * PARAMS
 *  name   [I] Printprovider to use for the backend. NULL for the local print provider
 *
 * RETURNS
 *  Success: PTR to the backend
 *  Failure: NULL
 *
 */
static backend_t * backend_load(LPWSTR dllname, LPWSTR name, LPWSTR regroot)
{

    BOOL (WINAPI *pInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);
    DWORD id;
    DWORD res;

    TRACE("(%s, %s, %s)\n", debugstr_w(dllname), debugstr_w(name), debugstr_w(regroot));

    EnterCriticalSection(&backend_cs);
    id = used_backends;

    backend[id] = calloc(1, sizeof(backend_t));
    if (!backend[id]) {
        LeaveCriticalSection(&backend_cs);
        return NULL;
    }

    backend[id]->dllname = wcsdup(dllname);
    backend[id]->name = wcsdup(name);
    backend[id]->regroot = wcsdup(regroot);

    backend[id]->dll = LoadLibraryW(dllname);
    if (backend[id]->dll) {
        pInitializePrintProvidor = (void *) GetProcAddress(backend[id]->dll, "InitializePrintProvidor");
        if (pInitializePrintProvidor) {

            /* native localspl does not clear unused entries */
            res = pInitializePrintProvidor((PRINTPROVIDOR *) backend[id], sizeof(PRINTPROVIDOR), regroot);
            if (res) {
                used_backends++;
                backend[id]->index = used_backends;
                LeaveCriticalSection(&backend_cs);
                TRACE("--> backend #%ld: %p (%s)\n", id, backend[id], debugstr_w(dllname));
                return backend[id];
            }
        }
        FreeLibrary(backend[id]->dll);
    }
    free(backend[id]->dllname);
    free(backend[id]->name);
    free(backend[id]->regroot);
    free(backend[id]);
    backend[id] = NULL;
    LeaveCriticalSection(&backend_cs);
    WARN("failed to init %s: %lu\n", debugstr_w(dllname), GetLastError());
    return NULL;
}

/******************************************************************************
 * backend_load_all [internal]
 *
 * load and init all backends
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
BOOL backend_load_all(void)
{
    static BOOL failed = FALSE;

    EnterCriticalSection(&backend_cs);

    /* if we failed before, don't try again */
    if (!failed && (used_backends == 0)) {
        backend_load(localsplW, NULL, NULL);

        /* ToDo: parse the registry and load all other backends */

        failed = (used_backends == 0);
    }
    LeaveCriticalSection(&backend_cs);
    TRACE("-> %d\n", !failed);
    return (!failed);
}

/******************************************************************************
 * backend_first [internal]
 *
 * find the first usable backend
 *
 * RETURNS
 *  Success: PTR to the backend
 *  Failure: NULL
 *
 */
static backend_t * backend_first(LPWSTR name)
{

    EnterCriticalSection(&backend_cs);
    /* Load all backends, when not done yet */
    if (used_backends || backend_load_all()) {

        /* test for the local system first */
        if (!name || !name[0]) {
            LeaveCriticalSection(&backend_cs);
            return backend[0];
        }
    }

    FIXME("server %s not supported in %ld backends\n", debugstr_w(name), used_backends);
    LeaveCriticalSection(&backend_cs);
    return NULL;
}

/******************************************************************
 * AddMonitorW (spoolss.@)
 *
 * Install a Printmonitor
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  Level       [I] Structure-Level (Must be 2)
 *  pMonitors   [I] PTR to MONITOR_INFO_2
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  All Files for the Monitor must already be copied to %winsysdir% ("%SystemRoot%\system32")
 *
 */
BOOL WINAPI AddMonitorW(LPWSTR pName, DWORD Level, LPBYTE pMonitors)
{
    backend_t * pb;
    DWORD res = ROUTER_UNKNOWN;

    TRACE("(%s, %ld, %p)\n", debugstr_w(pName), Level, pMonitors);

    if (Level != 2) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pb = backend_first(pName);
    if (pb && pb->fpAddMonitor)
        res = pb->fpAddMonitor(pName, Level, pMonitors);
    else
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    TRACE("got %lu with %lu\n", res, GetLastError());
    return (res == ROUTER_SUCCESS);
}

/******************************************************************
 * AddPrinterDriverExW (spoolss.@)
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 *
 * PARAMS
 *  pName           [I] Servername or NULL (local Computer)
 *  level           [I] Level for the supplied DRIVER_INFO_*W struct
 *  pDriverInfo     [I] PTR to DRIVER_INFO_*W struct with the Driver Parameter
 *  dwFileCopyFlags [I] How to Copy / Upgrade / Downgrade the needed Files
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
BOOL WINAPI AddPrinterDriverExW(LPWSTR pName, DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    backend_t * pb;
    DWORD res = ROUTER_UNKNOWN;

    TRACE("(%s, %ld, %p, 0x%lx)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);

    if (!pDriverInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pb = backend_first(pName);
    if (pb && pb->fpAddPrinterDriverEx)
        res = pb->fpAddPrinterDriverEx(pName, level, pDriverInfo, dwFileCopyFlags);
    else
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    TRACE("got %lu with %lu\n", res, GetLastError());
    return (res == ROUTER_SUCCESS);
}

/******************************************************************
 * DeleteMonitorW (spoolss.@)
 *
 * Delete a specific Printmonitor from a Printing-Environment
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  pEnvironment [I] Printing-Environment of the Monitor or NULL (Default)
 *  pMonitorName [I] Name of the Monitor, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
BOOL WINAPI DeleteMonitorW(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pMonitorName)
{
    backend_t * pb;
    DWORD res = ROUTER_UNKNOWN;

    TRACE("(%s, %s, %s)\n", debugstr_w(pName), debugstr_w(pEnvironment), debugstr_w(pMonitorName));

    pb = backend_first(pName);
    if (pb && pb->fpDeleteMonitor)
        res = pb->fpDeleteMonitor(pName, pEnvironment, pMonitorName);
    else
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    TRACE("got %lu with %lu\n", res, GetLastError());
    return (res == ROUTER_SUCCESS);
}

/******************************************************************
 * EnumMonitorsW (spoolss.@)
 *
 * Enumerate available Port-Monitors
 *
 * PARAMS
 *  pName      [I] Servername or NULL (local Computer)
 *  Level      [I] Structure-Level
 *  pMonitors  [O] PTR to Buffer that receives the Result
 *  cbBuf      [I] Size of Buffer at pMonitors
 *  pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / required for pMonitors
 *  pcReturned [O] PTR to DWORD that receives the number of Monitors in pMonitors
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pMonitors, if cbBuf is too small
 *
 */
BOOL WINAPI EnumMonitorsW(LPWSTR pName, DWORD Level, LPBYTE pMonitors, DWORD cbBuf,
                          LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    backend_t * pb;
    DWORD res = ROUTER_UNKNOWN;

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    if (pcbNeeded) *pcbNeeded = 0;
    if (pcReturned) *pcReturned = 0;

    pb = backend_first(pName);
    if (pb && pb->fpEnumMonitors)
        res = pb->fpEnumMonitors(pName, Level, pMonitors, cbBuf, pcbNeeded, pcReturned);
    else
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    TRACE("got %lu with %lu (%lu byte for %lu entries)\n\n", res, GetLastError(),
            pcbNeeded ? *pcbNeeded : 0, pcReturned ? *pcReturned : 0);

    return (res == ROUTER_SUCCESS);
}

/******************************************************************
 * EnumPortsW (spoolss.@)
 *
 * Enumerate available Ports
 *
 * PARAMS
 *  pName      [I] Servername or NULL (local Computer)
 *  Level      [I] Structure-Level (1 or 2)
 *  pPorts     [O] PTR to Buffer that receives the Result
 *  cbBuf      [I] Size of Buffer at pPorts
 *  pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / required for pPorts
 *  pcReturned [O] PTR to DWORD that receives the number of Ports in pPorts
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPorts, if cbBuf is too small
 *
 */
BOOL WINAPI EnumPortsW(LPWSTR pName, DWORD Level, LPBYTE pPorts, DWORD cbBuf,
                       LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    backend_t * pb;
    DWORD res = ROUTER_UNKNOWN;

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), Level, pPorts, cbBuf,
            pcbNeeded, pcReturned);

    if (pcbNeeded) *pcbNeeded = 0;
    if (pcReturned) *pcReturned = 0;

    pb = backend_first(pName);
    if (pb && pb->fpEnumPorts)
        res = pb->fpEnumPorts(pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);
    else
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    TRACE("got %lu with %lu (%lu byte for %lu entries)\n", res, GetLastError(),
            pcbNeeded ? *pcbNeeded : 0, pcReturned ? *pcReturned : 0);

    return (res == ROUTER_SUCCESS);
}

/******************************************************************
 * GetPrinterDriverDirectoryW (spoolss.@)
 *
 * Return the PATH for the Printer-Drivers
 *
 * PARAMS
 *   pName            [I] Servername or NULL (local Computer)
 *   pEnvironment     [I] Printing-Environment or NULL (Default)
 *   Level            [I] Structure-Level (must be 1)
 *   pDriverDirectory [O] PTR to Buffer that receives the Result
 *   cbBuf            [I] Size of Buffer at pDriverDirectory
 *   pcbNeeded        [O] PTR to DWORD that receives the size in Bytes used /
 *                        required for pDriverDirectory
 *
 * RETURNS
 *   Success: TRUE  and in pcbNeeded the Bytes used in pDriverDirectory
 *   Failure: FALSE and in pcbNeeded the Bytes required for pDriverDirectory,
 *   if cbBuf is too small
 *
 *   Native Values returned in pDriverDirectory on Success:
 *|  NT(Windows NT x86): "%winsysdir%\\spool\\DRIVERS\\w32x86"
 *|  NT(Windows x64):    "%winsysdir%\\spool\\DRIVERS\\x64"
 *|  NT(Windows 4.0):    "%winsysdir%\\spool\\DRIVERS\\win40"
 *|  win9x(Windows 4.0): "%winsysdir%"
 *
 *   "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
BOOL WINAPI GetPrinterDriverDirectoryW(LPWSTR pName, LPWSTR pEnvironment,
            DWORD Level, LPBYTE pDriverDirectory, DWORD cbBuf, LPDWORD pcbNeeded)
{
    backend_t * pb;
    DWORD res = ROUTER_UNKNOWN;

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_w(pName),
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (pcbNeeded) *pcbNeeded = 0;

    pb = backend_first(pName);
    if (pb && pb->fpGetPrinterDriverDirectory)
        res = pb->fpGetPrinterDriverDirectory(pName, pEnvironment, Level,
                                              pDriverDirectory, cbBuf, pcbNeeded);
    else
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    TRACE("got %lu with %lu (%lu byte)\n",
            res, GetLastError(), pcbNeeded ? *pcbNeeded : 0);

    return (res == ROUTER_SUCCESS);

}

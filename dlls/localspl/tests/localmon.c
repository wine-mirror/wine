/* 
 * Unit test suite for localspl API functions: local print monitor
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
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winreg.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/test.h"


/* ##### */

static HMODULE  hdll;
static LPMONITOREX (WINAPI *pInitializePrintMonitor)(LPWSTR);

static LPMONITOREX pm;
static BOOL  (WINAPI *pEnumPorts)(LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
static BOOL  (WINAPI *pOpenPort)(LPWSTR, PHANDLE);
static BOOL  (WINAPI *pOpenPortEx)(LPWSTR, LPWSTR, PHANDLE, struct _MONITOR *);
static BOOL  (WINAPI *pStartDocPort)(HANDLE, LPWSTR, DWORD, DWORD, LPBYTE);
static BOOL  (WINAPI *pWritePort)(HANDLE hPort, LPBYTE, DWORD, LPDWORD);
static BOOL  (WINAPI *pReadPort)(HANDLE hPort, LPBYTE, DWORD, LPDWORD);
static BOOL  (WINAPI *pEndDocPort)(HANDLE);
static BOOL  (WINAPI *pClosePort)(HANDLE);
static BOOL  (WINAPI *pAddPort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pAddPortEx)(LPWSTR, DWORD, LPBYTE, LPWSTR);
static BOOL  (WINAPI *pConfigurePort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pDeletePort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pGetPrinterDataFromPort)(HANDLE, DWORD, LPWSTR, LPWSTR, DWORD, LPWSTR, DWORD, LPDWORD);
static BOOL  (WINAPI *pSetPortTimeOuts)(HANDLE, LPCOMMTIMEOUTS, DWORD);
static BOOL  (WINAPI *pXcvOpenPort)(HANDLE, LPCWSTR, ACCESS_MASK, PHANDLE phXcv);
static DWORD (WINAPI *pXcvDataPort)(HANDLE, LPCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD);
static BOOL  (WINAPI *pXcvClosePort)(HANDLE);

static WCHAR emptyW[] = {0};
static WCHAR Monitors_LocalPortW[] = { \
                                'S','y','s','t','e','m','\\',
                                'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                'C','o','n','t','r','o','l','\\',
                                'P','r','i','n','t','\\',
                                'M','o','n','i','t','o','r','s','\\',
                                'L','o','c','a','l',' ','P','o','r','t',0};
                                       
/* ##### */

static void test_InitializePrintMonitor(void)
{
    LPMONITOREX res;

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(NULL);
    ok( (res == NULL) && (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %d\n (expected NULL with " \
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(emptyW);
    ok( (res == NULL) && (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %d\n (expected NULL with " \
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    /* Every call with a non-empty string returns the same Pointer */
    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(Monitors_LocalPortW);
    ok( res == pm,
        "returned %p with %d (expected %p)\n", res, GetLastError(), pm);
}

/* ########################### */

#define GET_MONITOR_FUNC(name) \
            if(numentries > 0) { \
                numentries--; \
                p##name = (void *) pm->Monitor.pfn##name ;  \
            }


START_TEST(localmon)
{
    /* This DLL does not exists on Win9x */
    hdll = LoadLibraryA("localspl.dll");
    if (!hdll) return;

    pInitializePrintMonitor = (void *) GetProcAddress(hdll, "InitializePrintMonitor");
    if (!pInitializePrintMonitor) return;

    /* Native localmon.dll / localspl.dll need a vaild Port-Entry in:
       a) since xp: HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports 
       b) upto w2k: Section "Ports" in win.ini
       or InitializePrintMonitor fails. */
    pm = pInitializePrintMonitor(Monitors_LocalPortW);
    if (pm) {
        DWORD   numentries;
        numentries = (pm->dwMonitorSize ) / sizeof(VOID *);
        /* NT4: 14, since w2k: 17 */
        ok( numentries == 14 || numentries == 17, 
            "dwMonitorSize (%d) => %d Functions\n", pm->dwMonitorSize, numentries);

        GET_MONITOR_FUNC(EnumPorts);
        GET_MONITOR_FUNC(OpenPort);
        GET_MONITOR_FUNC(OpenPortEx);
        GET_MONITOR_FUNC(StartDocPort);
        GET_MONITOR_FUNC(WritePort);
        GET_MONITOR_FUNC(ReadPort);
        GET_MONITOR_FUNC(EndDocPort);
        GET_MONITOR_FUNC(ClosePort);
        GET_MONITOR_FUNC(AddPort);
        GET_MONITOR_FUNC(AddPortEx);
        GET_MONITOR_FUNC(ConfigurePort);
        GET_MONITOR_FUNC(DeletePort);
        GET_MONITOR_FUNC(GetPrinterDataFromPort);
        GET_MONITOR_FUNC(SetPortTimeOuts);
        GET_MONITOR_FUNC(XcvOpenPort);
        GET_MONITOR_FUNC(XcvDataPort);
        GET_MONITOR_FUNC(XcvClosePort);
    }
    test_InitializePrintMonitor();
}

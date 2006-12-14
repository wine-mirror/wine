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
static HMODULE  hlocalmon;
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
static BOOL  (WINAPI *pXcvOpenPort)(LPCWSTR, ACCESS_MASK, PHANDLE phXcv);
static DWORD (WINAPI *pXcvDataPort)(HANDLE, LPCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD);
static BOOL  (WINAPI *pXcvClosePort)(HANDLE);

static WCHAR does_not_existW[] = {'d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};
static WCHAR emptyW[] = {0};
static WCHAR invalid_serverW[] = {'\\','\\','i','n','v','a','l','i','d','_','s','e','r','v','e','r',0};
static WCHAR Monitors_LocalPortW[] = { \
                                'S','y','s','t','e','m','\\',
                                'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                'C','o','n','t','r','o','l','\\',
                                'P','r','i','n','t','\\',
                                'M','o','n','i','t','o','r','s','\\',
                                'L','o','c','a','l',' ','P','o','r','t',0};

static WCHAR portname_com1W[] = {'C','O','M','1',':',0};
static WCHAR portname_fileW[] = {'F','I','L','E',':',0};
static WCHAR portname_lpt1W[] = {'L','P','T','1',':',0};

/* ########################### */

static void test_AddPort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pAddPort) return;

    if (0)
    {
    /* NT4 crash on this test */
    res = pAddPort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - The Servername is ignored
        - Case of MonitorName is ignored
    */

    SetLastError(0xdeadbeef);
    res = pAddPort(NULL, 0, emptyW);
    ok(!res, "returned %d with 0x%x (expected '0')\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pAddPort(NULL, 0, does_not_existW);
    ok(!res, "returned %d with 0x%x (expected '0')\n", res, GetLastError());

}

/* ########################### */
                                       
static void test_ConfigurePort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pConfigurePort) return;

    if (0)
    {
    /* NT4 crash on this test */
    res = pConfigurePort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - Case of Portname is ignored
        - "COM1:" and "COM01:" are the same (Compared by value)
        - Portname without ":" => Dialog "Nothing to configure" comes up; Success
        - "LPT1:", "LPT0:" and "LPT:" are the same (Numbers in "LPT:" are ignored)
        - Empty Servername (LPT1:) => Dialog comes up (Servername is ignored)
        - "FILE:" => Dialog "Nothing to configure" comes up; Success
        - Empty Portname =>  => Dialog "Nothing to configure" comes up; Success
        - Port "does_not_exist" => Dialog "Nothing to configure" comes up; Success
    */
    if (winetest_interactive > 0) {

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_com1W);
        trace("returned %d with %d\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_lpt1W);
        trace("returned %d with %d\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_fileW);
        trace("returned %d with %d\n", res, GetLastError());
    }
}

/* ########################### */

static void test_DeletePort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pDeletePort) return;

    if (0)
    {
    /* NT4 crash on this test */
    res = pDeletePort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - Case of Portname is ignored (returned '1' on Success)
        - "COM1:" and "COM01:" are different (Compared as string)
        - server_does_not_exist (LPT1:) => Port deleted, Success (Servername is ignored)
        - Empty Portname =>  => FALSE (LastError not changed)
        - Port "does_not_exist" => FALSE (LastError not changed)
    */

    SetLastError(0xdeadbeef);
    res = pDeletePort(NULL, 0, emptyW);
    ok(!res, "returned %d with 0x%x (expected '0')\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pDeletePort(NULL, 0, does_not_existW);
    ok(!res, "returned %d with 0x%x (expected '0')\n", res, GetLastError());

}

/* ########################### */

static void test_EnumPorts(void)
{
    DWORD   res;
    DWORD   level;
    LPBYTE  buffer;
    DWORD   cbBuf;
    DWORD   pcbNeeded;
    DWORD   pcReturned;

    if (!pEnumPorts) return;

    /* valid levels are 1 and 2 */
    for(level = 0; level < 4; level++) {

        cbBuf = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, NULL, 0, &cbBuf, &pcReturned);

        /* use only a short test, when we test with an invalid level */
        if(!level || (level > 2)) {
            /* NT4 fails with ERROR_INVALID_LEVEL (as expected)
               XP succeeds with ERROR_SUCCESS () */
            ok( (cbBuf == 0) && (pcReturned == 0),
                "(%d) returned %d with %d and %d, %d (expected 0, 0)\n",
                level, res, GetLastError(), cbBuf, pcReturned);
            continue;
        }        

        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%d) returned %d with %d and %d, %d (expected '0' with " \
            "ERROR_INSUFFICIENT_BUFFER)\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        buffer = HeapAlloc(GetProcessHeap(), 0, cbBuf * 2);
        if (buffer == NULL) continue;

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, buffer, cbBuf, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %d and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), cbBuf, pcReturned);
        /* We can compare the returned Data with the Registry / "win.ini",[Ports] here */

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %d and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, buffer, cbBuf-1, &pcbNeeded, &pcReturned);
        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%d) returned %d with %d and %d, %d (expected '0' with " \
            "ERROR_INSUFFICIENT_BUFFER)\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        if (0)
        {
        /* The following tests crash this app with native localmon/localspl */
        res = pEnumPorts(NULL, level, NULL, cbBuf, &pcbNeeded, &pcReturned);
        res = pEnumPorts(NULL, level, buffer, cbBuf, NULL, &pcReturned);
        res = pEnumPorts(NULL, level, buffer, cbBuf, &pcbNeeded, NULL);
        }

        /* The Servername is ignored */
        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(emptyW, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %d and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(invalid_serverW, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %d and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        HeapFree(GetProcessHeap(), 0, buffer);
    }
}

/* ########################### */


static void test_InitializePrintMonitor(void)
{
    LPMONITOREX res;

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(NULL);
    /* The Parameter was unchecked before w2k */
    ok( res || (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %d\n (expected '!= NULL' or: NULL with " \
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(emptyW);
    ok( res || (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %d\n (expected '!= NULL' or: NULL with " \
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());


    /* Every call with a non-empty string returns the same Pointer */
    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(Monitors_LocalPortW);
    ok( res == pm,
        "returned %p with %d (expected %p)\n", res, GetLastError(), pm);
}

/* ########################### */

static void test_XcvClosePort(void)
{
    DWORD   res;
    HANDLE hXcv;

    if ((pXcvOpenPort == NULL) || (pXcvClosePort == NULL)) return;

    if (0)
    {
    /* crash with native localspl.dll (w2k+xp) */
    res = pXcvClosePort(NULL);
    res = pXcvClosePort(INVALID_HANDLE_VALUE);
    }


    SetLastError(0xdeadbeef);
    hXcv = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv);
    ok(res, "returned %d with 0x%x and %p (expected '!= 0')\n", res, GetLastError(), hXcv);

    if (res) {
        SetLastError(0xdeadbeef);
        res = pXcvClosePort(hXcv);
        ok( res, "returned %d with 0x%x(expected '!= 0')\n", res, GetLastError());

        if (0)
        {
        /* test for "Double Free": crash with native localspl.dll (w2k+xp) */
        res = pXcvClosePort(hXcv);
        }
    }
}

/* ########################### */

static void test_XcvOpenPort(void)
{
    DWORD   res;
    HANDLE  hXcv;

    if ((pXcvOpenPort == NULL) || (pXcvClosePort == NULL)) return;

    if (0)
    {
    /* crash with native localspl.dll (w2k+xp) */
    res = pXcvOpenPort(NULL, SERVER_ACCESS_ADMINISTER, &hXcv);
    res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, NULL);
    }


    /* The returned handle is the result from a previous "spoolss.dll,DllAllocSplMem" */
    SetLastError(0xdeadbeef);
    hXcv = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv);
    ok(res, "returned %d with 0x%x and %p (expected '!= 0')\n", res, GetLastError(), hXcv);
    if (res) pXcvClosePort(hXcv);


    /* The ACCESS_MASK is not checked in XcvOpenPort */
    SetLastError(0xdeadbeef);
    hXcv = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(emptyW, 0, &hXcv);
    ok(res, "returned %d with 0x%x and %p (expected '!= 0')\n", res, GetLastError(), hXcv);
    if (res) pXcvClosePort(hXcv);


    /* A copy of pszObject is saved in the Memory-Block */
    SetLastError(0xdeadbeef);
    hXcv = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(portname_lpt1W, SERVER_ALL_ACCESS, &hXcv);
    ok(res, "returned %d with 0x%x and %p (expected '!= 0')\n", res, GetLastError(), hXcv);
    if (res) pXcvClosePort(hXcv);

    SetLastError(0xdeadbeef);
    hXcv = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(portname_fileW, SERVER_ALL_ACCESS, &hXcv);
    ok(res, "returned %d with 0x%x and %p (expected '!= 0')\n", res, GetLastError(), hXcv);
    if (res) pXcvClosePort(hXcv);

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
    if (!pInitializePrintMonitor) {
        /* The Monitor for "Local Ports" was in a seperate dll before w2k */
        hlocalmon = LoadLibraryA("localmon.dll");
        if (hlocalmon) {
            pInitializePrintMonitor = (void *) GetProcAddress(hlocalmon, "InitializePrintMonitor");
        }
    }
    if (!pInitializePrintMonitor) return;

    /* Native localmon.dll / localspl.dll need a vaild Port-Entry in:
       a) since xp: HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports 
       b) up to w2k: Section "Ports" in win.ini
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

    test_AddPort();
    test_ConfigurePort();
    test_DeletePort();
    test_EnumPorts();
    test_XcvClosePort();
    test_XcvOpenPort();
}

/* 
 * Unit test suite for localspl API functions: local print monitor
 *
 * Copyright 2006-2007 Detlef Riekenberg
 * Copyright 2019 Dmitry Timoshkov
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
static HANDLE   hmon;
static LPMONITOREX (WINAPI *pInitializePrintMonitor)(LPWSTR);
static LPMONITOR2  (WINAPI *pInitializePrintMonitor2)(PMONITORINIT, LPHANDLE);

static LPMONITOREX pm;
static LPMONITOR2 pm2;
static BOOL  (WINAPI *pEnumPorts)(LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
static BOOL  (WINAPI *pEnumPorts2)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
static BOOL  (WINAPI *pOpenPort)(LPWSTR, PHANDLE);
static BOOL  (WINAPI *pOpenPort2)(HANDLE, LPWSTR, PHANDLE);
static BOOL  (WINAPI *pStartDocPort)(HANDLE, LPWSTR, DWORD, DWORD, LPBYTE);
static BOOL  (WINAPI *pWritePort)(HANDLE hPort, LPBYTE, DWORD, LPDWORD);
static BOOL  (WINAPI *pReadPort)(HANDLE hPort, LPBYTE, DWORD, LPDWORD);
static BOOL  (WINAPI *pEndDocPort)(HANDLE);
static BOOL  (WINAPI *pClosePort)(HANDLE);
static BOOL  (WINAPI *pAddPort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pAddPort2)(HANDLE, LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pAddPortEx)(LPWSTR, DWORD, LPBYTE, LPWSTR);
static BOOL  (WINAPI *pAddPortEx2)(HANDLE, LPWSTR, DWORD, LPBYTE, LPWSTR);
static BOOL  (WINAPI *pConfigurePort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pConfigurePort2)(HANDLE, LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pDeletePort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pDeletePort2)(HANDLE, LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pGetPrinterDataFromPort)(HANDLE, DWORD, LPWSTR, LPWSTR, DWORD, LPWSTR, DWORD, LPDWORD);
static BOOL  (WINAPI *pSetPortTimeOuts)(HANDLE, LPCOMMTIMEOUTS, DWORD);
static BOOL  (WINAPI *pXcvOpenPort)(LPCWSTR, ACCESS_MASK, PHANDLE);
static BOOL  (WINAPI *pXcvOpenPort2)(HANDLE, LPCWSTR, ACCESS_MASK, PHANDLE);
static DWORD (WINAPI *pXcvDataPort)(HANDLE, LPCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD);
static BOOL  (WINAPI *pXcvClosePort)(HANDLE);

static HANDLE hXcv;
static HANDLE hXcv_noaccess;

/* ########################### */

static const WCHAR cmd_AddPortW[] = {'A','d','d','P','o','r','t',0};
static const WCHAR cmd_ConfigureLPTPortCommandOKW[] = {'C','o','n','f','i','g','u','r','e',
                                    'L','P','T','P','o','r','t',
                                    'C','o','m','m','a','n','d','O','K',0};
static const WCHAR cmd_DeletePortW[] = {'D','e','l','e','t','e','P','o','r','t',0};
static const WCHAR cmd_GetTransmissionRetryTimeoutW[] = {'G','e','t',
                                    'T','r','a','n','s','m','i','s','s','i','o','n',
                                    'R','e','t','r','y','T','i','m','e','o','u','t',0};

static const WCHAR cmd_MonitorUIW[] = {'M','o','n','i','t','o','r','U','I',0};
static const WCHAR cmd_MonitorUI_lcaseW[] = {'m','o','n','i','t','o','r','u','i',0};
static const WCHAR cmd_PortIsValidW[] = {'P','o','r','t','I','s','V','a','l','i','d',0};
static WCHAR does_not_existW[] = {'d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};
static const CHAR emptyA[] = "";
static WCHAR emptyW[] = {0};
static WCHAR LocalPortW[] = {'L','o','c','a','l',' ','P','o','r','t',0};
static WCHAR Monitors_LocalPortW[] = {
                                'S','y','s','t','e','m','\\',
                                'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                'C','o','n','t','r','o','l','\\',
                                'P','r','i','n','t','\\',
                                'M','o','n','i','t','o','r','s','\\',
                                'L','o','c','a','l',' ','P','o','r','t',0};

static const CHAR num_0A[] = "0";
static WCHAR num_0W[] = {'0',0};
static const CHAR num_1A[] = "1";
static WCHAR num_1W[] = {'1',0};
static const CHAR num_999999A[] = "999999";
static WCHAR num_999999W[] = {'9','9','9','9','9','9',0};
static const CHAR num_1000000A[] = "1000000";
static WCHAR num_1000000W[] = {'1','0','0','0','0','0','0',0};

static const WCHAR portname_comW[]  = {'C','O','M',0};
static WCHAR portname_com1W[] = {'C','O','M','1',':',0};
static WCHAR portname_com2W[] = {'C','O','M','2',':',0};
static WCHAR portname_fileW[] = {'F','I','L','E',':',0};
static const WCHAR portname_lptW[]  = {'L','P','T',0};
static WCHAR portname_lpt1W[] = {'L','P','T','1',':',0};
static WCHAR portname_lpt2W[] = {'L','P','T','2',':',0};
static WCHAR server_does_not_existW[] = {'\\','\\','d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};

static const CHAR TransmissionRetryTimeoutA[] = {'T','r','a','n','s','m','i','s','s','i','o','n',
                                    'R','e','t','r','y','T','i','m','e','o','u','t',0};

static const CHAR WinNT_CV_WindowsA[] = {'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'W','i','n','d','o','w','s',' ','N','T','\\',
                                         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                         'W','i','n','d','o','w','s',0};
static WCHAR wineW[] = {'W','i','n','e',0};

static WCHAR tempdirW[MAX_PATH];
static WCHAR tempfileW[MAX_PATH];

#define PORTNAME_PREFIX  3
#define PORTNAME_MINSIZE 5
#define PORTNAME_MAXSIZE 10
static WCHAR have_com[PORTNAME_MAXSIZE];
static WCHAR have_lpt[PORTNAME_MAXSIZE];
static WCHAR have_file[PORTNAME_MAXSIZE];

/* ########################### */

static LONG WINAPI CreateKey(HANDLE hcKey, LPCWSTR pszSubKey, DWORD dwOptions,
                REGSAM samDesired, PSECURITY_ATTRIBUTES pSecurityAttributes,
                PHANDLE phckResult, PDWORD pdwDisposition, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI OpenKey(HANDLE hcKey, LPCWSTR pszSubKey, REGSAM samDesired,
                PHANDLE phkResult, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI CloseKey(HANDLE hcKey, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI DeleteKey(HANDLE hcKey, LPCWSTR pszSubKey, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI EnumKey(HANDLE hcKey, DWORD dwIndex, LPWSTR pszName,
                PDWORD pcchName, PFILETIME pftLastWriteTime, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI QueryInfoKey(HANDLE hcKey, PDWORD pcSubKeys, PDWORD pcbKey,
                PDWORD pcValues, PDWORD pcbValue, PDWORD pcbData,
                PDWORD pcbSecurityDescriptor, PFILETIME pftLastWriteTime,
                HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI SetValue(HANDLE hcKey, LPCWSTR pszValue, DWORD dwType,
                const BYTE* pData, DWORD cbData, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI DeleteValue(HANDLE hcKey, LPCWSTR pszValue, HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI EnumValue(HANDLE hcKey, DWORD dwIndex, LPWSTR pszValue,
                PDWORD pcbValue, PDWORD pType, PBYTE pData, PDWORD pcbData,
                HANDLE hSpooler)
{
    ok(0, "should not be called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI QueryValue(HANDLE hcKey, LPCWSTR pszValue, PDWORD pType,
                PBYTE pData, PDWORD pcbData, HANDLE hSpooler)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static MONITORREG monreg =
{
    sizeof(MONITORREG),
    CreateKey,
    OpenKey,
    CloseKey,
    DeleteKey,
    EnumKey,
    QueryInfoKey,
    SetValue,
    DeleteValue,
    EnumValue,
    QueryValue
};

static BOOL delete_port(LPWSTR portname)
{
    if (pDeletePort)
        return pDeletePort(NULL, 0, portname);
    if (pDeletePort2)
        return pDeletePort2(hmon, NULL, 0, portname);
    return pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) portname,
            (lstrlenW(portname) + 1) * sizeof(WCHAR), NULL, 0, NULL) == ERROR_SUCCESS;
}

static BOOL call_EnumPorts(WCHAR *name, DWORD level, BYTE *buf, DWORD size,
        DWORD *needed, DWORD *count)
{
    if (pEnumPorts)
        return pEnumPorts(name, level, buf, size, needed, count);
    return pEnumPorts2(hmon, name, level, buf, size, needed, count);
}

static BOOL call_OpenPort(WCHAR *name, HANDLE *h)
{
    if (pOpenPort)
        return pOpenPort(name, h);
    return pOpenPort2(hmon, name, h);
}

static BOOL call_AddPortEx(WCHAR *name, DWORD level, BYTE *buf, WCHAR *mon)
{
    if (pAddPortEx)
        return pAddPortEx(name, level, buf, mon);
    return pAddPortEx2(hmon, name, level, buf, mon);
}

static BOOL call_XcvOpenPort(const WCHAR *name, ACCESS_MASK access, HANDLE *h)
{
    if (pXcvOpenPort)
        return pXcvOpenPort(name, access, h);
    return pXcvOpenPort2(hmon, name, access, h);
}

/* ########################### */

static void find_installed_ports(void)
{
    PORT_INFO_1W * pi = NULL;
    WCHAR   nameW[PORTNAME_MAXSIZE];
    DWORD   needed;
    DWORD   returned;
    DWORD   res;
    DWORD   id;

    have_com[0] = '\0';
    have_lpt[0] = '\0';
    have_file[0] = '\0';

    if (!pEnumPorts && !pEnumPorts2) return;

    res = call_EnumPorts(NULL, 1, NULL, 0, &needed, &returned);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        pi = HeapAlloc(GetProcessHeap(), 0, needed);
    }
    res = call_EnumPorts(NULL, 1, (LPBYTE) pi, needed, &needed, &returned);

    if (!res) {
        skip("no ports found\n");
        HeapFree(GetProcessHeap(), 0, pi);
        return;
    }

    id = 0;
    while (id < returned) {
        res = lstrlenW(pi[id].pName);
        if ((res >= PORTNAME_MINSIZE) && (res < PORTNAME_MAXSIZE) &&
            (pi[id].pName[res-1] == ':')) {
            /* copy only the prefix ("LPT" or "COM") */
            memcpy(&nameW, pi[id].pName, PORTNAME_PREFIX * sizeof(WCHAR));
            nameW[PORTNAME_PREFIX] = '\0';

            if (!have_com[0] && (lstrcmpiW(nameW, portname_comW) == 0)) {
                memcpy(&have_com, pi[id].pName, (res+1) * sizeof(WCHAR));
            }

            if (!have_lpt[0] && (lstrcmpiW(nameW, portname_lptW) == 0)) {
                memcpy(&have_lpt, pi[id].pName, (res+1) * sizeof(WCHAR));
            }

            if (!have_file[0] && (lstrcmpiW(pi[id].pName, portname_fileW) == 0)) {
                memcpy(&have_file, pi[id].pName, (res+1) * sizeof(WCHAR));
            }
        }
        id++;
    }

    HeapFree(GetProcessHeap(), 0, pi);
}

/* ########################### */

static void test_AddPort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pAddPort) return;

    if (0)
    {
        /* NT4 crash on this test */
        pAddPort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - The Servername is ignored
        - Case of MonitorName is ignored
    */

    SetLastError(0xdeadbeef);
    res = pAddPort(NULL, 0, emptyW);
    ok(!res, "returned %ld with %lu (expected '0')\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pAddPort(NULL, 0, does_not_existW);
    ok(!res, "returned %ld with %lu (expected '0')\n", res, GetLastError());

}

/* ########################### */

static void test_AddPortEx(void)
{
    PORT_INFO_2W pi;
    DWORD   res;

    if (!pAddPortEx && !pAddPortEx2) {
        skip("AddPortEx\n");
        return;
    }
    if (!pDeletePort && !pDeletePort2 && !pXcvDataPort) {
        skip("No API to delete a Port\n");
        return;
    }

    /* start test with clean ports */
    delete_port(tempfileW);

    pi.pPortName = tempfileW;
    if (0) {
        /* tests crash with native localspl.dll in w2k,
           but works with native localspl.dll in wine */
        SetLastError(0xdeadbeef);
        res = call_AddPortEx(NULL, 1, (LPBYTE) &pi, LocalPortW);
        trace("returned %lu with %lu\n", res, GetLastError() );
        ok( res, "got %lu with %lu (expected '!= 0')\n", res, GetLastError());

        /* port already exists: */
        SetLastError(0xdeadbeef);
        res = call_AddPortEx(NULL, 1, (LPBYTE) &pi, LocalPortW);
        trace("returned %lu with %lu\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        delete_port(tempfileW);


        /*  NULL for pMonitorName is documented for Printmonitors, but
            localspl.dll fails always with ERROR_INVALID_PARAMETER  */
        SetLastError(0xdeadbeef);
        res = call_AddPortEx(NULL, 1, (LPBYTE) &pi, NULL);
        trace("returned %lu with %lu\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        if (res) delete_port(tempfileW);


        SetLastError(0xdeadbeef);
        res = call_AddPortEx(NULL, 1, (LPBYTE) &pi, emptyW);
        trace("returned %lu with %lu\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        if (res) delete_port(tempfileW);


        SetLastError(0xdeadbeef);
        res = call_AddPortEx(NULL, 1, (LPBYTE) &pi, does_not_existW);
        trace("returned %lu with %lu\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        if (res) delete_port(tempfileW);
    }

    pi.pPortName = NULL;
    SetLastError(0xdeadbeef);
    res = call_AddPortEx(NULL, 1, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());

    /*  level 2 is documented as supported for Printmonitors,
        but localspl.dll fails always with ERROR_INVALID_LEVEL */

    pi.pPortName = tempfileW;
    pi.pMonitorName = LocalPortW;
    pi.pDescription = wineW;
    pi.fPortType = PORT_TYPE_WRITE;

    SetLastError(0xdeadbeef);
    res = call_AddPortEx(NULL, 2, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "got %lu with %lu (expected '0' with ERROR_INVALID_LEVEL)\n",
        res, GetLastError());
    if (res) delete_port(tempfileW);


    /* invalid levels */
    SetLastError(0xdeadbeef);
    res = call_AddPortEx(NULL, 0, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "got %lu with %lu (expected '0' with ERROR_INVALID_LEVEL)\n",
        res, GetLastError());
    if (res) delete_port(tempfileW);


    SetLastError(0xdeadbeef);
    res = call_AddPortEx(NULL, 3, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "got %lu with %lu (expected '0' with ERROR_INVALID_LEVEL)\n",
        res, GetLastError());
    if (res) delete_port(tempfileW);

    /* cleanup */
    delete_port(tempfileW);
}

/* ########################### */

static void test_ClosePort(void)
{
    HANDLE  hPort;
    HANDLE  hPort2;
    LPWSTR  nameW = NULL;
    DWORD   res;
    DWORD   res2;


    if ((!pOpenPort && !pOpenPort2) || !pClosePort) return;

    if (have_com[0]) {
        nameW = have_com;

        hPort = (HANDLE) 0xdeadbeef;
        res = call_OpenPort(nameW, &hPort);
        hPort2 = (HANDLE) 0xdeadbeef;
        res2 = call_OpenPort(nameW, &hPort2);

        if (res2 && (hPort2 != hPort)) {
            SetLastError(0xdeadbeef);
            res2 = pClosePort(hPort2);
            ok(res2, "got %lu with %lu (expected '!= 0')\n", res2, GetLastError());
        }

        if (res) {
            SetLastError(0xdeadbeef);
            res = pClosePort(hPort);
            ok(res, "got %lu with %lu (expected '!= 0')\n", res, GetLastError());
        }
    }


    if (have_lpt[0]) {
        nameW = have_lpt;

        hPort = (HANDLE) 0xdeadbeef;
        res = call_OpenPort(nameW, &hPort);
        hPort2 = (HANDLE) 0xdeadbeef;
        res2 = call_OpenPort(nameW, &hPort2);

        if (res2 && (hPort2 != hPort)) {
            SetLastError(0xdeadbeef);
            res2 = pClosePort(hPort2);
            ok(res2, "got %lu with %lu (expected '!= 0')\n", res2, GetLastError());
        }

        if (res) {
            SetLastError(0xdeadbeef);
            res = pClosePort(hPort);
            ok(res, "got %lu with %lu (expected '!= 0')\n", res, GetLastError());
        }
    }


    if (have_file[0]) {
        nameW = have_file;

        hPort = (HANDLE) 0xdeadbeef;
        res = call_OpenPort(nameW, &hPort);
        hPort2 = (HANDLE) 0xdeadbeef;
        res2 = call_OpenPort(nameW, &hPort2);

        if (res2 && (hPort2 != hPort)) {
            SetLastError(0xdeadbeef);
            res2 = pClosePort(hPort2);
            ok(res2, "got %lu with %lu (expected '!= 0')\n", res2, GetLastError());
        }

        if (res) {
            SetLastError(0xdeadbeef);
            res = pClosePort(hPort);
            ok(res, "got %lu with %lu (expected '!= 0')\n", res, GetLastError());
        }

    }

    if (0) {
        /* an invalid HANDLE crash native localspl.dll */

        SetLastError(0xdeadbeef);
        res = pClosePort(NULL);
        trace("got %lu with %lu\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pClosePort( (HANDLE) 0xdeadbeef);
        trace("got %lu with %lu\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pClosePort(INVALID_HANDLE_VALUE);
        trace("got %lu with %lu\n", res, GetLastError());
    }

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
        pConfigurePort(NULL, 0, NULL);
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
        trace("returned %ld with %lu\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_lpt1W);
        trace("returned %ld with %lu\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_fileW);
        trace("returned %ld with %lu\n", res, GetLastError());
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
        pDeletePort(NULL, 0, NULL);
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
    ok(!res, "returned %ld with %lu (expected '0')\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pDeletePort(NULL, 0, does_not_existW);
    ok(!res, "returned %ld with %lu (expected '0')\n", res, GetLastError());

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

    if (!pEnumPorts && !pEnumPorts2) return;

    /* valid levels are 1 and 2 */
    for(level = 0; level < 4; level++) {

        cbBuf = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_EnumPorts(NULL, level, NULL, 0, &cbBuf, &pcReturned);

        /* use only a short test, when we test with an invalid level */
        if(!level || (level > 2)) {
            todo_wine ok(!res || broken(res && !GetLastError()),
                    "EnumPorts succeeded with level %ld\n", level);
            todo_wine ok(GetLastError() == ERROR_INVALID_LEVEL || broken(res && !GetLastError()),
                    "GetLastError() = %ld\n", GetLastError());
            continue;
        }        

        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%ld) returned %ld with %lu and %ld, %ld (expected '0' with "
            "ERROR_INSUFFICIENT_BUFFER)\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        buffer = HeapAlloc(GetProcessHeap(), 0, cbBuf * 2);
        if (buffer == NULL) continue;

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_EnumPorts(NULL, level, buffer, cbBuf, &pcbNeeded, &pcReturned);
        ok( res, "(%ld) returned %ld with %lu and %ld, %ld (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);
        /* We can compare the returned Data with the Registry / "win.ini",[Ports] here */

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_EnumPorts(NULL, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%ld) returned %ld with %lu and %ld, %ld (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_EnumPorts(NULL, level, buffer, cbBuf-1, &pcbNeeded, &pcReturned);
        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%ld) returned %ld with %lu and %ld, %ld (expected '0' with "
            "ERROR_INSUFFICIENT_BUFFER)\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        if (0)
        {
            /* The following tests crash this app with native localmon/localspl */
            call_EnumPorts(NULL, level, NULL, cbBuf, &pcbNeeded, &pcReturned);
            call_EnumPorts(NULL, level, buffer, cbBuf, NULL, &pcReturned);
            call_EnumPorts(NULL, level, buffer, cbBuf, &pcbNeeded, NULL);
        }

        /* The Servername is ignored */
        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_EnumPorts(emptyW, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%ld) returned %ld with %lu and %ld, %ld (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_EnumPorts(server_does_not_existW, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%ld) returned %ld with %lu and %ld, %ld (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        HeapFree(GetProcessHeap(), 0, buffer);
    }
}

/* ########################### */


static void test_InitializePrintMonitor(void)
{
    LPMONITOREX res;

    if (!pInitializePrintMonitor) return;

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(NULL);
    /* The Parameter was unchecked before w2k */
    ok( res || (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %lu\n (expected '!= NULL' or: NULL with "
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(emptyW);
    ok( res || (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %lu\n (expected '!= NULL' or: NULL with "
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    /* Every call with a non-empty string returns the same Pointer */
    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(Monitors_LocalPortW);
    ok( res == pm,
        "returned %p with %lu (expected %p)\n", res, GetLastError(), pm);
    ok(res->dwMonitorSize == sizeof(MONITOR), "wrong dwMonitorSize %lu\n", res->dwMonitorSize);
}

static void test_InitializePrintMonitor2(void)
{
    MONITORINIT init;
    MONITOR2 *monitor2;
    HANDLE hmon;

    if (!pInitializePrintMonitor2) return;

    memset(&init, 0, sizeof(init));
    init.cbSize = sizeof(init);
    init.hckRegistryRoot = 0;
    init.pMonitorReg = &monreg;
    init.bLocal = TRUE;

    monitor2 = pInitializePrintMonitor2(&init, &hmon);
    ok(monitor2 != NULL, "InitializePrintMonitor2 error %lu\n", GetLastError());
    ok(monitor2->cbSize >= FIELD_OFFSET(MONITOR2, pfnSendRecvBidiDataFromPort), "wrong cbSize %lu\n", monitor2->cbSize);
}

/* ########################### */

static void test_OpenPort(void)
{
    HANDLE  hPort;
    HANDLE  hPort2;
    LPWSTR  nameW = NULL;
    DWORD   res;
    DWORD   res2;

    if ((!pOpenPort && !pOpenPort2) || !pClosePort) return;

    if (have_com[0]) {
        nameW = have_com;

        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_OpenPort(nameW, &hPort);
        ok( res, "got %lu with %lu and %p (expected '!= 0')\n",
            res, GetLastError(), hPort);

        /* the same HANDLE is returned for a second OpenPort in native localspl */
        hPort2 = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res2 = call_OpenPort(nameW, &hPort2);
        ok( res2, "got %lu with %lu and %p (expected '!= 0')\n",
            res2, GetLastError(), hPort2);

        if (res) pClosePort(hPort);
        if (res2 && (hPort2 != hPort)) pClosePort(hPort2);
    }

    if (have_lpt[0]) {
        nameW = have_lpt;

        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_OpenPort(nameW, &hPort);
        ok( res || (GetLastError() == ERROR_ACCESS_DENIED),
            "got %lu with %lu and %p (expected '!= 0' or '0' with ERROR_ACCESS_DENIED)\n",
            res, GetLastError(), hPort);

        /* the same HANDLE is returned for a second OpenPort in native localspl */
        hPort2 = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res2 = call_OpenPort(nameW, &hPort2);
        ok( res2 || (GetLastError() == ERROR_ACCESS_DENIED),
            "got %lu with %lu and %p (expected '!= 0' or '0' with ERROR_ACCESS_DENIED)\n",
            res2, GetLastError(), hPort2);

        if (res) pClosePort(hPort);
        if (res2 && (hPort2 != hPort)) pClosePort(hPort2);
    }

    if (have_file[0]) {
        nameW = have_file;

        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_OpenPort(nameW, &hPort);
        ok( res, "got %lu with %lu and %p (expected '!= 0')\n",
            res, GetLastError(), hPort);

        /* a different HANDLE is returned for a second OpenPort */
        hPort2 = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res2 = call_OpenPort(nameW, &hPort2);
        ok( res2 && (hPort2 != hPort),
            "got %lu with %lu and %p (expected '!= 0' and '!= %p')\n",
            res2, GetLastError(), hPort2, hPort);

        if (res) pClosePort(hPort);
        if (res2 && (hPort2 != hPort)) pClosePort(hPort2);
    }

    if (0) {
        /* this test crash native localspl (w2k+xp) */
        if (nameW) {
            hPort = (HANDLE) 0xdeadbeef;
            SetLastError(0xdeadbeef);
            res = call_OpenPort(nameW, NULL);
            trace("got %lu with %lu and %p\n", res, GetLastError(), hPort);
        }
    }

    SetLastError(0xdeadbeef);
    res = call_OpenPort(does_not_existW, &hPort);
    ok (!res, "got %lu with 0x%lx and %p (expected '0' and 0xdeadbeef)\n",
            res, GetLastError(), hPort);
    if (res) pClosePort(hPort);

    SetLastError(0xdeadbeef);
    res = call_OpenPort(emptyW, &hPort);
    ok (!res, "got %lu with 0x%lx and %p (expected '0' and 0xdeadbeef)\n",
            res, GetLastError(), hPort);
    if (res) pClosePort(hPort);


    /* NULL as name crash native localspl (w2k+xp) */
    if (0) {
        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = call_OpenPort(NULL, &hPort);
        trace("got %lu with %lu and %p\n", res, GetLastError(), hPort);
    }

}

static void test_file_port(void)
{
    WCHAR printer_name[256];
    DOC_INFO_1W doc_info;
    HANDLE hport, hf;
    BYTE buf[16];
    DWORD no;
    BOOL res;

    if ((!pOpenPort && !pOpenPort2) || !pClosePort || !pStartDocPort ||
            !pWritePort || !pEndDocPort)
    {
        win_skip("FILE: port\n");
        return;
    }

    no = ARRAY_SIZE(printer_name);
    if (!GetDefaultPrinterW(printer_name, &no))
    {
        skip("no default printer\n");
        return;
    }

    SetLastError(0xdeadbeef);
    res = call_OpenPort(tempfileW, &hport);
    ok(!res, "OpenPort succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "GetLastError() = %lu\n", GetLastError());

    res = call_OpenPort((WCHAR *)L"FILE", &hport);
    ok(!res, "OpenPort succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "GetLastError() = %lu\n", GetLastError());

    res = call_OpenPort((WCHAR *)L"FILE:", &hport);
    ok(res, "OpenPort failed (%lu)\n", GetLastError());

    SetLastError(0xdeadbeef);
    res = pWritePort(hport, (BYTE *)"test", 4, &no);
    ok(!res, "WritePort succeeded\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError() = %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    res = pStartDocPort(hport, printer_name, 1, 1, NULL);
    ok(!res, "StartDocPort succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %lu\n", GetLastError());

    memset(&doc_info, 0, sizeof(doc_info));
    SetLastError(0xdeadbeef);
    res = pStartDocPort(hport, printer_name, 1, 1, (BYTE *)&doc_info);
    ok(!res, "StartDocPort succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %lu\n", GetLastError());

    doc_info.pOutputFile = tempfileW;
    res = pStartDocPort(hport, printer_name, 1, 1, (BYTE *)&doc_info);
    ok(res, "StartDocPort failed (%lu)\n", GetLastError());

    res = pStartDocPort(hport, printer_name, 1, 1, (BYTE *)&doc_info);
    ok(res, "StartDocPort failed (%lu)\n", GetLastError());

    res = pWritePort(hport, (BYTE *)"test", 4, &no);
    ok(res, "WritePort failed (%lu)\n", GetLastError());
    ok(no == 4, "no = %ld\n", no);

    res = pEndDocPort(hport);
    ok(res, "EndDocPort failed (%lu)\n", GetLastError());
    res = pEndDocPort(hport);
    ok(res, "EndDocPort failed (%lu)\n", GetLastError());

    hf = CreateFileW(tempfileW, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(hf != INVALID_HANDLE_VALUE, "CreateFile failed (%lu)\n", GetLastError());
    res = ReadFile(hf, buf, sizeof(buf), &no, NULL);
    ok(res, "ReadFile failed (%lu)\n", GetLastError());
    ok(no == 4, "no = %ld\n", no);
    CloseHandle(hf);

    res = pClosePort(hport);
    ok(res, "ClosePort failed (%lu)\n", GetLastError());
}

/* ########################### */

static void test_XcvClosePort(void)
{
    DWORD   res;
    HANDLE hXcv2;


    if (0)
    {
        /* crash with native localspl.dll (w2k+xp) */
        pXcvClosePort(NULL);
        pXcvClosePort(INVALID_HANDLE_VALUE);
    }


    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = call_XcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv2);
    ok(res, "returned %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);

    if (res) {
        SetLastError(0xdeadbeef);
        res = pXcvClosePort(hXcv2);
        ok(res, "returned %ld with %lu (expected '!= 0')\n", res, GetLastError());

        if (0)
        {
            /* test for "Double Free": crash with native localspl.dll (w2k+xp) */
            pXcvClosePort(hXcv2);
        }
    }
}

/* ########################### */

static void test_XcvDataPort_AddPort(void)
{
    DWORD   res;

    /*
     * The following tests crash with native localspl.dll on w2k and xp,
     * but it works, when the native dll (w2k and xp) is used in wine.
     * also tested (same crash): replacing emptyW with portname_lpt1W
     * and replacing "NULL, 0, NULL" with "buffer, MAX_PATH, &needed"
     *
     * We need to use a different API (AddPortEx) instead
     */
    if (0)
    {
    /* create a Port for a normal, writable file */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());

    /* add our testport again */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    ok( res == ERROR_ALREADY_EXISTS, "returned %ld with %lu (expected ERROR_ALREADY_EXISTS)\n", res, GetLastError());

    /* create a well-known Port  */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) portname_lpt1W, (lstrlenW(portname_lpt1W) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    ok( res == ERROR_ALREADY_EXISTS, "returned %ld with %lu (expected ERROR_ALREADY_EXISTS)\n", res, GetLastError());

    /* ERROR_ALREADY_EXISTS is also returned from native localspl.dll on wine,
       when "RPT1:" was already installed for redmonnt.dll:
       res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) portname_rpt1W, ...
    */

    /* cleanup */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());
    }

}

/* ########################### */

static void test_XcvDataPort_ConfigureLPTPortCommandOK(void)
{
    CHAR    org_value[16];
    CHAR    buffer[16];
    HKEY    hroot = NULL;
    DWORD   res;
    DWORD   needed;


    /* Read the original value from the registry */
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WinNT_CV_WindowsA, 0, KEY_ALL_ACCESS, &hroot);
    if (res == ERROR_ACCESS_DENIED) {
        skip("ACCESS_DENIED\n");
        return;
    }

    if (res != ERROR_SUCCESS) {
        /* unable to open the registry: skip the test */
        skip("got %ld\n", res);
        return;
    }
    org_value[0] = '\0';
    needed = sizeof(org_value)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) org_value, &needed);
    ok( (res == ERROR_SUCCESS) || (res == ERROR_FILE_NOT_FOUND),
        "returned %lu and %lu for \"%s\" (expected ERROR_SUCCESS or "
        "ERROR_FILE_NOT_FOUND)\n", res, needed, org_value);

    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);

    /* set to "0" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_0W, sizeof(num_0W), NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'ConfigureLPTPortCommandOK' not supported\n");
        return;
    }
    if (res == ERROR_ACCESS_DENIED) {
        skip("'ConfigureLPTPortCommandOK' failed with ERROR_ACCESS_DENIED\n");
        return;
    }
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_0A) == 0),
        "returned %ld and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_0A);


    /* set to "1" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_1W, sizeof(num_1W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_1A) == 0),
        "returned %ld and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_1A);

    /* set to "999999" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_999999W, sizeof(num_999999W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_999999A) == 0),
        "returned %ld and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_999999A);

    /* set to "1000000" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_1000000W, sizeof(num_1000000W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_1000000A) == 0),
        "returned %ld and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_1000000A);

    /*  using cmd_ConfigureLPTPortCommandOKW with does_not_existW:
        the string "does_not_exist" is written to the registry */


    /* restore the original value */
    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);
    if (org_value[0]) {
        res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)org_value, lstrlenA(org_value)+1);
        ok(res == ERROR_SUCCESS, "unable to restore original value (got %lu): %s\n", res, org_value);
    }

    RegCloseKey(hroot);

}

/* ########################### */

static void test_XcvDataPort_DeletePort(void)
{
    DWORD   res;
    DWORD   needed;


    /* cleanup: just to make sure */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    ok( !res  || (res == ERROR_FILE_NOT_FOUND),
        "returned %ld with %lu (expected ERROR_SUCCESS or ERROR_FILE_NOT_FOUND)\n",
        res, GetLastError());


    /* ToDo: cmd_AddPortW for tempfileW, then cmd_DeletePortW for the existing Port */


    /* try to delete a nonexistent Port */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    ok( res == ERROR_FILE_NOT_FOUND,
        "returned %ld with %lu (expected ERROR_FILE_NOT_FOUND)\n", res, GetLastError());

    /* emptyW as Portname: ERROR_FILE_NOT_FOUND is returned */
    /* NULL as Portname: Native localspl.dll crashed */

}

/* ########################### */

static void test_XcvDataPort_GetTransmissionRetryTimeout(void)
{
    CHAR    org_value[16];
    HKEY    hroot = NULL;
    DWORD   buffer[2];
    DWORD   res;
    DWORD   needed;
    DWORD   len;


    /* ask for needed size */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'GetTransmissionRetryTimeout' not supported\n");
        return;
    }
    len = sizeof(DWORD);
    ok( (res == ERROR_INSUFFICIENT_BUFFER) && (needed == len),
        "returned %ld with %lu and %lu (expected ERROR_INSUFFICIENT_BUFFER "
        "and '%lu')\n", res, GetLastError(), needed, len);
    len = needed;

    /* Read the original value from the registry */
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WinNT_CV_WindowsA, 0, KEY_ALL_ACCESS, &hroot);
    if (res == ERROR_ACCESS_DENIED) {
        skip("ACCESS_DENIED\n");
        return;
    }

    if (res != ERROR_SUCCESS) {
        /* unable to open the registry: skip the test */
        skip("got %ld\n", res);
        return;
    }

    org_value[0] = '\0';
    needed = sizeof(org_value)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) org_value, &needed);
    ok( (res == ERROR_SUCCESS) || (res == ERROR_FILE_NOT_FOUND),
        "returned %lu and %lu for \"%s\" (expected ERROR_SUCCESS or "
        "ERROR_FILE_NOT_FOUND)\n", res, needed, org_value);

    /* Get default value (documented as 90 in the w2k reskit, but that is wrong) */
    res = RegDeleteValueA(hroot, TransmissionRetryTimeoutA);
    if (res == ERROR_ACCESS_DENIED) {
        RegCloseKey(hroot);
        skip("ACCESS_DENIED\n");
        return;
    }
    ok( res == ERROR_SUCCESS, "RegDeleteValue(TransmissionRetryTimeout) returned %lu\n", res);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 45),
        "returned %ld with %lu and %lu for %ld\n (expected ERROR_SUCCESS "
        "for '45')\n", res, GetLastError(), needed, buffer[0]);

    /* the default timeout is returned, when the value is empty */
    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)emptyA, 1);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu (%lu)\n", res, GetLastError());
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 45),
        "returned %ld with %lu and %lu for %ld\n (expected ERROR_SUCCESS "
        "for '45')\n", res, GetLastError(), needed, buffer[0]);

    /* the dialog is limited (1 - 999999), but that is done somewhere else */
    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_0A, lstrlenA(num_0A)+1);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", GetLastError());
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 0),
        "returned %ld with %lu and %lu for %ld\n (expected ERROR_SUCCESS "
        "for '0')\n", res, GetLastError(), needed, buffer[0]);


    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_1A, lstrlenA(num_1A)+1);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", GetLastError());
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 1),
        "returned %ld with %lu and %lu for %ld\n (expected 'ERROR_SUCCESS' "
        "for '1')\n", res, GetLastError(), needed, buffer[0]);

    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_999999A, lstrlenA(num_999999A)+1);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", GetLastError());
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 999999),
        "returned %ld with %lu and %lu for %ld\n (expected ERROR_SUCCESS "
        "for '999999')\n", res, GetLastError(), needed, buffer[0]);


    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_1000000A, lstrlenA(num_1000000A)+1);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", GetLastError());
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 1000000),
        "returned %ld with %lu and %lu for %ld\n (expected ERROR_SUCCESS "
        "for '1000000')\n", res, GetLastError(), needed, buffer[0]);

    /* restore the original value */
    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);
    if (org_value[0]) {
        res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)org_value, lstrlenA(org_value)+1);
        ok(res == ERROR_SUCCESS, "unable to restore original value (got %lu): %s\n", res, org_value);
    }

    RegCloseKey(hroot);
}

/* ########################### */

static void test_XcvDataPort_MonitorUI(void)
{
    DWORD   res;
    BYTE    buffer[MAX_PATH + 2];
    DWORD   needed;
    DWORD   len;


    /* ask for needed size */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'MonitorUI' nor supported\n");
        return;
    }
    ok( (res == ERROR_INSUFFICIENT_BUFFER) && (needed <= MAX_PATH),
        "returned %ld with %lu and 0x%lx (expected 'ERROR_INSUFFICIENT_BUFFER' "
        " and '<= MAX_PATH')\n", res, GetLastError(), needed);

    if (needed > MAX_PATH) {
        skip("buffer overflow (%lu)\n", needed);
        return;
    }
    len = needed;

    /* the command is required */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, emptyW, NULL, 0, NULL, 0, &needed);
    ok( res == ERROR_INVALID_PARAMETER, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_INVALID_PARAMETER')\n", res, GetLastError(), needed);

    if (0) {
        /* crash with native localspl.dll (w2k+xp) */
        pXcvDataPort(hXcv, NULL, NULL, 0, buffer, MAX_PATH, &needed);
        pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, NULL, len, &needed);
        pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len, NULL);
    }


    /* hXcv is ignored for the command "MonitorUI" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(NULL, cmd_MonitorUIW, NULL, 0, buffer, len, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);


    /* pszDataName is case-sensitive */
    memset(buffer, 0, len);
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUI_lcaseW, NULL, 0, buffer, len, &needed);
    ok( res == ERROR_INVALID_PARAMETER, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_INVALID_PARAMETER')\n", res, GetLastError(), needed);

    /* off by one: larger  */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len+1, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);


    /* off by one: smaller */
    /* the buffer is not modified for NT4, w2k, XP */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len-1, &needed);
    ok( res == ERROR_INSUFFICIENT_BUFFER, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_INSUFFICIENT_BUFFER')\n", res, GetLastError(), needed);

    /* Normal use. The DLL-Name without a Path is returned */
    memset(buffer, 0, len);
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);


    /* small check without access-rights: */
    if (!hXcv_noaccess) return;

    /* The ACCESS_MASK is ignored for "MonitorUI" */
    memset(buffer, 0, len);
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv_noaccess, cmd_MonitorUIW, NULL, 0, buffer, sizeof(buffer), &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu and 0x%lx "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);
}

/* ########################### */

static void test_XcvDataPort_PortIsValid(void)
{
    DWORD   res;
    DWORD   needed;

    /* normal use: "LPT1:" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'PostIsValid' not supported\n");
        return;
    }
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());


    if (0) {
        /* crash with native localspl.dll (w2k+xp) */
        pXcvDataPort(hXcv, cmd_PortIsValidW, NULL, 0, NULL, 0, &needed);
    }


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(NULL, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, &needed);
    todo_wine ok( res == ERROR_INVALID_PARAMETER || broken(res == ERROR_SUCCESS),
            "returned %ld with %lu\n", res, GetLastError());

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, NULL);
    todo_wine ok( res == ERROR_INVALID_PARAMETER || broken(res == ERROR_SUCCESS),
            "returned %ld with %lu\n", res, GetLastError());


    /* cbInputData is ignored */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, 0, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, 1, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W) -1, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W) -2, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    /* an empty name is not allowed */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) emptyW, sizeof(emptyW), NULL, 0, &needed);
    todo_wine ok( res == ERROR_FILE_NOT_FOUND || broken(res == ERROR_PATH_NOT_FOUND),
        "returned %ld with %lu and 0x%lx\n", res, GetLastError(), needed);


    /* a directory is not allowed */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) tempdirW, (lstrlenW(tempdirW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    /* XP(admin): ERROR_INVALID_NAME, XP(user): ERROR_PATH_NOT_FOUND, w2k ERROR_ACCESS_DENIED */
    ok( (res == ERROR_INVALID_NAME) || (res == ERROR_PATH_NOT_FOUND) ||
        (res == ERROR_ACCESS_DENIED), "returned %ld with %lu and 0x%lx "
        "(expected ERROR_INVALID_NAME, ERROR_PATH_NOT_FOUND or ERROR_ACCESS_DENIED)\n",
        res, GetLastError(), needed);


    /* test more valid well known Ports: */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt2W, sizeof(portname_lpt2W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_com1W, sizeof(portname_com1W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_com2W, sizeof(portname_com2W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_fileW, sizeof(portname_fileW), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    /* a normal, writable file is allowed */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %ld with %lu and 0x%lx (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    /* small check without access-rights: */
    if (!hXcv_noaccess) return;

    /* The ACCESS_MASK from XcvOpenPort is ignored in "PortIsValid" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv_noaccess, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %ld with %lu (expected ERROR_SUCCESS)\n", res, GetLastError());

}

/* ########################### */

static void test_XcvOpenPort(void)
{
    DWORD   res;
    HANDLE  hXcv2;


    if (0)
    {
        /* crash with native localspl.dll (w2k+xp) */
        call_XcvOpenPort(NULL, SERVER_ACCESS_ADMINISTER, &hXcv2);
        call_XcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, NULL);
    }


    /* The returned handle is the result from a previous "spoolss.dll,DllAllocSplMem" */
    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = call_XcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv2);
    ok(res, "returned %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);


    /* The ACCESS_MASK is not checked in XcvOpenPort */
    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = call_XcvOpenPort(emptyW, 0, &hXcv2);
    ok(res, "returned %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);


    /* A copy of pszObject is saved in the Memory-Block */
    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = call_XcvOpenPort(portname_lpt1W, SERVER_ALL_ACCESS, &hXcv2);
    ok(res, "returned %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);

    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = call_XcvOpenPort(portname_fileW, SERVER_ALL_ACCESS, &hXcv2);
    ok(res, "returned %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);

}

/* ########################### */

#define GET_MONITOR_FUNC(name) \
    if (pm) p##name = pm->Monitor.pfn##name; \
    else if (pm2) p##name = pm2->pfn##name;

#define GET_MONITOR_FUNC2(name) \
    if (pm) p##name = pm->Monitor.pfn##name; \
    else if (pm2) p##name##2 = pm2->pfn##name;

START_TEST(localmon)
{
    DWORD   numentries;
    DWORD   res;

    LoadLibraryA("winspool.drv");
    /* This DLL does not exist on Win9x */
    hdll = LoadLibraryA("localspl.dll");
    if (!hdll) {
        skip("localspl.dll cannot be loaded, most likely running on Win9x\n");
        return;
    }

    tempdirW[0] = '\0';
    tempfileW[0] = '\0';
    res = GetTempPathW(MAX_PATH, tempdirW);
    ok(res != 0, "with %lu\n", GetLastError());
    res = GetTempFileNameW(tempdirW, wineW, 0, tempfileW);
    ok(res != 0, "with %lu\n", GetLastError());

    pInitializePrintMonitor = (void *) GetProcAddress(hdll, "InitializePrintMonitor");
    pInitializePrintMonitor2 = (void *) GetProcAddress(hdll, "InitializePrintMonitor2");

    if (!pInitializePrintMonitor) {
        /* The Monitor for "Local Ports" was in a separate dll before w2k */
        hlocalmon = LoadLibraryA("localmon.dll");
        if (hlocalmon) {
            pInitializePrintMonitor = (void *) GetProcAddress(hlocalmon, "InitializePrintMonitor");
        }
    }
    if (!pInitializePrintMonitor && !pInitializePrintMonitor2) {
        skip("InitializePrintMonitor or InitializePrintMonitor2 not found\n");
        return;
    }

    /* Native localmon.dll / localspl.dll need a valid Port-Entry in:
       a) since xp: HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports 
       b) up to w2k: Section "Ports" in win.ini
       or InitializePrintMonitor fails. */
    if (pInitializePrintMonitor)
        pm = pInitializePrintMonitor(Monitors_LocalPortW);
    else if (pInitializePrintMonitor2) {
        MONITORINIT init;

        memset(&init, 0, sizeof(init));
        init.cbSize = sizeof(init);
        init.hckRegistryRoot = 0;
        init.pMonitorReg = &monreg;
        init.bLocal = TRUE;

        pm2 = pInitializePrintMonitor2(&init, &hmon);
        ok(pm2 != NULL, "InitializePrintMonitor2 error %lu\n", GetLastError());
        ok(pm2->cbSize >= FIELD_OFFSET(MONITOR2, pfnSendRecvBidiDataFromPort), "wrong cbSize %lu\n", pm2->cbSize);
    }

    if (pm || pm2) {
        if (pm) {
            ok(pm->dwMonitorSize == sizeof(MONITOR), "wrong dwMonitorSize %lu\n", pm->dwMonitorSize);
            numentries = (pm->dwMonitorSize ) / sizeof(VOID *);
            /* NT4: 14, since w2k: 17 */
            ok( numentries == 14 || numentries == 17,
                "dwMonitorSize (%lu) => %lu Functions\n", pm->dwMonitorSize, numentries);
        }
        else if (pm2) {
            numentries = (pm2->cbSize ) / sizeof(VOID *);
            ok( numentries >= 20,
                "cbSize (%lu) => %lu Functions\n", pm2->cbSize, numentries);
        }

        GET_MONITOR_FUNC2(EnumPorts);
        GET_MONITOR_FUNC2(OpenPort);
        GET_MONITOR_FUNC(StartDocPort);
        GET_MONITOR_FUNC(WritePort);
        GET_MONITOR_FUNC(ReadPort);
        GET_MONITOR_FUNC(EndDocPort);
        GET_MONITOR_FUNC(ClosePort);
        GET_MONITOR_FUNC2(AddPort);
        GET_MONITOR_FUNC2(AddPortEx);
        GET_MONITOR_FUNC2(ConfigurePort);
        GET_MONITOR_FUNC2(DeletePort);
        GET_MONITOR_FUNC(GetPrinterDataFromPort);
        GET_MONITOR_FUNC(SetPortTimeOuts);
        GET_MONITOR_FUNC2(XcvOpenPort);
        GET_MONITOR_FUNC(XcvDataPort);
        GET_MONITOR_FUNC(XcvClosePort);

        if ((pXcvOpenPort || pXcvOpenPort2) && pXcvDataPort && pXcvClosePort) {
            SetLastError(0xdeadbeef);
            res = call_XcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv);
            ok(res, "hXcv: %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv);

            SetLastError(0xdeadbeef);
            res = call_XcvOpenPort(emptyW, 0, &hXcv_noaccess);
            ok(res, "hXcv_noaccess: %ld with %lu and %p (expected '!= 0')\n", res, GetLastError(), hXcv_noaccess);
        }
    }

    test_InitializePrintMonitor();
    test_InitializePrintMonitor2();

    find_installed_ports();

    test_AddPort();
    test_AddPortEx();
    test_ClosePort();
    test_ConfigurePort();
    test_DeletePort();
    test_EnumPorts();
    test_OpenPort();
    test_file_port();

    if ( !hXcv ) {
        skip("Xcv not supported\n");
    }
    else
    {
        test_XcvClosePort();
        test_XcvDataPort_AddPort();
        test_XcvDataPort_ConfigureLPTPortCommandOK();
        test_XcvDataPort_DeletePort();
        test_XcvDataPort_GetTransmissionRetryTimeout();
        test_XcvDataPort_MonitorUI();
        test_XcvDataPort_PortIsValid();
        test_XcvOpenPort();

        pXcvClosePort(hXcv);
    }
    if (hXcv_noaccess) pXcvClosePort(hXcv_noaccess);

    /* Cleanup our temporary file */
    DeleteFileW(tempfileW);
}

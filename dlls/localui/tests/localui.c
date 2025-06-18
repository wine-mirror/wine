/*
 * Unit test suite for the Local Printmonitor User Interface
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winnls.h"
#include "winreg.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/test.h"


/* ##### */

static HMODULE  hdll;
static PMONITORUI (WINAPI *pInitializePrintMonitorUI)(VOID);
static PMONITORUI pui;
static BOOL  (WINAPI *pAddPortUI)(PCWSTR, HWND, PCWSTR, PWSTR *);
static BOOL  (WINAPI *pConfigurePortUI)(PCWSTR, HWND, PCWSTR);
static BOOL  (WINAPI *pDeletePortUI)(PCWSTR, HWND, PCWSTR);

static LPBYTE pi_buffer;
static DWORD pi_numports;
static DWORD pi_needed;

static PORT_INFO_2W * lpt_present;
static PORT_INFO_2W * com_present;
static PORT_INFO_2W * file_present;

static LPWSTR   lpt_absent;
static LPWSTR   com_absent;

/* ########################### */

static PORT_INFO_2W * find_portinfo2(LPCWSTR pPort)
{
    PORT_INFO_2W * pi;
    DWORD   res;

    if (!pi_buffer) {
        res = EnumPortsW(NULL, 2, NULL, 0, &pi_needed, &pi_numports);
        if (!res && (GetLastError() == RPC_S_SERVER_UNAVAILABLE)) {
            win_skip("The service 'Spooler' is required for many tests\n");
            return NULL;
        }
        ok(!res, "EnumPorts succeeded: got %ld\n", res);
        pi_buffer = malloc(pi_needed);
        res = EnumPortsW(NULL, 2, pi_buffer, pi_needed, &pi_needed, &pi_numports);
        ok(res == 1, "EnumPorts failed: got %ld\n", res);
    }
    if (pi_buffer) {
        pi = (PORT_INFO_2W *) pi_buffer;
        res = 0;
        while (pi_numports > res) {
            if (lstrcmpiW(pi->pPortName, pPort) == 0) {
                return pi;
            }
            pi++;
            res++;
        }
    }
    return NULL;
}


/* ########################### */

static LPCSTR load_functions(void)
{
    LPCSTR  ptr;

    ptr = "localui.dll";
    hdll = LoadLibraryA(ptr);
    if (!hdll) return ptr;

    ptr = "InitializePrintMonitorUI";
    pInitializePrintMonitorUI = (VOID *) GetProcAddress(hdll, ptr);
    if (!pInitializePrintMonitorUI) return ptr;

    return NULL;
}

/* ########################### */

static void test_AddPortUI(void)
{
    DWORD   res;
    LPWSTR  new_portname;

    /* not present before w2k */
    if (!pAddPortUI) {
        skip("AddPortUI not found\n");
        return;
    }

    SetLastError(0xdeadbeef);
    res = pAddPortUI(NULL, NULL, NULL, NULL);
    ok( !res &&
        ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pAddPortUI(NULL, NULL, L"", NULL);
    ok( !res &&
        ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pAddPortUI(NULL, NULL, L"does_not_exist", NULL);
    ok( !res &&
        ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    if (winetest_interactive) {
        SetLastError(0xdeadbeef);
        new_portname = NULL;
        /*
         * - On MSDN, you can read that no dialog should be displayed when hWnd
         *   is NULL, but native localui does not care
         * - When the new port already exists,
         *   TRUE is returned, but new_portname is NULL
         * - When the new port starts with "COM" or "LPT",
         *   FALSE is returned with ERROR_NOT_SUPPORTED on windows
         */
        res = pAddPortUI(NULL, NULL, L"Local Port", &new_portname);
        ok( res ||
            (GetLastError() == ERROR_CANCELLED) ||
            (GetLastError() == ERROR_ACCESS_DENIED) ||
            (GetLastError() == ERROR_NOT_SUPPORTED),
            "got %ld with %lu and %p (expected '!= 0' or '0' with: "
            "ERROR_CANCELLED, ERROR_ACCESS_DENIED or ERROR_NOT_SUPPORTED)\n",
            res, GetLastError(), new_portname);

        GlobalFree(new_portname);
    }
}

/* ########################### */

static void test_ConfigurePortUI(void)
{
    DWORD   res;

    /* not present before w2k */
    if (!pConfigurePortUI) {
        skip("ConfigurePortUI not found\n");
        return;
    }

    SetLastError(0xdeadbeef);
    res = pConfigurePortUI(NULL, NULL, NULL);
    ok( !res &&
        ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pConfigurePortUI(NULL, NULL, L"");
    ok( !res &&
        ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());


    SetLastError(0xdeadbeef);
    res = pConfigurePortUI(NULL, NULL, L"does_not_exist");
    ok( !res &&
        ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    if (winetest_interactive && lpt_present) {
        SetLastError(0xdeadbeef);
        res = pConfigurePortUI(NULL, NULL, lpt_present->pPortName);
        ok( res ||
            (GetLastError() == ERROR_CANCELLED) || (GetLastError() == ERROR_ACCESS_DENIED),
            "got %ld with %lu (expected '!= 0' or '0' with: ERROR_CANCELLED or "
            "ERROR_ACCESS_DENIED)\n", res, GetLastError());
    }

    if (lpt_absent) {
        SetLastError(0xdeadbeef);
        res = pConfigurePortUI(NULL, NULL, lpt_absent);
        ok( !res &&
            ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
            "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
            "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());
    }

    if (winetest_interactive && com_present) {
        SetLastError(0xdeadbeef);
        res = pConfigurePortUI(NULL, NULL, com_present->pPortName);
        ok( res ||
            (GetLastError() == ERROR_CANCELLED) || (GetLastError() == ERROR_ACCESS_DENIED),
            "got %ld with %lu (expected '!= 0' or '0' with: ERROR_CANCELLED or "
            "ERROR_ACCESS_DENIED)\n", res, GetLastError());
    }

    if (com_absent) {
        SetLastError(0xdeadbeef);
        res = pConfigurePortUI(NULL, NULL, com_absent);
        ok( !res &&
            ((GetLastError() == ERROR_UNKNOWN_PORT) || (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
            "got %ld with %lu (expected '0' with: ERROR_UNKNOWN_PORT or "
            "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    }

    if (winetest_interactive && file_present) {
        SetLastError(0xdeadbeef);
        res = pConfigurePortUI(NULL, NULL, L"FILE:");
        ok( !res &&
            ((GetLastError() == ERROR_CANCELLED) || (GetLastError() == ERROR_ACCESS_DENIED)),
            "got %ld with %lu (expected '0' with: ERROR_CANCELLED or "
            "ERROR_ACCESS_DENIED)\n", res, GetLastError());
    }
}

/* ########################### */

START_TEST(localui)
{
    LPCSTR   ptr;
    DWORD   numentries;
    PORT_INFO_2W * pi2;
    WCHAR   bufferW[16];
    DWORD   id;

    /* localui.dll does not exist before w2k */
    ptr = load_functions();
    if (ptr) {
        skip("%s not found\n", ptr);
        return;
    }

    pui = pInitializePrintMonitorUI();
    if (pui) {
        numentries = (pui->dwMonitorUISize - sizeof(DWORD)) / sizeof(VOID *);
        ok( numentries == 3,
                "dwMonitorUISize (%ld) => %ld Functions\n", pui->dwMonitorUISize, numentries);

        if (numentries > 2) {
            pAddPortUI = pui->pfnAddPortUI;
            pConfigurePortUI = pui->pfnConfigurePortUI;
            pDeletePortUI = pui->pfnDeletePortUI;
        }
    }

    /* find installed ports */

    /* "FILE:" */
    file_present = find_portinfo2(L"FILE:");

    if (!pi_numports)   /* Nothing to test without a port */
        return;

    id = 0;
    /* "LPT1:" - "LPT9:" */
    while (((lpt_present == NULL) || (lpt_absent == NULL)) && id < 9) {
        id++;
        swprintf(bufferW, ARRAY_SIZE(bufferW), L"LPT%lu:", id);
        pi2 = find_portinfo2(bufferW);
        if (pi2 && (lpt_present == NULL)) lpt_present = pi2;
        if (!pi2 && (lpt_absent == NULL)) lpt_absent = wcsdup(bufferW);
    }

    id = 0;
    /* "COM1:" - "COM9:" */
    while (((com_present == NULL) || (com_absent == NULL)) && id < 9) {
        id++;
        swprintf(bufferW, ARRAY_SIZE(bufferW), L"COM%lu:", id);
        pi2 = find_portinfo2(bufferW);
        if (pi2 && (com_present == NULL)) com_present = pi2;
        if (!pi2 && (com_absent == NULL)) com_absent = wcsdup(bufferW);
    }

    test_AddPortUI();
    test_ConfigurePortUI();

    free(lpt_absent);
    free(com_absent);
    free(pi_buffer);
}

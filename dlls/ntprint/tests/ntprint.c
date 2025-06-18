/*
 * Unit test suite for the Spooler Setup API (Printing)
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
#include "wine/test.h"


/* ##### */

static HMODULE  hdll;
static HANDLE (WINAPI *pPSetupCreateMonitorInfo)(DWORD, const WCHAR *);
static VOID   (WINAPI *pPSetupDestroyMonitorInfo)(HANDLE);
static BOOL   (WINAPI *pPSetupEnumMonitor)(HANDLE, DWORD, LPWSTR, LPDWORD);

/* ########################### */

static LPCSTR load_functions(void)
{
    LPCSTR  ptr;

    ptr = "ntprint.dll";
    hdll = LoadLibraryA(ptr);
    if (!hdll) return ptr;

    ptr = "PSetupCreateMonitorInfo";
    pPSetupCreateMonitorInfo = (VOID *) GetProcAddress(hdll, ptr);
    if (!pPSetupCreateMonitorInfo) return ptr;

    ptr = "PSetupDestroyMonitorInfo";
    pPSetupDestroyMonitorInfo = (VOID *) GetProcAddress(hdll, ptr);
    if (!pPSetupDestroyMonitorInfo) return ptr;

    ptr = "PSetupEnumMonitor";
    pPSetupEnumMonitor = (VOID *) GetProcAddress(hdll, ptr);
    if (!pPSetupEnumMonitor) return ptr;

    return NULL;
}

/* ########################### */

static void test_PSetupCreateMonitorInfo(VOID)
{
    HANDLE  mi;
    WCHAR buffer[1024] = {'\\','\\'};
    DWORD len = ARRAY_SIZE(buffer) - 2;
    GetComputerNameW(buffer + 2, &len);

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(0, NULL);
    if (!mi && (GetLastError() == RPC_S_SERVER_UNAVAILABLE)) {
        win_skip("The service 'Spooler' is required for many tests\n");
        return;
    }
    ok( mi != NULL, "got %p with %lu (expected '!= NULL')\n", mi, GetLastError());
    if (mi) pPSetupDestroyMonitorInfo(mi);

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(0, buffer);
    ok( mi != NULL, "got %p with %lu (expected '!= NULL')\n", mi, GetLastError());
    if (mi) pPSetupDestroyMonitorInfo(mi);

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(0, buffer + 1);
    todo_wine {
    ok( mi == NULL, "got %p\n", mi );
    ok( GetLastError() == ERROR_INVALID_NAME, "got %ld\n", GetLastError() );
    }
    if (mi) pPSetupDestroyMonitorInfo(mi);
}

/* ########################### */

static void test_PSetupDestroyMonitorInfo(VOID)
{
    HANDLE  mi;


    SetLastError(0xdeadbeef);
    pPSetupDestroyMonitorInfo(NULL);
    /* lasterror is returned */
    trace("returned with %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(0, NULL);
    if (!mi && (GetLastError() == RPC_S_SERVER_UNAVAILABLE)) {
        win_skip("The service 'Spooler' is required for many tests\n");
        return;
    }
    ok( mi != NULL, "got %p with %lu (expected '!= NULL')\n", mi, GetLastError());

    if (!mi) return;

    SetLastError(0xdeadbeef);
    pPSetupDestroyMonitorInfo(mi);
    /* lasterror is returned */
    trace("returned with %lu\n", GetLastError());

    /* Trying to destroy the handle twice crashes with native ntprint.dll */
    if (0) {
        SetLastError(0xdeadbeef);
        pPSetupDestroyMonitorInfo(mi);
        trace(" with %lu\n", GetLastError());
    }

}

/* ########################### */

static void test_PSetupEnumMonitor(VOID)
{
    HANDLE  mi;
    WCHAR   buffer[MAX_PATH+2];
    DWORD   minsize = 0;
    DWORD   size;
    DWORD   res;
    DWORD   index=0;

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(0, NULL);
    if (!mi) {
        skip("PSetupCreateMonitorInfo\n");
        return;
    }

    minsize = 0;
    SetLastError(0xdeadbeef);
    res = pPSetupEnumMonitor(mi, 0, NULL, &minsize);
    ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) && (minsize > 0),
        "got %lu with %lu and %lu (expected '0' with ERROR_INSUFFICIENT_BUFFER "
        "and '> 0')\n", res, GetLastError(), minsize);


    size = ARRAY_SIZE(buffer);
    if ((minsize + 1) > size) {
        skip("overflow: %lu\n", minsize);
        pPSetupDestroyMonitorInfo(mi);
        return;
    }

    if (0) {
        /* XP: ERROR_INVALID_PARAMETER,  w2k: Crash */
        SetLastError(0xdeadbeef);
        size = ARRAY_SIZE(buffer);
        res = pPSetupEnumMonitor(NULL, 0, buffer, &size);
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
    }

    if (0) {
        /* XP: Crash,  w2k: Success (how can that work?) */
        SetLastError(0xdeadbeef);
        size = ARRAY_SIZE(buffer);
        res = pPSetupEnumMonitor(mi, 0, NULL, &size);
        trace("got %lu with %lu and %lu\n", res, GetLastError(), size);
    }

    if (0) {
        /* XP: ERROR_INVALID_PARAMETER,  w2k: Crash */
        SetLastError(0xdeadbeef);
        res = pPSetupEnumMonitor(mi, 0, buffer, NULL);
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %lu with %lu (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
    }

    SetLastError(0xdeadbeef);
    size = minsize - 1;
    res = pPSetupEnumMonitor(mi, 0, buffer, &size);
    ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "got %lu with %lu and %lu (expected '0' with ERROR_INSUFFICIENT_BUFFER)\n",
        res, GetLastError(), size);


    SetLastError(0xdeadbeef);
    size = minsize;
    res = pPSetupEnumMonitor(mi, 0, buffer, &size);
    ok( res, "got %lu with %lu and %lu (expected '!= 0')\n",
        res, GetLastError(), size);

    SetLastError(0xdeadbeef);
    size = minsize + 1;
    res = pPSetupEnumMonitor(mi, 0, buffer, &size);
    ok( res, "got %lu with %lu and %lu (expected '!= 0')\n",
        res, GetLastError(), size);

    /* try max. 20 monitors */
    while (res && (index < 20)) {
        SetLastError(0xdeadbeef);
        buffer[0] = '\0';
        size = ARRAY_SIZE(buffer);
        res = pPSetupEnumMonitor(mi, index, buffer, &size);
        ok( res || (GetLastError() == ERROR_NO_MORE_ITEMS),
            "(%lu) got %lu with %lu and %lu (expected '!=0' or: '0' with "
            "ERROR_NO_MORE_ITEMS)\n", index, res, GetLastError(), size);

        if (res) index++;
    }
    pPSetupDestroyMonitorInfo(mi);

}

/* ########################### */

START_TEST(ntprint)
{
    load_functions();

    test_PSetupCreateMonitorInfo();
    test_PSetupDestroyMonitorInfo();
    test_PSetupEnumMonitor();

}

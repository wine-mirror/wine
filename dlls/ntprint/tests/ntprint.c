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
static HANDLE (WINAPI *pPSetupCreateMonitorInfo)(LPVOID, LPVOID, LPVOID);
static VOID   (WINAPI *pPSetupDestroyMonitorInfo)(HANDLE);

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


    return NULL;
}

/* ########################### */

static void test_PSetupCreateMonitorInfo(VOID)
{
    HANDLE  mi;
    BYTE    buffer[1024] ;

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(NULL, NULL, NULL);
    ok( mi != NULL, "got %p with %u (expected '!= NULL')\n", mi, GetLastError());
    if (mi) pPSetupDestroyMonitorInfo(mi);


    memset(buffer, 0, sizeof(buffer));
    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(buffer, NULL, NULL);
    ok( mi != NULL, "got %p with %u (expected '!= NULL')\n", mi, GetLastError());
    if (mi) pPSetupDestroyMonitorInfo(mi);

}

/* ########################### */

static void test_PSetupDestroyMonitorInfo(VOID)
{
    HANDLE  mi;


    SetLastError(0xdeadbeef);
    pPSetupDestroyMonitorInfo(NULL);
    /* lasterror is returned */
    trace("returned with %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    mi = pPSetupCreateMonitorInfo(NULL, NULL, NULL);
    ok( mi != NULL, "got %p with %u (expected '!= NULL')\n", mi, GetLastError());

    if (!mi) return;

    SetLastError(0xdeadbeef);
    pPSetupDestroyMonitorInfo(mi);
    /* lasterror is returned */
    trace("returned with %u\n", GetLastError());

    /* Try to destroy the handle twice crash with native ntprint.dll */
    if (0) {
        SetLastError(0xdeadbeef);
        pPSetupDestroyMonitorInfo(mi);
        trace(" with %u\n", GetLastError());
    }

}

/* ########################### */

START_TEST(ntprint)
{
    LPCSTR ptr;

    /* ntprint.dll does not exist on win9x */
    ptr = load_functions();
    if (ptr) {
        skip("%s not found\n", ptr);
        return;
    }

    test_PSetupCreateMonitorInfo();
    test_PSetupDestroyMonitorInfo();

}

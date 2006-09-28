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

static HANDLE  hdll;
static LPMONITOREX (WINAPI *pInitializePrintMonitor)(LPWSTR);

static const WCHAR emptyW[] =    {0};

/* ##### */

static void test_InitializePrintMonitor(void)
{
    LPMONITOREX res;

    if (!pInitializePrintMonitor) return;

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(NULL);
    ok( (res == NULL) && (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %ld\n (expected NULL with " \
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor((LPWSTR) emptyW);
    ok( (res == NULL) && (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %ld\n (expected NULL with " \
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());
}


START_TEST(localmon)
{
    hdll = LoadLibraryA("localspl.dll");
    if (!hdll) return;

    pInitializePrintMonitor = (void *) GetProcAddress(hdll, "InitializePrintMonitor");
    test_InitializePrintMonitor();
}

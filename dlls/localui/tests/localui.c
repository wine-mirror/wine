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
static PMONITORUI (WINAPI *pInitializePrintMonitorUI)(VOID);
static PMONITORUI pui;
static BOOL  (WINAPI *pAddPortUI)(PCWSTR, HWND, PCWSTR, PWSTR *);
static BOOL  (WINAPI *pConfigurePortUI)(PCWSTR, HWND, PCWSTR);
static BOOL  (WINAPI *pDeletePortUI)(PCWSTR, HWND, PCWSTR);

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

START_TEST(localui)
{
    LPCSTR   ptr;
    DWORD   numentries;


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
                "dwMonitorUISize (%d) => %d Functions\n", pui->dwMonitorUISize, numentries);

        if (numentries > 2) {
            pAddPortUI = pui->pfnAddPortUI;
            pConfigurePortUI = pui->pfnConfigurePortUI;
            pDeletePortUI = pui->pfnDeletePortUI;
        }
    }

}

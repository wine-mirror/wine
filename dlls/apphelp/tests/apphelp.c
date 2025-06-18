/*
 * Copyright 2012 Detlef Riekenberg
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <initguid.h>
#include <exdisp.h>
#include <shlobj.h>
#include <urlmon.h>

#include "wine/test.h"

static HMODULE hdll;
static BOOL (WINAPI *pApphelpCheckShellObject)(REFCLSID, BOOL, ULONGLONG *);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

DEFINE_GUID(test_Microsoft_Browser_Architecture, 0xa5e46e3a, 0x8849, 0x11d1, 0x9d, 0x8c, 0x00, 0xc0, 0x4f, 0xc9, 0x9d, 0x61);
DEFINE_GUID(CLSID_MenuBand, 0x5b4dae26, 0xb807, 0x11d0, 0x98, 0x15, 0x00, 0xc0, 0x4f, 0xd9, 0x19, 0x72);
DEFINE_GUID(test_UserAssist, 0xdd313e04, 0xfeff, 0x11d1, 0x8e, 0xcd, 0x00, 0x00, 0xf8, 0x7a, 0x47, 0x0c);

static const CLSID * objects[] = {
    &GUID_NULL,
    /* used by IE */
    &test_Microsoft_Browser_Architecture,
    &CLSID_MenuBand,
    &CLSID_ShellLink,
    &CLSID_ShellWindows,
    &CLSID_InternetSecurityManager,
    &test_UserAssist,
    NULL,};

static void test_ApphelpCheckShellObject(void)
{
    ULONGLONG flags;
    BOOL res;
    int i;

    if (!pApphelpCheckShellObject)
    {
        win_skip("ApphelpCheckShellObject not available\n");
        return;
    }

    for (i = 0; objects[i]; i++)
    {
        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(objects[i], FALSE, &flags);
        ok(res && (flags == 0), "%s 0: got %d and %s with 0x%lx (expected TRUE and 0)\n",
            wine_dbgstr_guid(objects[i]), res, wine_dbgstr_longlong(flags), GetLastError());

        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(objects[i], TRUE, &flags);
        ok(res && (flags == 0), "%s 1: got %d and %s with 0x%lx (expected TRUE and 0)\n",
            wine_dbgstr_guid(objects[i]), res, wine_dbgstr_longlong(flags), GetLastError());

    }

    /* NULL as pointer to flags is checked */
    SetLastError(0xdeadbeef);
    res = pApphelpCheckShellObject(&GUID_NULL, FALSE, NULL);
    ok(res, "%s 0: got %d with 0x%lx (expected != FALSE)\n", wine_dbgstr_guid(&GUID_NULL), res, GetLastError());

    /* NULL as CLSID* crash on Windows */
    if (0)
    {
        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(NULL, FALSE, &flags);
        trace("NULL as CLSID*: got %d and %s with 0x%lx\n", res, wine_dbgstr_longlong(flags), GetLastError());
    }
}

START_TEST(apphelp)
{

    hdll = LoadLibraryA("apphelp.dll");
    if (!hdll) {
        win_skip("apphelp.dll not available\n");
        return;
    }
    pApphelpCheckShellObject = (void *) GetProcAddress(hdll, "ApphelpCheckShellObject");

    test_ApphelpCheckShellObject();
}

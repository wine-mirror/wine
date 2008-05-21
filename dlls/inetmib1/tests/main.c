/*
 * Copyright 2008 Juan Lang
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
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <snmp.h>

#include "wine/test.h"

static HMODULE inetmib1;

static void testInit(void)
{
    BOOL (WINAPI *pInit)(DWORD, HANDLE *, AsnObjectIdentifier *);
    BOOL ret;
    HANDLE event;
    AsnObjectIdentifier oid;

    pInit = (void *)GetProcAddress(inetmib1, "SnmpExtensionInit");
    if (!pInit)
    {
        skip("no SnmpExtensionInit\n");
        return;
    }
    /* Crash
    ret = pInit(0, NULL, NULL);
    ret = pInit(0, NULL, &oid);
    ret = pInit(0, &event, NULL);
     */
    ret = pInit(0, &event, &oid);
    ok(ret, "SnmpExtensionInit failed: %d\n", GetLastError());
    todo_wine
    ok(!strcmp("1.3.6.1.2.1.1", SnmpUtilOidToA(&oid)),
        "Expected 1.3.6.1.2.1.1, got %s\n", SnmpUtilOidToA(&oid));
}

START_TEST(main)
{
    inetmib1 = LoadLibraryA("inetmib1");
    testInit();
}

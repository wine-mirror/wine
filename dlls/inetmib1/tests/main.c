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
    ok(!strcmp("1.3.6.1.2.1.1", SnmpUtilOidToA(&oid)),
        "Expected 1.3.6.1.2.1.1, got %s\n", SnmpUtilOidToA(&oid));
}

static void testQuery(void)
{
    BOOL (WINAPI *pQuery)(BYTE, SnmpVarBindList *, AsnInteger32 *,
        AsnInteger32 *);
    BOOL ret, moreData;
    SnmpVarBindList list;
    AsnInteger32 error, index;
    UINT bogus[] = { 1,2,3,4 };
    UINT mib2System[] = { 1,3,6,1,2,1,1 };
    UINT mib2If[] = { 1,3,6,1,2,1,2 };
    UINT mib2IfTable[] = { 1,3,6,1,2,1,2,2 };
    SnmpVarBind vars[3], vars2[3];

    pQuery = (void *)GetProcAddress(inetmib1, "SnmpExtensionQuery");
    if (!pQuery)
    {
        skip("couldn't find SnmpExtensionQuery\n");
        return;
    }
    /* Crash
    ret = pQuery(0, NULL, NULL, NULL);
    ret = pQuery(0, NULL, &error, NULL);
    ret = pQuery(0, NULL, NULL, &index);
    ret = pQuery(0, &list, NULL, NULL);
    ret = pQuery(0, &list, &error, NULL);
     */

    /* An empty list succeeds */
    list.len = 0;
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    todo_wine {
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);
    }

    /* Oddly enough, this "succeeds," even though the OID is clearly
     * unsupported.
     */
    vars[0].name.idLength = sizeof(bogus) / sizeof(bogus[0]);
    vars[0].name.ids = bogus;
    vars[0].value.asnType = 0;
    list.len = 1;
    list.list = vars;
    SetLastError(0xdeadbeef);
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    todo_wine {
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);
    }
    /* The OID isn't changed either: */
    ok(!strcmp("1.2.3.4", SnmpUtilOidToA(&vars[0].name)),
        "expected 1.2.3.4, got %s\n", SnmpUtilOidToA(&vars[0].name));

    /* The table is not an accessible variable, so it fails */
    vars[0].name.idLength = sizeof(mib2IfTable) / sizeof(mib2IfTable[0]);
    vars[0].name.ids = mib2IfTable;
    SetLastError(0xdeadbeef);
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    todo_wine {
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOSUCHNAME,
        "expected SNMP_ERRORSTATUS_NOSUCHNAME, got %d\n", error);
    /* The index is 1-based rather than 0-based */
    ok(index == 1, "expected index 1, got %d\n", index);
    }

    /* Even though SnmpExtensionInit says this DLL supports the MIB2 system
     * variables, the first variable it returns a value for is the first
     * interface.
     */
    vars[0].name.idLength = sizeof(mib2System) / sizeof(mib2System[0]);
    vars[0].name.ids = mib2System;
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    vars2[0].value.asnType = 0;
    list.len = 1;
    list.list = vars2;
    moreData = TRUE;
    ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
    todo_wine {
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);
    }
    vars[0].name.idLength = sizeof(mib2If) / sizeof(mib2If[0]);
    vars[0].name.ids = mib2If;
    todo_wine
    ok(!SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name, vars[0].name.idLength),
        "expected 1.3.6.1.2.1.2, got %s\n", SnmpUtilOidToA(&vars2[0].name));
    SnmpUtilVarBindFree(&vars2[0]);
}

START_TEST(main)
{
    inetmib1 = LoadLibraryA("inetmib1");
    testInit();
    testQuery();
}

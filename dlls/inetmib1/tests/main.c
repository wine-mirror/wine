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
    UINT mib2IfDescr[] = { 1,3,6,1,2,1,2,2,1,2 };
    UINT mib2IfAdminStatus[] = { 1,3,6,1,2,1,2,2,1,7 };
    UINT mib2IfOperStatus[] = { 1,3,6,1,2,1,2,2,1,8 };
    SnmpVarBind vars[3], vars2[3];
    UINT entry;

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
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);

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
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);
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
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOSUCHNAME,
        "expected SNMP_ERRORSTATUS_NOSUCHNAME, got %d\n", error);
    /* The index is 1-based rather than 0-based */
    ok(index == 1, "expected index 1, got %d\n", index);

    /* A Get fails on something that specifies a table (but not a particular
     * entry in it)...
     */
    vars[0].name.idLength = sizeof(mib2IfDescr) / sizeof(mib2IfDescr[0]);
    vars[0].name.ids = mib2IfDescr;
    vars[1].name.idLength =
        sizeof(mib2IfAdminStatus) / sizeof(mib2IfAdminStatus[0]);
    vars[1].name.ids = mib2IfAdminStatus;
    vars[2].name.idLength =
        sizeof(mib2IfOperStatus) / sizeof(mib2IfOperStatus[0]);
    vars[2].name.ids = mib2IfOperStatus;
    list.len = 3;
    SetLastError(0xdeadbeef);
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOSUCHNAME,
        "expected SNMP_ERRORSTATUS_NOSUCHNAME, got %d\n", error);
    ok(index == 1, "expected index 1, got %d\n", index);
    /* but a GetNext succeeds with the same values, because GetNext gets the
     * entry after the specified OID, not the entry specified by it.  The
     * successor to the table is the first entry in the table.
     * The OIDs need to be allocated, because GetNext modifies them to indicate
     * the end of data.
     */
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    SnmpUtilOidCpy(&vars2[1].name, &vars[1].name);
    SnmpUtilOidCpy(&vars2[2].name, &vars[2].name);
    list.list = vars2;
    moreData = TRUE;
    entry = 1;
    do {
        SetLastError(0xdeadbeef);
        error = 0xdeadbeef;
        index = 0xdeadbeef;
        ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
        ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
        ok(error == SNMP_ERRORSTATUS_NOERROR,
            "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
        ok(index == 0, "expected index 0, got %d\n", index);
        if (!ret)
            moreData = FALSE;
        else if (error)
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name,
            vars[0].name.idLength))
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[1].name, &vars[1].name,
            vars[1].name.idLength))
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[2].name, &vars[2].name,
            vars[2].name.idLength))
            moreData = FALSE;
        if (moreData)
        {
            /* Check the OIDs.  For these types of values (display strings and
             * integers) they increase by 1 for each element of the table.
             */
            ok(vars2[0].name.idLength == vars[0].name.idLength + 1,
                "expected length %d, got %d\n", vars[0].name.idLength + 1,
                vars2[0].name.idLength);
            ok(vars2[0].name.ids[vars2[0].name.idLength - 1] == entry,
                "expected %d, got %d\n", entry,
                vars2[0].name.ids[vars2[0].name.idLength - 1]);
            ok(vars2[1].name.idLength == vars[1].name.idLength + 1,
                "expected length %d, got %d\n", vars[1].name.idLength + 1,
                vars2[1].name.idLength);
            ok(vars2[1].name.ids[vars2[1].name.idLength - 1] == entry,
                "expected %d, got %d\n", entry,
                vars2[1].name.ids[vars2[1].name.idLength - 1]);
            ok(vars2[2].name.idLength == vars[2].name.idLength + 1,
                "expected length %d, got %d\n", vars[2].name.idLength + 1,
                vars2[2].name.idLength);
            ok(vars2[2].name.ids[vars2[2].name.idLength - 1] == entry,
                "expected %d, got %d\n", entry,
                vars2[2].name.ids[vars2[2].name.idLength - 1]);
            ++entry;
            /* Check the types while we're at it */
            ok(vars2[0].value.asnType == ASN_OCTETSTRING,
                "expected ASN_OCTETSTRING, got %02x\n", vars2[0].value.asnType);
            ok(vars2[1].value.asnType == ASN_INTEGER,
                "expected ASN_INTEGER, got %02x\n", vars2[1].value.asnType);
            ok(vars2[2].value.asnType == ASN_INTEGER,
                "expected ASN_INTEGER, got %02x\n", vars2[2].value.asnType);
            /* Check that the operational status of an interface correctly
             * follows the MIB2 definition of it, rather than the values
             * defined for IPHlpApi's dwOperStatus field.
             */
            ok(vars2[2].value.asnValue.unsigned32 <= 2,
                "expected a value of 0, 1, or 2, got %u\n",
                vars2[2].value.asnValue.unsigned32);
        }
    } while (moreData);
    SnmpUtilVarBindFree(&vars2[0]);
    SnmpUtilVarBindFree(&vars2[1]);
    SnmpUtilVarBindFree(&vars2[2]);

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
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);
    vars[0].name.idLength = sizeof(mib2If) / sizeof(mib2If[0]);
    vars[0].name.ids = mib2If;
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

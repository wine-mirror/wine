/*
 * Copyright 2005 Paul Vriens
 *
 * Conformance test of the ds functions.
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

#include <stdarg.h>

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <dsrole.h>

static void test_params(void)
{
    DWORD ret;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC dpdi;

    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, NULL);
    ok( ret == ERROR_INVALID_PARAMETER, "Expected error ERROR_INVALID_PARAMETER, got (%ld)\n", ret);

    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, 0, NULL);
    ok( ret == ERROR_INVALID_PARAMETER, "Expected error ERROR_INVALID_PARAMETER, got (%ld)\n", ret);
    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, 4, NULL);
    ok( ret == ERROR_INVALID_PARAMETER, "Expected error ERROR_INVALID_PARAMETER, got (%ld)\n", ret);

    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, 4, (BYTE **)&dpdi);
    ok( ret == ERROR_INVALID_PARAMETER, "Expected error ERROR_INVALID_PARAMETER, got (%ld)\n", ret);
}

static void test_get(void)
{
    DWORD ret;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC dpdi;
    PDSROLE_UPGRADE_STATUS_INFO dusi;
    PDSROLE_OPERATION_STATE_INFO dosi;

    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE **)&dpdi);
    ok( ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got (%ld)\n", ret);
    DsRoleFreeMemory(dpdi);

    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, DsRoleUpgradeStatus, (BYTE **)&dusi);
    todo_wine { ok( ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got (%ld)\n", ret); }
    DsRoleFreeMemory(dusi);
   
    SetLastError(0xdeadbeef);
    ret = DsRoleGetPrimaryDomainInformation(NULL, DsRoleOperationState, (BYTE **)&dosi);
    todo_wine { ok( ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got (%ld)\n", ret); }
    DsRoleFreeMemory(dosi);
}


START_TEST(ds)
{
    test_params();
    test_get();
}

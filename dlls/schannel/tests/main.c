/*
 * Schannel tests
 *
 * Copyright 2006 Yuval Fledel
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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>

#include "wine/test.h"

/* Helper macros to find the size of SECPKG_FUNCTION_TABLE */
#define SECPKG_FUNCTION_TABLE_SIZE_1 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    SetContextAttributes)
#define SECPKG_FUNCTION_TABLE_SIZE_2 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    SetCredentialsAttributes)
#define SECPKG_FUNCTION_TABLE_SIZE_3 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    ChangeAccountPassword)
#define SECPKG_FUNCTION_TABLE_SIZE_4 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    QueryMetaData)
#define SECPKG_FUNCTION_TABLE_SIZE_5 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    ValidateTargetInfo)
#define SECPKG_FUNCTION_TABLE_SIZE_6 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    PostLogonUser)
#define SECPKG_FUNCTION_TABLE_SIZE_7 FIELD_OFFSET(SECPKG_FUNCTION_TABLE, \
    GetRemoteCredGuardLogonBuffer)
#define SECPKG_FUNCTION_TABLE_SIZE_8 sizeof(SECPKG_FUNCTION_TABLE)

#define LSA_BASE_CAPS ( \
    SECPKG_FLAG_INTEGRITY         | \
    SECPKG_FLAG_PRIVACY           | \
    SECPKG_FLAG_CONNECTION        | \
    SECPKG_FLAG_MULTI_REQUIRED    | \
    SECPKG_FLAG_EXTENDED_ERROR    | \
    SECPKG_FLAG_IMPERSONATION     | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_STREAM            | \
    SECPKG_FLAG_MUTUAL_AUTH )

static NTSTATUS (NTAPI *pSpLsaModeInitialize)(ULONG, PULONG,
    PSECPKG_FUNCTION_TABLE*, PULONG);
static NTSTATUS (NTAPI *pSpUserModeInitialize)(ULONG, PULONG,
    PSECPKG_USER_FUNCTION_TABLE*, PULONG);

static void testInitialize(void)
{
    PSECPKG_USER_FUNCTION_TABLE pUserTables, pUserTables2;
    PSECPKG_FUNCTION_TABLE pTables, pTables2;
    ULONG cTables = 0, cUserTables = 0, Version = 0;
    NTSTATUS status;

    /* Passing NULL into one of the parameters of SpLsaModeInitialize or
       SpUserModeInitialize causes a crash. */

    /* SpLsaModeInitialize does not care about the LSA version. */
    status = pSpLsaModeInitialize(0, &Version, &pTables2, &cTables);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);
    ok(cTables == 2, "cTables: %ld\n", cTables);
    ok(pTables2 != NULL,"pTables: %p\n", pTables2);

    /* We can call it as many times we want. */
    status = pSpLsaModeInitialize(0x10000, &Version, &pTables, &cTables);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);
    ok(cTables == 2, "cTables: %ld\n", cTables);
    ok(pTables != NULL, "pTables: %p\n", pTables);
    /* It will always return the same pointer. */
    ok(pTables == pTables2, "pTables: %p, pTables2: %p\n", pTables, pTables2);

    status = pSpLsaModeInitialize(0x23456, &Version, &pTables, &cTables);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);
    ok(cTables == 2, "cTables: %ld\n", cTables);
    ok(pTables != NULL, "pTables: %p\n", pTables);
    ok(pTables == pTables2, "pTables: %p, pTables2: %p\n", pTables, pTables2);

    /* Bad versions to SpUserModeInitialize. Parameters unchanged */
    Version = 0xdead;
    cUserTables = 0xdead;
    pUserTables = NULL;
    status = pSpUserModeInitialize(0, &Version, &pUserTables, &cUserTables);
    ok(status == STATUS_INVALID_PARAMETER, "status: 0x%lx\n", status);
    ok(Version == 0xdead, "Version: 0x%lx\n", Version);
    ok(cUserTables == 0xdead, "cTables: %ld\n", cUserTables);
    ok(pUserTables == NULL, "pUserTables: %p\n", pUserTables);

    status = pSpUserModeInitialize(0x20000, &Version, &pUserTables,
                                   &cUserTables);
    ok(status == STATUS_INVALID_PARAMETER, "status: 0x%lx\n", status);
    ok(Version == 0xdead, "Version: 0x%lx\n", Version);
    ok(cUserTables == 0xdead, "cTables: %ld\n", cUserTables);
    ok(pUserTables == NULL, "pUserTables: %p\n", pUserTables);

    /* Good version to SpUserModeInitialize */
    status = pSpUserModeInitialize(SECPKG_INTERFACE_VERSION, &Version,
                                   &pUserTables, &cUserTables);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);
    ok(Version == SECPKG_INTERFACE_VERSION || Version == SECPKG_INTERFACE_VERSION_2 /* win11 */,
       "Version: 0x%lx\n", Version);
    ok(cUserTables == 2, "cUserTables: %ld\n", cUserTables);
    ok(pUserTables != NULL, "pUserTables: %p\n", pUserTables);

    /* Initializing user again */
    status = pSpUserModeInitialize(SECPKG_INTERFACE_VERSION, &Version,
                                   &pUserTables2, &cTables);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);
    ok(pUserTables == pUserTables2, "pUserTables: %p, pUserTables2: %p\n",
       pUserTables, pUserTables2);
}

/* A helper function to find the dispatch table of the next package.
   Needed because SECPKG_FUNCTION_TABLE's size depend on the version */
static PSECPKG_FUNCTION_TABLE getNextSecPkgTable(PSECPKG_FUNCTION_TABLE pTable,
                                                 ULONG Version)
{
    int detectedVersion = 0, size;
    PSECPKG_FUNCTION_TABLE pNextTable;

    if (Version == SECPKG_INTERFACE_VERSION)
        size = SECPKG_FUNCTION_TABLE_SIZE_1;
    else if (Version == SECPKG_INTERFACE_VERSION_2)
        size = SECPKG_FUNCTION_TABLE_SIZE_2;
    else if (Version == SECPKG_INTERFACE_VERSION_3)
        size = SECPKG_FUNCTION_TABLE_SIZE_3;
    else if (Version == SECPKG_INTERFACE_VERSION_4)
        size = SECPKG_FUNCTION_TABLE_SIZE_4;
    else if (Version == SECPKG_INTERFACE_VERSION_5)
        size = SECPKG_FUNCTION_TABLE_SIZE_5;
    else if (Version == SECPKG_INTERFACE_VERSION_6)
        size = SECPKG_FUNCTION_TABLE_SIZE_6;
    else if (Version == SECPKG_INTERFACE_VERSION_7)
        size = SECPKG_FUNCTION_TABLE_SIZE_7;
    else if (Version == SECPKG_INTERFACE_VERSION_8)
        size = SECPKG_FUNCTION_TABLE_SIZE_8;
    else {
        ok(FALSE, "Unknown package version 0x%lx\n", Version);
        return NULL;
    }

    pNextTable = (PSECPKG_FUNCTION_TABLE)((PBYTE)pTable + size);

    /* For any version of Windows beyond Vista SpLsaModeInitialize returns
       SECPKG_INTERFACE_VERSION_3, so try detecting the actual version here
       by iterating until we find the Intitalize function */
    if (broken((void *) pTable->Initialize != (void *) pNextTable->Initialize &&
               pTable->Initialize != NULL))
    {
        for (size = 1; size <= SECPKG_FUNCTION_TABLE_SIZE_8; size++)
        {
            pNextTable = (PSECPKG_FUNCTION_TABLE)((PBYTE)pTable + size);
            if ((void *) pTable->Initialize == (void *) pNextTable->Initialize)
            {
                if (size == SECPKG_FUNCTION_TABLE_SIZE_1)
                    detectedVersion = 1;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_2)
                    detectedVersion = 2;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_3)
                    detectedVersion = 3;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_4)
                    detectedVersion = 4;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_5)
                    detectedVersion = 5;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_6)
                    detectedVersion = 6;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_7)
                    detectedVersion = 7;
                else if (size == SECPKG_FUNCTION_TABLE_SIZE_8)
                    detectedVersion = 8;
                else
                    trace("Unknown package version with size %u\n", size);
                if (detectedVersion > 0)
                    trace("Detected SECPKG_INTERFACE_VERSION_%d\n", detectedVersion);
                return pNextTable;
            }
        }
        win_skip("Invalid function pointers for next package\n");
        return NULL;
    }

    return pNextTable;
}

static void testGetInfo(void)
{
    PSECPKG_FUNCTION_TABLE pTables;
    SecPkgInfoW PackageInfo;
    ULONG cTables, Version;
    NTSTATUS status;

    /* Get the dispatch table */
    status = pSpLsaModeInitialize(0, &Version, &pTables, &cTables);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);

    /* Passing NULL into ->GetInfo causes a crash. */

    /* First package: Unified */
    status = pTables->GetInfo(&PackageInfo);
    ok(status == STATUS_SUCCESS, "status: 0x%lx\n", status);
    ok(PackageInfo.fCapabilities == LSA_BASE_CAPS ||
       PackageInfo.fCapabilities == (LSA_BASE_CAPS|SECPKG_FLAG_APPCONTAINER_PASSTHROUGH),
       "fCapabilities: 0x%lx\n", PackageInfo.fCapabilities);
    ok(PackageInfo.wVersion == 1, "wVersion: %d\n", PackageInfo.wVersion);
    ok(PackageInfo.wRPCID == 14, "wRPCID: %d\n", PackageInfo.wRPCID);
    ok(PackageInfo.cbMaxToken == 0x4000 ||
       PackageInfo.cbMaxToken == 0x6000, /* Vista */
       "cbMaxToken: 0x%lx\n",
       PackageInfo.cbMaxToken);

    /* Second package */
    if (cTables == 1)
    {
        win_skip("Second package missing\n");
        return;
    }
    pTables = getNextSecPkgTable(pTables, Version);
    if (!pTables)
        return;
    if (!pTables->GetInfo)
    {
        win_skip("GetInfo function missing\n");
        return;
    }
    status = pTables->GetInfo(&PackageInfo);
    ok(SUCCEEDED(status), "status: 0x%lx\n", status);

    if (SUCCEEDED(status))
    {
        ok(PackageInfo.fCapabilities == LSA_BASE_CAPS ||
           PackageInfo.fCapabilities == (LSA_BASE_CAPS|SECPKG_FLAG_APPCONTAINER_PASSTHROUGH),
           "fCapabilities: 0x%lx\n", PackageInfo.fCapabilities);
        ok(PackageInfo.wVersion == 1, "wVersion: %d\n", PackageInfo.wVersion);
        ok(PackageInfo.wRPCID == 14, "wRPCID: %d\n", PackageInfo.wRPCID);
        ok(PackageInfo.cbMaxToken == 0x4000 ||
           PackageInfo.cbMaxToken == 0x6000, /* Win7 */
           "cbMaxToken: 0x%lx\n",
           PackageInfo.cbMaxToken);
    }
}

START_TEST(main)
{
    HMODULE hMod = LoadLibraryA("schannel.dll");
    if (!hMod) {
        win_skip("schannel.dll not available\n");
        return;
    }

    pSpLsaModeInitialize  = (void *)GetProcAddress(hMod, "SpLsaModeInitialize");
    pSpUserModeInitialize = (void *)GetProcAddress(hMod, "SpUserModeInitialize");

    if (pSpLsaModeInitialize && pSpUserModeInitialize)
    {
        testInitialize();
        testGetInfo();
    }
    else win_skip( "schannel functions not found\n" );

    FreeLibrary(hMod);
}

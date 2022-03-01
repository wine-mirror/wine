/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Conformance test of the workstation functions.
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winresrc.h" /* Ensure we use Unicode defns with native headers */
#include "nb30.h"
#include "lmcons.h"
#include "lmerr.h"
#include "lmwksta.h"
#include "lmapibuf.h"
#include "lmjoin.h"

static NET_API_STATUS (WINAPI *pNetpGetComputerName)(LPWSTR*)=NULL;

static WCHAR user_name[UNLEN + 1];
static WCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];

static void run_get_comp_name_tests(void)
{
    LPWSTR ws = NULL;

    ok(pNetpGetComputerName(&ws) == NERR_Success, "Computer name is retrieved\n");
    ok(!wcscmp(computer_name, ws), "Expected %s, got %s.\n", debugstr_w(computer_name), debugstr_w(ws));
    NetApiBufferFree(ws);
}

static void run_wkstausergetinfo_tests(void)
{
    LPWKSTA_USER_INFO_0 ui0 = NULL;
    LPWKSTA_USER_INFO_1 ui1 = NULL;
    LPWKSTA_USER_INFO_1101 ui1101 = NULL;
    DWORD dwSize;
    NET_API_STATUS rc;

    /* Level 0 */
    rc = NetWkstaUserGetInfo(NULL, 0, (LPBYTE *)&ui0);
    if (rc == NERR_WkstaNotStarted)
    {
        skip("Workstation service not running\n");
        return;
    }
    ok(!rc && ui0, "got %ld and %p (expected NERR_Success and != NULL\n", rc, ui0);
    ok(!wcscmp(user_name, ui0->wkui0_username), "Expected username %s, got %s.\n",
            debugstr_w(user_name), debugstr_w(ui0->wkui0_username));
    NetApiBufferSize(ui0, &dwSize);
    ok(dwSize >= (sizeof(WKSTA_USER_INFO_0) + wcslen(ui0->wkui0_username) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    /* Level 1 */
    ok(NetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&ui1) == NERR_Success,
       "NetWkstaUserGetInfo is successful\n");
    ok(!wcscmp(user_name, ui1->wkui1_username), "Expected username %s, got %s.\n",
            debugstr_w(user_name), debugstr_w(ui1->wkui1_username));
    NetApiBufferSize(ui1, &dwSize);
    ok(dwSize >= (sizeof(WKSTA_USER_INFO_1) +
                  (wcslen(ui1->wkui1_username) +
                   wcslen(ui1->wkui1_logon_domain) +
                   wcslen(ui1->wkui1_oth_domains) +
                   wcslen(ui1->wkui1_logon_server)) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    /* Level 1101 */
    ok(NetWkstaUserGetInfo(NULL, 1101, (LPBYTE *)&ui1101) == NERR_Success,
       "NetWkstaUserGetInfo is successful\n");
    ok(!wcscmp(ui1101->wkui1101_oth_domains, ui1->wkui1_oth_domains), "Expected %s, got %s.\n",
            debugstr_w(ui1->wkui1_oth_domains), debugstr_w(ui1101->wkui1101_oth_domains));
    NetApiBufferSize(ui1101, &dwSize);
    ok(dwSize >= (sizeof(WKSTA_USER_INFO_1101) + wcslen(ui1101->wkui1101_oth_domains) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    NetApiBufferFree(ui0);
    NetApiBufferFree(ui1);
    NetApiBufferFree(ui1101);

    /* errors handling */
    ok(NetWkstaUserGetInfo(NULL, 10000, (LPBYTE *)&ui0) == ERROR_INVALID_LEVEL,
       "Invalid level\n");
}

static void run_wkstatransportenum_tests(void)
{
    LPBYTE bufPtr;
    NET_API_STATUS apiReturn;
    DWORD entriesRead, totalEntries;

    /* 1st check: is param 2 (level) correct? (only if param 5 passed?) */
    apiReturn = NetWkstaTransportEnum(NULL, 1, NULL, MAX_PREFERRED_LENGTH,
        NULL, &totalEntries, NULL);
    ok(apiReturn == ERROR_INVALID_LEVEL || apiReturn == ERROR_INVALID_PARAMETER,
       "NetWkstaTransportEnum returned %ld\n", apiReturn);

    /* 2nd check: is param 5 passed? (only if level passes?) */
    apiReturn = NetWkstaTransportEnum(NULL, 0, NULL, MAX_PREFERRED_LENGTH,
        NULL, &totalEntries, NULL);

    /* if no network adapter present, bail, the rest of the test will fail */
    if (apiReturn == ERROR_NETWORK_UNREACHABLE)
        return;

    ok(apiReturn == STATUS_ACCESS_VIOLATION || apiReturn == ERROR_INVALID_PARAMETER,
       "NetWkstaTransportEnum returned %ld\n", apiReturn);

    /* 3rd check: is param 3 passed? */
    apiReturn = NetWkstaTransportEnum(NULL, 0, NULL, MAX_PREFERRED_LENGTH,
        NULL, NULL, NULL);
    ok(apiReturn == STATUS_ACCESS_VIOLATION || apiReturn == RPC_X_NULL_REF_POINTER || apiReturn == ERROR_INVALID_PARAMETER,
       "NetWkstaTransportEnum returned %ld\n", apiReturn);

    /* 4th check: is param 6 passed? */
    apiReturn = NetWkstaTransportEnum(NULL, 0, &bufPtr, MAX_PREFERRED_LENGTH,
        &entriesRead, NULL, NULL);
    ok(apiReturn == RPC_X_NULL_REF_POINTER,
       "NetWkstaTransportEnum returned %ld\n", apiReturn);

    /* final check: valid return, actually get data back */
    apiReturn = NetWkstaTransportEnum(NULL, 0, &bufPtr, MAX_PREFERRED_LENGTH,
        &entriesRead, &totalEntries, NULL);
    ok(apiReturn == NERR_Success || apiReturn == ERROR_NETWORK_UNREACHABLE || apiReturn == NERR_WkstaNotStarted,
       "NetWkstaTransportEnum returned %ld\n", apiReturn);
    if (apiReturn == NERR_Success) {
        /* WKSTA_TRANSPORT_INFO_0 *transports = (WKSTA_TRANSPORT_INFO_0 *)bufPtr; */

        ok(bufPtr != NULL, "got data back\n");
        ok(entriesRead > 0, "read at least one transport\n");
        ok(totalEntries > 0 || broken(totalEntries == 0) /* Win7 */,
           "at least one transport\n");
        NetApiBufferFree(bufPtr);
    }
}

static void run_wkstajoininfo_tests(void)
{
    NET_API_STATUS ret;
    LPWSTR buffer = NULL;
    NETSETUP_JOIN_STATUS buffertype = 0xdada;

    ret = NetGetJoinInformation(NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "NetJoinGetInformation returned unexpected 0x%08lx\n", ret);
    ok(buffertype == 0xdada, "buffertype set to unexpected value %d\n", buffertype);

    ret = NetGetJoinInformation(NULL, &buffer, &buffertype);
    ok(ret == NERR_Success, "NetJoinGetInformation returned unexpected 0x%08lx\n", ret);
    ok(buffertype != 0xdada && buffertype != NetSetupUnknownStatus, "buffertype set to unexpected value %d\n", buffertype);
    trace("workstation joined to %s with status %d\n", wine_dbgstr_w(buffer), buffertype);
    NetApiBufferFree(buffer);
}

START_TEST(wksta)
{
    DWORD size;
    BOOL ret;

    pNetpGetComputerName = (void *)GetProcAddress(GetModuleHandleA("netapi32.dll"), "NetpGetComputerName");

    size = ARRAY_SIZE(user_name);
    ret = GetUserNameW(user_name, &size);
    ok(ret, "Failed to get user name, error %lu.\n", GetLastError());
    size = ARRAY_SIZE(computer_name);
    ret = GetComputerNameW(computer_name, &size);
    ok(ret, "Failed to get computer name, error %lu.\n", GetLastError());

    if (pNetpGetComputerName)
        run_get_comp_name_tests();
    else
        win_skip("Function NetpGetComputerName not available\n");
    run_wkstausergetinfo_tests();
    run_wkstatransportenum_tests();
    run_wkstajoininfo_tests();
}

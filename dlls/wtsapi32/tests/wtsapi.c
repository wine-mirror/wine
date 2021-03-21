/*
 * Copyright 2014 Stefan Leichter
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
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <lmcons.h>
#include <wtsapi32.h>

#include "wine/test.h"

static void test_WTSEnumerateProcessesW(void)
{
    BOOL found = FALSE, ret;
    DWORD count, i;
    PWTS_PROCESS_INFOW info;
    WCHAR *pname, nameW[MAX_PATH];

    GetModuleFileNameW(NULL, nameW, MAX_PATH);
    for (pname = nameW + lstrlenW(nameW); pname > nameW; pname--)
    {
        if(*pname == '/' || *pname == '\\')
        {
            pname++;
            break;
        }
    }

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 1, 1, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 0, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 2, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    WTSFreeMemory(info);

    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, NULL, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, NULL);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    WTSFreeMemory(info);

    count = 0;
    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count);
    ok(ret || broken(!ret), /* fails on Win2K with error ERROR_APP_WRONG_OS */
        "expected WTSEnumerateProcessesW to succeed; failed with %d\n", GetLastError());
    for(i = 0; ret && i < count; i++)
    {
        found = found || !lstrcmpW(pname, info[i].pProcessName);
    }
    todo_wine
    ok(found || broken(!ret), "process name %s not found\n", wine_dbgstr_w(pname));
    WTSFreeMemory(info);
}

static void test_WTSQuerySessionInformation(void)
{
    WCHAR *buf1, usernameW[UNLEN + 1], computernameW[MAX_COMPUTERNAME_LENGTH + 1];
    char *buf2, username[UNLEN + 1], computername[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD count, tempsize;
    BOOL ret;

    SetLastError(0xdeadbeef);
    count = 0;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 0, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 1;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf1, NULL);
    ok(!ret, "got %u\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());

    count = 0;
    buf1 = NULL;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf1, &count);
    ok(ret, "got %u\n", GetLastError());
    ok(buf1 != NULL, "buf not set\n");
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %u, got %u\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
    tempsize = UNLEN + 1;
    GetUserNameW(usernameW, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase username, while the rest return lowercase. */
    ok(!wcsicmp(buf1, usernameW), "expected %s, got %s\n", wine_dbgstr_w(usernameW), wine_dbgstr_w(buf1));
    ok(count == tempsize * sizeof(WCHAR), "expected %u, got %u\n", tempsize * sizeof(WCHAR), count);
    WTSFreeMemory(buf1);

    count = 0;
    buf1 = NULL;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSDomainName, &buf1, &count);
    ok(ret, "got %u\n", GetLastError());
    ok(buf1 != NULL, "buf not set\n");
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %u, got %u\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
    tempsize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(computernameW, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase computername, while the rest return lowercase. */
    ok(!wcsicmp(buf1, computernameW), "expected %s, got %s\n", wine_dbgstr_w(computernameW), wine_dbgstr_w(buf1));
    ok(count == (tempsize + 1) * sizeof(WCHAR), "expected %u, got %u\n", (tempsize + 1) * sizeof(WCHAR), count);
    WTSFreeMemory(buf1);

    SetLastError(0xdeadbeef);
    count = 0;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 0, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 1;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf2, NULL);
    ok(!ret, "got %u\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %u\n", GetLastError());

    count = 0;
    buf2 = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf2, &count);
    ok(ret, "got %u\n", GetLastError());
    ok(buf2 != NULL, "buf not set\n");
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %u\n", lstrlenA(buf2) + 1, count);
    tempsize = UNLEN + 1;
    GetUserNameA(username, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase username, while the rest return lowercase. */
    ok(!stricmp(buf2, username), "expected %s, got %s\n", username, buf2);
    ok(count == tempsize, "expected %u, got %u\n", tempsize, count);
    WTSFreeMemory(buf2);

    count = 0;
    buf2 = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSDomainName, &buf2, &count);
    ok(ret, "got %u\n", GetLastError());
    ok(buf2 != NULL, "buf not set\n");
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %u\n", lstrlenA(buf2) + 1, count);
    tempsize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(computername, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase computername, while the rest return lowercase. */
    ok(!stricmp(buf2, computername), "expected %s, got %s\n", computername, buf2);
    ok(count == tempsize + 1, "expected %u, got %u\n", tempsize + 1, count);
    WTSFreeMemory(buf2);
}

static void test_WTSQueryUserToken(void)
{
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = WTSQueryUserToken(WTS_CURRENT_SESSION, NULL);
    ok(!ret, "expected WTSQueryUserToken to fail\n");
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
}

START_TEST (wtsapi)
{
    test_WTSEnumerateProcessesW();
    test_WTSQuerySessionInformation();
    test_WTSQueryUserToken();
}

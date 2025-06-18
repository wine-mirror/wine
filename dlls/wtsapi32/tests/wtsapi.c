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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winternl.h>
#include <lmcons.h>
#include <wtsapi32.h>
#define PSAPI_VERSION 1
#include <psapi.h>

#include "wine/test.h"

static BOOL (WINAPI *pWTSEnumerateProcessesExW)(HANDLE server, DWORD *level, DWORD session, WCHAR **info, DWORD *count);
static BOOL (WINAPI *pWTSFreeMemoryExW)(WTS_TYPE_CLASS class, void *memory, ULONG count);

static const SYSTEM_PROCESS_INFORMATION *find_nt_process_info(const SYSTEM_PROCESS_INFORMATION *head, DWORD pid)
{
    for (;;)
    {
        if ((DWORD)(DWORD_PTR)head->UniqueProcessId == pid)
            return head;
        if (!head->NextEntryOffset)
            break;
        head = (SYSTEM_PROCESS_INFORMATION *)((char *)head + head->NextEntryOffset);
    }
    return NULL;
}

static void check_wts_process_info(const WTS_PROCESS_INFOW *info, DWORD count)
{
    ULONG nt_length = 1024;
    SYSTEM_PROCESS_INFORMATION *nt_info = malloc(nt_length);
    WCHAR process_name[MAX_PATH], *process_filepart;
    BOOL ret, found = FALSE;
    NTSTATUS status;
    DWORD i;

    GetModuleFileNameW(NULL, process_name, MAX_PATH);
    process_filepart = wcsrchr(process_name, '\\') + 1;

    while ((status = NtQuerySystemInformation(SystemProcessInformation, nt_info,
            nt_length, NULL)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        nt_length *= 2;
        nt_info = realloc(nt_info, nt_length);
    }
    ok(!status, "got %#lx\n", status);

    for (i = 0; i < count; i++)
    {
        SID_AND_ATTRIBUTES *sid;
        const SYSTEM_PROCESS_INFORMATION *nt_process;
        HANDLE process, token;
        DWORD size;

        nt_process = find_nt_process_info(nt_info, info[i].ProcessId);
        ok(!!nt_process, "failed to find pid %#lx\n", info[i].ProcessId);

        winetest_push_context("pid %#lx", info[i].ProcessId);

        ok(info[i].SessionId == nt_process->SessionId, "expected session id %#lx, got %#lx\n",
                nt_process->SessionId, info[i].SessionId);

        ok(!memcmp(info[i].pProcessName, nt_process->ProcessName.Buffer, nt_process->ProcessName.Length),
                "expected process name %s, got %s\n",
                debugstr_w(nt_process->ProcessName.Buffer), debugstr_w(info[i].pProcessName));

        if ((process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info[i].ProcessId)))
        {
            sid = malloc(50);
            ret = OpenProcessToken(process, TOKEN_QUERY, &token);
            ok(ret, "failed to open token, error %lu\n", GetLastError());
            ret = GetTokenInformation(token, TokenUser, sid, 50, &size);
            ok(ret, "failed to get token user, error %lu\n", GetLastError());
            ok(EqualSid(info[i].pUserSid, sid->Sid), "SID did not match\n");
            free(sid);
            CloseHandle(token);
            CloseHandle(process);
        }

        winetest_pop_context();

        found = found || !wcscmp(info[i].pProcessName, process_filepart);
    }

    ok(found, "did not find current process\n");

    free(nt_info);
}

static void test_WTSEnumerateProcessesW(void)
{
    PWTS_PROCESS_INFOW info;
    DWORD count, level;
    BOOL ret;

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 1, 1, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
    WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 0, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
    WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 2, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
    WTSFreeMemory(info);

    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, NULL, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, NULL);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
    WTSFreeMemory(info);

    count = 0;
    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count);
    ok(ret, "expected success\n");
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    check_wts_process_info(info, count);
    WTSFreeMemory(info);

    if (!pWTSEnumerateProcessesExW)
    {
        win_skip("WTSEnumerateProcessesEx is not available\n");
        return;
    }

    level = 0;

    SetLastError(0xdeadbeef);
    count = 0xdeadbeef;
    ret = pWTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, NULL, &count);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());
    ok(count == 0xdeadbeef, "got count %lu\n", count);

    info = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pWTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, (WCHAR **)&info, NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());
    ok(info == (void *)0xdeadbeef, "got info %p\n", info);

    info = NULL;
    count = 0;
    SetLastError(0xdeadbeef);
    ret = pWTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, (WCHAR **)&info, &count);
    ok(ret, "expected success\n");
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    check_wts_process_info(info, count);
    pWTSFreeMemoryExW(WTSTypeProcessInfoLevel0, info, count);
}

static LONGLONG get_system_time_as_longlong(void)
{
    LARGE_INTEGER li;
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);
    li.u.LowPart = ft.dwLowDateTime;
    li.u.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
}

static void test_WTSQuerySessionInformation(void)
{
    WCHAR *buf1, usernameW[UNLEN + 1], computernameW[MAX_COMPUTERNAME_LENGTH + 1];
    char *buf2, username[UNLEN + 1], computername[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD count, tempsize, sessionId;
    WTS_CONNECTSTATE_CLASS *state;
    WTSINFOW *wtsinfoW;
    WTSINFOA *wtsinfoA;
    USHORT *protocol;
    LONGLONG t1, t2;
    BOOL ret;

    count = 0;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSConnectState, (WCHAR **)&state, &count);
    ok(ret, "got error %lu\n", GetLastError());
    ok(count == sizeof(*state), "got %lu\n", count);
    ok(*state == WTSActive, "got %d.\n", *state);
    WTSFreeMemory(state);

    count = 0;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSConnectState, (char **)&state, &count);
    ok(ret, "got error %lu\n", GetLastError());
    ok(count == sizeof(*state), "got %lu\n", count);
    ok(*state == WTSActive, "got %d.\n", *state);
    WTSFreeMemory(state);

    SetLastError(0xdeadbeef);
    count = 0;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 0, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 1;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf1, NULL);
    ok(!ret, "got %lu\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());

    count = 0;
    buf1 = NULL;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf1, &count);
    ok(ret, "got %lu\n", GetLastError());
    ok(buf1 != NULL, "buf not set\n");
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %Iu, got %lu\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
    tempsize = UNLEN + 1;
    GetUserNameW(usernameW, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase username, while the rest return lowercase. */
    ok(!wcsicmp(buf1, usernameW), "expected %s, got %s\n", wine_dbgstr_w(usernameW), wine_dbgstr_w(buf1));
    ok(count == tempsize * sizeof(WCHAR), "got %lu\n", count);
    WTSFreeMemory(buf1);

    count = 0;
    buf1 = NULL;
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSDomainName, &buf1, &count);
    ok(ret, "got %lu\n", GetLastError());
    ok(buf1 != NULL, "buf not set\n");
    ok(count == (lstrlenW(buf1) + 1) * sizeof(WCHAR), "expected %Iu, got %lu\n", (lstrlenW(buf1) + 1) * sizeof(WCHAR), count);
    tempsize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(computernameW, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase computername, while the rest return lowercase. */
    ok(!wcsicmp(buf1, computernameW), "expected %s, got %s\n", wine_dbgstr_w(computernameW), wine_dbgstr_w(buf1));
    ok(count == (tempsize + 1) * sizeof(WCHAR), "got %lu\n", count);
    WTSFreeMemory(buf1);

    SetLastError(0xdeadbeef);
    count = 0;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 0, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 1;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, NULL, &count);
    ok(!ret, "got %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf2, NULL);
    ok(!ret, "got %lu\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "got %lu\n", GetLastError());

    count = 0;
    buf2 = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, &buf2, &count);
    ok(ret, "got %lu\n", GetLastError());
    ok(buf2 != NULL, "buf not set\n");
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %lu\n", lstrlenA(buf2) + 1, count);
    tempsize = UNLEN + 1;
    GetUserNameA(username, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase username, while the rest return lowercase. */
    ok(!stricmp(buf2, username), "expected %s, got %s\n", username, buf2);
    ok(count == tempsize, "expected %lu, got %lu\n", tempsize, count);
    WTSFreeMemory(buf2);

    count = 0;
    buf2 = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSDomainName, &buf2, &count);
    ok(ret, "got %lu\n", GetLastError());
    ok(buf2 != NULL, "buf not set\n");
    ok(count == lstrlenA(buf2) + 1, "expected %u, got %lu\n", lstrlenA(buf2) + 1, count);
    tempsize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(computername, &tempsize);
    /* Windows Vista, 7 and 8 return uppercase computername, while the rest return lowercase. */
    ok(!stricmp(buf2, computername), "expected %s, got %s\n", computername, buf2);
    ok(count == tempsize + 1, "expected %lu, got %lu\n", tempsize + 1, count);
    WTSFreeMemory(buf2);

    count = 0;
    protocol = NULL;
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSClientProtocolType,
                                      (char **)&protocol, &count);
    ok(ret, "got %lu\n", GetLastError());
    ok(protocol != NULL, "protocol not set\n");
    ok(count == sizeof(*protocol), "got %lu\n", count);
    WTSFreeMemory(protocol);

    ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);

    count = 0;
    wtsinfoW = NULL;
    t1 = get_system_time_as_longlong();
    ret = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSSessionInfo,
                                      (WCHAR **)&wtsinfoW, &count);
    ok(ret, "got %lu\n", GetLastError());
    t2 = get_system_time_as_longlong();
    ok(count == sizeof(*wtsinfoW), "got %lu\n", count);
    ok(wtsinfoW->State == WTSActive, "got %d.\n", wtsinfoW->State);
    ok(wtsinfoW->SessionId == sessionId, "expected %lu, got %lu\n", sessionId, wtsinfoW->SessionId);
    ok(wtsinfoW->IncomingBytes == 0, "got %lu\n", wtsinfoW->IncomingBytes);
    ok(wtsinfoW->OutgoingBytes == 0, "got %lu\n", wtsinfoW->OutgoingBytes);
    ok(wtsinfoW->IncomingFrames == 0, "got %lu\n", wtsinfoW->IncomingFrames);
    ok(wtsinfoW->OutgoingFrames == 0, "got %lu\n", wtsinfoW->OutgoingFrames);
    ok(wtsinfoW->IncomingCompressedBytes == 0, "got %lu\n", wtsinfoW->IncomingCompressedBytes);
    ok(wtsinfoW->OutgoingCompressedBytes == 0, "got %lu\n", wtsinfoW->OutgoingCompressedBytes);
    ok(!wcscmp(wtsinfoW->WinStationName, L"Console"), "got %s\n", wine_dbgstr_w(wtsinfoW->WinStationName));
    ok(!wcsicmp(wtsinfoW->Domain, computernameW),
            "expected %s, got %s\n",
            wine_dbgstr_w(computernameW), wine_dbgstr_w(wtsinfoW->Domain));
    ok(!wcsicmp(wtsinfoW->UserName, usernameW),
            "expected %s, got %s\n",
            wine_dbgstr_w(usernameW), wine_dbgstr_w(wtsinfoW->UserName));
    ok(t1 <= wtsinfoW->CurrentTime.QuadPart,
            "out of order %s %s\n",
            wine_dbgstr_longlong(t1), wine_dbgstr_longlong(wtsinfoW->CurrentTime.QuadPart));
    ok(wtsinfoW->CurrentTime.QuadPart <= t2,
            "out of order %s %s\n",
            wine_dbgstr_longlong(wtsinfoW->CurrentTime.QuadPart), wine_dbgstr_longlong(t2));
    WTSFreeMemory(wtsinfoW);

    count = 0;
    wtsinfoA = NULL;
    t1 = get_system_time_as_longlong();
    ret = WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSSessionInfo,
                                      (char **)&wtsinfoA, &count);
    ok(ret, "got %lu\n", GetLastError());
    t2 = get_system_time_as_longlong();
    ok(count == sizeof(*wtsinfoA), "got %lu\n", count);
    ok(wtsinfoA->State == WTSActive, "got %d.\n", wtsinfoA->State);
    ok(wtsinfoA->SessionId == sessionId, "expected %lu, got %lu\n", sessionId, wtsinfoA->SessionId);
    ok(wtsinfoA->IncomingBytes == 0, "got %lu\n", wtsinfoA->IncomingBytes);
    ok(wtsinfoA->OutgoingBytes == 0, "got %lu\n", wtsinfoA->OutgoingBytes);
    ok(wtsinfoA->IncomingFrames == 0, "got %lu\n", wtsinfoA->IncomingFrames);
    ok(wtsinfoA->OutgoingFrames == 0, "got %lu\n", wtsinfoA->OutgoingFrames);
    ok(wtsinfoA->IncomingCompressedBytes == 0, "got %lu\n", wtsinfoA->IncomingCompressedBytes);
    ok(wtsinfoA->OutgoingCompressedBytes == 0, "got %lu\n", wtsinfoA->OutgoingCompressedBytes);
    ok(!strcmp(wtsinfoA->WinStationName, "Console"), "got %s\n", wtsinfoA->WinStationName);
    ok(!stricmp(wtsinfoA->Domain, computername), "expected %s, got %s\n", computername, wtsinfoA->Domain);
    ok(!stricmp(wtsinfoA->UserName, username), "expected %s, got %s\n", username, wtsinfoA->UserName);
    ok(t1 <= wtsinfoA->CurrentTime.QuadPart,
            "out of order %s %s\n",
            wine_dbgstr_longlong(t1), wine_dbgstr_longlong(wtsinfoA->CurrentTime.QuadPart));
    ok(wtsinfoA->CurrentTime.QuadPart <= t2,
            "out of order %s %s\n",
            wine_dbgstr_longlong(wtsinfoA->CurrentTime.QuadPart), wine_dbgstr_longlong(t2));
    WTSFreeMemory(wtsinfoA);
}

static void test_WTSQueryUserToken(void)
{
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = WTSQueryUserToken(WTS_CURRENT_SESSION, NULL);
    ok(!ret, "expected WTSQueryUserToken to fail\n");
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %ld\n", GetLastError());
}

static void test_WTSEnumerateSessions(void)
{
    BOOL console_found = FALSE, services_found = FALSE;
    WTS_SESSION_INFOW *info;
    WTS_SESSION_INFOA *infoA;
    DWORD count, count2;
    unsigned int i;
    BOOL bret;

    bret = WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count);
    ok(bret, "got error %lu.\n", GetLastError());
    todo_wine_if(count == 1) ok(count >= 2, "got %lu.\n", count);

    bret = WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &infoA, &count2);
    ok(bret, "got error %lu.\n", GetLastError());
    ok(count2 == count, "got %lu.\n", count2);

    for (i = 0; i < count; ++i)
    {
        trace("SessionId %lu, name %s, State %d.\n", info[i].SessionId, debugstr_w(info[i].pWinStationName), info[i].State);
        if (!wcscmp(info[i].pWinStationName, L"Console"))
        {
            console_found = TRUE;
            ok(info[i].State == WTSActive, "got State %d.\n", info[i].State);
            ok(!strcmp(infoA[i].pWinStationName, "Console"), "got %s.\n", debugstr_a(infoA[i].pWinStationName));
        }
        else if (!wcscmp(info[i].pWinStationName, L"Services"))
        {
            services_found = TRUE;
            ok(info[i].State == WTSDisconnected, "got State %d.\n", info[i].State);
            ok(!strcmp(infoA[i].pWinStationName, "Services"), "got %s.\n", debugstr_a(infoA[i].pWinStationName));
        }
    }
    ok(console_found, "Console session not found.\n");
    todo_wine ok(services_found, "Services session not found.\n");

    WTSFreeMemory(info);
    WTSFreeMemory(infoA);
}

START_TEST (wtsapi)
{
    pWTSEnumerateProcessesExW = (void *)GetProcAddress(GetModuleHandleA("wtsapi32"), "WTSEnumerateProcessesExW");
    pWTSFreeMemoryExW = (void *)GetProcAddress(GetModuleHandleA("wtsapi32"), "WTSFreeMemoryExW");

    test_WTSEnumerateProcessesW();
    test_WTSQuerySessionInformation();
    test_WTSQueryUserToken();
    test_WTSEnumerateSessions();
}

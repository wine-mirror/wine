/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include <windef.h>
#include <winsvc.h>
#include <stdio.h>
#include <winbase.h>
#include <winuser.h>

#include "wine/test.h"

static SERVICE_STATUS_HANDLE (WINAPI *pRegisterServiceCtrlHandlerExA)(LPCSTR,LPHANDLER_FUNCTION_EX,LPVOID);

static HANDLE pipe_handle = INVALID_HANDLE_VALUE;
static char service_name[100], named_pipe_name[114];
static SERVICE_STATUS_HANDLE service_handle;

/* Service process global variables */
static HANDLE service_stop_event;

static int monitor_count;

static void send_msg(const char *type, const char *msg)
{
    DWORD written = 0;
    char buf[512];

    sprintf(buf, "%s:%s", type, msg);
    WriteFile(pipe_handle, buf, strlen(buf)+1, &written, NULL);
}

static inline void service_trace(const char *msg)
{
    send_msg("TRACE", msg);
}

static inline void service_event(const char *event)
{
    send_msg("EVENT", event);
}

static void WINAPIV service_ok(int cnd, const char *msg, ...) __WINE_PRINTF_ATTR(2,3);
static void WINAPIV service_ok(int cnd, const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    vsprintf(buf, msg, valist);
    va_end(valist);

    send_msg(cnd ? "OK" : "FAIL", buf);
}

static void WINAPIV service_todo_wine_ok(int cnd, const char *msg, ...) __WINE_PRINTF_ATTR(2,3);
static void WINAPIV service_todo_wine_ok(int cnd, const char *msg, ...)
{
    va_list valist;
    char buf[512];

    va_start(valist, msg);
    vsprintf(buf, msg, valist);
    va_end(valist);

    send_msg(cnd ? "TODO_WINE OK" : "TODO_WINE FAIL", buf);
}

static void test_winstation(void)
{
    HWINSTA winstation;
    USEROBJECTFLAGS flags;
    BOOL r;

    winstation = GetProcessWindowStation();
    service_ok(winstation != NULL, "winstation = NULL\n");

    r = GetUserObjectInformationA(winstation, UOI_FLAGS, &flags, sizeof(flags), NULL);
    service_ok(r, "GetUserObjectInformation(UOI_NAME) failed: %lu\n", GetLastError());
    service_ok(!(flags.dwFlags & WSF_VISIBLE), "winstation has flags %lx\n", flags.dwFlags);
}

/*
 * Test creating window in a service process. Although services run in non-interactive,
 * they may create windows that will never be visible.
 */
static void test_create_window(void)
{
    DWORD style;
    ATOM class;
    HWND hwnd;
    BOOL r;

    static WNDCLASSEXA wndclass = {
        sizeof(WNDCLASSEXA),
        0,
        DefWindowProcA,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        "service_test",
        NULL
    };

    hwnd = GetDesktopWindow();
    service_ok(IsWindow(hwnd), "GetDesktopWindow returned invalid window %p\n", hwnd);

    class = RegisterClassExA(&wndclass);
    service_ok(class, "RegisterClassFailed\n");

    hwnd = CreateWindowA("service_test", "service_test",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            515, 530, NULL, NULL, NULL, NULL);
    service_ok(hwnd != NULL, "CreateWindow failed: %lu\n", GetLastError());

    style = GetWindowLongW(hwnd, GWL_STYLE);
    service_ok(!(style & WS_VISIBLE), "style = %lx, expected invisible\n", style);

    r = ShowWindow(hwnd, SW_SHOW);
    service_ok(!r, "ShowWindow returned %x\n", r);

    style = GetWindowLongW(hwnd, GWL_STYLE);
    service_ok(style & WS_VISIBLE, "style = %lx, expected visible\n", style);

    r = ShowWindow(hwnd, SW_SHOW);
    service_ok(r, "ShowWindow returned %x\n", r);

    r = DestroyWindow(hwnd);
    service_ok(r, "DestroyWindow failed: %08lx\n", GetLastError());
}

static BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC hdc, LPRECT lprc, LPARAM lparam)
{
    static const RECT expected_rect = {0, 0, 1024, 768};
    BOOL r;
    MONITORINFOEXA mi;

    service_ok(hmon != NULL, "Unexpected hmon %p\n", hmon);

    monitor_count++;

    mi.cbSize = sizeof(mi);

    SetLastError(0xdeadbeef);
    r = GetMonitorInfoA(NULL, (MONITORINFO*)&mi);
    service_ok(GetLastError() == ERROR_INVALID_MONITOR_HANDLE, "Unexpected GetLastError: %#lx.\n", GetLastError());
    service_ok(!r, "GetMonitorInfo with NULL HMONITOR succeeded.\n");

    r = GetMonitorInfoA(hmon, (MONITORINFO*)&mi);
    service_ok(r, "GetMonitorInfo failed.\n");

    service_ok(EqualRect(lprc, &expected_rect), "Unexpected rect: %s\n", wine_dbgstr_rect(lprc));
    service_ok(EqualRect(&mi.rcMonitor, &expected_rect), "Unexpected rcMonitor: %s\n",
               wine_dbgstr_rect(&mi.rcMonitor));
    service_ok(EqualRect(&mi.rcWork, &expected_rect), "Unexpected rcWork: %s\n",
               wine_dbgstr_rect(&mi.rcWork));
    service_ok(!strcmp(mi.szDevice, "WinDisc"), "Unexpected szDevice received: %s\n", mi.szDevice);

    service_ok(mi.dwFlags == MONITORINFOF_PRIMARY, "Unexpected secondary monitor info.\n");

    return TRUE;
}

/* query monitor information, even in non-interactive services */
static void test_monitors(void)
{
    BOOL r;

    r = EnumDisplayMonitors(0, 0, monitor_enum_proc, 0);
    service_ok(r, "EnumDisplayMonitors failed.\n");
    service_ok(monitor_count == 1, "Callback got called less or more than once. %d\n", monitor_count);
}

struct max_sid
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
};

static BOOL is_in_groups(const TOKEN_GROUPS *groups, const PSID sid, DWORD attributes)
{
    int i;

    for (i = 0; i < groups->GroupCount; i++)
    {
        if (EqualSid(groups->Groups[i].Sid, sid))
        {
            service_ok(groups->Groups[i].Attributes == attributes,
                       "Expected attribute %d %#lx, got %#lx.\n", i, attributes,
                       groups->Groups[i].Attributes);
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL is_in_privileges(const TOKEN_PRIVILEGES *privileges, const char *privilege, DWORD attributes)
{
    LUID luid;
    BOOL ret;
    int i;

    ret = LookupPrivilegeValueA(NULL, privilege, &luid);
    service_ok(ret, "LookupPrivilegeValueA %s failed, error %lu.\n", privilege, GetLastError());

    for (i = 0; i < privileges->PrivilegeCount; i++)
    {
        if (privileges->Privileges[i].Luid.LowPart == luid.LowPart
            && privileges->Privileges[i].Luid.HighPart == luid.HighPart)
        {
            service_ok(privileges->Privileges[i].Attributes == attributes,
                       "Expected attribute %#lx for %s, got %#lx.\n", attributes, privilege,
                       privileges->Privileges[i].Attributes);
            return TRUE;
        }
    }

    return FALSE;
}

static void check_token_information(const char *username, HANDLE token)
{
    struct max_sid system = {SID_REVISION, 1, {SECURITY_NT_AUTHORITY}, {SECURITY_LOCAL_SYSTEM_RID}};
    struct max_sid local_service = {SID_REVISION, 1, {SECURITY_NT_AUTHORITY}, {SECURITY_LOCAL_SERVICE_RID}};
    struct max_sid network_service = {SID_REVISION, 1, {SECURITY_NT_AUTHORITY}, {SECURITY_NETWORK_SERVICE_RID}};
    struct max_sid administrators = {SID_REVISION, 2, {SECURITY_NT_AUTHORITY}, {SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS}};
    struct max_sid everyone = {SID_REVISION, 1, {SECURITY_WORLD_SID_AUTHORITY}, {SECURITY_WORLD_RID}};
    struct max_sid authenticated_users = {SID_REVISION, 1, {SECURITY_NT_AUTHORITY}, {SECURITY_AUTHENTICATED_USER_RID}};
    struct max_sid users = {SID_REVISION, 2, {SECURITY_NT_AUTHORITY}, {SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS}};
    struct max_sid local = {SID_REVISION, 1, {SECURITY_LOCAL_SID_AUTHORITY}, {SECURITY_LOCAL_RID}};
    struct max_sid console_logon = {SID_REVISION, 1, {SECURITY_LOCAL_SID_AUTHORITY}, {SECURITY_LOCAL_LOGON_RID}};
    struct max_sid system_label = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY}, {SECURITY_MANDATORY_SYSTEM_RID}};
    unsigned char buffer[1024];
    DWORD size;
    BOOL ret;
    int i;

    static const TOKEN_INFORMATION_CLASS classes[] =
    {
        TokenUser,
        TokenGroups,
        TokenPrivileges,
        TokenOwner,
        TokenPrimaryGroup,
        TokenSource,
        TokenType,
        TokenSessionId,
        TokenElevationType,
        TokenIntegrityLevel,
        TokenUIAccess,
    };

    for (i = 0 ; i < ARRAY_SIZE(classes); i++)
    {
        memset(buffer, 0, sizeof(buffer));
        ret = GetTokenInformation(token, classes[i], buffer, sizeof(buffer), &size);
        if (classes[i] == TokenSource)
        {
            service_todo_wine_ok(ret, "GetTokenInformation %d failed, error %lu.\n", classes[i], GetLastError());
            continue;
        }
        else
        {
            service_ok(ret, "GetTokenInformation %d failed, error %lu.\n", classes[i], GetLastError());
        }

        switch (classes[i])
        {
        case TokenUser:
        {
            TOKEN_USER *user = (TOKEN_USER *)buffer;

            if (!strcmpi(username, "System"))
                service_ok(EqualSid(user->User.Sid, &system), "Got unexpected user.\n");
            else if (!strcmpi(username, "LocalService"))
                service_ok(EqualSid(user->User.Sid, &local_service), "Got unexpected user.\n");
            else if (!strcmpi(username, "NetworkService"))
                service_ok(EqualSid(user->User.Sid, &network_service), "Got unexpected user.\n");
            break;
        }
        case TokenGroups:
        {
            TOKEN_GROUPS *groups = (TOKEN_GROUPS *)buffer;

            if (!strcmpi(username, "System"))
            {
                service_ok(is_in_groups(groups, (PSID)&administrators, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_OWNER), "Not in Administrators group.\n");
                service_ok(is_in_groups(groups, (PSID)&everyone, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Everyone group.\n");
                service_ok(is_in_groups(groups, (PSID)&authenticated_users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Authenticated users group.\n");
            }
            else if (!strcmpi(username, "LocalService") || !strcmpi(username, "NetworkService"))
            {
                service_ok(is_in_groups(groups, (PSID)&everyone, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Everyone group.\n");
                service_ok(is_in_groups(groups, (PSID)&users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Users group.\n");
                service_ok(is_in_groups(groups, (PSID)&authenticated_users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Authenticated users group.\n");
                service_ok(is_in_groups(groups, (PSID)&local, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Local group.\n");
                service_ok(is_in_groups(groups, (PSID)&console_logon, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY), "Not in Console logon group.\n");
            }

            break;
        }
        case TokenPrivileges:
        {
            TOKEN_PRIVILEGES *privileges = (TOKEN_PRIVILEGES *)buffer;

            if (!strcmpi(username, "System"))
            {
                service_ok(!is_in_privileges(privileges, "SeCreateTokenPrivilege", 0), "Got SeCreateTokenPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeAssignPrimaryTokenPrivilege", 0), "No SeAssignPrimaryTokenPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeLockMemoryPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeLockMemoryPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseQuotaPrivilege", 0), "No SeIncreaseQuotaPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeMachineAccountPrivilege", 0), "Got SeMachineAccountPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeTcbPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeTcbPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeSecurityPrivilege", 0), "No SeSecurityPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeTakeOwnershipPrivilege", 0), "No SeTakeOwnershipPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeLoadDriverPrivilege", 0), "No SeLoadDriverPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeSystemProfilePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeSystemProfilePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeSystemtimePrivilege", 0), "No SeSystemtimePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeProfileSingleProcessPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeProfileSingleProcessPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseBasePriorityPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeIncreaseBasePriorityPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeCreatePagefilePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeCreatePagefilePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeCreatePermanentPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeCreatePermanentPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeBackupPrivilege", 0), "No SeBackupPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeRestorePrivilege", 0), "No SeRestorePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeShutdownPrivilege", 0), "No SeShutdownPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeDebugPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeDebugPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeAuditPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeAuditPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeSystemEnvironmentPrivilege", 0), "No SeSystemEnvironmentPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeChangeNotifyPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeChangeNotifyPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRemoteShutdownPrivilege", 0), "Got SeRemoteShutdownPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeUndockPrivilege", 0), "No SeUndockPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeEnableDelegationPrivilege", 0), "Got SeEnableDelegationPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeManageVolumePrivilege", 0), "No SeManageVolumePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeImpersonatePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeImpersonatePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeCreateGlobalPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeCreateGlobalPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTrustedCredManAccessPrivilege", 0), "Got SeTrustedCredManAccessPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRelabelPrivilege", 0), "Got SeRelabelPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseWorkingSetPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeIncreaseWorkingSetPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeTimeZonePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeTimeZonePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeCreateSymbolicLinkPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeCreateSymbolicLinkPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeDelegateSessionUserImpersonatePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeDelegateSessionUserImpersonatePrivilege.\n");
            }
            else if (!strcmpi(username, "LocalService"))
            {
                service_ok(!is_in_privileges(privileges, "SeCreateTokenPrivilege", 0), "Got SeCreateTokenPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeAssignPrimaryTokenPrivilege", 0), "No SeAssignPrimaryTokenPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeLockMemoryPrivilege", 0), "Got SeLockMemoryPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseQuotaPrivilege", 0), "No SeIncreaseQuotaPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeMachineAccountPrivilege", 0), "Got SeMachineAccountPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTcbPrivilege", 0), "Got SeTcbPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSecurityPrivilege", 0), "Got SeSecurityPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTakeOwnershipPrivilege", 0), "Got SeTakeOwnershipPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeLoadDriverPrivilege", 0), "Got SeLoadDriverPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSystemProfilePrivilege", 0), "Got SeSystemProfilePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeSystemtimePrivilege", 0), "No SeSystemtimePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeProfileSingleProcessPrivilege", 0), "Got SeProfileSingleProcessPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeIncreaseBasePriorityPrivilege", 0), "Got SeIncreaseBasePriorityPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeCreatePagefilePrivilege", 0), "Got SeCreatePagefilePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeCreatePermanentPrivilege", 0), "Got SeCreatePermanentPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeBackupPrivilege", 0), "Got SeBackupPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRestorePrivilege", 0), "Got SeRestorePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeShutdownPrivilege", 0), "No SeShutdownPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeDebugPrivilege", 0), "Got SeDebugPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeAuditPrivilege", 0), "No SeAuditPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSystemEnvironmentPrivilege", 0), "Got SeSystemEnvironmentPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeChangeNotifyPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeChangeNotifyPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRemoteShutdownPrivilege", 0), "Got SeRemoteShutdownPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeUndockPrivilege", 0), "No SeUndockPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeEnableDelegationPrivilege", 0), "Got SeEnableDelegationPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeManageVolumePrivilege", 0), "Got SeManageVolumePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeImpersonatePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeImpersonatePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeCreateGlobalPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeCreateGlobalPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTrustedCredManAccessPrivilege", 0), "Got SeTrustedCredManAccessPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRelabelPrivilege", 0), "Got SeRelabelPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseWorkingSetPrivilege", 0), "No SeIncreaseWorkingSetPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeTimeZonePrivilege", 0), "No SeTimeZonePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeCreateSymbolicLinkPrivilege", 0), "Got SeCreateSymbolicLinkPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeDelegateSessionUserImpersonatePrivilege", 0), "Got SeDelegateSessionUserImpersonatePrivilege.\n");
            }
            else if (!strcmpi(username, "NetworkService"))
            {
                service_ok(!is_in_privileges(privileges, "SeCreateTokenPrivilege", 0), "Got SeCreateTokenPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeAssignPrimaryTokenPrivilege", 0), "No SeAssignPrimaryTokenPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeLockMemoryPrivilege", 0), "Got SeLockMemoryPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseQuotaPrivilege", 0), "No SeIncreaseQuotaPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeMachineAccountPrivilege", 0), "Got SeMachineAccountPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTcbPrivilege", 0), "Got SeTcbPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSecurityPrivilege", 0), "Got SeSecurityPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTakeOwnershipPrivilege", 0), "Got SeTakeOwnershipPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeLoadDriverPrivilege", 0), "Got SeLoadDriverPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSystemProfilePrivilege", 0), "Got SeSystemProfilePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSystemtimePrivilege", 0), "Got SeSystemtimePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeProfileSingleProcessPrivilege", 0), "Got SeProfileSingleProcessPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeIncreaseBasePriorityPrivilege", 0), "Got SeIncreaseBasePriorityPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeCreatePagefilePrivilege", 0), "Got SeCreatePagefilePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeCreatePermanentPrivilege", 0), "Got SeCreatePermanentPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeBackupPrivilege", 0), "Got SeBackupPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRestorePrivilege", 0), "Got SeRestorePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeShutdownPrivilege", 0), "No SeShutdownPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeDebugPrivilege", 0), "Got SeDebugPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeAuditPrivilege", 0), "No SeAuditPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeSystemEnvironmentPrivilege", 0), "Got SeSystemEnvironmentPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeChangeNotifyPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeChangeNotifyPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRemoteShutdownPrivilege", 0), "Got SeRemoteShutdownPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeUndockPrivilege", 0), "No SeUndockPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeEnableDelegationPrivilege", 0), "Got SeEnableDelegationPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeManageVolumePrivilege", 0), "Got SeManageVolumePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeImpersonatePrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeImpersonatePrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeCreateGlobalPrivilege", SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED), "No SeCreateGlobalPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeTrustedCredManAccessPrivilege", 0), "Got SeTrustedCredManAccessPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeRelabelPrivilege", 0), "Got SeRelabelPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeIncreaseWorkingSetPrivilege", 0), "No SeIncreaseWorkingSetPrivilege.\n");
                service_ok(is_in_privileges(privileges, "SeTimeZonePrivilege", 0), "No SeTimeZonePrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeCreateSymbolicLinkPrivilege", 0), "Got SeCreateSymbolicLinkPrivilege.\n");
                service_ok(!is_in_privileges(privileges, "SeDelegateSessionUserImpersonatePrivilege", 0), "Got SeDelegateSessionUserImpersonatePrivilege.\n");
            }
            break;
        }
        case TokenOwner:
        {
            TOKEN_OWNER *owner = (TOKEN_OWNER *)buffer;

            if (!strcmpi(username, "System"))
                service_ok(EqualSid(owner->Owner, (PSID)&system), "Got unexpected owner.\n");
            else if (!strcmpi(username, "LocalService"))
                service_ok(EqualSid(owner->Owner, (PSID)&local_service), "Got unexpected owner.\n");
            else if (!strcmpi(username, "NetworkService"))
                service_ok(EqualSid(owner->Owner, (PSID)&network_service), "Got unexpected owner.\n");
            break;
        }
        case TokenPrimaryGroup:
        {
            TOKEN_PRIMARY_GROUP *group = (TOKEN_PRIMARY_GROUP *)buffer;

            if (!strcmpi(username, "System"))
                service_ok(EqualSid(group->PrimaryGroup, (PSID)&system), "Got unexpected primary group.\n");
            else if (!strcmpi(username, "LocalService"))
                service_ok(EqualSid(group->PrimaryGroup, (PSID)&local_service), "Got unexpected primary group.\n");
            else if (!strcmpi(username, "NetworkService"))
                service_ok(EqualSid(group->PrimaryGroup, (PSID)&network_service), "Got unexpected primary group.\n");
            break;
        }
        case TokenSource:
        {
            TOKEN_SOURCE *source = (TOKEN_SOURCE *)buffer;

            service_ok(!memcmp(source->SourceName, "Advapi  ", sizeof(source->SourceName)),
                       "Got unexpected source name %s.\n", wine_dbgstr_an(source->SourceName, sizeof(source->SourceName)));
            service_ok(source->SourceIdentifier.LowPart || source->SourceIdentifier.HighPart,
                       "Got unexpected luid.\n");
            break;
        }
        case TokenType:
        {
            TOKEN_TYPE *type = (TOKEN_TYPE *)buffer;

            service_ok(*type == TokenPrimary, "Got unexpected type %d.\n", *type);
            break;
        }
        case TokenSessionId:
        {
            DWORD *id = (DWORD *)buffer;

            service_todo_wine_ok(*id == 0, "Got unexpected id %#lx.\n", *id);
            break;
        }
        case TokenElevationType:
        {
            TOKEN_ELEVATION_TYPE *type = (TOKEN_ELEVATION_TYPE *)buffer;

            service_todo_wine_ok(*type == TokenElevationTypeDefault, "Got unexpected type %d.\n", *type);
            break;
        }
        case TokenIntegrityLevel:
        {
            TOKEN_MANDATORY_LABEL *label = (TOKEN_MANDATORY_LABEL *)buffer;

            service_todo_wine_ok(EqualSid(label->Label.Sid, (PSID)&system_label), "Got unexpected label.\n");
            break;
        }
        case TokenUIAccess:
        {
            DWORD *access = (DWORD *)buffer;

            service_todo_wine_ok(*access == 0, "Got unexpected access %#lx.\n", *access);
            break;
        }
        default:
            break;
        }
    }
}

static void test_GetUserName(const char *username, HANDLE token)
{
    char token_buf[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD) * SID_MAX_SUB_AUTHORITIES];
    TOKEN_USER *token_user = (TOKEN_USER *)token_buf;
    char buffer[512], buffer2[512];
    SID_NAME_USE name_use;
    DWORD size, size2;
    BOOL ret;

    ret = ImpersonateLoggedOnUser(token);
    service_ok(ret, "ImpersonateLoggedOnUser failed, error %lu.\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = GetUserNameA(buffer, &size);
    service_ok(ret, "GetUserNameA failed, error %lu.\n", GetLastError());
    if (!strcmpi(username, "System"))
    {
        service_ok(!strcmp("SYSTEM", buffer), "Expected username SYSTEM, got %s.\n", buffer);
    }
    else if (!strcmpi(username, "LocalService"))
    {
        service_ok(!strcmp("LOCAL SERVICE", buffer), "Expected username LOCAL SERVICE, got %s.\n", buffer);
    }
    else if (!strcmpi(username, "NetworkService"))
    {
        size = ARRAY_SIZE(buffer2);
        ret = GetComputerNameA(buffer2, &size);
        service_ok(ret, "GetComputerNameA failed, error %lu.\n", GetLastError());

        strcat(buffer2, "$");
        service_ok(!strcmp(buffer2, buffer), "Expected username %s, got %s.\n", buffer2, buffer);
    }

    size = sizeof(token_buf);
    ret = GetTokenInformation(token, TokenUser, token_user, size, &size);
    service_ok(ret, "GetTokenInformation failed, error %lu.\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    size2 = ARRAY_SIZE(buffer2);
    ret = LookupAccountSidA(NULL, token_user->User.Sid, buffer, &size, buffer2, &size2, &name_use);
    service_ok(ret, "LookupAccountSidA failed, error %lu.\n", GetLastError());

    if (!strcmpi(username, "System"))
        service_ok(!strcmp("SYSTEM", buffer), "Expected username SYSTEM, got %s.\n", buffer);
    else if (!strcmpi(username, "LocalService"))
        service_ok(!strcmp("LOCAL SERVICE", buffer), "Expected username LOCAL SERVICE, got %s.\n", buffer);
    else if (!strcmpi(username, "NetworkService"))
        service_ok(!strcmp("NETWORK SERVICE", buffer), "Expected username NETWORK SERVICE, got %s.\n", buffer);

    ret = RevertToSelf();
    service_ok(ret, "RevertToSelf failed, error %lu.\n", GetLastError());
}

/* LogonUser() needs the current thread user to be System to log into the System account. So the
 * tests need to be in a service */
static void test_LogonUser(void)
{
    HANDLE token;
    DWORD error;
    BOOL ret;
    int i;

    static struct
    {
        const char *username;
        const char *domain;
        const char *password;
        DWORD type;
        DWORD provider;
        BOOL expected_ret;
        DWORD expected_error;
        BOOL todo;
    }
    tests[] =
    {
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, TRUE, 0},
        {"LocalService", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, TRUE, 0},
        {"NetworkService", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, TRUE, 0},

        /* "LocalSystem" and "Service" are not accepted */
        {"LocalSystem", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},
        {"Service", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},

        /* Domain must be specified */
        {"System", NULL, NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},
        {"System", ".", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},

        /* User principal name formats are not accepted */
        {"System@NT AUTHORITY", NULL, NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},
        {"LocalService@NT AUTHORITY", NULL, NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},
        {"NetworkService@NT AUTHORITY", NULL, NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE},

        /* Passwords are ignored */
        {"System", "NT AUTHORITY", "123456", LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, TRUE, 0},
        {"LocalService", "NT AUTHORITY", "", LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, TRUE, 0},
        {"NetworkService", "NT AUTHORITY", "deadbeef", LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, TRUE, 0},

        /* Logon type should be LOGON32_LOGON_SERVICE. Using LOGON32_LOGON_NEW_CREDENTIALS can
         * succeed but it breaks the named pipe used by the tests. Exclude it for now */
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE, TRUE},
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE, TRUE},
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE, TRUE},
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_UNLOCK, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE, TRUE},
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_NETWORK_CLEARTEXT, LOGON32_PROVIDER_DEFAULT, FALSE, ERROR_LOGON_FAILURE, TRUE},

        /* Provider type should be LOGON32_PROVIDER_DEFAULT. Using LOGON32_PROVIDER_WINNT50 can
         * succeed but it breaks the named pipe used by the tests. Exclude it for now */
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_WINNT35, FALSE, ERROR_LOGON_FAILURE, TRUE},
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_WINNT40, FALSE, ERROR_LOGON_FAILURE, TRUE},
        {"System", "NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_VIRTUAL, FALSE, ERROR_ACCESS_DENIED, TRUE},
    };

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        SetLastError(0xdeadbeef);
        token = NULL;
        ret = LogonUserA(tests[i].username, tests[i].domain, tests[i].password, tests[i].type,
                         tests[i].provider, &token);
        error = GetLastError();

        if (tests[i].todo)
        {
            service_todo_wine_ok(ret == tests[i].expected_ret, "test %d: got unexpected return value %d.\n", i, ret);
            service_todo_wine_ok(error == tests[i].expected_error, "test %d: expected error %lu, got %lu.\n",
                                 i, tests[i].expected_error, error);
            if (ret && token && token != (HANDLE)0xdeadbeef /* Wine */)
                CloseHandle(token);
            continue;
        }
        else
        {
            service_ok(ret == tests[i].expected_ret, "test %d: got unexpected return value %d.\n", i, ret);
            service_ok(error == tests[i].expected_error, "test %d: expected error %lu, got %lu.\n",
                       i, tests[i].expected_error, error);
        }

        if (ret)
        {
            check_token_information(tests[i].username, token);
            test_GetUserName(tests[i].username, token);
            CloseHandle(token);
        }
    }
}

static DWORD WINAPI service_handler(DWORD ctrl, DWORD event_type, void *event_data, void *context)
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        service_event("STOP");
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus(service_handle, &status);
        SetEvent(service_stop_event);
        return NO_ERROR;
    case 128:
        test_winstation();
        test_create_window();
        test_monitors();
        test_LogonUser();
        service_event("CUSTOM");
        return 0xdeadbeef;
    default:
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI service_main(DWORD argc, char **argv)
{
    SERVICE_STATUS status;
    char buf[64];
    BOOL res;

    service_ok(argc == 3, "argc = %lu, expected 3\n", argc);
    service_ok(!strcmp(argv[0], service_name), "argv[0] = '%s', expected '%s'\n", argv[0], service_name);
    service_ok(!strcmp(argv[1], "param1"), "argv[1] = '%s', expected 'param1'\n", argv[1]);
    service_ok(!strcmp(argv[2], "param2"), "argv[2] = '%s', expected 'param2'\n", argv[2]);

    buf[0] = 0;
    GetEnvironmentVariableA("PATHEXT", buf, sizeof(buf));
    service_ok(buf[0], "did not find PATHEXT environment variable\n");

    service_handle = pRegisterServiceCtrlHandlerExA(service_name, service_handler, NULL);
    service_ok(service_handle != NULL, "RegisterServiceCtrlHandlerEx failed: %lu\n", GetLastError());
    if(!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %lu\n", GetLastError());

    service_event("RUNNING");

    WaitForSingleObject(service_stop_event, INFINITE);

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_STOPPED) failed: %lu\n", GetLastError());
}

static void service_process(void (WINAPI *p_service_main)(DWORD, char **))
{
    BOOL res;

    SERVICE_TABLE_ENTRYA servtbl[] = {
        {service_name, p_service_main},
        {NULL, NULL}
    };

    res = WaitNamedPipeA(named_pipe_name, NMPWAIT_USE_DEFAULT_WAIT);
    if(!res)
        return;

    pipe_handle = CreateFileA(named_pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(pipe_handle == INVALID_HANDLE_VALUE)
        return;

    service_trace("Starting...\n");

    service_stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    service_ok(service_stop_event != NULL, "Could not create event: %lu\n", GetLastError());
    if(!service_stop_event)
        return;

    res = StartServiceCtrlDispatcherA(servtbl);
    service_ok(res, "StartServiceCtrlDispatcher failed: %lu\n", GetLastError());

    /* Let service thread terminate */
    Sleep(50);

    CloseHandle(service_stop_event);
    CloseHandle(pipe_handle);
}

static DWORD WINAPI no_stop_handler(DWORD ctrl, DWORD event_type, void *event_data, void *context)
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            service_event("STOP");
            status.dwCurrentState     = SERVICE_STOPPED;
            status.dwControlsAccepted = 0;
            SetServiceStatus(service_handle, &status);
            SetEvent(service_stop_event);
            return NO_ERROR;
        default:
            status.dwCurrentState = SERVICE_RUNNING;
            SetServiceStatus( service_handle, &status );
            return NO_ERROR;
    }
}

static void WINAPI no_stop_main(DWORD argc, char **argv)
{
    SERVICE_STATUS status;
    BOOL res;

    service_ok(argc == 1, "argc = %lu, expected 1\n", argc);
    service_ok(!strcmp(argv[0], service_name), "argv[0] = '%s', expected '%s'\n", argv[0], service_name);

    service_handle = pRegisterServiceCtrlHandlerExA(service_name, no_stop_handler, NULL);
    service_ok(service_handle != NULL, "RegisterServiceCtrlHandlerEx failed: %lu\n", GetLastError());
    if(!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    res = SetServiceStatus(service_handle, &status);
    service_ok(res, "SetServiceStatus(SERVICE_RUNNING) failed: %lu\n", GetLastError());

    service_event("RUNNING");
}

/* Test process global variables */
static SC_HANDLE scm_handle;

static char current_event[32];
static HANDLE event_handle = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION event_cs;

static SC_HANDLE register_service(const char *test_name)
{
    char service_cmd[MAX_PATH+150], *ptr;
    SC_HANDLE service;

    ptr = service_cmd + GetModuleFileNameA(NULL, service_cmd, MAX_PATH);

    /* If the file doesn't exist, assume we're using built-in exe and append .so to the path */
    if(GetFileAttributesA(service_cmd) == INVALID_FILE_ATTRIBUTES) {
        strcpy(ptr, ".so");
        ptr += 3;
    }

    strcpy(ptr, " service ");
    ptr += strlen(ptr);
    sprintf(ptr, "%s ", test_name);
    ptr += strlen(ptr);
    strcpy(ptr, service_name);

    trace("service_cmd \"%s\"\n", service_cmd);

    service = CreateServiceA(scm_handle, service_name, service_name, GENERIC_ALL,
                             SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
                             service_cmd, NULL, NULL, NULL, NULL, NULL);
    if(!service && GetLastError() == ERROR_ACCESS_DENIED) {
        skip("Not enough access right to create service\n");
        return NULL;
    }

    ok(service != NULL, "CreateService failed: %lu\n", GetLastError());
    return service;
}

static void expect_event(const char *event_name)
{
    char evt[32];
    DWORD res;

    trace("waiting for %s\n", event_name);

    res = WaitForSingleObject(event_handle, 30000);
    ok(res == WAIT_OBJECT_0, "WaitForSingleObject failed: %lu\n", res);
    if(res != WAIT_OBJECT_0)
        return;

    EnterCriticalSection(&event_cs);
    strcpy(evt, current_event);
    *current_event = 0;
    LeaveCriticalSection(&event_cs);

    ok(!strcmp(evt, event_name), "Unexpected event: %s, expected %s\n", evt, event_name);
}

static DWORD WINAPI pipe_thread(void *arg)
{
    char buf[512], *ptr;
    DWORD read;
    BOOL res;

    res = ConnectNamedPipe(pipe_handle, NULL);
    ok(res || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe failed: %lu\n", GetLastError());

    while(1) {
        res = ReadFile(pipe_handle, buf, sizeof(buf), &read, NULL);
        if(!res) {
            ok(GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_INVALID_HANDLE,
               "ReadFile failed: %lu\n", GetLastError());
            break;
        }

        for(ptr = buf; ptr < buf+read; ptr += strlen(ptr)+1) {
            if(!strncmp(ptr, "TRACE:", 6)) {
                trace("service trace: %s", ptr+6);
            }else if(!strncmp(ptr, "OK:", 3)) {
                ok(1, "service: %s", ptr+3);
            }else if(!strncmp(ptr, "FAIL:", 5)) {
                ok(0, "service: %s", ptr+5);
            }else if(!strncmp(ptr, "TODO_WINE OK:", 13)) {
                todo_wine ok(1, "service: %s", ptr+13);
            }else if(!strncmp(ptr, "TODO_WINE FAIL:", 15)) {
                todo_wine ok(0, "service: %s", ptr+15);
            }else if(!strncmp(ptr, "EVENT:", 6)) {
                trace("service event: %s\n", ptr+6);
                EnterCriticalSection(&event_cs);
                ok(!current_event[0], "event %s still queued\n", current_event);
                strcpy(current_event, ptr+6);
                LeaveCriticalSection(&event_cs);
                SetEvent(event_handle);
            }else {
                ok(0, "malformed service message: %s\n", ptr);
            }
        }
    }

    DisconnectNamedPipe(pipe_handle);
    trace("pipe disconnected\n");
    return 0;
}

static void test_service(void)
{
    static const char *argv[2] = {"param1", "param2"};
    SC_HANDLE service_handle = register_service("simple_service");
    SERVICE_STATUS_PROCESS status2;
    SERVICE_STATUS status;
    DWORD bytes;
    BOOL res;

    if(!service_handle)
        return;

    trace("starting...\n");
    res = StartServiceA(service_handle, 2, argv);
    ok(res, "StartService failed: %lu\n", GetLastError());
    if(!res) {
        DeleteService(service_handle);
        CloseServiceHandle(service_handle);
        return;
    }
    expect_event("RUNNING");

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %ld\n", GetLastError());
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %lx\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_RUNNING, "status.dwCurrentState = %lx\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN),
            "status.dwControlsAccepted = %lx\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %ld\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %ld\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %ld\n", status.dwCheckPoint);
    todo_wine ok(status.dwWaitHint == 0, "status.dwWaitHint = %ld\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status2.dwCurrentState == SERVICE_RUNNING, "status2.dwCurrentState = %lx\n", status2.dwCurrentState);
    ok(status2.dwProcessId != 0, "status2.dwProcessId = %ld\n", status2.dwProcessId);

    res = ControlService(service_handle, 128, &status);
    ok(res, "ControlService failed: %lu\n", GetLastError());
    expect_event("CUSTOM");

    res = ControlService(service_handle, SERVICE_CONTROL_STOP, &status);
    ok(res, "ControlService failed: %lu\n", GetLastError());
    expect_event("STOP");

    res = DeleteService(service_handle);
    ok(res, "DeleteService failed: %lu\n", GetLastError());

    CloseServiceHandle(service_handle);
}

static inline void test_no_stop(void)
{
    SC_HANDLE service_handle = register_service("no_stop");
    SERVICE_STATUS_PROCESS status2;
    SERVICE_STATUS status;
    DWORD bytes;
    BOOL res;

    if(!service_handle)
        return;

    trace("starting...\n");
    res = StartServiceA(service_handle, 0, NULL);
    ok(res, "StartService failed: %lu\n", GetLastError());
    if(!res) {
        DeleteService(service_handle);
        CloseServiceHandle(service_handle);
        return;
    }
    expect_event("RUNNING");

    /* Let service thread terminate */
    Sleep(1000);

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %ld\n", GetLastError());
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %lx\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_RUNNING, "status.dwCurrentState = %lx\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN),
            "status.dwControlsAccepted = %lx\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %ld\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %ld\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %ld\n", status.dwCheckPoint);
    todo_wine ok(status.dwWaitHint == 0, "status.dwWaitHint = %ld\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status2.dwCurrentState == SERVICE_RUNNING, "status2.dwCurrentState = %lx\n", status2.dwCurrentState);
    ok(status2.dwProcessId != 0, "status2.dwProcessId = %ld\n", status2.dwProcessId);

    res = ControlService(service_handle, SERVICE_CONTROL_STOP, &status);
    ok(res, "ControlService failed: %lu\n", GetLastError());
    expect_event("STOP");

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %ld\n", GetLastError());
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %lx\n", status.dwServiceType);
    ok(status.dwCurrentState==SERVICE_STOPPED || status.dwCurrentState==SERVICE_STOP_PENDING,
            "status.dwCurrentState = %lx\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == 0, "status.dwControlsAccepted = %lx\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %ld\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %ld\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %ld\n", status.dwCheckPoint);
    ok(status.dwWaitHint == 0, "status.dwWaitHint = %ld\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status2.dwProcessId == 0 || broken(status2.dwProcessId != 0),
       "status2.dwProcessId = %ld\n", status2.dwProcessId);

    res = DeleteService(service_handle);
    ok(res, "DeleteService failed: %lu\n", GetLastError());

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "QueryServiceStatus failed: %ld\n", GetLastError());
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "status.dwServiceType = %lx\n", status.dwServiceType);
    ok(status.dwCurrentState==SERVICE_STOPPED || status.dwCurrentState==SERVICE_STOP_PENDING,
            "status.dwCurrentState = %lx\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == 0, "status.dwControlsAccepted = %lx\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == 0, "status.dwExitCode = %ld\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "status.dwServiceSpecificExitCode = %ld\n",
            status.dwServiceSpecificExitCode);
    ok(status.dwCheckPoint == 0, "status.dwCheckPoint = %ld\n", status.dwCheckPoint);
    ok(status.dwWaitHint == 0, "status.dwWaitHint = %ld\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status2.dwProcessId == 0 || broken(status2.dwProcessId != 0),
       "status2.dwProcessId = %ld\n", status2.dwProcessId);

    CloseServiceHandle(service_handle);

    res = QueryServiceStatus(service_handle, &status);
    ok(!res, "QueryServiceStatus should have failed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError = %ld\n", GetLastError());
}

static void test_runner(void (*p_run_test)(void))
{
    HANDLE thread;

    sprintf(service_name, "WineTestService%ld", GetTickCount());
    trace("service_name: %s\n", service_name);
    sprintf(named_pipe_name, "\\\\.\\pipe\\%s_pipe", service_name);

    pipe_handle = CreateNamedPipeA(named_pipe_name, PIPE_ACCESS_INBOUND,
                                   PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT, 10, 2048, 2048, 10000, NULL);
    ok(pipe_handle != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError());
    if(pipe_handle == INVALID_HANDLE_VALUE)
        return;

    event_handle = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(event_handle != INVALID_HANDLE_VALUE, "CreateEvent failed: %lu\n", GetLastError());
    if(event_handle == INVALID_HANDLE_VALUE)
        return;

    thread = CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL);
    ok(thread != NULL, "CreateThread failed: %lu\n", GetLastError());
    if(!thread)
        return;

    p_run_test();

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(event_handle);
    CloseHandle(pipe_handle);
    CloseHandle(thread);
}

static void CALLBACK notify_cb(void *user)
{
}

static inline void test_kill_service_process(void)
{
    static const char *argv[2] = {"param1", "param2"};
    SC_HANDLE service_handle = register_service("simple_service");
    SERVICE_STATUS_PROCESS status2;
    SERVICE_NOTIFYW notify;
    SERVICE_STATUS status;
    DWORD bytes, error;
    HANDLE process;
    BOOL res;

    if(!service_handle)
        return;

    trace("starting...\n");
    res = StartServiceA(service_handle, 2, argv);
    ok(res, "StartService failed: %lu\n", GetLastError());
    if(!res) {
        DeleteService(service_handle);
        CloseServiceHandle(service_handle);
        return;
    }
    expect_event("RUNNING");

    memset(&notify, 0, sizeof(notify));
    notify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notify.dwNotificationStatus = 0xdeadbeef;
    notify.pfnNotifyCallback = notify_cb;
    notify.pContext = &notify;
    error = NotifyServiceStatusChangeW(service_handle, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING
            | SERVICE_NOTIFY_PAUSED | SERVICE_NOTIFY_PAUSE_PENDING | SERVICE_NOTIFY_STOP_PENDING, &notify);
    ok(error == ERROR_SUCCESS, "got error %lu.\n", error);

    /* This shouldn't wait, we are supposed to get service start notification before. */
    SleepEx(5000, TRUE);
    ok(!notify.dwNotificationStatus, "got %#lx.\n", notify.dwNotificationStatus);
    ok(notify.dwNotificationTriggered == SERVICE_NOTIFY_RUNNING, "got %#lx.\n", notify.dwNotificationTriggered);
    ok(notify.ServiceStatus.dwCurrentState == SERVICE_RUNNING, "got %#lx.\n", notify.ServiceStatus.dwCurrentState);
    ok(!notify.ServiceStatus.dwWin32ExitCode, "got %#lx.\n", notify.ServiceStatus.dwWin32ExitCode);

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "got error %lu.\n", GetLastError());
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "got %lx.\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_RUNNING, "got %lx.\n", status.dwCurrentState);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "got error %lu.\n", GetLastError());
    ok(status2.dwCurrentState == SERVICE_RUNNING, "got %lx.\n", status2.dwCurrentState);
    ok(status2.dwProcessId, "got %ld\n", status2.dwProcessId);

    process = OpenProcess(PROCESS_TERMINATE, FALSE, status2.dwProcessId);
    ok(!!process, "got NULL.\n");
    res = TerminateProcess(process, 0xdeadbeef);
    ok(res, "got error %lu.\n", GetLastError());
    CloseHandle(process);

    memset(&notify, 0, sizeof(notify));
    notify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notify.dwNotificationStatus = 0xdeadbeef;
    notify.pfnNotifyCallback = notify_cb;
    notify.pContext = &notify;
    error = NotifyServiceStatusChangeW(service_handle, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING
            | SERVICE_NOTIFY_PAUSED | SERVICE_NOTIFY_PAUSE_PENDING | SERVICE_NOTIFY_STOP_PENDING, &notify);
    ok(error == ERROR_SUCCESS, "got error %lu.\n", error);

    SleepEx(3000, TRUE);
    ok(!notify.dwNotificationStatus, "got %#lx.\n", notify.dwNotificationStatus);
    ok(notify.dwNotificationTriggered == SERVICE_NOTIFY_STOPPED, "got %#lx.\n", notify.dwNotificationTriggered);
    ok(notify.ServiceStatus.dwCurrentState == SERVICE_STOPPED, "got %#lx.\n", notify.ServiceStatus.dwCurrentState);
    ok(notify.ServiceStatus.dwWin32ExitCode == ERROR_PROCESS_ABORTED, "got %#lx.\n", notify.ServiceStatus.dwWin32ExitCode);
    ok(!notify.ServiceStatus.dwServiceSpecificExitCode, "got %#lx.\n", notify.ServiceStatus.dwWin32ExitCode);

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "got error %lu.\n", error);
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "got %lx.\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_STOPPED, "got %lx.\n", status.dwCurrentState);
    ok(!status.dwControlsAccepted, "got %lx\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == ERROR_PROCESS_ABORTED, "got %ld.\n", status.dwWin32ExitCode);
    ok(!status.dwServiceSpecificExitCode, "got %ld.\n",
            status.dwServiceSpecificExitCode);
    ok(!status.dwCheckPoint, "got %ld.\n", status.dwCheckPoint);
    ok(!status.dwWaitHint, "got %ld.\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "got error %lu.\n", error);
    ok(!status2.dwProcessId, "got %ld.\n", status2.dwProcessId);

    res = DeleteService(service_handle);
    ok(res, "got error %lu.\n", error);

    res = QueryServiceStatus(service_handle, &status);
    ok(res, "got error %lu.\n", error);
    ok(status.dwServiceType == SERVICE_WIN32_OWN_PROCESS, "got %lx.\n", status.dwServiceType);
    ok(status.dwCurrentState == SERVICE_STOPPED, "got %lx.\n", status.dwCurrentState);
    ok(status.dwControlsAccepted == 0, "got %lx.\n", status.dwControlsAccepted);
    ok(status.dwWin32ExitCode == ERROR_PROCESS_ABORTED, "got %ld.\n", status.dwWin32ExitCode);
    ok(status.dwServiceSpecificExitCode == 0, "got %ld.\n",
            status.dwServiceSpecificExitCode);
    ok(!status.dwCheckPoint, "got %ld.\n", status.dwCheckPoint);
    ok(!status.dwWaitHint, "got %ld.\n", status.dwWaitHint);

    res = QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO, (BYTE *)&status2, sizeof(status2), &bytes);
    ok(res, "got error %lu.\n", GetLastError());
    ok(!status2.dwProcessId, "got %ld.\n", status2.dwProcessId);

    CloseServiceHandle(service_handle);

    res = QueryServiceStatus(service_handle, &status);
    ok(!res, "QueryServiceStatus should have failed.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %ld.\n", GetLastError());
}

START_TEST(service)
{
    char **argv;
    int argc;

    InitializeCriticalSection(&event_cs);

    pRegisterServiceCtrlHandlerExA = (void*)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegisterServiceCtrlHandlerExA");
    if(!pRegisterServiceCtrlHandlerExA) {
        win_skip("RegisterServiceCtrlHandlerExA not available, skipping tests\n");
        return;
    }

    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    ok(scm_handle != NULL || GetLastError() == ERROR_ACCESS_DENIED, "OpenSCManager failed: %lu\n", GetLastError());
    if(!scm_handle) {
        skip("OpenSCManager failed, skipping tests\n");
        return;
    }

    argc = winetest_get_mainargs(&argv);

    if(argc < 3) {
        test_runner(test_service);
        test_runner(test_no_stop);
        test_runner(test_kill_service_process);
    }else {
        strcpy(service_name, argv[3]);
        sprintf(named_pipe_name, "\\\\.\\pipe\\%s_pipe", service_name);

        if(!strcmp(argv[2], "simple_service"))
            service_process(service_main);
        else if(!strcmp(argv[2], "no_stop"))
            service_process(no_stop_main);
    }

    CloseServiceHandle(scm_handle);
}

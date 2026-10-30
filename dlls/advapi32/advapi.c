/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "wincred.h"
#include "wct.h"
#include "perflib.h"
#include "lmcons.h"

#include "wine/debug.h"

#include "advapi32_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 */
BOOL WINAPI GetUserNameA( LPSTR name, LPDWORD size )
{
    WCHAR nameW[UNLEN + 1];
    DWORD length, sizeW;

    sizeW = ARRAY_SIZE(nameW);
    if (!GetUserNameW( nameW, &sizeW ))
        return FALSE;

    length = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL );
    if (name && length <= *size)
    {
        WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, *size, NULL, NULL );
        *size = length;
        return TRUE;
    }
    else
    {
        *size = length;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 */
BOOL WINAPI GetUserNameW( LPWSTR name, LPDWORD size )
{
    static SID network_service = {SID_REVISION, 1, {SECURITY_NT_AUTHORITY}, {SECURITY_NETWORK_SERVICE_RID}};
    char token_buffer[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD) * SID_MAX_SUB_AUTHORITIES];
    TOKEN_USER *token_user = (TOKEN_USER *)token_buffer;
    DWORD token_buffer_size = 0, domain_size = MAX_PATH;
    WCHAR buffer[UNLEN + 2], domain[MAX_PATH];
    DWORD buffer_size = ARRAY_SIZE(buffer);
    SID_NAME_USE name_use;

    TRACE("name %p, size %p.\n", name, size);

    token_buffer_size = sizeof(token_buffer);
    if (!GetTokenInformation( GetCurrentThreadEffectiveToken(), TokenUser, token_user,
                              token_buffer_size, &token_buffer_size ))
        return FALSE;

    /* The NetworkService account returns "<HOSTNAME>$" */
    if (EqualSid( token_user->User.Sid, &network_service ))
    {
        if (!GetComputerNameW( buffer, &buffer_size ))
            return FALSE;

        wcscat( buffer, L"$" );
        buffer_size += 1;
    }
    else
    {
        if (!LookupAccountSidW( NULL, token_user->User.Sid, buffer, &buffer_size, domain,
                                &domain_size, &name_use ))
            return FALSE;
    }

    /* buffer_size from GetComputerNameW()/LookupAccountSidW() doesn't include the terminating NUL
     * character on success */
    buffer_size += 1;

    if (!name || *size < buffer_size)
    {
        *size = buffer_size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    memcpy( name, buffer, buffer_size * sizeof(WCHAR) );
    *size = buffer_size;
    return TRUE;
}

/******************************************************************************
 * GetCurrentHwProfileA [ADVAPI32.@]
 *
 * Get the current hardware profile.
 *
 * PARAMS
 *  pInfo [O] Destination for hardware profile information.
 *
 * RETURNS
 *  Success: TRUE. pInfo is updated with the hardware profile details.
 *  Failure: FALSE.
 */
BOOL WINAPI GetCurrentHwProfileA(LPHW_PROFILE_INFOA pInfo)
{
	FIXME("(%p) semi-stub\n", pInfo);
	pInfo->dwDockInfo = DOCKINFO_DOCKED;
	strcpy(pInfo->szHwProfileGuid,"{12340001-1234-1234-1234-123456789012}");
	strcpy(pInfo->szHwProfileName,"Wine Profile");
	return TRUE;
}

/******************************************************************************
 * GetCurrentHwProfileW [ADVAPI32.@]
 *
 * See GetCurrentHwProfileA.
 */
BOOL WINAPI GetCurrentHwProfileW(LPHW_PROFILE_INFOW pInfo)
{
       FIXME("(%p)\n", pInfo);
       return FALSE;
}


/**************************************************************************
 *	IsTextUnicode (ADVAPI32.@)
 *
 * Attempt to guess whether a text buffer is Unicode.
 *
 * PARAMS
 *  buf   [I] Text buffer to test
 *  len   [I] Length of buf
 *  flags [O] Destination for test results
 *
 * RETURNS
 *  TRUE if the buffer is likely Unicode, FALSE otherwise.
 */
BOOL WINAPI IsTextUnicode( LPCVOID buf, INT len, LPINT flags )
{
    return RtlIsTextUnicode( buf, len, flags );
}


/******************************************************************************
 * AbortSystemShutdownA [ADVAPI32.@]
 *
 * Stop a system shutdown if one is in progress.
 *
 * PARAMS
 *  lpMachineName [I] Name of machine to not shutdown.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES
 *  The Wine implementation of this function is a harmless stub.
 */
BOOL WINAPI AbortSystemShutdownA( LPSTR lpMachineName )
{
    TRACE("stub %s (harmless)\n", debugstr_a(lpMachineName));
    return TRUE;
}

/******************************************************************************
 * AbortSystemShutdownW [ADVAPI32.@]
 *
 * See AbortSystemShutdownA.
 */
BOOL WINAPI AbortSystemShutdownW( LPWSTR lpMachineName )
{
    TRACE("stub %s (harmless)\n", debugstr_w(lpMachineName));
    return TRUE;
}

/******************************************************************************
 * InitiateSystemShutdownExA [ADVAPI32.@]
 *
 * Initiate a shutdown or optionally restart the computer.
 *
 * PARAMS
 *  lpMachineName    [I] Network name of machine to shutdown.
 *  lpMessage        [I] Message displayed in shutdown dialog box.
 *  dwTimeout        [I] Number of seconds dialog is displayed before shutdown.
 *  bForceAppsClosed [I] If TRUE, apps close without saving, else dialog is
 *                       displayed requesting user to close apps.
 *  bRebootAfterShutdown [I] If TRUE, system reboots after restart, else the
 *                           system flushes all caches to disk and clears
 *                           the screen
 *  dwReason [I] Reason for shutting down.  Must be a system shutdown reason
 *               code.
 *
 *  RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 *
 *  NOTES
 *   if lpMachineName is NULL, the local computer is shutdown.
 */
BOOL WINAPI InitiateSystemShutdownExA( LPSTR lpMachineName, LPSTR lpMessage,
         DWORD dwTimeout, BOOL bForceAppsClosed, BOOL bRebootAfterShutdown,
         DWORD dwReason)
{
     FIXME("%s %s %ld %d %d %#lx\n", debugstr_a(lpMachineName),
            debugstr_a(lpMessage), dwTimeout, bForceAppsClosed,
            bRebootAfterShutdown, dwReason);
     return TRUE;
} 

/******************************************************************************
 * InitiateSystemShutdownExW [ADVAPI32.@]
 *
 * See InitiateSystemShutdownExA.
 */
BOOL WINAPI InitiateSystemShutdownExW( LPWSTR lpMachineName, LPWSTR lpMessage,
         DWORD dwTimeout, BOOL bForceAppsClosed, BOOL bRebootAfterShutdown,
         DWORD dwReason)
{
     FIXME("%s %s %ld %d %d %#lx\n", debugstr_w(lpMachineName),
            debugstr_w(lpMessage), dwTimeout, bForceAppsClosed,
            bRebootAfterShutdown, dwReason);
     return TRUE;
} 

BOOL WINAPI InitiateSystemShutdownA( LPSTR lpMachineName, LPSTR lpMessage, DWORD dwTimeout,
                                     BOOL bForceAppsClosed, BOOL bRebootAfterShutdown )
{
    return InitiateSystemShutdownExA( lpMachineName, lpMessage, dwTimeout,
                                      bForceAppsClosed, bRebootAfterShutdown,
                                      SHTDN_REASON_MAJOR_LEGACY_API );
}

BOOL WINAPI InitiateSystemShutdownW( LPWSTR lpMachineName, LPWSTR lpMessage, DWORD dwTimeout,
                                     BOOL bForceAppsClosed, BOOL bRebootAfterShutdown )
{
    return InitiateSystemShutdownExW( lpMachineName, lpMessage, dwTimeout,
                                      bForceAppsClosed, bRebootAfterShutdown,
                                      SHTDN_REASON_MAJOR_LEGACY_API );
}

/***********************************************************************
 *     InitiateShutdownA [ADVAPI32.@]
 */
DWORD WINAPI InitiateShutdownA(char *name, char *message, DWORD seconds, DWORD flags, DWORD reason)
{
    FIXME("%s, %s, %ld, %ld, %ld stub\n", debugstr_a(name), debugstr_a(message), seconds, flags, reason);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *     InitiateShutdownW [ADVAPI32.@]
 */
DWORD WINAPI InitiateShutdownW(WCHAR *name, WCHAR *message, DWORD seconds, DWORD flags, DWORD reason)
{
    FIXME("%s, %s, %ld, %ld, %ld stub\n", debugstr_w(name), debugstr_w(message), seconds, flags, reason);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI LogonUserA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword,
                        DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    WCHAR *usernameW = NULL, *domainW = NULL, *passwordW = NULL;
    BOOL ret = FALSE;

    TRACE("%s %s %p 0x%08lx 0x%08lx %p\n", debugstr_a(lpszUsername),
          debugstr_a(lpszDomain), lpszPassword, dwLogonType, dwLogonProvider, phToken);

    if (lpszUsername && !(usernameW = strdupAW( lpszUsername ))) return FALSE;
    if (lpszDomain && !(domainW = strdupAW( lpszDomain ))) goto done;
    if (lpszPassword && !(passwordW = strdupAW( lpszPassword ))) goto done;

    ret = LogonUserW( usernameW, domainW, passwordW, dwLogonType, dwLogonProvider, phToken );

done:
    free( usernameW );
    free( domainW );
    free( passwordW );
    return ret;
}

struct max_sid
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
};

static void init_sid_and_attributes( SID_AND_ATTRIBUTES *sid_attr, PSID sid, DWORD attr )
{
    sid_attr->Sid = sid;
    sid_attr->Attributes = attr;
}

static void init_privilege( LUID_AND_ATTRIBUTES *luid_attr, DWORD luid_low, DWORD attr )
{
    luid_attr->Luid.LowPart = luid_low;
    luid_attr->Luid.HighPart = 0;
    luid_attr->Attributes = attr;
}

BOOL WINAPI LogonUserW( LPCWSTR lpszUsername, LPCWSTR lpszDomain, LPCWSTR lpszPassword,
                        DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    if (dwLogonType == LOGON32_LOGON_SERVICE)
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
        DWORD_PTR privileges_buffer[(offsetof(TOKEN_PRIVILEGES, Privileges[28]) + sizeof(DWORD_PTR) - 1) / sizeof(DWORD_PTR)];
        DWORD_PTR groups_buffer[(offsetof(TOKEN_GROUPS, Groups[5]) + sizeof(DWORD_PTR) - 1) / sizeof(DWORD_PTR)];
        TOKEN_PRIVILEGES *privileges = (TOKEN_PRIVILEGES *)privileges_buffer;
        TOKEN_GROUPS *groups = (TOKEN_GROUPS *)groups_buffer;
        TOKEN_PRIMARY_GROUP primary_group;
        LUID_AND_ATTRIBUTES *luid_attr;
        SID_AND_ATTRIBUTES *sid_attr;
        LARGE_INTEGER expire = {0};
        TOKEN_SOURCE source = {0};
        OBJECT_ATTRIBUTES attr;
        TOKEN_USER user = {0};
        LUID auth_id = {0};
        TOKEN_OWNER owner;
        NTSTATUS status;

        TRACE("%s %s %p 0x%08lx 0x%08lx %p\n", debugstr_w( lpszUsername ), debugstr_w( lpszDomain ),
              lpszPassword, dwLogonType, dwLogonProvider, phToken);

        if (!(lpszDomain && lpszUsername && !lstrcmpiW( lpszDomain, L"NT AUTHORITY" )
             && (!lstrcmpiW( lpszUsername, L"System" )
             || !lstrcmpiW( lpszUsername, L"LocalService" )
             || !lstrcmpiW( lpszUsername, L"NetworkService" ))))
        {
            SetLastError( ERROR_LOGON_FAILURE );
            return FALSE;
        }

        if (!lstrcmpiW( lpszUsername, L"System" ))
        {
            user.User.Sid = (PSID)&system;
            owner.Owner = (PSID)&system;
            primary_group.PrimaryGroup = (PSID)&system;

            groups->GroupCount = 3;
            sid_attr = groups->Groups;
            init_sid_and_attributes( &sid_attr[0], (PSID)&administrators, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_OWNER );
            init_sid_and_attributes( &sid_attr[1], (PSID)&everyone, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[2], (PSID)&authenticated_users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );

            privileges->PrivilegeCount = 28;
            luid_attr = privileges->Privileges;
            init_privilege( &luid_attr[0], SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0 );
            init_privilege( &luid_attr[1], SE_LOCK_MEMORY_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[2], SE_INCREASE_QUOTA_PRIVILEGE, 0 );
            init_privilege( &luid_attr[3], SE_TCB_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[4], SE_SECURITY_PRIVILEGE, 0 );
            init_privilege( &luid_attr[5], SE_TAKE_OWNERSHIP_PRIVILEGE, 0 );
            init_privilege( &luid_attr[6], SE_LOAD_DRIVER_PRIVILEGE, 0 );
            init_privilege( &luid_attr[7], SE_SYSTEM_PROFILE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[8], SE_SYSTEMTIME_PRIVILEGE, 0 );
            init_privilege( &luid_attr[9], SE_PROF_SINGLE_PROCESS_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[10], SE_INC_BASE_PRIORITY_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[11], SE_CREATE_PAGEFILE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[12], SE_CREATE_PERMANENT_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[13], SE_BACKUP_PRIVILEGE, 0 );
            init_privilege( &luid_attr[14], SE_RESTORE_PRIVILEGE, 0 );
            init_privilege( &luid_attr[15], SE_SHUTDOWN_PRIVILEGE, 0 );
            init_privilege( &luid_attr[16], SE_DEBUG_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[17], SE_AUDIT_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[18], SE_SYSTEM_ENVIRONMENT_PRIVILEGE, 0 );
            init_privilege( &luid_attr[19], SE_CHANGE_NOTIFY_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[20], SE_UNDOCK_PRIVILEGE, 0 );
            init_privilege( &luid_attr[21], SE_MANAGE_VOLUME_PRIVILEGE, 0 );
            init_privilege( &luid_attr[22], SE_IMPERSONATE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[23], SE_CREATE_GLOBAL_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[24], SE_INC_WORKING_SET_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[25], SE_TIME_ZONE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[26], SE_CREATE_SYMBOLIC_LINK_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[27], SE_DELEGATE_SESSION_USER_IMPERSONATE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
        }
        else if (!lstrcmpiW(lpszUsername, L"LocalService"))
        {
            user.User.Sid = (PSID)&local_service;
            owner.Owner = (PSID)&local_service;
            primary_group.PrimaryGroup = (PSID)&local_service;

            groups->GroupCount = 5;
            sid_attr = groups->Groups;
            init_sid_and_attributes( &sid_attr[0], (PSID)&everyone, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[1], (PSID)&users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[2], (PSID)&authenticated_users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[3], (PSID)&local, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[4], (PSID)&console_logon, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );

            privileges->PrivilegeCount = 11;
            luid_attr = privileges->Privileges;
            init_privilege( &luid_attr[0], SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0 );
            init_privilege( &luid_attr[1], SE_INCREASE_QUOTA_PRIVILEGE, 0 );
            init_privilege( &luid_attr[2], SE_SYSTEMTIME_PRIVILEGE, 0 );
            init_privilege( &luid_attr[3], SE_SHUTDOWN_PRIVILEGE, 0 );
            init_privilege( &luid_attr[4], SE_AUDIT_PRIVILEGE, 0 );
            init_privilege( &luid_attr[5], SE_CHANGE_NOTIFY_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[6], SE_UNDOCK_PRIVILEGE, 0);
            init_privilege( &luid_attr[7], SE_IMPERSONATE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[8], SE_CREATE_GLOBAL_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[9], SE_INC_WORKING_SET_PRIVILEGE, 0 );
            init_privilege( &luid_attr[10], SE_TIME_ZONE_PRIVILEGE, 0 );
        }
        else
        {
            user.User.Sid = (PSID)&network_service;
            owner.Owner = (PSID)&network_service;
            primary_group.PrimaryGroup = (PSID)&network_service;

            groups->GroupCount = 5;
            sid_attr = groups->Groups;
            init_sid_and_attributes( &sid_attr[0], (PSID)&everyone, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[1], (PSID)&users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[2], (PSID)&authenticated_users, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[3], (PSID)&local, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );
            init_sid_and_attributes( &sid_attr[4], (PSID)&console_logon, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY );

            privileges->PrivilegeCount = 10;
            luid_attr = privileges->Privileges;
            init_privilege( &luid_attr[0], SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0 );
            init_privilege( &luid_attr[1], SE_INCREASE_QUOTA_PRIVILEGE, 0 );
            init_privilege( &luid_attr[2], SE_SHUTDOWN_PRIVILEGE, 0 );
            init_privilege( &luid_attr[3], SE_AUDIT_PRIVILEGE, 0 );
            init_privilege( &luid_attr[4], SE_CHANGE_NOTIFY_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[5], SE_UNDOCK_PRIVILEGE, 0);
            init_privilege( &luid_attr[6], SE_IMPERSONATE_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[7], SE_CREATE_GLOBAL_PRIVILEGE, SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED );
            init_privilege( &luid_attr[8], SE_INC_WORKING_SET_PRIVILEGE, 0 );
            init_privilege( &luid_attr[9], SE_TIME_ZONE_PRIVILEGE, 0 );
        }

        InitializeObjectAttributes( &attr, NULL, 0, 0, NULL );
        status = NtCreateToken( phToken, TOKEN_ALL_ACCESS, &attr, TokenPrimary, &auth_id, &expire,
                                &user, groups, privileges, &owner, &primary_group, NULL, &source );
        RtlSetLastWin32Error( RtlNtStatusToDosError( status ) );
        return !status;
    }

    FIXME("%s %s %p 0x%08lx 0x%08lx %p - stub\n", debugstr_w( lpszUsername ),
          debugstr_w( lpszDomain ), lpszPassword, dwLogonType, dwLogonProvider, phToken);

    *phToken = (HANDLE *)0xdeadbeef;
    return TRUE;
}

typedef UINT (WINAPI *fnMsiProvideComponentFromDescriptor)(LPCWSTR,LPWSTR,DWORD*,DWORD*);

DWORD WINAPI CommandLineFromMsiDescriptor( WCHAR *szDescriptor,
                    WCHAR *szCommandLine, DWORD *pcchCommandLine )
{
    fnMsiProvideComponentFromDescriptor mpcfd;
    HMODULE hmsi;
    UINT r = ERROR_CALL_NOT_IMPLEMENTED;

    TRACE("%s %p %p\n", debugstr_w(szDescriptor), szCommandLine, pcchCommandLine);

    hmsi = LoadLibraryW( L"msi" );
    if (!hmsi)
        return r;
    mpcfd = (fnMsiProvideComponentFromDescriptor)GetProcAddress( hmsi,
                                                                 "MsiProvideComponentFromDescriptorW" );
    if (mpcfd)
        r = mpcfd( szDescriptor, szCommandLine, pcchCommandLine, NULL );
    FreeLibrary( hmsi );
    return r;
}

/***********************************************************************
 *      RegisterWaitChainCOMCallback (ole32.@)
 */
void WINAPI RegisterWaitChainCOMCallback(PCOGETCALLSTATE call_state_cb,
                                         PCOGETACTIVATIONSTATE activation_state_cb)
{
    FIXME("%p, %p\n", call_state_cb, activation_state_cb);
}

HWCT WINAPI OpenThreadWaitChainSession(DWORD flags, PWAITCHAINCALLBACK callback)
{
    FIXME("flags %ld, callback %p stub!\n", flags, callback);
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

BOOL WINAPI GetThreadWaitChain(HWCT handle, DWORD_PTR ctx, DWORD flags, DWORD thread_id, DWORD *node_count,
    WAITCHAIN_NODE_INFO *node_info_arr, BOOL *is_cycle)
{
    FIXME( "handle %p, ctx %Ix, flags %ld, thread_id %ld, node_count %p, node_info_arr %p, is_cycle %p stub!\n",
           handle, ctx, flags, thread_id, node_count, node_info_arr, is_cycle );
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

ULONG WINAPI PerfCloseQueryHandle( HANDLE query )
{
    FIXME( "query %p stub.\n", query );

    return ERROR_SUCCESS;
}

ULONG WINAPI PerfOpenQueryHandle( const WCHAR *machine, HANDLE *query )
{
    FIXME( "machine %s, query %p.\n", debugstr_w(machine), query );

    if (!query) return ERROR_INVALID_PARAMETER;
    *query = (HANDLE)0xdeadbeef;

    return ERROR_SUCCESS;
}

ULONG WINAPI PerfAddCounters( HANDLE query, PERF_COUNTER_IDENTIFIER *id, DWORD size )
{
    FIXME( "query %p, id %p, size %lu stub.\n", query, id, size );

    if (!id || size < sizeof(*id) || id->Size < sizeof(*id)) return ERROR_INVALID_PARAMETER;

    id->Status = ERROR_WMI_GUID_NOT_FOUND;
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfQueryCounterData( HANDLE query, PERF_DATA_HEADER *data, DWORD data_size, DWORD *size_needed )
{
    FIXME( "query %p, data %p, data_size %lu, size_needed %p stub.\n", query, data, data_size, size_needed );

    if (!size_needed) return ERROR_INVALID_PARAMETER;

    *size_needed = sizeof(PERF_DATA_HEADER);

    if (!data || data_size < sizeof(PERF_DATA_HEADER)) return ERROR_NOT_ENOUGH_MEMORY;

    data->dwTotalSize = sizeof(PERF_DATA_HEADER);
    data->dwNumCounters = 0;
    QueryPerformanceCounter( (LARGE_INTEGER *)&data->PerfTimeStamp );
    QueryPerformanceFrequency( (LARGE_INTEGER *)&data->PerfFreq );
    GetSystemTimeAsFileTime( (FILETIME *)&data->PerfTime100NSec );
    FileTimeToSystemTime( (FILETIME *)&data->PerfTime100NSec, &data->SystemTime );

    return ERROR_SUCCESS;
}

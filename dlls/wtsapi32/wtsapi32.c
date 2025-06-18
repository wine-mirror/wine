/* Copyright 2005 Ulrich Czekalla
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "lmcons.h"
#include "wtsapi32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wtsapi);


/************************************************************
 *                WTSCloseServer  (WTSAPI32.@)
 */
void WINAPI WTSCloseServer(HANDLE hServer)
{
    FIXME("Stub %p\n", hServer);
}

/************************************************************
 *                WTSConnectSessionA  (WTSAPI32.@)
 */
BOOL WINAPI WTSConnectSessionA(ULONG LogonId, ULONG TargetLogonId, PSTR pPassword, BOOL bWait)
{
   FIXME("Stub %ld %ld (%s) %d\n", LogonId, TargetLogonId, debugstr_a(pPassword), bWait);
   return TRUE;
}

/************************************************************
 *                WTSConnectSessionW  (WTSAPI32.@)
 */
BOOL WINAPI WTSConnectSessionW(ULONG LogonId, ULONG TargetLogonId, PWSTR pPassword, BOOL bWait)
{
   FIXME("Stub %ld %ld (%s) %d\n", LogonId, TargetLogonId, debugstr_w(pPassword), bWait);
   return TRUE;
}

/************************************************************
 *                WTSDisconnectSession  (WTSAPI32.@)
 */
BOOL WINAPI WTSDisconnectSession(HANDLE hServer, DWORD SessionId, BOOL bWait)
{
    FIXME("Stub %p 0x%08lx %d\n", hServer, SessionId, bWait);
    return TRUE;
}

/************************************************************
 *                WTSEnableChildSessions  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnableChildSessions(BOOL enable)
{
    FIXME("Stub %d\n", enable);
    return TRUE;
}


/************************************************************
 *                WTSEnumerateProcessesExW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesExW(HANDLE server, DWORD *level, DWORD session_id,
        WCHAR **ret_info, DWORD *ret_count)
{
    SYSTEM_PROCESS_INFORMATION *nt_info, *nt_process;
    WTS_PROCESS_INFOW *info;
    ULONG nt_size = 4096;
    DWORD count, size;
    NTSTATUS status;
    char *p;

    TRACE("server %p, level %lu, session_id %#lx, ret_info %p, ret_count %p\n",
            server, *level, session_id, ret_info, ret_count);

    if (!ret_info || !ret_count)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (session_id != WTS_ANY_SESSION)
        FIXME("ignoring session id %#lx\n", session_id);

    if (*level)
    {
        FIXME("unhandled level %lu\n", *level);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!(nt_info = malloc(nt_size)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    while ((status = NtQuerySystemInformation(SystemProcessInformation, nt_info,
            nt_size, NULL)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        SYSTEM_PROCESS_INFORMATION *new_info;

        nt_size *= 2;
        if (!(new_info = realloc(nt_info, nt_size)))
        {
            free(nt_info);
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        nt_info = new_info;
    }
    if (status)
    {
        free(nt_info);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    size = 0;
    count = 0;
    nt_process = nt_info;
    for (;;)
    {
        size += sizeof(WTS_PROCESS_INFOW) + nt_process->ProcessName.Length + sizeof(WCHAR);
        size += offsetof(SID, SubAuthority[SID_MAX_SUB_AUTHORITIES]);
        ++count;

        if (!nt_process->NextEntryOffset)
            break;
        nt_process = (SYSTEM_PROCESS_INFORMATION *)((char *)nt_process + nt_process->NextEntryOffset);
    }

    if (!(info = malloc(size)))
    {
        free(nt_info);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    p = (char *)(info + count);

    count = 0;
    nt_process = nt_info;
    for (;;)
    {
        HANDLE process, token;

        info[count].SessionId = nt_process->SessionId;
        info[count].ProcessId = (DWORD_PTR)nt_process->UniqueProcessId;

        info[count].pProcessName = (WCHAR *)p;
        memcpy(p, nt_process->ProcessName.Buffer, nt_process->ProcessName.Length);
        info[count].pProcessName[nt_process->ProcessName.Length / sizeof(WCHAR)] = 0;
        p += nt_process->ProcessName.Length + sizeof(WCHAR);

        info[count].pUserSid = NULL;
        if ((process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info[count].ProcessId)))
        {
            if (OpenProcessToken(process, TOKEN_QUERY, &token))
            {
                char buffer[sizeof(TOKEN_USER) + offsetof(SID, SubAuthority[SID_MAX_SUB_AUTHORITIES])];
                TOKEN_USER *user = (TOKEN_USER *)buffer;
                DWORD size;

                GetTokenInformation(token, TokenUser, buffer, sizeof(buffer), &size);
                info[count].pUserSid = p;
                size = GetLengthSid(user->User.Sid);
                memcpy(p, user->User.Sid, size);
                p += size;
                CloseHandle(token);
            }
            CloseHandle(process);
        }

        ++count;
        if (!nt_process->NextEntryOffset)
            break;
        nt_process = (SYSTEM_PROCESS_INFORMATION *)((char *)nt_process + nt_process->NextEntryOffset);
    }

    *ret_info = (WCHAR *)info;
    *ret_count = count;
    SetLastError(0);
    return TRUE;
}

/************************************************************
 *                WTSEnumerateProcessesExA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesExA(HANDLE server, DWORD *level, DWORD session_id, char **info, DWORD *count)
{
    FIXME("Stub %p %p %ld %p %p\n", server, level, session_id, info, count);
    if (count) *count = 0;
    return FALSE;
}

/************************************************************
 *                WTSEnumerateProcessesA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesA(HANDLE hServer, DWORD Reserved, DWORD Version,
    PWTS_PROCESS_INFOA* ppProcessInfo, DWORD* pCount)
{
    FIXME("Stub %p 0x%08lx 0x%08lx %p %p\n", hServer, Reserved, Version,
          ppProcessInfo, pCount);

    if (!ppProcessInfo || !pCount) return FALSE;

    *pCount = 0;
    *ppProcessInfo = NULL;

    return TRUE;
}

/************************************************************
 *                WTSEnumerateProcessesW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesW(HANDLE server, DWORD reserved, DWORD version,
        WTS_PROCESS_INFOW **info, DWORD *count)
{
    DWORD level = 0;

    TRACE("server %p, reserved %#lx, version %lu, info %p, count %p\n", server, reserved, version, info, count);

    if (reserved || version != 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return WTSEnumerateProcessesExW(server, &level, WTS_ANY_SESSION, (WCHAR **)info, count);
}

/************************************************************
 *                WTSEnumerateServersA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateServersA(LPSTR pDomainName, DWORD Reserved, DWORD Version, PWTS_SERVER_INFOA *ppServerInfo, DWORD *pCount)
{
    FIXME("Stub %s 0x%08lx 0x%08lx %p %p\n", debugstr_a(pDomainName), Reserved, Version, ppServerInfo, pCount);
    return FALSE;
}

/************************************************************
 *                WTSEnumerateServersW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateServersW(LPWSTR pDomainName, DWORD Reserved, DWORD Version, PWTS_SERVER_INFOW *ppServerInfo, DWORD *pCount)
{
    FIXME("Stub %s 0x%08lx 0x%08lx %p %p\n", debugstr_w(pDomainName), Reserved, Version, ppServerInfo, pCount);
    return FALSE;
}

/************************************************************
 *                WTSEnumerateEnumerateSessionsExW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsExW(HANDLE server, DWORD *level, DWORD filter, WTS_SESSION_INFO_1W* info, DWORD *count)
{
    FIXME("Stub %p %p %ld %p %p\n", server, level, filter, info, count);
    if (count) *count = 0;
    return FALSE;
}

/************************************************************
 *                WTSEnumerateEnumerateSessionsExA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsExA(HANDLE server, DWORD *level, DWORD filter, WTS_SESSION_INFO_1A* info, DWORD *count)
{
    FIXME("Stub %p %p %ld %p %p\n", server, level, filter, info, count);
    if (count) *count = 0;
    return FALSE;
}

/************************************************************
 *                WTSEnumerateEnumerateSessionsA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsA(HANDLE server, DWORD reserved, DWORD version,
        PWTS_SESSION_INFOA *session_info, DWORD *count)
{
    PWTS_SESSION_INFOW infoW;
    DWORD size, offset;
    unsigned int i;
    int len;

    TRACE("%p 0x%08lx 0x%08lx %p %p.\n", server, reserved, version, session_info, count);

    if (!session_info || !count) return FALSE;

    if (!WTSEnumerateSessionsW(server, reserved, version, &infoW, count)) return FALSE;

    size = 0;
    for (i = 0; i < *count; ++i)
    {
        if (!(len = WideCharToMultiByte(CP_ACP, 0, infoW[i].pWinStationName, -1, NULL, 0, NULL, NULL)))
        {
            ERR("WideCharToMultiByte failed.\n");
            WTSFreeMemory(infoW);
            return FALSE;
        }
        size += sizeof(**session_info) + len;
    }

    if (!(*session_info = malloc(size)))
    {
        WTSFreeMemory(infoW);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    offset = *count * sizeof(**session_info);
    for (i = 0; i < *count; ++i)
    {
        (*session_info)[i].State = infoW[i].State;
        (*session_info)[i].SessionId = infoW[i].SessionId;
        (*session_info)[i].pWinStationName = (char *)(*session_info) + offset;
        len = WideCharToMultiByte(CP_ACP, 0, infoW[i].pWinStationName, -1, (*session_info)[i].pWinStationName,
                size - offset, NULL, NULL);
        if (!len)
        {
            ERR("WideCharToMultiByte failed.\n");
            WTSFreeMemory(*session_info);
            WTSFreeMemory(infoW);
        }
        offset += len;
    }

    WTSFreeMemory(infoW);
    return TRUE;
}

/************************************************************
 *                WTSEnumerateEnumerateSessionsW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsW(HANDLE server, DWORD reserved, DWORD version,
        PWTS_SESSION_INFOW *session_info, DWORD *count)
{
    static const WCHAR session_name[] = L"Console";

    FIXME("%p 0x%08lx 0x%08lx %p %p semi-stub.\n", server, reserved, version, session_info, count);

    if (!session_info || !count) return FALSE;

    if (!(*session_info = malloc(sizeof(**session_info) + sizeof(session_name))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    if (!ProcessIdToSessionId( GetCurrentProcessId(), &(*session_info)->SessionId))
    {
        WTSFreeMemory(*session_info);
        return FALSE;
    }
    *count = 1;
    (*session_info)->State = WTSActive;
    (*session_info)->pWinStationName = (WCHAR *)((char *)*session_info + sizeof(**session_info));
    memcpy((*session_info)->pWinStationName, session_name, sizeof(session_name));

    return TRUE;
}

/************************************************************
 *                WTSFreeMemory (WTSAPI32.@)
 */
void WINAPI WTSFreeMemory(PVOID pMemory)
{
    free(pMemory);
}

/************************************************************
 *                WTSFreeMemoryExA (WTSAPI32.@)
 */
BOOL WINAPI WTSFreeMemoryExA(WTS_TYPE_CLASS type, void *ptr, ULONG nmemb)
{
    TRACE("%d %p %ld\n", type, ptr, nmemb);
    free(ptr);
    return TRUE;
}

/************************************************************
 *                WTSFreeMemoryExW (WTSAPI32.@)
 */
BOOL WINAPI WTSFreeMemoryExW(WTS_TYPE_CLASS type, void *ptr, ULONG nmemb)
{
    TRACE("%d %p %ld\n", type, ptr, nmemb);
    free(ptr);
    return TRUE;
}


/************************************************************
 *                WTSLogoffSession (WTSAPI32.@)
 */
BOOL WINAPI WTSLogoffSession(HANDLE hserver, DWORD session_id, BOOL bwait)
{
    FIXME("(%p, 0x%lx, %d): stub\n", hserver, session_id, bwait);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/************************************************************
 *                WTSOpenServerExW (WTSAPI32.@)
 */
HANDLE WINAPI WTSOpenServerExW(WCHAR *server_name)
{
    FIXME("(%s) stub\n", debugstr_w(server_name));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/************************************************************
 *                WTSOpenServerExA (WTSAPI32.@)
 */
HANDLE WINAPI WTSOpenServerExA(char *server_name)
{
    FIXME("(%s) stub\n", debugstr_a(server_name));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/************************************************************
 *                WTSOpenServerA (WTSAPI32.@)
 */
HANDLE WINAPI WTSOpenServerA(LPSTR pServerName)
{
    FIXME("(%s) stub\n", debugstr_a(pServerName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/************************************************************
 *                WTSOpenServerW (WTSAPI32.@)
 */
HANDLE WINAPI WTSOpenServerW(LPWSTR pServerName)
{
    FIXME("(%s) stub\n", debugstr_w(pServerName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/************************************************************
 *                WTSQuerySessionInformationA  (WTSAPI32.@)
 */
BOOL WINAPI WTSQuerySessionInformationA(HANDLE server, DWORD session_id, WTS_INFO_CLASS class, char **buffer, DWORD *count)
{
    WCHAR *bufferW = NULL;

    TRACE("%p 0x%08lx %d %p %p\n", server, session_id, class, buffer, count);

    if (!buffer || !count)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (class == WTSClientProtocolType || class == WTSConnectState)
        return WTSQuerySessionInformationW(server, session_id, class, (WCHAR **)buffer, count);

    if (class == WTSSessionInfo)
    {
        DWORD size;
        WTSINFOA *info;
        WTSINFOW *infoW;

        if (!WTSQuerySessionInformationW(server, session_id, class, (WCHAR **)&infoW, &size))
            return FALSE;

        if (!(info = malloc(sizeof(*info))))
        {
            WTSFreeMemory(infoW);
            return FALSE;
        }
        memset(info, 0, sizeof(*info));
        info->State = infoW->State;
        info->SessionId = infoW->SessionId;
        size = sizeof(info->WinStationName);
        if (!WideCharToMultiByte(CP_ACP, 0, infoW->WinStationName, -1, info->WinStationName, size, NULL, NULL))
        {
            WTSFreeMemory(infoW);
            free(info);
            return FALSE;
        }
        size = sizeof(info->Domain);
        if (!WideCharToMultiByte(CP_ACP, 0, infoW->Domain, -1, info->Domain, size, NULL, NULL))
        {
            WTSFreeMemory(infoW);
            free(info);
            return FALSE;
        }
        size = sizeof(info->UserName);
        if (!WideCharToMultiByte(CP_ACP, 0, infoW->UserName, -1, info->UserName, size, NULL, NULL))
        {
            WTSFreeMemory(infoW);
            free(info);
            return FALSE;
        }
        info->CurrentTime = infoW->CurrentTime;
        WTSFreeMemory(infoW);
        *buffer = (char *)info;
        *count = sizeof(*info);
        return TRUE;
    }

    if (!WTSQuerySessionInformationW(server, session_id, class, &bufferW, count))
        return FALSE;

    *count = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
    if (!*count)
    {
        WTSFreeMemory(bufferW);
        return FALSE;
    }

    if (!(*buffer = malloc(*count)))
    {
        WTSFreeMemory(bufferW);
        return FALSE;
    }

    if (!(*count = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, *buffer, *count, NULL, NULL)))
    {
        WTSFreeMemory(bufferW);
        free(*buffer);
        return FALSE;
    }

    WTSFreeMemory(bufferW);
    return TRUE;
}

/************************************************************
 *                WTSQuerySessionInformationW  (WTSAPI32.@)
 */
BOOL WINAPI WTSQuerySessionInformationW(HANDLE server, DWORD session_id, WTS_INFO_CLASS class, WCHAR **buffer, DWORD *count)
{
    TRACE("%p 0x%08lx %d %p %p\n", server, session_id, class, buffer, count);

    if (!buffer || !count)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (class == WTSConnectState)
    {
        WTS_CONNECTSTATE_CLASS *state;

        if (!(state = malloc(sizeof(*state)))) return FALSE;
        *state = WTSActive;
        *buffer = (WCHAR *)state;
        *count = sizeof(*state);
        return TRUE;
    }

    if (class == WTSClientProtocolType)
    {
        USHORT *protocol;

        if (!(protocol = malloc(sizeof(*protocol)))) return FALSE;
        FIXME("returning 0 protocol type\n");
        *protocol = 0;
        *buffer = (WCHAR *)protocol;
        *count = sizeof(*protocol);
        return TRUE;
    }

    if (class == WTSUserName)
    {
        DWORD size = UNLEN + 1;
        WCHAR *username;

        if (!(username = malloc(size * sizeof(WCHAR)))) return FALSE;
        GetUserNameW(username, &size);
        *buffer = username;
        *count = size * sizeof(WCHAR);
        return TRUE;
    }

    if (class ==  WTSDomainName)
    {
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        WCHAR *computername;

        if (!(computername = malloc(size * sizeof(WCHAR)))) return FALSE;
        GetComputerNameW(computername, &size);
        *buffer = computername;
        /* GetComputerNameW() return size doesn't include terminator */
        size++;
        *count = size * sizeof(WCHAR);
        return TRUE;
    }

    if (class == WTSSessionInfo)
    {
        WTSINFOW *info;
        FILETIME ft;
        DWORD size;

        if (!(info = malloc(sizeof(*info)))) return FALSE;
        FIXME("returning partial WTSINFO\n");
        memset(info, 0, sizeof(*info));
        info->State = WTSActive;
        if (!ProcessIdToSessionId(GetCurrentProcessId(), &info->SessionId))
        {
            free(info);
            return FALSE;
        }
        wcscpy(info->WinStationName, L"Console");
        size = sizeof(info->Domain) / sizeof(WCHAR);
        GetComputerNameW(info->Domain, &size);
        size = sizeof(info->UserName) / sizeof(WCHAR);
        GetUserNameW(info->UserName, &size);
        GetSystemTimeAsFileTime(&ft);
        info->CurrentTime.LowPart = ft.dwLowDateTime;
        info->CurrentTime.HighPart = ft.dwHighDateTime;
        /* TODO: fill remaining timestamps with real data */
        *buffer = (WCHAR *)info;
        *count = sizeof(*info);
        return TRUE;
    }

    FIXME("Unimplemented class %d\n", class);

    *buffer = NULL;
    *count = 0;
    return FALSE;
}

/************************************************************
 *                WTSQueryUserToken (WTSAPI32.@)
 */
BOOL WINAPI WTSQueryUserToken(ULONG session_id, PHANDLE token)
{
    FIXME("%lu %p semi-stub!\n", session_id, token);

    if (!token)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return DuplicateHandle(GetCurrentProcess(), GetCurrentProcessToken(),
                           GetCurrentProcess(), token,
                           0, FALSE, DUPLICATE_SAME_ACCESS);
}

/************************************************************
 *                WTSQueryUserConfigA (WTSAPI32.@)
 */
BOOL WINAPI WTSQueryUserConfigA(LPSTR pServerName, LPSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPSTR *ppBuffer, DWORD *pBytesReturned)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %p\n", debugstr_a(pServerName), debugstr_a(pUserName), WTSConfigClass,
        ppBuffer, pBytesReturned);
   return FALSE;
}

/************************************************************
 *                WTSQueryUserConfigW (WTSAPI32.@)
 */
BOOL WINAPI WTSQueryUserConfigW(LPWSTR pServerName, LPWSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPWSTR *ppBuffer, DWORD *pBytesReturned)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %p\n", debugstr_w(pServerName), debugstr_w(pUserName), WTSConfigClass,
        ppBuffer, pBytesReturned);
   return FALSE;
}


/************************************************************
 *                WTSRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags)
{
    FIXME("Stub %p 0x%08lx\n", hWnd, dwFlags);
    return TRUE;
}

/************************************************************
 *                WTSRegisterSessionNotificationEx (WTSAPI32.@)
 */
BOOL WINAPI WTSRegisterSessionNotificationEx(HANDLE hServer, HWND hWnd, DWORD dwFlags)
{
    FIXME("Stub %p %p 0x%08lx\n", hServer, hWnd, dwFlags);
    return TRUE;
}


/************************************************************
 *                WTSSendMessageA (WTSAPI32.@)
 */
BOOL WINAPI WTSSendMessageA(HANDLE hServer, DWORD SessionId, LPSTR pTitle, DWORD TitleLength, LPSTR pMessage,
   DWORD MessageLength, DWORD Style, DWORD Timeout, DWORD *pResponse, BOOL bWait)
{
   FIXME("Stub %p 0x%08lx (%s) %ld (%s) %ld 0x%08lx %ld %p %d\n", hServer, SessionId, debugstr_a(pTitle), TitleLength, debugstr_a(pMessage), MessageLength, Style, Timeout, pResponse, bWait);
   return FALSE;
}

/************************************************************
 *                WTSSendMessageW (WTSAPI32.@)
 */
BOOL WINAPI WTSSendMessageW(HANDLE hServer, DWORD SessionId, LPWSTR pTitle, DWORD TitleLength, LPWSTR pMessage,
   DWORD MessageLength, DWORD Style, DWORD Timeout, DWORD *pResponse, BOOL bWait)
{
   FIXME("Stub %p 0x%08lx (%s) %ld (%s) %ld 0x%08lx %ld %p %d\n", hServer, SessionId, debugstr_w(pTitle), TitleLength, debugstr_w(pMessage), MessageLength, Style, Timeout, pResponse, bWait);
   return FALSE;
}

/************************************************************
 *                WTSSetUserConfigA (WTSAPI32.@)
 */
BOOL WINAPI WTSSetUserConfigA(LPSTR pServerName, LPSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPSTR pBuffer, DWORD DataLength)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %ld\n", debugstr_a(pServerName), debugstr_a(pUserName), WTSConfigClass,pBuffer, DataLength);
   return FALSE;
}

/************************************************************
 *                WTSSetUserConfigW (WTSAPI32.@)
 */
BOOL WINAPI WTSSetUserConfigW(LPWSTR pServerName, LPWSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPWSTR pBuffer, DWORD DataLength)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %ld\n", debugstr_w(pServerName), debugstr_w(pUserName), WTSConfigClass,pBuffer, DataLength);
   return FALSE;
}

/************************************************************
 *                WTSShutdownSystem (WTSAPI32.@)
 */
BOOL WINAPI WTSShutdownSystem(HANDLE hServer, DWORD ShutdownFlag)
{
   FIXME("Stub %p 0x%08lx\n", hServer,ShutdownFlag);
   return FALSE;
}

/************************************************************
 *                WTSStartRemoteControlSessionA (WTSAPI32.@)
 */
BOOL WINAPI WTSStartRemoteControlSessionA(LPSTR pTargetServerName, ULONG TargetLogonId, BYTE HotkeyVk, USHORT HotkeyModifiers)
{
   FIXME("Stub (%s) %ld %d %d\n", debugstr_a(pTargetServerName), TargetLogonId, HotkeyVk, HotkeyModifiers);
   return FALSE;
}

/************************************************************
 *                WTSStartRemoteControlSessionW (WTSAPI32.@)
 */
BOOL WINAPI WTSStartRemoteControlSessionW(LPWSTR pTargetServerName, ULONG TargetLogonId, BYTE HotkeyVk, USHORT HotkeyModifiers)
{
   FIXME("Stub (%s) %ld %d %d\n", debugstr_w(pTargetServerName), TargetLogonId, HotkeyVk, HotkeyModifiers);
   return FALSE;
}

/************************************************************
 *                WTSStopRemoteControlSession (WTSAPI32.@)
 */
BOOL WINAPI WTSStopRemoteControlSession(ULONG LogonId)
{
   FIXME("Stub %ld\n",  LogonId);
   return FALSE;
}

/************************************************************
 *                WTSTerminateProcess (WTSAPI32.@)
 */
BOOL WINAPI WTSTerminateProcess(HANDLE hServer, DWORD ProcessId, DWORD ExitCode)
{
   FIXME("Stub %p %ld %ld\n", hServer, ProcessId, ExitCode);
   return FALSE;
}

/************************************************************
 *                WTSUnRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSUnRegisterSessionNotification(HWND hWnd)
{
    FIXME("Stub %p\n", hWnd);
    return FALSE;
}

/************************************************************
 *                WTSUnRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSUnRegisterSessionNotificationEx(HANDLE hServer, HWND hWnd)
{
    FIXME("Stub %p %p\n", hServer, hWnd);
    return FALSE;
}


/************************************************************
 *                WTSVirtualChannelClose (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelClose(HANDLE hChannelHandle)
{
   FIXME("Stub %p\n", hChannelHandle);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelOpen (WTSAPI32.@)
 */
HANDLE WINAPI WTSVirtualChannelOpen(HANDLE hServer, DWORD SessionId, LPSTR pVirtualName)
{
   FIXME("Stub %p %ld (%s)\n", hServer, SessionId, debugstr_a(pVirtualName));
   return NULL;
}

/************************************************************
 *                WTSVirtualChannelOpen (WTSAPI32.@)
 */
HANDLE WINAPI WTSVirtualChannelOpenEx(DWORD SessionId, LPSTR pVirtualName, DWORD flags)
{
   FIXME("Stub %ld (%s) %ld\n",  SessionId, debugstr_a(pVirtualName), flags);
   return NULL;
}

/************************************************************
 *                WTSVirtualChannelPurgeInput (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelPurgeInput(HANDLE hChannelHandle)
{
   FIXME("Stub %p\n", hChannelHandle);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelPurgeOutput (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelPurgeOutput(HANDLE hChannelHandle)
{
   FIXME("Stub %p\n", hChannelHandle);
   return FALSE;
}


/************************************************************
 *                WTSVirtualChannelQuery (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelQuery(HANDLE hChannelHandle, WTS_VIRTUAL_CLASS WtsVirtualClass, PVOID *ppBuffer, DWORD *pBytesReturned)
{
   FIXME("Stub %p %d %p %p\n", hChannelHandle, WtsVirtualClass, ppBuffer, pBytesReturned);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelRead (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelRead(HANDLE hChannelHandle, ULONG TimeOut, PCHAR Buffer, ULONG BufferSize, PULONG pBytesRead)
{
   FIXME("Stub %p %ld %p %ld %p\n", hChannelHandle, TimeOut, Buffer, BufferSize, pBytesRead);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelWrite (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelWrite(HANDLE hChannelHandle, PCHAR Buffer, ULONG Length, PULONG pBytesWritten)
{
   FIXME("Stub %p %p %ld %p\n", hChannelHandle, Buffer, Length, pBytesWritten);
   return FALSE;
}

/************************************************************
 *                WTSWaitSystemEvent (WTSAPI32.@)
 */
BOOL WINAPI WTSWaitSystemEvent(HANDLE hServer, DWORD Mask, DWORD* Flags)
{
    /* FIXME: Forward request to winsta.dll::WinStationWaitSystemEvent */
    FIXME("Stub %p 0x%08lx %p\n", hServer, Mask, Flags);
    return FALSE;
}

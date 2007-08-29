/* Copyright (C) 2004 Juan Lang
 * Copyright (C) 2007 Kai Blin
 *
 * Local Security Authority functions, as far as secur32 has them.
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsa);

NTSTATUS WINAPI LsaCallAuthenticationPackage(HANDLE LsaHandle,
        ULONG AuthenticationPackage, PVOID ProtocolSubmitBuffer,
        ULONG SubmitBufferLength, PVOID* ProtocolReturnBuffer,
        PULONG ReturnBufferLength, PNTSTATUS ProtocolStatus)
{
    FIXME("%p %d %p %d %p %p %p stub\n", LsaHandle, AuthenticationPackage,
          ProtocolSubmitBuffer, SubmitBufferLength, ProtocolReturnBuffer,
          ReturnBufferLength, ProtocolStatus);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaConnectUntrusted(PHANDLE LsaHandle)
{
    FIXME("%p stub\n", LsaHandle);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaDeregisterLogonProcess(HANDLE LsaHandle)
{
    FIXME("%p stub\n", LsaHandle);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaEnumerateLogonSessions(PULONG LogonSessionCount,
        PLUID* LogonSessionList)
{
    FIXME("%p %p stub\n", LogonSessionCount, LogonSessionList);
    *LogonSessionCount = 0;
    *LogonSessionList = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaFreeReturnBuffer(PVOID Buffer)
{
    FIXME("%p stub\n", Buffer);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaLookupAuthenticationPackage(HANDLE LsaHandle,
        PLSA_STRING PackageName, PULONG AuthenticationPackage)
{
    FIXME("%p %p %p stub\n", LsaHandle, PackageName, AuthenticationPackage);
    AuthenticationPackage = NULL;
    return STATUS_SUCCESS;
}

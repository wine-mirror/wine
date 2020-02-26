/*
 * Process environment management
 *
 * Copyright 1996, 1998 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/library.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

/* Notes:
 * - contrary to Microsoft docs, the environment strings do not appear
 *   to be sorted on Win95 (although they are on NT); so we don't bother
 *   to sort them either.
 */

static STARTUPINFOA startup_infoA;

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    *info = startup_infoA;
}

/******************************************************************
 *		ENV_CopyStartupInformation (internal)
 *
 * Creates the STARTUPINFO information from the ntdll information
 */
void ENV_CopyStartupInformation(void)
{
    RTL_USER_PROCESS_PARAMETERS* rupp;
    ANSI_STRING         ansi;

    RtlAcquirePebLock();
    
    rupp = NtCurrentTeb()->Peb->ProcessParameters;

    startup_infoA.cb                   = sizeof(startup_infoA);
    startup_infoA.lpReserved           = NULL;
    startup_infoA.lpDesktop = RtlUnicodeStringToAnsiString( &ansi, &rupp->Desktop, TRUE ) == STATUS_SUCCESS ?
        ansi.Buffer : NULL;
    startup_infoA.lpTitle = RtlUnicodeStringToAnsiString( &ansi, &rupp->WindowTitle, TRUE ) == STATUS_SUCCESS ?
        ansi.Buffer : NULL;
    startup_infoA.dwX                  = rupp->dwX;
    startup_infoA.dwY                  = rupp->dwY;
    startup_infoA.dwXSize              = rupp->dwXSize;
    startup_infoA.dwYSize              = rupp->dwYSize;
    startup_infoA.dwXCountChars        = rupp->dwXCountChars;
    startup_infoA.dwYCountChars        = rupp->dwYCountChars;
    startup_infoA.dwFillAttribute      = rupp->dwFillAttribute;
    startup_infoA.dwFlags              = rupp->dwFlags;
    startup_infoA.wShowWindow          = rupp->wShowWindow;
    startup_infoA.cbReserved2          = rupp->RuntimeInfo.MaximumLength;
    startup_infoA.lpReserved2          = rupp->RuntimeInfo.MaximumLength ? (void*)rupp->RuntimeInfo.Buffer : NULL;
    startup_infoA.hStdInput            = rupp->hStdInput ? rupp->hStdInput : INVALID_HANDLE_VALUE;
    startup_infoA.hStdOutput           = rupp->hStdOutput ? rupp->hStdOutput : INVALID_HANDLE_VALUE;
    startup_infoA.hStdError            = rupp->hStdError ? rupp->hStdError : INVALID_HANDLE_VALUE;

    RtlReleasePebLock();
}

/***********************************************************************
 *              GetFirmwareEnvironmentVariableA         (KERNEL32.@)
 */
DWORD WINAPI GetFirmwareEnvironmentVariableA(LPCSTR name, LPCSTR guid, PVOID buffer, DWORD size)
{
    FIXME("stub: %s %s %p %u\n", debugstr_a(name), debugstr_a(guid), buffer, size);
    SetLastError(ERROR_INVALID_FUNCTION);
    return 0;
}

/***********************************************************************
 *              GetFirmwareEnvironmentVariableW         (KERNEL32.@)
 */
DWORD WINAPI GetFirmwareEnvironmentVariableW(LPCWSTR name, LPCWSTR guid, PVOID buffer, DWORD size)
{
    FIXME("stub: %s %s %p %u\n", debugstr_w(name), debugstr_w(guid), buffer, size);
    SetLastError(ERROR_INVALID_FUNCTION);
    return 0;
}

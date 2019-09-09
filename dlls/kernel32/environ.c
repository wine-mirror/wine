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

static STARTUPINFOW startup_infoW;
static STARTUPINFOA startup_infoA;


/***********************************************************************
 *           GetCommandLineA      (KERNEL32.@)
 *
 * WARNING: there's a Windows incompatibility lurking here !
 * Win32s always includes the full path of the program file,
 * whereas Windows NT only returns the full file path plus arguments
 * in case the program has been started with a full path.
 * Win9x seems to have inherited NT behaviour.
 *
 * Note that both Start Menu Execute and Explorer start programs with
 * fully specified quoted app file paths, which is why probably the only case
 * where you'll see single file names is in case of direct launch
 * via CreateProcess or WinExec.
 *
 * Perhaps we should take care of Win3.1 programs here (Win32s "feature").
 *
 * References: MS KB article q102762.txt (special Win32s handling)
 */
LPSTR WINAPI GetCommandLineA(void)
{
    static char *cmdlineA;  /* ASCII command line */
    
    if (!cmdlineA) /* make an ansi version if we don't have it */
    {
        ANSI_STRING     ansi;
        RtlAcquirePebLock();

        cmdlineA = (RtlUnicodeStringToAnsiString( &ansi, &NtCurrentTeb()->Peb->ProcessParameters->CommandLine, TRUE) == STATUS_SUCCESS) ?
            ansi.Buffer : NULL;
        RtlReleasePebLock();
    }
    return cmdlineA;
}

/***********************************************************************
 *           GetCommandLineW      (KERNEL32.@)
 */
LPWSTR WINAPI GetCommandLineW(void)
{
    return NtCurrentTeb()->Peb->ProcessParameters->CommandLine.Buffer;
}


/***********************************************************************
 *           GetStdHandle    (KERNEL32.@)
 */
HANDLE WINAPI GetStdHandle( DWORD std_handle )
{
    switch (std_handle)
    {
        case STD_INPUT_HANDLE:  return NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
        case STD_OUTPUT_HANDLE: return NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
        case STD_ERROR_HANDLE:  return NtCurrentTeb()->Peb->ProcessParameters->hStdError;
    }
    SetLastError( ERROR_INVALID_HANDLE );
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           SetStdHandle    (KERNEL32.@)
 */
BOOL WINAPI SetStdHandle( DWORD std_handle, HANDLE handle )
{
    switch (std_handle)
    {
        case STD_INPUT_HANDLE:  NtCurrentTeb()->Peb->ProcessParameters->hStdInput = handle;  return TRUE;
        case STD_OUTPUT_HANDLE: NtCurrentTeb()->Peb->ProcessParameters->hStdOutput = handle; return TRUE;
        case STD_ERROR_HANDLE:  NtCurrentTeb()->Peb->ProcessParameters->hStdError = handle;  return TRUE;
    }
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    *info = startup_infoA;
}


/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoW( LPSTARTUPINFOW info )
{
    *info = startup_infoW;
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

    startup_infoW.cb                   = sizeof(startup_infoW);
    startup_infoW.lpReserved           = NULL;
    startup_infoW.lpDesktop            = rupp->Desktop.Buffer;
    startup_infoW.lpTitle              = rupp->WindowTitle.Buffer;
    startup_infoW.dwX                  = rupp->dwX;
    startup_infoW.dwY                  = rupp->dwY;
    startup_infoW.dwXSize              = rupp->dwXSize;
    startup_infoW.dwYSize              = rupp->dwYSize;
    startup_infoW.dwXCountChars        = rupp->dwXCountChars;
    startup_infoW.dwYCountChars        = rupp->dwYCountChars;
    startup_infoW.dwFillAttribute      = rupp->dwFillAttribute;
    startup_infoW.dwFlags              = rupp->dwFlags;
    startup_infoW.wShowWindow          = rupp->wShowWindow;
    startup_infoW.cbReserved2          = rupp->RuntimeInfo.MaximumLength;
    startup_infoW.lpReserved2          = rupp->RuntimeInfo.MaximumLength ? (void*)rupp->RuntimeInfo.Buffer : NULL;
    startup_infoW.hStdInput            = rupp->hStdInput ? rupp->hStdInput : INVALID_HANDLE_VALUE;
    startup_infoW.hStdOutput           = rupp->hStdOutput ? rupp->hStdOutput : INVALID_HANDLE_VALUE;
    startup_infoW.hStdError            = rupp->hStdError ? rupp->hStdError : INVALID_HANDLE_VALUE;

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

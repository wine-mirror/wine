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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "thread.h"
#include "wine/winbase16.h"
#include "wine/server.h"
#include "wine/library.h"
#include "heap.h"
#include "winternl.h"
#include "selectors.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

/* TODO:
 * - 16 bit environment ??? (see generate_env_block16 for the details)
 */

/* Format of an environment block:
 * ASCIIZ   string 1 (xx=yy format)
 * ...
 * ASCIIZ   string n
 * BYTE     0
 * WORD     1
 * ASCIIZ   program name (e.g. C:\WINDOWS\SYSTEM\KRNL386.EXE)
 *
 * Notes:
 * - contrary to Microsoft docs, the environment strings do not appear
 *   to be sorted on Win95 (although they are on NT); so we don't bother
 *   to sort them either.
 * - on Win2K (and likely most of NT versions) the last part (WORD 1 and 
 *   program name no longer appear in the environment block (from the 32
 *   bit interface)
 */

static const char ENV_program_name[] = "C:\\WINDOWS\\SYSTEM\\KRNL386.EXE";

static STARTUPINFOW startup_infoW;
static STARTUPINFOA startup_infoA;

/* Maximum length of a Win16 environment string (including NULL) */
#define MAX_WIN16_LEN  128

static WORD env_sel;     /* selector to the 16 bit environment */

/******************************************************************
 *		generate_env_block16
 *
 * This internal function generates a suitable environment for the 16 bit
 * subsystem and returns the value as a segmented pointer.
 *
 * FIXME: current implementation will allocate a private copy of the
 *        environment strings, but will not follow the modifications (if any)
 *        from the unicode env block stored in the PEB
 *              => how should this be updated from the ntdll modifications ?
 *                 should we use the peb->EnvironmentUpdateCount field to 
 *                 know when environment has changed ???
 *        we currently regenerate this block each time an environment 
 *        variable is modified from a Win32 API call, but we'll miss all
 *        direct access to the NTDLL APIs
 */
static SEGPTR generate_env_block16(void)
{
    static LPSTR env16;

    DWORD       size, new_size;
    WORD        one = 1;

    if (env16) FreeEnvironmentStringsA( env16 );

    env16 = GetEnvironmentStringsA();
    size = HeapSize(GetProcessHeap(), 0, env16);
    new_size = size + sizeof(WORD) + sizeof(ENV_program_name);
    if (!(env16 = HeapReAlloc( GetProcessHeap(), 0, env16, new_size ))) return 0;

    memcpy(env16 + size, &one, sizeof(one));
    memcpy(env16 + size + sizeof(WORD), ENV_program_name, sizeof(ENV_program_name));
    if (env_sel)
        env_sel = SELECTOR_ReallocBlock( env_sel, env16, new_size );
    else
        env_sel = SELECTOR_AllocBlock( env16, 0x10000, WINE_LDT_FLAGS_DATA );

    return MAKESEGPTR( env_sel, 0 );
}

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
 *           GetEnvironmentStrings    (KERNEL32.@)
 *           GetEnvironmentStringsA   (KERNEL32.@)
 */
LPSTR WINAPI GetEnvironmentStringsA(void)
{
    LPWSTR      ptrW;
    unsigned    len, slen;
    LPSTR       ret, ptrA;

    RtlAcquirePebLock();

    len = 1;

    ptrW = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    while (*ptrW)
    {
        slen = strlenW(ptrW) + 1;
        len += WideCharToMultiByte( CP_ACP, 0, ptrW, slen, NULL, 0, NULL, NULL );
        ptrW += slen;
    }

    if ((ret = HeapAlloc( GetProcessHeap(), 0, len )) != NULL)
    {
        ptrW = NtCurrentTeb()->Peb->ProcessParameters->Environment;
        ptrA = ret;
        while (*ptrW)
        {
            slen = strlenW(ptrW) + 1;
            WideCharToMultiByte( CP_ACP, 0, ptrW, slen, ptrA, len, NULL, NULL );
            ptrW += slen;
            ptrA += strlen(ptrA) + 1;
        }
        *ptrA = 0;
    }

    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           GetEnvironmentStringsW   (KERNEL32.@)
 */
LPWSTR WINAPI GetEnvironmentStringsW(void)
{
    return NtCurrentTeb()->Peb->ProcessParameters->Environment;
}


/***********************************************************************
 *           FreeEnvironmentStringsA   (KERNEL32.@)
 */
BOOL WINAPI FreeEnvironmentStringsA( LPSTR ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           FreeEnvironmentStringsW   (KERNEL32.@)
 */
BOOL WINAPI FreeEnvironmentStringsW( LPWSTR ptr )
{
    return TRUE;
}


/***********************************************************************
 *           GetEnvironmentVariableA   (KERNEL32.@)
 */
DWORD WINAPI GetEnvironmentVariableA( LPCSTR name, LPSTR value, DWORD size )
{
    UNICODE_STRING      us_name;
    PWSTR               valueW;
    DWORD               ret;

    if (!name || !*name)
    {
        SetLastError(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }

    if (!(valueW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR))))
        return 0;

    RtlCreateUnicodeStringFromAsciiz( &us_name, name );
    SetLastError(0);
    ret = GetEnvironmentVariableW( us_name.Buffer, valueW, size);
    if (ret && ret < size)
    {
        WideCharToMultiByte( CP_ACP, 0, valueW, ret + 1, value, size, NULL, NULL );
    }
    /* this is needed to tell, with 0 as a return value, the difference between:
     * - an error (GetLastError() != 0)
     * - returning an empty string (in this case, we need to update the buffer)
     */
    if (ret == 0 && size && GetLastError() == 0)
        value[0] = '\0';

    RtlFreeUnicodeString( &us_name );
    HeapFree(GetProcessHeap(), 0, valueW);

    return ret;
}


/***********************************************************************
 *           GetEnvironmentVariableW   (KERNEL32.@)
 */
DWORD WINAPI GetEnvironmentVariableW( LPCWSTR name, LPWSTR val, DWORD size )
{
    UNICODE_STRING      us_name;
    UNICODE_STRING      us_value;
    NTSTATUS            status;
    unsigned            len;

    TRACE("(%s %p %lu)\n", debugstr_w(name), val, size);

    if (!name || !*name)
    {
        SetLastError(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }

    RtlInitUnicodeString(&us_name, name);
    us_value.Length = 0;
    us_value.MaximumLength = (size ? size - 1 : 0) * sizeof(WCHAR);
    us_value.Buffer = val;

    status = RtlQueryEnvironmentVariable_U(NULL, &us_name, &us_value);
    len = us_value.Length / sizeof(WCHAR);
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return (status == STATUS_BUFFER_TOO_SMALL) ? len + 1 : 0;
    }
    if (size) val[len] = '\0';

    return us_value.Length / sizeof(WCHAR);
}


/***********************************************************************
 *           SetEnvironmentVariableA   (KERNEL32.@)
 */
BOOL WINAPI SetEnvironmentVariableA( LPCSTR name, LPCSTR value )
{
    UNICODE_STRING      us_name;
    BOOL                ret;

    if (!name)
    {
        SetLastError(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }

    RtlCreateUnicodeStringFromAsciiz( &us_name, name );
    if (value)
    {
        UNICODE_STRING      us_value;

        RtlCreateUnicodeStringFromAsciiz( &us_value, value );
        ret = SetEnvironmentVariableW( us_name.Buffer, us_value.Buffer );
        RtlFreeUnicodeString( &us_value );
    }
    else ret = SetEnvironmentVariableW( us_name.Buffer, NULL );

    RtlFreeUnicodeString( &us_name );

    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariableW   (KERNEL32.@)
 */
BOOL WINAPI SetEnvironmentVariableW( LPCWSTR name, LPCWSTR value )
{
    UNICODE_STRING      us_name;
    NTSTATUS            status;

    TRACE("(%s %s)\n", debugstr_w(name), debugstr_w(value));

    if (!name)
    {
        SetLastError(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }

    RtlInitUnicodeString(&us_name, name);
    if (value)
    {
        UNICODE_STRING      us_value;

        RtlInitUnicodeString(&us_value, value);
        status = RtlSetEnvironmentVariable(NULL, &us_name, &us_value);
    }
    else status = RtlSetEnvironmentVariable(NULL, &us_name, NULL);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    /* FIXME: see comments in generate_env_block16 */
    if (env_sel) generate_env_block16();
    return TRUE;
}


/***********************************************************************
 *           ExpandEnvironmentStringsA   (KERNEL32.@)
 *
 * Note: overlapping buffers are not supported; this is how it should be.
 * FIXME: return value is wrong for MBCS
 */
DWORD WINAPI ExpandEnvironmentStringsA( LPCSTR src, LPSTR dst, DWORD count )
{
    UNICODE_STRING      us_src;
    PWSTR               dstW = NULL;
    DWORD               ret;

    RtlCreateUnicodeStringFromAsciiz( &us_src, src );
    if (count)
    {
        if (!(dstW = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR))))
            return 0;
        ret = ExpandEnvironmentStringsW( us_src.Buffer, dstW, count);
        if (ret)
            WideCharToMultiByte( CP_ACP, 0, dstW, ret, dst, count, NULL, NULL );
    }
    else ret = ExpandEnvironmentStringsW( us_src.Buffer, NULL, 0);

    RtlFreeUnicodeString( &us_src );
    if (dstW) HeapFree(GetProcessHeap(), 0, dstW);

    return ret;
}


/***********************************************************************
 *           ExpandEnvironmentStringsW   (KERNEL32.@)
 */
DWORD WINAPI ExpandEnvironmentStringsW( LPCWSTR src, LPWSTR dst, DWORD len )
{
    UNICODE_STRING      us_src;
    UNICODE_STRING      us_dst;
    NTSTATUS            status;
    DWORD               res;

    TRACE("(%s %p %lu)\n", debugstr_w(src), dst, len);

    RtlInitUnicodeString(&us_src, src);
    us_dst.Length = 0;
    us_dst.MaximumLength = len * sizeof(WCHAR);
    us_dst.Buffer = dst;

    res = 0;
    status = RtlExpandEnvironmentStrings_U(NULL, &us_src, &us_dst, &res);
    res /= sizeof(WCHAR);
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        if (status != STATUS_BUFFER_TOO_SMALL) return 0;
        if (len && dst) dst[len - 1] = '\0';
    }

    return res;
}


/***********************************************************************
 *           GetDOSEnvironment     (KERNEL.131)
 */
SEGPTR WINAPI GetDOSEnvironment16(void)
{
    return generate_env_block16();
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
    SetLastError( ERROR_INVALID_PARAMETER );
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
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    assert(startup_infoA.cb);
    memcpy(info, &startup_infoA, sizeof(startup_infoA));
}


/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoW( LPSTARTUPINFOW info )
{
    assert(startup_infoW.cb);
    memcpy(info, &startup_infoW, sizeof(startup_infoW));
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
    startup_infoW.cbReserved2          = 0;
    startup_infoW.lpReserved2          = NULL;
    startup_infoW.hStdInput            = rupp->hStdInput;
    startup_infoW.hStdOutput           = rupp->hStdOutput;
    startup_infoW.hStdError            = rupp->hStdError;

    startup_infoA.cb                   = sizeof(startup_infoW);
    startup_infoA.lpReserved           = NULL;
    startup_infoA.lpDesktop = (rupp->Desktop.Length &&
                               RtlUnicodeStringToAnsiString( &ansi, &rupp->Desktop, TRUE) == STATUS_SUCCESS) ?
        ansi.Buffer : NULL;
    startup_infoA.lpTitle = (rupp->WindowTitle.Length &&
                             RtlUnicodeStringToAnsiString( &ansi, &rupp->WindowTitle, TRUE) == STATUS_SUCCESS) ?
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
    startup_infoA.cbReserved2          = 0;
    startup_infoA.lpReserved2          = NULL;
    startup_infoA.hStdInput            = rupp->hStdInput;
    startup_infoA.hStdOutput           = rupp->hStdOutput;
    startup_infoA.hStdError            = rupp->hStdError;

    RtlReleasePebLock();
}

/*
 * Win32 processes
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

#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"

#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

static DWORD shutdown_flags = 0;
static DWORD shutdown_priority = 0x280;

/***********************************************************************
 * Processes
 ***********************************************************************/


/*********************************************************************
 *           CloseHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CloseHandle( HANDLE handle )
{
    if (handle == (HANDLE)STD_INPUT_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdInput, 0 );
    else if (handle == (HANDLE)STD_OUTPUT_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdOutput, 0 );
    else if (handle == (HANDLE)STD_ERROR_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdError, 0 );

    if (is_console_handle( handle )) handle = console_handle_map( handle );
    return set_ntstatus( NtClose( handle ));
}


/*********************************************************************
 *           DuplicateHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DuplicateHandle( HANDLE source_process, HANDLE source,
                                               HANDLE dest_process, HANDLE *dest,
                                               DWORD access, BOOL inherit, DWORD options )
{
    if (is_console_handle( source ))
    {
        source = console_handle_map( source );
        if (!set_ntstatus( NtDuplicateObject( source_process, source, dest_process, dest,
                                              access, inherit ? OBJ_INHERIT : 0, options )))
            return FALSE;
        *dest = console_handle_map( *dest );
        return TRUE;
    }
    return set_ntstatus( NtDuplicateObject( source_process, source, dest_process, dest,
                                            access, inherit ? OBJ_INHERIT : 0, options ));
}


/****************************************************************************
 *           FlushInstructionCache   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushInstructionCache( HANDLE process, LPCVOID addr, SIZE_T size )
{
    return set_ntstatus( NtFlushInstructionCache( process, addr, size ));
}


/***********************************************************************
 *           GetCurrentProcess   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}


/***********************************************************************
 *           GetCurrentProcessId   (kernelbase.@)
 */
DWORD WINAPI kernelbase_GetCurrentProcessId(void)
{
    return HandleToULong( NtCurrentTeb()->ClientId.UniqueProcess );
}


/***********************************************************************
 *           GetErrorMode   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetErrorMode(void)
{
    UINT mode;

    NtQueryInformationProcess( GetCurrentProcess(), ProcessDefaultHardErrorMode,
                               &mode, sizeof(mode), NULL );
    return mode;
}


/***********************************************************************
 *           GetExitCodeProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetExitCodeProcess( HANDLE process, LPDWORD exit_code )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;

    status = NtQueryInformationProcess( process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    if (status && exit_code) *exit_code = pbi.ExitStatus;
    return set_ntstatus( status );
}


/*********************************************************************
 *           GetHandleInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetHandleInformation( HANDLE handle, DWORD *flags )
{
    OBJECT_DATA_INFORMATION info;

    if (!set_ntstatus( NtQueryObject( handle, ObjectDataInformation, &info, sizeof(info), NULL )))
        return FALSE;

    if (flags)
    {
        *flags = 0;
        if (info.InheritHandle) *flags |= HANDLE_FLAG_INHERIT;
        if (info.ProtectFromClose) *flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
    }
    return TRUE;
}


/***********************************************************************
 *           GetPriorityClass   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetPriorityClass( HANDLE process )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessBasicInformation,
                                                  &pbi, sizeof(pbi), NULL )))
        return 0;

    switch (pbi.BasePriority)
    {
    case PROCESS_PRIOCLASS_IDLE: return IDLE_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_BELOW_NORMAL: return BELOW_NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_NORMAL: return NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_ABOVE_NORMAL: return ABOVE_NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_HIGH: return HIGH_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_REALTIME: return REALTIME_PRIORITY_CLASS;
    default: return 0;
    }
}


/******************************************************************
 *           GetProcessHandleCount   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessHandleCount( HANDLE process, DWORD *count )
{
    return set_ntstatus( NtQueryInformationProcess( process, ProcessHandleCount,
                                                    count, sizeof(*count), NULL ));
}


/***********************************************************************
 *           GetProcessHeap   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}


/*********************************************************************
 *           GetProcessId   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessId( HANDLE process )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessBasicInformation,
                                                  &pbi, sizeof(pbi), NULL )))
        return 0;
    return pbi.UniqueProcessId;
}


/**********************************************************************
 *           GetProcessMitigationPolicy   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ GetProcessMitigationPolicy( HANDLE process, PROCESS_MITIGATION_POLICY policy,
                                                          void *buffer, SIZE_T length )
{
    FIXME( "(%p, %u, %p, %lu): stub\n", process, policy, buffer, length );
    return TRUE;
}


/***********************************************************************
 *           GetProcessPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessPriorityBoost( HANDLE process, PBOOL disable )
{
    FIXME( "(%p,%p): semi-stub\n", process, disable );
    *disable = FALSE;  /* report that no boost is present */
    return TRUE;
}


/***********************************************************************
 *           GetProcessShutdownParameters   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessShutdownParameters( LPDWORD level, LPDWORD flags )
{
    *level = shutdown_priority;
    *flags = shutdown_flags;
    return TRUE;
}


/***********************************************************************
 *           GetProcessWorkingSetSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessWorkingSetSizeEx( HANDLE process, SIZE_T *minset,
                                                          SIZE_T *maxset, DWORD *flags)
{
    FIXME( "(%p,%p,%p,%p): stub\n", process, minset, maxset, flags );
    /* 32 MB working set size */
    if (minset) *minset = 32*1024*1024;
    if (maxset) *maxset = 32*1024*1024;
    if (flags) *flags = QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;
    return TRUE;
}


/******************************************************************************
 *           IsProcessInJob   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsProcessInJob( HANDLE process, HANDLE job, BOOL *result )
{
    NTSTATUS status = NtIsProcessInJob( process, job );

    switch (status)
    {
    case STATUS_PROCESS_IN_JOB:
        *result = TRUE;
        return TRUE;
    case STATUS_PROCESS_NOT_IN_JOB:
        *result = FALSE;
        return TRUE;
    default:
        return set_ntstatus( status );
    }
}


/***********************************************************************
 *           IsProcessorFeaturePresent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsProcessorFeaturePresent ( DWORD feature )
{
    return RtlIsProcessorFeaturePresent( feature );
}


/**********************************************************************
 *           IsWow64Process   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsWow64Process( HANDLE process, PBOOL wow64 )
{
    ULONG_PTR pbi;
    NTSTATUS status;

    status = NtQueryInformationProcess( process, ProcessWow64Information, &pbi, sizeof(pbi), NULL );
    if (!status) *wow64 = !!pbi;
    return set_ntstatus( status );
}


/*********************************************************************
 *           OpenProcess   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;

    if (GetVersion() & 0x80000000) access = PROCESS_ALL_ACCESS;

    attr.Length = sizeof(OBJECT_ATTRIBUTES);
    attr.RootDirectory = 0;
    attr.Attributes = inherit ? OBJ_INHERIT : 0;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    cid.UniqueProcess = ULongToHandle(id);
    cid.UniqueThread  = 0;

    if (!set_ntstatus( NtOpenProcess( &handle, access, &attr, &cid ))) return NULL;
    return handle;
}


/***********************************************************************
 *           SetErrorMode   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH SetErrorMode( UINT mode )
{
    UINT old = GetErrorMode();

    NtSetInformationProcess( GetCurrentProcess(), ProcessDefaultHardErrorMode,
                             &mode, sizeof(mode) );
    return old;
}


/*************************************************************************
 *           SetHandleCount   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH SetHandleCount( UINT count )
{
    return count;
}


/*********************************************************************
 *           SetHandleInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    OBJECT_DATA_INFORMATION info;

    /* if not setting both fields, retrieve current value first */
    if ((mask & (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE)) !=
        (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE))
    {
        if (!set_ntstatus( NtQueryObject( handle, ObjectDataInformation, &info, sizeof(info), NULL )))
            return FALSE;
    }
    if (mask & HANDLE_FLAG_INHERIT)
        info.InheritHandle = (flags & HANDLE_FLAG_INHERIT) != 0;
    if (mask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
        info.ProtectFromClose = (flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;

    return set_ntstatus( NtSetInformationObject( handle, ObjectDataInformation, &info, sizeof(info) ));
}


/***********************************************************************
 *           SetPriorityClass   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetPriorityClass( HANDLE process, DWORD class )
{
    PROCESS_PRIORITY_CLASS ppc;

    ppc.Foreground = FALSE;
    switch (class)
    {
    case IDLE_PRIORITY_CLASS:         ppc.PriorityClass = PROCESS_PRIOCLASS_IDLE; break;
    case BELOW_NORMAL_PRIORITY_CLASS: ppc.PriorityClass = PROCESS_PRIOCLASS_BELOW_NORMAL; break;
    case NORMAL_PRIORITY_CLASS:       ppc.PriorityClass = PROCESS_PRIOCLASS_NORMAL; break;
    case ABOVE_NORMAL_PRIORITY_CLASS: ppc.PriorityClass = PROCESS_PRIOCLASS_ABOVE_NORMAL; break;
    case HIGH_PRIORITY_CLASS:         ppc.PriorityClass = PROCESS_PRIOCLASS_HIGH; break;
    case REALTIME_PRIORITY_CLASS:     ppc.PriorityClass = PROCESS_PRIOCLASS_REALTIME; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( NtSetInformationProcess( process, ProcessPriorityClass, &ppc, sizeof(ppc) ));
}


/***********************************************************************
 *           SetProcessAffinityUpdateMode   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessAffinityUpdateMode( HANDLE process, DWORD flags )
{
    FIXME( "(%p,0x%08x): stub\n", process, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/**********************************************************************
 *           SetProcessMitigationPolicy   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ SetProcessMitigationPolicy( PROCESS_MITIGATION_POLICY policy,
                                                          void *buffer, SIZE_T length )
{
    FIXME( "(%d, %p, %lu): stub\n", policy, buffer, length );
    return TRUE;
}


/***********************************************************************
 *           SetProcessPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ SetProcessPriorityBoost( HANDLE process, BOOL disable )
{
    FIXME( "(%p,%d): stub\n", process, disable );
    return TRUE;
}


/***********************************************************************
 *           SetProcessShutdownParameters   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessShutdownParameters( DWORD level, DWORD flags )
{
    FIXME( "(%08x, %08x): partial stub.\n", level, flags );
    shutdown_flags = flags;
    shutdown_priority = level;
    return TRUE;
}


/***********************************************************************
 *           SetProcessWorkingSetSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessWorkingSetSizeEx( HANDLE process, SIZE_T minset,
                                                          SIZE_T maxset, DWORD flags )
{
    return TRUE;
}


/******************************************************************************
 *           TerminateProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TerminateProcess( HANDLE handle, DWORD exit_code )
{
    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return set_ntstatus( NtTerminateProcess( handle, exit_code ));
}


/***********************************************************************
 * Process startup information
 ***********************************************************************/


static STARTUPINFOW startup_infoW;
static char *command_lineA;
static WCHAR *command_lineW;

/******************************************************************
 *		init_startup_info
 */
void init_startup_info( RTL_USER_PROCESS_PARAMETERS *params )
{
    ANSI_STRING ansi;

    startup_infoW.cb              = sizeof(startup_infoW);
    startup_infoW.lpReserved      = NULL;
    startup_infoW.lpDesktop       = params->Desktop.Buffer;
    startup_infoW.lpTitle         = params->WindowTitle.Buffer;
    startup_infoW.dwX             = params->dwX;
    startup_infoW.dwY             = params->dwY;
    startup_infoW.dwXSize         = params->dwXSize;
    startup_infoW.dwYSize         = params->dwYSize;
    startup_infoW.dwXCountChars   = params->dwXCountChars;
    startup_infoW.dwYCountChars   = params->dwYCountChars;
    startup_infoW.dwFillAttribute = params->dwFillAttribute;
    startup_infoW.dwFlags         = params->dwFlags;
    startup_infoW.wShowWindow     = params->wShowWindow;
    startup_infoW.cbReserved2     = params->RuntimeInfo.MaximumLength;
    startup_infoW.lpReserved2     = params->RuntimeInfo.MaximumLength ? (void *)params->RuntimeInfo.Buffer : NULL;
    startup_infoW.hStdInput       = params->hStdInput ? params->hStdInput : INVALID_HANDLE_VALUE;
    startup_infoW.hStdOutput      = params->hStdOutput ? params->hStdOutput : INVALID_HANDLE_VALUE;
    startup_infoW.hStdError       = params->hStdError ? params->hStdError : INVALID_HANDLE_VALUE;

    command_lineW = params->CommandLine.Buffer;
    if (!RtlUnicodeStringToAnsiString( &ansi, &params->CommandLine, TRUE )) command_lineA = ansi.Buffer;
}


/***********************************************************************
 *           GetCommandLineA   (kernelbase.@)
 */
LPSTR WINAPI DECLSPEC_HOTPATCH GetCommandLineA(void)
{
    return command_lineA;
}


/***********************************************************************
 *           GetCommandLineW   (kernelbase.@)
 */
LPWSTR WINAPI DECLSPEC_HOTPATCH GetCommandLineW(void)
{
    return NtCurrentTeb()->Peb->ProcessParameters->CommandLine.Buffer;
}


/***********************************************************************
 *           GetStartupInfoW    (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetStartupInfoW( STARTUPINFOW *info )
{
    *info = startup_infoW;
}


/***********************************************************************
 *           GetStdHandle    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH GetStdHandle( DWORD std_handle )
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
 *           SetStdHandle    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetStdHandle( DWORD std_handle, HANDLE handle )
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
 *           SetStdHandleEx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetStdHandleEx( DWORD std_handle, HANDLE handle, HANDLE *prev )
{
    HANDLE *ptr;

    switch (std_handle)
    {
    case STD_INPUT_HANDLE:  ptr = &NtCurrentTeb()->Peb->ProcessParameters->hStdInput;  break;
    case STD_OUTPUT_HANDLE: ptr = &NtCurrentTeb()->Peb->ProcessParameters->hStdOutput; break;
    case STD_ERROR_HANDLE:  ptr = &NtCurrentTeb()->Peb->ProcessParameters->hStdError;  break;
    default:
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (prev) *prev = *ptr;
    *ptr = handle;
    return TRUE;
}


/***********************************************************************
 * Process environment
 ***********************************************************************/


static inline SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += lstrlenW(end) + 1;
    return end + 1 - env;
}

/***********************************************************************
 *           ExpandEnvironmentStringsA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH ExpandEnvironmentStringsA( LPCSTR src, LPSTR dst, DWORD count )
{
    UNICODE_STRING us_src;
    PWSTR dstW = NULL;
    DWORD ret;

    RtlCreateUnicodeStringFromAsciiz( &us_src, src );
    if (count)
    {
        if (!(dstW = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR)))) return 0;
        ret = ExpandEnvironmentStringsW( us_src.Buffer, dstW, count);
        if (ret) WideCharToMultiByte( CP_ACP, 0, dstW, ret, dst, count, NULL, NULL );
    }
    else ret = ExpandEnvironmentStringsW( us_src.Buffer, NULL, 0 );

    RtlFreeUnicodeString( &us_src );
    HeapFree( GetProcessHeap(), 0, dstW );
    return ret;
}


/***********************************************************************
 *           ExpandEnvironmentStringsW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH ExpandEnvironmentStringsW( LPCWSTR src, LPWSTR dst, DWORD len )
{
    UNICODE_STRING us_src, us_dst;
    NTSTATUS status;
    DWORD res;

    TRACE( "(%s %p %u)\n", debugstr_w(src), dst, len );

    RtlInitUnicodeString( &us_src, src );

    /* make sure we don't overflow the maximum UNICODE_STRING size */
    len = min( len, UNICODE_STRING_MAX_CHARS );

    us_dst.Length = 0;
    us_dst.MaximumLength = len * sizeof(WCHAR);
    us_dst.Buffer = dst;

    res = 0;
    status = RtlExpandEnvironmentStrings_U( NULL, &us_src, &us_dst, &res );
    res /= sizeof(WCHAR);
    if (!set_ntstatus( status ))
    {
        if (status != STATUS_BUFFER_TOO_SMALL) return 0;
        if (len && dst) dst[len - 1] = 0;
    }
    return res;
}


/***********************************************************************
 *           GetEnvironmentStrings    (kernelbase.@)
 *           GetEnvironmentStringsA   (kernelbase.@)
 */
LPSTR WINAPI DECLSPEC_HOTPATCH GetEnvironmentStringsA(void)
{
    LPWSTR env;
    LPSTR ret;
    SIZE_T lenA, lenW;

    RtlAcquirePebLock();
    env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    lenW = get_env_length( env );
    lenA = WideCharToMultiByte( CP_ACP, 0, env, lenW, NULL, 0, NULL, NULL );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, lenA )))
        WideCharToMultiByte( CP_ACP, 0, env, lenW, ret, lenA, NULL, NULL );
    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           GetEnvironmentStringsW   (kernelbase.@)
 */
LPWSTR WINAPI DECLSPEC_HOTPATCH GetEnvironmentStringsW(void)
{
    LPWSTR ret;
    SIZE_T len;

    RtlAcquirePebLock();
    len = get_env_length( NtCurrentTeb()->Peb->ProcessParameters->Environment ) * sizeof(WCHAR);
    if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
        memcpy( ret, NtCurrentTeb()->Peb->ProcessParameters->Environment, len );
    RtlReleasePebLock();
    return ret;
}


/***********************************************************************
 *           GetEnvironmentVariableA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetEnvironmentVariableA( LPCSTR name, LPSTR value, DWORD size )
{
    UNICODE_STRING us_name;
    PWSTR valueW;
    DWORD ret;

    /* limit the size to sane values */
    size = min( size, 32767 );
    if (!(valueW = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return 0;

    RtlCreateUnicodeStringFromAsciiz( &us_name, name );
    SetLastError( 0 );
    ret = GetEnvironmentVariableW( us_name.Buffer, valueW, size);
    if (ret && ret < size) WideCharToMultiByte( CP_ACP, 0, valueW, ret + 1, value, size, NULL, NULL );

    /* this is needed to tell, with 0 as a return value, the difference between:
     * - an error (GetLastError() != 0)
     * - returning an empty string (in this case, we need to update the buffer)
     */
    if (ret == 0 && size && GetLastError() == 0) value[0] = 0;
    RtlFreeUnicodeString( &us_name );
    HeapFree( GetProcessHeap(), 0, valueW );
    return ret;
}


/***********************************************************************
 *           GetEnvironmentVariableW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetEnvironmentVariableW( LPCWSTR name, LPWSTR val, DWORD size )
{
    UNICODE_STRING us_name, us_value;
    NTSTATUS status;
    DWORD len;

    TRACE( "(%s %p %u)\n", debugstr_w(name), val, size );

    RtlInitUnicodeString( &us_name, name );
    us_value.Length = 0;
    us_value.MaximumLength = (size ? size - 1 : 0) * sizeof(WCHAR);
    us_value.Buffer = val;

    status = RtlQueryEnvironmentVariable_U( NULL, &us_name, &us_value );
    len = us_value.Length / sizeof(WCHAR);
    if (!set_ntstatus( status )) return (status == STATUS_BUFFER_TOO_SMALL) ? len + 1 : 0;
    if (size) val[len] = 0;
    return len;
}


/***********************************************************************
 *           FreeEnvironmentStringsA   (kernelbase.@)
 *           FreeEnvironmentStringsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeEnvironmentStringsW( LPWSTR ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           SetEnvironmentVariableA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEnvironmentVariableA( LPCSTR name, LPCSTR value )
{
    UNICODE_STRING us_name, us_value;
    BOOL ret;

    if (!name)
    {
        SetLastError( ERROR_ENVVAR_NOT_FOUND );
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz( &us_name, name );
    if (value)
    {
        RtlCreateUnicodeStringFromAsciiz( &us_value, value );
        ret = SetEnvironmentVariableW( us_name.Buffer, us_value.Buffer );
        RtlFreeUnicodeString( &us_value );
    }
    else ret = SetEnvironmentVariableW( us_name.Buffer, NULL );
    RtlFreeUnicodeString( &us_name );
    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariableW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEnvironmentVariableW( LPCWSTR name, LPCWSTR value )
{
    UNICODE_STRING us_name, us_value;
    NTSTATUS status;

    TRACE( "(%s %s)\n", debugstr_w(name), debugstr_w(value) );

    if (!name)
    {
        SetLastError( ERROR_ENVVAR_NOT_FOUND );
        return FALSE;
    }

    RtlInitUnicodeString( &us_name, name );
    if (value)
    {
        RtlInitUnicodeString( &us_value, value );
        status = RtlSetEnvironmentVariable( NULL, &us_name, &us_value );
    }
    else status = RtlSetEnvironmentVariable( NULL, &us_name, NULL );

    return set_ntstatus( status );
}


/***********************************************************************
 * Process/thread attribute lists
 ***********************************************************************/


struct proc_thread_attr
{
    DWORD_PTR attr;
    SIZE_T size;
    void *value;
};

struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD mask;  /* bitmask of items in list */
    DWORD size;  /* max number of items in list */
    DWORD count; /* number of items in list */
    DWORD pad;
    DWORD_PTR unk;
    struct proc_thread_attr attrs[1];
};

/***********************************************************************
 *           InitializeProcThreadAttributeList   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitializeProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                                 DWORD count, DWORD flags, SIZE_T *size )
{
    SIZE_T needed;
    BOOL ret = FALSE;

    TRACE( "(%p %d %x %p)\n", list, count, flags, size );

    needed = FIELD_OFFSET( struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[count] );
    if (list && *size >= needed)
    {
        list->mask = 0;
        list->size = count;
        list->count = 0;
        list->unk = 0;
        ret = TRUE;
    }
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );

    *size = needed;
    return ret;
}


/***********************************************************************
 *           UpdateProcThreadAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UpdateProcThreadAttribute( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                         DWORD flags, DWORD_PTR attr, void *value,
                                                         SIZE_T size, void *prev_ret, SIZE_T *size_ret )
{
    DWORD mask;
    struct proc_thread_attr *entry;

    TRACE( "(%p %x %08lx %p %ld %p %p)\n", list, flags, attr, value, size, prev_ret, size_ret );

    if (list->count >= list->size)
    {
        SetLastError( ERROR_GEN_FAILURE );
        return FALSE;
    }

    switch (attr)
    {
    case PROC_THREAD_ATTRIBUTE_PARENT_PROCESS:
        if (size != sizeof(HANDLE))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    case PROC_THREAD_ATTRIBUTE_HANDLE_LIST:
        if ((size / sizeof(HANDLE)) * sizeof(HANDLE) != size)
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    case PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR:
        if (size != sizeof(PROCESSOR_NUMBER))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    case PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY:
       if (size != sizeof(DWORD) && size != sizeof(DWORD64))
       {
           SetLastError( ERROR_BAD_LENGTH );
           return FALSE;
       }
       break;

    case PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY:
        if (size != sizeof(DWORD) && size != sizeof(DWORD64) && size != sizeof(DWORD64) * 2)
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    default:
        SetLastError( ERROR_NOT_SUPPORTED );
        FIXME( "Unhandled attribute %lu\n", attr & PROC_THREAD_ATTRIBUTE_NUMBER );
        return FALSE;
    }

    mask = 1 << (attr & PROC_THREAD_ATTRIBUTE_NUMBER);
    if (list->mask & mask)
    {
        SetLastError( ERROR_OBJECT_NAME_EXISTS );
        return FALSE;
    }
    list->mask |= mask;

    entry = list->attrs + list->count;
    entry->attr = attr;
    entry->size = size;
    entry->value = value;
    list->count++;
    return TRUE;
}


/***********************************************************************
 *           DeleteProcThreadAttributeList   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH DeleteProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list )
{
    return;
}

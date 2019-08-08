/*
 * Kernel synchronization objects
 *
 * Copyright 1998 Alexandre Julliard
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

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "wine/asm.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "kernel_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

static void get_create_object_attributes( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *nameW,
                                          SECURITY_ATTRIBUTES *sa, const WCHAR *name )
{
    attr->Length                   = sizeof(*attr);
    attr->RootDirectory            = 0;
    attr->ObjectName               = NULL;
    attr->Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr->SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr->SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( nameW, name );
        attr->ObjectName = nameW;
        BaseGetNamedObjectDirectory( &attr->RootDirectory );
    }
}

static BOOL get_open_object_attributes( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *nameW,
                                        BOOL inherit, const WCHAR *name )
{
    HANDLE dir;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlInitUnicodeString( nameW, name );
    BaseGetNamedObjectDirectory( &dir );
    InitializeObjectAttributes( attr, nameW, inherit ? OBJ_INHERIT : 0, dir, NULL );
    return TRUE;
}

static HANDLE normalize_handle_if_console(HANDLE handle)
{
    if ((handle == (HANDLE)STD_INPUT_HANDLE) ||
        (handle == (HANDLE)STD_OUTPUT_HANDLE) ||
        (handle == (HANDLE)STD_ERROR_HANDLE))
        handle = GetStdHandle( HandleToULong(handle) );

    /* yes, even screen buffer console handles are waitable, and are
     * handled as a handle to the console itself !!
     */
    if (is_console_handle(handle))
    {
        if (VerifyConsoleIoHandle(handle))
            handle = GetConsoleInputWaitHandle();
    }
    return handle;
}

/***********************************************************************
 *           RegisterWaitForSingleObject   (KERNEL32.@)
 */
BOOL WINAPI RegisterWaitForSingleObject(PHANDLE phNewWaitObject, HANDLE hObject,
                WAITORTIMERCALLBACK Callback, PVOID Context,
                ULONG dwMilliseconds, ULONG dwFlags)
{
    NTSTATUS status;

    TRACE("%p %p %p %p %d %d\n",
          phNewWaitObject,hObject,Callback,Context,dwMilliseconds,dwFlags);

    hObject = normalize_handle_if_console(hObject);
    status = RtlRegisterWait( phNewWaitObject, hObject, Callback, Context, dwMilliseconds, dwFlags );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           UnregisterWait   (KERNEL32.@)
 */
BOOL WINAPI UnregisterWait( HANDLE WaitHandle ) 
{
    NTSTATUS status;

    TRACE("%p\n",WaitHandle);

    status = RtlDeregisterWait( WaitHandle );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           MakeCriticalSectionGlobal   (KERNEL32.@)
 */
void WINAPI MakeCriticalSectionGlobal( CRITICAL_SECTION *crit )
{
    /* let's assume that only one thread at a time will try to do this */
    HANDLE sem = crit->LockSemaphore;
    if (!sem) NtCreateSemaphore( &sem, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 );
    crit->LockSemaphore = ConvertToGlobalHandle( sem );
    if (crit->DebugInfo != (void *)(ULONG_PTR)-1)
        RtlFreeHeap( GetProcessHeap(), 0, crit->DebugInfo );
    crit->DebugInfo = NULL;
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.@)
 *
 * Initialise an already used critical section.
 *
 * PARAMS
 *  crit [O] Critical section to initialise.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    if ( !crit->LockSemaphore )
        RtlInitializeCriticalSection( crit );
}


/***********************************************************************
 *           UninitializeCriticalSection   (KERNEL32.@)
 *
 * UnInitialise a critical section after use.
 *
 * PARAMS
 *  crit [O] Critical section to uninitialise (destroy).
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI UninitializeCriticalSection( CRITICAL_SECTION *crit )
{
    RtlDeleteCriticalSection( crit );
}


/***********************************************************************
 *           OpenMutexA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenMutexA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenMutexW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenMutexW( access, inherit, buffer );
}




/*
 * Semaphores
 */


/***********************************************************************
 *           CreateSemaphoreA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreA( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max, LPCSTR name )
{
    return CreateSemaphoreExA( sa, initial, max, name, 0, SEMAPHORE_ALL_ACCESS );
}


/***********************************************************************
 *           CreateSemaphoreExA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreExA( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max,
                                                    LPCSTR name, DWORD flags, DWORD access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateSemaphoreExW( sa, initial, max, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateSemaphoreExW( sa, initial, max, buffer, flags, access );
}


/***********************************************************************
 *           OpenSemaphoreA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenSemaphoreA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenSemaphoreW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenSemaphoreW( access, inherit, buffer );
}


/*
 * Jobs
 */

/******************************************************************************
 *		CreateJobObjectW (KERNEL32.@)
 */
HANDLE WINAPI CreateJobObjectW( LPSECURITY_ATTRIBUTES sa, LPCWSTR name )
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateJobObject( &ret, JOB_OBJECT_ALL_ACCESS, &attr );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}

/******************************************************************************
 *		CreateJobObjectA (KERNEL32.@)
 */
HANDLE WINAPI CreateJobObjectA( LPSECURITY_ATTRIBUTES attr, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateJobObjectW( attr, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateJobObjectW( attr, buffer );
}

/******************************************************************************
 *		OpenJobObjectW (KERNEL32.@)
 */
HANDLE WINAPI OpenJobObjectW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    status = NtOpenJobObject( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}

/******************************************************************************
 *		OpenJobObjectA (KERNEL32.@)
 */
HANDLE WINAPI OpenJobObjectA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenJobObjectW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenJobObjectW( access, inherit, buffer );
}

/******************************************************************************
 *		TerminateJobObject (KERNEL32.@)
 */
BOOL WINAPI TerminateJobObject( HANDLE job, UINT exit_code )
{
    NTSTATUS status = NtTerminateJobObject( job, exit_code );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/******************************************************************************
 *		QueryInformationJobObject (KERNEL32.@)
 */
BOOL WINAPI QueryInformationJobObject( HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info,
                                       DWORD len, DWORD *ret_len )
{
    NTSTATUS status = NtQueryInformationJobObject( job, class, info, len, ret_len );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/******************************************************************************
 *		SetInformationJobObject (KERNEL32.@)
 */
BOOL WINAPI SetInformationJobObject( HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info, DWORD len )
{
    NTSTATUS status = NtSetInformationJobObject( job, class, info, len );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/******************************************************************************
 *		AssignProcessToJobObject (KERNEL32.@)
 */
BOOL WINAPI AssignProcessToJobObject( HANDLE job, HANDLE process )
{
    NTSTATUS status = NtAssignProcessToJobObject( job, process );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/******************************************************************************
 *		IsProcessInJob (KERNEL32.@)
 */
BOOL WINAPI IsProcessInJob( HANDLE process, HANDLE job, PBOOL result )
{
    NTSTATUS status = NtIsProcessInJob( process, job );
    switch(status)
    {
    case STATUS_PROCESS_IN_JOB:
        *result = TRUE;
        return TRUE;
    case STATUS_PROCESS_NOT_IN_JOB:
        *result = FALSE;
        return TRUE;
    default:
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
}


/*
 * Timers
 */


/***********************************************************************
 *           CreateWaitableTimerA    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerA( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCSTR name )
{
    return CreateWaitableTimerExA( sa, name, manual ? CREATE_WAITABLE_TIMER_MANUAL_RESET : 0,
                                   TIMER_ALL_ACCESS );
}


/***********************************************************************
 *           CreateWaitableTimerExA    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerExA( SECURITY_ATTRIBUTES *sa, LPCSTR name, DWORD flags, DWORD access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWaitableTimerExW( sa, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWaitableTimerExW( sa, buffer, flags, access );
}


/***********************************************************************
 *           OpenWaitableTimerA    (KERNEL32.@)
 */
HANDLE WINAPI OpenWaitableTimerA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenWaitableTimerW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenWaitableTimerW( access, inherit, buffer );
}


/***********************************************************************
 *           DeleteTimerQueue  (KERNEL32.@)
 */
BOOL WINAPI DeleteTimerQueue(HANDLE TimerQueue)
{
    return DeleteTimerQueueEx(TimerQueue, NULL);
}

/***********************************************************************
 *           CancelTimerQueueTimer    (KERNEL32.@)
 */
BOOL WINAPI CancelTimerQueueTimer(HANDLE queue, HANDLE timer)
{
    return DeleteTimerQueueTimer( queue, timer, NULL );
}



/*
 * Mappings
 */


/***********************************************************************
 *             CreateFileMappingA   (KERNEL32.@)
 */
HANDLE WINAPI CreateFileMappingA( HANDLE file, SECURITY_ATTRIBUTES *sa, DWORD protect,
                                  DWORD size_high, DWORD size_low, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateFileMappingW( file, sa, protect, size_high, size_low, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateFileMappingW( file, sa, protect, size_high, size_low, buffer );
}


/***********************************************************************
 *             OpenFileMappingA   (KERNEL32.@)
 */
HANDLE WINAPI OpenFileMappingA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenFileMappingW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenFileMappingW( access, inherit, buffer );
}


/*
 * Pipes
 */


/***********************************************************************
 *           CreateNamedPipeA   (KERNEL32.@)
 */
HANDLE WINAPI CreateNamedPipeA( LPCSTR name, DWORD dwOpenMode,
                                DWORD dwPipeMode, DWORD nMaxInstances,
                                DWORD nOutBufferSize, DWORD nInBufferSize,
                                DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES attr )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateNamedPipeW( NULL, dwOpenMode, dwPipeMode, nMaxInstances,
                                        nOutBufferSize, nInBufferSize, nDefaultTimeOut, attr );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return INVALID_HANDLE_VALUE;
    }
    return CreateNamedPipeW( buffer, dwOpenMode, dwPipeMode, nMaxInstances,
                             nOutBufferSize, nInBufferSize, nDefaultTimeOut, attr );
}


/***********************************************************************
 *           WaitNamedPipeA   (KERNEL32.@)
 */
BOOL WINAPI WaitNamedPipeA (LPCSTR name, DWORD nTimeOut)
{
    WCHAR buffer[MAX_PATH];

    if (!name) return WaitNamedPipeW( NULL, nTimeOut );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return FALSE;
    }
    return WaitNamedPipeW( buffer, nTimeOut );
}


/***********************************************************************
 *           GetNamedPipeClientProcessId  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeClientProcessId( HANDLE pipe, ULONG *id )
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    TRACE( "%p %p\n", pipe, id );

    status = NtFsControlFile( pipe, NULL, NULL, NULL, &iosb, FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                              (void *)"ClientProcessId", sizeof("ClientProcessId"), id, sizeof(*id) );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           GetNamedPipeServerProcessId  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeServerProcessId( HANDLE pipe, ULONG *id )
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    TRACE( "%p, %p\n", pipe, id );

    status = NtFsControlFile( pipe, NULL, NULL, NULL, &iosb, FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                              (void *)"ServerProcessId", sizeof("ServerProcessId"), id, sizeof(*id) );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           GetNamedPipeClientSessionId  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeClientSessionId( HANDLE pipe, ULONG *id )
{
    FIXME( "%p, %p\n", pipe, id );

    if (!id) return FALSE;
    *id = NtCurrentTeb()->Peb->SessionId;
    return TRUE;
}

/***********************************************************************
 *           GetNamedPipeServerSessionId  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeServerSessionId( HANDLE pipe, ULONG *id )
{
    FIXME( "%p, %p\n", pipe, id );

    if (!id) return FALSE;
    *id = NtCurrentTeb()->Peb->SessionId;
    return TRUE;
}

/***********************************************************************
 *           GetNamedPipeHandleStateA  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeHandleStateA(
    HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout,
    LPSTR lpUsername, DWORD nUsernameMaxSize)
{
    WCHAR *username = NULL;
    BOOL ret;

    WARN("%p %p %p %p %p %p %d: semi-stub\n", hNamedPipe, lpState, lpCurInstances,
         lpMaxCollectionCount, lpCollectDataTimeout, lpUsername, nUsernameMaxSize);

    if (lpUsername && nUsernameMaxSize &&
        !(username = HeapAlloc(GetProcessHeap(), 0, nUsernameMaxSize * sizeof(WCHAR)))) return FALSE;

    ret = GetNamedPipeHandleStateW(hNamedPipe, lpState, lpCurInstances, lpMaxCollectionCount,
                                   lpCollectDataTimeout, username, nUsernameMaxSize);
    if (ret && username)
        WideCharToMultiByte(CP_ACP, 0, username, -1, lpUsername, nUsernameMaxSize, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, username);
    return ret;
}

/***********************************************************************
 *           GetNamedPipeHandleStateW  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeHandleStateW(
    HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout,
    LPWSTR lpUsername, DWORD nUsernameMaxSize)
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    FIXME("%p %p %p %p %p %p %d: semi-stub\n", hNamedPipe, lpState, lpCurInstances,
          lpMaxCollectionCount, lpCollectDataTimeout, lpUsername, nUsernameMaxSize);

    if (lpMaxCollectionCount)
        *lpMaxCollectionCount = 0;

    if (lpCollectDataTimeout)
        *lpCollectDataTimeout = 0;

    if (lpUsername && nUsernameMaxSize)
    {
        const char *username = wine_get_user_name();
        int len = MultiByteToWideChar(CP_UNIXCP, 0, username, -1, lpUsername, nUsernameMaxSize);
        if (!len) *lpUsername = 0;
    }

    if (lpState)
    {
        FILE_PIPE_INFORMATION fpi;
        status = NtQueryInformationFile(hNamedPipe, &iosb, &fpi, sizeof(fpi),
                                        FilePipeInformation);
        if (status)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return FALSE;
        }

        *lpState = (fpi.ReadMode ? PIPE_READMODE_MESSAGE : PIPE_READMODE_BYTE) |
                   (fpi.CompletionMode ? PIPE_NOWAIT : PIPE_WAIT);
    }

    if (lpCurInstances)
    {
        FILE_PIPE_LOCAL_INFORMATION fpli;
        status = NtQueryInformationFile(hNamedPipe, &iosb, &fpli, sizeof(fpli),
                                        FilePipeLocalInformation);
        if (status)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return FALSE;
        }

        *lpCurInstances = fpli.CurrentInstances;
    }

    return TRUE;
}

/***********************************************************************
 *           CallNamedPipeA  (KERNEL32.@)
 */
BOOL WINAPI CallNamedPipeA(
    LPCSTR lpNamedPipeName, LPVOID lpInput, DWORD dwInputSize,
    LPVOID lpOutput, DWORD dwOutputSize,
    LPDWORD lpBytesRead, DWORD nTimeout)
{
    DWORD len;
    LPWSTR str = NULL;
    BOOL ret;

    TRACE("%s %p %d %p %d %p %d\n",
           debugstr_a(lpNamedPipeName), lpInput, dwInputSize,
           lpOutput, dwOutputSize, lpBytesRead, nTimeout);

    if( lpNamedPipeName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpNamedPipeName, -1, NULL, 0 );
        str = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpNamedPipeName, -1, str, len );
    }
    ret = CallNamedPipeW( str, lpInput, dwInputSize, lpOutput,
                          dwOutputSize, lpBytesRead, nTimeout );
    if( lpNamedPipeName )
        HeapFree( GetProcessHeap(), 0, str );

    return ret;
}

/******************************************************************
 *		CreatePipe (KERNEL32.@)
 *
 */
BOOL WINAPI CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe,
                        LPSECURITY_ATTRIBUTES sa, DWORD size )
{
    static unsigned     index /* = 0 */;
    WCHAR               name[64];
    HANDLE              hr, hw;
    unsigned            in_index = index;
    UNICODE_STRING      nt_name;
    OBJECT_ATTRIBUTES   attr;
    NTSTATUS            status;
    IO_STATUS_BLOCK     iosb;
    LARGE_INTEGER       timeout;

    *hReadPipe = *hWritePipe = INVALID_HANDLE_VALUE;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &nt_name;
    attr.Attributes               = OBJ_CASE_INSENSITIVE |
                                    ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    if (!size) size = 4096;

    timeout.QuadPart = (ULONGLONG)NMPWAIT_USE_DEFAULT_WAIT * -10000;
    /* generate a unique pipe name (system wide) */
    do
    {
        static const WCHAR nameFmt[] = { '\\','?','?','\\','p','i','p','e',
         '\\','W','i','n','3','2','.','P','i','p','e','s','.','%','0','8','l',
         'u','.','%','0','8','u','\0' };

        snprintfW(name, ARRAY_SIZE(name), nameFmt, GetCurrentProcessId(), ++index);
        RtlInitUnicodeString(&nt_name, name);
        status = NtCreateNamedPipeFile(&hr, GENERIC_READ | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                       &attr, &iosb, FILE_SHARE_WRITE, FILE_OVERWRITE_IF,
                                       FILE_SYNCHRONOUS_IO_NONALERT,
                                       FALSE, FALSE, FALSE, 
                                       1, size, size, &timeout);
        if (status)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            hr = INVALID_HANDLE_VALUE;
        }
    } while (hr == INVALID_HANDLE_VALUE && index != in_index);
    /* from completion sakeness, I think system resources might be exhausted before this happens !! */
    if (hr == INVALID_HANDLE_VALUE) return FALSE;

    status = NtOpenFile(&hw, GENERIC_WRITE | FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &iosb, 0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);

    if (status) 
    {
        SetLastError( RtlNtStatusToDosError(status) );
        NtClose(hr);
        return FALSE;
    }

    *hReadPipe = hr;
    *hWritePipe = hw;
    return TRUE;
}


/******************************************************************************
 * CreateMailslotA [KERNEL32.@]
 *
 * See CreateMailslotW.
 */
HANDLE WINAPI CreateMailslotA( LPCSTR lpName, DWORD nMaxMessageSize,
                               DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa )
{
    DWORD len;
    HANDLE handle;
    LPWSTR name = NULL;

    TRACE("%s %d %d %p\n", debugstr_a(lpName),
          nMaxMessageSize, lReadTimeout, sa);

    if( lpName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpName, -1, NULL, 0 );
        name = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpName, -1, name, len );
    }

    handle = CreateMailslotW( name, nMaxMessageSize, lReadTimeout, sa );

    HeapFree( GetProcessHeap(), 0, name );

    return handle;
}


/******************************************************************************
 * CreateMailslotW [KERNEL32.@]
 *
 * Create a mailslot with specified name.
 *
 * PARAMS
 *    lpName          [I] Pointer to string for mailslot name
 *    nMaxMessageSize [I] Maximum message size
 *    lReadTimeout    [I] Milliseconds before read time-out
 *    sa              [I] Pointer to security structure
 *
 * RETURNS
 *    Success: Handle to mailslot
 *    Failure: INVALID_HANDLE_VALUE
 */
HANDLE WINAPI CreateMailslotW( LPCWSTR lpName, DWORD nMaxMessageSize,
                               DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    LARGE_INTEGER timeout;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    TRACE("%s %d %d %p\n", debugstr_w(lpName),
          nMaxMessageSize, lReadTimeout, sa);

    if (!RtlDosPathNameToNtPathName_U( lpName, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    if (nameW.Length >= MAX_PATH * sizeof(WCHAR) )
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        RtlFreeUnicodeString( &nameW );
        return INVALID_HANDLE_VALUE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    if (lReadTimeout != MAILSLOT_WAIT_FOREVER)
        timeout.QuadPart = (ULONGLONG) lReadTimeout * -10000;
    else
        timeout.QuadPart = ((LONGLONG)0x7fffffff << 32) | 0xffffffff;

    status = NtCreateMailslotFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr,
                                   &iosb, 0, 0, nMaxMessageSize, &timeout );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        handle = INVALID_HANDLE_VALUE;
    }

    RtlFreeUnicodeString( &nameW );
    return handle;
}


/******************************************************************************
 * GetMailslotInfo [KERNEL32.@]
 *
 * Retrieve information about a mailslot.
 *
 * PARAMS
 *    hMailslot        [I] Mailslot handle
 *    lpMaxMessageSize [O] Address of maximum message size
 *    lpNextSize       [O] Address of size of next message
 *    lpMessageCount   [O] Address of number of messages
 *    lpReadTimeout    [O] Address of read time-out
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetMailslotInfo( HANDLE hMailslot, LPDWORD lpMaxMessageSize,
                               LPDWORD lpNextSize, LPDWORD lpMessageCount,
                               LPDWORD lpReadTimeout )
{
    FILE_MAILSLOT_QUERY_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    TRACE("%p %p %p %p %p\n",hMailslot, lpMaxMessageSize,
          lpNextSize, lpMessageCount, lpReadTimeout);

    status = NtQueryInformationFile( hMailslot, &iosb, &info, sizeof info,
                                     FileMailslotQueryInformation );

    if( status != STATUS_SUCCESS )
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    if( lpMaxMessageSize )
        *lpMaxMessageSize = info.MaximumMessageSize;
    if( lpNextSize )
        *lpNextSize = info.NextMessageSize;
    if( lpMessageCount )
        *lpMessageCount = info.MessagesAvailable;
    if( lpReadTimeout )
    {
        if (info.ReadTimeout.QuadPart == (((LONGLONG)0x7fffffff << 32) | 0xffffffff))
            *lpReadTimeout = MAILSLOT_WAIT_FOREVER;
        else
            *lpReadTimeout = info.ReadTimeout.QuadPart / -10000;
    }
    return TRUE;
}


/******************************************************************************
 * SetMailslotInfo [KERNEL32.@]
 *
 * Set the read timeout of a mailslot.
 *
 * PARAMS
 *  hMailslot     [I] Mailslot handle
 *  dwReadTimeout [I] Timeout in milliseconds.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetMailslotInfo( HANDLE hMailslot, DWORD dwReadTimeout)
{
    FILE_MAILSLOT_SET_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    TRACE("%p %d\n", hMailslot, dwReadTimeout);

    if (dwReadTimeout != MAILSLOT_WAIT_FOREVER)
        info.ReadTimeout.QuadPart = (ULONGLONG)dwReadTimeout * -10000;
    else
        info.ReadTimeout.QuadPart = ((LONGLONG)0x7fffffff << 32) | 0xffffffff;
    status = NtSetInformationFile( hMailslot, &iosb, &info, sizeof info,
                                   FileMailslotSetInformation );
    if( status != STATUS_SUCCESS )
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/******************************************************************************
 *		BindIoCompletionCallback (KERNEL32.@)
 */
BOOL WINAPI BindIoCompletionCallback( HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags)
{
    NTSTATUS status;

    TRACE("(%p, %p, %d)\n", FileHandle, Function, Flags);

    status = RtlSetIoCompletionCallback( FileHandle, (PRTL_OVERLAPPED_COMPLETION_ROUTINE)Function, Flags );
    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/***********************************************************************
 *           CreateMemoryResourceNotification   (KERNEL32.@)
 */
HANDLE WINAPI CreateMemoryResourceNotification(MEMORY_RESOURCE_NOTIFICATION_TYPE type)
{
    static const WCHAR lowmemW[] =
        {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
         '\\','L','o','w','M','e','m','o','r','y','C','o','n','d','i','t','i','o','n',0};
    static const WCHAR highmemW[] =
        {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
         '\\','H','i','g','h','M','e','m','o','r','y','C','o','n','d','i','t','i','o','n',0};
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    switch (type)
    {
    case LowMemoryResourceNotification:
        RtlInitUnicodeString( &nameW, lowmemW );
        break;
    case HighMemoryResourceNotification:
        RtlInitUnicodeString( &nameW, highmemW );
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &nameW;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    status = NtOpenEvent( &ret, EVENT_ALL_ACCESS, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}

/***********************************************************************
 *          QueryMemoryResourceNotification   (KERNEL32.@)
 */
BOOL WINAPI QueryMemoryResourceNotification(HANDLE handle, PBOOL state)
{
    switch (WaitForSingleObject( handle, 0 ))
    {
    case WAIT_OBJECT_0:
        *state = TRUE;
        return TRUE;
    case WAIT_TIMEOUT:
        *state = FALSE;
        return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/***********************************************************************
 *           InitOnceBeginInitialize    (KERNEL32.@)
 */
BOOL WINAPI InitOnceBeginInitialize( INIT_ONCE *once, DWORD flags, BOOL *pending, void **context )
{
    NTSTATUS status = RtlRunOnceBeginInitialize( once, flags, context );
    if (status >= 0) *pending = (status == STATUS_PENDING);
    else SetLastError( RtlNtStatusToDosError(status) );
    return status >= 0;
}

/***********************************************************************
 *           InitOnceComplete    (KERNEL32.@)
 */
BOOL WINAPI InitOnceComplete( INIT_ONCE *once, DWORD flags, void *context )
{
    NTSTATUS status = RtlRunOnceComplete( once, flags, context );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *           InitOnceExecuteOnce    (KERNEL32.@)
 */
BOOL WINAPI InitOnceExecuteOnce( INIT_ONCE *once, PINIT_ONCE_FN func, void *param, void **context )
{
    return !RtlRunOnceExecuteOnce( once, (PRTL_RUN_ONCE_INIT_FN)func, param, context );
}

#ifdef __i386__

/***********************************************************************
 *		InterlockedCompareExchange (KERNEL32.@)
 */
/* LONG WINAPI InterlockedCompareExchange( PLONG dest, LONG xchg, LONG compare ); */
__ASM_STDCALL_FUNC(InterlockedCompareExchange, 12,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret $12")

/***********************************************************************
 *		InterlockedExchange (KERNEL32.@)
 */
/* LONG WINAPI InterlockedExchange( PLONG dest, LONG val ); */
__ASM_STDCALL_FUNC(InterlockedExchange, 8,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret $8")

/***********************************************************************
 *		InterlockedExchangeAdd (KERNEL32.@)
 */
/* LONG WINAPI InterlockedExchangeAdd( PLONG dest, LONG incr ); */
__ASM_STDCALL_FUNC(InterlockedExchangeAdd, 8,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret $8")

/***********************************************************************
 *		InterlockedIncrement (KERNEL32.@)
 */
/* LONG WINAPI InterlockedIncrement( PLONG dest ); */
__ASM_STDCALL_FUNC(InterlockedIncrement, 4,
                  "movl 4(%esp),%edx\n\t"
                  "movl $1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "incl %eax\n\t"
                  "ret $4")

/***********************************************************************
 *		InterlockedDecrement (KERNEL32.@)
 */
__ASM_STDCALL_FUNC(InterlockedDecrement, 4,
                  "movl 4(%esp),%edx\n\t"
                  "movl $-1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "decl %eax\n\t"
                  "ret $4")

#endif  /* __i386__ */

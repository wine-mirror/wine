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

#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "wine/asm.h"
#include "kernel_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

static const struct _KUSER_SHARED_DATA *user_shared_data = (struct _KUSER_SHARED_DATA *)0x7ffe0000;


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

/******************************************************************************
 *           GetTickCount64       (KERNEL32.@)
 */
ULONGLONG WINAPI DECLSPEC_HOTPATCH GetTickCount64(void)
{
    ULONG high, low;

    do
    {
        high = user_shared_data->TickCount.High1Time;
        low = user_shared_data->TickCount.LowPart;
    }
    while (high != user_shared_data->TickCount.High2Time);
    /* note: we ignore TickCountMultiplier */
    return (ULONGLONG)high << 32 | low;
}

/***********************************************************************
 *           GetTickCount       (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTickCount(void)
{
    /* note: we ignore TickCountMultiplier */
    return user_shared_data->TickCount.LowPart;
}

/***********************************************************************
 *           RegisterWaitForSingleObject   (KERNEL32.@)
 */
BOOL WINAPI RegisterWaitForSingleObject( HANDLE *wait, HANDLE object, WAITORTIMERCALLBACK callback,
                                         void *context, ULONG timeout, ULONG flags )
{
    if (!set_ntstatus( RtlRegisterWait( wait, object, callback, context, timeout, flags ))) return FALSE;
    return TRUE;
}

/***********************************************************************
 *           UnregisterWait   (KERNEL32.@)
 */
BOOL WINAPI UnregisterWait( HANDLE handle )
{
    return set_ntstatus( RtlDeregisterWait( handle ));
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

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    if (!set_ntstatus( NtOpenJobObject( &ret, access, &attr ))) return 0;
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
    return set_ntstatus( NtTerminateJobObject( job, exit_code ));
}

/******************************************************************************
 *		QueryInformationJobObject (KERNEL32.@)
 */
BOOL WINAPI QueryInformationJobObject( HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info,
                                       DWORD len, DWORD *ret_len )
{
    return set_ntstatus( NtQueryInformationJobObject( job, class, info, len, ret_len ));
}

/******************************************************************************
 *		SetInformationJobObject (KERNEL32.@)
 */
BOOL WINAPI SetInformationJobObject( HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info, DWORD len )
{
    return set_ntstatus( NtSetInformationJobObject( job, class, info, len ));
}

/******************************************************************************
 *		AssignProcessToJobObject (KERNEL32.@)
 */
BOOL WINAPI AssignProcessToJobObject( HANDLE job, HANDLE process )
{
    return set_ntstatus( NtAssignProcessToJobObject( job, process ));
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

    return set_ntstatus( NtFsControlFile( pipe, NULL, NULL, NULL, &iosb,
                                          FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE, (void *)"ClientProcessId",
                                          sizeof("ClientProcessId"), id, sizeof(*id) ));
}

/***********************************************************************
 *           GetNamedPipeServerProcessId  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeServerProcessId( HANDLE pipe, ULONG *id )
{
    IO_STATUS_BLOCK iosb;

    return set_ntstatus( NtFsControlFile( pipe, NULL, NULL, NULL, &iosb,
                                          FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE, (void *)"ServerProcessId",
                                          sizeof("ServerProcessId"), id, sizeof(*id) ));
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

    WARN("%p %p %p %p %p %p %ld: semi-stub\n", hNamedPipe, lpState, lpCurInstances,
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

    TRACE("%s %p %ld %p %ld %p %ld\n",
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

    TRACE("%s %ld %ld %p\n", debugstr_a(lpName),
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

    TRACE("%s %ld %ld %p\n", debugstr_w(lpName),
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

    if (!set_ntstatus( NtCreateMailslotFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &iosb,
                                             FILE_SYNCHRONOUS_IO_NONALERT, 0, nMaxMessageSize, &timeout )))
        handle = INVALID_HANDLE_VALUE;

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

    TRACE("%p %p %p %p %p\n",hMailslot, lpMaxMessageSize,
          lpNextSize, lpMessageCount, lpReadTimeout);

    if (!set_ntstatus( NtQueryInformationFile( hMailslot, &iosb, &info, sizeof info,
                                               FileMailslotQueryInformation )))
        return FALSE;

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

    TRACE("%p %ld\n", hMailslot, dwReadTimeout);

    if (dwReadTimeout != MAILSLOT_WAIT_FOREVER)
        info.ReadTimeout.QuadPart = (ULONGLONG)dwReadTimeout * -10000;
    else
        info.ReadTimeout.QuadPart = ((LONGLONG)0x7fffffff << 32) | 0xffffffff;
    return set_ntstatus( NtSetInformationFile( hMailslot, &iosb, &info, sizeof info,
                                               FileMailslotSetInformation ));
}


/******************************************************************************
 *		BindIoCompletionCallback (KERNEL32.@)
 */
BOOL WINAPI BindIoCompletionCallback( HANDLE handle, LPOVERLAPPED_COMPLETION_ROUTINE func, ULONG flags )
{
    return set_ntstatus( RtlSetIoCompletionCallback( handle, (PRTL_OVERLAPPED_COMPLETION_ROUTINE)func, flags ));
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

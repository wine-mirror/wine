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

#include <stdarg.h>
#include <string.h>
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

#include "kernelbase.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

/* check if current version is NT or Win95 */
static inline BOOL is_version_nt(void)
{
    return !(GetVersion() & 0x80000000);
}

/* helper for kernel32->ntdll timeout format conversion */
static inline LARGE_INTEGER *get_nt_timeout( LARGE_INTEGER *time, DWORD timeout )
{
    if (timeout == INFINITE) return NULL;
    time->QuadPart = (ULONGLONG)timeout * -10000;
    return time;
}


/***********************************************************************
 *              BaseGetNamedObjectDirectory  (kernelbase.@)
 */
NTSTATUS WINAPI BaseGetNamedObjectDirectory( HANDLE *dir )
{
    static HANDLE handle;
    static const WCHAR basenameW[] = {'\\','S','e','s','s','i','o','n','s','\\','%','u',
                                      '\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',0};
    WCHAR buffer[64];
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status = STATUS_SUCCESS;

    if (!handle)
    {
        HANDLE dir;

        swprintf( buffer, ARRAY_SIZE(buffer), basenameW, NtCurrentTeb()->Peb->SessionId );
        RtlInitUnicodeString( &str, buffer );
        InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
        status = NtOpenDirectoryObject( &dir, DIRECTORY_CREATE_OBJECT|DIRECTORY_TRAVERSE, &attr );
        if (!status && InterlockedCompareExchangePointer( &handle, dir, 0 ) != 0)
        {
            /* someone beat us here... */
            CloseHandle( dir );
        }
    }
    *dir = handle;
    return status;
}

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


/***********************************************************************
 * Waits
 ***********************************************************************/


static HANDLE normalize_handle_if_console( HANDLE handle )
{
    if ((handle == (HANDLE)STD_INPUT_HANDLE) ||
        (handle == (HANDLE)STD_OUTPUT_HANDLE) ||
        (handle == (HANDLE)STD_ERROR_HANDLE))
        handle = GetStdHandle( HandleToULong(handle) );

    /* even screen buffer console handles are waitable, and are
     * handled as a handle to the console itself
     */
    if (is_console_handle( handle ) && VerifyConsoleIoHandle( handle ))
        handle = GetConsoleInputWaitHandle();

    return handle;
}


/***********************************************************************
 *           RegisterWaitForSingleObjectEx   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH RegisterWaitForSingleObjectEx( HANDLE handle, WAITORTIMERCALLBACK callback,
                                                               PVOID context, ULONG timeout, ULONG flags )
{
    NTSTATUS status;
    HANDLE ret;

    TRACE( "%p %p %p %d %d\n", handle, callback, context, timeout, flags );

    handle = normalize_handle_if_console( handle );
    status = RtlRegisterWait( &ret, handle, callback, context, timeout, flags );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }
    return ret;
}


/***********************************************************************
 *           SignalObjectAndWait  (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SignalObjectAndWait( HANDLE signal, HANDLE wait,
                                                    DWORD timeout, BOOL alertable )
{
    NTSTATUS status;
    LARGE_INTEGER time;

    TRACE( "%p %p %d %d\n", signal, wait, timeout, alertable );

    status = NtSignalAndWaitForSingleObject( signal, wait, alertable, get_nt_timeout( &time, timeout ) );
    if (HIWORD(status))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        status = WAIT_FAILED;
    }
    return status;
}


/***********************************************************************
 *              Sleep  (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH Sleep( DWORD timeout )
{
    LARGE_INTEGER time;

    NtDelayExecution( FALSE, get_nt_timeout( &time, timeout ) );
}


/******************************************************************************
 *              SleepEx   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SleepEx( DWORD timeout, BOOL alertable )
{
    NTSTATUS status;
    LARGE_INTEGER time;

    status = NtDelayExecution( alertable, get_nt_timeout( &time, timeout ) );
    if (status == STATUS_USER_APC) return WAIT_IO_COMPLETION;
    return 0;
}


/***********************************************************************
 *           UnregisterWaitEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnregisterWaitEx( HANDLE handle, HANDLE event )
{
    return set_ntstatus( RtlDeregisterWaitEx( handle, event ));
}


/***********************************************************************
 *           WaitForSingleObject   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH WaitForSingleObjectEx( HANDLE handle, DWORD timeout, BOOL alertable )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, alertable );
}


/***********************************************************************
 *           WaitForMultipleObjects   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                                       BOOL wait_all, DWORD timeout )
{
    return WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH WaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                         BOOL wait_all, DWORD timeout, BOOL alertable )
{
    NTSTATUS status;
    HANDLE hloc[MAXIMUM_WAIT_OBJECTS];
    LARGE_INTEGER time;
    unsigned int i;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }
    for (i = 0; i < count; i++) hloc[i] = normalize_handle_if_console( handles[i] );

    status = NtWaitForMultipleObjects( count, hloc, !wait_all, alertable,
                                       get_nt_timeout( &time, timeout ) );
    if (HIWORD(status))  /* is it an error code? */
    {
        SetLastError( RtlNtStatusToDosError(status) );
        status = WAIT_FAILED;
    }
    return status;
}


/***********************************************************************
 * Events
 ***********************************************************************/


/***********************************************************************
 *           CreateEventA    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventA( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                                              BOOL initial_state, LPCSTR name )
{
    DWORD flags = 0;

    if (manual_reset) flags |= CREATE_EVENT_MANUAL_RESET;
    if (initial_state) flags |= CREATE_EVENT_INITIAL_SET;
    return CreateEventExA( sa, name, flags, EVENT_ALL_ACCESS );
}


/***********************************************************************
 *           CreateEventW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventW( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                                              BOOL initial_state, LPCWSTR name )
{
    DWORD flags = 0;

    if (manual_reset) flags |= CREATE_EVENT_MANUAL_RESET;
    if (initial_state) flags |= CREATE_EVENT_INITIAL_SET;
    return CreateEventExW( sa, name, flags, EVENT_ALL_ACCESS );
}


/***********************************************************************
 *           CreateEventExA    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventExA( SECURITY_ATTRIBUTES *sa, LPCSTR name,
                                                DWORD flags, DWORD access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateEventExW( sa, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateEventExW( sa, buffer, flags, access );
}


/***********************************************************************
 *           CreateEventExW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name,
                                                DWORD flags, DWORD access )
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    /* one buggy program needs this
     * ("Van Dale Groot woordenboek der Nederlandse taal")
     */
    if (sa && IsBadReadPtr(sa,sizeof(SECURITY_ATTRIBUTES)))
    {
        ERR("Bad security attributes pointer %p\n",sa);
        SetLastError( ERROR_INVALID_PARAMETER);
        return 0;
    }

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateEvent( &ret, access, &attr,
                            (flags & CREATE_EVENT_MANUAL_RESET) ? NotificationEvent : SynchronizationEvent,
                            (flags & CREATE_EVENT_INITIAL_SET) != 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           OpenEventA    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenEventA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenEventW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenEventW( access, inherit, buffer );
}


/***********************************************************************
 *           OpenEventW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenEventW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = EVENT_ALL_ACCESS;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    status = NtOpenEvent( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}

/***********************************************************************
 *           PulseEvent    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PulseEvent( HANDLE handle )
{
    return set_ntstatus( NtPulseEvent( handle, NULL ));
}


/***********************************************************************
 *           SetEvent    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEvent( HANDLE handle )
{
    return set_ntstatus( NtSetEvent( handle, NULL ));
}


/***********************************************************************
 *           ResetEvent    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ResetEvent( HANDLE handle )
{
    return set_ntstatus( NtResetEvent( handle, NULL ));
}


/***********************************************************************
 * Mutexes
 ***********************************************************************/


/***********************************************************************
 *           CreateMutexA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    return CreateMutexExA( sa, name, owner ? CREATE_MUTEX_INITIAL_OWNER : 0, MUTEX_ALL_ACCESS );
}


/***********************************************************************
 *           CreateMutexW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexW( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCWSTR name )
{
    return CreateMutexExW( sa, name, owner ? CREATE_MUTEX_INITIAL_OWNER : 0, MUTEX_ALL_ACCESS );
}


/***********************************************************************
 *           CreateMutexExA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexExA( SECURITY_ATTRIBUTES *sa, LPCSTR name,
                                                DWORD flags, DWORD access )
{
    ANSI_STRING nameA;
    NTSTATUS status;

    if (!name) return CreateMutexExW( sa, NULL, flags, access );

    RtlInitAnsiString( &nameA, name );
    status = RtlAnsiStringToUnicodeString( &NtCurrentTeb()->StaticUnicodeString, &nameA, FALSE );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateMutexExW( sa, NtCurrentTeb()->StaticUnicodeString.Buffer, flags, access );
}


/***********************************************************************
 *           CreateMutexExW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name,
                                                DWORD flags, DWORD access )
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateMutant( &ret, access, &attr, (flags & CREATE_MUTEX_INITIAL_OWNER) != 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           OpenMutexW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenMutexW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = MUTEX_ALL_ACCESS;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    status = NtOpenMutant( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}


/***********************************************************************
 *           ReleaseMutex   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReleaseMutex( HANDLE handle )
{
    return set_ntstatus( NtReleaseMutant( handle, NULL ));
}


/***********************************************************************
 * Semaphores
 ***********************************************************************/


/***********************************************************************
 *           CreateSemaphoreW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreW( SECURITY_ATTRIBUTES *sa, LONG initial,
                                                  LONG max, LPCWSTR name )
{
    return CreateSemaphoreExW( sa, initial, max, name, 0, SEMAPHORE_ALL_ACCESS );
}


/***********************************************************************
 *           CreateSemaphoreExW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreExW( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max,
                                                    LPCWSTR name, DWORD flags, DWORD access )
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateSemaphore( &ret, access, &attr, initial, max );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           OpenSemaphoreW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenSemaphoreW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = SEMAPHORE_ALL_ACCESS;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    status = NtOpenSemaphore( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}


/***********************************************************************
 *           ReleaseSemaphore   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    return set_ntstatus( NtReleaseSemaphore( handle, count, (PULONG)previous ));
}


/***********************************************************************
 * Waitable timers
 ***********************************************************************/


/***********************************************************************
 *           CreateWaitableTimerW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateWaitableTimerW( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCWSTR name )
{
    return CreateWaitableTimerExW( sa, name, manual ? CREATE_WAITABLE_TIMER_MANUAL_RESET : 0,
                                   TIMER_ALL_ACCESS );
}


/***********************************************************************
 *           CreateWaitableTimerExW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateWaitableTimerExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name,
                                                        DWORD flags, DWORD access )
{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateTimer( &handle, access, &attr,
                 (flags & CREATE_WAITABLE_TIMER_MANUAL_RESET) ? NotificationTimer : SynchronizationTimer );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return handle;
}


/***********************************************************************
 *           OpenWaitableTimerW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenWaitableTimerW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE handle;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = TIMER_ALL_ACCESS;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    status = NtOpenTimer( &handle, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return handle;
}


/***********************************************************************
 *           SetWaitableTimer    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetWaitableTimer( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                                                PTIMERAPCROUTINE callback, LPVOID arg, BOOL resume )
{
    NTSTATUS status = NtSetTimer( handle, when, (PTIMER_APC_ROUTINE)callback,
                                  arg, resume, period, NULL );
    return set_ntstatus( status ) || status == STATUS_TIMER_RESUME_IGNORED;
}


/***********************************************************************
 *           SetWaitableTimerEx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetWaitableTimerEx( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                                                  PTIMERAPCROUTINE callback, LPVOID arg,
                                                  REASON_CONTEXT *context, ULONG tolerabledelay )
{
    static int once;
    if (!once++) FIXME( "(%p, %p, %d, %p, %p, %p, %d) semi-stub\n",
                        handle, when, period, callback, arg, context, tolerabledelay );

    return SetWaitableTimer( handle, when, period, callback, arg, FALSE );
}


/***********************************************************************
 *           CancelWaitableTimer    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelWaitableTimer( HANDLE handle )
{
    return set_ntstatus( NtCancelTimer( handle, NULL ));
}


/***********************************************************************
 * Timer queues
 ***********************************************************************/


/***********************************************************************
 *           CreateTimerQueue  (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateTimerQueue(void)
{
    HANDLE q;
    NTSTATUS status = RtlCreateTimerQueue( &q );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( status ));
        return NULL;
    }
    return q;
}


/***********************************************************************
 *           CreateTimerQueueTimer  (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateTimerQueueTimer( PHANDLE timer, HANDLE queue,
                                                     WAITORTIMERCALLBACK callback, PVOID arg,
                                                     DWORD when, DWORD period, ULONG flags )
{
    return set_ntstatus( RtlCreateTimer( timer, queue, callback, arg, when, period, flags ));
}


/***********************************************************************
 *           ChangeTimerQueueTimer  (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ChangeTimerQueueTimer( HANDLE queue, HANDLE timer,
                                                     ULONG when, ULONG period )
{
    return set_ntstatus( RtlUpdateTimer( queue, timer, when, period ));
}


/***********************************************************************
 *           DeleteTimerQueueEx  (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteTimerQueueEx( HANDLE queue, HANDLE event )
{
    return set_ntstatus( RtlDeleteTimerQueueEx( queue, event ));
}


/***********************************************************************
 *           DeleteTimerQueueTimer  (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteTimerQueueTimer( HANDLE queue, HANDLE timer, HANDLE event )
{
    return set_ntstatus( RtlDeleteTimer( queue, timer, event ));
}


/***********************************************************************
 * Critical sections
 ***********************************************************************/


/***********************************************************************
 *           InitializeCriticalSectionAndSpinCount   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitializeCriticalSectionAndSpinCount( CRITICAL_SECTION *crit, DWORD count )
{
    return !RtlInitializeCriticalSectionAndSpinCount( crit, count );
}

/***********************************************************************
 *           InitializeCriticalSectionEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitializeCriticalSectionEx( CRITICAL_SECTION *crit, DWORD spincount,
                                                           DWORD flags )
{
    NTSTATUS ret = RtlInitializeCriticalSectionEx( crit, spincount, flags );
    if (ret) RtlRaiseStatus( ret );
    return !ret;
}


/***********************************************************************
 * File mappings
 ***********************************************************************/


/***********************************************************************
 *             CreateFileMappingW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileMappingW( HANDLE file, LPSECURITY_ATTRIBUTES sa, DWORD protect,
                                                    DWORD size_high, DWORD size_low, LPCWSTR name )
{
    static const int sec_flags = (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_COMMIT |
                                  SEC_NOCACHE | SEC_WRITECOMBINE | SEC_LARGE_PAGES);
    HANDLE ret;
    NTSTATUS status;
    DWORD access, sec_type;
    LARGE_INTEGER size;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    sec_type = protect & sec_flags;
    protect &= ~sec_flags;
    if (!sec_type) sec_type = SEC_COMMIT;

    /* Win9x compatibility */
    if (!protect && !is_version_nt()) protect = PAGE_READONLY;

    switch(protect)
    {
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
        access = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
        break;
    case PAGE_READWRITE:
        access = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE;
        break;
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        access = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE;
        break;
    case PAGE_EXECUTE_READWRITE:
        access = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    size.u.LowPart  = size_low;
    size.u.HighPart = size_high;

    if (file == INVALID_HANDLE_VALUE)
    {
        file = 0;
        if (!size.QuadPart)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateSection( &ret, access, &attr, &size, protect, sec_type, file );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *             OpenFileMappingW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenFileMappingW( DWORD access, BOOL inherit, LPCWSTR name )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE ret;
    NTSTATUS status;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    if (access == FILE_MAP_COPY) access = SECTION_MAP_READ;

    if (!is_version_nt())
    {
        /* win9x doesn't do access checks, so try with full access first */
        if (!NtOpenSection( &ret, access | SECTION_MAP_READ | SECTION_MAP_WRITE, &attr )) return ret;
    }

    status = NtOpenSection( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}


/***********************************************************************
 * Condition variables
 ***********************************************************************/


/***********************************************************************
 *           SleepConditionVariableCS   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SleepConditionVariableCS( CONDITION_VARIABLE *variable,
                                                        CRITICAL_SECTION *crit, DWORD timeout )
{
    LARGE_INTEGER time;

    return set_ntstatus( RtlSleepConditionVariableCS( variable, crit, get_nt_timeout( &time, timeout )));
}


/***********************************************************************
 *           SleepConditionVariableSRW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable,
                                                         RTL_SRWLOCK *lock, DWORD timeout, ULONG flags )
{
    LARGE_INTEGER time;

    return set_ntstatus( RtlSleepConditionVariableSRW( variable, lock,
                                                       get_nt_timeout( &time, timeout ), flags ));
}


/***********************************************************************
 * I/O completions
 ***********************************************************************/


/******************************************************************************
 *		CreateIoCompletionPort   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateIoCompletionPort( HANDLE handle, HANDLE port,
                                                        ULONG_PTR key, DWORD threads )
{
    FILE_COMPLETION_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    HANDLE ret = port;

    TRACE( "(%p, %p, %08lx, %08x)\n", handle, port, key, threads );

    if (!port)
    {
        if ((status = NtCreateIoCompletion( &ret, IO_COMPLETION_ALL_ACCESS, NULL, threads )))
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return 0;
        }
    }
    else if (handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
        info.CompletionPort = ret;
        info.CompletionKey = key;
        if ((status = NtSetInformationFile( handle, &iosb, &info, sizeof(info), FileCompletionInformation )))
        {
            if (!port) CloseHandle( ret );
            SetLastError( RtlNtStatusToDosError(status) );
            return 0;
        }
    }
    return ret;
}


/******************************************************************************
 *		GetQueuedCompletionStatus   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetQueuedCompletionStatus( HANDLE port, LPDWORD count, PULONG_PTR key,
                                                         LPOVERLAPPED *overlapped, DWORD timeout )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER wait_time;

    TRACE( "(%p,%p,%p,%p,%d)\n", port, count, key, overlapped, timeout );

    *overlapped = NULL;
    status = NtRemoveIoCompletion( port, key, (PULONG_PTR)overlapped, &iosb,
                                   get_nt_timeout( &wait_time, timeout ) );
    if (status == STATUS_SUCCESS)
    {
        *count = iosb.Information;
        if (iosb.u.Status >= 0) return TRUE;
        SetLastError( RtlNtStatusToDosError(iosb.u.Status) );
        return FALSE;
    }

    if (status == STATUS_TIMEOUT) SetLastError( WAIT_TIMEOUT );
    else SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}

/******************************************************************************
 *              GetQueuedCompletionStatusEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetQueuedCompletionStatusEx( HANDLE port, OVERLAPPED_ENTRY *entries,
                                                           ULONG count, ULONG *written,
                                                           DWORD timeout, BOOL alertable )
{
    LARGE_INTEGER time;
    NTSTATUS ret;

    TRACE( "%p %p %u %p %u %u\n", port, entries, count, written, timeout, alertable );

    ret = NtRemoveIoCompletionEx( port, (FILE_IO_COMPLETION_INFORMATION *)entries, count,
                                  written, get_nt_timeout( &time, timeout ), alertable );
    if (ret == STATUS_SUCCESS) return TRUE;
    else if (ret == STATUS_TIMEOUT) SetLastError( WAIT_TIMEOUT );
    else if (ret == STATUS_USER_APC) SetLastError( WAIT_IO_COMPLETION );
    else SetLastError( RtlNtStatusToDosError(ret) );
    return FALSE;
}


/******************************************************************************
 *		PostQueuedCompletionStatus   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PostQueuedCompletionStatus( HANDLE port, DWORD count,
                                                          ULONG_PTR key, LPOVERLAPPED overlapped )
{
    TRACE( "%p %d %08lx %p\n", port, count, key, overlapped );

    return set_ntstatus( NtSetIoCompletion( port, key, (ULONG_PTR)overlapped, STATUS_SUCCESS, count ));
}


/***********************************************************************
 * Named pipes
 ***********************************************************************/


/***********************************************************************
 *           CallNamedPipeW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CallNamedPipeW( LPCWSTR name, LPVOID input, DWORD in_size,
                                              LPVOID output, DWORD out_size,
                                              LPDWORD read_size, DWORD timeout )
{
    HANDLE pipe;
    BOOL ret;
    DWORD mode;

    TRACE( "%s %p %d %p %d %p %d\n", debugstr_w(name),
           input, in_size, output, out_size, read_size, timeout );

    pipe = CreateFileW( name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (pipe == INVALID_HANDLE_VALUE)
    {
        if (!WaitNamedPipeW( name, timeout )) return FALSE;
        pipe = CreateFileW( name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
        if (pipe == INVALID_HANDLE_VALUE) return FALSE;
    }

    mode = PIPE_READMODE_MESSAGE;
    ret = SetNamedPipeHandleState( pipe, &mode, NULL, NULL );
    if (ret) ret = TransactNamedPipe( pipe, input, in_size, output, out_size, read_size, NULL );
    CloseHandle( pipe );
    return ret;
}


/***********************************************************************
 *           ConnectNamedPipe   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ConnectNamedPipe( HANDLE pipe, LPOVERLAPPED overlapped )
{
    NTSTATUS status;
    IO_STATUS_BLOCK status_block;
    LPVOID cvalue = NULL;

    TRACE( "(%p,%p)\n", pipe, overlapped );

    if (overlapped)
    {
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
        if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;
    }

    status = NtFsControlFile( pipe, overlapped ? overlapped->hEvent : NULL, NULL, cvalue,
                              overlapped ? (IO_STATUS_BLOCK *)overlapped : &status_block,
                              FSCTL_PIPE_LISTEN, NULL, 0, NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( pipe, INFINITE );
        status = status_block.u.Status;
    }
    return set_ntstatus( status );
}

/***********************************************************************
 *           CreateNamedPipeW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateNamedPipeW( LPCWSTR name, DWORD open_mode, DWORD pipe_mode,
                                                  DWORD instances, DWORD out_buff, DWORD in_buff,
                                                  DWORD timeout, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE handle;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    DWORD access, options, sharing;
    BOOLEAN pipe_type, read_mode, non_block;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER time;

    TRACE( "(%s, %#08x, %#08x, %d, %d, %d, %d, %p)\n", debugstr_w(name),
           open_mode, pipe_mode, instances, out_buff, in_buff, timeout, sa );

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }
    if (nt_name.Length >= MAX_PATH * sizeof(WCHAR) )
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        RtlFreeUnicodeString( &nt_name );
        return INVALID_HANDLE_VALUE;
    }

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &nt_name;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    switch (open_mode & 3)
    {
    case PIPE_ACCESS_INBOUND:
        sharing = FILE_SHARE_WRITE;
        access  = GENERIC_READ;
        break;
    case PIPE_ACCESS_OUTBOUND:
        sharing = FILE_SHARE_READ;
        access  = GENERIC_WRITE;
        break;
    case PIPE_ACCESS_DUPLEX:
        sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
        access  = GENERIC_READ | GENERIC_WRITE;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }
    access |= SYNCHRONIZE;
    options = 0;
    if (open_mode & WRITE_DAC) access |= WRITE_DAC;
    if (open_mode & WRITE_OWNER) access |= WRITE_OWNER;
    if (open_mode & ACCESS_SYSTEM_SECURITY) access |= ACCESS_SYSTEM_SECURITY;
    if (open_mode & FILE_FLAG_WRITE_THROUGH) options |= FILE_WRITE_THROUGH;
    if (!(open_mode & FILE_FLAG_OVERLAPPED)) options |= FILE_SYNCHRONOUS_IO_NONALERT;
    pipe_type = (pipe_mode & PIPE_TYPE_MESSAGE) != 0;
    read_mode = (pipe_mode & PIPE_READMODE_MESSAGE) != 0;
    non_block = (pipe_mode & PIPE_NOWAIT) != 0;
    if (instances >= PIPE_UNLIMITED_INSTANCES) instances = ~0U;

    time.QuadPart = (ULONGLONG)timeout * -10000;
    SetLastError( 0 );
    status = NtCreateNamedPipeFile( &handle, access, &attr, &iosb, sharing,
                                    FILE_OVERWRITE_IF, options, pipe_type,
                                    read_mode, non_block, instances, in_buff, out_buff, &time );
    RtlFreeUnicodeString( &nt_name );
    if (status)
    {
        handle = INVALID_HANDLE_VALUE;
        SetLastError( RtlNtStatusToDosError(status) );
    }
    return handle;
}


/***********************************************************************
 *           DisconnectNamedPipe   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DisconnectNamedPipe( HANDLE pipe )
{
    IO_STATUS_BLOCK io_block;

    TRACE( "(%p)\n", pipe );
    return set_ntstatus( NtFsControlFile( pipe, 0, NULL, NULL, &io_block,
                                          FSCTL_PIPE_DISCONNECT, NULL, 0, NULL, 0 ));
}


/***********************************************************************
 *           GetNamedPipeInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNamedPipeInfo( HANDLE pipe, LPDWORD flags, LPDWORD out_size,
                                                LPDWORD in_size, LPDWORD instances )
{
    FILE_PIPE_LOCAL_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    status = NtQueryInformationFile( pipe, &iosb, &info, sizeof(info), FilePipeLocalInformation );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    if (flags)
    {
        *flags = (info.NamedPipeEnd & FILE_PIPE_SERVER_END) ? PIPE_SERVER_END : PIPE_CLIENT_END;
        *flags |= (info.NamedPipeType & FILE_PIPE_TYPE_MESSAGE) ? PIPE_TYPE_MESSAGE : PIPE_TYPE_BYTE;
    }
    if (out_size) *out_size = info.OutboundQuota;
    if (in_size) *in_size = info.InboundQuota;
    if (instances) *instances = info.MaximumInstances;
    return TRUE;
}


/***********************************************************************
 *           PeekNamedPipe   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekNamedPipe( HANDLE pipe, LPVOID out_buffer, DWORD size,
                                             LPDWORD read_size, LPDWORD avail, LPDWORD message )
{
    FILE_PIPE_PEEK_BUFFER local_buffer;
    FILE_PIPE_PEEK_BUFFER *buffer = &local_buffer;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (size && !(buffer = HeapAlloc( GetProcessHeap(), 0,
                                      FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[size] ))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    status = NtFsControlFile( pipe, 0, NULL, NULL, &io, FSCTL_PIPE_PEEK, NULL, 0,
                              buffer, FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[size] ) );
    if (status == STATUS_BUFFER_OVERFLOW) status = STATUS_SUCCESS;
    if (!status)
    {
        ULONG count = io.Information - FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data );
        if (avail) *avail = buffer->ReadDataAvailable;
        if (read_size) *read_size = count;
        if (message) *message = buffer->MessageLength - count;
        if (out_buffer) memcpy( out_buffer, buffer->Data, count );
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    if (buffer != &local_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return !status;
}


/***********************************************************************
 *           SetNamedPipeHandleState  (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetNamedPipeHandleState( HANDLE pipe, LPDWORD mode,
                                                       LPDWORD count, LPDWORD timeout )
{
    FILE_PIPE_INFORMATION info;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status = STATUS_SUCCESS;

    /* should be a fixme, but this function is called a lot by the RPC
     * runtime, and it slows down InstallShield a fair bit. */
    WARN( "semi-stub: %p %p/%d %p %p\n", pipe, mode, mode ? *mode : 0, count, timeout );

    if (mode)
    {
        if (*mode & ~(PIPE_READMODE_MESSAGE | PIPE_NOWAIT)) status = STATUS_INVALID_PARAMETER;
        else
        {
            info.CompletionMode = (*mode & PIPE_NOWAIT) ?
                FILE_PIPE_COMPLETE_OPERATION : FILE_PIPE_QUEUE_OPERATION;
            info.ReadMode = (*mode & PIPE_READMODE_MESSAGE) ?
                FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
            status = NtSetInformationFile( pipe, &iosb, &info, sizeof(info), FilePipeInformation );
        }
    }
    return set_ntstatus( status );
}

/***********************************************************************
 *           TransactNamedPipe   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TransactNamedPipe( HANDLE handle, LPVOID write_buf, DWORD write_size,
                                                 LPVOID read_buf, DWORD read_size, LPDWORD bytes_read,
                                                 LPOVERLAPPED overlapped)
{
    IO_STATUS_BLOCK default_iosb, *iosb = &default_iosb;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "%p %p %u %p %u %p %p\n", handle,
           write_buf, write_size, read_buf, read_size, bytes_read, overlapped );

    if (overlapped)
    {
        event = overlapped->hEvent;
        iosb = (IO_STATUS_BLOCK *)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else
    {
        iosb->Information = 0;
    }

    status = NtFsControlFile( handle, event, NULL, cvalue, iosb, FSCTL_PIPE_TRANSCEIVE,
                              write_buf, write_size, read_buf, read_size );
    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject(handle, INFINITE);
        status = iosb->u.Status;
    }

    if (bytes_read) *bytes_read = overlapped && status ? 0 : iosb->Information;
    return set_ntstatus( status );
}


/***********************************************************************
 *           WaitNamedPipeW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WaitNamedPipeW( LPCWSTR name, DWORD timeout )
{
    static const WCHAR leadin[] = {'\\','?','?','\\','P','I','P','E','\\'};
    NTSTATUS status;
    UNICODE_STRING nt_name, pipe_dev_name;
    FILE_PIPE_WAIT_FOR_BUFFER *pipe_wait;
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES attr;
    ULONG wait_size;
    HANDLE pipe_dev;

    TRACE( "%s 0x%08x\n", debugstr_w(name), timeout );

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL )) return FALSE;

    if (nt_name.Length >= MAX_PATH * sizeof(WCHAR) ||
        nt_name.Length < sizeof(leadin) ||
        wcsnicmp( nt_name.Buffer, leadin, ARRAY_SIZE( leadin )) != 0)
    {
        RtlFreeUnicodeString( &nt_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    wait_size = sizeof(*pipe_wait) + nt_name.Length - sizeof(leadin) - sizeof(WCHAR);
    if (!(pipe_wait = HeapAlloc( GetProcessHeap(), 0,  wait_size)))
    {
        RtlFreeUnicodeString( &nt_name );
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    pipe_dev_name.Buffer = nt_name.Buffer;
    pipe_dev_name.Length = sizeof(leadin);
    pipe_dev_name.MaximumLength = sizeof(leadin);
    InitializeObjectAttributes( &attr,&pipe_dev_name, OBJ_CASE_INSENSITIVE, NULL, NULL );
    status = NtOpenFile( &pipe_dev, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr,
                         &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);
    if (status != STATUS_SUCCESS)
    {
        HeapFree( GetProcessHeap(), 0, pipe_wait );
        RtlFreeUnicodeString( &nt_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    pipe_wait->TimeoutSpecified = !(timeout == NMPWAIT_USE_DEFAULT_WAIT);
    if (timeout == NMPWAIT_WAIT_FOREVER)
        pipe_wait->Timeout.QuadPart = ((ULONGLONG)0x7fffffff << 32) | 0xffffffff;
    else
        pipe_wait->Timeout.QuadPart = (ULONGLONG)timeout * -10000;
    pipe_wait->NameLength = nt_name.Length - sizeof(leadin);
    memcpy( pipe_wait->Name, nt_name.Buffer + ARRAY_SIZE( leadin ), pipe_wait->NameLength );
    RtlFreeUnicodeString( &nt_name );

    status = NtFsControlFile( pipe_dev, NULL, NULL, NULL, &iosb, FSCTL_PIPE_WAIT,
                              pipe_wait, wait_size, NULL, 0 );

    HeapFree( GetProcessHeap(), 0, pipe_wait );
    NtClose( pipe_dev );
    return set_ntstatus( status );
}

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

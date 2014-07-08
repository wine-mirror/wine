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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "wine/unicode.h"
#include "kernel_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

/* check if current version is NT or Win95 */
static inline BOOL is_version_nt(void)
{
    return !(GetVersion() & 0x80000000);
}

/* returns directory handle to \\BaseNamedObjects */
HANDLE get_BaseNamedObjects_handle(void)
{
    static HANDLE handle = NULL;
    static const WCHAR basenameW[] =
        {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',0};
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;

    if (!handle)
    {
        HANDLE dir;

        RtlInitUnicodeString(&str, basenameW);
        InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
        NtOpenDirectoryObject(&dir, DIRECTORY_CREATE_OBJECT|DIRECTORY_TRAVERSE,
                              &attr);
        if (InterlockedCompareExchangePointer( &handle, dir, 0 ) != 0)
        {
            /* someone beat us here... */
            CloseHandle( dir );
        }
    }
    return handle;
}

/* helper for kernel32->ntdll timeout format conversion */
static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, DWORD timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

/***********************************************************************
 *              Sleep  (KERNEL32.@)
 */
VOID WINAPI DECLSPEC_HOTPATCH Sleep( DWORD timeout )
{
    SleepEx( timeout, FALSE );
}

/******************************************************************************
 *              SleepEx   (KERNEL32.@)
 */
DWORD WINAPI SleepEx( DWORD timeout, BOOL alertable )
{
    NTSTATUS status;
    LARGE_INTEGER time;

    status = NtDelayExecution( alertable, get_nt_timeout( &time, timeout ) );
    if (status == STATUS_USER_APC) return WAIT_IO_COMPLETION;
    return 0;
}


/***********************************************************************
 *		SwitchToThread (KERNEL32.@)
 */
BOOL WINAPI SwitchToThread(void)
{
    return (NtYieldExecution() != STATUS_NO_YIELD_PERFORMED);
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.@)
 */
DWORD WINAPI WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.@)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE handle, DWORD timeout,
                                    BOOL alertable )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, alertable );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.@)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                     BOOL wait_all, DWORD timeout )
{
    return WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.@)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout,
                                       BOOL alertable )
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
    for (i = 0; i < count; i++)
    {
        if ((handles[i] == (HANDLE)STD_INPUT_HANDLE) ||
            (handles[i] == (HANDLE)STD_OUTPUT_HANDLE) ||
            (handles[i] == (HANDLE)STD_ERROR_HANDLE))
            hloc[i] = GetStdHandle( HandleToULong(handles[i]) );
        else
            hloc[i] = handles[i];

        /* yes, even screen buffer console handles are waitable, and are
         * handled as a handle to the console itself !!
         */
        if (is_console_handle(hloc[i]))
        {
            if (VerifyConsoleIoHandle(hloc[i]))
                hloc[i] = GetConsoleInputWaitHandle();
        }
    }

    status = NtWaitForMultipleObjects( count, hloc, wait_all, alertable,
                                       get_nt_timeout( &time, timeout ) );

    if (HIWORD(status))  /* is it an error code? */
    {
        SetLastError( RtlNtStatusToDosError(status) );
        status = WAIT_FAILED;
    }
    return status;
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

    status = RtlRegisterWait( phNewWaitObject, hObject, Callback, Context, dwMilliseconds, dwFlags );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           RegisterWaitForSingleObjectEx   (KERNEL32.@)
 */
HANDLE WINAPI RegisterWaitForSingleObjectEx( HANDLE hObject, 
                WAITORTIMERCALLBACK Callback, PVOID Context,
                ULONG dwMilliseconds, ULONG dwFlags ) 
{
    NTSTATUS status;
    HANDLE hNewWaitObject;

    TRACE("%p %p %p %d %d\n",
          hObject,Callback,Context,dwMilliseconds,dwFlags);

    status = RtlRegisterWait( &hNewWaitObject, hObject, Callback, Context, dwMilliseconds, dwFlags );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }
    return hNewWaitObject;
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
 *           UnregisterWaitEx   (KERNEL32.@)
 */
BOOL WINAPI UnregisterWaitEx( HANDLE WaitHandle, HANDLE CompletionEvent ) 
{
    NTSTATUS status;

    TRACE("%p %p\n",WaitHandle, CompletionEvent);

    status = RtlDeregisterWaitEx( WaitHandle, CompletionEvent );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *           SignalObjectAndWait  (KERNEL32.@)
 *
 * Makes it possible to atomically signal any of the synchronization
 * objects (semaphore, mutex, event) and wait on another.
 */
DWORD WINAPI SignalObjectAndWait( HANDLE hObjectToSignal, HANDLE hObjectToWaitOn,
                                  DWORD dwMilliseconds, BOOL bAlertable )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    TRACE("%p %p %d %d\n", hObjectToSignal,
          hObjectToWaitOn, dwMilliseconds, bAlertable);

    status = NtSignalAndWaitForSingleObject( hObjectToSignal, hObjectToWaitOn, bAlertable,
                                             get_nt_timeout( &timeout, dwMilliseconds ) );
    if (HIWORD(status))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        status = WAIT_FAILED;
    }
    return status;
}

/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.@)
 *
 * Initialise a critical section before use.
 *
 * PARAMS
 *  crit [O] Critical section to initialise.
 *
 * RETURNS
 *  Nothing. If the function fails an exception is raised.
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    InitializeCriticalSectionEx( crit, 0, 0 );
}

/***********************************************************************
 *           InitializeCriticalSectionAndSpinCount   (KERNEL32.@)
 *
 * Initialise a critical section with a spin count.
 *
 * PARAMS
 *  crit      [O] Critical section to initialise.
 *  spincount [I] Number of times to spin upon contention.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: Nothing. If the function fails an exception is raised.
 *
 * NOTES
 *  spincount is ignored on uni-processor systems.
 */
BOOL WINAPI InitializeCriticalSectionAndSpinCount( CRITICAL_SECTION *crit, DWORD spincount )
{
    return InitializeCriticalSectionEx( crit, spincount, 0 );
}

/***********************************************************************
 *           InitializeCriticalSectionEx   (KERNEL32.@)
 *
 * Initialise a critical section with a spin count and flags.
 *
 * PARAMS
 *  crit      [O] Critical section to initialise.
 *  spincount [I] Number of times to spin upon contention.
 *  flags     [I] CRITICAL_SECTION_ flags from winbase.h.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: Nothing. If the function fails an exception is raised.
 *
 * NOTES
 *  spincount is ignored on uni-processor systems.
 */
BOOL WINAPI InitializeCriticalSectionEx( CRITICAL_SECTION *crit, DWORD spincount, DWORD flags )
{
    NTSTATUS ret = RtlInitializeCriticalSectionEx( crit, spincount, flags );
    if (ret) RtlRaiseStatus( ret );
    return !ret;
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
 *           CreateEventA    (KERNEL32.@)
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
 *           CreateEventW    (KERNEL32.@)
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
 *           CreateEventExA    (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventExA( SECURITY_ATTRIBUTES *sa, LPCSTR name, DWORD flags, DWORD access )
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
 *           CreateEventExW    (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name, DWORD flags, DWORD access )
{
    HANDLE ret;
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

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

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
 *           OpenEventA    (KERNEL32.@)
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
 *           OpenEventW    (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenEventW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = EVENT_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = inherit ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtOpenEvent( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}

/***********************************************************************
 *           PulseEvent    (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PulseEvent( HANDLE handle )
{
    NTSTATUS status;

    if ((status = NtPulseEvent( handle, NULL )))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           SetEvent    (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEvent( HANDLE handle )
{
    NTSTATUS status;

    if ((status = NtSetEvent( handle, NULL )))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           ResetEvent    (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ResetEvent( HANDLE handle )
{
    NTSTATUS status;

    if ((status = NtResetEvent( handle, NULL )))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           CreateMutexA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    return CreateMutexExA( sa, name, owner ? CREATE_MUTEX_INITIAL_OWNER : 0, MUTEX_ALL_ACCESS );
}


/***********************************************************************
 *           CreateMutexW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexW( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCWSTR name )
{
    return CreateMutexExW( sa, name, owner ? CREATE_MUTEX_INITIAL_OWNER : 0, MUTEX_ALL_ACCESS );
}


/***********************************************************************
 *           CreateMutexExA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexExA( SECURITY_ATTRIBUTES *sa, LPCSTR name, DWORD flags, DWORD access )
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
 *           CreateMutexExW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name, DWORD flags, DWORD access )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateMutant( &ret, access, &attr, (flags & CREATE_MUTEX_INITIAL_OWNER) != 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
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


/***********************************************************************
 *           OpenMutexW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenMutexW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = MUTEX_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = inherit ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtOpenMutant( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}


/***********************************************************************
 *           ReleaseMutex   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReleaseMutex( HANDLE handle )
{
    NTSTATUS    status;

    status = NtReleaseMutant(handle, NULL);
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
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
 *           CreateSemaphoreW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreW( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max, LPCWSTR name )
{
    return CreateSemaphoreExW( sa, initial, max, name, 0, SEMAPHORE_ALL_ACCESS );
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
 *           CreateSemaphoreExW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreExW( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max,
                                                    LPCWSTR name, DWORD flags, DWORD access )
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateSemaphore( &ret, access, &attr, initial, max );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
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


/***********************************************************************
 *           OpenSemaphoreW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenSemaphoreW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = SEMAPHORE_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = inherit ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtOpenSemaphore( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}


/***********************************************************************
 *           ReleaseSemaphore   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    NTSTATUS status = NtReleaseSemaphore( handle, count, (PULONG)previous );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
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

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

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

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = inherit ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

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
 *           CreateWaitableTimerW    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerW( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCWSTR name )
{
    return CreateWaitableTimerExW( sa, name, manual ? CREATE_WAITABLE_TIMER_MANUAL_RESET : 0,
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
 *           CreateWaitableTimerExW    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name, DWORD flags, DWORD access )
{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateTimer( &handle, access, &attr,
                 (flags & CREATE_WAITABLE_TIMER_MANUAL_RESET) ? NotificationTimer : SynchronizationTimer );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return handle;
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
 *           OpenWaitableTimerW    (KERNEL32.@)
 */
HANDLE WINAPI OpenWaitableTimerW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE handle;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = TIMER_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = inherit ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtOpenTimer(&handle, access, &attr);
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return handle;
}


/***********************************************************************
 *           SetWaitableTimer    (KERNEL32.@)
 */
BOOL WINAPI SetWaitableTimer( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                              PTIMERAPCROUTINE callback, LPVOID arg, BOOL resume )
{
    NTSTATUS status = NtSetTimer(handle, when, (PTIMER_APC_ROUTINE)callback,
                                 arg, resume, period, NULL);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        if (status != STATUS_TIMER_RESUME_IGNORED) return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           SetWaitableTimerEx    (KERNEL32.@)
 */
BOOL WINAPI SetWaitableTimerEx( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                              PTIMERAPCROUTINE callback, LPVOID arg, REASON_CONTEXT *context, ULONG tolerabledelay )
{
    static int once;
    if (!once++)
    {
        FIXME("(%p, %p, %d, %p, %p, %p, %d) semi-stub\n",
              handle, when, period, callback, arg, context, tolerabledelay);
    }
    return SetWaitableTimer(handle, when, period, callback, arg, FALSE);
}

/***********************************************************************
 *           CancelWaitableTimer    (KERNEL32.@)
 */
BOOL WINAPI CancelWaitableTimer( HANDLE handle )
{
    NTSTATUS status;

    status = NtCancelTimer(handle, NULL);
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           CreateTimerQueue  (KERNEL32.@)
 */
HANDLE WINAPI CreateTimerQueue(void)
{
    HANDLE q;
    NTSTATUS status = RtlCreateTimerQueue(&q);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return q;
}


/***********************************************************************
 *           DeleteTimerQueueEx  (KERNEL32.@)
 */
BOOL WINAPI DeleteTimerQueueEx(HANDLE TimerQueue, HANDLE CompletionEvent)
{
    NTSTATUS status = RtlDeleteTimerQueueEx(TimerQueue, CompletionEvent);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           DeleteTimerQueue  (KERNEL32.@)
 */
BOOL WINAPI DeleteTimerQueue(HANDLE TimerQueue)
{
    return DeleteTimerQueueEx(TimerQueue, NULL);
}

/***********************************************************************
 *           CreateTimerQueueTimer  (KERNEL32.@)
 *
 * Creates a timer-queue timer. This timer expires at the specified due
 * time (in ms), then after every specified period (in ms). When the timer
 * expires, the callback function is called.
 *
 * RETURNS
 *   nonzero on success or zero on failure
 */
BOOL WINAPI CreateTimerQueueTimer( PHANDLE phNewTimer, HANDLE TimerQueue,
                                   WAITORTIMERCALLBACK Callback, PVOID Parameter,
                                   DWORD DueTime, DWORD Period, ULONG Flags )
{
    NTSTATUS status = RtlCreateTimer(phNewTimer, TimerQueue, Callback,
                                     Parameter, DueTime, Period, Flags);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           ChangeTimerQueueTimer  (KERNEL32.@)
 *
 * Changes the times at which the timer expires.
 *
 * RETURNS
 *   nonzero on success or zero on failure
 */
BOOL WINAPI ChangeTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer,
                                   ULONG DueTime, ULONG Period )
{
    NTSTATUS status = RtlUpdateTimer(TimerQueue, Timer, DueTime, Period);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           DeleteTimerQueueTimer  (KERNEL32.@)
 *
 * Cancels a timer-queue timer.
 *
 * RETURNS
 *   nonzero on success or zero on failure
 */
BOOL WINAPI DeleteTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer,
                                   HANDLE CompletionEvent )
{
    NTSTATUS status = RtlDeleteTimer(TimerQueue, Timer, CompletionEvent);
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
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
 *           CreateNamedPipeW   (KERNEL32.@)
 */
HANDLE WINAPI CreateNamedPipeW( LPCWSTR name, DWORD dwOpenMode,
                                DWORD dwPipeMode, DWORD nMaxInstances,
                                DWORD nOutBufferSize, DWORD nInBufferSize,
                                DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE handle;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    DWORD access, options, sharing;
    BOOLEAN pipe_type, read_mode, non_block;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER timeout;

    TRACE("(%s, %#08x, %#08x, %d, %d, %d, %d, %p)\n",
          debugstr_w(name), dwOpenMode, dwPipeMode, nMaxInstances,
          nOutBufferSize, nInBufferSize, nDefaultTimeOut, sa );

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
    attr.Attributes               = OBJ_CASE_INSENSITIVE |
                                    ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    switch(dwOpenMode & 3)
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
    if (dwOpenMode & WRITE_DAC) access |= WRITE_DAC;
    if (dwOpenMode & WRITE_OWNER) access |= WRITE_OWNER;
    if (dwOpenMode & ACCESS_SYSTEM_SECURITY) access |= ACCESS_SYSTEM_SECURITY;
    if (dwOpenMode & FILE_FLAG_WRITE_THROUGH) options |= FILE_WRITE_THROUGH;
    if (!(dwOpenMode & FILE_FLAG_OVERLAPPED)) options |= FILE_SYNCHRONOUS_IO_NONALERT;
    pipe_type = (dwPipeMode & PIPE_TYPE_MESSAGE) != 0;
    read_mode = (dwPipeMode & PIPE_READMODE_MESSAGE) != 0;
    non_block = (dwPipeMode & PIPE_NOWAIT) != 0;
    if (nMaxInstances >= PIPE_UNLIMITED_INSTANCES) nMaxInstances = ~0U;

    timeout.QuadPart = (ULONGLONG)nDefaultTimeOut * -10000;

    SetLastError(0);

    status = NtCreateNamedPipeFile(&handle, access, &attr, &iosb, sharing,
                                   FILE_OVERWRITE_IF, options, pipe_type,
                                   read_mode, non_block, nMaxInstances,
                                   nInBufferSize, nOutBufferSize, &timeout);

    RtlFreeUnicodeString( &nt_name );
    if (status)
    {
        handle = INVALID_HANDLE_VALUE;
        SetLastError( RtlNtStatusToDosError(status) );
    }
    return handle;
}


/***********************************************************************
 *           PeekNamedPipe   (KERNEL32.@)
 */
BOOL WINAPI PeekNamedPipe( HANDLE hPipe, LPVOID lpvBuffer, DWORD cbBuffer,
                           LPDWORD lpcbRead, LPDWORD lpcbAvail, LPDWORD lpcbMessage )
{
    FILE_PIPE_PEEK_BUFFER local_buffer;
    FILE_PIPE_PEEK_BUFFER *buffer = &local_buffer;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (cbBuffer && !(buffer = HeapAlloc( GetProcessHeap(), 0,
                                          FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[cbBuffer] ))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    status = NtFsControlFile( hPipe, 0, NULL, NULL, &io, FSCTL_PIPE_PEEK, NULL, 0,
                              buffer, FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[cbBuffer] ) );
    if (!status)
    {
        ULONG read_size = io.Information - FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data );
        if (lpcbAvail) *lpcbAvail = buffer->ReadDataAvailable;
        if (lpcbRead) *lpcbRead = read_size;
        if (lpcbMessage) *lpcbMessage = 0;  /* FIXME */
        if (lpvBuffer) memcpy( lpvBuffer, buffer->Data, read_size );
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    if (buffer != &local_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return !status;
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
 *           WaitNamedPipeW   (KERNEL32.@)
 *
 *  Waits for a named pipe instance to become available
 *
 *  PARAMS
 *   name     [I] Pointer to a named pipe name to wait for
 *   nTimeOut [I] How long to wait in ms
 *
 *  RETURNS
 *   TRUE: Success, named pipe can be opened with CreateFile
 *   FALSE: Failure, GetLastError can be called for further details
 */
BOOL WINAPI WaitNamedPipeW (LPCWSTR name, DWORD nTimeOut)
{
    static const WCHAR leadin[] = {'\\','?','?','\\','P','I','P','E','\\'};
    NTSTATUS status;
    UNICODE_STRING nt_name, pipe_dev_name;
    FILE_PIPE_WAIT_FOR_BUFFER *pipe_wait;
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES attr;
    ULONG sz_pipe_wait;
    HANDLE pipe_dev;

    TRACE("%s 0x%08x\n",debugstr_w(name),nTimeOut);

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
        return FALSE;

    if (nt_name.Length >= MAX_PATH * sizeof(WCHAR) ||
        nt_name.Length < sizeof(leadin) ||
        strncmpiW( nt_name.Buffer, leadin, sizeof(leadin)/sizeof(WCHAR)) != 0)
    {
        RtlFreeUnicodeString( &nt_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    sz_pipe_wait = sizeof(*pipe_wait) + nt_name.Length - sizeof(leadin) - sizeof(WCHAR);
    if (!(pipe_wait = HeapAlloc( GetProcessHeap(), 0,  sz_pipe_wait)))
    {
        RtlFreeUnicodeString( &nt_name );
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    pipe_dev_name.Buffer = nt_name.Buffer;
    pipe_dev_name.Length = sizeof(leadin);
    pipe_dev_name.MaximumLength = sizeof(leadin);
    InitializeObjectAttributes(&attr,&pipe_dev_name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtOpenFile( &pipe_dev, FILE_READ_ATTRIBUTES, &attr,
                         &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);
    if (status != ERROR_SUCCESS)
    {
        HeapFree( GetProcessHeap(), 0, pipe_wait);
        RtlFreeUnicodeString( &nt_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    pipe_wait->TimeoutSpecified = !(nTimeOut == NMPWAIT_USE_DEFAULT_WAIT);
    if (nTimeOut == NMPWAIT_WAIT_FOREVER)
        pipe_wait->Timeout.QuadPart = ((ULONGLONG)0x7fffffff << 32) | 0xffffffff;
    else
        pipe_wait->Timeout.QuadPart = (ULONGLONG)nTimeOut * -10000;
    pipe_wait->NameLength = nt_name.Length - sizeof(leadin);
    memcpy(pipe_wait->Name, nt_name.Buffer + sizeof(leadin)/sizeof(WCHAR),
           pipe_wait->NameLength);
    RtlFreeUnicodeString( &nt_name );

    status = NtFsControlFile( pipe_dev, NULL, NULL, NULL, &iosb, FSCTL_PIPE_WAIT,
                              pipe_wait, sz_pipe_wait, NULL, 0 );

    HeapFree( GetProcessHeap(), 0, pipe_wait );
    NtClose( pipe_dev );

    if(status != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    else
        return TRUE;
}


/***********************************************************************
 *           ConnectNamedPipe   (KERNEL32.@)
 *
 *  Connects to a named pipe
 *
 *  Parameters
 *  hPipe: A handle to a named pipe returned by CreateNamedPipe
 *  overlapped: Optional OVERLAPPED struct
 *
 *  Return values
 *  TRUE: Success
 *  FALSE: Failure, GetLastError can be called for further details
 */
BOOL WINAPI ConnectNamedPipe(HANDLE hPipe, LPOVERLAPPED overlapped)
{
    NTSTATUS status;
    IO_STATUS_BLOCK status_block;
    LPVOID   cvalue = NULL;

    TRACE("(%p,%p)\n", hPipe, overlapped);

    if(overlapped)
    {
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
        if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;
    }

    status = NtFsControlFile(hPipe, overlapped ? overlapped->hEvent : NULL, NULL, cvalue,
                             overlapped ? (IO_STATUS_BLOCK *)overlapped : &status_block,
                             FSCTL_PIPE_LISTEN, NULL, 0, NULL, 0);

    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}

/***********************************************************************
 *           DisconnectNamedPipe   (KERNEL32.@)
 *
 *  Disconnects from a named pipe
 *
 *  Parameters
 *  hPipe: A handle to a named pipe returned by CreateNamedPipe
 *
 *  Return values
 *  TRUE: Success
 *  FALSE: Failure, GetLastError can be called for further details
 */
BOOL WINAPI DisconnectNamedPipe(HANDLE hPipe)
{
    NTSTATUS status;
    IO_STATUS_BLOCK io_block;

    TRACE("(%p)\n",hPipe);

    status = NtFsControlFile(hPipe, 0, NULL, NULL, &io_block, FSCTL_PIPE_DISCONNECT,
                             NULL, 0, NULL, 0);
    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}

/***********************************************************************
 *           TransactNamedPipe   (KERNEL32.@)
 *
 * BUGS
 *  should be done as a single operation in the wineserver or kernel
 */
BOOL WINAPI TransactNamedPipe(
    HANDLE handle, LPVOID write_buf, DWORD write_size, LPVOID read_buf,
    DWORD read_size, LPDWORD bytes_read, LPOVERLAPPED overlapped)
{
    BOOL r;
    DWORD count;

    TRACE("%p %p %d %p %d %p %p\n",
          handle, write_buf, write_size, read_buf,
          read_size, bytes_read, overlapped);

    if (overlapped)
    {
        FIXME("Doesn't support overlapped operation as yet\n");
        return FALSE;
    }

    r = WriteFile(handle, write_buf, write_size, &count, NULL);
    if (r)
        r = ReadFile(handle, read_buf, read_size, bytes_read, NULL);

    return r;
}

/***********************************************************************
 *           GetNamedPipeInfo   (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeInfo(
    HANDLE hNamedPipe, LPDWORD lpFlags, LPDWORD lpOutputBufferSize,
    LPDWORD lpInputBufferSize, LPDWORD lpMaxInstances)
{
    FILE_PIPE_LOCAL_INFORMATION fpli;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    status = NtQueryInformationFile(hNamedPipe, &iosb, &fpli, sizeof(fpli),
                                    FilePipeLocalInformation);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    if (lpFlags)
    {
        *lpFlags = (fpli.NamedPipeEnd & FILE_PIPE_SERVER_END) ?
            PIPE_SERVER_END : PIPE_CLIENT_END;
        *lpFlags |= (fpli.NamedPipeType & FILE_PIPE_TYPE_MESSAGE) ?
            PIPE_TYPE_MESSAGE : PIPE_TYPE_BYTE;
    }

    if (lpOutputBufferSize) *lpOutputBufferSize = fpli.OutboundQuota;
    if (lpInputBufferSize) *lpInputBufferSize = fpli.InboundQuota;
    if (lpMaxInstances) *lpMaxInstances = fpli.MaximumInstances;

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
    FIXME("%p %p %p %p %p %p %d\n",
          hNamedPipe, lpState, lpCurInstances,
          lpMaxCollectionCount, lpCollectDataTimeout,
          lpUsername, nUsernameMaxSize);

    return FALSE;
}

/***********************************************************************
 *           GetNamedPipeHandleStateW  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeHandleStateW(
    HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout,
    LPWSTR lpUsername, DWORD nUsernameMaxSize)
{
    FIXME("%p %p %p %p %p %p %d\n",
          hNamedPipe, lpState, lpCurInstances,
          lpMaxCollectionCount, lpCollectDataTimeout,
          lpUsername, nUsernameMaxSize);

    return FALSE;
}

/***********************************************************************
 *           SetNamedPipeHandleState  (KERNEL32.@)
 */
BOOL WINAPI SetNamedPipeHandleState(
    HANDLE hNamedPipe, LPDWORD lpMode, LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout)
{
    /* should be a fixme, but this function is called a lot by the RPC
     * runtime, and it slows down InstallShield a fair bit. */
    WARN("stub: %p %p/%d %p %p\n",
          hNamedPipe, lpMode, lpMode ? *lpMode : 0, lpMaxCollectionCount, lpCollectDataTimeout);
    return FALSE;
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

/***********************************************************************
 *           CallNamedPipeW  (KERNEL32.@)
 */
BOOL WINAPI CallNamedPipeW(
    LPCWSTR lpNamedPipeName, LPVOID lpInput, DWORD lpInputSize,
    LPVOID lpOutput, DWORD lpOutputSize,
    LPDWORD lpBytesRead, DWORD nTimeout)
{
    HANDLE pipe;
    BOOL ret;
    DWORD mode;

    TRACE("%s %p %d %p %d %p %d\n",
          debugstr_w(lpNamedPipeName), lpInput, lpInputSize,
          lpOutput, lpOutputSize, lpBytesRead, nTimeout);

    pipe = CreateFileW(lpNamedPipeName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        ret = WaitNamedPipeW(lpNamedPipeName, nTimeout);
        if (!ret)
            return FALSE;
        pipe = CreateFileW(lpNamedPipeName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (pipe == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    mode = PIPE_READMODE_MESSAGE;
    ret = SetNamedPipeHandleState(pipe, &mode, NULL, NULL);

    /* Currently SetNamedPipeHandleState() is a stub returning FALSE */
    if (ret) FIXME("Now that SetNamedPipeHandleState() is more than a stub, please update CallNamedPipeW\n");
    /*
    if (!ret)
    {
        CloseHandle(pipe);
        return FALSE;
    }*/

    ret = TransactNamedPipe(pipe, lpInput, lpInputSize, lpOutput, lpOutputSize, lpBytesRead, NULL);
    CloseHandle(pipe);
    if (!ret)
        return FALSE;

    return TRUE;
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

    timeout.QuadPart = (ULONGLONG)NMPWAIT_USE_DEFAULT_WAIT * -10000;
    /* generate a unique pipe name (system wide) */
    do
    {
        static const WCHAR nameFmt[] = { '\\','?','?','\\','p','i','p','e',
         '\\','W','i','n','3','2','.','P','i','p','e','s','.','%','0','8','l',
         'u','.','%','0','8','u','\0' };

        snprintfW(name, sizeof(name) / sizeof(name[0]), nameFmt,
                  GetCurrentProcessId(), ++index);
        RtlInitUnicodeString(&nt_name, name);
        status = NtCreateNamedPipeFile(&hr, GENERIC_READ | SYNCHRONIZE, &attr, &iosb,
                                       FILE_SHARE_WRITE, FILE_OVERWRITE_IF,
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

    status = NtOpenFile(&hw, GENERIC_WRITE | SYNCHRONIZE, &attr, &iosb, 0,
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
 *		CreateIoCompletionPort (KERNEL32.@)
 */
HANDLE WINAPI CreateIoCompletionPort(HANDLE hFileHandle, HANDLE hExistingCompletionPort,
                                     ULONG_PTR CompletionKey, DWORD dwNumberOfConcurrentThreads)
{
    NTSTATUS status;
    HANDLE ret = 0;

    TRACE("(%p, %p, %08lx, %08x)\n",
          hFileHandle, hExistingCompletionPort, CompletionKey, dwNumberOfConcurrentThreads);

    if (hExistingCompletionPort && hFileHandle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (hExistingCompletionPort)
        ret = hExistingCompletionPort;
    else
    {
        status = NtCreateIoCompletion( &ret, IO_COMPLETION_ALL_ACCESS, NULL, dwNumberOfConcurrentThreads );
        if (status != STATUS_SUCCESS) goto fail;
    }

    if (hFileHandle != INVALID_HANDLE_VALUE)
    {
        FILE_COMPLETION_INFORMATION info;
        IO_STATUS_BLOCK iosb;

        info.CompletionPort = ret;
        info.CompletionKey = CompletionKey;
        status = NtSetInformationFile( hFileHandle, &iosb, &info, sizeof(info), FileCompletionInformation );
        if (status != STATUS_SUCCESS) goto fail;
    }

    return ret;

fail:
    if (ret && !hExistingCompletionPort)
        CloseHandle( ret );
    SetLastError( RtlNtStatusToDosError(status) );
    return 0;
}

/******************************************************************************
 *		GetQueuedCompletionStatus (KERNEL32.@)
 */
BOOL WINAPI GetQueuedCompletionStatus( HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred,
                                       PULONG_PTR pCompletionKey, LPOVERLAPPED *lpOverlapped,
                                       DWORD dwMilliseconds )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER wait_time;

    TRACE("(%p,%p,%p,%p,%d)\n",
          CompletionPort,lpNumberOfBytesTransferred,pCompletionKey,lpOverlapped,dwMilliseconds);

    *lpOverlapped = NULL;

    status = NtRemoveIoCompletion( CompletionPort, pCompletionKey, (PULONG_PTR)lpOverlapped,
                                   &iosb, get_nt_timeout( &wait_time, dwMilliseconds ) );
    if (status == STATUS_SUCCESS)
    {
        *lpNumberOfBytesTransferred = iosb.Information;
        if (iosb.u.Status >= 0) return TRUE;
        SetLastError( RtlNtStatusToDosError(iosb.u.Status) );
        return FALSE;
    }

    if (status == STATUS_TIMEOUT) SetLastError( WAIT_TIMEOUT );
    else SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/******************************************************************************
 *		PostQueuedCompletionStatus (KERNEL32.@)
 */
BOOL WINAPI PostQueuedCompletionStatus( HANDLE CompletionPort, DWORD dwNumberOfBytes,
                                        ULONG_PTR dwCompletionKey, LPOVERLAPPED lpOverlapped)
{
    NTSTATUS status;

    TRACE("%p %d %08lx %p\n", CompletionPort, dwNumberOfBytes, dwCompletionKey, lpOverlapped );

    status = NtSetIoCompletion( CompletionPort, dwCompletionKey, (ULONG_PTR)lpOverlapped,
                                STATUS_SUCCESS, dwNumberOfBytes );

    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
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

/***********************************************************************
 *           SleepConditionVariableCS   (KERNEL32.@)
 */
BOOL WINAPI SleepConditionVariableCS( CONDITION_VARIABLE *variable, CRITICAL_SECTION *crit, DWORD timeout )
{
    NTSTATUS status;
    LARGE_INTEGER time;

    status = RtlSleepConditionVariableCS( variable, crit, get_nt_timeout( &time, timeout ) );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           SleepConditionVariableSRW   (KERNEL32.@)
 */
BOOL WINAPI SleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable, RTL_SRWLOCK *lock, DWORD timeout, ULONG flags )
{
    NTSTATUS status;
    LARGE_INTEGER time;

    status = RtlSleepConditionVariableSRW( variable, lock, get_nt_timeout( &time, timeout ), flags );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

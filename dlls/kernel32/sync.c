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
#include "wine/winbase16.h"
#include "kernel_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

/* check if current version is NT or Win95 */
static inline int is_version_nt(void)
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
        if (InterlockedCompareExchangePointer( (PVOID)&handle, dir, 0 ) != 0)
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
VOID WINAPI Sleep( DWORD timeout )
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
            if (!VerifyConsoleIoHandle(hloc[i]))
            {
                return FALSE;
            }
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
 *           WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WaitForSingleObject16( HANDLE handle, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForSingleObject( handle, timeout );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WaitForMultipleObjects16( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL.495)
 */
DWORD WINAPI WaitForMultipleObjectsEx16( DWORD count, const HANDLE *handles,
                                         BOOL wait_all, DWORD timeout, BOOL alertable )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, alertable );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           RegisterWaitForSingleObject   (KERNEL32.@)
 */
BOOL WINAPI RegisterWaitForSingleObject(PHANDLE phNewWaitObject, HANDLE hObject,
                WAITORTIMERCALLBACK Callback, PVOID Context,
                ULONG dwMilliseconds, ULONG dwFlags)
{
    FIXME("%p %p %p %p %d %d\n",
          phNewWaitObject,hObject,Callback,Context,dwMilliseconds,dwFlags);
    return FALSE;
}

/***********************************************************************
 *           RegisterWaitForSingleObjectEx   (KERNEL32.@)
 */
HANDLE WINAPI RegisterWaitForSingleObjectEx( HANDLE hObject, 
                WAITORTIMERCALLBACK Callback, PVOID Context,
                ULONG dwMilliseconds, ULONG dwFlags ) 
{
    FIXME("%p %p %p %d %d\n",
          hObject,Callback,Context,dwMilliseconds,dwFlags);
    return 0;
}

/***********************************************************************
 *           UnregisterWait   (KERNEL32.@)
 */
BOOL WINAPI UnregisterWait( HANDLE WaitHandle ) 
{
    FIXME("%p\n",WaitHandle);
    return FALSE;
}

/***********************************************************************
 *           UnregisterWaitEx   (KERNEL32.@)
 */
BOOL WINAPI UnregisterWaitEx( HANDLE WaitHandle, HANDLE CompletionEvent ) 
{
    FIXME("%p %p\n",WaitHandle, CompletionEvent);
    return FALSE;
}

/***********************************************************************
 *           SignalObjectAndWait  (KERNEL32.@)
 *
 * Allows to atomically signal any of the synchro objects (semaphore,
 * mutex, event) and wait on another.
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
    NTSTATUS ret = RtlInitializeCriticalSection( crit );
    if (ret) RtlRaiseStatus( ret );
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
    NTSTATUS ret = RtlInitializeCriticalSectionAndSpinCount( crit, spincount );
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
HANDLE WINAPI CreateEventA( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                            BOOL initial_state, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateEventW( sa, manual_reset, initial_state, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateEventW( sa, manual_reset, initial_state, buffer );
}


/***********************************************************************
 *           CreateEventW    (KERNEL32.@)
 */
HANDLE WINAPI CreateEventW( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                            BOOL initial_state, LPCWSTR name )
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
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                                    ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateEvent( &ret, EVENT_ALL_ACCESS, &attr, manual_reset, initial_state );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           CreateW32Event    (KERNEL.457)
 */
HANDLE WINAPI WIN16_CreateEvent( BOOL manual_reset, BOOL initial_state )
{
    return CreateEventW( NULL, manual_reset, initial_state, NULL );
}


/***********************************************************************
 *           OpenEventA    (KERNEL32.@)
 */
HANDLE WINAPI OpenEventA( DWORD access, BOOL inherit, LPCSTR name )
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
HANDLE WINAPI OpenEventW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = EVENT_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | (inherit ? OBJ_INHERIT : 0);
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
BOOL WINAPI PulseEvent( HANDLE handle )
{
    NTSTATUS status;

    if ((status = NtPulseEvent( handle, NULL )))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           SetW32Event (KERNEL.458)
 *           SetEvent    (KERNEL32.@)
 */
BOOL WINAPI SetEvent( HANDLE handle )
{
    NTSTATUS status;

    if ((status = NtSetEvent( handle, NULL )))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           ResetW32Event (KERNEL.459)
 *           ResetEvent    (KERNEL32.@)
 */
BOOL WINAPI ResetEvent( HANDLE handle )
{
    NTSTATUS status;

    if ((status = NtResetEvent( handle, NULL )))
        SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 * NOTE: The Win95 VWin32_Event routines given below are really low-level
 *       routines implemented directly by VWin32. The user-mode libraries
 *       implement Win32 synchronisation routines on top of these low-level
 *       primitives. We do it the other way around here :-)
 */

/***********************************************************************
 *       VWin32_EventCreate	(KERNEL.442)
 */
HANDLE WINAPI VWin32_EventCreate(VOID)
{
    HANDLE hEvent = CreateEventW( NULL, FALSE, 0, NULL );
    return ConvertToGlobalHandle( hEvent );
}

/***********************************************************************
 *       VWin32_EventDestroy	(KERNEL.443)
 */
VOID WINAPI VWin32_EventDestroy(HANDLE event)
{
    CloseHandle( event );
}

/***********************************************************************
 *       VWin32_EventWait	(KERNEL.450)
 */
VOID WINAPI VWin32_EventWait(HANDLE event)
{
    DWORD mutex_count;

    ReleaseThunkLock( &mutex_count );
    WaitForSingleObject( event, INFINITE );
    RestoreThunkLock( mutex_count );
}

/***********************************************************************
 *       VWin32_EventSet	(KERNEL.451)
 *       KERNEL_479             (KERNEL.479)
 */
VOID WINAPI VWin32_EventSet(HANDLE event)
{
    SetEvent( event );
}



/***********************************************************************
 *           CreateMutexA   (KERNEL32.@)
 */
HANDLE WINAPI CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateMutexW( sa, owner, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateMutexW( sa, owner, buffer );
}


/***********************************************************************
 *           CreateMutexW   (KERNEL32.@)
 */
HANDLE WINAPI CreateMutexW( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                                    ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateMutant( &ret, MUTEX_ALL_ACCESS, &attr, owner );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           OpenMutexA   (KERNEL32.@)
 */
HANDLE WINAPI OpenMutexA( DWORD access, BOOL inherit, LPCSTR name )
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
HANDLE WINAPI OpenMutexW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = MUTEX_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | (inherit ? OBJ_INHERIT : 0);
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
BOOL WINAPI ReleaseMutex( HANDLE handle )
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
HANDLE WINAPI CreateSemaphoreA( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateSemaphoreW( sa, initial, max, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateSemaphoreW( sa, initial, max, buffer );
}


/***********************************************************************
 *           CreateSemaphoreW   (KERNEL32.@)
 */
HANDLE WINAPI CreateSemaphoreW( SECURITY_ATTRIBUTES *sa, LONG initial,
                                LONG max, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                                    ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateSemaphore( &ret, SEMAPHORE_ALL_ACCESS, &attr, initial, max );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           OpenSemaphoreA   (KERNEL32.@)
 */
HANDLE WINAPI OpenSemaphoreA( DWORD access, BOOL inherit, LPCSTR name )
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
HANDLE WINAPI OpenSemaphoreW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = SEMAPHORE_ALL_ACCESS;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | (inherit ? OBJ_INHERIT : 0);
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
BOOL WINAPI ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    NTSTATUS status = NtReleaseSemaphore( handle, count, (PULONG)previous );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/*
 * Timers
 */


/***********************************************************************
 *           CreateWaitableTimerA    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerA( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWaitableTimerW( sa, manual, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWaitableTimerW( sa, manual, buffer );
}


/***********************************************************************
 *           CreateWaitableTimerW    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerW( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCWSTR name )
{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                                    ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateTimer(&handle, TIMER_ALL_ACCESS, &attr,
                           manual ? NotificationTimer : SynchronizationTimer);
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
    attr.Attributes               = OBJ_CASE_INSENSITIVE | (inherit ? OBJ_INHERIT : 0);
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
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


/***********************************************************************
 *           DeleteTimerQueueEx  (KERNEL32.@)
 */
BOOL WINAPI DeleteTimerQueueEx(HANDLE TimerQueue, HANDLE CompletionEvent)
{
    FIXME("(%p, %p): stub\n", TimerQueue, CompletionEvent);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *           CreateTimerQueueTimer  (KERNEL32.@)
 *
 * Creates a timer-queue timer. This timer expires at the specified due
 * time (in ms), then after every specified period (in ms). When the timer
 * expires, the callback function is called.
 *
 * RETURNS
 *   nonzero on success or zero on faillure
 *
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI CreateTimerQueueTimer( PHANDLE phNewTimer, HANDLE TimerQueue,
                                   WAITORTIMERCALLBACK Callback, PVOID Parameter,
                                   DWORD DueTime, DWORD Period, ULONG Flags )
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TRUE;
}

/***********************************************************************
 *           DeleteTimerQueueTimer  (KERNEL32.@)
 *
 * Cancels a timer-queue timer.
 *
 * RETURNS
 *   nonzero on success or zero on faillure
 *
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI DeleteTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer,
                                   HANDLE CompletionEvent )
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
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
    DWORD access, options;
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
        options = FILE_PIPE_INBOUND;
        access  = GENERIC_READ;
        break;
    case PIPE_ACCESS_OUTBOUND:
        options = FILE_PIPE_OUTBOUND;
        access  = GENERIC_WRITE;
        break;
    case PIPE_ACCESS_DUPLEX:
        options = FILE_PIPE_FULL_DUPLEX;
        access  = GENERIC_READ | GENERIC_WRITE;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }
    access |= SYNCHRONIZE;
    if (dwOpenMode & FILE_FLAG_WRITE_THROUGH) options |= FILE_WRITE_THROUGH;
    if (!(dwOpenMode & FILE_FLAG_OVERLAPPED)) options |= FILE_SYNCHRONOUS_IO_ALERT;
    pipe_type = (dwPipeMode & PIPE_TYPE_MESSAGE) ? TRUE : FALSE;
    read_mode = (dwPipeMode & PIPE_READMODE_MESSAGE) ? TRUE : FALSE;
    non_block = (dwPipeMode & PIPE_NOWAIT) ? TRUE : FALSE;
    if (nMaxInstances >= PIPE_UNLIMITED_INSTANCES) nMaxInstances = ~0U;

    timeout.QuadPart = (ULONGLONG)nDefaultTimeOut * -10000;

    SetLastError(0);

    status = NtCreateNamedPipeFile(&handle, access, &attr, &iosb, 0,
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
        return 0;
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
        strncmpiW( nt_name.Buffer, leadin, sizeof(leadin)/sizeof(WCHAR) != 0))
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

    TRACE("(%p,%p)\n", hPipe, overlapped);

    if(overlapped)
        overlapped->Internal = STATUS_PENDING;

    status = NtFsControlFile(hPipe, overlapped ? overlapped->hEvent : NULL, NULL, NULL,
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
    HANDLE handle, LPVOID lpInput, DWORD dwInputSize, LPVOID lpOutput,
    DWORD dwOutputSize, LPDWORD lpBytesRead, LPOVERLAPPED lpOverlapped)
{
    BOOL r;
    DWORD count;

    TRACE("%p %p %d %p %d %p %p\n",
          handle, lpInput, dwInputSize, lpOutput,
          dwOutputSize, lpBytesRead, lpOverlapped);

    if (lpOverlapped)
    {
        FIXME("Doesn't support overlapped operation as yet\n");
        return FALSE;
    }

    r = WriteFile(handle, lpOutput, dwOutputSize, &count, NULL);
    if (r)
        r = ReadFile(handle, lpInput, dwInputSize, lpBytesRead, NULL);

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
    if (!ret)
    {
        CloseHandle(pipe);
        return FALSE;
    }

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
                                       0, FILE_OVERWRITE_IF,
                                       FILE_SYNCHRONOUS_IO_ALERT | FILE_PIPE_INBOUND,
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
                        FILE_SYNCHRONOUS_IO_ALERT | FILE_NON_DIRECTORY_FILE);

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
 * See CreatMailslotW.
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
    FIXME("(%p, %p, %08lx, %08x): stub.\n",
          hFileHandle, hExistingCompletionPort, CompletionKey, dwNumberOfConcurrentThreads);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


/******************************************************************************
 *		GetQueuedCompletionStatus (KERNEL32.@)
 */
BOOL WINAPI GetQueuedCompletionStatus( HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred,
                                       PULONG_PTR pCompletionKey, LPOVERLAPPED *lpOverlapped,
                                       DWORD dwMilliseconds )
{
    FIXME("(%p,%p,%p,%p,%d), stub!\n",
          CompletionPort,lpNumberOfBytesTransferred,pCompletionKey,lpOverlapped,dwMilliseconds);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI PostQueuedCompletionStatus( HANDLE CompletionPort, DWORD dwNumberOfBytes,
                                        ULONG_PTR dwCompletionKey, LPOVERLAPPED lpOverlapped)
{
    FIXME("%p %d %08lx %p\n", CompletionPort, dwNumberOfBytes, dwCompletionKey, lpOverlapped );
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *		BindIoCompletionCallback (KERNEL32.@)
 */
BOOL WINAPI BindIoCompletionCallback( HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags)
{
    FIXME("%p, %p, %d, stub!\n", FileHandle, Function, Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *		CreateJobObjectW (KERNEL32.@)
 */
HANDLE WINAPI CreateJobObjectW( LPSECURITY_ATTRIBUTES attr, LPCWSTR name )
{
    FIXME("%p %s\n", attr, debugstr_w(name) );
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/******************************************************************************
 *		CreateJobObjectA (KERNEL32.@)
 */
HANDLE WINAPI CreateJobObjectA( LPSECURITY_ATTRIBUTES attr, LPCSTR name )
{
    LPWSTR str = NULL;
    UINT len;
    HANDLE r;

    TRACE("%p %s\n", attr, debugstr_a(name) );

    if( name )
    {
        len = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
        str = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !str )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
        len = MultiByteToWideChar( CP_ACP, 0, name, -1, str, len );
    }

    r = CreateJobObjectW( attr, str );

    HeapFree( GetProcessHeap(), 0, str );

    return r;
}

/******************************************************************************
 *		AssignProcessToJobObject (KERNEL32.@)
 */
BOOL WINAPI AssignProcessToJobObject( HANDLE hJob, HANDLE hProcess )
{
    FIXME("%p %p\n", hJob, hProcess);
    return TRUE;
}

#ifdef __i386__

/***********************************************************************
 *		InterlockedCompareExchange (KERNEL32.@)
 */
/* LONG WINAPI InterlockedCompareExchange( PLONG dest, LONG xchg, LONG compare ); */
__ASM_GLOBAL_FUNC(InterlockedCompareExchange,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret $12")

/***********************************************************************
 *		InterlockedExchange (KERNEL32.@)
 */
/* LONG WINAPI InterlockedExchange( PLONG dest, LONG val ); */
__ASM_GLOBAL_FUNC(InterlockedExchange,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret $8")

/***********************************************************************
 *		InterlockedExchangeAdd (KERNEL32.@)
 */
/* LONG WINAPI InterlockedExchangeAdd( PLONG dest, LONG incr ); */
__ASM_GLOBAL_FUNC(InterlockedExchangeAdd,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret $8")

/***********************************************************************
 *		InterlockedIncrement (KERNEL32.@)
 */
/* LONG WINAPI InterlockedIncrement( PLONG dest ); */
__ASM_GLOBAL_FUNC(InterlockedIncrement,
                  "movl 4(%esp),%edx\n\t"
                  "movl $1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "incl %eax\n\t"
                  "ret $4")

/***********************************************************************
 *		InterlockedDecrement (KERNEL32.@)
 */
__ASM_GLOBAL_FUNC(InterlockedDecrement,
                  "movl 4(%esp),%edx\n\t"
                  "movl $-1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "decl %eax\n\t"
                  "ret $4")

#else  /* __i386__ */

/***********************************************************************
 *		InterlockedCompareExchange (KERNEL32.@)
 *
 * Atomically swap one value with another.
 *
 * PARAMS
 *  dest    [I/O] The value to replace
 *  xchq    [I]   The value to be swapped
 *  compare [I]   The value to compare to dest
 *
 * RETURNS
 *  The resulting value of dest.
 *
 * NOTES
 *  dest is updated only if it is equal to compare, otherwise no swap is done.
 */
LONG WINAPI InterlockedCompareExchange( LONG volatile *dest, LONG xchg, LONG compare )
{
    return interlocked_cmpxchg( (int *)dest, xchg, compare );
}

/***********************************************************************
 *		InterlockedExchange (KERNEL32.@)
 *
 * Atomically swap one value with another.
 *
 * PARAMS
 *  dest [I/O] The value to replace
 *  val  [I]   The value to be swapped
 *
 * RETURNS
 *  The resulting value of dest.
 */
LONG WINAPI InterlockedExchange( LONG volatile *dest, LONG val )
{
    return interlocked_xchg( (int *)dest, val );
}

/***********************************************************************
 *		InterlockedExchangeAdd (KERNEL32.@)
 *
 * Atomically add one value to another.
 *
 * PARAMS
 *  dest [I/O] The value to add to
 *  incr [I]   The value to be added
 *
 * RETURNS
 *  The resulting value of dest.
 */
LONG WINAPI InterlockedExchangeAdd( LONG volatile *dest, LONG incr )
{
    return interlocked_xchg_add( (int *)dest, incr );
}

/***********************************************************************
 *		InterlockedIncrement (KERNEL32.@)
 *
 * Atomically increment a value.
 *
 * PARAMS
 *  dest [I/O] The value to increment
 *
 * RETURNS
 *  The resulting value of dest.
 */
LONG WINAPI InterlockedIncrement( LONG volatile *dest )
{
    return interlocked_xchg_add( (int *)dest, 1 ) + 1;
}

/***********************************************************************
 *		InterlockedDecrement (KERNEL32.@)
 *
 * Atomically decrement a value.
 *
 * PARAMS
 *  dest [I/O] The value to decrement
 *
 * RETURNS
 *  The resulting value of dest.
 */
LONG WINAPI InterlockedDecrement( LONG volatile *dest )
{
    return interlocked_xchg_add( (int *)dest, -1 ) - 1;
}

#endif  /* __i386__ */

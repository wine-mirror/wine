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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

#include "wine/server.h"
#include "wine/unicode.h"
#include "file.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);

/* check if current version is NT or Win95 */
inline static int is_version_nt(void)
{
    return !(GetVersion() & 0x80000000);
}


/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.@)
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    NTSTATUS ret = RtlInitializeCriticalSection( crit );
    if (ret) RtlRaiseStatus( ret );
}

/***********************************************************************
 *           InitializeCriticalSectionAndSpinCount   (KERNEL32.@)
 */
BOOL WINAPI InitializeCriticalSectionAndSpinCount( CRITICAL_SECTION *crit, DWORD spincount )
{
    NTSTATUS ret = RtlInitializeCriticalSectionAndSpinCount( crit, spincount );
    if (ret) RtlRaiseStatus( ret );
    return !ret;
}

/***********************************************************************
 *           SetCriticalSectionSpinCount   (KERNEL32.@)
 * This function is available on NT4SP3 or later, but not Win98
 * It is SMP related
 */
DWORD WINAPI SetCriticalSectionSpinCount( CRITICAL_SECTION *crit, DWORD spincount )
{
    ULONG_PTR oldspincount = crit->SpinCount;
    if(spincount) FIXME("critsection=%p: spincount=%ld not supported\n", crit, spincount);
    crit->SpinCount = spincount;
    return oldspincount;
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
    if (crit->DebugInfo)
    {
        RtlFreeHeap( GetProcessHeap(), 0, crit->DebugInfo );
        crit->DebugInfo = NULL;
    }
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.@)
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    if ( !crit->LockSemaphore )
        RtlInitializeCriticalSection( crit );
}


/***********************************************************************
 *           UninitializeCriticalSection   (KERNEL32.@)
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
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    /* one buggy program needs this
     * ("Van Dale Groot woordenboek der Nederlandse taal")
     */
    if (sa && IsBadReadPtr(sa,sizeof(SECURITY_ATTRIBUTES)))
    {
        ERR("Bad security attributes pointer %p\n",sa);
        SetLastError( ERROR_INVALID_PARAMETER);
        return 0;
    }
    SERVER_START_REQ( create_event )
    {
        req->manual_reset = manual_reset;
        req->initial_state = initial_state;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        SetLastError(0);
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           CreateW32Event    (KERNEL.457)
 */
HANDLE WINAPI WIN16_CreateEvent( BOOL manual_reset, BOOL initial_state )
{
    return CreateEventA( NULL, manual_reset, initial_state, NULL );
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
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    if (!is_version_nt()) access = EVENT_ALL_ACCESS;

    SERVER_START_REQ( open_event )
    {
        req->access  = access;
        req->inherit = inherit;
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           EVENT_Operation
 *
 * Execute an event operation (set,reset,pulse).
 */
static BOOL EVENT_Operation( HANDLE handle, enum event_op op )
{
    BOOL ret;
    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = op;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           PulseEvent    (KERNEL32.@)
 */
BOOL WINAPI PulseEvent( HANDLE handle )
{
    return EVENT_Operation( handle, PULSE_EVENT );
}


/***********************************************************************
 *           SetW32Event (KERNEL.458)
 *           SetEvent    (KERNEL32.@)
 */
BOOL WINAPI SetEvent( HANDLE handle )
{
    return EVENT_Operation( handle, SET_EVENT );
}


/***********************************************************************
 *           ResetW32Event (KERNEL.459)
 *           ResetEvent    (KERNEL32.@)
 */
BOOL WINAPI ResetEvent( HANDLE handle )
{
    return EVENT_Operation( handle, RESET_EVENT );
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
    HANDLE hEvent = CreateEventA( NULL, FALSE, 0, NULL );
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
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( create_mutex )
    {
        req->owned   = owner;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        SetLastError(0);
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
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
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    if (!is_version_nt()) access = MUTEX_ALL_ACCESS;

    SERVER_START_REQ( open_mutex )
    {
        req->access  = access;
        req->inherit = inherit;
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           ReleaseMutex   (KERNEL32.@)
 */
BOOL WINAPI ReleaseMutex( HANDLE handle )
{
    BOOL ret;
    SERVER_START_REQ( release_mutex )
    {
        req->handle = handle;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
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
    DWORD len = name ? strlenW(name) : 0;

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }

    SERVER_START_REQ( create_semaphore )
    {
        req->initial = (unsigned int)initial;
        req->max     = (unsigned int)max;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        SetLastError(0);
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
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
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    if (!is_version_nt()) access = SEMAPHORE_ALL_ACCESS;

    SERVER_START_REQ( open_semaphore )
    {
        req->access  = access;
        req->inherit = inherit;
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           ReleaseSemaphore   (KERNEL32.@)
 */
BOOL WINAPI ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    NTSTATUS status = NtReleaseSemaphore( handle, count, previous );
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
    HANDLE              handle;
    NTSTATUS            status;
    UNICODE_STRING      us;
    DWORD               attr = 0;
    OBJECT_ATTRIBUTES   oa;

    if (name) RtlInitUnicodeString(&us, name);
    if (sa && (sa->nLength >= sizeof(*sa)) && sa->bInheritHandle)
        attr |= OBJ_INHERIT;
    InitializeObjectAttributes(&oa, name ? &us : NULL, attr,
                               NULL /* FIXME */, NULL /* FIXME */);
    status = NtCreateTimer(&handle, TIMER_ALL_ACCESS, &oa,
                           manual ? NotificationTimer : SynchronizationTimer);

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
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
    NTSTATUS            status;
    ULONG               attr = 0;
    UNICODE_STRING      us;
    HANDLE              handle;
    OBJECT_ATTRIBUTES   oa;

    if (inherit) attr |= OBJ_INHERIT;

    if (name) RtlInitUnicodeString(&us, name);
    InitializeObjectAttributes(&oa, name ? &us : NULL, attr, NULL /* FIXME */, NULL /* FIXME */);
    status = NtOpenTimer(&handle, access, &oa);
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
    NTSTATUS status = NtSetTimer(handle, when, callback, arg, resume, period, NULL);

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
                                DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES attr )
{
    HANDLE ret;
    DWORD len;
    static const WCHAR leadin[] = {'\\','\\','.','\\','P','I','P','E','\\'};

    TRACE("(%s, %#08lx, %#08lx, %ld, %ld, %ld, %ld, %p)\n",
          debugstr_w(name), dwOpenMode, dwPipeMode, nMaxInstances,
          nOutBufferSize, nInBufferSize, nDefaultTimeOut, attr );

    if (!name)
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }
    len = strlenW(name);
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return INVALID_HANDLE_VALUE;
    }
    if (strncmpiW(name, leadin, sizeof(leadin)/sizeof(leadin[0])))
    {
        SetLastError( ERROR_INVALID_NAME );
        return INVALID_HANDLE_VALUE;
    }
    SERVER_START_REQ( create_named_pipe )
    {
        req->openmode = dwOpenMode;
        req->pipemode = dwPipeMode;
        req->maxinstances = nMaxInstances;
        req->outsize = nOutBufferSize;
        req->insize = nInBufferSize;
        req->timeout = nDefaultTimeOut;
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        SetLastError(0);
        if (!wine_server_call_err( req )) ret = reply->handle;
        else ret = INVALID_HANDLE_VALUE;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           PeekNamedPipe   (KERNEL32.@)
 */
BOOL WINAPI PeekNamedPipe( HANDLE hPipe, LPVOID lpvBuffer, DWORD cbBuffer,
                           LPDWORD lpcbRead, LPDWORD lpcbAvail, LPDWORD lpcbMessage )
{
#ifdef FIONREAD
    int avail=0,fd;

    fd = FILE_GetUnixHandle(hPipe, GENERIC_READ);
    if (fd == -1) return FALSE;

    if (ioctl(fd,FIONREAD, &avail ) != 0)
    {
        TRACE("FIONREAD failed reason: %s\n",strerror(errno));
        close(fd);
        return FALSE;
    }
    if (!avail)  /* check for closed pipe */
    {
        struct pollfd pollfd;
        pollfd.fd = fd;
        pollfd.events = POLLIN;
        pollfd.revents = 0;
        switch (poll( &pollfd, 1, 0 ))
        {
        case 0:
            break;
        case 1:  /* got something */
            if (!(pollfd.revents & (POLLHUP | POLLERR))) break;
            TRACE("POLLHUP | POLLERR\n");
            /* fall through */
        case -1:
            close(fd);
            SetLastError(ERROR_BROKEN_PIPE);
            return FALSE;
        }
    }
    close(fd);
    TRACE(" 0x%08x bytes available\n", avail );
    if (!lpvBuffer && lpcbAvail)
      {
	*lpcbAvail= avail;
	return TRUE;
      }
#endif /* defined(FIONREAD) */

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    FIXME("function not implemented\n");
    return FALSE;
}

/***********************************************************************
 *           SYNC_CompletePipeOverlapped   (Internal)
 */
static void SYNC_CompletePipeOverlapped (LPOVERLAPPED overlapped, DWORD result)
{
    TRACE("for %p result %08lx\n",overlapped,result);
    if(!overlapped)
        return;
    overlapped->Internal = result;
    SetEvent(overlapped->hEvent);
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
 */
BOOL WINAPI WaitNamedPipeW (LPCWSTR name, DWORD nTimeOut)
{
    DWORD len = name ? strlenW(name) : 0;
    BOOL ret;
    OVERLAPPED ov;

    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return FALSE;
    }

    TRACE("%s 0x%08lx\n",debugstr_w(name),nTimeOut);

    memset(&ov,0,sizeof(ov));
    ov.hEvent = CreateEventA( NULL, 0, 0, NULL );
    if (!ov.hEvent)
        return FALSE;

    SERVER_START_REQ( wait_named_pipe )
    {
        req->timeout = nTimeOut;
        req->overlapped = &ov;
        req->func = SYNC_CompletePipeOverlapped;
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if(ret)
    {
        if (WAIT_OBJECT_0==WaitForSingleObject(ov.hEvent,INFINITE))
        {
            SetLastError(ov.Internal);
            ret = (ov.Internal==STATUS_SUCCESS);
        }
    }
    CloseHandle(ov.hEvent);
    return ret;
}


/***********************************************************************
 *           SYNC_ConnectNamedPipe   (Internal)
 */
static BOOL SYNC_ConnectNamedPipe(HANDLE hPipe, LPOVERLAPPED overlapped)
{
    BOOL ret;

    if(!overlapped)
        return FALSE;

    overlapped->Internal = STATUS_PENDING;

    SERVER_START_REQ( connect_named_pipe )
    {
        req->handle = hPipe;
        req->overlapped = overlapped;
        req->func = SYNC_CompletePipeOverlapped;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    return ret;
}

/***********************************************************************
 *           ConnectNamedPipe   (KERNEL32.@)
 */
BOOL WINAPI ConnectNamedPipe(HANDLE hPipe, LPOVERLAPPED overlapped)
{
    OVERLAPPED ov;
    BOOL ret;

    TRACE("(%p,%p)\n",hPipe, overlapped);

    if(overlapped)
    {
        if(SYNC_ConnectNamedPipe(hPipe,overlapped))
            SetLastError( ERROR_IO_PENDING );
        return FALSE;
    }

    memset(&ov,0,sizeof(ov));
    ov.hEvent = CreateEventA(NULL,0,0,NULL);
    if (!ov.hEvent)
        return FALSE;

    ret=SYNC_ConnectNamedPipe(hPipe, &ov);
    if(ret)
    {
        if (WAIT_OBJECT_0==WaitForSingleObject(ov.hEvent,INFINITE))
        {
            SetLastError(ov.Internal);
            ret = (ov.Internal==STATUS_SUCCESS);
        }
    }

    CloseHandle(ov.hEvent);

    return ret;
}

/***********************************************************************
 *           DisconnectNamedPipe   (KERNEL32.@)
 */
BOOL WINAPI DisconnectNamedPipe(HANDLE hPipe)
{
    BOOL ret;

    TRACE("(%p)\n",hPipe);

    SERVER_START_REQ( disconnect_named_pipe )
    {
        req->handle = hPipe;
        ret = !wine_server_call_err( req );
        if (ret && reply->fd != -1) close( reply->fd );
    }
    SERVER_END_REQ;

    return ret;
}

/***********************************************************************
 *           TransactNamedPipe   (KERNEL32.@)
 */
BOOL WINAPI TransactNamedPipe(
    HANDLE hPipe, LPVOID lpInput, DWORD dwInputSize, LPVOID lpOutput,
    DWORD dwOutputSize, LPDWORD lpBytesRead, LPOVERLAPPED lpOverlapped)
{
    FIXME("%p %p %ld %p %ld %p %p\n",
          hPipe, lpInput, dwInputSize, lpOutput,
          dwOutputSize, lpBytesRead, lpOverlapped);
    if(lpBytesRead)
        *lpBytesRead=0;
    return FALSE;
}

/***********************************************************************
 *           GetNamedPipeInfo   (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeInfo(
    HANDLE hNamedPipe, LPDWORD lpFlags, LPDWORD lpOutputBufferSize,
    LPDWORD lpInputBufferSize, LPDWORD lpMaxInstances)
{
    BOOL ret;

    TRACE("%p %p %p %p %p\n", hNamedPipe, lpFlags,
          lpOutputBufferSize, lpInputBufferSize, lpMaxInstances);

    SERVER_START_REQ( get_named_pipe_info )
    {
        req->handle = hNamedPipe;
        ret = !wine_server_call_err( req );
        if(lpFlags) *lpFlags = reply->flags;
        if(lpOutputBufferSize) *lpOutputBufferSize = reply->outsize;
        if(lpInputBufferSize) *lpInputBufferSize = reply->outsize;
        if(lpMaxInstances) *lpMaxInstances = reply->maxinstances;
    }
    SERVER_END_REQ;

    return ret;
}

/***********************************************************************
 *           GetNamedPipeHandleStateA  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeHandleStateA(
    HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout,
    LPSTR lpUsername, DWORD nUsernameMaxSize)
{
    FIXME("%p %p %p %p %p %p %ld\n",
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
    FIXME("%p %p %p %p %p %p %ld\n",
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
    FIXME("%p %p %p %p\n",
          hNamedPipe, lpMode, lpMaxCollectionCount, lpCollectDataTimeout);
    return FALSE;
}

/***********************************************************************
 *           CallNamedPipeA  (KERNEL32.@)
 */
BOOL WINAPI CallNamedPipeA(
    LPCSTR lpNamedPipeName, LPVOID lpInput, DWORD lpInputSize,
    LPVOID lpOutput, DWORD lpOutputSize,
    LPDWORD lpBytesRead, DWORD nTimeout)
{
    FIXME("%s %p %ld %p %ld %p %ld\n",
           debugstr_a(lpNamedPipeName), lpInput, lpInputSize,
           lpOutput, lpOutputSize, lpBytesRead, nTimeout);
    return FALSE;
}

/***********************************************************************
 *           CallNamedPipeW  (KERNEL32.@)
 */
BOOL WINAPI CallNamedPipeW(
    LPCWSTR lpNamedPipeName, LPVOID lpInput, DWORD lpInputSize,
    LPVOID lpOutput, DWORD lpOutputSize,
    LPDWORD lpBytesRead, DWORD nTimeout)
{
    FIXME("%s %p %ld %p %ld %p %ld\n",
           debugstr_w(lpNamedPipeName), lpInput, lpInputSize,
           lpOutput, lpOutputSize, lpBytesRead, nTimeout);
    return FALSE;
}

/******************************************************************
 *		CreatePipe (KERNEL32.@)
 *
 */
BOOL WINAPI CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe,
                        LPSECURITY_ATTRIBUTES sa, DWORD size )
{
    static unsigned  index = 0;
    char        name[64];
    HANDLE      hr, hw;
    unsigned    in_index = index;

    *hReadPipe = *hWritePipe = INVALID_HANDLE_VALUE;
    /* generate a unique pipe name (system wide) */
    do
    {
        sprintf(name, "\\\\.\\pipe\\Win32.Pipes.%08lu.%08u", GetCurrentProcessId(), ++index);
        hr = CreateNamedPipeA(name, PIPE_ACCESS_INBOUND, 
                              PIPE_TYPE_BYTE | PIPE_WAIT, 1, size, size, 
                              NMPWAIT_USE_DEFAULT_WAIT, sa);
    } while (hr == INVALID_HANDLE_VALUE && index != in_index);
    /* from completion sakeness, I think system resources might be exhausted before this happens !! */
    if (hr == INVALID_HANDLE_VALUE) return FALSE;

    hw = CreateFileA(name, GENERIC_WRITE, 0, sa, OPEN_EXISTING, 0, 0);
    if (hw == INVALID_HANDLE_VALUE) 
    {
        CloseHandle(hr);
        return FALSE;
    }

    *hReadPipe = hr;
    *hWritePipe = hw;
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
                  "ret $12");

/***********************************************************************
 *		InterlockedExchange (KERNEL32.@)
 */
/* LONG WINAPI InterlockedExchange( PLONG dest, LONG val ); */
__ASM_GLOBAL_FUNC(InterlockedExchange,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret $8");

/***********************************************************************
 *		InterlockedExchangeAdd (KERNEL32.@)
 */
/* LONG WINAPI InterlockedExchangeAdd( PLONG dest, LONG incr ); */
__ASM_GLOBAL_FUNC(InterlockedExchangeAdd,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret $8");

/***********************************************************************
 *		InterlockedIncrement (KERNEL32.@)
 */
/* LONG WINAPI InterlockedIncrement( PLONG dest ); */
__ASM_GLOBAL_FUNC(InterlockedIncrement,
                  "movl 4(%esp),%edx\n\t"
                  "movl $1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "incl %eax\n\t"
                  "ret $4");

/***********************************************************************
 *		InterlockedDecrement (KERNEL32.@)
 */
__ASM_GLOBAL_FUNC(InterlockedDecrement,
                  "movl 4(%esp),%edx\n\t"
                  "movl $-1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "decl %eax\n\t"
                  "ret $4");

#else  /* __i386__ */

/***********************************************************************
 *		InterlockedCompareExchange (KERNEL32.@)
 */
LONG WINAPI InterlockedCompareExchange( PLONG dest, LONG xchg, LONG compare )
{
    return interlocked_cmpxchg( dest, xchg, compare );
}

/***********************************************************************
 *		InterlockedExchange (KERNEL32.@)
 */
LONG WINAPI InterlockedExchange( PLONG dest, LONG val )
{
    return interlocked_xchg( dest, val );
}

/***********************************************************************
 *		InterlockedExchangeAdd (KERNEL32.@)
 */
LONG WINAPI InterlockedExchangeAdd( PLONG dest, LONG incr )
{
    return interlocked_xchg_add( dest, incr );
}

/***********************************************************************
 *		InterlockedIncrement (KERNEL32.@)
 */
LONG WINAPI InterlockedIncrement( PLONG dest )
{
    return interlocked_xchg_add( dest, 1 ) + 1;
}

/***********************************************************************
 *		InterlockedDecrement (KERNEL32.@)
 */
LONG WINAPI InterlockedDecrement( PLONG dest )
{
    return interlocked_xchg_add( dest, -1 ) - 1;
}

#endif  /* __i386__ */

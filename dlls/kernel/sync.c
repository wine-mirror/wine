/*
 * Kernel synchronization objects
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <string.h>
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);

/*
 * Events
 */


/***********************************************************************
 *           CreateEventA    (KERNEL32.@)
 */
HANDLE WINAPI CreateEventA( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                            BOOL initial_state, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_VAR_REQ( create_event, len * sizeof(WCHAR) )
    {
        req->manual_reset = manual_reset;
        req->initial_state = initial_state;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( create_event, len * sizeof(WCHAR) )
    {
        req->manual_reset = manual_reset;
        req->initial_state = initial_state;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/***********************************************************************
 *           WIN16_CreateEvent    (KERNEL.457)
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
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_VAR_REQ( open_event, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( open_event, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
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
        ret = !SERVER_CALL_ERR();
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
 *           SetEvent    (KERNEL.458)
 *           SetEvent    (KERNEL32.@)
 */
BOOL WINAPI SetEvent( HANDLE handle )
{
    return EVENT_Operation( handle, SET_EVENT );
}


/***********************************************************************
 *           ResetEvent    (KERNEL.459)
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
 *       VWin32_EventSet	(KERNEL.479)
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
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_VAR_REQ( create_mutex, len * sizeof(WCHAR) )
    {
        req->owned   = owner;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( create_mutex, len * sizeof(WCHAR) )
    {
        req->owned   = owner;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/*
 * Mutexes
 */


/***********************************************************************
 *           OpenMutexA   (KERNEL32.@)
 */
HANDLE WINAPI OpenMutexA( DWORD access, BOOL inherit, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_VAR_REQ( open_mutex, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( open_mutex, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
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
        ret = !SERVER_CALL_ERR();
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
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;

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

    SERVER_START_VAR_REQ( create_semaphore, len * sizeof(WCHAR) )
    {
        req->initial = (unsigned int)initial;
        req->max     = (unsigned int)max;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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

    SERVER_START_VAR_REQ( create_semaphore, len * sizeof(WCHAR) )
    {
        req->initial = (unsigned int)initial;
        req->max     = (unsigned int)max;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/***********************************************************************
 *           OpenSemaphoreA   (KERNEL32.@)
 */
HANDLE WINAPI OpenSemaphoreA( DWORD access, BOOL inherit, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_VAR_REQ( open_semaphore, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( open_semaphore, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
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
    FIXME("(Name=%s, OpenMode=%#08lx, dwPipeMode=%#08lx, MaxInst=%ld, OutBSize=%ld, InBuffSize=%ld, DefTimeOut=%ld, SecAttr=%p): stub\n",
          debugstr_a(name), dwOpenMode, dwPipeMode, nMaxInstances,
          nOutBufferSize, nInBufferSize, nDefaultTimeOut, attr );
    SetLastError (ERROR_UNKNOWN);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           CreateNamedPipeW   (KERNEL32.@)
 */
HANDLE WINAPI CreateNamedPipeW( LPCWSTR name, DWORD dwOpenMode,
                                DWORD dwPipeMode, DWORD nMaxInstances,
                                DWORD nOutBufferSize, DWORD nInBufferSize,
                                DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES attr )
{
    FIXME("(Name=%s, OpenMode=%#08lx, dwPipeMode=%#08lx, MaxInst=%ld, OutBSize=%ld, InBuffSize=%ld, DefTimeOut=%ld, SecAttr=%p): stub\n",
          debugstr_w(name), dwOpenMode, dwPipeMode, nMaxInstances,
          nOutBufferSize, nInBufferSize, nDefaultTimeOut, attr );

    SetLastError (ERROR_UNKNOWN);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           PeekNamedPipe   (KERNEL32.@)
 */
BOOL WINAPI PeekNamedPipe( HANDLE hPipe, LPVOID lpvBuffer, DWORD cbBuffer,
                           LPDWORD lpcbRead, LPDWORD lpcbAvail, LPDWORD lpcbMessage )
{
    FIXME("(%08x, %p, %08lx, %p, %p, %p): stub\n",
          hPipe, lpvBuffer, cbBuffer, lpcbRead, lpcbAvail, lpcbMessage);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           WaitNamedPipeA   (KERNEL32.@)
 */
BOOL WINAPI WaitNamedPipeA (LPCSTR lpNamedPipeName, DWORD nTimeOut)
{
    FIXME("%s 0x%08lx\n",lpNamedPipeName,nTimeOut);
    SetLastError(ERROR_PIPE_NOT_CONNECTED);
    return FALSE;
}


/***********************************************************************
 *           WaitNamedPipeW   (KERNEL32.@)
 */
BOOL WINAPI WaitNamedPipeW (LPCWSTR lpNamedPipeName, DWORD nTimeOut)
{
    FIXME("%s 0x%08lx\n",debugstr_w(lpNamedPipeName),nTimeOut);
    SetLastError(ERROR_PIPE_NOT_CONNECTED);
    return FALSE;
}

/*
 * Win32 process and thread synchronisation
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "heap.h"
#include "file.h"  /* for DOSFS_UnixTimeToFileTime */
#include "thread.h"
#include "winerror.h"
#include "server.h"


/***********************************************************************
 *              get_timeout
 */
inline static void get_timeout( struct timeval *when, int timeout )
{
    gettimeofday( when, 0 );
    if (timeout)
    {
        long sec = timeout / 1000;
        if ((when->tv_usec += (timeout - 1000*sec) * 1000) >= 1000000)
        {
            when->tv_usec -= 1000000;
            when->tv_sec++;
        }
        when->tv_sec += sec;
    }
}


/***********************************************************************
 *              call_apcs
 *
 * Call outstanding APCs.
 */
static void call_apcs( BOOL alertable )
{
    FARPROC proc = NULL;
    FILETIME ft;
    void *args[4];

    for (;;)
    {
        int type = APC_NONE;
        SERVER_START_REQ
        {
            struct get_apc_request *req = server_alloc_req( sizeof(*req), sizeof(args) );
            req->alertable = alertable;
            if (!server_call( REQ_GET_APC ))
            {
                type = req->type;
                proc = req->func;
                memcpy( args, server_data_ptr(req), server_data_size(req) );
            }
        }
        SERVER_END_REQ;

        switch(type)
        {
        case APC_NONE:
            return;  /* no more APCs */
        case APC_ASYNC:
            proc( &args[0] );
            break;
        case APC_USER:
            proc( args[0] );
            break;
        case APC_TIMER:
            /* convert sec/usec to NT time */
            DOSFS_UnixTimeToFileTime( (time_t)args[0], &ft, (DWORD)args[1] * 10 );
            proc( args[2], ft.dwLowDateTime, ft.dwHighDateTime );
            break;
        default:
            server_protocol_error( "get_apc_request: bad type %d\n", type );
            break;
        }
    }
}

/***********************************************************************
 *              Sleep  (KERNEL32.679)
 */
VOID WINAPI Sleep( DWORD timeout )
{
    WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, FALSE );
}

/******************************************************************************
 *              SleepEx   (KERNEL32.680)
 */
DWORD WINAPI SleepEx( DWORD timeout, BOOL alertable )
{
    DWORD ret = WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, alertable );
    if (ret != WAIT_IO_COMPLETION) ret = 0;
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.723)
 */
DWORD WINAPI WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.724)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE handle, DWORD timeout,
                                    BOOL alertable )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, alertable );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.721)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                     BOOL wait_all, DWORD timeout )
{
    return WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.722)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout,
                                       BOOL alertable )
{
    int i, ret;
    struct timeval tv;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (timeout == INFINITE) tv.tv_sec = tv.tv_usec = 0;
    else get_timeout( &tv, timeout );

    for (;;)
    {
        SERVER_START_REQ
        {
            struct select_request *req = server_alloc_req( sizeof(*req), count * sizeof(int) );
            int *data = server_data_ptr( req );

            req->flags   = SELECT_INTERRUPTIBLE;
            req->sec     = tv.tv_sec;
            req->usec    = tv.tv_usec;
            for (i = 0; i < count; i++) data[i] = handles[i];

            if (wait_all) req->flags |= SELECT_ALL;
            if (alertable) req->flags |= SELECT_ALERTABLE;
            if (timeout != INFINITE) req->flags |= SELECT_TIMEOUT;

            server_call( REQ_SELECT );
            ret = req->signaled;
        }
        SERVER_END_REQ;
        if (ret != STATUS_USER_APC) break;
        call_apcs( alertable );
        if (alertable) break;
    }
    return ret;
}


/***********************************************************************
 *           WIN16_WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WIN16_WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForSingleObject( handle, timeout );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WIN16_WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WIN16_WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                           BOOL wait_all, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjects( count, handles, wait_all, timeout );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WIN16_WaitForMultipleObjectsEx   (KERNEL.495)
 */
DWORD WINAPI WIN16_WaitForMultipleObjectsEx( DWORD count, 
                                             const HANDLE *handles,
                                             BOOL wait_all, DWORD timeout,
                                             BOOL alertable )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, alertable );
    RestoreThunkLock( mutex_count );
    return retval;
}


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
#include "syslevel.h"
#include "server.h"


/***********************************************************************
 *              call_apcs
 *
 * Call outstanding APCs.
 */
static void call_apcs(void)
{
    FARPROC proc;
    struct get_apc_request *req = get_req_buffer();

    for (;;)
    {
        if (server_call( REQ_GET_APC )) return;
        switch(req->type)
        {
        case APC_NONE:
            return;  /* no more APCs */
        case APC_USER:
            if ((proc = req->func))
            {
                proc( req->args[0] );
            }
            break;
        case APC_TIMER:
            if ((proc = req->func))
            {
                FILETIME ft;
                /* convert sec/usec to NT time */
                DOSFS_UnixTimeToFileTime( (time_t)req->args[0], &ft, (DWORD)req->args[1] * 10 );
                proc( req->args[2], ft.dwLowDateTime, ft.dwHighDateTime );
            }
            break;
        default:
            server_protocol_error( "get_apc_request: bad type %d\n", req->type );
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
    struct select_request *req = get_req_buffer();
    int i, ret;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    req->count   = count;
    req->flags   = 0;
    req->timeout = timeout;
    for (i = 0; i < count; i++) req->handles[i] = handles[i];

    if (wait_all) req->flags |= SELECT_ALL;
    if (alertable) req->flags |= SELECT_ALERTABLE;
    if (timeout != INFINITE) req->flags |= SELECT_TIMEOUT;

    server_call( REQ_SELECT );
    if ((ret = req->signaled) == STATUS_USER_APC) call_apcs();
    return ret;
}


/***********************************************************************
 *           WIN16_WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WIN16_WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    DWORD retval;

    SYSLEVEL_ReleaseWin16Lock();
    retval = WaitForSingleObject( handle, timeout );
    SYSLEVEL_RestoreWin16Lock();

    return retval;
}

/***********************************************************************
 *           WIN16_WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WIN16_WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                           BOOL wait_all, DWORD timeout )
{
    DWORD retval;

    SYSLEVEL_ReleaseWin16Lock();
    retval = WaitForMultipleObjects( count, handles, wait_all, timeout );
    SYSLEVEL_RestoreWin16Lock();

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
    DWORD retval;

    SYSLEVEL_ReleaseWin16Lock();
    retval = WaitForMultipleObjectsEx( count, handles, 
                                       wait_all, timeout, alertable );
    SYSLEVEL_RestoreWin16Lock();

    return retval;
}


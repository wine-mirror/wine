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
#include "process.h"
#include "thread.h"
#include "winerror.h"
#include "syslevel.h"
#include "message.h"
#include "x11drv.h"
#include "server.h"

/***********************************************************************
 *              call_apcs
 *
 * Call outstanding APCs.
 */
static void call_apcs(void)
{
#define MAX_APCS 16
    int i;
    void *buffer[MAX_APCS * 2];
    struct get_apcs_request *req = get_req_buffer();

    if (server_call( REQ_GET_APCS ) || !req->count) return;
    assert( req->count <= MAX_APCS );
    memcpy( buffer, req->apcs, req->count * 2 * sizeof(req->apcs[0]) );
    for (i = 0; i < req->count * 2; i += 2)
    {
        PAPCFUNC func = (PAPCFUNC)req->apcs[i];
        if (func) func( (ULONG_PTR)req->apcs[i+1] );
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


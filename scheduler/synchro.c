/*
 * Win32 process and thread synchronisation
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "k32obj.h"
#include "heap.h"
#include "process.h"
#include "thread.h"
#include "winerror.h"
#include "syslevel.h"
#include "server.h"
#include "debug.h"

/***********************************************************************
 *           SYNC_BuildWaitStruct
 */
static BOOL32 SYNC_BuildWaitStruct( DWORD count, const HANDLE32 *handles,
                                    BOOL32 wait_all, WAIT_STRUCT *wait )
{
    DWORD i;
    K32OBJ **ptr;

    SYSTEM_LOCK();
    wait->count    = count;
    wait->wait_all = wait_all;
    for (i = 0, ptr = wait->objs; i < count; i++, ptr++)
    {
    	TRACE(win32,"handle %ld is %08x\n",i,handles[i]);
        if (!(*ptr = HANDLE_GetObjPtr( PROCESS_Current(), handles[i],
                                       K32OBJ_UNKNOWN, SYNCHRONIZE,
                                       &wait->server[i] )))
        { 
            ERR(win32, "Bad handle %08x\n", handles[i]); 
            break; 
        }
        if (wait->server[i] == -1)
            WARN(win32,"No server handle for %08x (type %d)\n",
                 handles[i], (*ptr)->type );
    }

    if (i != count)
    {
        /* There was an error */
        while (i--) K32OBJ_DecCount( wait->objs[i] );
    }
    SYSTEM_UNLOCK();
    return (i == count);
}


/***********************************************************************
 *           SYNC_FreeWaitStruct
 */
static void SYNC_FreeWaitStruct( WAIT_STRUCT *wait )
{
    DWORD i;
    K32OBJ **ptr;
    SYSTEM_LOCK();
    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_DecCount( *ptr );
    SYSTEM_UNLOCK();
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
DWORD WINAPI SleepEx( DWORD timeout, BOOL32 alertable )
{
    DWORD ret = WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, alertable );
    if (ret != WAIT_IO_COMPLETION) ret = 0;
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.723)
 */
DWORD WINAPI WaitForSingleObject( HANDLE32 handle, DWORD timeout )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.724)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE32 handle, DWORD timeout,
                                    BOOL32 alertable )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, alertable );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.721)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE32 *handles,
                                     BOOL32 wait_all, DWORD timeout )
{
    return WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.722)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE32 *handles,
                                       BOOL32 wait_all, DWORD timeout,
                                       BOOL32 alertable )
{
    WAIT_STRUCT *wait = &THREAD_Current()->wait_struct;
    struct select_request req;
    struct select_reply reply;
    void *apc[32];
    int len;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (!SYNC_BuildWaitStruct( count, handles, wait_all, wait ))
        return WAIT_FAILED;

    req.count   = count;
    req.flags   = 0;
    req.timeout = timeout;

    if (wait_all) req.flags |= SELECT_ALL;
    if (alertable) req.flags |= SELECT_ALERTABLE;
    if (timeout != INFINITE32) req.flags |= SELECT_TIMEOUT;

    CLIENT_SendRequest( REQ_SELECT, -1, 2,
                        &req, sizeof(req),
                        wait->server, count * sizeof(int) );
    CLIENT_WaitReply( &len, NULL, 2, &reply, sizeof(reply),
                      apc, sizeof(apc) );
    if ((reply.signaled == STATUS_USER_APC) && (len > sizeof(reply)))
    {
        int i;
        len -= sizeof(reply);
        for (i = 0; i < len / sizeof(void*); i += 2)
        {
            PAPCFUNC func = (PAPCFUNC)apc[i];
            func( (ULONG_PTR)apc[i+1] );
        }
    }
    SYNC_FreeWaitStruct( wait );
    return reply.signaled;
}


/***********************************************************************
 *           WIN16_WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WIN16_WaitForSingleObject( HANDLE32 handle, DWORD timeout )
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
DWORD WINAPI WIN16_WaitForMultipleObjects( DWORD count, const HANDLE32 *handles,
                                           BOOL32 wait_all, DWORD timeout )
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
                                             const HANDLE32 *handles,
                                             BOOL32 wait_all, DWORD timeout,
                                             BOOL32 alertable )
{
    DWORD retval;

    SYSLEVEL_ReleaseWin16Lock();
    retval = WaitForMultipleObjectsEx( count, handles, 
                                       wait_all, timeout, alertable );
    SYSLEVEL_RestoreWin16Lock();

    return retval;
}


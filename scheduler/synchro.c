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
                                    BOOL32 wait_all, BOOL32 wait_msg, 
                                    WAIT_STRUCT *wait )
{
    DWORD i;
    K32OBJ **ptr;

    SYSTEM_LOCK();
    wait->count    = count;
    wait->signaled = WAIT_FAILED;
    wait->wait_all = wait_all;
    wait->wait_msg = wait_msg;
    for (i = 0, ptr = wait->objs; i < count; i++, ptr++)
    {
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
        wait->wait_msg = FALSE;
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
    wait->wait_msg = FALSE;
    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_DecCount( *ptr );
    SYSTEM_UNLOCK();
}


/***********************************************************************
 *           SYNC_DoWait
 */
DWORD SYNC_DoWait( DWORD count, const HANDLE32 *handles,
                   BOOL32 wait_all, DWORD timeout, 
                   BOOL32 alertable, BOOL32 wait_msg )
{
    WAIT_STRUCT *wait = &THREAD_Current()->wait_struct;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (alertable)
        FIXME(win32, "alertable not implemented\n" );

    if (!SYNC_BuildWaitStruct( count, handles, wait_all, wait_msg, wait ))
        wait->signaled = WAIT_FAILED;
    else
    {
        int flags = 0;
        if (wait_all) flags |= SELECT_ALL;
        if (alertable) flags |= SELECT_ALERTABLE;
        if (wait_msg) flags |= SELECT_MSG;
        if (timeout != INFINITE32) flags |= SELECT_TIMEOUT;
        wait->signaled = CLIENT_Select( count, wait->server, flags, timeout );
        SYNC_FreeWaitStruct( wait );
    }
    return wait->signaled;
}

/***********************************************************************
 *              Sleep  (KERNEL32.679)
 */
VOID WINAPI Sleep( DWORD timeout )
{
    SYNC_DoWait( 0, NULL, FALSE, timeout, FALSE, FALSE );
}

/******************************************************************************
 *              SleepEx   (KERNEL32.680)
 */
DWORD WINAPI SleepEx( DWORD timeout, BOOL32 alertable )
{
    DWORD ret = SYNC_DoWait( 0, NULL, FALSE, timeout, alertable, FALSE );
    if (ret != WAIT_IO_COMPLETION) ret = 0;
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.723)
 */
DWORD WINAPI WaitForSingleObject( HANDLE32 handle, DWORD timeout )
{
    return SYNC_DoWait( 1, &handle, FALSE, timeout, FALSE, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.724)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE32 handle, DWORD timeout,
                                    BOOL32 alertable )
{
    return SYNC_DoWait( 1, &handle, FALSE, timeout, alertable, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.721)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE32 *handles,
                                     BOOL32 wait_all, DWORD timeout )
{
    return SYNC_DoWait( count, handles, wait_all, timeout, FALSE, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.722)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE32 *handles,
                                       BOOL32 wait_all, DWORD timeout,
                                       BOOL32 alertable )
{
    return SYNC_DoWait( count, handles, wait_all, timeout, alertable, FALSE );
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


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
#include "debug.h"

/***********************************************************************
 *           SYNC_BuildWaitStruct
 */
static BOOL32 SYNC_BuildWaitStruct( DWORD count, const HANDLE32 *handles,
                                    BOOL32 wait_all, WAIT_STRUCT *wait )
{
    DWORD i;
    K32OBJ **ptr;

    wait->count    = count;
    wait->signaled = WAIT_FAILED;
    wait->wait_all = wait_all;
    SYSTEM_LOCK();
    for (i = 0, ptr = wait->objs; i < count; i++, ptr++)
    {
        if (!(*ptr = HANDLE_GetObjPtr( PROCESS_Current(), handles[i],
                                       K32OBJ_UNKNOWN, SYNCHRONIZE )))
            break;
        if (!K32OBJ_OPS( *ptr )->signaled)
        {
            /* This object type cannot be waited upon */
            K32OBJ_DecCount( *ptr );
            break;
        }

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
 *           SYNC_CheckCondition
 */
static BOOL32 SYNC_CheckCondition( WAIT_STRUCT *wait, DWORD thread_id )
{
    DWORD i;
    K32OBJ **ptr;

    SYSTEM_LOCK();
    if (wait->wait_all)
    {
        for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        {
            if (!K32OBJ_OPS( *ptr )->signaled( *ptr, thread_id ))
            {
                SYSTEM_UNLOCK();
                return FALSE;
            }
        }
        /* Wait satisfied: tell it to all objects */
        wait->signaled = WAIT_OBJECT_0;
        for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
            if (K32OBJ_OPS( *ptr )->satisfied( *ptr, thread_id ))
                wait->signaled = WAIT_ABANDONED_0;
        SYSTEM_UNLOCK();
        return TRUE;
    }
    else
    {
        for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        {
            if (K32OBJ_OPS( *ptr )->signaled( *ptr, thread_id ))
            {
                /* Wait satisfied: tell it to the object */
                wait->signaled = WAIT_OBJECT_0 + i;
                if (K32OBJ_OPS( *ptr )->satisfied( *ptr, thread_id ))
                    wait->signaled = WAIT_ABANDONED_0 + i;
                SYSTEM_UNLOCK();
                return TRUE;
            }
        }
        SYSTEM_UNLOCK();
        return FALSE;
    }
}


/***********************************************************************
 *           SYNC_WaitForCondition
 */
void SYNC_WaitForCondition( WAIT_STRUCT *wait, DWORD timeout )
{
    DWORD i, thread_id = GetCurrentThreadId();
    LONG count;
    K32OBJ **ptr;
    sigset_t set;

    SYSTEM_LOCK();
    if (SYNC_CheckCondition( wait, thread_id ))
        goto done;  /* Condition already satisfied */
    if (!timeout)
    {
        /* No need to wait */
        wait->signaled = WAIT_TIMEOUT;
        goto done;
    }

    /* Add ourselves to the waiting list of all objects */

    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_OPS( *ptr )->add_wait( *ptr, thread_id );

    /* Release the system lock completely */

    count = SYSTEM_LOCK_COUNT();
    for (i = count; i > 0; i--) SYSTEM_UNLOCK();

    /* Now wait for it */

    TRACE(win32, "starting wait (%p %04x)\n",
		 THREAD_Current(), THREAD_Current()->teb_sel );

    sigprocmask( SIG_SETMASK, NULL, &set );
    sigdelset( &set, SIGUSR1 );
    sigdelset( &set, SIGALRM );
    if (timeout != INFINITE32)
    {
        while (wait->signaled == WAIT_FAILED)
        {
            struct itimerval timer;
            DWORD start_ticks, elapsed;
            timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
            timer.it_value.tv_sec = timeout / 1000;
            timer.it_value.tv_usec = (timeout % 1000) * 1000;
            start_ticks = GetTickCount();
            setitimer( ITIMER_REAL, &timer, NULL );
            sigsuspend( &set );
            if (wait->signaled != WAIT_FAILED) break;
            /* Recompute the timer value */
            elapsed = GetTickCount() - start_ticks;
            if (elapsed >= timeout) wait->signaled = WAIT_TIMEOUT;
            else timeout -= elapsed;
        }
    }
    else
    {
        while (wait->signaled == WAIT_FAILED)
        {
            sigsuspend( &set );
        }
    }

    /* Grab the system lock again */

    while (count--) SYSTEM_LOCK();
    TRACE(win32, "wait finished (%p %04x)\n",
		 THREAD_Current(), THREAD_Current()->teb_sel );

    /* Remove ourselves from the lists */

    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_OPS( *ptr )->remove_wait( *ptr, thread_id );

done:
    SYSTEM_UNLOCK();
}


/***********************************************************************
 *           SYNC_DummySigHandler
 *
 * Dummy signal handler
 */
static void SYNC_DummySigHandler(void)
{
}


/***********************************************************************
 *           SYNC_SetupSignals
 *
 * Setup signal handlers for a new thread.
 * FIXME: should merge with SIGNAL_Init.
 */
void SYNC_SetupSignals(void)
{
    sigset_t set;
    SIGNAL_SetHandler( SIGUSR1, SYNC_DummySigHandler, 0 );
    /* FIXME: conflicts with system timers */
    SIGNAL_SetHandler( SIGALRM, SYNC_DummySigHandler, 0 );
    sigemptyset( &set );
    /* Make sure these are blocked by default */
    sigaddset( &set, SIGUSR1 );
    sigaddset( &set, SIGALRM );
    sigprocmask( SIG_BLOCK , &set, NULL);
}


/***********************************************************************
 *           SYNC_WakeUp
 */
void SYNC_WakeUp( THREAD_QUEUE *wait_queue, DWORD max )
{
    THREAD_ENTRY *entry;

    if (!max) max = INFINITE32;
    SYSTEM_LOCK();
    if (!*wait_queue)
    {
        SYSTEM_UNLOCK();
        return;
    }
    entry = (*wait_queue)->next;
    for (;;)
    {
        THDB *thdb = entry->thread;
        if (SYNC_CheckCondition( &thdb->wait_struct, THDB_TO_THREAD_ID(thdb) ))
        {
            TRACE(win32, "waking up %04x\n", thdb->teb_sel );
            if (thdb->unix_pid)
	    	kill( thdb->unix_pid, SIGUSR1 );
	    else
	    	FIXME(win32,"have got unix_pid 0\n");
            if (!--max) break;
        }
        if (entry == *wait_queue) break;
        entry = entry->next;
    }
    SYSTEM_UNLOCK();
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
    return WaitForMultipleObjectsEx(count, handles, wait_all, timeout, FALSE);
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.722)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE32 *handles,
                                       BOOL32 wait_all, DWORD timeout,
                                       BOOL32 alertable )
{
    WAIT_STRUCT *wait = &THREAD_Current()->wait_struct;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (alertable)
        FIXME(win32, "alertable not implemented\n" );

    SYSTEM_LOCK();
    if (!SYNC_BuildWaitStruct( count, handles, wait_all, wait ))
        wait->signaled = WAIT_FAILED;
    else
    {
        /* Now wait for it */
        SYNC_WaitForCondition( wait, timeout );
        SYNC_FreeWaitStruct( wait );
    }
    SYSTEM_UNLOCK();
    return wait->signaled;
}

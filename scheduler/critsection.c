/*
 * Win32 critical sections
 *
 * Copyright 1998 Alexandre Julliard
 */

/* Note: critical sections are not implemented exactly the same way
 * than under NT (LockSemaphore should be a real semaphore handle).
 * But since they are even more different under Win95, it probably
 * doesn't matter... 
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "windows.h"
#include "winerror.h"
#include "winbase.h"
#include "heap.h"
#include "k32obj.h"
#include "thread.h"

typedef struct
{
    K32OBJ        header;
    THREAD_QUEUE  wait_queue;
    BOOL32        signaled;
} CRIT_SECTION;

/* On some systems this is supposed to be defined in the program */
#ifndef HAVE_UNION_SEMUN
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};
#endif

static BOOL32 CRIT_SECTION_Signaled( K32OBJ *obj, DWORD thread_id );
static BOOL32 CRIT_SECTION_Satisfied( K32OBJ *obj, DWORD thread_id );
static void CRIT_SECTION_AddWait( K32OBJ *obj, DWORD thread_id );
static void CRIT_SECTION_RemoveWait( K32OBJ *obj, DWORD thread_id );
static void CRIT_SECTION_Destroy( K32OBJ *obj );

const K32OBJ_OPS CRITICAL_SECTION_Ops =
{
    CRIT_SECTION_Signaled,     /* signaled */
    CRIT_SECTION_Satisfied,    /* satisfied */
    CRIT_SECTION_AddWait,      /* add_wait */
    CRIT_SECTION_RemoveWait,   /* remove_wait */
    CRIT_SECTION_Destroy       /* destroy */
};

/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.472) (NTDLL.406)
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    CRIT_SECTION *obj;

    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    if (SystemHeap)
    {
        if (!(obj = (CRIT_SECTION *)HeapAlloc( SystemHeap, 0, sizeof(*obj) )))
            return;  /* No way to return an error... */
        obj->header.type     = K32OBJ_CRITICAL_SECTION;
        obj->header.refcount = 1;
        obj->wait_queue      = NULL;
        obj->signaled        = FALSE;
        crit->LockSemaphore  = (HANDLE32)obj;
        crit->Reserved       = (DWORD)-1;
    }
    else
    {
        union semun val;
        crit->Reserved = (DWORD)semget( IPC_PRIVATE, 1, IPC_CREAT | 0777 );
        if (crit->Reserved == (DWORD)-1)
        {
            perror( "semget" );
            return;
        }
        val.val = 0;
        semctl( (int)crit->Reserved, 0, SETVAL, val );
    }
}


/***********************************************************************
 *           DeleteCriticalSection   (KERNEL32.185) (NTDLL.327)
 */
void WINAPI DeleteCriticalSection( CRITICAL_SECTION *crit )
{
    CRIT_SECTION *obj = (CRIT_SECTION *)crit->LockSemaphore;

    if (obj)
    {
        if (crit->RecursionCount)  /* Should not happen */
            fprintf( stderr, "Deleting owned critical section (%p)\n", crit );
        crit->LockCount      = -1;
        crit->RecursionCount = 0;
        crit->OwningThread   = 0;
        crit->LockSemaphore  = 0;
        K32OBJ_DecCount( &obj->header );
    }
    else if (crit->Reserved != (DWORD)-1)
    {
        semctl( (int)crit->Reserved, 0, IPC_RMID, (union semun)0 );
    }
}


/***********************************************************************
 *           EnterCriticalSection   (KERNEL32.195) (NTDLL.344)
 */
void WINAPI EnterCriticalSection( CRITICAL_SECTION *crit )
{
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return;
        }
        /* Now wait for it */
        if (crit->LockSemaphore)
        {
            WAIT_STRUCT *wait = &THREAD_Current()->wait_struct;
            SYSTEM_LOCK();
            wait->count    = 1;
            wait->signaled = WAIT_FAILED;
            wait->wait_all = FALSE;
            wait->objs[0] = (K32OBJ *)crit->LockSemaphore;
            K32OBJ_IncCount( wait->objs[0] );
            SYNC_WaitForCondition( wait, INFINITE32 );
            K32OBJ_DecCount( wait->objs[0] );
            SYSTEM_UNLOCK();
        }
        else if (crit->Reserved != (DWORD)-1)
        {
            int ret;
            struct sembuf sop;
            sop.sem_num = 0;
            sop.sem_op  = -1;
            sop.sem_flg = SEM_UNDO;
            do
            {
                ret = semop( (int)crit->Reserved, &sop, 1 );
            } while ((ret == -1) && (errno == EINTR));
        }
        else
        {
            fprintf( stderr, "Uninitialized critical section (%p)\n", crit );
            return;
        }
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
}


/***********************************************************************
 *           TryEnterCriticalSection   (KERNEL32.898) (NTDLL.969)
 */
BOOL32 WINAPI TryEnterCriticalSection( CRITICAL_SECTION *crit )
{
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return TRUE;
        }
        /* FIXME: this doesn't work */
        InterlockedDecrement( &crit->LockCount );
        return FALSE;
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
    return TRUE;
}


/***********************************************************************
 *           LeaveCriticalSection   (KERNEL32.494) (NTDLL.426)
 */
void WINAPI LeaveCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->OwningThread != GetCurrentThreadId()) return;
       
    if (--crit->RecursionCount)
    {
        InterlockedDecrement( &crit->LockCount );
        return;
    }
    crit->OwningThread = 0;
    if (InterlockedDecrement( &crit->LockCount ) >= 0)
    {
        /* Someone is waiting */
        if (crit->LockSemaphore)
        {
            CRIT_SECTION *obj = (CRIT_SECTION *)crit->LockSemaphore;
            SYSTEM_LOCK();
            obj->signaled = TRUE;
            SYNC_WakeUp( &obj->wait_queue, 1 );
            SYSTEM_UNLOCK();
        }
        else if (crit->Reserved != (DWORD)-1)
        {
            struct sembuf sop;
            sop.sem_num = 0;
            sop.sem_op  = 1;
            sop.sem_flg = SEM_UNDO;
            semop( (int)crit->Reserved, &sop, 1 );
        }
    }
}


/***********************************************************************
 *           MakeCriticalSectionGlobal   (KERNEL32.515)
 */
void WINAPI MakeCriticalSectionGlobal( CRITICAL_SECTION *crit )
{
    /* Nothing to do: a critical section is always global */
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.581)
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    DeleteCriticalSection( crit );
    InitializeCriticalSection( crit );
}


/***********************************************************************
 *           CRIT_SECTION_Signaled
 */
static BOOL32 CRIT_SECTION_Signaled( K32OBJ *obj, DWORD thread_id )
{
    CRIT_SECTION *crit = (CRIT_SECTION *)obj;
    assert( obj->type == K32OBJ_CRITICAL_SECTION );
    return crit->signaled;
}


/***********************************************************************
 *           CRIT_SECTION_Satisfied
 *
 * Wait on this object has been satisfied.
 */
static BOOL32 CRIT_SECTION_Satisfied( K32OBJ *obj, DWORD thread_id )
{
    CRIT_SECTION *crit = (CRIT_SECTION *)obj;
    assert( obj->type == K32OBJ_CRITICAL_SECTION );
    /* Only one thread is allowed to wake up */
    crit->signaled = FALSE;
    return FALSE;  /* Not abandoned */
}


/***********************************************************************
 *           CRIT_SECTION_AddWait
 *
 * Add thread to object wait queue.
 */
static void CRIT_SECTION_AddWait( K32OBJ *obj, DWORD thread_id )
{
    CRIT_SECTION *crit = (CRIT_SECTION *)obj;
    assert( obj->type == K32OBJ_CRITICAL_SECTION );
    THREAD_AddQueue( &crit->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           CRIT_SECTION_RemoveWait
 *
 * Remove current thread from object wait queue.
 */
static void CRIT_SECTION_RemoveWait( K32OBJ *obj, DWORD thread_id )
{
    CRIT_SECTION *crit = (CRIT_SECTION *)obj;
    assert( obj->type == K32OBJ_CRITICAL_SECTION );
    THREAD_RemoveQueue( &crit->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           CRIT_SECTION_Destroy
 */
static void CRIT_SECTION_Destroy( K32OBJ *obj )
{
    CRIT_SECTION *crit = (CRIT_SECTION *)obj;
    assert( obj->type == K32OBJ_CRITICAL_SECTION );
    /* There cannot be any thread on the list since the ref count is 0 */
    assert( crit->wait_queue == NULL );
    obj->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, crit );
}

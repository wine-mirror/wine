/*
 * Win32 semaphores
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include "windows.h"
#include "winerror.h"
#include "k32obj.h"
#include "process.h"
#include "thread.h"
#include "heap.h"

typedef struct
{
    K32OBJ        header;
    THREAD_QUEUE  wait_queue;
    LONG          count;
    LONG          max;
} SEMAPHORE;

static BOOL32 SEMAPHORE_Signaled( K32OBJ *obj, DWORD thread_id );
static BOOL32 SEMAPHORE_Satisfied( K32OBJ *obj, DWORD thread_id );
static void SEMAPHORE_AddWait( K32OBJ *obj, DWORD thread_id );
static void SEMAPHORE_RemoveWait( K32OBJ *obj, DWORD thread_id );
static void SEMAPHORE_Destroy( K32OBJ *obj );

const K32OBJ_OPS SEMAPHORE_Ops =
{
    SEMAPHORE_Signaled,    /* signaled */
    SEMAPHORE_Satisfied,   /* satisfied */
    SEMAPHORE_AddWait,     /* add_wait */
    SEMAPHORE_RemoveWait,  /* remove_wait */
    SEMAPHORE_Destroy      /* destroy */
};


/***********************************************************************
 *           CreateSemaphore32A   (KERNEL32.174)
 */
HANDLE32 WINAPI CreateSemaphore32A( SECURITY_ATTRIBUTES *sa, LONG initial,
                                    LONG max, LPCSTR name )
{
    HANDLE32 handle;
    SEMAPHORE *sem;

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE32;
    }

    SYSTEM_LOCK();
    sem = (SEMAPHORE *)K32OBJ_Create( K32OBJ_SEMAPHORE, sizeof(*sem),
                                      name, &handle );
    if (sem)
    {
        /* Finish initializing it */
        sem->wait_queue = NULL;
        sem->count      = initial;
        sem->max        = max;
        K32OBJ_DecCount( &sem->header );
    }
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *           CreateSemaphore32W   (KERNEL32.175)
 */
HANDLE32 WINAPI CreateSemaphore32W( SECURITY_ATTRIBUTES *sa, LONG initial,
                                    LONG max, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = CreateSemaphore32A( sa, initial, max, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           OpenSemaphore32A   (KERNEL32.545)
 */
HANDLE32 WINAPI OpenSemaphore32A( DWORD access, BOOL32 inherit, LPCSTR name )
{
    HANDLE32 handle = 0;
    K32OBJ *obj;
    SYSTEM_LOCK();
    if ((obj = K32OBJ_FindNameType( name, K32OBJ_SEMAPHORE )) != NULL)
    {
        handle = PROCESS_AllocHandle( obj, 0 );
        K32OBJ_DecCount( obj );
    }
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *           OpenSemaphore32W   (KERNEL32.546)
 */
HANDLE32 WINAPI OpenSemaphore32W( DWORD access, BOOL32 inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = OpenSemaphore32A( access, inherit, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           ReleaseSemaphore   (KERNEL32.583)
 */
BOOL32 WINAPI ReleaseSemaphore( HANDLE32 handle, LONG count, LONG *previous )
{
    SEMAPHORE *sem;

    SYSTEM_LOCK();
    if (!(sem = (SEMAPHORE *)PROCESS_GetObjPtr( handle, K32OBJ_SEMAPHORE )))
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }
    if (previous) *previous = sem->count;
    if (sem->count + count > sem->max)
    {
        SYSTEM_UNLOCK();
        SetLastError( ERROR_TOO_MANY_POSTS );
        return FALSE;
    }
    if (sem->count > 0)
    {
        /* There cannot be any thread waiting if the count is > 0 */
        assert( sem->wait_queue == NULL );
        sem->count += count;
    }
    else
    {
        sem->count = count;
        SYNC_WakeUp( &sem->wait_queue, count );
    }
    K32OBJ_DecCount( &sem->header );
    SYSTEM_UNLOCK();
    return TRUE;
}


/***********************************************************************
 *           SEMAPHORE_Signaled
 */
static BOOL32 SEMAPHORE_Signaled( K32OBJ *obj, DWORD thread_id )
{
    SEMAPHORE *sem = (SEMAPHORE *)obj;
    assert( obj->type == K32OBJ_SEMAPHORE );
    return (sem->count > 0);
}


/***********************************************************************
 *           SEMAPHORE_Satisfied
 *
 * Wait on this object has been satisfied.
 */
static BOOL32 SEMAPHORE_Satisfied( K32OBJ *obj, DWORD thread_id )
{
    SEMAPHORE *sem = (SEMAPHORE *)obj;
    assert( obj->type == K32OBJ_SEMAPHORE );
    assert( sem->count > 0 );
    sem->count--;
    return FALSE;  /* Not abandoned */
}


/***********************************************************************
 *           SEMAPHORE_AddWait
 *
 * Add current thread to object wait queue.
 */
static void SEMAPHORE_AddWait( K32OBJ *obj, DWORD thread_id )
{
    SEMAPHORE *sem = (SEMAPHORE *)obj;
    assert( obj->type == K32OBJ_SEMAPHORE );
    THREAD_AddQueue( &sem->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           SEMAPHORE_RemoveWait
 *
 * Remove thread from object wait queue.
 */
static void SEMAPHORE_RemoveWait( K32OBJ *obj, DWORD thread_id )
{
    SEMAPHORE *sem = (SEMAPHORE *)obj;
    assert( obj->type == K32OBJ_SEMAPHORE );
    THREAD_RemoveQueue( &sem->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           SEMAPHORE_Destroy
 */
static void SEMAPHORE_Destroy( K32OBJ *obj )
{
    SEMAPHORE *sem = (SEMAPHORE *)obj;
    assert( obj->type == K32OBJ_SEMAPHORE );
    /* There cannot be any thread on the list since the ref count is 0 */
    assert( sem->wait_queue == NULL );
    obj->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, sem );
}

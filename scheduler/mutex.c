/*
 * Win32 mutexes
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

typedef struct _MUTEX
{
    K32OBJ         header;
    THREAD_QUEUE   wait_queue;
    DWORD          owner;
    DWORD          count;
    BOOL32         abandoned;
    struct _MUTEX *next;
    struct _MUTEX *prev;
} MUTEX;

static BOOL32 MUTEX_Signaled( K32OBJ *obj, DWORD thread_id );
static BOOL32 MUTEX_Satisfied( K32OBJ *obj, DWORD thread_id );
static void MUTEX_AddWait( K32OBJ *obj, DWORD thread_id );
static void MUTEX_RemoveWait( K32OBJ *obj, DWORD thread_id );
static void MUTEX_Destroy( K32OBJ *obj );

const K32OBJ_OPS MUTEX_Ops =
{
    MUTEX_Signaled,    /* signaled */
    MUTEX_Satisfied,   /* satisfied */
    MUTEX_AddWait,     /* add_wait */
    MUTEX_RemoveWait,  /* remove_wait */
    MUTEX_Destroy      /* destroy */
};


/***********************************************************************
 *           MUTEX_Release
 *
 * Release a mutex once the count is 0.
 * Helper function for MUTEX_Abandon and ReleaseMutex.
 */
static void MUTEX_Release( MUTEX *mutex )
{
    /* Remove the mutex from the thread list of owned mutexes */
    if (mutex->next) mutex->next->prev = mutex->prev;
    if (mutex->prev) mutex->prev->next = mutex->next;
    else THREAD_Current()->mutex_list = &mutex->next->header;
    mutex->next = mutex->prev = NULL;
    mutex->owner = 0;
    SYNC_WakeUp( &mutex->wait_queue, INFINITE32 );
}


/***********************************************************************
 *           MUTEX_Abandon
 *
 * Abandon a mutex.
 */
void MUTEX_Abandon( K32OBJ *obj )
{
    MUTEX *mutex = (MUTEX *)obj;
    assert( obj->type == K32OBJ_MUTEX );
    assert( mutex->count && (mutex->owner == GetCurrentThreadId()) );
    mutex->count = 0;
    mutex->abandoned = TRUE;
    MUTEX_Release( mutex );
}


/***********************************************************************
 *           CreateMutex32A   (KERNEL32.166)
 */
HANDLE32 WINAPI CreateMutex32A( SECURITY_ATTRIBUTES *sa, BOOL32 owner,
                                LPCSTR name )
{
    HANDLE32 handle;
    MUTEX *mutex;

    SYSTEM_LOCK();
    mutex = (MUTEX *)K32OBJ_Create( K32OBJ_MUTEX, sizeof(*mutex),
                                    name, &handle );
    if (mutex)
    {
        /* Finish initializing it */
        mutex->wait_queue = NULL;
        mutex->abandoned  = FALSE;
        mutex->prev       = NULL;
        if (owner)
        {
            K32OBJ **list;
            mutex->owner = GetCurrentThreadId();
            mutex->count = 1;
            /* Add the mutex in the thread list of owned mutexes */
            list = &THREAD_Current()->mutex_list;
            if ((mutex->next = (MUTEX *)*list)) mutex->next->prev = mutex;
            *list = &mutex->header;
        }
        else
        {
            mutex->owner = 0;
            mutex->count = 0;
            mutex->next  = NULL;
        }
        K32OBJ_DecCount( &mutex->header );
    }
    SetLastError(0); /* FIXME */
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *           CreateMutex32W   (KERNEL32.167)
 */
HANDLE32 WINAPI CreateMutex32W( SECURITY_ATTRIBUTES *sa, BOOL32 owner,
                                LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = CreateMutex32A( sa, owner, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           OpenMutex32A   (KERNEL32.541)
 */
HANDLE32 WINAPI OpenMutex32A( DWORD access, BOOL32 inherit, LPCSTR name )
{
    HANDLE32 handle = 0;
    K32OBJ *obj;
    SYSTEM_LOCK();
    if ((obj = K32OBJ_FindNameType( name, K32OBJ_MUTEX )) != NULL)
    {
        handle = PROCESS_AllocHandle( obj, 0 );
        K32OBJ_DecCount( obj );
    }
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *           OpenMutex32W   (KERNEL32.542)
 */
HANDLE32 WINAPI OpenMutex32W( DWORD access, BOOL32 inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = OpenMutex32A( access, inherit, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           ReleaseMutex   (KERNEL32.582)
 */
BOOL32 WINAPI ReleaseMutex( HANDLE32 handle )
{
    MUTEX *mutex;
    SYSTEM_LOCK();
    if (!(mutex = (MUTEX *)PROCESS_GetObjPtr( handle, K32OBJ_MUTEX )))
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }
    if (!mutex->count || (mutex->owner != GetCurrentThreadId()))
    {
        SYSTEM_UNLOCK();
        SetLastError( ERROR_NOT_OWNER );
        return FALSE;
    }
    if (!--mutex->count) MUTEX_Release( mutex );
    K32OBJ_DecCount( &mutex->header );
    SYSTEM_UNLOCK();
    return TRUE;
}


/***********************************************************************
 *           MUTEX_Signaled
 */
static BOOL32 MUTEX_Signaled( K32OBJ *obj, DWORD thread_id )
{
    MUTEX *mutex = (MUTEX *)obj;
    assert( obj->type == K32OBJ_MUTEX );
    return (!mutex->count || (mutex->owner == thread_id));
}


/***********************************************************************
 *           MUTEX_Satisfied
 *
 * Wait on this object has been satisfied.
 */
static BOOL32 MUTEX_Satisfied( K32OBJ *obj, DWORD thread_id )
{
    BOOL32 ret;
    MUTEX *mutex = (MUTEX *)obj;
    assert( obj->type == K32OBJ_MUTEX );
    assert( !mutex->count || (mutex->owner == thread_id) );
    mutex->owner = thread_id;
    if (!mutex->count++)
    {
        /* Add the mutex in the thread list of owned mutexes */
        K32OBJ **list = &THREAD_ID_TO_THDB( thread_id )->mutex_list;
        assert( !mutex->next );
        if ((mutex->next = (MUTEX *)*list)) mutex->next->prev = mutex;
        *list = &mutex->header;
        mutex->prev = NULL;
    }
    ret = mutex->abandoned;
    mutex->abandoned = FALSE;
    return ret;
}


/***********************************************************************
 *           MUTEX_AddWait
 *
 * Add a thread to the object wait queue.
 */
static void MUTEX_AddWait( K32OBJ *obj, DWORD thread_id )
{
    MUTEX *mutex = (MUTEX *)obj;
    assert( obj->type == K32OBJ_MUTEX );
    THREAD_AddQueue( &mutex->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           MUTEX_RemoveWait
 *
 * Remove a thread from the object wait queue.
 */
static void MUTEX_RemoveWait( K32OBJ *obj, DWORD thread_id )
{
    MUTEX *mutex = (MUTEX *)obj;
    assert( obj->type == K32OBJ_MUTEX );
    THREAD_RemoveQueue( &mutex->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           MUTEX_Destroy
 */
static void MUTEX_Destroy( K32OBJ *obj )
{
    MUTEX *mutex = (MUTEX *)obj;
    assert( obj->type == K32OBJ_MUTEX );
    /* There cannot be any thread on the list since the ref count is 0 */
    assert( mutex->wait_queue == NULL );
    obj->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, mutex );
}

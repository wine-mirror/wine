/*
 * Win32 events
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
    BOOL32        manual_reset;
    BOOL32        signaled;
} EVENT;

static BOOL32 EVENT_Signaled( K32OBJ *obj, DWORD thread_id );
static BOOL32 EVENT_Satisfied( K32OBJ *obj, DWORD thread_id );
static void EVENT_AddWait( K32OBJ *obj, DWORD thread_id );
static void EVENT_RemoveWait( K32OBJ *obj, DWORD thread_id );
static void EVENT_Destroy( K32OBJ *obj );

const K32OBJ_OPS EVENT_Ops =
{
    EVENT_Signaled,     /* signaled */
    EVENT_Satisfied,    /* satisfied */
    EVENT_AddWait,      /* add_wait */
    EVENT_RemoveWait,   /* remove_wait */
    NULL,               /* read */
    NULL,               /* write */
    EVENT_Destroy       /* destroy */
};


/***********************************************************************
 *           EVENT_Set
 *
 * Implementation of SetEvent. Used by ExitThread and ExitProcess.
 */
void EVENT_Set( K32OBJ *obj )
{
    EVENT *event = (EVENT *)obj;
    assert( obj->type == K32OBJ_EVENT );
    SYSTEM_LOCK();
    event->signaled = TRUE;
    SYNC_WakeUp( &event->wait_queue, event->manual_reset ? INFINITE32 : 1 );
    SYSTEM_UNLOCK();
}

/***********************************************************************
 *           EVENT_Create
 *
 * Partial implementation of CreateEvent.
 * Used internally by processes and threads.
 */
K32OBJ *EVENT_Create( BOOL32 manual_reset, BOOL32 initial_state )
{
    EVENT *event;

    SYSTEM_LOCK();
    if ((event = HeapAlloc( SystemHeap, 0, sizeof(*event) )))
    {
        event->header.type     = K32OBJ_EVENT;
        event->header.refcount = 1;
        event->wait_queue      = NULL;
        event->manual_reset    = manual_reset;
        event->signaled        = initial_state;
    }
    SYSTEM_UNLOCK();
    return event ? &event->header : NULL;
}


/***********************************************************************
 *           CreateEvent32A    (KERNEL32.156)
 */
HANDLE32 WINAPI CreateEvent32A( SECURITY_ATTRIBUTES *sa, BOOL32 manual_reset,
                                BOOL32 initial_state, LPCSTR name )
{
    HANDLE32 handle;
    EVENT *event;

    SYSTEM_LOCK();
    event = (EVENT *)K32OBJ_Create( K32OBJ_EVENT, sizeof(*event),
                                    name, EVENT_ALL_ACCESS, sa, &handle );
    if (event)
    {
        /* Finish initializing it */
        event->wait_queue   = NULL;
        event->manual_reset = manual_reset;
        event->signaled     = initial_state;
        K32OBJ_DecCount( &event->header );
    }
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *           CreateEvent32W    (KERNEL32.157)
 */
HANDLE32 WINAPI CreateEvent32W( SECURITY_ATTRIBUTES *sa, BOOL32 manual_reset,
                                BOOL32 initial_state, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = CreateEvent32A( sa, manual_reset, initial_state, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           OpenEvent32A    (KERNEL32.536)
 */
HANDLE32 WINAPI OpenEvent32A( DWORD access, BOOL32 inherit, LPCSTR name )
{
    HANDLE32 handle = 0;
    K32OBJ *obj;
    SYSTEM_LOCK();
    if ((obj = K32OBJ_FindNameType( name, K32OBJ_EVENT )) != NULL)
    {
        handle = HANDLE_Alloc( PROCESS_Current(), obj, access, inherit, -1 );
        K32OBJ_DecCount( obj );
    }
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *           OpenEvent32W    (KERNEL32.537)
 */
HANDLE32 WINAPI OpenEvent32W( DWORD access, BOOL32 inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = OpenEvent32A( access, inherit, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           PulseEvent    (KERNEL32.557)
 */
BOOL32 WINAPI PulseEvent( HANDLE32 handle )
{
    EVENT *event;
    SYSTEM_LOCK();
    if (!(event = (EVENT *)HANDLE_GetObjPtr(PROCESS_Current(), handle,
                                            K32OBJ_EVENT, EVENT_MODIFY_STATE, NULL)))
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }
    event->signaled = TRUE;
    SYNC_WakeUp( &event->wait_queue, event->manual_reset ? INFINITE32 : 1 );
    event->signaled = FALSE;
    K32OBJ_DecCount( &event->header );
    SYSTEM_UNLOCK();
    return TRUE;
}


/***********************************************************************
 *           SetEvent    (KERNEL32.644)
 */
BOOL32 WINAPI SetEvent( HANDLE32 handle )
{
    EVENT *event;
    SYSTEM_LOCK();
    if (!(event = (EVENT *)HANDLE_GetObjPtr(PROCESS_Current(), handle,
                                            K32OBJ_EVENT, EVENT_MODIFY_STATE, NULL)))
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }
    event->signaled = TRUE;
    SYNC_WakeUp( &event->wait_queue, event->manual_reset ? INFINITE32 : 1 );
    K32OBJ_DecCount( &event->header );
    SYSTEM_UNLOCK();
    return TRUE;
}


/***********************************************************************
 *           ResetEvent    (KERNEL32.586)
 */
BOOL32 WINAPI ResetEvent( HANDLE32 handle )
{
    EVENT *event;
    SYSTEM_LOCK();
    if (!(event = (EVENT *)HANDLE_GetObjPtr(PROCESS_Current(), handle,
                                            K32OBJ_EVENT, EVENT_MODIFY_STATE, NULL)))
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }
    event->signaled = FALSE;
    K32OBJ_DecCount( &event->header );
    SYSTEM_UNLOCK();
    return TRUE;
}


/***********************************************************************
 *           EVENT_Signaled
 */
static BOOL32 EVENT_Signaled( K32OBJ *obj, DWORD thread_id )
{
    EVENT *event = (EVENT *)obj;
    assert( obj->type == K32OBJ_EVENT );
    return event->signaled;
}


/***********************************************************************
 *           EVENT_Satisfied
 *
 * Wait on this object has been satisfied.
 */
static BOOL32 EVENT_Satisfied( K32OBJ *obj, DWORD thread_id )
{
    EVENT *event = (EVENT *)obj;
    assert( obj->type == K32OBJ_EVENT );
    /* Reset if it's an auto-reset event */
    if (!event->manual_reset) event->signaled = FALSE;
    return FALSE;  /* Not abandoned */
}


/***********************************************************************
 *           EVENT_AddWait
 *
 * Add thread to object wait queue.
 */
static void EVENT_AddWait( K32OBJ *obj, DWORD thread_id )
{
    EVENT *event = (EVENT *)obj;
    assert( obj->type == K32OBJ_EVENT );
    THREAD_AddQueue( &event->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           EVENT_RemoveWait
 *
 * Remove thread from object wait queue.
 */
static void EVENT_RemoveWait( K32OBJ *obj, DWORD thread_id )
{
    EVENT *event = (EVENT *)obj;
    assert( obj->type == K32OBJ_EVENT );
    THREAD_RemoveQueue( &event->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}


/***********************************************************************
 *           EVENT_Destroy
 */
static void EVENT_Destroy( K32OBJ *obj )
{
    EVENT *event = (EVENT *)obj;
    assert( obj->type == K32OBJ_EVENT );
    /* There cannot be any thread on the list since the ref count is 0 */
    assert( event->wait_queue == NULL );
    obj->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, event );
}

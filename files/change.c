/*
 * Win32 file change notification functions
 *
 * Copyright 1998 Ulrich Weigand
 */

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "windows.h"
#include "winbase.h"
#include "winerror.h"
#include "process.h"
#include "thread.h"
#include "heap.h"
#include "debug.h"

static BOOL32 CHANGE_Signaled( K32OBJ *obj, DWORD thread_id );
static BOOL32 CHANGE_Satisfied( K32OBJ *obj, DWORD thread_id );
static void CHANGE_AddWait( K32OBJ *obj, DWORD thread_id );
static void CHANGE_RemoveWait( K32OBJ *obj, DWORD thread_id );
static void CHANGE_Destroy( K32OBJ *obj );

const K32OBJ_OPS CHANGE_Ops =
{
    CHANGE_Signaled,    /* signaled */
    CHANGE_Satisfied,   /* satisfied */
    CHANGE_AddWait,     /* add_wait */
    CHANGE_RemoveWait,  /* remove_wait */
    NULL,               /* read */
    NULL,               /* write */
    CHANGE_Destroy      /* destroy */
};

/* The change notification object */
typedef struct
{
    K32OBJ       header;

    LPSTR        lpPathName;
    BOOL32       bWatchSubtree;
    DWORD        dwNotifyFilter;

    THREAD_QUEUE wait_queue;    
    BOOL32       notify;

} CHANGE_OBJECT;

/***********************************************************************
 *           CHANGE_Signaled
 */
static BOOL32 CHANGE_Signaled( K32OBJ *obj, DWORD thread_id )
{
    CHANGE_OBJECT *change = (CHANGE_OBJECT *)obj;
    assert( obj->type == K32OBJ_CHANGE );
    return change->notify;
}

/***********************************************************************
 *           CHANGE_Satisfied
 *
 * Wait on this object has been satisfied.
 */
static BOOL32 CHANGE_Satisfied( K32OBJ *obj, DWORD thread_id )
{
    assert( obj->type == K32OBJ_CHANGE );
    return FALSE;  /* Not abandoned */
}

/***********************************************************************
 *           CHANGE_AddWait
 *
 * Add thread to object wait queue.
 */
static void CHANGE_AddWait( K32OBJ *obj, DWORD thread_id )
{
    CHANGE_OBJECT *change = (CHANGE_OBJECT *)obj;
    assert( obj->type == K32OBJ_CHANGE );
    THREAD_AddQueue( &change->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}

/***********************************************************************
 *           CHANGE_RemoveWait
 *
 * Remove thread from object wait queue.
 */
static void CHANGE_RemoveWait( K32OBJ *obj, DWORD thread_id )
{
    CHANGE_OBJECT *change = (CHANGE_OBJECT *)obj;
    assert( obj->type == K32OBJ_CHANGE );
    THREAD_RemoveQueue( &change->wait_queue, THREAD_ID_TO_THDB(thread_id) );
}

/****************************************************************************
 *		CHANGE_Destroy
 */
static void CHANGE_Destroy( K32OBJ *obj )
{
    CHANGE_OBJECT *change = (CHANGE_OBJECT *)obj;
    assert( obj->type == K32OBJ_CHANGE );

    if ( change->lpPathName )
    {
        HeapFree( SystemHeap, 0, change->lpPathName );
        change->lpPathName = NULL;
    }

    obj->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, change );
}

/****************************************************************************
 *		FindFirstChangeNotification32A (KERNEL32.248)
 */
HANDLE32 WINAPI FindFirstChangeNotification32A( LPCSTR lpPathName,
                                                BOOL32 bWatchSubtree,
                                                DWORD dwNotifyFilter ) 
{
    HANDLE32 handle;
    CHANGE_OBJECT *change;

    change = HeapAlloc( SystemHeap, 0, sizeof(CHANGE_OBJECT) );
    if (!change) return INVALID_HANDLE_VALUE32;

    change->header.type = K32OBJ_CHANGE;
    change->header.refcount = 1;

    change->lpPathName = HEAP_strdupA( SystemHeap, 0, lpPathName );
    change->bWatchSubtree = bWatchSubtree;
    change->dwNotifyFilter = dwNotifyFilter;

    change->wait_queue = NULL;
    change->notify = FALSE;

    handle = HANDLE_Alloc( PROCESS_Current(), &change->header, 
                           FILE_ALL_ACCESS /*FIXME*/, TRUE, -1 );
    /* If the allocation failed, the object is already destroyed */
    if (handle == INVALID_HANDLE_VALUE32) change = NULL;
    return handle;
}

/****************************************************************************
 *		FindFirstChangeNotification32W (KERNEL32.249)
 */
HANDLE32 WINAPI FindFirstChangeNotification32W( LPCWSTR lpPathName,
                                                BOOL32 bWatchSubtree,
                                                DWORD dwNotifyFilter) 
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpPathName );
    HANDLE32 ret = FindFirstChangeNotification32A( nameA, bWatchSubtree, 
                                                          dwNotifyFilter );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

/****************************************************************************
 *		FindNextChangeNotification (KERNEL32.252)
 */
BOOL32 WINAPI FindNextChangeNotification( HANDLE32 handle ) 
{
    CHANGE_OBJECT *change;

    SYSTEM_LOCK();
    if (!(change = (CHANGE_OBJECT *)HANDLE_GetObjPtr( PROCESS_Current(),
                                                      handle, K32OBJ_CHANGE, 
                                                      0 /*FIXME*/, NULL )) )
    {
        SYSTEM_UNLOCK();
        return FALSE;
    }

    change->notify = FALSE;
    K32OBJ_DecCount( &change->header );
    SYSTEM_UNLOCK();
    return TRUE;
}

/****************************************************************************
 *		FindCloseChangeNotification (KERNEL32.247)
 */
BOOL32 WINAPI FindCloseChangeNotification( HANDLE32 handle) 
{
    return CloseHandle( handle );
}


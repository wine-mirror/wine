/*
 * Win32 semaphores
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "k32obj.h"
#include "process.h"
#include "thread.h"
#include "heap.h"
#include "server/request.h"
#include "server.h"

typedef struct
{
    K32OBJ        header;
} SEMAPHORE;


/***********************************************************************
 *           CreateSemaphore32A   (KERNEL32.174)
 */
HANDLE WINAPI CreateSemaphoreA( SECURITY_ATTRIBUTES *sa, LONG initial,
                                    LONG max, LPCSTR name )
{
    struct create_semaphore_request req;
    struct create_semaphore_reply reply;
    int len = name ? strlen(name) + 1 : 0;
    HANDLE handle;
    SEMAPHORE *sem;

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    req.initial = (unsigned int)initial;
    req.max     = (unsigned int)max;
    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);

    CLIENT_SendRequest( REQ_CREATE_SEMAPHORE, -1, 2, &req, sizeof(req), name, len );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return 0;

    SYSTEM_LOCK();
    sem = (SEMAPHORE *)K32OBJ_Create( K32OBJ_SEMAPHORE, sizeof(*sem),
                                      name, reply.handle, SEMAPHORE_ALL_ACCESS,
                                      sa, &handle);
    if (sem) K32OBJ_DecCount( &sem->header );
    SYSTEM_UNLOCK();
    if (handle == INVALID_HANDLE_VALUE) handle = 0;
    return handle;
}


/***********************************************************************
 *           CreateSemaphore32W   (KERNEL32.175)
 */
HANDLE WINAPI CreateSemaphoreW( SECURITY_ATTRIBUTES *sa, LONG initial,
                                    LONG max, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE ret = CreateSemaphoreA( sa, initial, max, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           OpenSemaphore32A   (KERNEL32.545)
 */
HANDLE WINAPI OpenSemaphoreA( DWORD access, BOOL inherit, LPCSTR name )
{
    HANDLE handle = 0;
    K32OBJ *obj;
    struct open_named_obj_request req;
    struct open_named_obj_reply reply;
    int len = name ? strlen(name) + 1 : 0;

    req.type    = OPEN_SEMAPHORE;
    req.access  = access;
    req.inherit = inherit;
    CLIENT_SendRequest( REQ_OPEN_NAMED_OBJ, -1, 2, &req, sizeof(req), name, len );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle != -1)
    {
        SYSTEM_LOCK();
        if ((obj = K32OBJ_FindNameType( name, K32OBJ_SEMAPHORE )) != NULL)
        {
            handle = HANDLE_Alloc( PROCESS_Current(), obj, access, inherit, reply.handle );
            K32OBJ_DecCount( obj );
            if (handle == INVALID_HANDLE_VALUE)
                handle = 0; /* must return 0 on failure, not -1 */
        }
        else CLIENT_CloseHandle( reply.handle );
        SYSTEM_UNLOCK();
    }
    return handle;
}


/***********************************************************************
 *           OpenSemaphore32W   (KERNEL32.546)
 */
HANDLE WINAPI OpenSemaphoreW( DWORD access, BOOL inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE ret = OpenSemaphoreA( access, inherit, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           ReleaseSemaphore   (KERNEL32.583)
 */
BOOL WINAPI ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    struct release_semaphore_request req;
    struct release_semaphore_reply reply;

    if (count < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                         K32OBJ_SEMAPHORE, SEMAPHORE_MODIFY_STATE );
    if (req.handle == -1) return FALSE;
    req.count = (unsigned int)count;
    CLIENT_SendRequest( REQ_RELEASE_SEMAPHORE, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    if (previous) *previous = reply.prev_count;
    return TRUE;
}

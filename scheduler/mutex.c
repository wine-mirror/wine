/*
 * Win32 mutexes
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "heap.h"
#include "server/request.h"
#include "server.h"


/***********************************************************************
 *           CreateMutex32A   (KERNEL32.166)
 */
HANDLE WINAPI CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    struct create_mutex_request req;
    struct create_mutex_reply reply;
    int len = name ? strlen(name) + 1 : 0;

    req.owned   = owner;
    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    CLIENT_SendRequest( REQ_CREATE_MUTEX, -1, 2, &req, sizeof(req), name, len );
    SetLastError(0);
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return 0;
    return reply.handle;
}


/***********************************************************************
 *           CreateMutex32W   (KERNEL32.167)
 */
HANDLE WINAPI CreateMutexW( SECURITY_ATTRIBUTES *sa, BOOL owner,
                                LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE ret = CreateMutexA( sa, owner, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           OpenMutex32A   (KERNEL32.541)
 */
HANDLE WINAPI OpenMutexA( DWORD access, BOOL inherit, LPCSTR name )
{
    struct open_mutex_request req;
    struct open_mutex_reply reply;
    int len = name ? strlen(name) + 1 : 0;

    req.access  = access;
    req.inherit = inherit;
    CLIENT_SendRequest( REQ_OPEN_MUTEX, -1, 2, &req, sizeof(req), name, len );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return 0; /* must return 0 on failure, not -1 */
    return reply.handle;
}


/***********************************************************************
 *           OpenMutex32W   (KERNEL32.542)
 */
HANDLE WINAPI OpenMutexW( DWORD access, BOOL inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE ret = OpenMutexA( access, inherit, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           ReleaseMutex   (KERNEL32.582)
 */
BOOL WINAPI ReleaseMutex( HANDLE handle )
{
    struct release_mutex_request req;

    req.handle = handle;
    CLIENT_SendRequest( REQ_RELEASE_MUTEX, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}

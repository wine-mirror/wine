/*
 * Win32 mutexes
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "heap.h"
#include "server.h"


/***********************************************************************
 *           CreateMutex32A   (KERNEL32.166)
 */
HANDLE WINAPI CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    struct create_mutex_request *req = get_req_buffer();

    req->owned   = owner;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    lstrcpynA( req->name, name ? name : "", server_remaining(req->name) );
    SetLastError(0);
    server_call( REQ_CREATE_MUTEX );
    if (req->handle == -1) return 0;
    return req->handle;
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
    struct open_mutex_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    lstrcpynA( req->name, name ? name : "", server_remaining(req->name) );
    server_call( REQ_OPEN_MUTEX );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
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
    struct release_mutex_request *req = get_req_buffer();
    req->handle = handle;
    return !server_call( REQ_RELEASE_MUTEX );
}

/*
 * Win32 mutexes
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "server.h"


/***********************************************************************
 *           CreateMutexA   (KERNEL32.166)
 */
HANDLE WINAPI CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    struct create_mutex_request *req = get_req_buffer();

    req->owned   = owner;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyAtoW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_MUTEX );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *           CreateMutexW   (KERNEL32.167)
 */
HANDLE WINAPI CreateMutexW( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCWSTR name )
{
    struct create_mutex_request *req = get_req_buffer();

    req->owned   = owner;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_MUTEX );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *           OpenMutexA   (KERNEL32.541)
 */
HANDLE WINAPI OpenMutexA( DWORD access, BOOL inherit, LPCSTR name )
{
    struct open_mutex_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyAtoW( req->name, name );
    server_call( REQ_OPEN_MUTEX );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *           OpenMutexW   (KERNEL32.542)
 */
HANDLE WINAPI OpenMutexW( DWORD access, BOOL inherit, LPCWSTR name )
{
    struct open_mutex_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyW( req->name, name );
    server_call( REQ_OPEN_MUTEX );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
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

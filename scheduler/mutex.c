/*
 * Win32 mutexes
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "server.h"


/***********************************************************************
 *           CreateMutexA   (KERNEL32.166)
 */
HANDLE WINAPI CreateMutexA( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct create_mutex_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->owned   = owner;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        server_call( REQ_CREATE_MUTEX );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           CreateMutexW   (KERNEL32.167)
 */
HANDLE WINAPI CreateMutexW( SECURITY_ATTRIBUTES *sa, BOOL owner, LPCWSTR name )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct create_mutex_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->owned   = owner;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        server_call( REQ_CREATE_MUTEX );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           OpenMutexA   (KERNEL32.541)
 */
HANDLE WINAPI OpenMutexA( DWORD access, BOOL inherit, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct open_mutex_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        server_call( REQ_OPEN_MUTEX );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           OpenMutexW   (KERNEL32.542)
 */
HANDLE WINAPI OpenMutexW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct open_mutex_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        server_call( REQ_OPEN_MUTEX );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           ReleaseMutex   (KERNEL32.582)
 */
BOOL WINAPI ReleaseMutex( HANDLE handle )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct release_mutex_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        ret = !server_call( REQ_RELEASE_MUTEX );
    }
    SERVER_END_REQ;
    return ret;
}

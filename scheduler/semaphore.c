/*
 * Win32 semaphores
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
 *           CreateSemaphoreA   (KERNEL32.174)
 */
HANDLE WINAPI CreateSemaphoreA( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }

    SERVER_START_REQ
    {
        struct create_semaphore_request *req = server_alloc_req( sizeof(*req),
                                                                 len * sizeof(WCHAR) );

        req->initial = (unsigned int)initial;
        req->max     = (unsigned int)max;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        server_call( REQ_CREATE_SEMAPHORE );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           CreateSemaphoreW   (KERNEL32.175)
 */
HANDLE WINAPI CreateSemaphoreW( SECURITY_ATTRIBUTES *sa, LONG initial,
                                    LONG max, LPCWSTR name )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }

    SERVER_START_REQ
    {
        struct create_semaphore_request *req = server_alloc_req( sizeof(*req),
                                                                 len * sizeof(WCHAR) );

        req->initial = (unsigned int)initial;
        req->max     = (unsigned int)max;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        server_call( REQ_CREATE_SEMAPHORE );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           OpenSemaphoreA   (KERNEL32.545)
 */
HANDLE WINAPI OpenSemaphoreA( DWORD access, BOOL inherit, LPCSTR name )
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
        struct open_semaphore_request *req = server_alloc_req( sizeof(*req),
                                                               len * sizeof(WCHAR) );
        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        server_call( REQ_OPEN_SEMAPHORE );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           OpenSemaphoreW   (KERNEL32.546)
 */
HANDLE WINAPI OpenSemaphoreW( DWORD access, BOOL inherit, LPCWSTR name )
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
        struct open_semaphore_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );
        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        server_call( REQ_OPEN_SEMAPHORE );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           ReleaseSemaphore   (KERNEL32.583)
 */
BOOL WINAPI ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    NTSTATUS status = NtReleaseSemaphore( handle, count, previous );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/*
 * Win32 semaphores
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "server.h"


/***********************************************************************
 *           CreateSemaphore32A   (KERNEL32.174)
 */
HANDLE WINAPI CreateSemaphoreA( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max, LPCSTR name )
{
    struct create_semaphore_request *req = get_req_buffer();

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    req->initial = (unsigned int)initial;
    req->max     = (unsigned int)max;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyAtoW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_SEMAPHORE );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *           CreateSemaphore32W   (KERNEL32.175)
 */
HANDLE WINAPI CreateSemaphoreW( SECURITY_ATTRIBUTES *sa, LONG initial,
                                    LONG max, LPCWSTR name )
{
    struct create_semaphore_request *req = get_req_buffer();

    /* Check parameters */

    if ((max <= 0) || (initial < 0) || (initial > max))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    req->initial = (unsigned int)initial;
    req->max     = (unsigned int)max;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_SEMAPHORE );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *           OpenSemaphore32A   (KERNEL32.545)
 */
HANDLE WINAPI OpenSemaphoreA( DWORD access, BOOL inherit, LPCSTR name )
{
    struct open_semaphore_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyAtoW( req->name, name );
    server_call( REQ_OPEN_SEMAPHORE );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *           OpenSemaphore32W   (KERNEL32.546)
 */
HANDLE WINAPI OpenSemaphoreW( DWORD access, BOOL inherit, LPCWSTR name )
{
    struct open_semaphore_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyW( req->name, name );
    server_call( REQ_OPEN_SEMAPHORE );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *           ReleaseSemaphore   (KERNEL32.583)
 */
BOOL WINAPI ReleaseSemaphore( HANDLE handle, LONG count, LONG *previous )
{
    BOOL ret = FALSE;
    struct release_semaphore_request *req = get_req_buffer();

    if (count < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    req->handle = handle;
    req->count  = (unsigned int)count;
    if (!server_call( REQ_RELEASE_SEMAPHORE ))
    {
        if (previous) *previous = req->prev_count;
        ret = TRUE;
    }
    return ret;
}

/*
 * Win32 process handles
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include "winbase.h"
#include "server.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32)

/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL WINAPI CloseHandle( HANDLE handle )
{
    struct close_handle_request *req = get_req_buffer();
    /* stdio handles need special treatment */
    if ((handle == STD_INPUT_HANDLE) ||
        (handle == STD_OUTPUT_HANDLE) ||
        (handle == STD_ERROR_HANDLE))
        handle = GetStdHandle( handle );
    req->handle = handle;
    return !server_call( REQ_CLOSE_HANDLE );
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.336)
 */
BOOL WINAPI GetHandleInformation( HANDLE handle, LPDWORD flags )
{
    struct get_handle_info_request *req = get_req_buffer();
    req->handle = handle;
    if (server_call( REQ_GET_HANDLE_INFO )) return FALSE;
    if (flags) *flags = req->flags;
    return TRUE;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.653)
 */
BOOL WINAPI SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    struct set_handle_info_request *req = get_req_buffer();
    req->handle = handle;
    req->flags  = flags;
    req->mask   = mask;
    return !server_call( REQ_SET_HANDLE_INFO );
}


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.192)
 */
BOOL WINAPI DuplicateHandle( HANDLE source_process, HANDLE source,
                               HANDLE dest_process, HANDLE *dest,
                               DWORD access, BOOL inherit, DWORD options )
{
    struct dup_handle_request *req = get_req_buffer();

    req->src_process = source_process;
    req->src_handle  = source;
    req->dst_process = dest_process;
    req->access      = access;
    req->inherit     = inherit;
    req->options     = options;

    if (server_call( REQ_DUP_HANDLE )) return FALSE;
    if (dest) *dest = req->handle;
    return TRUE;
}


/***********************************************************************
 *           ConvertToGlobalHandle    		(KERNEL32)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    struct dup_handle_request *req = get_req_buffer();

    req->src_process = GetCurrentProcess();
    req->src_handle  = hSrc;
    req->dst_process = -1;
    req->access      = 0;
    req->inherit     = FALSE;
    req->options     = DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS | DUP_HANDLE_CLOSE_SOURCE;

    server_call( REQ_DUP_HANDLE );
    return req->handle;
}

/***********************************************************************
 *           SetHandleContext    		(KERNEL32)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context) {
    FIXME("(%d,%ld), stub. The external WSOCK32 will not work with WINE, do not use it.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetHandleContext    		(KERNEL32)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd) {
    FIXME("(%d), stub. The external WSOCK32 will not work with WINE, do not use it.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *           CreateSocketHandle    		(KERNEL32)
 */
HANDLE WINAPI CreateSocketHandle(void) {
    FIXME("(), stub. The external WSOCK32 will not work with WINE, do not use it.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}

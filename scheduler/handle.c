/*
 * Win32 process handles
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "winbase.h"
#include "server.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);

/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL WINAPI CloseHandle( HANDLE handle )
{
    NTSTATUS status;

    /* stdio handles need special treatment */
    if ((handle == STD_INPUT_HANDLE) ||
        (handle == STD_OUTPUT_HANDLE) ||
        (handle == STD_ERROR_HANDLE))
        handle = GetStdHandle( handle );

    status = NtClose( handle );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.336)
 */
BOOL WINAPI GetHandleInformation( HANDLE handle, LPDWORD flags )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct set_handle_info_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->flags  = 0;
        req->mask   = 0;
        req->fd     = -1;
        ret = !server_call( REQ_SET_HANDLE_INFO );
        if (ret && flags) *flags = req->old_flags;
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.653)
 */
BOOL WINAPI SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct set_handle_info_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->flags  = flags;
        req->mask   = mask;
        req->fd     = -1;
        ret = !server_call( REQ_SET_HANDLE_INFO );
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.192)
 */
BOOL WINAPI DuplicateHandle( HANDLE source_process, HANDLE source,
                               HANDLE dest_process, HANDLE *dest,
                               DWORD access, BOOL inherit, DWORD options )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct dup_handle_request *req = server_alloc_req( sizeof(*req), 0 );

        req->src_process = source_process;
        req->src_handle  = source;
        req->dst_process = dest_process;
        req->access      = access;
        req->inherit     = inherit;
        req->options     = options;

        ret = !server_call( REQ_DUP_HANDLE );
        if (ret)
        {
            if (dest) *dest = req->handle;
            if (req->fd != -1) close( req->fd );
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           ConvertToGlobalHandle    		(KERNEL32)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    HANDLE ret = INVALID_HANDLE_VALUE;
    DuplicateHandle( GetCurrentProcess(), hSrc, GetCurrentProcess(), &ret, 0, FALSE,
                     DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS | DUP_HANDLE_CLOSE_SOURCE );
    return ret;
}

/***********************************************************************
 *           SetHandleContext    		(KERNEL32)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context) {
    FIXME("(%d,%ld), stub. In case this got called by WSOCK32/WS2_32: the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetHandleContext    		(KERNEL32)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd) {
    FIXME("(%d), stub. In case this got called by WSOCK32/WS2_32: the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *           CreateSocketHandle    		(KERNEL32)
 */
HANDLE WINAPI CreateSocketHandle(void) {
    FIXME("(), stub. In case this got called by WSOCK32/WS2_32: the external WINSOCK DLLs won't work with WINE, don't use them.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}

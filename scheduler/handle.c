/*
 * Win32 process handles
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include "winbase.h"
#include "server.h"


/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL WINAPI CloseHandle( HANDLE handle )
{
    struct close_handle_request req = { handle };
    CLIENT_SendRequest( REQ_CLOSE_HANDLE, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.336)
 */
BOOL WINAPI GetHandleInformation( HANDLE handle, LPDWORD flags )
{
    struct get_handle_info_request req;
    struct get_handle_info_reply reply;

    req.handle = handle;
    CLIENT_SendRequest( REQ_GET_HANDLE_INFO, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    if (flags) *flags = reply.flags;
    return TRUE;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.653)
 */
BOOL WINAPI SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    struct set_handle_info_request req;

    req.handle = handle;
    req.flags  = flags;
    req.mask   = mask;
    CLIENT_SendRequest( REQ_SET_HANDLE_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.192)
 */
BOOL WINAPI DuplicateHandle( HANDLE source_process, HANDLE source,
                               HANDLE dest_process, HANDLE *dest,
                               DWORD access, BOOL inherit, DWORD options )
{
    struct dup_handle_request req;
    struct dup_handle_reply reply;

    req.src_process = source_process;
    req.src_handle  = source;
    req.dst_process = dest_process;
    req.access      = access;
    req.inherit     = inherit;
    req.options     = options;

    CLIENT_SendRequest( REQ_DUP_HANDLE, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    if (dest) *dest = reply.handle;
    return TRUE;
}


/***********************************************************************
 *           ConvertToGlobalHandle    		(KERNEL32)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    struct dup_handle_request req;
    struct dup_handle_reply reply;

    req.src_process = GetCurrentProcess();
    req.src_handle  = hSrc;
    req.dst_process = -1;
    req.access      = 0;
    req.inherit     = FALSE;
    req.options     = DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS;

    CLIENT_SendRequest( REQ_DUP_HANDLE, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    return reply.handle;
}

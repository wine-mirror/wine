/*
 * Win32 pipes
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include "winerror.h"
#include "winbase.h"
#include "server.h"


/***********************************************************************
 *	CreatePipe    (KERNEL32.170)
 */
BOOL WINAPI CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe,
                          LPSECURITY_ATTRIBUTES sa, DWORD size )
{
    struct create_pipe_request req;
    struct create_pipe_reply reply;
    int len;

    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    CLIENT_SendRequest( REQ_CREATE_PIPE, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitReply( &len, NULL, 1, &reply, sizeof(reply) ) != ERROR_SUCCESS)
        return FALSE;
    *hReadPipe  = reply.handle_read;
    *hWritePipe = reply.handle_write;
    return TRUE;
}

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
    BOOL ret;
    SERVER_START_REQ
    {
        struct create_pipe_request *req = server_alloc_req( sizeof(*req), 0 );

        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if ((ret = !server_call( REQ_CREATE_PIPE )))
        {
            *hReadPipe  = req->handle_read;
            *hWritePipe = req->handle_write;
        }
    }
    SERVER_END_REQ;
    return ret;
}

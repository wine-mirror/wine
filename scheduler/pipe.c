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
    struct create_pipe_request *req = get_req_buffer();

    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    if (server_call( REQ_CREATE_PIPE )) return FALSE;
    *hReadPipe  = req->handle_read;
    *hWritePipe = req->handle_write;
    return TRUE;
}

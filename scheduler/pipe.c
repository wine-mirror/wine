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
 *	CreatePipe    (KERNEL32.@)
 */
BOOL WINAPI CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe,
                          LPSECURITY_ATTRIBUTES sa, DWORD size )
{
    BOOL ret;
    SERVER_START_REQ( create_pipe )
    {
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if ((ret = !SERVER_CALL_ERR()))
        {
            *hReadPipe  = req->handle_read;
            *hWritePipe = req->handle_write;
        }
    }
    SERVER_END_REQ;
    return ret;
}

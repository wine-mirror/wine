/*
 * Win32 pipes
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include "windows.h"
#include "winerror.h"
#include "k32obj.h"
#include "process.h"
#include "thread.h"
#include "heap.h"
#include "server/request.h"
#include "server.h"

typedef struct _PIPE
{
    K32OBJ         header;
} PIPE;

static void PIPE_Destroy( K32OBJ *obj );

const K32OBJ_OPS PIPE_Ops =
{
    NULL,             /* signaled */
    NULL,             /* satisfied */
    NULL,             /* add_wait */
    NULL,             /* remove_wait */
    NULL,             /* read */
    NULL,             /* write */
    PIPE_Destroy      /* destroy */
};


/***********************************************************************
 *	CreatePipe    (KERNEL32.170)
 */
BOOL32 WINAPI CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe,
                          LPSECURITY_ATTRIBUTES sa, DWORD size )
{
    struct create_pipe_request req;
    struct create_pipe_reply reply;
    PIPE *read_pipe, *write_pipe;
    int len;

    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    CLIENT_SendRequest( REQ_CREATE_PIPE, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitReply( &len, NULL, 1, &reply, sizeof(reply) ) != ERROR_SUCCESS)
        return FALSE;

    SYSTEM_LOCK();
    if (!(read_pipe = (PIPE *)K32OBJ_Create( K32OBJ_PIPE, sizeof(*read_pipe),
                                             NULL, reply.handle_read,
                                             STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_READ,
                                             sa, hReadPipe )))
    {
        CLIENT_CloseHandle( reply.handle_write );
        /* handle_read already closed by K32OBJ_Create */
        SYSTEM_UNLOCK();
        return FALSE;
    }
    K32OBJ_DecCount( &read_pipe->header );
    if (!(write_pipe = (PIPE *)K32OBJ_Create( K32OBJ_PIPE, sizeof(*write_pipe),
                                              NULL, reply.handle_write,
                                              STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_WRITE,
                                              sa, hWritePipe )))
    {
        CloseHandle( *hReadPipe );
        *hReadPipe = INVALID_HANDLE_VALUE32;
        SYSTEM_UNLOCK();
        return FALSE;
    }
    /* everything OK */
    K32OBJ_DecCount( &write_pipe->header );
    SetLastError(0); /* FIXME */
    SYSTEM_UNLOCK();
    return TRUE;
}


/***********************************************************************
 *           PIPE_Destroy
 */
static void PIPE_Destroy( K32OBJ *obj )
{
    PIPE *pipe = (PIPE *)obj;
    assert( obj->type == K32OBJ_PIPE );
    obj->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, pipe );
}


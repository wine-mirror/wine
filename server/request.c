/*
 * Server-side request handling
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "winerror.h"
#include "winnt.h"
#include "winbase.h"
#include "wincon.h"
#define WANT_REQUEST_HANDLERS
#include "server.h"
#include "thread.h"
 
struct thread *current = NULL;  /* thread handling the current request */

/* complain about a protocol error and terminate the client connection */
void fatal_protocol_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%p: ", current );
    vfprintf( stderr, err, args );
    va_end( args );
    remove_client( current->client, -2 );
}

/* call a request handler */
void call_req_handler( struct thread *thread, enum request req,
                       void *data, int len, int fd )
{
    const struct handler *handler = &req_handlers[req];
    char *ptr;

    current = thread;
    if ((req < 0) || (req >= REQ_NB_REQUESTS))
    {
        fatal_protocol_error( "unknown request %d\n", req );
        return;
    }

    if (len < handler->min_size)
    {
        fatal_protocol_error( "req %d bad length %d < %d\n", req, len, handler->min_size );
        return;
    }

    /* now call the handler */
    if (current)
    {
        CLEAR_ERROR();
        if (debug_level) trace_request( req, data, len, fd );
    }
    len -= handler->min_size;
    ptr = (char *)data + handler->min_size;
    handler->handler( data, ptr, len, fd );
    current = NULL;
}

/* handle a client timeout */
void call_timeout_handler( void *thread )
{
    current = (struct thread *)thread;
    if (debug_level) trace_timeout();
    CLEAR_ERROR();
    thread_timeout();
    current = NULL;
}

/* a thread has been killed */
void call_kill_handler( struct thread *thread, int exit_code )
{
    /* must be reentrant WRT call_req_handler */
    struct thread *old_current = current;
    current = thread;
    if (current)
    {
        if (debug_level) trace_kill( exit_code );
        thread_killed( current, exit_code );
    }
    current = (old_current != thread) ? old_current : NULL;
}

/* set the debug level */
DECL_HANDLER(set_debug)
{
    debug_level = req->level;
    /* Make sure last_req is initialized */
    current->last_req = REQ_SET_DEBUG;
    CLEAR_ERROR();
    send_reply( current, -1, 0 );
}

/* debugger support operations */
DECL_HANDLER(debugger)
{
    switch ( req->op )
    {
    case DEBUGGER_FREEZE_ALL:
        suspend_all_threads();
        break;

    case DEBUGGER_UNFREEZE_ALL:
        resume_all_threads();
        break;
    }

    send_reply( current, -1, 0 );
}

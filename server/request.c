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
#include "thread.h"
#include "server.h"
#define WANT_REQUEST_HANDLERS
#include "request.h"
 
struct thread *current = NULL;  /* thread handling the current request */

/* complain about a protocol error and terminate the client connection */
void fatal_protocol_error( struct thread *thread, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%p: ", thread );
    vfprintf( stderr, err, args );
    va_end( args );
    remove_client( thread->client, PROTOCOL_ERROR );
}

/* call a request handler */
void call_req_handler( struct thread *thread, enum request req, int fd )
{
    current = thread;
    clear_error();

    if (debug_level) trace_request( req, fd );

    if (req < REQ_NB_REQUESTS)
    {
        req_handlers[req].handler( current->buffer, fd );
        if (current && current->state != SLEEPING) send_reply( current );
        current = NULL;
        return;
    }
    fatal_protocol_error( current, "bad request %d\n", req );
}

/* handle a client timeout */
void call_timeout_handler( void *thread )
{
    current = (struct thread *)thread;
    if (debug_level) trace_timeout();
    clear_error();
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

/* set the fd to pass to the thread */
void set_reply_fd( struct thread *thread, int pass_fd )
{
    client_pass_fd( thread->client, pass_fd );
}

/* send a reply to a thread */
void send_reply( struct thread *thread )
{
    if (thread->state == SLEEPING) thread->state = RUNNING;
    client_reply( thread->client, thread->error );
}

/* set the debug level */
DECL_HANDLER(set_debug)
{
    debug_level = req->level;
    /* Make sure last_req is initialized */
    current->last_req = REQ_SET_DEBUG;
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
}

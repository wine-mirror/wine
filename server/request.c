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
void fatal_protocol_error( const char *err )
{
    unsigned char *p;

    fprintf( stderr, "Protocol error:%p: %s\n    request:", current, err );
    for (p = (unsigned char *)current->buffer; p < (unsigned char *)current->req_end; p++)
        fprintf( stderr, " %02x", *p );
    fprintf( stderr, "\n" );
    remove_client( current->client, -2 );
}

/* call a request handler */
void call_req_handler( struct thread *thread, int fd )
{
    const struct handler *handler;
    struct header *head;
    unsigned int req, len;

    current = thread;
    assert (current);

    head = (struct header *)current->buffer;

    req = head->type;
    len = head->len;

    /* set the buffer pointers */
    current->req_pos = current->reply_pos = (char *)current->buffer + sizeof(struct header);
    current->req_end = (char *)current->buffer + len;
    clear_error();

    if ((len < sizeof(struct header)) || (len > MAX_MSG_LENGTH)) goto bad_header;
    if (req >= REQ_NB_REQUESTS) goto bad_header;

    if (debug_level) trace_request( req, fd );

    /* now call the handler */
    handler = &req_handlers[req];
    if (!check_req_data( handler->min_size )) goto bad_request;
    handler->handler( get_req_data( handler->min_size ), fd );
    if (current && current->state != SLEEPING) send_reply( current );
    current = NULL;
    return;

 bad_header:
    /* dump only the header */
    current->req_end = (char *)current->buffer + sizeof(struct header);
 bad_request:
    fatal_protocol_error( "bad request" );
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
    struct header *head = thread->buffer;
    int len = (char *)thread->reply_pos - (char *)thread->buffer;

    assert( len < MAX_MSG_LENGTH );

    head->len  = len;
    head->type = thread->error;
    if (thread->state == SLEEPING) thread->state = RUNNING;
    client_reply( thread->client );
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

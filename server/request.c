/*
 * Server-side request handling
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

#include "winerror.h"
#include "winnt.h"
#include "winbase.h"
#define WANT_REQUEST_HANDLERS
#include "server.h"
#include "server/request.h"
#include "server/thread.h"


struct thread *current = NULL;  /* thread handling the current request */

/* complain about a protocol error and terminate the client connection */
static void fatal_protocol_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%p: ", current );
    vfprintf( stderr, err, args );
    va_end( args );
    remove_client( current->client_fd, -2 );
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
        fatal_protocol_error( "req %d bad length %d < %d)\n", req, len, handler->min_size );
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

/* handle a client timeout (unused for now) */
void call_timeout_handler( struct thread *thread )
{
    current = thread;
    if (debug_level) trace_timeout();
    CLEAR_ERROR();
    send_reply( current, -1, 0 );
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


/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct new_thread_reply reply;
    struct thread *new_thread;
    int new_fd, err;

    if ((new_fd = dup(fd)) == -1)
    {
        new_thread = NULL;
        err = ERROR_TOO_MANY_OPEN_FILES;
        goto done;
    }
    if (!(new_thread = create_thread( new_fd, req->pid, &reply.thandle,
                                      &reply.phandle )))
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        close( new_fd );
        goto done;
    }
    reply.tid = new_thread;
    reply.pid = new_thread->process;
    err = ERROR_SUCCESS;

 done:
    if (!current)
    {
        /* first client doesn't have a current */
        struct iovec vec = { &reply, sizeof(reply) };
        send_reply_v( get_initial_client_fd(), err, -1, &vec, 1 );
    }
    else
    {
        SET_ERROR( err );
        send_reply( current, -1, 1, &reply, sizeof(reply) );
    }
}

/* create a new thread */
DECL_HANDLER(init_thread)
{
    current->unix_pid = req->unix_pid;
    if (!(current->name = malloc( len + 1 )))
    {
        SET_ERROR( ERROR_NOT_ENOUGH_MEMORY );
        goto done;
    }
    memcpy( current->name, data, len );
    current->name[len] = '\0';
    CLEAR_ERROR();
 done:
    send_reply( current, -1, 0 );
}

/* terminate a process */
DECL_HANDLER(terminate_process)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_TERMINATE )))
    {
        kill_process( process, req->exit_code );
        release_object( process );
    }
    if (current) send_reply( current, -1, 0 );
}

/* terminate a thread */
DECL_HANDLER(terminate_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_TERMINATE )))
    {
        kill_thread( thread, req->exit_code );
        release_object( thread );
    }
    if (current) send_reply( current, -1, 0 );
}

/* close a handle */
DECL_HANDLER(close_handle)
{
    close_handle( current->process, req->handle );
    send_reply( current, -1, 0 );
}

/* duplicate a handle */
DECL_HANDLER(dup_handle)
{
    struct dup_handle_reply reply = { -1 };
    struct process *src, *dst;

    if ((src = get_process_from_handle( req->src_process, PROCESS_DUP_HANDLE )))
    {
        if ((dst = get_process_from_handle( req->dst_process, PROCESS_DUP_HANDLE )))
        {
            reply.handle = duplicate_handle( src, req->src_handle, dst, req->dst_handle,
                                             req->access, req->inherit, req->options );
            release_object( dst );
        }
        /* close the handle no matter what happened */
        if (req->options & DUPLICATE_CLOSE_SOURCE)
            close_handle( src, req->src_handle );
        release_object( src );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* fetch information about a process */
DECL_HANDLER(get_process_info)
{
    struct process *process;
    struct get_process_info_reply reply = { 0, 0 };

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        get_process_info( process, &reply );
        release_object( process );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle a process */
DECL_HANDLER(open_process)
{
    struct open_process_reply reply = { -1 };
    struct process *process = get_process_from_id( req->pid );
    if (process)
    {
        reply.handle = alloc_handle( current->process, process,
                                     req->access, req->inherit );
        release_object( process );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

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
#include "server/request.h"
#include "server/process.h"
#include "server/thread.h"

/* check that the string is NULL-terminated and that the len is correct */
#define CHECK_STRING(func,str,len) \
  do { if (((str)[(len)-1] || strlen(str) != (len)-1)) \
         fatal_protocol_error( "%s: invalid string '%.*s'\n", (func), (len), (str) ); \
     } while(0)
 
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


/* create a new process */
DECL_HANDLER(new_process)
{
    struct new_process_reply reply;
    struct process *process;

    if ((process = create_process( req )))
    {
        reply.pid    = process;
        reply.handle = alloc_handle( current->process, process,
                                     PROCESS_ALL_ACCESS, req->inherit );
        release_object( process );
    }
    else
    {
        reply.handle = -1;
        reply.pid    = NULL;
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct new_thread_reply reply;
    int new_fd;

    if ((new_fd = dup(fd)) != -1)
    {
        reply.tid = create_thread( new_fd, req->pid, req->suspend,
                                   req->inherit, &reply.handle );
        if (!reply.tid) close( new_fd );
    }
    else
        SET_ERROR( ERROR_TOO_MANY_OPEN_FILES );

    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* initialize a new process */
DECL_HANDLER(init_process)
{
    struct init_process_reply reply;
    if (current->state != RUNNING)
    {
        fatal_protocol_error( "init_process: init_thread not called yet\n" );
        return;
    }
    if (!get_process_init_info( current->process, &reply ))
    {
        fatal_protocol_error( "init_process: called twice\n" );
        return;
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* initialize a new thread */
DECL_HANDLER(init_thread)
{
    struct init_thread_reply reply;

    if (current->state != STARTING)
    {
        fatal_protocol_error( "init_thread: already running\n" );
        return;
    }
    current->state    = RUNNING;
    current->unix_pid = req->unix_pid;
    if (current->suspend > 0)
        kill( current->unix_pid, SIGSTOP );
    reply.pid = current->process;
    reply.tid = current;
    send_reply( current, -1, 1, &reply, sizeof(reply) );
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

/* get information about a handle */
DECL_HANDLER(get_handle_info)
{
    struct get_handle_info_reply reply;
    reply.flags = set_handle_info( current->process, req->handle, 0, 0 );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set a handle information */
DECL_HANDLER(set_handle_info)
{
    set_handle_info( current->process, req->handle, req->mask, req->flags );
    send_reply( current, -1, 0 );
}

/* duplicate a handle */
DECL_HANDLER(dup_handle)
{
    struct dup_handle_reply reply = { -1 };
    struct process *src, *dst;

    if ((src = get_process_from_handle( req->src_process, PROCESS_DUP_HANDLE )))
    {
        if (req->options & DUP_HANDLE_MAKE_GLOBAL)
        {
            reply.handle = duplicate_handle( src, req->src_handle, NULL,
                                             req->access, req->inherit, req->options );
        }
        else if ((dst = get_process_from_handle( req->dst_process, PROCESS_DUP_HANDLE )))
        {
            reply.handle = duplicate_handle( src, req->src_handle, dst,
                                             req->access, req->inherit, req->options );
            release_object( dst );
        }
        /* close the handle no matter what happened */
        if (req->options & DUP_HANDLE_CLOSE_SOURCE)
            close_handle( src, req->src_handle );
        release_object( src );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* fetch information about a process */
DECL_HANDLER(get_process_info)
{
    struct process *process;
    struct get_process_info_reply reply = { 0, 0, 0 };

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        get_process_info( process, &reply );
        release_object( process );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set information about a process */
DECL_HANDLER(set_process_info)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_SET_INFORMATION )))
    {
        set_process_info( process, req );
        release_object( process );
    }
    send_reply( current, -1, 0 );
}

/* fetch information about a thread */
DECL_HANDLER(get_thread_info)
{
    struct thread *thread;
    struct get_thread_info_reply reply = { 0, 0 };

    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION )))
    {
        get_thread_info( thread, &reply );
        release_object( thread );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set information about a thread */
DECL_HANDLER(set_thread_info)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_INFORMATION )))
    {
        set_thread_info( thread, req );
        release_object( thread );
    }
    send_reply( current, -1, 0 );
}

/* suspend a thread */
DECL_HANDLER(suspend_thread)
{
    struct thread *thread;
    struct suspend_thread_reply reply = { -1 };
    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        reply.count = suspend_thread( thread );
        release_object( thread );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
    
}

/* resume a thread */
DECL_HANDLER(resume_thread)
{
    struct thread *thread;
    struct resume_thread_reply reply = { -1 };
    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        reply.count = resume_thread( thread );
        release_object( thread );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
    
}

/* queue an APC for a thread */
DECL_HANDLER(queue_apc)
{
    struct thread *thread;
    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT )))
    {
        thread_queue_apc( thread, req->func, req->param );
        release_object( thread );
    }
    send_reply( current, -1, 0 );
}

/* open a handle to a process */
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

/* select on a handle list */
DECL_HANDLER(select)
{
    if (len != req->count * sizeof(int))
        fatal_protocol_error( "select: bad length %d for %d handles\n",
                              len, req->count );
    sleep_on( current, req->count, (int *)data, req->flags, req->timeout );
}

/* create an event */
DECL_HANDLER(create_event)
{
    struct create_event_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_event", name, len );

    obj = create_event( name, req->manual_reset, req->initial_state );
    if (obj)
    {
        reply.handle = alloc_handle( current->process, obj, EVENT_ALL_ACCESS, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* do an event operation */
DECL_HANDLER(event_op)
{
    switch(req->op)
    {
    case PULSE_EVENT:
        pulse_event( req->handle );
        break;
    case SET_EVENT:
        set_event( req->handle );
        break;
    case RESET_EVENT:
        reset_event( req->handle );
        break;
    default:
        fatal_protocol_error( "event_op: invalid operation %d\n", req->op );
    }
    send_reply( current, -1, 0 );
}

/* create a mutex */
DECL_HANDLER(create_mutex)
{
    struct create_mutex_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_mutex", name, len );

    obj = create_mutex( name, req->owned );
    if (obj)
    {
        reply.handle = alloc_handle( current->process, obj, MUTEX_ALL_ACCESS, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* release a mutex */
DECL_HANDLER(release_mutex)
{
    if (release_mutex( req->handle )) CLEAR_ERROR();
    send_reply( current, -1, 0 );
}

/* create a semaphore */
DECL_HANDLER(create_semaphore)
{
    struct create_semaphore_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_semaphore", name, len );

    obj = create_semaphore( name, req->initial, req->max );
    if (obj)
    {
        reply.handle = alloc_handle( current->process, obj, SEMAPHORE_ALL_ACCESS, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* release a semaphore */
DECL_HANDLER(release_semaphore)
{
    struct release_semaphore_reply reply;
    if (release_semaphore( req->handle, req->count, &reply.prev_count )) CLEAR_ERROR();
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle to a named object (event, mutex, semaphore) */
DECL_HANDLER(open_named_obj)
{
    struct open_named_obj_reply reply;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "open_named_obj", name, len );

    switch(req->type)
    {
    case OPEN_EVENT:
        reply.handle = open_event( req->access, req->inherit, name );
        break;
    case OPEN_MUTEX:
        reply.handle = open_mutex( req->access, req->inherit, name );
        break;
    case OPEN_SEMAPHORE:
        reply.handle = open_semaphore( req->access, req->inherit, name );
        break;
    case OPEN_MAPPING:
        reply.handle = open_mapping( req->access, req->inherit, name );
        break;
    default:
        fatal_protocol_error( "open_named_obj: invalid type %d\n", req->type );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* create a file */
DECL_HANDLER(create_file)
{
    struct create_file_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_file", name, len );

    if ((obj = create_file( fd, name, req->access,
                            req->sharing, req->create, req->attrs )) != NULL)
    {
        reply.handle = alloc_handle( current->process, obj, req->access, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* get a Unix fd to read from a file */
DECL_HANDLER(get_read_fd)
{
    struct object *obj;
    int read_fd;

    if ((obj = get_handle_obj( current->process, req->handle, GENERIC_READ, NULL )))
    {
        read_fd = obj->ops->get_read_fd( obj );
        release_object( obj );
    }
    else read_fd = -1;
    send_reply( current, read_fd, 0 );
}

/* get a Unix fd to write to a file */
DECL_HANDLER(get_write_fd)
{
    struct object *obj;
    int write_fd;

    if ((obj = get_handle_obj( current->process, req->handle, GENERIC_WRITE, NULL )))
    {
        write_fd = obj->ops->get_write_fd( obj );
        release_object( obj );
    }
    else write_fd = -1;
    send_reply( current, write_fd, 0 );
}

/* set a file current position */
DECL_HANDLER(set_file_pointer)
{
    struct set_file_pointer_reply reply = { req->low, req->high };
    set_file_pointer( req->handle, &reply.low, &reply.high, req->whence );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* truncate (or extend) a file */
DECL_HANDLER(truncate_file)
{
    truncate_file( req->handle );
    send_reply( current, -1, 0 );
}

/* flush a file buffers */
DECL_HANDLER(flush_file)
{
    struct object *obj;

    if ((obj = get_handle_obj( current->process, req->handle, GENERIC_WRITE, NULL )))
    {
        obj->ops->flush( obj );
        release_object( obj );
    }
    send_reply( current, -1, 0 );
}

/* set a file access and modification times */
DECL_HANDLER(set_file_time)
{
    set_file_time( req->handle, req->access_time, req->write_time );
    send_reply( current, -1, 0 );
}

/* get a file information */
DECL_HANDLER(get_file_info)
{
    struct object *obj;
    struct get_file_info_reply reply;

    if ((obj = get_handle_obj( current->process, req->handle, 0, NULL )))
    {
        obj->ops->get_file_info( obj, &reply );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* lock a region of a file */
DECL_HANDLER(lock_file)
{
    struct file *file;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        file_lock( file, req->offset_high, req->offset_low,
                   req->count_high, req->count_low );
        release_object( file );
    }
    send_reply( current, -1, 0 );
}


/* unlock a region of a file */
DECL_HANDLER(unlock_file)
{
    struct file *file;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        file_unlock( file, req->offset_high, req->offset_low,
                     req->count_high, req->count_low );
        release_object( file );
    }
    send_reply( current, -1, 0 );
}


/* create an anonymous pipe */
DECL_HANDLER(create_pipe)
{
    struct create_pipe_reply reply = { -1, -1 };
    struct object *obj[2];
    if (create_pipe( obj ))
    {
        reply.handle_read = alloc_handle( current->process, obj[0],
                                          STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_READ,
                                          req->inherit );
        if (reply.handle_read != -1)
        {
            reply.handle_write = alloc_handle( current->process, obj[1],
                                               STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_WRITE,
                                               req->inherit );
            if (reply.handle_write == -1)
                close_handle( current->process, reply.handle_read );
        }
        release_object( obj[0] );
        release_object( obj[1] );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* allocate a console for the current process */
DECL_HANDLER(alloc_console)
{
    alloc_console( current->process );
    send_reply( current, -1, 0 );
}

/* free the console of the current process */
DECL_HANDLER(free_console)
{
    free_console( current->process );
    send_reply( current, -1, 0 );
}

/* open a handle to the process console */
DECL_HANDLER(open_console)
{
    struct object *obj;
    struct open_console_reply reply = { -1 };
    if ((obj = get_console( current->process, req->output )))
    {
        reply.handle = alloc_handle( current->process, obj, req->access, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set info about a console (output only) */
DECL_HANDLER(set_console_info)
{
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "set_console_info", name, len );
    set_console_info( req->handle, req, name );
    send_reply( current, -1, 0 );
}

/* get info about a console (output only) */
DECL_HANDLER(get_console_info)
{
    struct get_console_info_reply reply;
    const char *title;
    get_console_info( req->handle, &reply, &title );
    send_reply( current, -1, 2, &reply, sizeof(reply),
                title, title ? strlen(title)+1 : 0 );
}

/* set a console fd */
DECL_HANDLER(set_console_fd)
{
    set_console_fd( req->handle, fd, req->pid );
    send_reply( current, -1, 0 );
}

/* get a console mode (input or output) */
DECL_HANDLER(get_console_mode)
{
    struct get_console_mode_reply reply;
    get_console_mode( req->handle, &reply.mode );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set a console mode (input or output) */
DECL_HANDLER(set_console_mode)
{
    set_console_mode( req->handle, req->mode );
    send_reply( current, -1, 0 );
}

/* add input records to a console input queue */
DECL_HANDLER(write_console_input)
{
    struct write_console_input_reply reply;
    INPUT_RECORD *records = (INPUT_RECORD *)data;

    if (len != req->count * sizeof(INPUT_RECORD))
        fatal_protocol_error( "write_console_input: bad length %d for %d records\n",
                              len, req->count );
    reply.written = write_console_input( req->handle, req->count, records );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* fetch input records from a console input queue */
DECL_HANDLER(read_console_input)
{
    read_console_input( req->handle, req->count, req->flush );
}

/* create a change notification */
DECL_HANDLER(create_change_notification)
{
    struct object *obj;
    struct create_change_notification_reply reply = { -1 };

    if ((obj = create_change_notification( req->subtree, req->filter )))
    {
        reply.handle = alloc_handle( current->process, obj,
                                     STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE, 0 );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* create a file mapping */
DECL_HANDLER(create_mapping)
{
    struct object *obj;
    struct create_mapping_reply reply = { -1 };
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_mapping", name, len );

    if ((obj = create_mapping( req->size_high, req->size_low,
                               req->protect, req->handle, name )))
    {
        int access = FILE_MAP_ALL_ACCESS;
        if (!(req->protect & VPROT_WRITE)) access &= ~FILE_MAP_WRITE;
        reply.handle = alloc_handle( current->process, obj, access, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* get a mapping information */
DECL_HANDLER(get_mapping_info)
{
    struct get_mapping_info_reply reply;
    int map_fd = get_mapping_info( req->handle, &reply );
    send_reply( current, map_fd, 1, &reply, sizeof(reply) );
}

/* create a device */
DECL_HANDLER(create_device)
{
    struct object *obj;
    struct create_device_reply reply = { -1 };

    if ((obj = create_device( req->id )))
    {
        reply.handle = alloc_handle( current->process, obj,
                                     req->access, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* create a snapshot */
DECL_HANDLER(create_snapshot)
{
    struct object *obj;
    struct create_snapshot_reply reply = { -1 };

    if ((obj = create_snapshot( req->flags )))
    {
        reply.handle = alloc_handle( current->process, obj, 0, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* get the next process from a snapshot */
DECL_HANDLER(next_process)
{
    struct next_process_reply reply;
    snapshot_next_process( req->handle, req->reset, &reply );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}


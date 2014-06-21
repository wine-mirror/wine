/*
 * Server-side debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "file.h"
#include "process.h"
#include "thread.h"
#include "request.h"

enum debug_event_state { EVENT_QUEUED, EVENT_SENT, EVENT_CONTINUED };

/* debug event */
struct debug_event
{
    struct object          obj;       /* object header */
    struct list            entry;     /* entry in event queue */
    struct thread         *sender;    /* thread which sent this event */
    struct thread         *debugger;  /* debugger thread receiving the event */
    enum debug_event_state state;     /* event state */
    int                    status;    /* continuation status */
    debug_event_t          data;      /* event data */
    context_t              context;   /* register context */
};

/* debug context */
struct debug_ctx
{
    struct object        obj;         /* object header */
    struct list          event_queue; /* pending events queue */
    int                  kill_on_exit;/* kill debuggees on debugger exit ? */
};


static void debug_event_dump( struct object *obj, int verbose );
static int debug_event_signaled( struct object *obj, struct wait_queue_entry *entry );
static void debug_event_destroy( struct object *obj );

static const struct object_ops debug_event_ops =
{
    sizeof(struct debug_event),    /* size */
    debug_event_dump,              /* dump */
    no_get_type,                   /* get_type */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_event_signaled,          /* signaled */
    no_satisfied,                  /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    no_map_access,                 /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    no_lookup_name,                /* lookup_name */
    no_open_file,                  /* open_file */
    no_close_handle,               /* close_handle */
    debug_event_destroy            /* destroy */
};

static void debug_ctx_dump( struct object *obj, int verbose );
static int debug_ctx_signaled( struct object *obj, struct wait_queue_entry *entry );
static void debug_ctx_destroy( struct object *obj );

static const struct object_ops debug_ctx_ops =
{
    sizeof(struct debug_ctx),      /* size */
    debug_ctx_dump,                /* dump */
    no_get_type,                   /* get_type */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_ctx_signaled,            /* signaled */
    no_satisfied,                  /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    no_map_access,                 /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    no_lookup_name,                /* lookup_name */
    no_open_file,                  /* open_file */
    no_close_handle,               /* close_handle */
    debug_ctx_destroy              /* destroy */
};


/* routines to build an event according to its type */

static int fill_exception_event( struct debug_event *event, const void *arg )
{
    const debug_event_t *data = arg;
    event->data.exception = data->exception;
    event->data.exception.nb_params = min( event->data.exception.nb_params, EXCEPTION_MAXIMUM_PARAMETERS );
    return 1;
}

static int fill_create_thread_event( struct debug_event *event, const void *arg )
{
    struct process *debugger = event->debugger->process;
    struct thread *thread = event->sender;
    const client_ptr_t *entry = arg;
    obj_handle_t handle;

    /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
    if (!(handle = alloc_handle( debugger, thread, THREAD_ALL_ACCESS, 0 ))) return 0;
    event->data.create_thread.handle = handle;
    event->data.create_thread.teb    = thread->teb;
    if (entry) event->data.create_thread.start = *entry;
    return 1;
}

static int fill_create_process_event( struct debug_event *event, const void *arg )
{
    struct process *debugger = event->debugger->process;
    struct thread *thread = event->sender;
    struct process *process = thread->process;
    struct process_dll *exe_module = get_process_exe_module( process );
    const client_ptr_t *entry = arg;
    obj_handle_t handle;

    /* documented: PROCESS_VM_READ | PROCESS_VM_WRITE */
    if (!(handle = alloc_handle( debugger, process, PROCESS_ALL_ACCESS, 0 ))) return 0;
    event->data.create_process.process = handle;

    /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
    if (!(handle = alloc_handle( debugger, thread, THREAD_ALL_ACCESS, 0 )))
    {
        close_handle( debugger, event->data.create_process.process );
        return 0;
    }
    event->data.create_process.thread     = handle;
    event->data.create_process.file       = 0;
    event->data.create_process.teb        = thread->teb;
    event->data.create_process.base       = exe_module->base;
    event->data.create_process.start      = *entry;
    event->data.create_process.dbg_offset = exe_module->dbg_offset;
    event->data.create_process.dbg_size   = exe_module->dbg_size;
    event->data.create_process.name       = exe_module->name;
    event->data.create_process.unicode    = 1;

    if (exe_module->mapping)  /* the doc says write access too, but this doesn't seem a good idea */
        event->data.create_process.file = open_mapping_file( debugger, exe_module->mapping, GENERIC_READ,
                                                             FILE_SHARE_READ | FILE_SHARE_WRITE );
    return 1;
}

static int fill_exit_thread_event( struct debug_event *event, const void *arg )
{
    const struct thread *thread = arg;
    event->data.exit.exit_code = thread->exit_code;
    return 1;
}

static int fill_exit_process_event( struct debug_event *event, const void *arg )
{
    const struct process *process = arg;
    event->data.exit.exit_code = process->exit_code;
    return 1;
}

static int fill_load_dll_event( struct debug_event *event, const void *arg )
{
    struct process *debugger = event->debugger->process;
    const struct process_dll *dll = arg;

    event->data.load_dll.handle     = 0;
    event->data.load_dll.base       = dll->base;
    event->data.load_dll.dbg_offset = dll->dbg_offset;
    event->data.load_dll.dbg_size   = dll->dbg_size;
    event->data.load_dll.name       = dll->name;
    event->data.load_dll.unicode    = 1;
    if (dll->mapping)
        event->data.load_dll.handle = open_mapping_file( debugger, dll->mapping, GENERIC_READ,
                                                         FILE_SHARE_READ | FILE_SHARE_WRITE );
    return 1;
}

static int fill_unload_dll_event( struct debug_event *event, const void *arg )
{
    const mod_handle_t *base = arg;
    event->data.unload_dll.base = *base;
    return 1;
}

typedef int (*fill_event_func)( struct debug_event *event, const void *arg );

#define NB_DEBUG_EVENTS OUTPUT_DEBUG_STRING_EVENT  /* RIP_EVENT not supported */

static const fill_event_func fill_debug_event[NB_DEBUG_EVENTS] =
{
    fill_exception_event,            /* EXCEPTION_DEBUG_EVENT */
    fill_create_thread_event,        /* CREATE_THREAD_DEBUG_EVENT */
    fill_create_process_event,       /* CREATE_PROCESS_DEBUG_EVENT */
    fill_exit_thread_event,          /* EXIT_THREAD_DEBUG_EVENT */
    fill_exit_process_event,         /* EXIT_PROCESS_DEBUG_EVENT */
    fill_load_dll_event,             /* LOAD_DLL_DEBUG_EVENT */
    fill_unload_dll_event            /* UNLOAD_DLL_DEBUG_EVENT */
};


/* unlink the first event from the queue */
static void unlink_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    list_remove( &event->entry );
    if (event->sender->debug_event == event) event->sender->debug_event = NULL;
    release_object( event );
}

/* link an event at the end of the queue */
static void link_event( struct debug_event *event )
{
    struct debug_ctx *debug_ctx = event->debugger->debug_ctx;

    assert( debug_ctx );
    grab_object( event );
    list_add_tail( &debug_ctx->event_queue, &event->entry );
    if (!event->sender->debug_event)
    {
        /* grab reference since debugger could be killed while trying to wake up */
        grab_object( debug_ctx );
        wake_up( &debug_ctx->obj, 0 );
        release_object( debug_ctx );
    }
}

/* find the next event that we can send to the debugger */
static struct debug_event *find_event_to_send( struct debug_ctx *debug_ctx )
{
    struct debug_event *event;

    LIST_FOR_EACH_ENTRY( event, &debug_ctx->event_queue, struct debug_event, entry )
    {
        if (event->state == EVENT_SENT) continue;  /* already sent */
        if (event->sender->debug_event) continue;  /* thread busy with another one */
        return event;
    }
    return NULL;
}

static void debug_event_dump( struct object *obj, int verbose )
{
    struct debug_event *debug_event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    fprintf( stderr, "Debug event sender=%p code=%d state=%d\n",
             debug_event->sender, debug_event->data.code, debug_event->state );
}

static int debug_event_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct debug_event *debug_event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    return debug_event->state == EVENT_CONTINUED;
}

static void debug_event_destroy( struct object *obj )
{
    struct debug_event *event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );

    /* If the event has been sent already, the handles are now under the */
    /* responsibility of the debugger process, so we don't touch them    */
    if (event->state == EVENT_QUEUED)
    {
        struct process *debugger = event->debugger->process;
        switch(event->data.code)
        {
        case CREATE_THREAD_DEBUG_EVENT:
            close_handle( debugger, event->data.create_thread.handle );
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            if (event->data.create_process.file)
                close_handle( debugger, event->data.create_process.file );
            close_handle( debugger, event->data.create_process.thread );
            close_handle( debugger, event->data.create_process.process );
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (event->data.load_dll.handle)
                close_handle( debugger, event->data.load_dll.handle );
            break;
        }
    }
    if (event->sender->context == &event->context)
    {
        event->sender->context = NULL;
        stop_thread_if_suspended( event->sender );
    }
    release_object( event->sender );
    release_object( event->debugger );
}

static void debug_ctx_dump( struct object *obj, int verbose )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    fprintf( stderr, "Debug context head=%p tail=%p\n",
             debug_ctx->event_queue.next, debug_ctx->event_queue.prev );
}

static int debug_ctx_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    return find_event_to_send( debug_ctx ) != NULL;
}

static void debug_ctx_destroy( struct object *obj )
{
    struct list *ptr;
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );

    /* free all pending events */
    while ((ptr = list_head( &debug_ctx->event_queue )))
        unlink_event( debug_ctx, LIST_ENTRY( ptr, struct debug_event, entry ));
}

/* continue a debug event */
static int continue_debug_event( struct process *process, struct thread *thread, int status )
{
    struct debug_ctx *debug_ctx = current->debug_ctx;

    if (debug_ctx && process->debugger == current && thread->process == process)
    {
        struct debug_event *event;

        /* find the event in the queue */
        LIST_FOR_EACH_ENTRY( event, &debug_ctx->event_queue, struct debug_event, entry )
        {
            if (event->state != EVENT_SENT) continue;
            if (event->sender == thread)
            {
                assert( event->sender->debug_event == event );

                event->status = status;
                event->state  = EVENT_CONTINUED;
                wake_up( &event->obj, 0 );
                unlink_event( debug_ctx, event );
                resume_process( process );
                return 1;
            }
        }
    }
    /* not debugging this process, or no such event */
    set_error( STATUS_ACCESS_DENIED );  /* FIXME */
    return 0;
}

/* alloc a debug event for a debugger */
static struct debug_event *alloc_debug_event( struct thread *thread, int code, const void *arg )
{
    struct thread *debugger = thread->process->debugger;
    struct debug_event *event;

    assert( code > 0 && code <= NB_DEBUG_EVENTS );
    /* cannot queue a debug event for myself */
    assert( debugger->process != thread->process );

    /* build the event */
    if (!(event = alloc_object( &debug_event_ops ))) return NULL;
    event->state     = EVENT_QUEUED;
    event->sender    = (struct thread *)grab_object( thread );
    event->debugger  = (struct thread *)grab_object( debugger );
    memset( &event->data, 0, sizeof(event->data) );

    if (!fill_debug_event[code-1]( event, arg ))
    {
        event->data.code = -1;  /* make sure we don't attempt to close handles */
        release_object( event );
        return NULL;
    }
    event->data.code = code;
    return event;
}

/* generate a debug event from inside the server and queue it */
void generate_debug_event( struct thread *thread, int code, const void *arg )
{
    if (thread->process->debugger)
    {
        struct debug_event *event = alloc_debug_event( thread, code, arg );
        if (event)
        {
            link_event( event );
            suspend_process( thread->process );
            release_object( event );
        }
        clear_error();  /* ignore errors */
    }
}

/* attach a process to a debugger thread and suspend it */
static int debugger_attach( struct process *process, struct thread *debugger )
{
    if (process->debugger) goto error;  /* already being debugged */
    if (debugger->process == process) goto error;
    if (!is_process_init_done( process )) goto error;  /* still starting up */
    if (list_empty( &process->thread_list )) goto error;  /* no thread running in the process */

    /* don't let a debugger debug its console... won't work */
    if (debugger->process->console)
    {
        struct thread *renderer = console_get_renderer(debugger->process->console);
        if (renderer && renderer->process == process)
            goto error;
    }

    suspend_process( process );
    if (!set_process_debugger( process, debugger ))
    {
        resume_process( process );
        return 0;
    }
    if (!set_process_debug_flag( process, 1 ))
    {
        process->debugger = NULL;
        resume_process( process );
        return 0;
    }
    return 1;

 error:
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}


/* detach a process from a debugger thread (and resume it ?) */
int debugger_detach( struct process *process, struct thread *debugger )
{
    struct debug_event *event, *next;
    struct debug_ctx *debug_ctx;

    if (!process->debugger || process->debugger != debugger)
        goto error;  /* not currently debugged, or debugged by another debugger */
    if (!debugger->debug_ctx ) goto error; /* should be a debugger */
    /* init should be done, otherwise wouldn't be attached */
    assert(is_process_init_done(process));

    suspend_process( process );
    /* send continue indication for all events */
    debug_ctx = debugger->debug_ctx;

    /* free all events from this process */
    LIST_FOR_EACH_ENTRY_SAFE( event, next, &debug_ctx->event_queue, struct debug_event, entry )
    {
        if (event->sender->process != process) continue;

        assert( event->state != EVENT_CONTINUED );
        event->status = DBG_CONTINUE;
        event->state  = EVENT_CONTINUED;
        wake_up( &event->obj, 0 );
        unlink_event( debug_ctx, event );
        /* from queued debug event */
        resume_process( process );
    }

    /* remove relationships between process and its debugger */
    process->debugger = NULL;
    process->debug_children = 0;
    if (!set_process_debug_flag( process, 0 )) clear_error();  /* ignore error */

    /* from this function */
    resume_process( process );
    return 0;

 error:
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}

/* generate all startup events of a given process */
void generate_startup_debug_events( struct process *process, client_ptr_t entry )
{
    struct list *ptr;
    struct thread *thread, *first_thread = get_process_first_thread( process );

    /* generate creation events */
    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread == first_thread)
            generate_debug_event( thread, CREATE_PROCESS_DEBUG_EVENT, &entry );
        else
            generate_debug_event( thread, CREATE_THREAD_DEBUG_EVENT, NULL );
    }

    /* generate dll events (in loading order, i.e. reverse list order) */
    ptr = list_tail( &process->dlls );
    while (ptr != list_head( &process->dlls ))
    {
        struct process_dll *dll = LIST_ENTRY( ptr, struct process_dll, entry );
        generate_debug_event( first_thread, LOAD_DLL_DEBUG_EVENT, dll );
        ptr = list_prev( &process->dlls, ptr );
    }
}

/* set the debugger of a given process */
int set_process_debugger( struct process *process, struct thread *debugger )
{
    struct debug_ctx *debug_ctx;

    assert( !process->debugger );

    if (!debugger->debug_ctx)  /* need to allocate a context */
    {
        if (!(debug_ctx = alloc_object( &debug_ctx_ops ))) return 0;
        debug_ctx->kill_on_exit = 1;
        list_init( &debug_ctx->event_queue );
        debugger->debug_ctx = debug_ctx;
    }
    process->debugger = debugger;
    return 1;
}

/* a thread is exiting */
void debug_exit_thread( struct thread *thread )
{
    if (thread->debug_ctx)  /* this thread is a debugger */
    {
        if (thread->debug_ctx->kill_on_exit)
        {
            /* kill all debugged processes */
            kill_debugged_processes( thread, STATUS_DEBUGGER_INACTIVE );
        }
        else
        {
            detach_debugged_processes( thread );
        }
        release_object( thread->debug_ctx );
        thread->debug_ctx = NULL;
    }
}

/* Wait for a debug event */
DECL_HANDLER(wait_debug_event)
{
    struct debug_ctx *debug_ctx = current->debug_ctx;
    struct debug_event *event;

    if (!debug_ctx)  /* current thread is not a debugger */
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    reply->wait = 0;
    if ((event = find_event_to_send( debug_ctx )))
    {
        data_size_t size = get_reply_max_size();
        event->state = EVENT_SENT;
        event->sender->debug_event = event;
        reply->pid = get_process_id( event->sender->process );
        reply->tid = get_thread_id( event->sender );
        if (size > sizeof(debug_event_t)) size = sizeof(debug_event_t);
        set_reply_data( &event->data, size );
    }
    else  /* no event ready */
    {
        reply->pid  = 0;
        reply->tid  = 0;
        if (req->get_handle)
            reply->wait = alloc_handle( current->process, debug_ctx, SYNCHRONIZE, 0 );
    }
}

/* Continue a debug event */
DECL_HANDLER(continue_debug_event)
{
    struct process *process = get_process_from_id( req->pid );
    if (process)
    {
        struct thread *thread = get_thread_from_id( req->tid );
        if (thread)
        {
            continue_debug_event( process, thread, req->status );
            release_object( thread );
        }
        release_object( process );
    }
}

/* Start debugging an existing process */
DECL_HANDLER(debug_process)
{
    struct process *process = get_process_from_id( req->pid );
    if (!process) return;

    if (!req->attach)
    {
        debugger_detach( process, current );
    }
    else if (debugger_attach( process, current ))
    {
        generate_startup_debug_events( process, 0 );
        break_process( process );
        resume_process( process );
    }
    release_object( process );
}

/* queue an exception event */
DECL_HANDLER(queue_exception_event)
{
    reply->handle = 0;
    if (current->process->debugger)
    {
        debug_event_t data;
        struct debug_event *event;
        struct thread *thread = current;

        if ((req->len % sizeof(client_ptr_t)) != 0 ||
            req->len > get_req_data_size() ||
            req->len > EXCEPTION_MAXIMUM_PARAMETERS * sizeof(client_ptr_t))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
        memset( &data, 0, sizeof(data) );
        data.exception.first     = req->first;
        data.exception.exc_code  = req->code;
        data.exception.flags     = req->flags;
        data.exception.record    = req->record;
        data.exception.address   = req->address;
        data.exception.nb_params = req->len / sizeof(client_ptr_t);
        memcpy( data.exception.params, get_req_data(), req->len );

        if ((event = alloc_debug_event( thread, EXCEPTION_DEBUG_EVENT, &data )))
        {
            const context_t *context = (const context_t *)((const char *)get_req_data() + req->len);
            data_size_t size = get_req_data_size() - req->len;

            memset( &event->context, 0, sizeof(event->context) );
            memcpy( &event->context, context, min( sizeof(event->context), size ) );
            thread->context = &event->context;

            if ((reply->handle = alloc_handle( thread->process, event, SYNCHRONIZE, 0 )))
            {
                link_event( event );
                suspend_process( thread->process );
            }
            release_object( event );
        }
    }
}

/* retrieve the status of an exception event */
DECL_HANDLER(get_exception_status)
{
    struct debug_event *event;

    if ((event = (struct debug_event *)get_handle_obj( current->process, req->handle,
                                                       0, &debug_event_ops )))
    {
        close_handle( current->process, req->handle );
        if (event->state == EVENT_CONTINUED)
        {
            if (current->context == &event->context)
            {
                data_size_t size = min( sizeof(context_t), get_reply_max_size() );
                set_reply_data( &event->context, size );
                current->context = NULL;
                stop_thread_if_suspended( current );
            }
            set_error( event->status );
        }
        else set_error( STATUS_PENDING );
        release_object( event );
    }
}

/* simulate a breakpoint in a process */
DECL_HANDLER(debug_break)
{
    struct process *process;

    reply->self = 0;
    if ((process = get_process_from_handle( req->handle, PROCESS_SET_INFORMATION /*FIXME*/ )))
    {
        if (process != current->process) break_process( process );
        else reply->self = 1;
        release_object( process );
    }
}

/* set debugger kill on exit flag */
DECL_HANDLER(set_debugger_kill_on_exit)
{
    if (!current->debug_ctx)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    current->debug_ctx->kill_on_exit = req->kill_on_exit;
}

/*
 * Server-side debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "winbase.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"

enum debug_event_state { EVENT_QUEUED, EVENT_SENT, EVENT_CONTINUED };

/* debug event */
struct debug_event
{
    struct object          obj;       /* object header */
    struct debug_event    *next;      /* event queue */
    struct debug_event    *prev;
    struct thread         *sender;    /* thread which sent this event */
    struct thread         *debugger;  /* debugger thread receiving the event */
    enum debug_event_state state;     /* event state */
    int                    status;    /* continuation status */
    debug_event_t          data;      /* event data */
};

/* debug context */
struct debug_ctx
{
    struct object        obj;         /* object header */
    struct debug_event  *event_head;  /* head of pending events queue */
    struct debug_event  *event_tail;  /* tail of pending events queue */
};


static void debug_event_dump( struct object *obj, int verbose );
static int debug_event_signaled( struct object *obj, struct thread *thread );
static void debug_event_destroy( struct object *obj );

static const struct object_ops debug_event_ops =
{
    sizeof(struct debug_event),    /* size */
    debug_event_dump,              /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_event_signaled,          /* signaled */
    no_satisfied,                  /* satisfied */
    NULL,                          /* get_poll_events */
    NULL,                          /* poll_event */
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    debug_event_destroy            /* destroy */
};

static void debug_ctx_dump( struct object *obj, int verbose );
static int debug_ctx_signaled( struct object *obj, struct thread *thread );
static void debug_ctx_destroy( struct object *obj );

static const struct object_ops debug_ctx_ops =
{
    sizeof(struct debug_ctx),      /* size */
    debug_ctx_dump,                /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_ctx_signaled,            /* signaled */
    no_satisfied,                  /* satisfied */
    NULL,                          /* get_poll_events */
    NULL,                          /* poll_event */
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    debug_ctx_destroy              /* destroy */
};


/* routines to build an event according to its type */

static int fill_exception_event( struct debug_event *event, void *arg )
{
    memcpy( &event->data.info.exception, arg, sizeof(event->data.info.exception) );
    return 1;
}

static int fill_create_thread_event( struct debug_event *event, void *arg )
{
    struct process *debugger = event->debugger->process;
    struct thread *thread = event->sender;
    int handle;
    
    /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
    if ((handle = alloc_handle( debugger, thread, THREAD_ALL_ACCESS, FALSE )) == -1)
        return 0;
    event->data.info.create_thread.handle = handle;
    event->data.info.create_thread.teb    = thread->teb;
    event->data.info.create_thread.start  = arg;
    return 1;
}

static int fill_create_process_event( struct debug_event *event, void *arg )
{
    struct process *debugger = event->debugger->process;
    struct thread *thread = event->sender;
    struct process *process = thread->process;
    int handle;

    /* documented: PROCESS_VM_READ | PROCESS_VM_WRITE */
    if ((handle = alloc_handle( debugger, process, PROCESS_ALL_ACCESS, FALSE )) == -1)
        return 0;
    event->data.info.create_process.process = handle;

    /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
    if ((handle = alloc_handle( debugger, thread, THREAD_ALL_ACCESS, FALSE )) == -1)
    {
        close_handle( debugger, event->data.info.create_process.process );
        return 0;
    }
    event->data.info.create_process.thread = handle;

    handle = -1;
    if (process->exe.file &&
        /* the doc says write access too, but this doesn't seem a good idea */
        ((handle = alloc_handle( debugger, process->exe.file, GENERIC_READ, FALSE )) == -1))
    {
        close_handle( debugger, event->data.info.create_process.process );
        close_handle( debugger, event->data.info.create_process.thread );
        return 0;
    }
    event->data.info.create_process.file       = handle;
    event->data.info.create_process.teb        = thread->teb;
    event->data.info.create_process.base       = process->exe.base;
    event->data.info.create_process.start      = arg;
    event->data.info.create_process.dbg_offset = process->exe.dbg_offset;
    event->data.info.create_process.dbg_size   = process->exe.dbg_size;
    event->data.info.create_process.name       = 0;
    event->data.info.create_process.unicode    = 0;
    return 1;
}

static int fill_exit_thread_event( struct debug_event *event, void *arg )
{
    struct thread *thread = arg;
    event->data.info.exit.exit_code = thread->exit_code;
    return 1;
}

static int fill_exit_process_event( struct debug_event *event, void *arg )
{
    struct process *process = arg;
    event->data.info.exit.exit_code = process->exit_code;
    return 1;
}

static int fill_load_dll_event( struct debug_event *event, void *arg )
{
    struct process *debugger = event->debugger->process;
    struct process_dll *dll = arg;
    int handle = -1;

    if (dll->file && (handle = alloc_handle( debugger, dll->file, GENERIC_READ, FALSE )) == -1)
        return 0;
    event->data.info.load_dll.handle     = handle;
    event->data.info.load_dll.base       = dll->base;
    event->data.info.load_dll.dbg_offset = dll->dbg_offset;
    event->data.info.load_dll.dbg_size   = dll->dbg_size;
    event->data.info.load_dll.name       = dll->name;
    event->data.info.load_dll.unicode    = 0;
    return 1;
}

static int fill_unload_dll_event( struct debug_event *event, void *arg )
{
    event->data.info.unload_dll.base = arg;
    return 1;
}

static int fill_output_debug_string_event( struct debug_event *event, void *arg )
{
    struct output_debug_string_request *req = arg;
    event->data.info.output_string.string  = req->string;
    event->data.info.output_string.unicode = req->unicode;
    event->data.info.output_string.length  = req->length;
    return 1;
}

typedef int (*fill_event_func)( struct debug_event *event, void *arg );

#define NB_DEBUG_EVENTS OUTPUT_DEBUG_STRING_EVENT  /* RIP_EVENT not supported */

static const fill_event_func fill_debug_event[NB_DEBUG_EVENTS] =
{
    fill_exception_event,            /* EXCEPTION_DEBUG_EVENT */
    fill_create_thread_event,        /* CREATE_THREAD_DEBUG_EVENT */
    fill_create_process_event,       /* CREATE_PROCESS_DEBUG_EVENT */
    fill_exit_thread_event,          /* EXIT_THREAD_DEBUG_EVENT */
    fill_exit_process_event,         /* EXIT_PROCESS_DEBUG_EVENT */
    fill_load_dll_event,             /* LOAD_DLL_DEBUG_EVENT */
    fill_unload_dll_event,           /* UNLOAD_DLL_DEBUG_EVENT */
    fill_output_debug_string_event   /* OUTPUT_DEBUG_STRING_EVENT */
};


/* unlink the first event from the queue */
static void unlink_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    if (event->prev) event->prev->next = event->next;
    else debug_ctx->event_head = event->next;
    if (event->next) event->next->prev = event->prev;
    else debug_ctx->event_tail = event->prev;
    event->next = event->prev = NULL;
    if (event->sender->debug_event == event) event->sender->debug_event = NULL;
    release_object( event );
}

/* link an event at the end of the queue */
static void link_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    grab_object( event );
    event->next = NULL;
    event->prev = debug_ctx->event_tail;
    debug_ctx->event_tail = event;
    if (event->prev) event->prev->next = event;
    else debug_ctx->event_head = event;
    if (!event->sender->debug_event) wake_up( &debug_ctx->obj, 0 );
}

/* find the next event that we can send to the debugger */
static struct debug_event *find_event_to_send( struct debug_ctx *debug_ctx )
{
    struct debug_event *event;
    for (event = debug_ctx->event_head; event; event = event->next)
    {
        if (event->state == EVENT_SENT) continue;  /* already sent */
        if (event->sender->debug_event) continue;  /* thread busy with another one */
        break;
    }
    return event;
}

/* build a reply for the wait_debug_event request */
static void build_wait_debug_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct wait_debug_event_request *req = get_req_ptr( thread );

    if (obj)
    {
        struct debug_ctx *debug_ctx = (struct debug_ctx *)obj; 
        struct debug_event *event = find_event_to_send( debug_ctx );

        /* the object that woke us has to be our debug context */
        assert( obj->ops == &debug_ctx_ops );
        assert( event );

        event->state = EVENT_SENT;
        event->sender->debug_event = event;
        req->event.code = event->data.code;
        req->pid  = event->sender->process;
        req->tid  = event->sender;
        memcpy( &req->event, &event->data, sizeof(req->event) );
    }
    else  /* timeout or error */
    {
        req->event.code = 0;
        req->pid  = 0;
        req->tid  = 0;
    }
}

/* build a reply for the send_event request */
static void build_exception_event_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct exception_event_request *req = get_req_ptr( thread );
    struct debug_event *event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    req->status = event->status;
    thread->context = NULL;
}

static void debug_event_dump( struct object *obj, int verbose )
{
    struct debug_event *debug_event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    fprintf( stderr, "Debug event sender=%p code=%d state=%d\n",
             debug_event->sender, debug_event->data.code, debug_event->state );
}

static int debug_event_signaled( struct object *obj, struct thread *thread )
{
    struct debug_event *debug_event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    return debug_event->state == EVENT_CONTINUED;
}

static void debug_event_destroy( struct object *obj )
{
    struct debug_event *event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );

    /* cannot still be in the queue */
    assert( !event->next );
    assert( !event->prev );

    /* If the event has been sent already, the handles are now under the */
    /* responsibility of the debugger process, so we don't touch them    */
    if (event->state == EVENT_QUEUED)
    {
        struct process *debugger = event->debugger->process;
        switch(event->data.code)
        {
        case CREATE_THREAD_DEBUG_EVENT:
            close_handle( debugger, event->data.info.create_thread.handle );
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            if (event->data.info.create_process.file != -1)
                close_handle( debugger, event->data.info.create_process.file );
            close_handle( debugger, event->data.info.create_process.thread );
            close_handle( debugger, event->data.info.create_process.process );
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (event->data.info.load_dll.handle != -1)
                close_handle( debugger, event->data.info.load_dll.handle );
            break;
        }
    }
    release_object( event->sender );
    release_object( event->debugger );
}

static void debug_ctx_dump( struct object *obj, int verbose )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    fprintf( stderr, "Debug context head=%p tail=%p\n",
             debug_ctx->event_head, debug_ctx->event_tail );
}

static int debug_ctx_signaled( struct object *obj, struct thread *thread )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    return find_event_to_send( debug_ctx ) != NULL;
}

static void debug_ctx_destroy( struct object *obj )
{
    struct debug_event *event;
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );

    /* free all pending events */
    while ((event = debug_ctx->event_head) != NULL) unlink_event( debug_ctx, event );
}

/* wait for a debug event (or send a reply at once if one is pending) */
static int wait_for_debug_event( int timeout )
{
    struct debug_ctx *debug_ctx = current->debug_ctx;
    struct object *obj = &debug_ctx->obj;
    int flags = 0;

    if (!debug_ctx)  /* current thread is not a debugger */
    {
        set_error( STATUS_INVALID_HANDLE );
        return 0;
    }
    if (timeout != -1) flags = SELECT_TIMEOUT;
    return sleep_on( 1, &obj, flags, timeout, build_wait_debug_reply );
}

/* continue a debug event */
static int continue_debug_event( struct process *process, struct thread *thread, int status )
{
    struct debug_event *event;
    struct debug_ctx *debug_ctx = current->debug_ctx;

    if (!debug_ctx || process->debugger != current || thread->process != process) goto error;

    /* find the event in the queue */
    for (event = debug_ctx->event_head; event; event = event->next)
    {
        if (event->state != EVENT_SENT) continue;
        if (event->sender == thread) break;
    }
    if (!event) goto error;

    assert( event->sender->debug_event == event );

    event->status = status;
    event->state  = EVENT_CONTINUED;
    wake_up( &event->obj, 0 );

    unlink_event( debug_ctx, event );
    resume_process( process );
    return 1;
 error:
    /* not debugging this process, or no such event */
    set_error( STATUS_ACCESS_DENIED );  /* FIXME */
    return 0;
}

/* queue a debug event for a debugger */
static struct debug_event *queue_debug_event( struct thread *thread, int code, void *arg )
{
    struct thread *debugger = thread->process->debugger;
    struct debug_ctx *debug_ctx = debugger->debug_ctx;
    struct debug_event *event;

    assert( code > 0 && code <= NB_DEBUG_EVENTS );
    assert( debug_ctx );
    /* cannot queue a debug event for myself */
    assert( debugger->process != thread->process );

    /* build the event */
    if (!(event = alloc_object( &debug_event_ops, -1 ))) return NULL;
    event->next      = NULL;
    event->prev      = NULL;
    event->state     = EVENT_QUEUED;
    event->sender    = (struct thread *)grab_object( thread );
    event->debugger  = (struct thread *)grab_object( debugger );
    event->data.code = code;

    if (!fill_debug_event[code-1]( event, arg ))
    {
        event->data.code = -1;  /* make sure we don't attempt to close handles */
        release_object( event );
        return NULL;
    }

    link_event( debug_ctx, event );
    suspend_process( thread->process );
    return event;
}

/* generate a debug event from inside the server and queue it */
void generate_debug_event( struct thread *thread, int code, void *arg )
{
    if (thread->process->debugger)
    {
        struct debug_event *event = queue_debug_event( thread, code, arg );
        if (event) release_object( event );
    }
}

/* attach a process to a debugger thread and suspend it */
static int debugger_attach( struct process *process, struct thread *debugger )
{
    struct thread *thread;

    if (process->debugger) goto error;  /* already being debugged */
    if (process->init_event) goto error;  /* still starting up */

    /* make sure we don't create a debugging loop */
    for (thread = debugger; thread; thread = thread->process->debugger)
        if (thread->process == process) goto error;

    suspend_process( process );

    /* we must have been able to attach all threads */
    for (thread = process->thread_list; thread; thread = thread->proc_next)
        if (!thread->attached)
        {
            fprintf( stderr, "%p not attached\n", thread );
            resume_process( process );
            goto error;
        }

    if (set_process_debugger( process, debugger )) return 1;
    resume_process( process );
    return 0;

 error:
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}

/* generate all startup events of a given process */
void generate_startup_debug_events( struct process *process, void *entry )
{
    struct process_dll *dll;
    struct thread *thread = process->thread_list;

    /* generate creation events */
    generate_debug_event( thread, CREATE_PROCESS_DEBUG_EVENT, entry );
    while ((thread = thread->proc_next))
        generate_debug_event( thread, CREATE_THREAD_DEBUG_EVENT, NULL );

    /* generate dll events (in loading order, i.e. reverse list order) */
    for (dll = &process->exe; dll->next; dll = dll->next);
    while (dll != &process->exe)
    {
        generate_debug_event( process->thread_list, LOAD_DLL_DEBUG_EVENT, dll );
        dll = dll->prev;
    }
}

/* set the debugger of a given process */
int set_process_debugger( struct process *process, struct thread *debugger )
{
    struct debug_ctx *debug_ctx;

    assert( !process->debugger );

    if (!debugger->debug_ctx)  /* need to allocate a context */
    {
        if (!(debug_ctx = alloc_object( &debug_ctx_ops, -1 ))) return 0;
        debug_ctx->event_head = NULL;
        debug_ctx->event_tail = NULL;
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
        /* kill all debugged processes */
        kill_debugged_processes( thread, thread->exit_code );
        release_object( thread->debug_ctx );
        thread->debug_ctx = NULL;
    }
}

/* Wait for a debug event */
DECL_HANDLER(wait_debug_event)
{
    if (!wait_for_debug_event( req->timeout ))
    {
        req->event.code = 0;
        req->pid = NULL;
        req->tid = NULL;
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
    struct debug_event_exception data;
    struct debug_event *event;
    struct process *process = get_process_from_id( req->pid );
    if (!process) return;

    if (debugger_attach( process, current ))
    {
        generate_startup_debug_events( process, NULL );
        resume_process( process );

        data.record.ExceptionCode    = EXCEPTION_BREAKPOINT;
        data.record.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        data.record.ExceptionRecord  = NULL;
        data.record.ExceptionAddress = get_thread_ip( process->thread_list );
        data.record.NumberParameters = 0;
        data.first = 1;
        if ((event = queue_debug_event( process->thread_list, EXCEPTION_DEBUG_EVENT, &data )))
            release_object( event );
    }
    release_object( process );
}

/* send an exception event */
DECL_HANDLER(exception_event)
{
    if (current->process->debugger)
    {
        struct debug_event_exception data;
        struct debug_event *event;

        data.record = req->record;
        data.first  = req->first;
        if ((event = queue_debug_event( current, EXCEPTION_DEBUG_EVENT, &data )))
        {
            struct object *obj = &event->obj;
            current->context = &req->context;
            sleep_on( 1, &obj, 0, -1, build_exception_event_reply );
            release_object( event );
        }
    }
}

/* send an output string to the debugger */
DECL_HANDLER(output_debug_string)
{
    if (current->process->debugger)
    {
        struct debug_event *event = queue_debug_event( current, OUTPUT_DEBUG_STRING_EVENT, req );
        if (event) release_object( event );
    }
}

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
    struct debug_event  *to_send;     /* next event on the queue to send to debugger */
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


/* initialise the fields that do not need to be filled by the client */
static int fill_debug_event( struct debug_event *event, void *arg )
{
    struct process *debugger = event->debugger->process;
    struct process *process;
    struct thread *thread;
    struct process_dll *dll;
    int handle;

    /* some events need special handling */
    switch(event->data.code)
    {
    case CREATE_THREAD_DEBUG_EVENT:
        thread = arg;
        /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
        if ((handle = alloc_handle( debugger, thread, THREAD_ALL_ACCESS, FALSE )) == -1)
            return 0;
        event->data.info.create_thread.handle = handle;
        event->data.info.create_thread.teb    = thread->teb;
        event->data.info.create_thread.start  = thread->entry;
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        process = arg;
        thread = process->thread_list;
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
        event->data.info.create_process.start      = thread->entry;
        event->data.info.create_process.dbg_offset = process->exe.dbg_offset;
        event->data.info.create_process.dbg_size   = process->exe.dbg_size;
        event->data.info.create_process.name       = 0;
        event->data.info.create_process.unicode    = 0;
        break;
    case LOAD_DLL_DEBUG_EVENT:
        dll = arg;
        handle = -1;
        if (dll->file &&
            (handle = alloc_handle( debugger, dll->file, GENERIC_READ, FALSE )) == -1)
            return 0;
        event->data.info.load_dll.handle     = handle;
        event->data.info.load_dll.base       = dll->base;
        event->data.info.load_dll.dbg_offset = dll->dbg_offset;
        event->data.info.load_dll.dbg_size   = dll->dbg_size;
        event->data.info.load_dll.name       = dll->name;
        event->data.info.load_dll.unicode    = 0;
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        process = arg;
        event->data.info.exit.exit_code = process->exit_code;
        break;
    case EXIT_THREAD_DEBUG_EVENT:
        thread = arg;
        event->data.info.exit.exit_code = thread->exit_code;
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        event->data.info.unload_dll.base = arg;
        break;
    case EXCEPTION_DEBUG_EVENT:
    case OUTPUT_DEBUG_STRING_EVENT:
    case RIP_EVENT:
        break;
    }
    return 1;
}

/* unlink the first event from the queue */
static void unlink_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    if (event->prev) event->prev->next = event->next;
    else debug_ctx->event_head = event->next;
    if (event->next) event->next->prev = event->prev;
    else debug_ctx->event_tail = event->prev;
    if (debug_ctx->to_send == event) debug_ctx->to_send = event->next;
    event->next = event->prev = NULL;
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
    if (!debug_ctx->to_send)
    {
        debug_ctx->to_send = event;
        wake_up( &debug_ctx->obj, 0 );
    }
}

/* build a reply for the wait_debug_event request */
static void build_wait_debug_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct wait_debug_event_request *req = get_req_ptr( thread );

    if (obj)
    {
        struct debug_ctx *debug_ctx = (struct debug_ctx *)obj; 
        struct debug_event *event = debug_ctx->to_send;

        /* the object that woke us has to be our debug context */
        assert( obj->ops == &debug_ctx_ops );
        assert( event );

        event->state = EVENT_SENT;
        debug_ctx->to_send = event->next;
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
static void build_send_event_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct send_debug_event_request *req = get_req_ptr( thread );
    struct debug_event *event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );

    req->status = event->status;
    /* copy the context into the reply */
    if (event->data.code == EXCEPTION_DEBUG_EVENT)
        memcpy( &req->event.info.exception.context,
                &event->data.info.exception.context,
                sizeof(req->event.info.exception.context) );
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
    fprintf( stderr, "Debug context head=%p tail=%p to_send=%p\n",
             debug_ctx->event_head, debug_ctx->event_tail, debug_ctx->to_send );
}

static int debug_ctx_signaled( struct object *obj, struct thread *thread )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    return debug_ctx->to_send != NULL;
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
        if (event == debug_ctx->to_send) goto error;
        if (event->sender == thread) break;
    }
    if (!event) goto error;

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
static struct debug_event *queue_debug_event( struct thread *thread, int code, void *arg,
                                              debug_event_t *data )
{
    struct thread *debugger = thread->process->debugger;
    struct debug_ctx *debug_ctx = debugger->debug_ctx;
    struct debug_event *event;

    assert( debug_ctx );
    /* cannot queue a debug event for myself */
    assert( debugger->process != thread->process );

    /* build the event */
    if (!(event = alloc_object( &debug_event_ops, -1 ))) return NULL;
    event->next     = NULL;
    event->prev     = NULL;
    event->state    = EVENT_QUEUED;
    event->sender   = (struct thread *)grab_object( thread );
    event->debugger = (struct thread *)grab_object( debugger );
    if (data) memcpy( &event->data, data, sizeof(event->data) );
    event->data.code = code;

    if (!fill_debug_event( event, arg ))
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
        struct debug_event *event = queue_debug_event( thread, code, arg, NULL );
        if (event) release_object( event );
    }
}

/* generate all startup events of a given process */
void generate_startup_debug_events( struct process *process )
{
    struct process_dll *dll;
    struct thread *thread = process->thread_list;

    /* generate creation events */
    generate_debug_event( thread, CREATE_PROCESS_DEBUG_EVENT, process );
    while ((thread = thread->next))
        generate_debug_event( thread, CREATE_THREAD_DEBUG_EVENT, thread );

    /* generate dll events (in loading order, i.e. reverse list order) */
    for (dll = &process->exe; dll->next; dll = dll->next);
    while (dll != &process->exe)
    {
        generate_debug_event( process->thread_list, LOAD_DLL_DEBUG_EVENT, dll );
        dll = dll->prev;
    }
}

/* return a pointer to the context in case the thread is inside an exception event */
CONTEXT *get_debug_context( struct thread *thread )
{
    struct debug_event *event;
    struct thread *debugger = thread->process->debugger;

    if (!debugger) return NULL;  /* not being debugged */
    assert( debugger->debug_ctx );

    /* find the exception event in the debugger's queue */
    for (event = debugger->debug_ctx->event_head; event; event = event->next)
        if (event->sender == thread && (event->data.code == EXCEPTION_DEBUG_EVENT))
            return &event->data.info.exception.context;
    return NULL;
}

/* attach a process to a debugger thread */
int debugger_attach( struct process *process, struct thread *debugger )
{
    struct debug_ctx *debug_ctx;
    struct thread *thread;

    if (process->debugger)  /* already being debugged */
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    /* make sure we don't create a debugging loop */
    for (thread = debugger; thread; thread = thread->process->debugger)
        if (thread->process == process)
        {
            set_error( STATUS_ACCESS_DENIED );
            return 0;
        }

    if (!debugger->debug_ctx)  /* need to allocate a context */
    {
        if (!(debug_ctx = alloc_object( &debug_ctx_ops, -1 ))) return 0;
        debug_ctx->event_head = NULL;
        debug_ctx->event_tail = NULL;
        debug_ctx->to_send    = NULL;
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
    struct process *process = get_process_from_id( req->pid );
    if (!process) return;
    if (debugger_attach( process, current ))
    {
        generate_startup_debug_events( process );
        /* FIXME: breakpoint exception event */
    }
    release_object( process );
}

/* Send a debug event */
DECL_HANDLER(send_debug_event)
{
    struct debug_event *event;
    int code = req->event.code;

    if ((code <= 0) || (code > RIP_EVENT))
    {
        fatal_protocol_error( current, "send_debug_event: bad code %d\n", code );
        return;
    }
    req->status = 0;
    if (current->process->debugger && ((event = queue_debug_event( current, code,
                                                                   NULL, &req->event ))))
    {
        /* wait for continue_debug_event */
        struct object *obj = &event->obj;
        sleep_on( 1, &obj, 0, -1, build_send_event_reply );
        release_object( event );
    }
}

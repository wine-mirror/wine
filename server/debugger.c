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

enum debug_event_state { EVENT_QUEUED, EVENT_SENT, EVENT_DELAYED, EVENT_CONTINUED };

/* debug event */
struct debug_event
{
    struct object          obj;       /* object header */
    struct list            entry;     /* entry in event queue */
    struct thread         *sender;    /* thread which sent this event */
    struct file           *file;      /* file object for events that need one */
    enum debug_event_state state;     /* event state */
    int                    status;    /* continuation status */
    debug_event_t          data;      /* event data */
};

/* debug object */
struct debug_obj
{
    struct object        obj;         /* object header */
    struct list          event_queue; /* pending events queue */
    unsigned int         flags;       /* debug flags */
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
    no_get_full_name,              /* get_full_name */
    no_lookup_name,                /* lookup_name */
    no_link_name,                  /* link_name */
    NULL,                          /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    no_close_handle,               /* close_handle */
    debug_event_destroy            /* destroy */
};

static void debug_obj_dump( struct object *obj, int verbose );
static struct object_type *debug_obj_get_type( struct object *obj );
static int debug_obj_signaled( struct object *obj, struct wait_queue_entry *entry );
static unsigned int debug_obj_map_access( struct object *obj, unsigned int access );
static void debug_obj_destroy( struct object *obj );

static const struct object_ops debug_obj_ops =
{
    sizeof(struct debug_obj),      /* size */
    debug_obj_dump,                /* dump */
    debug_obj_get_type,            /* get_type */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_obj_signaled,            /* signaled */
    no_satisfied,                  /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    debug_obj_map_access,          /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    default_get_full_name,         /* get_full_name */
    no_lookup_name,                /* lookup_name */
    directory_link_name,           /* link_name */
    default_unlink_name,           /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    no_close_handle,               /* close_handle */
    debug_obj_destroy              /* destroy */
};


/* routines to build an event according to its type */

static void fill_exception_event( struct debug_event *event, const void *arg )
{
    const debug_event_t *data = arg;
    event->data.exception = data->exception;
    event->data.exception.nb_params = min( event->data.exception.nb_params, EXCEPTION_MAXIMUM_PARAMETERS );
}

static void fill_create_thread_event( struct debug_event *event, const void *arg )
{
    struct thread *thread = event->sender;
    const client_ptr_t *entry = arg;

    event->data.create_thread.teb    = thread->teb;
    if (entry) event->data.create_thread.start = *entry;
}

static void fill_create_process_event( struct debug_event *event, const void *arg )
{
    struct thread *thread = event->sender;
    struct process *process = thread->process;
    struct process_dll *exe_module = get_process_exe_module( process );
    const client_ptr_t *entry = arg;

    event->data.create_process.teb        = thread->teb;
    event->data.create_process.base       = exe_module->base;
    event->data.create_process.start      = *entry;
    event->data.create_process.dbg_offset = exe_module->dbg_offset;
    event->data.create_process.dbg_size   = exe_module->dbg_size;
    event->data.create_process.name       = exe_module->name;
    event->data.create_process.unicode    = 1;

    /* the doc says write access too, but this doesn't seem a good idea */
    event->file = get_mapping_file( process, exe_module->base, GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE );
}

static void fill_exit_thread_event( struct debug_event *event, const void *arg )
{
    const struct thread *thread = arg;
    event->data.exit.exit_code = thread->exit_code;
}

static void fill_exit_process_event( struct debug_event *event, const void *arg )
{
    const struct process *process = arg;
    event->data.exit.exit_code = process->exit_code;
}

static void fill_load_dll_event( struct debug_event *event, const void *arg )
{
    struct process *process = event->sender->process;
    const struct process_dll *dll = arg;

    event->data.load_dll.handle     = 0;
    event->data.load_dll.base       = dll->base;
    event->data.load_dll.dbg_offset = dll->dbg_offset;
    event->data.load_dll.dbg_size   = dll->dbg_size;
    event->data.load_dll.name       = dll->name;
    event->data.load_dll.unicode    = 1;
    event->file = get_mapping_file( process, dll->base, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE );
}

static void fill_unload_dll_event( struct debug_event *event, const void *arg )
{
    const mod_handle_t *base = arg;
    event->data.unload_dll.base = *base;
}

typedef void (*fill_event_func)( struct debug_event *event, const void *arg );

#define NB_DEBUG_EVENTS UNLOAD_DLL_DEBUG_EVENT

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

/* allocate the necessary handles in the event data */
static void alloc_event_handles( struct debug_event *event, struct process *process )
{
    switch (event->data.code)
    {
    case CREATE_THREAD_DEBUG_EVENT:
        /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
        event->data.create_thread.handle = alloc_handle( process, event->sender, THREAD_ALL_ACCESS, 0 );
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        event->data.create_process.thread = alloc_handle( process, event->sender, THREAD_ALL_ACCESS, 0 );
        /* documented: PROCESS_VM_READ | PROCESS_VM_WRITE */
        event->data.create_process.process = alloc_handle( process, event->sender->process,
                                                           PROCESS_ALL_ACCESS, 0 );
        if (event->file)
            event->data.create_process.file = alloc_handle( process, event->file, GENERIC_READ, 0 );
        break;
    case LOAD_DLL_DEBUG_EVENT:
        if (event->file)
            event->data.load_dll.handle = alloc_handle( process, event->file, GENERIC_READ, 0 );
        break;
    }
    clear_error();  /* ignore errors, simply set handles to 0 */
}

/* unlink the first event from the queue */
static void unlink_event( struct debug_obj *debug_obj, struct debug_event *event )
{
    list_remove( &event->entry );
    if (event->sender->process->debug_event == event) event->sender->process->debug_event = NULL;
    release_object( event );
}

/* link an event at the end of the queue */
static void link_event( struct debug_obj *debug_obj, struct debug_event *event )
{
    grab_object( event );
    list_add_tail( &debug_obj->event_queue, &event->entry );
    if (!event->sender->process->debug_event)
    {
        /* grab reference since debugger could be killed while trying to wake up */
        grab_object( debug_obj );
        wake_up( &debug_obj->obj, 0 );
        release_object( debug_obj );
    }
}

/* resume a delayed debug event already in the queue */
static void resume_event( struct debug_obj *debug_obj, struct debug_event *event )
{
    event->state = EVENT_QUEUED;
    if (!event->sender->process->debug_event)
    {
        grab_object( debug_obj );
        wake_up( &debug_obj->obj, 0 );
        release_object( debug_obj );
    }
}

/* delay a debug event already in the queue to be replayed when thread wakes up */
static void delay_event( struct debug_obj *debug_obj, struct debug_event *event )
{
    event->state = EVENT_DELAYED;
    if (event->sender->process->debug_event == event) event->sender->process->debug_event = NULL;
}

/* find the next event that we can send to the debugger */
static struct debug_event *find_event_to_send( struct debug_obj *debug_obj )
{
    struct debug_event *event;

    LIST_FOR_EACH_ENTRY( event, &debug_obj->event_queue, struct debug_event, entry )
    {
        if (event->state == EVENT_SENT) continue;  /* already sent */
        if (event->state == EVENT_DELAYED) continue;  /* delayed until thread resumes */
        if (event->sender->process->debug_event) continue;  /* process busy with another one */
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

    if (event->file) release_object( event->file );
    release_object( event->sender );
}

static void debug_obj_dump( struct object *obj, int verbose )
{
    struct debug_obj *debug_obj = (struct debug_obj *)obj;
    assert( obj->ops == &debug_obj_ops );
    fprintf( stderr, "Debug context head=%p tail=%p\n",
             debug_obj->event_queue.next, debug_obj->event_queue.prev );
}

static struct object_type *debug_obj_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','e','b','u','g','O','b','j','e','c','t'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static int debug_obj_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct debug_obj *debug_obj = (struct debug_obj *)obj;
    assert( obj->ops == &debug_obj_ops );
    return find_event_to_send( debug_obj ) != NULL;
}

static unsigned int debug_obj_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | DEBUG_READ_EVENT | DEBUG_QUERY_INFORMATION;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | DEBUG_SET_INFORMATION;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE | DEBUG_PROCESS_ASSIGN;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL | EVENT_QUERY_STATE | EVENT_MODIFY_STATE;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void debug_obj_destroy( struct object *obj )
{
    struct list *ptr;
    struct debug_obj *debug_obj = (struct debug_obj *)obj;
    assert( obj->ops == &debug_obj_ops );

    detach_debugged_processes( debug_obj,
                               (debug_obj->flags & DEBUG_KILL_ON_CLOSE) ? STATUS_DEBUGGER_INACTIVE : 0 );

    /* free all pending events */
    while ((ptr = list_head( &debug_obj->event_queue )))
        unlink_event( debug_obj, LIST_ENTRY( ptr, struct debug_event, entry ));
}

static struct debug_obj *create_debug_obj( struct object *root, const struct unicode_str *name,
                                           unsigned int attr, unsigned int flags,
                                           const struct security_descriptor *sd )
{
    struct debug_obj *debug_obj;

    if ((debug_obj = create_named_object( root, &debug_obj_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            debug_obj->flags = flags;
            list_init( &debug_obj->event_queue );
        }
    }
    return debug_obj;
}

/* continue a debug event */
static int continue_debug_event( struct process *process, struct thread *thread, int status )
{
    struct debug_obj *debug_obj = current->debug_obj;

    if (debug_obj && process->debug_obj == debug_obj && thread->process == process)
    {
        struct debug_event *event;

        if (status == DBG_REPLY_LATER)
        {
            /* if thread is suspended, delay all its events and resume process
             * if not, reset the event for immediate replay */
            LIST_FOR_EACH_ENTRY( event, &debug_obj->event_queue, struct debug_event, entry )
            {
                if (event->sender != thread) continue;
                if (thread->suspend)
                {
                    delay_event( debug_obj, event );
                    resume_process( process );
                }
                else if (event->state == EVENT_SENT)
                {
                    assert( event->sender->process->debug_event == event );
                    event->sender->process->debug_event = NULL;
                    resume_event( debug_obj, event );
                    return 1;
                }
            }
            return 1;
        }

        /* find the event in the queue */
        LIST_FOR_EACH_ENTRY( event, &debug_obj->event_queue, struct debug_event, entry )
        {
            if (event->state != EVENT_SENT) continue;
            if (event->sender == thread)
            {
                assert( event->sender->process->debug_event == event );
                event->status = status;
                event->state  = EVENT_CONTINUED;
                wake_up( &event->obj, 0 );
                unlink_event( debug_obj, event );
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
    struct debug_event *event;

    assert( code > 0 && code <= NB_DEBUG_EVENTS );

    /* build the event */
    if (!(event = alloc_object( &debug_event_ops ))) return NULL;
    event->state     = EVENT_QUEUED;
    event->sender    = (struct thread *)grab_object( thread );
    event->file      = NULL;
    memset( &event->data, 0, sizeof(event->data) );

    fill_debug_event[code-1]( event, arg );
    event->data.code = code;
    return event;
}

/* generate a debug event from inside the server and queue it */
void generate_debug_event( struct thread *thread, int code, const void *arg )
{
    struct debug_obj *debug_obj = thread->process->debug_obj;

    if (debug_obj)
    {
        struct debug_event *event = alloc_debug_event( thread, code, arg );
        if (event)
        {
            link_event( debug_obj, event );
            suspend_process( thread->process );
            release_object( event );
        }
        clear_error();  /* ignore errors */
    }
}

void resume_delayed_debug_events( struct thread *thread )
{
    struct debug_obj *debug_obj = thread->process->debug_obj;
    struct debug_event *event;

    if (debug_obj)
    {
        LIST_FOR_EACH_ENTRY( event, &debug_obj->event_queue, struct debug_event, entry )
        {
            if (event->sender != thread) continue;
            if (event->state != EVENT_DELAYED) continue;
            resume_event( debug_obj, event );
            suspend_process( thread->process );
        }
    }
}

/* attach a process to a debugger thread and suspend it */
static int debugger_attach( struct process *process, struct thread *debugger )
{
    if (process->debug_obj) goto error;  /* already being debugged */
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
        process->debug_obj = NULL;
        resume_process( process );
        return 0;
    }
    process->debug_children = 0;
    return 1;

 error:
    set_error( STATUS_ACCESS_DENIED );
    return 0;
}


/* detach a process from a debugger thread (and resume it) */
void debugger_detach( struct process *process, struct debug_obj *debug_obj )
{
    struct debug_event *event, *next;

    /* init should be done, otherwise wouldn't be attached */
    assert(is_process_init_done(process));

    suspend_process( process );

    /* send continue indication for all events */
    LIST_FOR_EACH_ENTRY_SAFE( event, next, &debug_obj->event_queue, struct debug_event, entry )
    {
        if (event->sender->process != process) continue;

        assert( event->state != EVENT_CONTINUED );
        event->status = DBG_CONTINUE;
        event->state  = EVENT_CONTINUED;
        wake_up( &event->obj, 0 );
        unlink_event( debug_obj, event );
        /* from queued debug event */
        resume_process( process );
    }

    /* remove relationships between process and its debugger */
    process->debug_obj = NULL;
    if (!set_process_debug_flag( process, 0 )) clear_error();  /* ignore error */

    /* from this function */
    resume_process( process );
}

/* generate all startup events of a given process */
void generate_startup_debug_events( struct process *process, client_ptr_t entry )
{
    struct list *ptr;
    struct thread *thread, *first_thread = get_process_first_thread( process );

    generate_debug_event( first_thread, CREATE_PROCESS_DEBUG_EVENT, &entry );
    ptr = list_head( &process->dlls ); /* skip main module reported in create process event */

    /* generate ntdll.dll load event */
    if (ptr && (ptr = list_next( &process->dlls, ptr )))
    {
        struct process_dll *dll = LIST_ENTRY( ptr, struct process_dll, entry );
        generate_debug_event( first_thread, LOAD_DLL_DEBUG_EVENT, dll );
    }

    /* generate creation events */
    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread != first_thread)
            generate_debug_event( thread, CREATE_THREAD_DEBUG_EVENT, NULL );
    }

    /* generate dll events (in loading order) */
    while (ptr && (ptr = list_next( &process->dlls, ptr )))
    {
        struct process_dll *dll = LIST_ENTRY( ptr, struct process_dll, entry );
        generate_debug_event( first_thread, LOAD_DLL_DEBUG_EVENT, dll );
    }
}

/* set the debugger of a given process */
int set_process_debugger( struct process *process, struct thread *debugger )
{
    struct debug_obj *debug_obj;

    assert( !process->debug_obj );

    if (!debugger->debug_obj)  /* need to allocate a context */
    {
        if (!(debug_obj = create_debug_obj( NULL, NULL, 0, DEBUG_KILL_ON_CLOSE, NULL ))) return 0;
        debugger->debug_obj = debug_obj;
    }
    process->debug_obj = debugger->debug_obj;
    return 1;
}

/* a thread is exiting */
void debug_exit_thread( struct thread *thread )
{
    struct debug_obj *debug_obj = thread->debug_obj;

    if (debug_obj)  /* this thread is a debugger */
    {
        detach_debugged_processes( debug_obj,
                                   (debug_obj->flags & DEBUG_KILL_ON_CLOSE) ? STATUS_DEBUGGER_INACTIVE : 0 );
        release_object( thread->debug_obj );
        thread->debug_obj = NULL;
    }
}

/* create a debug object */
DECL_HANDLER(create_debug_obj)
{
    struct debug_obj *debug_obj;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;
    if ((debug_obj = create_debug_obj( root, &name, objattr->attributes, req->flags, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, debug_obj, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, debug_obj,
                                                          req->access, objattr->attributes );
        release_object( debug_obj );
    }
    if (root) release_object( root );
}

/* Wait for a debug event */
DECL_HANDLER(wait_debug_event)
{
    struct debug_obj *debug_obj = current->debug_obj;
    struct debug_event *event;

    if (!debug_obj)  /* current thread is not a debugger */
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    reply->wait = 0;
    if ((event = find_event_to_send( debug_obj )))
    {
        event->state = EVENT_SENT;
        event->sender->process->debug_event = event;
        reply->pid = get_process_id( event->sender->process );
        reply->tid = get_thread_id( event->sender );
        alloc_event_handles( event, current->process );
        set_reply_data( &event->data, min( get_reply_max_size(), sizeof(event->data) ));
    }
    else  /* no event ready */
    {
        reply->pid  = 0;
        reply->tid  = 0;
        if (req->get_handle)
            reply->wait = alloc_handle( current->process, debug_obj, SYNCHRONIZE, 0 );
    }
}

/* Continue a debug event */
DECL_HANDLER(continue_debug_event)
{
    struct process *process;

    if (req->status != DBG_EXCEPTION_NOT_HANDLED &&
        req->status != DBG_EXCEPTION_HANDLED &&
        req->status != DBG_CONTINUE &&
        req->status != DBG_REPLY_LATER)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if ((process = get_process_from_id( req->pid )))
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
        if (current->debug_obj && process->debug_obj == current->debug_obj)
            debugger_detach( process, current->debug_obj );
        else
            set_error( STATUS_ACCESS_DENIED );
    }
    else if (debugger_attach( process, current ))
    {
        generate_startup_debug_events( process, 0 );
        resume_process( process );
    }
    release_object( process );
}

/* queue an exception event */
DECL_HANDLER(queue_exception_event)
{
    struct debug_obj *debug_obj = current->process->debug_obj;

    reply->handle = 0;
    if (debug_obj)
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
            if ((reply->handle = alloc_handle( thread->process, event, SYNCHRONIZE, 0 )))
            {
                link_event( debug_obj, event );
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
        set_error( event->state == EVENT_CONTINUED ? event->status : STATUS_PENDING );
        release_object( event );
    }
}

/* set debugger kill on exit flag */
DECL_HANDLER(set_debugger_kill_on_exit)
{
    if (!current->debug_obj)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    current->debug_obj->flags = req->kill_on_exit ? DEBUG_KILL_ON_CLOSE : 0;
}

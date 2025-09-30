/*
 * Server-side event management
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "thread.h"
#include "request.h"
#include "security.h"

static const WCHAR event_name[] = {'E','v','e','n','t'};

struct type_descr event_type =
{
    { event_name, sizeof(event_name) },   /* name */
    EVENT_ALL_ACCESS,                     /* valid_access */
    {                                     /* mapping */
        STANDARD_RIGHTS_READ | EVENT_QUERY_STATE,
        STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
        EVENT_ALL_ACCESS
    },
};

struct event_sync
{
    struct object  obj;             /* object header */
    unsigned int   manual : 1;      /* is it a manual reset event? */
    unsigned int   signaled : 1;    /* event has been signaled */
};

static void event_sync_dump( struct object *obj, int verbose );
static int event_sync_signaled( struct object *obj, struct wait_queue_entry *entry );
static void event_sync_satisfied( struct object *obj, struct wait_queue_entry *entry );
static int event_sync_signal( struct object *obj, unsigned int access, int signal );

static const struct object_ops event_sync_ops =
{
    sizeof(struct event_sync), /* size */
    &no_type,                  /* type */
    event_sync_dump,           /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    event_sync_signaled,       /* signaled */
    event_sync_satisfied,      /* satisfied */
    event_sync_signal,         /* signal */
    no_get_fd,                 /* get_fd */
    default_get_sync,          /* get_sync */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    no_destroy                 /* destroy */
};

static struct event_sync *create_event_sync( int manual, int signaled )
{
    struct event_sync *event;

    if (!(event = alloc_object( &event_sync_ops ))) return NULL;
    event->manual   = manual;
    event->signaled = signaled;

    return event;
}

struct event_sync *create_server_internal_sync( int manual, int signaled )
{
    struct event_sync *event;

    if (!(event = alloc_object( &event_sync_ops ))) return NULL;
    event->manual   = manual;
    event->signaled = signaled;

    return event;
}

struct event_sync *create_internal_sync( int manual, int signaled )
{
    return create_event_sync( manual, signaled );
}

static void event_sync_dump( struct object *obj, int verbose )
{
    struct event_sync *event = (struct event_sync *)obj;
    assert( obj->ops == &event_sync_ops );
    fprintf( stderr, "Event manual=%d signaled=%d\n",
             event->manual, event->signaled );
}

static int event_sync_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct event_sync *event = (struct event_sync *)obj;
    assert( obj->ops == &event_sync_ops );
    return event->signaled;
}

void signal_sync( struct event_sync *sync )
{
    sync->obj.ops->signal( &sync->obj, 0, 1 );
}

void reset_sync( struct event_sync *sync )
{
    sync->obj.ops->signal( &sync->obj, 0, 0 );
}

static void event_sync_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct event_sync *event = (struct event_sync *)obj;
    assert( obj->ops == &event_sync_ops );
    /* Reset if it's an auto-reset event */
    if (!event->manual) event->signaled = 0;
}

static int event_sync_signal( struct object *obj, unsigned int access, int signal )
{
    struct event_sync *event = (struct event_sync *)obj;
    assert( obj->ops == &event_sync_ops );

    /* wake up all waiters if manual reset, a single one otherwise */
    if ((event->signaled = !!signal)) wake_up( &event->obj, !event->manual );
    return 1;
}

struct event
{
    struct object      obj;             /* object header */
    struct event_sync *sync;            /* event sync object */
    struct list        kernel_object;   /* list of kernel object pointers */
};

static void event_dump( struct object *obj, int verbose );
static struct object *event_get_sync( struct object *obj );
static int event_signal( struct object *obj, unsigned int access, int signal );
static struct list *event_get_kernel_obj_list( struct object *obj );
static void event_destroy( struct object *obj );

static const struct object_ops event_ops =
{
    sizeof(struct event),      /* size */
    &event_type,               /* type */
    event_dump,                /* dump */
    NULL,                      /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    event_signal,              /* signal */
    no_get_fd,                 /* get_fd */
    event_get_sync,            /* get_sync */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    event_get_kernel_obj_list, /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    event_destroy,             /* destroy */
};


static const WCHAR keyed_event_name[] = {'K','e','y','e','d','E','v','e','n','t'};

struct type_descr keyed_event_type =
{
    { keyed_event_name, sizeof(keyed_event_name) },   /* name */
    KEYEDEVENT_ALL_ACCESS | SYNCHRONIZE,              /* valid_access */
    {                                                 /* mapping */
        STANDARD_RIGHTS_READ | KEYEDEVENT_WAIT,
        STANDARD_RIGHTS_WRITE | KEYEDEVENT_WAKE,
        STANDARD_RIGHTS_EXECUTE,
        KEYEDEVENT_ALL_ACCESS
    },
};

struct keyed_event
{
    struct object  obj;             /* object header */
};

static void keyed_event_dump( struct object *obj, int verbose );
static int keyed_event_signaled( struct object *obj, struct wait_queue_entry *entry );

static const struct object_ops keyed_event_ops =
{
    sizeof(struct keyed_event),  /* size */
    &keyed_event_type,           /* type */
    keyed_event_dump,            /* dump */
    add_queue,                   /* add_queue */
    remove_queue,                /* remove_queue */
    keyed_event_signaled,        /* signaled */
    no_satisfied,                /* satisfied */
    no_signal,                   /* signal */
    no_get_fd,                   /* get_fd */
    default_get_sync,            /* get_sync */
    default_map_access,          /* map_access */
    default_get_sd,              /* get_sd */
    default_set_sd,              /* set_sd */
    default_get_full_name,       /* get_full_name */
    no_lookup_name,              /* lookup_name */
    directory_link_name,         /* link_name */
    default_unlink_name,         /* unlink_name */
    no_open_file,                /* open_file */
    no_kernel_obj_list,          /* get_kernel_obj_list */
    no_close_handle,             /* close_handle */
    no_destroy                   /* destroy */
};


struct event *create_event( struct object *root, const struct unicode_str *name,
                            unsigned int attr, int manual_reset, int initial_state,
                            const struct security_descriptor *sd )
{
    struct event *event;

    if ((event = create_named_object( root, &event_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            event->sync = NULL;
            list_init( &event->kernel_object );

            if (!(event->sync = create_event_sync( manual_reset, initial_state )))
            {
                release_object( event );
                return NULL;
            }
        }
    }
    return event;
}

struct event *get_event_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct event *)get_handle_obj( process, handle, access, &event_ops );
}

void set_event( struct event *event )
{
    signal_sync( event->sync );
}

void reset_event( struct event *event )
{
    reset_sync( event->sync );
}

static void event_dump( struct object *obj, int verbose )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    event->sync->obj.ops->dump( &event->sync->obj, verbose );
}

static struct object *event_get_sync( struct object *obj )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    return grab_object( event->sync );
}

static int event_signal( struct object *obj, unsigned int access, int signal )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );

    assert( event->sync->obj.ops == &event_sync_ops ); /* never called with inproc syncs */
    assert( signal == -1 ); /* always called from signal_object */

    if (!(access & EVENT_MODIFY_STATE))
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    return event_sync_signal( &event->sync->obj, 0, 1 );
}

static struct list *event_get_kernel_obj_list( struct object *obj )
{
    struct event *event = (struct event *)obj;
    return &event->kernel_object;
}

static void event_destroy( struct object *obj )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );

    if (event->sync) release_object( event->sync );
}

struct keyed_event *create_keyed_event( struct object *root, const struct unicode_str *name,
                                        unsigned int attr, const struct security_descriptor *sd )
{
    struct keyed_event *event;

    if ((event = create_named_object( root, &keyed_event_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
        }
    }
    return event;
}

struct keyed_event *get_keyed_event_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct keyed_event *)get_handle_obj( process, handle, access, &keyed_event_ops );
}

static void keyed_event_dump( struct object *obj, int verbose )
{
    fputs( "Keyed event\n", stderr );
}

static enum select_opcode matching_op( enum select_opcode op )
{
    return op ^ (SELECT_KEYED_EVENT_WAIT ^ SELECT_KEYED_EVENT_RELEASE);
}

static int keyed_event_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct wait_queue_entry *ptr;
    struct process *process;
    enum select_opcode select_op;

    assert( obj->ops == &keyed_event_ops );

    process = get_wait_queue_thread( entry )->process;
    select_op = get_wait_queue_select_op( entry );
    if (select_op != SELECT_KEYED_EVENT_WAIT && select_op != SELECT_KEYED_EVENT_RELEASE) return 1;

    LIST_FOR_EACH_ENTRY( ptr, &obj->wait_queue, struct wait_queue_entry, entry )
    {
        if (ptr == entry) continue;
        if (get_wait_queue_thread( ptr )->process != process) continue;
        if (get_wait_queue_select_op( ptr ) != matching_op( select_op )) continue;
        if (get_wait_queue_key( ptr ) != get_wait_queue_key( entry )) continue;
        if (wake_thread_queue_entry( ptr )) return 1;
    }
    return 0;
}

/* create an event */
DECL_HANDLER(create_event)
{
    struct event *event;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((event = create_event( root, &name, objattr->attributes,
                               req->manual_reset, req->initial_state, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, event, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, event,
                                                          req->access, objattr->attributes );
        release_object( event );
    }

    if (root) release_object( root );
}

/* open a handle to an event */
DECL_HANDLER(open_event)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &event_ops, &name, req->attributes );
}

/* do an event operation */
DECL_HANDLER(event_op)
{
    struct event *event;

    if (!(event = get_event_obj( current->process, req->handle, EVENT_MODIFY_STATE ))) return;

    reply->state = event->sync->signaled;
    switch(req->op)
    {
    case PULSE_EVENT:
        set_event( event );
        reset_event( event );
        break;
    case SET_EVENT:
        set_event( event );
        break;
    case RESET_EVENT:
        reset_event( event );
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        break;
    }
    release_object( event );
}

/* return details about the event */
DECL_HANDLER(query_event)
{
    struct event *event;

    if (!(event = get_event_obj( current->process, req->handle, EVENT_QUERY_STATE ))) return;

    reply->manual_reset = event->sync->manual;
    reply->state = event->sync->signaled;

    release_object( event );
}

/* create a keyed event */
DECL_HANDLER(create_keyed_event)
{
    struct keyed_event *event;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((event = create_keyed_event( root, &name, objattr->attributes, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, event, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, event,
                                                          req->access, objattr->attributes );
        release_object( event );
    }
    if (root) release_object( root );
}

/* open a handle to a keyed event */
DECL_HANDLER(open_keyed_event)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &keyed_event_ops, &name, req->attributes );
}

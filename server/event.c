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
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

struct event
{
    struct object  obj;             /* object header */
    int            manual_reset;    /* is it a manual reset event? */
    int            signaled;        /* event has been signaled */
};

static void event_dump( struct object *obj, int verbose );
static int event_signaled( struct object *obj, struct thread *thread );
static int event_satisfied( struct object *obj, struct thread *thread );
static unsigned int event_map_access( struct object *obj, unsigned int access );
static int event_signal( struct object *obj, unsigned int access);

static const struct object_ops event_ops =
{
    sizeof(struct event),      /* size */
    event_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    event_signaled,            /* signaled */
    event_satisfied,           /* satisfied */
    event_signal,              /* signal */
    no_get_fd,                 /* get_fd */
    event_map_access,          /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
    no_close_handle,           /* close_handle */
    no_destroy                 /* destroy */
};


struct event *create_event( struct directory *root, const struct unicode_str *name,
                            unsigned int attr, int manual_reset, int initial_state )
{
    struct event *event;

    if ((event = create_named_object_dir( root, name, attr, &event_ops )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            event->manual_reset = manual_reset;
            event->signaled     = initial_state;
        }
    }
    return event;
}

struct event *get_event_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct event *)get_handle_obj( process, handle, access, &event_ops );
}

void pulse_event( struct event *event )
{
    event->signaled = 1;
    /* wake up all waiters if manual reset, a single one otherwise */
    wake_up( &event->obj, !event->manual_reset );
    event->signaled = 0;
}

void set_event( struct event *event )
{
    event->signaled = 1;
    /* wake up all waiters if manual reset, a single one otherwise */
    wake_up( &event->obj, !event->manual_reset );
}

void reset_event( struct event *event )
{
    event->signaled = 0;
}

static void event_dump( struct object *obj, int verbose )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    fprintf( stderr, "Event manual=%d signaled=%d ",
             event->manual_reset, event->signaled );
    dump_object_name( &event->obj );
    fputc( '\n', stderr );
}

static int event_signaled( struct object *obj, struct thread *thread )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    return event->signaled;
}

static int event_satisfied( struct object *obj, struct thread *thread )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    /* Reset if it's an auto-reset event */
    if (!event->manual_reset) event->signaled = 0;
    return 0;  /* Not abandoned */
}

static unsigned int event_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYNCHRONIZE | EVENT_QUERY_STATE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL | EVENT_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static int event_signal( struct object *obj, unsigned int access )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );

    if (!(access & EVENT_MODIFY_STATE))
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    set_event( event );
    return 1;
}

/* create an event */
DECL_HANDLER(create_event)
{
    struct event *event;
    struct unicode_str name;
    struct directory *root = NULL;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((event = create_event( root, &name, req->attributes, req->manual_reset, req->initial_state )))
    {
        reply->handle = alloc_handle( current->process, event, req->access, req->attributes );
        release_object( event );
    }

    if (root) release_object( root );
}

/* open a handle to an event */
DECL_HANDLER(open_event)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct event *event;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((event = open_object_dir( root, &name, req->attributes, &event_ops )))
    {
        reply->handle = alloc_handle( current->process, &event->obj, req->access, req->attributes );
        release_object( event );
    }

    if (root) release_object( root );
}

/* do an event operation */
DECL_HANDLER(event_op)
{
    struct event *event;

    if (!(event = get_event_obj( current->process, req->handle, EVENT_MODIFY_STATE ))) return;
    switch(req->op)
    {
    case PULSE_EVENT:
        pulse_event( event );
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

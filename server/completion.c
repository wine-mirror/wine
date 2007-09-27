/*
 * Server-side IO completion ports implementation
 *
 * Copyright (C) 2007 Andrey Turkin
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
 *
 */

/* FIXMEs:
 *  - built-in wait queues used which means:
 *    + threads are awaken FIFO and not LIFO as native does
 *    + "max concurrent active threads" parameter not used
 *    + completion handle is waitable, while native isn't
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "wine/unicode.h"
#include "object.h"
#include "file.h"
#include "handle.h"
#include "request.h"


struct completion
{
    struct object  obj;
    struct list    queue;
    unsigned int   depth;
};

static void completion_dump( struct object*, int );
static void completion_destroy( struct object * );
static int  completion_signaled( struct object *obj, struct thread *thread );

static const struct object_ops completion_ops =
{
    sizeof(struct completion), /* size */
    completion_dump,           /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    completion_signaled,       /* signaled */
    no_satisfied,              /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    no_map_access,             /* map_access */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
    no_close_handle,           /* close_handle */
    completion_destroy         /* destroy */
};

struct comp_msg
{
    struct   list queue_entry;
    unsigned long ckey;
    unsigned long cvalue;
    unsigned long information;
    unsigned int  status;
};

static void completion_destroy( struct object *obj)
{
    struct completion *completion = (struct completion *) obj;
    struct comp_msg *tmp, *next;

    LIST_FOR_EACH_ENTRY_SAFE( tmp, next, &completion->queue, struct comp_msg, queue_entry )
    {
        free( tmp );
    }
}

static void completion_dump( struct object *obj, int verbose )
{
    struct completion *completion = (struct completion *) obj;

    assert( obj->ops == &completion_ops );
    fprintf( stderr, "Completion " );
    dump_object_name( &completion->obj );
    fprintf( stderr, " (%u packets pending)\n", completion->depth );
}

static int completion_signaled( struct object *obj, struct thread *thread )
{
    struct completion *completion = (struct completion *)obj;

    return !list_empty( &completion->queue );
}

static struct completion *create_completion( struct directory *root, const struct unicode_str *name, unsigned int attr, unsigned int concurrent )
{
    struct completion *completion;

    if ((completion = create_named_object_dir( root, name, attr, &completion_ops )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            list_init( &completion->queue );
            completion->depth = 0;
        }
    }

    return completion;
}

struct completion *get_completion_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct completion *) get_handle_obj( process, handle, access, &completion_ops );
}

static void add_completion( struct completion *completion, unsigned long ckey, unsigned long cvalue, unsigned int status, unsigned long information )
{
    struct comp_msg *msg = mem_alloc( sizeof( *msg ) );

    if (!msg)
        return;

    msg->ckey = ckey;
    msg->cvalue = cvalue;
    msg->status = status;
    msg->information = information;

    list_add_tail( &completion->queue, &msg->queue_entry );
    completion->depth++;
    wake_up( &completion->obj, 1 );
}

/* create a completion */
DECL_HANDLER(create_completion)
{
    struct completion *completion;
    struct unicode_str name;
    struct directory *root = NULL;

    reply->handle = 0;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ( (completion = create_completion( root, &name, req->attributes, req->concurrent )) != NULL )
    {
        reply->handle = alloc_handle( current->process, completion, req->access, req->attributes );
        release_object( completion );
    }

    if (root) release_object( root );
}

/* open a completion */
DECL_HANDLER(open_completion)
{
    struct completion *completion;
    struct unicode_str name;
    struct directory *root = NULL;

    reply->handle = 0;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ( (completion = open_object_dir( root, &name, req->attributes, &completion_ops )) != NULL )
    {
        reply->handle = alloc_handle( current->process, completion, req->access, req->attributes );
        release_object( completion );
    }

    if (root) release_object( root );
}


/* add completion to completion port */
DECL_HANDLER(add_completion)
{
    struct completion* completion = get_completion_obj( current->process, req->handle, IO_COMPLETION_MODIFY_STATE );

    if (!completion) return;

    add_completion( completion, req->ckey, req->cvalue, req->status, req->information );

    release_object( completion );
}

/* get completion from completion port */
DECL_HANDLER(remove_completion)
{
    struct completion* completion = get_completion_obj( current->process, req->handle, IO_COMPLETION_MODIFY_STATE );
    struct list *entry;
    struct comp_msg *msg;

    if (!completion) return;

    entry = list_head( &completion->queue );
    if (!entry)
        set_error( STATUS_PENDING );
    else
    {
        list_remove( entry );
        completion->depth--;
        msg = LIST_ENTRY( entry, struct comp_msg, queue_entry );
        reply->ckey = msg->ckey;
        reply->cvalue = msg->cvalue;
        reply->status = msg->status;
        reply->information = msg->information;
        free( msg );
    }

    release_object( completion );
}

/* get queue depth for completion port */
DECL_HANDLER(query_completion)
{
    struct completion* completion = get_completion_obj( current->process, req->handle, IO_COMPLETION_QUERY_STATE );

    if (!completion) return;

    reply->depth = completion->depth;

    release_object( completion );
}

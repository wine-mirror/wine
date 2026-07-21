/*
 * Server-side advanced local procedure call management
 *
 * Copyright 2026 Zhiyi Zhang for CodeWeavers
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

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

#include "windef.h"
#include "winternl.h"
#include "ntstatus.h"

#include "handle.h"
#include "thread.h"
#include "security.h"
#include "request.h"

static const WCHAR alpc_port_name[] = {'A','L','P','C',' ','P','o','r','t'};

struct type_descr alpc_port_type =
{
    { alpc_port_name, sizeof(alpc_port_name) }, /* name */
    ALPC_PORT_ALL_ACCESS,                       /* valid_access */
    {                                           /* mapping */
         STANDARD_RIGHTS_READ | ALPC_PORT_QUERY_STATE,
         DELETE | ALPC_PORT_QUERY_STATE,
         0,
         ALPC_PORT_ALL_ACCESS
    },
};

enum alpc_port_enum_type
{
    CONNECTION_PORT,
    COMMUNICATION_PORT
};

enum alpc_port_status
{
    UNINITIALIZED,
    CONNECTED,
    REFUSED,
    DISCONNECTED
};

struct alpc_port
{
    struct object            obj;                   /* object header */
    enum alpc_port_enum_type type;                  /* communication port or connection port */
    enum alpc_port_status    status;                /* port status */
    struct thread           *thread;                /* thread owning the port */
    unsigned int             flags;                 /* flags in port attributes */
    mem_size_t               max_msg_len;           /* max message length in port attributes */
};

static void alpc_port_dump( struct object *obj, int verbose );
static void alpc_port_destroy( struct object *obj );

static const struct object_ops alpc_port_ops =
{
    sizeof(struct alpc_port),      /* size */
    &alpc_port_type,               /* type */
    alpc_port_dump,                /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    NULL,                          /* signaled */
    NULL,                          /* satisfied */
    NULL,                          /* signal */
    NULL,                          /* get_fd */
    NULL,                          /* get_sync */
    NULL,                          /* map_access */
    NULL,                          /* get_sd */
    NULL,                          /* set_sd */
    NULL,                          /* get_full_name */
    no_lookup_name,                /* lookup_name */
    directory_link_name,           /* link_name */
    default_unlink_name,           /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    no_close_handle,               /* close_handle */
    alpc_port_destroy              /* destroy */
};

static void alpc_port_dump( struct object *obj, int verbose )
{
    struct alpc_port *port = (struct alpc_port *)obj;
    assert( obj->ops == &alpc_port_ops );
    fprintf( stderr, "ALPC Port type=%d status=%d\n", port->type, port->status );
}

static void alpc_port_destroy( struct object *obj )
{
    struct alpc_port *port = (struct alpc_port *)obj;
    assert( obj->ops == &alpc_port_ops );
    release_object( port->thread );
}

static struct alpc_port *alpc_create_port( struct object *root, const struct unicode_str *name,
                                           unsigned int attr, const struct security_descriptor *sd,
                                           enum alpc_port_enum_type type, unsigned int flags,
                                           mem_size_t max_msg_len )
{
    struct alpc_port *port;

    port = create_named_object( root, &alpc_port_ops, name, attr, sd );
    if (port)
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            port->type = type;
            port->status = UNINITIALIZED;
            port->thread = (struct thread *)grab_object( current );
            port->flags = flags;
            port->max_msg_len = max_msg_len;
        }
        else
        {
            release_object( port );
            return NULL;
        }
    }

    return port;
}

/* Create an ALPC port */
DECL_HANDLER(alpc_create_port)
{
    struct alpc_port *alpc_port;
    const struct security_descriptor *sd;
    struct unicode_str name;
    struct object *root;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if ((alpc_port = alpc_create_port( root, &name, objattr->attributes, sd, CONNECTION_PORT,
                                       req->flags, req->max_msg_len )))
    {
        reply->handle = alloc_handle( current->process, alpc_port, ALPC_PORT_ALL_ACCESS, objattr->attributes );
        release_object( alpc_port );
    }

    if (root) release_object( root );
}

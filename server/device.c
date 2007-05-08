/*
 * Server-side device support
 *
 * Copyright (C) 2007 Alexandre Julliard
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "object.h"
#include "file.h"
#include "handle.h"
#include "request.h"

struct device_manager
{
    struct object          obj;           /* object header */
    struct list            devices;       /* list of devices */
};

static void device_manager_dump( struct object *obj, int verbose );
static void device_manager_destroy( struct object *obj );

static const struct object_ops device_manager_ops =
{
    sizeof(struct device_manager),    /* size */
    device_manager_dump,              /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    no_map_access,                    /* map_access */
    no_lookup_name,                   /* lookup_name */
    no_open_file,                     /* open_file */
    no_close_handle,                  /* close_handle */
    device_manager_destroy            /* destroy */
};


struct device
{
    struct object          obj;           /* object header */
    struct device_manager *manager;       /* manager for this device (or NULL if deleted) */
    struct fd             *fd;            /* file descriptor for ioctl */
    void                  *user_ptr;      /* opaque ptr for client side */
    struct list            entry;         /* entry in device manager list */
};

static void device_dump( struct object *obj, int verbose );
static struct fd *device_get_fd( struct object *obj );
static void device_destroy( struct object *obj );
static struct object *device_open_file( struct object *obj, unsigned int access,
                                        unsigned int sharing, unsigned int options );
static enum server_fd_type device_get_fd_type( struct fd *fd );

static const struct object_ops device_ops =
{
    sizeof(struct device),            /* size */
    device_dump,                      /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    device_get_fd,                    /* get_fd */
    no_map_access,                    /* map_access */
    no_lookup_name,                   /* lookup_name */
    device_open_file,                 /* open_file */
    no_close_handle,                  /* close_handle */
    device_destroy                    /* destroy */
};

static const struct fd_ops device_fd_ops =
{
    default_fd_get_poll_events,       /* get_poll_events */
    default_poll_event,               /* poll_event */
    no_flush,                         /* flush */
    device_get_fd_type,               /* get_fd_type */
    default_fd_ioctl,                 /* ioctl */
    default_fd_queue_async,           /* queue_async */
    default_fd_reselect_async,        /* reselect_async */
    default_fd_cancel_async           /* cancel_async */
};


static void device_dump( struct object *obj, int verbose )
{
    struct device *device = (struct device *)obj;

    fprintf( stderr, "Device " );
    dump_object_name( &device->obj );
    fputc( '\n', stderr );
}

static struct fd *device_get_fd( struct object *obj )
{
    struct device *device = (struct device *)obj;

    return (struct fd *)grab_object( device->fd );
}

static void device_destroy( struct object *obj )
{
    struct device *device = (struct device *)obj;

    if (device->fd) release_object( device->fd );
    if (device->manager) list_remove( &device->entry );
}

static struct object *device_open_file( struct object *obj, unsigned int access,
                                        unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static enum server_fd_type device_get_fd_type( struct fd *fd )
{
    return FD_TYPE_DEVICE;
}

static struct device *create_device( struct directory *root, const struct unicode_str *name,
                                     struct device_manager *manager, unsigned int attr )
{
    struct device *device;

    if ((device = create_named_object_dir( root, name, attr, &device_ops )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            device->manager = manager;
            list_add_tail( &manager->devices, &device->entry );
            if (!(device->fd = alloc_pseudo_fd( &device_fd_ops, &device->obj, 0 )))
            {
                release_object( device );
                device = NULL;
            }
        }
    }
    return device;
}

static void delete_device( struct device *device )
{
    if (!device->manager) return;  /* already deleted */

    unlink_named_object( &device->obj );
    list_remove( &device->entry );
    device->manager = NULL;
}


static void device_manager_dump( struct object *obj, int verbose )
{
    fprintf( stderr, "Device manager\n" );
}

static void device_manager_destroy( struct object *obj )
{
    struct device_manager *manager = (struct device_manager *)obj;
    struct list *ptr;

    while ((ptr = list_head( &manager->devices )))
    {
        struct device *device = LIST_ENTRY( ptr, struct device, entry );
        delete_device( device );
    }
}

static struct device_manager *create_device_manager(void)
{
    struct device_manager *manager;

    if ((manager = alloc_object( &device_manager_ops )))
    {
        list_init( &manager->devices );
    }
    return manager;
}


/* create a device manager */
DECL_HANDLER(create_device_manager)
{
    struct device_manager *manager = create_device_manager();

    if (manager)
    {
        reply->handle = alloc_handle( current->process, manager, req->access, req->attributes );
        release_object( manager );
    }
}


/* create a device */
DECL_HANDLER(create_device)
{
    struct device *device;
    struct unicode_str name;
    struct device_manager *manager;
    struct directory *root = NULL;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
    {
        release_object( manager );
        return;
    }

    if ((device = create_device( root, &name, manager, req->attributes )))
    {
        device->user_ptr = req->user_ptr;
        reply->handle = alloc_handle( current->process, device, req->access, req->attributes );
        release_object( device );
    }

    if (root) release_object( root );
    release_object( manager );
}


/* delete a device */
DECL_HANDLER(delete_device)
{
    struct device *device;

    if ((device = (struct device *)get_handle_obj( current->process, req->handle, 0, &device_ops )))
    {
        delete_device( device );
        release_object( device );
    }
}

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

struct ioctl_call
{
    struct object          obj;           /* object header */
    struct list            dev_entry;     /* entry in device queue */
    struct list            mgr_entry;     /* entry in manager queue */
    struct device         *device;        /* device containing this ioctl */
    struct thread         *thread;        /* thread that queued the ioctl */
    void                  *user_arg;      /* user arg used to identify the request */
    struct async          *async;         /* pending async op */
    ioctl_code_t           code;          /* ioctl code */
    unsigned int           status;        /* resulting status (or STATUS_PENDING) */
    data_size_t            in_size;       /* size of input data */
    void                  *in_data;       /* input data */
    data_size_t            out_size;      /* size of output data */
    void                  *out_data;      /* output data */
};

static void ioctl_call_dump( struct object *obj, int verbose );
static int ioctl_call_signaled( struct object *obj, struct thread *thread );
static void ioctl_call_destroy( struct object *obj );

static const struct object_ops ioctl_call_ops =
{
    sizeof(struct ioctl_call),        /* size */
    ioctl_call_dump,                  /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    ioctl_call_signaled,              /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    no_map_access,                    /* map_access */
    no_lookup_name,                   /* lookup_name */
    no_open_file,                     /* open_file */
    no_close_handle,                  /* close_handle */
    ioctl_call_destroy                /* destroy */
};


struct device_manager
{
    struct object          obj;           /* object header */
    struct list            devices;       /* list of devices */
    struct list            requests;      /* list of pending ioctls across all devices */
};

static void device_manager_dump( struct object *obj, int verbose );
static int device_manager_signaled( struct object *obj, struct thread *thread );
static void device_manager_destroy( struct object *obj );

static const struct object_ops device_manager_ops =
{
    sizeof(struct device_manager),    /* size */
    device_manager_dump,              /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    device_manager_signaled,          /* signaled */
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
    struct list            requests;      /* list of pending ioctl requests */
};

static void device_dump( struct object *obj, int verbose );
static struct fd *device_get_fd( struct object *obj );
static void device_destroy( struct object *obj );
static struct object *device_open_file( struct object *obj, unsigned int access,
                                        unsigned int sharing, unsigned int options );
static enum server_fd_type device_get_fd_type( struct fd *fd );
static obj_handle_t device_ioctl( struct fd *fd, ioctl_code_t code, const async_data_t *async_data,
                                  const void *data, data_size_t size );

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
    device_ioctl,                     /* ioctl */
    default_fd_queue_async,           /* queue_async */
    default_fd_reselect_async,        /* reselect_async */
    default_fd_cancel_async           /* cancel_async */
};


static void ioctl_call_dump( struct object *obj, int verbose )
{
    struct ioctl_call *ioctl = (struct ioctl_call *)obj;
    fprintf( stderr, "Ioctl call code=%08x device=%p\n", ioctl->code, ioctl->device );
}

static int ioctl_call_signaled( struct object *obj, struct thread *thread )
{
    struct ioctl_call *ioctl = (struct ioctl_call *)obj;

    return !ioctl->device;  /* device is cleared once the ioctl has completed */
}

static void ioctl_call_destroy( struct object *obj )
{
    struct ioctl_call *ioctl = (struct ioctl_call *)obj;

    free( ioctl->in_data );
    free( ioctl->out_data );
    if (ioctl->async)
    {
        async_terminate( ioctl->async, STATUS_CANCELLED );
        release_object( ioctl->async );
    }
    if (ioctl->device) release_object( ioctl->device );
    release_object( ioctl->thread );
}

static struct ioctl_call *create_ioctl( struct device *device, ioctl_code_t code,
                                        const void *in_data, data_size_t in_size,
                                        data_size_t out_size )
{
    struct ioctl_call *ioctl;

    if ((ioctl = alloc_object( &ioctl_call_ops )))
    {
        ioctl->device   = (struct device *)grab_object( device );
        ioctl->code     = code;
        ioctl->async    = NULL;
        ioctl->status   = STATUS_PENDING;
        ioctl->in_size  = in_size;
        ioctl->in_data  = NULL;
        ioctl->out_size = out_size;
        ioctl->out_data = NULL;

        if (ioctl->in_size && !(ioctl->in_data = memdup( in_data, in_size )))
        {
            release_object( ioctl );
            ioctl = NULL;
        }
    }
    return ioctl;
}

static void set_ioctl_result( struct ioctl_call *ioctl, unsigned int status,
                              const void *out_data, data_size_t out_size )
{
    struct device *device = ioctl->device;

    if (!device) return;  /* already finished */

    /* FIXME: handle the STATUS_PENDING case */
    ioctl->status = status;
    ioctl->out_size = min( ioctl->out_size, out_size );
    if (ioctl->out_size && !(ioctl->out_data = memdup( out_data, ioctl->out_size )))
        ioctl->out_size = 0;
    release_object( device );
    ioctl->device = NULL;
    if (ioctl->async)
    {
        async_terminate( ioctl->async, ioctl->out_size ? STATUS_ALERTED : status );
        release_object( ioctl->async );
        ioctl->async = NULL;
    }
    wake_up( &ioctl->obj, 0 );
}


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
    struct ioctl_call *ioctl, *next;

    LIST_FOR_EACH_ENTRY_SAFE( ioctl, next, &device->requests, struct ioctl_call, dev_entry )
    {
        list_remove( &ioctl->dev_entry );
        release_object( ioctl );  /* no longer on the device queue */
    }
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

static struct ioctl_call *find_ioctl_call( struct device *device, struct thread *thread,
                                           void *user_arg )
{
    struct ioctl_call *ioctl;

    LIST_FOR_EACH_ENTRY( ioctl, &device->requests, struct ioctl_call, dev_entry )
        if (ioctl->thread == thread && ioctl->user_arg == user_arg) return ioctl;

    set_error( STATUS_INVALID_PARAMETER );
    return NULL;
}

static obj_handle_t device_ioctl( struct fd *fd, ioctl_code_t code, const async_data_t *async_data,
                                  const void *data, data_size_t size )
{
    struct device *device = get_fd_user( fd );
    struct ioctl_call *ioctl;
    obj_handle_t handle;

    if (!device->manager)  /* it has been deleted */
    {
        set_error( STATUS_FILE_DELETED );
        return 0;
    }

    if (!(ioctl = create_ioctl( device, code, data, size, get_reply_max_size() )))
        return 0;

    ioctl->thread   = (struct thread *)grab_object( current );
    ioctl->user_arg = async_data->arg;

    if (!(handle = alloc_handle( current->process, ioctl, SYNCHRONIZE, 0 )))
    {
        release_object( ioctl );
        return 0;
    }

    if (!(ioctl->async = fd_queue_async( device->fd, async_data, ASYNC_TYPE_WAIT, 0 )))
    {
        close_handle( current->process, handle );
        release_object( ioctl );
        return 0;
    }
    list_add_tail( &device->requests, &ioctl->dev_entry );
    list_add_tail( &device->manager->requests, &ioctl->mgr_entry );
    if (list_head( &device->manager->requests ) == &ioctl->mgr_entry)  /* first one */
        wake_up( &device->manager->obj, 0 );
    /* don't release ioctl since it is now queued in the device */
    set_error( STATUS_PENDING );
    return handle;
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
            list_init( &device->requests );
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
    struct ioctl_call *ioctl;

    if (!device->manager) return;  /* already deleted */

    /* terminate all pending requests */
    LIST_FOR_EACH_ENTRY( ioctl, &device->requests, struct ioctl_call, dev_entry )
    {
        list_remove( &ioctl->mgr_entry );
        set_ioctl_result( ioctl, STATUS_FILE_DELETED, NULL, 0 );
    }
    unlink_named_object( &device->obj );
    list_remove( &device->entry );
    device->manager = NULL;
}


static void device_manager_dump( struct object *obj, int verbose )
{
    fprintf( stderr, "Device manager\n" );
}

static int device_manager_signaled( struct object *obj, struct thread *thread )
{
    struct device_manager *manager = (struct device_manager *)obj;

    return !list_empty( &manager->requests );
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
        list_init( &manager->requests );
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


/* retrieve the next pending device ioctl request */
DECL_HANDLER(get_next_device_request)
{
    struct ioctl_call *ioctl;
    struct device_manager *manager;
    struct list *ptr;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if (req->prev)
    {
        if ((ioctl = (struct ioctl_call *)get_handle_obj( current->process, req->prev,
                                                          0, &ioctl_call_ops )))
        {
            set_ioctl_result( ioctl, req->status, get_req_data(), get_req_data_size() );
            close_handle( current->process, req->prev );  /* avoid an extra round-trip for close */
            release_object( ioctl );
        }
        clear_error();
    }

    if ((ptr = list_head( &manager->requests )))
    {
        ioctl = LIST_ENTRY( ptr, struct ioctl_call, mgr_entry );
        reply->code = ioctl->code;
        reply->user_ptr = ioctl->device->user_ptr;
        reply->in_size = ioctl->in_size;
        reply->out_size = ioctl->out_size;
        if (ioctl->in_size > get_reply_max_size()) set_error( STATUS_BUFFER_OVERFLOW );
        else if ((reply->next = alloc_handle( current->process, ioctl, 0, 0 )))
        {
            set_reply_data_ptr( ioctl->in_data, ioctl->in_size );
            ioctl->in_data = NULL;
            ioctl->in_size = 0;
            list_remove( &ioctl->mgr_entry );
            list_init( &ioctl->mgr_entry );
        }
    }
    else set_error( STATUS_PENDING );

    release_object( manager );
}


/* retrieve results of an async ioctl */
DECL_HANDLER(get_ioctl_result)
{
    struct device *device;
    struct ioctl_call *ioctl;

    if (!(device = (struct device *)get_handle_obj( current->process, req->handle, 0, &device_ops )))
        return;

    if ((ioctl = find_ioctl_call( device, current, req->user_arg )))
    {
        if (ioctl->out_data)
        {
            data_size_t size = min( ioctl->out_size, get_reply_max_size() );
            if (size)
            {
                set_reply_data_ptr( ioctl->out_data, size );
                ioctl->out_data = NULL;
            }
        }
        set_error( ioctl->status );
        list_remove( &ioctl->dev_entry );
        release_object( ioctl );  /* no longer on the device queue */
    }
    release_object( device );
}

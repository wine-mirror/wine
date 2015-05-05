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
#include "ddk/wdm.h"

#include "object.h"
#include "file.h"
#include "handle.h"
#include "request.h"
#include "process.h"

struct irp_call
{
    struct object          obj;           /* object header */
    struct list            dev_entry;     /* entry in device queue */
    struct list            mgr_entry;     /* entry in manager queue */
    struct device         *device;        /* device containing this irp */
    struct thread         *thread;        /* thread that queued the irp */
    client_ptr_t           user_arg;      /* user arg used to identify the request */
    struct async          *async;         /* pending async op */
    unsigned int           type;          /* request type (IRP_MJ_*) */
    unsigned int           status;        /* resulting status (or STATUS_PENDING) */
    ioctl_code_t           code;          /* ioctl code */
    data_size_t            result;        /* size of result (input or output depending on the type) */
    data_size_t            in_size;       /* size of input data */
    void                  *in_data;       /* input data */
    data_size_t            out_size;      /* size of output data */
    void                  *out_data;      /* output data */
};

static void irp_call_dump( struct object *obj, int verbose );
static int irp_call_signaled( struct object *obj, struct wait_queue_entry *entry );
static void irp_call_destroy( struct object *obj );

static const struct object_ops irp_call_ops =
{
    sizeof(struct irp_call),          /* size */
    irp_call_dump,                    /* dump */
    no_get_type,                      /* get_type */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    irp_call_signaled,                /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    no_map_access,                    /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_lookup_name,                   /* lookup_name */
    no_open_file,                     /* open_file */
    no_close_handle,                  /* close_handle */
    irp_call_destroy                  /* destroy */
};


struct device_manager
{
    struct object          obj;           /* object header */
    struct list            devices;       /* list of devices */
    struct list            requests;      /* list of pending irps across all devices */
};

static void device_manager_dump( struct object *obj, int verbose );
static int device_manager_signaled( struct object *obj, struct wait_queue_entry *entry );
static void device_manager_destroy( struct object *obj );

static const struct object_ops device_manager_ops =
{
    sizeof(struct device_manager),    /* size */
    device_manager_dump,              /* dump */
    no_get_type,                      /* get_type */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    device_manager_signaled,          /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    no_map_access,                    /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_lookup_name,                   /* lookup_name */
    no_open_file,                     /* open_file */
    no_close_handle,                  /* close_handle */
    device_manager_destroy            /* destroy */
};


struct device
{
    struct object          obj;           /* object header */
    struct device_manager *manager;       /* manager for this device (or NULL if deleted) */
    struct fd             *fd;            /* file descriptor for irp */
    client_ptr_t           user_ptr;      /* opaque ptr for client side */
    struct list            entry;         /* entry in device manager list */
    struct list            requests;      /* list of pending irp requests */
};

static void device_dump( struct object *obj, int verbose );
static struct object_type *device_get_type( struct object *obj );
static struct fd *device_get_fd( struct object *obj );
static void device_destroy( struct object *obj );
static struct object *device_open_file( struct object *obj, unsigned int access,
                                        unsigned int sharing, unsigned int options );
static enum server_fd_type device_get_fd_type( struct fd *fd );
static obj_handle_t device_read( struct fd *fd, const async_data_t *async_data, int blocking,
                                 file_pos_t pos );
static obj_handle_t device_write( struct fd *fd, const async_data_t *async_data, int blocking,
                                  file_pos_t pos, data_size_t *written );
static obj_handle_t device_ioctl( struct fd *fd, ioctl_code_t code, const async_data_t *async_data,
                                  int blocking );

static const struct object_ops device_ops =
{
    sizeof(struct device),            /* size */
    device_dump,                      /* dump */
    device_get_type,                  /* get_type */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    device_get_fd,                    /* get_fd */
    default_fd_map_access,            /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_lookup_name,                   /* lookup_name */
    device_open_file,                 /* open_file */
    no_close_handle,                  /* close_handle */
    device_destroy                    /* destroy */
};

static const struct fd_ops device_fd_ops =
{
    default_fd_get_poll_events,       /* get_poll_events */
    default_poll_event,               /* poll_event */
    device_get_fd_type,               /* get_fd_type */
    device_read,                      /* read */
    device_write,                     /* write */
    no_fd_flush,                      /* flush */
    device_ioctl,                     /* ioctl */
    default_fd_queue_async,           /* queue_async */
    default_fd_reselect_async,        /* reselect_async */
    default_fd_cancel_async           /* cancel_async */
};


static void irp_call_dump( struct object *obj, int verbose )
{
    struct irp_call *irp = (struct irp_call *)obj;
    fprintf( stderr, "IRP call device=%p\n", irp->device );
}

static int irp_call_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct irp_call *irp = (struct irp_call *)obj;

    return !irp->device;  /* device is cleared once the irp has completed */
}

static void irp_call_destroy( struct object *obj )
{
    struct irp_call *irp = (struct irp_call *)obj;

    free( irp->in_data );
    free( irp->out_data );
    if (irp->async)
    {
        async_terminate( irp->async, STATUS_CANCELLED );
        release_object( irp->async );
    }
    if (irp->device) release_object( irp->device );
    release_object( irp->thread );
}

static struct irp_call *create_irp( struct device *device, unsigned int type, const void *in_data,
                                    data_size_t in_size, data_size_t out_size )
{
    struct irp_call *irp;

    if (!device->manager)  /* it has been deleted */
    {
        set_error( STATUS_FILE_DELETED );
        return NULL;
    }

    if ((irp = alloc_object( &irp_call_ops )))
    {
        irp->device   = (struct device *)grab_object( device );
        irp->async    = NULL;
        irp->type     = type;
        irp->code     = 0;
        irp->status   = STATUS_PENDING;
        irp->result   = 0;
        irp->in_size  = in_size;
        irp->in_data  = NULL;
        irp->out_size = out_size;
        irp->out_data = NULL;

        if (irp->in_size && !(irp->in_data = memdup( in_data, in_size )))
        {
            release_object( irp );
            irp = NULL;
        }
    }
    return irp;
}

static void set_irp_result( struct irp_call *irp, unsigned int status,
                            const void *out_data, data_size_t out_size, data_size_t result )
{
    struct device *device = irp->device;

    if (!device) return;  /* already finished */

    /* FIXME: handle the STATUS_PENDING case */
    irp->status = status;
    irp->result = result;
    irp->out_size = min( irp->out_size, out_size );
    if (irp->out_size && !(irp->out_data = memdup( out_data, irp->out_size )))
        irp->out_size = 0;
    release_object( device );
    irp->device = NULL;
    if (irp->async)
    {
        if (result) status = STATUS_ALERTED;
        async_terminate( irp->async, status );
        release_object( irp->async );
        irp->async = NULL;
    }
    wake_up( &irp->obj, 0 );

    if (status != STATUS_ALERTED)
    {
        /* remove it from the device queue */
        /* (for STATUS_ALERTED this will be done in get_irp_result) */
        list_remove( &irp->dev_entry );
        release_object( irp );  /* no longer on the device queue */
    }
}


static void device_dump( struct object *obj, int verbose )
{
    struct device *device = (struct device *)obj;

    fprintf( stderr, "Device " );
    dump_object_name( &device->obj );
    fputc( '\n', stderr );
}

static struct object_type *device_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','e','v','i','c','e'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static struct fd *device_get_fd( struct object *obj )
{
    struct device *device = (struct device *)obj;

    return (struct fd *)grab_object( device->fd );
}

static void device_destroy( struct object *obj )
{
    struct device *device = (struct device *)obj;
    struct irp_call *irp, *next;

    LIST_FOR_EACH_ENTRY_SAFE( irp, next, &device->requests, struct irp_call, dev_entry )
    {
        list_remove( &irp->dev_entry );
        release_object( irp );  /* no longer on the device queue */
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

static struct irp_call *find_irp_call( struct device *device, struct thread *thread,
                                       client_ptr_t user_arg )
{
    struct irp_call *irp;

    LIST_FOR_EACH_ENTRY( irp, &device->requests, struct irp_call, dev_entry )
        if (irp->thread == thread && irp->user_arg == user_arg) return irp;

    set_error( STATUS_INVALID_PARAMETER );
    return NULL;
}

/* queue an irp to the device */
static obj_handle_t queue_irp( struct device *device, struct irp_call *irp,
                               const async_data_t *async_data, int blocking )
{
    obj_handle_t handle = 0;

    if (blocking && !(handle = alloc_handle( current->process, irp, SYNCHRONIZE, 0 ))) return 0;

    if (!(irp->async = fd_queue_async( device->fd, async_data, ASYNC_TYPE_WAIT )))
    {
        if (handle) close_handle( current->process, handle );
        return 0;
    }
    irp->thread   = (struct thread *)grab_object( current );
    irp->user_arg = async_data->arg;
    grab_object( irp );  /* grab reference for queued irp */

    list_add_tail( &device->requests, &irp->dev_entry );
    list_add_tail( &device->manager->requests, &irp->mgr_entry );
    if (list_head( &device->manager->requests ) == &irp->mgr_entry)  /* first one */
        wake_up( &device->manager->obj, 0 );
    set_error( STATUS_PENDING );
    return handle;
}

static obj_handle_t device_read( struct fd *fd, const async_data_t *async_data, int blocking,
                                 file_pos_t pos )
{
    struct device *device = get_fd_user( fd );
    struct irp_call *irp;
    obj_handle_t handle;

    irp = create_irp( device, IRP_MJ_READ, NULL, 0, get_reply_max_size() );
    if (!irp) return 0;

    handle = queue_irp( device, irp, async_data, blocking );
    release_object( irp );
    return handle;
}

static obj_handle_t device_write( struct fd *fd, const async_data_t *async_data, int blocking,
                                  file_pos_t pos, data_size_t *written )
{
    struct device *device = get_fd_user( fd );
    struct irp_call *irp;
    obj_handle_t handle;

    irp = create_irp( device, IRP_MJ_WRITE, get_req_data(), get_req_data_size(), 0 );
    if (!irp) return 0;

    handle = queue_irp( device, irp, async_data, blocking );
    release_object( irp );
    return handle;
}

static obj_handle_t device_ioctl( struct fd *fd, ioctl_code_t code, const async_data_t *async_data,
                                  int blocking )
{
    struct device *device = get_fd_user( fd );
    struct irp_call *irp;
    obj_handle_t handle;

    irp = create_irp( device, IRP_MJ_DEVICE_CONTROL, get_req_data(), get_req_data_size(),
                      get_reply_max_size() );
    if (!irp) return 0;

    irp->code = code;

    handle = queue_irp( device, irp, async_data, blocking );
    release_object( irp );
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
    struct irp_call *irp, *next;

    if (!device->manager) return;  /* already deleted */

    /* terminate all pending requests */
    LIST_FOR_EACH_ENTRY_SAFE( irp, next, &device->requests, struct irp_call, dev_entry )
    {
        list_remove( &irp->mgr_entry );
        set_irp_result( irp, STATUS_FILE_DELETED, NULL, 0, 0 );
    }
    unlink_named_object( &device->obj );
    list_remove( &device->entry );
    device->manager = NULL;
}


static void device_manager_dump( struct object *obj, int verbose )
{
    fprintf( stderr, "Device manager\n" );
}

static int device_manager_signaled( struct object *obj, struct wait_queue_entry *entry )
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


/* retrieve the next pending device irp request */
DECL_HANDLER(get_next_device_request)
{
    struct irp_call *irp;
    struct device_manager *manager;
    struct list *ptr;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if (req->prev)
    {
        if ((irp = (struct irp_call *)get_handle_obj( current->process, req->prev,
                                                          0, &irp_call_ops )))
        {
            set_irp_result( irp, req->status, NULL, 0, 0 );
            close_handle( current->process, req->prev );  /* avoid an extra round-trip for close */
            release_object( irp );
        }
        clear_error();
    }

    if ((ptr = list_head( &manager->requests )))
    {
        irp = LIST_ENTRY( ptr, struct irp_call, mgr_entry );
        reply->type = irp->type;
        reply->code = irp->code;
        reply->user_ptr = irp->device->user_ptr;
        reply->client_pid = get_process_id( irp->thread->process );
        reply->client_tid = get_thread_id( irp->thread );
        reply->in_size = irp->in_size;
        reply->out_size = irp->out_size;
        if (irp->in_size > get_reply_max_size()) set_error( STATUS_BUFFER_OVERFLOW );
        else if ((reply->next = alloc_handle( current->process, irp, 0, 0 )))
        {
            set_reply_data_ptr( irp->in_data, irp->in_size );
            irp->in_data = NULL;
            irp->in_size = 0;
            list_remove( &irp->mgr_entry );
            list_init( &irp->mgr_entry );
        }
    }
    else set_error( STATUS_PENDING );

    release_object( manager );
}


/* store results of an async irp */
DECL_HANDLER(set_irp_result)
{
    struct irp_call *irp;
    struct device_manager *manager;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if ((irp = (struct irp_call *)get_handle_obj( current->process, req->handle, 0, &irp_call_ops )))
    {
        set_irp_result( irp, req->status, get_req_data(), get_req_data_size(), req->size );
        close_handle( current->process, req->handle );  /* avoid an extra round-trip for close */
        release_object( irp );
    }
    release_object( manager );
}


/* retrieve results of an async irp */
DECL_HANDLER(get_irp_result)
{
    struct device *device;
    struct irp_call *irp;

    if (!(device = (struct device *)get_handle_obj( current->process, req->handle, 0, &device_ops )))
        return;

    if ((irp = find_irp_call( device, current, req->user_arg )))
    {
        if (irp->out_data)
        {
            data_size_t size = min( irp->out_size, get_reply_max_size() );
            if (size)
            {
                set_reply_data_ptr( irp->out_data, size );
                irp->out_data = NULL;
            }
        }
        reply->size = irp->result;
        set_error( irp->status );
        list_remove( &irp->dev_entry );
        release_object( irp );  /* no longer on the device queue */
    }
    release_object( device );
}

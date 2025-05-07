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

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/rbtree.h"

#include "object.h"
#include "file.h"
#include "handle.h"
#include "request.h"
#include "process.h"

/* IRP object */

struct irp_call
{
    struct object          obj;           /* object header */
    struct list            dev_entry;     /* entry in device queue */
    struct list            mgr_entry;     /* entry in manager queue */
    struct device_file    *file;          /* file containing this irp */
    struct thread         *thread;        /* thread that queued the irp */
    struct async          *async;         /* pending async op */
    union irp_params       params;        /* irp parameters */
    struct iosb           *iosb;          /* I/O status block */
    int                    canceled;      /* the call was canceled */
    client_ptr_t           user_ptr;      /* client side pointer */
};

static void irp_call_dump( struct object *obj, int verbose );
static void irp_call_destroy( struct object *obj );

static const struct object_ops irp_call_ops =
{
    sizeof(struct irp_call),          /* size */
    &no_type,                         /* type */
    irp_call_dump,                    /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    no_open_file,                     /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    irp_call_destroy                  /* destroy */
};


/* device manager (a list of devices managed by the same client process) */

struct device_manager
{
    struct object          obj;            /* object header */
    struct list            devices;        /* list of devices */
    struct list            requests;       /* list of pending irps across all devices */
    struct irp_call       *current_call;   /* call currently executed on client side */
    struct wine_rb_tree    kernel_objects; /* map of objects that have client side pointer associated */
};

static void device_manager_dump( struct object *obj, int verbose );
static int device_manager_signaled( struct object *obj, struct wait_queue_entry *entry );
static void device_manager_destroy( struct object *obj );

static const struct object_ops device_manager_ops =
{
    sizeof(struct device_manager),    /* size */
    &no_type,                         /* type */
    device_manager_dump,              /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    device_manager_signaled,          /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    no_open_file,                     /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    device_manager_destroy            /* destroy */
};


/* device (a single device object) */

static const WCHAR device_name[] = {'D','e','v','i','c','e'};

struct type_descr device_type =
{
    { device_name, sizeof(device_name) },   /* name */
    FILE_ALL_ACCESS,                        /* valid_access */
    {                                       /* mapping */
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
    },
};

struct device
{
    struct object          obj;           /* object header */
    struct device_manager *manager;       /* manager for this device (or NULL if deleted) */
    char                  *unix_path;     /* path to unix device if any */
    struct list            kernel_object; /* list of kernel object pointers */
    struct list            entry;         /* entry in device manager list */
    struct list            files;         /* list of open files */
};

static void device_dump( struct object *obj, int verbose );
static void device_destroy( struct object *obj );
static struct object *device_open_file( struct object *obj, unsigned int access,
                                        unsigned int sharing, unsigned int options );
static struct list *device_get_kernel_obj_list( struct object *obj );

static const struct object_ops device_ops =
{
    sizeof(struct device),            /* size */
    &device_type,                     /* type */
    device_dump,                      /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    default_get_full_name,            /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    device_open_file,                 /* open_file */
    device_get_kernel_obj_list,       /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    device_destroy                    /* destroy */
};


/* device file (an open file handle to a device) */

struct device_file
{
    struct object          obj;           /* object header */
    struct device         *device;        /* device for this file */
    struct fd             *fd;            /* file descriptor for irp */
    struct list            kernel_object; /* list of kernel object pointers */
    int                    closed;        /* closed file flag */
    struct list            entry;         /* entry in device list */
    struct list            requests;      /* list of pending irp requests */
};

static void device_file_dump( struct object *obj, int verbose );
static struct fd *device_file_get_fd( struct object *obj );
static WCHAR *device_file_get_full_name( struct object *obj, data_size_t max, data_size_t *len );
static struct list *device_file_get_kernel_obj_list( struct object *obj );
static int device_file_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void device_file_destroy( struct object *obj );
static enum server_fd_type device_file_get_fd_type( struct fd *fd );
static void device_file_read( struct fd *fd, struct async *async, file_pos_t pos );
static void device_file_write( struct fd *fd, struct async *async, file_pos_t pos );
static void device_file_flush( struct fd *fd, struct async *async );
static void device_file_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );
static void device_file_cancel_async( struct fd *fd, struct async *async );
static void device_file_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class );

static const struct object_ops device_file_ops =
{
    sizeof(struct device_file),       /* size */
    &file_type,                       /* type */
    device_file_dump,                 /* dump */
    NULL,                             /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    no_signal,                        /* signal */
    device_file_get_fd,               /* get_fd */
    default_fd_get_sync,              /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    device_file_get_full_name,        /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    no_open_file,                     /* open_file */
    device_file_get_kernel_obj_list,  /* get_kernel_obj_list */
    device_file_close_handle,         /* close_handle */
    device_file_destroy               /* destroy */
};

static const struct fd_ops device_file_fd_ops =
{
    default_fd_get_poll_events,       /* get_poll_events */
    default_poll_event,               /* poll_event */
    device_file_get_fd_type,          /* get_fd_type */
    device_file_read,                 /* read */
    device_file_write,                /* write */
    device_file_flush,                /* flush */
    default_fd_get_file_info,         /* get_file_info */
    device_file_get_volume_info,      /* get_volume_info */
    device_file_ioctl,                /* ioctl */
    device_file_cancel_async,         /* cancel_async */
    default_fd_queue_async,           /* queue_async */
    default_fd_reselect_async,        /* reselect_async */
};


struct list *no_kernel_obj_list( struct object *obj )
{
    return NULL;
}

struct kernel_object
{
    struct device_manager  *manager;
    client_ptr_t            user_ptr;
    struct object          *object;
    int                     owned;
    struct list             list_entry;
    struct wine_rb_entry    rb_entry;
};

static int compare_kernel_object( const void *k, const struct wine_rb_entry *entry )
{
    struct kernel_object *ptr = WINE_RB_ENTRY_VALUE( entry, struct kernel_object, rb_entry );
    return memcmp( k, &ptr->user_ptr, sizeof(client_ptr_t) );
}

static struct kernel_object *kernel_object_from_obj( struct device_manager *manager, struct object *obj )
{
    struct kernel_object *kernel_object;
    struct list *list;

    if (!(list = obj->ops->get_kernel_obj_list( obj ))) return NULL;
    LIST_FOR_EACH_ENTRY( kernel_object, list, struct kernel_object, list_entry )
    {
        if (kernel_object->manager != manager) continue;
        return kernel_object;
    }
    return NULL;
}

static client_ptr_t get_kernel_object_ptr( struct device_manager *manager, struct object *obj )
{
    struct kernel_object *kernel_object = kernel_object_from_obj( manager, obj );
    return kernel_object ? kernel_object->user_ptr : 0;
}

static struct kernel_object *set_kernel_object( struct device_manager *manager, struct object *obj, client_ptr_t user_ptr )
{
    struct kernel_object *kernel_object;
    struct list *list;

    if (!(list = obj->ops->get_kernel_obj_list( obj ))) return NULL;

    if (!(kernel_object = malloc( sizeof(*kernel_object) ))) return NULL;
    kernel_object->manager  = manager;
    kernel_object->user_ptr = user_ptr;
    kernel_object->object   = obj;
    kernel_object->owned    = 0;

    if (wine_rb_put( &manager->kernel_objects, &user_ptr, &kernel_object->rb_entry ))
    {
        /* kernel_object pointer already set */
        free( kernel_object );
        return NULL;
    }

    list_add_head( list, &kernel_object->list_entry );
    return kernel_object;
}

static struct kernel_object *kernel_object_from_ptr( struct device_manager *manager, client_ptr_t client_ptr )
{
    struct wine_rb_entry *entry = wine_rb_get( &manager->kernel_objects, &client_ptr );
    return entry ? WINE_RB_ENTRY_VALUE( entry, struct kernel_object, rb_entry ) : NULL;
}

static void grab_kernel_object( struct kernel_object *ptr )
{
    if (!ptr->owned)
    {
        grab_object( ptr->object );
        ptr->owned = 1;
    }
}

static void irp_call_dump( struct object *obj, int verbose )
{
    struct irp_call *irp = (struct irp_call *)obj;
    fprintf( stderr, "IRP call file=%p\n", irp->file );
}

static void irp_call_destroy( struct object *obj )
{
    struct irp_call *irp = (struct irp_call *)obj;

    if (irp->async)
    {
        async_terminate( irp->async, STATUS_CANCELLED );
        release_object( irp->async );
    }
    if (irp->iosb) release_object( irp->iosb );
    if (irp->file) release_object( irp->file );
    if (irp->thread) release_object( irp->thread );
}

static struct irp_call *create_irp( struct device_file *file, const union irp_params *params,
                                    struct async *async )
{
    struct irp_call *irp;

    if (file && !file->device->manager)  /* it has been deleted */
    {
        set_error( STATUS_FILE_DELETED );
        return NULL;
    }

    if ((irp = alloc_object( &irp_call_ops )))
    {
        irp->file     = file ? (struct device_file *)grab_object( file ) : NULL;
        irp->thread   = NULL;
        irp->async    = NULL;
        irp->params   = *params;
        irp->iosb     = NULL;
        irp->canceled = 0;
        irp->user_ptr = 0;

        if (async) irp->iosb = async_get_iosb( async );
    }
    return irp;
}

static void set_irp_result( struct irp_call *irp, unsigned int status,
                            const void *out_data, data_size_t out_size, data_size_t result )
{
    struct device_file *file = irp->file;

    if (!file) return;  /* already finished */

    /* remove it from the device queue */
    list_remove( &irp->dev_entry );
    irp->file = NULL;
    if (irp->async)
    {
        out_size = min( irp->iosb->out_size, out_size );
        async_request_complete_alloc( irp->async, status, result, out_size, out_data );
        release_object( irp->async );
        irp->async = NULL;
    }

    release_object( irp );  /* no longer on the device queue */
    release_object( file );
}


static void device_dump( struct object *obj, int verbose )
{
    fputs( "Device\n", stderr );
}

static void device_destroy( struct object *obj )
{
    struct device *device = (struct device *)obj;

    assert( list_empty( &device->files ));

    free( device->unix_path );
    if (device->manager) list_remove( &device->entry );
}

static void add_irp_to_queue( struct device_manager *manager, struct irp_call *irp, struct thread *thread )
{
    grab_object( irp );  /* grab reference for queued irp */
    irp->thread = thread ? (struct thread *)grab_object( thread ) : NULL;
    if (irp->file) list_add_tail( &irp->file->requests, &irp->dev_entry );
    list_add_tail( &manager->requests, &irp->mgr_entry );
    if (list_head( &manager->requests ) == &irp->mgr_entry) wake_up( &manager->obj, 0 );  /* first one */
}

static struct object *device_open_file( struct object *obj, unsigned int access,
                                        unsigned int sharing, unsigned int options )
{
    struct device *device = (struct device *)obj;
    struct device_file *file;
    struct unicode_str nt_name;
    WCHAR *fullname;

    if (!(file = alloc_object( &device_file_ops ))) return NULL;

    file->device = (struct device *)grab_object( device );
    file->closed = 0;
    list_init( &file->kernel_object );
    list_init( &file->requests );
    list_add_tail( &device->files, &file->entry );
    if (device->unix_path)
    {
        if ((fullname = device->obj.ops->get_full_name( &device->obj, ~0u, &nt_name.len )))
        {
            mode_t mode = 0666;
            access = file->obj.ops->map_access( &file->obj, access );
            nt_name.str = fullname;
            file->fd = open_fd( NULL, device->unix_path, nt_name, O_NONBLOCK, &mode, access, sharing, options );
            if (file->fd) set_fd_user( file->fd, &device_file_fd_ops, &file->obj );
            free( fullname );
        }
        else file->fd = NULL;
    }
    else file->fd = alloc_pseudo_fd( &device_file_fd_ops, &file->obj, options );

    if (!file->fd)
    {
        release_object( file );
        return NULL;
    }

    allow_fd_caching( file->fd );

    if (device->manager)
    {
        struct irp_call *irp;
        union irp_params params;

        memset( &params, 0, sizeof(params) );
        params.create.type    = IRP_CALL_CREATE;
        params.create.access  = access;
        params.create.sharing = sharing;
        params.create.options = options;
        params.create.device  = get_kernel_object_ptr( device->manager, &device->obj );

        if ((irp = create_irp( file, &params, NULL )))
        {
            add_irp_to_queue( device->manager, irp, current );
            release_object( irp );
        }
    }
    return &file->obj;
}

static struct list *device_get_kernel_obj_list( struct object *obj )
{
    struct device *device = (struct device *)obj;
    return &device->kernel_object;
}

static void device_file_dump( struct object *obj, int verbose )
{
    struct device_file *file = (struct device_file *)obj;

    fprintf( stderr, "File on device %p\n", file->device );
}

static struct fd *device_file_get_fd( struct object *obj )
{
    struct device_file *file = (struct device_file *)obj;

    return (struct fd *)grab_object( file->fd );
}

static WCHAR *device_file_get_full_name( struct object *obj, data_size_t max, data_size_t *len )
{
    struct device_file *file = (struct device_file *)obj;
    WCHAR *ret = file->device->obj.ops->get_full_name( &file->device->obj, max, len );
    if (*len > max) set_error( STATUS_BUFFER_OVERFLOW );
    return ret;
}

static struct list *device_file_get_kernel_obj_list( struct object *obj )
{
    struct device_file *file = (struct device_file *)obj;
    return &file->kernel_object;
}

static int device_file_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    struct device_file *file = (struct device_file *)obj;

    if (!file->closed && file->device->manager && obj->handle_count == 1)  /* last handle */
    {
        struct irp_call *irp;
        union irp_params params;

        file->closed = 1;
        memset( &params, 0, sizeof(params) );
        params.close.type = IRP_CALL_CLOSE;

        if ((irp = create_irp( file, &params, NULL )))
        {
            add_irp_to_queue( file->device->manager, irp, current );
            release_object( irp );
        }
    }
    return 1;
}

static void device_file_destroy( struct object *obj )
{
    struct device_file *file = (struct device_file *)obj;
    struct irp_call *irp, *next;

    LIST_FOR_EACH_ENTRY_SAFE( irp, next, &file->requests, struct irp_call, dev_entry )
    {
        list_remove( &irp->dev_entry );
        release_object( irp );  /* no longer on the device queue */
    }
    if (file->fd) release_object( file->fd );
    list_remove( &file->entry );
    release_object( file->device );
}

static int fill_irp_params( struct device_manager *manager, struct irp_call *irp, union irp_params *params )
{
    switch (irp->params.type)
    {
    case IRP_CALL_NONE:
    case IRP_CALL_FREE:
    case IRP_CALL_CANCEL:
        break;
    case IRP_CALL_CREATE:
        irp->params.create.file    = alloc_handle( current->process, irp->file,
                                                   irp->params.create.access, 0 );
        if (!irp->params.create.file) return 0;
        break;
    case IRP_CALL_CLOSE:
        irp->params.close.file     = get_kernel_object_ptr( manager, &irp->file->obj );
        break;
    case IRP_CALL_READ:
        irp->params.read.file      = get_kernel_object_ptr( manager, &irp->file->obj );
        irp->params.read.out_size  = irp->iosb->out_size;
        break;
    case IRP_CALL_WRITE:
        irp->params.write.file     = get_kernel_object_ptr( manager, &irp->file->obj );
        break;
    case IRP_CALL_FLUSH:
        irp->params.flush.file     = get_kernel_object_ptr( manager, &irp->file->obj );
        break;
    case IRP_CALL_IOCTL:
        irp->params.ioctl.file     = get_kernel_object_ptr( manager, &irp->file->obj );
        irp->params.ioctl.out_size = irp->iosb->out_size;
        break;
    case IRP_CALL_VOLUME:
        irp->params.volume.file     = get_kernel_object_ptr( manager, &irp->file->obj );
        irp->params.volume.out_size = irp->iosb->out_size;
        break;
    }

    *params = irp->params;
    return 1;
}

static void free_irp_params( struct irp_call *irp )
{
    switch (irp->params.type)
    {
    case IRP_CALL_CREATE:
        close_handle( current->process, irp->params.create.file );
        break;
    default:
        break;
    }
}

/* queue an irp to the device */
static void queue_irp( struct device_file *file, const union irp_params *params, struct async *async )
{
    struct irp_call *irp = create_irp( file, params, async );
    if (!irp) return;

    fd_queue_async( file->fd, async, ASYNC_TYPE_WAIT );
    irp->async = (struct async *)grab_object( async );
    add_irp_to_queue( file->device->manager, irp, current );
    release_object( irp );
    async_set_unknown_status( async );
}

static enum server_fd_type device_file_get_fd_type( struct fd *fd )
{
    return FD_TYPE_DEVICE;
}

static void device_file_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class )
{
    struct device_file *file = get_fd_user( fd );
    union irp_params params;

    memset( &params, 0, sizeof(params) );
    params.volume.type = IRP_CALL_VOLUME;
    params.volume.info_class = info_class;
    queue_irp( file, &params, async );
}

static void device_file_read( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct device_file *file = get_fd_user( fd );
    union irp_params params;

    memset( &params, 0, sizeof(params) );
    params.read.type = IRP_CALL_READ;
    params.read.key  = 0;
    params.read.pos  = pos;
    queue_irp( file, &params, async );
}

static void device_file_write( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct device_file *file = get_fd_user( fd );
    union irp_params params;

    memset( &params, 0, sizeof(params) );
    params.write.type = IRP_CALL_WRITE;
    params.write.key  = 0;
    params.write.pos  = pos;
    queue_irp( file, &params, async );
}

static void device_file_flush( struct fd *fd, struct async *async )
{
    struct device_file *file = get_fd_user( fd );
    union irp_params params;

    memset( &params, 0, sizeof(params) );
    params.flush.type = IRP_CALL_FLUSH;
    queue_irp( file, &params, async );
}

static void device_file_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct device_file *file = get_fd_user( fd );
    union irp_params params;

    memset( &params, 0, sizeof(params) );
    params.ioctl.type = IRP_CALL_IOCTL;
    params.ioctl.code = code;
    queue_irp( file, &params, async );
}

static void cancel_irp_call( struct irp_call *irp )
{
    struct irp_call *cancel_irp;
    union irp_params params;

    irp->canceled = 1;
    if (!irp->user_ptr || !irp->file || !irp->file->device->manager) return;

    memset( &params, 0, sizeof(params) );
    params.cancel.type = IRP_CALL_CANCEL;
    params.cancel.irp  = irp->user_ptr;

    if ((cancel_irp = create_irp( NULL, &params, NULL )))
    {
        add_irp_to_queue( irp->file->device->manager, cancel_irp, NULL );
        release_object( cancel_irp );
    }
}

static void device_file_cancel_async( struct fd *fd, struct async *async )
{
    struct device_file *file = get_fd_user( fd );
    struct irp_call *irp;

    LIST_FOR_EACH_ENTRY( irp, &file->requests, struct irp_call, dev_entry )
    {
        if (irp->async == async)
        {
            cancel_irp_call( irp );
            return;
        }
    }
}

static struct device *create_device( struct object *root, const struct unicode_str *name,
                                     struct device_manager *manager )
{
    struct device *device;

    if ((device = create_named_object( root, &device_ops, name, 0, NULL )))
    {
        device->unix_path = NULL;
        device->manager = manager;
        grab_object( device );
        list_add_tail( &manager->devices, &device->entry );
        list_init( &device->kernel_object );
        list_init( &device->files );
    }
    return device;
}

struct object *create_unix_device( struct object *root, const struct unicode_str *name,
                                   unsigned int attr, const struct security_descriptor *sd,
                                   const char *unix_path )
{
    struct device *device;

    if ((device = create_named_object( root, &device_ops, name, attr, sd )))
    {
        device->unix_path = strdup( unix_path );
        device->manager = NULL;  /* no manager, requests go straight to the Unix device */
        list_init( &device->kernel_object );
        list_init( &device->files );
    }
    return &device->obj;

}

/* terminate requests when the underlying device is deleted */
static void delete_file( struct device_file *file )
{
    struct irp_call *irp, *next;

    /* the pending requests may be the only thing holding a reference to the file */
    grab_object( file );

    /* terminate all pending requests */
    LIST_FOR_EACH_ENTRY_SAFE( irp, next, &file->requests, struct irp_call, dev_entry )
    {
        list_remove( &irp->mgr_entry );
        set_irp_result( irp, STATUS_FILE_DELETED, NULL, 0, 0 );
    }

    release_object( file );
}

static void delete_device( struct device *device )
{
    struct device_file *file, *next;

    if (!device->manager) return;  /* already deleted */

    LIST_FOR_EACH_ENTRY_SAFE( file, next, &device->files, struct device_file, entry )
        delete_file( file );

    unlink_named_object( &device->obj );
    list_remove( &device->entry );
    device->manager = NULL;
    release_object( device );
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
    struct kernel_object *kernel_object;
    struct list *ptr;

    if (manager->current_call)
    {
        release_object( manager->current_call );
        manager->current_call = NULL;
    }

    while (manager->kernel_objects.root)
    {
        kernel_object = WINE_RB_ENTRY_VALUE( manager->kernel_objects.root, struct kernel_object, rb_entry );
        wine_rb_remove( &manager->kernel_objects, &kernel_object->rb_entry );
        list_remove( &kernel_object->list_entry );
        if (kernel_object->owned) release_object( kernel_object->object );
        free( kernel_object );
    }

    while ((ptr = list_head( &manager->devices )))
    {
        struct device *device = LIST_ENTRY( ptr, struct device, entry );
        delete_device( device );
    }

    while ((ptr = list_head( &manager->requests )))
    {
        struct irp_call *irp = LIST_ENTRY( ptr, struct irp_call, mgr_entry );
        list_remove( &irp->mgr_entry );
        assert( !irp->file && !irp->async );
        release_object( irp );
    }
}

static struct device_manager *create_device_manager(void)
{
    struct device_manager *manager;

    if ((manager = alloc_object( &device_manager_ops )))
    {
        manager->current_call = NULL;
        list_init( &manager->devices );
        list_init( &manager->requests );
        wine_rb_init( &manager->kernel_objects, compare_kernel_object );
    }
    return manager;
}

void free_kernel_objects( struct object *obj )
{
    struct list *ptr, *list;

    if (!(list = obj->ops->get_kernel_obj_list( obj ))) return;

    while ((ptr = list_head( list )))
    {
        struct kernel_object *kernel_object = LIST_ENTRY( ptr, struct kernel_object, list_entry );
        struct irp_call *irp;
        union irp_params params;

        assert( !kernel_object->owned );

        memset( &params, 0, sizeof(params) );
        params.free.type = IRP_CALL_FREE;
        params.free.obj  = kernel_object->user_ptr;

        if ((irp = create_irp( NULL, &params, NULL )))
        {
            add_irp_to_queue( kernel_object->manager, irp, NULL );
            release_object( irp );
        }

        list_remove( &kernel_object->list_entry );
        wine_rb_remove( &kernel_object->manager->kernel_objects, &kernel_object->rb_entry );
        free( kernel_object );
    }
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
    struct unicode_str name = get_req_unicode_str();
    struct device_manager *manager;
    struct object *root = NULL;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir )))
    {
        release_object( manager );
        return;
    }

    if ((device = create_device( root, &name, manager )))
    {
        struct kernel_object *ptr = set_kernel_object( manager, &device->obj, req->user_ptr );
        if (ptr)
            grab_kernel_object( ptr );
        else
            set_error( STATUS_NO_MEMORY );
        release_object( device );
    }

    if (root) release_object( root );
    release_object( manager );
}


/* delete a device */
DECL_HANDLER(delete_device)
{
    struct device_manager *manager;
    struct kernel_object *ref;
    struct device *device;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if ((ref = kernel_object_from_ptr( manager, req->device )) && ref->object->ops == &device_ops)
    {
        device = (struct device *)grab_object( ref->object );
        delete_device( device );
        release_object( device );
    }
    else set_error( STATUS_INVALID_HANDLE );

    release_object( manager );
}


/* retrieve the next pending device irp request */
DECL_HANDLER(get_next_device_request)
{
    struct irp_call *irp;
    struct device_manager *manager;
    struct list *ptr;
    struct iosb *iosb;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    /* process result of previous call */
    if (manager->current_call)
    {
        irp = manager->current_call;
        irp->user_ptr = req->user_ptr;

        if (irp->async)
        {
            if (req->pending)
                set_async_pending( irp->async );
            async_set_initial_status( irp->async, req->status );

            if (req->prev)
            {
                set_irp_result( irp, req->iosb_status, get_req_data(), get_req_data_size(), req->result );
            }
            else
            {
                async_wake_obj( irp->async );
                if (irp->canceled)
                {
                    /* if it was canceled during dispatch, we couldn't queue cancel
                     * call without client pointer, so we need to do it now */
                    cancel_irp_call( irp );
                }
            }
        }
        else
        {
            set_irp_result( irp, req->status, NULL, 0, 0 );
        }

        if (req->prev)
            close_handle( current->process, req->prev );  /* avoid an extra round-trip for close */

        free_irp_params( irp );
        release_object( irp );
        manager->current_call = NULL;
    }

    clear_error();

    if ((ptr = list_head( &manager->requests )))
    {
        struct thread *thread;

        irp = LIST_ENTRY( ptr, struct irp_call, mgr_entry );

        thread = irp->thread ? irp->thread : current;
        reply->client_thread = get_kernel_object_ptr( manager, &thread->obj );
        reply->client_tid    = get_thread_id( thread );

        iosb = irp->iosb;
        if (iosb)
            reply->in_size = iosb->in_size;

        if (iosb && iosb->in_size > get_reply_max_size())
            set_error( STATUS_BUFFER_OVERFLOW );
        else if (!irp->file || (reply->next = alloc_handle_no_access_check( current->process, irp, 0, 0 )))
        {
            if (fill_irp_params( manager, irp, &reply->params ))
            {
                if (iosb)
                {
                    set_reply_data_ptr( iosb->in_data, iosb->in_size );
                    iosb->in_data = NULL;
                    iosb->in_size = 0;
                }
                list_remove( &irp->mgr_entry );
                list_init( &irp->mgr_entry );
                /* we already own the object if it's only on manager queue */
                if (irp->file) grab_object( irp );
                manager->current_call = irp;
            }
            else close_handle( current->process, reply->next );
        }
    }
    else set_error( STATUS_PENDING );

    release_object( manager );
}


/* store results of an async irp */
DECL_HANDLER(set_irp_result)
{
    struct irp_call *irp;

    if ((irp = (struct irp_call *)get_handle_obj( current->process, req->handle, 0, &irp_call_ops )))
    {
        set_irp_result( irp, req->status, get_req_data(), get_req_data_size(), req->size );
        close_handle( current->process, req->handle );  /* avoid an extra round-trip for close */
        release_object( irp );
    }
}


/* get kernel pointer from server object */
DECL_HANDLER(get_kernel_object_ptr)
{
    struct device_manager *manager;
    struct object *object = NULL;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if ((object = get_handle_obj( current->process, req->handle, 0, NULL )))
    {
        reply->user_ptr = get_kernel_object_ptr( manager, object );
        release_object( object );
    }

    release_object( manager );
}


/* associate kernel pointer with server object */
DECL_HANDLER(set_kernel_object_ptr)
{
    struct device_manager *manager;
    struct object *object = NULL;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if (!(object = get_handle_obj( current->process, req->handle, 0, NULL )))
    {
        release_object( manager );
        return;
    }

    if (!set_kernel_object( manager, object, req->user_ptr ))
        set_error( STATUS_INVALID_HANDLE );

    release_object( object );
    release_object( manager );
}


/* grab server object reference from kernel object pointer */
DECL_HANDLER(grab_kernel_object)
{
    struct device_manager *manager;
    struct kernel_object *ref;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if ((ref = kernel_object_from_ptr( manager, req->user_ptr )) && !ref->owned)
        grab_kernel_object( ref );
    else
        set_error( STATUS_INVALID_HANDLE );

    release_object( manager );
}


/* release server object reference from kernel object pointer */
DECL_HANDLER(release_kernel_object)
{
    struct device_manager *manager;
    struct kernel_object *ref;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if ((ref = kernel_object_from_ptr( manager, req->user_ptr )) && ref->owned)
    {
        ref->owned = 0;
        release_object( ref->object );
    }
    else set_error( STATUS_INVALID_HANDLE );

    release_object( manager );
}


/* get handle from kernel object pointer */
DECL_HANDLER(get_kernel_object_handle)
{
    struct device_manager *manager;
    struct kernel_object *ref;

    if (!(manager = (struct device_manager *)get_handle_obj( current->process, req->manager,
                                                             0, &device_manager_ops )))
        return;

    if ((ref = kernel_object_from_ptr( manager, req->user_ptr )))
        reply->handle = alloc_handle_no_access_check( current->process, ref->object, req->access, 0 );
    else
        set_error( STATUS_INVALID_HANDLE );

    release_object( manager );
}

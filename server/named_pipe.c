/*
 * Server-side pipe management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2001 Mike McCormack
 * Copyright 2016 Jacek Caban for CodeWeavers
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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "security.h"
#include "process.h"

struct named_pipe;

struct pipe_message
{
    struct list          entry;      /* entry in message queue */
    data_size_t          read_pos;   /* already read bytes */
    struct iosb         *iosb;       /* message iosb */
    struct async        *async;      /* async of pending write */
};

struct pipe_end
{
    struct object        obj;        /* object header */
    struct fd           *fd;         /* pipe file descriptor */
    unsigned int         flags;      /* pipe flags */
    unsigned int         state;      /* pipe state */
    struct named_pipe   *pipe;
    struct pipe_end     *connection; /* the other end of the pipe */
    process_id_t         client_pid; /* process that created the client */
    process_id_t         server_pid; /* process that created the server */
    data_size_t          buffer_size;/* size of buffered data that doesn't block caller */
    struct list          message_queue;
    struct async_queue   read_q;     /* read queue */
    struct async_queue   write_q;    /* write queue */
};

struct pipe_server
{
    struct pipe_end      pipe_end;   /* common header for both pipe ends */
    struct list          entry;      /* entry in named pipe listeners list */
    unsigned int         options;    /* pipe options */
    struct async_queue   listen_q;   /* listen queue */
};

struct named_pipe
{
    struct object       obj;         /* object header */
    int                 message_mode;
    unsigned int        sharing;
    unsigned int        maxinstances;
    unsigned int        outsize;
    unsigned int        insize;
    unsigned int        instances;
    timeout_t           timeout;
    struct list         listeners;   /* list of servers listening on this pipe */
    struct async_queue  waiters;     /* list of clients waiting to connect */
};

struct named_pipe_device
{
    struct object       obj;         /* object header */
    struct namespace   *pipes;       /* named pipe namespace */
};

struct named_pipe_device_file
{
    struct object             obj;         /* object header */
    struct fd                *fd;          /* pseudo-fd for ioctls */
    struct named_pipe_device *device;      /* named pipe device */
};

static void named_pipe_dump( struct object *obj, int verbose );
static unsigned int named_pipe_map_access( struct object *obj, unsigned int access );
static WCHAR *named_pipe_get_full_name( struct object *obj, data_size_t *ret_len );
static int named_pipe_link_name( struct object *obj, struct object_name *name, struct object *parent );
static struct object *named_pipe_open_file( struct object *obj, unsigned int access,
                                            unsigned int sharing, unsigned int options );
static void named_pipe_destroy( struct object *obj );

static const struct object_ops named_pipe_ops =
{
    sizeof(struct named_pipe),    /* size */
    &no_type,                     /* type */
    named_pipe_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    named_pipe_map_access,        /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    named_pipe_get_full_name,     /* get_full_name */
    no_lookup_name,               /* lookup_name */
    named_pipe_link_name,         /* link_name */
    default_unlink_name,          /* unlink_name */
    named_pipe_open_file,         /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    no_close_handle,              /* close_handle */
    named_pipe_destroy            /* destroy */
};

/* common server and client pipe end functions */
static void pipe_end_destroy( struct object *obj );
static enum server_fd_type pipe_end_get_fd_type( struct fd *fd );
static struct fd *pipe_end_get_fd( struct object *obj );
static struct security_descriptor *pipe_end_get_sd( struct object *obj );
static int pipe_end_set_sd( struct object *obj, const struct security_descriptor *sd,
                            unsigned int set_info );
static WCHAR *pipe_end_get_full_name( struct object *obj, data_size_t *len );
static void pipe_end_read( struct fd *fd, struct async *async, file_pos_t pos );
static void pipe_end_write( struct fd *fd, struct async *async_data, file_pos_t pos );
static void pipe_end_flush( struct fd *fd, struct async *async );
static void pipe_end_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class );
static void pipe_end_reselect_async( struct fd *fd, struct async_queue *queue );
static void pipe_end_get_file_info( struct fd *fd, obj_handle_t handle, unsigned int info_class );

/* server end functions */
static void pipe_server_dump( struct object *obj, int verbose );
static struct object *pipe_server_lookup_name( struct object *obj, struct unicode_str *name,
                                               unsigned int attr, struct object *root );
static struct object *pipe_server_open_file( struct object *obj, unsigned int access,
                                             unsigned int sharing, unsigned int options );
static void pipe_server_destroy( struct object *obj);
static void pipe_server_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct object_ops pipe_server_ops =
{
    sizeof(struct pipe_server),   /* size */
    &file_type,                   /* type */
    pipe_server_dump,             /* dump */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    no_signal,                    /* signal */
    pipe_end_get_fd,              /* get_fd */
    default_map_access,           /* map_access */
    pipe_end_get_sd,              /* get_sd */
    pipe_end_set_sd,              /* set_sd */
    pipe_end_get_full_name,       /* get_full_name */
    pipe_server_lookup_name,      /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    pipe_server_open_file,        /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    async_close_obj_handle,       /* close_handle */
    pipe_server_destroy           /* destroy */
};

static const struct fd_ops pipe_server_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    pipe_end_get_fd_type,         /* get_fd_type */
    pipe_end_read,                /* read */
    pipe_end_write,               /* write */
    pipe_end_flush,               /* flush */
    pipe_end_get_file_info,       /* get_file_info */
    pipe_end_get_volume_info,     /* get_volume_info */
    pipe_server_ioctl,            /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    no_fd_queue_async,            /* queue_async */
    pipe_end_reselect_async       /* reselect_async */
};

/* client end functions */
static void pipe_client_dump( struct object *obj, int verbose );
static void pipe_client_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct object_ops pipe_client_ops =
{
    sizeof(struct pipe_end),      /* size */
    &file_type,                   /* type */
    pipe_client_dump,             /* dump */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    no_signal,                    /* signal */
    pipe_end_get_fd,              /* get_fd */
    default_map_access,           /* map_access */
    pipe_end_get_sd,              /* get_sd */
    pipe_end_set_sd,              /* set_sd */
    pipe_end_get_full_name,       /* get_full_name */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    async_close_obj_handle,       /* close_handle */
    pipe_end_destroy              /* destroy */
};

static const struct fd_ops pipe_client_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    pipe_end_get_fd_type,         /* get_fd_type */
    pipe_end_read,                /* read */
    pipe_end_write,               /* write */
    pipe_end_flush,               /* flush */
    pipe_end_get_file_info,       /* get_file_info */
    pipe_end_get_volume_info,     /* get_volume_info */
    pipe_client_ioctl,            /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    no_fd_queue_async,            /* queue_async */
    pipe_end_reselect_async       /* reselect_async */
};

static void named_pipe_device_dump( struct object *obj, int verbose );
static struct object *named_pipe_device_lookup_name( struct object *obj,
    struct unicode_str *name, unsigned int attr, struct object *root );
static struct object *named_pipe_device_open_file( struct object *obj, unsigned int access,
                                                   unsigned int sharing, unsigned int options );
static void named_pipe_device_destroy( struct object *obj );

static const struct object_ops named_pipe_device_ops =
{
    sizeof(struct named_pipe_device), /* size */
    &device_type,                     /* type */
    named_pipe_device_dump,           /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    default_get_full_name,            /* get_full_name */
    named_pipe_device_lookup_name,    /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    named_pipe_device_open_file,      /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    named_pipe_device_destroy         /* destroy */
};

static void named_pipe_device_file_dump( struct object *obj, int verbose );
static struct fd *named_pipe_device_file_get_fd( struct object *obj );
static WCHAR *named_pipe_device_file_get_full_name( struct object *obj, data_size_t *len );
static void named_pipe_device_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );
static enum server_fd_type named_pipe_device_file_get_fd_type( struct fd *fd );
static void named_pipe_device_file_destroy( struct object *obj );

static const struct object_ops named_pipe_device_file_ops =
{
    sizeof(struct named_pipe_device_file),   /* size */
    &file_type,                              /* type */
    named_pipe_device_file_dump,             /* dump */
    add_queue,                               /* add_queue */
    remove_queue,                            /* remove_queue */
    default_fd_signaled,                     /* signaled */
    no_satisfied,                            /* satisfied */
    no_signal,                               /* signal */
    named_pipe_device_file_get_fd,           /* get_fd */
    default_map_access,                      /* map_access */
    default_get_sd,                          /* get_sd */
    default_set_sd,                          /* set_sd */
    named_pipe_device_file_get_full_name,    /* get_full_name */
    no_lookup_name,                          /* lookup_name */
    no_link_name,                            /* link_name */
    NULL,                                    /* unlink_name */
    no_open_file,                            /* open_file */
    no_kernel_obj_list,                      /* get_kernel_obj_list */
    no_close_handle,                         /* close_handle */
    named_pipe_device_file_destroy           /* destroy */
};

static const struct fd_ops named_pipe_device_fd_ops =
{
    default_fd_get_poll_events,              /* get_poll_events */
    default_poll_event,                      /* poll_event */
    named_pipe_device_file_get_fd_type,      /* get_fd_type */
    no_fd_read,                              /* read */
    no_fd_write,                             /* write */
    no_fd_flush,                             /* flush */
    default_fd_get_file_info,                /* get_file_info */
    no_fd_get_volume_info,                   /* get_volume_info */
    named_pipe_device_ioctl,                 /* ioctl */
    default_fd_cancel_async,                 /* cancel_async */
    default_fd_queue_async,                  /* queue_async */
    default_fd_reselect_async                /* reselect_async */
};

static void named_pipe_dir_dump( struct object *obj, int verbose );
static struct fd *named_pipe_dir_get_fd( struct object *obj );
static WCHAR *named_pipe_dir_get_full_name( struct object *obj, data_size_t *ret_len );
static void named_pipe_dir_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );
static struct object *named_pipe_dir_lookup_name( struct object *obj, struct unicode_str *name,
                                                  unsigned int attr, struct object *root );
static struct object *named_pipe_dir_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options );
static void named_pipe_dir_destroy( struct object *obj );

static const struct object_ops named_pipe_dir_ops =
{
    sizeof(struct named_pipe_device_file),   /* size */
    &file_type,                              /* type */
    named_pipe_dir_dump,                     /* dump */
    add_queue,                               /* add_queue */
    remove_queue,                            /* remove_queue */
    default_fd_signaled,                     /* signaled */
    no_satisfied,                            /* satisfied */
    no_signal,                               /* signal */
    named_pipe_dir_get_fd,                   /* get_fd */
    default_map_access,                      /* map_access */
    default_get_sd,                          /* get_sd */
    default_set_sd,                          /* set_sd */
    named_pipe_dir_get_full_name,            /* get_full_name */
    named_pipe_dir_lookup_name,              /* lookup_name */
    no_link_name,                            /* link_name */
    NULL,                                    /* unlink_name */
    named_pipe_dir_open_file,                /* open_file */
    no_kernel_obj_list,                      /* get_kernel_obj_list */
    no_close_handle,                         /* close_handle */
    named_pipe_dir_destroy                   /* destroy */
};

static const struct fd_ops named_pipe_dir_fd_ops =
{
    default_fd_get_poll_events,              /* get_poll_events */
    default_poll_event,                      /* poll_event */
    NULL,                                    /* get_fd_type */
    no_fd_read,                              /* read */
    no_fd_write,                             /* write */
    no_fd_flush,                             /* flush */
    default_fd_get_file_info,                /* get_file_info */
    no_fd_get_volume_info,                   /* get_volume_info */
    named_pipe_dir_ioctl,                    /* ioctl */
    default_fd_cancel_async,                 /* cancel_async */
    default_fd_queue_async,                  /* queue_async */
    default_fd_reselect_async                /* reselect_async */
};

static void named_pipe_dump( struct object *obj, int verbose )
{
    fputs( "Named pipe\n", stderr );
}

static unsigned int named_pipe_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | FILE_CREATE_PIPE_INSTANCE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static WCHAR *named_pipe_get_full_name( struct object *obj, data_size_t *ret_len )
{
    WCHAR *ret;

    if (!(ret = default_get_full_name( obj, ret_len )))
        set_error( STATUS_OBJECT_PATH_INVALID );
    return ret;
}

static void pipe_server_dump( struct object *obj, int verbose )
{
    struct pipe_server *server = (struct pipe_server *) obj;
    assert( obj->ops == &pipe_server_ops );
    fprintf( stderr, "Named pipe server pipe=%p state=%d\n", server->pipe_end.pipe,
             server->pipe_end.state );
}

static void pipe_client_dump( struct object *obj, int verbose )
{
    struct pipe_end *client = (struct pipe_end *) obj;
    assert( obj->ops == &pipe_client_ops );
    fprintf( stderr, "Named pipe client server=%p\n", client->connection );
}

static void named_pipe_destroy( struct object *obj)
{
    struct named_pipe *pipe = (struct named_pipe *) obj;

    assert( list_empty( &pipe->listeners ) );
    assert( !pipe->instances );
    free_async_queue( &pipe->waiters );
}

static struct fd *pipe_end_get_fd( struct object *obj )
{
    struct pipe_end *pipe_end = (struct pipe_end *) obj;
    return (struct fd *) grab_object( pipe_end->fd );
}

static struct pipe_message *queue_message( struct pipe_end *pipe_end, struct iosb *iosb )
{
    struct pipe_message *message;

    if (!(message = mem_alloc( sizeof(*message) ))) return NULL;
    message->iosb = (struct iosb *)grab_object( iosb );
    message->async = NULL;
    message->read_pos = 0;
    list_add_tail( &pipe_end->message_queue, &message->entry );
    return message;
}

static void wake_message( struct pipe_message *message, data_size_t result )
{
    struct async *async = message->async;

    message->async = NULL;
    if (!async) return;

    async_request_complete( async, STATUS_SUCCESS, result, 0, NULL );
    release_object( async );
}

static void free_message( struct pipe_message *message )
{
    list_remove( &message->entry );
    if (message->iosb) release_object( message->iosb );
    free( message );
}

static void pipe_end_disconnect( struct pipe_end *pipe_end, unsigned int status )
{
    struct pipe_end *connection = pipe_end->connection;
    struct pipe_message *message, *next;
    struct async *async;

    pipe_end->connection = NULL;

    pipe_end->state = status == STATUS_PIPE_DISCONNECTED
        ? FILE_PIPE_DISCONNECTED_STATE : FILE_PIPE_CLOSING_STATE;
    fd_async_wake_up( pipe_end->fd, ASYNC_TYPE_WAIT, status );
    async_wake_up( &pipe_end->read_q, status );
    LIST_FOR_EACH_ENTRY_SAFE( message, next, &pipe_end->message_queue, struct pipe_message, entry )
    {
        async = message->async;
        if (async || status == STATUS_PIPE_DISCONNECTED) free_message( message );
        if (!async) continue;
        async_terminate( async, status );
        release_object( async );
    }
    if (status == STATUS_PIPE_DISCONNECTED) set_fd_signaled( pipe_end->fd, 0 );

    if (connection)
    {
        connection->connection = NULL;
        pipe_end_disconnect( connection, status );
    }
}

static void pipe_end_destroy( struct object *obj )
{
    struct pipe_end *pipe_end = (struct pipe_end *)obj;
    struct pipe_message *message;

    pipe_end_disconnect( pipe_end, STATUS_PIPE_BROKEN );

    while (!list_empty( &pipe_end->message_queue ))
    {
        message = LIST_ENTRY( list_head(&pipe_end->message_queue), struct pipe_message, entry );
        assert( !message->async );
        free_message( message );
    }

    free_async_queue( &pipe_end->read_q );
    free_async_queue( &pipe_end->write_q );
    if (pipe_end->fd) release_object( pipe_end->fd );
    if (pipe_end->pipe) release_object( pipe_end->pipe );
}

static struct object *pipe_server_lookup_name( struct object *obj, struct unicode_str *name,
                                               unsigned int attr, struct object *root )
{
    if (name && name->len)
        set_error( STATUS_OBJECT_NAME_INVALID );

    return NULL;
}

static struct object *pipe_server_open_file( struct object *obj, unsigned int access,
                                             unsigned int sharing, unsigned int options )
{
    struct pipe_server *server = (struct pipe_server *)obj;

    return server->pipe_end.pipe->obj.ops->open_file( &server->pipe_end.pipe->obj, access, sharing, options );
}

static void pipe_server_destroy( struct object *obj )
{
    struct pipe_server *server = (struct pipe_server *)obj;
    struct named_pipe *pipe = server->pipe_end.pipe;

    assert( obj->ops == &pipe_server_ops );

    assert( pipe->instances );
    if (!--pipe->instances) unlink_named_object( &pipe->obj );
    if (server->pipe_end.state == FILE_PIPE_LISTENING_STATE)
        list_remove( &server->entry );

    free_async_queue( &server->listen_q );
    pipe_end_destroy( obj );
}

static void named_pipe_device_dump( struct object *obj, int verbose )
{
    fputs( "Named pipe device\n", stderr );
}

static struct object *named_pipe_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                     unsigned int attr, struct object *root )
{
    struct named_pipe_device *device = (struct named_pipe_device*)obj;
    struct object *found;

    assert( obj->ops == &named_pipe_device_ops );
    assert( device->pipes );

    if (!name) return NULL;  /* open the device itself */
    if (!name->len && name->str)
    {
        /* open the root directory */
        struct named_pipe_device_file *dir = alloc_object( &named_pipe_dir_ops );

        if (!dir) return NULL;

        dir->fd = NULL;  /* defer alloc_pseudo_fd() until after we have options */
        dir->device = (struct named_pipe_device *)grab_object( obj );

        return &dir->obj;
    }

    if ((found = find_object( device->pipes, name, attr | OBJ_CASE_INSENSITIVE )))
        name->len = 0;

    return found;
}

static struct object *named_pipe_device_open_file( struct object *obj, unsigned int access,
                                                   unsigned int sharing, unsigned int options )
{
    struct named_pipe_device_file *file;

    if (!(file = alloc_object( &named_pipe_device_file_ops ))) return NULL;
    file->device = (struct named_pipe_device *)grab_object( obj );
    if (!(file->fd = alloc_pseudo_fd( &named_pipe_device_fd_ops, obj, options )))
    {
        release_object( file );
        return NULL;
    }
    allow_fd_caching( file->fd );
    return &file->obj;
}

static void named_pipe_device_destroy( struct object *obj )
{
    struct named_pipe_device *device = (struct named_pipe_device*)obj;
    assert( obj->ops == &named_pipe_device_ops );
    free( device->pipes );
}

struct object *create_named_pipe_device( struct object *root, const struct unicode_str *name,
                                         unsigned int attr, const struct security_descriptor *sd )
{
    struct named_pipe_device *dev;

    if ((dev = create_named_object( root, &named_pipe_device_ops, name, attr, sd )) &&
        get_error() != STATUS_OBJECT_NAME_EXISTS)
    {
        dev->pipes = NULL;
        if (!(dev->pipes = create_namespace( 7 )))
        {
            release_object( dev );
            dev = NULL;
        }
    }
    return &dev->obj;
}

static void named_pipe_device_file_dump( struct object *obj, int verbose )
{
    struct named_pipe_device_file *file = (struct named_pipe_device_file *)obj;

    fprintf( stderr, "File on named pipe device %p\n", file->device );
}

static struct fd *named_pipe_device_file_get_fd( struct object *obj )
{
    struct named_pipe_device_file *file = (struct named_pipe_device_file *)obj;
    return (struct fd *)grab_object( file->fd );
}

static WCHAR *named_pipe_device_file_get_full_name( struct object *obj, data_size_t *len )
{
    struct named_pipe_device_file *file = (struct named_pipe_device_file *)obj;
    return file->device->obj.ops->get_full_name( &file->device->obj, len );
}

static enum server_fd_type named_pipe_device_file_get_fd_type( struct fd *fd )
{
    return FD_TYPE_DEVICE;
}

static void named_pipe_device_file_destroy( struct object *obj )
{
    struct named_pipe_device_file *file = (struct named_pipe_device_file*)obj;
    assert( obj->ops == &named_pipe_device_file_ops );
    if (file->fd) release_object( file->fd );
    release_object( file->device );
}

static void named_pipe_dir_dump( struct object *obj, int verbose )
{
    struct named_pipe_device_file *dir = (struct named_pipe_device_file *)obj;

    fprintf( stderr, "Root directory of named pipe device %p\n", dir->device );
}

static struct fd *named_pipe_dir_get_fd( struct object *obj )
{
    struct named_pipe_device_file *dir = (struct named_pipe_device_file *)obj;
    return (struct fd *)grab_object( dir->fd );
}

static WCHAR *named_pipe_dir_get_full_name( struct object *obj, data_size_t *ret_len )
{
    struct named_pipe_device_file *dir = (struct named_pipe_device_file *)obj;
    data_size_t len;
    char *device_name, *ret;

    device_name = (char *)dir->device->obj.ops->get_full_name( &dir->device->obj, &len );
    if (!device_name) return NULL;

    len += sizeof(WCHAR);
    ret = realloc(device_name, len);
    if (!ret)
    {
        free(device_name);
        return NULL;
    }
    *(WCHAR *)(ret + len - sizeof(WCHAR)) = '\\';

    *ret_len = len;
    return (WCHAR *)ret;
}

static struct object *named_pipe_dir_lookup_name( struct object *obj, struct unicode_str *name,
                                                  unsigned int attr, struct object *root )
{
    struct named_pipe_device_file *dir = (struct named_pipe_device_file *)obj;
    if (!name || !name->len) return NULL;  /* open the directory itself */
    return dir->device->obj.ops->lookup_name( &dir->device->obj, name, attr, root );
}

static struct object *named_pipe_dir_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options )
{
    struct named_pipe_device_file *dir = (struct named_pipe_device_file *)obj;

    if (dir->fd)
    {
        /* Trying to open by (already opened) file object */
        return no_open_file( obj, access, sharing, options );
    }

    /* Turn this "proto-object" into an actual file object */

    if (!(dir->fd = alloc_pseudo_fd( &named_pipe_dir_fd_ops, obj, options )))
        return NULL;

    allow_fd_caching( dir->fd );
    return grab_object( obj );
}

static void named_pipe_dir_destroy( struct object *obj )
{
    struct named_pipe_device_file *file = (struct named_pipe_device_file *)obj;
    assert( obj->ops == &named_pipe_dir_ops );
    if (file->fd) release_object( file->fd );
    release_object( file->device );
}

static void pipe_end_flush( struct fd *fd, struct async *async )
{
    struct pipe_end *pipe_end = get_fd_user( fd );

    if (!pipe_end->pipe)
    {
        set_error( STATUS_PIPE_DISCONNECTED );
        return;
    }

    if (pipe_end->connection && !list_empty( &pipe_end->connection->message_queue ))
    {
        fd_queue_async( pipe_end->fd, async, ASYNC_TYPE_WAIT );
        set_error( STATUS_PENDING );
    }
}

static data_size_t pipe_end_get_avail( struct pipe_end *pipe_end )
{
    struct pipe_message *message;
    data_size_t avail = 0;

    LIST_FOR_EACH_ENTRY( message, &pipe_end->message_queue, struct pipe_message, entry )
        avail += message->iosb->in_size - message->read_pos;

    return avail;
}

static void pipe_end_get_file_info( struct fd *fd, obj_handle_t handle, unsigned int info_class )
{
    struct pipe_end *pipe_end = get_fd_user( fd );
    struct named_pipe *pipe = pipe_end->pipe;

    switch (info_class)
    {
    case FileNameInformation:
        {
            FILE_NAME_INFORMATION *name_info;
            data_size_t name_len, reply_size;
            const WCHAR *name;

            if (get_reply_max_size() < sizeof(*name_info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }

            /* FIXME: We should be able to return on unlinked pipe */
            if (!pipe || !(name = get_object_name( &pipe->obj, &name_len )))
            {
                set_error( STATUS_PIPE_DISCONNECTED );
                return;
            }

            reply_size = offsetof( FILE_NAME_INFORMATION, FileName[name_len/sizeof(WCHAR) + 1] );
            if (reply_size > get_reply_max_size())
            {
                reply_size = get_reply_max_size();
                set_error( STATUS_BUFFER_OVERFLOW );
            }

            if (!(name_info = set_reply_data_size( reply_size ))) return;
            name_info->FileNameLength = name_len + sizeof(WCHAR);
            name_info->FileName[0] = '\\';
            reply_size -= offsetof( FILE_NAME_INFORMATION, FileName[1] );
            if (reply_size) memcpy( &name_info->FileName[1], name, reply_size );
            break;
        }
    case FilePipeInformation:
    {
            FILE_PIPE_INFORMATION *pipe_info;

            if (!(get_handle_access( current->process, handle) & FILE_READ_ATTRIBUTES))
            {
                set_error( STATUS_ACCESS_DENIED );
                return;
            }

            if (get_reply_max_size() < sizeof(*pipe_info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }

            if (!pipe)
            {
                set_error( STATUS_PIPE_DISCONNECTED );
                return;
            }

            if (!(pipe_info = set_reply_data_size( sizeof(*pipe_info) ))) return;
            pipe_info->ReadMode       = (pipe_end->flags & NAMED_PIPE_MESSAGE_STREAM_READ)
                ? FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
            pipe_info->CompletionMode = (pipe_end->flags & NAMED_PIPE_NONBLOCKING_MODE)
                ? FILE_PIPE_COMPLETE_OPERATION : FILE_PIPE_QUEUE_OPERATION;
            break;
        }
    case FilePipeLocalInformation:
        {
            FILE_PIPE_LOCAL_INFORMATION *pipe_info;

            if (!(get_handle_access( current->process, handle) & FILE_READ_ATTRIBUTES))
            {
                set_error( STATUS_ACCESS_DENIED );
                return;
            }

            if (get_reply_max_size() < sizeof(*pipe_info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }

            if (!pipe)
            {
                set_error( STATUS_PIPE_DISCONNECTED );
                return;
            }

            if (!(pipe_info = set_reply_data_size( sizeof(*pipe_info) ))) return;
            pipe_info->NamedPipeType = pipe->message_mode;
            switch (pipe->sharing)
            {
            case FILE_SHARE_READ:
                pipe_info->NamedPipeConfiguration = FILE_PIPE_OUTBOUND;
                break;
            case FILE_SHARE_WRITE:
                pipe_info->NamedPipeConfiguration = FILE_PIPE_INBOUND;
                break;
            case FILE_SHARE_READ | FILE_SHARE_WRITE:
                pipe_info->NamedPipeConfiguration = FILE_PIPE_FULL_DUPLEX;
                break;
            }
            pipe_info->MaximumInstances    = pipe->maxinstances;
            pipe_info->CurrentInstances    = pipe->instances;
            pipe_info->InboundQuota        = pipe->insize;

            pipe_info->ReadDataAvailable   = pipe_end_get_avail( pipe_end );

            pipe_info->OutboundQuota       = pipe->outsize;
            pipe_info->NamedPipeState      = pipe_end->state;
            pipe_info->NamedPipeEnd        = pipe_end->obj.ops == &pipe_server_ops
                ? FILE_PIPE_SERVER_END : FILE_PIPE_CLIENT_END;

            pipe_info->WriteQuotaAvailable = pipe_info->NamedPipeEnd == FILE_PIPE_CLIENT_END
                ? pipe_info->InboundQuota : pipe_info->OutboundQuota;
                /* FIXME: Needs to be reduced by ReadDataAvailable at the other end of the pipe. */
            break;
        }
    case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION *std_info;

            if (!(get_handle_access( current->process, handle) & FILE_READ_ATTRIBUTES))
            {
                set_error( STATUS_ACCESS_DENIED );
                return;
            }

            if (get_reply_max_size() < sizeof(*std_info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }

            if (!pipe)
            {
                set_error( STATUS_PIPE_DISCONNECTED );
                return;
            }

            if (!(std_info = set_reply_data_size( sizeof(*std_info) ))) return;
            std_info->AllocationSize.QuadPart = pipe->outsize + pipe->insize;
            std_info->EndOfFile.QuadPart      = pipe_end_get_avail( pipe_end );
            std_info->NumberOfLinks           = 1; /* FIXME */
            std_info->DeletePending           = 0; /* FIXME */
            std_info->Directory               = 0;
            break;
        }
    default:
        default_fd_get_file_info( fd, handle, info_class );
    }
}

static struct security_descriptor *pipe_end_get_sd( struct object *obj )
{
    struct pipe_end *pipe_end = (struct pipe_end *) obj;
    if (pipe_end->pipe) return default_get_sd( &pipe_end->pipe->obj );
    set_error( STATUS_PIPE_DISCONNECTED );
    return NULL;
}

static int pipe_end_set_sd( struct object *obj, const struct security_descriptor *sd,
                            unsigned int set_info )
{
    struct pipe_end *pipe_end = (struct pipe_end *) obj;
    if (pipe_end->pipe) return default_set_sd( &pipe_end->pipe->obj, sd, set_info );
    set_error( STATUS_PIPE_DISCONNECTED );
    return 0;
}

static WCHAR *pipe_end_get_full_name( struct object *obj, data_size_t *len )
{
    struct pipe_end *pipe_end = (struct pipe_end *) obj;
    return pipe_end->pipe->obj.ops->get_full_name( &pipe_end->pipe->obj, len );
}

static void pipe_end_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class )
{
    switch (info_class)
    {
    case FileFsDeviceInformation:
        {
            static const FILE_FS_DEVICE_INFORMATION device_info =
            {
                FILE_DEVICE_NAMED_PIPE,
                FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL
            };
            if (get_reply_max_size() >= sizeof(device_info))
                set_reply_data( &device_info, sizeof(device_info) );
            else
                set_error( STATUS_BUFFER_TOO_SMALL );
            break;
        }
    default:
        set_error( STATUS_NOT_IMPLEMENTED );
    }
}

static void message_queue_read( struct pipe_end *pipe_end, struct async *async )
{
    struct iosb *iosb = async_get_iosb( async );
    unsigned int status = STATUS_SUCCESS;
    struct pipe_message *message;
    data_size_t out_size;

    if (pipe_end->flags & NAMED_PIPE_MESSAGE_STREAM_READ)
    {
        message = LIST_ENTRY( list_head(&pipe_end->message_queue), struct pipe_message, entry );
        out_size = min( iosb->out_size, message->iosb->in_size - message->read_pos );

        if (message->read_pos + out_size < message->iosb->in_size)
            status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        data_size_t avail = 0;
        LIST_FOR_EACH_ENTRY( message, &pipe_end->message_queue, struct pipe_message, entry )
        {
            avail += message->iosb->in_size - message->read_pos;
            if (avail >= iosb->out_size) break;
        }
        out_size = min( iosb->out_size, avail );
    }

    message = LIST_ENTRY( list_head(&pipe_end->message_queue), struct pipe_message, entry );
    if (!message->read_pos && message->iosb->in_size == iosb->out_size) /* fast path */
    {
        async_request_complete( async, status, out_size, out_size, message->iosb->in_data );
        message->iosb->in_data = NULL;
        wake_message( message, message->iosb->in_size );
        free_message( message );
    }
    else
    {
        data_size_t write_pos = 0, writing;
        char *buf = NULL;

        if (out_size && !(buf = malloc( out_size )))
        {
            async_terminate( async, STATUS_NO_MEMORY );
            release_object( iosb );
            return;
        }

        do
        {
            message = LIST_ENTRY( list_head(&pipe_end->message_queue), struct pipe_message, entry );
            writing = min( out_size - write_pos, message->iosb->in_size - message->read_pos );
            if (writing) memcpy( buf + write_pos, (const char *)message->iosb->in_data + message->read_pos, writing );
            write_pos += writing;
            message->read_pos += writing;
            if (message->read_pos == message->iosb->in_size)
            {
                wake_message(message, message->iosb->in_size);
                free_message(message);
            }
        } while (write_pos < out_size);

        async_request_complete( async, status, out_size, out_size, buf );
    }

    release_object( iosb );
}

/* We call async_terminate in our reselect implementation, which causes recursive reselect.
 * We're not interested in such reselect calls, so we ignore them. */
static int ignore_reselect;

static void reselect_write_queue( struct pipe_end *pipe_end );

static void reselect_read_queue( struct pipe_end *pipe_end, int reselect_write )
{
    struct async *async;

    ignore_reselect = 1;
    while (!list_empty( &pipe_end->message_queue ) && (async = find_pending_async( &pipe_end->read_q )))
    {
        message_queue_read( pipe_end, async );
        release_object( async );
        reselect_write = 1;
    }
    ignore_reselect = 0;

    if (pipe_end->connection)
    {
        if (list_empty( &pipe_end->message_queue ))
            fd_async_wake_up( pipe_end->connection->fd, ASYNC_TYPE_WAIT, STATUS_SUCCESS );
        else if (reselect_write)
            reselect_write_queue( pipe_end->connection );
    }
}

static void reselect_write_queue( struct pipe_end *pipe_end )
{
    struct pipe_message *message, *next;
    struct pipe_end *reader = pipe_end->connection;
    data_size_t avail = 0;

    if (!reader) return;

    ignore_reselect = 1;

    LIST_FOR_EACH_ENTRY_SAFE( message, next, &reader->message_queue, struct pipe_message, entry )
    {
        if (message->async && message->iosb->status != STATUS_PENDING)
        {
            release_object( message->async );
            message->async = NULL;
            free_message( message );
        }
        else
        {
            avail += message->iosb->in_size - message->read_pos;
            if (message->async && (avail <= reader->buffer_size || !message->iosb->in_size))
            {
                wake_message( message, message->iosb->in_size );
            }
            else if (message->async && (pipe_end->flags & NAMED_PIPE_NONBLOCKING_MODE))
            {
                wake_message( message, message->read_pos );
                free_message( message );
            }
        }
    }

    ignore_reselect = 0;
    reselect_read_queue( reader, 0 );
}

static void pipe_end_read( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct pipe_end *pipe_end = get_fd_user( fd );

    switch (pipe_end->state)
    {
    case FILE_PIPE_CONNECTED_STATE:
        if ((pipe_end->flags & NAMED_PIPE_NONBLOCKING_MODE) && list_empty( &pipe_end->message_queue ))
        {
            set_error( STATUS_PIPE_EMPTY );
            return;
        }
        break;
    case FILE_PIPE_DISCONNECTED_STATE:
        set_error( STATUS_PIPE_DISCONNECTED );
        return;
    case FILE_PIPE_LISTENING_STATE:
        set_error( STATUS_PIPE_LISTENING );
        return;
    case FILE_PIPE_CLOSING_STATE:
        if (!list_empty( &pipe_end->message_queue )) break;
        set_error( STATUS_PIPE_BROKEN );
        return;
    }

    queue_async( &pipe_end->read_q, async );
    reselect_read_queue( pipe_end, 0 );
    set_error( STATUS_PENDING );
}

static void pipe_end_write( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct pipe_end *pipe_end = get_fd_user( fd );
    struct pipe_message *message;
    struct iosb *iosb;

    switch (pipe_end->state)
    {
    case FILE_PIPE_CONNECTED_STATE:
        break;
    case FILE_PIPE_DISCONNECTED_STATE:
        set_error( STATUS_PIPE_DISCONNECTED );
        return;
    case FILE_PIPE_LISTENING_STATE:
        set_error( STATUS_PIPE_LISTENING );
        return;
    case FILE_PIPE_CLOSING_STATE:
        set_error( STATUS_PIPE_CLOSING );
        return;
    }

    if (!pipe_end->pipe->message_mode && !get_req_data_size()) return;

    iosb = async_get_iosb( async );
    message = queue_message( pipe_end->connection, iosb );
    release_object( iosb );
    if (!message) return;

    message->async = (struct async *)grab_object( async );
    queue_async( &pipe_end->write_q, async );
    reselect_read_queue( pipe_end->connection, 1 );
    set_error( STATUS_PENDING );
}

static void pipe_end_reselect_async( struct fd *fd, struct async_queue *queue )
{
    struct pipe_end *pipe_end = get_fd_user( fd );

    if (ignore_reselect) return;

    if (&pipe_end->write_q == queue)
        reselect_write_queue( pipe_end );
    else if (&pipe_end->read_q == queue)
        reselect_read_queue( pipe_end, 0 );
}

static enum server_fd_type pipe_end_get_fd_type( struct fd *fd )
{
    return FD_TYPE_DEVICE;
}

static void pipe_end_peek( struct pipe_end *pipe_end )
{
    unsigned reply_size = get_reply_max_size();
    FILE_PIPE_PEEK_BUFFER *buffer;
    struct pipe_message *message;
    data_size_t avail = 0;
    data_size_t message_length = 0;

    if (reply_size < offsetof( FILE_PIPE_PEEK_BUFFER, Data ))
    {
        set_error( STATUS_INFO_LENGTH_MISMATCH );
        return;
    }
    reply_size -= offsetof( FILE_PIPE_PEEK_BUFFER, Data );

    switch (pipe_end->state)
    {
    case FILE_PIPE_CONNECTED_STATE:
        break;
    case FILE_PIPE_CLOSING_STATE:
        if (!list_empty( &pipe_end->message_queue )) break;
        set_error( STATUS_PIPE_BROKEN );
        return;
    default:
        set_error( pipe_end->pipe ? STATUS_INVALID_PIPE_STATE : STATUS_PIPE_DISCONNECTED );
        return;
    }

    LIST_FOR_EACH_ENTRY( message, &pipe_end->message_queue, struct pipe_message, entry )
        avail += message->iosb->in_size - message->read_pos;
    reply_size = min( reply_size, avail );

    if (avail && pipe_end->pipe->message_mode)
    {
        message = LIST_ENTRY( list_head(&pipe_end->message_queue), struct pipe_message, entry );
        message_length = message->iosb->in_size - message->read_pos;
        reply_size = min( reply_size, message_length );
    }

    if (!(buffer = set_reply_data_size( offsetof( FILE_PIPE_PEEK_BUFFER, Data[reply_size] )))) return;
    buffer->NamedPipeState    = pipe_end->state;
    buffer->ReadDataAvailable = avail;
    buffer->NumberOfMessages  = 0;  /* FIXME */
    buffer->MessageLength     = message_length;

    if (reply_size)
    {
        data_size_t write_pos = 0, writing;
        LIST_FOR_EACH_ENTRY( message, &pipe_end->message_queue, struct pipe_message, entry )
        {
            writing = min( reply_size - write_pos, message->iosb->in_size - message->read_pos );
            memcpy( buffer->Data + write_pos, (const char *)message->iosb->in_data + message->read_pos,
                    writing );
            write_pos += writing;
            if (write_pos == reply_size) break;
        }
    }
    if (message_length > reply_size) set_error( STATUS_BUFFER_OVERFLOW );
}

static void pipe_end_transceive( struct pipe_end *pipe_end, struct async *async )
{
    struct pipe_message *message;
    struct iosb *iosb;

    if (!pipe_end->connection)
    {
        set_error( pipe_end->pipe ? STATUS_INVALID_PIPE_STATE : STATUS_PIPE_DISCONNECTED );
        return;
    }

    if (!(pipe_end->flags & NAMED_PIPE_MESSAGE_STREAM_READ))
    {
        set_error( STATUS_INVALID_READ_MODE );
        return;
    }

    /* not allowed if we already have read data buffered */
    if (!list_empty( &pipe_end->message_queue ))
    {
        set_error( STATUS_PIPE_BUSY );
        return;
    }

    iosb = async_get_iosb( async );
    /* ignore output buffer copy transferred because of METHOD_NEITHER */
    iosb->in_size -= iosb->out_size;
    /* transaction never blocks on write, so just queue a message without async */
    message = queue_message( pipe_end->connection, iosb );
    release_object( iosb );
    if (!message) return;
    reselect_read_queue( pipe_end->connection, 0 );

    queue_async( &pipe_end->read_q, async );
    reselect_read_queue( pipe_end, 0 );
    set_error( STATUS_PENDING );
}

static void pipe_end_get_connection_attribute( struct pipe_end *pipe_end )
{
    const char *attr = get_req_data();
    data_size_t value_size, attr_size = get_req_data_size();
    void *value;

    if (attr_size == sizeof("ClientProcessId") && !memcmp( attr, "ClientProcessId", attr_size ))
    {
        value = &pipe_end->client_pid;
        value_size = sizeof(pipe_end->client_pid);
    }
    else if (attr_size == sizeof("ServerProcessId") && !memcmp( attr, "ServerProcessId", attr_size ))
    {
        value = &pipe_end->server_pid;
        value_size = sizeof(pipe_end->server_pid);
    }
    else
    {
        set_error( STATUS_ILLEGAL_FUNCTION );
        return;
    }

    if (get_reply_max_size() < value_size)
    {
        set_error( STATUS_INFO_LENGTH_MISMATCH );
        return;
    }

    set_reply_data( value, value_size );
}

static void pipe_end_ioctl( struct pipe_end *pipe_end, ioctl_code_t code, struct async *async )
{
    switch(code)
    {
    case FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE:
        pipe_end_get_connection_attribute( pipe_end );
        break;

    case FSCTL_PIPE_PEEK:
        pipe_end_peek( pipe_end );
        break;

    case FSCTL_PIPE_TRANSCEIVE:
        pipe_end_transceive( pipe_end, async );
        break;

    default:
        default_fd_ioctl( pipe_end->fd, code, async );
    }
}

static void pipe_server_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct pipe_server *server = get_fd_user( fd );

    switch(code)
    {
    case FSCTL_PIPE_LISTEN:
        switch(server->pipe_end.state)
        {
        case FILE_PIPE_LISTENING_STATE:
            break;
        case FILE_PIPE_DISCONNECTED_STATE:
            server->pipe_end.state = FILE_PIPE_LISTENING_STATE;
            list_add_tail( &server->pipe_end.pipe->listeners, &server->entry );
            break;
        case FILE_PIPE_CONNECTED_STATE:
            set_error( STATUS_PIPE_CONNECTED );
            return;
        case FILE_PIPE_CLOSING_STATE:
            set_error( STATUS_PIPE_CLOSING );
            return;
        }

        if (server->pipe_end.flags & NAMED_PIPE_NONBLOCKING_MODE)
        {
            set_error( STATUS_PIPE_LISTENING );
            return;
        }
        queue_async( &server->listen_q, async );
        async_wake_up( &server->pipe_end.pipe->waiters, STATUS_SUCCESS );
        set_error( STATUS_PENDING );
        return;

    case FSCTL_PIPE_DISCONNECT:
        switch(server->pipe_end.state)
        {
        case FILE_PIPE_CONNECTED_STATE:
            /* dump the client connection - all data is lost */
            assert( server->pipe_end.connection );
            release_object( server->pipe_end.connection->pipe );
            server->pipe_end.connection->pipe = NULL;
            break;
        case FILE_PIPE_CLOSING_STATE:
            break;
        case FILE_PIPE_LISTENING_STATE:
            set_error( STATUS_PIPE_LISTENING );
            return;
        case FILE_PIPE_DISCONNECTED_STATE:
            set_error( STATUS_PIPE_DISCONNECTED );
            return;
        }

        pipe_end_disconnect( &server->pipe_end, STATUS_PIPE_DISCONNECTED );
        return;

    case FSCTL_PIPE_IMPERSONATE:
        if (current->process->token) /* FIXME: use the client token */
        {
            struct token *token;
            if (!(token = token_duplicate( current->process->token, 0, SecurityImpersonation, NULL, NULL, 0, NULL, 0 )))
                return;
            if (current->token) release_object( current->token );
            current->token = token;
        }
        return;

    default:
        pipe_end_ioctl( &server->pipe_end, code, async );
    }
}

static void pipe_client_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct pipe_end *client = get_fd_user( fd );

    switch(code)
    {
    case FSCTL_PIPE_LISTEN:
        set_error( STATUS_ILLEGAL_FUNCTION );
        return;

    default:
        pipe_end_ioctl( client, code, async );
    }
}

static void init_pipe_end( struct pipe_end *pipe_end, struct named_pipe *pipe,
                           unsigned int pipe_flags, data_size_t buffer_size )
{
    pipe_end->pipe = (struct named_pipe *)grab_object( pipe );
    pipe_end->fd = NULL;
    pipe_end->flags = pipe_flags;
    pipe_end->connection = NULL;
    pipe_end->buffer_size = buffer_size;
    init_async_queue( &pipe_end->read_q );
    init_async_queue( &pipe_end->write_q );
    list_init( &pipe_end->message_queue );
}

static struct pipe_server *create_pipe_server( struct named_pipe *pipe, unsigned int options,
                                               unsigned int pipe_flags )
{
    struct pipe_server *server;

    server = alloc_object( &pipe_server_ops );
    if (!server)
        return NULL;

    server->options = options;
    init_pipe_end( &server->pipe_end, pipe, pipe_flags, pipe->insize );
    server->pipe_end.state = FILE_PIPE_LISTENING_STATE;
    server->pipe_end.server_pid = get_process_id( current->process );
    init_async_queue( &server->listen_q );

    list_add_tail( &pipe->listeners, &server->entry );
    if (!(server->pipe_end.fd = alloc_pseudo_fd( &pipe_server_fd_ops, &server->pipe_end.obj, options )))
    {
        release_object( server );
        return NULL;
    }
    allow_fd_caching( server->pipe_end.fd );
    set_fd_signaled( server->pipe_end.fd, 1 );
    async_wake_up( &pipe->waiters, STATUS_SUCCESS );
    return server;
}

static struct pipe_end *create_pipe_client( struct named_pipe *pipe, data_size_t buffer_size, unsigned int options )
{
    struct pipe_end *client;

    client = alloc_object( &pipe_client_ops );
    if (!client)
        return NULL;

    init_pipe_end( client, pipe, 0, buffer_size );
    client->state = FILE_PIPE_CONNECTED_STATE;
    client->client_pid = get_process_id( current->process );

    client->fd = alloc_pseudo_fd( &pipe_client_fd_ops, &client->obj, options );
    if (!client->fd)
    {
        release_object( client );
        return NULL;
    }
    allow_fd_caching( client->fd );
    set_fd_signaled( client->fd, 1 );

    return client;
}

static int named_pipe_link_name( struct object *obj, struct object_name *name, struct object *parent )
{
    if (parent->ops == &named_pipe_dir_ops) parent = &((struct named_pipe_device_file *)parent)->device->obj;
    if (parent->ops != &named_pipe_device_ops)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return 0;
    }
    namespace_add( ((struct named_pipe_device *)parent)->pipes, name );
    name->parent = grab_object( parent );
    return 1;
}

static struct object *named_pipe_open_file( struct object *obj, unsigned int access,
                                            unsigned int sharing, unsigned int options )
{
    struct named_pipe *pipe = (struct named_pipe *)obj;
    struct pipe_server *server;
    struct pipe_end *client;
    unsigned int pipe_sharing;

    if (list_empty( &pipe->listeners ))
    {
        set_error( STATUS_PIPE_NOT_AVAILABLE );
        return NULL;
    }
    server = LIST_ENTRY( list_head( &pipe->listeners ), struct pipe_server, entry );

    pipe_sharing = pipe->sharing;
    if (((access & GENERIC_READ) && !(pipe_sharing & FILE_SHARE_READ)) ||
        ((access & GENERIC_WRITE) && !(pipe_sharing & FILE_SHARE_WRITE)))
    {
        set_error( STATUS_ACCESS_DENIED );
        return NULL;
    }

    if ((client = create_pipe_client( pipe, pipe->outsize, options )))
    {
        async_wake_up( &server->listen_q, STATUS_SUCCESS );
        server->pipe_end.state = FILE_PIPE_CONNECTED_STATE;
        server->pipe_end.connection = client;
        client->connection = &server->pipe_end;
        server->pipe_end.client_pid = client->client_pid;
        client->server_pid = server->pipe_end.server_pid;
        list_remove( &server->entry );
    }
    return &client->obj;
}

static void named_pipe_device_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    switch(code)
    {
    case FSCTL_PIPE_WAIT:
    case FSCTL_PIPE_LISTEN:
    case FSCTL_PIPE_IMPERSONATE:
        set_error( STATUS_ILLEGAL_FUNCTION );
        return;

    case FSCTL_PIPE_DISCONNECT:
    case FSCTL_PIPE_TRANSCEIVE:
        set_error( STATUS_PIPE_DISCONNECTED );
        return;

    case FSCTL_PIPE_QUERY_CLIENT_PROCESS:
        set_error( STATUS_INVALID_PARAMETER );
        return;

    default:
        default_fd_ioctl( fd, code, async );
    }
}

static void named_pipe_dir_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct named_pipe_device *device = get_fd_user( fd );

    switch(code)
    {
    case FSCTL_PIPE_WAIT:
        {
            const FILE_PIPE_WAIT_FOR_BUFFER *buffer = get_req_data();
            data_size_t size = get_req_data_size();
            struct named_pipe *pipe;
            struct unicode_str name;
            timeout_t when;

            if (size < sizeof(*buffer) ||
                size < FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name[buffer->NameLength/sizeof(WCHAR)]))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            name.str = buffer->Name;
            name.len = (buffer->NameLength / sizeof(WCHAR)) * sizeof(WCHAR);
            if (!(pipe = open_named_object( &device->obj, &named_pipe_ops, &name, 0 ))) return;

            if (list_empty( &pipe->listeners ))
            {
                queue_async( &pipe->waiters, async );
                when = buffer->TimeoutSpecified ? buffer->Timeout.QuadPart : pipe->timeout;
                async_set_timeout( async, when, STATUS_IO_TIMEOUT );
                set_error( STATUS_PENDING );
            }

            release_object( pipe );
            return;
        }

    default:
        named_pipe_device_ioctl( fd, code, async );
    }
}


DECL_HANDLER(create_named_pipe)
{
    struct named_pipe *pipe;
    struct pipe_server *server;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if (!req->sharing || (req->sharing & ~(FILE_SHARE_READ | FILE_SHARE_WRITE)) ||
        (!(req->flags & NAMED_PIPE_MESSAGE_STREAM_WRITE) && (req->flags & NAMED_PIPE_MESSAGE_STREAM_READ)))
    {
        if (root) release_object( root );
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!req->access)
    {
        if (root) release_object( root );
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    if (!name.len)  /* pipes need a root directory even without a name */
    {
        if (!objattr->rootdir)
        {
            set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
            return;
        }
        if (!(root = get_handle_obj( current->process, objattr->rootdir, 0, NULL ))) return;
    }

    switch (req->disposition)
    {
    case FILE_OPEN:
        pipe = open_named_object( root, &named_pipe_ops, &name, objattr->attributes );
        break;
    case FILE_CREATE:
    case FILE_OPEN_IF:
        pipe = create_named_object( root, &named_pipe_ops, &name, objattr->attributes | OBJ_OPENIF, NULL );
        break;
    default:
        pipe = NULL;
        set_error( STATUS_INVALID_PARAMETER );
        break;
    }

    if (root) release_object( root );
    if (!pipe) return;

    if (get_error() != STATUS_OBJECT_NAME_EXISTS && req->disposition != FILE_OPEN)
    {
        /* initialize it if it didn't already exist */
        pipe->instances = 0;
        init_async_queue( &pipe->waiters );
        list_init( &pipe->listeners );
        pipe->insize = req->insize;
        pipe->outsize = req->outsize;
        pipe->maxinstances = req->maxinstances;
        pipe->timeout = req->timeout;
        pipe->message_mode = (req->flags & NAMED_PIPE_MESSAGE_STREAM_WRITE) != 0;
        pipe->sharing = req->sharing;
        if (sd) default_set_sd( &pipe->obj, sd, OWNER_SECURITY_INFORMATION |
                                                GROUP_SECURITY_INFORMATION |
                                                DACL_SECURITY_INFORMATION |
                                                SACL_SECURITY_INFORMATION );
        reply->created = 1;
    }
    else
    {
        if (pipe->maxinstances <= pipe->instances)
        {
            set_error( STATUS_INSTANCE_NOT_AVAILABLE );
            release_object( pipe );
            return;
        }
        if (pipe->sharing != req->sharing || req->disposition == FILE_CREATE)
        {
            set_error( STATUS_ACCESS_DENIED );
            release_object( pipe );
            return;
        }
        clear_error(); /* clear the name collision */
    }

    server = create_pipe_server( pipe, req->options, req->flags );
    if (server)
    {
        reply->handle = alloc_handle( current->process, server, req->access, objattr->attributes );
        pipe->instances++;
        release_object( server );
    }

    release_object( pipe );
}

DECL_HANDLER(set_named_pipe_info)
{
    struct pipe_end *pipe_end;

    pipe_end = (struct pipe_end *)get_handle_obj( current->process, req->handle,
                                                  FILE_WRITE_ATTRIBUTES, &pipe_server_ops );
    if (!pipe_end)
    {
        if (get_error() != STATUS_OBJECT_TYPE_MISMATCH)
            return;

        clear_error();
        pipe_end = (struct pipe_end *)get_handle_obj( current->process, req->handle,
                                                      0, &pipe_client_ops );
        if (!pipe_end) return;
    }

    if (!pipe_end->pipe)
    {
        set_error( STATUS_PIPE_DISCONNECTED );
    }
    else if ((req->flags & ~(NAMED_PIPE_MESSAGE_STREAM_READ | NAMED_PIPE_NONBLOCKING_MODE)) ||
            ((req->flags & NAMED_PIPE_MESSAGE_STREAM_READ) && !pipe_end->pipe->message_mode))
    {
        set_error( STATUS_INVALID_PARAMETER );
    }
    else
    {
        pipe_end->flags = req->flags;
    }

    release_object( pipe_end );
}

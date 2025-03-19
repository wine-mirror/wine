/*
 * Server-side console management
 *
 * Copyright (C) 1998 Alexandre Julliard
 *               2001 Eric Pouech
 * Copyright 2020 Jacek Caban for CodeWeavers
 *
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
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "handle.h"
#include "process.h"
#include "request.h"
#include "file.h"
#include "unicode.h"
#include "wincon.h"
#include "winternl.h"
#include "wine/condrv.h"

struct screen_buffer;

struct history_line
{
    data_size_t len;
    WCHAR       text[1];
};

struct console
{
    struct object                obj;           /* object header */
    struct event_sync           *sync;          /* sync object for wait/signal */
    int                          signaled;      /* is console signaled */
    struct thread               *renderer;      /* console renderer thread */
    struct screen_buffer        *active;        /* active screen buffer */
    struct console_server       *server;        /* console server object */
    unsigned int                 last_id;       /* id of last created console buffer */
    struct fd                   *fd;            /* for bare console, attached input fd */
    struct async_queue           ioctl_q;       /* ioctl queue */
    struct async_queue           read_q;        /* read queue */
    struct list                  screen_buffers;/* attached screen buffers */
    struct list                  inputs;        /* attached console inputs */
    struct list                  outputs;       /* attached console outputs */
};

static void console_dump( struct object *obj, int verbose );
static void console_destroy( struct object *obj );
static struct fd *console_get_fd( struct object *obj );
static struct object *console_get_sync( struct object *obj );
static struct object *console_lookup_name( struct object *obj, struct unicode_str *name,
                                           unsigned int attr, struct object *root );
static struct object *console_open_file( struct object *obj, unsigned int access,
                                         unsigned int sharing, unsigned int options );

static const struct object_ops console_ops =
{
    sizeof(struct console),           /* size */
    &file_type,                       /* type */
    console_dump,                     /* dump */
    NULL,                             /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    no_signal,                        /* signal */
    console_get_fd,                   /* get_fd */
    console_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    console_lookup_name,              /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    console_open_file,                /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_destroy                   /* destroy */
};

static enum server_fd_type console_get_fd_type( struct fd *fd );
static void console_get_file_info( struct fd *fd, obj_handle_t handle, unsigned int info_class );
static void console_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class );
static void console_read( struct fd *fd, struct async *async, file_pos_t pos );
static void console_flush( struct fd *fd, struct async *async );
static void console_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    console_read,                 /* read */
    no_fd_write,                  /* write */
    console_flush,                /* flush */
    console_get_file_info,        /* get_file_info */
    console_get_volume_info,      /* get_volume_info */
    console_ioctl,                /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

struct console_host_ioctl
{
    unsigned int          code;        /* ioctl code */
    int                   output;      /* output id for screen buffer ioctls */
    struct async         *async;       /* ioctl async */
    struct list           entry;       /* list entry */
};

struct console_server
{
    struct object         obj;            /* object header */
    struct event_sync    *sync;           /* sync object for wait/signal */
    struct fd            *fd;             /* pseudo-fd for ioctls */
    struct console       *console;        /* attached console */
    struct list           queue;          /* ioctl queue */
    struct list           read_queue;     /* blocking read queue */
    unsigned int          busy : 1;       /* flag if server processing an ioctl */
    unsigned int          once_input : 1; /* flag if input thread has already been requested */
    int                   term_fd;        /* UNIX terminal fd */
    struct termios        termios;        /* original termios */
};

static void console_server_dump( struct object *obj, int verbose );
static void console_server_destroy( struct object *obj );
static struct fd *console_server_get_fd( struct object *obj );
static struct object *console_server_get_sync( struct object *obj );
static struct object *console_server_lookup_name( struct object *obj, struct unicode_str *name,
                                                unsigned int attr, struct object *root );
static struct object *console_server_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options );

static const struct object_ops console_server_ops =
{
    sizeof(struct console_server),    /* size */
    &file_type,                       /* type */
    console_server_dump,              /* dump */
    NULL,                             /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    no_signal,                        /* signal */
    console_server_get_fd,            /* get_fd */
    console_server_get_sync,          /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    console_server_lookup_name,       /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    console_server_open_file,         /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_server_destroy            /* destroy */
};

static void console_server_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_server_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    no_fd_flush,                  /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    console_server_ioctl,         /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

struct font_info
{
    short int width;
    short int height;
    short int weight;
    short int pitch_family;
    WCHAR    *face_name;
    data_size_t face_len;
};

struct screen_buffer
{
    struct object         obj;           /* object header */
    struct list           entry;         /* entry in list of all screen buffers */
    struct console       *input;         /* associated console input */
    unsigned int          id;            /* buffer id */
    struct fd            *fd;            /* for bare console, attached output fd */
    struct async_queue    ioctl_q;       /* ioctl queue */
};

static void screen_buffer_dump( struct object *obj, int verbose );
static void screen_buffer_destroy( struct object *obj );
static int screen_buffer_signaled( struct object *obj, struct wait_queue_entry *entry );
static struct fd *screen_buffer_get_fd( struct object *obj );
static struct object *screen_buffer_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops screen_buffer_ops =
{
    sizeof(struct screen_buffer),     /* size */
    &file_type,                       /* type */
    screen_buffer_dump,               /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    screen_buffer_signaled,           /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    screen_buffer_get_fd,             /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    screen_buffer_open_file,          /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    screen_buffer_destroy             /* destroy */
};

static void screen_buffer_write( struct fd *fd, struct async *async, file_pos_t pos );
static void screen_buffer_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops screen_buffer_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    screen_buffer_write,          /* write */
    no_fd_flush,                  /* flush */
    console_get_file_info,        /* get_file_info */
    console_get_volume_info,      /* get_volume_info */
    screen_buffer_ioctl,          /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

static void console_device_dump( struct object *obj, int verbose );
static struct object *console_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                unsigned int attr, struct object *root );
static struct object *console_device_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options );

static const struct object_ops console_device_ops =
{
    sizeof(struct object),            /* size */
    &device_type,                     /* type */
    console_device_dump,              /* dump */
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
    console_device_lookup_name,       /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    console_device_open_file,         /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    no_destroy                        /* destroy */
};

struct console_input
{
    struct object         obj;         /* object header */
    struct fd            *fd;          /* pseudo-fd */
    struct list           entry;       /* entry in console->inputs */
    struct console       *console;     /* associated console at creation time */
};

static void console_input_dump( struct object *obj, int verbose );
static int console_input_signaled( struct object *obj, struct wait_queue_entry *entry );
static struct object *console_input_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );
static struct fd *console_input_get_fd( struct object *obj );
static void console_input_destroy( struct object *obj );

static const struct object_ops console_input_ops =
{
    sizeof(struct console_input),     /* size */
    &device_type,                     /* type */
    console_input_dump,               /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    console_input_signaled,           /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_input_get_fd,             /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    console_input_open_file,          /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_input_destroy             /* destroy */
};

static void console_input_read( struct fd *fd, struct async *async, file_pos_t pos );
static void console_input_flush( struct fd *fd, struct async *async );
static void console_input_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_input_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    console_input_read,           /* read */
    no_fd_write,                  /* write */
    console_input_flush,          /* flush */
    console_get_file_info,        /* get_file_info */
    console_get_volume_info,      /* get_volume_info */
    console_input_ioctl,          /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

struct console_output
{
    struct object         obj;         /* object header */
    struct fd            *fd;          /* pseudo-fd */
    struct list           entry;       /* entry in console->outputs */
    struct console       *console;     /* associated console at creation time */
};

static void console_output_dump( struct object *obj, int verbose );
static int console_output_signaled( struct object *obj, struct wait_queue_entry *entry );
static struct fd *console_output_get_fd( struct object *obj );
static struct object *console_output_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options );
static void console_output_destroy( struct object *obj );

static const struct object_ops console_output_ops =
{
    sizeof(struct console_output),    /* size */
    &device_type,                     /* type */
    console_output_dump,              /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    console_output_signaled,          /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_output_get_fd,            /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    console_output_open_file,         /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_output_destroy            /* destroy */
};

static void console_output_write( struct fd *fd, struct async *async, file_pos_t pos );
static void console_output_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_output_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    console_output_write,         /* write */
    no_fd_flush,                  /* flush */
    console_get_file_info,        /* get_file_info */
    console_get_volume_info,      /* get_volume_info */
    console_output_ioctl,         /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

struct console_connection
{
    struct object         obj;         /* object header */
    struct fd            *fd;          /* pseudo-fd for ioctls */
};

static void console_connection_dump( struct object *obj, int verbose );
static struct fd *console_connection_get_fd( struct object *obj );
static struct object *console_connection_lookup_name( struct object *obj, struct unicode_str *name,
                                                    unsigned int attr, struct object *root );
static struct object *console_connection_open_file( struct object *obj, unsigned int access,
                                                    unsigned int sharing, unsigned int options );
static int console_connection_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void console_connection_destroy( struct object *obj );

static const struct object_ops console_connection_ops =
{
    sizeof(struct console_connection),/* size */
    &device_type,                     /* type */
    console_connection_dump,          /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_connection_get_fd,        /* get_fd */
    default_get_sync,                 /* get_sync */
    default_map_access,               /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    console_connection_lookup_name,   /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    console_connection_open_file,     /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    console_connection_close_handle,  /* close_handle */
    console_connection_destroy        /* destroy */
};

static void console_connection_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_connection_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    no_fd_flush,                  /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    console_connection_ioctl,     /* ioctl */
    default_fd_cancel_async,      /* cancel_async */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

static int queue_host_ioctl( struct console_server *server, unsigned int code, unsigned int output,
                             struct async *async, struct async_queue *queue );

static struct fd *console_get_fd( struct object *obj )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );
    return (struct fd *)grab_object( console->fd );
}

static struct object *console_get_sync( struct object *obj )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );

    /* before waiting, ensure conhost's input thread has been started */
    if (console->server && !console->server->once_input)
    {
        console->server->once_input = 1;
        if (console->server->term_fd == -1)
            queue_host_ioctl( console->server, IOCTL_CONDRV_PEEK, 0, NULL, NULL );
    }

    return grab_object( console->sync );
}

static enum server_fd_type console_get_fd_type( struct fd *fd )
{
    return FD_TYPE_CHAR;
}

static void console_get_file_info( struct fd *fd, obj_handle_t handle, unsigned int info_class )
{
    set_error( STATUS_INVALID_DEVICE_REQUEST );
}

static void console_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class )
{
    switch (info_class)
    {
    case FileFsDeviceInformation:
        {
            static const FILE_FS_DEVICE_INFORMATION device_info =
            {
                FILE_DEVICE_CONSOLE,
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

static struct object *create_console(void)
{
    struct console *console;

    if (!(console = alloc_object( &console_ops ))) return NULL;
    console->sync          = NULL;
    console->renderer      = NULL;
    console->signaled      = 0;
    console->active        = NULL;
    console->server        = NULL;
    console->fd            = NULL;
    console->last_id       = 0;
    list_init( &console->screen_buffers );
    list_init( &console->inputs );
    list_init( &console->outputs );
    init_async_queue( &console->ioctl_q );
    init_async_queue( &console->read_q );

    if (!(console->sync = create_event_sync( 1, 0 ))) goto error;
    if (!(console->fd = alloc_pseudo_fd( &console_fd_ops, &console->obj, FILE_SYNCHRONOUS_IO_NONALERT ))) goto error;
    allow_fd_caching( console->fd );
    return &console->obj;

error:
    release_object( console );
    return NULL;
}

static void console_host_ioctl_terminate( struct console_host_ioctl *call, unsigned int status )
{
    if (call->async)
    {
        async_terminate( call->async, status );
        release_object( call->async );
    }
    free( call );
}

static int queue_host_ioctl( struct console_server *server, unsigned int code, unsigned int output,
                             struct async *async, struct async_queue *queue )
{
    struct console_host_ioctl *ioctl;

    if (!(ioctl = mem_alloc( sizeof(*ioctl) ))) return 0;
    ioctl->code   = code;
    ioctl->output = output;
    ioctl->async  = NULL;
    if (async)
    {
        ioctl->async = (struct async *)grab_object( async );
        queue_async( queue, async );
    }
    list_add_tail( &server->queue, &ioctl->entry );
    signal_sync( server->sync );
    if (async) set_error( STATUS_PENDING );
    return 1;
}

static void disconnect_console_server( struct console_server *server )
{
    while (!list_empty( &server->queue ))
    {
        struct console_host_ioctl *call = LIST_ENTRY( list_head( &server->queue ), struct console_host_ioctl, entry );
        list_remove( &call->entry );
        console_host_ioctl_terminate( call, STATUS_CANCELLED );
    }
    while (!list_empty( &server->read_queue ))
    {
        struct console_host_ioctl *call = LIST_ENTRY( list_head( &server->read_queue ), struct console_host_ioctl, entry );
        list_remove( &call->entry );
        console_host_ioctl_terminate( call, STATUS_CANCELLED );
    }

    if (server->term_fd != -1)
    {
        tcsetattr( server->term_fd, TCSANOW, &server->termios );
        close( server->term_fd );
        server->term_fd = -1;
    }

    if (server->console)
    {
        assert( server->console->server == server );
        server->console->server = NULL;
        server->console = NULL;
        signal_sync( server->sync );
    }
}

static void set_active_screen_buffer( struct console *console, struct screen_buffer *screen_buffer )
{
    if (console->active == screen_buffer) return;
    if (console->active) release_object( console->active );
    console->active = (struct screen_buffer *)grab_object( screen_buffer );

    if (console->server) queue_host_ioctl( console->server, IOCTL_CONDRV_ACTIVATE,
                                           screen_buffer->id, NULL, NULL );
}

static struct object *create_screen_buffer( struct console *console )
{
    struct screen_buffer *screen_buffer;

    if (console->last_id == ~0)
    {
        set_error( STATUS_NO_MEMORY );
        return NULL;
    }

    if (!(screen_buffer = alloc_object( &screen_buffer_ops )))
        return NULL;

    screen_buffer->id    = ++console->last_id;
    screen_buffer->input = console;
    init_async_queue( &screen_buffer->ioctl_q );
    list_add_head( &console->screen_buffers, &screen_buffer->entry );

    screen_buffer->fd = alloc_pseudo_fd( &screen_buffer_fd_ops, &screen_buffer->obj,
                                         FILE_SYNCHRONOUS_IO_NONALERT );
    if (!screen_buffer->fd)
    {
        release_object( screen_buffer );
        return NULL;
    }
    allow_fd_caching(screen_buffer->fd);

    if (console->server) queue_host_ioctl( console->server, IOCTL_CONDRV_INIT_OUTPUT,
                                           screen_buffer->id, NULL, NULL );
    if (!console->active) set_active_screen_buffer( console, screen_buffer );
    return &screen_buffer->obj;
}

struct thread *console_get_renderer( struct console *console )
{
    return console->renderer;
}

struct console_signal_info
{
    struct console        *console;
    process_id_t           group;
    int                    signal;
};

static int propagate_console_signal_cb(struct process *process, void *user)
{
    struct console_signal_info* csi = (struct console_signal_info*)user;

    if (process->console == csi->console && (!csi->group || process->group_id == csi->group))
    {
        /* find a suitable thread to signal */
        struct thread *thread;
        LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
        {
            if (send_thread_signal( thread, csi->signal )) break;
        }
    }
    return FALSE;
}

static void propagate_console_signal( struct console *console,
                                      int sig, process_id_t group_id )
{
    struct console_signal_info csi;

    if (!console)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    switch (sig)
    {
    case CTRL_C_EVENT:     csi.signal = SIGINT; break;
    case CTRL_BREAK_EVENT: csi.signal = SIGQUIT; break;
    default:
        /* FIXME: should support the other events */
        set_error( STATUS_NOT_IMPLEMENTED );
        return;
    }
    csi.console = console;
    csi.group   = group_id;

    enum_processes(propagate_console_signal_cb, &csi);
}

struct console_process_list
{
    unsigned int    size;
    unsigned int    count;
    process_id_t   *processes;
    struct console *console;
};

static int console_process_list_cb(struct process *process, void *user)
{
    struct console_process_list *cpl = user;

    if (process->console == cpl->console)
    {
        if (cpl->count < cpl->size) cpl->processes[cpl->count] = process->id;
        cpl->count++;
    }

    return 0;
}

/* dumb dump */
static void console_dump( struct object *obj, int verbose )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );
    fprintf( stderr, "Console input active=%p server=%p\n",
             console->active, console->server );
}

static void console_destroy( struct object *obj )
{
    struct console *console = (struct console *)obj;
    struct console_output *output;
    struct console_input *input;
    struct screen_buffer *curr;

    assert( obj->ops == &console_ops );

    if (console->server)
    {
        assert( console->server->console == console );
        disconnect_console_server( console->server );
    }

    if (console->active) release_object( console->active );
    console->active = NULL;

    LIST_FOR_EACH_ENTRY( curr, &console->screen_buffers, struct screen_buffer, entry )
        curr->input = NULL;

    LIST_FOR_EACH_ENTRY( input, &console->inputs, struct console_input, entry )
        input->console = NULL;

    LIST_FOR_EACH_ENTRY( output, &console->outputs, struct console_output, entry )
        output->console = NULL;

    if (console->sync) release_object( console->sync );

    free_async_queue( &console->ioctl_q );
    free_async_queue( &console->read_q );
    if (console->fd)
        release_object( console->fd );
}

static struct object *create_console_connection( struct console *console )
{
    struct console_connection *connection;

    if (current->process->console)
    {
        set_error( STATUS_ACCESS_DENIED );
        return NULL;
    }

    if (!(connection = alloc_object( &console_connection_ops ))) return NULL;
    if (!(connection->fd = alloc_pseudo_fd( &console_connection_fd_ops, &connection->obj, 0 )))
    {
        release_object( connection );
        return NULL;
    }

    if (console)
        current->process->console = (struct console *)grab_object( console );

    return &connection->obj;
}

static struct object *console_lookup_name( struct object *obj, struct unicode_str *name,
                                           unsigned int attr, struct object *root )
{
    struct console *console = (struct console *)obj;
    static const WCHAR connectionW[]    = {'C','o','n','n','e','c','t','i','o','n'};
    assert( obj->ops == &console_ops );

    if (!name)
    {
        set_error( STATUS_NOT_FOUND );
        return NULL;
    }

    if (name->len == sizeof(connectionW) && !memcmp( name->str, connectionW, name->len ))
    {
        name->len = 0;
        return create_console_connection( console );
    }

    return NULL;
}

static struct object *console_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static void screen_buffer_dump( struct object *obj, int verbose )
{
    struct screen_buffer *screen_buffer = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );

    fprintf(stderr, "Console screen buffer input=%p\n", screen_buffer->input );
}

static void screen_buffer_destroy( struct object *obj )
{
    struct screen_buffer *screen_buffer = (struct screen_buffer *)obj;

    assert( obj->ops == &screen_buffer_ops );

    if (screen_buffer->input)
    {
        list_remove( &screen_buffer->entry );
        if (screen_buffer->input->server)
            queue_host_ioctl( screen_buffer->input->server, IOCTL_CONDRV_CLOSE_OUTPUT,
                              screen_buffer->id, NULL, NULL );
    }
    if (screen_buffer->fd) release_object( screen_buffer->fd );
    free_async_queue( &screen_buffer->ioctl_q );
}

static int screen_buffer_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct screen_buffer *screen_buffer = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );
    if (!screen_buffer->input) return 0;
    return screen_buffer->input->signaled;
}

static struct object *screen_buffer_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static struct fd *screen_buffer_get_fd( struct object *obj )
{
    struct screen_buffer *screen_buffer = (struct screen_buffer*)obj;
    assert( obj->ops == &screen_buffer_ops );
    if (screen_buffer->fd)
        return (struct fd*)grab_object( screen_buffer->fd );
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return NULL;
}

static void console_server_dump( struct object *obj, int verbose )
{
    assert( obj->ops == &console_server_ops );
    fprintf( stderr, "Console server\n" );
}

static void console_server_destroy( struct object *obj )
{
    struct console_server *server = (struct console_server *)obj;
    assert( obj->ops == &console_server_ops );
    disconnect_console_server( server );
    if (server->sync) release_object( server->sync );
    if (server->fd) release_object( server->fd );
}

static struct object *console_server_lookup_name( struct object *obj, struct unicode_str *name,
                                                  unsigned int attr, struct object *root )
{
    struct console_server *server = (struct console_server*)obj;
    static const WCHAR referenceW[] = {'R','e','f','e','r','e','n','c','e'};
    assert( obj->ops == &console_server_ops );

    if (!name)
    {
        set_error( STATUS_NOT_FOUND );
        return NULL;
    }

    if (name->len == sizeof(referenceW) && !memcmp( name->str, referenceW, name->len ))
    {
        struct screen_buffer *screen_buffer;
        name->len = 0;
        if (server->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }
        if (!(server->console = (struct console *)create_console())) return NULL;
        if (!(screen_buffer = (struct screen_buffer *)create_screen_buffer( server->console )))
        {
            release_object( server->console );
            server->console = NULL;
            return NULL;
        }
        release_object( screen_buffer );
        server->console->server = server;

        if (list_empty( &server->queue )) reset_sync( server->sync );
        return &server->console->obj;
    }

    return NULL;
}

static struct fd *console_server_get_fd( struct object* obj )
{
    struct console_server *server = (struct console_server*)obj;
    assert( obj->ops == &console_server_ops );
    return (struct fd *)grab_object( server->fd );
}

static struct object *console_server_get_sync( struct object *obj )
{
    struct console_server *server = (struct console_server *)obj;
    assert( obj->ops == &console_server_ops );
    return grab_object( server->sync );
}

static struct object *console_server_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static struct object *create_console_server( void )
{
    struct console_server *server;

    if (!(server = alloc_object( &console_server_ops ))) return NULL;
    server->sync       = NULL;
    server->fd         = NULL;
    server->console    = NULL;
    server->busy       = 0;
    server->once_input = 0;
    server->term_fd    = -1;
    list_init( &server->queue );
    list_init( &server->read_queue );

    if (!(server->sync = create_event_sync( 1, 1 ))) goto error;
    if (!(server->fd = alloc_pseudo_fd( &console_server_fd_ops, &server->obj, FILE_SYNCHRONOUS_IO_NONALERT ))) goto error;
    allow_fd_caching(server->fd);
    return &server->obj;

error:
    release_object( server );
    return NULL;
}

static int is_blocking_read_ioctl( unsigned int code )
{
    switch (code)
    {
    case IOCTL_CONDRV_READ_INPUT:
    case IOCTL_CONDRV_READ_CONSOLE:
    case IOCTL_CONDRV_READ_CONSOLE_CONTROL:
    case IOCTL_CONDRV_READ_FILE:
        return 1;
    default:
        return 0;
    }
}

static void console_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console *console = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_CTRL_EVENT:
        {
            const struct condrv_ctrl_event *event = get_req_data();
            process_id_t group;
            if (get_req_data_size() != sizeof(*event))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            group = event->group_id ? event->group_id : current->process->group_id;
            if (!group)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            propagate_console_signal( console, event->event, group );
            return;
        }

    case IOCTL_CONDRV_GET_PROCESS_LIST:
        {
            struct console_process_list cpl;
            if (get_reply_max_size() < sizeof(unsigned int))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }

            cpl.count = 0;
            cpl.size = 0;
            cpl.console = console;
            enum_processes( console_process_list_cb, &cpl );
            if (cpl.count * sizeof(process_id_t) > get_reply_max_size())
            {
                set_reply_data( &cpl.count, sizeof(cpl.count) );
                set_error( STATUS_BUFFER_TOO_SMALL );
                return;
            }

            cpl.size = cpl.count;
            cpl.count = 0;
            if ((cpl.processes = set_reply_data_size( cpl.size * sizeof(process_id_t) )))
                enum_processes( console_process_list_cb, &cpl );
            return;
        }

    default:
        if (!console->server || code >> 16 != FILE_DEVICE_CONSOLE)
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
        queue_host_ioctl( console->server, code, 0, async, &console->ioctl_q );
    }
}

static void console_read( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct console *console = get_fd_user( fd );

    if (!console->server)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    queue_host_ioctl( console->server, IOCTL_CONDRV_READ_FILE, 0, async, &console->ioctl_q );
}

static void console_flush( struct fd *fd, struct async *async )
{
    struct console *console = get_fd_user( fd );

    if (!console->server)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    queue_host_ioctl( console->server, IOCTL_CONDRV_FLUSH, 0, NULL, NULL );
}

static void screen_buffer_write( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct screen_buffer *screen_buffer = get_fd_user( fd );

    if (!screen_buffer->input || !screen_buffer->input->server)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }

    queue_host_ioctl( screen_buffer->input->server, IOCTL_CONDRV_WRITE_FILE,
                      screen_buffer->id, async, &screen_buffer->ioctl_q );
}

static void screen_buffer_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct screen_buffer *screen_buffer = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_ACTIVATE:
        if (!screen_buffer->input)
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }

        set_active_screen_buffer( screen_buffer->input, screen_buffer );
        return;

    default:
        if (!screen_buffer->input || !screen_buffer->input->server || code >> 16 != FILE_DEVICE_CONSOLE ||
            is_blocking_read_ioctl( code ))
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
        queue_host_ioctl( screen_buffer->input->server, code, screen_buffer->id,
                          async, &screen_buffer->ioctl_q );
    }
}

static void console_connection_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console_connection *console_connection = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_BIND_PID:
        {
            struct process *process;
            unsigned int pid;
            if (get_req_data_size() != sizeof(unsigned int))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            if (current->process->console)
            {
                set_error( STATUS_INVALID_HANDLE );
                return;
            }

            pid = *(unsigned int *)get_req_data();
            if (pid == ATTACH_PARENT_PROCESS) pid = current->process->parent_id;
            if (!(process = get_process_from_id( pid ))) return;

            if (process->console)
                current->process->console = (struct console *)grab_object( process->console );
            else set_error( STATUS_ACCESS_DENIED );
            release_object( process );
            return;
        }

    default:
        default_fd_ioctl( console_connection->fd, code, async );
    }
}

static void console_server_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console_server *server = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_CTRL_EVENT:
        {
            const struct condrv_ctrl_event *event = get_req_data();
            if (get_req_data_size() != sizeof(*event))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            if (!server->console)
            {
                set_error( STATUS_INVALID_HANDLE );
                return;
            }
            propagate_console_signal( server->console, event->event, event->group_id );
            return;
        }

    case IOCTL_CONDRV_SETUP_INPUT:
        {
            struct termios term;
            obj_handle_t handle;
            struct file *file;
            int unix_fd;

            if (get_req_data_size() != sizeof(unsigned int) || get_reply_max_size())
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            if (server->term_fd != -1)
            {
                tcsetattr( server->term_fd, TCSANOW, &server->termios );
                close( server->term_fd );
                server->term_fd = -1;
            }
            handle = *(unsigned int *)get_req_data();
            if (!handle) return;
            if (!(file = get_file_obj( current->process, handle, FILE_READ_DATA  )))
            {
                return;
            }
            unix_fd = get_file_unix_fd( file );
            release_object( file );

            if (tcgetattr( unix_fd, &server->termios ))
            {
                file_set_error();
                return;
            }
            term = server->termios;
            term.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
            term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            term.c_cflag &= ~(CSIZE | PARENB);
            term.c_cflag |= CS8;
            term.c_cc[VMIN] = 1;
            term.c_cc[VTIME] = 0;
            if (tcsetattr( unix_fd, TCSANOW, &term ) || (server->term_fd = dup( unix_fd )) == -1)
                file_set_error();
            return;
        }

    default:
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
}

static void console_connection_dump( struct object *obj, int verbose )
{
    fputs( "console connection\n", stderr );
}

static struct fd *console_connection_get_fd( struct object *obj )
{
    struct console_connection *connection = (struct console_connection *)obj;
    return (struct fd *)grab_object( connection->fd );
}

static struct object *console_connection_lookup_name( struct object *obj, struct unicode_str *name,
                                                      unsigned int attr, struct object *root )
{
    static const WCHAR referenceW[] = {'R','e','f','e','r','e','n','c','e'};

    if (name->len == sizeof(referenceW) && !memcmp( name->str, referenceW, name->len ))
    {
        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }
        name->len = 0;
        return grab_object( current->process->console );
    }

    return NULL;
}

static struct object *console_connection_open_file( struct object *obj, unsigned int access,
                                                    unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static int console_connection_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    struct console *console = process->console;

    if (console)
    {
        process->console = NULL;
        release_object( console );
    }
    return 1;
}

static void console_connection_destroy( struct object *obj )
{
    struct console_connection *connection = (struct console_connection *)obj;
    if (connection->fd) release_object( connection->fd );
}

static void console_device_dump( struct object *obj, int verbose )
{
    fputs( "Console device\n", stderr );
}

static struct object *console_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                  unsigned int attr, struct object *root )
{
    static const WCHAR connectionW[]    = {'C','o','n','n','e','c','t','i','o','n'};
    static const WCHAR consoleW[]       = {'C','o','n','s','o','l','e'};
    static const WCHAR current_inW[]    = {'C','u','r','r','e','n','t','I','n'};
    static const WCHAR current_outW[]   = {'C','u','r','r','e','n','t','O','u','t'};
    static const WCHAR inputW[]         = {'I','n','p','u','t'};
    static const WCHAR outputW[]        = {'O','u','t','p','u','t'};
    static const WCHAR screen_bufferW[] = {'S','c','r','e','e','n','B','u','f','f','e','r'};
    static const WCHAR serverW[]        = {'S','e','r','v','e','r'};

    if (name->len == sizeof(current_inW) && !memcmp( name->str, current_inW, name->len ))
    {
        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }
        name->len = 0;
        return grab_object( current->process->console );
    }

    if (name->len == sizeof(current_outW) && !memcmp( name->str, current_outW, name->len ))
    {
        if (!current->process->console || !current->process->console->active)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }
        name->len = 0;
        return grab_object( current->process->console->active );
    }

    if (name->len == sizeof(consoleW) && !memcmp( name->str, consoleW, name->len ))
    {
        name->len = 0;
        return grab_object( obj );
    }

    if (name->len == sizeof(inputW) && !memcmp( name->str, inputW, name->len ))
    {
        struct console_input *console_input;

        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }

        name->len = 0;
        if (!(console_input = alloc_object( &console_input_ops ))) return NULL;
        console_input->fd = alloc_pseudo_fd( &console_input_fd_ops, &console_input->obj,
                                             FILE_SYNCHRONOUS_IO_NONALERT );
        if (!console_input->fd)
        {
            release_object( console_input );
            return NULL;
        }
        console_input->console = current->process->console;
        list_add_head( &current->process->console->inputs, &console_input->entry );
        return &console_input->obj;
    }

    if (name->len == sizeof(outputW) && !memcmp( name->str, outputW, name->len ))
    {
        struct console_output *console_output;

        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }

        name->len = 0;
        if (!(console_output = alloc_object( &console_output_ops ))) return NULL;
        console_output->fd = alloc_pseudo_fd( &console_output_fd_ops, &console_output->obj,
                                             FILE_SYNCHRONOUS_IO_NONALERT );
        if (!console_output->fd)
        {
            release_object( console_output );
            return NULL;
        }
        console_output->console = current->process->console;
        list_add_head( &current->process->console->outputs, &console_output->entry );
        return &console_output->obj;
    }

    if (name->len == sizeof(screen_bufferW) && !memcmp( name->str, screen_bufferW, name->len ))
    {
        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }
        name->len = 0;
        return create_screen_buffer( current->process->console );
    }

    if (name->len == sizeof(serverW) && !memcmp( name->str, serverW, name->len ))
    {
        name->len = 0;
        return create_console_server();
    }

    if (name->len == sizeof(connectionW) && !memcmp( name->str, connectionW, name->len ))
    {
        name->len = 0;
        return create_console_connection( NULL );
    }

    return NULL;
}

static struct object *console_device_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options )
{
    int is_output;
    access = default_map_access( obj, access );
    is_output = access & FILE_WRITE_DATA;
    if (!current->process->console || (is_output && !current->process->console))
    {
        set_error( STATUS_INVALID_HANDLE );
        return NULL;
    }
    if (is_output && (access & FILE_READ_DATA))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    return is_output ? grab_object( current->process->console->active ) : grab_object( current->process->console );
}

static void console_input_dump( struct object *obj, int verbose )
{
    fputs( "console Input device\n", stderr );
}

static int console_input_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct console_input *console_input = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    if (!console_input->console) return 0;
    return console_input->console->signaled;
}

static struct fd *console_input_get_fd( struct object *obj )
{
    struct console_input *console_input = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    return (struct fd *)grab_object( console_input->fd );
}

static struct object *console_input_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static void console_input_destroy( struct object *obj )
{
    struct console_input *console_input = (struct console_input *)obj;

    assert( obj->ops == &console_input_ops );
    if (console_input->fd) release_object( console_input->fd );
    if (console_input->console) list_remove( &console_input->entry );
}

static void console_input_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console *console = current->process->console;

    if (!console)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    console_ioctl( console->fd, code, async );
}

static void console_input_read( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct console *console = current->process->console;

    if (!console)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    console_read( console->fd, async, pos );
}

static void console_input_flush( struct fd *fd, struct async *async )
{
    struct console *console = current->process->console;

    if (!console)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    console_flush( console->fd, async );
}

static void console_output_dump( struct object *obj, int verbose )
{
    fputs( "console Output device\n", stderr );
}

static int console_output_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct console_output *console_output = (struct console_output *)obj;
    assert( obj->ops == &console_output_ops );
    if (!console_output->console) return 0;
    return console_output->console->signaled;
}

static struct fd *console_output_get_fd( struct object *obj )
{
    struct console_output *console_output = (struct console_output *)obj;
    assert( obj->ops == &console_output_ops );
    return (struct fd *)grab_object( console_output->fd );
}

static struct object *console_output_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static void console_output_destroy( struct object *obj )
{
    struct console_output *console_output = (struct console_output *)obj;

    assert( obj->ops == &console_output_ops );
    if (console_output->fd) release_object( console_output->fd );
    if (console_output->console) list_remove( &console_output->entry );
}

static void console_output_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console *console = current->process->console;

    if (!console || !console->active)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    screen_buffer_ioctl( console->active->fd, code, async );
}

static void console_output_write( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct console *console = current->process->console;

    if (!console || !console->active)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    screen_buffer_write( console->active->fd, async, pos );
}

struct object *create_console_device( struct object *root, const struct unicode_str *name,
                                      unsigned int attr, const struct security_descriptor *sd )
{
    return create_named_object( root, &console_device_ops, name, attr, sd );
}

/* retrieve the next pending console ioctl request */
DECL_HANDLER(get_next_console_request)
{
    struct console_host_ioctl *ioctl = NULL, *next;
    struct screen_buffer *screen_buffer;
    struct console_server *server;
    struct console_output *output;
    struct console_input *input;
    struct iosb *iosb = NULL;

    server = (struct console_server *)get_handle_obj( current->process, req->handle, 0, &console_server_ops );
    if (!server) return;

    if (!server->console)
    {
        set_error( STATUS_INVALID_HANDLE );
        release_object( server );
        return;
    }

    if (!server->console->renderer) server->console->renderer = current;

    if (!req->signal)
    {
        server->console->signaled = 0;
        reset_sync( server->console->sync );
    }
    else if (!server->console->signaled)
    {
        server->console->signaled = 1;
        signal_sync( server->console->sync );
        LIST_FOR_EACH_ENTRY( screen_buffer, &server->console->screen_buffers, struct screen_buffer, entry )
            wake_up( &screen_buffer->obj, 0 );
        LIST_FOR_EACH_ENTRY( input, &server->console->inputs, struct console_input, entry )
            wake_up( &input->obj, 0 );
        LIST_FOR_EACH_ENTRY( output, &server->console->outputs, struct console_output, entry )
            wake_up( &output->obj, 0 );
    }

    if (req->read)
    {
        /* set result of current pending ioctl */
        if (list_empty( &server->read_queue ))
        {
            set_error( STATUS_INVALID_HANDLE );
            release_object( server );
            return;
        }

        ioctl = LIST_ENTRY( list_head( &server->read_queue ), struct console_host_ioctl, entry );
        list_remove( &ioctl->entry );
        list_move_tail( &server->queue, &server->read_queue );
    }
    else if (server->busy)
    {
        /* set result of previous ioctl */
        ioctl = LIST_ENTRY( list_head( &server->queue ), struct console_host_ioctl, entry );
        list_remove( &ioctl->entry );
    }

    if (ioctl)
    {
        struct async *async = ioctl->async;
        unsigned int status = req->status;

        if (status == STATUS_PENDING) status = STATUS_INVALID_PARAMETER;
        if (async)
        {
            iosb = async_get_iosb( async );
            if (iosb->status == STATUS_PENDING)
            {
                data_size_t out_size = min( iosb->out_size, get_req_data_size() );
                data_size_t result = ioctl->code == IOCTL_CONDRV_WRITE_FILE ? iosb->in_size : out_size;
                async_request_complete_alloc( async, status, result, out_size, get_req_data() );
            }

            release_object( async );
        }
        free( ioctl );
        if (iosb) release_object( iosb );

        if (req->read) goto done;
        server->busy = 0;
    }

    /* if we have a blocking read ioctl in queue head and previous blocking read is still waiting,
     * move it to read queue for execution after current read is complete. move all blocking
     * ioctl at the same time to preserve their order. */
    if (!list_empty( &server->queue ) && !list_empty( &server->read_queue ))
    {
        ioctl = LIST_ENTRY( list_head( &server->queue ), struct console_host_ioctl, entry );
        if (is_blocking_read_ioctl( ioctl->code ))
        {
            LIST_FOR_EACH_ENTRY_SAFE( ioctl, next, &server->queue, struct console_host_ioctl, entry )
            {
                if (!is_blocking_read_ioctl( ioctl->code )) continue;
                list_remove( &ioctl->entry );
                list_add_tail( &server->read_queue, &ioctl->entry );
            }
        }
    }

    /* return the next ioctl */
    if (!list_empty( &server->queue ))
    {
        ioctl = LIST_ENTRY( list_head( &server->queue ), struct console_host_ioctl, entry );
        iosb = ioctl->async ? async_get_iosb( ioctl->async ) : NULL;

        if (!iosb || get_reply_max_size() >= iosb->in_size)
        {
            reply->code   = ioctl->code;
            reply->output = ioctl->output;

            if (iosb)
            {
                reply->out_size = iosb->out_size;
                set_reply_data_ptr( iosb->in_data, iosb->in_size );
                iosb->in_data = NULL;
            }

            if (is_blocking_read_ioctl( ioctl->code ))
            {
                list_remove( &ioctl->entry );
                assert( list_empty( &server->read_queue ));
                list_add_tail( &server->read_queue, &ioctl->entry );
            }
            else server->busy = 1;
        }
        else
        {
            reply->out_size = iosb->in_size;
            set_error( STATUS_BUFFER_OVERFLOW );
        }
        if (iosb) release_object( iosb );
    }
    else
    {
        set_error( STATUS_PENDING );
    }

done:
    if (list_empty( &server->queue )) reset_sync( server->sync );
    release_object( server );
}

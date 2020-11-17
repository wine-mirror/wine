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
#include "wine/port.h"

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

struct console_input
{
    struct object                obj;           /* object header */
    int                          signaled;      /* is console signaled */
    int                          num_proc;      /* number of processes attached to this console */
    struct thread               *renderer;      /* console renderer thread */
    struct screen_buffer        *active;        /* active screen buffer */
    struct console_server       *server;        /* console server object */
    unsigned int                 last_id;       /* id of last created console buffer */
    struct fd                   *fd;            /* for bare console, attached input fd */
    struct async_queue           ioctl_q;       /* ioctl queue */
    struct async_queue           read_q;        /* read queue */
};

static void console_input_dump( struct object *obj, int verbose );
static void console_input_destroy( struct object *obj );
static int console_input_signaled( struct object *obj, struct wait_queue_entry *entry );
static struct fd *console_input_get_fd( struct object *obj );
static struct object *console_input_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr );
static struct object *console_input_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops console_input_ops =
{
    sizeof(struct console_input),     /* size */
    console_input_dump,               /* dump */
    no_get_type,                      /* get_type */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    console_input_signaled,           /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_input_get_fd,             /* get_fd */
    default_fd_map_access,            /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    console_input_lookup_name,        /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    console_input_open_file,          /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_input_destroy             /* destroy */
};

static enum server_fd_type console_get_fd_type( struct fd *fd );
static int console_input_flush( struct fd *fd, struct async *async );
static int console_input_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_input_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    console_input_flush,          /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    console_input_ioctl,          /* ioctl */
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
    struct object         obj;         /* object header */
    struct fd            *fd;          /* pseudo-fd for ioctls */
    struct console_input *console;     /* attached console */
    struct list           queue;       /* ioctl queue */
    struct list           read_queue;  /* blocking read queue */
    int                   busy;        /* flag if server processing an ioctl */
    int                   term_fd;     /* UNIX terminal fd */
    struct termios        termios;     /* original termios */
};

static void console_server_dump( struct object *obj, int verbose );
static void console_server_destroy( struct object *obj );
static int console_server_signaled( struct object *obj, struct wait_queue_entry *entry );
static struct fd *console_server_get_fd( struct object *obj );
static struct object *console_server_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr );
static struct object *console_server_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options );

static const struct object_ops console_server_ops =
{
    sizeof(struct console_server),    /* size */
    console_server_dump,              /* dump */
    no_get_type,                      /* get_type */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    console_server_signaled,          /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_server_get_fd,            /* get_fd */
    default_fd_map_access,            /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    console_server_lookup_name,       /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    console_server_open_file,         /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    fd_close_handle,                  /* close_handle */
    console_server_destroy            /* destroy */
};

static int console_server_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

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
    struct console_input *input;         /* associated console input */
    unsigned int          id;            /* buffer id */
    struct fd            *fd;            /* for bare console, attached output fd */
    struct async_queue    ioctl_q;       /* ioctl queue */
};

static void screen_buffer_dump( struct object *obj, int verbose );
static void screen_buffer_destroy( struct object *obj );
static int screen_buffer_add_queue( struct object *obj, struct wait_queue_entry *entry );
static struct fd *screen_buffer_get_fd( struct object *obj );
static struct object *screen_buffer_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops screen_buffer_ops =
{
    sizeof(struct screen_buffer),     /* size */
    screen_buffer_dump,               /* dump */
    no_get_type,                      /* get_type */
    screen_buffer_add_queue,          /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    no_signal,                        /* signal */
    screen_buffer_get_fd,             /* get_fd */
    default_fd_map_access,            /* map_access */
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

static int screen_buffer_write( struct fd *fd, struct async *async, file_pos_t pos );
static int screen_buffer_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops screen_buffer_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    screen_buffer_write,          /* write */
    no_fd_flush,                  /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    screen_buffer_ioctl,          /* ioctl */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

static struct object_type *console_device_get_type( struct object *obj );
static void console_device_dump( struct object *obj, int verbose );
static struct object *console_device_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr );
static struct object *console_device_open_file( struct object *obj, unsigned int access,
                                                unsigned int sharing, unsigned int options );

static const struct object_ops console_device_ops =
{
    sizeof(struct object),            /* size */
    console_device_dump,              /* dump */
    console_device_get_type,          /* get_type */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    no_get_fd,                        /* get_fd */
    default_fd_map_access,            /* map_access */
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

static void input_device_dump( struct object *obj, int verbose );
static struct object *input_device_open_file( struct object *obj, unsigned int access,
                                              unsigned int sharing, unsigned int options );
static int input_device_add_queue( struct object *obj, struct wait_queue_entry *entry );
static struct fd *input_device_get_fd( struct object *obj );

static const struct object_ops input_device_ops =
{
    sizeof(struct object),            /* size */
    input_device_dump,                /* dump */
    console_device_get_type,          /* get_type */
    input_device_add_queue,           /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    input_device_get_fd,              /* get_fd */
    no_map_access,                    /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    input_device_open_file,           /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    no_destroy                        /* destroy */
};

static void output_device_dump( struct object *obj, int verbose );
static int output_device_add_queue( struct object *obj, struct wait_queue_entry *entry );
static struct fd *output_device_get_fd( struct object *obj );
static struct object *output_device_open_file( struct object *obj, unsigned int access,
                                              unsigned int sharing, unsigned int options );

static const struct object_ops output_device_ops =
{
    sizeof(struct object),            /* size */
    output_device_dump,               /* dump */
    console_device_get_type,          /* get_type */
    output_device_add_queue,          /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    output_device_get_fd,             /* get_fd */
    no_map_access,                    /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_get_full_name,                 /* get_full_name */
    no_lookup_name,                   /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    output_device_open_file,          /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    no_destroy                        /* destroy */
};

struct console_connection
{
    struct object         obj;         /* object header */
    struct fd            *fd;          /* pseudo-fd for ioctls */
};

static void console_connection_dump( struct object *obj, int verbose );
static struct fd *console_connection_get_fd( struct object *obj );
static struct object *console_connection_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr );
static struct object *console_connection_open_file( struct object *obj, unsigned int access,
                                                    unsigned int sharing, unsigned int options );
static int console_connection_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void console_connection_destroy( struct object *obj );

static const struct object_ops console_connection_ops =
{
    sizeof(struct console_connection),/* size */
    console_connection_dump,          /* dump */
    console_device_get_type,          /* get_type */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_connection_get_fd,        /* get_fd */
    no_map_access,                    /* map_access */
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

static int console_connection_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

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
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

static struct list screen_buffer_list = LIST_INIT(screen_buffer_list);

static int console_input_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct console_input *console = (struct console_input*)obj;
    return console->signaled;
}

static struct fd *console_input_get_fd( struct object* obj )
{
    struct console_input *console_input = (struct console_input*)obj;
    assert( obj->ops == &console_input_ops );
    return (struct fd *)grab_object( console_input->fd );
}

static enum server_fd_type console_get_fd_type( struct fd *fd )
{
    return FD_TYPE_CHAR;
}

static struct object *create_console_input(void)
{
    struct console_input *console_input;

    if (!(console_input = alloc_object( &console_input_ops )))
        return NULL;

    console_input->renderer      = NULL;
    console_input->signaled      = 0;
    console_input->num_proc      = 0;
    console_input->active        = NULL;
    console_input->server        = NULL;
    console_input->fd            = NULL;
    console_input->last_id       = 0;
    init_async_queue( &console_input->ioctl_q );
    init_async_queue( &console_input->read_q );

    console_input->fd = alloc_pseudo_fd( &console_input_fd_ops, &console_input->obj,
                                         FILE_SYNCHRONOUS_IO_NONALERT );
    if (!console_input->fd)
    {
        release_object( console_input );
        return NULL;
    }
    allow_fd_caching( console_input->fd );
    return &console_input->obj;
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
    wake_up( &server->obj, 0 );
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
        wake_up( &server->obj, 0 );
    }
}

static void set_active_screen_buffer( struct console_input *console_input, struct screen_buffer *screen_buffer )
{
    if (console_input->active == screen_buffer) return;
    if (console_input->active) release_object( console_input->active );
    console_input->active = (struct screen_buffer *)grab_object( screen_buffer );

    if (console_input->server) queue_host_ioctl( console_input->server, IOCTL_CONDRV_ACTIVATE,
                                                 screen_buffer->id, NULL, NULL );
}

static struct object *create_console_output( struct console_input *console_input )
{
    struct screen_buffer *screen_buffer;

    if (console_input->last_id == ~0)
    {
        set_error( STATUS_NO_MEMORY );
        return NULL;
    }

    if (!(screen_buffer = alloc_object( &screen_buffer_ops )))
        return NULL;

    screen_buffer->id    = ++console_input->last_id;
    screen_buffer->input = console_input;
    init_async_queue( &screen_buffer->ioctl_q );
    list_add_head( &screen_buffer_list, &screen_buffer->entry );

    screen_buffer->fd = alloc_pseudo_fd( &screen_buffer_fd_ops, &screen_buffer->obj,
                                         FILE_SYNCHRONOUS_IO_NONALERT );
    if (!screen_buffer->fd)
    {
        release_object( screen_buffer );
        return NULL;
    }
    allow_fd_caching(screen_buffer->fd);

    if (console_input->server) queue_host_ioctl( console_input->server, IOCTL_CONDRV_INIT_OUTPUT,
                                                 screen_buffer->id, NULL, NULL );
    if (!console_input->active) set_active_screen_buffer( console_input, screen_buffer );
    return &screen_buffer->obj;
}

/* free the console for this process */
int free_console( struct process *process )
{
    struct console_input* console = process->console;

    if (!console) return 0;

    process->console = NULL;
    console->num_proc--;
    release_object( console );

    return 1;
}

/* let process inherit the console from parent... this handle two cases :
 *	1/ generic console inheritance
 *	2/ parent is a renderer which launches process, and process should attach to the console
 *	   rendered by parent
 */
obj_handle_t inherit_console( struct thread *parent_thread, obj_handle_t handle, struct process *process,
                              obj_handle_t hconin )
{
    struct console_input *console = NULL;

    if (handle) return duplicate_handle( current->process, handle, process, 0, 0, DUP_HANDLE_SAME_ACCESS );

    /* if parent is a renderer, then attach current process to its console
     * a bit hacky....
     */
    if (hconin && parent_thread)
    {
        /* FIXME: should we check some access rights ? */
        if (!(console = (struct console_input *)get_handle_obj( parent_thread->process, hconin,
                                                                0, &console_input_ops )))
            clear_error();  /* ignore error */
    }
    if (!console) return 0;

    process->console = console;
    console->num_proc++;
    return alloc_handle( process, process->console,
                         SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE, 0 );
}

struct thread *console_get_renderer( struct console_input *console )
{
    return console->renderer;
}

struct console_signal_info
{
    struct console_input        *console;
    process_id_t                 group;
    int                          signal;
};

static int propagate_console_signal_cb(struct process *process, void *user)
{
    struct console_signal_info* csi = (struct console_signal_info*)user;

    if (process->console == csi->console && process->running_threads &&
        (!csi->group || process->group_id == csi->group))
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

static void propagate_console_signal( struct console_input *console,
                                      int sig, process_id_t group_id )
{
    struct console_signal_info csi;

    if (!console)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    /* FIXME: should support the other events (like CTRL_BREAK) */
    if (sig != CTRL_C_EVENT)
    {
        set_error( STATUS_NOT_IMPLEMENTED );
        return;
    }
    csi.console = console;
    csi.signal  = SIGINT;
    csi.group   = group_id;

    enum_processes(propagate_console_signal_cb, &csi);
}

/* dumb dump */
static void console_input_dump( struct object *obj, int verbose )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    fprintf( stderr, "Console input active=%p server=%p\n",
             console->active, console->server );
}

static void console_input_destroy( struct object *obj )
{
    struct console_input *console_in = (struct console_input *)obj;
    struct screen_buffer *curr;

    assert( obj->ops == &console_input_ops );

    if (console_in->server)
    {
        assert( console_in->server->console == console_in );
        disconnect_console_server( console_in->server );
    }

    if (console_in->active) release_object( console_in->active );
    console_in->active = NULL;

    LIST_FOR_EACH_ENTRY( curr, &screen_buffer_list, struct screen_buffer, entry )
    {
        if (curr->input == console_in) curr->input = NULL;
    }

    free_async_queue( &console_in->ioctl_q );
    free_async_queue( &console_in->read_q );
    if (console_in->fd)
        release_object( console_in->fd );
}

static struct object *create_console_connection( struct console_input *console )
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
    {
        current->process->console = (struct console_input *)grab_object( console );
        console->num_proc++;
    }

    return &connection->obj;
}

static struct object *console_input_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr )
{
    struct console_input *console = (struct console_input *)obj;
    static const WCHAR connectionW[]    = {'C','o','n','n','e','c','t','i','o','n'};
    assert( obj->ops == &console_input_ops );

    if (name->len == sizeof(connectionW) && !memcmp( name->str, connectionW, name->len ))
    {
        name->len = 0;
        return create_console_connection( console );
    }

    return NULL;
}

static struct object *console_input_open_file( struct object *obj, unsigned int access,
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

    list_remove( &screen_buffer->entry );
    if (screen_buffer->input && screen_buffer->input->server)
        queue_host_ioctl( screen_buffer->input->server, IOCTL_CONDRV_CLOSE_OUTPUT,
                          screen_buffer->id, NULL, NULL );
    if (screen_buffer->fd) release_object( screen_buffer->fd );
    free_async_queue( &screen_buffer->ioctl_q );
}

static struct object *screen_buffer_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static int screen_buffer_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct screen_buffer *screen_buffer = (struct screen_buffer*)obj;
    if (!screen_buffer->input)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    return add_queue( &screen_buffer->input->obj, entry );
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
    if (server->fd) release_object( server->fd );
}

static struct object *console_server_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr )
{
    struct console_server *server = (struct console_server*)obj;
    static const WCHAR referenceW[] = {'R','e','f','e','r','e','n','c','e'};
    assert( obj->ops == &console_server_ops );

    if (name->len == sizeof(referenceW) && !memcmp( name->str, referenceW, name->len ))
    {
        struct screen_buffer *screen_buffer;
        name->len = 0;
        if (server->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }
        if (!(server->console = (struct console_input *)create_console_input())) return NULL;
        if (!(screen_buffer = (struct screen_buffer *)create_console_output( server->console )))
        {
            release_object( server->console );
            server->console = NULL;
            return NULL;
        }
        release_object( screen_buffer );
        server->console->server = server;

        return &server->console->obj;
    }

    return NULL;
}

static int console_server_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct console_server *server = (struct console_server*)obj;
    assert( obj->ops == &console_server_ops );
    return !server->console || !list_empty( &server->queue );
}

static struct fd *console_server_get_fd( struct object* obj )
{
    struct console_server *server = (struct console_server*)obj;
    assert( obj->ops == &console_server_ops );
    return (struct fd *)grab_object( server->fd );
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
    server->console = NULL;
    server->busy    = 0;
    server->term_fd = -1;
    list_init( &server->queue );
    list_init( &server->read_queue );
    server->fd = alloc_pseudo_fd( &console_server_fd_ops, &server->obj, FILE_SYNCHRONOUS_IO_NONALERT );
    if (!server->fd)
    {
        release_object( server );
        return NULL;
    }
    allow_fd_caching(server->fd);

    return &server->obj;
}

static int is_blocking_read_ioctl( unsigned int code )
{
    return code == IOCTL_CONDRV_READ_INPUT || code == IOCTL_CONDRV_READ_CONSOLE;
}

static int console_input_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console_input *console = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_CTRL_EVENT:
        {
            const struct condrv_ctrl_event *event = get_req_data();
            process_id_t group;
            if (get_req_data_size() != sizeof(*event))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            group = event->group_id ? event->group_id : current->process->group_id;
            if (!group)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            propagate_console_signal( console, event->event, group );
            return !get_error();
        }

    default:
        if (!console->server || code >> 16 != FILE_DEVICE_CONSOLE)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }
        return queue_host_ioctl( console->server, code, 0, async, &console->ioctl_q );
    }
}

static int console_input_flush( struct fd *fd, struct async *async )
{
    struct console_input *console = get_fd_user( fd );

    if (!console->server)
    {
        set_error( STATUS_INVALID_HANDLE );
        return 0;
    }
    return queue_host_ioctl( console->server, IOCTL_CONDRV_FLUSH, 0, NULL, NULL );
}

static int screen_buffer_write( struct fd *fd, struct async *async, file_pos_t pos )
{
    struct screen_buffer *screen_buffer = get_fd_user( fd );
    struct iosb *iosb;

    if (!screen_buffer->input || !screen_buffer->input->server)
    {
        set_error( STATUS_INVALID_HANDLE );
        return 0;
    }

    if (!queue_host_ioctl( screen_buffer->input->server, IOCTL_CONDRV_WRITE_FILE,
                           screen_buffer->id, async, &screen_buffer->ioctl_q ))
        return 0;

    /* we can't use default async handling, because write result is not
     * compatible with ioctl result */
    iosb = async_get_iosb( async );
    iosb->status = STATUS_SUCCESS;
    iosb->result = iosb->in_size;
    async_terminate( async, iosb->result ? STATUS_ALERTED : STATUS_SUCCESS );
    release_object( iosb );
    return 1;
}

static int screen_buffer_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct screen_buffer *screen_buffer = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_ACTIVATE:
        if (!screen_buffer->input)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }

        set_active_screen_buffer( screen_buffer->input, screen_buffer );
        return 1;

    default:
        if (!screen_buffer->input || !screen_buffer->input->server || code >> 16 != FILE_DEVICE_CONSOLE ||
            is_blocking_read_ioctl( code ))
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }
        return queue_host_ioctl( screen_buffer->input->server, code, screen_buffer->id,
                                 async, &screen_buffer->ioctl_q );
    }
}

static int console_connection_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
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
                return 0;
            }
            if (current->process->console)
            {
                set_error( STATUS_INVALID_HANDLE );
                return 0;
            }

            pid = *(unsigned int *)get_req_data();
            if (pid == ATTACH_PARENT_PROCESS) pid = current->process->parent_id;
            if (!(process = get_process_from_id( pid ))) return 0;

            if (process->console)
            {
                current->process->console = (struct console_input *)grab_object( process->console );
                process->console->num_proc++;
            }
            else set_error( STATUS_ACCESS_DENIED );
            release_object( process );
            return !get_error();
        }

    default:
        return default_fd_ioctl( console_connection->fd, code, async );
    }
}

static int console_server_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
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
                return 0;
            }
            if (!server->console)
            {
                set_error( STATUS_INVALID_HANDLE );
                return 0;
            }
            propagate_console_signal( server->console, event->event, event->group_id );
            return !get_error();
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
                return 0;
            }
            if (server->term_fd != -1)
            {
                tcsetattr( server->term_fd, TCSANOW, &server->termios );
                close( server->term_fd );
                server->term_fd = -1;
            }
            handle = *(unsigned int *)get_req_data();
            if (!handle) return 1;
            if (!(file = get_file_obj( current->process, handle, FILE_READ_DATA  )))
            {
                return 0;
            }
            unix_fd = get_file_unix_fd( file );
            release_object( file );

            if (tcgetattr( unix_fd, &server->termios ))
            {
                file_set_error();
                return 0;
            }
            term = server->termios;
            term.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
            term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            term.c_cflag &= ~(CSIZE | PARENB);
            term.c_cflag |= CS8;
            term.c_cc[VMIN] = 1;
            term.c_cc[VTIME] = 0;
            if (tcsetattr( unix_fd, TCSANOW, &term ) || (server->term_fd = dup( unix_fd )) == -1)
            {
                file_set_error();
                return 0;
            }
            return 1;
        }

    default:
        set_error( STATUS_INVALID_HANDLE );
        return 0;
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

static struct object *console_connection_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr )
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
    free_console( process );
    return 1;
}

static void console_connection_destroy( struct object *obj )
{
    struct console_connection *connection = (struct console_connection *)obj;
    if (connection->fd) release_object( connection->fd );
}

static struct object_type *console_device_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','e','v','i','c','e'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static void console_device_dump( struct object *obj, int verbose )
{
    fputs( "Console device\n", stderr );
}

static struct object *console_device_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr )
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
        name->len = 0;
        return alloc_object( &input_device_ops );
    }

    if (name->len == sizeof(outputW) && !memcmp( name->str, outputW, name->len ))
    {
        name->len = 0;
        return alloc_object( &output_device_ops );
    }

    if (name->len == sizeof(screen_bufferW) && !memcmp( name->str, screen_bufferW, name->len ))
    {
        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }
        name->len = 0;
        return create_console_output( current->process->console );
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
    access = default_fd_map_access( obj, access );
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

static void input_device_dump( struct object *obj, int verbose )
{
    fputs( "console Input device\n", stderr );
}

static int input_device_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    if (!current->process->console)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    return add_queue( &current->process->console->obj, entry );
}

static struct fd *input_device_get_fd( struct object *obj )
{
    if (!current->process->console)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    return get_obj_fd( &current->process->console->obj );
}

static struct object *input_device_open_file( struct object *obj, unsigned int access,
                                              unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static void output_device_dump( struct object *obj, int verbose )
{
    fputs( "console Output device\n", stderr );
}

static int output_device_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    if (!current->process->console || !current->process->console->active)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    return add_queue( &current->process->console->obj, entry );
}

static struct fd *output_device_get_fd( struct object *obj )
{
    if (!current->process->console || !current->process->console->active)
    {
        set_error( STATUS_ACCESS_DENIED );
        return NULL;
    }

    return get_obj_fd( &current->process->console->active->obj );
}

static struct object *output_device_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
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
    struct console_server *server;
    struct iosb *iosb = NULL;

    server = (struct console_server *)get_handle_obj( current->process, req->handle, 0, &console_server_ops );
    if (!server) return;

    if (!server->console)
    {
        set_error( STATUS_INVALID_HANDLE );
        release_object( server );
        return;
    }

    if (!req->signal) server->console->signaled = 0;
    else if (!server->console->signaled)
    {
        server->console->signaled = 1;
        wake_up( &server->console->obj, 0 );
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
        unsigned int status = req->status;
        if (status == STATUS_PENDING) status = STATUS_INVALID_PARAMETER;
        if (ioctl->async)
        {
            iosb = async_get_iosb( ioctl->async );
            if (iosb->status == STATUS_PENDING)
            {
                iosb->status = status;
                iosb->out_size = min( iosb->out_size, get_req_data_size() );
                if (iosb->out_size)
                {
                    if ((iosb->out_data = memdup( get_req_data(), iosb->out_size )))
                    {
                        iosb->result = iosb->out_size;
                        status = STATUS_ALERTED;
                    }
                    else if (!status)
                    {
                        iosb->status = STATUS_NO_MEMORY;
                        iosb->out_size = 0;
                    }
                }
            }
            else
            {
                release_object( ioctl->async );
                ioctl->async = NULL;
            }
        }
        console_host_ioctl_terminate( ioctl, status );
        if (iosb) release_object( iosb );

        if (req->read)
        {
            release_object( server );
            return;
        }
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

    release_object( server );
}

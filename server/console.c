/*
 * Server-side console management
 *
 * Copyright (C) 1998 Alexandre Julliard
 *               2001 Eric Pouech
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
struct console_input_events;

struct history_line
{
    data_size_t len;
    WCHAR       text[1];
};

struct console_input
{
    struct object                obj;           /* object header */
    int                          num_proc;      /* number of processes attached to this console */
    struct thread               *renderer;      /* console renderer thread */
    int                          mode;          /* input mode */
    struct screen_buffer        *active;        /* active screen buffer */
    int                          recnum;        /* number of input records */
    INPUT_RECORD                *records;       /* input records */
    struct console_input_events *evt;           /* synchronization event with renderer */
    WCHAR                       *title;         /* console title */
    data_size_t                  title_len;     /* length of console title */
    struct history_line        **history;       /* lines history */
    int                          history_size;  /* number of entries in history array */
    int                          history_index; /* number of used entries in history array */
    int                          history_mode;  /* mode of history (non zero means remove doubled strings */
    int                          edition_mode;  /* index to edition mode flavors */
    int                          input_cp;      /* console input codepage */
    int                          output_cp;     /* console output codepage */
    user_handle_t                win;           /* window handle if backend supports it */
    struct event                *event;         /* event to wait on for input queue */
    struct fd                   *fd;            /* for bare console, attached input fd */
    struct async_queue           read_q;        /* read queue */
};

static void console_input_dump( struct object *obj, int verbose );
static void console_input_destroy( struct object *obj );
static struct fd *console_input_get_fd( struct object *obj );
static struct object *console_input_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops console_input_ops =
{
    sizeof(struct console_input),     /* size */
    console_input_dump,               /* dump */
    no_get_type,                      /* get_type */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_input_get_fd,             /* get_fd */
    default_fd_map_access,            /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    console_input_open_file,          /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_input_destroy             /* destroy */
};

static enum server_fd_type console_get_fd_type( struct fd *fd );
static int console_input_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_input_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    no_fd_flush,                  /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    console_input_ioctl,          /* ioctl */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

static void console_input_events_dump( struct object *obj, int verbose );
static void console_input_events_destroy( struct object *obj );
static struct fd *console_input_events_get_fd( struct object *obj );
static struct object *console_input_events_open_file( struct object *obj, unsigned int access,
                                                      unsigned int sharing, unsigned int options );

struct console_input_events
{
    struct object                  obj;         /* object header */
    struct fd                     *fd;          /* pseudo-fd for ioctls */
    struct console_input          *console;     /* attached console */
    int                            num_alloc;   /* number of allocated events */
    int                            num_used;    /* number of actually used events */
    struct condrv_renderer_event  *events;
    struct async_queue             read_q;      /* read queue */
};

static const struct object_ops console_input_events_ops =
{
    sizeof(struct console_input_events), /* size */
    console_input_events_dump,        /* dump */
    no_get_type,                      /* get_type */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    NULL,                             /* signaled */
    no_satisfied,                     /* satisfied */
    no_signal,                        /* signal */
    console_input_events_get_fd,      /* get_fd */
    default_fd_map_access,            /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    console_input_events_open_file,   /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    console_input_events_destroy      /* destroy */
};

static int console_input_events_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops console_input_events_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    no_fd_flush,                  /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    console_input_events_ioctl,   /* ioctl */
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
    unsigned int          mode;          /* output mode */
    int                   cursor_size;   /* size of cursor (percentage filled) */
    int                   cursor_visible;/* cursor visibility flag */
    int                   cursor_x;      /* position of cursor */
    int                   cursor_y;      /* position of cursor */
    int                   width;         /* size (w-h) of the screen buffer */
    int                   height;
    int                   max_width;     /* size (w-h) of the window given font size */
    int                   max_height;
    char_info_t          *data;          /* the data for each cell - a width x height matrix */
    unsigned short        attr;          /* default fill attributes (screen colors) */
    unsigned short        popup_attr;    /* pop-up color attributes */
    unsigned int          color_map[16]; /* color table */
    rectangle_t           win;           /* current visible window on the screen buffer *
					  * as seen in wineconsole */
    struct font_info      font;          /* console font information */
    struct fd            *fd;            /* for bare console, attached output fd */
};

static void screen_buffer_dump( struct object *obj, int verbose );
static void screen_buffer_destroy( struct object *obj );
static struct fd *screen_buffer_get_fd( struct object *obj );
static struct object *screen_buffer_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops screen_buffer_ops =
{
    sizeof(struct screen_buffer),     /* size */
    screen_buffer_dump,               /* dump */
    no_get_type,                      /* get_type */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    no_signal,                        /* signal */
    screen_buffer_get_fd,             /* get_fd */
    default_fd_map_access,            /* map_access */
    default_get_sd,                   /* get_sd */
    default_set_sd,                   /* set_sd */
    no_lookup_name,                   /* lookup_name */
    no_link_name,                     /* link_name */
    NULL,                             /* unlink_name */
    screen_buffer_open_file,          /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    screen_buffer_destroy             /* destroy */
};

static int screen_buffer_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );

static const struct fd_ops screen_buffer_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    console_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
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
    console_device_lookup_name,       /* lookup_name */
    directory_link_name,              /* link_name */
    default_unlink_name,              /* unlink_name */
    console_device_open_file,         /* open_file */
    no_kernel_obj_list,               /* get_kernel_obj_list */
    no_close_handle,                  /* close_handle */
    no_destroy                        /* destroy */
};

static struct list screen_buffer_list = LIST_INIT(screen_buffer_list);

static const char_info_t empty_char_info = { ' ', 0x000f };  /* white on black space */

static int console_input_is_bare( struct console_input* cin )
{
    return cin->evt == NULL;
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

/* dumps the renderer events of a console */
static void console_input_events_dump( struct object *obj, int verbose )
{
    struct console_input_events *evts = (struct console_input_events *)obj;
    assert( obj->ops == &console_input_events_ops );
    fprintf( stderr, "Console input events: %d/%d events\n",
	     evts->num_used, evts->num_alloc );
}

/* destroys the renderer events of a console */
static void console_input_events_destroy( struct object *obj )
{
    struct console_input_events *evts = (struct console_input_events *)obj;
    assert( obj->ops == &console_input_events_ops );
    if (evts->console) evts->console->evt = NULL;
    free_async_queue( &evts->read_q );
    if (evts->fd) release_object( evts->fd );
    free( evts->events );
}

static struct fd *console_input_events_get_fd( struct object* obj )
{
    struct console_input_events *evts = (struct console_input_events*)obj;
    assert( obj->ops == &console_input_events_ops );
    return (struct fd*)grab_object( evts->fd );
}

static struct object *console_input_events_open_file( struct object *obj, unsigned int access,
                                                      unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

/* retrieves events from the console's renderer events list */
static int get_renderer_events( struct console_input_events* evts, struct async *async )
{
    struct iosb *iosb = async_get_iosb( async );
    data_size_t num;

    num = min( iosb->out_size / sizeof(evts->events[0]), evts->num_used );
    if (num && !(iosb->out_data = malloc( num * sizeof(evts->events[0] ))))
    {
        async_terminate( async, STATUS_NO_MEMORY );
        release_object( iosb );
        return 0;
    }

    iosb->status = STATUS_SUCCESS;
    iosb->out_size = iosb->result = num * sizeof(evts->events[0]);
    if (num) memcpy( iosb->out_data, evts->events, iosb->result );
    release_object( iosb );
    async_terminate( async, STATUS_ALERTED );

    if (num && num < evts->num_used)
    {
        memmove( &evts->events[0], &evts->events[num],
                 (evts->num_used - num) * sizeof(evts->events[0]) );
    }
    evts->num_used -= num;
    return 1;
}

/* add an event to the console's renderer events list */
static void console_input_events_append( struct console_input* console,
					 struct condrv_renderer_event* evt)
{
    struct console_input_events* evts;
    int collapsed = FALSE;
    struct async *async;

    if (!(evts = console->evt)) return;
    /* to be done even when evt has been generated by the renderer ? */

    /* try to collapse evt into current queue's events */
    if (evts->num_used)
    {
        struct condrv_renderer_event* last = &evts->events[evts->num_used - 1];

        if (last->event == CONSOLE_RENDERER_UPDATE_EVENT &&
            evt->event == CONSOLE_RENDERER_UPDATE_EVENT)
        {
            /* if two update events overlap, collapse them into a single one */
            if (last->u.update.bottom + 1 >= evt->u.update.top &&
                evt->u.update.bottom + 1 >= last->u.update.top)
            {
                last->u.update.top = min(last->u.update.top, evt->u.update.top);
                last->u.update.bottom = max(last->u.update.bottom, evt->u.update.bottom);
                collapsed = TRUE;
            }
        }
    }
    if (!collapsed)
    {
        if (evts->num_used == evts->num_alloc)
        {
            evts->num_alloc += 16;
            evts->events = realloc( evts->events, evts->num_alloc * sizeof(*evt) );
            assert(evts->events);
        }
        evts->events[evts->num_used++] = *evt;
    }
    while (evts->num_used && (async = find_pending_async( &evts->read_q )))
    {
        get_renderer_events( evts, async );
        release_object( async );
    }
}

static struct object *create_console_input_events(void)
{
    struct console_input_events*	evt;

    if (!(evt = alloc_object( &console_input_events_ops ))) return NULL;
    evt->console = NULL;
    evt->num_alloc = evt->num_used = 0;
    evt->events = NULL;
    init_async_queue( &evt->read_q );
    if (!(evt->fd = alloc_pseudo_fd( &console_input_events_fd_ops, &evt->obj, 0 )))
    {
        release_object( evt );
        return NULL;
    }
    return &evt->obj;
}

static struct object *create_console_input( int fd )
{
    struct console_input *console_input;

    if (!(console_input = alloc_object( &console_input_ops )))
    {
        if (fd != -1) close( fd );
        return NULL;
    }
    console_input->renderer      = NULL;
    console_input->mode          = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                                   ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE |
                                   ENABLE_EXTENDED_FLAGS;
    console_input->num_proc      = 0;
    console_input->active        = NULL;
    console_input->recnum        = 0;
    console_input->records       = NULL;
    console_input->evt           = NULL;
    console_input->title         = NULL;
    console_input->title_len     = 0;
    console_input->history_size  = 50;
    console_input->history       = calloc( console_input->history_size, sizeof(*console_input->history) );
    console_input->history_index = 0;
    console_input->history_mode  = 0;
    console_input->edition_mode  = 0;
    console_input->input_cp      = 0;
    console_input->output_cp     = 0;
    console_input->win           = 0;
    console_input->event         = create_event( NULL, NULL, 0, 1, 0, NULL );
    console_input->fd            = NULL;
    init_async_queue( &console_input->read_q );

    if (!console_input->history || !console_input->event)
    {
        if (fd != -1) close( fd );
        console_input->history_size = 0;
        release_object( console_input );
        return NULL;
    }
    if (fd != -1) /* bare console */
    {
        console_input->fd = create_anonymous_fd( &console_input_fd_ops, fd, &console_input->obj,
                                                 FILE_SYNCHRONOUS_IO_NONALERT );
    }
    else
    {
        console_input->fd = alloc_pseudo_fd( &console_input_fd_ops, &console_input->obj,
                                             FILE_SYNCHRONOUS_IO_NONALERT );
    }
    if (!console_input->fd)
    {
        release_object( console_input );
        return NULL;
    }
    allow_fd_caching( console_input->fd );
    return &console_input->obj;
}

static void generate_sb_initial_events( struct console_input *console_input )
{
    struct screen_buffer *screen_buffer = console_input->active;
    struct condrv_renderer_event evt;

    evt.event = CONSOLE_RENDERER_SB_RESIZE_EVENT;
    evt.u.resize.width  = screen_buffer->width;
    evt.u.resize.height = screen_buffer->height;
    console_input_events_append( console_input, &evt );

    evt.event = CONSOLE_RENDERER_DISPLAY_EVENT;
    evt.u.display.left   = screen_buffer->win.left;
    evt.u.display.top    = screen_buffer->win.top;
    evt.u.display.width  = screen_buffer->win.right - screen_buffer->win.left + 1;
    evt.u.display.height = screen_buffer->win.bottom - screen_buffer->win.top + 1;
    console_input_events_append( console_input, &evt );

    evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
    evt.u.update.top    = 0;
    evt.u.update.bottom = screen_buffer->height - 1;
    console_input_events_append( console_input, &evt );

    evt.event = CONSOLE_RENDERER_CURSOR_GEOM_EVENT;
    evt.u.cursor_geom.size    = screen_buffer->cursor_size;
    evt.u.cursor_geom.visible = screen_buffer->cursor_visible;
    console_input_events_append( console_input, &evt );

    evt.event = CONSOLE_RENDERER_CURSOR_POS_EVENT;
    evt.u.cursor_pos.x = screen_buffer->cursor_x;
    evt.u.cursor_pos.y = screen_buffer->cursor_y;
    console_input_events_append( console_input, &evt );
}

static struct object *create_console_output( struct console_input *console_input, int fd )
{
    struct screen_buffer *screen_buffer;
    int	i;

    if (!(screen_buffer = alloc_object( &screen_buffer_ops )))
    {
        if (fd != -1) close( fd );
        return NULL;
    }
    screen_buffer->mode           = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    screen_buffer->input          = console_input;
    screen_buffer->cursor_size    = 100;
    screen_buffer->cursor_visible = 1;
    screen_buffer->width          = 80;
    screen_buffer->height         = 150;
    screen_buffer->max_width      = 80;
    screen_buffer->max_height     = 25;
    screen_buffer->cursor_x       = 0;
    screen_buffer->cursor_y       = 0;
    screen_buffer->attr           = 0x0F;
    screen_buffer->popup_attr     = 0xF5;
    screen_buffer->win.left       = 0;
    screen_buffer->win.right      = screen_buffer->max_width - 1;
    screen_buffer->win.top        = 0;
    screen_buffer->win.bottom     = screen_buffer->max_height - 1;
    screen_buffer->data           = NULL;
    screen_buffer->font.width     = 0;
    screen_buffer->font.height    = 0;
    screen_buffer->font.weight    = FW_NORMAL;
    screen_buffer->font.pitch_family  = FIXED_PITCH | FF_DONTCARE;
    screen_buffer->font.face_name = NULL;
    screen_buffer->font.face_len  = 0;
    memset( screen_buffer->color_map, 0, sizeof(screen_buffer->color_map) );
    list_add_head( &screen_buffer_list, &screen_buffer->entry );

    if (fd != -1)
        screen_buffer->fd = create_anonymous_fd( &screen_buffer_fd_ops, fd, &screen_buffer->obj,
                                                 FILE_SYNCHRONOUS_IO_NONALERT );
    else
        screen_buffer->fd = alloc_pseudo_fd( &screen_buffer_fd_ops, &screen_buffer->obj,
                                             FILE_SYNCHRONOUS_IO_NONALERT );
    if (!screen_buffer->fd)
    {
        release_object( screen_buffer );
        return NULL;
    }
    allow_fd_caching(screen_buffer->fd);

    if (!(screen_buffer->data = malloc( screen_buffer->width * screen_buffer->height *
                                        sizeof(*screen_buffer->data) )))
    {
        release_object( screen_buffer );
        return NULL;
    }
    /* clear the first row */
    for (i = 0; i < screen_buffer->width; i++) screen_buffer->data[i] = empty_char_info;
    /* and copy it to all other rows */
    for (i = 1; i < screen_buffer->height; i++)
        memcpy( &screen_buffer->data[i * screen_buffer->width], screen_buffer->data,
                screen_buffer->width * sizeof(char_info_t) );

    if (!console_input->active)
    {
	console_input->active = (struct screen_buffer*)grab_object( screen_buffer );
        generate_sb_initial_events( console_input );
    }
    return &screen_buffer->obj;
}

/* free the console for this process */
int free_console( struct process *process )
{
    struct console_input* console = process->console;

    if (!console) return 0;

    process->console = NULL;
    if (--console->num_proc == 0 && console->renderer)
    {
	/* all processes have terminated... tell the renderer to terminate too */
	struct condrv_renderer_event evt;
	evt.event = CONSOLE_RENDERER_EXIT_EVENT;
        memset(&evt.u, 0, sizeof(evt.u));
	console_input_events_append( console, &evt );
    }
    release_object( console );

    return 1;
}

/* let process inherit the console from parent... this handle two cases :
 *	1/ generic console inheritance
 *	2/ parent is a renderer which launches process, and process should attach to the console
 *	   rendered by parent
 */
obj_handle_t inherit_console( struct thread *parent_thread, struct process *parent, struct process *process,
                              obj_handle_t hconin )
{
    int done = 0;

    /* if parent is a renderer, then attach current process to its console
     * a bit hacky....
     */
    if (hconin && parent_thread)
    {
        struct console_input *console;

        /* FIXME: should we check some access rights ? */
        if ((console = (struct console_input *)get_handle_obj( parent, hconin,
                                                               0, &console_input_ops )))
        {
            if (console->renderer == parent_thread)
            {
                process->console = (struct console_input *)grab_object( console );
                process->console->num_proc++;
                done = 1;
            }
            release_object( console );
        }
        else clear_error();  /* ignore error */
    }
    /* otherwise, if parent has a console, attach child to this console */
    if (!done && parent->console)
    {
        process->console = (struct console_input *)grab_object( parent->console );
        process->console->num_proc++;
    }

    if (!process->console) return 0;
    return alloc_handle( process, process->console,
                         SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, 0 );
}

struct thread *console_get_renderer( struct console_input *console )
{
    return console->renderer;
}

static struct console_input* console_input_get( obj_handle_t handle, unsigned access )
{
    struct console_input*	console = NULL;

    if (handle)
	console = (struct console_input *)get_handle_obj( current->process, handle,
							  access, &console_input_ops );
    else if (current->process->console)
    {
	console = (struct console_input *)grab_object( current->process->console );
    }

    if (!console && !get_error()) set_error(STATUS_INVALID_PARAMETER);
    return console;
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

/* retrieve a pointer to the console input records */
static int read_console_input( struct console_input *console, struct async *async, int flush )
{
    struct iosb *iosb = async_get_iosb( async );
    data_size_t count;

    count = min( iosb->out_size / sizeof(INPUT_RECORD), console->recnum );
    if (count)
    {
        if (!(iosb->out_data = malloc( count * sizeof(INPUT_RECORD) )))
        {
            set_error( STATUS_NO_MEMORY );
            release_object( iosb );
            return 0;
        }
        iosb->out_size = iosb->result = count * sizeof(INPUT_RECORD);
        memcpy( iosb->out_data, console->records, iosb->result );
        iosb->status = STATUS_SUCCESS;
        async_terminate( async, STATUS_ALERTED );
    }
    else
    {
        async_terminate( async, STATUS_SUCCESS );
    }

    release_object( iosb );

    if (flush && count)
    {
        if (console->recnum > count)
        {
            INPUT_RECORD *new_rec;
            memmove( console->records, console->records + count, (console->recnum - count) * sizeof(*console->records) );
            console->recnum -= count;
            new_rec = realloc( console->records, console->recnum * sizeof(*console->records) );
            if (new_rec) console->records = new_rec;
        }
        else
        {
            console->recnum = 0;
            free( console->records );
            console->records = NULL;
            reset_event( console->event );
        }
    }

    return 1;
}

/* add input events to a console input queue */
static int write_console_input( struct console_input* console, int count,
                                const INPUT_RECORD *records )
{
    INPUT_RECORD *new_rec;
    struct async *async;

    if (!count) return 1;
    if (!(new_rec = realloc( console->records,
                             (console->recnum + count) * sizeof(INPUT_RECORD) )))
    {
        set_error( STATUS_NO_MEMORY );
        return 0;
    }
    console->records = new_rec;
    memcpy( new_rec + console->recnum, records, count * sizeof(INPUT_RECORD) );

    if (console->mode & ENABLE_PROCESSED_INPUT)
    {
        int i = 0;
        while (i < count)
        {
            if (records[i].EventType == KEY_EVENT &&
		records[i].Event.KeyEvent.uChar.UnicodeChar == 'C' - 64 &&
		!(records[i].Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
            {
                if (i != count - 1)
                    memcpy( &console->records[console->recnum + i],
                            &console->records[console->recnum + i + 1],
                            (count - i - 1) * sizeof(INPUT_RECORD) );
                count--;
                if (records[i].Event.KeyEvent.bKeyDown)
                {
                    /* send SIGINT to all processes attached to this console */
                    propagate_console_signal( console, CTRL_C_EVENT, 0 );
                }
            }
            else i++;
        }
    }
    console->recnum += count;
    while (console->recnum && (async = find_pending_async( &console->read_q )))
    {
        read_console_input( console, async, 1 );
        release_object( async );
    }
    if (console->recnum) set_event( console->event );
    return 1;
}

/* set misc console input information */
static int set_console_input_info( const struct set_console_input_info_request *req,
				   const WCHAR *title, data_size_t len )
{
    struct console_input *console;
    struct condrv_renderer_event evt;

    if (!(console = console_input_get( req->handle, FILE_WRITE_PROPERTIES ))) goto error;
    if (console_input_is_bare(console) && (req->mask & SET_CONSOLE_INPUT_INFO_WIN))
    {
        set_error( STATUS_UNSUCCESSFUL );
        goto error;
    }

    memset(&evt.u, 0, sizeof(evt.u));
    if (req->mask & SET_CONSOLE_INPUT_INFO_TITLE)
    {
        WCHAR *new_title = NULL;

        len = (len / sizeof(WCHAR)) * sizeof(WCHAR);
        if (len && !(new_title = memdup( title, len ))) goto error;
        free( console->title );
        console->title = new_title;
        console->title_len = len;
        evt.event = CONSOLE_RENDERER_TITLE_EVENT;
        console_input_events_append( console, &evt );
    }
    if (req->mask & SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE)
    {
        console->input_cp = req->input_cp;
    }
    if (req->mask & SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE)
    {
        console->output_cp = req->output_cp;
    }
    release_object( console );
    return 1;
 error:
    if (console) release_object( console );
    return 0;
}

/* resize a screen buffer */
static int change_screen_buffer_size( struct screen_buffer *screen_buffer,
                                      int new_width, int new_height )
{
    int i, old_width, old_height, copy_width, copy_height;
    char_info_t *new_data;

    if (!(new_data = malloc( new_width * new_height * sizeof(*new_data) )))
    {
        set_error( STATUS_NO_MEMORY );
        return 0;
    }
    old_width = screen_buffer->width;
    old_height = screen_buffer->height;
    copy_width = min( old_width, new_width );
    copy_height = min( old_height, new_height );

    /* copy all the rows */
    for (i = 0; i < copy_height; i++)
    {
        memcpy( &new_data[i * new_width], &screen_buffer->data[i * old_width],
                copy_width * sizeof(char_info_t) );
    }

    /* clear the end of each row */
    if (new_width > old_width)
    {
        /* fill first row */
        for (i = old_width; i < new_width; i++) new_data[i] = empty_char_info;
        /* and blast it to the other rows */
        for (i = 1; i < copy_height; i++)
            memcpy( &new_data[i * new_width + old_width], &new_data[old_width],
                    (new_width - old_width) * sizeof(char_info_t) );
    }

    /* clear remaining rows */
    if (new_height > old_height)
    {
        /* fill first row */
        for (i = 0; i < new_width; i++) new_data[old_height * new_width + i] = empty_char_info;
        /* and blast it to the other rows */
        for (i = old_height+1; i < new_height; i++)
            memcpy( &new_data[i * new_width], &new_data[old_height * new_width],
                    new_width * sizeof(char_info_t) );
    }
    free( screen_buffer->data );
    screen_buffer->data = new_data;
    screen_buffer->width = new_width;
    screen_buffer->height = new_height;
    return 1;
}

static int set_output_info( struct screen_buffer *screen_buffer,
                            const struct condrv_output_info_params *params, data_size_t extra_size )
{
    const struct condrv_output_info *info = &params->info;
    struct condrv_renderer_event evt;
    WCHAR *font_name;

    if (params->mask & SET_CONSOLE_OUTPUT_INFO_CURSOR_GEOM)
    {
        if (info->cursor_size < 1 || info->cursor_size > 100)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        if (screen_buffer->cursor_size != info->cursor_size ||
            screen_buffer->cursor_visible != info->cursor_visible)
        {
            screen_buffer->cursor_size    = info->cursor_size;
            screen_buffer->cursor_visible = info->cursor_visible;
            evt.event = CONSOLE_RENDERER_CURSOR_GEOM_EVENT;
            memset( &evt.u, 0, sizeof(evt.u) );
            evt.u.cursor_geom.size    = info->cursor_size;
            evt.u.cursor_geom.visible = info->cursor_visible;
            console_input_events_append( screen_buffer->input, &evt );
        }
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_CURSOR_POS)
    {
        if (info->cursor_x < 0 || info->cursor_x >= screen_buffer->width ||
            info->cursor_y < 0 || info->cursor_y >= screen_buffer->height)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        if (screen_buffer->cursor_x != info->cursor_x || screen_buffer->cursor_y != info->cursor_y)
        {
            screen_buffer->cursor_x       = info->cursor_x;
            screen_buffer->cursor_y       = info->cursor_y;
            evt.event = CONSOLE_RENDERER_CURSOR_POS_EVENT;
            memset( &evt.u, 0, sizeof(evt.u) );
            evt.u.cursor_pos.x = info->cursor_x;
            evt.u.cursor_pos.y = info->cursor_y;
            console_input_events_append( screen_buffer->input, &evt );
        }
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_SIZE)
    {
        unsigned cc;

        /* new screen-buffer cannot be smaller than actual window */
        if (info->width < screen_buffer->win.right - screen_buffer->win.left + 1 ||
            info->height < screen_buffer->win.bottom - screen_buffer->win.top + 1)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        /* FIXME: there are also some basic minimum and max size to deal with */
        if (!change_screen_buffer_size( screen_buffer, info->width, info->height )) return 0;

        evt.event = CONSOLE_RENDERER_SB_RESIZE_EVENT;
        memset(  &evt.u, 0, sizeof(evt.u) );
        evt.u.resize.width  = info->width;
        evt.u.resize.height = info->height;
        console_input_events_append( screen_buffer->input, &evt );

        evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
        memset( &evt.u, 0, sizeof(evt.u) );
        evt.u.update.top    = 0;
        evt.u.update.bottom = screen_buffer->height - 1;
        console_input_events_append( screen_buffer->input, &evt );

        /* scroll window to display sb */
        if (screen_buffer->win.right >= info->width)
        {
            screen_buffer->win.right -= screen_buffer->win.left;
            screen_buffer->win.left = 0;
        }
        if (screen_buffer->win.bottom >= info->height)
        {
            screen_buffer->win.bottom -= screen_buffer->win.top;
            screen_buffer->win.top = 0;
        }
        /* reset cursor if needed (normally, if cursor was outside of new sb, the
         * window has been shifted so that the new position of the cursor will be
         * visible */
        cc = 0;
        if (screen_buffer->cursor_x >= info->width)
        {
            screen_buffer->cursor_x = info->width - 1;
            cc++;
        }
        if (screen_buffer->cursor_y >= info->height)
        {
            screen_buffer->cursor_y = info->height - 1;
            cc++;
        }
        if (cc)
        {
            evt.event = CONSOLE_RENDERER_CURSOR_POS_EVENT;
            memset( &evt.u, 0, sizeof(evt.u) );
            evt.u.cursor_pos.x = info->cursor_x;
            evt.u.cursor_pos.y = info->cursor_y;
            console_input_events_append( screen_buffer->input, &evt );
        }

        if (screen_buffer == screen_buffer->input->active &&
            screen_buffer->input->mode & ENABLE_WINDOW_INPUT)
        {
            INPUT_RECORD ir;
            ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
            ir.Event.WindowBufferSizeEvent.dwSize.X = info->width;
            ir.Event.WindowBufferSizeEvent.dwSize.Y = info->height;
            write_console_input( screen_buffer->input, 1, &ir );
        }
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_ATTR)
    {
        screen_buffer->attr = info->attr;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_POPUP_ATTR)
    {
        screen_buffer->popup_attr = info->popup_attr;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW)
    {
        if (info->win_left < 0 || info->win_left > info->win_right ||
            info->win_right >= screen_buffer->width ||
            info->win_top < 0  || info->win_top > info->win_bottom ||
            info->win_bottom >= screen_buffer->height)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        if (screen_buffer->win.left != info->win_left || screen_buffer->win.top != info->win_top ||
            screen_buffer->win.right != info->win_right || screen_buffer->win.bottom != info->win_bottom)
        {
            screen_buffer->win.left   = info->win_left;
            screen_buffer->win.top    = info->win_top;
            screen_buffer->win.right  = info->win_right;
            screen_buffer->win.bottom = info->win_bottom;
            evt.event = CONSOLE_RENDERER_DISPLAY_EVENT;
            memset( &evt.u, 0, sizeof(evt.u) );
            evt.u.display.left   = info->win_left;
            evt.u.display.top    = info->win_top;
            evt.u.display.width  = info->win_right - info->win_left + 1;
            evt.u.display.height = info->win_bottom - info->win_top + 1;
            console_input_events_append( screen_buffer->input, &evt );
        }
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_MAX_SIZE)
    {
        screen_buffer->max_width  = info->max_width;
        screen_buffer->max_height = info->max_height;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_COLORTABLE)
    {
        memcpy( screen_buffer->color_map, info->color_map, sizeof(info->color_map) );
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_FONT)
    {
        screen_buffer->font.width  = info->font_width;
        screen_buffer->font.height = info->font_height;
        screen_buffer->font.weight = info->font_weight;
        screen_buffer->font.pitch_family = info->font_pitch_family;
        if (extra_size)
        {
            extra_size = extra_size / sizeof(WCHAR) * sizeof(WCHAR);
            font_name = mem_alloc( extra_size );
            if (font_name)
            {
                memcpy( font_name, info + 1, extra_size );
                free( screen_buffer->font.face_name );
                screen_buffer->font.face_name = font_name;
                screen_buffer->font.face_len  = extra_size;
            }
        }
    }

    return 1;
}

/* appends a new line to history (history is a fixed size array) */
static void console_input_append_hist( struct console_input* console, const WCHAR* buf, data_size_t len )
{
    struct history_line *ptr;

    if (!console || !console->history_size)
    {
	set_error( STATUS_INVALID_PARAMETER ); /* FIXME */
	return;
    }

    len = (len / sizeof(WCHAR)) * sizeof(WCHAR);
    if (console->history_mode && console->history_index &&
        console->history[console->history_index - 1]->len == len &&
        !memcmp( console->history[console->history_index - 1]->text, buf, len ))
    {
        /* don't duplicate entry */
	set_error( STATUS_ALIAS_EXISTS );
	return;
    }
    if (!(ptr = mem_alloc( offsetof( struct history_line, text[len / sizeof(WCHAR)] )))) return;
    ptr->len = len;
    memcpy( ptr->text, buf, len );

    if (console->history_index < console->history_size)
    {
	console->history[console->history_index++] = ptr;
    }
    else
    {
	free( console->history[0]) ;
	memmove( &console->history[0], &console->history[1],
		 (console->history_size - 1) * sizeof(*console->history) );
	console->history[console->history_size - 1] = ptr;
    }
}

/* returns a line from the cache */
static data_size_t console_input_get_hist( struct console_input *console, int index )
{
    data_size_t ret = 0;

    if (index >= console->history_index) set_error( STATUS_INVALID_PARAMETER );
    else
    {
        ret = console->history[index]->len;
        set_reply_data( console->history[index]->text, min( ret, get_reply_max_size() ));
    }
    return ret;
}

/* dumb dump */
static void console_input_dump( struct object *obj, int verbose )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    fprintf( stderr, "Console input active=%p evt=%p\n",
	     console->active, console->evt );
}

static void console_input_destroy( struct object *obj )
{
    struct console_input*	console_in = (struct console_input *)obj;
    struct screen_buffer*	curr;
    int				i;

    assert( obj->ops == &console_input_ops );
    free( console_in->title );
    free( console_in->records );

    if (console_in->active) release_object( console_in->active );
    console_in->active = NULL;

    LIST_FOR_EACH_ENTRY( curr, &screen_buffer_list, struct screen_buffer, entry )
    {
        if (curr->input == console_in) curr->input = NULL;
    }

    free_async_queue( &console_in->read_q );
    if (console_in->evt)
        console_in->evt->console = NULL;
    if (console_in->event)
        release_object( console_in->event );
    if (console_in->fd)
        release_object( console_in->fd );

    for (i = 0; i < console_in->history_size; i++)
        free( console_in->history[i] );
    free( console_in->history );
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

    if (screen_buffer->input && screen_buffer->input->active == screen_buffer)
    {
        struct screen_buffer *sb;

        screen_buffer->input->active = NULL;
        LIST_FOR_EACH_ENTRY( sb, &screen_buffer_list, struct screen_buffer, entry )
        {
            if (sb->input == screen_buffer->input)
            {
                sb->input->active = sb;
                break;
            }
        }
    }
    if (screen_buffer->fd) release_object( screen_buffer->fd );
    free( screen_buffer->data );
    free( screen_buffer->font.face_name );
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

/* read data from a screen buffer */
static void read_console_output( struct screen_buffer *screen_buffer, unsigned int x, unsigned int y,
                                 enum char_info_mode mode, unsigned int width )
{
    unsigned int i, count;
    char_info_t *src;

    if (x >= screen_buffer->width || y >= screen_buffer->height)
    {
        if (width) set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    src = screen_buffer->data + y * screen_buffer->width + x;

    switch(mode)
    {
    case CHAR_INFO_MODE_TEXT:
        {
            WCHAR *data;
            count = min( screen_buffer->data + screen_buffer->height * screen_buffer->width - src,
                         get_reply_max_size() / sizeof(*data) );
            if ((data = set_reply_data_size( count * sizeof(*data) )))
            {
                for (i = 0; i < count; i++) data[i] = src[i].ch;
            }
        }
        break;
    case CHAR_INFO_MODE_ATTR:
        {
            unsigned short *data;
            count = min( screen_buffer->data + screen_buffer->height * screen_buffer->width - src,
                         get_reply_max_size() / sizeof(*data) );
            if ((data = set_reply_data_size( count * sizeof(*data) )))
            {
                for (i = 0; i < count; i++) data[i] = src[i].attr;
            }
        }
        break;
    case CHAR_INFO_MODE_TEXTATTR:
        {
            char_info_t *data;
            SMALL_RECT *region;
            if (!width || get_reply_max_size() < sizeof(*region))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            count  = min( (get_reply_max_size() - sizeof(*region)) / (width * sizeof(*data)), screen_buffer->height - y );
            width  = min( width, screen_buffer->width - x );
            if (!(region = set_reply_data_size( sizeof(*region) + width * count * sizeof(*data) ))) return;
            region->Left   = x;
            region->Top    = y;
            region->Right  = x + width - 1;
            region->Bottom = y + count - 1;
            data = (char_info_t *)(region + 1);
            for (i = 0; i < count; i++)
            {
                memcpy( &data[i * width], &src[i * screen_buffer->width], width * sizeof(*data) );
            }
        }
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        break;
    }
}

/* write data into a screen buffer */
static void write_console_output( struct screen_buffer *screen_buffer, const struct condrv_output_params *params,
                                  data_size_t size )
{
    unsigned int i, entry_size, entry_cnt, x, y;
    char_info_t *dest;
    char *src;

    entry_size = params->mode == CHAR_INFO_MODE_TEXTATTR ? sizeof(char_info_t) : sizeof(WCHAR);
    if (size % entry_size)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (params->x >= screen_buffer->width) return;
    entry_cnt = size / entry_size;

    for (i = 0, src = (char *)(params + 1); i < entry_cnt; i++, src += entry_size)
    {
        if (params->width)
        {
            x = params->x + i % params->width;
            y = params->y + i / params->width;
            if (x >= screen_buffer->width) continue;
        }
        else
        {
            x = (params->x + i) % screen_buffer->width;
            y = params->y + (params->x + i) / screen_buffer->width;
        }
        if (y >= screen_buffer->height) break;

        dest = &screen_buffer->data[y * screen_buffer->width + x];
        switch(params->mode)
        {
        case CHAR_INFO_MODE_TEXT:
            dest->ch = *(const WCHAR *)src;
            break;
        case CHAR_INFO_MODE_ATTR:
            dest->attr = *(const unsigned short *)src;
            break;
        case CHAR_INFO_MODE_TEXTATTR:
            *dest = *(const char_info_t *)src;
            break;
        case CHAR_INFO_MODE_TEXTSTDATTR:
            dest->ch   = *(const WCHAR *)src;
            dest->attr = screen_buffer->attr;
            break;
        default:
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }

    if (i && screen_buffer == screen_buffer->input->active)
    {
        struct condrv_renderer_event evt;
        evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
        memset(&evt.u, 0, sizeof(evt.u));
        evt.u.update.top    = params->y;
        evt.u.update.bottom = params->width
            ? min( params->y + entry_cnt / params->width, screen_buffer->height ) - 1
            : params->y + (params->x + i - 1) / screen_buffer->width;
        console_input_events_append( screen_buffer->input, &evt );
    }

    if (get_reply_max_size() == sizeof(SMALL_RECT))
    {
        SMALL_RECT region;
        region.Left   = params->x;
        region.Top    = params->y;
        region.Right  = min( params->x + params->width, screen_buffer->width ) - 1;
        region.Bottom = min( params->y + entry_cnt / params->width, screen_buffer->height ) - 1;
        set_reply_data( &region, sizeof(region) );
    }
    else set_reply_data( &i, sizeof(i) );
}

/* fill a screen buffer with uniform data */
static int fill_console_output( struct screen_buffer *screen_buffer, char_info_t data,
                                enum char_info_mode mode, int x, int y, int count, int wrap )
{
    int i;
    char_info_t *end, *dest = screen_buffer->data + y * screen_buffer->width + x;

    if (y >= screen_buffer->height) return 0;

    if (wrap)
        end = screen_buffer->data + screen_buffer->height * screen_buffer->width;
    else
        end = screen_buffer->data + (y+1) * screen_buffer->width;

    if (count > end - dest) count = end - dest;

    switch(mode)
    {
    case CHAR_INFO_MODE_TEXT:
        for (i = 0; i < count; i++) dest[i].ch = data.ch;
        break;
    case CHAR_INFO_MODE_ATTR:
        for (i = 0; i < count; i++) dest[i].attr = data.attr;
        break;
    case CHAR_INFO_MODE_TEXTATTR:
        for (i = 0; i < count; i++) dest[i] = data;
        break;
    case CHAR_INFO_MODE_TEXTSTDATTR:
        for (i = 0; i < count; i++)
        {
            dest[i].ch   = data.ch;
            dest[i].attr = screen_buffer->attr;
        }
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }

    if (count && screen_buffer == screen_buffer->input->active)
    {
        struct condrv_renderer_event evt;
        evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
        memset(&evt.u, 0, sizeof(evt.u));
        evt.u.update.top    = y;
        evt.u.update.bottom = (y * screen_buffer->width + x + count - 1) / screen_buffer->width;
        console_input_events_append( screen_buffer->input, &evt );
    }
    return i;
}

/* scroll parts of a screen buffer */
static void scroll_console_output( struct screen_buffer *screen_buffer, int xsrc, int ysrc, int xdst, int ydst,
                                   int w, int h, const rectangle_t *clip, char_info_t fill )
{
    struct condrv_renderer_event evt;
    rectangle_t src, dst;
    int x, y;

    src.left   = max( xsrc, clip->left );
    src.top    = max( ysrc, clip->top );
    src.right  = min( xsrc + w - 1, clip->right );
    src.bottom = min( ysrc + h - 1, clip->bottom );

    dst.left   = xdst;
    dst.top    = ydst;
    dst.right  = xdst + w - 1;
    dst.bottom = ydst + h - 1;

    if (dst.left < clip->left)
    {
        xsrc += clip->left - dst.left;
        w -= clip->left - dst.left;
        dst.left = clip->left;
    }
    if (dst.top < clip->top)
    {
        ysrc += clip->top - dst.top;
        h -= clip->top - dst.top;
        dst.top = clip->top;
    }
    if (dst.right  > clip->right)  w -= dst.right  - clip->right;
    if (dst.bottom > clip->bottom) h -= dst.bottom - clip->bottom;

    if (w > 0 && h > 0)
    {
        if (ysrc < ydst)
        {
            for (y = h; y > 0; y--)
            {
                memcpy( &screen_buffer->data[(dst.top + y - 1) * screen_buffer->width + dst.left],
                        &screen_buffer->data[(ysrc + y - 1) * screen_buffer->width + xsrc],
                        w * sizeof(screen_buffer->data[0]) );
            }
        }
        else
        {
            for (y = 0; y < h; y++)
            {
                /* we use memmove here because when psrc and pdst are the same,
                 * copies are done on the same row, so the dst and src blocks
                 * can overlap */
                memmove( &screen_buffer->data[(dst.top + y) * screen_buffer->width + dst.left],
                         &screen_buffer->data[(ysrc + y) * screen_buffer->width + xsrc],
                         w * sizeof(screen_buffer->data[0]) );
            }
        }
    }

    for (y = src.top; y <= src.bottom; y++)
    {
        int left  = src.left;
        int right = src.right;
        if (dst.top <= y && y <= dst.bottom)
        {
            if (dst.left <= src.left) left  = max( left, dst.right + 1 );
            if (dst.left >= src.left) right = min( right, dst.left - 1 );
        }
        for (x = left; x <= right; x++) screen_buffer->data[y * screen_buffer->width + x] = fill;
    }

    /* FIXME: this could be enhanced, by signalling scroll */
    evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
    memset(&evt.u, 0, sizeof(evt.u));
    evt.u.update.top    = min( src.top, dst.top );
    evt.u.update.bottom = max( src.bottom, dst.bottom );
    console_input_events_append( screen_buffer->input, &evt );
}

static int console_input_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console_input *console = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_GET_MODE:
        if (get_reply_max_size() != sizeof(console->mode))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        return set_reply_data( &console->mode, sizeof(console->mode) ) != NULL;

    case IOCTL_CONDRV_SET_MODE:
        if (get_req_data_size() != sizeof(console->mode))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        console->mode = *(unsigned int *)get_req_data();
        return 1;

    case IOCTL_CONDRV_READ_INPUT:
        {
            int blocking = 0;
            if (get_reply_max_size() % sizeof(INPUT_RECORD))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            if (get_req_data_size())
            {
                if (get_req_data_size() != sizeof(int))
                {
                    set_error( STATUS_INVALID_PARAMETER );
                    return 0;
                }
                blocking = *(int *)get_req_data();
            }
            set_error( STATUS_PENDING );
            if (blocking && !console->recnum)
            {
                queue_async( &console->read_q, async );
                return 1;
            }
            return read_console_input( console, async, 1 );
        }

    case IOCTL_CONDRV_WRITE_INPUT:
        return write_console_input( console, get_req_data_size() / sizeof(INPUT_RECORD), get_req_data() );

    case IOCTL_CONDRV_PEEK:
        if (get_reply_max_size() % sizeof(INPUT_RECORD))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        set_error( STATUS_PENDING );
        return read_console_input( console, async, 0 );

    case IOCTL_CONDRV_GET_INPUT_INFO:
        {
            struct condrv_input_info info;
            if (get_reply_max_size() != sizeof(info))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            info.input_cp      = console->input_cp;
            info.output_cp     = console->output_cp;
            info.history_mode  = console->history_mode;
            info.history_size  = console->history_size;
            info.history_index = console->history_index;
            info.edition_mode  = console->edition_mode;
            info.input_count   = console->recnum;
            info.win           = console->win;
            return set_reply_data( &info, sizeof(info) ) != NULL;
        }

    case IOCTL_CONDRV_SET_INPUT_INFO:
        {
            const struct condrv_input_info_params *params = get_req_data();
            if (get_req_data_size() != sizeof(*params))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_HISTORY_MODE)
            {
                console->history_mode = params->info.history_mode;
            }
            if ((params->mask & SET_CONSOLE_INPUT_INFO_HISTORY_SIZE) &&
                console->history_size != params->info.history_size)
            {
                struct history_line **mem = NULL;
                int i, delta;

                if (params->info.history_size)
                {
                    if (!(mem = mem_alloc( params->info.history_size * sizeof(*mem) ))) return 0;
                    memset( mem, 0, params->info.history_size * sizeof(*mem) );
                }

                delta = (console->history_index > params->info.history_size) ?
                    (console->history_index - params->info.history_size) : 0;

                for (i = delta; i < console->history_index; i++)
                {
                    mem[i - delta] = console->history[i];
                    console->history[i] = NULL;
                }
                console->history_index -= delta;

                for (i = 0; i < console->history_size; i++)
                    free( console->history[i] );
                free( console->history );
                console->history = mem;
                console->history_size = params->info.history_size;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_EDITION_MODE)
            {
                console->edition_mode = params->info.edition_mode;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE)
            {
                console->input_cp = params->info.input_cp;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE)
            {
                console->output_cp = params->info.output_cp;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_WIN)
            {
                console->win = params->info.win;
            }
            return 1;
        }

    case IOCTL_CONDRV_GET_TITLE:
        if (!console->title_len) return 1;
        return set_reply_data( console->title, min( console->title_len, get_reply_max_size() )) != NULL;

    case IOCTL_CONDRV_SET_TITLE:
        {
            data_size_t len = get_req_data_size();
            struct condrv_renderer_event evt;
            WCHAR *title = NULL;

            if (len % sizeof(WCHAR))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }

            if (len && !(title = memdup( get_req_data(), len ))) return 0;
            free( console->title );
            console->title = title;
            console->title_len = len;
            evt.event = CONSOLE_RENDERER_TITLE_EVENT;
            console_input_events_append( console, &evt );
            return 1;
        }

    default:
        set_error( STATUS_INVALID_HANDLE );
        return 0;
    }
}

static int screen_buffer_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct screen_buffer *screen_buffer = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_GET_MODE:
        if (get_reply_max_size() != sizeof(screen_buffer->mode))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        return set_reply_data( &screen_buffer->mode, sizeof(screen_buffer->mode) ) != NULL;

    case IOCTL_CONDRV_SET_MODE:
        if (get_req_data_size() != sizeof(screen_buffer->mode))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        screen_buffer->mode = *(unsigned int *)get_req_data();
        return 1;

    case IOCTL_CONDRV_READ_OUTPUT:
        {
            const struct condrv_output_params *params = get_req_data();
            if (get_req_data_size() != sizeof(*params))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            if (console_input_is_bare( screen_buffer->input ))
            {
                set_error( STATUS_OBJECT_TYPE_MISMATCH );
                return 0;
            }
            read_console_output( screen_buffer, params->x, params->y, params->mode, params->width );
            return !get_error();
        }

    case IOCTL_CONDRV_WRITE_OUTPUT:
        if (get_req_data_size() < sizeof(struct condrv_output_params) ||
            (get_reply_max_size() != sizeof(SMALL_RECT) && get_reply_max_size() != sizeof(unsigned int)))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        if (console_input_is_bare( screen_buffer->input ))
        {
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
            return 0;
        }
        write_console_output( screen_buffer, get_req_data(), get_req_data_size() - sizeof(struct condrv_output_params) );
        return !get_error();

    case IOCTL_CONDRV_GET_OUTPUT_INFO:
        {
            struct condrv_output_info *info;
            data_size_t size;

            size = min( sizeof(*info) + screen_buffer->font.face_len, get_reply_max_size() );
            if (size < sizeof(*info))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            if (!(info = set_reply_data_size( size ))) return 0;

            info->cursor_size       = screen_buffer->cursor_size;
            info->cursor_visible    = screen_buffer->cursor_visible;
            info->cursor_x          = screen_buffer->cursor_x;
            info->cursor_y          = screen_buffer->cursor_y;
            info->width             = screen_buffer->width;
            info->height            = screen_buffer->height;
            info->attr              = screen_buffer->attr;
            info->popup_attr        = screen_buffer->popup_attr;
            info->win_left          = screen_buffer->win.left;
            info->win_top           = screen_buffer->win.top;
            info->win_right         = screen_buffer->win.right;
            info->win_bottom        = screen_buffer->win.bottom;
            info->max_width         = screen_buffer->max_width;
            info->max_height        = screen_buffer->max_height;
            info->font_width        = screen_buffer->font.width;
            info->font_height       = screen_buffer->font.height;
            info->font_weight       = screen_buffer->font.weight;
            info->font_pitch_family = screen_buffer->font.pitch_family;
            memcpy( info->color_map, screen_buffer->color_map, sizeof(info->color_map) );
            size -= sizeof(*info);
            if (size) memcpy( info + 1, screen_buffer->font.face_name, size );
            return 1;
        }

    case IOCTL_CONDRV_SET_OUTPUT_INFO:
        {
            const struct condrv_output_info_params *params = get_req_data();
            if (get_req_data_size() < sizeof(*params))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            if (!screen_buffer->input)
            {
                set_error( STATUS_INVALID_HANDLE );
                return 0;
            }
            return set_output_info( screen_buffer, params, get_req_data_size() - sizeof(*params) );
        }

    case IOCTL_CONDRV_ACTIVATE:
        if (!screen_buffer->input)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }

        if (screen_buffer != screen_buffer->input->active)
        {
            if (screen_buffer->input->active) release_object( screen_buffer->input->active );
            screen_buffer->input->active = (struct screen_buffer *)grab_object( screen_buffer );
            generate_sb_initial_events( screen_buffer->input );
        }
        return 1;

    case IOCTL_CONDRV_FILL_OUTPUT:
        {
            const struct condrv_fill_output_params *params = get_req_data();
            char_info_t data;
            DWORD written;
            if (get_req_data_size() != sizeof(*params) ||
                (get_reply_max_size() && get_reply_max_size() != sizeof(written)))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            data.ch   = params->ch;
            data.attr = params->attr;
            written = fill_console_output( screen_buffer, data, params->mode,
                                           params->x, params->y, params->count, params->wrap );
            if (written && get_reply_max_size() == sizeof(written))
                set_reply_data( &written, sizeof(written) );
            return !get_error();
        }

    case IOCTL_CONDRV_SCROLL:
        {
            const struct condrv_scroll_params *params = get_req_data();
            rectangle_t clip;
            if (get_req_data_size() != sizeof(*params))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            if (console_input_is_bare( screen_buffer->input ) || !screen_buffer->input)
            {
                set_error( STATUS_OBJECT_TYPE_MISMATCH );
                return 0;
            }
            clip.left   = max( params->clip.Left, 0 );
            clip.top    = max( params->clip.Top,  0 );
            clip.right  = min( params->clip.Right,  screen_buffer->width - 1 );
            clip.bottom = min( params->clip.Bottom, screen_buffer->height - 1 );
            if (clip.left > clip.right || clip.top > clip.bottom || params->scroll.Left < 0 || params->scroll.Top < 0 ||
                params->scroll.Right >= screen_buffer->width || params->scroll.Bottom >= screen_buffer->height ||
                params->scroll.Right < params->scroll.Left || params->scroll.Top > params->scroll.Bottom ||
                params->origin.X < 0 || params->origin.X >= screen_buffer->width || params->origin.Y < 0 ||
                params->origin.Y >= screen_buffer->height)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }

            scroll_console_output( screen_buffer, params->scroll.Left, params->scroll.Top, params->origin.X, params->origin.Y,
                                   params->scroll.Right - params->scroll.Left + 1, params->scroll.Bottom - params->scroll.Top + 1,
                                   &clip, params->fill );
            return !get_error();
        }

    default:
        set_error( STATUS_INVALID_HANDLE );
        return 0;
    }
}

static int console_input_events_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct console_input_events *evts = get_fd_user( fd );

    switch (code)
    {
    case IOCTL_CONDRV_GET_RENDERER_EVENTS:
        set_error( STATUS_PENDING );
        if (evts->num_used) return get_renderer_events( evts, async );
        queue_async( &evts->read_q, async );
        return 1;

    case IOCTL_CONDRV_ATTACH_RENDERER:
        {
            struct console_input *console_input;
            if (get_req_data_size() != sizeof(condrv_handle_t))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return 0;
            }
            console_input = (struct console_input *)get_handle_obj( current->process, *(condrv_handle_t *)get_req_data(),
                                                                    0, &console_input_ops );
            if (!console_input) return 0;

            if (!console_input->evt && !evts->console)
            {
                console_input->evt = evts;
                console_input->renderer = current;
                evts->console = console_input;
            }
            else set_error( STATUS_INVALID_HANDLE );

            release_object( console_input );
            return !get_error();
        }

    case IOCTL_CONDRV_SCROLL:
    case IOCTL_CONDRV_SET_MODE:
    case IOCTL_CONDRV_WRITE_OUTPUT:
    case IOCTL_CONDRV_READ_OUTPUT:
    case IOCTL_CONDRV_FILL_OUTPUT:
    case IOCTL_CONDRV_GET_OUTPUT_INFO:
    case IOCTL_CONDRV_SET_OUTPUT_INFO:
        if (!evts->console || !evts->console->active)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }
        return screen_buffer_ioctl( evts->console->active->fd, code, async );

    default:
        if (!evts->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return 0;
        }
        return console_input_ioctl( evts->console->fd, code, async );
    }
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
    static const WCHAR consoleW[]       = {'C','o','n','s','o','l','e'};
    static const WCHAR current_inW[]    = {'C','u','r','r','e','n','t','I','n'};
    static const WCHAR current_outW[]   = {'C','u','r','r','e','n','t','O','u','t'};
    static const WCHAR rendererW[]      = {'R','e','n','d','e','r','e','r'};
    static const WCHAR screen_bufferW[] = {'S','c','r','e','e','n','B','u','f','f','e','r'};

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

    if (name->len == sizeof(rendererW) && !memcmp( name->str, rendererW, name->len ))
    {
        name->len = 0;
        return create_console_input_events();
    }

    if (name->len == sizeof(screen_bufferW) && !memcmp( name->str, screen_bufferW, name->len ))
    {
        if (!current->process->console)
        {
            set_error( STATUS_INVALID_HANDLE );
            return NULL;
        }
        name->len = 0;
        return create_console_output( current->process->console, -1 );
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

struct object *create_console_device( struct object *root, const struct unicode_str *name )
{
    return create_named_object( root, &console_device_ops, name, 0, NULL );
}

/* allocate a console for the renderer */
DECL_HANDLER(alloc_console)
{
    struct process *process;
    struct console_input *console;
    int fd;
    int attach = 0;

    if (req->input_fd != -1)
    {
        if ((fd = thread_get_inflight_fd( current, req->input_fd )) == -1)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }
    else fd = -1;

    switch (req->pid)
    {
    case 0:
        /* console to be attached to parent process */
        if (!(process = get_process_from_id( current->process->parent_id )))
        {
            if (fd != -1) close( fd );
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
        attach = 1;
        break;
    case 0xffffffff:
        /* console to be attached to current process */
        process = current->process;
        grab_object( process );
        attach = 1;
        break;
    default:
        /* console to be attached to req->pid */
        if (!(process = get_process_from_id( req->pid )))
        {
            if (fd != -1) close( fd );
            return;
        }
    }

    if (attach && process->console)
    {
        if (fd != -1) close( fd );
        set_error( STATUS_ACCESS_DENIED );
    }
    else if ((console = (struct console_input*)create_console_input( fd )))
    {
        if ((reply->handle_in = alloc_handle( current->process, console, req->access,
                                              req->attributes )) && attach)
        {
            process->console = (struct console_input*)grab_object( console );
            console->num_proc++;
        }
        release_object( console );
    }
    release_object( process );
}

/* free the console of the current process */
DECL_HANDLER(free_console)
{
    free_console( current->process );
}

/* attach to a other process's console */
DECL_HANDLER(attach_console)
{
    struct process *process;

    if (current->process->console)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    process = get_process_from_id( req->pid == ATTACH_PARENT_PROCESS
                                   ? current->process->parent_id : req->pid );
    if (!process) return;

    if (process->console && process->console->active)
    {
        current->process->console = (struct console_input *)grab_object( process->console );
        current->process->console->num_proc++;
    }
    else
    {
        set_error( STATUS_INVALID_HANDLE );
    }

    release_object( process );
    return;
}

/* set info about a console input */
DECL_HANDLER(set_console_input_info)
{
    set_console_input_info( req, get_req_data(), get_req_data_size() );
}

/* appends a string to console's history */
DECL_HANDLER(append_console_input_history)
{
    struct console_input *console;

    if (!(console = console_input_get( req->handle, FILE_WRITE_PROPERTIES ))) return;
    console_input_append_hist( console, get_req_data(), get_req_data_size() );
    release_object( console );
}

/* appends a string to console's history */
DECL_HANDLER(get_console_input_history)
{
    struct console_input *console;

    if (!(console = console_input_get( req->handle, FILE_READ_PROPERTIES ))) return;
    reply->total = console_input_get_hist( console, req->index );
    release_object( console );
}

/* creates a screen buffer */
DECL_HANDLER(create_console_output)
{
    struct console_input *console;
    struct object        *screen_buffer;
    int                   fd;

    if (req->fd != -1)
    {
        if ((fd = thread_get_inflight_fd( current, req->fd )) == -1)
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
    }
    else fd = -1;
    if (!(console = console_input_get( req->handle_in, FILE_WRITE_PROPERTIES )))
    {
        if (fd != -1) close( fd );
        return;
    }
    if (console_input_is_bare( console ) ^ (fd != -1))
    {
        if (fd != -1) close( fd );
        release_object( console );
        set_error( STATUS_INVALID_HANDLE );
        return;
    }

    screen_buffer = create_console_output( console, fd );
    if (screen_buffer)
    {
        /* FIXME: should store sharing and test it when opening the CONOUT$ device
         * see file.c on how this could be done */
        reply->handle_out = alloc_handle( current->process, screen_buffer, req->access, req->attributes );
        release_object( screen_buffer );
    }
    release_object( console );
}

/* sends a signal to a console (process, group...) */
DECL_HANDLER(send_console_signal)
{
    process_id_t group;

    group = req->group_id ? req->group_id : current->process->group_id;

    if (!group)
        set_error( STATUS_INVALID_PARAMETER );
    else
        propagate_console_signal( current->process->console, req->signal, group );
}

/* get console which renderer is 'current' */
static int cgwe_enum( struct process* process, void* user)
{
    if (process->console && process->console->renderer == current)
    {
        *(struct console_input**)user = (struct console_input *)grab_object( process->console );
        return 1;
    }
    return 0;
}

DECL_HANDLER(get_console_wait_event)
{
    struct console_input* console = NULL;
    struct object *obj;

    if (req->handle)
    {
        if (!(obj = get_handle_obj( current->process, req->handle, FILE_READ_PROPERTIES, NULL ))) return;
        if (obj->ops == &console_input_ops)
            console = (struct console_input *)grab_object( obj );
        else if (obj->ops == &screen_buffer_ops)
            console = (struct console_input *)grab_object( ((struct screen_buffer *)obj)->input );
        else
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
    }
    else if (current->process->console)
        console = (struct console_input *)grab_object( current->process->console );
    else enum_processes(cgwe_enum, &console);

    if (console)
    {
        reply->event = alloc_handle( current->process, console->event, EVENT_ALL_ACCESS, 0 );
        release_object( console );
    }
    else set_error( STATUS_INVALID_PARAMETER );
}

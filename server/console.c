/*
 * Server-side console management
 *
 * Copyright (C) 1998 Alexandre Julliard
 *               2001 Eric Pouech
 *
 */

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "winnt.h"
#include "winbase.h"
#include "wincon.h"

#include "handle.h"
#include "process.h"
#include "request.h"
#include "unicode.h"
#include "console.h"


static void console_input_dump( struct object *obj, int verbose );
static void console_input_destroy( struct object *obj );
static int console_input_signaled( struct object *obj, struct thread *thread );

/* common routine */
static int console_get_file_info( struct object *obj, struct get_file_info_request *req );

static const struct object_ops console_input_ops =
{
    sizeof(struct console_input),     /* size */
    console_input_dump,               /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    console_input_signaled,           /* signaled */
    no_satisfied,                     /* satisfied */
    NULL,                             /* get_poll_events */
    NULL,                             /* poll_event */
    no_get_fd,                        /* get_fd */
    no_flush,                         /* flush */
    console_get_file_info,            /* get_file_info */
    console_input_destroy             /* destroy */
};

static void console_input_events_dump( struct object *obj, int verbose );
static void console_input_events_destroy( struct object *obj );
static int  console_input_events_signaled( struct object *obj, struct thread *thread );

struct console_input_events 
{
    struct object         obj;         /* object header */
    int			  num_alloc;   /* number of allocated events */
    int 		  num_used;    /* number of actually used events */
    struct console_renderer_event*	events;
};

static const struct object_ops console_input_events_ops =
{
    sizeof(struct console_input_events), /* size */
    console_input_events_dump,        /* dump */
    add_queue,                        /* add_queue */
    remove_queue,                     /* remove_queue */
    console_input_events_signaled,    /* signaled */
    no_satisfied,                     /* satisfied */
    NULL,                             /* get_poll_events */
    NULL,                             /* poll_event */
    no_get_fd,                        /* get_fd */
    no_flush,                         /* flush */
    no_get_file_info,                 /* get_file_info */
    console_input_events_destroy      /* destroy */
};

struct screen_buffer
{
    struct object         obj;           /* object header */
    int                   mode;          /* output mode */
    struct console_input *input;         /* associated console input */
    struct screen_buffer *next;          /* linked list of all screen buffers */
    short int             cursor_size;   /* size of cursor (percentage filled) */
    short int             cursor_visible;/* cursor visibility flag */
    COORD                 cursor;        /* position of cursor */
    short int             width;         /* size (w-h) of the screen buffer */
    short int             height;
    short int             max_width;     /* size (w-h) of the window given font size */
    short int             max_height;
    unsigned             *data;          /* the data for each cell - a width x height matrix */
    unsigned short        attr;          /* default attribute for screen buffer */
    SMALL_RECT            win;           /* current visible window on the screen buffer *
					  * as seen in wineconsole */
};

static void screen_buffer_dump( struct object *obj, int verbose );
static void screen_buffer_destroy( struct object *obj );

static const struct object_ops screen_buffer_ops =
{
    sizeof(struct screen_buffer),     /* size */
    screen_buffer_dump,               /* dump */
    no_add_queue,                     /* add_queue */
    NULL,                             /* remove_queue */
    NULL,                             /* signaled */
    NULL,                             /* satisfied */
    NULL,                             /* get_poll_events */
    NULL,                             /* poll_event */
    no_get_fd,                        /* get_fd */
    no_flush,                         /* flush */
    console_get_file_info,            /* get_file_info */
    screen_buffer_destroy             /* destroy */
};

static struct screen_buffer *screen_buffer_list;

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
    free( evts->events );
}

/* the rendere events list is signaled when it's not empty */
static int  console_input_events_signaled( struct object *obj, struct thread *thread )
{
    struct console_input_events *evts = (struct console_input_events *)obj;
    assert( obj->ops == &console_input_events_ops );
    return evts->num_used ? 1 : 0;
}

/* add an event to the console's renderer events list */
static void console_input_events_append( struct console_input_events* evts, 
					 struct console_renderer_event* evt)
{
    if (!evt) return;

    /* to be done even when the renderer generates the events ? */
    if (evts->num_used == evts->num_alloc)
    {
	evts->num_alloc += 16;
	evts->events = realloc( evts->events, evts->num_alloc * sizeof(*evt) );
	assert(evts->events);
    }
    evts->events[evts->num_used++] = *evt;
    wake_up( &evts->obj, 0 );
}

/* retrieves events from the console's renderer events list */
static size_t  console_input_events_get( struct console_input_events* evts, 
					 struct console_renderer_event* evt, size_t num )
{
    if (num % sizeof(*evt) != 0)
    {
	set_error( STATUS_INVALID_PARAMETER );
	return 0;
    }
    num /= sizeof(*evt);
    if (num > evts->num_used)
	num = evts->num_used;
    memcpy( evt, evts->events, num * sizeof(*evt) );
    if (num < evts->num_used)
    {
	memmove( &evts->events[0], &evts->events[num], 
		 (evts->num_used - num) * sizeof(*evt) );
    }
    evts->num_used -= num;
    return num * sizeof(struct console_renderer_event);
}

static struct console_input_events *create_console_input_events(void)
{
    struct console_input_events*	evt;

    if (!(evt = alloc_object( &console_input_events_ops, -1 ))) return NULL;
    evt->num_alloc = evt->num_used = 0;
    evt->events = NULL;
    return evt;
}

static struct object *create_console_input( struct process* renderer )
{
    struct console_input *console_input;

    if (!(console_input = alloc_object( &console_input_ops, -1 ))) return NULL;
    console_input->renderer      = renderer;
    console_input->mode          = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
	                           ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT;
    console_input->num_proc      = 0;
    console_input->active        = NULL;
    console_input->recnum        = 0;
    console_input->records       = NULL;
    console_input->evt           = create_console_input_events();
    console_input->title         = NULL;
    console_input->history_size  = 50;
    console_input->history       = calloc( console_input->history_size, sizeof(WCHAR*) );
    console_input->history_index = 0;
    console_input->history_mode  = 0;

    if (!console_input->history || !console_input->evt)
    {
	release_object( console_input );
	return NULL;
    }
    return &console_input->obj;
}

static struct object *create_console_output( struct console_input *console_input )
{
    struct screen_buffer *screen_buffer;
    struct console_renderer_event evt;
    int	i;

    if (!(screen_buffer = alloc_object( &screen_buffer_ops, -1 ))) return NULL;
    screen_buffer->mode           = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    screen_buffer->input          = console_input;
    screen_buffer->cursor_size    = 100;
    screen_buffer->cursor_visible = 1;
    screen_buffer->width          = 80;
    screen_buffer->height         = 150;
    screen_buffer->max_width      = 80;
    screen_buffer->max_height     = 25;
    screen_buffer->data           = malloc( 4 * screen_buffer->width * screen_buffer->height );
    /* fill the buffer with white on black spaces */
    for (i = 0; i < screen_buffer->width * screen_buffer->height; i++)
    {
	screen_buffer->data[i] = 0x00F00020;
    }
    screen_buffer->cursor.X	  = 0;
    screen_buffer->cursor.Y	  = 0;
    screen_buffer->attr           = 0xF0;
    screen_buffer->win.Left       = 0;
    screen_buffer->win.Right      = screen_buffer->max_width - 1;
    screen_buffer->win.Top        = 0;
    screen_buffer->win.Bottom     = screen_buffer->max_height - 1;

    screen_buffer->next = screen_buffer_list;
    screen_buffer_list = screen_buffer;

    if (!console_input->active)
    {
	console_input->active = (struct screen_buffer*)grab_object( screen_buffer );

	/* generate the fist events */
	evt.event = CONSOLE_RENDERER_ACTIVE_SB_EVENT;
	console_input_events_append( console_input->evt, &evt );

	evt.event = CONSOLE_RENDERER_SB_RESIZE_EVENT;
	evt.u.resize.width  = screen_buffer->width;
	evt.u.resize.height = screen_buffer->height;
	console_input_events_append( console_input->evt, &evt );

	evt.event = CONSOLE_RENDERER_DISPLAY_EVENT;
	evt.u.display.left   = screen_buffer->win.Left;
	evt.u.display.top    = screen_buffer->win.Top;
	evt.u.display.width  = screen_buffer->win.Right - screen_buffer->win.Left + 1;
	evt.u.display.height = screen_buffer->win.Bottom - screen_buffer->win.Top + 1;
	console_input_events_append( console_input->evt, &evt );

	evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
	evt.u.update.top    = 0;
	evt.u.update.bottom = screen_buffer->height - 1;
	console_input_events_append( console_input->evt, &evt );

	evt.event = CONSOLE_RENDERER_CURSOR_GEOM_EVENT;
	evt.u.cursor_geom.size    = screen_buffer->cursor_size;
	evt.u.cursor_geom.visible = screen_buffer->cursor_visible;
	console_input_events_append( console_input->evt, &evt );

	evt.event = CONSOLE_RENDERER_CURSOR_POS_EVENT;
	evt.u.cursor_pos.x = screen_buffer->cursor.X;
	evt.u.cursor_pos.y = screen_buffer->cursor.Y;
	console_input_events_append( console_input->evt, &evt );
    }

    return &screen_buffer->obj;
}

/* free the console for this process */
int free_console( struct process *process )
{
    struct console_input* console = process->console;

    if (!console || !console->renderer) return 0;

    process->console = NULL;
    if (--console->num_proc == 0)
    {
	/* all processes have terminated... tell the renderer to terminate too */
	struct console_renderer_event evt;
	evt.event = CONSOLE_RENDERER_EXIT_EVENT;
	console_input_events_append( console->evt, &evt );
    }
    release_object( console );

    return 1;
}

/* let process inherit the console from parent... this handle two cases :
 *	1/ generic console inheritance
 *	2/ parent is a renderer which launches process, and process should attach to the console
 *	   renderered by parent
 */
void inherit_console(struct process *parent, struct process *process, handle_t hconin)
{
    int done = 0;

    /* if parent is a renderer, then attach current process to its console
     * a bit hacky....
     */
    if (hconin)
    {
	struct console_input* console;

        if ((console = (struct console_input*)get_handle_obj( parent, hconin, 0, NULL )))
	{
	    if (console->renderer == parent)
	    {
		process->console = (struct console_input*)grab_object( console );
		process->console->num_proc++;
		done = 1;
	    }
	    release_object( console );
	}
    }
    /* otherwise, if parent has a console, attach child to this console */
    if (!done && parent->console)
    {
	assert(parent->console->renderer);
	process->console = (struct console_input*)grab_object( parent->console );
	process->console->num_proc++;
    }
}

static struct console_input* console_input_get( handle_t handle, unsigned access )
{
    struct console_input*	console = 0;

    if (handle)
	console = (struct console_input *)get_handle_obj( current->process, handle,
							  access, &console_input_ops );
    else if (current->process->console)
    {
	assert( current->process->console->renderer );
	console = (struct console_input *)grab_object( current->process->console );
    }

    if (!console && !get_error()) set_error(STATUS_INVALID_PARAMETER);
    return console;
}

/* check if a console input is signaled: yes if non read input records */
static int console_input_signaled( struct object *obj, struct thread *thread )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    return console->recnum ? 1 : 0;
}

static int get_console_mode( handle_t handle )
{
    struct object *obj;
    int ret = 0;

    if ((obj = get_handle_obj( current->process, handle, GENERIC_READ, NULL )))
    {
        if (obj->ops == &console_input_ops)
            ret = ((struct console_input *)obj)->mode;
        else if (obj->ops == &screen_buffer_ops)
            ret = ((struct screen_buffer *)obj)->mode;
        else
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
    }
    return ret;
}

/* changes the mode of either a console input or a screen buffer */
static int set_console_mode( handle_t handle, int mode )
{
    struct object *obj;
    int ret = 0;

    if (!(obj = get_handle_obj( current->process, handle, GENERIC_WRITE, NULL )))
        return 0;
    if (obj->ops == &console_input_ops)
    {
	/* FIXME: if we remove the edit mode bits, we need (???) to clean up the history */
        ((struct console_input *)obj)->mode = mode;
        ret = 1;
    }
    else if (obj->ops == &screen_buffer_ops)
    {
        ((struct screen_buffer *)obj)->mode = mode;
        ret = 1;
    }
    else set_error( STATUS_OBJECT_TYPE_MISMATCH );
    release_object( obj );
    return ret;
}

/* add input events to a console input queue */
static int write_console_input( struct console_input* console, int count, INPUT_RECORD *records )
{
    INPUT_RECORD *new_rec;

    assert(count);
    if (!(new_rec = realloc( console->records,
                             (console->recnum + count) * sizeof(INPUT_RECORD) )))
    {
        set_error( STATUS_NO_MEMORY );
        release_object( console );
        return -1;
    }
    console->records = new_rec;
    memcpy( new_rec + console->recnum, records, count * sizeof(INPUT_RECORD) );
    console->recnum += count;

    /* wake up all waiters */
    wake_up( &console->obj, 0 );
    return count;
}

/* retrieve a pointer to the console input records */
static int read_console_input( handle_t handle, int count, INPUT_RECORD *rec, int flush )
{
    struct console_input *console;

    if (!(console = (struct console_input *)get_handle_obj( current->process, handle,
                                                            GENERIC_READ, &console_input_ops )))
        return -1;

    if (!count)
    {
        /* special case: do not retrieve anything, but return
         * the total number of records available */
        count = console->recnum;
    }
    else
    {
        if (count > console->recnum) count = console->recnum;
        memcpy( rec, console->records, count * sizeof(INPUT_RECORD) );
    }
    if (flush)
    {
        int i;
        for (i = count; i < console->recnum; i++)
            ((INPUT_RECORD*)console->records)[i-count] = ((INPUT_RECORD*)console->records)[i];
        if ((console->recnum -= count) > 0)
        {
            INPUT_RECORD *new_rec = realloc( console->records,
                                             console->recnum * sizeof(INPUT_RECORD) );
            if (new_rec) console->records = new_rec;
        }
        else
        {
            free( console->records );
            console->records = NULL;
        }
    }
    release_object( console );
    return count;
}

/* set misc console input information */
static int set_console_input_info( struct set_console_input_info_request *req, 
				   const WCHAR *title, size_t len )
{
    struct console_input *console;
    struct console_renderer_event evt;

    if (!(console = console_input_get( req->handle, GENERIC_WRITE ))) goto error;

    if (req->mask & SET_CONSOLE_INPUT_INFO_ACTIVE_SB)
    {
	struct screen_buffer *screen_buffer;

	screen_buffer = (struct screen_buffer *)get_handle_obj( current->process, req->active_sb, 
								GENERIC_READ, &screen_buffer_ops );
	if (!screen_buffer || screen_buffer->input != console)
	{
	    set_error( STATUS_INVALID_PARAMETER );
	    if (screen_buffer) release_object( screen_buffer );
	    goto error;
	}

	if (screen_buffer != console->active)
	{
	    if (console->active) release_object( console->active );
	    console->active = screen_buffer;
	    evt.event = CONSOLE_RENDERER_ACTIVE_SB_EVENT;
	    console_input_events_append( console->evt, &evt );
	}
	else
	    release_object( screen_buffer );
    }
    if (req->mask & SET_CONSOLE_INPUT_INFO_TITLE)
    {
        WCHAR *new_title = mem_alloc( len + sizeof(WCHAR) );
        if (new_title)
        {
            memcpy( new_title, title, len + sizeof(WCHAR) );
            new_title[len / sizeof(WCHAR)] = 0;
            if (console->title) free( console->title );
            console->title = new_title;
        }
    }
    if (req->mask & SET_CONSOLE_INPUT_INFO_HISTORY_MODE)
    {
	console->history_mode = req->history_mode;
    }
    if ((req->mask & SET_CONSOLE_INPUT_INFO_HISTORY_SIZE) && 
	console->history_size != req->history_size)
    {
	WCHAR**	mem = NULL;
	int	i;
	int	delta;

	if (req->history_size)
	{
	    mem = mem_alloc( req->history_size * sizeof(WCHAR*) );
	    if (!mem) goto error;
	    memset( mem, 0, req->history_size * sizeof(WCHAR*) );
	}

	delta = (console->history_index > req->history_size) ? 
	    (console->history_index - req->history_size) : 0;

	for (i = delta; i < console->history_index; i++)
	{
	    mem[i - delta] = console->history[i];
	    console->history[i] = NULL;
	}
	console->history_index -= delta;

	for (i = 0; i < console->history_size; i++)
	    if (console->history[i]) free( console->history[i] );
	free( console->history );
	console->history = mem;
	console->history_size = req->history_size;
    }
    release_object( console );
    return 1;
 error:
    if (console) release_object( console );
    return 0;
}

/* set misc screen buffer information */
static int set_console_output_info( struct screen_buffer *screen_buffer, 
				    struct set_console_output_info_request *req )
{
    struct console_renderer_event evt;

    if (req->mask & SET_CONSOLE_OUTPUT_INFO_CURSOR_GEOM)
    {
	if (req->cursor_size < 1 || req->cursor_size > 100)
	{
	    set_error( STATUS_INVALID_PARAMETER );
	    return 0;
	}
        if (screen_buffer->cursor_size != req->cursor_size || 
	    screen_buffer->cursor_visible != req->cursor_visible)
	{
	    screen_buffer->cursor_size    = req->cursor_size;
	    screen_buffer->cursor_visible = req->cursor_visible;
	    evt.event = CONSOLE_RENDERER_CURSOR_GEOM_EVENT;
	    evt.u.cursor_geom.size    = req->cursor_size;
	    evt.u.cursor_geom.visible = req->cursor_visible;
	    console_input_events_append( screen_buffer->input->evt, &evt );
	}
    }
    if (req->mask & SET_CONSOLE_OUTPUT_INFO_CURSOR_POS)
    {
	if (req->cursor_x < 0 || req->cursor_x >= screen_buffer->width || 
	    req->cursor_y < 0 || req->cursor_y >= screen_buffer->height)
	{
	    set_error( STATUS_INVALID_PARAMETER );
	    return 0;
	}
	if (screen_buffer->cursor.X != req->cursor_x || screen_buffer->cursor.Y != req->cursor_y)
	{
	    screen_buffer->cursor.X       = req->cursor_x;
	    screen_buffer->cursor.Y       = req->cursor_y;
	    evt.event = CONSOLE_RENDERER_CURSOR_POS_EVENT;
	    evt.u.cursor_pos.x = req->cursor_x;
	    evt.u.cursor_pos.y = req->cursor_y;
	    console_input_events_append( screen_buffer->input->evt, &evt );
	}
    }
    if (req->mask & SET_CONSOLE_OUTPUT_INFO_SIZE)
    {
	int		i, j;
	/* FIXME: there are also some basic minimum and max size to deal with */
	unsigned*	new_data = mem_alloc( 4 * req->width * req->height );

	if (!new_data) return 0;

	/* fill the buffer with either the old buffer content or white on black spaces */
	for (j = 0; j < req->height; j++)
	{
	    for (i = 0; i < req->width; i++)
	    {
		new_data[j * req->width + i] = 
		    (i < screen_buffer->width && j < screen_buffer->height) ?
		    screen_buffer->data[j * screen_buffer->width + i] : 0x00F00020;
	    }	
	}
	free( screen_buffer->data );
	screen_buffer->data = new_data;
	screen_buffer->width = req->width;
	screen_buffer->height = req->height;
	evt.event = CONSOLE_RENDERER_SB_RESIZE_EVENT;
	evt.u.resize.width  = req->width;
	evt.u.resize.height = req->height;
	console_input_events_append( screen_buffer->input->evt, &evt );

	if (screen_buffer == screen_buffer->input->active && 
	    screen_buffer->input->mode & ENABLE_WINDOW_INPUT)
	{
	    INPUT_RECORD	ir;
	    ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
	    ir.Event.WindowBufferSizeEvent.dwSize.X = req->width;
	    ir.Event.WindowBufferSizeEvent.dwSize.Y = req->height;
	    write_console_input( screen_buffer->input, 1, &ir );
	}
    }
    if (req->mask & SET_CONSOLE_OUTPUT_INFO_ATTR)
    {
	screen_buffer->attr = req->attr;
    }
    if (req->mask & SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW)
    {
	if (req->win_left < 0 || req->win_left > req->win_right || 
	    req->win_right >= screen_buffer->width ||
	    req->win_top < 0  || req->win_top > req->win_bottom || 
	    req->win_bottom >= screen_buffer->height)
	{
	    set_error( STATUS_INVALID_PARAMETER );
	    return 0;
	}
	if (screen_buffer->win.Left != req->win_left || screen_buffer->win.Top != req->win_top ||
	    screen_buffer->win.Right != req->win_right || screen_buffer->win.Bottom != req->win_bottom)
	{
	    screen_buffer->win.Left   = req->win_left;
	    screen_buffer->win.Top    = req->win_top;
	    screen_buffer->win.Right  = req->win_right;
	    screen_buffer->win.Bottom = req->win_bottom;
	    evt.event = CONSOLE_RENDERER_DISPLAY_EVENT;
	    evt.u.display.left   = req->win_left;
	    evt.u.display.top    = req->win_top;
	    evt.u.display.width  = req->win_right - req->win_left + 1;
	    evt.u.display.height = req->win_bottom - req->win_top + 1;
	    console_input_events_append( screen_buffer->input->evt, &evt );
	}
    }
    if (req->mask & SET_CONSOLE_OUTPUT_INFO_MAX_SIZE)
    {
	/* can only be done by renderer */
	if (current->process->console != screen_buffer->input)
	{
	    set_error( STATUS_INVALID_PARAMETER );
	    return 0;
	}

	screen_buffer->max_width  = req->max_width;
	screen_buffer->max_height = req->max_height;
    }

    return 1;
}

/* appends a new line to history (history is a fixed size array) */
static void console_input_append_hist( struct console_input* console, const WCHAR* buf, size_t len )
{
    WCHAR*	ptr = mem_alloc( (len + 1) * sizeof(WCHAR) );

    if (!ptr)
    {
	set_error( STATUS_NO_MEMORY );
	return;
    }
    if (!console || !console->history_size)
    {
	set_error( STATUS_INVALID_PARAMETER ); /* FIXME */
	return;
    }

    memcpy( ptr, buf, len * sizeof(WCHAR) );
    ptr[len] = 0;

    if (console->history_mode && console->history_index &&
	strncmpW( console->history[console->history_index - 1], ptr, len * sizeof(WCHAR) ) == 0)
    {
	/* ok, mode ask us to not use twice the same string...
	 * so just free mem and returns
	 */
	set_error( STATUS_ALIAS_EXISTS );
	free(ptr);
	return;
    }

    if (console->history_index < console->history_size)
    {
	console->history[console->history_index++] = ptr;
    }
    else
    {
	free( console->history[0]) ;
	memmove( &console->history[0], &console->history[1], 
		 (console->history_size - 1) * sizeof(WCHAR*) );
	console->history[console->history_size - 1] = ptr;
    }
}

/* returns a line from the cachde */
static int console_input_get_hist( struct console_input* console, WCHAR* buf, size_t len, int index )
{
    int	ret;

    /* FIXME: don't use len yet */
    if (!console || index >= console->history_index)
    {
	set_error( STATUS_INVALID_PARAMETER );
	return 0;
    }
    ret = strlenW(console->history[index]);
    memcpy( buf, console->history[index], ret * sizeof(WCHAR) ); /* FIXME should use len */
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

static int console_get_file_info( struct object *obj, struct get_file_info_request *req )
{
    if (req)
    {
        req->type        = FILE_TYPE_CHAR;
        req->attr        = 0;
        req->access_time = 0;
        req->write_time  = 0;
        req->size_high   = 0;
        req->size_low    = 0;
        req->links       = 0;
        req->index_high  = 0;
        req->index_low   = 0;
        req->serial      = 0;
    }
    return FD_TYPE_CONSOLE;
}

static void console_input_destroy( struct object *obj )
{
    struct console_input*	console_in = (struct console_input *)obj;
    struct screen_buffer*	curr;
    int				i;

    assert( obj->ops == &console_input_ops );
    if (console_in->title) free( console_in->title );
    if (console_in->records) free( console_in->records );

    if (console_in->active)	release_object( console_in->active );
    console_in->active = NULL;

    for (curr = screen_buffer_list; curr; curr = curr->next)
    {
	if (curr->input == console_in) curr->input = NULL;
    }

    release_object( console_in->evt );
    console_in->evt = NULL;

    for (i = 0; i < console_in->history_size; i++)
	if (console_in->history[i]) free( console_in->history[i] );
    if (console_in->history) free( console_in->history );
}

static void screen_buffer_dump( struct object *obj, int verbose )
{
    struct screen_buffer *screen_buffer = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );

    fprintf(stderr, "Console screen buffer input=%p\n", screen_buffer->input );
}

static void screen_buffer_destroy( struct object *obj )
{
    struct screen_buffer*	screen_buffer = (struct screen_buffer *)obj;
    struct screen_buffer**	psb;

    assert( obj->ops == &screen_buffer_ops );

    for (psb = &screen_buffer_list; *psb; *psb = (*psb)->next)
    {
	if (*psb == screen_buffer)
	{
	    *psb = screen_buffer->next;
	    break;
	}
    }	
    if (screen_buffer->input && screen_buffer->input->active == screen_buffer)
    {
	struct screen_buffer*	sb;
	for (sb = screen_buffer_list; sb && sb->input != screen_buffer->input; sb = sb->next);
	screen_buffer->input->active = sb;
    }
}

/* write data into a screen buffer */
static int write_console_output( struct screen_buffer *screen_buffer, size_t size, 
				 const unsigned char* data, int mode, short int x, short int y )
{
    int 			uniform = mode & WRITE_CONSOLE_MODE_UNIFORM;
    unsigned		       *ptr;
    unsigned			i, inc;
    int				len;

    mode &= ~WRITE_CONSOLE_MODE_UNIFORM;

    if (mode < 0 || mode > 3)
    {
	set_error(STATUS_INVALID_PARAMETER);
	return 0;
    }

    /* set destination pointer and increment */
    ptr = screen_buffer->data + (y * screen_buffer->width + x);
    if (mode == WRITE_CONSOLE_MODE_ATTR) ptr = (unsigned*)((char*)ptr + 2);
    inc = (mode == WRITE_CONSOLE_MODE_TEXTATTR) ? 4 : 2;
    len = size / inc;

    /* crop if needed */
    if (x + len > screen_buffer->width) len = screen_buffer->width - x;

    for (i = 0; i < len; i++)
    {
	if (mode == WRITE_CONSOLE_MODE_TEXTSTDATTR)
	{
	    memcpy( (char*)ptr + 2, &screen_buffer->attr, 2 );
	}	
	memcpy( ptr++, data, inc );
	if (!uniform) data += inc;
    }

    if (len && screen_buffer == screen_buffer->input->active)
    {
	int	y2;
	struct console_renderer_event evt;

	y2 = (y * screen_buffer->width + x + len - 1) / screen_buffer->width;

	evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
	evt.u.update.top    = y;
	evt.u.update.bottom = y2;
	console_input_events_append( screen_buffer->input->evt, &evt );
    }
    return len;
}

/* read data from a screen buffer */
static int read_console_output( struct screen_buffer *screen_buffer, size_t size, void* data, 
				short int x, short int y, short int w, short int h, 
				short int* eff_w, short int* eff_h )
{
    int	j;

    if (size < w * h * 4 || x >= screen_buffer->width || y >= screen_buffer->height)
    {	
	set_error(STATUS_INVALID_PARAMETER);
	return 0;
    }

    *eff_w = w;
    *eff_h = h;
    if (x + w > screen_buffer->width)  *eff_w = screen_buffer->width  - x;
    if (y + h > screen_buffer->height) *eff_h = screen_buffer->height - y;

    for (j = 0; j < *eff_h; j++)
    {
	memcpy( (char*)data + 4 * j * w, &screen_buffer->data[(y + j) * screen_buffer->width + x],
	        *eff_w * 4 );
    }

    return *eff_w * *eff_h;
}

/* scroll parts of a screen buffer */
static void scroll_console_output( handle_t handle, short int xsrc, short int ysrc, 
				   short int xdst, short int ydst, short int w, short int h )
{
    struct screen_buffer *screen_buffer;
    int				j;
    unsigned*			psrc;
    unsigned*			pdst;
    struct console_renderer_event evt;

    if (!(screen_buffer = (struct screen_buffer *)get_handle_obj( current->process, handle,
								  GENERIC_READ, &screen_buffer_ops )))
	return;
    if (xsrc < 0 || ysrc < 0 || xdst < 0 || ydst < 0 ||
	xsrc + w > screen_buffer->width  ||
	xdst + w > screen_buffer->width  ||
	ysrc + h > screen_buffer->height ||
	ydst + h > screen_buffer->height ||
	w == 0 || h == 0)
    {
	set_error( STATUS_INVALID_PARAMETER );
	release_object( screen_buffer );
	return;
    }

    if (ysrc < ydst)
    {
	psrc = &screen_buffer->data[(ysrc + h - 1) * screen_buffer->width + xsrc];
	pdst = &screen_buffer->data[(ydst + h - 1) * screen_buffer->width + xdst];

	for (j = h; j > 0; j--)
	{
	    memcpy(pdst, psrc, w * 4);
	    pdst -= screen_buffer->width;
	    psrc -= screen_buffer->width;
	}
    }
    else
    {
	psrc = &screen_buffer->data[ysrc * screen_buffer->width + xsrc];
	pdst = &screen_buffer->data[ydst * screen_buffer->width + xdst];

	for (j = 0; j < h; j++)
	{
	    /* we use memmove here because when psrc and pdst are the same, 
	     * copies are done on the same row, so the dst and src blocks
	     * can overlap */
	    memmove( pdst, psrc, w * 4 );
	    pdst += screen_buffer->width;
	    psrc += screen_buffer->width;
	}
    }

    /* FIXME: this could be enhanced, by signalling scroll */
    evt.event = CONSOLE_RENDERER_UPDATE_EVENT;
    evt.u.update.top    = min(ysrc, ydst);
    evt.u.update.bottom = max(ysrc, ydst) + h - 1;
    console_input_events_append( screen_buffer->input->evt, &evt );

    release_object( screen_buffer );
}

/* allocate a console for the renderer */
DECL_HANDLER(alloc_console)
{
    handle_t in = 0;
    handle_t evt = 0;
    struct process *process;
    struct process *renderer = current->process;
    struct console_input *console;

    process = (req->pid) ? get_process_from_id( req->pid ) :
              (struct process *)grab_object( renderer->parent );

    req->handle_in = 0;
    req->event = 0;
    if (!process) return;
    if (process != renderer && process->console)
    {	
	set_error( STATUS_ACCESS_DENIED );
	goto the_end;
    }

    if ((console = (struct console_input*)create_console_input( renderer )))
    {
	if ((in = alloc_handle( renderer, console, req->access, req->inherit )))
	{
	    if ((evt = alloc_handle( renderer, console->evt, 
				     SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE, FALSE )))
	    {
		if (process != renderer)
		{
		    process->console = (struct console_input*)grab_object( console );
		    console->num_proc++;
		}
		req->handle_in = in;
		req->event = evt;
		release_object( console );
		goto the_end;
	    }
	    close_handle( renderer, in, NULL );
	}
	free_console( process );
    }
 the_end:
    release_object( process );
}

/* free the console of the current process */
DECL_HANDLER(free_console)
{
    free_console( current->process );
}

/* let the renderer peek the events it's waiting on */
DECL_HANDLER(get_console_renderer_events)
{
    struct console_input_events*	evt;
    size_t len = 0;

    evt = (struct console_input_events *)get_handle_obj( current->process, req->handle,
							 GENERIC_WRITE, &console_input_events_ops );
    if (!evt) return;
    len = console_input_events_get( evt, get_req_data(req), get_req_data_size(req) );
    set_req_data_size( req, len );
    release_object( evt );
}

/* open a handle to the process console */
DECL_HANDLER(open_console)
{
    struct object      *obj = NULL;

    req->handle = 0;
    switch (req->from)
    {
    case 0: 
	if (current->process->console && current->process->console->renderer)
	    obj = grab_object( (struct object*)current->process->console );
	break;
    case 1: 
	 if (current->process->console && current->process->console->renderer && 
	     current->process->console->active)
	     obj = grab_object( (struct object*)current->process->console->active );
	break;
    default:
	if ((obj = get_handle_obj( current->process, (handle_t)req->from,
				   GENERIC_READ|GENERIC_WRITE, &console_input_ops )))
	{
	    struct console_input* console = (struct console_input*)obj;
	    obj = (console->active) ? grab_object( console->active ) : NULL;
	    release_object( console );
	}
	break;
    }

    /* FIXME: req->share is not used (as in screen buffer creation)  */
    if (obj)
    {
	req->handle = alloc_handle( current->process, obj, req->access, req->inherit );
	release_object( obj );
    }
    if (!req->handle && !get_error()) set_error( STATUS_ACCESS_DENIED );
}

/* set info about a console input */
DECL_HANDLER(set_console_input_info)
{
    set_console_input_info( req, get_req_data(req), get_req_data_size(req) );
}

/* get info about a console (output only) */
DECL_HANDLER(get_console_input_info)
{
    struct console_input *console = 0;

    set_req_data_size( req, 0 );
    if (!(console = console_input_get( req->handle, GENERIC_READ ))) return;

    if (console->title)
    {
	size_t len = strlenW( console->title ) * sizeof(WCHAR);
	if (len > get_req_data_size(req)) len = get_req_data_size(req);
	memcpy( get_req_data(req), console->title, len );
	set_req_data_size( req, len );
    }
    req->history_mode  = console->history_mode;
    req->history_size  = console->history_size;
    req->history_index = console->history_index;

    release_object( console );
}

/* get a console mode (input or output) */
DECL_HANDLER(get_console_mode)
{
    req->mode = get_console_mode( req->handle );
}

/* set a console mode (input or output) */
DECL_HANDLER(set_console_mode)
{
    set_console_mode( req->handle, req->mode );
}

/* add input records to a console input queue */
DECL_HANDLER(write_console_input)
{
    struct console_input *console;

    req->written = 0;
    if (!(console = (struct console_input *)get_handle_obj( current->process, req->handle,
                                                            GENERIC_WRITE, &console_input_ops )))
        return;

    req->written = write_console_input( console, get_req_data_size(req) / sizeof(INPUT_RECORD),
                                        get_req_data(req) );
    release_object( console );
}

/* fetch input records from a console input queue */
DECL_HANDLER(read_console_input)
{
    size_t size = get_req_data_size(req) / sizeof(INPUT_RECORD);
    int res = read_console_input( req->handle, size, get_req_data(req), req->flush );
    /* if size was 0 we didn't fetch anything */
    if (size) set_req_data_size( req, res * sizeof(INPUT_RECORD) );
    req->read = res;
}

/* appends a string to console's history */
DECL_HANDLER(append_console_input_history)
{
    struct console_input*	console;

    if (!(console = console_input_get( req->handle, GENERIC_WRITE ))) return;
    console_input_append_hist( console, get_req_data(req), 
			       get_req_data_size(req) / sizeof(WCHAR) );
    release_object( console );
}

/* appends a string to console's history */
DECL_HANDLER(get_console_input_history)
{
    struct console_input*	console;
    int len;

    if (!(console = console_input_get( req->handle, GENERIC_WRITE ))) return;

    len = console_input_get_hist( console, get_req_data(req), 0 /* FIXME */, req->index );
    set_req_data_size( req, len * sizeof(WCHAR));
    release_object( console );
}

/* creates a screen buffer */
DECL_HANDLER(create_console_output)
{
    struct console_input*	console;
    struct screen_buffer*	screen_buffer;

    if (!(console = console_input_get( req->handle_in, GENERIC_WRITE))) return;

    screen_buffer = (struct screen_buffer*)create_console_output( console );
    if (screen_buffer)
    {
	/* FIXME: should store sharing and test it when opening the CONOUT$ device 
	 * see file.c on how this could be done
	 */
	req->handle_out = alloc_handle( current->process, screen_buffer, req->access, req->inherit );
	release_object( screen_buffer );
    }
    release_object( console );
}

/* set info about a console screen buffer */
DECL_HANDLER(set_console_output_info)
{
    struct screen_buffer       *screen_buffer;

    if (!(screen_buffer = (struct screen_buffer*)get_handle_obj( current->process, req->handle,
								 GENERIC_WRITE, &screen_buffer_ops )))
	return;

    set_console_output_info( screen_buffer, req );
    release_object( screen_buffer );
}

/* get info about a console screen buffer */
DECL_HANDLER(get_console_output_info)
{
    struct screen_buffer *screen_buffer;
    size_t len = 0;

    if ((screen_buffer = (struct screen_buffer *)get_handle_obj( current->process, req->handle,
								 GENERIC_READ, &screen_buffer_ops )))
    {
        req->cursor_size    = screen_buffer->cursor_size;
        req->cursor_visible = screen_buffer->cursor_visible;
	req->cursor_x       = screen_buffer->cursor.X;
	req->cursor_y       = screen_buffer->cursor.Y;
	req->width          = screen_buffer->width;
	req->height         = screen_buffer->height;
	req->attr           = screen_buffer->attr;
	req->win_left       = screen_buffer->win.Left;
	req->win_top        = screen_buffer->win.Top;
	req->win_right      = screen_buffer->win.Right;
	req->win_bottom     = screen_buffer->win.Bottom;
	req->max_width      = screen_buffer->max_width;
	req->max_height     = screen_buffer->max_height;

        release_object( screen_buffer );
    }
    set_req_data_size( req, len );
}

/* read data (chars & attrs) from a screen buffer */
DECL_HANDLER(read_console_output)
{
    struct screen_buffer       *screen_buffer;
    size_t 			size = get_req_data_size(req);
    int				res;

    if (!(screen_buffer = (struct screen_buffer*)get_handle_obj( current->process, req->handle,
								 GENERIC_READ, &screen_buffer_ops )))
	return;

    res = read_console_output( screen_buffer, size, get_req_data(req), 
			       req->x, req->y, req->w, req->h, &req->eff_w, &req->eff_h);
    /* if size was 0 we didn't fetch anything */
    if (size) set_req_data_size( req, res * 4 );
    release_object( screen_buffer );
}

/* write data (char and/or attrs) to a screen buffer */
DECL_HANDLER(write_console_output)
{
    struct screen_buffer       *screen_buffer;
    size_t 			size = get_req_data_size(req);
    int				res;

    if (!(screen_buffer = (struct screen_buffer*)get_handle_obj( current->process, req->handle,
								 GENERIC_WRITE, &screen_buffer_ops )))
	return;

    res = write_console_output( screen_buffer, size, get_req_data(req), req->mode, req->x, req->y );

    /* if size was 0 we didn't fetch anything */
    if (size) set_req_data_size( req, res );
    req->written = res;
    release_object( screen_buffer );
}

/* move a rect of data in a screen buffer */
DECL_HANDLER(move_console_output)
{
    scroll_console_output( req->handle, req->x_src, req->y_src, req->x_dst, req->y_dst, 
			   req->w, req->h );
}

/*
 * Server-side console management
 *
 * Copyright (C) 1998 Alexandre Julliard
 *
 * FIXME: all this stuff is a hack to avoid breaking
 *        the client-side console support.
 */

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "winerror.h"
#include "winnt.h"
#include "wincon.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"

struct screen_buffer;

struct console_input
{
    struct object         obj;           /* object header */
    struct select_user    select;        /* select user */
    int                   mode;          /* input mode */
    struct screen_buffer *output;        /* associated screen buffer */
    int                   recnum;        /* number of input records */
    INPUT_RECORD         *records;       /* input records */
};

struct screen_buffer
{
    struct object         obj;           /* object header */
    struct select_user    select;        /* select user */
    int                   mode;          /* output mode */
    struct console_input *input;         /* associated console input */
    int                   cursor_size;   /* size of cursor (percentage filled) */
    int                   cursor_visible;/* cursor visibility flag */
    int                   pid;           /* xterm pid (hack) */
    char                 *title;         /* console title */
};


static void console_input_dump( struct object *obj, int verbose );
static int console_input_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void console_input_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int console_input_signaled( struct object *obj, struct thread *thread );
static int console_input_get_read_fd( struct object *obj );
static void console_input_destroy( struct object *obj );

static void screen_buffer_dump( struct object *obj, int verbose );
static int screen_buffer_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void screen_buffer_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int screen_buffer_signaled( struct object *obj, struct thread *thread );
static int screen_buffer_get_write_fd( struct object *obj );
static void screen_buffer_destroy( struct object *obj );

/* common routine */
static int console_get_info( struct object *obj, struct get_file_info_reply *reply );

static const struct object_ops console_input_ops =
{
    sizeof(struct console_input),
    console_input_dump,
    console_input_add_queue,
    console_input_remove_queue,
    console_input_signaled,
    no_satisfied,
    console_input_get_read_fd,
    no_write_fd,
    no_flush,
    console_get_info,
    console_input_destroy
};

static const struct object_ops screen_buffer_ops =
{
    sizeof(struct screen_buffer),
    screen_buffer_dump,
    screen_buffer_add_queue,
    screen_buffer_remove_queue,
    screen_buffer_signaled,
    no_satisfied,
    no_read_fd,
    screen_buffer_get_write_fd,
    no_flush,
    console_get_info,
    screen_buffer_destroy
};


static struct object *create_console_input( int fd )
{
    struct console_input *console_input;

    if ((fd = (fd != -1) ? dup(fd) : dup(0)) == -1)
    {
        file_set_error();
        return NULL;
    }
    if ((console_input = alloc_object( &console_input_ops )))
    {
        console_input->select.fd      = fd;
        console_input->select.func    = default_select_event;
        console_input->select.private = console_input;
        console_input->mode           = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                                        ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT;
        console_input->output         = NULL;
        console_input->recnum         = 0;
        console_input->records        = NULL;
        register_select_user( &console_input->select );
        return &console_input->obj;
    }
    close( fd );
    return NULL;
}

static struct object *create_console_output( int fd, struct object *input )
{
    struct console_input *console_input = (struct console_input *)input;
    struct screen_buffer *screen_buffer;

    if ((fd = (fd != -1) ? dup(fd) : dup(1)) == -1)
    {
        file_set_error();
        return NULL;
    }
    if ((screen_buffer = alloc_object( &screen_buffer_ops )))
    {
        screen_buffer->select.fd      = fd;
        screen_buffer->select.func    = default_select_event;
        screen_buffer->select.private = screen_buffer;
        screen_buffer->mode           = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
        screen_buffer->input          = console_input;
        screen_buffer->cursor_size    = 100;
        screen_buffer->cursor_visible = 1;
        screen_buffer->pid            = 0;
        screen_buffer->title          = strdup( "Wine console" );
        register_select_user( &screen_buffer->select );
        console_input->output = screen_buffer;
        return &screen_buffer->obj;
    }
    close( fd );
    return NULL;
}

/* allocate a console for this process */
int alloc_console( struct process *process )
{
    if (process->console_in || process->console_out)
    {
        set_error( ERROR_ACCESS_DENIED );
        return 0;
    }
    if ((process->console_in = create_console_input( -1 )))
    {
        if ((process->console_out = create_console_output( -1, process->console_in )))
            return 1;
        release_object( process->console_in );
    }
    return 0;
}

/* free the console for this process */
int free_console( struct process *process )
{
    if (process->console_in) release_object( process->console_in );
    if (process->console_out) release_object( process->console_out );
    process->console_in = process->console_out = NULL;
    return 1;
}

static int set_console_fd( int handle, int fd, int pid )
{
    struct console_input *input;
    struct screen_buffer *output;
    struct object *obj;
    int fd_in, fd_out;

    if (!(obj = get_handle_obj( current->process, handle, 0, NULL )))
        return 0;
    if (obj->ops == &console_input_ops)
    {
        input = (struct console_input *)obj;
        output = input->output;
        grab_object( output );
    }
    else if (obj->ops == &screen_buffer_ops)
    {
        output = (struct screen_buffer *)obj;
        input = output->input;
        grab_object( input );
    }
    else
    {
        set_error( ERROR_INVALID_HANDLE );
        release_object( obj );
        return 0;
    }

    /* can't change the fd if someone is waiting on it */
    assert( !input->obj.head );
    assert( !output->obj.head );

    if ((fd_in = dup(fd)) == -1)
    {
        file_set_error();
        release_object( input );
        release_object( output );
        return 0;
    }
    if ((fd_out = dup(fd)) == -1)
    {
        file_set_error();
        close( fd_in );
        release_object( input );
        release_object( output );
        return 0;
    }
    unregister_select_user( &input->select );
    unregister_select_user( &output->select );
    close( input->select.fd );
    close( output->select.fd );
    input->select.fd  = fd_in;
    output->select.fd = fd_out;
    output->pid       = pid;
    register_select_user( &input->select );
    register_select_user( &output->select );
    release_object( input );
    release_object( output );
    return 1;
}

static int get_console_mode( int handle, int *mode )
{
    struct object *obj;
    int ret = 0;

    if (!(obj = get_handle_obj( current->process, handle, GENERIC_READ, NULL )))
        return 0;
    if (obj->ops == &console_input_ops)
    {
        *mode = ((struct console_input *)obj)->mode;
        ret = 1;
    }
    else if (obj->ops == &screen_buffer_ops)
    {
        *mode = ((struct screen_buffer *)obj)->mode;
        ret = 1;
    }
    else set_error( ERROR_INVALID_HANDLE );
    release_object( obj );
    return ret;
}

static int set_console_mode( int handle, int mode )
{
    struct object *obj;
    int ret = 0;

    if (!(obj = get_handle_obj( current->process, handle, GENERIC_READ, NULL )))
        return 0;
    if (obj->ops == &console_input_ops)
    {
        ((struct console_input *)obj)->mode = mode;
        ret = 1;
    }
    else if (obj->ops == &screen_buffer_ops)
    {
        ((struct screen_buffer *)obj)->mode = mode;
        ret = 1;
    }
    else set_error( ERROR_INVALID_HANDLE );
    release_object( obj );
    return ret;
}

/* set misc console information (output handle only) */
static int set_console_info( int handle, struct set_console_info_request *req,
                             const char *title, size_t len )
{
    struct screen_buffer *console;
    if (!(console = (struct screen_buffer *)get_handle_obj( current->process, handle,
                                                            GENERIC_WRITE, &screen_buffer_ops )))
        return 0;
    if (req->mask & SET_CONSOLE_INFO_CURSOR)
    {
        console->cursor_size    = req->cursor_size;
        console->cursor_visible = req->cursor_visible;
    }
    if (req->mask & SET_CONSOLE_INFO_TITLE)
    {
        char *new_title = mem_alloc( len + 1 );
        if (new_title)
        {
            memcpy( new_title, title, len );
            new_title[len] = 0;
            if (console->title) free( console->title );
            console->title = new_title;
        }
    }
    release_object( console );
    return 1;
}

/* get misc console information (output handle only) */
static int get_console_info( int handle, struct get_console_info_reply *reply, const char **title )
{
    struct screen_buffer *console;
    if (!(console = (struct screen_buffer *)get_handle_obj( current->process, handle,
                                                            GENERIC_READ, &screen_buffer_ops )))
        return 0;
    reply->cursor_size    = console->cursor_size;
    reply->cursor_visible = console->cursor_visible;
    reply->pid            = console->pid;
    *title                = console->title;
    release_object( console );
    return 1;
}

/* add input events to a console input queue */
static int write_console_input( int handle, int count, INPUT_RECORD *records )
{
    INPUT_RECORD *new_rec;
    struct console_input *console;

    if (!(console = (struct console_input *)get_handle_obj( current->process, handle,
                                                            GENERIC_WRITE, &console_input_ops )))
        return -1;
    if (!(new_rec = realloc( console->records,
                             (console->recnum + count) * sizeof(INPUT_RECORD) )))
    {
        set_error( ERROR_NOT_ENOUGH_MEMORY );
        release_object( console );
        return -1;
    }
    console->records = new_rec;
    memcpy( new_rec + console->recnum, records, count * sizeof(INPUT_RECORD) );
    console->recnum += count;
    release_object( console );
    return count;
}

/* retrieve a pointer to the console input records */
static int read_console_input( int handle, int count, int flush )
{
    struct console_input *console;
    struct read_console_input_reply *reply = push_reply_data( current, sizeof(*reply) );

    if (!(console = (struct console_input *)get_handle_obj( current->process, handle,
                                                            GENERIC_READ, &console_input_ops )))
        return -1;
    if ((count < 0) || (count > console->recnum)) count = console->recnum;
    add_reply_data( current, console->records, count * sizeof(INPUT_RECORD) );
    if (flush)
    {
        int i;
        for (i = count; i < console->recnum; i++)
            console->records[i-count] = console->records[i];
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

static void console_input_dump( struct object *obj, int verbose )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    fprintf( stderr, "Console input fd=%d\n", console->select.fd );
}

static int console_input_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    if (!obj->head)  /* first on the queue */
        set_select_events( &console->select, READ_EVENT );
    add_queue( obj, entry );
    return 1;
}

static void console_input_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct console_input *console = (struct console_input *)grab_object(obj);
    assert( obj->ops == &console_input_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        set_select_events( &console->select, 0 );
    release_object( obj );
}

static int console_input_signaled( struct object *obj, struct thread *thread )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );

    if (check_select_events( &console->select, READ_EVENT ))
    {
        /* stop waiting on select() if we are signaled */
        set_select_events( &console->select, 0 );
        return 1;
    }
    else
    {
        /* restart waiting on select() if we are no longer signaled */
        if (obj->head) set_select_events( &console->select, READ_EVENT );
        return 0;
    }
}

static int console_input_get_read_fd( struct object *obj )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    return dup( console->select.fd );
}

static int console_get_info( struct object *obj, struct get_file_info_reply *reply )
{
    memset( reply, 0, sizeof(*reply) );
    reply->type = FILE_TYPE_CHAR;
    return 1;
}

static void console_input_destroy( struct object *obj )
{
    struct console_input *console = (struct console_input *)obj;
    assert( obj->ops == &console_input_ops );
    unregister_select_user( &console->select );
    close( console->select.fd );
    if (console->output) console->output->input = NULL;
}

static void screen_buffer_dump( struct object *obj, int verbose )
{
    struct screen_buffer *console = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );
    fprintf( stderr, "Console screen buffer fd=%d\n", console->select.fd );
}

static int screen_buffer_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct screen_buffer *console = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );
    if (!obj->head)  /* first on the queue */
        set_select_events( &console->select, WRITE_EVENT );
    add_queue( obj, entry );
    return 1;
}

static void screen_buffer_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct screen_buffer *console = (struct screen_buffer *)grab_object(obj);
    assert( obj->ops == &screen_buffer_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        set_select_events( &console->select, 0 );
    release_object( obj );
}

static int screen_buffer_signaled( struct object *obj, struct thread *thread )
{
    struct screen_buffer *console = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );

    if (check_select_events( &console->select, WRITE_EVENT ))
    {
        /* stop waiting on select() if we are signaled */
        set_select_events( &console->select, 0 );
        return 1;
    }
    else
    {
        /* restart waiting on select() if we are no longer signaled */
        if (obj->head) set_select_events( &console->select, WRITE_EVENT );
        return 0;
    }
}

static int screen_buffer_get_write_fd( struct object *obj )
{
    struct screen_buffer *console = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );
    return dup( console->select.fd );
}

static void screen_buffer_destroy( struct object *obj )
{
    struct screen_buffer *console = (struct screen_buffer *)obj;
    assert( obj->ops == &screen_buffer_ops );
    unregister_select_user( &console->select );
    close( console->select.fd );
    if (console->input) console->input->output = NULL;
    if (console->pid) kill( console->pid, SIGTERM );
    if (console->title) free( console->title );
}

/* allocate a console for the current process */
DECL_HANDLER(alloc_console)
{
    struct alloc_console_reply *reply = push_reply_data( current, sizeof(*reply) );
    int in = -1, out = -1;

    if (!alloc_console( current->process )) goto done;

    if ((in = alloc_handle( current->process, current->process->console_in,
                            req->access, req->inherit )) != -1)
    {
        if ((out = alloc_handle( current->process, current->process->console_out,
                                 req->access, req->inherit )) != -1)
            goto done;  /* everything is fine */
        close_handle( current->process, in );
        in = -1;
    }
    free_console( current->process );

 done:
    reply->handle_in  = in;
    reply->handle_out = out;
}

/* free the console of the current process */
DECL_HANDLER(free_console)
{
    free_console( current->process );
}

/* open a handle to the process console */
DECL_HANDLER(open_console)
{
    struct open_console_reply *reply = push_reply_data( current, sizeof(*reply) );
    struct object *obj= req->output ? current->process->console_out : current->process->console_in;

    if (obj) reply->handle = alloc_handle( current->process, obj, req->access, req->inherit );
    else set_error( ERROR_ACCESS_DENIED );
}

/* set info about a console (output only) */
DECL_HANDLER(set_console_info)
{
    size_t len = get_req_strlen();
    set_console_info( req->handle, req, get_req_data( len + 1 ), len );
}

/* get info about a console (output only) */
DECL_HANDLER(get_console_info)
{
    struct get_console_info_reply *reply = push_reply_data( current, sizeof(*reply) );
    const char *title;
    get_console_info( req->handle, reply, &title );
    if (title) add_reply_data( current, title, strlen(title) + 1 );
}

/* set a console fd */
DECL_HANDLER(set_console_fd)
{
    set_console_fd( req->handle, fd, req->pid );
}

/* get a console mode (input or output) */
DECL_HANDLER(get_console_mode)
{
    struct get_console_mode_reply *reply = push_reply_data( current, sizeof(*reply) );
    get_console_mode( req->handle, &reply->mode );
}

/* set a console mode (input or output) */
DECL_HANDLER(set_console_mode)
{
    set_console_mode( req->handle, req->mode );
}

/* add input records to a console input queue */
DECL_HANDLER(write_console_input)
{
    struct write_console_input_reply *reply = push_reply_data( current, sizeof(*reply) );

    if (check_req_data( req->count * sizeof(INPUT_RECORD)))
    {
        INPUT_RECORD *records = get_req_data( req->count * sizeof(INPUT_RECORD) );
        reply->written = write_console_input( req->handle, req->count, records );
    }
    else fatal_protocol_error( "write_console_input: bad length" );
}

/* fetch input records from a console input queue */
DECL_HANDLER(read_console_input)
{
    read_console_input( req->handle, req->count, req->flush );
}

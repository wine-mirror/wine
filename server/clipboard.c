/*
 * Server-side clipboard management
 *
 * Copyright 2002 Ulrich Czekalla
 * Copyright 2016 Alexandre Julliard
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "request.h"
#include "object.h"
#include "file.h"
#include "process.h"
#include "user.h"
#include "winuser.h"
#include "winternl.h"

struct clip_format
{
    struct list    entry;            /* entry in format list */
    unsigned int   id;               /* format id */
    data_size_t    size;             /* size of the data block */
    void          *data;             /* data contents, or NULL for delay-rendered */
};

struct clipboard
{
    struct object  obj;              /* object header */
    struct thread *open_thread;      /* thread id that has clipboard open */
    user_handle_t  open_win;         /* window that has clipboard open */
    struct thread *owner_thread;     /* thread id that owns the clipboard */
    user_handle_t  owner_win;        /* window that owns the clipboard data */
    user_handle_t  viewer;           /* first window in clipboard viewer list */
    unsigned int   seqno;            /* clipboard change sequence number */
    unsigned int   open_seqno;       /* sequence number at open time */
    struct list    formats;          /* list of data formats */
    unsigned int   format_count;     /* count of data formats */
    unsigned int   listen_size;      /* size of listeners array */
    unsigned int   listen_count;     /* count of listeners */
    user_handle_t *listeners;        /* array of listener windows */
};

static void clipboard_dump( struct object *obj, int verbose );
static void clipboard_destroy( struct object *obj );

static const struct object_ops clipboard_ops =
{
    sizeof(struct clipboard),     /* size */
    clipboard_dump,               /* dump */
    no_get_type,                  /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    no_map_access,                /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    clipboard_destroy             /* destroy */
};


/* find a data format in the clipboard */
static struct clip_format *get_format( struct clipboard *clipboard, unsigned int id )
{
    struct clip_format *format;

    LIST_FOR_EACH_ENTRY( format, &clipboard->formats, struct clip_format, entry )
        if (format->id == id) return format;

    return NULL;
}

/* add a data format to the clipboard */
static struct clip_format *add_format( struct clipboard *clipboard, unsigned int id )
{
    struct clip_format *format;

    if (!(format = mem_alloc( sizeof(*format )))) return NULL;
    format->id   = id;
    format->size = 0;
    format->data = NULL;
    list_add_tail( &clipboard->formats, &format->entry );
    clipboard->format_count++;
    return format;
}

/* free all clipboard formats */
static void free_clipboard_formats( struct clipboard *clipboard )
{
    struct clip_format *format, *next;

    LIST_FOR_EACH_ENTRY_SAFE( format, next, &clipboard->formats, struct clip_format, entry )
    {
        list_remove( &format->entry );
        free( format->data );
        free( format );
    }
    clipboard->format_count = 0;
}

/* dump a clipboard object */
static void clipboard_dump( struct object *obj, int verbose )
{
    struct clipboard *clipboard = (struct clipboard *)obj;

    fprintf( stderr, "Clipboard open_thread=%p open_win=%08x owner_thread=%p owner_win=%08x viewer=%08x seq=%u\n",
             clipboard->open_thread, clipboard->open_win, clipboard->owner_thread,
             clipboard->owner_win, clipboard->viewer, clipboard->seqno );
}

static void clipboard_destroy( struct object *obj )
{
    struct clipboard *clipboard = (struct clipboard *)obj;

    free( clipboard->listeners );
    free_clipboard_formats( clipboard );
}

/* retrieve the clipboard info for the current process, allocating it if needed */
static struct clipboard *get_process_clipboard(void)
{
    struct clipboard *clipboard;
    struct winstation *winstation = get_process_winstation( current->process, WINSTA_ACCESSCLIPBOARD );

    if (!winstation) return NULL;

    if (!(clipboard = winstation->clipboard))
    {
        if ((clipboard = alloc_object( &clipboard_ops )))
        {
            clipboard->open_thread = NULL;
            clipboard->open_win = 0;
            clipboard->owner_thread = NULL;
            clipboard->owner_win = 0;
            clipboard->viewer = 0;
            clipboard->seqno = 0;
            clipboard->format_count = 0;
            clipboard->listen_size = 0;
            clipboard->listen_count = 0;
            clipboard->listeners = NULL;
            list_init( &clipboard->formats );
            winstation->clipboard = clipboard;
        }
    }
    release_object( winstation );
    return clipboard;
}

/* add a clipboard listener */
static void add_listener( struct clipboard *clipboard, user_handle_t window )
{
    unsigned int i;

    for (i = 0; i < clipboard->listen_count; i++)
    {
        if (clipboard->listeners[i] != window) continue;
        set_error( STATUS_INVALID_PARAMETER );  /* already set */
        return;
    }
    if (clipboard->listen_size == clipboard->listen_count)
    {
        unsigned int new_size = max( 8, clipboard->listen_size * 2 );
        user_handle_t *new = realloc( clipboard->listeners, new_size * sizeof(*new) );
        if (!new)
        {
            set_error( STATUS_NO_MEMORY );
            return;
        }
        clipboard->listeners = new;
        clipboard->listen_size = new_size;
    }
    clipboard->listeners[clipboard->listen_count++] = window;
}

/* remove a clipboard listener */
static int remove_listener( struct clipboard *clipboard, user_handle_t window )
{
    unsigned int i;

    for (i = 0; i < clipboard->listen_count; i++)
    {
        if (clipboard->listeners[i] != window) continue;
        memmove( clipboard->listeners + i, clipboard->listeners + i + 1,
                 (clipboard->listen_count - i - 1) * sizeof(*clipboard->listeners) );
        clipboard->listen_count--;
        return 1;
    }
    return 0;
}

/* notify all listeners, and return the viewer window that should be notified if any */
static user_handle_t notify_listeners( struct clipboard *clipboard )
{
    unsigned int i;

    for (i = 0; i < clipboard->listen_count; i++)
        post_message( clipboard->listeners[i], WM_CLIPBOARDUPDATE, 0, 0 );
    return clipboard->viewer;
}

/* close the clipboard, and return the viewer window that should be notified if any */
static user_handle_t close_clipboard( struct clipboard *clipboard )
{
    clipboard->open_win = 0;
    clipboard->open_thread = NULL;
    if (clipboard->seqno == clipboard->open_seqno) return 0;  /* unchanged */
    return notify_listeners( clipboard );
}

/* release the clipboard owner, and return the viewer window that should be notified if any */
static user_handle_t release_clipboard( struct clipboard *clipboard )
{
    struct clip_format *format, *next;
    int changed = 0;

    clipboard->owner_win = 0;
    clipboard->owner_thread = NULL;

    /* free the delayed-rendered formats, since we no longer have an owner to render them */
    LIST_FOR_EACH_ENTRY_SAFE( format, next, &clipboard->formats, struct clip_format, entry )
    {
        if (format->data) continue;
        list_remove( &format->entry );
        clipboard->format_count--;
        free( format );
        changed = 1;
    }

    if (!changed) return 0;
    clipboard->seqno++;
    return notify_listeners( clipboard );
}

/* cleanup clipboard information upon window destruction */
void cleanup_clipboard_window( struct desktop *desktop, user_handle_t window )
{
    struct clipboard *clipboard = desktop->winstation->clipboard;

    if (!clipboard) return;

    remove_listener( clipboard, window );
    if (clipboard->viewer == window) clipboard->viewer = 0;
    if (clipboard->owner_win == window) release_clipboard( clipboard );
    if (clipboard->open_win == window)
    {
        user_handle_t viewer = close_clipboard( clipboard );
        if (viewer) send_notify_message( viewer, WM_DRAWCLIPBOARD, clipboard->owner_win, 0 );
    }
}

/* Called when thread terminates to allow release of clipboard */
void cleanup_clipboard_thread(struct thread *thread)
{
    struct clipboard *clipboard;
    struct winstation *winstation;

    if (!thread->process->winstation) return;
    if (!(winstation = get_process_winstation( thread->process, WINSTA_ACCESSCLIPBOARD ))) return;

    if ((clipboard = winstation->clipboard))
    {
        if (thread == clipboard->owner_thread) clipboard->owner_thread = NULL;
        if (thread == clipboard->open_thread)
        {
            user_handle_t viewer = close_clipboard( clipboard );
            if (viewer) send_notify_message( viewer, WM_DRAWCLIPBOARD, clipboard->owner_win, 0 );
        }
    }
    release_object( winstation );
}

static int release_clipboard_owner( struct clipboard *clipboard, user_handle_t win )
{
    if ((clipboard->open_thread && clipboard->open_thread->process != current->process) ||
        (win && clipboard->owner_win != get_user_full_handle( win )))
    {
        set_win32_error( ERROR_CLIPBOARD_NOT_OPEN );
        return 0;
    }
    clipboard->owner_win = 0;
    clipboard->owner_thread = NULL;
    return 1;
}


/* open the clipboard */
DECL_HANDLER(open_clipboard)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t win = 0;

    if (!clipboard) return;
    if (req->window && !(win = get_valid_window_handle( req->window ))) return;

    if (clipboard->open_thread && clipboard->open_win != win)
    {
        set_error( STATUS_INVALID_LOCK_SEQUENCE );
        return;
    }

    if (!clipboard->open_thread) clipboard->open_seqno = clipboard->seqno; /* first open */
    clipboard->open_win = win;
    clipboard->open_thread = current;

    reply->owner = (clipboard->owner_thread && clipboard->owner_thread->process == current->process);
}


/* close the clipboard */
DECL_HANDLER(close_clipboard)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    if (clipboard->open_thread != current)
    {
        set_win32_error( ERROR_CLIPBOARD_NOT_OPEN );
        return;
    }
    reply->viewer = close_clipboard( clipboard );
    reply->owner  = clipboard->owner_win;
}


DECL_HANDLER(set_clipboard_info)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    reply->old_clipboard = clipboard->open_win;
    reply->old_owner     = clipboard->owner_win;
    reply->old_viewer    = clipboard->viewer;

    if (req->flags & SET_CB_RELOWNER)
    {
        if (!release_clipboard_owner( clipboard, req->owner )) return;
    }

    if (req->flags & SET_CB_SEQNO) clipboard->seqno++;

    reply->seqno = clipboard->seqno;

    if (clipboard->open_thread) reply->flags |= CB_OPEN_ANY;
    if (clipboard->open_thread == current) reply->flags |= CB_OPEN;
    if (clipboard->owner_thread == current) reply->flags |= CB_OWNER;
    if (clipboard->owner_thread && clipboard->owner_thread->process == current->process)
        reply->flags |= CB_PROCESS;
}


/* add a data format to the clipboard */
DECL_HANDLER(set_clipboard_data)
{
    struct clip_format *format;
    struct clipboard *clipboard = get_process_clipboard();
    void *data = NULL;

    if (!clipboard) return;

    if (!req->format || !clipboard->open_thread)
    {
        set_win32_error( ERROR_CLIPBOARD_NOT_OPEN );
        return;
    }

    if (get_req_data_size() && !(data = memdup( get_req_data(), get_req_data_size() ))) return;

    if (!(format = get_format( clipboard, req->format )))
    {
        if (!(format = add_format( clipboard, req->format )))
        {
            free( data );
            return;
        }
    }

    clipboard->seqno++;
    format->size   = get_req_data_size();
    format->data   = data;
}


/* empty the clipboard and grab ownership */
DECL_HANDLER(empty_clipboard)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    if (clipboard->open_thread != current)
    {
        set_win32_error( ERROR_CLIPBOARD_NOT_OPEN );
        return;
    }

    free_clipboard_formats( clipboard );
    clipboard->owner_win = clipboard->open_win;
    clipboard->owner_thread = clipboard->open_thread;
    clipboard->seqno++;
}


/* release ownership of the clipboard */
DECL_HANDLER(release_clipboard)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t owner;

    if (!clipboard) return;

    if (!(owner = get_valid_window_handle( req->owner ))) return;

    if (clipboard->owner_win == owner)
    {
        reply->viewer = release_clipboard( clipboard );
        reply->owner = clipboard->owner_win;
    }
    else set_error( STATUS_INVALID_OWNER );
}


/* get clipboard information */
DECL_HANDLER(get_clipboard_info)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    reply->window = clipboard->open_win;
    reply->owner  = clipboard->owner_win;
    reply->viewer = clipboard->viewer;
    reply->seqno  = clipboard->seqno;
}


/* set the clipboard viewer window */
DECL_HANDLER(set_clipboard_viewer)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t viewer = 0, previous = 0;

    if (!clipboard) return;
    if (req->viewer && !(viewer = get_valid_window_handle( req->viewer ))) return;
    if (req->previous && !(previous = get_valid_window_handle( req->previous ))) return;

    reply->old_viewer = clipboard->viewer;
    reply->owner      = clipboard->owner_win;

    if (!previous || clipboard->viewer == previous)
        clipboard->viewer = viewer;
    else
        set_error( STATUS_PENDING );  /* need to send message instead */
}


/* add a clipboard listener window */
DECL_HANDLER(add_clipboard_listener)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t win;

    if (!clipboard) return;
    if (!(win = get_valid_window_handle( req->window ))) return;

    add_listener( clipboard, win );
}


/* remove a clipboard listener window */
DECL_HANDLER(remove_clipboard_listener)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t win;

    if (!clipboard) return;
    if (!(win = get_valid_window_handle( req->window ))) return;

    if (!remove_listener( clipboard, win )) set_error( STATUS_INVALID_PARAMETER );
}

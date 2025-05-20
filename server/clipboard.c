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
    unsigned int   from;             /* for synthesized data, format to generate it from */
    unsigned int   seqno;            /* sequence number when the data was set */
    data_size_t    size;             /* size of the data block */
    void          *data;             /* data contents, or NULL for delay-rendered */
};

struct clipboard
{
    struct object  obj;              /* object header */
    struct thread *open_thread;      /* thread id that has clipboard open */
    user_handle_t  open_win;         /* window that has clipboard open */
    user_handle_t  owner;            /* window that owns the clipboard */
    user_handle_t  viewer;           /* first window in clipboard viewer list */
    unsigned int   lcid;             /* locale id to use for synthesizing text formats */
    unsigned int   seqno;            /* clipboard change sequence number */
    unsigned int   open_seqno;       /* sequence number at open time */
    unsigned int   rendering;        /* format rendering recursion counter */
    struct list    formats;          /* list of data formats */
    unsigned int   format_count;     /* count of data formats */
    unsigned int   format_map;       /* existence bitmap for formats < CF_MAX */
    unsigned int   listen_size;      /* size of listeners array */
    unsigned int   listen_count;     /* count of listeners */
    user_handle_t *listeners;        /* array of listener windows */
};

static void clipboard_dump( struct object *obj, int verbose );
static void clipboard_destroy( struct object *obj );

static const struct object_ops clipboard_ops =
{
    sizeof(struct clipboard),     /* size */
    &no_type,                     /* type */
    clipboard_dump,               /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_get_sync,             /* get_sync */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_get_full_name,             /* get_full_name */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    no_close_handle,              /* close_handle */
    clipboard_destroy             /* destroy */
};


#define HAS_FORMAT(map,id) ((map) & (1 << (id)))  /* only for formats < CF_MAX */

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
    format->from = 0;
    format->size = 0;
    format->data = NULL;
    list_add_tail( &clipboard->formats, &format->entry );
    clipboard->format_count++;
    if (id < CF_MAX) clipboard->format_map |= 1 << id;
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
    clipboard->format_map = 0;
}

/* dump a clipboard object */
static void clipboard_dump( struct object *obj, int verbose )
{
    struct clipboard *clipboard = (struct clipboard *)obj;

    fprintf( stderr, "Clipboard open_thread=%p open_win=%08x owner=%08x viewer=%08x seq=%u\n",
             clipboard->open_thread, clipboard->open_win,
             clipboard->owner, clipboard->viewer, clipboard->seqno );
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
            clipboard->owner = 0;
            clipboard->viewer = 0;
            clipboard->seqno = 0;
            clipboard->format_count = 0;
            clipboard->format_map = 0;
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

/* add synthesized formats upon clipboard close */
static int synthesize_formats( struct clipboard *clipboard )
{
    static const unsigned int formats[][3] =
    {
        { CF_TEXT, CF_OEMTEXT, CF_UNICODETEXT },
        { CF_OEMTEXT, CF_UNICODETEXT, CF_TEXT },
        { CF_UNICODETEXT, CF_TEXT, CF_OEMTEXT },
        { CF_METAFILEPICT, CF_ENHMETAFILE },
        { CF_ENHMETAFILE, CF_METAFILEPICT },
        { CF_BITMAP, CF_DIB, CF_DIBV5 },
        { CF_DIB, CF_BITMAP, CF_DIBV5 },
        { CF_DIBV5, CF_BITMAP, CF_DIB }
    };
    unsigned int i, from, total = 0, map = clipboard->format_map;
    struct clip_format *format;

    if (!HAS_FORMAT( map, CF_LOCALE ) &&
        (HAS_FORMAT( map, CF_TEXT ) || HAS_FORMAT( map, CF_OEMTEXT ) || HAS_FORMAT( map, CF_UNICODETEXT )))
    {
        void *data = memdup( &clipboard->lcid, sizeof(clipboard->lcid) );
        if (data && (format = add_format( clipboard, CF_LOCALE )))
        {
            format->seqno = clipboard->seqno++;
            format->data  = data;
            format->size  = sizeof(clipboard->lcid);
        }
        else free( data );
    }

    for (i = 0; i < ARRAY_SIZE( formats ); i++)
    {
        if (HAS_FORMAT( map, formats[i][0] )) continue;
        if (HAS_FORMAT( map, formats[i][1] )) from = formats[i][1];
        else if (HAS_FORMAT( map, formats[i][2] )) from = formats[i][2];
        else continue;
        if (!(format = add_format( clipboard, formats[i][0] ))) continue;
        format->from = from;
        format->seqno = clipboard->seqno;
        total++;
    }
    return total;
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
    if (synthesize_formats( clipboard )) clipboard->seqno++;
    return notify_listeners( clipboard );
}

/* release the clipboard owner, and return the viewer window that should be notified if any */
static user_handle_t release_clipboard( struct clipboard *clipboard )
{
    struct clip_format *format, *next;
    int changed = 0;

    clipboard->owner = 0;

    /* free the delayed-rendered formats, since we no longer have an owner to render them */
    LIST_FOR_EACH_ENTRY_SAFE( format, next, &clipboard->formats, struct clip_format, entry )
    {
        if (format->data) continue;
        /* format->from is earlier in the list and thus has already been
         * removed if not available anymore (it is also < CF_MAX)
         */
        if (format->from && HAS_FORMAT( clipboard->format_map, format->from )) continue;
        list_remove( &format->entry );
        if (format->id < CF_MAX) clipboard->format_map &= ~(1 << format->id);
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
    if (clipboard->owner == window) release_clipboard( clipboard );
    if (clipboard->open_win == window)
    {
        user_handle_t viewer = close_clipboard( clipboard );
        if (viewer) send_notify_message( viewer, WM_DRAWCLIPBOARD, clipboard->owner, 0 );
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
        if (thread == clipboard->open_thread)
        {
            user_handle_t viewer = close_clipboard( clipboard );
            if (viewer) send_notify_message( viewer, WM_DRAWCLIPBOARD, clipboard->owner, 0 );
        }
    }
    release_object( winstation );
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
    if (clipboard->open_thread != current) clipboard->rendering = 0;
    clipboard->open_win = win;
    clipboard->open_thread = current;

    reply->owner = clipboard->owner;
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
    reply->owner  = clipboard->owner;
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

    free( format->data );
    format->from   = 0;
    format->seqno  = clipboard->seqno;
    format->size   = get_req_data_size();
    format->data   = data;
    if (!clipboard->rendering) clipboard->seqno++;

    if (req->format == CF_TEXT || req->format == CF_OEMTEXT || req->format == CF_UNICODETEXT)
        clipboard->lcid = req->lcid;

    reply->seqno = format->seqno;
}


/* fetch a data format from the clipboard */
DECL_HANDLER(get_clipboard_data)
{
    struct clip_format *format;
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    if (clipboard->open_thread != current)
    {
        set_win32_error( ERROR_CLIPBOARD_NOT_OPEN );
        return;
    }
    if (!(format = get_format( clipboard, req->format )))
    {
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
        goto done;
    }
    reply->from   = format->from;
    reply->total  = format->size;
    reply->seqno  = format->seqno;
    reply->owner  = clipboard->owner;

    if (!format->data && req->render)  /* try rendering it client-side */
    {
        if (format->from || clipboard->owner) clipboard->rendering++;
        return;
    }

    if (req->cached && req->seqno == format->seqno) goto done;  /* client-side cache still valid */

    if (format->data)
    {
        if (format->size > get_reply_max_size())
        {
            set_error( STATUS_BUFFER_OVERFLOW );
            return;
        }
        set_reply_data( format->data, format->size );
    }

done:
    if (!req->render) clipboard->rendering--;
}


/* retrieve a list of available formats */
DECL_HANDLER(get_clipboard_formats)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    if (!req->format)
    {
        struct clip_format *format;
        unsigned int i = 0, *ptr;
        data_size_t size = clipboard->format_count * sizeof(unsigned int);

        reply->count = clipboard->format_count;
        if (size <= get_reply_max_size())
        {
            if ((ptr = mem_alloc( size )))
            {
                LIST_FOR_EACH_ENTRY( format, &clipboard->formats, struct clip_format, entry )
                    ptr[i++] = format->id;
                assert( i == clipboard->format_count );
                set_reply_data_ptr( ptr, size );
            }
        }
        else set_error( STATUS_BUFFER_TOO_SMALL );
    }
    else reply->count = (get_format( clipboard, req->format ) != NULL);  /* query a single format */
}


/* retrieve the next available format */
DECL_HANDLER(enum_clipboard_formats)
{
    struct list *ptr;
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    if (clipboard->open_thread != current)
    {
        set_win32_error( ERROR_CLIPBOARD_NOT_OPEN );
        return;
    }

    ptr = list_head( &clipboard->formats );
    if (req->previous)
    {
        while (ptr && LIST_ENTRY( ptr, struct clip_format, entry )->id != req->previous)
            ptr = list_next( &clipboard->formats, ptr );
        if (ptr) ptr = list_next( &clipboard->formats, ptr );
    }
    if (ptr) reply->format = LIST_ENTRY( ptr, struct clip_format, entry )->id;
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
    clipboard->owner = clipboard->open_win;
    clipboard->seqno++;
}


/* release ownership of the clipboard */
DECL_HANDLER(release_clipboard)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t owner;

    if (!clipboard) return;

    if (!(owner = get_valid_window_handle( req->owner ))) return;

    if (clipboard->owner == owner)
    {
        reply->viewer = release_clipboard( clipboard );
        reply->owner = clipboard->owner;
    }
    else set_error( STATUS_INVALID_OWNER );
}


/* get clipboard information */
DECL_HANDLER(get_clipboard_info)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    reply->window = clipboard->open_win;
    reply->owner  = clipboard->owner;
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
    reply->owner      = clipboard->owner;

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

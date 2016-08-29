/*
 * Server-side clipboard management
 *
 * Copyright (C) 2002 Ulrich Czekalla
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

struct clipboard
{
    struct object  obj;              /* object header */
    struct thread *open_thread;      /* thread id that has clipboard open */
    user_handle_t  open_win;         /* window that has clipboard open */
    struct thread *owner_thread;     /* thread id that owns the clipboard */
    user_handle_t  owner_win;        /* window that owns the clipboard data */
    user_handle_t  viewer;           /* first window in clipboard viewer list */
    unsigned int   seqno;            /* clipboard change sequence number */
    timeout_t      seqno_timestamp;  /* time stamp of last seqno increment */
};

static void clipboard_dump( struct object *obj, int verbose );

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
    no_destroy                    /* destroy */
};


#define MINUPDATELAPSE (2 * TICKS_PER_SEC)

/* dump a clipboard object */
static void clipboard_dump( struct object *obj, int verbose )
{
    struct clipboard *clipboard = (struct clipboard *)obj;

    fprintf( stderr, "Clipboard open_thread=%p open_win=%08x owner_thread=%p owner_win=%08x viewer=%08x seq=%u\n",
             clipboard->open_thread, clipboard->open_win, clipboard->owner_thread,
             clipboard->owner_win, clipboard->viewer, clipboard->seqno );
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
            clipboard->seqno_timestamp = 0;
            winstation->clipboard = clipboard;
        }
    }
    release_object( winstation );
    return clipboard;
}

/* cleanup clipboard information upon window destruction */
void cleanup_clipboard_window( struct desktop *desktop, user_handle_t window )
{
    struct clipboard *clipboard = desktop->winstation->clipboard;

    if (!clipboard) return;

    if (clipboard->open_win == window)
    {
        clipboard->open_win = 0;
        clipboard->open_thread = NULL;
    }
    if (clipboard->owner_win == window)
    {
        clipboard->owner_win = 0;
        clipboard->owner_thread = NULL;
    }
    if (clipboard->viewer == window) clipboard->viewer = 0;
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
            clipboard->open_win = 0;
            clipboard->open_thread = NULL;
        }
        if (thread == clipboard->owner_thread)
        {
            clipboard->owner_win = 0;
            clipboard->owner_thread = NULL;
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


static int get_seqno( struct clipboard *clipboard )
{
    if (!clipboard->owner_thread && (current_time - clipboard->seqno_timestamp > MINUPDATELAPSE))
    {
        clipboard->seqno_timestamp = current_time;
        clipboard->seqno++;
    }
    return clipboard->seqno;
}


/* open the clipboard */
DECL_HANDLER(open_clipboard)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t win = req->window;

    if (!clipboard) return;

    if (win && !get_user_object_handle( &win, USER_WINDOW ))
    {
        set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
        return;
    }
    if (clipboard->open_thread && clipboard->open_win != win)
    {
        set_error( STATUS_INVALID_LOCK_SEQUENCE );
        return;
    }
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
    if (req->changed) clipboard->seqno++;
    clipboard->open_thread = NULL;
    clipboard->open_win = 0;

    reply->viewer = clipboard->viewer;
    reply->owner  = (clipboard->owner_thread && clipboard->owner_thread->process == current->process);
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

    reply->seqno = get_seqno( clipboard );

    if (clipboard->open_thread) reply->flags |= CB_OPEN_ANY;
    if (clipboard->open_thread == current) reply->flags |= CB_OPEN;
    if (clipboard->owner_thread == current) reply->flags |= CB_OWNER;
    if (clipboard->owner_thread && clipboard->owner_thread->process == current->process)
        reply->flags |= CB_PROCESS;
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
    clipboard->owner_win = clipboard->open_win;
    clipboard->owner_thread = clipboard->open_thread;
    clipboard->seqno++;
}


/* Get clipboard information */
DECL_HANDLER(get_clipboard_info)
{
    struct clipboard *clipboard = get_process_clipboard();

    if (!clipboard) return;

    reply->window = clipboard->open_win;
    reply->owner  = clipboard->owner_win;
    reply->viewer = clipboard->viewer;
    reply->seqno  = get_seqno( clipboard );
}


/* set the clipboard viewer window */
DECL_HANDLER(set_clipboard_viewer)
{
    struct clipboard *clipboard = get_process_clipboard();
    user_handle_t viewer = req->viewer;
    user_handle_t previous = req->previous;

    if (!clipboard) return;

    if (viewer && !get_user_object_handle( &viewer, USER_WINDOW ))
    {
        set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
        return;
    }
    if (previous && !get_user_object_handle( &previous, USER_WINDOW ))
    {
        set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
        return;
    }

    reply->old_viewer = clipboard->viewer;
    reply->owner      = clipboard->owner_win;

    if (!previous || clipboard->viewer == previous)
        clipboard->viewer = viewer;
    else
        set_error( STATUS_PENDING );  /* need to send message instead */
}

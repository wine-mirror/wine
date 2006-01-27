/*
 * Server-side change notification management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2006 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "winternl.h"

#ifdef linux
#ifndef F_NOTIFY
#define F_NOTIFY 1026
#define DN_ACCESS       0x00000001      /* File accessed */
#define DN_MODIFY       0x00000002      /* File modified */
#define DN_CREATE       0x00000004      /* File created */
#define DN_DELETE       0x00000008      /* File removed */
#define DN_RENAME       0x00000010      /* File renamed */
#define DN_ATTRIB       0x00000020      /* File changed attibutes */
#define DN_MULTISHOT    0x80000000      /* Don't remove notifier */
#endif
#endif

struct dir
{
    struct object  obj;      /* object header */
    struct fd     *fd;       /* file descriptor to the directory */
    struct list    entry;    /* entry in global change notifications list */
    struct event  *event;
    unsigned int   filter;   /* notification filter */
    int            notified; /* SIGIO counter */
    long           signaled; /* the file changed */
};

static struct fd *dir_get_fd( struct object *obj );
static unsigned int dir_map_access( struct object *obj, unsigned int access );
static void dir_dump( struct object *obj, int verbose );
static void dir_destroy( struct object *obj );
static int dir_signaled( struct object *obj, struct thread *thread );

static const struct object_ops dir_ops =
{
    sizeof(struct dir),       /* size */
    dir_dump,                 /* dump */
    add_queue,                /* add_queue */
    remove_queue,             /* remove_queue */
    dir_signaled,             /* signaled */
    no_satisfied,             /* satisfied */
    no_signal,                /* signal */
    dir_get_fd,               /* get_fd */
    dir_map_access,           /* map_access */
    no_lookup_name,           /* lookup_name */
    no_close_handle,          /* close_handle */
    dir_destroy               /* destroy */
};

static int dir_get_poll_events( struct fd *fd );
static int dir_get_info( struct fd *fd );

static const struct fd_ops dir_fd_ops =
{
    dir_get_poll_events,      /* get_poll_events */
    default_poll_event,       /* poll_event */
    no_flush,                 /* flush */
    dir_get_info,             /* get_file_info */
    default_fd_queue_async,   /* queue_async */
    default_fd_cancel_async   /* cancel_async */
};

static struct list change_list = LIST_INIT(change_list);

static void adjust_changes( int fd, unsigned int filter )
{
#if defined(F_SETSIG) && defined(F_NOTIFY)
    unsigned int val;
    if ( 0 > fcntl( fd, F_SETSIG, SIGIO) )
        return;

    val = DN_MULTISHOT;
    if (filter & FILE_NOTIFY_CHANGE_FILE_NAME)
        val |= DN_RENAME | DN_DELETE | DN_CREATE;
    if (filter & FILE_NOTIFY_CHANGE_DIR_NAME)
        val |= DN_RENAME | DN_DELETE | DN_CREATE;
    if (filter & FILE_NOTIFY_CHANGE_ATTRIBUTES)
        val |= DN_ATTRIB;
    if (filter & FILE_NOTIFY_CHANGE_SIZE)
        val |= DN_MODIFY;
    if (filter & FILE_NOTIFY_CHANGE_LAST_WRITE)
        val |= DN_MODIFY;
    if (filter & FILE_NOTIFY_CHANGE_LAST_ACCESS)
        val |= DN_ACCESS;
    if (filter & FILE_NOTIFY_CHANGE_CREATION)
        val |= DN_CREATE;
    if (filter & FILE_NOTIFY_CHANGE_SECURITY)
        val |= DN_ATTRIB;
    fcntl( fd, F_NOTIFY, val );
#endif
}

/* insert change in the global list */
static inline void insert_change( struct dir *dir )
{
    sigset_t sigset;

    sigemptyset( &sigset );
    sigaddset( &sigset, SIGIO );
    sigprocmask( SIG_BLOCK, &sigset, NULL );
    list_add_head( &change_list, &dir->entry );
    sigprocmask( SIG_UNBLOCK, &sigset, NULL );
}

/* remove change from the global list */
static inline void remove_change( struct dir *dir )
{
    sigset_t sigset;

    sigemptyset( &sigset );
    sigaddset( &sigset, SIGIO );
    sigprocmask( SIG_BLOCK, &sigset, NULL );
    list_remove( &dir->entry );
    sigprocmask( SIG_UNBLOCK, &sigset, NULL );
}

struct object *create_dir_obj( struct fd *fd )
{
    struct dir *dir;

    dir = alloc_object( &dir_ops );
    if (!dir)
        return NULL;
 
    dir->event = NULL;
    dir->filter = 0;
    dir->notified = 0;
    dir->signaled = 0;
    grab_object( fd );
    dir->fd = fd;
    set_fd_user( fd, &dir_fd_ops, &dir->obj );

    return &dir->obj;
}

static void dir_dump( struct object *obj, int verbose )
{
    struct dir *dir = (struct dir *)obj;
    assert( obj->ops == &dir_ops );
    fprintf( stderr, "Dirfile fd=%p event=%p filter=%08x\n",
             dir->fd, dir->event, dir->filter );
}

static int dir_signaled( struct object *obj, struct thread *thread )
{
    struct dir *dir = (struct dir *)obj;
    assert (obj->ops == &dir_ops);
    return (dir->event == NULL) && dir->signaled;
}

/* enter here directly from SIGIO signal handler */
void do_change_notify( int unix_fd )
{
    struct list *ptr;

    /* FIXME: this is O(n) ... probably can be improved */
    LIST_FOR_EACH( ptr, &change_list )
    {
        struct dir *dir = LIST_ENTRY( ptr, struct dir, entry );
        if (get_unix_fd( dir->fd ) != unix_fd) continue;
        interlocked_xchg_add( &dir->notified, 1 );
        break;
    }
}

/* SIGIO callback, called synchronously with the poll loop */
void sigio_callback(void)
{
    struct list *ptr;

    LIST_FOR_EACH( ptr, &change_list )
    {
        struct dir *dir = LIST_ENTRY( ptr, struct dir, entry );
        long count = interlocked_xchg( &dir->notified, 0 );
        if (count)
        {
            dir->signaled += count;
            if (dir->signaled == count)  /* was it 0? */
            {
                if (dir->event)
                    set_event( dir->event );
                else
                    wake_up( &dir->obj, 0 );
            }
        }
    }
}

static struct fd *dir_get_fd( struct object *obj )
{
    struct dir *dir = (struct dir *)obj;
    assert( obj->ops == &dir_ops );
    return (struct fd *)grab_object( dir->fd );
}

static unsigned int dir_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= FILE_GENERIC_READ;
    if (access & GENERIC_WRITE)   access |= FILE_GENERIC_WRITE;
    if (access & GENERIC_EXECUTE) access |= FILE_GENERIC_EXECUTE;
    if (access & GENERIC_ALL)     access |= FILE_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void dir_destroy( struct object *obj )
{
    struct dir *dir = (struct dir *)obj;
    assert (obj->ops == &dir_ops);

    if (dir->filter)
        remove_change( dir );

    if (dir->event)
    {
        set_event( dir->event );
        release_object( dir->event );
    }
    release_object( dir->fd );
}

static struct dir *
get_dir_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct dir *)get_handle_obj( process, handle, access, &dir_ops );
}

static int dir_get_poll_events( struct fd *fd )
{
    return 0;
}

static int dir_get_info( struct fd *fd )
{
    return 0;
}

/* enable change notifications for a directory */
DECL_HANDLER(read_directory_changes)
{
    struct event *event = NULL;
    struct dir *dir;

    if (!req->filter)
    {
        set_error(STATUS_INVALID_PARAMETER);
        return;
    }

    dir = get_dir_obj( current->process, req->handle, 0 );
    if (!dir)
        return;

    /* possibly send changes through an event flag */
    if (req->event)
    {
        event = get_event_obj( current->process, req->event, EVENT_MODIFY_STATE );
        if (!event)
            goto end;
    }

    /* discard the current data, and move onto the next event */
    if (dir->event) release_object( dir->event );
    dir->event = event;

    /* assign it once */
    if (!dir->filter)
    {
        insert_change( dir );
        dir->filter = req->filter;
    }

    /* remove any notifications */
    if (dir->signaled>0)
        dir->signaled--;

    adjust_changes( get_unix_fd( dir->fd ), dir->filter );

    set_error(STATUS_PENDING);

end:
    release_object( dir );
}

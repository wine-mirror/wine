/*
 * Server-side change notification management
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#include "windef.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"

struct change
{
    struct object  obj;      /* object header */
    struct fd     *fd;       /* file descriptor to the directory */
    struct list    entry;    /* entry in global change notifications list */
    int            subtree;  /* watch all the subtree */
    unsigned int   filter;   /* notification filter */
    long           notified; /* SIGIO counter */
    long           signaled; /* the file changed */
};

static void change_dump( struct object *obj, int verbose );
static int change_signaled( struct object *obj, struct thread *thread );
static void change_destroy( struct object *obj );

static const struct object_ops change_ops =
{
    sizeof(struct change),    /* size */
    change_dump,              /* dump */
    add_queue,                /* add_queue */
    remove_queue,             /* remove_queue */
    change_signaled,          /* signaled */
    no_satisfied,             /* satisfied */
    no_get_fd,                /* get_fd */
    change_destroy            /* destroy */
};

static struct list change_list = LIST_INIT(change_list);

static void adjust_changes( int fd, unsigned int filter )
{
#ifdef F_NOTIFY
    unsigned int val;
    if ( 0 > fcntl( fd, F_SETSIG, SIGIO) )
        return;

    val = DN_MULTISHOT;
    if( filter & FILE_NOTIFY_CHANGE_FILE_NAME )
        val |= DN_RENAME | DN_DELETE | DN_CREATE;
    if( filter & FILE_NOTIFY_CHANGE_DIR_NAME )
        val |= DN_RENAME | DN_DELETE | DN_CREATE;
    if( filter & FILE_NOTIFY_CHANGE_ATTRIBUTES )
        val |= DN_ATTRIB;
    if( filter & FILE_NOTIFY_CHANGE_SIZE )
        val |= DN_MODIFY;
    if( filter & FILE_NOTIFY_CHANGE_LAST_WRITE )
        val |= DN_MODIFY;
    if( filter & FILE_NOTIFY_CHANGE_SECURITY )
        val |= DN_ATTRIB;
    fcntl( fd, F_NOTIFY, val );
#endif
}

/* insert change in the global list */
static inline void insert_change( struct change *change )
{
    sigset_t sigset;

    sigemptyset( &sigset );
    sigaddset( &sigset, SIGIO );
    sigprocmask( SIG_BLOCK, &sigset, NULL );
    list_add_head( &change_list, &change->entry );
    sigprocmask( SIG_UNBLOCK, &sigset, NULL );
}

/* remove change from the global list */
static inline void remove_change( struct change *change )
{
    sigset_t sigset;

    sigemptyset( &sigset );
    sigaddset( &sigset, SIGIO );
    sigprocmask( SIG_BLOCK, &sigset, NULL );
    list_remove( &change->entry );
    sigprocmask( SIG_UNBLOCK, &sigset, NULL );
}

static struct change *create_change_notification( struct fd *fd, int subtree, unsigned int filter )
{
    struct change *change;

    if ((change = alloc_object( &change_ops )))
    {
        change->fd       = (struct fd *)grab_object( fd );
        change->subtree  = subtree;
        change->filter   = filter;
        change->notified = 0;
        change->signaled = 0;
        insert_change( change );
        adjust_changes( get_unix_fd(fd), filter );
    }
    return change;
}

static void change_dump( struct object *obj, int verbose )
{
    struct change *change = (struct change *)obj;
    assert( obj->ops == &change_ops );
    fprintf( stderr, "Change notification fd=%p sub=%d filter=%08x\n",
             change->fd, change->subtree, change->filter );
}

static int change_signaled( struct object *obj, struct thread *thread )
{
    struct change *change = (struct change *)obj;

    return change->signaled != 0;
}

static void change_destroy( struct object *obj )
{
    struct change *change = (struct change *)obj;

    release_object( change->fd );
    remove_change( change );
}

/* enter here directly from SIGIO signal handler */
void do_change_notify( int unix_fd )
{
    struct list *ptr;

    /* FIXME: this is O(n) ... probably can be improved */
    LIST_FOR_EACH( ptr, &change_list )
    {
        struct change *change = LIST_ENTRY( ptr, struct change, entry );
        if (get_unix_fd( change->fd ) != unix_fd) continue;
        interlocked_xchg_add( &change->notified, 1 );
        break;
    }
}

/* SIGIO callback, called synchronously with the poll loop */
void sigio_callback(void)
{
    struct list *ptr;

    LIST_FOR_EACH( ptr, &change_list )
    {
        struct change *change = LIST_ENTRY( ptr, struct change, entry );
        long count = interlocked_xchg( &change->notified, 0 );
        if (count)
        {
            change->signaled += count;
            if (change->signaled == count)  /* was it 0? */
                wake_up( &change->obj, 0 );
        }
    }
}

/* create a change notification */
DECL_HANDLER(create_change_notification)
{
    struct change *change;
    struct file *file;
    struct fd *fd;

    if (!(file = get_file_obj( current->process, req->handle, 0 ))) return;
    fd = get_obj_fd( (struct object *)file );
    release_object( file );
    if (!fd) return;

    if ((change = create_change_notification( fd, req->subtree, req->filter )))
    {
        reply->handle = alloc_handle( current->process, change,
                                      STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE, 0 );
        release_object( change );
    }
    release_object( fd );
}

/* move to the next change notification */
DECL_HANDLER(next_change_notification)
{
    struct change *change;

    if ((change = (struct change *)get_handle_obj( current->process, req->handle,
                                                   0, &change_ops )))
    {
        if (change->signaled > 0) change->signaled--;
        release_object( change );
    }
}

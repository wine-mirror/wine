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
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "winternl.h"

/* dnotify support */

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

/* inotify support */

#if defined(__linux__) && defined(__i386__)

#define SYS_inotify_init	291
#define SYS_inotify_add_watch	292
#define SYS_inotify_rm_watch	293

struct inotify_event {
    int           wd;
    unsigned int  mask;
    unsigned int  cookie;
    unsigned int  len;
    char          name[1];
};

#define IN_ACCESS        0x00000001
#define IN_MODIFY        0x00000002
#define IN_ATTRIB        0x00000004
#define IN_CLOSE_WRITE   0x00000008
#define IN_CLOSE_NOWRITE 0x00000010
#define IN_OPEN          0x00000020
#define IN_MOVED_FROM    0x00000040
#define IN_MOVED_TO      0x00000080
#define IN_CREATE        0x00000100
#define IN_DELETE        0x00000200
#define IN_DELETE_SELF   0x00000400

static inline int inotify_init( void )
{
    int ret;
    __asm__ __volatile__( "int $0x80"
                          : "=a" (ret)
                          : "0" (SYS_inotify_init));
    if (ret<0) { errno = -ret; ret = -1; }
    return ret;
}

static inline int inotify_add_watch( int fd, const char *name, unsigned int mask )
{
    int ret;
    __asm__ __volatile__( "pushl %%ebx;\n\t"
                          "movl %2,%%ebx;\n\t"
                          "int $0x80;\n\t"
                          "popl %%ebx"
                          : "=a" (ret) : "0" (SYS_inotify_add_watch),
                            "r" (fd), "c" (name), "d" (mask) );
    if (ret<0) { errno = -ret; ret = -1; }
    return ret;
}

static inline int inotify_remove_watch( int fd, int wd )
{
    int ret;
    __asm__ __volatile__( "pushl %%ebx;\n\t"
                          "movl %2,%%ebx;\n\t"
                          "int $0x80;\n\t"
                          "popl %%ebx"
                          : "=a" (ret) : "0" (SYS_inotify_rm_watch),
                            "r" (fd), "c" (wd) );
    if (ret<0) { errno = -ret; ret = -1; }
    return ret;
}

#define USE_INOTIFY

#endif

struct change_record {
    struct list entry;
    int action;
    int len;
    char name[1];
};

struct dir
{
    struct object  obj;      /* object header */
    struct fd     *fd;       /* file descriptor to the directory */
    struct list    entry;    /* entry in global change notifications list */
    struct event  *event;
    unsigned int   filter;   /* notification filter */
    int            notified; /* SIGIO counter */
    int            want_data; /* return change data */
    long           signaled; /* the file changed */
    struct fd     *inotify_fd; /* inotify file descriptor */
    int            wd;       /* inotify watch descriptor */
    struct list    change_q; /* change readers */
    struct list    change_records;   /* data for the change */
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
static void dir_cancel_async( struct fd *fd );

static const struct fd_ops dir_fd_ops =
{
    dir_get_poll_events,      /* get_poll_events */
    default_poll_event,       /* poll_event */
    no_flush,                 /* flush */
    dir_get_info,             /* get_file_info */
    default_fd_queue_async,   /* queue_async */
    dir_cancel_async          /* cancel_async */
};

static struct list change_list = LIST_INIT(change_list);

static void dnotify_adjust_changes( struct dir *dir )
{
#if defined(F_SETSIG) && defined(F_NOTIFY)
    int fd = get_unix_fd( dir->fd );
    unsigned int filter = dir->filter;
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

    list_init( &dir->change_q );
    list_init( &dir->change_records );
    dir->event = NULL;
    dir->filter = 0;
    dir->notified = 0;
    dir->signaled = 0;
    dir->want_data = 0;
    dir->inotify_fd = NULL;
    dir->wd = -1;
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
    struct dir *dir;

    /* FIXME: this is O(n) ... probably can be improved */
    LIST_FOR_EACH_ENTRY( dir, &change_list, struct dir, entry )
    {
        if (get_unix_fd( dir->fd ) != unix_fd) continue;
        interlocked_xchg_add( &dir->notified, 1 );
        break;
    }
}

static void dir_signal_changed( struct dir *dir )
{
    if (dir->event)
        set_event( dir->event );
    else
        wake_up( &dir->obj, 0 );
}

/* SIGIO callback, called synchronously with the poll loop */
void sigio_callback(void)
{
    struct dir *dir;

    LIST_FOR_EACH_ENTRY( dir, &change_list, struct dir, entry )
    {
        long count = interlocked_xchg( &dir->notified, 0 );
        if (count)
        {
            dir->signaled += count;
            if (dir->signaled == count)  /* was it 0? */
                dir_signal_changed( dir );
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

static struct change_record *get_first_change_record( struct dir *dir )
{
    struct list *ptr = list_head( &dir->change_records );
    if (!ptr) return NULL;
    list_remove( ptr );
    return LIST_ENTRY( ptr, struct change_record, entry );
}

static void dir_destroy( struct object *obj )
{
    struct change_record *record;
    struct dir *dir = (struct dir *)obj;
    assert (obj->ops == &dir_ops);

    if (dir->filter)
        remove_change( dir );

    if (dir->inotify_fd)
        release_object( dir->inotify_fd );

    async_terminate_queue( &dir->change_q, STATUS_CANCELLED );
    while ((record = get_first_change_record( dir ))) free( record );

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

static void dir_cancel_async( struct fd *fd )
{
    struct dir *dir = (struct dir *) get_fd_user( fd );
    async_terminate_queue( &dir->change_q, STATUS_CANCELLED );
}


#ifdef USE_INOTIFY

static int inotify_get_poll_events( struct fd *fd );
static void inotify_poll_event( struct fd *fd, int event );
static int inotify_get_info( struct fd *fd );

static const struct fd_ops inotify_fd_ops =
{
    inotify_get_poll_events,  /* get_poll_events */
    inotify_poll_event,       /* poll_event */
    no_flush,                 /* flush */
    inotify_get_info,         /* get_file_info */
    default_fd_queue_async,   /* queue_async */
    default_fd_cancel_async,  /* cancel_async */
};

static int inotify_get_poll_events( struct fd *fd )
{
    return POLLIN;
}

static void inotify_do_change_notify( struct dir *dir, struct inotify_event *ie )
{
    struct change_record *record;

    if (dir->want_data)
    {
        size_t len = strlen(ie->name);
        record = malloc( offsetof(struct change_record, name[len]) );
        if (!record)
            return;

        if( ie->mask & IN_CREATE )
            record->action = FILE_ACTION_ADDED;
        else if( ie->mask & IN_DELETE )
            record->action = FILE_ACTION_REMOVED;
        else
            record->action = FILE_ACTION_MODIFIED;
        memcpy( record->name, ie->name, len );
        record->len = len;

        list_add_tail( &dir->change_records, &record->entry );
    }

    if (!list_empty( &dir->change_q ))
        async_terminate_head( &dir->change_q, STATUS_ALERTED );
    else
    {
        dir->signaled++;
        dir_signal_changed( dir );
    }
}

static void inotify_poll_event( struct fd *fd, int event )
{
    int r, ofs, unix_fd;
    char buffer[0x1000];
    struct inotify_event *ie;
    struct dir *dir = get_fd_user( fd );

    unix_fd = get_unix_fd( fd );
    r = read( unix_fd, buffer, sizeof buffer );
    if (r < 0)
    {
        fprintf(stderr,"inotify_poll_event(): inotify read failed!\n");
        return;
    }

    for( ofs = 0; ofs < r - offsetof(struct inotify_event, name); )
    {
        ie = (struct inotify_event*) &buffer[ofs];
        if (!ie->len)
            break;
        ofs += offsetof( struct inotify_event, name[ie->len] );
        if (ofs > r) break;
        inotify_do_change_notify( dir, ie );
    }
}

static int inotify_get_info( struct fd *fd )
{
    return 0;
}

static inline struct fd *create_inotify_fd( struct dir *dir )
{
    int unix_fd;

    unix_fd = inotify_init();
    if (unix_fd<0)
        return NULL;
    return create_anonymous_fd( &inotify_fd_ops, unix_fd, &dir->obj );
}

static int inotify_adjust_changes( struct dir *dir )
{
    unsigned int filter = dir->filter;
    unsigned int mask = 0;
    char link[32];

    if (!dir->inotify_fd)
    {
        if (!(dir->inotify_fd = create_inotify_fd( dir ))) return 0;
    }

    if (filter & FILE_NOTIFY_CHANGE_FILE_NAME)
        mask |= (IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE | IN_CREATE);
    if (filter & FILE_NOTIFY_CHANGE_DIR_NAME)
        mask |= (IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE | IN_CREATE | IN_DELETE_SELF);
    if (filter & FILE_NOTIFY_CHANGE_ATTRIBUTES)
        mask |= IN_ATTRIB;
    if (filter & FILE_NOTIFY_CHANGE_SIZE)
        mask |= IN_MODIFY;
    if (filter & FILE_NOTIFY_CHANGE_LAST_WRITE)
        mask |= IN_MODIFY;
    if (filter & FILE_NOTIFY_CHANGE_LAST_ACCESS)
        mask |= IN_ACCESS;
    if (filter & FILE_NOTIFY_CHANGE_CREATION)
        mask |= IN_CREATE;
    if (filter & FILE_NOTIFY_CHANGE_SECURITY)
        mask |= IN_ATTRIB;

    sprintf( link, "/proc/self/fd/%u", get_unix_fd( dir->fd ) );
    dir->wd = inotify_add_watch( get_unix_fd( dir->inotify_fd ), link, mask );
    if (dir->wd != -1)
        set_fd_events( dir->inotify_fd, POLLIN );
    return 1;
}

#endif  /* USE_INOTIFY */

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

    /* requests don't timeout */
    if ( req->io_apc && !create_async( current, NULL, &dir->change_q,
                        req->io_apc, req->io_user, req->io_sb ))
        return;

    /* assign it once */
    if (!dir->filter)
    {
        insert_change( dir );
        dir->filter = req->filter;
        dir->want_data = req->want_data;
    }

    /* remove any notifications */
    if (dir->signaled>0)
        dir->signaled--;

    /* clear the event */
    if (event)
        reset_event( event );

    /* setup the real notification */
#ifdef USE_INOTIFY
    if (!inotify_adjust_changes( dir ))
#endif
        dnotify_adjust_changes( dir );

    set_error(STATUS_PENDING);

end:
    release_object( dir );
}

DECL_HANDLER(read_change)
{
    struct change_record *record;
    struct dir *dir;

    dir = get_dir_obj( current->process, req->handle, 0 );
    if (!dir)
        return;

    if ((record = get_first_change_record( dir )) != NULL)
    {
        reply->action = record->action;
        set_reply_data( record->name, record->len );
        free( record );
    }
    else
        set_error( STATUS_NO_DATA_DETECTED );

    /* now signal it */
    dir->signaled++;
    dir_signal_changed( dir );

    release_object( dir );
}

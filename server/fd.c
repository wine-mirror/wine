/*
 * Server-side file descriptor management
 *
 * Copyright (C) 2000, 2003 Alexandre Julliard
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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <poll.h>
#ifdef HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
/* Work around a conflict with Solaris' system list defined in sys/list.h. */
#define list SYSLIST
#define list_next SYSLIST_NEXT
#define list_prev SYSLIST_PREV
#define list_head SYSLIST_HEAD
#define list_tail SYSLIST_TAIL
#define list_move_tail SYSLIST_MOVE_TAIL
#define list_remove SYSLIST_REMOVE
#include <sys/vfs.h>
#undef list
#undef list_next
#undef list_prev
#undef list_head
#undef list_tail
#undef list_move_tail
#undef list_remove
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_EVENT_H
#include <sys/event.h>
#undef LIST_INIT
#undef LIST_ENTRY
#endif
#include <sys/stat.h>
#include <sys/time.h>
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "object.h"
#include "file.h"
#include "handle.h"
#include "process.h"
#include "request.h"

#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#if defined(HAVE_SYS_EPOLL_H) && defined(HAVE_EPOLL_CREATE)
# include <sys/epoll.h>
# define USE_EPOLL
#endif /* HAVE_SYS_EPOLL_H && HAVE_EPOLL_CREATE */

#if defined(HAVE_PORT_H) && defined(HAVE_PORT_CREATE)
# include <port.h>
# define USE_EVENT_PORTS
#endif /* HAVE_PORT_H && HAVE_PORT_CREATE */

/* Because of the stupid Posix locking semantics, we need to keep
 * track of all file descriptors referencing a given file, and not
 * close a single one until all the locks are gone (sigh).
 */

/* file descriptor object */

/* closed_fd is used to keep track of the unix fd belonging to a closed fd object */
struct closed_fd
{
    struct list     entry;      /* entry in inode closed list */
    int             unix_fd;    /* the unix file descriptor */
    unsigned int    disp_flags; /* the disposition flags */
    char           *unix_name;  /* name to unlink on close, points to parent fd unix_name */
};

struct fd
{
    struct object        obj;         /* object header */
    const struct fd_ops *fd_ops;      /* file descriptor operations */
    struct inode        *inode;       /* inode that this fd belongs to */
    struct list          inode_entry; /* entry in inode fd list */
    struct closed_fd    *closed;      /* structure to store the unix fd at destroy time */
    struct object       *user;        /* object using this file descriptor */
    struct list          locks;       /* list of locks on this fd */
    client_ptr_t         map_addr;    /* default mapping address for PE files */
    mem_size_t           map_size;    /* mapping size for PE files */
    unsigned int         access;      /* file access (FILE_READ_DATA etc.) */
    unsigned int         options;     /* file options (FILE_DELETE_ON_CLOSE, FILE_SYNCHRONOUS...) */
    unsigned int         sharing;     /* file sharing mode */
    char                *unix_name;   /* unix file name */
    WCHAR               *nt_name;     /* NT file name */
    data_size_t          nt_namelen;  /* length of NT file name */
    int                  unix_fd;     /* unix file descriptor */
    unsigned int         no_fd_status;/* status to return when unix_fd is -1 */
    unsigned int         cacheable :1;/* can the fd be cached on the client side? */
    unsigned int         signaled :1; /* is the fd signaled? */
    unsigned int         fs_locks :1; /* can we use filesystem locks for this fd? */
    int                  poll_index;  /* index of fd in poll array */
    struct async_queue   read_q;      /* async readers of this fd */
    struct async_queue   write_q;     /* async writers of this fd */
    struct async_queue   wait_q;      /* other async waiters of this fd */
    struct completion   *completion;  /* completion object attached to this fd */
    apc_param_t          comp_key;    /* completion key to set in completion events */
    unsigned int         comp_flags;  /* completion flags */
};

static void fd_dump( struct object *obj, int verbose );
static int fd_signaled( struct object *obj, struct wait_queue_entry *entry );
static void fd_destroy( struct object *obj );

static const struct object_ops fd_ops =
{
    sizeof(struct fd),        /* size */
    &no_type,                 /* type */
    fd_dump,                  /* dump */
    add_queue,                /* add_queue */
    remove_queue,             /* remove_queue */
    fd_signaled,              /* signaled */
    no_satisfied,             /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    default_get_sync,         /* get_sync */
    default_map_access,       /* map_access */
    default_get_sd,           /* get_sd */
    default_set_sd,           /* set_sd */
    no_get_full_name,         /* get_full_name */
    no_lookup_name,           /* lookup_name */
    no_link_name,             /* link_name */
    NULL,                     /* unlink_name */
    no_open_file,             /* open_file */
    no_kernel_obj_list,       /* get_kernel_obj_list */
    no_close_handle,          /* close_handle */
    fd_destroy                /* destroy */
};

/* device object */

#define DEVICE_HASH_SIZE 7
#define INODE_HASH_SIZE 17

struct device
{
    struct object       obj;        /* object header */
    struct list         entry;      /* entry in device hash list */
    dev_t               dev;        /* device number */
    int                 removable;  /* removable device? (or -1 if unknown) */
    struct list         inode_hash[INODE_HASH_SIZE];  /* inodes hash table */
};

static void device_dump( struct object *obj, int verbose );
static void device_destroy( struct object *obj );

static const struct object_ops device_ops =
{
    sizeof(struct device),    /* size */
    &no_type,                 /* type */
    device_dump,              /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    default_get_sync,         /* get_sync */
    default_map_access,       /* map_access */
    default_get_sd,           /* get_sd */
    default_set_sd,           /* set_sd */
    no_get_full_name,         /* get_full_name */
    no_lookup_name,           /* lookup_name */
    no_link_name,             /* link_name */
    NULL,                     /* unlink_name */
    no_open_file,             /* open_file */
    no_kernel_obj_list,       /* get_kernel_obj_list */
    no_close_handle,          /* close_handle */
    device_destroy            /* destroy */
};

/* inode object */

struct inode
{
    struct object       obj;        /* object header */
    struct list         entry;      /* inode hash list entry */
    struct device      *device;     /* device containing this inode */
    ino_t               ino;        /* inode number */
    struct list         open;       /* list of open file descriptors */
    struct list         locks;      /* list of file locks */
    struct list         closed;     /* list of file descriptors to close at destroy time */
};

static void inode_dump( struct object *obj, int verbose );
static void inode_destroy( struct object *obj );

static const struct object_ops inode_ops =
{
    sizeof(struct inode),     /* size */
    &no_type,                 /* type */
    inode_dump,               /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    default_get_sync,         /* get_sync */
    default_map_access,       /* map_access */
    default_get_sd,           /* get_sd */
    default_set_sd,           /* set_sd */
    no_get_full_name,         /* get_full_name */
    no_lookup_name,           /* lookup_name */
    no_link_name,             /* link_name */
    NULL,                     /* unlink_name */
    no_open_file,             /* open_file */
    no_kernel_obj_list,       /* get_kernel_obj_list */
    no_close_handle,          /* close_handle */
    inode_destroy             /* destroy */
};

/* file lock object */

struct file_lock
{
    struct object       obj;         /* object header */
    struct fd          *fd;          /* fd owning this lock */
    struct list         fd_entry;    /* entry in list of locks on a given fd */
    struct list         inode_entry; /* entry in inode list of locks */
    int                 shared;      /* shared lock? */
    file_pos_t          start;       /* locked region is interval [start;end) */
    file_pos_t          end;
    struct process     *process;     /* process owning this lock */
    struct list         proc_entry;  /* entry in list of locks owned by the process */
};

static void file_lock_dump( struct object *obj, int verbose );
static int file_lock_signaled( struct object *obj, struct wait_queue_entry *entry );

static const struct object_ops file_lock_ops =
{
    sizeof(struct file_lock),   /* size */
    &no_type,                   /* type */
    file_lock_dump,             /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    file_lock_signaled,         /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_get_sync,           /* get_sync */
    default_map_access,         /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    no_get_full_name,           /* get_full_name */
    no_lookup_name,             /* lookup_name */
    no_link_name,               /* link_name */
    NULL,                       /* unlink_name */
    no_open_file,               /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    no_destroy                  /* destroy */
};


#define OFF_T_MAX       (~((file_pos_t)1 << (8*sizeof(off_t)-1)))
#define FILE_POS_T_MAX  (~(file_pos_t)0)

static file_pos_t max_unix_offset = OFF_T_MAX;

#define DUMP_LONG_LONG(val) do { \
    if (sizeof(val) > sizeof(unsigned long) && (val) > ~0UL) \
        fprintf( stderr, "%lx%08lx", (unsigned long)((unsigned long long)(val) >> 32), (unsigned long)(val) ); \
    else \
        fprintf( stderr, "%lx", (unsigned long)(val) ); \
  } while (0)



/****************************************************************/
/* timeouts support */

struct timeout_user
{
    struct list           entry;      /* entry in sorted timeout list */
    abstime_t             when;       /* timeout expiry */
    timeout_callback      callback;   /* callback function */
    void                 *private;    /* callback private data */
};

static struct list abs_timeout_list = LIST_INIT(abs_timeout_list); /* sorted absolute timeouts list */
static struct list rel_timeout_list = LIST_INIT(rel_timeout_list); /* sorted relative timeouts list */
timeout_t current_time;
timeout_t monotonic_time;

struct _KUSER_SHARED_DATA *user_shared_data = NULL;
static const timeout_t user_shared_data_timeout = 16 * 10000;

static void atomic_store_ulong(volatile ULONG *ptr, ULONG value)
{
    /* on x86 there should be total store order guarantees, so volatile is
     * enough to ensure the stores aren't reordered by the compiler, and then
     * they will always be seen in-order from other CPUs. On other archs, we
     * need atomic intrinsics to guarantee that. */
#if defined(__i386__) || defined(__x86_64__)
    *ptr = value;
#else
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
#endif
}

static void atomic_store_long(volatile LONG *ptr, LONG value)
{
#if defined(__i386__) || defined(__x86_64__)
    *ptr = value;
#else
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
#endif
}

static void set_user_shared_data_time(void)
{
    timeout_t tick_count = monotonic_time / 10000;
    static timeout_t last_timezone_update;
    timeout_t timezone_bias;
    struct tm *tm;
    time_t now;

    if (monotonic_time - last_timezone_update > TICKS_PER_SEC)
    {
        now = time( NULL );
        tm = gmtime( &now );
        timezone_bias = mktime( tm ) - now;
        tm = localtime( &now );
        if (tm->tm_isdst) timezone_bias -= 3600;
        timezone_bias *= TICKS_PER_SEC;

        atomic_store_long(&user_shared_data->TimeZoneBias.High2Time, timezone_bias >> 32);
        atomic_store_ulong(&user_shared_data->TimeZoneBias.LowPart, timezone_bias);
        atomic_store_long(&user_shared_data->TimeZoneBias.High1Time, timezone_bias >> 32);

        last_timezone_update = monotonic_time;
    }

    atomic_store_long(&user_shared_data->SystemTime.High2Time, current_time >> 32);
    atomic_store_ulong(&user_shared_data->SystemTime.LowPart, current_time);
    atomic_store_long(&user_shared_data->SystemTime.High1Time, current_time >> 32);

    atomic_store_long(&user_shared_data->InterruptTime.High2Time, monotonic_time >> 32);
    atomic_store_ulong(&user_shared_data->InterruptTime.LowPart, monotonic_time);
    atomic_store_long(&user_shared_data->InterruptTime.High1Time, monotonic_time >> 32);

    atomic_store_long(&user_shared_data->TickCount.High2Time, tick_count >> 32);
    atomic_store_ulong(&user_shared_data->TickCount.LowPart, tick_count);
    atomic_store_long(&user_shared_data->TickCount.High1Time, tick_count >> 32);
    atomic_store_ulong(&user_shared_data->TickCountLowDeprecated, tick_count);
}

void set_current_time(void)
{
    static const timeout_t ticks_1601_to_1970 = (timeout_t)86400 * (369 * 365 + 89) * TICKS_PER_SEC;
    struct timeval now;
    gettimeofday( &now, NULL );
    current_time = (timeout_t)now.tv_sec * TICKS_PER_SEC + now.tv_usec * 10 + ticks_1601_to_1970;
    monotonic_time = monotonic_counter();
    if (user_shared_data) set_user_shared_data_time();
}

/* add a timeout user */
struct timeout_user *add_timeout_user( timeout_t when, timeout_callback func, void *private )
{
    struct timeout_user *user;
    struct list *ptr;

    if (!(user = mem_alloc( sizeof(*user) ))) return NULL;
    user->when     = timeout_to_abstime( when );
    user->callback = func;
    user->private  = private;

    /* Now insert it in the linked list */

    if (user->when > 0)
    {
        LIST_FOR_EACH( ptr, &abs_timeout_list )
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            if (timeout->when >= user->when) break;
        }
    }
    else
    {
        LIST_FOR_EACH( ptr, &rel_timeout_list )
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            if (timeout->when <= user->when) break;
        }
    }
    list_add_before( ptr, &user->entry );
    return user;
}

/* remove a timeout user */
void remove_timeout_user( struct timeout_user *user )
{
    list_remove( &user->entry );
    free( user );
}

/* return a text description of a timeout for debugging purposes */
const char *get_timeout_str( timeout_t timeout )
{
    static char buffer[64];
    long secs, nsecs;

    if (!timeout) return "0";
    if (timeout == TIMEOUT_INFINITE) return "infinite";

    if (timeout < 0)  /* relative */
    {
        secs = -timeout / TICKS_PER_SEC;
        nsecs = -timeout % TICKS_PER_SEC;
        snprintf( buffer, sizeof(buffer), "+%ld.%07ld", secs, nsecs );
    }
    else  /* absolute */
    {
        secs = (timeout - current_time) / TICKS_PER_SEC;
        nsecs = (timeout - current_time) % TICKS_PER_SEC;
        if (nsecs < 0)
        {
            nsecs += TICKS_PER_SEC;
            secs--;
        }
        if (secs >= 0)
            snprintf( buffer, sizeof(buffer), "%x%08x (+%ld.%07ld)",
                      (unsigned int)(timeout >> 32), (unsigned int)timeout, secs, nsecs );
        else
            snprintf( buffer, sizeof(buffer), "%x%08x (-%ld.%07ld)",
                      (unsigned int)(timeout >> 32), (unsigned int)timeout,
                      -(secs + 1), TICKS_PER_SEC - nsecs );
    }
    return buffer;
}


/****************************************************************/
/* poll support */

static struct fd **poll_users;              /* users array */
static struct pollfd *pollfd;               /* poll fd array */
static int nb_users;                        /* count of array entries actually in use */
static int active_users;                    /* current number of active users */
static int allocated_users;                 /* count of allocated entries in the array */
static struct fd **freelist;                /* list of free entries in the array */

static int get_next_timeout( struct timespec *ts );

static inline void fd_poll_event( struct fd *fd, int event )
{
    fd->fd_ops->poll_event( fd, event );
}

#ifdef USE_EPOLL

static int epoll_fd = -1;

static inline void init_epoll(void)
{
    epoll_fd = epoll_create( 128 );
}

/* set the events that epoll waits for on this fd; helper for set_fd_events */
static inline void set_fd_epoll_events( struct fd *fd, int user, int events )
{
    struct epoll_event ev;
    int ctl;

    if (epoll_fd == -1) return;

    if (events == -1)  /* stop waiting on this fd completely */
    {
        if (pollfd[user].fd == -1) return;  /* already removed */
        ctl = EPOLL_CTL_DEL;
    }
    else if (pollfd[user].fd == -1)
    {
        ctl = EPOLL_CTL_ADD;
    }
    else
    {
        if (pollfd[user].events == events) return;  /* nothing to do */
        ctl = EPOLL_CTL_MOD;
    }

    ev.events = events;
    memset(&ev.data, 0, sizeof(ev.data));
    ev.data.u32 = user;

    if (epoll_ctl( epoll_fd, ctl, fd->unix_fd, &ev ) == -1)
    {
        if (errno == ENOMEM)  /* not enough memory, give up on epoll */
        {
            close( epoll_fd );
            epoll_fd = -1;
        }
        else perror( "epoll_ctl" );  /* should not happen */
    }
}

static inline void remove_epoll_user( struct fd *fd, int user )
{
    if (epoll_fd == -1) return;

    if (pollfd[user].fd != -1)
    {
        struct epoll_event dummy;
        epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd->unix_fd, &dummy );
    }
}

static inline void main_loop_epoll(void)
{
    int i, ret, timeout;
    struct timespec ts;
    struct epoll_event events[128];
#ifdef HAVE_EPOLL_PWAIT2
    static int failed_epoll_pwait2 = 0;
#endif

    assert( POLLIN == EPOLLIN );
    assert( POLLOUT == EPOLLOUT );
    assert( POLLERR == EPOLLERR );
    assert( POLLHUP == EPOLLHUP );

    if (epoll_fd == -1) return;

    while (active_users)
    {
        timeout = get_next_timeout( &ts );

        if (!active_users) break;  /* last user removed by a timeout */
        if (epoll_fd == -1) break;  /* an error occurred with epoll */

#ifdef HAVE_EPOLL_PWAIT2
        if (!failed_epoll_pwait2)
        {
            ret = epoll_pwait2( epoll_fd, events, ARRAY_SIZE( events ), timeout == -1 ? NULL : &ts, NULL );
            if (ret == -1 && errno == ENOSYS)
                failed_epoll_pwait2 = 1;
        }
        if (failed_epoll_pwait2)
#endif
            ret = epoll_wait( epoll_fd, events, ARRAY_SIZE( events ), timeout );

        set_current_time();

        /* put the events into the pollfd array first, like poll does */
        for (i = 0; i < ret; i++)
        {
            int user = events[i].data.u32;
            pollfd[user].revents = events[i].events;
        }

        /* read events from the pollfd array, as set_fd_events may modify them */
        for (i = 0; i < ret; i++)
        {
            int user = events[i].data.u32;
            if (pollfd[user].revents) fd_poll_event( poll_users[user], pollfd[user].revents );
        }
    }
}

#elif defined(HAVE_KQUEUE)

static int kqueue_fd = -1;

static inline void init_epoll(void)
{
    kqueue_fd = kqueue();
}

static inline void set_fd_epoll_events( struct fd *fd, int user, int events )
{
    struct kevent ev[2];

    if (kqueue_fd == -1) return;

    EV_SET( &ev[0], fd->unix_fd, EVFILT_READ, 0, NOTE_LOWAT, 1, (void *)(long)user );
    EV_SET( &ev[1], fd->unix_fd, EVFILT_WRITE, 0, NOTE_LOWAT, 1, (void *)(long)user );

    if (events == -1)  /* stop waiting on this fd completely */
    {
        if (pollfd[user].fd == -1) return;  /* already removed */
        ev[0].flags |= EV_DELETE;
        ev[1].flags |= EV_DELETE;
    }
    else if (pollfd[user].fd == -1)
    {
        ev[0].flags |= EV_ADD | ((events & POLLIN) ? EV_ENABLE : EV_DISABLE);
        ev[1].flags |= EV_ADD | ((events & POLLOUT) ? EV_ENABLE : EV_DISABLE);
    }
    else
    {
        if (pollfd[user].events == events) return;  /* nothing to do */
        ev[0].flags |= (events & POLLIN) ? EV_ENABLE : EV_DISABLE;
        ev[1].flags |= (events & POLLOUT) ? EV_ENABLE : EV_DISABLE;
    }

    if (kevent( kqueue_fd, ev, 2, NULL, 0, NULL ) == -1)
    {
        if (errno == ENOMEM)  /* not enough memory, give up on kqueue */
        {
            close( kqueue_fd );
            kqueue_fd = -1;
        }
        else perror( "kevent" );  /* should not happen */
    }
}

static inline void remove_epoll_user( struct fd *fd, int user )
{
    if (kqueue_fd == -1) return;

    if (pollfd[user].fd != -1)
    {
        struct kevent ev[2];

        EV_SET( &ev[0], fd->unix_fd, EVFILT_READ, EV_DELETE, 0, 0, 0 );
        EV_SET( &ev[1], fd->unix_fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0 );
        kevent( kqueue_fd, ev, 2, NULL, 0, NULL );
    }
}

static inline void main_loop_epoll(void)
{
    int i, ret, timeout;
    struct timespec ts;
    struct kevent events[128];

    if (kqueue_fd == -1) return;

    while (active_users)
    {
        timeout = get_next_timeout( &ts );

        if (!active_users) break;  /* last user removed by a timeout */
        if (kqueue_fd == -1) break;  /* an error occurred with kqueue */

        ret = kevent( kqueue_fd, NULL, 0, events, ARRAY_SIZE( events ), timeout == -1 ? NULL : &ts );

        set_current_time();

        /* put the events into the pollfd array first, like poll does */
        for (i = 0; i < ret; i++)
        {
            long user = (long)events[i].udata;
            pollfd[user].revents = 0;
        }
        for (i = 0; i < ret; i++)
        {
            long user = (long)events[i].udata;
            if (events[i].filter == EVFILT_READ) pollfd[user].revents |= POLLIN;
            else if (events[i].filter == EVFILT_WRITE) pollfd[user].revents |= POLLOUT;
            if (events[i].flags & EV_EOF) pollfd[user].revents |= POLLHUP;
            if (events[i].flags & EV_ERROR) pollfd[user].revents |= POLLERR;
        }

        /* read events from the pollfd array, as set_fd_events may modify them */
        for (i = 0; i < ret; i++)
        {
            long user = (long)events[i].udata;
            if (pollfd[user].revents) fd_poll_event( poll_users[user], pollfd[user].revents );
            pollfd[user].revents = 0;
        }
    }
}

#elif defined(USE_EVENT_PORTS)

static int port_fd = -1;

static inline void init_epoll(void)
{
    port_fd = port_create();
}

static inline void set_fd_epoll_events( struct fd *fd, int user, int events )
{
    int ret;

    if (port_fd == -1) return;

    if (events == -1)  /* stop waiting on this fd completely */
    {
        if (pollfd[user].fd == -1) return;  /* already removed */
        port_dissociate( port_fd, PORT_SOURCE_FD, fd->unix_fd );
    }
    else if (pollfd[user].fd == -1)
    {
        ret = port_associate( port_fd, PORT_SOURCE_FD, fd->unix_fd, events, (void *)user );
    }
    else
    {
        if (pollfd[user].events == events) return;  /* nothing to do */
        ret = port_associate( port_fd, PORT_SOURCE_FD, fd->unix_fd, events, (void *)user );
    }

    if (ret == -1)
    {
        if (errno == ENOMEM)  /* not enough memory, give up on port_associate */
        {
            close( port_fd );
            port_fd = -1;
        }
        else perror( "port_associate" );  /* should not happen */
    }
}

static inline void remove_epoll_user( struct fd *fd, int user )
{
    if (port_fd == -1) return;

    if (pollfd[user].fd != -1)
    {
        port_dissociate( port_fd, PORT_SOURCE_FD, fd->unix_fd );
    }
}

static inline void main_loop_epoll(void)
{
    int i, nget, ret, timeout;
    struct timespec ts;
    port_event_t events[128];

    if (port_fd == -1) return;

    while (active_users)
    {
        timeout = get_next_timeout( &ts );
        nget = 1;

        if (!active_users) break;  /* last user removed by a timeout */
        if (port_fd == -1) break;  /* an error occurred with event completion */

        ret = port_getn( port_fd, events, ARRAY_SIZE( events ), &nget, timeout == -1 ? NULL : &ts );

	if (ret == -1) break;  /* an error occurred with event completion */

        set_current_time();

        /* put the events into the pollfd array first, like poll does */
        for (i = 0; i < nget; i++)
        {
            long user = (long)events[i].portev_user;
            pollfd[user].revents = events[i].portev_events;
        }

        /* read events from the pollfd array, as set_fd_events may modify them */
        for (i = 0; i < nget; i++)
        {
            long user = (long)events[i].portev_user;
            if (pollfd[user].revents) fd_poll_event( poll_users[user], pollfd[user].revents );
            /* if we are still interested, reassociate the fd */
            if (pollfd[user].fd != -1) {
                port_associate( port_fd, PORT_SOURCE_FD, pollfd[user].fd, pollfd[user].events, (void *)user );
            }
        }
    }
}

#else /* HAVE_KQUEUE */

static inline void init_epoll(void) { }
static inline void set_fd_epoll_events( struct fd *fd, int user, int events ) { }
static inline void remove_epoll_user( struct fd *fd, int user ) { }
static inline void main_loop_epoll(void) { }

#endif /* USE_EPOLL */


/* add a user in the poll array and return its index, or -1 on failure */
static int add_poll_user( struct fd *fd )
{
    int ret;
    if (freelist)
    {
        ret = freelist - poll_users;
        freelist = (struct fd **)poll_users[ret];
    }
    else
    {
        if (nb_users == allocated_users)
        {
            struct fd **newusers;
            struct pollfd *newpoll;
            int new_count = allocated_users ? (allocated_users + allocated_users / 2) : 16;
            if (!(newusers = realloc( poll_users, new_count * sizeof(*poll_users) ))) return -1;
            if (!(newpoll = realloc( pollfd, new_count * sizeof(*pollfd) )))
            {
                if (allocated_users)
                    poll_users = newusers;
                else
                    free( newusers );
                return -1;
            }
            poll_users = newusers;
            pollfd = newpoll;
            if (!allocated_users) init_epoll();
            allocated_users = new_count;
        }
        ret = nb_users++;
    }
    pollfd[ret].fd = -1;
    pollfd[ret].events = 0;
    pollfd[ret].revents = 0;
    poll_users[ret] = fd;
    active_users++;
    return ret;
}

/* remove a user from the poll list */
static void remove_poll_user( struct fd *fd, int user )
{
    assert( user >= 0 );
    assert( poll_users[user] == fd );

    remove_epoll_user( fd, user );
    pollfd[user].fd = -1;
    pollfd[user].events = 0;
    pollfd[user].revents = 0;
    poll_users[user] = (struct fd *)freelist;
    freelist = &poll_users[user];
    active_users--;
}

/* process pending timeouts and return the time until the next timeout in milliseconds,
 * and full nanosecond precision in the timespec parameter if given */
static int get_next_timeout( struct timespec *ts )
{
    timeout_t ret = user_shared_data ? user_shared_data_timeout : -1;

    if (!list_empty( &abs_timeout_list ) || !list_empty( &rel_timeout_list ))
    {
        struct list expired_list, *ptr;

        /* first remove all expired timers from the list */

        list_init( &expired_list );
        while ((ptr = list_head( &abs_timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );

            if (timeout->when <= current_time)
            {
                list_remove( &timeout->entry );
                list_add_tail( &expired_list, &timeout->entry );
            }
            else break;
        }
        while ((ptr = list_head( &rel_timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );

            if (-timeout->when <= monotonic_time)
            {
                list_remove( &timeout->entry );
                list_add_tail( &expired_list, &timeout->entry );
            }
            else break;
        }

        /* now call the callback for all the removed timers */

        while ((ptr = list_head( &expired_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            list_remove( &timeout->entry );
            timeout->callback( timeout->private );
            free( timeout );
        }

        if ((ptr = list_head( &abs_timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            timeout_t diff = timeout->when - current_time;
            if (diff < 0) diff = 0;
            if (ret == -1 || diff < ret) ret = diff;
        }

        if ((ptr = list_head( &rel_timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            timeout_t diff = -timeout->when - monotonic_time;
            if (diff < 0) diff = 0;
            if (ret == -1 || diff < ret) ret = diff;
        }
    }

    /* infinite */
    if (ret == -1) return -1;

    if (ts)
    {
        ts->tv_sec = ret / TICKS_PER_SEC;
        ts->tv_nsec = (ret % TICKS_PER_SEC) * 100;
    }

    /* convert to milliseconds, ceil to avoid spinning with 0 timeout */
    ret = (ret + 9999) / 10000;
    if (ret > INT_MAX) ret = INT_MAX;
    return ret;
}

/* server main poll() loop */
void main_loop(void)
{
    int i, ret, timeout;

    set_current_time();
    server_start_time = current_time;

    main_loop_epoll();
    /* fall through to normal poll loop */

    while (active_users)
    {
        timeout = get_next_timeout( NULL );

        if (!active_users) break;  /* last user removed by a timeout */

        ret = poll( pollfd, nb_users, timeout );
        set_current_time();

        if (ret > 0)
        {
            for (i = 0; i < nb_users; i++)
            {
                if (pollfd[i].revents)
                {
                    fd_poll_event( poll_users[i], pollfd[i].revents );
                    if (!--ret) break;
                }
            }
        }
    }
}


/****************************************************************/
/* device functions */

static struct list device_hash[DEVICE_HASH_SIZE];

static int is_device_removable( dev_t dev, int unix_fd )
{
#if defined(linux) && defined(HAVE_FSTATFS)
    struct statfs stfs;

    /* check for floppy disk */
    if (major(dev) == FLOPPY_MAJOR) return 1;

    if (fstatfs( unix_fd, &stfs ) == -1) return 0;
    return (stfs.f_type == 0x9660 ||    /* iso9660 */
            stfs.f_type == 0x9fa1 ||    /* supermount */
            stfs.f_type == 0x15013346); /* udf */
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__APPLE__)
    struct statfs stfs;

    if (fstatfs( unix_fd, &stfs ) == -1) return 0;
    return (!strcmp("cd9660", stfs.f_fstypename) || !strcmp("udf", stfs.f_fstypename));
#elif defined(__NetBSD__)
    struct statvfs stfs;

    if (fstatvfs( unix_fd, &stfs ) == -1) return 0;
    return (!strcmp("cd9660", stfs.f_fstypename) || !strcmp("udf", stfs.f_fstypename));
#elif defined(sun)
# include <sys/dkio.h>
# include <sys/vtoc.h>
    struct dk_cinfo dkinf;
    if (ioctl( unix_fd, DKIOCINFO, &dkinf ) == -1) return 0;
    return (dkinf.dki_ctype == DKC_CDROM ||
            dkinf.dki_ctype == DKC_NCRFLOPPY ||
            dkinf.dki_ctype == DKC_SMSFLOPPY ||
            dkinf.dki_ctype == DKC_INTEL82072 ||
            dkinf.dki_ctype == DKC_INTEL82077);
#else
    return 0;
#endif
}

/* retrieve the device object for a given fd, creating it if needed */
static struct device *get_device( dev_t dev, int unix_fd )
{
    struct device *device;
    unsigned int i, hash = dev % DEVICE_HASH_SIZE;

    if (device_hash[hash].next)
    {
        LIST_FOR_EACH_ENTRY( device, &device_hash[hash], struct device, entry )
            if (device->dev == dev) return (struct device *)grab_object( device );
    }
    else list_init( &device_hash[hash] );

    /* not found, create it */

    if (unix_fd == -1) return NULL;
    if ((device = alloc_object( &device_ops )))
    {
        device->dev = dev;
        device->removable = is_device_removable( dev, unix_fd );
        for (i = 0; i < INODE_HASH_SIZE; i++) list_init( &device->inode_hash[i] );
        list_add_head( &device_hash[hash], &device->entry );
    }
    return device;
}

static void device_dump( struct object *obj, int verbose )
{
    struct device *device = (struct device *)obj;
    fprintf( stderr, "Device dev=" );
    DUMP_LONG_LONG( device->dev );
    fprintf( stderr, "\n" );
}

static void device_destroy( struct object *obj )
{
    struct device *device = (struct device *)obj;
    unsigned int i;

    for (i = 0; i < INODE_HASH_SIZE; i++)
        assert( list_empty(&device->inode_hash[i]) );

    list_remove( &device->entry );  /* remove it from the hash table */
}

/****************************************************************/
/* inode functions */

static void unlink_closed_fd( struct inode *inode, struct closed_fd *fd )
{
    /* make sure it is still the same file */
    struct stat st;
    if (!stat( fd->unix_name, &st ) && st.st_dev == inode->device->dev && st.st_ino == inode->ino)
    {
        if (S_ISDIR(st.st_mode)) rmdir( fd->unix_name );
        else unlink( fd->unix_name );
    }
}

/* close all pending file descriptors in the closed list */
static void inode_close_pending( struct inode *inode, int keep_unlinks )
{
    struct list *ptr = list_head( &inode->closed );

    while (ptr)
    {
        struct closed_fd *fd = LIST_ENTRY( ptr, struct closed_fd, entry );
        struct list *next = list_next( &inode->closed, ptr );

        if (fd->unix_fd != -1)
        {
            close( fd->unix_fd );
            fd->unix_fd = -1;
        }

        /* get rid of it unless there's an unlink pending on that file */
        if (!keep_unlinks || !(fd->disp_flags & FILE_DISPOSITION_DELETE))
        {
            list_remove( ptr );
            free( fd->unix_name );
            free( fd );
        }
        ptr = next;
    }
}

static void inode_dump( struct object *obj, int verbose )
{
    struct inode *inode = (struct inode *)obj;
    fprintf( stderr, "Inode device=%p ino=", inode->device );
    DUMP_LONG_LONG( inode->ino );
    fprintf( stderr, "\n" );
}

static void inode_destroy( struct object *obj )
{
    struct inode *inode = (struct inode *)obj;
    struct list *ptr;

    assert( list_empty(&inode->open) );
    assert( list_empty(&inode->locks) );

    list_remove( &inode->entry );

    while ((ptr = list_head( &inode->closed )))
    {
        struct closed_fd *fd = LIST_ENTRY( ptr, struct closed_fd, entry );
        list_remove( ptr );
        if (fd->unix_fd != -1) close( fd->unix_fd );
        if (fd->disp_flags & FILE_DISPOSITION_DELETE)
            unlink_closed_fd( inode, fd );
        free( fd->unix_name );
        free( fd );
    }
    release_object( inode->device );
}

/* retrieve the inode object for a given fd, creating it if needed */
static struct inode *get_inode( dev_t dev, ino_t ino, int unix_fd )
{
    struct device *device;
    struct inode *inode;
    unsigned int hash = ino % INODE_HASH_SIZE;

    if (!(device = get_device( dev, unix_fd ))) return NULL;

    LIST_FOR_EACH_ENTRY( inode, &device->inode_hash[hash], struct inode, entry )
    {
        if (inode->ino == ino)
        {
            release_object( device );
            return (struct inode *)grab_object( inode );
        }
    }

    /* not found, create it */
    if ((inode = alloc_object( &inode_ops )))
    {
        inode->device = device;
        inode->ino    = ino;
        list_init( &inode->open );
        list_init( &inode->locks );
        list_init( &inode->closed );
        list_add_head( &device->inode_hash[hash], &inode->entry );
    }
    else release_object( device );

    return inode;
}

static int inode_has_pending_close( struct inode *inode, const char *path )
{
    struct closed_fd *fd;

    LIST_FOR_EACH_ENTRY( fd, &inode->closed, struct closed_fd, entry )
        if (!strcmp( fd->unix_name, path )) return 1;
    return 0;
}

/* add fd to the inode list of file descriptors to close */
static void inode_add_closed_fd( struct inode *inode, struct closed_fd *fd )
{
    if (!list_empty( &inode->locks ))
    {
        list_add_head( &inode->closed, &fd->entry );
    }
    else if ((fd->disp_flags & FILE_DISPOSITION_DELETE) &&
        (fd->disp_flags & FILE_DISPOSITION_POSIX_SEMANTICS))
    {
        /* close the fd and unlink it at once */
        if (fd->unix_fd != -1) close( fd->unix_fd );
        unlink_closed_fd( inode, fd );
        free( fd->unix_name );
        free( fd );
    }
    else if ((fd->disp_flags & FILE_DISPOSITION_DELETE) && !inode_has_pending_close( inode, fd->unix_name ))
    {
        /* close the fd but keep the structure around for unlink */
        if (fd->unix_fd != -1) close( fd->unix_fd );
        fd->unix_fd = -1;
        list_add_head( &inode->closed, &fd->entry );
    }
    else  /* no locks on this inode and no unlink, get rid of the fd */
    {
        if (fd->unix_fd != -1) close( fd->unix_fd );
        free( fd->unix_name );
        free( fd );
    }
}


/****************************************************************/
/* file lock functions */

static void file_lock_dump( struct object *obj, int verbose )
{
    struct file_lock *lock = (struct file_lock *)obj;
    fprintf( stderr, "Lock %s fd=%p proc=%p start=",
             lock->shared ? "shared" : "excl", lock->fd, lock->process );
    DUMP_LONG_LONG( lock->start );
    fprintf( stderr, " end=" );
    DUMP_LONG_LONG( lock->end );
    fprintf( stderr, "\n" );
}

static int file_lock_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct file_lock *lock = (struct file_lock *)obj;
    /* lock is signaled if it has lost its owner */
    return !lock->process;
}

/* set (or remove) a Unix lock if possible for the given range */
static int set_unix_lock( struct fd *fd, file_pos_t start, file_pos_t end, int type )
{
    struct flock fl;

    if (!fd->fs_locks) return 1;  /* no fs locks possible for this fd */
    for (;;)
    {
        if (start == end) return 1;  /* can't set zero-byte lock */
        if (start > max_unix_offset) return 1;  /* ignore it */
        fl.l_type   = type;
        fl.l_whence = SEEK_SET;
        fl.l_start  = start;
        if (!end || end > max_unix_offset) fl.l_len = 0;
        else fl.l_len = end - start;
        if (fcntl( fd->unix_fd, F_SETLK, &fl ) != -1) return 1;

        switch(errno)
        {
        case EACCES:
            /* check whether locks work at all on this file system */
            if (fcntl( fd->unix_fd, F_GETLK, &fl ) != -1)
            {
                set_error( STATUS_FILE_LOCK_CONFLICT );
                return 0;
            }
            /* fall through */
        case EIO:
        case ENOLCK:
        case ENOTSUP:
            /* no locking on this fs, just ignore it */
            fd->fs_locks = 0;
            return 1;
        case EAGAIN:
            set_error( STATUS_FILE_LOCK_CONFLICT );
            return 0;
        case EBADF:
            /* this can happen if we try to set a write lock on a read-only file */
            /* try to at least grab a read lock */
            if (fl.l_type == F_WRLCK)
            {
                type = F_RDLCK;
                break; /* retry */
            }
            set_error( STATUS_ACCESS_DENIED );
            return 0;
#ifdef EOVERFLOW
        case EOVERFLOW:
#endif
        case EINVAL:
            /* this can happen if off_t is 64-bit but the kernel only supports 32-bit */
            /* in that case we shrink the limit and retry */
            if (max_unix_offset > INT_MAX)
            {
                max_unix_offset = INT_MAX;
                break;  /* retry */
            }
            /* fall through */
        default:
            file_set_error();
            return 0;
        }
    }
}

/* check if interval [start;end) overlaps the lock */
static inline int lock_overlaps( struct file_lock *lock, file_pos_t start, file_pos_t end )
{
    if (lock->end && start >= lock->end) return 0;
    if (end && lock->start >= end) return 0;
    return 1;
}

/* remove Unix locks for all bytes in the specified area that are no longer locked */
static void remove_unix_locks( struct fd *fd, file_pos_t start, file_pos_t end )
{
    struct hole
    {
        struct hole *next;
        struct hole *prev;
        file_pos_t   start;
        file_pos_t   end;
    } *first, *cur, *next, *buffer;

    struct list *ptr;
    int count = 0;

    if (!fd->inode) return;
    if (!fd->fs_locks) return;
    if (start == end || start > max_unix_offset) return;
    if (!end || end > max_unix_offset) end = max_unix_offset + 1;

    /* count the number of locks overlapping the specified area */

    LIST_FOR_EACH( ptr, &fd->inode->locks )
    {
        struct file_lock *lock = LIST_ENTRY( ptr, struct file_lock, inode_entry );
        if (lock->start == lock->end) continue;
        if (lock_overlaps( lock, start, end )) count++;
    }

    if (!count)  /* no locks at all, we can unlock everything */
    {
        set_unix_lock( fd, start, end, F_UNLCK );
        return;
    }

    /* allocate space for the list of holes */
    /* max. number of holes is number of locks + 1 */

    if (!(buffer = malloc( sizeof(*buffer) * (count+1) ))) return;
    first = buffer;
    first->next  = NULL;
    first->prev  = NULL;
    first->start = start;
    first->end   = end;
    next = first + 1;

    /* build a sorted list of unlocked holes in the specified area */

    LIST_FOR_EACH( ptr, &fd->inode->locks )
    {
        struct file_lock *lock = LIST_ENTRY( ptr, struct file_lock, inode_entry );
        if (lock->start == lock->end) continue;
        if (!lock_overlaps( lock, start, end )) continue;

        /* go through all the holes touched by this lock */
        for (cur = first; cur; cur = cur->next)
        {
            if (cur->end <= lock->start) continue; /* hole is before start of lock */
            if (lock->end && cur->start >= lock->end) break;  /* hole is after end of lock */

            /* now we know that lock is overlapping hole */

            if (cur->start >= lock->start)  /* lock starts before hole, shrink from start */
            {
                cur->start = lock->end;
                if (cur->start && cur->start < cur->end) break;  /* done with this lock */
                /* now hole is empty, remove it */
                if (cur->next) cur->next->prev = cur->prev;
                if (cur->prev) cur->prev->next = cur->next;
                else if (!(first = cur->next)) goto done;  /* no more holes at all */
            }
            else if (!lock->end || cur->end <= lock->end)  /* lock larger than hole, shrink from end */
            {
                cur->end = lock->start;
                assert( cur->start < cur->end );
            }
            else  /* lock is in the middle of hole, split hole in two */
            {
                next->prev = cur;
                next->next = cur->next;
                cur->next = next;
                next->start = lock->end;
                next->end = cur->end;
                cur->end = lock->start;
                assert( next->start < next->end );
                assert( cur->end < next->start );
                next++;
                break;  /* done with this lock */
            }
        }
    }

    /* clear Unix locks for all the holes */

    for (cur = first; cur; cur = cur->next)
        set_unix_lock( fd, cur->start, cur->end, F_UNLCK );

 done:
    free( buffer );
}

/* create a new lock on a fd */
static struct file_lock *add_lock( struct fd *fd, int shared, file_pos_t start, file_pos_t end )
{
    struct file_lock *lock;

    if (!(lock = alloc_object( &file_lock_ops ))) return NULL;
    lock->shared  = shared;
    lock->start   = start;
    lock->end     = end;
    lock->fd      = fd;
    lock->process = current->process;

    /* now try to set a Unix lock */
    if (!set_unix_lock( lock->fd, lock->start, lock->end, lock->shared ? F_RDLCK : F_WRLCK ))
    {
        release_object( lock );
        return NULL;
    }
    list_add_tail( &fd->locks, &lock->fd_entry );
    list_add_tail( &fd->inode->locks, &lock->inode_entry );
    list_add_tail( &lock->process->locks, &lock->proc_entry );
    return lock;
}

/* remove an existing lock */
static void remove_lock( struct file_lock *lock, int remove_unix )
{
    struct inode *inode = lock->fd->inode;

    list_remove( &lock->fd_entry );
    list_remove( &lock->inode_entry );
    list_remove( &lock->proc_entry );
    if (remove_unix) remove_unix_locks( lock->fd, lock->start, lock->end );
    if (list_empty( &inode->locks )) inode_close_pending( inode, 1 );
    lock->process = NULL;
    wake_up( &lock->obj, 0 );
    release_object( lock );
}

/* remove all locks owned by a given process */
void remove_process_locks( struct process *process )
{
    struct list *ptr;

    while ((ptr = list_head( &process->locks )))
    {
        struct file_lock *lock = LIST_ENTRY( ptr, struct file_lock, proc_entry );
        remove_lock( lock, 1 );  /* this removes it from the list */
    }
}

/* remove all locks on a given fd */
static void remove_fd_locks( struct fd *fd )
{
    file_pos_t start = FILE_POS_T_MAX, end = 0;
    struct list *ptr;

    while ((ptr = list_head( &fd->locks )))
    {
        struct file_lock *lock = LIST_ENTRY( ptr, struct file_lock, fd_entry );
        if (lock->start < start) start = lock->start;
        if (!lock->end || lock->end > end) end = lock->end - 1;
        remove_lock( lock, 0 );
    }
    if (start < end) remove_unix_locks( fd, start, end + 1 );
}

/* add a lock on an fd */
/* returns handle to wait on */
obj_handle_t lock_fd( struct fd *fd, file_pos_t start, file_pos_t count, int shared, int wait )
{
    struct list *ptr;
    file_pos_t end = start + count;

    if (!fd->inode)  /* not a regular file */
    {
        set_error( STATUS_INVALID_DEVICE_REQUEST );
        return 0;
    }

    /* don't allow wrapping locks */
    if (end && end < start)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }

    /* check if another lock on that file overlaps the area */
    LIST_FOR_EACH( ptr, &fd->inode->locks )
    {
        struct file_lock *lock = LIST_ENTRY( ptr, struct file_lock, inode_entry );
        if (!lock_overlaps( lock, start, end )) continue;
        if (shared && (lock->shared || lock->fd == fd)) continue;
        /* found one */
        if (!wait)
        {
            set_error( STATUS_FILE_LOCK_CONFLICT );
            return 0;
        }
        set_error( STATUS_PENDING );
        return alloc_handle( current->process, lock, SYNCHRONIZE, 0 );
    }

    /* not found, add it */
    if (add_lock( fd, shared, start, end )) return 0;
    if (get_error() == STATUS_FILE_LOCK_CONFLICT)
    {
        /* Unix lock conflict -> tell client to wait and retry */
        if (wait) set_error( STATUS_PENDING );
    }
    return 0;
}

/* remove a lock on an fd */
void unlock_fd( struct fd *fd, file_pos_t start, file_pos_t count )
{
    struct list *ptr;
    file_pos_t end = start + count;

    /* find an existing lock with the exact same parameters */
    LIST_FOR_EACH( ptr, &fd->locks )
    {
        struct file_lock *lock = LIST_ENTRY( ptr, struct file_lock, fd_entry );
        if ((lock->start == start) && (lock->end == end))
        {
            remove_lock( lock, 1 );
            return;
        }
    }
    set_error( STATUS_FILE_LOCK_CONFLICT );
}


/****************************************************************/
/* file descriptor functions */

static void fd_dump( struct object *obj, int verbose )
{
    struct fd *fd = (struct fd *)obj;
    fprintf( stderr, "Fd unix_fd=%d user=%p options=%08x", fd->unix_fd, fd->user, fd->options );
    if (fd->inode) fprintf( stderr, " inode=%p disp_flags=%x", fd->inode, fd->closed->disp_flags );
    fprintf( stderr, "\n" );
}

static void fd_destroy( struct object *obj )
{
    struct fd *fd = (struct fd *)obj;

    free_async_queue( &fd->read_q );
    free_async_queue( &fd->write_q );
    free_async_queue( &fd->wait_q );

    if (fd->map_addr) free_map_addr( fd->map_addr, fd->map_size );
    if (fd->completion) release_object( fd->completion );
    remove_fd_locks( fd );
    list_remove( &fd->inode_entry );
    if (fd->poll_index != -1) remove_poll_user( fd, fd->poll_index );
    free( fd->nt_name );
    if (fd->inode)
    {
        inode_add_closed_fd( fd->inode, fd->closed );
        release_object( fd->inode );
    }
    else  /* no inode, close it right away */
    {
        if (fd->unix_fd != -1) close( fd->unix_fd );
        free( fd->unix_name );
    }
}

/* check if the desired access is possible without violating */
/* the sharing mode of other opens of the same file */
static unsigned int check_sharing( struct fd *fd, unsigned int access, unsigned int sharing,
                                   unsigned int open_flags, unsigned int options )
{
    /* only a few access bits are meaningful wrt sharing */
    const unsigned int read_access = FILE_READ_DATA | FILE_EXECUTE;
    const unsigned int write_access = FILE_WRITE_DATA | FILE_APPEND_DATA;
    const unsigned int all_access = read_access | write_access | DELETE;

    unsigned int existing_sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    unsigned int existing_access = 0;
    struct list *ptr;

    fd->access = access;
    fd->sharing = sharing;

    LIST_FOR_EACH( ptr, &fd->inode->open )
    {
        struct fd *fd_ptr = LIST_ENTRY( ptr, struct fd, inode_entry );
        if (fd_ptr != fd)
        {
            /* if access mode is 0, sharing mode is ignored */
            if (fd_ptr->access & all_access) existing_sharing &= fd_ptr->sharing;
            existing_access |= fd_ptr->access;
        }
    }

    if (((access & read_access) && !(existing_sharing & FILE_SHARE_READ)) ||
        ((access & write_access) && !(existing_sharing & FILE_SHARE_WRITE)) ||
        ((access & DELETE) && !(existing_sharing & FILE_SHARE_DELETE)))
        return STATUS_SHARING_VIOLATION;
    if (((existing_access & FILE_MAPPING_WRITE) && !(sharing & FILE_SHARE_WRITE)) ||
        ((existing_access & FILE_MAPPING_IMAGE) && (access & FILE_WRITE_DATA)))
        return STATUS_SHARING_VIOLATION;
    if ((existing_access & FILE_MAPPING_IMAGE) && (options & FILE_DELETE_ON_CLOSE))
        return STATUS_CANNOT_DELETE;
    if ((existing_access & FILE_MAPPING_ACCESS) && (open_flags & O_TRUNC))
        return STATUS_USER_MAPPED_FILE;
    if (!(access & all_access))
        return 0;  /* if access mode is 0, sharing mode is ignored (except for mappings) */
    if (((existing_access & read_access) && !(sharing & FILE_SHARE_READ)) ||
        ((existing_access & write_access) && !(sharing & FILE_SHARE_WRITE)) ||
        ((existing_access & DELETE) && !(sharing & FILE_SHARE_DELETE)))
        return STATUS_SHARING_VIOLATION;
    return 0;
}

/* set the events that select waits for on this fd */
void set_fd_events( struct fd *fd, int events )
{
    int user = fd->poll_index;
    assert( poll_users[user] == fd );

    set_fd_epoll_events( fd, user, events );

    if (events == -1)  /* stop waiting on this fd completely */
    {
        pollfd[user].fd = -1;
        pollfd[user].events = POLLERR;
        pollfd[user].revents = 0;
    }
    else
    {
        pollfd[user].fd = fd->unix_fd;
        pollfd[user].events = events;
    }
}

/* prepare an fd for unmounting its corresponding device */
static inline void unmount_fd( struct fd *fd )
{
    assert( fd->inode );

    async_wake_up( &fd->read_q, STATUS_VOLUME_DISMOUNTED );
    async_wake_up( &fd->write_q, STATUS_VOLUME_DISMOUNTED );

    if (fd->poll_index != -1) set_fd_events( fd, -1 );

    if (fd->unix_fd != -1) close( fd->unix_fd );

    fd->unix_fd = -1;
    fd->no_fd_status = STATUS_VOLUME_DISMOUNTED;
    fd->closed->unix_fd = -1;
    fd->closed->disp_flags = 0;

    /* stop using Unix locks on this fd (existing locks have been removed by close) */
    fd->fs_locks = 0;
}

/* allocate an fd object, without setting the unix fd yet */
static struct fd *alloc_fd_object(void)
{
    struct fd *fd = alloc_object( &fd_ops );

    if (!fd) return NULL;

    fd->fd_ops     = NULL;
    fd->user       = NULL;
    fd->inode      = NULL;
    fd->closed     = NULL;
    fd->map_addr   = 0;
    fd->map_size   = 0;
    fd->access     = 0;
    fd->options    = 0;
    fd->sharing    = 0;
    fd->unix_fd    = -1;
    fd->unix_name  = NULL;
    fd->nt_name    = NULL;
    fd->nt_namelen = 0;
    fd->cacheable  = 0;
    fd->signaled   = 1;
    fd->fs_locks   = 1;
    fd->poll_index = -1;
    fd->completion = NULL;
    fd->comp_flags = 0;
    init_async_queue( &fd->read_q );
    init_async_queue( &fd->write_q );
    init_async_queue( &fd->wait_q );
    list_init( &fd->inode_entry );
    list_init( &fd->locks );

    if ((fd->poll_index = add_poll_user( fd )) == -1)
    {
        release_object( fd );
        return NULL;
    }
    return fd;
}

/* allocate a pseudo fd object, for objects that need to behave like files but don't have a unix fd */
struct fd *alloc_pseudo_fd( const struct fd_ops *fd_user_ops, struct object *user, unsigned int options )
{
    struct fd *fd = alloc_object( &fd_ops );

    if (!fd) return NULL;

    fd->fd_ops     = fd_user_ops;
    fd->user       = user;
    fd->inode      = NULL;
    fd->closed     = NULL;
    fd->map_addr   = 0;
    fd->map_size   = 0;
    fd->access     = 0;
    fd->options    = options;
    fd->sharing    = 0;
    fd->unix_name  = NULL;
    fd->nt_name    = NULL;
    fd->nt_namelen = 0;
    fd->unix_fd    = -1;
    fd->cacheable  = 0;
    fd->signaled   = 1;
    fd->fs_locks   = 0;
    fd->poll_index = -1;
    fd->completion = NULL;
    fd->comp_flags = 0;
    fd->no_fd_status = STATUS_BAD_DEVICE_TYPE;
    init_async_queue( &fd->read_q );
    init_async_queue( &fd->write_q );
    init_async_queue( &fd->wait_q );
    list_init( &fd->inode_entry );
    list_init( &fd->locks );
    return fd;
}

/* duplicate an fd object for a different user */
struct fd *dup_fd_object( struct fd *orig, unsigned int access, unsigned int sharing, unsigned int options )
{
    unsigned int err;
    struct fd *fd = alloc_fd_object();

    if (!fd) return NULL;

    fd->options    = options;
    fd->cacheable  = orig->cacheable;

    if (orig->unix_name)
    {
        if (!(fd->unix_name = mem_alloc( strlen(orig->unix_name) + 1 ))) goto failed;
        strcpy( fd->unix_name, orig->unix_name );
    }
    if (orig->nt_namelen)
    {
        if (!(fd->nt_name = memdup( orig->nt_name, orig->nt_namelen ))) goto failed;
        fd->nt_namelen = orig->nt_namelen;
    }

    if (orig->inode)
    {
        struct closed_fd *closed = mem_alloc( sizeof(*closed) );
        if (!closed) goto failed;
        if ((fd->unix_fd = dup( orig->unix_fd )) == -1)
        {
            file_set_error();
            free( closed );
            goto failed;
        }
        closed->unix_fd = fd->unix_fd;
        closed->disp_flags = 0;
        closed->unix_name = fd->unix_name;
        fd->closed = closed;
        fd->inode = (struct inode *)grab_object( orig->inode );
        list_add_head( &fd->inode->open, &fd->inode_entry );
        if ((err = check_sharing( fd, access, sharing, 0, options )))
        {
            set_error( err );
            goto failed;
        }
    }
    else if ((fd->unix_fd = dup( orig->unix_fd )) == -1)
    {
        file_set_error();
        goto failed;
    }
    return fd;

failed:
    release_object( fd );
    return NULL;
}

/* find an existing fd object that can be reused for a mapping */
struct fd *get_fd_object_for_mapping( struct fd *fd, unsigned int access, unsigned int sharing )
{
    struct fd *fd_ptr;

    if (!fd->inode) return NULL;

    LIST_FOR_EACH_ENTRY( fd_ptr, &fd->inode->open, struct fd, inode_entry )
        if (fd_ptr->access == access && fd_ptr->sharing == sharing)
            return (struct fd *)grab_object( fd_ptr );

    return NULL;
}

/* sets the user of an fd that previously had no user */
void set_fd_user( struct fd *fd, const struct fd_ops *user_ops, struct object *user )
{
    assert( fd->fd_ops == NULL );
    fd->fd_ops = user_ops;
    fd->user   = user;
}

char *dup_fd_name( struct fd *root, const char *name )
{
    char *ret;

    if (!root) return strdup( name );
    if (!root->unix_name) return NULL;

    /* skip . prefix */
    if (name[0] == '.' && (!name[1] || name[1] == '/')) name++;

    if ((ret = malloc( strlen(root->unix_name) + strlen(name) + 2 )))
    {
        strcpy( ret, root->unix_name );
        if (name[0] && name[0] != '/') strcat( ret, "/" );
        strcat( ret, name );
    }
    return ret;
}

static WCHAR *dup_nt_name( struct fd *root, struct unicode_str name, data_size_t *len )
{
    WCHAR *ptr, *ret;

    if (!root)
    {
        *len = name.len;
        if (!name.len) return NULL;
        return memdup( name.str, name.len );
    }
    if (!root->nt_namelen) return NULL;

    /* skip . prefix */
    if (name.len && name.str[0] == '.' && (name.len == sizeof(WCHAR) || name.str[1] == '\\'))
    {
        name.str++;
        name.len -= sizeof(WCHAR);
    }
    if ((ret = malloc( root->nt_namelen + name.len + sizeof(WCHAR) )))
    {
        ptr = mem_append( ret, root->nt_name, root->nt_namelen );
        if (name.len && name.str[0] != '\\' && ptr[-1] != '\\') *ptr++ = '\\';
        ptr = mem_append( ptr, name.str, name.len );
        *len = (ptr - ret) * sizeof(WCHAR);
    }
    return ret;
}

void get_nt_name( struct fd *fd, struct unicode_str *name )
{
    name->str = fd->nt_name;
    name->len = fd->nt_namelen;
}

/* open() wrapper that returns a struct fd with no fd user set */
struct fd *open_fd( struct fd *root, const char *name, struct unicode_str nt_name,
                    int flags, mode_t *mode, unsigned int access,
                    unsigned int sharing, unsigned int options )
{
    struct stat st;
    struct closed_fd *closed_fd;
    struct fd *fd;
    int root_fd = -1;
    int rw_mode;
    char *path;

    if (((options & FILE_DELETE_ON_CLOSE) && !(access & DELETE)) ||
        ((options & FILE_DIRECTORY_FILE) && (flags & O_TRUNC)))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if (!(fd = alloc_fd_object())) return NULL;

    fd->options = options;
    if (!(closed_fd = mem_alloc( sizeof(*closed_fd) )))
    {
        release_object( fd );
        return NULL;
    }

    if (root)
    {
        if ((root_fd = get_unix_fd( root )) == -1) goto error;
        if (fchdir( root_fd ) == -1)
        {
            file_set_error();
            root_fd = -1;
            goto error;
        }
    }

    /* create the directory if needed */
    if ((options & FILE_DIRECTORY_FILE) && (flags & O_CREAT))
    {
        if (mkdir( name, *mode ) == -1)
        {
            if (errno != EEXIST || (flags & O_EXCL))
            {
                file_set_error();
                goto error;
            }
        }
        flags &= ~(O_CREAT | O_EXCL | O_TRUNC);
    }

    if ((access & FILE_UNIX_WRITE_ACCESS) && !(options & FILE_DIRECTORY_FILE))
    {
        if (access & FILE_UNIX_READ_ACCESS) rw_mode = O_RDWR;
        else rw_mode = O_WRONLY;
    }
    else rw_mode = O_RDONLY;

    if ((fd->unix_fd = open( name, rw_mode | (flags & ~O_TRUNC), *mode )) == -1)
    {
        /* if we tried to open a directory for write access, retry read-only */
        if (errno == EISDIR)
        {
            if ((access & FILE_UNIX_WRITE_ACCESS) || (flags & O_CREAT))
                fd->unix_fd = open( name, O_RDONLY | (flags & ~(O_TRUNC | O_CREAT | O_EXCL)), *mode );
        }

        if (fd->unix_fd == -1)
        {
            /* check for trailing slash on file path */
            if ((errno == ENOENT || (errno == ENOTDIR && !(options & FILE_DIRECTORY_FILE))) && name[strlen(name) - 1] == '/')
                set_error( STATUS_OBJECT_NAME_INVALID );
            else
                file_set_error();
            goto error;
        }
    }

    fd->nt_name = dup_nt_name( root, nt_name, &fd->nt_namelen );
    fd->unix_name = NULL;
    fstat( fd->unix_fd, &st );
    *mode = st.st_mode;

    /* only bother with an inode for normal files and directories */
    if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))
    {
        unsigned int err;
        struct inode *inode = get_inode( st.st_dev, st.st_ino, fd->unix_fd );

        if (!inode)
        {
            /* we can close the fd because there are no others open on the same file,
             * otherwise we wouldn't have failed to allocate a new inode
             */
            goto error;
        }

        if ((path = dup_fd_name( root, name )))
        {
            fd->unix_name = realpath( path, NULL );
            free( path );
        }

        closed_fd->unix_fd = fd->unix_fd;
        closed_fd->unix_name = fd->unix_name;
        closed_fd->disp_flags = 0;
        fd->inode = inode;
        fd->closed = closed_fd;
        fd->cacheable = !inode->device->removable;
        list_add_head( &inode->open, &fd->inode_entry );
        closed_fd = NULL;

        /* check directory options */
        if ((options & FILE_DIRECTORY_FILE) && !S_ISDIR(st.st_mode))
        {
            set_error( STATUS_NOT_A_DIRECTORY );
            goto error;
        }
        if ((options & FILE_NON_DIRECTORY_FILE) && S_ISDIR(st.st_mode))
        {
            set_error( STATUS_FILE_IS_A_DIRECTORY );
            goto error;
        }
        if ((err = check_sharing( fd, access, sharing, flags, options )))
        {
            set_error( err );
            goto error;
        }

        /* can't unlink files if we don't have permission to access */
        if ((options & FILE_DELETE_ON_CLOSE) && !(flags & O_CREAT) &&
            !(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
        {
            set_error( STATUS_CANNOT_DELETE );
            goto error;
        }

        fd->closed->disp_flags = (options & FILE_DELETE_ON_CLOSE) ?
            FILE_DISPOSITION_DELETE : 0;
        if (flags & O_TRUNC)
        {
            if (S_ISDIR(st.st_mode))
            {
                set_error( STATUS_OBJECT_NAME_COLLISION );
                goto error;
            }
            ftruncate( fd->unix_fd, 0 );
        }
    }
    else  /* special file */
    {
        if (options & FILE_DELETE_ON_CLOSE)  /* we can't unlink special files */
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto error;
        }
        free( closed_fd );
        fd->cacheable = 1;
    }

#ifdef HAVE_POSIX_FADVISE
    switch (options & (FILE_SEQUENTIAL_ONLY | FILE_RANDOM_ACCESS))
    {
    case FILE_SEQUENTIAL_ONLY:
        posix_fadvise( fd->unix_fd, 0, 0, POSIX_FADV_SEQUENTIAL );
        break;
    case FILE_RANDOM_ACCESS:
        posix_fadvise( fd->unix_fd, 0, 0, POSIX_FADV_RANDOM );
        break;
    }
#endif

    if (root_fd != -1) fchdir( server_dir_fd ); /* go back to the server dir */
    return fd;

error:
    release_object( fd );
    free( closed_fd );
    if (root_fd != -1) fchdir( server_dir_fd ); /* go back to the server dir */
    return NULL;
}

/* create an fd for an anonymous file */
/* if the function fails the unix fd is closed */
struct fd *create_anonymous_fd( const struct fd_ops *fd_user_ops, int unix_fd, struct object *user,
                                unsigned int options )
{
    struct fd *fd = alloc_fd_object();

    if (fd)
    {
        set_fd_user( fd, fd_user_ops, user );
        fd->unix_fd = unix_fd;
        fd->options = options;
        return fd;
    }
    close( unix_fd );
    return NULL;
}

/* retrieve the object that is using an fd */
void *get_fd_user( struct fd *fd )
{
    return fd->user;
}

/* retrieve the opening options for the fd */
unsigned int get_fd_options( struct fd *fd )
{
    return fd->options;
}

/* retrieve the completion flags for the fd */
unsigned int get_fd_comp_flags( struct fd *fd )
{
    return fd->comp_flags;
}

/* check if fd is in overlapped mode */
int is_fd_overlapped( struct fd *fd )
{
    return !(fd->options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));
}

/* retrieve the unix fd for an object */
int get_unix_fd( struct fd *fd )
{
    if (fd->unix_fd == -1) set_error( fd->no_fd_status );
    return fd->unix_fd;
}

/* retrieve the suggested mapping address for the fd */
client_ptr_t get_fd_map_address( struct fd *fd )
{
    return fd->map_addr;
}

/* set the suggested mapping address for the fd */
void set_fd_map_address( struct fd *fd, client_ptr_t addr, mem_size_t size )
{
    assert( !fd->map_addr );
    fd->map_addr = addr;
    fd->map_size = size;
}

/* check if two file descriptors point to the same file */
int is_same_file_fd( struct fd *fd1, struct fd *fd2 )
{
    return fd1->inode == fd2->inode;
}

/* allow the fd to be cached (can't be reset once set) */
void allow_fd_caching( struct fd *fd )
{
    fd->cacheable = 1;
}

/* check if fd is on a removable device */
int is_fd_removable( struct fd *fd )
{
    return (fd->inode && fd->inode->device->removable);
}

/* set or clear the fd signaled state */
void set_fd_signaled( struct fd *fd, int signaled )
{
    if (fd->comp_flags & FILE_SKIP_SET_EVENT_ON_HANDLE) return;
    fd->signaled = signaled;
    if (signaled) wake_up( &fd->obj, 0 );
}

/* check if events are pending and if yes return which one(s) */
int check_fd_events( struct fd *fd, int events )
{
    struct pollfd pfd;

    if (fd->unix_fd == -1) return POLLERR;
    if (fd->inode) return events;  /* regular files are always signaled */

    pfd.fd     = fd->unix_fd;
    pfd.events = events;
    if (poll( &pfd, 1, 0 ) <= 0) return 0;
    return pfd.revents;
}

static int fd_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct fd *fd = (struct fd *)obj;
    assert( obj->ops == &fd_ops );
    return fd->signaled;
}

/* default get_sync() routine for objects that poll() on an fd */
struct object *default_fd_get_sync( struct object *obj )
{
    struct fd *fd = get_obj_fd( obj );
    struct object *sync = get_obj_sync( &fd->obj );
    release_object( fd );
    return sync;
}

/* default get_full_name() routine for objects with an fd */
WCHAR *default_fd_get_full_name( struct object *obj, data_size_t max, data_size_t *ret_len )
{
    struct fd *fd = get_obj_fd( obj );
    WCHAR *ret = NULL;

    if (fd->nt_name)
    {
        *ret_len = fd->nt_namelen;
        ret = memdup( fd->nt_name, fd->nt_namelen );
    }
    release_object( fd );
    if (*ret_len > max) set_error( STATUS_BUFFER_OVERFLOW );
    return ret;
}

int default_fd_get_poll_events( struct fd *fd )
{
    int events = 0;

    if (async_waiting( &fd->read_q )) events |= POLLIN;
    if (async_waiting( &fd->write_q )) events |= POLLOUT;
    return events;
}

/* default handler for poll() events */
void default_poll_event( struct fd *fd, int event )
{
    if (event & (POLLIN | POLLERR | POLLHUP)) async_wake_up( &fd->read_q, STATUS_ALERTED );
    if (event & (POLLOUT | POLLERR | POLLHUP)) async_wake_up( &fd->write_q, STATUS_ALERTED );

    /* if an error occurred, stop polling this fd to avoid busy-looping */
    if (event & (POLLERR | POLLHUP)) set_fd_events( fd, -1 );
    else if (!fd->inode) set_fd_events( fd, fd->fd_ops->get_poll_events( fd ) );
}

void fd_queue_async( struct fd *fd, struct async *async, int type )
{
    struct async_queue *queue;

    switch (type)
    {
    case ASYNC_TYPE_READ:
        queue = &fd->read_q;
        break;
    case ASYNC_TYPE_WRITE:
        queue = &fd->write_q;
        break;
    case ASYNC_TYPE_WAIT:
        queue = &fd->wait_q;
        break;
    default:
        queue = NULL;
        assert(0);
    }

    queue_async( queue, async );

    if (type != ASYNC_TYPE_WAIT)
    {
        if (!fd->inode)
            set_fd_events( fd, fd->fd_ops->get_poll_events( fd ) );
        else  /* regular files are always ready for read and write */
            async_wake_up( queue, STATUS_ALERTED );
    }
}

void fd_async_wake_up( struct fd *fd, int type, unsigned int status )
{
    switch (type)
    {
    case ASYNC_TYPE_READ:
        async_wake_up( &fd->read_q, status );
        break;
    case ASYNC_TYPE_WRITE:
        async_wake_up( &fd->write_q, status );
        break;
    case ASYNC_TYPE_WAIT:
        async_wake_up( &fd->wait_q, status );
        break;
    default:
        assert(0);
    }
}

void fd_cancel_async( struct fd *fd, struct async *async )
{
    fd->fd_ops->cancel_async( fd, async );
}

void fd_reselect_async( struct fd *fd, struct async_queue *queue )
{
    fd->fd_ops->reselect_async( fd, queue );
}

void no_fd_queue_async( struct fd *fd, struct async *async, int type, int count )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

void default_fd_cancel_async( struct fd *fd, struct async *async )
{
    async_terminate( async, STATUS_CANCELLED );
}

void default_fd_queue_async( struct fd *fd, struct async *async, int type, int count )
{
    fd_queue_async( fd, async, type );
    set_error( STATUS_PENDING );
}

/* default reselect_async() fd routine */
void default_fd_reselect_async( struct fd *fd, struct async_queue *queue )
{
    if (queue == &fd->read_q || queue == &fd->write_q)
    {
        int poll_events = fd->fd_ops->get_poll_events( fd );
        int events = check_fd_events( fd, poll_events );
        if (events) fd->fd_ops->poll_event( fd, events );
        else set_fd_events( fd, poll_events );
    }
}

static inline int is_valid_mounted_device( struct stat *st )
{
#if defined(linux) || defined(__sun__)
    return S_ISBLK( st->st_mode );
#else
    /* disks are char devices on *BSD */
    return S_ISCHR( st->st_mode );
#endif
}

/* close all Unix file descriptors on a device to allow unmounting it */
static void unmount_device( struct fd *device_fd )
{
    unsigned int i;
    struct stat st;
    struct device *device;
    struct inode *inode;
    struct fd *fd;
    int unix_fd = get_unix_fd( device_fd );

    if (unix_fd == -1) return;

    if (fstat( unix_fd, &st ) == -1 || !is_valid_mounted_device( &st ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!(device = get_device( st.st_rdev, -1 ))) return;

    for (i = 0; i < INODE_HASH_SIZE; i++)
    {
        LIST_FOR_EACH_ENTRY( inode, &device->inode_hash[i], struct inode, entry )
        {
            LIST_FOR_EACH_ENTRY( fd, &inode->open, struct fd, inode_entry )
            {
                unmount_fd( fd );
            }
            inode_close_pending( inode, 0 );
        }
    }
    /* remove it from the hash table */
    list_remove( &device->entry );
    list_init( &device->entry );
    release_object( device );
}

/* default read() routine */
void no_fd_read( struct fd *fd, struct async *async, file_pos_t pos )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default write() routine */
void no_fd_write( struct fd *fd, struct async *async, file_pos_t pos )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default flush() routine */
void no_fd_flush( struct fd *fd, struct async *async )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default get_file_info() routine */
void no_fd_get_file_info( struct fd *fd, obj_handle_t handle, unsigned int info_class )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default get_file_info() routine */
void default_fd_get_file_info( struct fd *fd, obj_handle_t handle, unsigned int info_class )
{
    switch (info_class)
    {
    case FileAccessInformation:
        {
            FILE_ACCESS_INFORMATION info;
            if (get_reply_max_size() < sizeof(info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }
            info.AccessFlags = get_handle_access( current->process, handle );
            set_reply_data( &info, sizeof(info) );
            break;
        }
    case FileModeInformation:
        {
            FILE_MODE_INFORMATION info;
            if (get_reply_max_size() < sizeof(info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }
            info.Mode = fd->options & ( FILE_WRITE_THROUGH
                                      | FILE_SEQUENTIAL_ONLY
                                      | FILE_NO_INTERMEDIATE_BUFFERING
                                      | FILE_SYNCHRONOUS_IO_ALERT
                                      | FILE_SYNCHRONOUS_IO_NONALERT );
            set_reply_data( &info, sizeof(info) );
            break;
        }
    case FileIoCompletionNotificationInformation:
        {
            FILE_IO_COMPLETION_NOTIFICATION_INFORMATION info;
            if (get_reply_max_size() < sizeof(info))
            {
                set_error( STATUS_INFO_LENGTH_MISMATCH );
                return;
            }
            info.Flags = fd->comp_flags;
            set_reply_data( &info, sizeof(info) );
            break;
        }
    default:
        set_error( STATUS_NOT_IMPLEMENTED );
    }
}

/* default get_volume_info() routine */
void no_fd_get_volume_info( struct fd *fd, struct async *async, unsigned int info_class )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default ioctl() routine */
void no_fd_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default ioctl() routine */
void default_fd_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    switch(code)
    {
    case FSCTL_DISMOUNT_VOLUME:
        unmount_device( fd );
        break;

    default:
        set_error( STATUS_NOT_SUPPORTED );
    }
}

/* same as get_handle_obj but retrieve the struct fd associated to the object */
static struct fd *get_handle_fd_obj( struct process *process, obj_handle_t handle,
                                     unsigned int access )
{
    struct fd *fd = NULL;
    struct object *obj;

    if ((obj = get_handle_obj( process, handle, access, NULL )))
    {
        fd = get_obj_fd( obj );
        release_object( obj );
    }
    return fd;
}

static int is_dir_empty( int fd )
{
    DIR *dir;
    int empty;
    struct dirent *de;

    if ((fd = dup( fd )) == -1)
        return -1;

    if (!(dir = fdopendir( fd )))
    {
        close( fd );
        return -1;
    }

    empty = 1;
    while (empty && (de = readdir( dir )))
    {
        if (!strcmp( de->d_name, "." ) || !strcmp( de->d_name, ".." )) continue;
        empty = 0;
    }
    closedir( dir );
    return empty;
}

/* set disposition for the fd */
static void set_fd_disposition( struct fd *fd, unsigned int flags )
{
    struct stat st;

    if (!fd->inode)
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return;
    }

    if (fd->unix_fd == -1)
    {
        set_error( fd->no_fd_status );
        return;
    }

    if (flags & FILE_DISPOSITION_DELETE)
    {
        struct fd *fd_ptr;

        LIST_FOR_EACH_ENTRY( fd_ptr, &fd->inode->open, struct fd, inode_entry )
        {
            if (fd_ptr->access & FILE_MAPPING_ACCESS)
            {
                set_error( STATUS_CANNOT_DELETE );
                return;
            }
        }

        if (fstat( fd->unix_fd, &st ) == -1)
        {
            file_set_error();
            return;
        }
        if (S_ISREG( st.st_mode ))  /* can't unlink files we don't have permission to write */
        {
            if (!(flags & FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE) &&
                !(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
            {
                set_error( STATUS_CANNOT_DELETE );
                return;
            }
        }
        else if (S_ISDIR( st.st_mode ))  /* can't remove non-empty directories */
        {
            switch (is_dir_empty( fd->unix_fd ))
            {
            case -1:
                file_set_error();
                return;
            case 0:
                set_error( STATUS_DIRECTORY_NOT_EMPTY );
                return;
            }
        }
        else  /* can't unlink special files */
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }

    if (flags & FILE_DISPOSITION_ON_CLOSE)
        fd->options &= ~FILE_DELETE_ON_CLOSE;

    fd->closed->disp_flags =
        (flags & (FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS)) |
        ((fd->options & FILE_DELETE_ON_CLOSE) ? FILE_DISPOSITION_DELETE : 0);
}

/* set new name for the fd */
static void set_fd_name( struct fd *fd, struct fd *root, const char *nameptr, data_size_t len,
                         struct unicode_str nt_name, int create_link, unsigned int flags )
{
    struct inode *inode;
    struct stat st, st2;
    char *name;
    const unsigned int replace = flags & FILE_RENAME_REPLACE_IF_EXISTS;

    if (!fd->inode || !fd->unix_name)
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return;
    }
    if (fd->unix_fd == -1)
    {
        set_error( fd->no_fd_status );
        return;
    }

    if (!len || ((nameptr[0] == '/') ^ !root))
    {
        set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
        return;
    }
    if (!(name = mem_alloc( len + 1 ))) return;
    memcpy( name, nameptr, len );
    name[len] = 0;

    if (root)
    {
        char *combined_name = dup_fd_name( root, name );
        if (!combined_name)
        {
            set_error( STATUS_NO_MEMORY );
            goto failed;
        }
        free( name );
        name = combined_name;
    }

    /* when creating a hard link, source cannot be a dir */
    if (create_link && !fstat( fd->unix_fd, &st ) && S_ISDIR( st.st_mode ))
    {
        set_error( STATUS_FILE_IS_A_DIRECTORY );
        goto failed;
    }

    if (!stat( name, &st ))
    {
        if (!fstat( fd->unix_fd, &st2 ) && st.st_ino == st2.st_ino && st.st_dev == st2.st_dev)
        {
            if (create_link && !replace) set_error( STATUS_OBJECT_NAME_COLLISION );
            free( name );
            return;
        }

        if (!replace)
        {
            set_error( STATUS_OBJECT_NAME_COLLISION );
            goto failed;
        }

        /* can't replace directories or special files */
        if (!S_ISREG( st.st_mode ))
        {
            set_error( STATUS_ACCESS_DENIED );
            goto failed;
        }

        /* read-only files cannot be replaced */
        if (!(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) &&
            !(flags & FILE_RENAME_IGNORE_READONLY_ATTRIBUTE))
        {
            set_error( STATUS_ACCESS_DENIED );
            goto failed;
        }

        /* can't replace an opened file */
        if ((inode = get_inode( st.st_dev, st.st_ino, -1 )))
        {
            int is_empty = list_empty( &inode->open );
            release_object( inode );
            if (!is_empty)
            {
                set_error( STATUS_ACCESS_DENIED );
                goto failed;
            }
        }

        /* link() expects that the target doesn't exist */
        /* rename() cannot replace files with directories */
        if (create_link || S_ISDIR( st2.st_mode ))
        {
            if (unlink( name ))
            {
                file_set_error();
                goto failed;
            }
        }
    }

    if (create_link)
    {
        if (link( fd->unix_name, name ))
            file_set_error();
        free( name );
        return;
    }

    if (rename( fd->unix_name, name ))
    {
        file_set_error();
        goto failed;
    }

    if (is_file_executable( fd->unix_name ) != is_file_executable( name ) && !fstat( fd->unix_fd, &st ))
    {
        if (is_file_executable( name ))
            /* set executable bit where read bit is set */
            st.st_mode |= (st.st_mode & 0444) >> 2;
        else
            st.st_mode &= ~0111;
        fchmod( fd->unix_fd, st.st_mode );
    }

    free( fd->nt_name );
    fd->nt_name = dup_nt_name( root, nt_name, &fd->nt_namelen );
    free( fd->unix_name );
    fd->closed->unix_name = fd->unix_name = realpath( name, NULL );
    free( name );
    if (!fd->unix_name)
        set_error( STATUS_NO_MEMORY );
    return;

failed:
    free( name );
}

static void set_fd_eof( struct fd *fd, file_pos_t eof )
{
    struct stat st;

    if (!fd->inode)
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return;
    }

    if (fd->unix_fd == -1)
    {
        set_error( fd->no_fd_status );
        return;
    }
    if (fstat( fd->unix_fd, &st) == -1)
    {
        file_set_error();
        return;
    }
    if (eof < st.st_size)
    {
        struct fd *fd_ptr;
        LIST_FOR_EACH_ENTRY( fd_ptr, &fd->inode->open, struct fd, inode_entry )
        {
            if (fd_ptr->access & FILE_MAPPING_ACCESS)
            {
                set_error( STATUS_USER_MAPPED_FILE );
                return;
            }
        }
        if (ftruncate( fd->unix_fd, eof ) == -1) file_set_error();
    }
    else grow_file( fd->unix_fd, eof );
}

struct completion *fd_get_completion( struct fd *fd, apc_param_t *p_key )
{
    *p_key = fd->comp_key;
    return fd->completion ? (struct completion *)grab_object( fd->completion ) : NULL;
}

void fd_copy_completion( struct fd *src, struct fd *dst )
{
    assert( !dst->completion );
    dst->completion = fd_get_completion( src, &dst->comp_key );
    dst->comp_flags = src->comp_flags;
}

/* flush a file buffers */
DECL_HANDLER(flush)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->async.handle, 0 );
    struct async *async;

    if (!fd) return;

    if ((async = create_request_async( fd, fd->comp_flags, &req->async, 0 )))
    {
        fd->fd_ops->flush( fd, async );
        reply->event = async_handoff( async, NULL, 1 );
        release_object( async );
    }
    release_object( fd );
}

/* query file info */
DECL_HANDLER(get_file_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    if (fd)
    {
        fd->fd_ops->get_file_info( fd, req->handle, req->info_class );
        release_object( fd );
    }
}

/* query volume info */
DECL_HANDLER(get_volume_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );
    struct async *async;

    if (!fd) return;

    if ((async = create_request_async( fd, fd->comp_flags, &req->async, 0 )))
    {
        fd->fd_ops->get_volume_info( fd, async, req->info_class );
        reply->wait = async_handoff( async, NULL, 1 );
        release_object( async );
    }
    release_object( fd );
}

/* open a file object */
DECL_HANDLER(open_file_object)
{
    struct unicode_str name = get_req_unicode_str();
    struct object *obj, *result, *root = NULL;

    if (req->rootdir && !(root = get_handle_obj( current->process, req->rootdir, 0, NULL ))) return;

    obj = open_named_object( root, NULL, &name, req->attributes );
    if (root) release_object( root );
    if (!obj) return;

    if ((result = obj->ops->open_file( obj, req->access, req->sharing, req->options )))
    {
        reply->handle = alloc_handle( current->process, result, req->access, req->attributes );
        release_object( result );
    }
    release_object( obj );
}

/* get the Unix name from a file handle */
DECL_HANDLER(get_handle_unix_name)
{
    struct fd *fd;

    if ((fd = get_handle_fd_obj( current->process, req->handle, 0 )))
    {
        if (fd->unix_name)
        {
            data_size_t name_len = strlen( fd->unix_name );
            reply->name_len = name_len;
            if (name_len <= get_reply_max_size()) set_reply_data( fd->unix_name, name_len );
            else set_error( STATUS_BUFFER_OVERFLOW );
        }
        else set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( fd );
    }
}

/* get a Unix fd to access a file */
DECL_HANDLER(get_handle_fd)
{
    struct fd *fd;

    if ((fd = get_handle_fd_obj( current->process, req->handle, 0 )))
    {
        int unix_fd = get_unix_fd( fd );
        reply->cacheable = fd->cacheable;
        if (unix_fd != -1)
        {
            reply->type = fd->fd_ops->get_fd_type( fd );
            reply->options = fd->options;
            reply->access = get_handle_access( current->process, req->handle );
            send_client_fd( current->process, unix_fd, req->handle );
        }
        release_object( fd );
    }
}

/* perform a read on a file object */
DECL_HANDLER(read)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->async.handle, FILE_READ_DATA );
    struct async *async;

    if (!fd) return;

    if ((async = create_request_async( fd, fd->comp_flags, &req->async, 0 )))
    {
        fd->fd_ops->read( fd, async, req->pos );
        reply->wait = async_handoff( async, NULL, 0 );
        reply->options = fd->options;
        release_object( async );
    }
    release_object( fd );
}

/* perform a write on a file object */
DECL_HANDLER(write)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->async.handle, FILE_WRITE_DATA );
    struct async *async;

    if (!fd) return;

    if ((async = create_request_async( fd, fd->comp_flags, &req->async, 0 )))
    {
        fd->fd_ops->write( fd, async, req->pos );
        reply->wait = async_handoff( async, &reply->size, 0 );
        reply->options = fd->options;
        release_object( async );
    }
    release_object( fd );
}

/* perform an ioctl on a file */
DECL_HANDLER(ioctl)
{
    unsigned int access = (req->code >> 14) & (FILE_READ_DATA|FILE_WRITE_DATA);
    struct fd *fd = get_handle_fd_obj( current->process, req->async.handle, access );
    struct async *async;

    if (!fd) return;

    if ((async = create_request_async( fd, fd->comp_flags, &req->async, 0 )))
    {
        fd->fd_ops->ioctl( fd, req->code, async );
        reply->wait = async_handoff( async, NULL, 0 );
        reply->options = fd->options;
        release_object( async );
    }
    release_object( fd );
}

/* create / reschedule an async I/O */
DECL_HANDLER(register_async)
{
    unsigned int access;
    struct async *async;
    struct fd *fd;

    switch(req->type)
    {
    case ASYNC_TYPE_READ:
        access = FILE_READ_DATA;
        break;
    case ASYNC_TYPE_WRITE:
        access = FILE_WRITE_DATA;
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if ((fd = get_handle_fd_obj( current->process, req->async.handle, access )))
    {
        if (get_unix_fd( fd ) != -1 && (async = create_async( fd, current, &req->async, NULL )))
        {
            fd->fd_ops->queue_async( fd, async, req->type, req->count );
            release_object( async );
        }
        release_object( fd );
    }
}

/* attach completion object to a fd */
DECL_HANDLER(set_completion_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    if (fd)
    {
        if (is_fd_overlapped( fd ) && !fd->completion)
        {
            fd->completion = get_completion_obj( current->process, req->chandle, IO_COMPLETION_MODIFY_STATE );
            fd->comp_key = req->ckey;
            set_fd_signaled( fd, 1 );
        }
        else set_error( STATUS_INVALID_PARAMETER );
        release_object( fd );
    }
}

/* push new completion msg into a completion queue attached to the fd */
DECL_HANDLER(add_fd_completion)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );
    if (fd)
    {
        if (fd->completion && (req->async || !(fd->comp_flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)))
            add_completion( fd->completion, fd->comp_key, req->cvalue, req->status, req->information );
        release_object( fd );
    }
}

/* set fd completion information */
DECL_HANDLER(set_fd_completion_mode)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );
    if (fd)
    {
        if (is_fd_overlapped( fd ))
        {
            if (req->flags & FILE_SKIP_SET_EVENT_ON_HANDLE)
                set_fd_signaled( fd, 0 );
            /* removing flags is not allowed */
            fd->comp_flags |= req->flags & ( FILE_SKIP_COMPLETION_PORT_ON_SUCCESS
                                           | FILE_SKIP_SET_EVENT_ON_HANDLE
                                           | FILE_SKIP_SET_USER_EVENT_ON_FAST_IO );
        }
        else
            set_error( STATUS_INVALID_PARAMETER );
        release_object( fd );
    }
}

/* set fd disposition information */
DECL_HANDLER(set_fd_disp_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, DELETE );
    if (fd)
    {
        set_fd_disposition( fd, req->flags );
        release_object( fd );
    }
}

/* set fd name information */
DECL_HANDLER(set_fd_name_info)
{
    struct fd *fd, *root_fd = NULL;
    struct unicode_str nt_name;

    if (req->namelen > get_req_data_size())
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    nt_name.str = get_req_data();
    nt_name.len = (req->namelen / sizeof(WCHAR)) * sizeof(WCHAR);

    if (req->rootdir)
    {
        struct dir *root;

        if (!(root = get_dir_obj( current->process, req->rootdir, 0 ))) return;
        root_fd = get_obj_fd( (struct object *)root );
        release_object( root );
        if (!root_fd) return;
    }

    if ((fd = get_handle_fd_obj( current->process, req->handle, 0 )))
    {
        set_fd_name( fd, root_fd, (const char *)get_req_data() + req->namelen,
                     get_req_data_size() - req->namelen, nt_name, req->link, req->flags );
        release_object( fd );
    }
    if (root_fd) release_object( root_fd );
}

/* set fd eof information */
DECL_HANDLER(set_fd_eof_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );
    if (fd)
    {
        set_fd_eof( fd, req->eof );
        release_object( fd );
    }
}

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
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
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
#ifdef HAVE_SYS_EVENT_H
#include <sys/event.h>
#undef LIST_INIT
#undef LIST_ENTRY
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "object.h"
#include "file.h"
#include "handle.h"
#include "process.h"
#include "request.h"

#include "winternl.h"

#if defined(HAVE_SYS_EPOLL_H) && defined(HAVE_EPOLL_CREATE)
# include <sys/epoll.h>
# define USE_EPOLL
#elif defined(linux) && defined(__i386__) && defined(HAVE_STDINT_H)
# define USE_EPOLL
# define EPOLLIN POLLIN
# define EPOLLOUT POLLOUT
# define EPOLLERR POLLERR
# define EPOLLHUP POLLHUP
# define EPOLL_CTL_ADD 1
# define EPOLL_CTL_DEL 2
# define EPOLL_CTL_MOD 3

typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;
  epoll_data_t data;
};

#define SYSCALL_RET(ret) do { \
        if (ret < 0) { errno = -ret; ret = -1; } \
        return ret; \
    } while(0)

static inline int epoll_create( int size )
{
    int ret;
    __asm__( "pushl %%ebx; movl %2,%%ebx; int $0x80; popl %%ebx"
             : "=a" (ret) : "0" (254 /*NR_epoll_create*/), "r" (size) );
    SYSCALL_RET(ret);
}

static inline int epoll_ctl( int epfd, int op, int fd, const struct epoll_event *event )
{
    int ret;
    __asm__( "pushl %%ebx; movl %2,%%ebx; int $0x80; popl %%ebx"
             : "=a" (ret)
             : "0" (255 /*NR_epoll_ctl*/), "r" (epfd), "c" (op), "d" (fd), "S" (event), "m" (*event) );
    SYSCALL_RET(ret);
}

static inline int epoll_wait( int epfd, struct epoll_event *events, int maxevents, int timeout )
{
    int ret;
    __asm__( "pushl %%ebx; movl %2,%%ebx; int $0x80; popl %%ebx"
             : "=a" (ret)
             : "0" (256 /*NR_epoll_wait*/), "r" (epfd), "c" (events), "d" (maxevents), "S" (timeout)
             : "memory" );
    SYSCALL_RET(ret);
}
#undef SYSCALL_RET

#endif /* linux && __i386__ && HAVE_STDINT_H */


/* Because of the stupid Posix locking semantics, we need to keep
 * track of all file descriptors referencing a given file, and not
 * close a single one until all the locks are gone (sigh).
 */

/* file descriptor object */

/* closed_fd is used to keep track of the unix fd belonging to a closed fd object */
struct closed_fd
{
    struct list entry;       /* entry in inode closed list */
    int         unix_fd;     /* the unix file descriptor */
    char        unlink[1];   /* name to unlink on close (if any) */
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
    unsigned int         access;      /* file access (FILE_READ_DATA etc.) */
    unsigned int         sharing;     /* file sharing mode */
    int                  unix_fd;     /* unix file descriptor */
    int                  fs_locks :1; /* can we use filesystem locks for this fd? */
    int                  unmounted :1;/* has the device been unmounted? */
    int                  poll_index;  /* index of fd in poll array */
    struct list          read_q;      /* async readers of this fd */
    struct list          write_q;     /* async writers of this fd */
};

static void fd_dump( struct object *obj, int verbose );
static void fd_destroy( struct object *obj );

static const struct object_ops fd_ops =
{
    sizeof(struct fd),        /* size */
    fd_dump,                  /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    no_map_access,            /* map_access */
    no_lookup_name,           /* lookup_name */
    no_open_file,             /* open_file */
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
    device_dump,              /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    no_map_access,            /* map_access */
    no_lookup_name,           /* lookup_name */
    no_open_file,             /* open_file */
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
    inode_dump,               /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    no_map_access,            /* map_access */
    no_lookup_name,           /* lookup_name */
    no_open_file,             /* open_file */
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
static int file_lock_signaled( struct object *obj, struct thread *thread );

static const struct object_ops file_lock_ops =
{
    sizeof(struct file_lock),   /* size */
    file_lock_dump,             /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    file_lock_signaled,         /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    no_map_access,              /* map_access */
    no_lookup_name,             /* lookup_name */
    no_open_file,               /* open_file */
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
    struct timeval        when;       /* timeout expiry (absolute time) */
    timeout_callback      callback;   /* callback function */
    void                 *private;    /* callback private data */
};

static struct list timeout_list = LIST_INIT(timeout_list);   /* sorted timeouts list */
struct timeval current_time;

/* add a timeout user */
struct timeout_user *add_timeout_user( const struct timeval *when, timeout_callback func,
                                       void *private )
{
    struct timeout_user *user;
    struct list *ptr;

    if (!(user = mem_alloc( sizeof(*user) ))) return NULL;
    user->when     = *when;
    user->callback = func;
    user->private  = private;

    /* Now insert it in the linked list */

    LIST_FOR_EACH( ptr, &timeout_list )
    {
        struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
        if (!time_before( &timeout->when, when )) break;
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

/* add a timeout in milliseconds to an absolute time */
void add_timeout( struct timeval *when, int timeout )
{
    if (timeout)
    {
        long sec = timeout / 1000;
        if ((when->tv_usec += (timeout - 1000*sec) * 1000) >= 1000000)
        {
            when->tv_usec -= 1000000;
            when->tv_sec++;
        }
        when->tv_sec += sec;
    }
}


/****************************************************************/
/* poll support */

static struct fd **poll_users;              /* users array */
static struct pollfd *pollfd;               /* poll fd array */
static int nb_users;                        /* count of array entries actually in use */
static int active_users;                    /* current number of active users */
static int allocated_users;                 /* count of allocated entries in the array */
static struct fd **freelist;                /* list of free entries in the array */

static int get_next_timeout(void);

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
        if (pollfd[user].events) return;  /* stopped waiting on it, don't restart */
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
    struct epoll_event events[128];

    assert( POLLIN == EPOLLIN );
    assert( POLLOUT == EPOLLOUT );
    assert( POLLERR == EPOLLERR );
    assert( POLLHUP == EPOLLHUP );

    if (epoll_fd == -1) return;

    while (active_users)
    {
        timeout = get_next_timeout();

        if (!active_users) break;  /* last user removed by a timeout */
        if (epoll_fd == -1) break;  /* an error occurred with epoll */

        ret = epoll_wait( epoll_fd, events, sizeof(events)/sizeof(events[0]), timeout );
        gettimeofday( &current_time, NULL );

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
#ifndef __APPLE__ /* kqueue support is broken in the MacOS kernel so we can't use it */
    kqueue_fd = kqueue();
#endif
}

static inline void set_fd_epoll_events( struct fd *fd, int user, int events )
{
    struct kevent ev[2];

    if (kqueue_fd == -1) return;

    EV_SET( &ev[0], fd->unix_fd, EVFILT_READ, 0, NOTE_LOWAT, 1, (void *)user );
    EV_SET( &ev[1], fd->unix_fd, EVFILT_WRITE, 0, NOTE_LOWAT, 1, (void *)user );

    if (events == -1)  /* stop waiting on this fd completely */
    {
        if (pollfd[user].fd == -1) return;  /* already removed */
        ev[0].flags |= EV_DELETE;
        ev[1].flags |= EV_DELETE;
    }
    else if (pollfd[user].fd == -1)
    {
        if (pollfd[user].events) return;  /* stopped waiting on it, don't restart */
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
    struct kevent events[128];

    if (kqueue_fd == -1) return;

    while (active_users)
    {
        timeout = get_next_timeout();

        if (!active_users) break;  /* last user removed by a timeout */
        if (kqueue_fd == -1) break;  /* an error occurred with kqueue */

        if (timeout != -1)
        {
            struct timespec ts;

            ts.tv_sec = timeout / 1000;
            ts.tv_nsec = (timeout % 1000) * 1000000;
            ret = kevent( kqueue_fd, NULL, 0, events, sizeof(events)/sizeof(events[0]), &ts );
        }
        else ret = kevent( kqueue_fd, NULL, 0, events, sizeof(events)/sizeof(events[0]), NULL );

        gettimeofday( &current_time, NULL );

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

/* process pending timeouts and return the time until the next timeout, in milliseconds */
static int get_next_timeout(void)
{
    if (!list_empty( &timeout_list ))
    {
        struct list expired_list, *ptr;

        /* first remove all expired timers from the list */

        list_init( &expired_list );
        while ((ptr = list_head( &timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );

            if (!time_before( &current_time, &timeout->when ))
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

        if ((ptr = list_head( &timeout_list )) != NULL)
        {
            struct timeout_user *timeout = LIST_ENTRY( ptr, struct timeout_user, entry );
            int diff = (timeout->when.tv_sec - current_time.tv_sec) * 1000
                     + (timeout->when.tv_usec - current_time.tv_usec + 999) / 1000;
            if (diff < 0) diff = 0;
            return diff;
        }
    }
    return -1;  /* no pending timeouts */
}

/* server main poll() loop */
void main_loop(void)
{
    int i, ret, timeout;

    gettimeofday( &current_time, NULL );

    main_loop_epoll();
    /* fall through to normal poll loop */

    while (active_users)
    {
        timeout = get_next_timeout();

        if (!active_users) break;  /* last user removed by a timeout */

        ret = poll( pollfd, nb_users, timeout );
        gettimeofday( &current_time, NULL );

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
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
    struct statfs stfs;

    if (fstatfs( unix_fd, &stfs ) == -1) return 0;
    return (!strncmp("cd9660", stfs.f_fstypename, sizeof(stfs.f_fstypename)) ||
            !strncmp("udf", stfs.f_fstypename, sizeof(stfs.f_fstypename)));
#elif defined(__NetBSD__)
    struct statvfs stfs;

    if (fstatvfs( unix_fd, &stfs ) == -1) return 0;
    return (!strncmp("cd9660", stfs.f_fstypename, sizeof(stfs.f_fstypename)) ||
            !strncmp("udf", stfs.f_fstypename, sizeof(stfs.f_fstypename)));
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
        if (!keep_unlinks || !fd->unlink[0])  /* get rid of it unless there's an unlink pending on that file */
        {
            list_remove( ptr );
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
        if (fd->unlink[0])
        {
            /* make sure it is still the same file */
            struct stat st;
            if (!stat( fd->unlink, &st ) && st.st_dev == inode->device->dev && st.st_ino == inode->ino)
            {
                if (S_ISDIR(st.st_mode)) rmdir( fd->unlink );
                else unlink( fd->unlink );
            }
        }
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

/* add fd to the inode list of file descriptors to close */
static void inode_add_closed_fd( struct inode *inode, struct closed_fd *fd )
{
    if (!list_empty( &inode->locks ))
    {
        list_add_head( &inode->closed, &fd->entry );
    }
    else if (fd->unlink[0])  /* close the fd but keep the structure around for unlink */
    {
        if (fd->unix_fd != -1) close( fd->unix_fd );
        fd->unix_fd = -1;
        list_add_head( &inode->closed, &fd->entry );
    }
    else  /* no locks on this inode and no unlink, get rid of the fd */
    {
        if (fd->unix_fd != -1) close( fd->unix_fd );
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

static int file_lock_signaled( struct object *obj, struct thread *thread )
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
            /* no locking on this fs, just ignore it */
            fd->fs_locks = 0;
            return 1;
        case EAGAIN:
            set_error( STATUS_FILE_LOCK_CONFLICT );
            return 0;
        case EBADF:
            /* this can happen if we try to set a write lock on a read-only file */
            /* we just ignore that error */
            if (fl.l_type == F_WRLCK) return 1;
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

    if (!fd->inode)  /* not a regular file */
    {
        set_error( STATUS_INVALID_HANDLE );
        return NULL;
    }

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
    list_add_head( &fd->locks, &lock->fd_entry );
    list_add_head( &fd->inode->locks, &lock->inode_entry );
    list_add_head( &lock->process->locks, &lock->proc_entry );
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
        if (lock->shared && shared) continue;
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
    fprintf( stderr, "Fd unix_fd=%d user=%p", fd->unix_fd, fd->user );
    if (fd->inode) fprintf( stderr, " inode=%p unlink='%s'", fd->inode, fd->closed->unlink );
    fprintf( stderr, "\n" );
}

static void fd_destroy( struct object *obj )
{
    struct fd *fd = (struct fd *)obj;

    async_terminate_queue( &fd->read_q, STATUS_CANCELLED );
    async_terminate_queue( &fd->write_q, STATUS_CANCELLED );

    remove_fd_locks( fd );
    list_remove( &fd->inode_entry );
    if (fd->poll_index != -1) remove_poll_user( fd, fd->poll_index );
    if (fd->inode)
    {
        inode_add_closed_fd( fd->inode, fd->closed );
        release_object( fd->inode );
    }
    else  /* no inode, close it right away */
    {
        if (fd->unix_fd != -1) close( fd->unix_fd );
    }
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
    else if (pollfd[user].fd != -1 || !pollfd[user].events)
    {
        pollfd[user].fd = fd->unix_fd;
        pollfd[user].events = events;
    }
}

/* prepare an fd for unmounting its corresponding device */
static inline void unmount_fd( struct fd *fd )
{
    assert( fd->inode );

    async_terminate_queue( &fd->read_q, STATUS_VOLUME_DISMOUNTED );
    async_terminate_queue( &fd->write_q, STATUS_VOLUME_DISMOUNTED );

    if (fd->poll_index != -1) set_fd_events( fd, -1 );

    if (fd->unix_fd != -1) close( fd->unix_fd );

    fd->unix_fd = -1;
    fd->unmounted = 1;
    fd->closed->unix_fd = -1;
    fd->closed->unlink[0] = 0;

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
    fd->access     = 0;
    fd->sharing    = 0;
    fd->unix_fd    = -1;
    fd->fs_locks   = 1;
    fd->unmounted  = 0;
    fd->poll_index = -1;
    list_init( &fd->inode_entry );
    list_init( &fd->locks );
    list_init( &fd->read_q );
    list_init( &fd->write_q );

    if ((fd->poll_index = add_poll_user( fd )) == -1)
    {
        release_object( fd );
        return NULL;
    }
    return fd;
}

/* allocate a pseudo fd object, for objects that need to behave like files but don't have a unix fd */
struct fd *alloc_pseudo_fd( const struct fd_ops *fd_user_ops, struct object *user )
{
    struct fd *fd = alloc_object( &fd_ops );

    if (!fd) return NULL;

    fd->fd_ops     = fd_user_ops;
    fd->user       = user;
    fd->inode      = NULL;
    fd->closed     = NULL;
    fd->access     = 0;
    fd->sharing    = 0;
    fd->unix_fd    = -1;
    fd->fs_locks   = 0;
    fd->unmounted  = 0;
    fd->poll_index = -1;
    list_init( &fd->inode_entry );
    list_init( &fd->locks );
    list_init( &fd->read_q );
    list_init( &fd->write_q );
    return fd;
}

/* check if the desired access is possible without violating */
/* the sharing mode of other opens of the same file */
static int check_sharing( struct fd *fd, unsigned int access, unsigned int sharing )
{
    unsigned int existing_sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    unsigned int existing_access = 0;
    struct list *ptr;

    /* if access mode is 0, sharing mode is ignored */
    if (!access) sharing = existing_sharing;
    fd->access = access;
    fd->sharing = sharing;

    LIST_FOR_EACH( ptr, &fd->inode->open )
    {
        struct fd *fd_ptr = LIST_ENTRY( ptr, struct fd, inode_entry );
        if (fd_ptr != fd)
        {
            existing_sharing &= fd_ptr->sharing;
            existing_access  |= fd_ptr->access;
        }
    }

    if ((access & FILE_UNIX_READ_ACCESS) && !(existing_sharing & FILE_SHARE_READ)) return 0;
    if ((access & FILE_UNIX_WRITE_ACCESS) && !(existing_sharing & FILE_SHARE_WRITE)) return 0;
    if ((access & DELETE) && !(existing_sharing & FILE_SHARE_DELETE)) return 0;
    if ((existing_access & FILE_UNIX_READ_ACCESS) && !(sharing & FILE_SHARE_READ)) return 0;
    if ((existing_access & FILE_UNIX_WRITE_ACCESS) && !(sharing & FILE_SHARE_WRITE)) return 0;
    if ((existing_access & DELETE) && !(sharing & FILE_SHARE_DELETE)) return 0;
    return 1;
}

/* sets the user of an fd that previously had no user */
void set_fd_user( struct fd *fd, const struct fd_ops *user_ops, struct object *user )
{
    assert( fd->fd_ops == NULL );
    fd->fd_ops = user_ops;
    fd->user   = user;
}

/* open() wrapper that returns a struct fd with no fd user set */
struct fd *open_fd( const char *name, int flags, mode_t *mode, unsigned int access,
                    unsigned int sharing, unsigned int options )
{
    struct stat st;
    struct closed_fd *closed_fd;
    struct fd *fd;
    const char *unlink_name = "";
    int rw_mode;

    if ((options & FILE_DELETE_ON_CLOSE) && !(access & DELETE))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if (!(fd = alloc_fd_object())) return NULL;

    if (options & FILE_DELETE_ON_CLOSE) unlink_name = name;
    if (!(closed_fd = mem_alloc( sizeof(*closed_fd) + strlen(unlink_name) )))
    {
        release_object( fd );
        return NULL;
    }

    /* create the directory if needed */
    if ((options & FILE_DIRECTORY_FILE) && (flags & O_CREAT))
    {
        if (mkdir( name, 0777 ) == -1)
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
        if (errno != EISDIR ||
            !(access & FILE_UNIX_WRITE_ACCESS) ||
            (fd->unix_fd = open( name, O_RDONLY | (flags & ~O_TRUNC), *mode )) == -1)
        {
            file_set_error();
            goto error;
        }
    }

    closed_fd->unix_fd = fd->unix_fd;
    closed_fd->unlink[0] = 0;
    fstat( fd->unix_fd, &st );
    *mode = st.st_mode;

    /* only bother with an inode for normal files and directories */
    if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))
    {
        struct inode *inode = get_inode( st.st_dev, st.st_ino, fd->unix_fd );

        if (!inode)
        {
            /* we can close the fd because there are no others open on the same file,
             * otherwise we wouldn't have failed to allocate a new inode
             */
            goto error;
        }
        fd->inode = inode;
        fd->closed = closed_fd;
        list_add_head( &inode->open, &fd->inode_entry );

        /* check directory options */
        if ((options & FILE_DIRECTORY_FILE) && !S_ISDIR(st.st_mode))
        {
            release_object( fd );
            set_error( STATUS_NOT_A_DIRECTORY );
            return NULL;
        }
        if ((options & FILE_NON_DIRECTORY_FILE) && S_ISDIR(st.st_mode))
        {
            release_object( fd );
            set_error( STATUS_FILE_IS_A_DIRECTORY );
            return NULL;
        }
        if (!check_sharing( fd, access, sharing ))
        {
            release_object( fd );
            set_error( STATUS_SHARING_VIOLATION );
            return NULL;
        }
        strcpy( closed_fd->unlink, unlink_name );
        if (flags & O_TRUNC) ftruncate( fd->unix_fd, 0 );
    }
    else  /* special file */
    {
        if (options & FILE_DIRECTORY_FILE)
        {
            set_error( STATUS_NOT_A_DIRECTORY );
            goto error;
        }
        if (unlink_name[0])  /* we can't unlink special files */
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto error;
        }
        free( closed_fd );
    }
    return fd;

error:
    release_object( fd );
    free( closed_fd );
    return NULL;
}

/* create an fd for an anonymous file */
/* if the function fails the unix fd is closed */
struct fd *create_anonymous_fd( const struct fd_ops *fd_user_ops, int unix_fd, struct object *user )
{
    struct fd *fd = alloc_fd_object();

    if (fd)
    {
        set_fd_user( fd, fd_user_ops, user );
        fd->unix_fd = unix_fd;
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

/* retrieve the unix fd for an object */
int get_unix_fd( struct fd *fd )
{
    if (fd->unix_fd == -1)
    {
        if (fd->unmounted) set_error( STATUS_VOLUME_DISMOUNTED );
        else set_error( STATUS_BAD_DEVICE_TYPE );
    }
    return fd->unix_fd;
}

/* check if two file descriptors point to the same file */
int is_same_file_fd( struct fd *fd1, struct fd *fd2 )
{
    return fd1->inode == fd2->inode;
}

/* check if fd is on a removable device */
int is_fd_removable( struct fd *fd )
{
    return (fd->inode && fd->inode->device->removable);
}

/* handler for close_handle that refuses to close fd-associated handles in other processes */
int fd_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    return (!current || current->process == process);
}

/* callback for event happening in the main poll() loop */
void fd_poll_event( struct fd *fd, int event )
{
    return fd->fd_ops->poll_event( fd, event );
}

/* check if events are pending and if yes return which one(s) */
int check_fd_events( struct fd *fd, int events )
{
    struct pollfd pfd;

    if (fd->unix_fd == -1) return POLLERR;

    pfd.fd     = fd->unix_fd;
    pfd.events = events;
    if (poll( &pfd, 1, 0 ) <= 0) return 0;
    return pfd.revents;
}

/* default add_queue() routine for objects that poll() on an fd */
int default_fd_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct fd *fd = get_obj_fd( obj );

    if (!fd) return 0;
    if (!fd->inode && list_empty( &obj->wait_queue ))  /* first on the queue */
        set_fd_events( fd, fd->fd_ops->get_poll_events( fd ) );
    add_queue( obj, entry );
    release_object( fd );
    return 1;
}

/* default remove_queue() routine for objects that poll() on an fd */
void default_fd_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct fd *fd = get_obj_fd( obj );

    grab_object( obj );
    remove_queue( obj, entry );
    if (!fd->inode && list_empty( &obj->wait_queue ))  /* last on the queue is gone */
        set_fd_events( fd, 0 );
    release_object( obj );
    release_object( fd );
}

/* default signaled() routine for objects that poll() on an fd */
int default_fd_signaled( struct object *obj, struct thread *thread )
{
    int events, ret;
    struct fd *fd = get_obj_fd( obj );

    if (fd->inode) ret = 1; /* regular files are always signaled */
    else
    {
        events = fd->fd_ops->get_poll_events( fd );
        ret = check_fd_events( fd, events ) != 0;

        if (ret)
        {
            /* stop waiting on select() if we are signaled */
            set_fd_events( fd, 0 );
        }
        else if (!list_empty( &obj->wait_queue ))
        {
            /* restart waiting on poll() if we are no longer signaled */
            set_fd_events( fd, events );
        }
    }
    release_object( fd );
    return ret;
}

int default_fd_get_poll_events( struct fd *fd )
{
    int events = 0;

    if (!list_empty( &fd->read_q ))
        events |= POLLIN;
    if (!list_empty( &fd->write_q ))
        events |= POLLOUT;

    return events;
}

/* default handler for poll() events */
void default_poll_event( struct fd *fd, int event )
{
    if (!list_empty( &fd->read_q ) && (POLLIN & event) )
    {
        async_terminate_head( &fd->read_q, STATUS_ALERTED );
        return;
    }
    if (!list_empty( &fd->write_q ) && (POLLOUT & event) )
    {
        async_terminate_head( &fd->write_q, STATUS_ALERTED );
        return;
    }

    /* if an error occurred, stop polling this fd to avoid busy-looping */
    if (event & (POLLERR | POLLHUP)) set_fd_events( fd, -1 );
    wake_up( fd->user, 0 );
}

void fd_queue_async_timeout( struct fd *fd, const async_data_t *data, int type, int count,
                             const struct timeval *timeout )
{
    struct list *queue;
    int events, flags;

    fd->fd_ops->get_file_info( fd, &flags );
    if (!(flags & (FD_FLAG_OVERLAPPED|FD_FLAG_TIMEOUT)))
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }

    switch (type)
    {
    case ASYNC_TYPE_READ:
        queue = &fd->read_q;
        break;
    case ASYNC_TYPE_WRITE:
        queue = &fd->write_q;
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!create_async( current, timeout, queue, data )) return;
    set_error( STATUS_PENDING );

    /* Check if the new pending request can be served immediately */
    events = check_fd_events( fd, fd->fd_ops->get_poll_events( fd ) );
    if (events) fd->fd_ops->poll_event( fd, events );

    set_fd_events( fd, fd->fd_ops->get_poll_events( fd ) );
}

void default_fd_queue_async( struct fd *fd, const async_data_t *data, int type, int count )
{
    fd_queue_async_timeout( fd, data, type, count, NULL );
}

void default_fd_cancel_async( struct fd *fd )
{
    async_terminate_queue( &fd->read_q, STATUS_CANCELLED );
    async_terminate_queue( &fd->write_q, STATUS_CANCELLED );
}

/* default flush() routine */
int no_flush( struct fd *fd, struct event **event )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

/* default get_file_info() routine */
enum server_fd_type no_get_file_info( struct fd *fd, int *flags )
{
    *flags = 0;
    return FD_TYPE_INVALID;
}

/* default queue_async() routine */
void no_queue_async( struct fd *fd, const async_data_t *data, int type, int count)
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* default cancel_async() routine */
void no_cancel_async( struct fd *fd )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
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

/* flush a file buffers */
DECL_HANDLER(flush_file)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );
    struct event * event = NULL;

    if (fd)
    {
        fd->fd_ops->flush( fd, &event );
        if ( event )
        {
            reply->event = alloc_handle( current->process, event, SYNCHRONIZE, 0 );
        }
        release_object( fd );
    }
}

/* open a file object */
DECL_HANDLER(open_file_object)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct object *obj, *result;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((obj = open_object_dir( root, &name, req->attributes, NULL )))
    {
        if ((result = obj->ops->open_file( obj, req->access, req->sharing, req->options )))
        {
            reply->handle = alloc_handle( current->process, result, req->access, req->attributes );
            release_object( result );
        }
        release_object( obj );
    }

    if (root) release_object( root );
}

/* get a Unix fd to access a file */
DECL_HANDLER(get_handle_fd)
{
    struct fd *fd;

    if ((fd = get_handle_fd_obj( current->process, req->handle, req->access )))
    {
        reply->type = fd->fd_ops->get_file_info( fd, &reply->flags );
        if (reply->type != FD_TYPE_INVALID)
        {
            if (is_fd_removable(fd)) reply->flags |= FD_FLAG_REMOVABLE;
            if (!req->cached)
            {
                int unix_fd = get_unix_fd( fd );
                if (unix_fd != -1) send_client_fd( current->process, unix_fd, req->handle );
            }
        }
        else set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( fd );
    }
}

/* get ready to unmount a Unix device */
DECL_HANDLER(unmount_device)
{
    struct fd *fd;

    if ((fd = get_handle_fd_obj( current->process, req->handle, 0 )))
    {
        unmount_device( fd );
        release_object( fd );
    }
}

/* create / reschedule an async I/O */
DECL_HANDLER(register_async)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    /*
     * The queue_async method must do the following:
     *
     * 1. Get the async_queue for the request of given type.
     * 2. Create a new asynchronous request for the selected queue
     * 3. Carry out any operations necessary to adjust the object's poll events
     *    Usually: set_elect_events (obj, obj->ops->get_poll_events()).
     * 4. When the async request is triggered, then send back (with a proper APC)
     *    the trigger (STATUS_ALERTED) to the thread that posted the request.
     * See also the implementations in file.c, serial.c, and sock.c.
     */

    if (fd)
    {
        fd->fd_ops->queue_async( fd, &req->async, req->type, req->count );
        release_object( fd );
    }
}

/* cancels all async I/O */
DECL_HANDLER(cancel_async)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );
    if (fd)
    {
        /* Note: we don't kill the queued APC_ASYNC_IO on this thread because
         * NtCancelIoFile() will force the pending APC to be run. Since, 
         * Windows only guarantees that the current thread will have no async 
         * operation on the current fd when NtCancelIoFile returns, this shall
         * do the work.
         */
        fd->fd_ops->cancel_async( fd );
        release_object( fd );
    }        
}

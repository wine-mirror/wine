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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "object.h"
#include "file.h"
#include "handle.h"
#include "process.h"
#include "request.h"
#include "console.h"

/* Because of the stupid Posix locking semantics, we need to keep
 * track of all file descriptors referencing a given file, and not
 * close a single one until all the locks are gone (sigh).
 */

/* file descriptor object */

/* closed_fd is used to keep track of the unix fd belonging to a closed fd object */
struct closed_fd
{
    struct closed_fd *next;   /* next fd in close list */
    int               fd;     /* the unix file descriptor */
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
    int                  unix_fd;     /* unix file descriptor */
    int                  poll_index;  /* index of fd in poll array */
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
    no_get_fd,                /* get_fd */
    fd_destroy                /* destroy */
};

/* inode object */

struct inode
{
    struct object       obj;        /* object header */
    struct list         entry;      /* inode hash list entry */
    unsigned int        hash;       /* hashing code */
    dev_t               dev;        /* device number */
    ino_t               ino;        /* inode number */
    struct list         open;       /* list of open file descriptors */
    struct list         locks;      /* list of file locks */
    struct closed_fd   *closed;     /* list of file descriptors to close at destroy time */
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
    no_get_fd,                /* get_fd */
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
    no_get_fd,                  /* get_fd */
    no_destroy                  /* destroy */
};


#define OFF_T_MAX       (~((file_pos_t)1 << (8*sizeof(off_t)-1)))
#define FILE_POS_T_MAX  (~(file_pos_t)0)

static file_pos_t max_unix_offset = OFF_T_MAX;

#define DUMP_LONG_LONG(val) do { \
    if (sizeof(val) > sizeof(unsigned long) && (val) > ~0UL) \
        fprintf( stderr, "%lx%08lx", (unsigned long)((val) >> 32), (unsigned long)(val) ); \
    else \
        fprintf( stderr, "%lx", (unsigned long)(val) ); \
  } while (0)



/****************************************************************/
/* timeouts support */

struct timeout_user
{
    struct timeout_user  *next;       /* next in sorted timeout list */
    struct timeout_user  *prev;       /* prev in sorted timeout list */
    struct timeval        when;       /* timeout expiry (absolute time) */
    timeout_callback      callback;   /* callback function */
    void                 *private;    /* callback private data */
};

static struct timeout_user *timeout_head;   /* sorted timeouts list head */
static struct timeout_user *timeout_tail;   /* sorted timeouts list tail */

/* add a timeout user */
struct timeout_user *add_timeout_user( struct timeval *when, timeout_callback func, void *private )
{
    struct timeout_user *user;
    struct timeout_user *pos;

    if (!(user = mem_alloc( sizeof(*user) ))) return NULL;
    user->when     = *when;
    user->callback = func;
    user->private  = private;

    /* Now insert it in the linked list */

    for (pos = timeout_head; pos; pos = pos->next)
        if (!time_before( &pos->when, when )) break;

    if (pos)  /* insert it before 'pos' */
    {
        if ((user->prev = pos->prev)) user->prev->next = user;
        else timeout_head = user;
        user->next = pos;
        pos->prev = user;
    }
    else  /* insert it at the tail */
    {
        user->next = NULL;
        if (timeout_tail) timeout_tail->next = user;
        else timeout_head = user;
        user->prev = timeout_tail;
        timeout_tail = user;
    }
    return user;
}

/* remove a timeout user */
void remove_timeout_user( struct timeout_user *user )
{
    if (user->next) user->next->prev = user->prev;
    else timeout_tail = user->prev;
    if (user->prev) user->prev->next = user->next;
    else timeout_head = user->next;
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

/* handle the next expired timeout */
inline static void handle_timeout(void)
{
    struct timeout_user *user = timeout_head;
    timeout_head = user->next;
    if (user->next) user->next->prev = user->prev;
    else timeout_tail = user->prev;
    user->callback( user->private );
    free( user );
}


/****************************************************************/
/* poll support */

static struct fd **poll_users;              /* users array */
static struct pollfd *pollfd;               /* poll fd array */
static int nb_users;                        /* count of array entries actually in use */
static int active_users;                    /* current number of active users */
static int allocated_users;                 /* count of allocated entries in the array */
static struct fd **freelist;                /* list of free entries in the array */

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
    pollfd[user].fd = -1;
    pollfd[user].events = 0;
    pollfd[user].revents = 0;
    poll_users[user] = (struct fd *)freelist;
    freelist = &poll_users[user];
    active_users--;
}


/* server main poll() loop */
void main_loop(void)
{
    int ret;

    while (active_users)
    {
        long diff = -1;
        if (timeout_head)
        {
            struct timeval now;
            gettimeofday( &now, NULL );
            while (timeout_head)
            {
                if (!time_before( &now, &timeout_head->when )) handle_timeout();
                else
                {
                    diff = (timeout_head->when.tv_sec - now.tv_sec) * 1000
                            + (timeout_head->when.tv_usec - now.tv_usec) / 1000;
                    break;
                }
            }
            if (!active_users) break;  /* last user removed by a timeout */
        }
        ret = poll( pollfd, nb_users, diff );
        if (ret > 0)
        {
            int i;
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
/* inode functions */

#define HASH_SIZE 37

static struct list inode_hash[HASH_SIZE];

/* close all pending file descriptors in the closed list */
static void inode_close_pending( struct inode *inode )
{
    while (inode->closed)
    {
        struct closed_fd *fd = inode->closed;
        inode->closed = fd->next;
        close( fd->fd );
        free( fd );
    }
}


static void inode_dump( struct object *obj, int verbose )
{
    struct inode *inode = (struct inode *)obj;
    fprintf( stderr, "Inode dev=" );
    DUMP_LONG_LONG( inode->dev );
    fprintf( stderr, " ino=" );
    DUMP_LONG_LONG( inode->ino );
    fprintf( stderr, "\n" );
}

static void inode_destroy( struct object *obj )
{
    struct inode *inode = (struct inode *)obj;

    assert( list_empty(&inode->open) );
    assert( list_empty(&inode->locks) );

    list_remove( &inode->entry );
    inode_close_pending( inode );
}

/* retrieve the inode object for a given fd, creating it if needed */
static struct inode *get_inode( dev_t dev, ino_t ino )
{
    struct list *ptr;
    struct inode *inode;
    unsigned int hash = (dev ^ ino) % HASH_SIZE;

    if (inode_hash[hash].next)
    {
        LIST_FOR_EACH( ptr, &inode_hash[hash] )
        {
            inode = LIST_ENTRY( ptr, struct inode, entry );
            if (inode->dev == dev && inode->ino == ino)
                return (struct inode *)grab_object( inode );
        }
    }
    else list_init( &inode_hash[hash] );

    /* not found, create it */
    if ((inode = alloc_object( &inode_ops )))
    {
        inode->hash   = hash;
        inode->dev    = dev;
        inode->ino    = ino;
        inode->closed = NULL;
        list_init( &inode->open );
        list_init( &inode->locks );
        list_add_head( &inode_hash[hash], &inode->entry );
    }
    return inode;
}

/* add fd to the indoe list of file descriptors to close */
static void inode_add_closed_fd( struct inode *inode, struct closed_fd *fd )
{
    if (!list_empty( &inode->locks ))
    {
        fd->next = inode->closed;
        inode->closed = fd;
    }
    else  /* no locks on this inode, we can close the fd right away */
    {
        close( fd->fd );
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
static int set_unix_lock( const struct fd *fd, file_pos_t start, file_pos_t end, int type )
{
    struct flock fl;

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
        case EIO:
        case ENOLCK:
            /* no locking on this fs, just ignore it */
            return 1;
        case EACCES:
        case EAGAIN:
            set_error( STATUS_FILE_LOCK_CONFLICT );
            return 0;
        case EBADF:
            /* this can happen if we try to set a write lock on a read-only file */
            /* we just ignore that error */
            if (fl.l_type == F_WRLCK) return 1;
            set_error( STATUS_ACCESS_DENIED );
            return 0;
        case EOVERFLOW:
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
inline static int lock_overlaps( struct file_lock *lock, file_pos_t start, file_pos_t end )
{
    if (lock->end && start >= lock->end) return 0;
    if (end && lock->start >= end) return 0;
    return 1;
}

/* remove Unix locks for all bytes in the specified area that are no longer locked */
static void remove_unix_locks( const struct fd *fd, file_pos_t start, file_pos_t end )
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
    if (list_empty( &inode->locks )) inode_close_pending( inode );
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
    fprintf( stderr, "Fd unix_fd=%d user=%p\n", fd->unix_fd, fd->user );
}

static void fd_destroy( struct object *obj )
{
    struct fd *fd = (struct fd *)obj;

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

/* allocate an fd object, without setting the unix fd yet */
struct fd *alloc_fd( const struct fd_ops *fd_user_ops, struct object *user )
{
    struct fd *fd = alloc_object( &fd_ops );

    if (!fd) return NULL;

    fd->fd_ops     = fd_user_ops;
    fd->user       = user;
    fd->inode      = NULL;
    fd->closed     = NULL;
    fd->unix_fd    = -1;
    fd->poll_index = -1;
    list_init( &fd->inode_entry );
    list_init( &fd->locks );

    if ((fd->poll_index = add_poll_user( fd )) == -1)
    {
        release_object( fd );
        return NULL;
    }
    return fd;
}

/* open() wrapper using a struct fd */
/* the fd must have been created with alloc_fd */
/* on error the fd object is released */
struct fd *open_fd( struct fd *fd, const char *name, int flags, mode_t *mode )
{
    struct stat st;
    struct closed_fd *closed_fd;

    assert( fd->unix_fd == -1 );

    if (!(closed_fd = mem_alloc( sizeof(*closed_fd) )))
    {
        release_object( fd );
        return NULL;
    }
    if ((fd->unix_fd = open( name, flags, *mode )) == -1)
    {
        file_set_error();
        release_object( fd );
        free( closed_fd );
        return NULL;
    }
    closed_fd->fd = fd->unix_fd;
    fstat( fd->unix_fd, &st );
    *mode = st.st_mode;

    if (S_ISREG(st.st_mode))  /* only bother with an inode for normal files */
    {
        struct inode *inode = get_inode( st.st_dev, st.st_ino );

        if (!inode)
        {
            /* we can close the fd because there are no others open on the same file,
             * otherwise we wouldn't have failed to allocate a new inode
             */
            release_object( fd );
            free( closed_fd );
            return NULL;
        }
        fd->inode = inode;
        fd->closed = closed_fd;
        list_add_head( &inode->open, &fd->inode_entry );
    }
    else
    {
        free( closed_fd );
    }
    return fd;
}

/* create an fd for an anonymous file */
/* if the function fails the unix fd is closed */
struct fd *create_anonymous_fd( const struct fd_ops *fd_user_ops, int unix_fd, struct object *user )
{
    struct fd *fd = alloc_fd( fd_user_ops, user );

    if (fd)
    {
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
    return fd->unix_fd;
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
    if (!obj->head)  /* first on the queue */
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
    if (!obj->head)  /* last on the queue is gone */
        set_fd_events( fd, 0 );
    release_object( obj );
    release_object( fd );
}

/* default signaled() routine for objects that poll() on an fd */
int default_fd_signaled( struct object *obj, struct thread *thread )
{
    struct fd *fd = get_obj_fd( obj );
    int events = fd->fd_ops->get_poll_events( fd );
    int ret = check_fd_events( fd, events ) != 0;

    if (ret)
        set_fd_events( fd, 0 ); /* stop waiting on select() if we are signaled */
    else if (obj->head)
        set_fd_events( fd, events ); /* restart waiting on poll() if we are no longer signaled */

    release_object( fd );
    return ret;
}

/* default handler for poll() events */
void default_poll_event( struct fd *fd, int event )
{
    /* an error occurred, stop polling this fd to avoid busy-looping */
    if (event & (POLLERR | POLLHUP)) set_fd_events( fd, -1 );
    wake_up( fd->user, 0 );
}

/* default flush() routine */
int no_flush( struct fd *fd, struct event **event )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

/* default get_file_info() routine */
int no_get_file_info( struct fd *fd, struct get_file_info_reply *info, int *flags )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    *flags = 0;
    return FD_TYPE_INVALID;
}

/* default queue_async() routine */
void no_queue_async( struct fd *fd, void* ptr, unsigned int status, int type, int count )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
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
        if( event )
        {
            reply->event = alloc_handle( current->process, event, SYNCHRONIZE, 0 );
        }
        release_object( fd );
    }
}

/* get a Unix fd to access a file */
DECL_HANDLER(get_handle_fd)
{
    struct fd *fd;

    reply->fd = -1;
    reply->type = FD_TYPE_INVALID;

    if ((fd = get_handle_fd_obj( current->process, req->handle, req->access )))
    {
        int unix_fd = get_handle_unix_fd( current->process, req->handle, req->access );
        if (unix_fd != -1) reply->fd = unix_fd;
        else if (!get_error())
        {
            unix_fd = fd->unix_fd;
            if (unix_fd != -1) send_client_fd( current->process, unix_fd, req->handle );
        }
        reply->type = fd->fd_ops->get_file_info( fd, NULL, &reply->flags );
        release_object( fd );
    }
    else  /* check for console handle (FIXME: should be done in the client) */
    {
        struct object *obj;

        if ((obj = get_handle_obj( current->process, req->handle, req->access, NULL )))
        {
            if (is_console_object( obj )) reply->type = FD_TYPE_CONSOLE;
            release_object( obj );
        }
    }
}

/* get a file information */
DECL_HANDLER(get_file_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    if (fd)
    {
        int flags;
        fd->fd_ops->get_file_info( fd, reply, &flags );
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
 * 2. Call find_async() to look for the specific client request in the queue (=> NULL if not found).
 * 3. If status is STATUS_PENDING:
 *      a) If no async request found in step 2 (new request): call create_async() to initialize one.
 *      b) Set request's status to STATUS_PENDING.
 *      c) If the "queue" field of the async request is NULL: call async_insert() to put it into the queue.
 *    Otherwise:
 *      If the async request was found in step 2, destroy it by calling destroy_async().
 * 4. Carry out any operations necessary to adjust the object's poll events
 *    Usually: set_elect_events (obj, obj->ops->get_poll_events()).
 *
 * See also the implementations in file.c, serial.c, and sock.c.
*/

    if (fd)
    {
        fd->fd_ops->queue_async( fd, req->overlapped, req->status, req->type, req->count );
        release_object( fd );
    }
}

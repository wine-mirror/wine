/*
 * Server-side file descriptor management
 *
 * Copyright (C) 2003 Alexandre Julliard
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
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "object.h"
#include "file.h"
#include "handle.h"
#include "process.h"
#include "request.h"
#include "console.h"

struct fd
{
    struct object        obj;        /* object header */
    const struct fd_ops *fd_ops;     /* file descriptor operations */
    struct object       *user;       /* object using this file descriptor */
    int                  unix_fd;    /* unix file descriptor */
    int                  poll_index; /* index of fd in poll array */
    int                  mode;       /* file protection mode */
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


/* SIGHUP handler */
static void sighup_handler()
{
#ifdef DEBUG_OBJECTS
    dump_objects();
#endif
}

/* SIGTERM handler */
static void sigterm_handler()
{
    flush_registry();
    exit(1);
}

/* SIGINT handler */
static void sigint_handler()
{
    kill_all_processes( NULL, 1 );
    flush_registry();
    exit(1);
}

/* server main poll() loop */
void main_loop(void)
{
    int ret;
    sigset_t sigset;
    struct sigaction action;

    /* block the signals we use */
    sigemptyset( &sigset );
    sigaddset( &sigset, SIGCHLD );
    sigaddset( &sigset, SIGHUP );
    sigaddset( &sigset, SIGINT );
    sigaddset( &sigset, SIGQUIT );
    sigaddset( &sigset, SIGTERM );
    sigprocmask( SIG_BLOCK, &sigset, NULL );

    /* set the handlers */
    action.sa_mask = sigset;
    action.sa_flags = 0;
    action.sa_handler = sigchld_handler;
    sigaction( SIGCHLD, &action, NULL );
    action.sa_handler = sighup_handler;
    sigaction( SIGHUP, &action, NULL );
    action.sa_handler = sigint_handler;
    sigaction( SIGINT, &action, NULL );
    action.sa_handler = sigterm_handler;
    sigaction( SIGQUIT, &action, NULL );
    sigaction( SIGTERM, &action, NULL );

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

        sigprocmask( SIG_UNBLOCK, &sigset, NULL );

        /* Note: we assume that the signal handlers do not manipulate the pollfd array
         *       or the timeout list, otherwise there is a race here.
         */
        ret = poll( pollfd, nb_users, diff );

        sigprocmask( SIG_BLOCK, &sigset, NULL );

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
/* file descriptor functions */

static void fd_dump( struct object *obj, int verbose )
{
    struct fd *fd = (struct fd *)obj;
    fprintf( stderr, "Fd unix_fd=%d mode=%06o user=%p\n", fd->unix_fd, fd->mode, fd->user );
}

static void fd_destroy( struct object *obj )
{
    struct fd *fd = (struct fd *)obj;

    if (fd->poll_index != -1) remove_poll_user( fd, fd->poll_index );
    close( fd->unix_fd );
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

/* allocate an fd object */
/* if the function fails the unix fd is closed */
struct fd *alloc_fd( const struct fd_ops *fd_user_ops, int unix_fd, struct object *user )
{
    struct fd *fd = alloc_object( &fd_ops );

    if (!fd)
    {
        close( unix_fd );
        return NULL;
    }
    fd->fd_ops     = fd_user_ops;
    fd->user       = user;
    fd->unix_fd    = unix_fd;
    fd->poll_index = -1;
    fd->mode       = 0;

    if ((unix_fd != -1) && ((fd->poll_index = add_poll_user( fd )) == -1))
    {
        release_object( fd );
        return NULL;
    }
    return fd;
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
int no_flush( struct fd *fd )
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
        if (!(fd = get_obj_fd( obj ))) set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
    }
    return fd;
}

/* flush a file buffers */
DECL_HANDLER(flush_file)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    if (fd)
    {
        fd->fd_ops->flush( fd );
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

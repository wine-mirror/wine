/*
 * Server main select() loop
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "object.h"

/* select user fd */
struct user
{
    struct timeval           when;    /* timeout expiry (absolute time) */
    struct user             *next;    /* next in sorted timeout list */
    struct user             *prev;    /* prev in sorted timeout list */
    const struct select_ops *ops;   /* user operations list */
    int                      fd;      /* user fd */
    void                    *private; /* user private data */
};

static struct user *users[FD_SETSIZE];      /* users array */
static fd_set read_set, write_set;          /* current select sets */
static int nb_users;                        /* current number of users */
static int max_fd;                          /* max fd in use */
static struct user *timeout_head;           /* sorted timeouts list head */
static struct user *timeout_tail;           /* sorted timeouts list tail */


/* add a user */
int add_select_user( int fd, int events, const struct select_ops *ops, void *private )
{
    int flags;
    struct user *user = malloc( sizeof(*user) );
    if (!user) return -1;
    assert( !users[fd] );

    user->ops          = ops;
    user->when.tv_sec  = 0;
    user->when.tv_usec = 0;
    user->fd           = fd;
    user->private      = private;

    flags = fcntl( fd, F_GETFL, 0 );
    fcntl( fd, F_SETFL, flags | O_NONBLOCK );

    users[fd] = user;
    set_select_events( fd, events );
    if (fd > max_fd) max_fd = fd;
    nb_users++;
    return fd;
}

/* remove a user */
void remove_select_user( int fd )
{
    struct user *user = users[fd];
    assert( user );

    set_select_timeout( fd, 0 );
    set_select_events( fd, 0 );
    users[fd] = NULL;
    if (max_fd == fd) while (max_fd && !users[max_fd]) max_fd--;
    nb_users--;
    free( user );
}

/* set a user timeout */
void set_select_timeout( int fd, struct timeval *when )
{
    struct user *user = users[fd];
    struct user *pos;
    assert( user );

    if (user->when.tv_sec || user->when.tv_usec)
    {
        /* there is already a timeout */
        if (user->next) user->next->prev = user->prev;
        else timeout_tail = user->prev;
        if (user->prev) user->prev->next = user->next;
        else timeout_head = user->next;
        user->when.tv_sec = user->when.tv_usec = 0;
    }
    if (!when) return;  /* no timeout */
    user->when = *when;

    /* Now insert it in the linked list */

    for (pos = timeout_head; pos; pos = pos->next)
    {
        if (pos->when.tv_sec > user->when.tv_sec) break;
        if ((pos->when.tv_sec == user->when.tv_sec) &&
            (pos->when.tv_usec > user->when.tv_usec)) break;
    }

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
}

/* set the events that select waits for on this fd */
void set_select_events( int fd, int events )
{
    if (events & READ_EVENT) FD_SET( fd, &read_set );
    else FD_CLR( fd, &read_set );
    if (events & WRITE_EVENT) FD_SET( fd, &write_set );
    else FD_CLR( fd, &write_set );
}

/* get a user private data, checking the type */
void *get_select_private_data( const struct select_ops *ops, int fd )
{
    struct user *user = users[fd];
    assert( user );
    assert( user->ops == ops );
    return user->private;
}

/* server main loop */
void select_loop(void)
{
    int i, ret;

    setsid();
    signal( SIGPIPE, SIG_IGN );

    while (nb_users)
    {
        fd_set read = read_set, write = write_set;
#if 0
        printf( "select: " );
        for (i = 0; i <= max_fd; i++) printf( "%c", FD_ISSET( i, &read_set ) ? 'r' :
                                                    (FD_ISSET( i, &write_set ) ? 'w' : '-') );
        printf( "\n" );
#endif
        if (timeout_head)
        {
            struct timeval tv, now;
            gettimeofday( &now, NULL );
            if ((timeout_head->when.tv_sec < now.tv_sec) ||
                ((timeout_head->when.tv_sec == now.tv_sec) &&
                 (timeout_head->when.tv_usec < now.tv_usec)))
            {
                timeout_head->ops->timeout( timeout_head->fd, timeout_head->private );
                continue;
            }
            tv.tv_sec = timeout_head->when.tv_sec - now.tv_sec;
            if ((tv.tv_usec = timeout_head->when.tv_usec - now.tv_usec) < 0)
            {
                tv.tv_usec += 1000000;
                tv.tv_sec--;
            }
            ret = select( max_fd + 1, &read, &write, NULL, &tv );
        }
        else  /* no timeout */
        {
            ret = select( max_fd + 1, &read, &write, NULL, NULL );
        }

        if (!ret) continue;
        if (ret == -1) {
            if (errno == EINTR) continue;
            perror("select");
        }

        for (i = 0; i <= max_fd; i++)
        {
            int event = 0;
            if (FD_ISSET( i, &write )) event |= WRITE_EVENT;
            if (FD_ISSET( i, &read ))  event |= READ_EVENT;

            /* Note: users[i] might be NULL here, because an event routine
               called in an earlier pass of this loop might have removed 
               the current user ... */
            if (event && users[i]) 
                users[i]->ops->event( i, event, users[i]->private );
        }
    }
}

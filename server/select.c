/*
 * Server main select() loop
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "object.h"

struct timeout_user
{
    struct timeout_user  *next;       /* next in sorted timeout list */
    struct timeout_user  *prev;       /* prev in sorted timeout list */
    struct timeval        when;       /* timeout expiry (absolute time) */
    timeout_callback      callback;   /* callback function */
    void                 *private;    /* callback private data */
};

static struct select_user *users[FD_SETSIZE]; /* users array */
static fd_set read_set, write_set;          /* current select sets */
static int nb_users;                        /* current number of users */
static int max_fd;                          /* max fd in use */
static struct timeout_user *timeout_head;   /* sorted timeouts list head */
static struct timeout_user *timeout_tail;   /* sorted timeouts list tail */


/* register a user */
void register_select_user( struct select_user *user )
{
    assert( !users[user->fd] );

    users[user->fd] = user;
    if (user->fd > max_fd) max_fd = user->fd;
    nb_users++;
}

/* remove a user */
void unregister_select_user( struct select_user *user )
{
    assert( users[user->fd] == user );

    FD_CLR( user->fd, &read_set );
    FD_CLR( user->fd, &write_set );
    users[user->fd] = NULL;
    if (max_fd == user->fd) while (max_fd && !users[max_fd]) max_fd--;
    nb_users--;
}

/* set the events that select waits for on this fd */
void set_select_events( struct select_user *user, int events )
{
    assert( users[user->fd] == user );
    if (events & READ_EVENT) FD_SET( user->fd, &read_set );
    else FD_CLR( user->fd, &read_set );
    if (events & WRITE_EVENT) FD_SET( user->fd, &write_set );
    else FD_CLR( user->fd, &write_set );
}

/* check if events are pending */
int check_select_events( struct select_user *user, int events )
{
    fd_set read_fds, write_fds;
    struct timeval tv = { 0, 0 };

    FD_ZERO( &read_fds );
    FD_ZERO( &write_fds );
    if (events & READ_EVENT) FD_SET( user->fd, &read_fds );
    if (events & WRITE_EVENT) FD_SET( user->fd, &write_fds );
    return select( user->fd + 1, &read_fds, &write_fds, NULL, &tv ) > 0;
}

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

/* make an absolute timeout value from a relative timeout in milliseconds */
void make_timeout( struct timeval *when, int timeout )
{
    gettimeofday( when, 0 );
    if (!timeout) return;
    if ((when->tv_usec += (timeout % 1000) * 1000) >= 1000000)
    {
        when->tv_usec -= 1000000;
        when->tv_sec++;
    }
    when->tv_sec += timeout / 1000;
}

/* handle an expired timeout */
static void handle_timeout( struct timeout_user *user )
{
    if (user->next) user->next->prev = user->prev;
    else timeout_tail = user->prev;
    if (user->prev) user->prev->next = user->next;
    else timeout_head = user->next;
    user->callback( user->private );
    free( user );
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
        if (timeout_head)
        {
            struct timeval tv, now;
            gettimeofday( &now, NULL );
            if ((timeout_head->when.tv_sec < now.tv_sec) ||
                ((timeout_head->when.tv_sec == now.tv_sec) &&
                 (timeout_head->when.tv_usec < now.tv_usec)))
            {
                handle_timeout( timeout_head );
                continue;
            }
            tv.tv_sec = timeout_head->when.tv_sec - now.tv_sec;
            if ((tv.tv_usec = timeout_head->when.tv_usec - now.tv_usec) < 0)
            {
                tv.tv_usec += 1000000;
                tv.tv_sec--;
            }
#if 0
            printf( "select: " );
            for (i = 0; i <= max_fd; i++) printf( "%c", FD_ISSET( i, &read_set ) ? 'r' :
                                                  (FD_ISSET( i, &write_set ) ? 'w' : '-') );
            printf( " timeout %d.%06d\n", tv.tv_sec, tv.tv_usec );
#endif
            ret = select( max_fd + 1, &read, &write, NULL, &tv );
        }
        else  /* no timeout */
        {
#if 0
            printf( "select: " );
            for (i = 0; i <= max_fd; i++) printf( "%c", FD_ISSET( i, &read_set ) ? 'r' :
                                                  (FD_ISSET( i, &write_set ) ? 'w' : '-') );
            printf( " no timeout\n" );
#endif
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
                users[i]->func( event, users[i]->private );
        }
    }
}

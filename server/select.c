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
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "object.h"
#include "thread.h"


struct poll_user
{
    void (*func)(int event, void *private); /* callback function */
    void  *private;                         /* callback private data */
};

struct timeout_user
{
    struct timeout_user  *next;       /* next in sorted timeout list */
    struct timeout_user  *prev;       /* prev in sorted timeout list */
    struct timeval        when;       /* timeout expiry (absolute time) */
    timeout_callback      callback;   /* callback function */
    void                 *private;    /* callback private data */
};

static struct poll_user *poll_users;        /* users array */
static struct pollfd *pollfd;               /* poll fd array */
static int nb_users;                        /* count of array entries actually in use */
static int active_users;                    /* current number of active users */
static int allocated_users;                 /* count of allocated entries in the array */
static struct poll_user *freelist;          /* list of free entries in the array */

static struct timeout_user *timeout_head;   /* sorted timeouts list head */
static struct timeout_user *timeout_tail;   /* sorted timeouts list tail */


/* add a user and return an opaque handle to it, or -1 on failure */
int add_select_user( int fd, void (*func)(int, void *), void *private )
{
    int ret;
    if (freelist)
    {
        ret = freelist - poll_users;
        freelist = poll_users[ret].private;
        assert( !poll_users[ret].func );
    }
    else
    {
        if (nb_users == allocated_users)
        {
            struct poll_user *newusers;
            struct pollfd *newpoll;
            int new_count = allocated_users ? (allocated_users + allocated_users / 2) : 16;
            if (!(newusers = realloc( poll_users, new_count * sizeof(*poll_users) ))) return -1;
            if (!(newpoll = realloc( pollfd, new_count * sizeof(*pollfd) )))
            {
                free( newusers );
                return -1;
            }
            poll_users = newusers;
            pollfd = newpoll;
            allocated_users = new_count;
        }
        ret = nb_users++;
    }
    pollfd[ret].fd = fd;
    pollfd[ret].events = 0;
    pollfd[ret].revents = 0;
    poll_users[ret].func = func;
    poll_users[ret].private = private;
    active_users++;
    return ret;
}

/* remove a user and close its fd */
void remove_select_user( int user )
{
    if (user == -1) return;  /* avoids checking in all callers */
    assert( poll_users[user].func );
    close( pollfd[user].fd );
    pollfd[user].fd = -1;
    pollfd[user].events = 0;
    pollfd[user].revents = 0;
    poll_users[user].func = NULL;
    poll_users[user].private = freelist;
    freelist = &poll_users[user];
    active_users--;
}

/* change the fd of a select user (the old fd is closed) */
void change_select_fd( int user, int fd )
{
    assert( poll_users[user].func );
    close( pollfd[user].fd );
    pollfd[user].fd = fd;
}

/* set the events that select waits for on this fd */
void set_select_events( int user, int events )
{
    assert( poll_users[user].func );
    pollfd[user].events = events;
}

/* check if events are pending */
int check_select_events( int fd, int events )
{
    struct pollfd pfd;
    pfd.fd     = fd;
    pfd.events = events;
    return poll( &pfd, 1, 0 ) > 0;
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
static void handle_timeout(void)
{
    struct timeout_user *user = timeout_head;
    timeout_head = user->next;
    if (user->next) user->next->prev = user->prev;
    else timeout_tail = user->prev;
    user->callback( user->private );
    free( user );
}

/* SIGHUP handler */
static void sighup_handler()
{
#ifdef DEBUG_OBJECTS
    dump_objects();
#endif
}

/* server main loop */
void select_loop(void)
{
    int ret;
    sigset_t sigset;
    struct sigaction action;

    setsid();
    signal( SIGPIPE, SIG_IGN );

    /* block the signals we use */
    sigemptyset( &sigset );
    sigaddset( &sigset, SIGCHLD );
    sigaddset( &sigset, SIGHUP );
    sigprocmask( SIG_BLOCK, &sigset, NULL );

    /* set the handlers */
    action.sa_mask = sigset;
    action.sa_flags = 0;
    action.sa_handler = sigchld_handler;
    sigaction( SIGCHLD, &action, NULL );
    action.sa_handler = sighup_handler;
    sigaction( SIGHUP, &action, NULL );

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
                    poll_users[i].func( pollfd[i].revents, poll_users[i].private );
                    if (!--ret) break;
                }
            }
        }
    }
}

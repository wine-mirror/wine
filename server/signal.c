/*
 * Server signal handling
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

#include <signal.h>
#include <stdio.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#include <unistd.h>

#include "file.h"
#include "object.h"
#include "process.h"
#include "thread.h"

typedef void (*signal_callback)(void);

struct handler
{
    struct object    obj;         /* object header */
    struct fd       *fd;          /* file descriptor for the pipe side */
    int              pipe_write;  /* unix fd for the pipe write side */
    volatile int     pending;     /* is signal pending? */
    signal_callback  callback;    /* callback function */
};

static void handler_dump( struct object *obj, int verbose );
static void handler_destroy( struct object *obj );

static const struct object_ops handler_ops =
{
    sizeof(struct handler),   /* size */
    handler_dump,             /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_get_fd,                /* get_fd */
    handler_destroy           /* destroy */
};

static void handler_poll_event( struct fd *fd, int event );

static const struct fd_ops handler_fd_ops =
{
    NULL,                     /* get_poll_events */
    handler_poll_event,       /* poll_event */
    no_flush,                 /* flush */
    no_get_file_info,         /* get_file_info */
    no_queue_async            /* queue_async */
};

static struct handler *handler_sighup;
static struct handler *handler_sigterm;
static struct handler *handler_sigint;
static struct handler *handler_sigchld;
static struct handler *handler_sigio;

static sigset_t blocked_sigset;

/* create a signal handler */
static struct handler *create_handler( signal_callback callback )
{
    struct handler *handler;
    int fd[2];

    if (pipe( fd ) == -1) return NULL;
    if (!(handler = alloc_object( &handler_ops )))
    {
        close( fd[0] );
        close( fd[1] );
        return NULL;
    }
    handler->pipe_write = fd[1];
    handler->pending    = 0;
    handler->callback   = callback;

    if (!(handler->fd = create_anonymous_fd( &handler_fd_ops, fd[0], &handler->obj )))
    {
        release_object( handler );
        return NULL;
    }
    set_fd_events( handler->fd, POLLIN );
    return handler;
}

/* handle a signal received for a given handler */
static void do_signal( struct handler *handler )
{
    if (!handler->pending)
    {
        char dummy;
        handler->pending = 1;
        write( handler->pipe_write, &dummy, 1 );
    }
}

static void handler_dump( struct object *obj, int verbose )
{
    struct handler *handler = (struct handler *)obj;
    fprintf( stderr, "Signal handler fd=%p\n", handler->fd );
}

static void handler_destroy( struct object *obj )
{
    struct handler *handler = (struct handler *)obj;
    if (handler->fd) release_object( handler->fd );
    close( handler->pipe_write );
}

static void handler_poll_event( struct fd *fd, int event )
{
    struct handler *handler = get_fd_user( fd );

    if (event & (POLLERR | POLLHUP))
    {
        /* this is not supposed to happen */
        fprintf( stderr, "wineserver: Error on signal handler pipe\n" );
        release_object( handler );
    }
    else if (event & POLLIN)
    {
        char dummy;

        handler->pending = 0;
        read( get_unix_fd( handler->fd ), &dummy, 1 );
        handler->callback();
    }
}

/* SIGHUP callback */
static void sighup_callback(void)
{
#ifdef DEBUG_OBJECTS
    dump_objects();
#endif
}

/* SIGTERM callback */
static void sigterm_callback(void)
{
    flush_registry();
    exit(1);
}

/* SIGINT callback */
static void sigint_callback(void)
{
    kill_all_processes( NULL, 1 );
    flush_registry();
    exit(1);
}

/* SIGIO callback */
static void sigio_callback(void)
{
    /* nothing here yet */
}

/* SIGHUP handler */
static void do_sighup()
{
    do_signal( handler_sighup );
}

/* SIGTERM handler */
static void do_sigterm()
{
    do_signal( handler_sigterm );
}

/* SIGINT handler */
static void do_sigint()
{
    do_signal( handler_sigint );
}

/* SIGCHLD handler */
static void do_sigchld()
{
    do_signal( handler_sigchld );
}

/* SIGIO handler */
static void do_sigio( int signum, siginfo_t *si, void *x )
{
    /* do_change_notify( si->si_fd ); */
    do_signal( handler_sigio );
}

void init_signals(void)
{
    struct sigaction action;

    if (!(handler_sighup  = create_handler( sighup_callback ))) goto error;
    if (!(handler_sigterm = create_handler( sigterm_callback ))) goto error;
    if (!(handler_sigint  = create_handler( sigint_callback ))) goto error;
    if (!(handler_sigchld = create_handler( sigchld_callback ))) goto error;
    if (!(handler_sigio   = create_handler( sigio_callback ))) goto error;

    sigemptyset( &blocked_sigset );
    sigaddset( &blocked_sigset, SIGCHLD );
    sigaddset( &blocked_sigset, SIGHUP );
    sigaddset( &blocked_sigset, SIGINT );
    sigaddset( &blocked_sigset, SIGIO );
    sigaddset( &blocked_sigset, SIGQUIT );
    sigaddset( &blocked_sigset, SIGTERM );

    action.sa_mask = blocked_sigset;
    action.sa_flags = 0;
    action.sa_handler = do_sigchld;
    sigaction( SIGCHLD, &action, NULL );
    action.sa_handler = do_sighup;
    sigaction( SIGHUP, &action, NULL );
    action.sa_handler = do_sigint;
    sigaction( SIGINT, &action, NULL );
    action.sa_handler = do_sigterm;
    sigaction( SIGQUIT, &action, NULL );
    sigaction( SIGTERM, &action, NULL );
    action.sa_sigaction = do_sigio;
    action.sa_flags = SA_SIGINFO;
    sigaction( SIGIO, &action, NULL );
    return;

error:
    fprintf( stderr, "failed to initialize signal handlers\n" );
    exit(1);
}

void close_signals(void)
{
    sigprocmask( SIG_BLOCK, &blocked_sigset, NULL );
    release_object( handler_sighup );
    release_object( handler_sigterm );
    release_object( handler_sigint );
    release_object( handler_sigchld );
    release_object( handler_sigio );
}

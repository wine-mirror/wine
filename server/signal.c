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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <poll.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <unistd.h>

#include "file.h"
#include "object.h"
#include "process.h"
#include "thread.h"
#include "request.h"

#if defined(linux) && defined(__SIGRTMIN)
/* the signal used by linuxthreads as exit signal for clone() threads */
# define SIG_PTHREAD_CANCEL (__SIGRTMIN+1)
#endif

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
    &no_type,                 /* type */
    handler_dump,             /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* get_esync_fd */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
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
    handler_destroy           /* destroy */
};

static void handler_poll_event( struct fd *fd, int event );

static const struct fd_ops handler_fd_ops =
{
    NULL,                     /* get_poll_events */
    handler_poll_event,       /* poll_event */
    NULL,                     /* flush */
    NULL,                     /* get_fd_type */
    NULL,                     /* ioctl */
    NULL,                     /* queue_async */
    NULL                      /* reselect_async */
};

static struct handler *handler_sighup;
static struct handler *handler_sigterm;
static struct handler *handler_sigint;
static struct handler *handler_sigchld;
static struct handler *handler_sigio;

static int watchdog;

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

    if (!(handler->fd = create_anonymous_fd( &handler_fd_ops, fd[0], &handler->obj, 0 )))
    {
        release_object( handler );
        return NULL;
    }
    set_fd_events( handler->fd, POLLIN );
    make_object_permanent( &handler->obj );
    return handler;
}

/* handle a signal received for a given handler */
static void do_signal( struct handler *handler )
{
    if (!handler->pending)
    {
        char dummy = 0;
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
    shutdown_master_socket();
}

/* SIGHUP handler */
static void do_sighup( int signum )
{
    do_signal( handler_sighup );
}

/* SIGTERM handler */
static void do_sigterm( int signum )
{
    do_signal( handler_sigterm );
}

/* SIGINT handler */
static void do_sigint( int signum )
{
    do_signal( handler_sigint );
}

/* SIGALRM handler */
static void do_sigalrm( int signum )
{
    watchdog = 1;
}

/* SIGCHLD handler */
static void do_sigchld( int signum )
{
    do_signal( handler_sigchld );
}

/* SIGSEGV handler */
static void do_sigsegv( int signum )
{
    fprintf( stderr, "wineserver crashed, please enable coredumps (ulimit -c unlimited) and restart.\n");
    abort();
}

/* SIGIO handler */
#ifdef HAVE_SIGINFO_T_SI_FD
static void do_sigio( int signum, siginfo_t *si, void *x )
{
    do_signal( handler_sigio );
    do_change_notify( si->si_fd );
}
#endif

void start_watchdog(void)
{
    alarm( 3 );
    watchdog = 0;
}

void stop_watchdog(void)
{
    alarm( 0 );
    watchdog = 0;
}

int watchdog_triggered(void)
{
    return watchdog != 0;
}

static int core_dump_disabled( void )
{
    int r = 0;
#ifdef RLIMIT_CORE
    struct rlimit lim;

    r = !getrlimit(RLIMIT_CORE, &lim) && (lim.rlim_cur == 0);
#endif
    return r;
}

void init_signals(void)
{
    struct sigaction action;
    sigset_t blocked_sigset;

    if (!(handler_sighup  = create_handler( sighup_callback ))) goto error;
    if (!(handler_sigterm = create_handler( sigterm_callback ))) goto error;
    if (!(handler_sigint  = create_handler( sigint_callback ))) goto error;
    if (!(handler_sigchld = create_handler( sigchld_callback ))) goto error;
    if (!(handler_sigio   = create_handler( sigio_callback ))) goto error;

    sigemptyset( &blocked_sigset );
    sigaddset( &blocked_sigset, SIGCHLD );
    sigaddset( &blocked_sigset, SIGHUP );
    sigaddset( &blocked_sigset, SIGINT );
    sigaddset( &blocked_sigset, SIGALRM );
    sigaddset( &blocked_sigset, SIGIO );
    sigaddset( &blocked_sigset, SIGQUIT );
    sigaddset( &blocked_sigset, SIGTERM );
#ifdef SIG_PTHREAD_CANCEL
    sigaddset( &blocked_sigset, SIG_PTHREAD_CANCEL );
#endif

    action.sa_mask = blocked_sigset;
    action.sa_flags = 0;
    action.sa_handler = do_sigchld;
    sigaction( SIGCHLD, &action, NULL );
#ifdef SIG_PTHREAD_CANCEL
    sigaction( SIG_PTHREAD_CANCEL, &action, NULL );
#endif
    action.sa_handler = do_sighup;
    sigaction( SIGHUP, &action, NULL );
    action.sa_handler = do_sigint;
    sigaction( SIGINT, &action, NULL );
    action.sa_handler = do_sigalrm;
    sigaction( SIGALRM, &action, NULL );
    action.sa_handler = do_sigterm;
    sigaction( SIGQUIT, &action, NULL );
    sigaction( SIGTERM, &action, NULL );
    if (core_dump_disabled())
    {
        action.sa_handler = do_sigsegv;
        sigaction( SIGSEGV, &action, NULL );
    }
    action.sa_handler = SIG_IGN;
    sigaction( SIGXFSZ, &action, NULL );
#ifdef HAVE_SIGINFO_T_SI_FD
    action.sa_sigaction = do_sigio;
    action.sa_flags = SA_SIGINFO;
    sigaction( SIGIO, &action, NULL );
#endif
    return;

error:
    fprintf( stderr, "failed to initialize signal handlers\n" );
    exit(1);
}

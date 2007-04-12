/*
 * Server-side request handling
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#include <unistd.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winternl.h"
#include "wine/library.h"

#include "file.h"
#include "process.h"
#define WANT_REQUEST_HANDLERS
#include "request.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

/* path names for server master Unix socket */
static const char * const server_socket_name = "socket";   /* name of the socket file */
static const char * const server_lock_name = "lock";       /* name of the server lock file */

struct master_socket
{
    struct object        obj;        /* object header */
    struct fd           *fd;         /* file descriptor of the master socket */
    struct timeout_user *timeout;    /* timeout on last process exit */
};

static void master_socket_dump( struct object *obj, int verbose );
static void master_socket_destroy( struct object *obj );
static void master_socket_poll_event( struct fd *fd, int event );

static const struct object_ops master_socket_ops =
{
    sizeof(struct master_socket),  /* size */
    master_socket_dump,            /* dump */
    no_add_queue,                  /* add_queue */
    NULL,                          /* remove_queue */
    NULL,                          /* signaled */
    NULL,                          /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    no_map_access,                 /* map_access */
    no_lookup_name,                /* lookup_name */
    no_open_file,                  /* open_file */
    no_close_handle,               /* close_handle */
    master_socket_destroy          /* destroy */
};

static const struct fd_ops master_socket_fd_ops =
{
    NULL,                          /* get_poll_events */
    master_socket_poll_event,      /* poll_event */
    NULL,                          /* flush */
    NULL,                          /* get_fd_type */
    NULL,                          /* queue_async */
    NULL,                          /* reselect_async */
    NULL                           /* cancel_async */
};


struct thread *current = NULL;  /* thread handling the current request */
unsigned int global_error = 0;  /* global error code for when no thread is current */
struct timeval server_start_time = { 0, 0 };  /* server startup time */

static struct master_socket *master_socket;  /* the master socket object */
static int force_shutdown;

/* socket communication static structures */
static struct iovec myiovec;
static struct msghdr msghdr;
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
struct cmsg_fd
{
    struct
    {
        size_t len;   /* size of structure */
        int    level; /* SOL_SOCKET */
        int    type;  /* SCM_RIGHTS */
    } header;
    int fd;           /* fd to pass */
};
static struct cmsg_fd cmsg = { { sizeof(cmsg.header) + sizeof(cmsg.fd), SOL_SOCKET, SCM_RIGHTS }, -1 };
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

/* complain about a protocol error and terminate the client connection */
void fatal_protocol_error( struct thread *thread, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%04x: ", thread->id );
    vfprintf( stderr, err, args );
    va_end( args );
    thread->exit_code = 1;
    kill_thread( thread, 1 );
}

/* complain about a protocol error and terminate the client connection */
void fatal_protocol_perror( struct thread *thread, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%04x: ", thread->id );
    vfprintf( stderr, err, args );
    perror( " " );
    va_end( args );
    thread->exit_code = 1;
    kill_thread( thread, 1 );
}

/* die on a fatal error */
void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wineserver: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

/* die on a fatal error */
void fatal_perror( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wineserver: " );
    vfprintf( stderr, err, args );
    perror( " " );
    va_end( args );
    exit(1);
}

/* allocate the reply data */
void *set_reply_data_size( data_size_t size )
{
    assert( size <= get_reply_max_size() );
    if (size && !(current->reply_data = mem_alloc( size ))) size = 0;
    current->reply_size = size;
    return current->reply_data;
}

/* write the remaining part of the reply */
void write_reply( struct thread *thread )
{
    int ret;

    if ((ret = write( get_unix_fd( thread->reply_fd ),
                      (char *)thread->reply_data + thread->reply_size - thread->reply_towrite,
                      thread->reply_towrite )) >= 0)
    {
        if (!(thread->reply_towrite -= ret))
        {
            free( thread->reply_data );
            thread->reply_data = NULL;
            /* sent everything, can go back to waiting for requests */
            set_fd_events( thread->request_fd, POLLIN );
            set_fd_events( thread->reply_fd, 0 );
        }
        return;
    }
    if (errno == EPIPE)
        kill_thread( thread, 0 );  /* normal death */
    else if (errno != EWOULDBLOCK && errno != EAGAIN)
        fatal_protocol_perror( thread, "reply write" );
}

/* send a reply to the current thread */
static void send_reply( union generic_reply *reply )
{
    int ret;

    if (!current->reply_size)
    {
        if ((ret = write( get_unix_fd( current->reply_fd ),
                          reply, sizeof(*reply) )) != sizeof(*reply)) goto error;
    }
    else
    {
        struct iovec vec[2];

        vec[0].iov_base = (void *)reply;
        vec[0].iov_len  = sizeof(*reply);
        vec[1].iov_base = current->reply_data;
        vec[1].iov_len  = current->reply_size;

        if ((ret = writev( get_unix_fd( current->reply_fd ), vec, 2 )) < sizeof(*reply)) goto error;

        if ((current->reply_towrite = current->reply_size - (ret - sizeof(*reply))))
        {
            /* couldn't write it all, wait for POLLOUT */
            set_fd_events( current->reply_fd, POLLOUT );
            set_fd_events( current->request_fd, 0 );
            return;
        }
    }
    free( current->reply_data );
    current->reply_data = NULL;
    return;

 error:
    if (ret >= 0)
        fatal_protocol_error( current, "partial write %d\n", ret );
    else if (errno == EPIPE)
        kill_thread( current, 0 );  /* normal death */
    else
        fatal_protocol_perror( current, "reply write" );
}

/* call a request handler */
static void call_req_handler( struct thread *thread )
{
    union generic_reply reply;
    enum request req = thread->req.request_header.req;

    current = thread;
    current->reply_size = 0;
    clear_error();
    memset( &reply, 0, sizeof(reply) );

    if (debug_level) trace_request();

    if (req < REQ_NB_REQUESTS)
        req_handlers[req]( &current->req, &reply );
    else
        set_error( STATUS_NOT_IMPLEMENTED );

    if (current)
    {
        if (current->reply_fd)
        {
            reply.reply_header.error = current->error;
            reply.reply_header.reply_size = current->reply_size;
            if (debug_level) trace_reply( req, &reply );
            send_reply( &reply );
        }
        else
        {
            current->exit_code = 1;
            kill_thread( current, 1 );  /* no way to continue without reply fd */
        }
    }
    current = NULL;
}

/* read a request from a thread */
void read_request( struct thread *thread )
{
    int ret;

    if (!thread->req_toread)  /* no pending request */
    {
        if ((ret = read( get_unix_fd( thread->request_fd ), &thread->req,
                         sizeof(thread->req) )) != sizeof(thread->req)) goto error;
        if (!(thread->req_toread = thread->req.request_header.request_size))
        {
            /* no data, handle request at once */
            call_req_handler( thread );
            return;
        }
        if (!(thread->req_data = malloc( thread->req_toread )))
        {
            fatal_protocol_error( thread, "no memory for %u bytes request %d\n",
                                  thread->req_toread, thread->req.request_header.req );
            return;
        }
    }

    /* read the variable sized data */
    for (;;)
    {
        ret = read( get_unix_fd( thread->request_fd ),
                    (char *)thread->req_data + thread->req.request_header.request_size
                      - thread->req_toread,
                    thread->req_toread );
        if (ret <= 0) break;
        if (!(thread->req_toread -= ret))
        {
            call_req_handler( thread );
            free( thread->req_data );
            thread->req_data = NULL;
            return;
        }
    }

error:
    if (!ret)  /* closed pipe */
        kill_thread( thread, 0 );
    else if (ret > 0)
        fatal_protocol_error( thread, "partial read %d\n", ret );
    else if (errno != EWOULDBLOCK && errno != EAGAIN)
        fatal_protocol_perror( thread, "read" );
}

/* receive a file descriptor on the process socket */
int receive_fd( struct process *process )
{
    struct send_fd data;
    int fd, ret;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(int);
    msghdr.msg_accrights = (void *)&fd;
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg.header) + sizeof(fd);
    cmsg.fd = -1;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    myiovec.iov_base = (void *)&data;
    myiovec.iov_len  = sizeof(data);

    ret = recvmsg( get_unix_fd( process->msg_fd ), &msghdr, 0 );
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    fd = cmsg.fd;
#endif

    if (ret == sizeof(data))
    {
        struct thread *thread;

        if (data.tid) thread = get_thread_from_id( data.tid );
        else thread = (struct thread *)grab_object( get_process_first_thread( process ));

        if (!thread || thread->process != process || thread->state == TERMINATED)
        {
            if (debug_level)
                fprintf( stderr, "%04x: *fd* %d <- %d bad thread id\n",
                         data.tid, data.fd, fd );
            close( fd );
        }
        else
        {
            if (debug_level)
                fprintf( stderr, "%04x: *fd* %d <- %d\n",
                         thread->id, data.fd, fd );
            thread_add_inflight_fd( thread, data.fd, fd );
        }
        if (thread) release_object( thread );
        return 0;
    }

    if (!ret)
    {
        kill_process( process, 0 );
    }
    else if (ret > 0)
    {
        fprintf( stderr, "Protocol error: process %04x: partial recvmsg %d for fd\n",
                 process->id, ret );
        kill_process( process, 1 );
    }
    else
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            fprintf( stderr, "Protocol error: process %04x: ", process->id );
            perror( "recvmsg" );
            kill_process( process, 1 );
        }
    }
    return -1;
}

/* send an fd to a client */
int send_client_fd( struct process *process, int fd, obj_handle_t handle )
{
    int ret;

    if (debug_level)
        fprintf( stderr, "%04x: *fd* %p -> %d\n",
                 current ? current->id : process->id, handle, fd );

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(fd);
    msghdr.msg_accrights = (void *)&fd;
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg.header) + sizeof(fd);
    cmsg.fd = fd;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    myiovec.iov_base = (void *)&handle;
    myiovec.iov_len  = sizeof(handle);

    ret = sendmsg( get_unix_fd( process->msg_fd ), &msghdr, 0 );

    if (ret == sizeof(handle)) return 0;

    if (ret >= 0)
    {
        fprintf( stderr, "Protocol error: process %04x: partial sendmsg %d\n", process->id, ret );
        kill_process( process, 1 );
    }
    else if (errno == EPIPE)
    {
        kill_process( process, 0 );
    }
    else
    {
        fprintf( stderr, "Protocol error: process %04x: ", process->id );
        perror( "sendmsg" );
        kill_process( process, 1 );
    }
    return -1;
}

/* get current tick count to return to client */
unsigned int get_tick_count(void)
{
    return ((current_time.tv_sec - server_start_time.tv_sec) * 1000) +
           ((current_time.tv_usec - server_start_time.tv_usec) / 1000);
}

static void master_socket_dump( struct object *obj, int verbose )
{
    struct master_socket *sock = (struct master_socket *)obj;
    assert( obj->ops == &master_socket_ops );
    fprintf( stderr, "Master socket fd=%p\n", sock->fd );
}

static void master_socket_destroy( struct object *obj )
{
    struct master_socket *sock = (struct master_socket *)obj;
    assert( obj->ops == &master_socket_ops );
    release_object( sock->fd );
}

/* handle a socket event */
static void master_socket_poll_event( struct fd *fd, int event )
{
    struct master_socket *sock = get_fd_user( fd );
    assert( master_socket->obj.ops == &master_socket_ops );

    assert( sock == master_socket );  /* there is only one master socket */

    if (event & (POLLERR | POLLHUP))
    {
        /* this is not supposed to happen */
        fprintf( stderr, "wineserver: Error on master socket\n" );
        set_fd_events( sock->fd, -1 );
    }
    else if (event & POLLIN)
    {
        struct sockaddr_un dummy;
        unsigned int len = sizeof(dummy);
        int client = accept( get_unix_fd( master_socket->fd ), (struct sockaddr *) &dummy, &len );
        if (client == -1) return;
        if (sock->timeout)
        {
            remove_timeout_user( sock->timeout );
            sock->timeout = NULL;
        }
        fcntl( client, F_SETFL, O_NONBLOCK );
        create_process( client, NULL, 0 );
    }
}

/* remove the socket upon exit */
static void socket_cleanup(void)
{
    static int do_it_once;
    if (!do_it_once++) unlink( server_socket_name );
}

/* create a directory and check its permissions */
static void create_dir( const char *name, struct stat *st )
{
    if (lstat( name, st ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "lstat %s", name );
        if (mkdir( name, 0700 ) == -1 && errno != EEXIST) fatal_perror( "mkdir %s", name );
        if (lstat( name, st ) == -1) fatal_perror( "lstat %s", name );
    }
    if (!S_ISDIR(st->st_mode)) fatal_error( "%s is not a directory\n", name );
    if (st->st_uid != getuid()) fatal_error( "%s is not owned by you\n", name );
    if (st->st_mode & 077) fatal_error( "%s must not be accessible by other users\n", name );
}

/* create the server directory and chdir to it */
static void create_server_dir( const char *dir )
{
    char *p, *server_dir;
    struct stat st, st2;

    if (!(server_dir = strdup( dir ))) fatal_error( "out of memory\n" );

    /* first create the base directory if needed */

    p = strrchr( server_dir, '/' );
    *p = 0;
    create_dir( server_dir, &st );

    /* now create the server directory */

    *p = '/';
    create_dir( server_dir, &st );

    if (chdir( server_dir ) == -1) fatal_perror( "chdir %s", server_dir );
    if (stat( ".", &st2 ) == -1) fatal_perror( "stat %s", server_dir );
    if (st.st_dev != st2.st_dev || st.st_ino != st2.st_ino)
        fatal_error( "chdir did not end up in %s\n", server_dir );

    free( server_dir );
}

/* create the lock file and return its file descriptor */
static int create_server_lock(void)
{
    struct stat st;
    int fd;

    if (lstat( server_lock_name, &st ) == -1)
    {
        if (errno != ENOENT)
            fatal_perror( "lstat %s/%s", wine_get_server_dir(), server_lock_name );
    }
    else
    {
        if (!S_ISREG(st.st_mode))
            fatal_error( "%s/%s is not a regular file\n", wine_get_server_dir(), server_lock_name );
    }

    if ((fd = open( server_lock_name, O_CREAT|O_TRUNC|O_WRONLY, 0600 )) == -1)
        fatal_perror( "error creating %s/%s", wine_get_server_dir(), server_lock_name );
    return fd;
}

/* wait for the server lock */
int wait_for_lock(void)
{
    const char *server_dir = wine_get_server_dir();
    int fd, r;
    struct flock fl;

    if (!server_dir) return 0;  /* no server dir, so no lock to wait on */

    create_server_dir( server_dir );
    fd = create_server_lock();

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 1;
    r = fcntl( fd, F_SETLKW, &fl );
    close(fd);

    return r;
}

/* kill the wine server holding the lock */
int kill_lock_owner( int sig )
{
    const char *server_dir = wine_get_server_dir();
    int fd, i, ret = 0;
    pid_t pid = 0;
    struct flock fl;

    if (!server_dir) return 0;  /* no server dir, nothing to do */

    create_server_dir( server_dir );
    fd = create_server_lock();

    for (i = 0; i < 10; i++)
    {
        fl.l_type   = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start  = 0;
        fl.l_len    = 1;
        if (fcntl( fd, F_GETLK, &fl ) == -1) goto done;
        if (fl.l_type != F_WRLCK) goto done;  /* the file is not locked */
        if (!pid)  /* first time around */
        {
            if (!(pid = fl.l_pid)) goto done;  /* shouldn't happen */
            if (sig == -1)
            {
                if (kill( pid, SIGINT ) == -1) goto done;
                kill( pid, SIGCONT );
                ret = 1;
            }
            else  /* just send the specified signal and return */
            {
                ret = (kill( pid, sig ) != -1);
                goto done;
            }
        }
        else if (fl.l_pid != pid) goto done;  /* no longer the same process */
        sleep( 1 );
    }
    /* waited long enough, now kill it */
    kill( pid, SIGKILL );

 done:
    close( fd );
    return ret;
}

/* acquire the main server lock */
static void acquire_lock(void)
{
    struct sockaddr_un addr;
    struct stat st;
    struct flock fl;
    int fd, slen, got_lock = 0;

    fd = create_server_lock();

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 1;
    if (fcntl( fd, F_SETLK, &fl ) != -1)
    {
        /* check for crashed server */
        if (stat( server_socket_name, &st ) != -1 &&   /* there is a leftover socket */
            stat( "core", &st ) != -1 && st.st_size)   /* and there is a non-empty core file */
        {
            fprintf( stderr,
                     "Warning: a previous instance of the wine server seems to have crashed.\n"
                     "Please run 'gdb %s %s/core',\n"
                     "type 'backtrace' at the gdb prompt and report the results. Thanks.\n\n",
                     server_argv0, wine_get_server_dir() );
        }
        unlink( server_socket_name ); /* we got the lock, we can safely remove the socket */
        got_lock = 1;
        /* in that case we reuse fd without closing it, this ensures
         * that we hold the lock until the process exits */
    }
    else
    {
        switch(errno)
        {
        case ENOLCK:
            break;
        case EACCES:
            /* check whether locks work at all on this file system */
            if (fcntl( fd, F_GETLK, &fl ) == -1) break;
            /* fall through */
        case EAGAIN:
            exit(2); /* we didn't get the lock, exit with special status */
        default:
            fatal_perror( "fcntl %s/%s", wine_get_server_dir(), server_lock_name );
        }
        /* it seems we can't use locks on this fs, so we will use the socket existence as lock */
        close( fd );
    }

    if ((fd = socket( AF_UNIX, SOCK_STREAM, 0 )) == -1) fatal_perror( "socket" );
    addr.sun_family = AF_UNIX;
    strcpy( addr.sun_path, server_socket_name );
    slen = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
    addr.sun_len = slen;
#endif
    if (bind( fd, (struct sockaddr *)&addr, slen ) == -1)
    {
        if ((errno == EEXIST) || (errno == EADDRINUSE))
        {
            if (got_lock)
                fatal_error( "couldn't bind to the socket even though we hold the lock\n" );
            exit(2); /* we didn't get the lock, exit with special status */
        }
        fatal_perror( "bind" );
    }
    atexit( socket_cleanup );
    chmod( server_socket_name, 0600 );  /* make sure no other user can connect */
    if (listen( fd, 5 ) == -1) fatal_perror( "listen" );

    if (!(master_socket = alloc_object( &master_socket_ops )) ||
        !(master_socket->fd = create_anonymous_fd( &master_socket_fd_ops, fd, &master_socket->obj, 0 )))
        fatal_error( "out of memory\n" );
    master_socket->timeout = NULL;
    set_fd_events( master_socket->fd, POLLIN );
    make_object_static( &master_socket->obj );
}

/* open the master server socket and start waiting for new clients */
void open_master_socket(void)
{
    const char *server_dir = wine_get_server_dir();
    int fd, pid, status, sync_pipe[2];
    char dummy;

    /* make sure no request is larger than the maximum size */
    assert( sizeof(union generic_request) == sizeof(struct request_max_size) );
    assert( sizeof(union generic_reply) == sizeof(struct request_max_size) );

    if (!server_dir) fatal_error( "directory %s cannot be accessed\n", wine_get_config_dir() );
    create_server_dir( server_dir );

    if (!foreground)
    {
        if (pipe( sync_pipe ) == -1) fatal_perror( "pipe" );
        pid = fork();
        switch( pid )
        {
        case 0:  /* child */
            setsid();
            close( sync_pipe[0] );

            acquire_lock();

            /* close stdin and stdout */
            if ((fd = open( "/dev/null", O_RDWR )) != -1)
            {
                dup2( fd, 0 );
                dup2( fd, 1 );
                close( fd );
            }

            /* signal parent */
            dummy = 0;
            write( sync_pipe[1], &dummy, 1 );
            close( sync_pipe[1] );
            break;

        case -1:
            fatal_perror( "fork" );
            break;

        default:  /* parent */
            close( sync_pipe[1] );

            /* wait for child to signal us and then exit */
            if (read( sync_pipe[0], &dummy, 1 ) == 1) _exit(0);

            /* child terminated, propagate exit status */
            wait4( pid, &status, 0, NULL );
            if (WIFEXITED(status)) _exit( WEXITSTATUS(status) );
            _exit(1);
        }
    }
    else  /* remain in the foreground */
    {
        acquire_lock();
    }

    /* setup msghdr structure constant fields */
    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &myiovec;
    msghdr.msg_iovlen  = 1;

    /* init startup time */
    gettimeofday( &server_start_time, NULL );

    /* init the process tracing mechanism */
    init_tracing_mechanism();
}

/* master socket timer expiration handler */
static void close_socket_timeout( void *arg )
{
    master_socket->timeout = NULL;
    flush_registry();

    /* if a new client is waiting, we keep on running */
    if (!force_shutdown && check_fd_events( master_socket->fd, POLLIN )) return;

    if (debug_level) fprintf( stderr, "wineserver: exiting (pid=%ld)\n", (long) getpid() );

#ifdef DEBUG_OBJECTS
    close_objects();  /* shut down everything properly */
#endif
    exit( force_shutdown );
}

/* close the master socket and stop waiting for new clients */
void close_master_socket(void)
{
    if (master_socket_timeout == -1) return;  /* just keep running forever */

    if (master_socket_timeout)
    {
        struct timeval when = current_time;
        add_timeout( &when, master_socket_timeout * 1000 );
        master_socket->timeout = add_timeout_user( &when, close_socket_timeout, NULL );
    }
    else close_socket_timeout( NULL );  /* close it right away */
}

/* forced shutdown, used for wineserver -k */
void shutdown_master_socket(void)
{
    force_shutdown = 1;
    master_socket_timeout = 0;
    if (master_socket->timeout)
    {
        remove_timeout_user( master_socket->timeout );
        close_socket_timeout( NULL );
    }
    set_fd_events( master_socket->fd, -1 ); /* stop waiting for new clients */
}

/*
 * Server-side request handling
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
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
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

#include "winnt.h"
#include "winbase.h"
#include "wincon.h"
#include "thread.h"
#include "process.h"
#include "server.h"
#define WANT_REQUEST_HANDLERS
#include "request.h"
#include "wine/port.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

 /* path names for server master Unix socket */
#define CONFDIR    "/.wine"        /* directory for Wine config relative to $HOME */
#define SERVERDIR  "/wineserver-"  /* server socket directory (hostname appended) */
#define SOCKETNAME "socket"        /* name of the socket file */

struct master_socket
{
    struct object       obj;         /* object header */
};

static void master_socket_dump( struct object *obj, int verbose );
static void master_socket_poll_event( struct object *obj, int event );
static void master_socket_destroy( struct object *obj );

static const struct object_ops master_socket_ops =
{
    sizeof(struct master_socket),  /* size */
    master_socket_dump,            /* dump */
    no_add_queue,                  /* add_queue */
    NULL,                          /* remove_queue */
    NULL,                          /* signaled */
    NULL,                          /* satisfied */
    NULL,                          /* get_poll_events */
    master_socket_poll_event,      /* poll_event */
    no_get_fd,                     /* get_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    master_socket_destroy          /* destroy */
};


struct thread *current = NULL;  /* thread handling the current request */
unsigned int global_error = 0;  /* global error code for when no thread is current */

static struct master_socket *master_socket;  /* the master socket object */

/* socket communication static structures */
static struct iovec myiovec;
static struct msghdr msghdr;
#ifndef HAVE_MSGHDR_ACCRIGHTS
struct cmsg_fd
{
    int len;   /* sizeof structure */
    int level; /* SOL_SOCKET */
    int type;  /* SCM_RIGHTS */
    int fd;    /* fd to pass */
};
static struct cmsg_fd cmsg = { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS, -1 };
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

/* complain about a protocol error and terminate the client connection */
void fatal_protocol_error( struct thread *thread, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%p: ", thread );
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
    fprintf( stderr, "Protocol error:%p: ", thread );
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

/* call a request handler */
static inline void call_req_handler( struct thread *thread, union generic_request *request )
{
    enum request req = request->header.req;

    current = thread;
    clear_error();

    if (debug_level) trace_request( thread, request );

    if (request->header.var_size)
    {
        if ((unsigned int)request->header.var_offset +
                          request->header.var_size > MAX_REQUEST_LENGTH)
        {
            fatal_protocol_error( current, "bad request offset/size %d/%d\n",
                                  request->header.var_offset, request->header.var_size );
            return;
        }
    }

    if (req < REQ_NB_REQUESTS)
    {
        req_handlers[req]( request );
        if (current) send_reply( current, request );
        current = NULL;
        return;
    }
    fatal_protocol_error( current, "bad request %d\n", req );
}

/* read a request from a thread */
void read_request( struct thread *thread )
{
    union generic_request req;
    int ret;

    if ((ret = read( thread->obj.fd, &req, sizeof(req) )) == sizeof(req))
    {
        call_req_handler( thread, &req );
        return;
    }
    if (!ret)  /* closed pipe */
        kill_thread( thread, 0 );
    else if (ret > 0)
        fatal_protocol_error( thread, "partial read %d\n", ret );
    else
        fatal_protocol_perror( thread, "read" );
}

/* send a reply to a thread */
void send_reply( struct thread *thread, union generic_request *request )
{
    int ret;

    if (debug_level) trace_reply( thread, request );

    request->header.error = thread->error;

    if ((ret = write( thread->reply_fd, request, sizeof(*request) )) != sizeof(*request))
    {
        if (ret >= 0)
            fatal_protocol_error( thread, "partial write %d\n", ret );
        else if (errno == EPIPE)
            kill_thread( thread, 0 );  /* normal death */
        else
            fatal_protocol_perror( thread, "reply write" );
    }
}

/* receive a file descriptor on the process socket */
int receive_fd( struct process *process )
{
    struct send_fd data;
    int fd, ret;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(int);
    msghdr.msg_accrights = (void *)&fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    cmsg.fd = -1;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    myiovec.iov_base = &data;
    myiovec.iov_len  = sizeof(data);

    ret = recvmsg( process->obj.fd, &msghdr, 0 );
#ifndef HAVE_MSGHDR_ACCRIGHTS
    fd = cmsg.fd;
#endif

    if (ret == sizeof(data))
    {
        struct thread *thread;

        if (data.tid) thread = get_thread_from_id( data.tid );
        else thread = (struct thread *)grab_object( process->thread_list );

        if (!thread || thread->process != process)
        {
            if (debug_level)
                fprintf( stderr, "%08x: *fd* %d <- %d bad thread id\n",
                         (unsigned int)data.tid, data.fd, fd );
            close( fd );
        }
        else
        {
            if (debug_level)
                fprintf( stderr, "%08x: *fd* %d <- %d\n",
                         (unsigned int)thread, data.fd, fd );
            thread_add_inflight_fd( thread, data.fd, fd );
        }
        if (thread) release_object( thread );
        return 0;
    }

    if (ret >= 0)
    {
        if (ret > 0)
            fprintf( stderr, "Protocol error: process %p: partial recvmsg %d for fd\n",
                     process, ret );
        kill_process( process, NULL, 1 );
    }
    else
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            fprintf( stderr, "Protocol error: process %p: ", process );
            perror( "recvmsg" );
            kill_process( process, NULL, 1 );
        }
    }
    return -1;
}

/* send an fd to a client */
int send_client_fd( struct process *process, int fd, handle_t handle )
{
    int ret;

    if (debug_level)
        fprintf( stderr, "%08x: *fd* %d -> %d\n", (unsigned int)current, handle, fd );

#ifdef HAVE_MSGHDR_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(fd);
    msghdr.msg_accrights = (void *)&fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    cmsg.fd = fd;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    myiovec.iov_base = (void *)&handle;
    myiovec.iov_len  = sizeof(handle);

    ret = sendmsg( process->obj.fd, &msghdr, 0 );

    if (ret == sizeof(handle)) return 0;

    if (ret >= 0)
    {
        if (ret > 0)
            fprintf( stderr, "Protocol error: process %p: partial sendmsg %d\n", process, ret );
        kill_process( process, NULL, 1 );
    }
    else
    {
        fprintf( stderr, "Protocol error: process %p: ", process );
        perror( "sendmsg" );
        kill_process( process, NULL, 1 );
    }
    return -1;
}

static void master_socket_dump( struct object *obj, int verbose )
{
    struct master_socket *sock = (struct master_socket *)obj;
    assert( obj->ops == &master_socket_ops );
    fprintf( stderr, "Master socket fd=%d\n", sock->obj.fd );
}

/* handle a socket event */
static void master_socket_poll_event( struct object *obj, int event )
{
    struct master_socket *sock = (struct master_socket *)obj;
    assert( obj->ops == &master_socket_ops );

    assert( sock == master_socket );  /* there is only one master socket */

    if (event & (POLLERR | POLLHUP))
    {
        /* this is not supposed to happen */
        fprintf( stderr, "wineserver: Error on master socket\n" );
        release_object( obj );
    }
    else if (event & POLLIN)
    {
        struct sockaddr_un dummy;
        int len = sizeof(dummy);
        int client = accept( master_socket->obj.fd, (struct sockaddr *) &dummy, &len );
        if (client == -1) return;
        fcntl( client, F_SETFL, O_NONBLOCK );
        create_process( client );
    }
}

/* remove the socket upon exit */
static void socket_cleanup(void)
{
    static int do_it_once;
    if (!do_it_once++) unlink( SOCKETNAME );
}

static void master_socket_destroy( struct object *obj )
{
    socket_cleanup();
}

/* return the configuration directory ($WINEPREFIX or $HOME/.wine) */
const char *get_config_dir(void)
{
    static char *confdir;
    if (!confdir)
    {
        const char *prefix = getenv( "WINEPREFIX" );
        if (prefix)
        {
            int len = strlen(prefix);
            if (!(confdir = strdup( prefix ))) fatal_error( "out of memory\n" );
            if (len > 1 && confdir[len-1] == '/') confdir[len-1] = 0;
        }
        else
        {
            const char *home = getenv( "HOME" );
            if (!home)
            {
                struct passwd *pwd = getpwuid( getuid() );
                if (!pwd) fatal_error( "could not find your home directory\n" );
                home = pwd->pw_dir;
            }
            if (!(confdir = malloc( strlen(home) + strlen(CONFDIR) + 1 )))
                fatal_error( "out of memory\n" );
            strcpy( confdir, home );
            strcat( confdir, CONFDIR );
        }
        mkdir( confdir, 0755 );  /* just in case */
    }
    return confdir;
}

/* create the server directory and chdir to it */
static void create_server_dir(void)
{
    char hostname[64];
    char *serverdir;
    const char *confdir = get_config_dir();
    struct stat st;

    if (gethostname( hostname, sizeof(hostname) ) == -1) fatal_perror( "gethostname" );

    if (!(serverdir = malloc( strlen(confdir) + strlen(SERVERDIR) + strlen(hostname) + 1 )))
        fatal_error( "out of memory\n" );

    strcpy( serverdir, confdir );
    strcat( serverdir, SERVERDIR );
    strcat( serverdir, hostname );

    if (chdir( serverdir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir %s", serverdir );
        if (mkdir( serverdir, 0700 ) == -1) fatal_perror( "mkdir %s", serverdir );
        if (chdir( serverdir ) == -1) fatal_perror( "chdir %s", serverdir );
    }
    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", serverdir );
    if (!S_ISDIR(st.st_mode)) fatal_error( "%s is not a directory\n", serverdir );
    if (st.st_uid != getuid()) fatal_error( "%s is not owned by you\n", serverdir );
    if (st.st_mode & 077) fatal_error( "%s must not be accessible by other users\n", serverdir );
}

/* open the master server socket and start waiting for new clients */
void open_master_socket(void)
{
    struct sockaddr_un addr;
    int fd, slen;

    /* make sure no request is larger than the maximum size */
    assert( sizeof(union generic_request) == sizeof(struct request_max_size) );

    create_server_dir();
    if ((fd = socket( AF_UNIX, SOCK_STREAM, 0 )) == -1) fatal_perror( "socket" );
    addr.sun_family = AF_UNIX;
    strcpy( addr.sun_path, SOCKETNAME );
    slen = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
#ifdef HAVE_SOCKADDR_SUN_LEN
    addr.sun_len = slen;
#endif
    if (bind( fd, (struct sockaddr *)&addr, slen ) == -1)
    {
        if ((errno == EEXIST) || (errno == EADDRINUSE))
            exit(0);  /* pretend we succeeded to start */
        else
            fatal_perror( "bind" );
    }
    atexit( socket_cleanup );

    chmod( SOCKETNAME, 0600 );  /* make sure no other user can connect */
    if (listen( fd, 5 ) == -1) fatal_perror( "listen" );

    if (!(master_socket = alloc_object( &master_socket_ops, fd )))
        fatal_error( "out of memory\n" );
    set_select_events( &master_socket->obj, POLLIN );

    /* setup msghdr structure constant fields */
    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &myiovec;
    msghdr.msg_iovlen  = 1;

    /* go in the background */
    switch(fork())
    {
    case -1:
        fatal_perror( "fork" );
    case 0:
        setsid();
        break;
    default:
        _exit(0);  /* do not call atexit functions */
    }
}

/* close the master socket and stop waiting for new clients */
void close_master_socket(void)
{
    /* if a new client is waiting, we keep on running */
    if (!check_select_events( master_socket->obj.fd, POLLIN ))
        release_object( master_socket );
}

/* lock/unlock the master socket to stop accepting new clients */
void lock_master_socket( int locked )
{
    set_select_events( &master_socket->obj, locked ? 0 : POLLIN );
}

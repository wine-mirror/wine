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
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    master_socket_destroy          /* destroy */
};


struct thread *current = NULL;  /* thread handling the current request */

static struct master_socket *master_socket;  /* the master socket object */

/* socket communication static structures */
static struct iovec myiovec;
static struct msghdr msghdr = { NULL, 0, &myiovec, 1, };
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

/* die on a fatal error */
static void fatal_error( const char *err, ... ) WINE_NORETURN;
static void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wineserver: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

/* die on a fatal error */
static void fatal_perror( const char *err, ... ) WINE_NORETURN;
static void fatal_perror( const char *err, ... )
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
static inline void call_req_handler( struct thread *thread, enum request req )
{
    current = thread;
    clear_error();

    if (debug_level) trace_request( req );

    if (req < REQ_NB_REQUESTS)
    {
        req_handlers[req]( current->buffer );
        if (current && !current->wait) send_reply( current );
        current = NULL;
        return;
    }
    fatal_protocol_error( current, "bad request %d\n", req );
}

/* set the fd to pass to the thread */
void set_reply_fd( struct thread *thread, int pass_fd )
{
    assert( thread->pass_fd == -1 );
    thread->pass_fd = pass_fd;
}

/* send a reply to a thread */
void send_reply( struct thread *thread )
{
    assert( !thread->wait );
    if (debug_level) trace_reply( thread );
    if (!write_request( thread )) set_select_events( &thread->obj, POLLOUT );
}

/* read a message from a client that has something to say */
void read_request( struct thread *thread )
{
    int ret;
    enum request req;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(int);
    msghdr.msg_accrights = (void *)&thread->pass_fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    cmsg.fd = -1;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    assert( thread->pass_fd == -1 );

    myiovec.iov_base = (void *)&req;
    myiovec.iov_len  = sizeof(req);

    ret = recvmsg( thread->obj.fd, &msghdr, 0 );
#ifndef HAVE_MSGHDR_ACCRIGHTS
    thread->pass_fd = cmsg.fd;
#endif

    if (ret == sizeof(req))
    {
        call_req_handler( thread, req );
        thread->pass_fd = -1;
        return;
    }
    if (ret == -1)
    {
        perror("recvmsg");
        thread->exit_code = 1;
        kill_thread( thread, 1 );
        return;
    }
    if (!ret)  /* closed pipe */
    {
        kill_thread( thread, 0 );
        return;
    }
    fatal_protocol_error( thread, "partial message received %d/%d\n", ret, sizeof(req) );
}

/* send a message to a client that is ready to receive something */
int write_request( struct thread *thread )
{
    int ret;

    if (thread->pass_fd == -1)
    {
        ret = write( thread->obj.fd, &thread->error, sizeof(thread->error) );
    }
    else  /* we have an fd to send */
    {
#ifdef HAVE_MSGHDR_ACCRIGHTS
        msghdr.msg_accrightslen = sizeof(int);
        msghdr.msg_accrights = (void *)&thread->pass_fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
        msghdr.msg_control    = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
        cmsg.fd = thread->pass_fd;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

        myiovec.iov_base = (void *)&thread->error;
        myiovec.iov_len  = sizeof(thread->error);

        ret = sendmsg( thread->obj.fd, &msghdr, 0 );
        close( thread->pass_fd );
        thread->pass_fd = -1;
    }
    if (ret == sizeof(thread->error))
    {
        set_select_events( &thread->obj, POLLIN );
        return 1;
    }
    if (ret == -1)
    {
        if (errno == EWOULDBLOCK) return 0;  /* not a fatal error */
        if (errno == EPIPE)
        {
            kill_thread( thread, 0 );  /* normal death */
        }
        else
        {
            perror("sendmsg");
            thread->exit_code = 1;
            kill_thread( thread, 1 );
        }
    }
    else
    {
        thread->exit_code = 1;
        kill_thread( thread, 1 );
        fprintf( stderr, "Partial message sent %d/%d\n", ret, sizeof(thread->error) );
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
        if (client != -1) create_process( client );
    }
}

/* remove the socket upon exit */
static void socket_cleanup(void)
{
    unlink( SOCKETNAME );
}

static void master_socket_destroy( struct object *obj )
{
    socket_cleanup();
}

/* return the configuration directory ($WINEPREFIX or $HOME/.wine) */
static const char *get_config_dir(void)
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
            fatal_error( "another server is already running\n" );
        else
            fatal_perror( "bind" );
    }
    atexit( socket_cleanup );

    chmod( SOCKETNAME, 0600 );  /* make sure no other user can connect */
    if (listen( fd, 5 ) == -1) fatal_perror( "listen" );

    if (!(master_socket = alloc_object( &master_socket_ops, fd )))
        fatal_error( "out of memory\n" );
    set_select_events( &master_socket->obj, POLLIN );

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
    release_object( master_socket );
}

/* lock/unlock the master socket to stop accepting new clients */
void lock_master_socket( int locked )
{
    set_select_events( &master_socket->obj, locked ? 0 : POLLIN );
}

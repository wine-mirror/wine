/*
 * Client part of the client/server communication
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>

#include "thread.h"
#include "wine/library.h"
#include "wine/server.h"
#include "winerror.h"
#include "options.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

#define SOCKETNAME "socket"        /* name of the socket file */
#define LOCKNAME   "lock"          /* name of the lock file */

#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
/* data structure used to pass an fd with sendmsg/recvmsg */
struct cmsg_fd
{
    int len;   /* sizeof structure */
    int level; /* SOL_SOCKET */
    int type;  /* SCM_RIGHTS */
    int fd;    /* fd to pass */
};
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

static DWORD boot_thread_id;
static sigset_t block_set;  /* signals to block during server calls */
static int fd_socket;  /* socket to exchange file descriptors with the server */

#ifdef __GNUC__
static void fatal_error( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
static void fatal_perror( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
static void server_connect_error( const char *serverdir ) __attribute__((noreturn));
#endif

/* die on a fatal error; use only during initialization */
static void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

/* die on a fatal error; use only during initialization */
static void fatal_perror( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    perror( " " );
    va_end( args );
    exit(1);
}

/***********************************************************************
 *           server_protocol_error
 */
void server_protocol_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine client error:%lx: ", NtCurrentTeb()->tid );
    vfprintf( stderr, err, args );
    va_end( args );
    SYSDEPS_AbortThread(1);
}


/***********************************************************************
 *           server_protocol_perror
 */
void server_protocol_perror( const char *err )
{
    fprintf( stderr, "wine client error:%lx: ", NtCurrentTeb()->tid );
    perror( err );
    SYSDEPS_AbortThread(1);
}


/***********************************************************************
 *           send_request
 *
 * Send a request to the server.
 */
static void send_request( const struct __server_request_info *req )
{
    int i, ret;

    if (!req->u.req.request_header.request_size)
    {
        if ((ret = write( NtCurrentTeb()->request_fd, &req->u.req,
                          sizeof(req->u.req) )) == sizeof(req->u.req)) return;

    }
    else
    {
        struct iovec vec[__SERVER_MAX_DATA+1];

        vec[0].iov_base = (void *)&req->u.req;
        vec[0].iov_len = sizeof(req->u.req);
        for (i = 0; i < req->data_count; i++)
        {
            vec[i+1].iov_base = (void *)req->data[i].ptr;
            vec[i+1].iov_len = req->data[i].size;
        }
        if ((ret = writev( NtCurrentTeb()->request_fd, vec, i+1 )) ==
            req->u.req.request_header.request_size + sizeof(req->u.req)) return;
    }

    if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
    if (errno == EPIPE) SYSDEPS_AbortThread(0);
    server_protocol_perror( "sendmsg" );
}


/***********************************************************************
 *           read_reply_data
 *
 * Read data from the reply buffer; helper for wait_reply.
 */
static void read_reply_data( void *buffer, size_t size )
{
    int ret;

    for (;;)
    {
        if ((ret = read( NtCurrentTeb()->reply_fd, buffer, size )) > 0)
        {
            if (!(size -= ret)) return;
            buffer = (char *)buffer + ret;
            continue;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("read");
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_AbortThread(0);
}


/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
inline static void wait_reply( struct __server_request_info *req )
{
    read_reply_data( &req->u.reply, sizeof(req->u.reply) );
    if (req->u.reply.reply_header.reply_size)
        read_reply_data( req->reply_data, req->u.reply.reply_header.reply_size );
}


/***********************************************************************
 *           wine_server_call (NTDLL.@)
 *
 * Perform a server call.
 */
unsigned int wine_server_call( void *req_ptr )
{
    struct __server_request_info * const req = req_ptr;
    sigset_t old_set;

    memset( (char *)&req->u.req + req->size, 0, sizeof(req->u.req) - req->size );
    sigprocmask( SIG_BLOCK, &block_set, &old_set );
    send_request( req );
    wait_reply( req );
    sigprocmask( SIG_SETMASK, &old_set, NULL );
    return req->u.reply.reply_header.error;
}


/***********************************************************************
 *           wine_server_send_fd
 *
 * Send a file descriptor to the server.
 */
void wine_server_send_fd( int fd )
{
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    struct cmsg_fd cmsg;
#endif
    struct send_fd data;
    struct msghdr msghdr;
    struct iovec vec;
    int ret;

    vec.iov_base = (void *)&data;
    vec.iov_len  = sizeof(data);

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    cmsg.len   = sizeof(cmsg);
    cmsg.level = SOL_SOCKET;
    cmsg.type  = SCM_RIGHTS;
    cmsg.fd    = fd;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    msghdr.msg_flags      = 0;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    data.tid = GetCurrentThreadId();
    data.fd  = fd;

    for (;;)
    {
        if ((ret = sendmsg( fd_socket, &msghdr, 0 )) == sizeof(data)) return;
        if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
        if (errno == EINTR) continue;
        if (errno == EPIPE) SYSDEPS_AbortThread(0);
        server_protocol_perror( "sendmsg" );
    }
}


/***********************************************************************
 *           receive_fd
 *
 * Receive a file descriptor passed from the server.
 */
static int receive_fd( obj_handle_t *handle )
{
    struct iovec vec;
    int ret, fd;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    struct msghdr msghdr;

    fd = -1;
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    struct msghdr msghdr;
    struct cmsg_fd cmsg;

    cmsg.len   = sizeof(cmsg);
    cmsg.level = SOL_SOCKET;
    cmsg.type  = SCM_RIGHTS;
    cmsg.fd    = -1;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    msghdr.msg_flags      = 0;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;
    vec.iov_base = (void *)handle;
    vec.iov_len  = sizeof(*handle);

    for (;;)
    {
        if ((ret = recvmsg( fd_socket, &msghdr, 0 )) > 0)
        {
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
            fd = cmsg.fd;
#endif
            if (fd == -1) server_protocol_error( "no fd received for handle %d\n", *handle );
            fcntl( fd, F_SETFD, 1 ); /* set close on exec flag */
            return fd;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("recvmsg");
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_AbortThread(0);
}


/***********************************************************************
 *           store_cached_fd
 *
 * Store the cached fd value for a given handle back into the server.
 * Returns the new fd, which can be different if there was already an
 * fd in the cache for that handle.
 */
inline static int store_cached_fd( int fd, obj_handle_t handle )
{
    SERVER_START_REQ( set_handle_info )
    {
        req->handle = handle;
        req->flags  = 0;
        req->mask   = 0;
        req->fd     = fd;
        if (!wine_server_call( req ))
        {
            if (reply->cur_fd != fd)
            {
                /* someone was here before us */
                close( fd );
                fd = reply->cur_fd;
            }
        }
        else
        {
            close( fd );
            fd = -1;
        }
    }
    SERVER_END_REQ;
    return fd;
}


/***********************************************************************
 *           wine_server_fd_to_handle   (NTDLL.@)
 *
 * Allocate a file handle for a Unix fd.
 */
int wine_server_fd_to_handle( int fd, unsigned int access, int inherit, obj_handle_t *handle )
{
    int ret;

    *handle = 0;
    wine_server_send_fd( fd );

    SERVER_START_REQ( alloc_file_handle )
    {
        req->access  = access;
        req->inherit = inherit;
        req->fd      = fd;
        if (!(ret = wine_server_call( req ))) *handle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           wine_server_handle_to_fd   (NTDLL.@)
 *
 * Retrieve the Unix fd corresponding to a file handle.
 */
int wine_server_handle_to_fd( obj_handle_t handle, unsigned int access, int *unix_fd,
                              enum fd_type *type, int *flags )
{
    obj_handle_t fd_handle;
    int ret, fd = -1;

    *unix_fd = -1;
    for (;;)
    {
        SERVER_START_REQ( get_handle_fd )
        {
            req->handle = handle;
            req->access = access;
            if (!(ret = wine_server_call( req ))) fd = reply->fd;
            if (type) *type = reply->type;
            if (flags) *flags = reply->flags;
        }
        SERVER_END_REQ;
        if (ret) return ret;

        if (fd != -1) break;

        /* it wasn't in the cache, get it from the server */
        fd = receive_fd( &fd_handle );
        /* and store it back into the cache */
        fd = store_cached_fd( fd, fd_handle );

        if (fd_handle == handle) break;
        /* if we received a different handle this means there was
         * a race with another thread; we restart everything from
         * scratch in this case.
         */
    }

    if ((fd != -1) && ((fd = dup(fd)) == -1)) return STATUS_TOO_MANY_OPENED_FILES;
    *unix_fd = fd;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           start_server
 *
 * Start a new wine server.
 */
static void start_server( const char *oldcwd )
{
    static int started;  /* we only try once */
    char *path, *p;
    if (!started)
    {
        int status;
        int pid = fork();
        if (pid == -1) fatal_perror( "fork" );
        if (!pid)
        {
            /* if server is explicitly specified, use this */
            if ((p = getenv("WINESERVER")))
            {
                if (p[0] != '/' && oldcwd[0] == '/')  /* make it an absolute path */
                {
                    if (!(path = malloc( strlen(oldcwd) + strlen(p) + 1 )))
                        fatal_error( "out of memory\n" );
                    sprintf( path, "%s/%s", oldcwd, p );
                    p = path;
                }
                execl( p, p, NULL );
                fatal_perror( "could not exec the server '%s'\n"
                              "    specified in the WINESERVER environment variable", p );
            }

            /* first try the installation dir */
            execl( BINDIR "/wineserver", "wineserver", NULL );

            /* now try the dir we were launched from */
            if (full_argv0)
            {
                if (!(path = malloc( strlen(full_argv0) + 20 )))
                    fatal_error( "out of memory\n" );
                if ((p = strrchr( strcpy( path, full_argv0 ), '/' )))
                {
                    strcpy( p, "/wineserver" );
                    execl( path, path, NULL );
                }
                free(path);
            }

            /* finally try the path */
            execlp( "wineserver", "wineserver", NULL );
            fatal_error( "could not exec wineserver\n" );
        }
        waitpid( pid, &status, 0 );
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (status == 2) return;  /* server lock held by someone else, will retry later */
        if (status) exit(status);  /* server failed */
        started = 1;
    }
}


/***********************************************************************
 *           server_connect_error
 *
 * Try to display a meaningful explanation of why we couldn't connect
 * to the server.
 */
static void server_connect_error( const char *serverdir )
{
    int fd;
    struct flock fl;

    if ((fd = open( LOCKNAME, O_WRONLY )) == -1)
        fatal_error( "for some mysterious reason, the wine server never started.\n" );

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 1;
    if (fcntl( fd, F_GETLK, &fl ) != -1)
    {
        if (fl.l_type == F_WRLCK)  /* the file is locked */
            fatal_error( "a wine server seems to be running, but I cannot connect to it.\n"
                         "   You probably need to kill that process (it might be pid %d).\n",
                         (int)fl.l_pid );
        fatal_error( "for some mysterious reason, the wine server failed to run.\n" );
    }
    fatal_error( "the file system of '%s' doesn't support locks,\n"
          "   and there is a 'socket' file in that directory that prevents wine from starting.\n"
          "   You should make sure no wine server is running, remove that file and try again.\n",
                 serverdir );
}


/***********************************************************************
 *           server_connect
 *
 * Attempt to connect to an existing server socket.
 * We need to be in the server directory already.
 */
static int server_connect( const char *oldcwd, const char *serverdir )
{
    struct sockaddr_un addr;
    struct stat st;
    int s, slen, retry;

    /* chdir to the server directory */
    if (chdir( serverdir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s", serverdir );
        start_server( "." );
        if (chdir( serverdir ) == -1) fatal_perror( "chdir to %s", serverdir );
    }

    /* make sure we are at the right place */
    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", serverdir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", serverdir );
    if (st.st_mode & 077) fatal_error( "'%s' must not be accessible by other users\n", serverdir );

    for (retry = 0; retry < 6; retry++)
    {
        /* if not the first try, wait a bit to leave the previous server time to exit */
        if (retry)
        {
            usleep( 100000 * retry * retry );
            start_server( oldcwd );
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }
        else if (lstat( SOCKETNAME, &st ) == -1) /* check for an already existing socket */
        {
            if (errno != ENOENT) fatal_perror( "lstat %s/%s", serverdir, SOCKETNAME );
            start_server( oldcwd );
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }

        /* make sure the socket is sane (ISFIFO needed for Solaris) */
        if (!S_ISSOCK(st.st_mode) && !S_ISFIFO(st.st_mode))
            fatal_error( "'%s/%s' is not a socket\n", serverdir, SOCKETNAME );
        if (st.st_uid != getuid())
            fatal_error( "'%s/%s' is not owned by you\n", serverdir, SOCKETNAME );

        /* try to connect to it */
        addr.sun_family = AF_UNIX;
        strcpy( addr.sun_path, SOCKETNAME );
        slen = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
        addr.sun_len = slen;
#endif
        if ((s = socket( AF_UNIX, SOCK_STREAM, 0 )) == -1) fatal_perror( "socket" );
        if (connect( s, (struct sockaddr *)&addr, slen ) != -1)
        {
            fcntl( s, F_SETFD, 1 ); /* set close on exec flag */
            return s;
        }
        close( s );
    }
    server_connect_error( serverdir );
}


/***********************************************************************
 *           CLIENT_InitServer
 *
 * Start the server and create the initial socket pair.
 */
void CLIENT_InitServer(void)
{
    int size;
    char *oldcwd;
    obj_handle_t dummy_handle;

    /* retrieve the current directory */
    for (size = 512; ; size *= 2)
    {
        if (!(oldcwd = malloc( size ))) break;
        if (getcwd( oldcwd, size )) break;
        free( oldcwd );
        if (errno == ERANGE) continue;
        oldcwd = NULL;
        break;
    }

    /* if argv[0] is a relative path, make it absolute */
    full_argv0 = argv0;
    if (oldcwd && argv0[0] != '/' && strchr( argv0, '/' ))
    {
        char *new_argv0 = malloc( strlen(oldcwd) + strlen(argv0) + 2 );
        if (new_argv0)
        {
            strcpy( new_argv0, oldcwd );
            strcat( new_argv0, "/" );
            strcat( new_argv0, argv0 );
            full_argv0 = new_argv0;
        }
    }

    /* connect to the server */
    fd_socket = server_connect( oldcwd, wine_get_server_dir() );

    /* switch back to the starting directory */
    if (oldcwd)
    {
        chdir( oldcwd );
        free( oldcwd );
    }

    /* setup the signal mask */
    sigemptyset( &block_set );
    sigaddset( &block_set, SIGALRM );
    sigaddset( &block_set, SIGIO );
    sigaddset( &block_set, SIGINT );
    sigaddset( &block_set, SIGHUP );
    sigaddset( &block_set, SIGUSR1 );
    sigaddset( &block_set, SIGUSR2 );

    /* receive the first thread request fd on the main socket */
    NtCurrentTeb()->request_fd = receive_fd( &dummy_handle );

    CLIENT_InitThread();
}


/***********************************************************************
 *           CLIENT_InitThread
 *
 * Send an init thread request. Return 0 if OK.
 */
void CLIENT_InitThread(void)
{
    TEB *teb = NtCurrentTeb();
    int version, ret;
    int reply_pipe[2];

    /* ignore SIGPIPE so that we get a EPIPE error instead  */
    signal( SIGPIPE, SIG_IGN );
    /* automatic child reaping to avoid zombies */
    signal( SIGCHLD, SIG_IGN );

    /* create the server->client communication pipes */
    if (pipe( reply_pipe ) == -1) server_protocol_perror( "pipe" );
    if (pipe( teb->wait_fd ) == -1) server_protocol_perror( "pipe" );
    wine_server_send_fd( reply_pipe[1] );
    wine_server_send_fd( teb->wait_fd[1] );
    teb->reply_fd = reply_pipe[0];
    close( reply_pipe[1] );

    /* set close on exec flag */
    fcntl( teb->reply_fd, F_SETFD, 1 );
    fcntl( teb->wait_fd[0], F_SETFD, 1 );
    fcntl( teb->wait_fd[1], F_SETFD, 1 );

    SERVER_START_REQ( init_thread )
    {
        req->unix_pid    = getpid();
        req->unix_tid    = -1;
        req->teb         = teb;
        req->entry       = teb->entry_point;
        req->reply_fd    = reply_pipe[1];
        req->wait_fd     = teb->wait_fd[1];
        ret = wine_server_call( req );
        teb->pid = reply->pid;
        teb->tid = reply->tid;
        version  = reply->version;
        if (reply->boot) boot_thread_id = teb->tid;
        else if (boot_thread_id == teb->tid) boot_thread_id = 0;
    }
    SERVER_END_REQ;

    if (ret) server_protocol_error( "init_thread failed with status %x\n", ret );
    if (version != SERVER_PROTOCOL_VERSION)
        server_protocol_error( "version mismatch %d/%d.\n"
                               "Your %s binary was not upgraded correctly,\n"
                               "or you have an older one somewhere in your PATH.\n"
                               "Or maybe the wrong wineserver is still running?\n",
                               version, SERVER_PROTOCOL_VERSION,
                               (version > SERVER_PROTOCOL_VERSION) ? "wine" : "wineserver" );
}


/***********************************************************************
 *           CLIENT_BootDone
 *
 * Signal that we have finished booting, and set debug level.
 */
void CLIENT_BootDone( int debug_level )
{
    SERVER_START_REQ( boot_done )
    {
        req->debug_level = debug_level;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           CLIENT_IsBootThread
 *
 * Return TRUE if current thread is the boot thread.
 */
int CLIENT_IsBootThread(void)
{
    return (GetCurrentThreadId() == (DWORD)boot_thread_id);
}

/*
 * Client part of the client/server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/un.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>

#include "process.h"
#include "thread.h"
#include "server.h"
#include "winerror.h"
#include "options.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

#define SERVERDIR  "/wineserver-"  /* server socket directory (hostname appended) */
#define SOCKETNAME "socket"        /* name of the socket file */

/* data structure used to pass an fd with sendmsg/recvmsg */
struct cmsg_fd
{
    int len;   /* sizeof structure */
    int level; /* SOL_SOCKET */
    int type;  /* SCM_RIGHTS */
    int fd;    /* fd to pass */
};

static void *boot_thread_id;


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
    fprintf( stderr, "Client protocol error:%p: ", NtCurrentTeb()->tid );
    vfprintf( stderr, err, args );
    va_end( args );
    SYSDEPS_ExitThread(1);
}


/***********************************************************************
 *           server_perror
 */
static void server_perror( const char *err )
{
    fprintf( stderr, "Client protocol error:%p: ", NtCurrentTeb()->tid );
    perror( err );
    SYSDEPS_ExitThread(1);
}


/***********************************************************************
 *           send_request
 *
 * Send a request to the server.
 */
static void send_request( enum request req )
{
    int ret;
    if ((ret = write( NtCurrentTeb()->socket, &req, sizeof(req) )) == sizeof(req))
        return;
    if (ret == -1)
    {
        if (errno == EPIPE) SYSDEPS_ExitThread(0);
        server_perror( "sendmsg" );
    }
    server_protocol_error( "partial msg sent %d/%d\n", ret, sizeof(req) );
}

/***********************************************************************
 *           send_request_fd
 *
 * Send a request to the server, passing a file descriptor.
 */
static void send_request_fd( enum request req, int fd )
{
    int ret;
#ifndef HAVE_MSGHDR_ACCRIGHTS
    struct cmsg_fd cmsg;
#endif
    struct msghdr msghdr;
    struct iovec vec;

    vec.iov_base = (void *)&req;
    vec.iov_len  = sizeof(req);

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    cmsg.len   = sizeof(cmsg);
    cmsg.level = SOL_SOCKET;
    cmsg.type  = SCM_RIGHTS;
    cmsg.fd    = fd;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    msghdr.msg_flags      = 0;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    if ((ret = sendmsg( NtCurrentTeb()->socket, &msghdr, 0 )) == sizeof(req)) return;
    if (ret == -1)
    {
        if (errno == EPIPE) SYSDEPS_ExitThread(0);
        server_perror( "sendmsg" );
    }
    server_protocol_error( "partial msg sent %d/%d\n", ret, sizeof(req) );
}

/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
static unsigned int wait_reply(void)
{
    int ret;
    unsigned int res;

    for (;;)
    {
        if ((ret = read( NtCurrentTeb()->socket, &res, sizeof(res) )) == sizeof(res))
            return res;
        if (!ret) break;
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) break;
            server_perror("read");
        }
        server_protocol_error( "partial msg received %d/%d\n", ret, sizeof(res) );
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_ExitThread(0);
}


/***********************************************************************
 *           wait_reply_fd
 *
 * Wait for a reply from the server, when a file descriptor is passed.
 */
static unsigned int wait_reply_fd( int *fd )
{
    struct iovec vec;
    int ret;
    unsigned int res;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr;

    *fd = -1;
    msghdr.msg_accrights    = (void *)fd;
    msghdr.msg_accrightslen = sizeof(*fd);
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    struct msghdr msghdr;
    struct cmsg_fd cmsg;

    cmsg.len   = sizeof(cmsg);
    cmsg.level = SOL_SOCKET;
    cmsg.type  = SCM_RIGHTS;
    cmsg.fd    = -1;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    msghdr.msg_flags      = 0;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;
    vec.iov_base = (void *)&res;
    vec.iov_len  = sizeof(res);

    for (;;)
    {
        if ((ret = recvmsg( NtCurrentTeb()->socket, &msghdr, 0 )) == sizeof(res))
        {
#ifndef HAVE_MSGHDR_ACCRIGHTS
            *fd = cmsg.fd;
#endif
            return res;
        }
        if (!ret) break;
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) break;
            server_perror("recvmsg");
        }
        server_protocol_error( "partial seq received %d/%d\n", ret, sizeof(res) );
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_ExitThread(0);
}


/***********************************************************************
 *           server_call_noerr
 *
 * Perform a server call.
 */
unsigned int server_call_noerr( enum request req )
{
    send_request( req );
    return wait_reply();
}


/***********************************************************************
 *           server_call_fd
 *
 * Perform a server call, passing a file descriptor.
 * If *fd is != -1, it will be passed to the server.
 * If the server passes an fd, it will be stored into *fd.
 */
unsigned int server_call_fd( enum request req, int fd_out, int *fd_in )
{
    unsigned int res;

    if (fd_out == -1) send_request( req );
    else send_request_fd( req, fd_out );

    if (fd_in) res = wait_reply_fd( fd_in );
    else res = wait_reply();
    if (res) SetLastError( RtlNtStatusToDosError(res) );
    return res;  /* error code */
}


/***********************************************************************
 *           start_server
 *
 * Start a new wine server.
 */
static void start_server( const char *oldcwd )
{
    static int started;  /* we only try once */
    if (!started)
    {
        int pid = fork();
        if (pid == -1) fatal_perror( "fork" );
        if (!pid)
        {
            char *path, *p;
            /* first try the installation dir */
            execl( BINDIR "/wineserver", "wineserver", NULL );
            if (oldcwd) chdir( oldcwd );
            /* now try the dir we were launched from */
            if (!(path = malloc( strlen(argv0) + 20 )))
                fatal_error( "out of memory\n" );
            if ((p = strrchr( strcpy( path, argv0 ), '/' )))
            {
                strcpy( p, "/wineserver" );
                execl( path, "wineserver", NULL );
                strcpy( p, "/server/wineserver" );
                execl( path, "wineserver", NULL );
            }
            /* now try the path */
            execlp( "wineserver", "wineserver", NULL );
            /* and finally the current dir */
            execl( "./server/wineserver", "wineserver", NULL );
            fatal_error( "could not exec wineserver\n" );
        }
        started = 1;
    }
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
    int s, slen;

    if (chdir( serverdir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s", serverdir );
        start_server( NULL );
        return -1;
    }
    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", serverdir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", serverdir );
    if (st.st_mode & 077) fatal_error( "'%s' must not be accessible by other users\n", serverdir );

    if (lstat( SOCKETNAME, &st ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "lstat %s/%s", serverdir, SOCKETNAME );
        start_server( oldcwd );
        return -1;
    }
    if (!S_ISSOCK(st.st_mode))
        fatal_error( "'%s/%s' is not a socket\n", serverdir, SOCKETNAME );
    if (st.st_uid != getuid())
        fatal_error( "'%s/%s' is not owned by you\n", serverdir, SOCKETNAME );

    if ((s = socket( AF_UNIX, SOCK_STREAM, 0 )) == -1) fatal_perror( "socket" );
    addr.sun_family = AF_UNIX;
    strcpy( addr.sun_path, SOCKETNAME );
    slen = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
#ifdef HAVE_SOCKADDR_SUN_LEN
    addr.sun_len = slen;
#endif
    if (connect( s, (struct sockaddr *)&addr, slen ) == -1)
    {
        close( s );
        return -2;
    }
    fcntl( s, F_SETFD, 1 ); /* set close on exec flag */
    return s;
}


/***********************************************************************
 *           CLIENT_InitServer
 *
 * Start the server and create the initial socket pair.
 */
int CLIENT_InitServer(void)
{
    int delay, fd, size;
    const char *env_fd;
    char hostname[64];
    char *oldcwd, *serverdir;
    const char *configdir;

    /* first check if we inherited the socket fd */
    if ((env_fd = getenv( "__WINE_FD" )) && isdigit(env_fd[0]))
    {
        fd = atoi( env_fd );
        if (fcntl( fd, F_SETFD, 1 ) != -1) return fd; /* set close on exec flag */
    }

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

    /* get the server directory name */
    if (gethostname( hostname, sizeof(hostname) ) == -1) fatal_perror( "gethostname" );
    configdir = PROFILE_GetConfigDir();
    serverdir = malloc( strlen(configdir) + strlen(SERVERDIR) + strlen(hostname) + 1 );
    if (!serverdir) fatal_error( "out of memory\n" );
    strcpy( serverdir, configdir );
    strcat( serverdir, SERVERDIR );
    strcat( serverdir, hostname );

    /* try to connect, leaving some time for the server to start up */
    for (delay = 10000; delay < 500000; delay += delay / 2)
    {
        if ((fd = server_connect( oldcwd, serverdir )) >= 0) goto done;
        usleep( delay );
    }

    if (fd == -2)
    {
        fatal_error( "'%s/%s' exists,\n"
                     "   but I cannot connect to it; maybe the server has crashed?\n"
                     "   If this is the case, you should remove the socket file and try again.\n",
                     serverdir, SOCKETNAME );
    }
    fatal_error( "could not start wineserver, giving up\n" );

 done:
    /* switch back to the starting directory */
    if (oldcwd)
    {
        chdir( oldcwd );
        free( oldcwd );
    }
    return fd;
}


/***********************************************************************
 *           CLIENT_InitThread
 *
 * Send an init thread request. Return 0 if OK.
 */
int CLIENT_InitThread(void)
{
    struct get_thread_buffer_request *first_req;
    struct init_thread_request *req;
    TEB *teb = NtCurrentTeb();
    int fd;

    if (wait_reply_fd( &fd ) || (fd == -1))
        server_protocol_error( "no fd passed on first request\n" );
    if ((teb->buffer_size = lseek( fd, 0, SEEK_END )) == -1) server_perror( "lseek" );
    teb->buffer = mmap( 0, teb->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    close( fd );
    if (teb->buffer == (void*)-1) server_perror( "mmap" );
    first_req = teb->buffer;
    teb->process->server_pid = first_req->pid;
    teb->tid = first_req->tid;
    if (first_req->version != SERVER_PROTOCOL_VERSION)
        server_protocol_error( "version mismatch %d/%d.\n"
                               "Your %s binary was not upgraded correctly,\n"
                               "or you have an older one somewhere in your PATH.\n",
                               first_req->version, SERVER_PROTOCOL_VERSION,
                               (first_req->version > SERVER_PROTOCOL_VERSION) ? "wine" : "wineserver" );
    if (first_req->boot) boot_thread_id = teb->tid;
    else if (boot_thread_id == teb->tid) boot_thread_id = 0;

    req = teb->buffer;
    req->unix_pid = getpid();
    req->teb      = teb;
    req->entry    = teb->entry_point;
    return server_call_noerr( REQ_INIT_THREAD );
}

/***********************************************************************
 *           CLIENT_BootDone
 *
 * Signal that we have finished booting, and set debug level.
 */
int CLIENT_BootDone( int debug_level )
{
    struct boot_done_request *req = get_req_buffer();
    req->debug_level = debug_level;
    return server_call( REQ_BOOT_DONE );
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

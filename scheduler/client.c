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
#include <pwd.h>
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
#include <sys/un.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>

#include "wine/port.h"
#include "thread.h"
#include "server.h"
#include "winerror.h"
#include "options.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

#define CONFDIR    "/.wine"        /* directory for Wine config relative to $HOME */
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
static sigset_t block_set;  /* signals to block during server calls */
static int fd_socket;  /* socket to exchange file descriptors with the server */

/* die on a fatal error; use only during initialization */
static void fatal_error( const char *err, ... ) WINE_NORETURN;
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
static void fatal_perror( const char *err, ... ) WINE_NORETURN;
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
    fprintf( stderr, "wine client error:%p: ", NtCurrentTeb()->tid );
    vfprintf( stderr, err, args );
    va_end( args );
    SYSDEPS_ExitThread(1);
}


/***********************************************************************
 *           server_protocol_perror
 */
void server_protocol_perror( const char *err )
{
    fprintf( stderr, "wine client error:%p: ", NtCurrentTeb()->tid );
    perror( err );
    SYSDEPS_ExitThread(1);
}


/***********************************************************************
 *           __wine_server_exception_handler (NTDLL.@)
 */
DWORD __wine_server_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                       CONTEXT *context, EXCEPTION_FRAME **pdispatcher )
{
    struct __server_exception_frame *server_frame = (struct __server_exception_frame *)frame;
    if ((record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        NtCurrentTeb()->buffer_pos = server_frame->buffer_pos;
    return ExceptionContinueSearch;
}


/***********************************************************************
 *           wine_server_alloc_req (NTDLL.@)
 */
void wine_server_alloc_req( union generic_request *req, size_t size )
{
    unsigned int pos = NtCurrentTeb()->buffer_pos;

    assert( size <= REQUEST_MAX_VAR_SIZE );

    if (pos + size > NtCurrentTeb()->buffer_size)
        server_protocol_error( "buffer overflow %d bytes\n",
                               pos + size - NtCurrentTeb()->buffer_pos );

    NtCurrentTeb()->buffer_pos = pos + size;
    req->header.var_offset = pos;
    req->header.var_size = size;
}


/***********************************************************************
 *           send_request
 *
 * Send a request to the server.
 */
static void send_request( union generic_request *request )
{
    int ret;

    if ((ret = write( NtCurrentTeb()->request_fd, request, sizeof(*request) )) == sizeof(*request))
        return;
    if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
    if (errno == EPIPE) SYSDEPS_ExitThread(0);
    server_protocol_perror( "sendmsg" );
}

/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
static void wait_reply( union generic_request *req )
{
    int ret;

    for (;;)
    {
        if ((ret = read( NtCurrentTeb()->reply_fd, req, sizeof(*req) )) == sizeof(*req))
            return;
        if (!ret) break;
        if (ret > 0) server_protocol_error( "partial read %d\n", ret );
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("read");
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_ExitThread(0);
}


/***********************************************************************
 *           wine_server_call (NTDLL.@)
 *
 * Perform a server call.
 */
unsigned int wine_server_call( union generic_request *req, size_t size )
{
    sigset_t old_set;

    memset( (char *)req + size, 0, sizeof(*req) - size );
    sigprocmask( SIG_BLOCK, &block_set, &old_set );
    send_request( req );
    wait_reply( req );
    sigprocmask( SIG_SETMASK, &old_set, NULL );
    return req->header.error;
}


/***********************************************************************
 *           wine_server_send_fd
 *
 * Send a file descriptor to the server.
 */
void wine_server_send_fd( int fd )
{
#ifndef HAVE_MSGHDR_ACCRIGHTS
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

    data.tid = (void *)GetCurrentThreadId();
    data.fd  = fd;

    for (;;)
    {
        if ((ret = sendmsg( fd_socket, &msghdr, 0 )) == sizeof(data)) return;
        if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
        if (errno == EINTR) continue;
        if (errno == EPIPE) SYSDEPS_ExitThread(0);
        server_protocol_perror( "sendmsg" );
    }
}


/***********************************************************************
 *           receive_fd
 *
 * Receive a file descriptor passed from the server.
 */
static int receive_fd( handle_t *handle )
{
    struct iovec vec;
    int ret, fd;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr;

    fd = -1;
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
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
    vec.iov_base = (void *)handle;
    vec.iov_len  = sizeof(*handle);

    for (;;)
    {
        if ((ret = recvmsg( fd_socket, &msghdr, 0 )) > 0)
        {
#ifndef HAVE_MSGHDR_ACCRIGHTS
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
    SYSDEPS_ExitThread(0);
}


/***********************************************************************
 *           wine_server_recv_fd
 *
 * Receive a file descriptor passed from the server.
 * The file descriptor must not be closed.
 * Return -2 if a race condition stole our file descriptor.
 */
int wine_server_recv_fd( handle_t handle )
{
    handle_t fd_handle;

    int fd = receive_fd( &fd_handle );

    /* now store it in the server fd cache for this handle */

    SERVER_START_REQ( set_handle_info )
    {
        req->handle = fd_handle;
        req->flags  = 0;
        req->mask   = 0;
        req->fd     = fd;
        if (!SERVER_CALL())
        {
            if (req->cur_fd != fd)
            {
                /* someone was here before us */
                close( fd );
                fd = req->cur_fd;
            }
        }
        else
        {
            close( fd );
            fd = -1;
        }
    }
    SERVER_END_REQ;

    if (handle != fd_handle) fd = -2;  /* not the one we expected */
    return fd;
}


/***********************************************************************
 *           get_config_dir
 *
 * Return the configuration directory ($WINEPREFIX or $HOME/.wine)
 */
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
                execl( p, "wineserver", NULL );
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
                    execl( path, "wineserver", NULL );
                    strcpy( p, "/server/wineserver" );
                    execl( path, "wineserver", NULL );
                }
		free(path);
            }

            /* now try the path */
            execlp( "wineserver", "wineserver", NULL );

            /* and finally the current dir */
            if (!(path = malloc( strlen(oldcwd) + 20 )))
                fatal_error( "out of memory\n" );
            p = strcpy( path, oldcwd ) + strlen( oldcwd );
            strcpy( p, "/wineserver" );
            execl( path, "wineserver", NULL );
            strcpy( p, "/server/wineserver" );
            execl( path, "wineserver", NULL );
            free(path);
            fatal_error( "could not exec wineserver\n" );
        }
        started = 1;
        waitpid( pid, &status, 0 );
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (status) exit(status);  /* server failed */
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

    for (retry = 0; retry < 3; retry++)
    {
        /* if not the first try, wait a bit to leave the server time to exit */
        if (retry) usleep( 100000 * retry * retry );

        /* check for an existing socket */
        if (lstat( SOCKETNAME, &st ) == -1)
        {
            if (errno != ENOENT) fatal_perror( "lstat %s/%s", serverdir, SOCKETNAME );
            start_server( oldcwd );
            if (lstat( SOCKETNAME, &st ) == -1) fatal_perror( "lstat %s/%s", serverdir, SOCKETNAME );
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
#ifdef HAVE_SOCKADDR_SUN_LEN
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
    fatal_error( "file '%s/%s' exists,\n"
                 "   but I cannot connect to it; maybe the wineserver has crashed?\n"
                 "   If this is the case, you should remove this socket file and try again.\n",
                 serverdir, SOCKETNAME );
}


/***********************************************************************
 *           CLIENT_InitServer
 *
 * Start the server and create the initial socket pair.
 */
void CLIENT_InitServer(void)
{
    int size;
    char hostname[64];
    char *oldcwd, *serverdir;
    const char *configdir;
    handle_t dummy_handle;

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

    /* get the server directory name */
    if (gethostname( hostname, sizeof(hostname) ) == -1) fatal_perror( "gethostname" );
    configdir = get_config_dir();
    serverdir = malloc( strlen(configdir) + strlen(SERVERDIR) + strlen(hostname) + 1 );
    if (!serverdir) fatal_error( "out of memory\n" );
    strcpy( serverdir, configdir );
    strcat( serverdir, SERVERDIR );
    strcat( serverdir, hostname );

    /* connect to the server */
    fd_socket = server_connect( oldcwd, serverdir );

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

    /* receive the first thread request fd on the main socket */
    NtCurrentTeb()->request_fd = receive_fd( &dummy_handle );

    CLIENT_InitThread();
}


/***********************************************************************
 *           set_request_buffer
 */
inline static void set_request_buffer(void)
{
    char *name;
    int fd, ret;
    unsigned int offset, size;

    /* create a temporary file */
    do
    {
        if (!(name = tmpnam(NULL))) server_protocol_perror( "tmpnam" );
        fd = open( name, O_CREAT | O_EXCL | O_RDWR, 0600 );
    } while ((fd == -1) && (errno == EEXIST));

    if (fd == -1) server_protocol_perror( "create" );
    unlink( name );

    wine_server_send_fd( fd );

    SERVER_START_REQ( set_thread_buffer )
    {
        req->fd = fd;
        ret = SERVER_CALL();
        offset = req->offset;
        size = req->size;
    }
    SERVER_END_REQ;
    if (ret) server_protocol_error( "set_thread_buffer failed with status %x\n", ret );

    if ((NtCurrentTeb()->buffer = mmap( 0, size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, offset )) == (void*)-1)
        server_protocol_perror( "mmap" );

    close( fd );
    NtCurrentTeb()->buffer_pos  = 0;
    NtCurrentTeb()->buffer_size = size;
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

    /* create the server->client communication pipes */
    if (pipe( reply_pipe ) == -1) server_protocol_perror( "pipe" );
    if (pipe( teb->wait_fd ) == -1) server_protocol_perror( "pipe" );
    wine_server_send_fd( reply_pipe[1] );
    wine_server_send_fd( teb->wait_fd[1] );
    teb->reply_fd = reply_pipe[0];

    /* set close on exec flag */
    fcntl( teb->reply_fd, F_SETFD, 1 );
    fcntl( teb->wait_fd[0], F_SETFD, 1 );
    fcntl( teb->wait_fd[1], F_SETFD, 1 );

    SERVER_START_REQ( init_thread )
    {
        req->unix_pid    = getpid();
        req->teb         = teb;
        req->entry       = teb->entry_point;
        req->reply_fd    = reply_pipe[1];
        req->wait_fd     = teb->wait_fd[1];
        ret = SERVER_CALL();
        teb->pid = req->pid;
        teb->tid = req->tid;
        version  = req->version;
        if (req->boot) boot_thread_id = teb->tid;
        else if (boot_thread_id == teb->tid) boot_thread_id = 0;
        close( reply_pipe[1] );
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
    set_request_buffer();
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
        SERVER_CALL();
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

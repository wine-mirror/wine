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

#undef server_alloc_req

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
 *           __wine_server_exception_handler (NTDLL.@)
 */
DWORD __wine_server_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                       CONTEXT *context, EXCEPTION_FRAME **pdispatcher )
{
    struct __server_exception_frame *server_frame = (struct __server_exception_frame *)frame;
    if ((record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        *NtCurrentTeb()->buffer_info = server_frame->info;
    return ExceptionContinueSearch;
}


/***********************************************************************
 *           wine_server_alloc_req (NTDLL.@)
 */
void *wine_server_alloc_req( size_t fixed_size, size_t var_size )
{
    unsigned int pos = NtCurrentTeb()->buffer_info->cur_pos;
    union generic_request *req = (union generic_request *)((char *)NtCurrentTeb()->buffer + pos);
    size_t size = sizeof(*req) + var_size;

    assert( fixed_size <= sizeof(*req) );
    assert( var_size <= REQUEST_MAX_VAR_SIZE );

    if ((char *)req + size > (char *)NtCurrentTeb()->buffer_info)
        server_protocol_error( "buffer overflow %d bytes\n",
                               (char *)req + size - (char *)NtCurrentTeb()->buffer_info );
    NtCurrentTeb()->buffer_info->cur_pos = pos + size;
    req->header.fixed_size = fixed_size;
    req->header.var_size = var_size;
    return req;
}


/***********************************************************************
 *           send_request
 *
 * Send a request to the server.
 */
static void send_request( enum request req, struct request_header *header )
{
    header->req = req;
    NtCurrentTeb()->buffer_info->cur_req = (char *)header - (char *)NtCurrentTeb()->buffer;
    /* write a single byte; the value is ignored anyway */
    if (write( NtCurrentTeb()->request_fd, header, 1 ) == -1)
    {
        if (errno == EPIPE) SYSDEPS_ExitThread(0);
        server_perror( "sendmsg" );
    }
}

/***********************************************************************
 *           send_request_fd
 *
 * Send a request to the server, passing a file descriptor.
 */
static void send_request_fd( enum request req, struct request_header *header, int fd )
{
#ifndef HAVE_MSGHDR_ACCRIGHTS
    struct cmsg_fd cmsg;
#endif
    struct msghdr msghdr;
    struct iovec vec;

    /* write a single byte; the value is ignored anyway */
    vec.iov_base = (void *)header;
    vec.iov_len  = 1;

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

    header->req = req;

    if (sendmsg( NtCurrentTeb()->socket, &msghdr, 0 ) == -1)
    {
        if (errno == EPIPE) SYSDEPS_ExitThread(0);
        server_perror( "sendmsg" );
    }
}

/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
static void wait_reply(void)
{
    int ret;
    char dummy[1];

    for (;;)
    {
        if ((ret = read( NtCurrentTeb()->reply_fd, dummy, 1 )) > 0) return;
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_perror("read");
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_ExitThread(0);
}


/***********************************************************************
 *           wine_server_call (NTDLL.@)
 *
 * Perform a server call.
 */
unsigned int wine_server_call( enum request req )
{
    void *req_ptr = NtCurrentTeb()->buffer;
    send_request( req, req_ptr );
    wait_reply();
    return ((struct request_header *)req_ptr)->error;
}


/***********************************************************************
 *           server_call_fd
 *
 * Perform a server call, passing a file descriptor.
 */
unsigned int server_call_fd( enum request req, int fd_out )
{
    unsigned int res;
    void *req_ptr = NtCurrentTeb()->buffer;

    send_request_fd( req, req_ptr, fd_out );
    wait_reply();

    if ((res = ((struct request_header *)req_ptr)->error))
        SetLastError( RtlNtStatusToDosError(res) );
    return res;  /* error code */
}


/***********************************************************************
 *           set_handle_fd
 *
 * Store the fd for a given handle in the server
 */
static int set_handle_fd( int handle, int fd )
{
    SERVER_START_REQ
    {
        struct set_handle_info_request *req = wine_server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->flags  = 0;
        req->mask   = 0;
        req->fd     = fd;
        if (!server_call( REQ_SET_HANDLE_INFO ))
        {
            if (req->cur_fd != fd)
            {
                /* someone was here before us */
                close( fd );
                fd = req->cur_fd;
            }
            else fcntl( fd, F_SETFD, 1 ); /* set close on exec flag */
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
 *           wine_server_recv_fd
 *
 * Receive a file descriptor passed from the server.
 * The file descriptor must be closed after use.
 */
int wine_server_recv_fd( int handle, int cache )
{
    struct iovec vec;
    int ret, fd, server_handle;

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
    vec.iov_base = (void *)&server_handle;
    vec.iov_len  = sizeof(server_handle);

    for (;;)
    {
        if ((ret = recvmsg( NtCurrentTeb()->socket, &msghdr, 0 )) > 0)
        {
#ifndef HAVE_MSGHDR_ACCRIGHTS
            fd = cmsg.fd;
#endif
            if (handle != server_handle)
                server_protocol_error( "recv_fd: got handle %d, expected %d\n",
                                       server_handle, handle );
            if (cache)
            {
                fd = set_handle_fd( handle, fd );
                if (fd != -1) fd = dup(fd);
            }
            return fd;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_perror("recvmsg");
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_ExitThread(0);
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
int CLIENT_InitServer(void)
{
    int fd, size;
    char hostname[64];
    char *oldcwd, *serverdir;
    const char *configdir;

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
    fd = server_connect( oldcwd, serverdir );

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
    struct get_thread_buffer_request *req;
    TEB *teb = NtCurrentTeb();
    int fd, ret, size;

    /* ignore SIGPIPE so that we get a EPIPE error instead  */
    signal( SIGPIPE, SIG_IGN );

    teb->request_fd = wine_server_recv_fd( 0, 0 );
    if (teb->request_fd == -1) server_protocol_error( "no request fd passed on first request\n" );
    fcntl( teb->request_fd, F_SETFD, 1 ); /* set close on exec flag */

    teb->reply_fd = wine_server_recv_fd( 0, 0 );
    if (teb->reply_fd == -1) server_protocol_error( "no reply fd passed on first request\n" );
    fcntl( teb->reply_fd, F_SETFD, 1 ); /* set close on exec flag */

    fd = wine_server_recv_fd( 0, 0 );
    if (fd == -1) server_protocol_error( "no fd received for thread buffer\n" );

    if ((size = lseek( fd, 0, SEEK_END )) == -1) server_perror( "lseek" );
    teb->buffer = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    close( fd );
    if (teb->buffer == (void*)-1) server_perror( "mmap" );
    teb->buffer_info = (struct server_buffer_info *)((char *)teb->buffer + size) - 1;

    wait_reply();

    req = (struct get_thread_buffer_request *)teb->buffer;
    teb->pid = req->pid;
    teb->tid = req->tid;
    if (req->version != SERVER_PROTOCOL_VERSION)
        server_protocol_error( "version mismatch %d/%d.\n"
                               "Your %s binary was not upgraded correctly,\n"
                               "or you have an older one somewhere in your PATH.\nOr maybe wrong wineserver still running ?",
                               req->version, SERVER_PROTOCOL_VERSION,
                               (req->version > SERVER_PROTOCOL_VERSION) ? "wine" : "wineserver" );
    if (req->boot) boot_thread_id = teb->tid;
    else if (boot_thread_id == teb->tid) boot_thread_id = 0;

    SERVER_START_REQ
    {
        struct init_thread_request *req = wine_server_alloc_req( sizeof(*req), 0 );
        req->unix_pid = getpid();
        req->teb      = teb;
        req->entry    = teb->entry_point;
        ret = wine_server_call( REQ_INIT_THREAD );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           CLIENT_BootDone
 *
 * Signal that we have finished booting, and set debug level.
 */
int CLIENT_BootDone( int debug_level )
{
    int ret;
    SERVER_START_REQ
    {
        struct boot_done_request *req = wine_server_alloc_req( sizeof(*req), 0 );
        req->debug_level = debug_level;
        ret = server_call( REQ_BOOT_DONE );
    }
    SERVER_END_REQ;
    return ret;
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

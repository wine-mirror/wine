/*
 * Wine server communication
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
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
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
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/library.h"
#include "wine/pthread.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(server);

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
    struct
    {
        size_t len;   /* size of structure */
        int    level; /* SOL_SOCKET */
        int    type;  /* SCM_RIGHTS */
    } header;
    int fd;          /* fd to pass */
};
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

timeout_t server_start_time = 0;  /* time of server startup */

extern struct wine_pthread_functions pthread_functions;

sigset_t server_block_set;  /* signals to block during server calls */
static int fd_socket = -1;  /* socket to exchange file descriptors with the server */

static RTL_CRITICAL_SECTION fd_cache_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &fd_cache_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": fd_cache_section") }
};
static RTL_CRITICAL_SECTION fd_cache_section = { &critsect_debug, -1, 0, 0, 0, 0 };


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
 *           server_exit_thread
 */
void server_exit_thread( int status )
{
    struct wine_pthread_thread_info info;
    SIZE_T size;
    int fds[4];

    RtlAcquirePebLock();
    RemoveEntryList( &NtCurrentTeb()->TlsLinks );
    RtlReleasePebLock();

    info.stack_base  = NtCurrentTeb()->DeallocationStack;
    info.teb_base    = NtCurrentTeb();
    info.teb_sel     = wine_get_fs();
    info.exit_status = status;

    fds[0] = ntdll_get_thread_data()->wait_fd[0];
    fds[1] = ntdll_get_thread_data()->wait_fd[1];
    fds[2] = ntdll_get_thread_data()->reply_fd;
    fds[3] = ntdll_get_thread_data()->request_fd;
    pthread_functions.sigprocmask( SIG_BLOCK, &server_block_set, NULL );

    size = 0;
    NtFreeVirtualMemory( GetCurrentProcess(), &info.stack_base, &size, MEM_RELEASE | MEM_SYSTEM );
    info.stack_size = size;

    size = 0;
    NtFreeVirtualMemory( GetCurrentProcess(), &info.teb_base, &size, MEM_RELEASE | MEM_SYSTEM );
    info.teb_size = size;

    close( fds[0] );
    close( fds[1] );
    close( fds[2] );
    close( fds[3] );
    pthread_functions.exit_thread( &info );
}


/***********************************************************************
 *           server_abort_thread
 */
void server_abort_thread( int status )
{
    pthread_functions.sigprocmask( SIG_BLOCK, &server_block_set, NULL );
    close( ntdll_get_thread_data()->wait_fd[0] );
    close( ntdll_get_thread_data()->wait_fd[1] );
    close( ntdll_get_thread_data()->reply_fd );
    close( ntdll_get_thread_data()->request_fd );
    pthread_functions.abort_thread( status );
}


/***********************************************************************
 *           server_protocol_error
 */
void server_protocol_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    vfprintf( stderr, err, args );
    va_end( args );
    server_abort_thread(1);
}


/***********************************************************************
 *           server_protocol_perror
 */
void server_protocol_perror( const char *err )
{
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    perror( err );
    server_abort_thread(1);
}


/***********************************************************************
 *           send_request
 *
 * Send a request to the server.
 */
static unsigned int send_request( const struct __server_request_info *req )
{
    unsigned int i;
    int ret;

    if (!req->u.req.request_header.request_size)
    {
        if ((ret = write( ntdll_get_thread_data()->request_fd, &req->u.req,
                          sizeof(req->u.req) )) == sizeof(req->u.req)) return STATUS_SUCCESS;

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
        if ((ret = writev( ntdll_get_thread_data()->request_fd, vec, i+1 )) ==
            req->u.req.request_header.request_size + sizeof(req->u.req)) return STATUS_SUCCESS;
    }

    if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
    if (errno == EPIPE) server_abort_thread(0);
    if (errno == EFAULT) return STATUS_ACCESS_VIOLATION;
    server_protocol_perror( "write" );
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
        if ((ret = read( ntdll_get_thread_data()->reply_fd, buffer, size )) > 0)
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
    server_abort_thread(0);
}


/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
static inline unsigned int wait_reply( struct __server_request_info *req )
{
    read_reply_data( &req->u.reply, sizeof(req->u.reply) );
    if (req->u.reply.reply_header.reply_size)
        read_reply_data( req->reply_data, req->u.reply.reply_header.reply_size );
    return req->u.reply.reply_header.error;
}


/***********************************************************************
 *           wine_server_call (NTDLL.@)
 *
 * Perform a server call.
 *
 * PARAMS
 *     req_ptr [I/O] Function dependent data
 *
 * RETURNS
 *     Depends on server function being called, but usually an NTSTATUS code.
 *
 * NOTES
 *     Use the SERVER_START_REQ and SERVER_END_REQ to help you fill out the
 *     server request structure for the particular call. E.g:
 *|     SERVER_START_REQ( event_op )
 *|     {
 *|         req->handle = handle;
 *|         req->op     = SET_EVENT;
 *|         ret = wine_server_call( req );
 *|     }
 *|     SERVER_END_REQ;
 */
unsigned int wine_server_call( void *req_ptr )
{
    struct __server_request_info * const req = req_ptr;
    sigset_t old_set;
    unsigned int ret;

    pthread_functions.sigprocmask( SIG_BLOCK, &server_block_set, &old_set );
    ret = send_request( req );
    if (!ret) ret = wait_reply( req );
    pthread_functions.sigprocmask( SIG_SETMASK, &old_set, NULL );
    return ret;
}


/***********************************************************************
 *           server_enter_uninterrupted_section
 */
void server_enter_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset )
{
    pthread_functions.sigprocmask( SIG_BLOCK, &server_block_set, sigset );
    RtlEnterCriticalSection( cs );
}


/***********************************************************************
 *           server_leave_uninterrupted_section
 */
void server_leave_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset )
{
    RtlLeaveCriticalSection( cs );
    pthread_functions.sigprocmask( SIG_SETMASK, sigset, NULL );
}


/***********************************************************************
 *           wine_server_send_fd   (NTDLL.@)
 *
 * Send a file descriptor to the server.
 *
 * PARAMS
 *     fd [I] file descriptor to send
 *
 * RETURNS
 *     nothing
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
    cmsg.header.len   = sizeof(cmsg.header) + sizeof(fd);
    cmsg.header.level = SOL_SOCKET;
    cmsg.header.type  = SCM_RIGHTS;
    cmsg.fd           = fd;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg.header) + sizeof(fd);
    msghdr.msg_flags      = 0;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    data.tid = GetCurrentThreadId();
    data.fd  = fd;

    for (;;)
    {
        if ((ret = sendmsg( fd_socket, &msghdr, 0 )) == sizeof(data)) return;
        if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
        if (errno == EINTR) continue;
        if (errno == EPIPE) server_abort_thread(0);
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

    cmsg.header.len   = sizeof(cmsg.header) + sizeof(fd);
    cmsg.header.level = SOL_SOCKET;
    cmsg.header.type  = SCM_RIGHTS;
    cmsg.fd           = -1;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg.header) + sizeof(fd);
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
            if (fd != -1) fcntl( fd, F_SETFD, 1 ); /* set close on exec flag */
            return fd;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("recvmsg");
    }
    /* the server closed the connection; time to die... */
    server_abort_thread(0);
}


/***********************************************************************/
/* fd cache support */

struct fd_cache_entry
{
    int fd;
    enum server_fd_type type : 6;
    unsigned int        access : 2;
    unsigned int        options : 24;
};

#define FD_CACHE_BLOCK_SIZE  (65536 / sizeof(struct fd_cache_entry))
#define FD_CACHE_ENTRIES     128

static struct fd_cache_entry *fd_cache[FD_CACHE_ENTRIES];
static struct fd_cache_entry fd_cache_initial_block[FD_CACHE_BLOCK_SIZE];

static inline unsigned int handle_to_index( obj_handle_t handle, unsigned int *entry )
{
    unsigned long idx = ((unsigned long)handle >> 2) - 1;
    *entry = idx / FD_CACHE_BLOCK_SIZE;
    return idx % FD_CACHE_BLOCK_SIZE;
}


/***********************************************************************
 *           add_fd_to_cache
 *
 * Caller must hold fd_cache_section.
 */
static int add_fd_to_cache( obj_handle_t handle, int fd, enum server_fd_type type,
                            unsigned int access, unsigned int options )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int prev_fd;

    if (entry >= FD_CACHE_ENTRIES)
    {
        FIXME( "too many allocated handles, not caching %p\n", handle );
        return 0;
    }

    if (!fd_cache[entry])  /* do we need to allocate a new block of entries? */
    {
        if (!entry) fd_cache[0] = fd_cache_initial_block;
        else
        {
            void *ptr = wine_anon_mmap( NULL, FD_CACHE_BLOCK_SIZE * sizeof(struct fd_cache_entry),
                                        PROT_READ | PROT_WRITE, 0 );
            if (ptr == MAP_FAILED) return 0;
            fd_cache[entry] = ptr;
        }
    }
    /* store fd+1 so that 0 can be used as the unset value */
    prev_fd = interlocked_xchg( &fd_cache[entry][idx].fd, fd + 1 ) - 1;
    fd_cache[entry][idx].type = type;
    fd_cache[entry][idx].access = access;
    fd_cache[entry][idx].options = options;
    if (prev_fd != -1) close( prev_fd );
    return 1;
}


/***********************************************************************
 *           get_cached_fd
 *
 * Caller must hold fd_cache_section.
 */
static inline int get_cached_fd( obj_handle_t handle, enum server_fd_type *type,
                                 unsigned int *access, unsigned int *options )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int fd = -1;

    if (entry < FD_CACHE_ENTRIES && fd_cache[entry])
    {
        fd = fd_cache[entry][idx].fd - 1;
        if (type) *type = fd_cache[entry][idx].type;
        if (access) *access = fd_cache[entry][idx].access;
        if (options) *options = fd_cache[entry][idx].options;
    }
    return fd;
}


/***********************************************************************
 *           server_remove_fd_from_cache
 */
int server_remove_fd_from_cache( obj_handle_t handle )
{
    unsigned int entry, idx = handle_to_index( handle, &entry );
    int fd = -1;

    if (entry < FD_CACHE_ENTRIES && fd_cache[entry])
        fd = interlocked_xchg( &fd_cache[entry][idx].fd, 0 ) - 1;

    return fd;
}


/***********************************************************************
 *           server_get_unix_fd
 *
 * The returned unix_fd should be closed iff needs_close is non-zero.
 */
int server_get_unix_fd( obj_handle_t handle, unsigned int wanted_access, int *unix_fd,
                        int *needs_close, enum server_fd_type *type, unsigned int *options )
{
    sigset_t sigset;
    obj_handle_t fd_handle;
    int ret = 0, fd;
    unsigned int access = 0;

    *unix_fd = -1;
    *needs_close = 0;
    wanted_access &= FILE_READ_DATA | FILE_WRITE_DATA;

    server_enter_uninterrupted_section( &fd_cache_section, &sigset );

    fd = get_cached_fd( handle, type, &access, options );
    if (fd != -1) goto done;

    SERVER_START_REQ( get_handle_fd )
    {
        req->handle = handle;
        if (!(ret = wine_server_call( req )))
        {
            if (type) *type = reply->type;
            if (options) *options = reply->options;
            access = reply->access;
            if ((fd = receive_fd( &fd_handle )) != -1)
            {
                assert( fd_handle == handle );
                *needs_close = (reply->removable ||
                                !add_fd_to_cache( handle, fd, reply->type,
                                                  reply->access, reply->options ));
            }
            else ret = STATUS_TOO_MANY_OPENED_FILES;
        }
    }
    SERVER_END_REQ;

done:
    server_leave_uninterrupted_section( &fd_cache_section, &sigset );
    if (!ret && ((access & wanted_access) != wanted_access))
    {
        ret = STATUS_ACCESS_DENIED;
        if (*needs_close) close( fd );
    }
    if (!ret) *unix_fd = fd;
    return ret;
}


/***********************************************************************
 *           wine_server_fd_to_handle   (NTDLL.@)
 *
 * Allocate a file handle for a Unix file descriptor.
 *
 * PARAMS
 *     fd      [I] Unix file descriptor.
 *     access  [I] Win32 access flags.
 *     attributes [I] Object attributes.
 *     handle  [O] Address where Wine file handle will be stored.
 *
 * RETURNS
 *     NTSTATUS code
 */
int wine_server_fd_to_handle( int fd, unsigned int access, unsigned int attributes, obj_handle_t *handle )
{
    int ret;

    *handle = 0;
    wine_server_send_fd( fd );

    SERVER_START_REQ( alloc_file_handle )
    {
        req->access     = access;
        req->attributes = attributes;
        req->fd         = fd;
        if (!(ret = wine_server_call( req ))) *handle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           wine_server_handle_to_fd   (NTDLL.@)
 *
 * Retrieve the file descriptor corresponding to a file handle.
 *
 * PARAMS
 *     handle  [I] Wine file handle.
 *     access  [I] Win32 file access rights requested.
 *     unix_fd [O] Address where Unix file descriptor will be stored.
 *     options [O] Address where the file open options will be stored. Optional.
 *
 * RETURNS
 *     NTSTATUS code
 */
int wine_server_handle_to_fd( obj_handle_t handle, unsigned int access, int *unix_fd,
                              unsigned int *options )
{
    int needs_close, ret = server_get_unix_fd( handle, access, unix_fd, &needs_close, NULL, options );

    if (!ret && !needs_close)
    {
        if ((*unix_fd = dup(*unix_fd)) == -1) ret = FILE_GetNtStatus();
    }
    return ret;
}


/***********************************************************************
 *           wine_server_release_fd   (NTDLL.@)
 *
 * Release the Unix file descriptor returned by wine_server_handle_to_fd.
 *
 * PARAMS
 *     handle  [I] Wine file handle.
 *     unix_fd [I] Unix file descriptor to release.
 *
 * RETURNS
 *     nothing
 */
void wine_server_release_fd( obj_handle_t handle, int unix_fd )
{
    close( unix_fd );
}


/***********************************************************************
 *           start_server
 *
 * Start a new wine server.
 */
static void start_server(void)
{
    static int started;  /* we only try once */
    char *argv[3];
    static char wineserver[] = "server/wineserver";
    static char debug[] = "-d";

    if (!started)
    {
        int status;
        int pid = fork();
        if (pid == -1) fatal_perror( "fork" );
        if (!pid)
        {
            argv[0] = wineserver;
            argv[1] = TRACE_ON(server) ? debug : NULL;
            argv[2] = NULL;
            wine_exec_wine_binary( argv[0], argv, getenv("WINESERVER") );
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
static int server_connect( const char *serverdir )
{
    struct sockaddr_un addr;
    struct stat st;
    int s, slen, retry, fd_cwd;

    /* retrieve the current directory */
    fd_cwd = open( ".", O_RDONLY );
    if (fd_cwd != -1) fcntl( fd_cwd, F_SETFD, 1 ); /* set close on exec flag */

    /* chdir to the server directory */
    if (chdir( serverdir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s", serverdir );
        start_server();
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
            start_server();
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }
        else if (lstat( SOCKETNAME, &st ) == -1) /* check for an already existing socket */
        {
            if (errno != ENOENT) fatal_perror( "lstat %s/%s", serverdir, SOCKETNAME );
            start_server();
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
            /* switch back to the starting directory */
            if (fd_cwd != -1)
            {
                fchdir( fd_cwd );
                close( fd_cwd );
            }
            fcntl( s, F_SETFD, 1 ); /* set close on exec flag */
            return s;
        }
        close( s );
    }
    server_connect_error( serverdir );
}


/***********************************************************************
 *           rm_rf
 *
 * Remove a directory and all its contents; helper for create_config_dir.
 */
static void rm_rf( const char *path )
{
    int err = errno;  /* preserve errno */
    DIR *dir;
    char *buffer, *p;
    struct stat st;
    struct dirent *de;

    if (!(buffer = malloc( strlen(path) + 256 + 1 ))) goto done;
    strcpy( buffer, path );
    p = buffer + strlen(buffer);
    *p++ = '/';

    if ((dir = opendir( path )))
    {
        while ((de = readdir( dir )))
        {
            if (!strcmp( de->d_name, "." ) || !strcmp( de->d_name, ".." )) continue;
            strcpy( p, de->d_name );
            if (unlink( buffer ) != -1) continue;
            if (errno == EISDIR ||
                (errno == EPERM && !lstat( buffer, &st ) && S_ISDIR(st.st_mode)))
            {
                /* recurse in the sub-directory */
                rm_rf( buffer );
            }
        }
        closedir( dir );
    }
    free( buffer );
    rmdir( path );
done:
    errno = err;
}


/***********************************************************************
 *           create_config_dir
 *
 * Create the wine configuration dir (~/.wine).
 */
static void create_config_dir(void)
{
    const char *config_dir = wine_get_config_dir();
    char *tmp_dir;
    int fd;
    pid_t pid, wret;

    if (!(tmp_dir = malloc( strlen(config_dir) + sizeof("-XXXXXX") )))
        fatal_error( "out of memory\n" );
    strcpy( tmp_dir, config_dir );
    strcat( tmp_dir, "-XXXXXX" );
    if ((fd = mkstemps( tmp_dir, 0 )) == -1)
        fatal_perror( "can't get temp file name for %s", config_dir );
    close( fd );
    unlink( tmp_dir );
    if (mkdir( tmp_dir, 0777 ) == -1)
        fatal_perror( "cannot create temp dir %s", tmp_dir );

    MESSAGE( "wine: creating configuration directory '%s'...\n", config_dir );
    pid = fork();
    if (pid == -1)
    {
        rmdir( tmp_dir );
        fatal_perror( "fork" );
    }
    if (!pid)
    {
        char *argv[6];
        static char argv0[] = "tools/wineprefixcreate",
                    argv1[] = "--quiet",
                    argv2[] = "--wait",
                    argv3[] = "--prefix";

        argv[0] = argv0;
        argv[1] = argv1;
        argv[2] = argv2;
        argv[3] = argv3;
        argv[4] = tmp_dir;
        argv[5] = NULL;
        wine_exec_wine_binary( argv[0], argv, NULL );
        rmdir( tmp_dir );
        fatal_perror( "could not exec wineprefixcreate" );
    }
    else
    {
        int status;

        while ((wret = waitpid( pid, &status, 0 )) != pid)
        {
            if (wret == -1 && errno != EINTR) fatal_perror( "wait4" );
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status))
        {
            rm_rf( tmp_dir );
            fatal_error( "wineprefixcreate failed while creating '%s'.\n", config_dir );
        }
    }
    if (rename( tmp_dir, config_dir ) == -1)
    {
        rm_rf( tmp_dir );
        if (errno != EEXIST && errno != ENOTEMPTY)
            fatal_perror( "rename '%s' to '%s'", tmp_dir, config_dir );
        /* else it was probably created by a concurrent wine process */
    }
    free( tmp_dir );
    MESSAGE( "wine: '%s' created successfully.\n", config_dir );
}


#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>

/* send our task port to the server */
static void send_server_task_port(void)
{
    mach_port_t bootstrap_port, wineserver_port;
    kern_return_t kret;

    struct {
        mach_msg_header_t           header;
        mach_msg_body_t             body;
        mach_msg_port_descriptor_t  task_port;
    } msg;

    if (task_get_bootstrap_port(mach_task_self(), &bootstrap_port) != KERN_SUCCESS) return;

    kret = bootstrap_look_up(bootstrap_port, (char*)wine_get_server_dir(), &wineserver_port);
    if (kret != KERN_SUCCESS)
        fatal_error( "cannot find the server port: 0x%08x\n", kret );

    mach_port_deallocate(mach_task_self(), bootstrap_port);

    msg.header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0) | MACH_MSGH_BITS_COMPLEX;
    msg.header.msgh_size        = sizeof(msg);
    msg.header.msgh_remote_port = wineserver_port;
    msg.header.msgh_local_port  = MACH_PORT_NULL;

    msg.body.msgh_descriptor_count  = 1;
    msg.task_port.name              = mach_task_self();
    msg.task_port.disposition       = MACH_MSG_TYPE_COPY_SEND;
    msg.task_port.type              = MACH_MSG_PORT_DESCRIPTOR;

    kret = mach_msg_send(&msg.header);
    if (kret != KERN_SUCCESS)
        server_protocol_error( "mach_msg_send failed: 0x%08x\n", kret );

    mach_port_deallocate(mach_task_self(), wineserver_port);
}
#endif  /* __APPLE__ */

/***********************************************************************
 *           server_init_process
 *
 * Start the server and create the initial socket pair.
 */
void server_init_process(void)
{
    obj_handle_t dummy_handle;
    const char *env_socket = getenv( "WINESERVERSOCKET" );

    if (env_socket)
    {
        fd_socket = atoi( env_socket );
        if (fcntl( fd_socket, F_SETFD, 1 ) == -1)
            fatal_perror( "Bad server socket %d", fd_socket );
        unsetenv( "WINESERVERSOCKET" );
    }
    else
    {
        const char *server_dir = wine_get_server_dir();

        if (!server_dir)  /* this means the config dir doesn't exist */
        {
            create_config_dir();
            server_dir = wine_get_server_dir();
        }

        /* connect to the server */
        fd_socket = server_connect( server_dir );
    }

    /* setup the signal mask */
    sigemptyset( &server_block_set );
    sigaddset( &server_block_set, SIGALRM );
    sigaddset( &server_block_set, SIGIO );
    sigaddset( &server_block_set, SIGINT );
    sigaddset( &server_block_set, SIGHUP );
    sigaddset( &server_block_set, SIGUSR1 );
    sigaddset( &server_block_set, SIGUSR2 );
    sigaddset( &server_block_set, SIGCHLD );
    pthread_functions.sigprocmask( SIG_BLOCK, &server_block_set, NULL );

    /* receive the first thread request fd on the main socket */
    ntdll_get_thread_data()->request_fd = receive_fd( &dummy_handle );

#ifdef __APPLE__
    send_server_task_port();
#endif
}


/***********************************************************************
 *           server_init_process_done
 */
NTSTATUS server_init_process_done(void)
{
    PEB *peb = NtCurrentTeb()->Peb;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( peb->ImageBaseAddress );
    NTSTATUS status;

    /* Install signal handlers; this cannot be done earlier, since we cannot
     * send exceptions to the debugger before the create process event that
     * is sent by REQ_INIT_PROCESS_DONE.
     * We do need the handlers in place by the time the request is over, so
     * we set them up here. If we segfault between here and the server call
     * something is very wrong... */
    if (!SIGNAL_Init()) exit(1);

    /* Signal the parent process to continue */
    SERVER_START_REQ( init_process_done )
    {
        req->module = peb->ImageBaseAddress;
        req->entry  = (char *)peb->ImageBaseAddress + nt->OptionalHeader.AddressOfEntryPoint;
        req->gui    = (nt->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_CUI);
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    pthread_functions.sigprocmask( SIG_UNBLOCK, &server_block_set, NULL );
    return status;
}


/***********************************************************************
 *           server_init_thread
 *
 * Send an init thread request. Return 0 if OK.
 */
size_t server_init_thread( int unix_pid, int unix_tid, void *entry_point )
{
    int version, ret;
    int reply_pipe[2];
    struct sigaction sig_act;
    size_t info_size;

    sig_act.sa_handler = SIG_IGN;
    sig_act.sa_flags   = 0;
    sigemptyset( &sig_act.sa_mask );

    /* ignore SIGPIPE so that we get an EPIPE error instead  */
    sigaction( SIGPIPE, &sig_act, NULL );
    /* automatic child reaping to avoid zombies */
#ifdef SA_NOCLDWAIT
    sig_act.sa_flags |= SA_NOCLDWAIT;
#endif
    sigaction( SIGCHLD, &sig_act, NULL );

    /* create the server->client communication pipes */
    if (pipe( reply_pipe ) == -1) server_protocol_perror( "pipe" );
    if (pipe( ntdll_get_thread_data()->wait_fd ) == -1) server_protocol_perror( "pipe" );
    wine_server_send_fd( reply_pipe[1] );
    wine_server_send_fd( ntdll_get_thread_data()->wait_fd[1] );
    ntdll_get_thread_data()->reply_fd = reply_pipe[0];
    close( reply_pipe[1] );

    /* set close on exec flag */
    fcntl( ntdll_get_thread_data()->reply_fd, F_SETFD, 1 );
    fcntl( ntdll_get_thread_data()->wait_fd[0], F_SETFD, 1 );
    fcntl( ntdll_get_thread_data()->wait_fd[1], F_SETFD, 1 );

    SERVER_START_REQ( init_thread )
    {
        req->unix_pid    = unix_pid;
        req->unix_tid    = unix_tid;
        req->teb         = NtCurrentTeb();
        req->peb         = NtCurrentTeb()->Peb;
        req->entry       = entry_point;
        req->ldt_copy    = &wine_ldt_copy;
        req->reply_fd    = reply_pipe[1];
        req->wait_fd     = ntdll_get_thread_data()->wait_fd[1];
        req->debug_level = (TRACE_ON(server) != 0);
        ret = wine_server_call( req );
        NtCurrentTeb()->ClientId.UniqueProcess = ULongToHandle(reply->pid);
        NtCurrentTeb()->ClientId.UniqueThread  = ULongToHandle(reply->tid);
        info_size         = reply->info_size;
        version           = reply->version;
        server_start_time = reply->server_start;
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
    return info_size;
}

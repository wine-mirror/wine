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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_LWP_H
#include <lwp.h>
#endif
#ifdef HAVE_PTHREAD_NP_H
# include <pthread_np.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
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
#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_THR_H
#include <sys/thr.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef __APPLE__
#include <crt_externs.h>
#include <spawn.h>
#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR 0x0100
#endif
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "unix_private.h"
#include "ddk/wdm.h"

WINE_DECLARE_DEBUG_CHANNEL(server);

#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0
#endif

#define SOCKETNAME "socket"        /* name of the socket file */
#define LOCKNAME   "lock"          /* name of the lock file */

#ifdef __i386__
static const enum cpu_type client_cpu = CPU_x86;
#elif defined(__x86_64__)
static const enum cpu_type client_cpu = CPU_x86_64;
#elif defined(__powerpc__)
static const enum cpu_type client_cpu = CPU_POWERPC;
#elif defined(__arm__)
static const enum cpu_type client_cpu = CPU_ARM;
#elif defined(__aarch64__)
static const enum cpu_type client_cpu = CPU_ARM64;
#else
#error Unsupported CPU
#endif

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static const char *server_dir;

unsigned int server_cpus = 0;
BOOL is_wow64 = FALSE;

timeout_t server_start_time = 0;  /* time of server startup */

sigset_t server_block_set;  /* signals to block during server calls */
static int fd_socket = -1;  /* socket to exchange file descriptors with the server */
static pid_t server_pid;

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
static DECLSPEC_NORETURN void server_protocol_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}


/***********************************************************************
 *           server_protocol_perror
 */
static DECLSPEC_NORETURN void server_protocol_perror( const char *err )
{
    fprintf( stderr, "wine client error:%x: ", GetCurrentThreadId() );
    perror( err );
    exit(1);
}


/***********************************************************************
 *           server_send_fd
 *
 * Send a file descriptor to the server.
 */
void CDECL server_send_fd( int fd )
{
    struct send_fd data;
    struct msghdr msghdr;
    struct iovec vec;
    int ret;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    char cmsg_buffer[256];
    struct cmsghdr *cmsg;
    msghdr.msg_control    = cmsg_buffer;
    msghdr.msg_controllen = sizeof(cmsg_buffer);
    msghdr.msg_flags      = 0;
    cmsg = CMSG_FIRSTHDR( &msghdr );
    cmsg->cmsg_len   = CMSG_LEN( sizeof(fd) );
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    *(int *)CMSG_DATA(cmsg) = fd;
    msghdr.msg_controllen = cmsg->cmsg_len;
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;

    vec.iov_base = (void *)&data;
    vec.iov_len  = sizeof(data);

    data.tid = GetCurrentThreadId();
    data.fd  = fd;

    for (;;)
    {
        if ((ret = sendmsg( fd_socket, &msghdr, 0 )) == sizeof(data)) return;
        if (ret >= 0) server_protocol_error( "partial write %d\n", ret );
        if (errno == EINTR) continue;
        if (errno == EPIPE) NtTerminateThread( GetCurrentThread(), 0 );
        server_protocol_perror( "sendmsg" );
    }
}


/***********************************************************************
 *           receive_fd
 *
 * Receive a file descriptor passed from the server.
 */
int CDECL receive_fd( obj_handle_t *handle )
{
    struct iovec vec;
    struct msghdr msghdr;
    int ret, fd = -1;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(fd);
#else  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
    char cmsg_buffer[256];
    msghdr.msg_control    = cmsg_buffer;
    msghdr.msg_controllen = sizeof(cmsg_buffer);
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
        if ((ret = recvmsg( fd_socket, &msghdr, MSG_CMSG_CLOEXEC )) > 0)
        {
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
            struct cmsghdr *cmsg;
            for (cmsg = CMSG_FIRSTHDR( &msghdr ); cmsg; cmsg = CMSG_NXTHDR( &msghdr, cmsg ))
            {
                if (cmsg->cmsg_level != SOL_SOCKET) continue;
                if (cmsg->cmsg_type == SCM_RIGHTS) fd = *(int *)CMSG_DATA(cmsg);
#ifdef SCM_CREDENTIALS
                else if (cmsg->cmsg_type == SCM_CREDENTIALS)
                {
                    struct ucred *ucred = (struct ucred *)CMSG_DATA(cmsg);
                    server_pid = ucred->pid;
                }
#endif
            }
#endif  /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */
            if (fd != -1) fcntl( fd, F_SETFD, FD_CLOEXEC ); /* in case MSG_CMSG_CLOEXEC is not supported */
            return fd;
        }
        if (!ret) break;
        if (errno == EINTR) continue;
        if (errno == EPIPE) break;
        server_protocol_perror("recvmsg");
    }
    /* the server closed the connection; time to die... */
    for (;;) NtTerminateThread( GetCurrentThread(), 0 );
}


/***********************************************************************
 *           server_pipe
 *
 * Create a pipe for communicating with the server.
 */
int CDECL server_pipe( int fd[2] )
{
    int ret;
#ifdef HAVE_PIPE2
    static BOOL have_pipe2 = TRUE;

    if (have_pipe2)
    {
        if (!(ret = pipe2( fd, O_CLOEXEC ))) return ret;
        if (errno == ENOSYS || errno == EINVAL) have_pipe2 = FALSE;  /* don't try again */
    }
#endif
    if (!(ret = pipe( fd )))
    {
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }
    return ret;
}


/***********************************************************************
 *           init_server_dir
 */
static const char *init_server_dir( dev_t dev, ino_t ino )
{
    char *p, *dir;
    size_t len = sizeof("/server-") + 2 * sizeof(dev) + 2 * sizeof(ino) + 2;

#ifdef __ANDROID__  /* there's no /tmp dir on Android */
    len += strlen( config_dir ) + sizeof("/.wineserver");
    dir = malloc( len );
    strcpy( dir, config_dir );
    strcat( dir, "/.wineserver/server-" );
#else
    len += sizeof("/tmp/.wine-") + 12;
    dir = malloc( len );
    sprintf( dir, "/tmp/.wine-%u/server-", getuid() );
#endif
    p = dir + strlen( dir );
    if (dev != (unsigned long)dev)
        p += sprintf( p, "%lx%08lx-", (unsigned long)((unsigned long long)dev >> 32), (unsigned long)dev );
    else
        p += sprintf( p, "%lx-", (unsigned long)dev );

    if (ino != (unsigned long)ino)
        sprintf( p, "%lx%08lx", (unsigned long)((unsigned long long)ino >> 32), (unsigned long)ino );
    else
        sprintf( p, "%lx", (unsigned long)ino );
    return dir;
}


/***********************************************************************
 *           setup_config_dir
 *
 * Setup the wine configuration dir.
 */
static int setup_config_dir(void)
{
    char *p;
    struct stat st;
    int fd_cwd = open( ".", O_RDONLY );

    if (chdir( config_dir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "cannot use directory %s", config_dir );
        if ((p = strrchr( config_dir, '/' )) && p != config_dir)
        {
            while (p > config_dir + 1 && p[-1] == '/') p--;
            *p = 0;
            if (!stat( config_dir, &st ) && st.st_uid != getuid())
                fatal_error( "'%s' is not owned by you, refusing to create a configuration directory there\n",
                             config_dir );
            *p = '/';
        }
        mkdir( config_dir, 0777 );
        if (chdir( config_dir ) == -1) fatal_perror( "chdir to %s", config_dir );
        MESSAGE( "wine: created the configuration directory '%s'\n", config_dir );
    }

    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", config_dir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", config_dir );

    server_dir = init_server_dir( st.st_dev, st.st_ino );

    if (!mkdir( "dosdevices", 0777 ))
    {
        mkdir( "drive_c", 0777 );
        symlink( "../drive_c", "dosdevices/c:" );
        symlink( "/", "dosdevices/z:" );
    }
    else if (errno != EEXIST) fatal_perror( "cannot create %s/dosdevices", config_dir );

    if (fd_cwd == -1) fd_cwd = open( "dosdevices/c:", O_RDONLY );
    fcntl( fd_cwd, F_SETFD, FD_CLOEXEC );
    return fd_cwd;
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
static int server_connect(void)
{
    struct sockaddr_un addr;
    struct stat st;
    int s, slen, retry, fd_cwd;

    fd_cwd = setup_config_dir();

    /* chdir to the server directory */
    if (chdir( server_dir ) == -1)
    {
        if (errno != ENOENT) fatal_perror( "chdir to %s", server_dir );
        start_server( TRACE_ON(server) );
        if (chdir( server_dir ) == -1) fatal_perror( "chdir to %s", server_dir );
    }

    /* make sure we are at the right place */
    if (stat( ".", &st ) == -1) fatal_perror( "stat %s", server_dir );
    if (st.st_uid != getuid()) fatal_error( "'%s' is not owned by you\n", server_dir );
    if (st.st_mode & 077) fatal_error( "'%s' must not be accessible by other users\n", server_dir );

    for (retry = 0; retry < 6; retry++)
    {
        /* if not the first try, wait a bit to leave the previous server time to exit */
        if (retry)
        {
            usleep( 100000 * retry * retry );
            start_server( TRACE_ON(server) );
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }
        else if (lstat( SOCKETNAME, &st ) == -1) /* check for an already existing socket */
        {
            if (errno != ENOENT) fatal_perror( "lstat %s/%s", server_dir, SOCKETNAME );
            start_server( TRACE_ON(server) );
            if (lstat( SOCKETNAME, &st ) == -1) continue;  /* still no socket, wait a bit more */
        }

        /* make sure the socket is sane (ISFIFO needed for Solaris) */
        if (!S_ISSOCK(st.st_mode) && !S_ISFIFO(st.st_mode))
            fatal_error( "'%s/%s' is not a socket\n", server_dir, SOCKETNAME );
        if (st.st_uid != getuid())
            fatal_error( "'%s/%s' is not owned by you\n", server_dir, SOCKETNAME );

        /* try to connect to it */
        addr.sun_family = AF_UNIX;
        strcpy( addr.sun_path, SOCKETNAME );
        slen = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
        addr.sun_len = slen;
#endif
        if ((s = socket( AF_UNIX, SOCK_STREAM, 0 )) == -1) fatal_perror( "socket" );
#ifdef SO_PASSCRED
        else
        {
            int enable = 1;
            setsockopt( s, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
        }
#endif
        if (connect( s, (struct sockaddr *)&addr, slen ) != -1)
        {
            /* switch back to the starting directory */
            if (fd_cwd != -1)
            {
                fchdir( fd_cwd );
                close( fd_cwd );
            }
            fcntl( s, F_SETFD, FD_CLOEXEC );
            return s;
        }
        close( s );
    }
    server_connect_error( server_dir );
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

    if (!server_dir)
    {
        struct stat st;
        stat( config_dir, &st );
        server_dir = init_server_dir( st.st_dev, st.st_ino );
    }
    kret = bootstrap_look_up(bootstrap_port, server_dir, &wineserver_port);
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
 *           get_unix_tid
 *
 * Retrieve the Unix tid to use on the server side for the current thread.
 */
static int get_unix_tid(void)
{
    int ret = -1;
#ifdef HAVE_PTHREAD_GETTHREADID_NP
    ret = pthread_getthreadid_np();
#elif defined(linux)
    ret = syscall( __NR_gettid );
#elif defined(__sun)
    ret = pthread_self();
#elif defined(__APPLE__)
    ret = mach_thread_self();
    mach_port_deallocate(mach_task_self(), ret);
#elif defined(__NetBSD__)
    ret = _lwp_self();
#elif defined(__FreeBSD__)
    long lwpid;
    thr_self( &lwpid );
    ret = lwpid;
#elif defined(__DragonFly__)
    ret = lwp_gettid();
#endif
    return ret;
}


/***********************************************************************
 *           server_init_process
 *
 * Start the server and create the initial socket pair.
 */
void CDECL server_init_process(void)
{
    obj_handle_t version;
    const char *env_socket = getenv( "WINESERVERSOCKET" );

    server_pid = -1;
    if (env_socket)
    {
        fd_socket = atoi( env_socket );
        if (fcntl( fd_socket, F_SETFD, FD_CLOEXEC ) == -1)
            fatal_perror( "Bad server socket %d", fd_socket );
        unsetenv( "WINESERVERSOCKET" );
    }
    else
    {
        const char *arch = getenv( "WINEARCH" );

        if (arch && strcmp( arch, "win32" ) && strcmp( arch, "win64" ))
            fatal_error( "WINEARCH set to invalid value '%s', it must be either win32 or win64.\n", arch );

        fd_socket = server_connect();
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
    pthread_sigmask( SIG_BLOCK, &server_block_set, NULL );

    /* receive the first thread request fd on the main socket */
    ntdll_get_thread_data()->request_fd = receive_fd( &version );

#ifdef SO_PASSCRED
    /* now that we hopefully received the server_pid, disable SO_PASSCRED */
    {
        int enable = 0;
        setsockopt( fd_socket, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif

    if (version != SERVER_PROTOCOL_VERSION)
        server_protocol_error( "version mismatch %d/%d.\n"
                               "Your %s binary was not upgraded correctly,\n"
                               "or you have an older one somewhere in your PATH.\n"
                               "Or maybe the wrong wineserver is still running?\n",
                               version, SERVER_PROTOCOL_VERSION,
                               (version > SERVER_PROTOCOL_VERSION) ? "wine" : "wineserver" );
#if defined(__linux__) && defined(HAVE_PRCTL)
    /* work around Ubuntu's ptrace breakage */
    if (server_pid != -1) prctl( 0x59616d61 /* PR_SET_PTRACER */, server_pid );
#endif
}


/***********************************************************************
 *           server_init_process_done
 */
void CDECL server_init_process_done(void)
{
#ifdef __APPLE__
    send_server_task_port();
#endif
}


/***********************************************************************
 *           server_init_thread
 *
 * Send an init thread request.
 */
size_t CDECL server_init_thread( void *entry_point, BOOL *suspend, unsigned int *cpus,
                                 BOOL *wow64, timeout_t *start_time )
{
    static const char *cpu_names[] = { "x86", "x86_64", "PowerPC", "ARM", "ARM64" };
    const char *arch = getenv( "WINEARCH" );
    int ret;
    int reply_pipe[2];
    struct sigaction sig_act;
    size_t info_size;

    sig_act.sa_handler = SIG_IGN;
    sig_act.sa_flags   = 0;
    sigemptyset( &sig_act.sa_mask );

    /* ignore SIGPIPE so that we get an EPIPE error instead  */
    sigaction( SIGPIPE, &sig_act, NULL );

    /* create the server->client communication pipes */
    if (server_pipe( reply_pipe ) == -1) server_protocol_perror( "pipe" );
    if (server_pipe( ntdll_get_thread_data()->wait_fd ) == -1) server_protocol_perror( "pipe" );
    wine_server_send_fd( reply_pipe[1] );
    wine_server_send_fd( ntdll_get_thread_data()->wait_fd[1] );
    ntdll_get_thread_data()->reply_fd = reply_pipe[0];
    close( reply_pipe[1] );

    SERVER_START_REQ( init_thread )
    {
        req->unix_pid    = getpid();
        req->unix_tid    = get_unix_tid();
        req->teb         = wine_server_client_ptr( NtCurrentTeb() );
        req->entry       = wine_server_client_ptr( entry_point );
        req->reply_fd    = reply_pipe[1];
        req->wait_fd     = ntdll_get_thread_data()->wait_fd[1];
        req->debug_level = (TRACE_ON(server) != 0);
        req->cpu         = client_cpu;
        ret = wine_server_call( req );
        NtCurrentTeb()->ClientId.UniqueProcess = ULongToHandle(reply->pid);
        NtCurrentTeb()->ClientId.UniqueThread  = ULongToHandle(reply->tid);
        info_size         = reply->info_size;
        server_start_time = reply->server_start;
        server_cpus       = reply->all_cpus;
        *suspend          = reply->suspend;
    }
    SERVER_END_REQ;

    is_wow64 = !is_win64 && (server_cpus & ((1 << CPU_x86_64) | (1 << CPU_ARM64))) != 0;
    ntdll_get_thread_data()->wow64_redir = is_wow64;

    switch (ret)
    {
    case STATUS_SUCCESS:
        if (arch)
        {
            if (!strcmp( arch, "win32" ) && (is_win64 || is_wow64))
                fatal_error( "WINEARCH set to win32 but '%s' is a 64-bit installation.\n", config_dir );
            if (!strcmp( arch, "win64" ) && !is_win64 && !is_wow64)
                fatal_error( "WINEARCH set to win64 but '%s' is a 32-bit installation.\n", config_dir );
        }
        if (cpus) *cpus = server_cpus;
        if (wow64) *wow64 = is_wow64;
        if (start_time) *start_time = server_start_time;
        return info_size;
    case STATUS_INVALID_IMAGE_WIN_64:
        fatal_error( "'%s' is a 32-bit installation, it cannot support 64-bit applications.\n", config_dir );
    case STATUS_NOT_SUPPORTED:
        fatal_error( "'%s' is a 64-bit installation, it cannot be used with a 32-bit wineserver.\n", config_dir );
    case STATUS_INVALID_IMAGE_FORMAT:
        fatal_error( "wineserver doesn't support the %s architecture\n", cpu_names[client_cpu] );
    default:
        server_protocol_error( "init_thread failed with status %x\n", ret );
    }
}

/*
 * Client part of the client/server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>

#include "process.h"
#include "thread.h"
#include "server.h"
#include "winerror.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

/* data structure used to pass an fd with sendmsg/recvmsg */
struct cmsg_fd
{
    int len;   /* sizeof structure */
    int level; /* SOL_SOCKET */
    int type;  /* SCM_RIGHTS */
    int fd;    /* fd to pass */
};

/***********************************************************************
 *           CLIENT_Die
 *
 * Die on protocol errors or socket close
 */
static void CLIENT_Die(void)
{
    close( NtCurrentTeb()->socket );
    SYSDEPS_ExitThread();
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
    CLIENT_Die();
}


/***********************************************************************
 *           server_perror
 */
static void server_perror( const char *err )
{
    fprintf( stderr, "Client protocol error:%p: ", NtCurrentTeb()->tid );
    perror( err );
    CLIENT_Die();
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
        if (errno == EPIPE) CLIENT_Die();
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
        if (errno == EPIPE) CLIENT_Die();
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
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) CLIENT_Die();
            server_perror("read");
        }
        if (!ret) CLIENT_Die(); /* the server closed the connection; time to die... */
        server_protocol_error( "partial msg received %d/%d\n", ret, sizeof(res) );
    }
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
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) CLIENT_Die();
            server_perror("recvmsg");
        }
        if (!ret) CLIENT_Die(); /* the server closed the connection; time to die... */
        server_protocol_error( "partial seq received %d/%d\n", ret, sizeof(res) );
    }
}


/***********************************************************************
 *           server_call
 *
 * Perform a server call.
 */
unsigned int server_call( enum request req )
{
    unsigned int res;

    send_request( req );
    res = wait_reply();
    if (res) SetLastError( res );
    return res;  /* error code */
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
    if (res) SetLastError( res );
    return res;  /* error code */
}


/***********************************************************************
 *           CLIENT_InitServer
 *
 * Start the server and create the initial socket pair.
 */
int CLIENT_InitServer(void)
{
    int fd[2];
    char buffer[16];
    extern void create_initial_thread( int fd );

    if (socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) == -1)
    {
        perror("socketpair");
        exit(1);
    }
    switch(fork())
    {
    case -1:  /* error */
        perror("fork");
        exit(1);
    case 0:  /* child */
        close( fd[1] );
        break;
    default:  /* parent */
        close( fd[0] );
        sprintf( buffer, "%d", fd[1] );
/*#define EXEC_SERVER*/
#ifdef EXEC_SERVER
        execlp( "wineserver", "wineserver", buffer, NULL );
        execl( "/usr/local/bin/wineserver", "wineserver", buffer, NULL );
        execl( "./server/wineserver", "wineserver", buffer, NULL );
#endif
        create_initial_thread( fd[1] );
        exit(0);
    }
    return fd[0];
}


/***********************************************************************
 *           CLIENT_InitThread
 *
 * Send an init thread request. Return 0 if OK.
 */
int CLIENT_InitThread(void)
{
    struct init_thread_request *req;
    TEB *teb = NtCurrentTeb();
    int fd;

    if (wait_reply_fd( &fd ) || (fd == -1))
        server_protocol_error( "no fd passed on first request\n" );
    if ((teb->buffer_size = lseek( fd, 0, SEEK_END )) == -1) server_perror( "lseek" );
    teb->buffer = mmap( 0, teb->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    close( fd );
    if (teb->buffer == (void*)-1) server_perror( "mmap" );

    req = get_req_buffer();
    req->unix_pid = getpid();
    req->teb      = teb;
    if (server_call( REQ_INIT_THREAD )) return -1;
    teb->process->server_pid = req->pid;
    teb->tid = req->tid;
    return 0;
}


/***********************************************************************
 *           CLIENT_SetDebug
 *
 * Send a set debug level request. Return 0 if OK.
 */
int CLIENT_SetDebug( int level )
{
    struct set_debug_request *req = get_req_buffer();
    req->level = level;
    return server_call( REQ_SET_DEBUG );
}

/***********************************************************************
 *           CLIENT_DebuggerRequest
 *
 * Send a debugger support request. Return 0 if OK.
 */
int CLIENT_DebuggerRequest( int op )
{
    struct debugger_request *req = get_req_buffer();
    req->op = op;
    return server_call( REQ_DEBUGGER );
}


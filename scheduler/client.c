/*
 * Client part of the client/server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
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
 *           CLIENT_ProtocolError
 */
void CLIENT_ProtocolError( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Client protocol error:%p: ", NtCurrentTeb()->tid );
    vfprintf( stderr, err, args );
    va_end( args );
    CLIENT_Die();
}


/***********************************************************************
 *           CLIENT_perror
 */
void CLIENT_perror( const char *err )
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
    struct header *head = NtCurrentTeb()->buffer;
    int ret, seq = NtCurrentTeb()->seq++;

    assert( server_remaining() >= 0 );

    head->type = req;
    head->len  = (char *)NtCurrentTeb()->buffer_args - (char *)NtCurrentTeb()->buffer;

    NtCurrentTeb()->buffer_args = head + 1;  /* reset the args buffer */

    if ((ret = write( NtCurrentTeb()->socket, &seq, sizeof(seq) )) == sizeof(seq))
        return;
    if (ret == -1)
    {
        if (errno == EPIPE) CLIENT_Die();
        CLIENT_perror( "sendmsg" );
    }
    CLIENT_ProtocolError( "partial seq sent %d/%d\n", ret, sizeof(seq) );
}

/***********************************************************************
 *           send_request_fd
 *
 * Send a request to the server, passing a file descriptor.
 */
static void send_request_fd( enum request req, int fd )
{
    struct header *head = NtCurrentTeb()->buffer;
    int ret, seq = NtCurrentTeb()->seq++;
#ifndef HAVE_MSGHDR_ACCRIGHTS
    struct cmsg_fd cmsg;
#endif
    struct msghdr msghdr;
    struct iovec vec;

    assert( server_remaining() >= 0 );

    head->type = req;
    head->len  = (char *)NtCurrentTeb()->buffer_args - (char *)NtCurrentTeb()->buffer;

    NtCurrentTeb()->buffer_args = head + 1;  /* reset the args buffer */

    vec.iov_base = &seq;
    vec.iov_len  = sizeof(seq);

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

    if ((ret = sendmsg( NtCurrentTeb()->socket, &msghdr, 0 )) == sizeof(seq)) return;
    if (ret == -1)
    {
        if (errno == EPIPE) CLIENT_Die();
        CLIENT_perror( "sendmsg" );
    }
    CLIENT_ProtocolError( "partial seq sent %d/%d\n", ret, sizeof(seq) );
}

/***********************************************************************
 *           wait_reply
 *
 * Wait for a reply from the server.
 */
static void wait_reply(void)
{
    int seq, ret;

    for (;;)
    {
        if ((ret = read( NtCurrentTeb()->socket, &seq, sizeof(seq) )) == sizeof(seq))
        {
            if (seq != NtCurrentTeb()->seq++)
                CLIENT_ProtocolError( "sequence %08x instead of %08x\n", seq, NtCurrentTeb()->seq - 1 );
            return;
        }
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) CLIENT_Die();
            CLIENT_perror("read");
        }
        if (!ret) CLIENT_Die(); /* the server closed the connection; time to die... */
        CLIENT_ProtocolError( "partial seq received %d/%d\n", ret, sizeof(seq) );
    }
}


/***********************************************************************
 *           wait_reply_fd
 *
 * Wait for a reply from the server, when a file descriptor is passed.
 */
static int wait_reply_fd(void)
{
    struct iovec vec;
    int seq, ret;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr;
    int fd = -1;

    msghdr.msg_accrights    = (void *)&fd;
    msghdr.msg_accrightslen = sizeof(int);
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
    vec.iov_base = &seq;
    vec.iov_len  = sizeof(seq);

    for (;;)
    {
        if ((ret = recvmsg( NtCurrentTeb()->socket, &msghdr, 0 )) == sizeof(seq))
        {
            if (seq != NtCurrentTeb()->seq++)
                CLIENT_ProtocolError( "sequence %08x instead of %08x\n", seq, NtCurrentTeb()->seq - 1 );
#ifdef HAVE_MSGHDR_ACCRIGHTS
            return fd;
#else
            return cmsg.fd;
#endif
        }
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) CLIENT_Die();
            CLIENT_perror("recvmsg");
        }
        if (!ret) CLIENT_Die(); /* the server closed the connection; time to die... */
        CLIENT_ProtocolError( "partial seq received %d/%d\n", ret, sizeof(seq) );
    }
}


/***********************************************************************
 *           server_call
 *
 * Perform a server call.
 */
unsigned int server_call( enum request req )
{
    struct header *head;

    send_request( req );
    wait_reply();

    head = (struct header *)NtCurrentTeb()->buffer;
    if ((head->len < sizeof(*head)) || (head->len > NtCurrentTeb()->buffer_size))
        CLIENT_ProtocolError( "header length %d\n", head->len );
    if (head->type) SetLastError( head->type );
    return head->type;  /* error code */
}

/***********************************************************************
 *           server_call_fd
 *
 * Perform a server call, passing a file descriptor.
 * If *fd is != -1, it will be passed to the server.
 * If the server passes an fd, it will be stored into *fd.
 */
unsigned int server_call_fd( enum request req, int *fd )
{
    struct header *head;

    if (*fd == -1) send_request( req );
    else send_request_fd( req, *fd );
    *fd = wait_reply_fd();

    head = (struct header *)NtCurrentTeb()->buffer;
    if ((head->len < sizeof(*head)) || (head->len > NtCurrentTeb()->buffer_size))
        CLIENT_ProtocolError( "header length %d\n", head->len );
    if (head->type) SetLastError( head->type );
    return head->type;  /* error code */
}

/***********************************************************************
 *           CLIENT_SendRequest
 *
 * Send a request to the server.
 */
void CLIENT_SendRequest( enum request req, int pass_fd,
                         int n, ... /* arg_1, len_1, etc. */ )
{
    va_list args;

    va_start( args, n );
    while (n--)
    {
        void *ptr = va_arg( args, void * );
        int len   = va_arg( args, int );
        memcpy( server_add_data( len ), ptr, len );
    }
    va_end( args );

    if (pass_fd == -1) send_request( req );
    else
    {
        send_request_fd( req, pass_fd );
        close( pass_fd ); /* we passed the fd now we can close it */
    }
}


/***********************************************************************
 *           CLIENT_WaitReply_v
 *
 * Wait for a reply from the server.
 * Returns the error code (or 0 if OK).
 */
static unsigned int CLIENT_WaitReply_v( int *len, int *passed_fd,
                                        struct iovec *vec, int veclen )
{
    struct header *head;
    char *ptr;
    int i, remaining;

    if (passed_fd) *passed_fd = wait_reply_fd();
    else wait_reply();

    head = (struct header *)NtCurrentTeb()->buffer;
    if ((head->len < sizeof(*head)) || (head->len > NtCurrentTeb()->buffer_size))
        CLIENT_ProtocolError( "header length %d\n", head->len );

    remaining = head->len - sizeof(*head);
    ptr = (char *)(head + 1);
    for (i = 0; i < veclen; i++, vec++)
    {
        int len = MIN( remaining, vec->iov_len );
        memcpy( vec->iov_base, ptr, len );
        ptr += len;
        if (!(remaining -= len)) break;
    }
    if (len) *len = head->len - sizeof(*head);
    if (head->type) SetLastError( head->type );
    return head->type;  /* error code */
}


/***********************************************************************
 *           CLIENT_WaitReply
 *
 * Wait for a reply from the server.
 */
unsigned int CLIENT_WaitReply( int *len, int *passed_fd,
                               int n, ... /* arg_1, len_1, etc. */ )
{
    struct iovec vec[16];
    va_list args;
    int i;

    assert( n < 16 );
    va_start( args, n );
    for (i = 0; i < n; i++)
    {
        vec[i].iov_base = va_arg( args, void * );
        vec[i].iov_len  = va_arg( args, int );
    }
    va_end( args );
    return CLIENT_WaitReply_v( len, passed_fd, vec, n );
}


/***********************************************************************
 *           CLIENT_WaitSimpleReply
 *
 * Wait for a simple fixed-length reply from the server.
 */
unsigned int CLIENT_WaitSimpleReply( void *reply, int len, int *passed_fd )
{
    struct iovec vec;
    unsigned int ret;
    int got;

    vec.iov_base = reply;
    vec.iov_len  = len;
    ret = CLIENT_WaitReply_v( &got, passed_fd, &vec, 1 );
    if (got != len)
        CLIENT_ProtocolError( "WaitSimpleReply: len %d != %d\n", len, got );
    return ret;
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
    struct init_thread_request req;
    struct init_thread_reply reply;
    TEB *teb = NtCurrentTeb();

    int fd = wait_reply_fd();
    if (fd == -1) CLIENT_ProtocolError( "no fd passed on first request\n" );
    if ((teb->buffer_size = lseek( fd, 0, SEEK_END )) == -1) CLIENT_perror( "lseek" );
    teb->buffer = mmap( 0, teb->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    close( fd );
    if (teb->buffer == (void*)-1) CLIENT_perror( "mmap" );
    teb->buffer_args = (char *)teb->buffer + sizeof(struct header);

    req.unix_pid = getpid();
    req.teb      = teb;
    CLIENT_SendRequest( REQ_INIT_THREAD, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return -1;
    teb->process->server_pid = reply.pid;
    teb->tid = reply.tid;
    return 0;
}


/***********************************************************************
 *           CLIENT_SetDebug
 *
 * Send a set debug level request. Return 0 if OK.
 */
int CLIENT_SetDebug( int level )
{
    CLIENT_SendRequest( REQ_SET_DEBUG, -1, 1, &level, sizeof(level) );
    return CLIENT_WaitReply( NULL, NULL, 0 );
}

/***********************************************************************
 *           CLIENT_DebuggerRequest
 *
 * Send a debugger support request. Return 0 if OK.
 */
int CLIENT_DebuggerRequest( int op )
{
    struct debugger_request req;
    req.op = op;
    CLIENT_SendRequest( REQ_DEBUGGER, -1, 1, &req, sizeof(req) );
    return CLIENT_WaitReply( NULL, NULL, 0 );
}


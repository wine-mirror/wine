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
#include <sys/uio.h>
#include <unistd.h>

#include "process.h"
#include "thread.h"
#include "server/request.h"
#include "server.h"
#include "winerror.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif


/***********************************************************************
 *           CLIENT_ProtocolError
 */
static void CLIENT_ProtocolError( const char *err, ... )
{
    THDB *thdb = THREAD_Current();
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Client protocol error:%p: ", thdb->server_tid );
    vfprintf( stderr, err, args );
    va_end( args );
    ExitThread(1);
}


/***********************************************************************
 *           CLIENT_SendRequest_v
 *
 * Send a request to the server.
 */
static void CLIENT_SendRequest_v( enum request req, int pass_fd,
                                  struct iovec *vec, int veclen )
{
    THDB *thdb = THREAD_Current();
#ifndef HAVE_MSGHDR_ACCRIGHTS
    struct cmsg_fd cmsg  = { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS, pass_fd };
#endif
    struct msghdr msghdr = { NULL, 0, vec, veclen, };
    struct header head;
    int i, ret, len;

    assert( veclen > 0 );
    vec[0].iov_base = &head;
    vec[0].iov_len  = sizeof(head);
    for (i = len = 0; i < veclen; i++) len += vec[i].iov_len;

    assert( len <= MAX_MSG_LENGTH );
    head.type = req;
    head.len  = len;
    head.seq  = thdb->seq++;

    if (pass_fd != -1)  /* we have an fd to send */
    {
#ifdef HAVE_MSGHDR_ACCRIGHTS
        msghdr.msg_accrights = (void *)&pass_fd;
        msghdr.msg_accrightslen = sizeof(pass_fd);
#else
        msghdr.msg_control    = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
#endif
    }

    if ((ret = sendmsg( thdb->socket, &msghdr, 0 )) < len)
    {
        if (ret == -1) perror( "sendmsg" );
        CLIENT_ProtocolError( "partial msg sent %d/%d\n", ret, len );
    }
    /* we passed the fd now we can close it */
    if (pass_fd != -1) close( pass_fd );
}


/***********************************************************************
 *           CLIENT_SendRequest
 *
 * Send a request to the server.
 */
void CLIENT_SendRequest( enum request req, int pass_fd,
                         int n, ... /* arg_1, len_1, etc. */ )
{
    struct iovec vec[16];
    va_list args;
    int i;

    n++;  /* for vec[0] */
    assert( n < 16 );
    va_start( args, n );
    for (i = 1; i < n; i++)
    {
        vec[i].iov_base = va_arg( args, void * );
        vec[i].iov_len  = va_arg( args, int );
    }
    va_end( args );
    return CLIENT_SendRequest_v( req, pass_fd, vec, n );
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
    THDB *thdb = THREAD_Current();
    int pass_fd = -1;
#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr = { NULL, 0, vec, veclen, (void*)&pass_fd, sizeof(int) };
#else
    struct cmsg_fd cmsg  = { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS, -1 };
    struct msghdr msghdr = { NULL, 0, vec, veclen, &cmsg, sizeof(cmsg), 0 };
#endif
    struct header head;
    int ret, remaining;

    assert( veclen > 0 );
    vec[0].iov_base       = &head;
    vec[0].iov_len        = sizeof(head);

    while ((ret = recvmsg( thdb->socket, &msghdr, 0 )) == -1)
    {
        if (errno == EINTR) continue;
        perror("recvmsg");
        CLIENT_ProtocolError( "recvmsg\n" );
    }
    if (!ret) ExitThread(1); /* the server closed the connection; time to die... */

    /* sanity checks */

    if (ret < sizeof(head))
        CLIENT_ProtocolError( "partial header received %d/%d\n", ret, sizeof(head) );

    if ((head.len < sizeof(head)) || (head.len > MAX_MSG_LENGTH))
        CLIENT_ProtocolError( "header length %d\n", head.len );

    if (head.seq != thdb->seq++)
        CLIENT_ProtocolError( "sequence %08x instead of %08x\n", head.seq, thdb->seq - 1 );

#ifndef HAVE_MSGHDR_ACCRIGHTS
    pass_fd = cmsg.fd;
#endif

    if (head.type != ERROR_SUCCESS)
    {
        SetLastError( head.type );
    }
    else if (passed_fd)
    {
        *passed_fd = pass_fd;
        pass_fd = -1;
    }

    if (len) *len = ret - sizeof(head);
    if (pass_fd != -1) close( pass_fd );
    remaining = head.len - ret;
    while (remaining > 0)  /* drop remaining data */
    {
        char buffer[1024];
        int len = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if ((len = recv( thdb->socket, buffer, len, 0 )) == -1)
        {
            perror( "recv" );
            CLIENT_ProtocolError( "recv\n" );
        }
        if (!len) ExitThread(1); /* the server closed the connection; time to die... */
        remaining -= len;
    }

    return head.type;  /* error code */
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

    n++;  /* for vec[0] */
    assert( n < 16 );
    va_start( args, n );
    for (i = 1; i < n; i++)
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
    struct iovec vec[2];
    unsigned int ret;
    int got;

    vec[1].iov_base = reply;
    vec[1].iov_len  = len;
    ret = CLIENT_WaitReply_v( &got, passed_fd, vec, 2 );
    if (got != len)
        CLIENT_ProtocolError( "WaitSimpleReply: len %d != %d\n", len, got );
    return ret;
}


/***********************************************************************
 *           CLIENT_NewThread
 *
 * Send a new thread request.
 */
int CLIENT_NewThread( THDB *thdb, int *thandle, int *phandle )
{
    struct new_thread_request request;
    struct new_thread_reply reply;
    int fd[2];
    extern BOOL32 THREAD_InitDone;
    extern void server_init( int fd );
    extern void select_loop(void);

    if (!THREAD_InitDone)  /* first thread -> start the server */
    {
        int tmpfd[2];
        char buffer[16];

        if (socketpair( AF_UNIX, SOCK_STREAM, 0, tmpfd ) == -1)
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
            close( tmpfd[0] );
            sprintf( buffer, "%d", tmpfd[1] );
/*#define EXEC_SERVER*/
#ifdef EXEC_SERVER
            execlp( "wineserver", "wineserver", buffer, NULL );
            execl( "/usr/local/bin/wineserver", "wineserver", buffer, NULL );
            execl( "./server/wineserver", "wineserver", buffer, NULL );
#endif
            server_init( tmpfd[1] );
            select_loop();
            exit(0);
        default:  /* parent */
            close( tmpfd[1] );
            SET_CUR_THREAD( thdb );
            THREAD_InitDone = TRUE;
            thdb->socket = tmpfd[0];
            break;
        }
    }

    if (socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );  /* FIXME */
        return -1;
    }

    request.pid = thdb->process->server_pid;
    CLIENT_SendRequest( REQ_NEW_THREAD, fd[1], 1, &request, sizeof(request) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) goto error;
    thdb->server_tid = reply.tid;
    thdb->process->server_pid = reply.pid;
    if (thdb->socket != -1) close( thdb->socket );
    thdb->socket = fd[0];
    thdb->seq = 0;  /* reset the sequence number for the new fd */
    fcntl( fd[0], F_SETFD, 1 ); /* set close on exec flag */

    if (thandle) *thandle = reply.thandle;
    else if (reply.thandle != -1) CLIENT_CloseHandle( reply.thandle );
    if (phandle) *phandle = reply.phandle;
    else if (reply.phandle != -1) CLIENT_CloseHandle( reply.phandle );
    return 0;

 error:
    close( fd[0] );
    return -1;
}


/***********************************************************************
 *           CLIENT_InitThread
 *
 * Send an init thread request. Return 0 if OK.
 */
int CLIENT_InitThread(void)
{
    THDB *thdb = THREAD_Current();
    struct init_thread_request init;
    int len = strlen( thdb->process->env_db->cmd_line );

    init.unix_pid = getpid();
    len = MIN( len, MAX_MSG_LENGTH - sizeof(init) );

    CLIENT_SendRequest( REQ_INIT_THREAD, -1, 2,
                        &init, sizeof(init),
                        thdb->process->env_db->cmd_line, len );
    return CLIENT_WaitReply( NULL, NULL, 0 );
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
 *           CLIENT_CloseHandle
 *
 * Send a close handle request. Return 0 if OK.
 */
int CLIENT_CloseHandle( int handle )
{
    CLIENT_SendRequest( REQ_CLOSE_HANDLE, -1, 1, &handle, sizeof(handle) );
    return CLIENT_WaitReply( NULL, NULL, 0 );
}

/***********************************************************************
 *           CLIENT_DuplicateHandle
 *
 * Send a duplicate handle request. Return 0 if OK.
 */
int CLIENT_DuplicateHandle( int src_process, int src_handle, int dst_process, int dst_handle,
                            DWORD access, BOOL32 inherit, DWORD options )
{
    struct dup_handle_request req;
    struct dup_handle_reply reply;

    req.src_process = src_process;
    req.src_handle  = src_handle;
    req.dst_process = dst_process;
    req.dst_handle  = dst_handle;
    req.access      = access;
    req.inherit     = inherit;
    req.options     = options;

    CLIENT_SendRequest( REQ_DUP_HANDLE, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    return reply.handle;
}


/***********************************************************************
 *           CLIENT_OpenProcess
 *
 * Open a handle to a process.
 */
int CLIENT_OpenProcess( void *pid, DWORD access, BOOL32 inherit )
{
    struct open_process_request req;
    struct open_process_reply reply;

    req.pid     = pid;
    req.access  = access;
    req.inherit = inherit;

    CLIENT_SendRequest( REQ_OPEN_PROCESS, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    return reply.handle;
}

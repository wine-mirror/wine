/*
 * Client part of the client/server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "process.h"
#include "thread.h"
#include "server.h"
#include "winerror.h"


/***********************************************************************
 *           CLIENT_SendRequest_v
 *
 * Send a request to the server.
 */
static void CLIENT_SendRequest_v( enum request req, int pass_fd,
                                  struct iovec *vec, int veclen )
{
    THDB *thdb = THREAD_Current();
    struct { struct cmsghdr hdr; int fd; } cmsg =
                         { { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS }, pass_fd };
    struct msghdr msghdr;
    struct header head;
    int i, ret, len;

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;

    assert( veclen > 0 );

    vec[0].iov_base   = &head;
    vec[0].iov_len    = sizeof(head);
    msghdr.msg_iov    = vec;
    msghdr.msg_iovlen = veclen;
    for (i = len = 0; i < veclen; i++) len += vec[i].iov_len;

    assert( len <= MAX_MSG_LENGTH );
    head.type = req;
    head.len  = len;
    head.seq  = thdb->seq++;

    if (pass_fd == -1)  /* no fd to pass */
    {
        msghdr.msg_control = NULL;
        msghdr.msg_controllen = 0;
    }
    else
    {
        msghdr.msg_control = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
    }

    if ((ret = sendmsg( thdb->socket, &msghdr, 0 )) < len)
    {
        fprintf( stderr, "Fatal protocol error: " );
        if (ret == -1) perror( "sendmsg" );
        else fprintf( stderr, "partial msg sent %d/%d\n", ret, len );
        ExitThread(1);
    }
    /* we passed the fd now we can close it */
    if (pass_fd != -1) close( pass_fd );
}


/***********************************************************************
 *           CLIENT_SendRequest
 *
 * Send a request to the server.
 */
static void CLIENT_SendRequest( enum request req, int pass_fd,
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
    struct { struct cmsghdr hdr; int fd; } cmsg =
                         { { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS }, -1 };
    struct msghdr msghdr;
    struct header head;
    int ret, remaining;

    assert( veclen > 0 );
    vec[0].iov_base       = &head;
    vec[0].iov_len        = sizeof(head);
    msghdr.msg_name       = NULL;
    msghdr.msg_namelen    = 0;
    msghdr.msg_iov        = vec;
    msghdr.msg_iovlen     = veclen;
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);

    if ((ret = recvmsg( thdb->socket, &msghdr, 0 )) == -1)
    {
        fprintf( stderr, "Fatal protocol error: " );
        perror("recvmsg");
        ExitThread(1);
    }
    if (ret < sizeof(head))
    {
        fprintf( stderr,
                 "Fatal protocol error: partial header received %d/%d\n",
                 ret, sizeof(head));
        ExitThread(1);
    }
    if ((head.len < sizeof(head)) || (head.len > MAX_MSG_LENGTH))
    {
        fprintf( stderr, "Fatal protocol error: header length %d\n",
                 head.len );
        ExitThread(1);
    }
    if (head.seq != thdb->seq++)
    {
        fprintf( stderr,
                 "Fatal protocol error: sequence %08x instead of %08x\n",
                 head.seq, thdb->seq - 1 );
        ExitThread(1);
    }

    if (head.type != ERROR_SUCCESS)
    {
        SetLastError( head.type );
    }
    else if (passed_fd)
    {
        *passed_fd = cmsg.fd;
        cmsg.fd = -1;
    }

    if (len) *len = ret - sizeof(head);
    if (cmsg.fd != -1) close( cmsg.fd );
    remaining = head.len - ret;
    while (remaining > 0)  /* drop remaining data */
    {
        char buffer[1024];
        int len = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if ((len = recv( thdb->socket, buffer, len, 0 )) == -1)
        {
            fprintf( stderr, "Fatal protocol error: " );
            perror( "recv" );
            ExitThread(1);
        }
        remaining -= len;
    }

    return head.type;  /* error code */
}


/***********************************************************************
 *           CLIENT_WaitReply
 *
 * Wait for a reply from the server.
 */
static unsigned int CLIENT_WaitReply( int *len, int *passed_fd,
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
 *           send_new_thread
 *
 * Send a new thread request. Helper function for CLIENT_NewThread.
 */
static int send_new_thread( THDB *thdb )
{
    struct new_thread_request request;
    struct new_thread_reply reply;
    int len, fd[2];

    if (socketpair( AF_UNIX, SOCK_STREAM, PF_UNIX, fd ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );  /* FIXME */
        return -1;
    }

    request.pid = thdb->process->server_pid;
    CLIENT_SendRequest( REQ_NEW_THREAD, fd[1], 1, &request, sizeof(request) );

    if (CLIENT_WaitReply( &len, NULL, 1, &reply, sizeof(reply) )) goto error;
    if (len < sizeof(reply)) goto error;
    thdb->server_tid = reply.tid;
    thdb->process->server_pid = reply.pid;
    if (thdb->socket != -1) close( thdb->socket );
    thdb->socket = fd[0];
    thdb->seq = 0;  /* reset the sequence number for the new fd */
    return 0;

 error:
    close( fd[0] );
    return -1;
}


/***********************************************************************
 *           CLIENT_NewThread
 *
 * Send a new thread request.
 */
int CLIENT_NewThread( THDB *thdb )
{
    extern BOOL32 THREAD_InitDone;

    if (!THREAD_InitDone)  /* first thread -> start the server */
    {
        int tmpfd[2];
        char buffer[16];

        if (socketpair( AF_UNIX, SOCK_STREAM, PF_UNIX, tmpfd ) == -1)
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
#ifdef EXEC_SERVER
            execlp( "wineserver", "wineserver", buffer, NULL );
            execl( "/usr/local/bin/wineserver", "wineserver", buffer, NULL );
            execl( "./server/wineserver", "wineserver", buffer, NULL );
#endif
            server_main_loop( tmpfd[1] );
            exit(0);
        default:  /* parent */
            close( tmpfd[1] );
            SET_CUR_THREAD( thdb );
            THREAD_InitDone = TRUE;
            thdb->socket = tmpfd[0];
            break;
        }
    }

    return send_new_thread( thdb );
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

    init.pid = getpid();
    len = MIN( len, MAX_MSG_LENGTH - sizeof(init) );

    CLIENT_SendRequest( REQ_INIT_THREAD, -1, 2,
                        &init, sizeof(init),
                        thdb->process->env_db->cmd_line, len );
    return CLIENT_WaitReply( NULL, NULL, NULL, 0 );
}

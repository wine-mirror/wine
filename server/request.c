/*
 * Server-side request handling
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/uio.h>
#include <unistd.h>

#include "winnt.h"
#include "winbase.h"
#include "wincon.h"
#include "thread.h"
#include "server.h"
#define WANT_REQUEST_HANDLERS
#include "request.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

 
struct thread *current = NULL;  /* thread handling the current request */


/* socket communication static structures */
static struct iovec myiovec;
static struct msghdr msghdr = { NULL, 0, &myiovec, 1, };
#ifndef HAVE_MSGHDR_ACCRIGHTS
struct cmsg_fd
{
    int len;   /* sizeof structure */
    int level; /* SOL_SOCKET */
    int type;  /* SCM_RIGHTS */
    int fd;    /* fd to pass */
};
static struct cmsg_fd cmsg = { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS, -1 };
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

/* complain about a protocol error and terminate the client connection */
void fatal_protocol_error( struct thread *thread, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%p: ", thread );
    vfprintf( stderr, err, args );
    va_end( args );
    kill_thread( thread, PROTOCOL_ERROR );
}

/* call a request handler */
static void call_req_handler( struct thread *thread, enum request req, int fd )
{
    current = thread;
    clear_error();

    if (debug_level) trace_request( req, fd );

    if (req < REQ_NB_REQUESTS)
    {
        req_handlers[req].handler( current->buffer, fd );
        if (current && !current->wait) send_reply( current );
        current = NULL;
        return;
    }
    fatal_protocol_error( current, "bad request %d\n", req );
}

/* set the fd to pass to the thread */
void set_reply_fd( struct thread *thread, int pass_fd )
{
    assert( thread->pass_fd == -1 );
    thread->pass_fd = pass_fd;
}

/* send a reply to a thread */
void send_reply( struct thread *thread )
{
    assert( !thread->wait );
    if (debug_level) trace_reply( thread );
    if (!write_request( thread )) set_select_events( &thread->obj, POLLOUT );
}

/* read a message from a client that has something to say */
void read_request( struct thread *thread )
{
    int ret;
    enum request req;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(int);
    msghdr.msg_accrights = (void *)&thread->pass_fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    cmsg.fd = -1;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    assert( thread->pass_fd == -1 );

    myiovec.iov_base = (void *)&req;
    myiovec.iov_len  = sizeof(req);

    ret = recvmsg( thread->obj.fd, &msghdr, 0 );
#ifndef HAVE_MSGHDR_ACCRIGHTS
    thread->pass_fd = cmsg.fd;
#endif

    if (ret == sizeof(req))
    {
        int pass_fd = thread->pass_fd;
        thread->pass_fd = -1;
        call_req_handler( thread, req, pass_fd );
        if (pass_fd != -1) close( pass_fd );
        return;
    }
    if (ret == -1)
    {
        perror("recvmsg");
        kill_thread( thread, BROKEN_PIPE );
        return;
    }
    if (!ret)  /* closed pipe */
    {
        kill_thread( thread, BROKEN_PIPE );
        return;
    }
    fatal_protocol_error( thread, "partial message received %d/%d\n", ret, sizeof(req) );
}

/* send a message to a client that is ready to receive something */
int write_request( struct thread *thread )
{
    int ret;

    if (thread->pass_fd == -1)
    {
        ret = write( thread->obj.fd, &thread->error, sizeof(thread->error) );
        if (ret == sizeof(thread->error)) goto ok;
    }
    else  /* we have an fd to send */
    {
#ifdef HAVE_MSGHDR_ACCRIGHTS
        msghdr.msg_accrightslen = sizeof(int);
        msghdr.msg_accrights = (void *)&thread->pass_fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
        msghdr.msg_control    = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
        cmsg.fd = thread->pass_fd;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

        myiovec.iov_base = (void *)&thread->error;
        myiovec.iov_len  = sizeof(thread->error);

        ret = sendmsg( thread->obj.fd, &msghdr, 0 );
        close( thread->pass_fd );
        thread->pass_fd = -1;
        if (ret == sizeof(thread->error)) goto ok;
    }
    if (ret == -1)
    {
        if (errno == EWOULDBLOCK) return 0;  /* not a fatal error */
        if (errno != EPIPE) perror("sendmsg");
    }
    else fprintf( stderr, "Partial message sent %d/%d\n", ret, sizeof(thread->error) );
    kill_thread( thread, BROKEN_PIPE );
    return -1;

 ok:
    set_select_events( &thread->obj, POLLIN );
    return 1;
}

/* set the debug level */
DECL_HANDLER(set_debug)
{
    debug_level = req->level;
    /* Make sure last_req is initialized */
    current->last_req = REQ_SET_DEBUG;
}

/* debugger support operations */
DECL_HANDLER(debugger)
{
    switch ( req->op )
    {
    case DEBUGGER_FREEZE_ALL:
        suspend_all_threads();
        break;

    case DEBUGGER_UNFREEZE_ALL:
        resume_all_threads();
        break;
    }
}

/*
 * Server-side socket communication functions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "config.h"
#include "object.h"
#include "request.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

/* client structure */
struct client
{
    struct select_user   select;     /* select user */
    unsigned int         seq;        /* current sequence number */
    int                  pass_fd;    /* fd to pass to and from the client */
    struct thread       *self;       /* client thread (opaque pointer) */
    struct timeout_user *timeout;    /* current timeout (opaque pointer) */
};


/* exit code passed to remove_client */
#define OUT_OF_MEMORY  -1
#define BROKEN_PIPE    -2
#define PROTOCOL_ERROR -3


/* signal a client protocol error */
static void protocol_error( struct client *client, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%p: ", client->self );
    vfprintf( stderr, err, args );
    va_end( args );
    remove_client( client, PROTOCOL_ERROR );
}

/* send a message to a client that is ready to receive something */
static void do_write( struct client *client )
{
    int ret;

    if (client->pass_fd == -1)
    {
        ret = write( client->select.fd, &client->seq, sizeof(client->seq) );
    }
    else  /* we have an fd to send */
    {
        struct iovec vec;
        struct msghdr msghdr;

#ifdef HAVE_MSGHDR_ACCRIGHTS
        msghdr.msg_accrights = (void *)&client->pass_fd;
        msghdr.msg_accrightslen = sizeof(client->pass_fd);
#else  /* HAVE_MSGHDR_ACCRIGHTS */
        struct cmsg_fd cmsg;

        cmsg.len   = sizeof(cmsg);
        cmsg.level = SOL_SOCKET;
        cmsg.type  = SCM_RIGHTS;
        cmsg.fd    = client->pass_fd;
        msghdr.msg_control    = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
        msghdr.msg_flags      = 0;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

        msghdr.msg_name    = NULL;
        msghdr.msg_namelen = 0;
        msghdr.msg_iov     = &vec;
        msghdr.msg_iovlen  = 1;
        vec.iov_base = (char *)&client->seq;
        vec.iov_len  = sizeof(client->seq);

        ret = sendmsg( client->select.fd, &msghdr, 0 );
        close( client->pass_fd );
        client->pass_fd = -1;
    }
    if (ret == sizeof(client->seq))
    {
        /* everything OK */
        client->seq++;
        set_select_events( &client->select, READ_EVENT );
        return;
    }
    if (ret == -1)
    {
        if (errno != EPIPE) perror("sendmsg");
        remove_client( client, BROKEN_PIPE );
        return;
    }
    fprintf( stderr, "Partial sequence sent (%d)\n", ret );
    remove_client( client, BROKEN_PIPE );
}


/* read a message from a client that has something to say */
static void do_read( struct client *client )
{
    struct iovec vec;
    int ret, seq;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr;

    msghdr.msg_accrights    = (void *)&client->pass_fd;
    msghdr.msg_accrightslen = sizeof(client->pass_fd);
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

    assert( client->pass_fd == -1 );

    msghdr.msg_name    = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov     = &vec;
    msghdr.msg_iovlen  = 1;

    vec.iov_base = &seq;
    vec.iov_len  = sizeof(seq);

    ret = recvmsg( client->select.fd, &msghdr, 0 );
#ifndef HAVE_MSGHDR_ACCRIGHTS
    client->pass_fd = cmsg.fd;
#endif

    if (ret == sizeof(seq))
    {
        int pass_fd = client->pass_fd;
        if (seq != client->seq++)
        {
            protocol_error( client, "bad sequence %08x instead of %08x\n",
                            seq, client->seq - 1 );
            return;
        }
        client->pass_fd = -1;
        call_req_handler( client->self, pass_fd );
        if (pass_fd != -1) close( pass_fd );
        return;
    }
    if (ret == -1)
    {
        perror("recvmsg");
        remove_client( client, BROKEN_PIPE );
        return;
    }
    if (!ret)  /* closed pipe */
    {
        remove_client( client, BROKEN_PIPE );
        return;
    }
    protocol_error( client, "partial sequence received %d/%d\n", ret, sizeof(seq) );
}

/* handle a client event */
static void client_event( int event, void *private )
{
    struct client *client = (struct client *)private;
    if (event & WRITE_EVENT) do_write( client );
    if (event & READ_EVENT) do_read( client );
}

/*******************************************************************/
/* server-side exported functions                                  */

/* add a client */
struct client *add_client( int fd, struct thread *self )
{
    int flags;
    struct client *client = mem_alloc( sizeof(*client) );
    if (!client) return NULL;

    flags = fcntl( fd, F_GETFL, 0 );
    fcntl( fd, F_SETFL, flags | O_NONBLOCK );

    client->select.fd            = fd;
    client->select.func          = client_event;
    client->select.private       = client;
    client->seq                  = 0;
    client->self                 = self;
    client->timeout              = NULL;
    client->pass_fd              = -1;
    register_select_user( &client->select );
    set_select_events( &client->select, READ_EVENT );
    return client;
}

/* remove a client */
void remove_client( struct client *client, int exit_code )
{
    assert( client );

    call_kill_handler( client->self, exit_code );

    if (client->timeout) remove_timeout_user( client->timeout );
    unregister_select_user( &client->select );
    close( client->select.fd );

    /* Purge messages */
    if (client->pass_fd != -1) close( client->pass_fd );
    free( client );
}

/* set the fd to pass to the client */
void client_pass_fd( struct client *client, int pass_fd )
{
    assert( client->pass_fd == -1 );
    client->pass_fd = pass_fd;
}

/* send a reply to a client */
void client_reply( struct client *client )
{
    if (debug_level) trace_reply( client->self, client->pass_fd );
    set_select_events( &client->select, WRITE_EVENT );
}

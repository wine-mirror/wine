/*
 * Server-side socket communication functions
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
    unsigned int         res;        /* current result to send */
    int                  pass_fd;    /* fd to pass to and from the client */
    struct thread       *self;       /* client thread (opaque pointer) */
    struct timeout_user *timeout;    /* current timeout (opaque pointer) */
};


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


/* send a message to a client that is ready to receive something */
static int do_write( struct client *client )
{
    int ret;

    if (client->pass_fd == -1)
    {
        ret = write( client->select.fd, &client->res, sizeof(client->res) );
        if (ret == sizeof(client->res)) goto ok;
    }
    else  /* we have an fd to send */
    {
#ifdef HAVE_MSGHDR_ACCRIGHTS
        msghdr.msg_accrightslen = sizeof(int);
        msghdr.msg_accrights = (void *)&client->pass_fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
        msghdr.msg_control    = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
        cmsg.fd = client->pass_fd;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

        myiovec.iov_base = (void *)&client->res;
        myiovec.iov_len  = sizeof(client->res);

        ret = sendmsg( client->select.fd, &msghdr, 0 );
        close( client->pass_fd );
        client->pass_fd = -1;
        if (ret == sizeof(client->res)) goto ok;
    }
    if (ret == -1)
    {
        if (errno == EWOULDBLOCK) return 0;  /* not a fatal error */
        if (errno != EPIPE) perror("sendmsg");
    }
    else fprintf( stderr, "Partial message sent %d/%d\n", ret, sizeof(client->res) );
    remove_client( client, BROKEN_PIPE );
    return 0;

 ok:
    set_select_events( &client->select, READ_EVENT );
    return 1;
}


/* read a message from a client that has something to say */
static void do_read( struct client *client )
{
    int ret;
    enum request req;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    msghdr.msg_accrightslen = sizeof(int);
    msghdr.msg_accrights = (void *)&client->pass_fd;
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    msghdr.msg_control    = &cmsg;
    msghdr.msg_controllen = sizeof(cmsg);
    cmsg.fd = -1;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    assert( client->pass_fd == -1 );

    myiovec.iov_base = (void *)&req;
    myiovec.iov_len  = sizeof(req);

    ret = recvmsg( client->select.fd, &msghdr, 0 );
#ifndef HAVE_MSGHDR_ACCRIGHTS
    client->pass_fd = cmsg.fd;
#endif

    if (ret == sizeof(req))
    {
        int pass_fd = client->pass_fd;
        client->pass_fd = -1;
        call_req_handler( client->self, req, pass_fd );
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
    fatal_protocol_error( client->self, "partial message received %d/%d\n", ret, sizeof(req) );
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
void client_reply( struct client *client, unsigned int res )
{
    if (debug_level) trace_reply( client->self, res, client->pass_fd );
    client->res = res;
    if (!do_write( client )) set_select_events( &client->select, WRITE_EVENT );
}

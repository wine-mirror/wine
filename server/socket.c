/*
 * Server-side socket communication functions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
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
#include "server.h"

#include "server/object.h"

/* Some versions of glibc don't define this */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 1
#endif

/* client state */
enum state
{
    RUNNING,   /* running normally */
    SENDING,   /* sending us a request */
    WAITING,   /* waiting for us to reply */
    READING    /* reading our reply */
};

/* client structure */
struct client
{
    enum state         state;        /* client state */
    unsigned int       seq;          /* current sequence number */
    struct header      head;         /* current msg header */
    char              *data;         /* current msg data */
    int                count;        /* bytes sent/received so far */
    int                pass_fd;      /* fd to pass to and from the client */
    struct thread     *self;         /* client thread (opaque pointer) */
};


/* exit code passed to remove_client */
#define OUT_OF_MEMORY  -1
#define BROKEN_PIPE    -2
#define PROTOCOL_ERROR -3


/* signal a client protocol error */
static void protocol_error( int client_fd, const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "Protocol error:%d: ", client_fd );
    vfprintf( stderr, err, args );
    va_end( args );
}

/* send a message to a client that is ready to receive something */
static void do_write( struct client *client, int client_fd )
{
    struct iovec vec[2];
#ifndef HAVE_MSGHDR_ACCRIGHTS
    struct cmsg_fd cmsg;
#endif
    struct msghdr msghdr;
    int ret;

    /* make sure we have something to send */
    assert( client->count < client->head.len );
    /* make sure the client is listening */
    assert( client->state == READING );

    msghdr.msg_name = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov = vec;

    if (client->count < sizeof(client->head))
    {
        vec[0].iov_base = (char *)&client->head + client->count;
        vec[0].iov_len  = sizeof(client->head) - client->count;
        vec[1].iov_base = client->data;
        vec[1].iov_len  = client->head.len - sizeof(client->head);
        msghdr.msg_iovlen = 2;
    }
    else
    {
        vec[0].iov_base = client->data + client->count - sizeof(client->head);
        vec[0].iov_len  = client->head.len - client->count;
        msghdr.msg_iovlen = 1;
    }

#ifdef HAVE_MSGHDR_ACCRIGHTS
    if (client->pass_fd != -1)  /* we have an fd to send */
    {
        msghdr.msg_accrights = (void *)&client->pass_fd;
        msghdr.msg_accrightslen = sizeof(client->pass_fd);
    }
    else
    {
        msghdr.msg_accrights = NULL;
        msghdr.msg_accrightslen = 0;
    }
#else  /* HAVE_MSGHDR_ACCRIGHTS */
    if (client->pass_fd != -1)  /* we have an fd to send */
    {
        cmsg.len = sizeof(cmsg);
        cmsg.level = SOL_SOCKET;
        cmsg.type = SCM_RIGHTS;
        cmsg.fd = client->pass_fd;
        msghdr.msg_control = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
    }
    else
    {
        msghdr.msg_control = NULL;
        msghdr.msg_controllen = 0;
    }
    msghdr.msg_flags = 0;
#endif  /* HAVE_MSGHDR_ACCRIGHTS */

    ret = sendmsg( client_fd, &msghdr, 0 );
    if (ret == -1)
    {
        if (errno != EPIPE) perror("sendmsg");
        remove_client( client_fd, BROKEN_PIPE );
        return;
    }
    if (client->pass_fd != -1)  /* We sent the fd, now we can close it */
    {
        close( client->pass_fd );
        client->pass_fd = -1;
    }
    if ((client->count += ret) < client->head.len) return;

    /* we have finished with this message */
    if (client->data) free( client->data );
    client->data  = NULL;
    client->count = 0;
    client->state = RUNNING;
    client->seq++;
    set_select_events( client_fd, READ_EVENT );
}


/* read a message from a client that has something to say */
static void do_read( struct client *client, int client_fd )
{
    struct iovec vec;
    int pass_fd = -1;
    int ret;

#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr;

    msghdr.msg_accrights    = (void *)&pass_fd;
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

    if (client->count < sizeof(client->head))
    {
        vec.iov_base = (char *)&client->head + client->count;
        vec.iov_len  = sizeof(client->head) - client->count;
    }
    else
    {
        if (!client->data &&
            !(client->data = malloc(client->head.len-sizeof(client->head))))
        {
            remove_client( client_fd, OUT_OF_MEMORY );
            return;
        }
        vec.iov_base = client->data + client->count - sizeof(client->head);
        vec.iov_len  = client->head.len - client->count;
    }

    ret = recvmsg( client_fd, &msghdr, 0 );
    if (ret == -1)
    {
        perror("recvmsg");
        remove_client( client_fd, BROKEN_PIPE );
        return;
    }
#ifndef HAVE_MSGHDR_ACCRIGHTS
    pass_fd = cmsg.fd;
#endif
    if (pass_fd != -1)
    {
        /* can only receive one fd per message */
        if (client->pass_fd != -1) close( client->pass_fd );
        client->pass_fd = pass_fd;
    }
    else if (!ret)  /* closed pipe */
    {
        remove_client( client_fd, BROKEN_PIPE );
        return;
    }

    if (client->state == RUNNING) client->state = SENDING;
    assert( client->state == SENDING );

    client->count += ret;

    /* received the complete header yet? */
    if (client->count < sizeof(client->head)) return;

    /* sanity checks */
    if (client->head.seq != client->seq)
    {
        protocol_error( client_fd, "bad sequence %08x instead of %08x\n",
                        client->head.seq, client->seq );
        remove_client( client_fd, PROTOCOL_ERROR );
        return;
    }
    if ((client->head.len < sizeof(client->head)) ||
        (client->head.len > MAX_MSG_LENGTH + sizeof(client->head)))
    {
        protocol_error( client_fd, "bad header length %08x\n",
                        client->head.len );
        remove_client( client_fd, PROTOCOL_ERROR );
        return;
    }

    /* received the whole message? */
    if (client->count == client->head.len)
    {
        /* done reading the data, call the callback function */

        int len = client->head.len - sizeof(client->head);
        char *data = client->data;
        int passed_fd = client->pass_fd;
        enum request type = client->head.type;

        /* clear the info now, as the client may be deleted by the callback */
        client->head.len  = 0;
        client->head.type = 0;
        client->count     = 0;
        client->data      = NULL;
        client->pass_fd   = -1;
        client->state     = WAITING;
        client->seq++;

        call_req_handler( client->self, type, data, len, passed_fd );
        if (passed_fd != -1) close( passed_fd );
        if (data) free( data );
    }
}

/* handle a client timeout */
static void client_timeout( int client_fd, void *private )
{
    struct client *client = (struct client *)private;
    set_select_timeout( client_fd, 0 );  /* Remove the timeout */
    call_timeout_handler( client->self );
}

/* handle a client event */
static void client_event( int client_fd, int event, void *private )
{
    struct client *client = (struct client *)private;
    if (event & WRITE_EVENT)
        do_write( client, client_fd );
    if (event & READ_EVENT)
        do_read( client, client_fd );
}

static const struct select_ops client_ops =
{
    client_event,
    client_timeout
};

/*******************************************************************/
/* server-side exported functions                                  */

/* add a client */
int add_client( int client_fd, struct thread *self )
{
    struct client *client = malloc( sizeof(*client) );
    if (!client) return -1;

    client->state                = RUNNING;
    client->seq                  = 0;
    client->head.len             = 0;
    client->head.type            = 0;
    client->count                = 0;
    client->data                 = NULL;
    client->self                 = self;
    client->pass_fd              = -1;

    if (add_select_user( client_fd, READ_EVENT, &client_ops, client ) == -1)
    {
        free( client );
        return -1;
    }
    return client_fd;
}

/* remove a client */
void remove_client( int client_fd, int exit_code )
{
    struct client *client = (struct client *)get_select_private_data( &client_ops, client_fd );
    assert( client );

    call_kill_handler( client->self, exit_code );

    remove_select_user( client_fd );
    close( client_fd );

    /* Purge messages */
    if (client->data) free( client->data );
    if (client->pass_fd != -1) close( client->pass_fd );
    free( client );
}

/* send a reply to a client */
int send_reply_v( int client_fd, int type, int pass_fd,
                  struct iovec *vec, int veclen )
{
    int i;
    unsigned int len;
    char *p;
    struct client *client = (struct client *)get_select_private_data( &client_ops, client_fd );

    assert( client );
    assert( client->state == WAITING );
    assert( !client->data );

    if (debug_level) trace_reply( client->self, type, pass_fd, vec, veclen );

    for (i = len = 0; i < veclen; i++) len += vec[i].iov_len;
    assert( len < MAX_MSG_LENGTH );

    if (len && !(client->data = malloc( len ))) return -1;
    client->count     = 0;
    client->head.len  = len + sizeof(client->head);
    client->head.type = type;
    client->head.seq  = client->seq;
    client->pass_fd   = pass_fd;

    for (i = 0, p = client->data; i < veclen; i++)
    {
        memcpy( p, vec[i].iov_base, vec[i].iov_len );
        p += vec[i].iov_len;
    }

    client->state = READING;
    set_select_events( client_fd, WRITE_EVENT );
    return 0;
}

/*
 * Server-side socket communication functions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
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

/* client timeout */
struct timeout
{
    struct timeval  when;    /* timeout expiry (absolute time) */
    struct timeout *next;    /* next in sorted list */
    struct timeout *prev;    /* prev in sorted list */
    int             client;  /* client id */
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
    struct timeout     timeout;      /* client timeout */
};


static struct client *clients[FD_SETSIZE];  /* clients array */
static fd_set read_set, write_set;          /* current select sets */
static int nb_clients;                      /* current number of clients */
static int max_fd;                          /* max fd in use */
static int initial_client_fd;               /* fd of the first client */
static struct timeout *timeout_head;        /* sorted timeouts list head */
static struct timeout *timeout_tail;        /* sorted timeouts list tail */

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
static void do_write( int client_fd )
{
    struct client *client = clients[client_fd];
    struct iovec vec[2];
#ifndef HAVE_MSGHDR_ACCRIGHTS
    struct cmsg_fd cmsg  = { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS,
                             client->pass_fd };
#endif
    struct msghdr msghdr = { NULL, 0, vec, 2, };
    int ret;

    /* make sure we have something to send */
    assert( client->count < client->head.len );
    /* make sure the client is listening */
    assert( client->state == READING );

    if (client->count < sizeof(client->head))
    {
        vec[0].iov_base = (char *)&client->head + client->count;
        vec[0].iov_len  = sizeof(client->head) - client->count;
        vec[1].iov_base = client->data;
        vec[1].iov_len  = client->head.len - sizeof(client->head);
    }
    else
    {
        vec[0].iov_base = client->data + client->count - sizeof(client->head);
        vec[0].iov_len  = client->head.len - client->count;
        msghdr.msg_iovlen = 1;
    }
    if (client->pass_fd != -1)  /* we have an fd to send */
    {
#ifdef HAVE_MSGHDR_ACCRIGHTS
        msghdr.msg_accrights = (void *)&client->pass_fd;
        msghdr.msg_accrightslen = sizeof(client->pass_fd);
#else
        msghdr.msg_control = &cmsg;
        msghdr.msg_controllen = sizeof(cmsg);
#endif
    }
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
    FD_CLR( client_fd, &write_set );
    FD_SET( client_fd, &read_set );
}


/* read a message from a client that has something to say */
static void do_read( int client_fd )
{
    struct client *client = clients[client_fd];
    struct iovec vec;
    int pass_fd = -1;
#ifdef HAVE_MSGHDR_ACCRIGHTS
    struct msghdr msghdr = { NULL, 0, &vec, 1, (void*)&pass_fd, sizeof(int) };
#else
    struct cmsg_fd cmsg  = { sizeof(cmsg), SOL_SOCKET, SCM_RIGHTS, -1 };
    struct msghdr msghdr = { NULL, 0, &vec, 1, &cmsg, sizeof(cmsg), 0 };
#endif
    int ret;

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
static void do_timeout( int client_fd )
{
    struct client *client = clients[client_fd];
    set_timeout( client_fd, 0 );  /* Remove the timeout */
    call_timeout_handler( client->self );
}


/* server main loop */
void server_main_loop( int fd )
{
    int i, ret;

    setsid();
    signal( SIGPIPE, SIG_IGN );

    /* special magic to create the initial thread */
    initial_client_fd = fd;
    add_client( initial_client_fd, NULL );

    while (nb_clients)
    {
        fd_set read = read_set, write = write_set;
#if 0
        printf( "select: " );
        for (i = 0; i <= max_fd; i++) printf( "%c", FD_ISSET( i, &read_set ) ? 'r' :
                                                    (FD_ISSET( i, &write_set ) ? 'w' : '-') );
        printf( "\n" );
#endif
        if (timeout_head)
        {
            struct timeval tv, now;
            gettimeofday( &now, NULL );
            if ((timeout_head->when.tv_sec < now.tv_sec) ||
                ((timeout_head->when.tv_sec == now.tv_sec) &&
                 (timeout_head->when.tv_usec < now.tv_usec)))
            {
                do_timeout( timeout_head->client );
                continue;
            }
            tv.tv_sec = timeout_head->when.tv_sec - now.tv_sec;
            if ((tv.tv_usec = timeout_head->when.tv_usec - now.tv_usec) < 0)
            {
                tv.tv_usec += 1000000;
                tv.tv_sec--;
            }
            ret = select( max_fd + 1, &read, &write, NULL, &tv );
        }
        else  /* no timeout */
        {
            ret = select( max_fd + 1, &read, &write, NULL, NULL );
        }

        if (!ret) continue;
        if (ret == -1) perror("select");

        for (i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET( i, &write ))
            {
                if (clients[i]) do_write( i );
            }
            else if (FD_ISSET( i, &read ))
            {
                if (clients[i]) do_read( i );
            }
        }
    }
}


/*******************************************************************/
/* server-side exported functions                                  */

/* add a client */
int add_client( int client_fd, struct thread *self )
{
    int flags;
    struct client *client = malloc( sizeof(*client) );
    if (!client) return -1;
    assert( !clients[client_fd] );

    client->state                = RUNNING;
    client->seq                  = 0;
    client->head.len             = 0;
    client->head.type            = 0;
    client->count                = 0;
    client->data                 = NULL;
    client->self                 = self;
    client->pass_fd              = -1;
    client->timeout.when.tv_sec  = 0;
    client->timeout.when.tv_usec = 0;
    client->timeout.client       = client_fd;

    flags = fcntl( client_fd, F_GETFL, 0 );
    fcntl( client_fd, F_SETFL, flags | O_NONBLOCK );

    clients[client_fd] = client;
    FD_SET( client_fd, &read_set );
    if (client_fd > max_fd) max_fd = client_fd;
    nb_clients++;
    return client_fd;
}

/* remove a client */
void remove_client( int client_fd, int exit_code )
{
    struct client *client = clients[client_fd];
    assert( client );

    call_kill_handler( client->self, exit_code );

    set_timeout( client_fd, 0 );
    clients[client_fd] = NULL;
    FD_CLR( client_fd, &read_set );
    FD_CLR( client_fd, &write_set );
    if (max_fd == client_fd) while (max_fd && !clients[max_fd]) max_fd--;
    if (initial_client_fd == client_fd) initial_client_fd = -1;
    close( client_fd );
    nb_clients--;

    /* Purge messages */
    if (client->data) free( client->data );
    if (client->pass_fd != -1) close( client->pass_fd );
    free( client );
}

/* return the fd of the initial client */
int get_initial_client_fd(void)
{
    assert( initial_client_fd != -1 );
    return initial_client_fd;
}

/* set a client timeout */
void set_timeout( int client_fd, struct timeval *when )
{
    struct timeout *tm, *pos;
    struct client *client = clients[client_fd];
    assert( client );

    tm = &client->timeout;
    if (tm->when.tv_sec || tm->when.tv_usec)
    {
        /* there is already a timeout */
        if (tm->next) tm->next->prev = tm->prev;
        else timeout_tail = tm->prev;
        if (tm->prev) tm->prev->next = tm->next;
        else timeout_head = tm->next;
        tm->when.tv_sec = tm->when.tv_usec = 0;
    }
    if (!when) return;  /* no timeout */
    tm->when = *when;

    /* Now insert it in the linked list */

    for (pos = timeout_head; pos; pos = pos->next)
    {
        if (pos->when.tv_sec > tm->when.tv_sec) break;
        if ((pos->when.tv_sec == tm->when.tv_sec) &&
            (pos->when.tv_usec > tm->when.tv_usec)) break;
    }

    if (pos)  /* insert it before 'pos' */
    {
        if ((tm->prev = pos->prev)) tm->prev->next = tm;
        else timeout_head = tm;
        tm->next = pos;
        pos->prev = tm;
    }
    else  /* insert it at the tail */
    {
        tm->next = NULL;
        if (timeout_tail) timeout_tail->next = tm;
        else timeout_head = tm;
        tm->prev = timeout_tail;
        timeout_tail = tm;
    }
}


/* send a reply to a client */
int send_reply_v( int client_fd, int type, int pass_fd,
                  struct iovec *vec, int veclen )
{
    int i;
    unsigned int len;
    char *p;
    struct client *client = clients[client_fd];

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
    FD_CLR( client_fd, &read_set );
    FD_SET( client_fd, &write_set );
    return 0;
}

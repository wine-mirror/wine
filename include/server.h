/*
 * Wine server definitions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_H
#define __WINE_SERVER_H

/* message header as sent on the wire */
struct header
{
    unsigned int  len;     /* total msg length (including this header) */
    unsigned int  type;    /* msg type */
    unsigned int  seq;     /* sequence number */
};

/* max msg length (not including the header) */
#define MAX_MSG_LENGTH (16384 - sizeof(struct header))

/* request from client to server */

enum request
{
    REQ_TIMEOUT,         /* internal timeout msg */
    REQ_KILL_THREAD,     /* internal kill thread msg */
    REQ_NEW_THREAD,      /* create a new thread (called from the creator) */
    REQ_INIT_THREAD,     /* init a new thread (called by itself) */
    REQ_NB_REQUESTS
};

/* request structures */

struct new_thread_request
{
    void *pid;  /* process id for the new thread (or 0 if none yet) */
};

struct new_thread_reply
{
    void *tid;  /* thread id */
    void *pid;  /* process id (created if necessary) */
};

struct init_thread_request
{
    int  pid;
/*    char name[...];*/
};

/* server-side functions */

extern void server_main_loop( int fd );


/* client-side functions */

#ifndef __WINE_SERVER__
struct _THDB;
extern int CLIENT_NewThread( struct _THDB *thdb );
extern int CLIENT_InitThread(void);
#endif  /* __WINE_SERVER__ */

#endif  /* __WINE_SERVER_H */

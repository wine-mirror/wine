/*
 * Wine server threads
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_THREAD_H
#define __WINE_SERVER_THREAD_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include "server/object.h"

/* thread structure */

struct process;

struct thread
{
    struct object   obj;       /* object header */
    struct thread  *next;      /* system-wide thread list */
    struct thread  *prev;
    struct thread  *proc_next; /* per-process thread list */
    struct thread  *proc_prev;
    struct process *process;
    int             error;     /* current error code */
    int             exit_code; /* thread exit code */
    int             client_fd; /* client fd for socket communications */
    int             unix_pid;  /* Unix pid of client */
    enum request    last_req;  /* last request received (for debugging) */
    char           *name;
};

extern struct thread *current;

/* thread functions */

extern struct thread *create_thread( int fd, void *pid, int *thread_handle,
                                     int *process_handle );
extern struct thread *get_thread_from_id( void *id );
extern struct thread *get_thread_from_handle( int handle, unsigned int access );
extern int send_reply( struct thread *thread, int pass_fd,
                       int n, ... /* arg_1, len_1, ..., arg_n, len_n */ );
extern void kill_thread( struct thread *thread, int exit_code );
extern void thread_killed( struct thread *thread, int exit_code );

#define SET_ERROR(err)  (current->error = (err))
#define CLEAR_ERROR()   (current->error = 0)

#endif  /* __WINE_SERVER_THREAD_H */

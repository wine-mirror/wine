/*
 * Wine server definitions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_H
#define __WINE_SERVER_H

#include <stdlib.h>
#include <time.h>

/* message header as sent on the wire */
struct header
{
    unsigned int  len;     /* total msg length (including this header) */
    unsigned int  type;    /* msg type */
    unsigned int  seq;     /* sequence number */
};

/* max msg length (not including the header) */
#define MAX_MSG_LENGTH (16384 - sizeof(struct header))

/* data structure used to pass an fd with sendmsg/recvmsg */
struct cmsg_fd
{
    int len;   /* sizeof structure */
    int level; /* SOL_SOCKET */
    int type;  /* SCM_RIGHTS */
    int fd;    /* fd to pass */
};


/* Request structures */

/* following are the definitions of all the client<->server  */
/* communication format; requests are from client to server, */
/* replies are from server to client. All requests must have */
/* a corresponding structure; the replies can be empty in    */
/* which case it isn't necessary to define a structure.      */


/* Create a new thread from the context of the parent */
struct new_thread_request
{
    void*        pid;          /* process id for the new thread (or 0 if none yet) */
};
struct new_thread_reply
{
    void*        tid;          /* thread id */
    int          thandle;      /* thread handle (in the current process) */
    void*        pid;          /* process id (created if necessary) */
    int          phandle;      /* process handle (in the current process) */
};


/* Set the server debug level */
struct set_debug_request
{
    int          level;        /* New debug level */
};


/* Initialize a thread; called from the child after fork()/clone() */
struct init_thread_request
{
    int          unix_pid;     /* Unix pid of new thread */
    char         cmd_line[0];  /* Thread command line */
};


/* Terminate a process */
struct terminate_process_request
{
    int          handle;       /* process handle to terminate */
    int          exit_code;    /* process exit code */
};


/* Terminate a thread */
struct terminate_thread_request
{
    int          handle;       /* thread handle to terminate */
    int          exit_code;    /* thread exit code */
};


/* Retrieve information about a process */
struct get_process_info_request
{
    int          handle;       /* process handle */
};
struct get_process_info_reply
{
    void*        pid;          /* server process id */
    int          exit_code;    /* process exit code */
};


/* Retrieve information about a thread */
struct get_thread_info_request
{
    int          handle;       /* thread handle */
};
struct get_thread_info_reply
{
    void*        pid;          /* server thread id */
    int          exit_code;    /* thread exit code */
};


/* Close a handle for the current process */
struct close_handle_request
{
    int          handle;       /* handle to close */
};


/* Duplicate a handle */
struct dup_handle_request
{
    int          src_process;  /* src process handle */
    int          src_handle;   /* src handle to duplicate */
    int          dst_process;  /* dst process handle */
    int          dst_handle;   /* handle to duplicate to (or -1 for any) */
    unsigned int access;       /* wanted access rights */
    int          inherit;      /* inherit flag */
    int          options;      /* duplicate options (see below) */
};
struct dup_handle_reply
{
    int          handle;       /* duplicated handle in dst process */
};


/* Open a handle to a process */
struct open_process_request
{
    void*        pid;          /* process id to open */
    unsigned int access;       /* wanted access rights */
    int          inherit;      /* inherit flag */
};
struct open_process_reply
{
    int          handle;       /* handle to the process */
};


/* Wait for handles */
struct select_request
{
    int          count;        /* handles count */
    int          flags;        /* wait flags (see below) */
    int          timeout;      /* timeout in ms */
    /* int handles[] */
};
struct select_reply
{
    int          signaled;     /* signaled handle */
};
#define SELECT_ALL       1
#define SELECT_MSG       2
#define SELECT_ALERTABLE 4
#define SELECT_TIMEOUT   8


/* Create an event */
struct create_event_request
{
    int          manual_reset;  /* manual reset event */
    int          initial_state; /* initial state of the event */
    int          inherit;       /* inherit flag */
    char         name[0];       /* event name */
};
struct create_event_reply
{
    int          handle;        /* handle to the event */
};

/* Event operation */
struct event_op_request
{
    int           handle;       /* handle to event */
    int           op;           /* event operation (see below) */
};
enum event_op { PULSE_EVENT, SET_EVENT, RESET_EVENT };


/* Create a mutex */
struct create_mutex_request
{
    int          owned;         /* initially owned? */
    int          inherit;       /* inherit flag */
    char         name[0];       /* mutex name */
};
struct create_mutex_reply
{
    int          handle;        /* handle to the mutex */
};


/* Release a mutex */
struct release_mutex_request
{
    int          handle;        /* handle to the mutex */
};


/* Create a semaphore */
struct create_semaphore_request
{
    unsigned int initial;       /* initial count */
    unsigned int max;           /* maximum count */
    int          inherit;       /* inherit flag */
    /* char name[] */
};
struct create_semaphore_reply
{
    int          handle;        /* handle to the semaphore */
};


/* Release a semaphore */
struct release_semaphore_request
{
    int          handle;        /* handle to the semaphore */
    unsigned int count;         /* count to add to semaphore */
};
struct release_semaphore_reply
{
    unsigned int prev_count;    /* previous semaphore count */
};


/* Open a named object (event, mutex, semaphore) */
struct open_named_obj_request
{
    int          type;         /* object type (see below) */
    unsigned int access;       /* wanted access rights */
    int          inherit;      /* inherit flag */
    /* char name[] */
};
enum open_named_obj { OPEN_EVENT, OPEN_MUTEX, OPEN_SEMAPHORE };

struct open_named_obj_reply
{
    int          handle;        /* handle to the object */
};


/* Create a file */
struct create_file_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
};
struct create_file_reply
{
    int          handle;        /* handle to the file */
};


/* Get a Unix handle to a file */
struct get_unix_handle_request
{
    int          handle;        /* handle to the file */
    unsigned int access;        /* desired access */
};


/* Get a Unix fd to read from a file */
struct get_read_fd_request
{
    int          handle;        /* handle to the file */
};


/* Get a Unix fd to write to a file */
struct get_write_fd_request
{
    int          handle;        /* handle to the file */
};


/* Set a file current position */
struct set_file_pointer_request
{
    int          handle;        /* handle to the file */
    int          low;           /* position low word */
    int          high;          /* position high word */
    int          whence;        /* whence to seek */
};
struct set_file_pointer_reply
{
    int          low;           /* new position low word */
    int          high;          /* new position high word */
};


/* Truncate (or extend) a file */
struct truncate_file_request
{
    int          handle;        /* handle to the file */
};


/* Flush a file buffers */
struct flush_file_request
{
    int          handle;        /* handle to the file */
};


/* Get information about a file */
struct get_file_info_request
{
    int          handle;        /* handle to the file */
};
struct get_file_info_reply
{
    int          attr;          /* file attributes */
    time_t       access_time;   /* last access time */
    time_t       write_time;    /* last write time */
    int          size_high;     /* file size */
    int          size_low;      /* file size */
    int          links;         /* number of links */
    int          index_high;    /* unique index */
    int          index_low;     /* unique index */
    unsigned int serial;        /* volume serial number */
};


/* Create an anonymous pipe */
struct create_pipe_request
{
    int          inherit;       /* inherit flag */
};
struct create_pipe_reply
{
    int          handle_read;   /* handle to the read-side of the pipe */
    int          handle_write;  /* handle to the write-side of the pipe */
};


/* Create a console */
struct create_console_request
{
    int          inherit;       /* inherit flag */
};
struct create_console_reply
{
    int          handle_read;   /* handle to read from the console */
    int          handle_write;  /* handle to write to the console */
};


/* Set a console file descriptor */
struct set_console_fd_request
{
    int          handle;        /* handle to the console */
};


/* client-side functions */

#ifndef __WINE_SERVER__

/* client communication functions */
enum request;
#define CHECK_LEN(len,wanted) \
  if ((len) == (wanted)) ; \
  else CLIENT_ProtocolError( __FUNCTION__ ": len %d != %d\n", (len), (wanted) );
extern void CLIENT_ProtocolError( const char *err, ... );
extern void CLIENT_SendRequest( enum request req, int pass_fd,
                                int n, ... /* arg_1, len_1, etc. */ );
extern unsigned int CLIENT_WaitReply( int *len, int *passed_fd,
                                      int n, ... /* arg_1, len_1, etc. */ );

struct _THDB;
extern int CLIENT_NewThread( struct _THDB *thdb, int *thandle, int *phandle );
extern int CLIENT_SetDebug( int level );
extern int CLIENT_InitThread(void);
extern int CLIENT_TerminateProcess( int handle, int exit_code );
extern int CLIENT_TerminateThread( int handle, int exit_code );
extern int CLIENT_CloseHandle( int handle );
extern int CLIENT_DuplicateHandle( int src_process, int src_handle, int dst_process,
                                   int dst_handle, DWORD access, BOOL32 inherit, DWORD options );
extern int CLIENT_GetThreadInfo( int handle, struct get_thread_info_reply *reply );
extern int CLIENT_GetProcessInfo( int handle, struct get_process_info_reply *reply );
extern int CLIENT_OpenProcess( void *pid, DWORD access, BOOL32 inherit );
extern int CLIENT_Select( int count, int *handles, int flags, int timeout );
#endif  /* __WINE_SERVER__ */

#endif  /* __WINE_SERVER_H */

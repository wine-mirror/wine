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


/* Initialize a thread; called from the child after fork()/clone() */
struct init_thread_request
{
    int          unix_pid;     /* Unix pid of new thread */
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

/* client-side functions */

#ifndef __WINE_SERVER__
struct _THDB;
extern int CLIENT_NewThread( struct _THDB *thdb, int *thandle, int *phandle );
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

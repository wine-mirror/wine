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
};

/* max msg length (including the header) */
#define MAX_MSG_LENGTH  16384

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


/* Create a new process from the context of the parent */
struct new_process_request
{
    int          inherit;      /* inherit flag */
    int          inherit_all;  /* inherit all handles from parent */
    int          create_flags; /* creation flags */
    int          start_flags;  /* flags from startup info */
    int          hstdin;       /* handle for stdin */
    int          hstdout;      /* handle for stdout */
    int          hstderr;      /* handle for stderr */
    int          cmd_show;     /* main window show mode */
    void*        env_ptr;      /* pointer to environment (FIXME: hack) */
    char         cmd_line[0];  /* command line */
};
struct new_process_reply
{
    void*        pid;          /* process id */
    int          handle;       /* process handle (in the current process) */
};


/* Create a new thread from the context of the parent */
struct new_thread_request
{
    void*        pid;          /* process id for the new thread */
    int          suspend;      /* new thread should be suspended on creation */ 
    int          inherit;      /* inherit flag */
};
struct new_thread_reply
{
    void*        tid;          /* thread id */
    int          handle;       /* thread handle (in the current process) */
};


/* Set the server debug level */
struct set_debug_request
{
    int          level;        /* New debug level */
};


/* Initialize a process; called from the new process context */
struct init_process_request
{
    int          dummy;
};
struct init_process_reply
{
    int          start_flags;  /* flags from startup info */
    int          hstdin;       /* handle for stdin */
    int          hstdout;      /* handle for stdout */
    int          hstderr;      /* handle for stderr */
    int          cmd_show;     /* main window show mode */
    void*        env_ptr;      /* pointer to environment (FIXME: hack) */
    char         cmdline[0];   /* command line */
};


/* Initialize a thread; called from the child after fork()/clone() */
struct init_thread_request
{
    int          unix_pid;     /* Unix pid of new thread */
    void*        teb;          /* TEB of new thread (in thread address space) */
};
struct init_thread_reply
{
    void*        pid;          /* process id of the new thread's process */
    void*        tid;          /* thread id of the new thread */
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
    void*        pid;              /* server process id */
    int          exit_code;        /* process exit code */
    int          priority;         /* priority class */
    int          process_affinity; /* process affinity mask */
    int          system_affinity;  /* system affinity mask */
};


/* Set a process informations */
struct set_process_info_request
{
    int          handle;       /* process handle */
    int          mask;         /* setting mask (see below) */
    int          priority;     /* priority class */
    int          affinity;     /* affinity mask */
};
#define SET_PROCESS_INFO_PRIORITY 0x01
#define SET_PROCESS_INFO_AFFINITY 0x02


/* Retrieve information about a thread */
struct get_thread_info_request
{
    int          handle;       /* thread handle */
};
struct get_thread_info_reply
{
    void*        tid;          /* server thread id */
    int          exit_code;    /* thread exit code */
    int          priority;     /* thread priority level */
};


/* Set a thread informations */
struct set_thread_info_request
{
    int          handle;       /* thread handle */
    int          mask;         /* setting mask (see below) */
    int          priority;     /* priority class */
    int          affinity;     /* affinity mask */
};
#define SET_THREAD_INFO_PRIORITY 0x01
#define SET_THREAD_INFO_AFFINITY 0x02


/* Suspend a thread */
struct suspend_thread_request
{
    int          handle;       /* thread handle */
};
struct suspend_thread_reply
{
    int          count;        /* new suspend count */
};


/* Resume a thread */
struct resume_thread_request
{
    int          handle;       /* thread handle */
};
struct resume_thread_reply
{
    int          count;        /* new suspend count */
};


/* Debugger support: freeze / unfreeze */
struct debugger_request
{
    int          op;           /* operation type */
};

enum debugger_op { DEBUGGER_FREEZE_ALL, DEBUGGER_UNFREEZE_ALL };


/* Queue an APC for a thread */
struct queue_apc_request
{
    int          handle;       /* thread handle */
    void*        func;         /* function to call */
    void*        param;        /* param for function to call */
};


/* Close a handle for the current process */
struct close_handle_request
{
    int          handle;       /* handle to close */
};


/* Get information about a handle */
struct get_handle_info_request
{
    int          handle;       /* handle we are interested in */
};
struct get_handle_info_reply
{
    int          flags;        /* handle flags */
};


/* Set a handle information */
struct set_handle_info_request
{
    int          handle;       /* handle we are interested in */
    int          flags;        /* new handle flags */
    int          mask;         /* mask for flags to set */
};


/* Duplicate a handle */
struct dup_handle_request
{
    int          src_process;  /* src process handle */
    int          src_handle;   /* src handle to duplicate */
    int          dst_process;  /* dst process handle */
    unsigned int access;       /* wanted access rights */
    int          inherit;      /* inherit flag */
    int          options;      /* duplicate options (see below) */
};
#define DUP_HANDLE_CLOSE_SOURCE  DUPLICATE_CLOSE_SOURCE
#define DUP_HANDLE_SAME_ACCESS   DUPLICATE_SAME_ACCESS
#define DUP_HANDLE_MAKE_GLOBAL   0x80000000  /* Not a Windows flag */
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
    int          handles[0];   /* handles to select on */
};
struct select_reply
{
    int          signaled;     /* signaled handle */
    void*        apcs[0];      /* async procedures to call */
};
#define SELECT_ALL       1
#define SELECT_ALERTABLE 2
#define SELECT_TIMEOUT   4


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


/* Open an event */
struct open_event_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
    char         name[0];       /* object name */
};
struct open_event_reply
{
    int          handle;        /* handle to the event */
};


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


/* Open a mutex */
struct open_mutex_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
    char         name[0];       /* object name */
};
struct open_mutex_reply
{
    int          handle;        /* handle to the mutex */
};


/* Create a semaphore */
struct create_semaphore_request
{
    unsigned int initial;       /* initial count */
    unsigned int max;           /* maximum count */
    int          inherit;       /* inherit flag */
    char         name[0];       /* semaphore name */
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


/* Open a semaphore */
struct open_semaphore_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
    char         name[0];       /* object name */
};
struct open_semaphore_reply
{
    int          handle;        /* handle to the semaphore */
};


/* Create a file */
struct create_file_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
    unsigned int sharing;       /* sharing flags */
    int          create;        /* file create action */
    unsigned int attrs;         /* file attributes for creation */
    char         name[0];       /* file name */
};
struct create_file_reply
{
    int          handle;        /* handle to the file */
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


/* Set a file access and modification times */
struct set_file_time_request
{
    int          handle;        /* handle to the file */
    time_t       access_time;   /* last access time */
    time_t       write_time;    /* last write time */
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
    int          type;          /* file type */
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


/* Lock a region of a file */
struct lock_file_request
{
    int          handle;        /* handle to the file */
    unsigned int offset_low;    /* offset of start of lock */
    unsigned int offset_high;   /* offset of start of lock */
    unsigned int count_low;     /* count of bytes to lock */
    unsigned int count_high;    /* count of bytes to lock */
};


/* Unlock a region of a file */
struct unlock_file_request
{
    int          handle;        /* handle to the file */
    unsigned int offset_low;    /* offset of start of unlock */
    unsigned int offset_high;   /* offset of start of unlock */
    unsigned int count_low;     /* count of bytes to unlock */
    unsigned int count_high;    /* count of bytes to unlock */
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


/* Allocate a console for the current process */
struct alloc_console_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
};
struct alloc_console_reply
{
    int          handle_in;     /* handle to console input */
    int          handle_out;    /* handle to console output */
};


/* Free the console of the current process */
struct free_console_request
{
    int dummy;
};


/* Open a handle to the process console */
struct open_console_request
{
    int          output;        /* input or output? */
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
};
struct open_console_reply
{
    int          handle;        /* handle to the console */
};


/* Set a console file descriptor */
struct set_console_fd_request
{
    int          handle;        /* handle to the console */
    int          pid;           /* pid of xterm (hack) */
};


/* Get a console mode (input or output) */
struct get_console_mode_request
{
    int          handle;        /* handle to the console */
};
struct get_console_mode_reply
{
    int          mode;          /* console mode */
};


/* Set a console mode (input or output) */
struct set_console_mode_request
{
    int          handle;        /* handle to the console */
    int          mode;          /* console mode */
};


/* Set info about a console (output only) */
struct set_console_info_request
{
    int          handle;        /* handle to the console */
    int          mask;          /* setting mask (see below) */
    int          cursor_size;   /* size of cursor (percentage filled) */
    int          cursor_visible;/* cursor visibility flag */
    char         title[0];      /* console title */
};
#define SET_CONSOLE_INFO_CURSOR 0x01
#define SET_CONSOLE_INFO_TITLE  0x02

/* Get info about a console (output only) */
struct get_console_info_request
{
    int          handle;        /* handle to the console */
};
struct get_console_info_reply
{
    int          cursor_size;   /* size of cursor (percentage filled) */
    int          cursor_visible;/* cursor visibility flag */
    int          pid;           /* pid of xterm (hack) */
    char         title[0];      /* console title */
};


/* Add input records to a console input queue */
struct write_console_input_request
{
    int          handle;        /* handle to the console input */
    int          count;         /* number of input records */
/*  INPUT_RECORD records[0]; */ /* input records */
};
struct write_console_input_reply
{
    int          written;       /* number of records written */
};

/* Fetch input records from a console input queue */
struct read_console_input_request
{
    int          handle;        /* handle to the console input */
    int          count;         /* max number of records to retrieve */
    int          flush;         /* flush the retrieved records from the queue? */
};
struct read_console_input_reply
{
    int dummy;
/*  INPUT_RECORD records[0]; */ /* input records */
};


/* Create a change notification */
struct create_change_notification_request
{
    int          subtree;       /* watch all the subtree */
    int          filter;        /* notification filter */
};
struct create_change_notification_reply
{
    int          handle;        /* handle to the change notification */
};


/* Create a file mapping */
struct create_mapping_request
{
    int          size_high;     /* mapping size */
    int          size_low;      /* mapping size */
    int          protect;       /* protection flags (see below) */
    int          inherit;       /* inherit flag */
    int          handle;        /* file handle */
    char         name[0];       /* object name */
};
struct create_mapping_reply
{
    int          handle;        /* handle to the mapping */
};
/* protection flags */
#define VPROT_READ       0x01
#define VPROT_WRITE      0x02
#define VPROT_EXEC       0x04
#define VPROT_WRITECOPY  0x08
#define VPROT_GUARD      0x10
#define VPROT_NOCACHE    0x20
#define VPROT_COMMITTED  0x40


/* Open a mapping */
struct open_mapping_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
    char         name[0];       /* object name */
};
struct open_mapping_reply
{
    int          handle;        /* handle to the mapping */
};


/* Get information about a file mapping */
struct get_mapping_info_request
{
    int          handle;        /* handle to the mapping */
};
struct get_mapping_info_reply
{
    int          size_high;     /* mapping size */
    int          size_low;      /* mapping size */
    int          protect;       /* protection flags */
};


/* Create a device */
struct create_device_request
{
    unsigned int access;        /* wanted access rights */
    int          inherit;       /* inherit flag */
    int          id;            /* client private id */
};
struct create_device_reply
{
    int          handle;        /* handle to the device */
};


/* Create a snapshot */
struct create_snapshot_request
{
    int          inherit;       /* inherit flag */
    int          flags;         /* snapshot flags (TH32CS_*) */
};
struct create_snapshot_reply
{
    int          handle;        /* handle to the snapshot */
};


/* Get the next process from a snapshot */
struct next_process_request
{
    int          handle;        /* handle to the snapshot */
    int          reset;         /* reset snapshot position? */
};
struct next_process_reply
{
    void*        pid;          /* process id */
    int          threads;      /* number of threads */
    int          priority;     /* process priority */
};


/* Wait for a debug event */
struct wait_debug_event_request
{
    int          timeout;      /* timeout in ms */
};
struct wait_debug_event_reply
{
    int          code;         /* event code */
    void*        pid;          /* process id */
    void*        tid;          /* thread id */
    /* followed by the event data (see below) */
};


/* Send a debug event */
struct send_debug_event_request
{
    int          code;         /* event code */
    /* followed by the event data (see below) */
};
struct send_debug_event_reply
{
    int          status;       /* event continuation status */
};


/* definitions of the event data depending on the event code */
struct debug_event_exception
{
    int        code;           /* exception code */
    int        flags;          /* exception flags */
    void      *record;         /* exception record ptr */
    void      *addr;           /* exception address */
    int        nb_params;      /* exceptions parameters */
    int        params[15];
    int        first_chance;   /* first chance to handle it? */
};
struct debug_event_create_thread
{
    int         handle;     /* handle to the new thread */
    void       *teb;        /* thread teb (in debugged process address space) */
    void       *start;      /* thread startup routine */
};
struct debug_event_create_process
{
    int         file;       /* handle to the process exe file */
    int         process;    /* handle to the new process */
    int         thread;     /* handle to the new thread */
    void       *base;       /* base of executable image */
    int         dbg_offset; /* offset of debug info in file */
    int         dbg_size;   /* size of debug info */
    void       *teb;        /* thread teb (in debugged process address space) */
    void       *start;      /* thread startup routine */
    void       *name;       /* image name (optional) */
    int         unicode;    /* is it Unicode? */
};
struct debug_event_exit
{
    int         exit_code;  /* thread or process exit code */
};
struct debug_event_load_dll
{
    int         handle;     /* file handle for the dll */
    void       *base;       /* base address of the dll */
    int         dbg_offset; /* offset of debug info in file */
    int         dbg_size;   /* size of debug info */
    void       *name;       /* image name (optional) */
    int         unicode;    /* is it Unicode? */
};
struct debug_event_unload_dll
{
    void       *base;       /* base address of the dll */
};
struct debug_event_output_string
{
    void       *string;     /* string to display (in debugged process address space) */
    int         unicode;    /* is it Unicode? */
    int         length;     /* string length */
};
struct debug_event_rip_info
{
    int         error;      /* ??? */
    int         type;       /* ??? */
};
union debug_event_data
{
    struct debug_event_exception      exception;
    struct debug_event_create_thread  create_thread;
    struct debug_event_create_process create_process;
    struct debug_event_exit           exit;
    struct debug_event_load_dll       load_dll;
    struct debug_event_unload_dll     unload_dll;
    struct debug_event_output_string  output_string;
    struct debug_event_rip_info       rip_info;
};


/* Continue a debug event */
struct continue_debug_event_request
{
    void*        pid;          /* process id to continue */
    void*        tid;          /* thread id to continue */
    int          status;       /* continuation status */
};


/* Start debugging an existing process */
struct debug_process_request
{
    void*        pid;          /* id of the process to debug */
};


/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

enum request
{
    REQ_NEW_PROCESS,
    REQ_NEW_THREAD,
    REQ_SET_DEBUG,
    REQ_INIT_PROCESS,
    REQ_INIT_THREAD,
    REQ_TERMINATE_PROCESS,
    REQ_TERMINATE_THREAD,
    REQ_GET_PROCESS_INFO,
    REQ_SET_PROCESS_INFO,
    REQ_GET_THREAD_INFO,
    REQ_SET_THREAD_INFO,
    REQ_SUSPEND_THREAD,
    REQ_RESUME_THREAD,
    REQ_DEBUGGER,
    REQ_QUEUE_APC,
    REQ_CLOSE_HANDLE,
    REQ_GET_HANDLE_INFO,
    REQ_SET_HANDLE_INFO,
    REQ_DUP_HANDLE,
    REQ_OPEN_PROCESS,
    REQ_SELECT,
    REQ_CREATE_EVENT,
    REQ_EVENT_OP,
    REQ_OPEN_EVENT,
    REQ_CREATE_MUTEX,
    REQ_RELEASE_MUTEX,
    REQ_OPEN_MUTEX,
    REQ_CREATE_SEMAPHORE,
    REQ_RELEASE_SEMAPHORE,
    REQ_OPEN_SEMAPHORE,
    REQ_CREATE_FILE,
    REQ_GET_READ_FD,
    REQ_GET_WRITE_FD,
    REQ_SET_FILE_POINTER,
    REQ_TRUNCATE_FILE,
    REQ_SET_FILE_TIME,
    REQ_FLUSH_FILE,
    REQ_GET_FILE_INFO,
    REQ_LOCK_FILE,
    REQ_UNLOCK_FILE,
    REQ_CREATE_PIPE,
    REQ_ALLOC_CONSOLE,
    REQ_FREE_CONSOLE,
    REQ_OPEN_CONSOLE,
    REQ_SET_CONSOLE_FD,
    REQ_GET_CONSOLE_MODE,
    REQ_SET_CONSOLE_MODE,
    REQ_SET_CONSOLE_INFO,
    REQ_GET_CONSOLE_INFO,
    REQ_WRITE_CONSOLE_INPUT,
    REQ_READ_CONSOLE_INPUT,
    REQ_CREATE_CHANGE_NOTIFICATION,
    REQ_CREATE_MAPPING,
    REQ_OPEN_MAPPING,
    REQ_GET_MAPPING_INFO,
    REQ_CREATE_DEVICE,
    REQ_CREATE_SNAPSHOT,
    REQ_NEXT_PROCESS,
    REQ_WAIT_DEBUG_EVENT,
    REQ_SEND_DEBUG_EVENT,
    REQ_CONTINUE_DEBUG_EVENT,
    REQ_DEBUG_PROCESS,
    REQ_NB_REQUESTS
};

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */


/* client-side functions */

#ifndef __WINE_SERVER__

#include "thread.h"

/* make space for some data in the server arguments buffer */
static inline void *server_add_data( int len )
{
    void *old = NtCurrentTeb()->buffer_args;
    NtCurrentTeb()->buffer_args = (char *)old + len;
    return old;
}

/* maximum remaining size in the server arguments buffer */
static inline int server_remaining(void)
{
    TEB *teb = NtCurrentTeb();
    return (char *)teb->buffer + teb->buffer_size - (char *)teb->buffer_args;
}

extern unsigned int server_call( enum request req );
extern unsigned int server_call_fd( enum request req, int *fd );

/* client communication functions */
extern void CLIENT_ProtocolError( const char *err, ... );
extern void CLIENT_SendRequest( enum request req, int pass_fd,
                                int n, ... /* arg_1, len_1, etc. */ );
extern unsigned int CLIENT_WaitReply( int *len, int *passed_fd,
                                      int n, ... /* arg_1, len_1, etc. */ );
extern unsigned int CLIENT_WaitSimpleReply( void *reply, int len, int *passed_fd );
extern int CLIENT_InitServer(void);
extern int CLIENT_SetDebug( int level );
extern int CLIENT_DebuggerRequest( int op );
extern int CLIENT_InitThread(void);
#endif  /* __WINE_SERVER__ */

#endif  /* __WINE_SERVER_H */

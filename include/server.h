/*
 * Wine server definitions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_H
#define __WINE_SERVER_H

#include <stdlib.h>
#include <time.h>

/* Request structures */

/* Following are the definitions of all the client<->server   */
/* communication format; if you make any change in this file, */
/* you must run tools/make_requests again. */


/* These empty macros are used by tools/make_requests */
/* to generate the request/reply tracing functions */
#define IN  /*nothing*/
#define OUT /*nothing*/


/* Create a new process from the context of the parent */
struct new_process_request
{
    IN  int          inherit;      /* inherit flag */
    IN  int          inherit_all;  /* inherit all handles from parent */
    IN  int          create_flags; /* creation flags */
    IN  int          start_flags;  /* flags from startup info */
    IN  int          hstdin;       /* handle for stdin */
    IN  int          hstdout;      /* handle for stdout */
    IN  int          hstderr;      /* handle for stderr */
    IN  int          event;        /* event to signal startup */
    IN  int          cmd_show;     /* main window show mode */
    IN  void*        env_ptr;      /* pointer to environment (FIXME: hack) */
    OUT void*        pid;          /* process id */
    OUT int          handle;       /* process handle (in the current process) */
    IN  char         cmdline[1];   /* command line */
};


/* Create a new thread from the context of the parent */
struct new_thread_request
{
    IN  void*        pid;          /* process id for the new thread */
    IN  int          suspend;      /* new thread should be suspended on creation */ 
    IN  int          inherit;      /* inherit flag */
    OUT void*        tid;          /* thread id */
    OUT int          handle;       /* thread handle (in the current process) */
};


/* Set the server debug level */
struct set_debug_request
{
    IN  int          level;        /* New debug level */
};


/* Initialize a process; called from the new process context */
struct init_process_request
{
    OUT int          start_flags;  /* flags from startup info */
    OUT int          hstdin;       /* handle for stdin */
    OUT int          hstdout;      /* handle for stdout */
    OUT int          hstderr;      /* handle for stderr */
    OUT int          cmd_show;     /* main window show mode */
    OUT void*        env_ptr;      /* pointer to environment (FIXME: hack) */
    OUT char         cmdline[1];   /* command line */
};


/* Signal the end of the process initialization */
struct init_process_done_request
{
    IN  int          dummy;
};


/* Initialize a thread; called from the child after fork()/clone() */
struct init_thread_request
{
    IN  int          unix_pid;     /* Unix pid of new thread */
    IN  void*        teb;          /* TEB of new thread (in thread address space) */
    OUT void*        pid;          /* process id of the new thread's process */
    OUT void*        tid;          /* thread id of the new thread */
};


/* Retrieve the thread buffer file descriptor */
/* The reply to this request is the first thing a newly */
/* created thread gets (without having to request it) */
struct get_thread_buffer_request
{
    IN  int          dummy;
};


/* Terminate a process */
struct terminate_process_request
{
    IN  int          handle;       /* process handle to terminate */
    IN  int          exit_code;    /* process exit code */
};


/* Terminate a thread */
struct terminate_thread_request
{
    IN  int          handle;       /* thread handle to terminate */
    IN  int          exit_code;    /* thread exit code */
};


/* Retrieve information about a process */
struct get_process_info_request
{
    IN  int          handle;       /* process handle */
    OUT void*        pid;              /* server process id */
    OUT int          exit_code;        /* process exit code */
    OUT int          priority;         /* priority class */
    OUT int          process_affinity; /* process affinity mask */
    OUT int          system_affinity;  /* system affinity mask */
};


/* Set a process informations */
struct set_process_info_request
{
    IN  int          handle;       /* process handle */
    IN  int          mask;         /* setting mask (see below) */
    IN  int          priority;     /* priority class */
    IN  int          affinity;     /* affinity mask */
};
#define SET_PROCESS_INFO_PRIORITY 0x01
#define SET_PROCESS_INFO_AFFINITY 0x02


/* Retrieve information about a thread */
struct get_thread_info_request
{
    IN  int          handle;       /* thread handle */
    OUT void*        tid;          /* server thread id */
    OUT int          exit_code;    /* thread exit code */
    OUT int          priority;     /* thread priority level */
};


/* Set a thread informations */
struct set_thread_info_request
{
    IN  int          handle;       /* thread handle */
    IN  int          mask;         /* setting mask (see below) */
    IN  int          priority;     /* priority class */
    IN  int          affinity;     /* affinity mask */
};
#define SET_THREAD_INFO_PRIORITY 0x01
#define SET_THREAD_INFO_AFFINITY 0x02


/* Suspend a thread */
struct suspend_thread_request
{
    IN  int          handle;       /* thread handle */
    OUT int          count;        /* new suspend count */
};


/* Resume a thread */
struct resume_thread_request
{
    IN  int          handle;       /* thread handle */
    OUT int          count;        /* new suspend count */
};


/* Debugger support: freeze / unfreeze */
struct debugger_request
{
    IN  int          op;           /* operation type */
};

enum debugger_op { DEBUGGER_FREEZE_ALL, DEBUGGER_UNFREEZE_ALL };


/* Queue an APC for a thread */
struct queue_apc_request
{
    IN  int          handle;       /* thread handle */
    IN  void*        func;         /* function to call */
    IN  void*        param;        /* param for function to call */
};


/* Get list of APC to call */
struct get_apcs_request
{
    OUT int          count;        /* number of apcs */
    OUT void*        apcs[1];      /* async procedures to call */
};


/* Close a handle for the current process */
struct close_handle_request
{
    IN  int          handle;       /* handle to close */
};


/* Get information about a handle */
struct get_handle_info_request
{
    IN  int          handle;       /* handle we are interested in */
    OUT int          flags;        /* handle flags */
};


/* Set a handle information */
struct set_handle_info_request
{
    IN  int          handle;       /* handle we are interested in */
    IN  int          flags;        /* new handle flags */
    IN  int          mask;         /* mask for flags to set */
};


/* Duplicate a handle */
struct dup_handle_request
{
    IN  int          src_process;  /* src process handle */
    IN  int          src_handle;   /* src handle to duplicate */
    IN  int          dst_process;  /* dst process handle */
    IN  unsigned int access;       /* wanted access rights */
    IN  int          inherit;      /* inherit flag */
    IN  int          options;      /* duplicate options (see below) */
    OUT int          handle;       /* duplicated handle in dst process */
};
#define DUP_HANDLE_CLOSE_SOURCE  DUPLICATE_CLOSE_SOURCE
#define DUP_HANDLE_SAME_ACCESS   DUPLICATE_SAME_ACCESS
#define DUP_HANDLE_MAKE_GLOBAL   0x80000000  /* Not a Windows flag */


/* Open a handle to a process */
struct open_process_request
{
    IN  void*        pid;          /* process id to open */
    IN  unsigned int access;       /* wanted access rights */
    IN  int          inherit;      /* inherit flag */
    OUT int          handle;       /* handle to the process */
};


/* Wait for handles */
struct select_request
{
    IN  int          count;        /* handles count */
    IN  int          flags;        /* wait flags (see below) */
    IN  int          timeout;      /* timeout in ms */
    OUT int          signaled;     /* signaled handle */
    IN  int          handles[1];   /* handles to select on */
};
#define SELECT_ALL       1
#define SELECT_ALERTABLE 2
#define SELECT_TIMEOUT   4


/* Create an event */
struct create_event_request
{
    IN  int          manual_reset;  /* manual reset event */
    IN  int          initial_state; /* initial state of the event */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the event */
    IN  char         name[1];       /* event name */
};

/* Event operation */
struct event_op_request
{
    IN  int           handle;       /* handle to event */
    IN  int           op;           /* event operation (see below) */
};
enum event_op { PULSE_EVENT, SET_EVENT, RESET_EVENT };


/* Open an event */
struct open_event_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the event */
    IN  char         name[1];       /* object name */
};


/* Create a mutex */
struct create_mutex_request
{
    IN  int          owned;         /* initially owned? */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the mutex */
    IN  char         name[1];       /* mutex name */
};


/* Release a mutex */
struct release_mutex_request
{
    IN  int          handle;        /* handle to the mutex */
};


/* Open a mutex */
struct open_mutex_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the mutex */
    IN  char         name[1];       /* object name */
};


/* Create a semaphore */
struct create_semaphore_request
{
    IN  unsigned int initial;       /* initial count */
    IN  unsigned int max;           /* maximum count */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the semaphore */
    IN  char         name[1];       /* semaphore name */
};


/* Release a semaphore */
struct release_semaphore_request
{
    IN  int          handle;        /* handle to the semaphore */
    IN  unsigned int count;         /* count to add to semaphore */
    OUT unsigned int prev_count;    /* previous semaphore count */
};


/* Open a semaphore */
struct open_semaphore_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the semaphore */
    IN  char         name[1];       /* object name */
};


/* Create a file */
struct create_file_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    IN  unsigned int sharing;       /* sharing flags */
    IN  int          create;        /* file create action */
    IN  unsigned int attrs;         /* file attributes for creation */
    OUT int          handle;        /* handle to the file */
    IN  char         name[1];       /* file name */
};


/* Allocate a file handle for a Unix fd */
struct alloc_file_handle_request
{
    IN  unsigned int access;        /* wanted access rights */
    OUT int          handle;        /* handle to the file */
};


/* Get a Unix fd to read from a file */
struct get_read_fd_request
{
    IN  int          handle;        /* handle to the file */
};


/* Get a Unix fd to write to a file */
struct get_write_fd_request
{
    IN  int          handle;        /* handle to the file */
};


/* Set a file current position */
struct set_file_pointer_request
{
    IN  int          handle;        /* handle to the file */
    IN  int          low;           /* position low word */
    IN  int          high;          /* position high word */
    IN  int          whence;        /* whence to seek */
    OUT int          new_low;       /* new position low word */
    OUT int          new_high;      /* new position high word */
};


/* Truncate (or extend) a file */
struct truncate_file_request
{
    IN  int          handle;        /* handle to the file */
};


/* Set a file access and modification times */
struct set_file_time_request
{
    IN  int          handle;        /* handle to the file */
    IN  time_t       access_time;   /* last access time */
    IN  time_t       write_time;    /* last write time */
};


/* Flush a file buffers */
struct flush_file_request
{
    IN  int          handle;        /* handle to the file */
};


/* Get information about a file */
struct get_file_info_request
{
    IN  int          handle;        /* handle to the file */
    OUT int          type;          /* file type */
    OUT int          attr;          /* file attributes */
    OUT time_t       access_time;   /* last access time */
    OUT time_t       write_time;    /* last write time */
    OUT int          size_high;     /* file size */
    OUT int          size_low;      /* file size */
    OUT int          links;         /* number of links */
    OUT int          index_high;    /* unique index */
    OUT int          index_low;     /* unique index */
    OUT unsigned int serial;        /* volume serial number */
};


/* Lock a region of a file */
struct lock_file_request
{
    IN  int          handle;        /* handle to the file */
    IN  unsigned int offset_low;    /* offset of start of lock */
    IN  unsigned int offset_high;   /* offset of start of lock */
    IN  unsigned int count_low;     /* count of bytes to lock */
    IN  unsigned int count_high;    /* count of bytes to lock */
};


/* Unlock a region of a file */
struct unlock_file_request
{
    IN  int          handle;        /* handle to the file */
    IN  unsigned int offset_low;    /* offset of start of unlock */
    IN  unsigned int offset_high;   /* offset of start of unlock */
    IN  unsigned int count_low;     /* count of bytes to unlock */
    IN  unsigned int count_high;    /* count of bytes to unlock */
};


/* Create an anonymous pipe */
struct create_pipe_request
{
    IN  int          inherit;       /* inherit flag */
    OUT int          handle_read;   /* handle to the read-side of the pipe */
    OUT int          handle_write;  /* handle to the write-side of the pipe */
};


/* Create a socket */
struct create_socket_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    IN  int          family;        /* family, see socket manpage */
    IN  int          type;          /* type, see socket manpage */
    IN  int          protocol;      /* protocol, see socket manpage */
    OUT int          handle;        /* handle to the new socket */
};


/* Accept a socket */
struct accept_socket_request
{
    IN  int          lhandle;       /* handle to the listening socket */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the new socket */
};


/* Set socket event parameters */
struct set_socket_event_request
{
    IN  int          handle;        /* handle to the socket */
    IN  unsigned int mask;          /* event mask */
    IN  int          event;         /* event object */
};


/* Get socket event parameters */
struct get_socket_event_request
{
    IN  int          handle;        /* handle to the socket */
    IN  int          service;       /* clear pending? */
    IN  int          s_event;       /* "expected" event object */
    OUT unsigned int mask;          /* event mask */
    OUT unsigned int pmask;         /* pending events */
    OUT unsigned int state;         /* status bits */
    OUT int          errors[1];     /* event errors */
};


/* Reenable pending socket events */
struct enable_socket_event_request
{
    IN  int          handle;        /* handle to the socket */
    IN  unsigned int mask;          /* events to re-enable */
    IN  unsigned int sstate;        /* status bits to set */
    IN  unsigned int cstate;        /* status bits to clear */
};


/* Allocate a console for the current process */
struct alloc_console_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle_in;     /* handle to console input */
    OUT int          handle_out;    /* handle to console output */
};


/* Free the console of the current process */
struct free_console_request
{
    IN  int dummy;
};


/* Open a handle to the process console */
struct open_console_request
{
    IN  int          output;        /* input or output? */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the console */
};


/* Set a console file descriptor */
struct set_console_fd_request
{
    IN  int          handle;        /* handle to the console */
    IN  int          file_handle;   /* handle of file to use as file descriptor */
    IN  int          pid;           /* pid of xterm (hack) */
};


/* Get a console mode (input or output) */
struct get_console_mode_request
{
    IN  int          handle;        /* handle to the console */
    OUT int          mode;          /* console mode */
};


/* Set a console mode (input or output) */
struct set_console_mode_request
{
    IN  int          handle;        /* handle to the console */
    IN  int          mode;          /* console mode */
};


/* Set info about a console (output only) */
struct set_console_info_request
{
    IN  int          handle;        /* handle to the console */
    IN  int          mask;          /* setting mask (see below) */
    IN  int          cursor_size;   /* size of cursor (percentage filled) */
    IN  int          cursor_visible;/* cursor visibility flag */
    IN  char         title[1];      /* console title */
};
#define SET_CONSOLE_INFO_CURSOR 0x01
#define SET_CONSOLE_INFO_TITLE  0x02

/* Get info about a console (output only) */
struct get_console_info_request
{
    IN  int          handle;        /* handle to the console */
    OUT int          cursor_size;   /* size of cursor (percentage filled) */
    OUT int          cursor_visible;/* cursor visibility flag */
    OUT int          pid;           /* pid of xterm (hack) */
    OUT char         title[1];      /* console title */
};


/* Add input records to a console input queue */
struct write_console_input_request
{
    IN  int          handle;        /* handle to the console input */
    IN  int          count;         /* number of input records */
    OUT int          written;       /* number of records written */
 /* INPUT_RECORD records[0]; */     /* input records */
};

/* Fetch input records from a console input queue */
struct read_console_input_request
{
    IN  int          handle;        /* handle to the console input */
    IN  int          count;         /* max number of records to retrieve */
    IN  int          flush;         /* flush the retrieved records from the queue? */
    OUT int          read;          /* number of records read */
 /* INPUT_RECORD records[0]; */     /* input records */
};


/* Create a change notification */
struct create_change_notification_request
{
    IN  int          subtree;       /* watch all the subtree */
    IN  int          filter;        /* notification filter */
    OUT int          handle;        /* handle to the change notification */
};


/* Create a file mapping */
struct create_mapping_request
{
    IN  int          size_high;     /* mapping size */
    IN  int          size_low;      /* mapping size */
    IN  int          protect;       /* protection flags (see below) */
    IN  int          inherit;       /* inherit flag */
    IN  int          file_handle;   /* file handle */
    OUT int          handle;        /* handle to the mapping */
    IN  char         name[1];       /* object name */
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
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT int          handle;        /* handle to the mapping */
    IN  char         name[1];       /* object name */
};


/* Get information about a file mapping */
struct get_mapping_info_request
{
    IN  int          handle;        /* handle to the mapping */
    OUT int          size_high;     /* mapping size */
    OUT int          size_low;      /* mapping size */
    OUT int          protect;       /* protection flags */
};


/* Create a device */
struct create_device_request
{
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    IN  int          id;            /* client private id */
    OUT int          handle;        /* handle to the device */
};


/* Create a snapshot */
struct create_snapshot_request
{
    IN  int          inherit;       /* inherit flag */
    IN  int          flags;         /* snapshot flags (TH32CS_*) */
    OUT int          handle;        /* handle to the snapshot */
};


/* Get the next process from a snapshot */
struct next_process_request
{
    IN  int          handle;        /* handle to the snapshot */
    IN  int          reset;         /* reset snapshot position? */
    OUT void*        pid;          /* process id */
    OUT int          threads;      /* number of threads */
    OUT int          priority;     /* process priority */
};


/* Wait for a debug event */
struct wait_debug_event_request
{
    IN  int          timeout;      /* timeout in ms */
    OUT int          code;         /* event code */
    OUT void*        pid;          /* process id */
    OUT void*        tid;          /* thread id */
/*  OUT union debug_event_data data; */
};


/* Send a debug event */
struct send_debug_event_request
{
    IN  int          code;         /* event code */
    OUT int          status;       /* event continuation status */
/*  IN  union debug_event_data data; */
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
    IN  void*        pid;          /* process id to continue */
    IN  void*        tid;          /* thread id to continue */
    IN  int          status;       /* continuation status */
};


/* Start debugging an existing process */
struct debug_process_request
{
    IN  void*        pid;          /* id of the process to debug */
};


/* Read data from a process address space */
struct read_process_memory_request
{
    IN  int          handle;       /* process handle */
    IN  void*        addr;         /* addr to read from (must be int-aligned) */
    IN  int          len;          /* number of ints to read */
    OUT unsigned int data[1];      /* result data */
};


/* Write data to a process address space */
struct write_process_memory_request
{
    IN  int          handle;       /* process handle */
    IN  void*        addr;         /* addr to write to (must be int-aligned) */
    IN  int          len;          /* number of ints to write */
    IN  unsigned int first_mask;   /* mask for first word */
    IN  unsigned int last_mask;    /* mask for last word */
    IN  unsigned int data[1];      /* data to write */
};


/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

enum request
{
    REQ_NEW_PROCESS,
    REQ_NEW_THREAD,
    REQ_SET_DEBUG,
    REQ_INIT_PROCESS,
    REQ_INIT_PROCESS_DONE,
    REQ_INIT_THREAD,
    REQ_GET_THREAD_BUFFER,
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
    REQ_GET_APCS,
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
    REQ_ALLOC_FILE_HANDLE,
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
    REQ_CREATE_SOCKET,
    REQ_ACCEPT_SOCKET,
    REQ_SET_SOCKET_EVENT,
    REQ_GET_SOCKET_EVENT,
    REQ_ENABLE_SOCKET_EVENT,
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
    REQ_READ_PROCESS_MEMORY,
    REQ_WRITE_PROCESS_MEMORY,
    REQ_NB_REQUESTS
};

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */


/* client-side functions */

#ifndef __WINE_SERVER__

#include "thread.h"

/* client communication functions */

/* get a pointer to the request buffer */
static inline void * WINE_UNUSED get_req_buffer(void)
{
    return NtCurrentTeb()->buffer;
}

/* maximum remaining size in the server buffer */
static inline int WINE_UNUSED server_remaining( const void *ptr )
{
    return (char *)NtCurrentTeb()->buffer + NtCurrentTeb()->buffer_size - (char *)ptr;
}

extern unsigned int server_call( enum request req );
extern unsigned int server_call_fd( enum request req, int fd_out, int *fd_in );
extern void server_protocol_error( const char *err, ... );

extern int CLIENT_InitServer(void);
extern int CLIENT_SetDebug( int level );
extern int CLIENT_DebuggerRequest( int op );
extern int CLIENT_InitThread(void);
#endif  /* __WINE_SERVER__ */

#endif  /* __WINE_SERVER_H */

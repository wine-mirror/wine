/*
 * Wine server definitions
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_H
#define __WINE_SERVER_H

#include <stdlib.h>
#include <time.h>
#include "winbase.h"

/* Request structures */

/* Following are the definitions of all the client<->server   */
/* communication format; if you make any change in this file, */
/* you must run tools/make_requests again. */


/* These empty macros are used by tools/make_requests */
/* to generate the request/reply tracing functions */
#define IN  /*nothing*/
#define OUT /*nothing*/
#define VARARG(name,func) /*nothing*/

struct request_header
{
    IN  int            req;          /* request code */
    IN  unsigned short fixed_size;   /* size of the fixed part of the request */
    IN  unsigned short var_size;     /* size of the variable part of the request */
    OUT unsigned int   error;        /* error result */
};
#define REQUEST_HEADER  struct request_header header


/* placeholder structure for the maximum allowed request size */
/* this is used to construct the generic_request union */
struct request_max_size
{
    int pad[16]; /* the max request size is 16 ints */
};

/* max size of the variable part of a request */
#define REQUEST_MAX_VAR_SIZE  1024

typedef int handle_t;

/* definitions of the event data depending on the event code */
struct debug_event_exception
{
    EXCEPTION_RECORD record;   /* exception record */
    int              first;    /* first chance exception? */
};
struct debug_event_create_thread
{
    handle_t    handle;     /* handle to the new thread */
    void       *teb;        /* thread teb (in debugged process address space) */
    void       *start;      /* thread startup routine */
};
struct debug_event_create_process
{
    handle_t    file;       /* handle to the process exe file */
    handle_t    process;    /* handle to the new process */
    handle_t    thread;     /* handle to the new thread */
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
    handle_t    handle;     /* file handle for the dll */
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

/* debug event data */
typedef struct
{
    int                      code;   /* event code */
    union debug_event_data   info;   /* event information */
} debug_event_t;


/* Create a new process from the context of the parent */
struct new_process_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          inherit_all;  /* inherit all handles from parent */
    IN  int          create_flags; /* creation flags */
    IN  int          start_flags;  /* flags from startup info */
    IN  handle_t     exe_file;     /* file handle for main exe */
    IN  handle_t     hstdin;       /* handle for stdin */
    IN  handle_t     hstdout;      /* handle for stdout */
    IN  handle_t     hstderr;      /* handle for stderr */
    IN  int          cmd_show;     /* main window show mode */
    OUT handle_t     info;         /* new process info handle */
    IN  VARARG(filename,string);   /* file name of main exe */
};


/* Retrieve information about a newly started process */
struct get_new_process_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     info;         /* info handle returned from new_process_request */
    IN  int          pinherit;     /* process handle inherit flag */
    IN  int          tinherit;     /* thread handle inherit flag */
    OUT void*        pid;          /* process id */
    OUT handle_t     phandle;      /* process handle (in the current process) */
    OUT void*        tid;          /* thread id */
    OUT handle_t     thandle;      /* thread handle (in the current process) */
    OUT handle_t     event;        /* event handle to signal startup */
};


/* Create a new thread from the context of the parent */
struct new_thread_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          suspend;      /* new thread should be suspended on creation */
    IN  int          inherit;      /* inherit flag */
    OUT void*        tid;          /* thread id */
    OUT handle_t     handle;       /* thread handle (in the current process) */
};


/* Signal that we are finished booting on the client side */
struct boot_done_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          debug_level;  /* new debug level */
};


/* Initialize a process; called from the new process context */
struct init_process_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*        ldt_copy;     /* addr of LDT copy */
    IN  int          ppid;         /* parent Unix pid */
    OUT int          start_flags;  /* flags from startup info */
    OUT unsigned int server_start; /* server start time (GetTickCount) */
    OUT handle_t     exe_file;     /* file handle for main exe */
    OUT handle_t     hstdin;       /* handle for stdin */
    OUT handle_t     hstdout;      /* handle for stdout */
    OUT handle_t     hstderr;      /* handle for stderr */
    OUT int          cmd_show;     /* main window show mode */
    OUT VARARG(filename,string);   /* file name of main exe */
};


/* Signal the end of the process initialization */
struct init_process_done_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*        module;       /* main module base address */
    IN  void*        entry;        /* process entry point */
    IN  void*        name;         /* ptr to ptr to name (in process addr space) */
    IN  handle_t     exe_file;     /* file handle for main exe */
    IN  int          gui;          /* is it a GUI process? */
    OUT int          debugged;     /* being debugged? */
};


/* Initialize a thread; called from the child after fork()/clone() */
struct init_thread_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          unix_pid;     /* Unix pid of new thread */
    IN  void*        teb;          /* TEB of new thread (in thread address space) */
    IN  void*        entry;        /* thread entry point (in thread address space) */
};


/* Retrieve the thread buffer file descriptor */
/* The reply to this request is the first thing a newly */
/* created thread gets (without having to request it) */
struct get_thread_buffer_request
{
    REQUEST_HEADER;                /* request header */
    OUT void*        pid;          /* process id of the new thread's process */
    OUT void*        tid;          /* thread id of the new thread */
    OUT int          boot;         /* is this the boot thread? */
    OUT int          version;      /* protocol version */
};


/* Terminate a process */
struct terminate_process_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* process handle to terminate */
    IN  int          exit_code;    /* process exit code */
    OUT int          self;         /* suicide? */
};


/* Terminate a thread */
struct terminate_thread_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle to terminate */
    IN  int          exit_code;    /* thread exit code */
    OUT int          self;         /* suicide? */
    OUT int          last;         /* last thread in this process? */
};


/* Retrieve information about a process */
struct get_process_info_request
{
    REQUEST_HEADER;                    /* request header */
    IN  handle_t     handle;           /* process handle */
    OUT void*        pid;              /* server process id */
    OUT int          debugged;         /* debugged? */
    OUT int          exit_code;        /* process exit code */
    OUT int          priority;         /* priority class */
    OUT int          process_affinity; /* process affinity mask */
    OUT int          system_affinity;  /* system affinity mask */
};


/* Set a process informations */
struct set_process_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* process handle */
    IN  int          mask;         /* setting mask (see below) */
    IN  int          priority;     /* priority class */
    IN  int          affinity;     /* affinity mask */
};
#define SET_PROCESS_INFO_PRIORITY 0x01
#define SET_PROCESS_INFO_AFFINITY 0x02


/* Retrieve information about a thread */
struct get_thread_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    IN  void*        tid_in;       /* thread id (optional) */
    OUT void*        tid;          /* server thread id */
    OUT void*        teb;          /* thread teb pointer */
    OUT int          exit_code;    /* thread exit code */
    OUT int          priority;     /* thread priority level */
};


/* Set a thread informations */
struct set_thread_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    IN  int          mask;         /* setting mask (see below) */
    IN  int          priority;     /* priority class */
    IN  int          affinity;     /* affinity mask */
};
#define SET_THREAD_INFO_PRIORITY 0x01
#define SET_THREAD_INFO_AFFINITY 0x02


/* Suspend a thread */
struct suspend_thread_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    OUT int          count;        /* new suspend count */
};


/* Resume a thread */
struct resume_thread_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    OUT int          count;        /* new suspend count */
};


/* Notify the server that a dll has been loaded */
struct load_dll_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* file handle */
    IN  void*        base;         /* base address */
    IN  int          dbg_offset;   /* debug info offset */
    IN  int          dbg_size;     /* debug info size */
    IN  void*        name;         /* ptr to ptr to name (in process addr space) */
};


/* Notify the server that a dll is being unloaded */
struct unload_dll_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*        base;         /* base address */
};


/* Queue an APC for a thread */
struct queue_apc_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    IN  int          user;         /* user or system apc? */
    IN  void*        func;         /* function to call */
    IN  void*        param;        /* param for function to call */
};


/* Get next APC to call */
struct get_apc_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          alertable;    /* is thread alertable? */
    OUT void*        func;         /* function to call */
    OUT int          type;         /* function type */
    OUT VARARG(args,ptrs);         /* function arguments */
};
enum apc_type { APC_NONE, APC_USER, APC_TIMER, APC_ASYNC };


/* Close a handle for the current process */
struct close_handle_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* handle to close */
    OUT int          fd;           /* associated fd to close */
};


/* Set a handle information */
struct set_handle_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* handle we are interested in */
    IN  int          flags;        /* new handle flags */
    IN  int          mask;         /* mask for flags to set */
    IN  int          fd;           /* file descriptor or -1 */
    OUT int          old_flags;    /* old flag value */
    OUT int          cur_fd;       /* current file descriptor */
};


/* Duplicate a handle */
struct dup_handle_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     src_process;  /* src process handle */
    IN  handle_t     src_handle;   /* src handle to duplicate */
    IN  handle_t     dst_process;  /* dst process handle */
    IN  unsigned int access;       /* wanted access rights */
    IN  int          inherit;      /* inherit flag */
    IN  int          options;      /* duplicate options (see below) */
    OUT handle_t     handle;       /* duplicated handle in dst process */
    OUT int          fd;           /* associated fd to close */
};
#define DUP_HANDLE_CLOSE_SOURCE  DUPLICATE_CLOSE_SOURCE
#define DUP_HANDLE_SAME_ACCESS   DUPLICATE_SAME_ACCESS
#define DUP_HANDLE_MAKE_GLOBAL   0x80000000  /* Not a Windows flag */


/* Open a handle to a process */
struct open_process_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*        pid;          /* process id to open */
    IN  unsigned int access;       /* wanted access rights */
    IN  int          inherit;      /* inherit flag */
    OUT handle_t     handle;       /* handle to the process */
};


/* Wait for handles */
struct select_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          flags;        /* wait flags (see below) */
    IN  int          sec;          /* absolute timeout */
    IN  int          usec;         /* absolute timeout */
    OUT int          signaled;     /* signaled handle */
    IN  VARARG(handles,handles);   /* handles to select on */
};
#define SELECT_ALL           1
#define SELECT_ALERTABLE     2
#define SELECT_INTERRUPTIBLE 4
#define SELECT_TIMEOUT       8


/* Create an event */
struct create_event_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          manual_reset;  /* manual reset event */
    IN  int          initial_state; /* initial state of the event */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the event */
    IN  VARARG(name,unicode_str);   /* object name */
};

/* Event operation */
struct event_op_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t      handle;       /* handle to event */
    IN  int           op;           /* event operation (see below) */
};
enum event_op { PULSE_EVENT, SET_EVENT, RESET_EVENT };


/* Open an event */
struct open_event_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the event */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Create a mutex */
struct create_mutex_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          owned;         /* initially owned? */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the mutex */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Release a mutex */
struct release_mutex_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the mutex */
};


/* Open a mutex */
struct open_mutex_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the mutex */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Create a semaphore */
struct create_semaphore_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int initial;       /* initial count */
    IN  unsigned int max;           /* maximum count */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the semaphore */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Release a semaphore */
struct release_semaphore_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the semaphore */
    IN  unsigned int count;         /* count to add to semaphore */
    OUT unsigned int prev_count;    /* previous semaphore count */
};


/* Open a semaphore */
struct open_semaphore_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the semaphore */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Create a file */
struct create_file_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    IN  unsigned int sharing;       /* sharing flags */
    IN  int          create;        /* file create action */
    IN  unsigned int attrs;         /* file attributes for creation */
    OUT handle_t     handle;        /* handle to the file */
    IN  VARARG(filename,string);    /* file name */
};


/* Allocate a file handle for a Unix fd */
struct alloc_file_handle_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    OUT handle_t     handle;        /* handle to the file */
};


/* Get a Unix fd to access a file */
struct get_handle_fd_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
    IN  unsigned int access;        /* wanted access rights */
    OUT int          fd;            /* file descriptor */
};


/* Set a file current position */
struct set_file_pointer_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
    IN  int          low;           /* position low word */
    IN  int          high;          /* position high word */
    IN  int          whence;        /* whence to seek */
    OUT int          new_low;       /* new position low word */
    OUT int          new_high;      /* new position high word */
};


/* Truncate (or extend) a file */
struct truncate_file_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
};


/* Set a file access and modification times */
struct set_file_time_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
    IN  time_t       access_time;   /* last access time */
    IN  time_t       write_time;    /* last write time */
};


/* Flush a file buffers */
struct flush_file_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
};


/* Get information about a file */
struct get_file_info_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
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
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
    IN  unsigned int offset_low;    /* offset of start of lock */
    IN  unsigned int offset_high;   /* offset of start of lock */
    IN  unsigned int count_low;     /* count of bytes to lock */
    IN  unsigned int count_high;    /* count of bytes to lock */
};


/* Unlock a region of a file */
struct unlock_file_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the file */
    IN  unsigned int offset_low;    /* offset of start of unlock */
    IN  unsigned int offset_high;   /* offset of start of unlock */
    IN  unsigned int count_low;     /* count of bytes to unlock */
    IN  unsigned int count_high;    /* count of bytes to unlock */
};


/* Create an anonymous pipe */
struct create_pipe_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle_read;   /* handle to the read-side of the pipe */
    OUT handle_t     handle_write;  /* handle to the write-side of the pipe */
};


/* Create a socket */
struct create_socket_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    IN  int          family;        /* family, see socket manpage */
    IN  int          type;          /* type, see socket manpage */
    IN  int          protocol;      /* protocol, see socket manpage */
    OUT handle_t     handle;        /* handle to the new socket */
};


/* Accept a socket */
struct accept_socket_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     lhandle;       /* handle to the listening socket */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the new socket */
};


/* Set socket event parameters */
struct set_socket_event_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the socket */
    IN  unsigned int mask;          /* event mask */
    IN  handle_t     event;         /* event object */
};


/* Get socket event parameters */
struct get_socket_event_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the socket */
    IN  int          service;       /* clear pending? */
    IN  handle_t     s_event;       /* "expected" event object */
    IN  handle_t     c_event;       /* event to clear */
    OUT unsigned int mask;          /* event mask */
    OUT unsigned int pmask;         /* pending events */
    OUT unsigned int state;         /* status bits */
    OUT VARARG(errors,ints);        /* event errors */
};


/* Reenable pending socket events */
struct enable_socket_event_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the socket */
    IN  unsigned int mask;          /* events to re-enable */
    IN  unsigned int sstate;        /* status bits to set */
    IN  unsigned int cstate;        /* status bits to clear */
};


/* Allocate a console for the current process */
struct alloc_console_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle_in;     /* handle to console input */
    OUT handle_t     handle_out;    /* handle to console output */
};


/* Free the console of the current process */
struct free_console_request
{
    REQUEST_HEADER;                 /* request header */
};


/* Open a handle to the process console */
struct open_console_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          output;        /* input or output? */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the console */
};


/* Set a console file descriptor */
struct set_console_fd_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console */
    IN  handle_t     file_handle;   /* handle of file to use as file descriptor */
    IN  int          pid;           /* pid of xterm (hack) */
};


/* Get a console mode (input or output) */
struct get_console_mode_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console */
    OUT int          mode;          /* console mode */
};


/* Set a console mode (input or output) */
struct set_console_mode_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console */
    IN  int          mode;          /* console mode */
};


/* Set info about a console (output only) */
struct set_console_info_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console */
    IN  int          mask;          /* setting mask (see below) */
    IN  int          cursor_size;   /* size of cursor (percentage filled) */
    IN  int          cursor_visible;/* cursor visibility flag */
    IN  VARARG(title,string);       /* console title */
};
#define SET_CONSOLE_INFO_CURSOR 0x01
#define SET_CONSOLE_INFO_TITLE  0x02

/* Get info about a console (output only) */
struct get_console_info_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console */
    OUT int          cursor_size;   /* size of cursor (percentage filled) */
    OUT int          cursor_visible;/* cursor visibility flag */
    OUT int          pid;           /* pid of xterm (hack) */
    OUT VARARG(title,string);       /* console title */
};


/* Add input records to a console input queue */
struct write_console_input_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console input */
    OUT int          written;       /* number of records written */
    IN  VARARG(rec,input_records);  /* input records */
};

/* Fetch input records from a console input queue */
struct read_console_input_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the console input */
    IN  int          flush;         /* flush the retrieved records from the queue? */
    OUT int          read;          /* number of records read */
    OUT VARARG(rec,input_records);  /* input records */
};


/* Create a change notification */
struct create_change_notification_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          subtree;       /* watch all the subtree */
    IN  int          filter;        /* notification filter */
    OUT handle_t     handle;        /* handle to the change notification */
};


/* Create a file mapping */
struct create_mapping_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          size_high;     /* mapping size */
    IN  int          size_low;      /* mapping size */
    IN  int          protect;       /* protection flags (see below) */
    IN  int          inherit;       /* inherit flag */
    IN  handle_t     file_handle;   /* file handle */
    OUT handle_t     handle;        /* handle to the mapping */
    IN  VARARG(name,unicode_str);   /* object name */
};
/* protection flags */
#define VPROT_READ       0x01
#define VPROT_WRITE      0x02
#define VPROT_EXEC       0x04
#define VPROT_WRITECOPY  0x08
#define VPROT_GUARD      0x10
#define VPROT_NOCACHE    0x20
#define VPROT_COMMITTED  0x40
#define VPROT_IMAGE      0x80


/* Open a mapping */
struct open_mapping_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the mapping */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Get information about a file mapping */
struct get_mapping_info_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the mapping */
    OUT int          size_high;     /* mapping size */
    OUT int          size_low;      /* mapping size */
    OUT int          protect;       /* protection flags */
    OUT int          header_size;   /* header size (for VPROT_IMAGE mapping) */
    OUT void*        base;          /* default base addr (for VPROT_IMAGE mapping) */
    OUT handle_t     shared_file;   /* shared mapping file handle */
    OUT int          shared_size;   /* shared mapping size */
    OUT int          anonymous;     /* anonymous mapping? */
};


/* Create a device */
struct create_device_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    IN  int          id;            /* client private id */
    OUT handle_t     handle;        /* handle to the device */
};


/* Create a snapshot */
struct create_snapshot_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          inherit;       /* inherit flag */
    IN  int          flags;         /* snapshot flags (TH32CS_*) */
    IN  void*        pid;           /* process id */
    OUT handle_t     handle;        /* handle to the snapshot */
};


/* Get the next process from a snapshot */
struct next_process_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the snapshot */
    IN  int          reset;         /* reset snapshot position? */
    OUT int          count;         /* process usage count */
    OUT void*        pid;           /* process id */
    OUT int          threads;       /* number of threads */
    OUT int          priority;      /* process priority */
};


/* Get the next thread from a snapshot */
struct next_thread_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the snapshot */
    IN  int          reset;         /* reset snapshot position? */
    OUT int          count;         /* thread usage count */
    OUT void*        pid;           /* process id */
    OUT void*        tid;           /* thread id */
    OUT int          base_pri;      /* base priority */
    OUT int          delta_pri;     /* delta priority */
};


/* Get the next module from a snapshot */
struct next_module_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the snapshot */
    IN  int          reset;         /* reset snapshot position? */
    OUT void*        pid;           /* process id */
    OUT void*        base;          /* module base address */
};


/* Wait for a debug event */
struct wait_debug_event_request
{
    REQUEST_HEADER;                /* request header */
    IN  int           get_handle;  /* should we alloc a handle for waiting? */
    OUT void*         pid;         /* process id */
    OUT void*         tid;         /* thread id */
    OUT handle_t      wait;        /* wait handle if no event ready */
    OUT VARARG(event,debug_event); /* debug event data */
};


/* Send an exception event */
struct exception_event_request
{
    REQUEST_HEADER;                /* request header */
    IN  int              first;    /* first chance exception? */
    OUT int              status;   /* event continuation status */
    IN  VARARG(record,exc_event);  /* thread context followed by exception record */
    OUT VARARG(context,context);   /* modified thread context */
};


/* Send an output string to the debugger */
struct output_debug_string_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*         string;      /* string to display (in debugged process address space) */
    IN  int           unicode;     /* is it Unicode? */
    IN  int           length;      /* string length */
};


/* Continue a debug event */
struct continue_debug_event_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*        pid;          /* process id to continue */
    IN  void*        tid;          /* thread id to continue */
    IN  int          status;       /* continuation status */
};


/* Start debugging an existing process */
struct debug_process_request
{
    REQUEST_HEADER;                /* request header */
    IN  void*        pid;          /* id of the process to debug */
};


/* Read data from a process address space */
struct read_process_memory_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* process handle */
    IN  void*        addr;         /* addr to read from (must be int-aligned) */
    IN  int          len;          /* number of ints to read */
    OUT VARARG(data,bytes);        /* result data */
};


/* Write data to a process address space */
struct write_process_memory_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* process handle */
    IN  void*        addr;         /* addr to write to (must be int-aligned) */
    IN  int          len;          /* number of ints to write */
    IN  unsigned int first_mask;   /* mask for first word */
    IN  unsigned int last_mask;    /* mask for last word */
    IN  VARARG(data,bytes);        /* result data */
};


/* Create a registry key */
struct create_key_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     parent;       /* handle to the parent key */
    IN  unsigned int access;       /* desired access rights */
    IN  unsigned int options;      /* creation options */
    IN  time_t       modif;        /* last modification time */
    OUT handle_t     hkey;         /* handle to the created key */
    OUT int          created;      /* has it been newly created? */
    IN  VARARG(name,unicode_len_str);  /* key name */
    IN  VARARG(class,unicode_str);     /* class name */
};

/* Open a registry key */
struct open_key_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     parent;       /* handle to the parent key */
    IN  unsigned int access;       /* desired access rights */
    OUT handle_t     hkey;         /* handle to the open key */
    IN  VARARG(name,unicode_str);  /* key name */
};


/* Delete a registry key */
struct delete_key_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* handle to the key */
};


/* Enumerate registry subkeys */
struct enum_key_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* handle to registry key */
    IN  int          index;        /* index of subkey (or -1 for current key) */
    IN  int          full;         /* return the full info? */
    OUT int          subkeys;      /* number of subkeys */
    OUT int          max_subkey;   /* longest subkey name */
    OUT int          max_class;    /* longest class name */
    OUT int          values;       /* number of values */
    OUT int          max_value;    /* longest value name */
    OUT int          max_data;     /* longest value data */
    OUT time_t       modif;        /* last modification time */
    OUT VARARG(name,unicode_len_str);  /* key name */
    OUT VARARG(class,unicode_str);     /* class name */
};


/* Set a value of a registry key */
struct set_key_value_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* handle to registry key */
    IN  int          type;         /* value type */
    IN  unsigned int total;        /* total value len */
    IN  unsigned int offset;       /* offset for setting data */
    IN  VARARG(name,unicode_len_str);  /* value name */
    IN  VARARG(data,bytes);        /* value data */
};


/* Retrieve the value of a registry key */
struct get_key_value_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* handle to registry key */
    IN  unsigned int offset;       /* offset for getting data */
    OUT int          type;         /* value type */
    OUT int          len;          /* value data len */
    IN  VARARG(name,unicode_len_str);  /* value name */
    OUT VARARG(data,bytes);        /* value data */
};


/* Enumerate a value of a registry key */
struct enum_key_value_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* handle to registry key */
    IN  int          index;        /* value index */
    IN  unsigned int offset;       /* offset for getting data */
    OUT int          type;         /* value type */
    OUT int          len;          /* value data len */
    OUT VARARG(name,unicode_len_str);  /* value name */
    OUT VARARG(data,bytes);        /* value data */
};


/* Delete a value of a registry key */
struct delete_key_value_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* handle to registry key */
    IN  VARARG(name,unicode_str);  /* value name */
};


/* Load a registry branch from a file */
struct load_registry_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* root key to load to */
    IN  handle_t     file;         /* file to load from */
    IN  VARARG(name,unicode_str);  /* subkey name */
};


/* Save a registry branch to a file */
struct save_registry_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* key to save */
    IN  handle_t     file;         /* file to save to */
};


/* Save a registry branch at server exit */
struct save_registry_atexit_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     hkey;         /* key to save */
    IN  VARARG(file,string);       /* file to save to */
};


/* Set the current and saving level for the registry */
struct set_registry_levels_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          current;      /* new current level */
    IN  int          saving;       /* new saving level */
    IN  int          period;       /* duration between periodic saves (milliseconds) */
};


/* Create a waitable timer */
struct create_timer_request
{
    REQUEST_HEADER;                 /* request header */
    IN  int          inherit;       /* inherit flag */
    IN  int          manual;        /* manual reset */
    OUT handle_t     handle;        /* handle to the timer */
    IN  VARARG(name,unicode_str);   /* object name */
};


/* Open a waitable timer */
struct open_timer_request
{
    REQUEST_HEADER;                 /* request header */
    IN  unsigned int access;        /* wanted access rights */
    IN  int          inherit;       /* inherit flag */
    OUT handle_t     handle;        /* handle to the timer */
    IN  VARARG(name,unicode_str);   /* object name */
};

/* Set a waitable timer */
struct set_timer_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the timer */
    IN  int          sec;           /* next expiration absolute time */
    IN  int          usec;          /* next expiration absolute time */
    IN  int          period;        /* timer period in ms */
    IN  void*        callback;      /* callback function */
    IN  void*        arg;           /* callback argument */
};

/* Cancel a waitable timer */
struct cancel_timer_request
{
    REQUEST_HEADER;                 /* request header */
    IN  handle_t     handle;        /* handle to the timer */
};


/* Retrieve the current context of a thread */
struct get_thread_context_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    IN  unsigned int flags;        /* context flags */
    OUT VARARG(context,context);   /* thread context */
};


/* Set the current context of a thread */
struct set_thread_context_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* thread handle */
    IN  unsigned int flags;        /* context flags */
    IN  VARARG(context,context);   /* thread context */
};


/* Fetch a selector entry for a thread */
struct get_selector_entry_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t      handle;      /* thread handle */
    IN  int           entry;       /* LDT entry */
    OUT unsigned int  base;        /* selector base */
    OUT unsigned int  limit;       /* selector limit */
    OUT unsigned char flags;       /* selector flags */
};


/* Add an atom */
struct add_atom_request
{
    REQUEST_HEADER;                /* request header */
    IN  int           local;       /* is atom in local process table? */
    OUT int           atom;        /* resulting atom */
    IN  VARARG(name,unicode_str);  /* atom name */
};


/* Delete an atom */
struct delete_atom_request
{
    REQUEST_HEADER;                /* request header */
    IN  int           atom;        /* atom handle */
    IN  int           local;       /* is atom in local process table? */
};


/* Find an atom */
struct find_atom_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          local;        /* is atom in local process table? */
    OUT int          atom;         /* atom handle */
    IN  VARARG(name,unicode_str);  /* atom name */
};


/* Get an atom name */
struct get_atom_name_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          atom;         /* atom handle */
    IN  int          local;        /* is atom in local process table? */
    OUT int          count;        /* atom lock count */
    OUT VARARG(name,unicode_str);  /* atom name */
};


/* Init the process atom table */
struct init_atom_table_request
{
    REQUEST_HEADER;                /* request header */
    IN  int          entries;      /* number of entries */
};


/* Get the message queue of the current thread */
struct get_msg_queue_request
{
    REQUEST_HEADER;                /* request header */
    OUT handle_t     handle;       /* handle to the queue */
};

/* Wake up a message queue */
struct wake_queue_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* handle to the queue */
    IN  unsigned int bits;         /* wake bits */
};

/* Wait for a process to start waiting on input */
struct wait_input_idle_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* process handle */
    IN  int          timeout;      /* timeout */
    OUT handle_t     event;        /* handle to idle event */
};

struct create_serial_request
{
    REQUEST_HEADER;                /* request header */
    IN  unsigned int access;       /* wanted access rights */
    IN  int          inherit;      /* inherit flag */
    IN  unsigned int sharing;      /* sharing flags */
    OUT handle_t     handle;       /* handle to the port */
    IN  VARARG(name,string);       /* file name */
};

struct get_serial_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* handle to comm port */
    OUT unsigned int readinterval;
    OUT unsigned int readconst;
    OUT unsigned int readmult;
    OUT unsigned int writeconst;
    OUT unsigned int writemult;
    OUT unsigned int eventmask;
    OUT unsigned int commerror;
};

struct set_serial_info_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     handle;       /* handle to comm port */
    IN  int          flags;        /* bitmask to set values (see below) */
    IN  unsigned int readinterval;
    IN  unsigned int readconst;
    IN  unsigned int readmult;
    IN  unsigned int writeconst;
    IN  unsigned int writemult;
    IN  unsigned int eventmask;
    IN  unsigned int commerror;
};
#define SERIALINFO_SET_TIMEOUTS  0x01
#define SERIALINFO_SET_MASK      0x02
#define SERIALINFO_SET_ERROR     0x04

struct create_async_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     file_handle;  /* handle to comm port */
    IN  void*        overlapped;
    IN  void*        buffer;
    IN  int          count;
    IN  void*        func;
    IN  int          type;
    OUT handle_t     ov_handle;
};
#define ASYNC_TYPE_READ  0x01
#define ASYNC_TYPE_WRITE 0x02
#define ASYNC_TYPE_WAIT  0x03

/*
 * Used by service thread to tell the server that the current
 * operation has completed
 */
struct async_result_request
{
    REQUEST_HEADER;                /* request header */
    IN  handle_t     ov_handle;
    IN  int          result;       /* NT status code */
};

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

enum request
{
    REQ_NEW_PROCESS,
    REQ_GET_NEW_PROCESS_INFO,
    REQ_NEW_THREAD,
    REQ_BOOT_DONE,
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
    REQ_LOAD_DLL,
    REQ_UNLOAD_DLL,
    REQ_QUEUE_APC,
    REQ_GET_APC,
    REQ_CLOSE_HANDLE,
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
    REQ_GET_HANDLE_FD,
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
    REQ_NEXT_THREAD,
    REQ_NEXT_MODULE,
    REQ_WAIT_DEBUG_EVENT,
    REQ_EXCEPTION_EVENT,
    REQ_OUTPUT_DEBUG_STRING,
    REQ_CONTINUE_DEBUG_EVENT,
    REQ_DEBUG_PROCESS,
    REQ_READ_PROCESS_MEMORY,
    REQ_WRITE_PROCESS_MEMORY,
    REQ_CREATE_KEY,
    REQ_OPEN_KEY,
    REQ_DELETE_KEY,
    REQ_ENUM_KEY,
    REQ_SET_KEY_VALUE,
    REQ_GET_KEY_VALUE,
    REQ_ENUM_KEY_VALUE,
    REQ_DELETE_KEY_VALUE,
    REQ_LOAD_REGISTRY,
    REQ_SAVE_REGISTRY,
    REQ_SAVE_REGISTRY_ATEXIT,
    REQ_SET_REGISTRY_LEVELS,
    REQ_CREATE_TIMER,
    REQ_OPEN_TIMER,
    REQ_SET_TIMER,
    REQ_CANCEL_TIMER,
    REQ_GET_THREAD_CONTEXT,
    REQ_SET_THREAD_CONTEXT,
    REQ_GET_SELECTOR_ENTRY,
    REQ_ADD_ATOM,
    REQ_DELETE_ATOM,
    REQ_FIND_ATOM,
    REQ_GET_ATOM_NAME,
    REQ_INIT_ATOM_TABLE,
    REQ_GET_MSG_QUEUE,
    REQ_WAKE_QUEUE,
    REQ_WAIT_INPUT_IDLE,
    REQ_CREATE_SERIAL,
    REQ_GET_SERIAL_INFO,
    REQ_SET_SERIAL_INFO,
    REQ_CREATE_ASYNC,
    REQ_ASYNC_RESULT,
    REQ_NB_REQUESTS
};

union generic_request
{
    struct request_max_size max_size;
    struct request_header header;
    struct new_process_request new_process;
    struct get_new_process_info_request get_new_process_info;
    struct new_thread_request new_thread;
    struct boot_done_request boot_done;
    struct init_process_request init_process;
    struct init_process_done_request init_process_done;
    struct init_thread_request init_thread;
    struct get_thread_buffer_request get_thread_buffer;
    struct terminate_process_request terminate_process;
    struct terminate_thread_request terminate_thread;
    struct get_process_info_request get_process_info;
    struct set_process_info_request set_process_info;
    struct get_thread_info_request get_thread_info;
    struct set_thread_info_request set_thread_info;
    struct suspend_thread_request suspend_thread;
    struct resume_thread_request resume_thread;
    struct load_dll_request load_dll;
    struct unload_dll_request unload_dll;
    struct queue_apc_request queue_apc;
    struct get_apc_request get_apc;
    struct close_handle_request close_handle;
    struct set_handle_info_request set_handle_info;
    struct dup_handle_request dup_handle;
    struct open_process_request open_process;
    struct select_request select;
    struct create_event_request create_event;
    struct event_op_request event_op;
    struct open_event_request open_event;
    struct create_mutex_request create_mutex;
    struct release_mutex_request release_mutex;
    struct open_mutex_request open_mutex;
    struct create_semaphore_request create_semaphore;
    struct release_semaphore_request release_semaphore;
    struct open_semaphore_request open_semaphore;
    struct create_file_request create_file;
    struct alloc_file_handle_request alloc_file_handle;
    struct get_handle_fd_request get_handle_fd;
    struct set_file_pointer_request set_file_pointer;
    struct truncate_file_request truncate_file;
    struct set_file_time_request set_file_time;
    struct flush_file_request flush_file;
    struct get_file_info_request get_file_info;
    struct lock_file_request lock_file;
    struct unlock_file_request unlock_file;
    struct create_pipe_request create_pipe;
    struct create_socket_request create_socket;
    struct accept_socket_request accept_socket;
    struct set_socket_event_request set_socket_event;
    struct get_socket_event_request get_socket_event;
    struct enable_socket_event_request enable_socket_event;
    struct alloc_console_request alloc_console;
    struct free_console_request free_console;
    struct open_console_request open_console;
    struct set_console_fd_request set_console_fd;
    struct get_console_mode_request get_console_mode;
    struct set_console_mode_request set_console_mode;
    struct set_console_info_request set_console_info;
    struct get_console_info_request get_console_info;
    struct write_console_input_request write_console_input;
    struct read_console_input_request read_console_input;
    struct create_change_notification_request create_change_notification;
    struct create_mapping_request create_mapping;
    struct open_mapping_request open_mapping;
    struct get_mapping_info_request get_mapping_info;
    struct create_device_request create_device;
    struct create_snapshot_request create_snapshot;
    struct next_process_request next_process;
    struct next_thread_request next_thread;
    struct next_module_request next_module;
    struct wait_debug_event_request wait_debug_event;
    struct exception_event_request exception_event;
    struct output_debug_string_request output_debug_string;
    struct continue_debug_event_request continue_debug_event;
    struct debug_process_request debug_process;
    struct read_process_memory_request read_process_memory;
    struct write_process_memory_request write_process_memory;
    struct create_key_request create_key;
    struct open_key_request open_key;
    struct delete_key_request delete_key;
    struct enum_key_request enum_key;
    struct set_key_value_request set_key_value;
    struct get_key_value_request get_key_value;
    struct enum_key_value_request enum_key_value;
    struct delete_key_value_request delete_key_value;
    struct load_registry_request load_registry;
    struct save_registry_request save_registry;
    struct save_registry_atexit_request save_registry_atexit;
    struct set_registry_levels_request set_registry_levels;
    struct create_timer_request create_timer;
    struct open_timer_request open_timer;
    struct set_timer_request set_timer;
    struct cancel_timer_request cancel_timer;
    struct get_thread_context_request get_thread_context;
    struct set_thread_context_request set_thread_context;
    struct get_selector_entry_request get_selector_entry;
    struct add_atom_request add_atom;
    struct delete_atom_request delete_atom;
    struct find_atom_request find_atom;
    struct get_atom_name_request get_atom_name;
    struct init_atom_table_request init_atom_table;
    struct get_msg_queue_request get_msg_queue;
    struct wake_queue_request wake_queue;
    struct wait_input_idle_request wait_input_idle;
    struct create_serial_request create_serial;
    struct get_serial_info_request get_serial_info;
    struct set_serial_info_request set_serial_info;
    struct create_async_request create_async;
    struct async_result_request async_result;
};

#define SERVER_PROTOCOL_VERSION 35

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

#undef REQUEST_HEADER
#undef VARARG

/* format of the server buffer */
struct server_buffer_info
{
    volatile unsigned int cur_req;    /* offset of the current request */
    volatile unsigned int cur_pos;    /* first available position in buffer */
};

/* client-side functions */

#ifndef __WINE_SERVER__

#include "thread.h"
#include "ntddk.h"
#include "wine/exception.h"

/* client communication functions */

extern unsigned int wine_server_call( enum request req );
extern unsigned int server_call_fd( enum request req, int fd_out );
extern void server_protocol_error( const char *err, ... ) WINE_NORETURN;
extern void *wine_server_alloc_req( size_t fixed_size, size_t var_size );
extern int wine_server_recv_fd( int handle, int cache );
extern const char *get_config_dir(void);

/* compatibility macros */
#define server_alloc_req(f,v) wine_server_alloc_req(f,v)
#define server_call_noerr(req) wine_server_call(req)

/* do a server call and set the last error code */
inline static unsigned int server_call( enum request req )
{
    unsigned int res = wine_server_call( req );
    if (res) SetLastError( RtlNtStatusToDosError(res) );
    return res;
}

/* get a pointer to the variable part of the request */
inline static void *server_data_ptr( const void *req )
{
    return (union generic_request *)req + 1;
}

/* get the size of the variable part of the request */
inline static size_t server_data_size( const void *req )
{
    return ((struct request_header *)req)->var_size;
}


/* exception support for server calls */

extern DWORD __wine_server_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                              CONTEXT *context, EXCEPTION_FRAME **pdispatcher );

struct __server_exception_frame
{
    EXCEPTION_FRAME           frame;
    struct server_buffer_info info;  /* saved buffer information */
};

#define SERVER_START_REQ \
    do { \
        struct __server_exception_frame __f; \
        __f.frame.Handler = __wine_server_exception_handler; \
        __f.info = *NtCurrentTeb()->buffer_info; \
        __wine_push_frame( &__f.frame ); \
        do {

#define SERVER_END_REQ \
        } while(0); \
        *NtCurrentTeb()->buffer_info = __f.info; \
        __wine_pop_frame( &__f.frame ); \
    } while(0);

extern int CLIENT_InitServer(void);
extern int CLIENT_BootDone( int debug_level );
extern int CLIENT_IsBootThread(void);
extern int CLIENT_InitThread(void);
#endif  /* __WINE_SERVER__ */

#endif  /* __WINE_SERVER_H */

/*
 * Wine server processes
 *
 * Copyright (C) 1999 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SERVER_PROCESS_H
#define __WINE_SERVER_PROCESS_H

#include "object.h"

struct atom_table;
struct handle_table;
struct startup_info;
struct job;

/* process startup state */
enum startup_state { STARTUP_IN_PROGRESS, STARTUP_DONE, STARTUP_ABORTED };

/* process structures */

struct process
{
    struct object        obj;             /* object header */
    struct event_sync   *sync;            /* sync object for wait/signal */
    struct list          entry;           /* entry in system-wide process list */
    process_id_t         parent_id;       /* parent process id (at the time of creation) */
    struct list          thread_list;     /* thread list */
    struct debug_obj    *debug_obj;       /* debug object debugging this process */
    struct debug_event  *debug_event;     /* debug event being sent to debugger */
    struct handle_table *handles;         /* handle entries */
    struct fd           *msg_fd;          /* fd for sendmsg/recvmsg */
    process_id_t         id;              /* id of the process */
    process_id_t         group_id;        /* group id of the process */
    unsigned int         session_id;      /* session id */
    struct timeout_user *sigkill_timeout; /* timeout for final SIGKILL */
    timeout_t            sigkill_delay;   /* delay before final SIGKILL */
    unsigned short       machine;         /* client machine type */
    int                  unix_pid;        /* Unix pid for final SIGKILL */
    int                  exit_code;       /* process exit code */
    int                  running_threads; /* number of threads running in this process */
    timeout_t            start_time;      /* absolute time at process start */
    timeout_t            end_time;        /* absolute time at process end */
    affinity_t           affinity;        /* process affinity mask */
    int                  priority;        /* priority class */
    int                  base_priority;   /* base priority to calculate thread priority */
    int                  suspend;         /* global process suspend count */
    unsigned int         is_system:1;     /* is it a system process? */
    unsigned int         debug_children:1;/* also debug all child processes */
    unsigned int         is_terminating:1;/* is process terminating? */
    data_size_t          imagelen;        /* length of image path in bytes */
    WCHAR               *image;           /* main exe image full path */
    struct job          *job;             /* job object associated with this process */
    struct list          job_entry;       /* list entry for job object */
    struct list          asyncs;          /* list of async object owned by the process */
    struct list          locks;           /* list of file locks owned by the process */
    struct list          classes;         /* window classes owned by the process */
    struct console      *console;         /* console input */
    enum startup_state   startup_state;   /* startup state */
    struct startup_info *startup_info;    /* startup info while init is in progress */
    struct event        *idle_event;      /* event for input idle */
    obj_handle_t         winstation;      /* main handle to process window station */
    obj_handle_t         desktop;         /* handle to desktop to use for new threads */
    struct token        *token;           /* security token associated with this process */
    struct list          views;           /* list of memory views */
    client_ptr_t         peb;             /* PEB address in client address space */
    client_ptr_t         ldt_copy;        /* pointer to LDT copy in client addr space */
    struct dir_cache    *dir_cache;       /* map of client-side directory cache */
    unsigned int         trace_data;      /* opaque data used by the process tracing mechanism */
    struct rawinput_device *rawinput_devices;     /* list of registered rawinput devices */
    unsigned int         rawinput_device_count;   /* number of registered rawinput devices */
    const struct rawinput_device *rawinput_mouse; /* rawinput mouse device, if any */
    const struct rawinput_device *rawinput_kbd;   /* rawinput keyboard device, if any */
    struct list          rawinput_entry;  /* entry in the rawinput process list */
    struct list          kernel_object;   /* list of kernel object pointers */
    struct pe_image_info image_info;      /* main exe image info */
};

/* process functions */

extern unsigned int alloc_ptid( void *ptr );
extern void free_ptid( unsigned int id );
extern void *get_ptid_entry( unsigned int id );
extern struct process *create_process( int fd, struct process *parent, unsigned int flags,
                                       const struct startup_info_data *info,
                                       const struct security_descriptor *sd, const obj_handle_t *handles,
                                       unsigned int handle_count, struct token *token );
extern data_size_t get_process_startup_info_size( struct process *process );
extern struct thread *get_process_first_thread( struct process *process );
extern struct process *get_process_from_id( process_id_t id );
extern struct process *get_process_from_handle( obj_handle_t handle, unsigned int access );
extern struct debug_obj *get_debug_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern int process_set_debugger( struct process *process, struct thread *thread );
extern void debugger_detach( struct process *process, struct debug_obj *debug_obj );
extern int set_process_debug_flag( struct process *process, int flag );

extern void add_process_thread( struct process *process,
                                struct thread *thread );
extern void remove_process_thread( struct process *process,
                                   struct thread *thread );
extern void suspend_process( struct process *process );
extern void resume_process( struct process *process );
extern void kill_process( struct process *process, int violent_death );
extern void kill_console_processes( struct thread *renderer, int exit_code );
extern void detach_debugged_processes( struct debug_obj *debug_obj, int exit_code );
extern void enum_processes( int (*cb)(struct process*, void*), void *user);
extern void set_process_base_priority( struct process *process, int base_priority );

/* console functions */
extern struct thread *console_get_renderer( struct console *console );

/* process tracing mechanism to use */
#ifdef __APPLE__
#define USE_MACH
#elif defined(__sun)
#define USE_PROCFS
#else
#define USE_PTRACE
#endif

extern void init_tracing_mechanism(void);
extern void init_process_tracing( struct process *process );
extern void finish_process_tracing( struct process *process );
extern int read_process_memory( struct process *process, client_ptr_t ptr, data_size_t size, char *dest );
extern int write_process_memory( struct process *process, client_ptr_t ptr, data_size_t size, const char *src );

static inline process_id_t get_process_id( struct process *process ) { return process->id; }

static inline int is_process_init_done( struct process *process )
{
    return process->startup_state == STARTUP_DONE;
}

static inline int is_wow64_process( struct process *process )
{
    return is_machine_64bit( native_machine ) && !is_machine_64bit( process->machine );
}

static const unsigned int default_session_id = 1;

#endif  /* __WINE_SERVER_PROCESS_H */

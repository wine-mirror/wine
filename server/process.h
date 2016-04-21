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

struct process_dll
{
    struct list          entry;           /* entry in per-process dll list */
    struct mapping      *mapping;         /* dll file */
    mod_handle_t         base;            /* dll base address (in process addr space) */
    client_ptr_t         name;            /* ptr to ptr to name (in process addr space) */
    data_size_t          size;            /* dll size */
    int                  dbg_offset;      /* debug info offset */
    int                  dbg_size;        /* debug info size */
    data_size_t          namelen;         /* length of dll file name */
    WCHAR               *filename;        /* dll file name */
};

struct rawinput_device_entry
{
    struct list            entry;
    struct rawinput_device device;
};

struct process
{
    struct object        obj;             /* object header */
    struct list          entry;           /* entry in system-wide process list */
    process_id_t         parent_id;       /* parent process id (at the time of creation) */
    struct list          thread_list;     /* thread list */
    struct thread       *debugger;        /* thread debugging this process */
    struct handle_table *handles;         /* handle entries */
    struct fd           *msg_fd;          /* fd for sendmsg/recvmsg */
    process_id_t         id;              /* id of the process */
    process_id_t         group_id;        /* group id of the process */
    struct timeout_user *sigkill_timeout; /* timeout for final SIGKILL */
    enum cpu_type        cpu;             /* client CPU type */
    int                  unix_pid;        /* Unix pid for final SIGKILL */
    int                  exit_code;       /* process exit code */
    int                  running_threads; /* number of threads running in this process */
    timeout_t            start_time;      /* absolute time at process start */
    timeout_t            end_time;        /* absolute time at process end */
    affinity_t           affinity;        /* process affinity mask */
    int                  priority;        /* priority class */
    int                  suspend;         /* global process suspend count */
    unsigned int         is_system:1;     /* is it a system process? */
    unsigned int         debug_children:1;/* also debug all child processes */
    unsigned int         is_terminating:1;/* is process terminating? */
    struct job          *job;             /* job object ascoicated with this process */
    struct list          job_entry;       /* list entry for job object */
    struct list          locks;           /* list of file locks owned by the process */
    struct list          classes;         /* window classes owned by the process */
    struct console_input*console;         /* console input */
    enum startup_state   startup_state;   /* startup state */
    struct startup_info *startup_info;    /* startup info while init is in progress */
    struct event        *idle_event;      /* event for input idle */
    obj_handle_t         winstation;      /* main handle to process window station */
    obj_handle_t         desktop;         /* handle to desktop to use for new threads */
    struct token        *token;           /* security token associated with this process */
    struct list          dlls;            /* list of loaded dlls */
    client_ptr_t         peb;             /* PEB address in client address space */
    client_ptr_t         ldt_copy;        /* pointer to LDT copy in client addr space */
    unsigned int         trace_data;      /* opaque data used by the process tracing mechanism */
    struct list          rawinput_devices;/* list of registered rawinput devices */
    const struct rawinput_device *rawinput_mouse; /* rawinput mouse device, if any */
    const struct rawinput_device *rawinput_kbd;   /* rawinput keyboard device, if any */
};

struct process_snapshot
{
    struct process *process;  /* process ptr */
    int             count;    /* process refcount */
    int             threads;  /* number of threads */
    int             priority; /* priority class */
    int             handles;  /* number of handles */
    int             unix_pid; /* Unix pid */
};

#define CPU_FLAG(cpu) (1 << (cpu))
#define CPU_64BIT_MASK (CPU_FLAG(CPU_x86_64) | CPU_FLAG(CPU_ARM64))

/* process functions */

extern unsigned int alloc_ptid( void *ptr );
extern void free_ptid( unsigned int id );
extern void *get_ptid_entry( unsigned int id );
extern struct thread *create_process( int fd, struct thread *parent_thread, int inherit_all );
extern data_size_t init_process( struct thread *thread );
extern struct thread *get_process_first_thread( struct process *process );
extern struct process *get_process_from_id( process_id_t id );
extern struct process *get_process_from_handle( obj_handle_t handle, unsigned int access );
extern int process_set_debugger( struct process *process, struct thread *thread );
extern int debugger_detach( struct process* process, struct thread* debugger );
extern int set_process_debug_flag( struct process *process, int flag );

extern void add_process_thread( struct process *process,
                                struct thread *thread );
extern void remove_process_thread( struct process *process,
                                   struct thread *thread );
extern void suspend_process( struct process *process );
extern void resume_process( struct process *process );
extern void kill_process( struct process *process, int violent_death );
extern void kill_console_processes( struct thread *renderer, int exit_code );
extern void kill_debugged_processes( struct thread *debugger, int exit_code );
extern void break_process( struct process *process );
extern void detach_debugged_processes( struct thread *debugger );
extern struct process_snapshot *process_snap( int *count );
extern void enum_processes( int (*cb)(struct process*, void*), void *user);

/* console functions */
extern void inherit_console(struct thread *parent_thread, struct process *process, obj_handle_t hconin);
extern int free_console( struct process *process );
extern struct thread *console_get_renderer( struct console_input *console );

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

static inline struct process_dll *get_process_exe_module( struct process *process )
{
    struct list *ptr = list_head( &process->dlls );
    return ptr ? LIST_ENTRY( ptr, struct process_dll, entry ) : NULL;
}

#endif  /* __WINE_SERVER_PROCESS_H */

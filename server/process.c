/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "winerror.h"
#include "winbase.h"
#include "winnt.h"

#include "server.h"
#include "server/thread.h"

/* reserved handle access rights */
#define RESERVED_SHIFT         25
#define RESERVED_INHERIT       (HANDLE_FLAG_INHERIT << RESERVED_SHIFT)
#define RESERVED_CLOSE_PROTECT (HANDLE_FLAG_PROTECT_FROM_CLOSE << RESERVED_SHIFT)
#define RESERVED_ALL           (RESERVED_INHERIT | RESERVED_CLOSE_PROTECT)

/* global handle macros */
#define HANDLE_OBFUSCATOR         0x544a4def
#define HANDLE_IS_GLOBAL(h)       (((h) ^ HANDLE_OBFUSCATOR) < 0x10000)
#define HANDLE_LOCAL_TO_GLOBAL(h) ((h) ^ HANDLE_OBFUSCATOR)
#define HANDLE_GLOBAL_TO_LOCAL(h) ((h) ^ HANDLE_OBFUSCATOR)

struct handle_entry
{
    struct object *ptr;
    unsigned int   access;
};

/* process structure; not much for now... */

struct process
{
    struct object        obj;             /* object header */
    struct process      *next;            /* system-wide process list */
    struct process      *prev;
    struct thread       *thread_list;     /* head of the thread list */
    struct handle_entry *entries;         /* handle entry table */
    int                  handle_count;    /* nb of allocated handle entries */
    int                  handle_last;     /* last used handle entry */
    int                  exit_code;       /* process exit code */
    int                  running_threads; /* number of threads running in this process */
    struct timeval       start_time;      /* absolute time at process start */
    struct timeval       end_time;        /* absolute time at process end */
    int                  priority;        /* priority class */
    int                  affinity;        /* process affinity mask */
    struct object       *console_in;      /* console input */
    struct object       *console_out;     /* console output */
};

static struct process *first_process;
static struct process *initial_process;

#define MIN_HANDLE_ENTRIES  32

/* process operations */

static void process_dump( struct object *obj, int verbose );
static int process_signaled( struct object *obj, struct thread *thread );
static void process_destroy( struct object *obj );
static void free_handles( struct process *process );
static int copy_handle_table( struct process *process, struct process *parent );

static const struct object_ops process_ops =
{
    process_dump,
    add_queue,
    remove_queue,
    process_signaled,
    no_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    process_destroy
};

/* create a new process */
struct process *create_process(void)
{
    struct process *process, *parent;

    if (!(process = mem_alloc( sizeof(*process) ))) return NULL;

    parent = current ? current->process : NULL;
    if (!copy_handle_table( process, parent ))
    {
        free( process );
        return NULL;
    }
    init_object( &process->obj, &process_ops, NULL );
    process->next            = first_process;
    process->prev            = NULL;
    process->thread_list     = NULL;
    process->exit_code       = 0x103;  /* STILL_ACTIVE */
    process->running_threads = 0;
    process->priority        = NORMAL_PRIORITY_CLASS;
    process->affinity        = 1;
    process->console_in      = NULL;
    process->console_out     = NULL;
    if (parent)
    {
        if (parent->console_in) process->console_in = grab_object( parent->console_in );
        if (parent->console_out) process->console_out = grab_object( parent->console_out );
    }

    if (first_process) first_process->prev = process;
    first_process = process;
    if (!initial_process)
    {
        initial_process = process;
        grab_object( initial_process ); /* so that we never free it */
    }

    gettimeofday( &process->start_time, NULL );
    /* alloc a handle for the process itself */
    alloc_handle( process, process, PROCESS_ALL_ACCESS, 0 );
    return process;
}

/* destroy a process when its refcount is 0 */
static void process_destroy( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );
    assert( process != initial_process );

    /* we can't have a thread remaining */
    assert( !process->thread_list );
    if (process->next) process->next->prev = process->prev;
    if (process->prev) process->prev->next = process->next;
    else first_process = process->next;
    free_console( process );
    free_handles( process );
    if (debug_level) memset( process, 0xbb, sizeof(process) );  /* catch errors */
    free( process );
}

/* dump a process on stdout for debugging purposes */
static void process_dump( struct object *obj, int verbose )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    printf( "Process next=%p prev=%p\n", process->next, process->prev );
}

static int process_signaled( struct object *obj, struct thread *thread )
{
    struct process *process = (struct process *)obj;
    return (process->running_threads > 0);
}

/* get a process from an id (and increment the refcount) */
struct process *get_process_from_id( void *id )
{
    struct process *p = first_process;
    while (p && (p != id)) p = p->next;
    if (p) grab_object( p );
    else SET_ERROR( ERROR_INVALID_PARAMETER );
    return p;
}

/* get a process from a handle (and increment the refcount) */
struct process *get_process_from_handle( int handle, unsigned int access )
{
    return (struct process *)get_handle_obj( current->process, handle,
                                             access, &process_ops );
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process, int exit_code )
{
    assert( !process->thread_list );
    process->exit_code = exit_code;
    gettimeofday( &process->end_time, NULL );
    wake_up( &process->obj, 0 );
    free_handles( process );
}

/* free the process handle entries */
static void free_handles( struct process *process )
{
    struct handle_entry *entry;
    int handle;

    if (!(entry = process->entries)) return;
    for (handle = 0; handle <= process->handle_last; handle++, entry++)
    {
        struct object *obj = entry->ptr;
        entry->ptr = NULL;
        if (obj) release_object( obj );
    }
    free( process->entries );
    process->handle_count = 0;
    process->handle_last  = -1;
    process->entries = NULL;
}

/* add a thread to a process running threads list */
void add_process_thread( struct process *process, struct thread *thread )
{
    thread->proc_next = process->thread_list;
    thread->proc_prev = NULL;
    if (thread->proc_next) thread->proc_next->proc_prev = thread;
    process->thread_list = thread;
    process->running_threads++;
    grab_object( thread );
}

/* remove a thread from a process running threads list */
void remove_process_thread( struct process *process, struct thread *thread )
{
    assert( process->running_threads > 0 );
    assert( process->thread_list );

    if (thread->proc_next) thread->proc_next->proc_prev = thread->proc_prev;
    if (thread->proc_prev) thread->proc_prev->proc_next = thread->proc_next;
    else process->thread_list = thread->proc_next;

    if (!--process->running_threads)
    {
        /* we have removed the last running thread, exit the process */
        process_killed( process, thread->exit_code );
    }
    release_object( thread );
}

/* grow a handle table */
/* return 1 if OK, 0 on error */
static int grow_handle_table( struct process *process )
{
    struct handle_entry *new_entries;
    int count = process->handle_count;

    if (count >= INT_MAX / 2) return 0;
    count *= 2;
    if (!(new_entries = realloc( process->entries, count * sizeof(struct handle_entry) )))
    {
        SET_ERROR( ERROR_OUTOFMEMORY );
        return 0;
    }
    process->handle_count = count;
    process->entries      = new_entries;
    return 1;
}

/* allocate a handle for an object, incrementing its refcount */
/* return the handle, or -1 on error */
int alloc_handle( struct process *process, void *obj, unsigned int access,
                  int inherit )
{
    struct handle_entry *entry;
    int handle;

    assert( !(access & RESERVED_ALL) );
    if (inherit) access |= RESERVED_INHERIT;

    /* find the first free entry */

    if (!(entry = process->entries)) return -1;
    for (handle = 0; handle <= process->handle_last; handle++, entry++)
        if (!entry->ptr) goto found;

    if (handle >= process->handle_count)
    {
        if (!grow_handle_table( process )) return -1;
        entry = process->entries + handle;  /* the table may have moved */
    }
    process->handle_last = handle;

 found:
    entry->ptr    = grab_object( obj );
    entry->access = access;
    return handle + 1;  /* avoid handle 0 */
}

/* allocate a specific handle for an object, incrementing its refcount */
static int alloc_specific_handle( struct process *process, void *obj, int handle,
                                  unsigned int access, int inherit )
{
    struct handle_entry *entry;
    struct object *old;

    if (handle == -1) return alloc_handle( process, obj, access, inherit );

    assert( !(access & RESERVED_ALL) );
    if (inherit) access |= RESERVED_INHERIT;

    handle--;  /* handles start at 1 */
    if ((handle < 0) || (handle > process->handle_last))
    {
        SET_ERROR( ERROR_INVALID_HANDLE );
        return -1;
    }
    entry = process->entries + handle;

    old = entry->ptr;
    entry->ptr = grab_object( obj );
    entry->access = access;
    if (old) release_object( old );
    return handle + 1;
}

/* return an handle entry, or NULL if the handle is invalid */
static struct handle_entry *get_handle( struct process *process, int handle )
{
    struct handle_entry *entry;

    if (HANDLE_IS_GLOBAL(handle))
    {
        handle = HANDLE_GLOBAL_TO_LOCAL(handle);
        process = initial_process;
    }
    handle--;  /* handles start at 1 */
    if ((handle < 0) || (handle > process->handle_last)) goto error;
    entry = process->entries + handle;
    if (!entry->ptr) goto error;
    return entry;

 error:
    SET_ERROR( ERROR_INVALID_HANDLE );
    return NULL;
}

/* attempt to shrink a table */
/* return 1 if OK, 0 on error */
static int shrink_handle_table( struct process *process )
{
    struct handle_entry *new_entries;
    struct handle_entry *entry = process->entries + process->handle_last;
    int count = process->handle_count;

    while (process->handle_last >= 0)
    {
        if (entry->ptr) break;
        process->handle_last--;
        entry--;
    }
    if (process->handle_last >= count / 4) return 1;  /* no need to shrink */
    if (count < MIN_HANDLE_ENTRIES * 2) return 1;  /* too small to shrink */
    count /= 2;
    if (!(new_entries = realloc( process->entries,
                                 count * sizeof(struct handle_entry) )))
        return 0;
    process->handle_count = count;
    process->entries      = new_entries;
    return 1;
}

/* copy the handle table of the parent process */
/* return 1 if OK, 0 on error */
static int copy_handle_table( struct process *process, struct process *parent )
{
    struct handle_entry *ptr;
    int i, count, last;

    if (!parent)  /* first process */
    {
        count = MIN_HANDLE_ENTRIES;
        last  = -1;
    }
    else
    {
        assert( parent->entries );
        count = parent->handle_count;
        last  = parent->handle_last;
    }

    if (!(ptr = mem_alloc( count * sizeof(struct handle_entry)))) return 0;
    process->entries      = ptr;
    process->handle_count = count;
    process->handle_last  = last;

    if (last >= 0)
    {
        memcpy( ptr, parent->entries, (last + 1) * sizeof(struct handle_entry) );
        for (i = 0; i <= last; i++, ptr++)
        {
            if (!ptr->ptr) continue;
            if (ptr->access & RESERVED_INHERIT) grab_object( ptr->ptr );
            else ptr->ptr = NULL; /* don't inherit this entry */
        }
    }
    /* attempt to shrink the table */
    shrink_handle_table( process );
    return 1;
}

/* close a handle and decrement the refcount of the associated object */
/* return 1 if OK, 0 on error */
int close_handle( struct process *process, int handle )
{
    struct handle_entry *entry;
    struct object *obj;

    if (HANDLE_IS_GLOBAL(handle))
    {
        handle = HANDLE_GLOBAL_TO_LOCAL(handle);
        process = initial_process;
    }
    if (!(entry = get_handle( process, handle ))) return 0;
    if (entry->access & RESERVED_CLOSE_PROTECT) return 0;  /* FIXME: error code */
    obj = entry->ptr;
    entry->ptr = NULL;
    if (handle-1 == process->handle_last) shrink_handle_table( process );
    release_object( obj );
    return 1;
}

/* retrieve the object corresponding to a handle, incrementing its refcount */
struct object *get_handle_obj( struct process *process, int handle,
                               unsigned int access, const struct object_ops *ops )
{
    struct handle_entry *entry;
    struct object *obj;

    switch( handle )
    {
    case 0xfffffffe:  /* current thread pseudo-handle */
        obj = &current->obj;
        break;
    case 0x7fffffff:  /* current process pseudo-handle */
        obj = (struct object *)current->process;
        break;
    default:
        if (!(entry = get_handle( process, handle ))) return NULL;
        if ((entry->access & access) != access)
        {
            SET_ERROR( ERROR_ACCESS_DENIED );
            return NULL;
        }
        obj = entry->ptr;
        break;
    }
    if (ops && (obj->ops != ops))
    {
        SET_ERROR( ERROR_INVALID_HANDLE );  /* not the right type */
        return NULL;
    }
    return grab_object( obj );
}

/* get/set the handle reserved flags */
/* return the new flags (or -1 on error) */
int set_handle_info( struct process *process, int handle, int mask, int flags )
{
    struct handle_entry *entry;

    if (!(entry = get_handle( process, handle ))) return -1;
    mask  = (mask << RESERVED_SHIFT) & RESERVED_ALL;
    flags = (flags << RESERVED_SHIFT) & mask;
    entry->access = (entry->access & ~mask) | flags;
    return (entry->access & RESERVED_ALL) >> RESERVED_SHIFT;
}

/* duplicate a handle */
int duplicate_handle( struct process *src, int src_handle, struct process *dst,
                      int dst_handle, unsigned int access, int inherit, int options )
{
    int res;
    struct handle_entry *entry = get_handle( src, src_handle );
    if (!entry) return -1;

    if (options & DUP_HANDLE_SAME_ACCESS) access = entry->access;
    if (options & DUP_HANDLE_MAKE_GLOBAL) dst = initial_process;
    access &= ~RESERVED_ALL;
    res = alloc_specific_handle( dst, entry->ptr, dst_handle, access, inherit );
    if (options & DUP_HANDLE_MAKE_GLOBAL) res = HANDLE_LOCAL_TO_GLOBAL(res);
    return res;
}

/* open a new handle to an existing object */
int open_object( const char *name, const struct object_ops *ops,
                 unsigned int access, int inherit )
{
    struct object *obj = find_object( name );
    if (!obj) return -1;  /* FIXME: set error code */
    if (ops && obj->ops != ops)
    {
        release_object( obj );
        return -1;  /* FIXME: set error code */
    }
    return alloc_handle( current->process, obj, access, inherit );
}

/* dump a handle table on stdout */
void dump_handles( struct process *process )
{
    struct handle_entry *entry;
    int i;

    if (!process->entries) return;
    entry = process->entries;
    for (i = 0; i <= process->handle_last; i++, entry++)
    {
        if (!entry->ptr) continue;
        printf( "%5d: %p %08x ", i + 1, entry->ptr, entry->access );
        entry->ptr->ops->dump( entry->ptr, 0 );
    }
}

/* kill a process on the spot */
void kill_process( struct process *process, int exit_code )
{
    while (process->thread_list)
        kill_thread( process->thread_list, exit_code );
}

/* get all information about a process */
void get_process_info( struct process *process,
                       struct get_process_info_reply *reply )
{
    reply->pid              = process;
    reply->exit_code        = process->exit_code;
    reply->priority         = process->priority;
    reply->process_affinity = process->affinity;
    reply->system_affinity  = 1;
}

/* set all information about a process */
void set_process_info( struct process *process,
                       struct set_process_info_request *req )
{
    if (req->mask & SET_PROCESS_INFO_PRIORITY)
        process->priority = req->priority;
    if (req->mask & SET_PROCESS_INFO_AFFINITY)
    {
        if (req->affinity != 1) SET_ERROR( ERROR_INVALID_PARAMETER );
        else process->affinity = req->affinity;
    }
}

/* allocate a console for this process */
int alloc_console( struct process *process )
{
    struct object *obj[2];
    if (process->console_in || process->console_out)
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return 0;
    }
    if (!create_console( -1, obj )) return 0;
    process->console_in  = obj[0];
    process->console_out = obj[1];
    return 1;
}

/* free the console for this process */
int free_console( struct process *process )
{
    if (process->console_in) release_object( process->console_in );
    if (process->console_out) release_object( process->console_out );
    process->console_in = process->console_out = NULL;
    return 1;
}

/* get the process console */
struct object *get_console( struct process *process, int output )
{
    struct object *obj;
    if (!(obj = output ? process->console_out : process->console_in))
        return NULL;
    return grab_object( obj );
}

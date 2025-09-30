/*
 * Wine server objects
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#ifndef __WINE_SERVER_OBJECT_H
#define __WINE_SERVER_OBJECT_H

#include <poll.h>
#include <sys/time.h>
#include "wine/server_protocol.h"
#include "wine/list.h"

#define DEBUG_OBJECTS

/* kernel objects */

struct namespace;
struct object;
struct object_name;
struct thread;
struct process;
struct token;
struct file;
struct wait_queue_entry;
struct async;
struct async_queue;
struct winstation;
struct object_type;


struct unicode_str
{
    const WCHAR *str;
    data_size_t  len;
};

/* object type descriptor */
struct type_descr
{
    struct unicode_str name;          /* type name */
    unsigned int       valid_access;  /* mask for valid access bits */
    struct generic_map mapping;       /* generic access mapping */
    unsigned int       index;         /* index in global array of types */
    unsigned int       obj_count;     /* count of objects of this type */
    unsigned int       handle_count;  /* count of handles of this type */
    unsigned int       obj_max;       /* max count of objects of this type */
    unsigned int       handle_max;    /* max count of handles of this type */
};

/* operations valid on all objects */
struct object_ops
{
    /* size of this object type */
    size_t size;
    /* type descriptor */
    struct type_descr *type;
    /* dump the object (for debugging) */
    void (*dump)(struct object *,int);
    /* add a thread to the object wait queue */
    int  (*add_queue)(struct object *,struct wait_queue_entry *);
    /* remove a thread from the object wait queue */
    void (*remove_queue)(struct object *,struct wait_queue_entry *);
    /* is object signaled? */
    int  (*signaled)(struct object *,struct wait_queue_entry *);
    /* wait satisfied */
    void (*satisfied)(struct object *,struct wait_queue_entry *);
    /* signal an object */
    int  (*signal)(struct object *, unsigned int);
    /* return an fd object that can be used to read/write from the object */
    struct fd *(*get_fd)(struct object *);
    /* return a sync that can be used to wait/signal the object */
    struct object *(*get_sync)(struct object *);
    /* map access rights to the specific rights for this object */
    unsigned int (*map_access)(struct object *, unsigned int);
    /* returns the security descriptor of the object */
    struct security_descriptor *(*get_sd)( struct object * );
    /* sets the security descriptor of the object */
    int (*set_sd)( struct object *, const struct security_descriptor *, unsigned int );
    /* get the object full name */
    WCHAR *(*get_full_name)(struct object *, data_size_t, data_size_t *);
    /* lookup a name if an object has a namespace */
    struct object *(*lookup_name)(struct object *, struct unicode_str *,unsigned int,struct object *);
    /* link an object's name into a parent object */
    int (*link_name)(struct object *, struct object_name *, struct object *);
    /* unlink an object's name from its parent */
    void (*unlink_name)(struct object *, struct object_name *);
    /* open a file object to access this object */
    struct object *(*open_file)(struct object *, unsigned int access, unsigned int sharing,
                                unsigned int options);
    /* return list of kernel objects */
    struct list *(*get_kernel_obj_list)(struct object *);
    /* close a handle to this object */
    int (*close_handle)(struct object *,struct process *,obj_handle_t);
    /* destroy on refcount == 0 */
    void (*destroy)(struct object *);
};

struct object
{
    unsigned int              refcount;    /* reference count */
    unsigned int              handle_count;/* handle count */
    const struct object_ops  *ops;
    struct list               wait_queue;
    struct object_name       *name;
    struct security_descriptor *sd;
    unsigned int              is_permanent:1;
#ifdef DEBUG_OBJECTS
    struct list               obj_list;
#endif
};

struct object_name
{
    struct list         entry;           /* entry in the hash list */
    struct object      *obj;             /* object owning this name */
    struct object      *parent;          /* parent object */
    data_size_t         len;             /* name length in bytes */
    WCHAR               name[1];
};

struct wait_queue_entry
{
    struct list         entry;
    struct object      *obj;
    struct thread_wait *wait;
};

extern void mark_block_noaccess( void *ptr, size_t size );
extern void mark_block_uninitialized( void *ptr, size_t size );
extern void *mem_alloc( size_t size ) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(free) __WINE_MALLOC;
extern void *memdup( const void *data, size_t len ) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(free);
extern void *alloc_object( const struct object_ops *ops );
extern void namespace_add( struct namespace *namespace, struct object_name *ptr );
extern const WCHAR *get_object_name( struct object *obj, data_size_t *len );
extern WCHAR *default_get_full_name( struct object *obj, data_size_t max, data_size_t *ret_len ) __WINE_DEALLOC(free) __WINE_MALLOC;
extern void dump_object_name( struct object *obj );
extern struct object *lookup_named_object( struct object *root, const struct unicode_str *name,
                                           unsigned int attr, struct unicode_str *name_left );
extern data_size_t get_path_element( const WCHAR *name, data_size_t len );
extern void *create_named_object( struct object *parent, const struct object_ops *ops,
                                  const struct unicode_str *name, unsigned int attributes,
                                  const struct security_descriptor *sd );
extern void *open_named_object( struct object *parent, const struct object_ops *ops,
                                const struct unicode_str *name, unsigned int attributes );
extern void unlink_named_object( struct object *obj );
extern struct namespace *create_namespace( unsigned int hash_size );
extern void free_kernel_objects( struct object *obj );
/* grab/release_object can take any pointer, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern struct object *grab_object( void *obj );
extern void release_object( void *obj );
extern struct object *find_object( const struct namespace *namespace, const struct unicode_str *name,
                                   unsigned int attributes );
extern struct object *find_object_index( const struct namespace *namespace, unsigned int index );
extern int no_add_queue( struct object *obj, struct wait_queue_entry *entry );
extern void no_satisfied( struct object *obj, struct wait_queue_entry *entry );
extern int no_signal( struct object *obj, unsigned int access );
extern struct fd *no_get_fd( struct object *obj );
extern struct object *default_get_sync( struct object *obj );
static inline struct object *get_obj_sync( struct object *obj ) { return obj->ops->get_sync( obj ); }
extern unsigned int default_map_access( struct object *obj, unsigned int access );
extern struct security_descriptor *default_get_sd( struct object *obj );
extern int default_set_sd( struct object *obj, const struct security_descriptor *sd, unsigned int set_info );
extern int set_sd_defaults_from_token( struct object *obj, const struct security_descriptor *sd,
                                       unsigned int set_info, struct token *token );
extern WCHAR *no_get_full_name( struct object *obj, data_size_t max, data_size_t *ret_len );
extern struct object *no_lookup_name( struct object *obj, struct unicode_str *name,
                                      unsigned int attributes, struct object *root );
extern int no_link_name( struct object *obj, struct object_name *name, struct object *parent );
extern void default_unlink_name( struct object *obj, struct object_name *name );
extern struct object *no_open_file( struct object *obj, unsigned int access, unsigned int sharing,
                                    unsigned int options );
extern struct list *no_kernel_obj_list( struct object *obj );
extern int no_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
extern void no_destroy( struct object *obj );
#ifdef DEBUG_OBJECTS
extern void dump_objects(void);
extern void close_objects(void);
#endif

struct reserve *reserve_obj_associate_apc( struct process *process, obj_handle_t handle, struct object *apc );
void reserve_obj_unbind( struct reserve *reserve );

static inline void make_object_permanent( struct object *obj ) { obj->is_permanent = 1; }
static inline void make_object_temporary( struct object *obj ) { obj->is_permanent = 0; }

static inline unsigned int map_access( unsigned int access, const struct generic_map *mapping )
{
    if (access & GENERIC_READ)    access |= mapping->read;
    if (access & GENERIC_WRITE)   access |= mapping->write;
    if (access & GENERIC_EXECUTE) access |= mapping->exec;
    if (access & GENERIC_ALL)     access |= mapping->all;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static inline void *mem_append( void *ptr, const void *src, data_size_t len )
{
    if (!len) return ptr;
    memcpy( ptr, src, len );
    return (char *)ptr + len;
}

/* event functions */

struct event_sync;
struct event;
struct keyed_event;

extern struct event_sync *create_server_internal_sync( int manual, int signaled );
extern struct event_sync *create_internal_sync( int manual, int signaled );
extern void signal_sync( struct event_sync *sync );
extern void reset_sync( struct event_sync *sync );

extern struct event *create_event( struct object *root, const struct unicode_str *name,
                                   unsigned int attr, int manual_reset, int initial_state,
                                   const struct security_descriptor *sd );
extern struct keyed_event *create_keyed_event( struct object *root, const struct unicode_str *name,
                                               unsigned int attr, const struct security_descriptor *sd );
extern struct event *get_event_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern struct keyed_event *get_keyed_event_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern void set_event( struct event *event );
extern void reset_event( struct event *event );

/* mutex functions */

extern void abandon_mutexes( struct thread *thread );

/* in-process synchronization functions */

struct inproc_sync;
extern int get_inproc_device_fd(void);
extern struct inproc_sync *create_inproc_internal_sync( int manual, int signaled );
extern void signal_inproc_sync( struct inproc_sync *sync );
extern void reset_inproc_sync( struct inproc_sync *sync );

/* serial functions */

int get_serial_async_timeout(struct object *obj, int type, int count);

/* socket functions */

extern void sock_init(void);

/* debugger functions */

extern void generate_debug_event( struct thread *thread, int code, const void *arg );
extern void resume_delayed_debug_events( struct thread *thread );
extern void generate_startup_debug_events( struct process *process );

/* registry functions */

extern unsigned int supported_machines_count;
extern unsigned short supported_machines[8];
extern unsigned short native_machine;
extern void init_registry(void);
extern void flush_registry(void);

static inline int is_machine_32bit( unsigned short machine )
{
    return machine == IMAGE_FILE_MACHINE_I386 || machine == IMAGE_FILE_MACHINE_ARMNT;
}
static inline int is_machine_64bit( unsigned short machine )
{
    return machine == IMAGE_FILE_MACHINE_AMD64 || machine == IMAGE_FILE_MACHINE_ARM64;
}
static inline int is_machine_supported( unsigned short machine )
{
    unsigned int i;
    for (i = 0; i < supported_machines_count; i++) if (supported_machines[i] == machine) return 1;
    if (native_machine == IMAGE_FILE_MACHINE_ARM64) return machine == IMAGE_FILE_MACHINE_AMD64;
    return 0;
}

/* signal functions */

extern void start_watchdog(void);
extern void stop_watchdog(void);
extern int watchdog_triggered(void);
extern void init_signals(void);

/* atom functions */

extern struct object *create_atom_table(void);
extern void set_global_atom_table( struct object *obj );
extern void set_user_atom_table( struct object *obj );

struct atom_table;
extern struct atom_table *get_global_atom_table(void);
extern struct atom_table *get_user_atom_table(void);
extern atom_t add_atom( struct atom_table *table, const struct unicode_str *str );
extern atom_t find_atom( struct atom_table *table, const struct unicode_str *str );
extern atom_t grab_atom( struct atom_table *table, atom_t atom );
extern void release_atom( struct atom_table *table, atom_t atom );

/* directory functions */

extern struct object *get_root_directory(void);
extern struct object *get_directory_obj( struct process *process, obj_handle_t handle );
extern int directory_link_name( struct object *obj, struct object_name *name, struct object *parent );
extern void init_directories( struct fd *intl_fd );

/* thread functions */

extern void init_threading(void);

/* symbolic link functions */

extern struct object *create_root_symlink( struct object *root, const struct unicode_str *name,
                                           unsigned int attr, const struct security_descriptor *sd );
extern struct object *create_obj_symlink( struct object *root, const struct unicode_str *name,
                                          unsigned int attr, struct object *target,
                                          const struct security_descriptor *sd );
extern struct object *create_symlink( struct object *root, const struct unicode_str *name,
                                      unsigned int attr, const struct unicode_str *target,
                                      const struct security_descriptor *sd );

/* global variables */

  /* command-line options */
extern int debug_level;
extern int foreground;
extern timeout_t master_socket_timeout;
extern const char *server_argv0;

  /* server start time used for GetTickCount() */
extern timeout_t server_start_time;

/* object types */
extern struct type_descr no_type;
extern struct type_descr objtype_type;
extern struct type_descr directory_type;
extern struct type_descr symlink_type;
extern struct type_descr token_type;
extern struct type_descr job_type;
extern struct type_descr process_type;
extern struct type_descr thread_type;
extern struct type_descr debug_obj_type;
extern struct type_descr event_type;
extern struct type_descr mutex_type;
extern struct type_descr semaphore_type;
extern struct type_descr timer_type;
extern struct type_descr keyed_event_type;
extern struct type_descr winstation_type;
extern struct type_descr desktop_type;
extern struct type_descr device_type;
extern struct type_descr completion_type;
extern struct type_descr file_type;
extern struct type_descr mapping_type;
extern struct type_descr key_type;
extern struct type_descr apc_reserve_type;
extern struct type_descr completion_reserve_type;

#define KEYEDEVENT_WAIT       0x0001
#define KEYEDEVENT_WAKE       0x0002
#define KEYEDEVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x0003)

#endif  /* __WINE_SERVER_OBJECT_H */

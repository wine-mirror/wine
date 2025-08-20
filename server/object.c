/*
 * Server-side objects
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

#include "config.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "process.h"
#include "thread.h"
#include "unicode.h"
#include "security.h"
#include "handle.h"
#include "request.h"


struct namespace
{
    unsigned int        hash_size;       /* size of hash table */
    struct list         names[1];        /* array of hash entry lists */
};


struct type_descr no_type =
{
    { NULL, 0 },                /* name */
    STANDARD_RIGHTS_REQUIRED,   /* valid_access */
    {                           /* mapping */
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_REQUIRED
    },
};

struct reserve
{
    struct object obj;          /* object header */
    int    type;                /* reserve object type. See MEMORY_RESERVE_OBJECT_TYPE */
    struct object *bound_obj;   /* object using reserve object */
    /* BYTE *memory */;         /* reserved memory */
};

static const WCHAR apc_reserve_type_name[] = {'U','s','e','r','A','p','c','R','e','s','e','r','v','e'};
static const WCHAR completion_reserve_name[] = {'I','o','C','o','m','p','l','e','t','i','o','n','R','e','s','e','r','v','e'};

struct type_descr apc_reserve_type =
{
    { apc_reserve_type_name, sizeof(apc_reserve_type_name) },       /* name */
    STANDARD_RIGHTS_REQUIRED | 0x3,                                 /* valid_access */
    {                                                               /* mapping */
        STANDARD_RIGHTS_READ | 0x1,
        STANDARD_RIGHTS_WRITE | 0x2,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_REQUIRED | 0x3
    },
};

struct type_descr completion_reserve_type =
{
    { completion_reserve_name, sizeof(completion_reserve_name) },   /* name */
    STANDARD_RIGHTS_REQUIRED | 0x3,                                 /* valid_access */
    {                                                               /* mapping */
        STANDARD_RIGHTS_READ | 0x1,
        STANDARD_RIGHTS_WRITE | 0x2,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_REQUIRED | 0x3
    },
};

static void dump_reserve( struct object *obj, int verbose );

static const struct object_ops apc_reserve_ops =
{
    sizeof(struct reserve),     /* size */
    &apc_reserve_type,          /* type */
    dump_reserve,               /* dump */
    no_add_queue,               /* add_queue */
    NULL,                       /* remove_queue */
    NULL,                       /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_get_sync,           /* get_sync */
    default_map_access,         /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    default_get_full_name,      /* get_full_name */
    no_lookup_name,             /* lookup_name */
    directory_link_name,        /* link_name */
    default_unlink_name,        /* unlink_name */
    no_open_file,               /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    no_destroy                  /* destroy */
};

static const struct object_ops completion_reserve_ops =
{
    sizeof(struct reserve),    /* size */
    &completion_reserve_type,  /* type */
    dump_reserve,              /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    no_satisfied,              /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_get_sync,          /* get_sync */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    no_destroy                 /* destroy */
};

#ifdef DEBUG_OBJECTS
static struct list object_list = LIST_INIT(object_list);

void dump_objects(void)
{
    struct object *ptr;

    LIST_FOR_EACH_ENTRY( ptr, &object_list, struct object, obj_list )
    {
        fprintf( stderr, "%p:%d: ", ptr, ptr->refcount );
        dump_object_name( ptr );
        ptr->ops->dump( ptr, 1 );
    }
}

void close_objects(void)
{
    /* release the permanent objects */
    for (;;)
    {
        struct object *obj;
        int found = 0;

        LIST_FOR_EACH_ENTRY( obj, &object_list, struct object, obj_list )
        {
            if (!(found = obj->is_permanent)) continue;
            obj->is_permanent = 0;
            release_object( obj );
            break;
        }
        if (!found) break;
    }

    dump_objects();  /* dump any remaining objects */
}

#endif  /* DEBUG_OBJECTS */

/*****************************************************************/

/* mark a block of memory as not accessible for debugging purposes */
void mark_block_noaccess( void *ptr, size_t size )
{
    memset( ptr, 0xfe, size );
#if defined(VALGRIND_MAKE_MEM_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_NOACCESS( ptr, size ) );
#elif defined(VALGRIND_MAKE_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_NOACCESS( ptr, size ) );
#endif
}

/* mark a block of memory as uninitialized for debugging purposes */
void mark_block_uninitialized( void *ptr, size_t size )
{
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_UNDEFINED( ptr, size ));
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_DISCARD( VALGRIND_MAKE_WRITABLE( ptr, size ));
#endif
    memset( ptr, 0x55, size );
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_UNDEFINED( ptr, size ));
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_DISCARD( VALGRIND_MAKE_WRITABLE( ptr, size ));
#endif
}

/* malloc replacement */
void *mem_alloc( size_t size )
{
    void *ptr = malloc( size );
    if (ptr) mark_block_uninitialized( ptr, size );
    else set_error( STATUS_NO_MEMORY );
    return ptr;
}

/* duplicate a block of memory */
void *memdup( const void *data, size_t len )
{
    void *ptr = malloc( len );
    if (ptr) memcpy( ptr, data, len );
    else set_error( STATUS_NO_MEMORY );
    return ptr;
}


/*****************************************************************/

void namespace_add( struct namespace *namespace, struct object_name *ptr )
{
    unsigned int hash = hash_strW( ptr->name, ptr->len, namespace->hash_size );

    list_add_head( &namespace->names[hash], &ptr->entry );
}

/* allocate a name for an object */
static struct object_name *alloc_name( const struct unicode_str *name )
{
    struct object_name *ptr;

    if ((ptr = mem_alloc( sizeof(*ptr) + name->len - sizeof(ptr->name) )))
    {
        ptr->len = name->len;
        ptr->parent = NULL;
        memcpy( ptr->name, name->str, name->len );
    }
    return ptr;
}

/* get the name of an existing object */
const WCHAR *get_object_name( struct object *obj, data_size_t *len )
{
    struct object_name *ptr = obj->name;
    if (!ptr) return NULL;
    *len = ptr->len;
    return ptr->name;
}

/* get the full path name of an existing object */
WCHAR *default_get_full_name( struct object *obj, data_size_t max, data_size_t *ret_len )
{
    static const WCHAR backslash = '\\';
    struct object *ptr = obj;
    data_size_t len = 0;
    char *ret;

    while (ptr && ptr->name)
    {
        struct object_name *name = ptr->name;
        len += name->len + sizeof(WCHAR);
        ptr = name->parent;
    }
    if (!len) return NULL;
    if (!(ret = malloc( len ))) return NULL;

    *ret_len = len;
    while (obj && obj->name)
    {
        struct object_name *name = obj->name;
        memcpy( ret + len - name->len, name->name, name->len );
        len -= name->len + sizeof(WCHAR);
        memcpy( ret + len, &backslash, sizeof(WCHAR) );
        obj = name->parent;
    }
    if (*ret_len > max) set_error( STATUS_INFO_LENGTH_MISMATCH );
    return (WCHAR *)ret;
}

/* allocate and initialize an object */
void *alloc_object( const struct object_ops *ops )
{
    struct object *obj = mem_alloc( ops->size );
    if (obj)
    {
        obj->refcount     = 1;
        obj->handle_count = 0;
        obj->is_permanent = 0;
        obj->ops          = ops;
        obj->name         = NULL;
        obj->sd           = NULL;
        list_init( &obj->wait_queue );
#ifdef DEBUG_OBJECTS
        list_add_head( &object_list, &obj->obj_list );
#endif
        obj->ops->type->obj_count++;
        obj->ops->type->obj_max = max( obj->ops->type->obj_max, obj->ops->type->obj_count );
        return obj;
    }
    return NULL;
}

/* free an object once it has been destroyed */
static void free_object( struct object *obj )
{
    free( obj->sd );
    obj->ops->type->obj_count--;
#ifdef DEBUG_OBJECTS
    list_remove( &obj->obj_list );
    memset( obj, 0xaa, obj->ops->size );
#endif
    free( obj );
}

/* find an object by name starting from the specified root */
/* if it doesn't exist, its parent is returned, and name_left contains the remaining name */
struct object *lookup_named_object( struct object *root, const struct unicode_str *name,
                                    unsigned int attr, struct unicode_str *name_left )
{
    static int recursion_count;
    struct object *obj, *parent;
    struct unicode_str name_tmp = *name, *ptr = &name_tmp;

    if (root)
    {
        /* if root is specified path shouldn't start with backslash */
        if (name_tmp.len && name_tmp.str[0] == '\\')
        {
            set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
            return NULL;
        }
        parent = grab_object( root );
    }
    else
    {
        if (!name_tmp.len || name_tmp.str[0] != '\\')
        {
            set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
            return NULL;
        }
        /* skip leading backslash */
        name_tmp.str++;
        name_tmp.len -= sizeof(WCHAR);
        parent = root = get_root_directory();
    }

    if (!name_tmp.len) ptr = NULL;  /* special case for empty path */

    if (recursion_count > 32)
    {
        set_error( STATUS_INVALID_PARAMETER );
        release_object( parent );
        return NULL;
    }
    recursion_count++;
    clear_error();

    while ((obj = parent->ops->lookup_name( parent, ptr, attr, root )))
    {
        /* move to the next element */
        release_object ( parent );
        parent = obj;
    }

    recursion_count--;
    if (get_error())
    {
        release_object( parent );
        return NULL;
    }

    if (name_left) *name_left = name_tmp;
    return parent;
}

/* return length of first path element in name */
data_size_t get_path_element( const WCHAR *name, data_size_t len )
{
    data_size_t i;

    for (i = 0; i < len / sizeof(WCHAR); i++) if (name[i] == '\\') break;
    return i * sizeof(WCHAR);
}

static struct object *create_object( struct object *parent, const struct object_ops *ops,
                                     const struct unicode_str *name, unsigned int attributes,
                                     const struct security_descriptor *sd )
{
    struct object *obj;
    struct object_name *name_ptr;

    if (!(name_ptr = alloc_name( name ))) return NULL;
    if (!(obj = alloc_object( ops ))) goto failed;
    if (sd && !default_set_sd( obj, sd, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                               DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION ))
        goto failed;
    if (!obj->ops->link_name( obj, name_ptr, parent )) goto failed;

    name_ptr->obj = obj;
    obj->name = name_ptr;
    return obj;

failed:
    if (obj) free_object( obj );
    free( name_ptr );
    return NULL;
}

/* create an object as named child under the specified parent */
void *create_named_object( struct object *parent, const struct object_ops *ops,
                           const struct unicode_str *name, unsigned int attributes,
                           const struct security_descriptor *sd )
{
    struct object *obj, *new_obj;
    struct unicode_str new_name;

    clear_error();

    if (!name || !name->len)
    {
        if (!(new_obj = alloc_object( ops ))) return NULL;
        if (sd && !default_set_sd( new_obj, sd, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION ))
        {
            free_object( new_obj );
            return NULL;
        }
    }
    else
    {
        if (!(obj = lookup_named_object( parent, name, attributes, &new_name ))) return NULL;

        if (!new_name.len)
        {
            if (attributes & OBJ_OPENIF && obj->ops == ops)
            {
                set_error( STATUS_OBJECT_NAME_EXISTS );
                return obj;
            }
            release_object( obj );
            if (attributes & OBJ_OPENIF)
                set_error( STATUS_OBJECT_TYPE_MISMATCH );
            else
                set_error( STATUS_OBJECT_NAME_COLLISION );
            return NULL;
        }

        new_obj = create_object( obj, ops, &new_name, attributes, sd );
        release_object( obj );
        if (!new_obj) return NULL;
    }

    if (attributes & OBJ_PERMANENT)
    {
        make_object_permanent( new_obj );
        grab_object( new_obj );
    }
    return new_obj;
}

/* open a object by name under the specified parent */
void *open_named_object( struct object *parent, const struct object_ops *ops,
                         const struct unicode_str *name, unsigned int attributes )
{
    struct unicode_str name_left;
    struct object *obj;

    if ((obj = lookup_named_object( parent, name, attributes, &name_left )))
    {
        if (name_left.len) /* not fully parsed */
            set_error( STATUS_OBJECT_NAME_NOT_FOUND );
        else if (ops && obj->ops != ops)
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
        else
            return obj;

        release_object( obj );
    }
    return NULL;
}

/* recursive helper for dump_object_name */
static void dump_name( struct object *obj )
{
    struct object_name *name = obj->name;

    if (!name) return;
    if (name->parent) dump_name( name->parent );
    fputs( "\\\\", stderr );
    dump_strW( name->name, name->len, stderr, "[]" );
}

/* dump the name of an object to stderr */
void dump_object_name( struct object *obj )
{
    if (!obj->name) return;
    fputc( '[', stderr );
    dump_name( obj );
    fputs( "] ", stderr );
}

/* unlink a named object from its namespace, without freeing the object itself */
void unlink_named_object( struct object *obj )
{
    struct object_name *name_ptr = obj->name;

    if (!name_ptr) return;
    obj->name = NULL;
    obj->ops->unlink_name( obj, name_ptr );
    if (name_ptr->parent) release_object( name_ptr->parent );
    free( name_ptr );
}

/* grab an object (i.e. increment its refcount) and return the object */
struct object *grab_object( void *ptr )
{
    struct object *obj = (struct object *)ptr;
    assert( obj->refcount < INT_MAX );
    obj->refcount++;
    return obj;
}

/* release an object (i.e. decrement its refcount) */
void release_object( void *ptr )
{
    struct object *obj = (struct object *)ptr;
    assert( obj->refcount );
    if (!--obj->refcount)
    {
        assert( !obj->handle_count );
        /* if the refcount is 0, nobody can be in the wait queue */
        assert( list_empty( &obj->wait_queue ));
        free_kernel_objects( obj );
        unlink_named_object( obj );
        obj->ops->destroy( obj );
        free_object( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const struct namespace *namespace, const struct unicode_str *name,
                            unsigned int attributes )
{
    const struct list *list;
    const struct object_name *ptr;

    if (!name || !name->len) return NULL;

    list = &namespace->names[ hash_strW( name->str, name->len, namespace->hash_size ) ];
    LIST_FOR_EACH_ENTRY( ptr, list, struct object_name, entry )
    {
        if (ptr->len != name->len) continue;
        if (attributes & OBJ_CASE_INSENSITIVE)
        {
            if (!memicmp_strW( ptr->name, name->str, name->len ))
                return grab_object( ptr->obj );
        }
        else
        {
            if (!memcmp( ptr->name, name->str, name->len ))
                return grab_object( ptr->obj );
        }
    }
    return NULL;
}

/* find an object by its index; the refcount is incremented */
struct object *find_object_index( const struct namespace *namespace, unsigned int index )
{
    unsigned int i;

    /* FIXME: not efficient at all */
    for (i = 0; i < namespace->hash_size; i++)
    {
        const struct object_name *ptr;
        LIST_FOR_EACH_ENTRY( ptr, &namespace->names[i], const struct object_name, entry )
        {
            if (!index--) return grab_object( ptr->obj );
        }
    }
    return NULL;
}

/* allocate a namespace */
struct namespace *create_namespace( unsigned int hash_size )
{
    struct namespace *namespace;
    unsigned int i;

    namespace = mem_alloc( sizeof(*namespace) + (hash_size - 1) * sizeof(namespace->names[0]) );
    if (namespace)
    {
        namespace->hash_size      = hash_size;
        for (i = 0; i < hash_size; i++) list_init( &namespace->names[i] );
    }
    return namespace;
}

/* functions for unimplemented/default object operations */

int no_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

void no_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
}

int no_signal( struct object *obj, unsigned int access )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

struct fd *no_get_fd( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return NULL;
}

struct object *default_get_sync( struct object *obj )
{
    return grab_object( obj );
}

unsigned int default_map_access( struct object *obj, unsigned int access )
{
    return map_access( access, &obj->ops->type->mapping );
}

struct security_descriptor *default_get_sd( struct object *obj )
{
    return obj->sd;
}

int set_sd_defaults_from_token( struct object *obj, const struct security_descriptor *sd,
                                unsigned int set_info, struct token *token )
{
    struct security_descriptor new_sd, *new_sd_ptr;
    int present;
    const struct sid *owner = NULL, *group = NULL;
    const struct acl *sacl, *dacl;
    struct acl *replaced_sacl = NULL;
    char *ptr;

    if (!set_info) return 1;

    new_sd.control = sd->control & ~SE_SELF_RELATIVE;

    if (set_info & OWNER_SECURITY_INFORMATION && sd->owner_len)
    {
        owner = sd_get_owner( sd );
        new_sd.owner_len = sd->owner_len;
    }
    else if (obj->sd && obj->sd->owner_len)
    {
        owner = sd_get_owner( obj->sd );
        new_sd.owner_len = obj->sd->owner_len;
    }
    else if (token)
    {
        owner = token_get_owner( token );
        new_sd.owner_len = sid_len( owner );
    }
    else new_sd.owner_len = 0;

    if (set_info & GROUP_SECURITY_INFORMATION && sd->group_len)
    {
        group = sd_get_group( sd );
        new_sd.group_len = sd->group_len;
    }
    else if (obj->sd && obj->sd->group_len)
    {
        group = sd_get_group( obj->sd );
        new_sd.group_len = obj->sd->group_len;
    }
    else if (token)
    {
        group = token_get_primary_group( token );
        new_sd.group_len = sid_len( group );
    }
    else new_sd.group_len = 0;

    sacl = sd_get_sacl( sd, &present );
    if (set_info & SACL_SECURITY_INFORMATION && present)
    {
        new_sd.control |= SE_SACL_PRESENT;
        new_sd.sacl_len = sd->sacl_len;
    }
    else if (set_info & LABEL_SECURITY_INFORMATION && present)
    {
        const struct acl *old_sacl = NULL;
        if (obj->sd && obj->sd->control & SE_SACL_PRESENT) old_sacl = sd_get_sacl( obj->sd, &present );
        if (!(replaced_sacl = replace_security_labels( old_sacl, sacl ))) return 0;
        new_sd.control |= SE_SACL_PRESENT;
        new_sd.sacl_len = replaced_sacl->size;
        sacl = replaced_sacl;
    }
    else
    {
        if (obj->sd) sacl = sd_get_sacl( obj->sd, &present );

        if (obj->sd && present)
        {
            new_sd.control |= SE_SACL_PRESENT;
            new_sd.sacl_len = obj->sd->sacl_len;
        }
        else
            new_sd.sacl_len = 0;
    }

    dacl = sd_get_dacl( sd, &present );
    if (set_info & DACL_SECURITY_INFORMATION && present)
    {
        new_sd.control |= SE_DACL_PRESENT;
        new_sd.dacl_len = sd->dacl_len;
    }
    else
    {
        if (obj->sd) dacl = sd_get_dacl( obj->sd, &present );

        if (obj->sd && present)
        {
            new_sd.control |= SE_DACL_PRESENT;
            new_sd.dacl_len = obj->sd->dacl_len;
        }
        else if (token)
        {
            dacl = token_get_default_dacl( token );
            new_sd.control |= SE_DACL_PRESENT;
            new_sd.dacl_len = dacl->size;
        }
        else new_sd.dacl_len = 0;
    }

    ptr = mem_alloc( sizeof(new_sd) + new_sd.owner_len + new_sd.group_len +
                     new_sd.sacl_len + new_sd.dacl_len );
    if (!ptr)
    {
        free( replaced_sacl );
        return 0;
    }
    new_sd_ptr = (struct security_descriptor*)ptr;

    ptr = mem_append( ptr, &new_sd, sizeof(new_sd) );
    ptr = mem_append( ptr, owner, new_sd.owner_len );
    ptr = mem_append( ptr, group, new_sd.group_len );
    ptr = mem_append( ptr, sacl, new_sd.sacl_len );
    mem_append( ptr, dacl, new_sd.dacl_len );

    free( replaced_sacl );
    free( obj->sd );
    obj->sd = new_sd_ptr;
    return 1;
}

/** Set the security descriptor using the current primary token for defaults. */
int default_set_sd( struct object *obj, const struct security_descriptor *sd,
                    unsigned int set_info )
{
    return set_sd_defaults_from_token( obj, sd, set_info, current->process->token );
}

WCHAR *no_get_full_name( struct object *obj, data_size_t max, data_size_t *ret_len )
{
    return NULL;
}

struct object *no_lookup_name( struct object *obj, struct unicode_str *name,
                               unsigned int attr, struct object *root )
{
    if (!name) set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return NULL;
}

int no_link_name( struct object *obj, struct object_name *name, struct object *parent )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

void default_unlink_name( struct object *obj, struct object_name *name )
{
    list_remove( &name->entry );
}

struct object *no_open_file( struct object *obj, unsigned int access, unsigned int sharing,
                             unsigned int options )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return NULL;
}

int no_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    return 1;  /* ok to close */
}

void no_destroy( struct object *obj )
{
}

static void dump_reserve( struct object *obj, int verbose )
{
    struct reserve *reserve = (struct reserve *) obj;

    assert( obj->ops == &apc_reserve_ops || obj->ops == &completion_reserve_ops );
    fprintf( stderr, "reserve type=%d\n", reserve->type);
}

static struct reserve *create_reserve( struct object *root, const struct unicode_str *name,
                                       unsigned int attr, int type, const struct security_descriptor *sd )
{
    struct reserve *reserve;

    if (name->len)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return NULL;
    }

    if (type == MemoryReserveObjectTypeUserApc)
    {
        reserve = create_named_object( root, &apc_reserve_ops, name, attr, sd );
    }
    else if (type == MemoryReserveObjectTypeIoCompletion)
    {
        reserve = create_named_object( root, &completion_reserve_ops, name, attr, sd );
    }
    else
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if (reserve && get_error() != STATUS_OBJECT_NAME_EXISTS)
    {
        reserve->type = type;
        reserve->bound_obj = NULL;
    }

    return reserve;
}

struct reserve *get_completion_reserve_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct reserve *)get_handle_obj( process, handle, access, &completion_reserve_ops );
}

struct reserve *reserve_obj_associate_apc( struct process *process, obj_handle_t handle, struct object *apc )
{
    struct reserve *reserve;

    if (!(reserve = (struct reserve *)get_handle_obj( process, handle, 0, &apc_reserve_ops ))) return NULL;
    if (reserve->bound_obj)
    {
        release_object( reserve );
        set_error( STATUS_INVALID_PARAMETER_2 );
        return NULL;
    }
    reserve->bound_obj = apc;
    return reserve;
}

void reserve_obj_unbind( struct reserve *reserve )
{
    if (!reserve) return;
    reserve->bound_obj = NULL;
    release_object( reserve );
}

/* Allocate a reserve object for pre-allocating memory for object types */
DECL_HANDLER(allocate_reserve_object)
{
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );
    struct reserve *reserve;

    if (!objattr) return;

    if ((reserve = create_reserve( root, &name, objattr->attributes, req->type, sd )))
    {
        reply->handle = alloc_handle_no_access_check( current->process, reserve, GENERIC_READ | GENERIC_WRITE,
                                                      objattr->attributes );
        release_object( reserve );
    }

    if (root) release_object( root );
}

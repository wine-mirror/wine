/*
 * Server-side directory object management
 *
 * Copyright (C) 2005 Vitaliy Margolen
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ddk/wdm.h"

#include "handle.h"
#include "request.h"
#include "process.h"
#include "file.h"
#include "unicode.h"

#define HASH_SIZE 7  /* default hash size */

struct object_type
{
    struct object     obj;        /* object header */
};

static void object_type_dump( struct object *obj, int verbose );
static struct object_type *object_type_get_type( struct object *obj );

static const struct object_ops object_type_ops =
{
    sizeof(struct object_type),   /* size */
    object_type_dump,             /* dump */
    object_type_get_type,         /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    no_map_access,                /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    directory_link_name,          /* link_name */
    default_unlink_name,          /* unlink_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    no_destroy                    /* destroy */
};


struct directory
{
    struct object     obj;        /* object header */
    struct namespace *entries;    /* directory's name space */
};

static void directory_dump( struct object *obj, int verbose );
static struct object_type *directory_get_type( struct object *obj );
static struct object *directory_lookup_name( struct object *obj, struct unicode_str *name,
                                             unsigned int attr );
static void directory_destroy( struct object *obj );

static const struct object_ops directory_ops =
{
    sizeof(struct directory),     /* size */
    directory_dump,               /* dump */
    directory_get_type,           /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_fd_map_access,        /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    directory_lookup_name,        /* lookup_name */
    directory_link_name,          /* link_name */
    default_unlink_name,          /* unlink_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    directory_destroy             /* destroy */
};

static struct directory *root_directory;
static struct directory *dir_objtype;


static void object_type_dump( struct object *obj, int verbose )
{
    fputs( "Object type\n", stderr );
}

static struct object_type *object_type_get_type( struct object *obj )
{
    static const WCHAR name[] = {'O','b','j','e','c','t','T','y','p','e'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static void directory_dump( struct object *obj, int verbose )
{
    fputs( "Directory\n", stderr );
}

static struct object_type *directory_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','i','r','e','c','t','o','r','y'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static struct object *directory_lookup_name( struct object *obj, struct unicode_str *name,
                                             unsigned int attr )
{
    struct directory *dir = (struct directory *)obj;
    struct object *found;
    struct unicode_str tmp;
    const WCHAR *p;

    assert( obj->ops == &directory_ops );

    if (!name) return NULL;  /* open the directory itself */

    if (!(p = memchrW( name->str, '\\', name->len / sizeof(WCHAR) )))
        /* Last element in the path name */
        tmp.len = name->len;
    else
        tmp.len = (p - name->str) * sizeof(WCHAR);

    tmp.str = name->str;
    if ((found = find_object( dir->entries, &tmp, attr )))
    {
        /* Skip trailing \\ */
        if (p)
        {
            p++;
            tmp.len += sizeof(WCHAR);
        }
        /* Move to the next element*/
        name->str = p;
        name->len -= tmp.len;
        return found;
    }

    if (name->str)  /* not the last element */
    {
        if (tmp.len == 0) /* Double backslash */
            set_error( STATUS_OBJECT_NAME_INVALID );
        else if (p)  /* Path still has backslashes */
            set_error( STATUS_OBJECT_PATH_NOT_FOUND );
    }
    return NULL;
}

int directory_link_name( struct object *obj, struct object_name *name, struct object *parent )
{
    struct directory *dir = (struct directory *)parent;

    if (parent->ops != &directory_ops)
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return 0;
    }
    namespace_add( dir->entries, name );
    name->parent = grab_object( parent );
    return 1;
}

static void directory_destroy( struct object *obj )
{
    struct directory *dir = (struct directory *)obj;
    assert( obj->ops == &directory_ops );
    free( dir->entries );
}

static struct directory *create_directory( struct object *root, const struct unicode_str *name,
                                           unsigned int attr, unsigned int hash_size,
                                           const struct security_descriptor *sd )
{
    struct directory *dir;

    if ((dir = create_named_object( root, &directory_ops, name, attr, sd )) &&
        get_error() != STATUS_OBJECT_NAME_EXISTS)
    {
        if (!(dir->entries = create_namespace( hash_size )))
        {
            release_object( dir );
            return NULL;
        }
    }
    return dir;
}

struct object *get_root_directory(void)
{
    return grab_object( root_directory );
}

/* return a directory object for creating/opening some object; no access rights are required */
struct object *get_directory_obj( struct process *process, obj_handle_t handle )
{
    return get_handle_obj( process, handle, 0, &directory_ops );
}

/* retrieve an object type, creating it if needed */
struct object_type *get_object_type( const struct unicode_str *name )
{
    struct object_type *type;

    if ((type = create_named_object( &dir_objtype->obj, &object_type_ops, name, OBJ_OPENIF, NULL )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            grab_object( type );
            make_object_static( &type->obj );
        }
        clear_error();
    }
    return type;
}

/* Global initialization */

static void create_session( unsigned int id )
{
    /* directories */
    static const WCHAR dir_sessionsW[] = {'S','e','s','s','i','o','n','s'};
    static const WCHAR dir_bnolinksW[] = {'B','N','O','L','I','N','K','S'};
    static const WCHAR dir_bnoW[] = {'B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s'};
    static const WCHAR dir_dosdevicesW[] = {'D','o','s','D','e','v','i','c','e','s'};
    static const WCHAR dir_windowsW[] = {'W','i','n','d','o','w','s'};
    static const WCHAR dir_winstationsW[] = {'W','i','n','d','o','w','S','t','a','t','i','o','n','s'};
    static const struct unicode_str dir_sessions_str = {dir_sessionsW, sizeof(dir_sessionsW)};
    static const struct unicode_str dir_bnolinks_str = {dir_bnolinksW, sizeof(dir_bnolinksW)};
    static const struct unicode_str dir_bno_str = {dir_bnoW, sizeof(dir_bnoW)};
    static const struct unicode_str dir_dosdevices_str = {dir_dosdevicesW, sizeof(dir_dosdevicesW)};
    static const struct unicode_str dir_windows_str = {dir_windowsW, sizeof(dir_windowsW)};
    static const struct unicode_str dir_winstations_str = {dir_winstationsW, sizeof(dir_winstationsW)};

    /* symlinks */
    static const WCHAR link_globalW[] = {'G','l','o','b','a','l'};
    static const WCHAR link_localW[]  = {'L','o','c','a','l'};
    static const WCHAR link_sessionW[] = {'S','e','s','s','i','o','n'};
    static const struct unicode_str link_global_str = {link_globalW, sizeof(link_globalW)};
    static const struct unicode_str link_local_str = {link_localW, sizeof(link_localW)};
    static const struct unicode_str link_session_str = {link_sessionW, sizeof(link_sessionW)};

    static const WCHAR fmt_u[] = {'%','u',0};
    static struct directory *dir_bno_global, *dir_sessions, *dir_bnolinks;
    struct directory *dir_id, *dir_bno, *dir_dosdevices, *dir_windows, *dir_winstation;
    struct object *link_global, *link_local, *link_session, *link_bno, *link_windows;
    struct unicode_str id_str;
    WCHAR id_strW[10];

    if (!id)
    {
        dir_bno_global = create_directory( &root_directory->obj, &dir_bno_str, 0, HASH_SIZE, NULL );
        dir_sessions   = create_directory( &root_directory->obj, &dir_sessions_str, 0, HASH_SIZE, NULL );
        dir_bnolinks   = create_directory( &dir_sessions->obj, &dir_bnolinks_str, 0, HASH_SIZE, NULL );
        make_object_static( (struct object *)dir_bno_global );
        make_object_static( (struct object *)dir_bnolinks );
        make_object_static( (struct object *)dir_sessions );
    }

    sprintfW( id_strW, fmt_u, id );
    id_str.str = id_strW;
    id_str.len = strlenW( id_strW ) * sizeof(WCHAR);
    dir_id = create_directory( &dir_sessions->obj, &id_str, 0, HASH_SIZE, NULL );
    dir_dosdevices = create_directory( &dir_id->obj, &dir_dosdevices_str, 0, HASH_SIZE, NULL );

    /* for session 0, directories are created under the root */
    if (!id)
    {
        dir_bno      = (struct directory *)grab_object( dir_bno_global );
        dir_windows  = create_directory( &root_directory->obj, &dir_windows_str, 0, HASH_SIZE, NULL );
        link_bno     = create_obj_symlink( &dir_id->obj, &dir_bno_str, 0, &dir_bno->obj, NULL );
        link_windows = create_obj_symlink( &dir_id->obj, &dir_windows_str, 0, &dir_windows->obj, NULL );
        make_object_static( link_bno );
        make_object_static( link_windows );
    }
    else
    {
        /* use a larger hash table for this one since it can contain a lot of objects */
        dir_bno     = create_directory( &dir_id->obj, &dir_bno_str, 0, 37, NULL );
        dir_windows = create_directory( &dir_id->obj, &dir_windows_str, 0, HASH_SIZE, NULL );
    }
    dir_winstation = create_directory( &dir_windows->obj, &dir_winstations_str, 0, HASH_SIZE, NULL );

    link_global  = create_obj_symlink( &dir_bno->obj, &link_global_str, 0, &dir_bno_global->obj, NULL );
    link_local   = create_obj_symlink( &dir_bno->obj, &link_local_str, 0, &dir_bno->obj, NULL );
    link_session = create_obj_symlink( &dir_bno->obj, &link_session_str, 0, &dir_bnolinks->obj, NULL );
    link_bno     = create_obj_symlink( &dir_bnolinks->obj, &id_str, 0, &dir_bno->obj, NULL );
    make_object_static( link_global );
    make_object_static( link_local );
    make_object_static( link_session );
    make_object_static( link_bno );

    make_object_static( &dir_dosdevices->obj );
    make_object_static( &dir_winstation->obj );
    release_object( dir_windows );
    release_object( dir_bno );
    release_object( dir_id );
}

void init_directories(void)
{
    /* Directories */
    static const WCHAR dir_globalW[] = {'?','?'};
    static const WCHAR dir_driverW[] = {'D','r','i','v','e','r'};
    static const WCHAR dir_deviceW[] = {'D','e','v','i','c','e'};
    static const WCHAR dir_objtypeW[] = {'O','b','j','e','c','t','T','y','p','e','s'};
    static const WCHAR dir_kernelW[] = {'K','e','r','n','e','l','O','b','j','e','c','t','s'};
    static const struct unicode_str dir_global_str = {dir_globalW, sizeof(dir_globalW)};
    static const struct unicode_str dir_driver_str = {dir_driverW, sizeof(dir_driverW)};
    static const struct unicode_str dir_device_str = {dir_deviceW, sizeof(dir_deviceW)};
    static const struct unicode_str dir_objtype_str = {dir_objtypeW, sizeof(dir_objtypeW)};
    static const struct unicode_str dir_kernel_str = {dir_kernelW, sizeof(dir_kernelW)};

    /* symlinks */
    static const WCHAR link_dosdevW[] = {'D','o','s','D','e','v','i','c','e','s'};
    static const WCHAR link_globalW[] = {'G','l','o','b','a','l'};
    static const WCHAR link_nulW[]    = {'N','U','L'};
    static const WCHAR link_pipeW[]   = {'P','I','P','E'};
    static const WCHAR link_mailslotW[] = {'M','A','I','L','S','L','O','T'};
    static const struct unicode_str link_dosdev_str = {link_dosdevW, sizeof(link_dosdevW)};
    static const struct unicode_str link_global_str = {link_globalW, sizeof(link_globalW)};
    static const struct unicode_str link_nul_str    = {link_nulW, sizeof(link_nulW)};
    static const struct unicode_str link_pipe_str   = {link_pipeW, sizeof(link_pipeW)};
    static const struct unicode_str link_mailslot_str = {link_mailslotW, sizeof(link_mailslotW)};

    /* devices */
    static const WCHAR named_pipeW[] = {'N','a','m','e','d','P','i','p','e'};
    static const WCHAR mailslotW[] = {'M','a','i','l','S','l','o','t'};
    static const WCHAR nullW[] = {'N','u','l','l'};
    static const struct unicode_str named_pipe_str = {named_pipeW, sizeof(named_pipeW)};
    static const struct unicode_str mailslot_str = {mailslotW, sizeof(mailslotW)};
    static const struct unicode_str null_str = {nullW, sizeof(nullW)};

    /* events */
    static const WCHAR event_low_memW[] = {'L','o','w','M','e','m','o','r','y','C','o','n','d','i','t','i','o','n'};
    static const WCHAR event_low_pagedW[] = {'L','o','w','P','a','g','e','d','P','o','o','l','C','o','n','d','i','t','i','o','n'};
    static const WCHAR event_low_nonpgW[] = {'L','o','w','N','o','n','P','a','g','e','d','P','o','o','l','C','o','n','d','i','t','i','o','n'};
    static const WCHAR event_high_memW[] = {'H','i','g','h','M','e','m','o','r','y','C','o','n','d','i','t','i','o','n'};
    static const WCHAR event_high_pagedW[] = {'H','i','g','h','P','a','g','e','d','P','o','o','l','C','o','n','d','i','t','i','o','n'};
    static const WCHAR event_high_nonpgW[] = {'H','i','g','h','N','o','n','P','a','g','e','d','P','o','o','l','C','o','n','d','i','t','i','o','n'};
    static const WCHAR keyed_event_crit_sectW[] = {'C','r','i','t','S','e','c','O','u','t','O','f','M','e','m','o','r','y','E','v','e','n','t'};
    static const struct unicode_str kernel_events[] =
    {
        { event_low_memW, sizeof(event_low_memW) },
        { event_low_pagedW, sizeof(event_low_pagedW) },
        { event_low_nonpgW, sizeof(event_low_nonpgW) },
        { event_high_memW, sizeof(event_high_memW) },
        { event_high_pagedW, sizeof(event_high_pagedW) },
        { event_high_nonpgW, sizeof(event_high_nonpgW) }
    };
    static const struct unicode_str keyed_event_crit_sect_str = {keyed_event_crit_sectW, sizeof(keyed_event_crit_sectW)};

    struct directory *dir_driver, *dir_device, *dir_global, *dir_kernel;
    struct object *link_dosdev, *link_global, *link_nul, *link_pipe, *link_mailslot;
    struct object *named_pipe_device, *mailslot_device, *null_device;
    struct keyed_event *keyed_event;
    unsigned int i;

    root_directory = create_directory( NULL, NULL, 0, HASH_SIZE, NULL );
    dir_driver     = create_directory( &root_directory->obj, &dir_driver_str, 0, HASH_SIZE, NULL );
    dir_device     = create_directory( &root_directory->obj, &dir_device_str, 0, HASH_SIZE, NULL );
    dir_objtype    = create_directory( &root_directory->obj, &dir_objtype_str, 0, HASH_SIZE, NULL );
    dir_kernel     = create_directory( &root_directory->obj, &dir_kernel_str, 0, HASH_SIZE, NULL );
    dir_global     = create_directory( &root_directory->obj, &dir_global_str, 0, HASH_SIZE, NULL );
    make_object_static( &root_directory->obj );
    make_object_static( &dir_driver->obj );
    make_object_static( &dir_objtype->obj );

    /* devices */
    named_pipe_device = create_named_pipe_device( &dir_device->obj, &named_pipe_str );
    mailslot_device   = create_mailslot_device( &dir_device->obj, &mailslot_str );
    null_device       = create_unix_device( &dir_device->obj, &null_str, "/dev/null" );
    make_object_static( named_pipe_device );
    make_object_static( mailslot_device );
    make_object_static( null_device );

    /* sessions */
    create_session( 0 );
    create_session( 1 );

    /* symlinks */
    link_dosdev    = create_obj_symlink( &root_directory->obj, &link_dosdev_str, 0, &dir_global->obj, NULL );
    link_global    = create_obj_symlink( &dir_global->obj, &link_global_str, 0, &dir_global->obj, NULL );
    link_nul       = create_obj_symlink( &dir_global->obj, &link_nul_str, 0, null_device, NULL );
    link_pipe      = create_obj_symlink( &dir_global->obj, &link_pipe_str, 0, named_pipe_device, NULL );
    link_mailslot  = create_obj_symlink( &dir_global->obj, &link_mailslot_str, 0, mailslot_device, NULL );
    make_object_static( link_dosdev );
    make_object_static( link_global );
    make_object_static( link_nul );
    make_object_static( link_pipe );
    make_object_static( link_mailslot );

    /* events */
    for (i = 0; i < sizeof(kernel_events)/sizeof(kernel_events[0]); i++)
    {
        struct event *event = create_event( &dir_kernel->obj, &kernel_events[i], 0, 1, 0, NULL );
        make_object_static( (struct object *)event );
    }
    keyed_event = create_keyed_event( &dir_kernel->obj, &keyed_event_crit_sect_str, 0, NULL );
    make_object_static( (struct object *)keyed_event );

    /* the objects hold references so we can release these directories */
    release_object( dir_global );
    release_object( dir_device );
    release_object( dir_kernel );
}

/* create a directory object */
DECL_HANDLER(create_directory)
{
    struct unicode_str name;
    struct object *root;
    struct directory *dir;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((dir = create_directory( root, &name, objattr->attributes, HASH_SIZE, sd )))
    {
        reply->handle = alloc_handle( current->process, dir, req->access, objattr->attributes );
        release_object( dir );
    }

    if (root) release_object( root );
}

/* open a directory object */
DECL_HANDLER(open_directory)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &directory_ops, &name, req->attributes );
}

/* get a directory entry by index */
DECL_HANDLER(get_directory_entry)
{
    struct directory *dir = (struct directory *)get_handle_obj( current->process, req->handle,
                                                                DIRECTORY_QUERY, &directory_ops );
    if (dir)
    {
        struct object *obj = find_object_index( dir->entries, req->index );
        if (obj)
        {
            data_size_t name_len, type_len = 0;
            const WCHAR *type_name = NULL;
            const WCHAR *name = get_object_name( obj, &name_len );
            struct object_type *type = obj->ops->get_type( obj );

            if (type) type_name = get_object_name( &type->obj, &type_len );

            if (name_len + type_len <= get_reply_max_size())
            {
                void *ptr = set_reply_data_size( name_len + type_len );
                if (ptr)
                {
                    reply->name_len = name_len;
                    memcpy( ptr, name, name_len );
                    memcpy( (char *)ptr + name_len, type_name, type_len );
                }
            }
            else set_error( STATUS_BUFFER_OVERFLOW );

            if (type) release_object( type );
            release_object( obj );
        }
        release_object( dir );
    }
}

/* unlink a named object */
DECL_HANDLER(unlink_object)
{
    struct object *obj = get_handle_obj( current->process, req->handle, 0, NULL );

    if (obj)
    {
        unlink_named_object( obj );
        release_object( obj );
    }
}

/* query object type name information */
DECL_HANDLER(get_object_type)
{
    struct object *obj;
    struct object_type *type;
    const WCHAR *name;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL ))) return;

    if ((type = obj->ops->get_type( obj )))
    {
        if ((name = get_object_name( &type->obj, &reply->total )))
            set_reply_data( name, min( reply->total, get_reply_max_size() ) );
        release_object( type );
    }
    release_object( obj );
}

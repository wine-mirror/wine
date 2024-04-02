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

static const WCHAR objtype_name[] = {'T','y','p','e'};

struct type_descr objtype_type =
{
    { objtype_name, sizeof(objtype_name) },        /* name */
    STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1,  /* valid_access */
    {                                              /* mapping */
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_REQUIRED | 0x1
    },
};

struct object_type
{
    struct object     obj;        /* object header */
};

static void object_type_dump( struct object *obj, int verbose );

static const struct object_ops object_type_ops =
{
    sizeof(struct object_type),   /* size */
    &objtype_type,                /* type */
    object_type_dump,             /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* get_esync_fd */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    default_get_full_name,        /* get_full_name */
    no_lookup_name,               /* lookup_name */
    directory_link_name,          /* link_name */
    default_unlink_name,          /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    no_close_handle,              /* close_handle */
    no_destroy                    /* destroy */
};


static const WCHAR directory_name[] = {'D','i','r','e','c','t','o','r','y'};

struct type_descr directory_type =
{
    { directory_name, sizeof(directory_name) },   /* name */
    DIRECTORY_ALL_ACCESS,                         /* valid_access */
    {                                             /* mapping */
        STANDARD_RIGHTS_READ | DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
        STANDARD_RIGHTS_WRITE | DIRECTORY_CREATE_SUBDIRECTORY | DIRECTORY_CREATE_OBJECT,
        STANDARD_RIGHTS_EXECUTE | DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
        DIRECTORY_ALL_ACCESS
    },
};

struct directory
{
    struct object     obj;        /* object header */
    struct namespace *entries;    /* directory's name space */
};

static void directory_dump( struct object *obj, int verbose );
static struct object *directory_lookup_name( struct object *obj, struct unicode_str *name,
                                             unsigned int attr, struct object *root );
static void directory_destroy( struct object *obj );

static const struct object_ops directory_ops =
{
    sizeof(struct directory),     /* size */
    &directory_type,              /* type */
    directory_dump,               /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* get_esync_fd */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    default_get_full_name,        /* get_full_name */
    directory_lookup_name,        /* lookup_name */
    directory_link_name,          /* link_name */
    default_unlink_name,          /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    no_close_handle,              /* close_handle */
    directory_destroy             /* destroy */
};

static struct directory *root_directory;
static struct directory *dir_objtype;


static struct type_descr *types[] =
{
    &objtype_type,
    &directory_type,
    &symlink_type,
    &token_type,
    &job_type,
    &process_type,
    &thread_type,
    &debug_obj_type,
    &event_type,
    &mutex_type,
    &semaphore_type,
    &timer_type,
    &keyed_event_type,
    &winstation_type,
    &desktop_type,
    &device_type,
    &completion_type,
    &file_type,
    &mapping_type,
    &key_type,
};

static void object_type_dump( struct object *obj, int verbose )
{
    fputs( "Object type\n", stderr );
}

static struct object_type *create_object_type( struct object *root, unsigned int index,
                                               unsigned int attr, const struct security_descriptor *sd )
{
    struct type_descr *descr = types[index];
    struct object_type *type;

    if ((type = create_named_object( root, &object_type_ops, &descr->name, attr, sd )))
    {
        descr->index = index;
    }
    return type;
}

static void directory_dump( struct object *obj, int verbose )
{
    fputs( "Directory\n", stderr );
}

static struct object *directory_lookup_name( struct object *obj, struct unicode_str *name,
                                             unsigned int attr, struct object *root )
{
    struct directory *dir = (struct directory *)obj;
    struct object *found;
    struct unicode_str tmp;

    assert( obj->ops == &directory_ops );

    if (!name) return NULL;  /* open the directory itself */

    tmp.str = name->str;
    tmp.len = get_path_element( name->str, name->len );

    if ((found = find_object( dir->entries, &tmp, attr )))
    {
        /* Skip trailing \\ and move to the next element */
        if (tmp.len < name->len)
        {
            tmp.len += sizeof(WCHAR);
            name->str += tmp.len / sizeof(WCHAR);
            name->len -= tmp.len;
        }
        else
        {
            name->str = NULL;
            name->len = 0;
        }
        return found;
    }

    if (name->str)  /* not the last element */
    {
        if (tmp.len == 0) /* Double backslash */
            set_error( STATUS_OBJECT_NAME_INVALID );
        else if (tmp.len < name->len)  /* Path still has backslashes */
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

    static struct directory *dir_bno_global, *dir_sessions, *dir_bnolinks;
    struct directory *dir_id, *dir_bno, *dir_dosdevices, *dir_windows, *dir_winstation;
    struct object *link_global, *link_local, *link_session, *link_bno, *link_windows;
    struct unicode_str id_str;
    char id_strA[10];
    WCHAR *id_strW;

    if (!id)
    {
        dir_bno_global = create_directory( &root_directory->obj, &dir_bno_str, OBJ_PERMANENT, HASH_SIZE, NULL );
        dir_sessions   = create_directory( &root_directory->obj, &dir_sessions_str, OBJ_PERMANENT, HASH_SIZE, NULL );
        dir_bnolinks   = create_directory( &dir_sessions->obj, &dir_bnolinks_str, OBJ_PERMANENT, HASH_SIZE, NULL );
        release_object( dir_bno_global );
        release_object( dir_bnolinks );
        release_object( dir_sessions );
    }

    sprintf( id_strA, "%u", id );
    id_strW = ascii_to_unicode_str( id_strA, &id_str );
    dir_id = create_directory( &dir_sessions->obj, &id_str, 0, HASH_SIZE, NULL );
    dir_dosdevices = create_directory( &dir_id->obj, &dir_dosdevices_str, OBJ_PERMANENT, HASH_SIZE, NULL );

    /* for session 0, directories are created under the root */
    if (!id)
    {
        dir_bno      = (struct directory *)grab_object( dir_bno_global );
        dir_windows  = create_directory( &root_directory->obj, &dir_windows_str, 0, HASH_SIZE, NULL );
        link_bno     = create_obj_symlink( &dir_id->obj, &dir_bno_str, OBJ_PERMANENT, &dir_bno->obj, NULL );
        link_windows = create_obj_symlink( &dir_id->obj, &dir_windows_str, OBJ_PERMANENT, &dir_windows->obj, NULL );
        release_object( link_bno );
        release_object( link_windows );
    }
    else
    {
        /* use a larger hash table for this one since it can contain a lot of objects */
        dir_bno     = create_directory( &dir_id->obj, &dir_bno_str, 0, 37, NULL );
        dir_windows = create_directory( &dir_id->obj, &dir_windows_str, 0, HASH_SIZE, NULL );
    }
    dir_winstation = create_directory( &dir_windows->obj, &dir_winstations_str, OBJ_PERMANENT, HASH_SIZE, NULL );

    link_global  = create_obj_symlink( &dir_bno->obj, &link_global_str, OBJ_PERMANENT, &dir_bno_global->obj, NULL );
    link_local   = create_obj_symlink( &dir_bno->obj, &link_local_str, OBJ_PERMANENT, &dir_bno->obj, NULL );
    link_session = create_obj_symlink( &dir_bno->obj, &link_session_str, OBJ_PERMANENT, &dir_bnolinks->obj, NULL );
    link_bno     = create_obj_symlink( &dir_bnolinks->obj, &id_str, OBJ_PERMANENT, &dir_bno->obj, NULL );
    release_object( link_global );
    release_object( link_local );
    release_object( link_session );
    release_object( link_bno );

    release_object( dir_dosdevices );
    release_object( dir_winstation );
    release_object( dir_windows );
    release_object( dir_bno );
    release_object( dir_id );
    free( id_strW );
}

void init_directories( struct fd *intl_fd )
{
    /* Directories */
    static const WCHAR dir_globalW[] = {'?','?'};
    static const WCHAR dir_driverW[] = {'D','r','i','v','e','r'};
    static const WCHAR dir_deviceW[] = {'D','e','v','i','c','e'};
    static const WCHAR dir_objtypeW[] = {'O','b','j','e','c','t','T','y','p','e','s'};
    static const WCHAR dir_kernelW[] = {'K','e','r','n','e','l','O','b','j','e','c','t','s'};
    static const WCHAR dir_nlsW[] = {'N','L','S'};
    static const struct unicode_str dir_global_str = {dir_globalW, sizeof(dir_globalW)};
    static const struct unicode_str dir_driver_str = {dir_driverW, sizeof(dir_driverW)};
    static const struct unicode_str dir_device_str = {dir_deviceW, sizeof(dir_deviceW)};
    static const struct unicode_str dir_objtype_str = {dir_objtypeW, sizeof(dir_objtypeW)};
    static const struct unicode_str dir_kernel_str = {dir_kernelW, sizeof(dir_kernelW)};
    static const struct unicode_str dir_nls_str = {dir_nlsW, sizeof(dir_nlsW)};

    /* symlinks */
    static const WCHAR link_dosdevW[] = {'D','o','s','D','e','v','i','c','e','s'};
    static const WCHAR link_globalrootW[] = {'G','L','O','B','A','L','R','O','O','T'};
    static const WCHAR link_globalW[] = {'G','l','o','b','a','l'};
    static const WCHAR link_nulW[]    = {'N','U','L'};
    static const WCHAR link_pipeW[]   = {'P','I','P','E'};
    static const WCHAR link_mailslotW[] = {'M','A','I','L','S','L','O','T'};
    static const WCHAR link_coninW[]  = {'C','O','N','I','N','$'};
    static const WCHAR link_conoutW[] = {'C','O','N','O','U','T','$'};
    static const WCHAR link_conW[]    = {'C','O','N'};
    static const WCHAR link_currentinW[]  = {'\\','D','e','v','i','c','e','\\','C','o','n','D','r','v',
        '\\','C','u','r','r','e','n','t','I','n'};
    static const WCHAR link_currentoutW[] = {'\\','D','e','v','i','c','e','\\','C','o','n','D','r','v',
        '\\','C','u','r','r','e','n','t','O','u','t'};
    static const WCHAR link_consoleW[]    = {'\\','D','e','v','i','c','e','\\','C','o','n','D','r','v',
        '\\','C','o','n','s','o','l','e'};
    static const struct unicode_str link_dosdev_str = {link_dosdevW, sizeof(link_dosdevW)};
    static const struct unicode_str link_globalroot_str = {link_globalrootW, sizeof(link_globalrootW)};
    static const struct unicode_str link_global_str = {link_globalW, sizeof(link_globalW)};
    static const struct unicode_str link_nul_str    = {link_nulW, sizeof(link_nulW)};
    static const struct unicode_str link_pipe_str   = {link_pipeW, sizeof(link_pipeW)};
    static const struct unicode_str link_mailslot_str = {link_mailslotW, sizeof(link_mailslotW)};
    static const struct unicode_str link_con_str      = {link_conW, sizeof(link_conW)};
    static const struct unicode_str link_conin_str    = {link_coninW, sizeof(link_coninW)};
    static const struct unicode_str link_conout_str   = {link_conoutW, sizeof(link_conoutW)};
    static const struct unicode_str link_currentin_str = {link_currentinW, sizeof(link_currentinW)};
    static const struct unicode_str link_currentout_str = {link_currentoutW, sizeof(link_currentoutW)};
    static const struct unicode_str link_console_str  = {link_consoleW, sizeof(link_consoleW)};

    /* devices */
    static const WCHAR named_pipeW[] = {'N','a','m','e','d','P','i','p','e'};
    static const WCHAR mailslotW[] = {'M','a','i','l','S','l','o','t'};
    static const WCHAR condrvW[] = {'C','o','n','D','r','v'};
    static const WCHAR nullW[] = {'N','u','l','l'};
    static const WCHAR afdW[] = {'A','f','d'};
    static const struct unicode_str named_pipe_str = {named_pipeW, sizeof(named_pipeW)};
    static const struct unicode_str mailslot_str = {mailslotW, sizeof(mailslotW)};
    static const struct unicode_str condrv_str = {condrvW, sizeof(condrvW)};
    static const struct unicode_str null_str = {nullW, sizeof(nullW)};
    static const struct unicode_str afd_str = {afdW, sizeof(afdW)};

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

    /* mappings */
    static const WCHAR intlW[] = {'N','l','s','S','e','c','t','i','o','n','L','A','N','G','_','I','N','T','L'};
    static const WCHAR user_dataW[] = {'_','_','w','i','n','e','_','u','s','e','r','_','s','h','a','r','e','d','_','d','a','t','a'};
    static const struct unicode_str intl_str = {intlW, sizeof(intlW)};
    static const struct unicode_str user_data_str = {user_dataW, sizeof(user_dataW)};

    struct directory *dir_driver, *dir_device, *dir_global, *dir_kernel, *dir_nls;
    struct object *named_pipe_device, *mailslot_device, *null_device;
    unsigned int i;

    root_directory = create_directory( NULL, NULL, OBJ_PERMANENT, HASH_SIZE, NULL );
    dir_driver     = create_directory( &root_directory->obj, &dir_driver_str, OBJ_PERMANENT, HASH_SIZE, NULL );
    dir_device     = create_directory( &root_directory->obj, &dir_device_str, OBJ_PERMANENT, HASH_SIZE, NULL );
    dir_objtype    = create_directory( &root_directory->obj, &dir_objtype_str, OBJ_PERMANENT, HASH_SIZE, NULL );
    dir_kernel     = create_directory( &root_directory->obj, &dir_kernel_str, OBJ_PERMANENT, HASH_SIZE, NULL );
    dir_global     = create_directory( &root_directory->obj, &dir_global_str, OBJ_PERMANENT, HASH_SIZE, NULL );
    dir_nls        = create_directory( &root_directory->obj, &dir_nls_str, OBJ_PERMANENT, HASH_SIZE, NULL );

    /* devices */
    named_pipe_device = create_named_pipe_device( &dir_device->obj, &named_pipe_str, OBJ_PERMANENT, NULL );
    mailslot_device   = create_mailslot_device( &dir_device->obj, &mailslot_str, OBJ_PERMANENT, NULL );
    null_device       = create_unix_device( &dir_device->obj, &null_str, OBJ_PERMANENT, NULL, "/dev/null" );
    release_object( create_console_device( &dir_device->obj, &condrv_str, OBJ_PERMANENT, NULL ));
    release_object( create_socket_device( &dir_device->obj, &afd_str, OBJ_PERMANENT, NULL ));

    /* sessions */
    create_session( 0 );
    create_session( default_session_id );

    /* object types */

    for (i = 0; i < ARRAY_SIZE(types); i++)
        release_object( create_object_type( &dir_objtype->obj, i, OBJ_PERMANENT, NULL ));

    /* symlinks */
    release_object( create_obj_symlink( &root_directory->obj, &link_dosdev_str, OBJ_PERMANENT, &dir_global->obj, NULL ));
    release_object( create_root_symlink( &dir_global->obj, &link_globalroot_str, OBJ_PERMANENT, NULL ));
    release_object( create_obj_symlink( &dir_global->obj, &link_global_str, OBJ_PERMANENT, &dir_global->obj, NULL ));
    release_object( create_obj_symlink( &dir_global->obj, &link_nul_str, OBJ_PERMANENT, null_device, NULL ));
    release_object( create_obj_symlink( &dir_global->obj, &link_pipe_str, OBJ_PERMANENT, named_pipe_device, NULL ));
    release_object( create_obj_symlink( &dir_global->obj, &link_mailslot_str, OBJ_PERMANENT, mailslot_device, NULL ));
    release_object( create_symlink( &dir_global->obj, &link_conin_str, OBJ_PERMANENT, &link_currentin_str, NULL ));
    release_object( create_symlink( &dir_global->obj, &link_conout_str, OBJ_PERMANENT, &link_currentout_str, NULL ));
    release_object( create_symlink( &dir_global->obj, &link_con_str, OBJ_PERMANENT, &link_console_str, NULL ));

    /* events */
    for (i = 0; i < ARRAY_SIZE( kernel_events ); i++)
        release_object( create_event( &dir_kernel->obj, &kernel_events[i], OBJ_PERMANENT, 1, 0, NULL ));
    release_object( create_keyed_event( &dir_kernel->obj, &keyed_event_crit_sect_str, OBJ_PERMANENT, NULL ));

    /* mappings */
    release_object( create_fd_mapping( &dir_nls->obj, &intl_str, intl_fd, OBJ_PERMANENT, NULL ));
    release_object( create_user_data_mapping( &dir_kernel->obj, &user_data_str, OBJ_PERMANENT, NULL ));
    release_object( intl_fd );

    release_object( named_pipe_device );
    release_object( mailslot_device );
    release_object( null_device );
    release_object( root_directory );
    release_object( dir_driver );
    release_object( dir_device );
    release_object( dir_objtype );
    release_object( dir_kernel );
    release_object( dir_nls );
    release_object( dir_global );
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
            data_size_t name_len;
            const struct unicode_str *type_name = &obj->ops->type->name;
            const WCHAR *name = get_object_name( obj, &name_len );

            reply->total_len = name_len + type_name->len;

            if (reply->total_len <= get_reply_max_size())
            {
                void *ptr = set_reply_data_size( reply->total_len );
                if (ptr)
                {
                    reply->name_len = name_len;
                    memcpy( ptr, name, name_len );
                    memcpy( (char *)ptr + name_len, type_name->str, type_name->len );
                }
            }
            else set_error( STATUS_BUFFER_TOO_SMALL );

            release_object( obj );
        }
        release_object( dir );
    }
}

/* query object type name information */
DECL_HANDLER(get_object_type)
{
    struct object *obj;
    struct type_descr *type;
    struct object_type_info *info;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL ))) return;

    type = obj->ops->type;
    if (sizeof(*info) + type->name.len <= get_reply_max_size())
    {
        if ((info = set_reply_data_size( sizeof(*info) + type->name.len )))
        {
            info->name_len     = type->name.len;
            info->index        = type->index;
            info->obj_count    = type->obj_count;
            info->handle_count = type->handle_count;
            info->obj_max      = type->obj_max;
            info->handle_max   = type->handle_max;
            info->valid_access = type->valid_access;
            info->mapping      = type->mapping;
            memcpy( info + 1, type->name.str, type->name.len );
        }
    }
    else set_error( STATUS_BUFFER_OVERFLOW );

    release_object( obj );
}

/* query type information for all types */
DECL_HANDLER(get_object_types)
{
    struct object_type_info *info;
    data_size_t size = ARRAY_SIZE(types) * sizeof(*info);
    unsigned int i;
    char *next;

    for (i = 0; i < ARRAY_SIZE(types); i++) size += (types[i]->name.len + 3) & ~3;

    if (size <= get_reply_max_size())
    {
        if ((info = set_reply_data_size( size )))
        {
            for (i = 0; i < ARRAY_SIZE(types); i++)
            {
                info->name_len     = types[i]->name.len;
                info->index        = types[i]->index;
                info->obj_count    = types[i]->obj_count;
                info->handle_count = types[i]->handle_count;
                info->obj_max      = types[i]->obj_max;
                info->handle_max   = types[i]->handle_max;
                info->valid_access = types[i]->valid_access;
                info->mapping      = types[i]->mapping;
                memcpy( info + 1, types[i]->name.str, types[i]->name.len );
                next = (char *)(info + 1) + types[i]->name.len;
                if (types[i]->name.len & 3)
                {
                    memset( next, 0, 4 - (types[i]->name.len & 3) );
                    next += 4 - (types[i]->name.len & 3);
                }
                info = (struct object_type_info *)next;
            }
            reply->count = i;
        }
    }
    else set_error( STATUS_BUFFER_OVERFLOW );
}

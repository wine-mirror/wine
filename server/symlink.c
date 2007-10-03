/*
 * Server-side symbolic link object management
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
#include "object.h"
#include "unicode.h"

struct symlink
{
    struct object    obj;       /* object header */
    WCHAR           *target;    /* target of the symlink */
    data_size_t      len;       /* target len in bytes */
};

static void symlink_dump( struct object *obj, int verbose );
static unsigned int symlink_map_access( struct object *obj, unsigned int access );
static struct object *symlink_lookup_name( struct object *obj, struct unicode_str *name,
                                           unsigned int attr );
static void symlink_destroy( struct object *obj );

static const struct object_ops symlink_ops =
{
    sizeof(struct symlink),       /* size */
    symlink_dump,                 /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    symlink_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    symlink_lookup_name,          /* lookup_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    symlink_destroy               /* destroy */
};

static void symlink_dump( struct object *obj, int verbose )
{
    struct symlink *symlink = (struct symlink *)obj;
    assert( obj->ops == &symlink_ops );

    fprintf( stderr, "Symlink " );
    dump_object_name( obj );
    fprintf( stderr, " -> L\"" );
    dump_strW( symlink->target, symlink->len / sizeof(WCHAR), stderr, "\"\"" );
    fprintf( stderr, "\"\n" );
}

static struct object *symlink_lookup_name( struct object *obj, struct unicode_str *name,
                                           unsigned int attr )
{
    struct symlink *symlink = (struct symlink *)obj;
    struct unicode_str target_str, name_left;
    struct object *target;

    assert( obj->ops == &symlink_ops );
    if (attr & OBJ_OPENLINK) return NULL;

    target_str.str = symlink->target;
    target_str.len = symlink->len;
    if ((target = find_object_dir( NULL, &target_str, attr, &name_left )))
    {
        if (name_left.len)
        {
            release_object( target );
            target = NULL;
            set_error( STATUS_OBJECT_PATH_NOT_FOUND );
        }
    }
    return target;
}

static unsigned int symlink_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYMBOLIC_LINK_QUERY;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= SYMBOLIC_LINK_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void symlink_destroy( struct object *obj )
{
    struct symlink *symlink = (struct symlink *)obj;
    assert( obj->ops == &symlink_ops );
    free( symlink->target );
}

struct symlink *create_symlink( struct directory *root, const struct unicode_str *name,
                                unsigned int attr, const struct unicode_str *target )
{
    struct symlink *symlink;

    if (!target->len)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    if ((symlink = create_named_object_dir( root, name, attr, &symlink_ops )) &&
        (get_error() != STATUS_OBJECT_NAME_EXISTS))
    {
        if ((symlink->target = memdup( target->str, target->len )))
            symlink->len = target->len;
        else
        {
            release_object( symlink );
            symlink = NULL;
        }
    }
    return symlink;
}


/* create a symbolic link object */
DECL_HANDLER(create_symlink)
{
    struct symlink *symlink;
    struct unicode_str name, target;
    struct directory *root = NULL;

    if (req->name_len > get_req_data_size())
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    name.str   = get_req_data();
    target.str = name.str + req->name_len / sizeof(WCHAR);
    name.len   = (target.str - name.str) * sizeof(WCHAR);
    target.len = ((get_req_data_size() - name.len) / sizeof(WCHAR)) * sizeof(WCHAR);

    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((symlink = create_symlink( root, &name, req->attributes, &target )))
    {
        reply->handle = alloc_handle( current->process, symlink, req->access, req->attributes );
        release_object( symlink );
    }

    if (root) release_object( root );
}

/* open a symbolic link object */
DECL_HANDLER(open_symlink)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct symlink *symlink;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((symlink = open_object_dir( root, &name, req->attributes | OBJ_OPENLINK, &symlink_ops )))
    {
        reply->handle = alloc_handle( current->process, &symlink->obj, req->access, req->attributes );
        release_object( symlink );
    }

    if (root) release_object( root );
}

/* query a symbolic link object */
DECL_HANDLER(query_symlink)
{
    struct symlink *symlink;

    symlink = (struct symlink *)get_handle_obj( current->process, req->handle,
                                                SYMBOLIC_LINK_QUERY, &symlink_ops );
    if (!symlink) return;

    if (get_reply_max_size() < symlink->len)
        set_error( STATUS_BUFFER_TOO_SMALL );
    else
        set_reply_data( symlink->target, symlink->len );
    release_object( symlink );
}

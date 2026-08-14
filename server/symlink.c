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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "ntstatus.h"
#include "winternl.h"
#include "ddk/wdm.h"

#include "handle.h"
#include "request.h"
#include "object.h"
#include "unicode.h"

static const WCHAR symlink_name[] = {'S','y','m','b','o','l','i','c','L','i','n','k'};

struct type_descr symlink_type =
{
    { symlink_name, sizeof(symlink_name) },   /* name */
    SYMBOLIC_LINK_ALL_ACCESS,                 /* valid_access */
    {                                         /* mapping */
        STANDARD_RIGHTS_READ | SYMBOLIC_LINK_QUERY,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE | SYMBOLIC_LINK_QUERY,
        SYMBOLIC_LINK_ALL_ACCESS
    },
};

struct symlink
{
    struct object    obj;       /* object header */
    WCHAR           *target;    /* target of the symlink */
    data_size_t      len;       /* target len in bytes */
};

static void symlink_dump( struct object *obj, int verbose );
static bool symlink_init( struct object *obj, const void *init_params );
static struct object *symlink_lookup_name( struct object *obj, struct unicode_str *name,
                                           unsigned int attr, struct object *root );
static void symlink_destroy( struct object *obj );

static const struct object_ops symlink_ops =
{
    .size        = sizeof(struct symlink),
    .type        = &symlink_type,
    .dump        = symlink_dump,
    .init        = symlink_init,
    .lookup_name = symlink_lookup_name,
    .destroy     = symlink_destroy,
};

static void symlink_dump( struct object *obj, int verbose )
{
    struct symlink *symlink = (struct symlink *)obj;
    assert( obj->ops == &symlink_ops );

    fputs( "Symlink target=\"", stderr );
    dump_strW( symlink->target, symlink->len, stderr, "\"\"" );
    fputs( "\"\n", stderr );
}

static bool symlink_init( struct object *obj, const void *init_data )
{
    struct symlink *symlink = (struct symlink *)obj;
    const struct unicode_str *target = init_data;

    if (!target)  /* root symlink */
    {
        symlink->len = 0;
        symlink->target = NULL;
        return true;
    }
    symlink->len = target->len;
    return !!(symlink->target = memdup( target->str, target->len ));
}

static struct object *symlink_lookup_name( struct object *obj, struct unicode_str *name,
                                           unsigned int attr, struct object *root )
{
    struct symlink *symlink = (struct symlink *)obj;
    struct unicode_str target_str, name_left;
    struct object *target;

    assert( obj->ops == &symlink_ops );

    if (!name) return NULL;
    if (!name->len && (attr & OBJ_OPENLINK)) return NULL;
    if (obj == root) return NULL;

    if (!symlink->len) return get_root_directory();

    target_str.str = symlink->target;
    target_str.len = symlink->len;
    if ((target = lookup_named_object( NULL, target_str, attr, &name_left )))
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

static void symlink_destroy( struct object *obj )
{
    struct symlink *symlink = (struct symlink *)obj;
    assert( obj->ops == &symlink_ops );
    free( symlink->target );
}

struct object *create_root_symlink( struct object *root, struct unicode_str name,
                                    unsigned int attr, const struct security_descriptor *sd )
{
    struct object_params params = { .ops = &symlink_ops, .root = root,
                                    .name = name, .attr = attr, .sd = sd };

    return create_named_object( &params );
}

struct object *create_symlink( struct object *root, struct unicode_str name,
                               unsigned int attr, struct unicode_str target,
                               const struct security_descriptor *sd )
{
    struct object_params params = { .ops = &symlink_ops, .root = root, .name = name,
                                    .attr = attr | OBJ_OPENLINK, .sd = sd, .init_data = &target };

    if (!target.len)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    return create_named_object( &params );
}

/* create a symlink pointing to an existing object */
struct object *create_obj_symlink( struct object *root, struct unicode_str name,
                                    unsigned int attr, struct object *target,
                                    const struct security_descriptor *sd )
{
    struct symlink *symlink;
    data_size_t len;
    WCHAR *target_name;
    struct unicode_str target_str;
    struct object_params params = { .ops = &symlink_ops, .root = root, .name = name,
                                    .attr = attr, .sd = sd, .init_data = &target_str };

    if (target->ops->get_full_name) target_name = target->ops->get_full_name( target, ~0u, &len );
    else target_name = default_get_full_name( target, ~0u, &len );

    if (!target_name)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    target_str.str = target_name;
    target_str.len = len;
    symlink = create_named_object( &params );
    free( target_name );
    return &symlink->obj;
}


/* create a symbolic link object */
DECL_HANDLER(create_symlink)
{
    struct unicode_str target;
    struct object_params params = { .ops = &symlink_ops, .access = req->access, .init_data = &target };

    if (!get_req_object_attributes( &params )) return;

    params.attr |= OBJ_OPENLINK;
    target.str = get_req_data_after_objattr( &params, &target.len );
    target.len = (target.len / sizeof(WCHAR)) * sizeof(WCHAR);

    if (target.len)
    {
        reply->handle = create_named_obj_handle( current->process, &params );
        if (reply->handle) clear_error();
    }
    else set_error( STATUS_INVALID_PARAMETER );

    if (params.root) release_object( params.root );
}

/* open a symbolic link object */
DECL_HANDLER(open_symlink)
{
    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &symlink_ops, get_req_unicode_str(), req->attributes | OBJ_OPENLINK );
}

/* query a symbolic link object */
DECL_HANDLER(query_symlink)
{
    struct symlink *symlink;

    symlink = (struct symlink *)get_handle_obj( current->process, req->handle,
                                                SYMBOLIC_LINK_QUERY, &symlink_ops );
    if (!symlink) return;

    reply->total = symlink->len;
    if (get_reply_max_size() < symlink->len)
        set_error( STATUS_BUFFER_TOO_SMALL );
    else
        set_reply_data( symlink->target, symlink->len );
    release_object( symlink );
}

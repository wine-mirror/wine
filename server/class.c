/*
 * Server-side window class management
 *
 * Copyright (C) 2002 Mike McCormack
 * Copyright (C) 2003 Alexandre Julliard
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "wine/list.h"

#include "request.h"
#include "object.h"
#include "process.h"
#include "user.h"
#include "file.h"
#include "winuser.h"
#include "winternl.h"

struct window_class
{
    struct list         entry;           /* entry in process list */
    struct winstation  *winstation;      /* winstation the class was created on */
    struct process     *process;         /* process owning the class */
    int                 count;           /* reference count */
    int                 local;           /* local class? */
    atom_t              atom;            /* class atom for versioned class */
    client_ptr_t        client_ptr;      /* pointer to class in client address space */
    class_shm_t        *shared;          /* class in session shared memory */
};

C_ASSERT( sizeof(class_shm_t) == offsetof(class_shm_t, extra[0]) );

static struct window_class *create_class( struct process *process, int local, struct unicode_str *name, unsigned int name_offset,
                                          atom_t atom, mod_handle_t instance, unsigned int style, int cls_extra, int win_extra )
{
    struct window_class *class;

    if (!(class = mem_alloc( sizeof(*class) ))) return NULL;

    class->process = (struct process *)grab_object( process );
    class->count = 0;
    class->local = local;

    if (!(class->shared = alloc_shared_object( offsetof(class_shm_t, extra[cls_extra]) ))) goto failed;
    SHARED_WRITE_BEGIN( class->shared, class_shm_t )
    {
        memcpy( (void *)shared->name, name->str, name->len );
        shared->name_offset  = name_offset;
        shared->name_len     = name->len;
        shared->atom         = atom;
        shared->instance     = instance;
        shared->style        = style;
        shared->win_extra    = win_extra;
        shared->cls_extra    = cls_extra;
        memset( (void *)shared->extra, 0, cls_extra );
    }
    SHARED_WRITE_END;

    /* other fields are initialized by caller */

    /* local classes have priority so we put them first in the list */
    if (local) list_add_head( &process->classes, &class->entry );
    else list_add_tail( &process->classes, &class->entry );
    return class;

failed:
    free( class );
    return NULL;
}

static void destroy_class( struct window_class *class )
{
    struct atom_table *table = get_user_atom_table();

    release_atom( table, class->atom );
    release_atom( table, class->shared->atom );
    list_remove( &class->entry );
    release_object( class->process );
    if (class->shared) free_shared_object( class->shared );
    free( class );
}

void destroy_process_classes( struct process *process )
{
    struct list *ptr;

    while ((ptr = list_head( &process->classes )))
    {
        struct window_class *class = LIST_ENTRY( ptr, struct window_class, entry );
        destroy_class( class );
    }
}

static struct window_class *find_class( struct process *process, atom_t atom, mod_handle_t instance )
{
    struct window_class *class;
    int is_win16;

    LIST_FOR_EACH_ENTRY( class, &process->classes, struct window_class, entry )
    {
        const class_shm_t *shared = class->shared;
        if (class->atom != atom) continue;
        is_win16 = !(shared->instance >> 16);
        if (!instance || !class->local || shared->instance == instance ||
            (!is_win16 && ((shared->instance & ~0xffff) == (instance & ~0xffff)))) return class;
    }
    return NULL;
}

struct window_class *grab_class( struct process *process, atom_t atom, mod_handle_t instance,
                                 int *extra_bytes, struct obj_locator *locator )
{
    struct window_class *class = find_class( process, atom, instance );
    if (class)
    {
        class->count++;
        *extra_bytes = class->shared->win_extra;
        *locator = get_shared_object_locator( class->shared );
    }
    else set_error( STATUS_INVALID_HANDLE );
    return class;
}

void release_class( struct window_class *class )
{
    assert( class->count > 0 );
    class->count--;
}

int is_desktop_class( struct window_class *class )
{
    return (class->shared->atom == DESKTOP_ATOM && !class->local);
}

int is_message_class( struct window_class *class )
{
    static const WCHAR messageW[] = {'M','e','s','s','a','g','e'};
    static const struct unicode_str name = { messageW, sizeof(messageW) };
    struct atom_table *table = get_user_atom_table();

    return (!class->local && class->shared->atom == find_atom( table, &name ));
}

int get_class_style( struct window_class *class )
{
    return class->shared->style;
}

atom_t get_class_atom( struct window_class *class )
{
    return class->shared->atom;
}

client_ptr_t get_class_client_ptr( struct window_class *class )
{
    return class->client_ptr;
}

static struct unicode_str integral_atom_name( WCHAR *buffer, atom_t atom )
{
    struct unicode_str name;
    char tmp[16];
    int ret = snprintf( tmp, sizeof(tmp), "#%u", atom );
    for (int i = ret; i >= 0; i--) buffer[i] = tmp[i];
    name.len = ret * sizeof(WCHAR);
    name.str = buffer;
    return name;
}

/* create a window class */
DECL_HANDLER(create_class)
{
    struct window_class *class;
    struct unicode_str name = get_req_unicode_str();
    struct atom_table *table = get_user_atom_table();
    atom_t atom = req->atom, base_atom;
    unsigned int offset = 0;
    WCHAR buffer[16];

    if (atom && !name.len) name = integral_atom_name( buffer, atom );
    if (!atom && !(atom = add_atom( table, &name ))) return;

    if (req->name_offset && req->name_offset < name.len / sizeof(WCHAR))
    {
        struct unicode_str base = name;

        offset = req->name_offset;
        base.str += offset;
        base.len -= offset * sizeof(WCHAR);

        if (!(base_atom = add_atom( table, &base )))
        {
            release_atom( table, atom );
            return;
        }
    }
    else
    {
        base_atom = grab_atom( table, atom );
    }

    class = find_class( current->process, atom, req->instance );
    if (class && !class->local == !req->local)
    {
        set_win32_error( ERROR_CLASS_ALREADY_EXISTS );
        release_atom( table, atom );
        release_atom( table, base_atom );
        return;
    }
    if (req->cls_extra < 0 || req->cls_extra > 4096 || req->win_extra < 0 || req->win_extra > 4096)
    {
        /* don't allow stupid values here */
        set_error( STATUS_INVALID_PARAMETER );
        release_atom( table, atom );
        release_atom( table, base_atom );
        return;
    }

    if (!(class = create_class( current->process, req->local, &name, offset, base_atom,
                                req->instance, req->style, req->cls_extra, req->win_extra )))
    {
        release_atom( table, atom );
        release_atom( table, base_atom );
        return;
    }
    class->atom       = atom;
    class->client_ptr = req->client_ptr;
    reply->locator   = get_shared_object_locator( class->shared );
    reply->atom      = base_atom;
}

/* destroy a window class */
DECL_HANDLER(destroy_class)
{
    struct window_class *class;
    struct unicode_str name = get_req_unicode_str();
    struct atom_table *table = get_user_atom_table();
    atom_t atom = req->atom;

    if (!atom) atom = find_atom( table, &name );

    if (!(class = find_class( current->process, atom, req->instance )))
        set_win32_error( ERROR_CLASS_DOES_NOT_EXIST );
    else if (class->count)
        set_win32_error( ERROR_CLASS_HAS_WINDOWS );
    else
    {
        reply->client_ptr = class->client_ptr;
        destroy_class( class );
    }
}


/* set some information in a class */
DECL_HANDLER(set_class_info)
{
    struct window_class *class = get_window_class( req->window );

    if (!class) return;

    if (class->process != current->process)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    SHARED_WRITE_BEGIN( class->shared, class_shm_t )
    {
        switch (req->offset)
        {
        case GCL_STYLE:
            reply->old_info = shared->style;
            shared->style = req->new_info;
            break;
        case GCL_CBWNDEXTRA:
            if (req->new_info > 4096)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            reply->old_info = shared->win_extra;
            shared->win_extra = req->new_info;
            break;
        case GCL_CBCLSEXTRA:
            set_win32_error( ERROR_INVALID_INDEX );
            break;
        case GCLP_HMODULE:
            reply->old_info = shared->instance;
            shared->instance = req->new_info;
            break;
        default:
            if (req->size > sizeof(req->new_info) || req->offset < 0 ||
                req->offset > class->shared->cls_extra - (int)req->size)
            {
                set_win32_error( ERROR_INVALID_INDEX );
                return;
            }
            memcpy( &reply->old_info, (char *)shared->extra + req->offset, req->size );
            memcpy( (char *)shared->extra + req->offset, &req->new_info, req->size );
            break;
        }
    }
    SHARED_WRITE_END;
}


/* get some information in a class */
DECL_HANDLER(get_class_info)
{
    struct window_class *class;

    if (!(class = get_window_class( req->window ))) return;

    switch (req->offset)
    {
    case GCLP_HBRBACKGROUND:
    case GCLP_HCURSOR:
    case GCLP_HICON:
    case GCLP_HICONSM:
    case GCLP_WNDPROC:
    case GCLP_MENUNAME:
        /* not supported */
        set_win32_error( ERROR_INVALID_HANDLE );
        break;
    case GCL_CBCLSEXTRA:     reply->info = class->shared->cls_extra; break;
    default:
        if (req->size > sizeof(reply->info) || req->offset < 0 ||
            req->offset > class->shared->cls_extra - (int)req->size)
        {
            set_win32_error( ERROR_INVALID_INDEX );
            return;
        }
        memcpy( &reply->info, (char *)class->shared->extra + req->offset, req->size );
        break;
    }
}

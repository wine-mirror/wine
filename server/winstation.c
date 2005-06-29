/*
 * Server-side window stations and desktops handling
 *
 * Copyright (C) 2002, 2005 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "object.h"
#include "handle.h"
#include "request.h"
#include "process.h"
#include "user.h"
#include "wine/unicode.h"

struct winstation
{
    struct object      obj;                /* object header */
    unsigned int       flags;              /* winstation flags */
    struct list        entry;              /* entry in global winstation list */
    struct list        desktops;           /* list of desktops of this winstation */
    struct clipboard  *clipboard;          /* clipboard information */
};

struct desktop
{
    struct object      obj;           /* object header */
    unsigned int       flags;         /* desktop flags */
    struct winstation *winstation;    /* winstation this desktop belongs to */
    struct list        entry;         /* entry in winstation list of desktops */
};

static struct list winstation_list = LIST_INIT(winstation_list);
static struct winstation *interactive_winstation;
static struct namespace *winstation_namespace;

static void winstation_dump( struct object *obj, int verbose );
static int winstation_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void winstation_destroy( struct object *obj );
static void desktop_dump( struct object *obj, int verbose );
static int desktop_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void desktop_destroy( struct object *obj );

static const struct object_ops winstation_ops =
{
    sizeof(struct winstation),    /* size */
    winstation_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    winstation_close_handle,      /* close_handle */
    winstation_destroy            /* destroy */
};


static const struct object_ops desktop_ops =
{
    sizeof(struct desktop),       /* size */
    desktop_dump,                 /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    desktop_close_handle,         /* close_handle */
    desktop_destroy               /* destroy */
};

#define DESKTOP_ALL_ACCESS 0x01ff

/* create a winstation object */
static struct winstation *create_winstation( const WCHAR *name, size_t len, unsigned int flags )
{
    struct winstation *winstation;

    if (!winstation_namespace && !(winstation_namespace = create_namespace( 7, FALSE )))
        return NULL;

    if (memchrW( name, '\\', len / sizeof(WCHAR) ))  /* no backslash allowed in name */
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if ((winstation = create_named_object( winstation_namespace, &winstation_ops, name, len )))
    {
        if (get_error() != STATUS_OBJECT_NAME_COLLISION)
        {
            /* initialize it if it didn't already exist */
            winstation->flags = flags;
            winstation->clipboard = NULL;
            list_add_tail( &winstation_list, &winstation->entry );
            list_init( &winstation->desktops );
        }
    }
    return winstation;
}

static void winstation_dump( struct object *obj, int verbose )
{
    struct winstation *winstation = (struct winstation *)obj;

    fprintf( stderr, "Winstation flags=%x ", winstation->flags );
    dump_object_name( &winstation->obj );
    fputc( '\n', stderr );
}

static int winstation_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    return (process->winstation != handle);
}

static void winstation_destroy( struct object *obj )
{
    struct winstation *winstation = (struct winstation *)obj;

    if (winstation == interactive_winstation) interactive_winstation = NULL;
    list_remove( &winstation->entry );
    if (winstation->clipboard) release_object( winstation->clipboard );
}

/* retrieve the process window station, checking the handle access rights */
struct winstation *get_process_winstation( struct process *process, unsigned int access )
{
    return (struct winstation *)get_handle_obj( process, process->winstation,
                                                access, &winstation_ops );
}

/* set the pointer to the (opaque) clipboard info */
void set_winstation_clipboard( struct winstation *winstation, struct clipboard *clipboard )
{
    winstation->clipboard = clipboard;
}

/* retrieve the pointer to the (opaque) clipboard info */
struct clipboard *get_winstation_clipboard( struct winstation *winstation )
{
    return winstation->clipboard;
}

/* build the full name of a desktop object */
static WCHAR *build_desktop_name( const WCHAR *name, size_t len,
                                  struct winstation *winstation, size_t *res_len )
{
    const WCHAR *winstation_name;
    WCHAR *full_name;
    size_t winstation_len;

    if (memchrW( name, '\\', len / sizeof(WCHAR) ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if (!(winstation_name = get_object_name( &winstation->obj, &winstation_len )))
        winstation_len = 0;

    *res_len = winstation_len + len + sizeof(WCHAR);
    if (!(full_name = mem_alloc( *res_len ))) return NULL;
    memcpy( full_name, winstation_name, winstation_len );
    full_name[winstation_len / sizeof(WCHAR)] = '\\';
    memcpy( full_name + winstation_len / sizeof(WCHAR) + 1, name, len );
    return full_name;
}

/* retrieve a pointer to a desktop object */
inline static struct desktop *get_desktop_obj( struct process *process, obj_handle_t handle,
                                               unsigned int access )
{
    return (struct desktop *)get_handle_obj( process, handle, access, &desktop_ops );
}

/* create a desktop object */
static struct desktop *create_desktop( const WCHAR *name, size_t len, unsigned int flags,
                                       struct winstation *winstation )
{
    struct desktop *desktop;
    WCHAR *full_name;
    size_t full_len;

    if (!(full_name = build_desktop_name( name, len, winstation, &full_len ))) return NULL;

    if ((desktop = create_named_object( winstation_namespace, &desktop_ops, full_name, full_len )))
    {
        if (get_error() != STATUS_OBJECT_NAME_COLLISION)
        {
            /* initialize it if it didn't already exist */
            desktop->flags = flags;
            desktop->winstation = (struct winstation *)grab_object( winstation );
            list_add_tail( &winstation->desktops, &desktop->entry );
        }
    }
    free( full_name );
    return desktop;
}

static void desktop_dump( struct object *obj, int verbose )
{
    struct desktop *desktop = (struct desktop *)obj;

    fprintf( stderr, "Desktop flags=%x winstation=%p ", desktop->flags, desktop->winstation );
    dump_object_name( &desktop->obj );
    fputc( '\n', stderr );
}

static int desktop_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    struct thread *thread;

    /* check if the handle is currently used by the process or one of its threads */
    if (process->desktop == handle) return 0;
    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
        if (thread->desktop == handle) return 0;
    return 1;
}

static void desktop_destroy( struct object *obj )
{
    struct desktop *desktop = (struct desktop *)obj;

    list_remove( &desktop->entry );
    release_object( desktop->winstation );
}

/* connect a process to its window station */
void connect_process_winstation( struct process *process, const WCHAR *name, size_t len )
{
    struct winstation *winstation;

    if (process->winstation) return;  /* already has one */

    /* check for an inherited winstation handle (don't ask...) */
    if ((process->winstation = find_inherited_handle( process, &winstation_ops )) != 0) return;

    if (name)
    {
        winstation = create_winstation( name, len, 0 );
    }
    else
    {
        if (!interactive_winstation)
        {
            static const WCHAR winsta0W[] = {'W','i','n','S','t','a','0'};
            interactive_winstation = create_winstation( winsta0W, sizeof(winsta0W), 0 );
            winstation = interactive_winstation;
        }
        else winstation = (struct winstation *)grab_object( interactive_winstation );
    }
    if (winstation)
    {
        process->winstation = alloc_handle( process, winstation, WINSTA_ALL_ACCESS, FALSE );
        release_object( winstation );
    }
    clear_error();  /* ignore errors */
}

/* connect a process to its main desktop */
void connect_process_desktop( struct process *process, const WCHAR *name, size_t len )
{
    struct desktop *desktop;
    struct winstation *winstation;

    if (process->desktop) return;  /* already has one */

    if ((winstation = get_process_winstation( process, WINSTA_CREATEDESKTOP )))
    {
        static const WCHAR defaultW[] = {'D','e','f','a','u','l','t'};

        if (!name)
        {
            name = defaultW;
            len = sizeof(defaultW);
        }
        if ((desktop = create_desktop( name, len, 0, winstation )))
        {
            process->desktop = alloc_handle( process, desktop, DESKTOP_ALL_ACCESS, FALSE );
            release_object( desktop );
        }
        release_object( winstation );
    }
    clear_error();  /* ignore errors */
}

/* close the desktop of a given thread */
void close_thread_desktop( struct thread *thread )
{
    obj_handle_t handle = thread->desktop;

    thread->desktop = 0;
    if (handle) close_handle( thread->process, handle, NULL );
    clear_error();  /* ignore errors */
}


/* create a window station */
DECL_HANDLER(create_winstation)
{
    struct winstation *winstation;

    reply->handle = 0;
    if ((winstation = create_winstation( get_req_data(), get_req_data_size(), req->flags )))
    {
        reply->handle = alloc_handle( current->process, winstation, req->access, req->inherit );
        release_object( winstation );
    }
}

/* open a handle to a window station */
DECL_HANDLER(open_winstation)
{
    if (winstation_namespace)
        reply->handle = open_object( winstation_namespace, get_req_data(), get_req_data_size(),
                                     &winstation_ops, req->access, req->inherit );
    else
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
}


/* close a window station */
DECL_HANDLER(close_winstation)
{
    struct winstation *winstation;

    if ((winstation = (struct winstation *)get_handle_obj( current->process, req->handle,
                                                           0, &winstation_ops )))
    {
        if (!close_handle( current->process, req->handle, NULL )) set_error( STATUS_ACCESS_DENIED );
        release_object( winstation );
    }
}


/* get the process current window station */
DECL_HANDLER(get_process_winstation)
{
    reply->handle = current->process->winstation;
}


/* set the process current window station */
DECL_HANDLER(set_process_winstation)
{
    struct winstation *winstation;

    if ((winstation = (struct winstation *)get_handle_obj( current->process, req->handle,
                                                           0, &winstation_ops )))
    {
        /* FIXME: should we close the old one? */
        current->process->winstation = req->handle;
        release_object( winstation );
    }
}

/* create a desktop */
DECL_HANDLER(create_desktop)
{
    struct desktop *desktop;
    struct winstation *winstation;

    reply->handle = 0;
    if ((winstation = get_process_winstation( current->process, WINSTA_CREATEDESKTOP )))
    {
        if ((desktop = create_desktop( get_req_data(), get_req_data_size(),
                                       req->flags, winstation )))
        {
            reply->handle = alloc_handle( current->process, desktop, req->access, req->inherit );
            release_object( desktop );
        }
        release_object( winstation );
    }
}

/* open a handle to a desktop */
DECL_HANDLER(open_desktop)
{
    struct winstation *winstation;

    if ((winstation = get_process_winstation( current->process, 0 /* FIXME: access rights? */ )))
    {
        size_t full_len;
        WCHAR *full_name;

        if ((full_name = build_desktop_name( get_req_data(), get_req_data_size(),
                                             winstation, &full_len )))
        {
            reply->handle = open_object( winstation_namespace, full_name, full_len,
                                         &desktop_ops, req->access, req->inherit );
            free( full_name );
        }
        release_object( winstation );
    }
}


/* close a desktop */
DECL_HANDLER(close_desktop)
{
    struct desktop *desktop;

    /* make sure it is a desktop handle */
    if ((desktop = (struct desktop *)get_handle_obj( current->process, req->handle,
                                                     0, &desktop_ops )))
    {
        if (!close_handle( current->process, req->handle, NULL )) set_error( STATUS_DEVICE_BUSY );
        release_object( desktop );
    }
}


/* get the thread current desktop */
DECL_HANDLER(get_thread_desktop)
{
    struct thread *thread;

    if (!(thread = get_thread_from_id( req->tid ))) return;
    reply->handle = thread->desktop;
    release_object( thread );
}


/* set the thread current desktop */
DECL_HANDLER(set_thread_desktop)
{
    struct desktop *old_desktop, *new_desktop;
    struct winstation *winstation;

    if (!(winstation = get_process_winstation( current->process, 0 /* FIXME: access rights? */ )))
        return;

    if (!(new_desktop = get_desktop_obj( current->process, req->handle, 0 )))
    {
        release_object( winstation );
        return;
    }
    if (new_desktop->winstation != winstation)
    {
        set_error( STATUS_ACCESS_DENIED );
        release_object( new_desktop );
        release_object( winstation );
        return;
    }

    /* check if we are changing to a new desktop */

    if (!(old_desktop = get_desktop_obj( current->process, current->desktop, 0)))
        clear_error();  /* ignore error */

    /* when changing desktop, we can't have any users on the current one */
    if (old_desktop != new_desktop && current->desktop_users > 0)
        set_error( STATUS_DEVICE_BUSY );
    else
        current->desktop = req->handle;  /* FIXME: should we close the old one? */

    if (old_desktop) release_object( old_desktop );
    release_object( new_desktop );
    release_object( winstation );
}


/* get/set information about a user object (window station or desktop) */
DECL_HANDLER(set_user_object_info)
{
    struct object *obj;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL ))) return;

    if (obj->ops == &desktop_ops)
    {
        struct desktop *desktop = (struct desktop *)obj;
        reply->is_desktop = 1;
        reply->old_obj_flags = desktop->flags;
        if (req->flags & SET_USER_OBJECT_FLAGS) desktop->flags = req->obj_flags;
    }
    else if (obj->ops == &winstation_ops)
    {
        struct winstation *winstation = (struct winstation *)obj;
        reply->is_desktop = 0;
        reply->old_obj_flags = winstation->flags;
        if (req->flags & SET_USER_OBJECT_FLAGS) winstation->flags = req->obj_flags;
    }
    else
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
        return;
    }
    if (get_reply_max_size())
    {
        size_t len;
        const WCHAR *ptr, *name = get_object_name( obj, &len );

        /* if there is a backslash return the part of the name after it */
        if (name && (ptr = memchrW( name, '\\', len )))
        {
            len -= (ptr + 1 - name) * sizeof(WCHAR);
            name = ptr + 1;
        }
        if (name) set_reply_data( name, min( len, get_reply_max_size() ));
    }
    release_object( obj );
}

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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winternl.h"
#include "ntuser.h"

#include "object.h"
#include "handle.h"
#include "request.h"
#include "process.h"
#include "user.h"
#include "file.h"
#include "security.h"

#define DESKTOP_ALL_ACCESS 0x01ff

static struct list winstation_list = LIST_INIT(winstation_list);

static void winstation_dump( struct object *obj, int verbose );
static int winstation_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static struct object *winstation_lookup_name( struct object *obj, struct unicode_str *name,
                                              unsigned int attr, struct object *root );
static void winstation_destroy( struct object *obj );
static void desktop_dump( struct object *obj, int verbose );
static int desktop_link_name( struct object *obj, struct object_name *name, struct object *parent );
static int desktop_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void desktop_destroy( struct object *obj );

static const WCHAR winstation_name[] = {'W','i','n','d','o','w','S','t','a','t','i','o','n'};

struct type_descr winstation_type =
{
    { winstation_name, sizeof(winstation_name) },   /* name */
    STANDARD_RIGHTS_REQUIRED | WINSTA_ALL_ACCESS,   /* valid_access */
    {                                               /* mapping */
        STANDARD_RIGHTS_READ | WINSTA_READSCREEN | WINSTA_ENUMERATE | WINSTA_READATTRIBUTES | WINSTA_ENUMDESKTOPS,
        STANDARD_RIGHTS_WRITE | WINSTA_WRITEATTRIBUTES | WINSTA_CREATEDESKTOP | WINSTA_ACCESSCLIPBOARD,
        STANDARD_RIGHTS_EXECUTE | WINSTA_EXITWINDOWS | WINSTA_ACCESSGLOBALATOMS,
        STANDARD_RIGHTS_REQUIRED | WINSTA_ALL_ACCESS
    },
};

static const struct object_ops winstation_ops =
{
    sizeof(struct winstation),    /* size */
    &winstation_type,             /* type */
    winstation_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    default_get_full_name,        /* get_full_name */
    winstation_lookup_name,       /* lookup_name */
    directory_link_name,          /* link_name */
    default_unlink_name,          /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    winstation_close_handle,      /* close_handle */
    winstation_destroy            /* destroy */
};


static const WCHAR desktop_name[] = {'D','e','s','k','t','o','p'};

struct type_descr desktop_type =
{
    { desktop_name, sizeof(desktop_name) },   /* name */
    STANDARD_RIGHTS_REQUIRED | DESKTOP_ALL_ACCESS,  /* valid_access */
    {                                         /* mapping */
        STANDARD_RIGHTS_READ | DESKTOP_ENUMERATE | DESKTOP_READOBJECTS,
        STANDARD_RIGHTS_WRITE | DESKTOP_WRITEOBJECTS | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD
        | DESKTOP_HOOKCONTROL | DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW,
        STANDARD_RIGHTS_EXECUTE | DESKTOP_SWITCHDESKTOP,
        STANDARD_RIGHTS_REQUIRED | DESKTOP_ALL_ACCESS
    },
};

static const struct object_ops desktop_ops =
{
    sizeof(struct desktop),       /* size */
    &desktop_type,                /* type */
    desktop_dump,                 /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    default_get_full_name,        /* get_full_name */
    no_lookup_name,               /* lookup_name */
    desktop_link_name,            /* link_name */
    default_unlink_name,          /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    desktop_close_handle,         /* close_handle */
    desktop_destroy               /* destroy */
};

/* create a winstation object */
static struct winstation *create_winstation( struct object *root, const struct unicode_str *name,
                                             unsigned int attr, unsigned int flags )
{
    struct winstation *winstation;

    if ((winstation = create_named_object( root, &winstation_ops, name, attr, NULL )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            winstation->flags = flags;
            winstation->input_desktop = NULL;
            winstation->clipboard = NULL;
            winstation->atom_table = NULL;
            winstation->monitors = NULL;
            winstation->monitor_count = 0;
            winstation->monitor_serial = 1;
            list_add_tail( &winstation_list, &winstation->entry );
            list_init( &winstation->desktops );
            if (!(winstation->desktop_names = create_namespace( 7 )))
            {
                release_object( winstation );
                return NULL;
            }
        }
        else clear_error();
    }
    return winstation;
}

static void winstation_dump( struct object *obj, int verbose )
{
    struct winstation *winstation = (struct winstation *)obj;

    fprintf( stderr, "Winstation flags=%x clipboard=%p atoms=%p\n",
             winstation->flags, winstation->clipboard, winstation->atom_table );
}

static int winstation_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    return (process->winstation != handle);
}

static struct object *winstation_lookup_name( struct object *obj, struct unicode_str *name,
                                              unsigned int attr, struct object *root )
{
    struct winstation *winstation = (struct winstation *)obj;
    struct object *found;

    assert( obj->ops == &winstation_ops );

    if (!name) return NULL;  /* open the winstation itself */

    if (get_path_element( name->str, name->len ) < name->len)  /* no backslash allowed in name */
    {
        set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
        return NULL;
    }

    if ((found = find_object( winstation->desktop_names, name, attr )))
        name->len = 0;

    return found;
}

static void winstation_destroy( struct object *obj )
{
    struct winstation *winstation = (struct winstation *)obj;

    list_remove( &winstation->entry );
    if (winstation->clipboard) release_object( winstation->clipboard );
    if (winstation->atom_table) release_object( winstation->atom_table );
    free( winstation->desktop_names );
    free( winstation->monitors );
}

/* retrieve the process window station, checking the handle access rights */
struct winstation *get_process_winstation( struct process *process, unsigned int access )
{
    return (struct winstation *)get_handle_obj( process, process->winstation,
                                                access, &winstation_ops );
}

/* retrieve the visible winstation */
struct winstation *get_visible_winstation(void)
{
    struct winstation *winstation;
    LIST_FOR_EACH_ENTRY( winstation, &winstation_list, struct winstation, entry )
        if (winstation->flags & WSF_VISIBLE) return winstation;
    return NULL;
}

/* retrieve the winstation current input desktop */
struct desktop *get_input_desktop( struct winstation *winstation )
{
    struct desktop *desktop;
    if (!(desktop = winstation->input_desktop)) return NULL;
    return (struct desktop *)grab_object( desktop );
}

/* changes the winstation current input desktop and update its input time */
int set_input_desktop( struct winstation *winstation, struct desktop *new_desktop )
{
    struct desktop *old_desktop = winstation->input_desktop;
    struct thread *thread;

    if (!(winstation->flags & WSF_VISIBLE)) return 0;
    if (new_desktop) new_desktop->input_time = current_time;
    if (old_desktop == new_desktop) return 1;

    if (old_desktop)
    {
        /* disconnect every process of the old input desktop from rawinput */
        LIST_FOR_EACH_ENTRY( thread, &old_desktop->threads, struct thread, desktop_entry )
            set_rawinput_process( thread->process, 0 );
    }

    if ((winstation->input_desktop = new_desktop))
    {
        /* connect every process of the new input desktop to rawinput */
        LIST_FOR_EACH_ENTRY( thread, &new_desktop->threads, struct thread, desktop_entry )
            set_rawinput_process( thread->process, 1 );
    }

    return 1;
}

/* retrieve a pointer to a desktop object */
struct desktop *get_desktop_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct desktop *)get_handle_obj( process, handle, access, &desktop_ops );
}

/* create a desktop object */
static struct desktop *create_desktop( const struct unicode_str *name, unsigned int attr,
                                       unsigned int flags, struct winstation *winstation )
{
    struct desktop *desktop, *current_desktop;

    if ((desktop = create_named_object( &winstation->obj, &desktop_ops, name, attr, NULL )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */

            /* inherit DF_WINE_*_DESKTOP flags if none of them are specified */
            if (!(flags & (DF_WINE_ROOT_DESKTOP | DF_WINE_VIRTUAL_DESKTOP))
                && (current_desktop = get_thread_desktop( current, 0 )))
            {
                flags |= current_desktop->shared->flags & (DF_WINE_VIRTUAL_DESKTOP | DF_WINE_ROOT_DESKTOP);
                release_object( current_desktop );
            }

            desktop->winstation = (struct winstation *)grab_object( winstation );
            desktop->top_window = NULL;
            desktop->msg_window = NULL;
            desktop->shell_window = NULL;
            desktop->shell_listview = NULL;
            desktop->progman_window = NULL;
            desktop->taskman_window = NULL;
            desktop->global_hooks = NULL;
            desktop->close_timeout = NULL;
            desktop->foreground_input = NULL;
            desktop->users = 0;
            list_init( &desktop->threads );
            desktop->clip_flags = 0;
            desktop->cursor_win = 0;
            desktop->alt_pressed = 0;
            memset( &desktop->key_repeat, 0, sizeof(desktop->key_repeat) );
            list_add_tail( &winstation->desktops, &desktop->entry );
            list_init( &desktop->hotkeys );
            list_init( &desktop->pointers );

            if (!(desktop->shared = alloc_shared_object()))
            {
                release_object( desktop );
                return NULL;
            }

            SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
            {
                shared->flags = flags;
                shared->cursor.x = 0;
                shared->cursor.y = 0;
                shared->cursor.last_change = 0;
                shared->cursor.clip.left = 0;
                shared->cursor.clip.top = 0;
                shared->cursor.clip.right = 0;
                shared->cursor.clip.bottom = 0;
                memset( (void *)shared->keystate, 0, sizeof(shared->keystate) );
                shared->monitor_serial = winstation->monitor_serial;
            }
            SHARED_WRITE_END;
        }
        else
        {
            SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
            {
                shared->flags |= flags & (DF_WINE_VIRTUAL_DESKTOP | DF_WINE_ROOT_DESKTOP);
            }
            SHARED_WRITE_END;

            clear_error();
        }
    }
    return desktop;
}

static void desktop_dump( struct object *obj, int verbose )
{
    struct desktop *desktop = (struct desktop *)obj;

    fprintf( stderr, "Desktop flags=%x winstation=%p top_win=%p hooks=%p\n",
             desktop->shared->flags, desktop->winstation, desktop->top_window, desktop->global_hooks );
}

static int desktop_link_name( struct object *obj, struct object_name *name, struct object *parent )
{
    struct winstation *winstation = (struct winstation *)parent;

    if (parent->ops != &winstation_ops)
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return 0;
    }
    if (get_path_element( name->name, name->len ) < name->len)  /* no backslash allowed in name */
    {
        set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
        return 0;
    }
    namespace_add( winstation->desktop_names, name );
    return 1;
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
    struct winstation *winstation = desktop->winstation;

    list_remove( &desktop->entry );

    if (desktop == winstation->input_desktop)
    {
        struct desktop *other, *found = NULL;
        LIST_FOR_EACH_ENTRY(other, &winstation->desktops, struct desktop, entry)
            if (!found || other->input_time > found->input_time) found = other;
        set_input_desktop( winstation, found );
    }

    free_hotkeys( desktop, 0 );
    free_pointers( desktop );
    if (desktop->top_window) free_window_handle( desktop->top_window );
    if (desktop->msg_window) free_window_handle( desktop->msg_window );
    if (desktop->global_hooks) release_object( desktop->global_hooks );
    if (desktop->close_timeout) remove_timeout_user( desktop->close_timeout );
    if (desktop->key_repeat.timeout) remove_timeout_user( desktop->key_repeat.timeout );
    release_object( desktop->winstation );
    if (desktop->shared) free_shared_object( desktop->shared );
}

/* retrieve the thread desktop, checking the handle access rights */
struct desktop *get_thread_desktop( struct thread *thread, unsigned int access )
{
    return get_desktop_obj( thread->process, thread->desktop, access );
}

static void close_desktop_timeout( void *private )
{
    struct desktop *desktop = private;

    desktop->close_timeout = NULL;
    unlink_named_object( &desktop->obj );  /* make sure no other process can open it */
    post_desktop_message( desktop, WM_CLOSE, 0, 0 );  /* and signal the owner to quit */
}

/* add a user of the desktop and cancel the close timeout */
static void add_desktop_thread( struct desktop *desktop, struct thread *thread )
{
    list_add_tail( &desktop->threads, &thread->desktop_entry );
    add_desktop_hook_count( desktop, thread, 1 );

    if (!thread->process->is_system)
    {
        desktop->users++;
        if (desktop->close_timeout)
        {
            remove_timeout_user( desktop->close_timeout );
            desktop->close_timeout = NULL;
        }
    }

    /* if thread process is now connected to the input desktop, let it receive rawinput */
    if (desktop == desktop->winstation->input_desktop) set_rawinput_process( thread->process, 1 );
}

/* remove a user of the desktop and start the close timeout if necessary */
static void remove_desktop_user( struct desktop *desktop, struct thread *thread )
{
    struct process *process;

    assert( desktop->users > 0 );
    desktop->users--;

    /* if we have one remaining user, it has to be the manager of the desktop window */
    if ((process = get_top_window_owner( desktop )) && desktop->users == process->running_threads && !desktop->close_timeout)
        desktop->close_timeout = add_timeout_user( -TICKS_PER_SEC, close_desktop_timeout, desktop );
}

/* remove a thread from the list of threads attached to a desktop */
static void remove_desktop_thread( struct desktop *desktop, struct thread *thread )
{
    add_desktop_hook_count( desktop, thread, -1 );
    list_remove( &thread->desktop_entry );

    if (!thread->process->is_system) remove_desktop_user( desktop, thread );

    if (desktop == desktop->winstation->input_desktop)
    {
        /* thread process might still be connected the input desktop through another thread, update the full list */
        LIST_FOR_EACH_ENTRY( thread, &desktop->threads, struct thread, desktop_entry )
            set_rawinput_process( thread->process, 1 );
    }
}

/* set the thread default desktop handle */
void set_thread_default_desktop( struct thread *thread, struct desktop *desktop, obj_handle_t handle )
{
    if (thread->desktop) return;  /* nothing to do */

    thread->desktop = handle;
    add_desktop_thread( desktop, thread );
}

/* set the process default desktop handle */
void set_process_default_desktop( struct process *process, struct desktop *desktop,
                                  obj_handle_t handle )
{
    struct thread *thread;

    if (process->desktop == handle) return;  /* nothing to do */

    process->desktop = handle;

    /* set desktop for threads that don't have one yet */
    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
        set_thread_default_desktop( thread, desktop, handle );
}

/* connect a process to its window station */
void connect_process_winstation( struct process *process, struct unicode_str *desktop_path,
                                 struct thread *parent_thread, struct process *parent_process )
{
    struct unicode_str desktop_name = *desktop_path, winstation_name = {0};
    const int attributes = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    struct winstation *winstation = NULL;
    struct desktop *desktop = NULL;
    const WCHAR *wch, *end;
    obj_handle_t handle;

    for (wch = desktop_name.str, end = wch + desktop_name.len / sizeof(WCHAR); wch != end; wch++)
    {
        if (*wch == '\\')
        {
            winstation_name.str = desktop_name.str;
            winstation_name.len = (wch - winstation_name.str) * sizeof(WCHAR);
            desktop_name.str = wch + 1;
            desktop_name.len = (end - desktop_name.str) * sizeof(WCHAR);
            break;
        }
    }

    /* check for an inherited winstation handle (don't ask...) */
    if ((handle = find_inherited_handle( process, &winstation_ops )))
    {
        winstation = (struct winstation *)get_handle_obj( process, handle, 0, &winstation_ops );
    }
    else if (winstation_name.len && (winstation = open_named_object( NULL, &winstation_ops, &winstation_name, attributes )))
    {
        handle = alloc_handle( process, winstation, STANDARD_RIGHTS_REQUIRED | WINSTA_ALL_ACCESS, 0 );
    }
    else if (parent_process->winstation)
    {
        handle = duplicate_handle( parent_process, parent_process->winstation,
                                   process, 0, 0, DUPLICATE_SAME_ACCESS );
        winstation = (struct winstation *)get_handle_obj( process, handle, 0, &winstation_ops );
    }
    if (!winstation) goto done;
    process->winstation = handle;

    if ((handle = find_inherited_handle( process, &desktop_ops )))
    {
        desktop = get_desktop_obj( process, handle, 0 );
        if (!desktop || desktop->winstation != winstation) goto done;
    }
    else if (desktop_name.len && (desktop = open_named_object( &winstation->obj, &desktop_ops, &desktop_name, attributes )))
    {
        handle = alloc_handle( process, desktop, STANDARD_RIGHTS_REQUIRED | DESKTOP_ALL_ACCESS, 0 );
    }
    else
    {
        if (parent_thread && parent_thread->desktop)
            handle = parent_thread->desktop;
        else if (parent_process->desktop)
            handle = parent_process->desktop;
        else
            goto done;

        desktop = get_desktop_obj( parent_process, handle, 0 );

        if (!desktop || desktop->winstation != winstation) goto done;

        handle = duplicate_handle( parent_process, handle, process, 0, 0, DUPLICATE_SAME_ACCESS );
    }
    if (handle) set_process_default_desktop( process, desktop, handle );

done:
    if (desktop) release_object( desktop );
    if (winstation) release_object( winstation );
    clear_error();
}

/* close the desktop of a given process */
void close_process_desktop( struct process *process )
{
    obj_handle_t handle;

    if (!(handle = process->desktop)) return;

    process->desktop = 0;
    close_handle( process, handle );
}

/* release (and eventually close) the desktop of a given thread */
void release_thread_desktop( struct thread *thread, int close )
{
    struct desktop *desktop;
    obj_handle_t handle;

    if (!(handle = thread->desktop)) return;

    if (!(desktop = get_desktop_obj( thread->process, handle, 0 ))) clear_error();  /* ignore errors */
    else
    {
        if (close) remove_desktop_thread( desktop, thread );
        else remove_desktop_user( desktop, thread );
        release_object( desktop );
    }

    if (close)
    {
        thread->desktop = 0;
        close_handle( thread->process, handle );
    }
}

/* create a window station */
DECL_HANDLER(create_winstation)
{
    struct winstation *winstation;
    struct unicode_str name = get_req_unicode_str();
    struct object *root = NULL;

    reply->handle = 0;
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir ))) return;

    if ((winstation = create_winstation( root, &name, req->attributes, req->flags )))
    {
        reply->handle = alloc_handle( current->process, winstation, req->access, req->attributes );
        release_object( winstation );
    }
    if (root) release_object( root );
}

/* open a handle to a window station */
DECL_HANDLER(open_winstation)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &winstation_ops, &name, req->attributes );
}


/* close a window station */
DECL_HANDLER(close_winstation)
{
    struct winstation *winstation;

    if ((winstation = (struct winstation *)get_handle_obj( current->process, req->handle,
                                                           0, &winstation_ops )))
    {
        if (close_handle( current->process, req->handle )) set_error( STATUS_ACCESS_DENIED );
        release_object( winstation );
    }
}


/* set the process current window station monitors */
DECL_HANDLER(set_winstation_monitors)
{
    struct winstation *winstation;
    struct desktop *desktop;
    unsigned int size = get_req_data_size();

    if (!(winstation = (struct winstation *)get_handle_obj( current->process, current->process->winstation,
                                                           0, &winstation_ops )))
        return;

    if (req->increment || winstation->monitor_count != size / sizeof(*winstation->monitors) ||
        !winstation->monitors || memcmp( winstation->monitors, get_req_data(), size ))
        winstation->monitor_serial++;

    free( winstation->monitors );
    winstation->monitors = NULL;
    winstation->monitor_count = 0;

    LIST_FOR_EACH_ENTRY(desktop, &winstation->desktops, struct desktop, entry)
    {
        SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
        {
            shared->monitor_serial = winstation->monitor_serial;
        }
        SHARED_WRITE_END;
    }

    if (size && (winstation->monitors = memdup( get_req_data(), size )))
        winstation->monitor_count = size / sizeof(*winstation->monitors);

    reply->serial = winstation->monitor_serial;
    release_object( winstation );
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
    struct unicode_str name = get_req_unicode_str();

    reply->handle = 0;
    if (!name.len)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if ((winstation = get_process_winstation( current->process, WINSTA_CREATEDESKTOP )))
    {
        if ((desktop = create_desktop( &name, req->attributes, req->flags, winstation )))
        {
            if (!winstation->input_desktop) set_input_desktop( winstation, desktop );
            reply->handle = alloc_handle( current->process, desktop, req->access, req->attributes );
            release_object( desktop );
        }
        release_object( winstation );
    }
}

/* open a handle to a desktop */
DECL_HANDLER(open_desktop)
{
    struct winstation *winstation;
    struct object *obj;
    struct unicode_str name = get_req_unicode_str();

    /* FIXME: check access rights */
    if (!req->winsta)
        winstation = get_process_winstation( current->process, 0 );
    else
        winstation = (struct winstation *)get_handle_obj( current->process, req->winsta, 0, &winstation_ops );

    if (!winstation) return;

    if ((obj = open_named_object( &winstation->obj, &desktop_ops, &name, req->attributes )))
    {
        reply->handle = alloc_handle( current->process, obj, req->access, req->attributes );
        release_object( obj );
    }

    release_object( winstation );
}

/* open a handle to current input desktop */
DECL_HANDLER(open_input_desktop)
{
    /* FIXME: check access rights */
    struct winstation *winstation = get_process_winstation( current->process, 0 );
    struct desktop *desktop;

    if (!winstation) return;

    if (!(winstation->flags & WSF_VISIBLE))
    {
        set_error( STATUS_ILLEGAL_FUNCTION );
        release_object( winstation );
        return;
    }

    if ((desktop = get_input_desktop( winstation )))
    {
        reply->handle = alloc_handle( current->process, desktop, req->access, req->attributes );
        release_object( desktop );
    }
    release_object( winstation );
}

/* changes the current input desktop */
DECL_HANDLER(set_input_desktop)
{
    /* FIXME: check access rights */
    struct winstation *winstation;
    struct desktop *desktop;

    if (!(winstation = get_process_winstation( current->process, 0 ))) return;

    if ((desktop = (struct desktop *)get_handle_obj( current->process, req->handle, 0, &desktop_ops )))
    {
        if (!set_input_desktop( winstation, desktop )) set_error( STATUS_ILLEGAL_FUNCTION );
        release_object( desktop );
    }

    release_object( winstation );
}

/* close a desktop */
DECL_HANDLER(close_desktop)
{
    struct desktop *desktop;

    /* make sure it is a desktop handle */
    if ((desktop = (struct desktop *)get_handle_obj( current->process, req->handle,
                                                     0, &desktop_ops )))
    {
        if (close_handle( current->process, req->handle )) set_error( STATUS_DEVICE_BUSY );
        release_object( desktop );
    }
}


/* get the thread current desktop */
DECL_HANDLER(get_thread_desktop)
{
    struct desktop *desktop;
    struct thread *thread;

    if (!(thread = get_thread_from_id( req->tid ))) return;
    reply->handle = thread->desktop;

    if (!(desktop = get_thread_desktop( thread, 0 ))) clear_error();
    else
    {
        if (desktop->shared) reply->locator = get_shared_object_locator( desktop->shared );
        release_object( desktop );
    }

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
    {
        current->desktop = req->handle;  /* FIXME: should we close the old one? */
        if (old_desktop != new_desktop)
        {
            if (old_desktop) remove_desktop_thread( old_desktop, current );
            add_desktop_thread( new_desktop, current );
        }
        reply->locator = get_shared_object_locator( new_desktop->shared );
    }

    if (!current->process->desktop)
        set_process_default_desktop( current->process, new_desktop, req->handle );

    if (old_desktop != new_desktop && current->queue) detach_thread_input( current );

    if (old_desktop) release_object( old_desktop );
    release_object( new_desktop );
    release_object( winstation );
}


/* get/set information about a user object (window station or desktop) */
DECL_HANDLER(set_user_object_info)
{
    struct object *obj;
    data_size_t len;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL ))) return;

    if (obj->ops == &desktop_ops)
    {
        struct desktop *desktop = (struct desktop *)obj;
        reply->is_desktop = 1;
        reply->old_obj_flags = desktop->shared->flags;
        if (req->flags & SET_USER_OBJECT_SET_FLAGS)
        {
            SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
            {
                shared->flags = req->obj_flags;
            }
            SHARED_WRITE_END;
        }
    }
    else if (obj->ops == &winstation_ops)
    {
        struct winstation *winstation = (struct winstation *)obj;
        reply->is_desktop = 0;
        reply->old_obj_flags = winstation->flags;
        if (req->flags & SET_USER_OBJECT_SET_FLAGS) winstation->flags = req->obj_flags;
    }
    else
    {
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
        return;
    }
    if (obj->ops == &desktop_ops && get_reply_max_size() && (req->flags & SET_USER_OBJECT_GET_FULL_NAME))
    {
        struct desktop *desktop = (struct desktop *)obj;
        data_size_t winstation_len, desktop_len;
        const WCHAR *winstation_name = get_object_name( &desktop->winstation->obj, &winstation_len );
        const WCHAR *desktop_name = get_object_name( obj, &desktop_len );
        WCHAR *full_name;

        if (!winstation_name) winstation_len = 0;
        if (!desktop_name) desktop_len = 0;
        len = winstation_len + desktop_len + sizeof(WCHAR);
        if ((full_name = mem_alloc( len )))
        {
            WCHAR *ptr = mem_append( full_name, winstation_name, winstation_len );
            *ptr++ = '\\';
            mem_append( ptr, desktop_name, desktop_len );
            set_reply_data_ptr( full_name, min( len, get_reply_max_size() ));
        }
    }
    else
    {
        const WCHAR *name = get_object_name( obj, &len );
        if (name) set_reply_data( name, min( len, get_reply_max_size() ));
    }
    release_object( obj );
}


/* enumerate window stations and desktops */
DECL_HANDLER(enum_winstation)
{
    data_size_t size = get_reply_max_size() / sizeof(WCHAR) * sizeof(WCHAR);
    struct object_name *name;
    struct winstation *winstation;
    WCHAR *data = NULL, *ptr;

    if (size && !(data = mem_alloc( size ))) return;
    ptr = data;

    if (req->handle)
    {
        struct desktop *desktop;

        if (!(winstation = (struct winstation *)get_handle_obj( current->process, req->handle,
                                                                WINSTA_ENUMDESKTOPS, &winstation_ops )))
        {
            free( data );
            return;
        }

        LIST_FOR_EACH_ENTRY( desktop, &winstation->desktops, struct desktop, entry )
        {
            unsigned int access = DESKTOP_ENUMERATE;
            if (!(name = desktop->obj.name)) continue;
            if (!check_object_access( NULL, &desktop->obj, &access )) continue;
            reply->count++;
            reply->total += name->len + sizeof(WCHAR);
            if (reply->total <= size)
            {
                ptr = mem_append( ptr, name->name, name->len );
                *ptr++ = 0;
            }
        }
        release_object( winstation );
    }
    else
    {
        LIST_FOR_EACH_ENTRY( winstation, &winstation_list, struct winstation, entry )
        {
            unsigned int access = WINSTA_ENUMERATE;
            if (!(name = winstation->obj.name)) continue;
            if (!check_object_access( NULL, &winstation->obj, &access )) continue;
            reply->count++;
            reply->total += name->len + sizeof(WCHAR);
            if (reply->total <= size)
            {
                ptr = mem_append( ptr, name->name, name->len );
                *ptr++ = 0;
            }
        }
    }

    if (reply->total <= size)
    {
        set_reply_data_ptr( data, reply->total );
        clear_error();
    }
    else
    {
        set_reply_data_ptr( data, (ptr - data) * sizeof(WCHAR) );
        set_error( STATUS_BUFFER_TOO_SMALL );
    }
}

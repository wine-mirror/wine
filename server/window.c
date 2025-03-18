/*
 * Server-side window handling
 *
 * Copyright (C) 2001 Alexandre Julliard
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
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"

#include "object.h"
#include "request.h"
#include "thread.h"
#include "process.h"
#include "user.h"
#include "unicode.h"

/* a window property */
struct property
{
    unsigned short type;     /* property type (see below) */
    atom_t         atom;     /* property atom */
    lparam_t       data;     /* property data (user-defined storage) */
};

enum property_type
{
    PROP_TYPE_FREE,   /* free entry */
    PROP_TYPE_STRING, /* atom that was originally a string */
    PROP_TYPE_ATOM    /* plain atom */
};


struct window
{
    struct object    obj;             /* object header */
    struct window   *parent;          /* parent window */
    user_handle_t    owner;           /* owner of this window */
    struct list      children;        /* list of children in Z-order */
    struct list      unlinked;        /* list of children not linked in the Z-order list */
    struct list      entry;           /* entry in parent's children list */
    user_handle_t    handle;          /* full handle for this window */
    struct thread   *thread;          /* thread owning the window */
    struct desktop  *desktop;         /* desktop that the window belongs to */
    struct window_class *class;       /* window class */
    atom_t           atom;            /* class atom */
    user_handle_t    last_active;     /* last active popup */
    struct rectangle window_rect;     /* window rectangle (relative to parent client area) */
    struct rectangle visible_rect;    /* visible part of window rect (relative to parent client area) */
    struct rectangle surface_rect;    /* window surface rectangle (relative to parent client area) */
    struct rectangle client_rect;     /* client rectangle (relative to parent client area) */
    struct region   *win_region;      /* region for shaped windows (relative to window rect) */
    struct region   *update_region;   /* update region (relative to window rect) */
    unsigned int     style;           /* window style */
    unsigned int     ex_style;        /* window extended style */
    lparam_t         id;              /* window id */
    mod_handle_t     instance;        /* creator instance */
    unsigned int     is_unicode : 1;  /* ANSI or unicode */
    unsigned int     is_linked : 1;   /* is it linked into the parent z-order list? */
    unsigned int     is_layered : 1;  /* has layered info been set? */
    unsigned int     is_orphan : 1;   /* is window orphaned */
    unsigned int     color_key;       /* color key for a layered window */
    unsigned int     alpha;           /* alpha value for a layered window */
    unsigned int     layered_flags;   /* flags for a layered window */
    unsigned int     dpi_context;     /* DPI awareness context */
    unsigned int     monitor_dpi;     /* DPI of the window monitor */
    lparam_t         user_data;       /* user-specific data */
    WCHAR           *text;            /* window caption text */
    data_size_t      text_len;        /* length of window caption */
    unsigned int     paint_flags;     /* various painting flags */
    int              prop_inuse;      /* number of in-use window properties */
    int              prop_alloc;      /* number of allocated window properties */
    struct property *properties;      /* window properties array */
    int              nb_extra_bytes;  /* number of extra bytes */
    char            *extra_bytes;     /* extra bytes storage */
};

static void window_dump( struct object *obj, int verbose );
static void window_destroy( struct object *obj );

static const struct object_ops window_ops =
{
    sizeof(struct window),    /* size */
    &no_type,                 /* type */
    window_dump,              /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    default_map_access,       /* map_access */
    default_get_sd,           /* get_sd */
    default_set_sd,           /* set_sd */
    no_get_full_name,         /* get_full_name */
    no_lookup_name,           /* lookup_name */
    no_link_name,             /* link_name */
    NULL,                     /* unlink_name */
    no_open_file,             /* open_file */
    no_kernel_obj_list,       /* get_kernel_obj_list */
    no_close_handle,          /* close_handle */
    window_destroy            /* destroy */
};

/* flags that can be set by the client */
#define PAINT_HAS_SURFACE          SET_WINPOS_PAINT_SURFACE
#define PAINT_HAS_PIXEL_FORMAT     SET_WINPOS_PIXEL_FORMAT
#define PAINT_HAS_LAYERED_SURFACE  SET_WINPOS_LAYERED_WINDOW
#define PAINT_CLIENT_FLAGS         (PAINT_HAS_SURFACE | PAINT_HAS_PIXEL_FORMAT | PAINT_HAS_LAYERED_SURFACE)
/* flags only manipulated by the server */
#define PAINT_INTERNAL           0x0010  /* internal WM_PAINT pending */
#define PAINT_ERASE              0x0020  /* needs WM_ERASEBKGND */
#define PAINT_NONCLIENT          0x0040  /* needs WM_NCPAINT */
#define PAINT_DELAYED_ERASE      0x0080  /* still needs erase after WM_ERASEBKGND */
#define PAINT_PIXEL_FORMAT_CHILD 0x0100  /* at least one child has a custom pixel format */

/* growable array of user handles */
struct user_handle_array
{
    user_handle_t *handles;
    int            count;
    int            total;
};

static const struct rectangle empty_rect;

/* magic HWND_TOP etc. pointers */
#define WINPTR_TOP       ((struct window *)1L)
#define WINPTR_BOTTOM    ((struct window *)2L)
#define WINPTR_TOPMOST   ((struct window *)3L)
#define WINPTR_NOTOPMOST ((struct window *)4L)

static void window_dump( struct object *obj, int verbose )
{
    struct window *win = (struct window *)obj;
    assert( obj->ops == &window_ops );
    fprintf( stderr, "window %p handle %x\n", win, win->handle );
}

static void window_destroy( struct object *obj )
{
    struct window *win = (struct window *)obj;

    assert( !win->handle );

    if (win->parent)
    {
        list_remove( &win->entry );
        release_object( win->parent );
    }

    if (win->win_region) free_region( win->win_region );
    if (win->update_region) free_region( win->update_region );
    if (win->class) release_class( win->class );
    free( win->text );

    if (win->nb_extra_bytes)
    {
        memset( win->extra_bytes, 0x55, win->nb_extra_bytes );
        free( win->extra_bytes );
    }
}

/* retrieve a pointer to a window from its handle */
static inline struct window *get_window( user_handle_t handle )
{
    struct window *ret = get_user_object( handle, NTUSER_OBJ_WINDOW );
    if (!ret) set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
    return ret;
}

/* check if window is the desktop */
static inline int is_desktop_window( const struct window *win )
{
    return !win->parent;  /* only desktop windows have no parent */
}

/* check if window is orphaned */
static int is_orphan_window( struct window *win )
{
    do if (win->is_orphan) return 1;
    while ((win = win->parent));
    return 0;
}

/* get next window in Z-order list */
static inline struct window *get_next_window( struct window *win )
{
    struct list *ptr = list_next( &win->parent->children, &win->entry );
    return ptr ? LIST_ENTRY( ptr, struct window, entry ) : NULL;
}

/* get previous window in Z-order list */
static inline struct window *get_prev_window( struct window *win )
{
    struct list *ptr = list_prev( &win->parent->children, &win->entry );
    return ptr ? LIST_ENTRY( ptr, struct window, entry ) : NULL;
}

/* get first child in Z-order list */
static inline struct window *get_first_child( struct window *win )
{
    struct list *ptr = list_head( &win->children );
    return ptr ? LIST_ENTRY( ptr, struct window, entry ) : NULL;
}

/* get last child in Z-order list */
static inline struct window *get_last_child( struct window *win )
{
    struct list *ptr = list_tail( &win->children );
    return ptr ? LIST_ENTRY( ptr, struct window, entry ) : NULL;
}

/* set the PAINT_PIXEL_FORMAT_CHILD flag on all the parents */
/* note: we never reset the flag, it's just a heuristic */
static inline void update_pixel_format_flags( struct window *win )
{
    for (win = win->parent; win && win->parent; win = win->parent)
        win->paint_flags |= PAINT_PIXEL_FORMAT_CHILD;
}

static struct rectangle monitors_get_union_rect( struct winstation *winstation, int is_raw )
{
    struct monitor_info *monitor, *end;
    struct rectangle rect = {0};

    for (monitor = winstation->monitors, end = monitor + winstation->monitor_count; monitor < end; monitor++)
    {
        struct rectangle monitor_rect = is_raw ? monitor->raw : monitor->virt;
        if (monitor->flags & (MONITOR_FLAG_CLONE | MONITOR_FLAG_INACTIVE)) continue;
        union_rect( &rect, &rect, &monitor_rect );
    }

    return rect;
}

/* returns the largest intersecting or nearest monitor, keep in sync with win32u/sysparams.c */
static struct monitor_info *get_monitor_from_rect( struct winstation *winstation, const struct rectangle *rect, int is_raw )
{
    struct monitor_info *monitor, *nearest = NULL, *found = NULL, *end;
    unsigned int max_area = 0, min_distance = -1;

    for (monitor = winstation->monitors, end = monitor + winstation->monitor_count; monitor < end; monitor++)
    {
        struct rectangle intersect, target = is_raw ? monitor->raw : monitor->virt;

        if (monitor->flags & (MONITOR_FLAG_CLONE | MONITOR_FLAG_INACTIVE)) continue;

        if (intersect_rect( &intersect, &target, rect ))
        {
            /* check for larger intersecting area */
            unsigned int area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);

            if (area > max_area)
            {
                max_area = area;
                found = monitor;
            }
        }

        if (!found)  /* if not intersecting, check for min distance */
        {
            unsigned int distance, x, y;

            if (rect->right <= target.left) x = target.left - rect->right;
            else if (target.right <= rect->left) x = rect->left - target.right;
            else x = 0;

            if (rect->bottom <= target.top) y = target.top - rect->bottom;
            else if (target.bottom <= rect->top) y = rect->top - target.bottom;
            else y = 0;

            distance = x * x + y * y;
            if (distance < min_distance)
            {
                min_distance = distance;
                nearest = monitor;
            }
        }
    }

    return found ? found : nearest;
}

static void map_point_raw_to_virt( struct desktop *desktop, int *x, int *y )
{
    int width_from, height_from, width_to, height_to;
    struct rectangle rect = {*x, *y, *x + 1, *y + 1};
    struct monitor_info *monitor;

    if (!(monitor = get_monitor_from_rect( desktop->winstation, &rect, 1 ))) return;
    width_to = monitor->virt.right - monitor->virt.left;
    height_to = monitor->virt.bottom - monitor->virt.top;
    width_from = monitor->raw.right - monitor->raw.left;
    height_from = monitor->raw.bottom - monitor->raw.top;

    *x = *x * 2 - (monitor->raw.left * 2 + width_from);
    *x = (*x * width_to * 2 + width_from) / (width_from * 2);
    *x = (*x + monitor->virt.left * 2 + width_to) / 2;

    *y = *y * 2 - (monitor->raw.top * 2 + height_from);
    *y = (*y * height_to * 2 + height_from) / (height_from * 2);
    *y = (*y + monitor->virt.top * 2 + height_to) / 2;
}

/* get the per-monitor DPI for a window */
static unsigned int get_monitor_dpi( struct window *win )
{
    while (win->parent && !is_desktop_window( win->parent )) win = win->parent;
    return win->monitor_dpi;
}

static unsigned int get_window_dpi( struct window *win )
{
    if (NTUSER_DPI_CONTEXT_IS_MONITOR_AWARE( win->dpi_context )) return get_monitor_dpi( win );
    return NTUSER_DPI_CONTEXT_GET_DPI( win->dpi_context );
}

/* link a window at the right place in the siblings list */
static int link_window( struct window *win, struct window *previous )
{
    struct list *old_prev;

    if (previous == WINPTR_NOTOPMOST)
    {
        if (!(win->ex_style & WS_EX_TOPMOST) && win->is_linked) return 0;  /* nothing to do */
        win->ex_style &= ~WS_EX_TOPMOST;
        previous = WINPTR_TOP;  /* fallback to the HWND_TOP case */
    }

    old_prev = win->is_linked ? win->entry.prev : NULL;
    list_remove( &win->entry );  /* unlink it from the previous location */

    if (previous == WINPTR_BOTTOM)
    {
        list_add_tail( &win->parent->children, &win->entry );
        win->ex_style &= ~WS_EX_TOPMOST;
    }
    else if (previous == WINPTR_TOPMOST)
    {
        list_add_head( &win->parent->children, &win->entry );
        win->ex_style |= WS_EX_TOPMOST;
    }
    else if (previous == WINPTR_TOP)
    {
        struct list *entry = win->parent->children.next;
        if (!(win->ex_style & WS_EX_TOPMOST))  /* put it above the first non-topmost window */
        {
            while (entry != &win->parent->children)
            {
                struct window *next = LIST_ENTRY( entry, struct window, entry );
                if (!(next->ex_style & WS_EX_TOPMOST)) break;
                if (next->handle == win->owner)  /* keep it above owner */
                {
                    win->ex_style |= WS_EX_TOPMOST;
                    break;
                }
                entry = entry->next;
            }
        }
        list_add_before( entry, &win->entry );
    }
    else
    {
        list_add_after( &previous->entry, &win->entry );
        if (!(previous->ex_style & WS_EX_TOPMOST)) win->ex_style &= ~WS_EX_TOPMOST;
        else
        {
            struct window *next = get_next_window( win );
            if (next && (next->ex_style & WS_EX_TOPMOST)) win->ex_style |= WS_EX_TOPMOST;
        }
    }

    win->is_linked = 1;
    return old_prev != win->entry.prev;
}

/* change the parent of a window (or unlink the window if the new parent is NULL) */
static int set_parent_window( struct window *win, struct window *parent )
{
    struct window *ptr;

    /* make sure parent is not a child of window */
    for (ptr = parent; ptr; ptr = ptr->parent)
    {
        if (ptr == win)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
    }

    if (parent)
    {
        if (win->parent) release_object( win->parent );
        win->parent = (struct window *)grab_object( parent );
        link_window( win, WINPTR_TOP );

        if (!is_desktop_window( parent )) win->dpi_context = parent->dpi_context;

        /* if parent belongs to a different thread and the window isn't */
        /* top-level, attach the two threads */
        if (parent->thread && parent->thread != win->thread && !is_desktop_window(parent))
            attach_thread_input( win->thread, parent->thread );

        if (win->paint_flags & (PAINT_HAS_PIXEL_FORMAT | PAINT_PIXEL_FORMAT_CHILD))
            update_pixel_format_flags( win );
    }
    else  /* move it to parent unlinked list */
    {
        list_remove( &win->entry );  /* unlink it from the previous location */
        list_add_head( &win->parent->unlinked, &win->entry );
        win->is_linked = 0;
        win->is_orphan = 1;
    }
    return 1;
}

/* append a user handle to a handle array */
static int add_handle_to_array( struct user_handle_array *array, user_handle_t handle )
{
    if (array->count >= array->total)
    {
        int new_total = max( array->total * 2, 32 );
        user_handle_t *new_array = realloc( array->handles, new_total * sizeof(*new_array) );
        if (!new_array)
        {
            free( array->handles );
            set_error( STATUS_NO_MEMORY );
            return 0;
        }
        array->handles = new_array;
        array->total = new_total;
    }
    array->handles[array->count++] = handle;
    return 1;
}

/* set a window property */
static void set_property( struct window *win, atom_t atom, lparam_t data, enum property_type type )
{
    int i, free = -1;
    struct property *new_props;

    /* check if it exists already */
    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE)
        {
            free = i;
            continue;
        }
        if (win->properties[i].atom == atom)
        {
            win->properties[i].type = type;
            win->properties[i].data = data;
            return;
        }
    }

    /* need to add an entry */
    if (!grab_global_atom( NULL, atom )) return;
    if (free == -1)
    {
        /* no free entry */
        if (win->prop_inuse >= win->prop_alloc)
        {
            /* need to grow the array */
            if (!(new_props = realloc( win->properties,
                                       sizeof(*new_props) * (win->prop_alloc + 16) )))
            {
                set_error( STATUS_NO_MEMORY );
                release_global_atom( NULL, atom );
                return;
            }
            win->prop_alloc += 16;
            win->properties = new_props;
        }
        free = win->prop_inuse++;
    }
    win->properties[free].atom = atom;
    win->properties[free].type = type;
    win->properties[free].data = data;
}

/* remove a window property */
static lparam_t remove_property( struct window *win, atom_t atom )
{
    int i;

    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        if (win->properties[i].atom == atom)
        {
            release_global_atom( NULL, atom );
            win->properties[i].type = PROP_TYPE_FREE;
            return win->properties[i].data;
        }
    }
    /* FIXME: last error? */
    return 0;
}

/* find a window property */
static lparam_t get_property( struct window *win, atom_t atom )
{
    int i;

    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        if (win->properties[i].atom == atom) return win->properties[i].data;
    }
    /* FIXME: last error? */
    return 0;
}

/* destroy all properties of a window */
static inline void destroy_properties( struct window *win )
{
    int i;

    if (!win->properties) return;
    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        release_global_atom( NULL, win->properties[i].atom );
    }
    free( win->properties );
}

/* detach a window from its owner thread but keep the window around */
static void detach_window_thread( struct window *win )
{
    struct thread *thread = win->thread;

    if (!thread) return;
    if (thread->queue)
    {
        if (win->update_region) inc_queue_paint_count( thread, -1 );
        if (win->paint_flags & PAINT_INTERNAL) inc_queue_paint_count( thread, -1 );
        queue_cleanup_window( thread, win->handle );
    }
    assert( thread->desktop_users > 0 );
    thread->desktop_users--;
    release_class( win->class );
    win->class = NULL;

    /* don't hold a reference to the desktop so that the desktop window can be */
    /* destroyed when the desktop ref count reaches zero */
    release_object( win->desktop );
    win->thread = NULL;
}

/* get the process owning the top window of a given desktop */
struct process *get_top_window_owner( struct desktop *desktop )
{
    struct window *win = desktop->top_window;
    if (!win || !win->thread) return NULL;
    return win->thread->process;
}

/* get the top window size of a given desktop */
void get_virtual_screen_rect( struct desktop *desktop, struct rectangle *rect, int is_raw )
{
    *rect = monitors_get_union_rect( desktop->winstation, is_raw );
}

/* post a message to the desktop window */
void post_desktop_message( struct desktop *desktop, unsigned int message,
                           lparam_t wparam, lparam_t lparam )
{
    struct window *win = desktop->top_window;
    if (win && win->thread) post_message( win->handle, message, wparam, lparam );
}

/* create a new window structure (note: the window is not linked in the window tree) */
static struct window *create_window( struct window *parent, struct window *owner,
                                     atom_t atom, mod_handle_t instance )
{
    int extra_bytes;
    struct window *win = NULL;
    struct desktop *desktop;
    struct window_class *class;

    if (!(desktop = get_thread_desktop( current, DESKTOP_CREATEWINDOW ))) return NULL;

    if (!(class = grab_class( current->process, atom, instance, &extra_bytes )))
    {
        release_object( desktop );
        return NULL;
    }

    if (!parent)  /* null parent is only allowed for desktop or HWND_MESSAGE top window */
    {
        if (is_desktop_class( class ))
            parent = desktop->top_window;  /* use existing desktop if any */
        else if (is_hwnd_message_class( class ))
            /* use desktop window if message window is already created */
            parent = desktop->msg_window ? desktop->top_window : NULL;
        else if (!(parent = desktop->top_window))  /* must already have a desktop then */
        {
            set_error( STATUS_ACCESS_DENIED );
            goto failed;
        }
    }

    /* parent must be on the same desktop */
    if (parent && parent->desktop != desktop)
    {
        set_error( STATUS_ACCESS_DENIED );
        goto failed;
    }

    if (!(win = alloc_object( &window_ops ))) goto failed;
    win->parent         = parent ? (struct window *)grab_object( parent ) : NULL;
    win->owner          = owner ? owner->handle : 0;
    win->thread         = current;
    win->desktop        = desktop;
    win->class          = class;
    win->atom           = atom;
    win->win_region     = NULL;
    win->update_region  = NULL;
    win->style          = 0;
    win->ex_style       = 0;
    win->id             = 0;
    win->instance       = 0;
    win->is_unicode     = 1;
    win->is_linked      = 0;
    win->is_layered     = 0;
    win->is_orphan      = 0;
    win->dpi_context    = NTUSER_DPI_PER_MONITOR_AWARE;
    win->monitor_dpi    = USER_DEFAULT_SCREEN_DPI;
    win->user_data      = 0;
    win->text           = NULL;
    win->text_len       = 0;
    win->paint_flags    = 0;
    win->prop_inuse     = 0;
    win->prop_alloc     = 0;
    win->properties     = NULL;
    win->nb_extra_bytes = 0;
    win->extra_bytes    = NULL;
    win->window_rect = win->visible_rect = win->surface_rect = win->client_rect = empty_rect;
    list_init( &win->children );
    list_init( &win->unlinked );

    if (extra_bytes)
    {
        if (!(win->extra_bytes = mem_alloc( extra_bytes ))) goto failed;
        memset( win->extra_bytes, 0, extra_bytes );
        win->nb_extra_bytes = extra_bytes;
    }
    if (!(win->handle = alloc_user_handle( win, NTUSER_OBJ_WINDOW ))) goto failed;
    win->last_active = win->handle;

    /* if parent belongs to a different thread and the window isn't */
    /* top-level, attach the two threads */
    if (parent && parent->thread && parent->thread != current && !is_desktop_window(parent))
    {
        if (!attach_thread_input( current, parent->thread )) goto failed;
    }
    else  /* otherwise just make sure that the thread has a message queue */
    {
        if (!current->queue && !init_thread_queue( current )) goto failed;
    }

    /* put it on parent unlinked list */
    if (parent) list_add_head( &parent->unlinked, &win->entry );
    else
    {
        list_init( &win->entry );
        if (is_desktop_class( class ))
        {
            assert( !desktop->top_window );
            desktop->top_window = win;
            set_process_default_desktop( current->process, desktop, current->desktop );
        }
        else
        {
            assert( !desktop->msg_window );
            desktop->msg_window = win;
        }
    }

    current->desktop_users++;
    return win;

failed:
    if (win)
    {
        if (win->handle)
        {
            free_user_handle( win->handle );
            win->handle = 0;
        }
        release_object( win );
    }
    release_object( desktop );
    release_class( class );
    return NULL;
}

/* destroy all windows belonging to a given thread */
void destroy_thread_windows( struct thread *thread )
{
    user_handle_t handle = 0;
    struct window *win;

    while ((win = next_user_handle( &handle, NTUSER_OBJ_WINDOW )))
    {
        if (win->thread != thread) continue;
        if (is_desktop_window( win )) detach_window_thread( win );
        else free_window_handle( win );
    }
}

/* get the desktop window */
static struct window *get_desktop_window( struct thread *thread )
{
    struct window *top_window;
    struct desktop *desktop = get_thread_desktop( thread, 0 );

    if (!desktop) return NULL;
    top_window = desktop->top_window;
    release_object( desktop );
    return top_window;
}

/* check whether child is a descendant of parent */
int is_child_window( user_handle_t parent, user_handle_t child )
{
    struct window *child_ptr = get_user_object( child, NTUSER_OBJ_WINDOW );
    struct window *parent_ptr = get_user_object( parent, NTUSER_OBJ_WINDOW );

    if (!child_ptr || !parent_ptr) return 0;
    while (child_ptr->parent)
    {
        if (child_ptr->parent == parent_ptr) return 1;
        child_ptr = child_ptr->parent;
    }
    return 0;
}

/* check if window can be set as foreground window */
int is_valid_foreground_window( user_handle_t window )
{
    struct window *win = get_user_object( window, NTUSER_OBJ_WINDOW );
    return win && (win->style & (WS_POPUP|WS_CHILD)) != WS_CHILD;
}

/* make a window active if possible */
int make_window_active( user_handle_t window )
{
    struct window *owner, *win = get_window( window );

    if (!win) return 0;

    /* set last active for window and its owners */
    owner = win;
    while (owner)
    {
        owner->last_active = win->handle;
        owner = get_user_object( owner->owner, NTUSER_OBJ_WINDOW );
    }
    return 1;
}

/* increment (or decrement) the window paint count */
static inline void inc_window_paint_count( struct window *win, int incr )
{
    if (win->thread) inc_queue_paint_count( win->thread, incr );
}

/* map a point between different DPI scaling levels */
static void map_dpi_point( struct window *win, int *x, int *y, unsigned int from, unsigned int to )
{
    if (!from) from = get_monitor_dpi( win );
    if (!to) to = get_monitor_dpi( win );
    if (from == to) return;
    *x = scale_dpi( *x, from, to );
    *y = scale_dpi( *y, from, to );
}

/* map a window rectangle between different DPI scaling levels */
static void map_dpi_rect( struct window *win, struct rectangle *rect, unsigned int from, unsigned int to )
{
    if (!from) from = get_monitor_dpi( win );
    if (!to) to = get_monitor_dpi( win );
    if (from == to) return;
    scale_dpi_rect( rect, from, to );
}

/* map a region between different DPI scaling levels */
static void map_dpi_region( struct window *win, struct region *region, unsigned int from, unsigned int to )
{
    if (!from) from = get_monitor_dpi( win );
    if (!to) to = get_monitor_dpi( win );
    if (from == to) return;
    scale_region( region, from, to );
}

/* convert coordinates from client to screen coords */
static inline void client_to_screen( struct window *win, int *x, int *y )
{
    for ( ; win && !is_desktop_window(win); win = win->parent)
    {
        *x += win->client_rect.left;
        *y += win->client_rect.top;
    }
}

/* convert coordinates from screen to client coords and dpi */
static void screen_to_client( struct window *win, int *x, int *y, unsigned int dpi )
{
    int offset_x = 0, offset_y = 0;

    if (is_desktop_window( win )) return;

    client_to_screen( win, &offset_x, &offset_y );
    map_dpi_point( win, x, y, dpi, get_window_dpi( win ) );
    *x -= offset_x;
    *y -= offset_y;
}

/* check if window and all its ancestors are visible */
static int is_visible( const struct window *win )
{
    while (win)
    {
        if (!(win->style & WS_VISIBLE)) return 0;
        win = win->parent;
        /* if parent is minimized children are not visible */
        if (win && (win->style & WS_MINIMIZE)) return 0;
    }
    return 1;
}

/* same as is_visible but takes a window handle */
int is_window_visible( user_handle_t window )
{
    struct window *win = get_user_object( window, NTUSER_OBJ_WINDOW );
    if (!win) return 0;
    return is_visible( win );
}

int is_window_transparent( user_handle_t window )
{
    struct window *win = get_user_object( window, NTUSER_OBJ_WINDOW );
    if (!win) return 0;
    return (win->ex_style & (WS_EX_LAYERED|WS_EX_TRANSPARENT)) == (WS_EX_LAYERED|WS_EX_TRANSPARENT);
}

static int is_window_using_parent_dc( struct window *win )
{
    return (win->style & (WS_POPUP|WS_CHILD)) == WS_CHILD && (get_class_style( win->class ) & CS_PARENTDC) != 0;
}

static int is_window_composited( struct window *win )
{
    return (win->ex_style & WS_EX_COMPOSITED) != 0 && !is_window_using_parent_dc(win);
}

static int is_parent_composited( struct window *win )
{
    return win->parent && is_window_composited( win->parent );
}

/* check if point is inside the window, and map to window dpi */
static int is_point_in_window( struct window *win, int *x, int *y, unsigned int dpi )
{
    if (!(win->style & WS_VISIBLE)) return 0; /* not visible */
    if ((win->style & (WS_POPUP|WS_CHILD|WS_DISABLED)) == (WS_CHILD|WS_DISABLED))
        return 0;  /* disabled child */
    if ((win->ex_style & (WS_EX_LAYERED|WS_EX_TRANSPARENT)) == (WS_EX_LAYERED|WS_EX_TRANSPARENT))
        return 0;  /* transparent */
    map_dpi_point( win, x, y, dpi, get_window_dpi( win ) );
    if (!point_in_rect( &win->visible_rect, *x, *y ))
        return 0;  /* not in window */
    if (win->win_region &&
        !point_in_region( win->win_region, *x - win->window_rect.left, *y - win->window_rect.top ))
        return 0;  /* not in window region */
    return 1;
}

/* helper for get_window_list */
static void append_window_to_list( struct window *win, struct thread *thread, atom_t atom,
                                   user_handle_t *handles, unsigned int *count, unsigned int max_count )
{
    if (thread && win->thread != thread) return;
    if (atom && get_class_atom( win->class ) != atom) return;
    if (*count < max_count) handles[*count] = win->handle;
    (*count)++;
}

/* fill an array with the handles of siblings or children */
static void get_window_list( struct desktop *desktop, struct window *win, struct thread *thread,
                             int children, user_handle_t *handles,
                             unsigned int *count, unsigned int max_count )
{
    struct window *child;

    if (desktop)  /* top-level windows of specified desktop */
    {
        if (children) return;
        if (!desktop->top_window) return;
        LIST_FOR_EACH_ENTRY( child, &desktop->top_window->children, struct window, entry )
            append_window_to_list( child, thread, 0, handles, count, max_count );
    }
    else if (!win)  /* top-level windows of current desktop */
    {
        if (!(win = get_desktop_window( current ))) return;
        LIST_FOR_EACH_ENTRY( child, &win->children, struct window, entry )
            append_window_to_list( child, thread, 0, handles, count, max_count );
    }
    else if (children)  /* children (recursively) of specified window */
    {
        LIST_FOR_EACH_ENTRY( child, &win->children, struct window, entry )
        {
            append_window_to_list( child, thread, 0, handles, count, max_count );
            get_window_list( NULL, child, thread, TRUE, handles, count, max_count );
        }
    }
    else if (!is_desktop_window( win ))  /* siblings starting from specified window */
    {
        for (child = win; child; child = get_next_window( child ))
            append_window_to_list( child, thread, 0, handles, count, max_count );
    }
    else  /* desktop window siblings */
    {
        append_window_to_list( win, thread, 0, handles, count, max_count );
        if (win == win->desktop->top_window && win->desktop->msg_window)
            append_window_to_list( win->desktop->msg_window, thread, 0, handles, count, max_count );
    }
}

/* find child of 'parent' that contains the given point (in parent-relative coords) */
static struct window *child_window_from_point( struct window *parent, int x, int y )
{
    struct window *ptr;

    LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
    {
        int x_child = x, y_child = y;

        if (!is_point_in_window( ptr, &x_child, &y_child, get_window_dpi( parent ) )) continue;  /* skip it */

        /* if window is minimized or disabled, return at once */
        if (ptr->style & (WS_MINIMIZE|WS_DISABLED)) return ptr;

        /* if point is not in client area, return at once */
        if (!point_in_rect( &ptr->client_rect, x_child, y_child )) return ptr;

        return child_window_from_point( ptr, x_child - ptr->client_rect.left,
                                        y_child - ptr->client_rect.top );
    }
    return parent;  /* not found any child */
}

/* find all children of 'parent' that contain the given point */
static int get_window_children_from_point( struct window *parent, int x, int y,
                                           struct user_handle_array *array )
{
    struct window *ptr;

    LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
    {
        int x_child = x, y_child = y;

        if (!is_point_in_window( ptr, &x_child, &y_child, get_window_dpi( parent ) )) continue;  /* skip it */

        /* if point is in client area, and window is not minimized or disabled, check children */
        if (!(ptr->style & (WS_MINIMIZE|WS_DISABLED)) && point_in_rect( &ptr->client_rect, x_child, y_child ))
        {
            if (!get_window_children_from_point( ptr, x_child - ptr->client_rect.left,
                                                 y_child - ptr->client_rect.top, array ))
                return 0;
        }

        /* now add window to the array */
        if (!add_handle_to_array( array, ptr->handle )) return 0;
    }
    return 1;
}

/* get handle of root of top-most window containing point (in absolute raw coords) */
user_handle_t shallow_window_from_point( struct desktop *desktop, int x, int y )
{
    struct window *ptr;

    if (!desktop->top_window) return 0;

    map_point_raw_to_virt( desktop, &x, &y );

    LIST_FOR_EACH_ENTRY( ptr, &desktop->top_window->children, struct window, entry )
    {
        int x_child = x, y_child = y;

        if (!is_point_in_window( ptr, &x_child, &y_child, 0 )) continue;  /* skip it */
        return ptr->handle;
    }
    return desktop->top_window->handle;
}

/* return thread of top-most window containing point (in absolute raw coords) */
struct thread *window_thread_from_point( user_handle_t scope, int x, int y )
{
    struct window *win = get_user_object( scope, NTUSER_OBJ_WINDOW );

    if (!win) return NULL;

    map_point_raw_to_virt( win->desktop, &x, &y );

    screen_to_client( win, &x, &y, 0 );
    win = child_window_from_point( win, x, y );
    if (!win->thread) return NULL;
    return (struct thread *)grab_object( win->thread );
}

/* return list of all windows containing point (in absolute coords) */
static int all_windows_from_point( struct window *top, int x, int y, unsigned int dpi,
                                   struct user_handle_array *array )
{
    if (!is_desktop_window( top ) && !is_desktop_window( top->parent ))
    {
        screen_to_client( top->parent, &x, &y, dpi );
        dpi = get_window_dpi( top->parent );
    }

    if (!is_point_in_window( top, &x, &y, dpi )) return 1;
    /* if point is in client area, and window is not minimized or disabled, check children */
    if (!(top->style & (WS_MINIMIZE|WS_DISABLED)) && point_in_rect( &top->client_rect, x, y ))
    {
        if (!is_desktop_window(top))
        {
            x -= top->client_rect.left;
            y -= top->client_rect.top;
        }
        if (!get_window_children_from_point( top, x, y, array )) return 0;
    }
    /* now add window to the array */
    if (!add_handle_to_array( array, top->handle )) return 0;
    return 1;
}


/* return the thread owning a window */
struct thread *get_window_thread( user_handle_t handle )
{
    struct window *win = get_user_object( handle, NTUSER_OBJ_WINDOW );
    if (!win || !win->thread) return NULL;
    return (struct thread *)grab_object( win->thread );
}


/* check if any area of a window needs repainting */
static inline int win_needs_repaint( struct window *win )
{
    return win->update_region || (win->paint_flags & PAINT_INTERNAL);
}


/* find a child of the specified window that needs repainting */
static struct window *find_child_to_repaint( struct window *parent, struct thread *thread )
{
    struct window *ptr, *ret = NULL;

    LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
    {
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->thread == thread && win_needs_repaint( ptr ))
            ret = ptr;
        else if (!(ptr->style & WS_MINIMIZE)) /* explore its children */
            ret = find_child_to_repaint( ptr, thread );
        if (ret) break;
    }

    if (ret && (ret->ex_style & WS_EX_TRANSPARENT))
    {
        /* transparent window, check for non-transparent sibling to paint first */
        for (ptr = get_next_window(ret); ptr; ptr = get_next_window(ptr))
        {
            if (!(ptr->style & WS_VISIBLE)) continue;
            if (ptr->ex_style & WS_EX_TRANSPARENT) continue;
            if (ptr->thread != thread) continue;
            if (win_needs_repaint( ptr )) return ptr;
        }
    }
    return ret;
}


/* find a window that needs to receive a WM_PAINT; also clear its internal paint flag */
user_handle_t find_window_to_repaint( user_handle_t parent, struct thread *thread )
{
    struct window *ptr, *win, *top_window = get_desktop_window( thread );

    if (!top_window) return 0;

    if (top_window->thread == thread && win_needs_repaint( top_window )) win = top_window;
    else win = find_child_to_repaint( top_window, thread );

    if (win && parent)
    {
        /* check that it is a child of the specified parent */
        for (ptr = win; ptr; ptr = ptr->parent)
            if (ptr->handle == parent) break;
        /* otherwise don't return any window, we don't repaint a child before its parent */
        if (!ptr) win = NULL;
    }
    if (!win) return 0;
    win->paint_flags &= ~PAINT_INTERNAL;
    return win->handle;
}


/* intersect the window region with the specified region, relative to the window parent */
static struct region *intersect_window_region( struct region *region, struct window *win )
{
    /* make region relative to window rect */
    offset_region( region, -win->window_rect.left, -win->window_rect.top );
    if (!intersect_region( region, region, win->win_region )) return NULL;
    /* make region relative to parent again */
    offset_region( region, win->window_rect.left, win->window_rect.top );
    return region;
}


/* convert coordinates from client to screen coords */
static inline void client_to_screen_rect( struct window *win, struct rectangle *rect )
{
    for ( ; win && !is_desktop_window(win); win = win->parent)
        offset_rect( rect, win->client_rect.left, win->client_rect.top );
}

/* map the region from window to screen coordinates */
static inline void map_win_region_to_screen( struct window *win, struct region *region )
{
    if (!is_desktop_window(win))
    {
        int x = win->window_rect.left;
        int y = win->window_rect.top;
        client_to_screen( win->parent, &x, &y );
        offset_region( region, x, y );
    }
}


/* clip all children of a given window out of the visible region */
static struct region *clip_children( struct window *parent, struct window *last,
                                     struct region *region, int offset_x, int offset_y )
{
    struct window *ptr;
    struct region *tmp = create_empty_region();

    if (!tmp) return NULL;
    LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
    {
        if (ptr == last) break;
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->ex_style & WS_EX_TRANSPARENT) continue;
        set_region_rect( tmp, &ptr->visible_rect );
        if (ptr->win_region && !intersect_window_region( tmp, ptr ))
        {
            free_region( tmp );
            return NULL;
        }
        offset_region( tmp, offset_x, offset_y );
        if (!(region = subtract_region( region, region, tmp ))) break;
        if (is_region_empty( region )) break;
    }
    free_region( tmp );
    return region;
}


/* set the region to the client rect clipped by the window rect, in parent-relative coordinates */
static void set_region_client_rect( struct region *region, struct window *win )
{
    struct rectangle rect;

    intersect_rect( &rect, &win->window_rect, &win->client_rect );
    intersect_rect( &rect, &rect, &win->surface_rect );
    set_region_rect( region, &rect );
}


/* set the region to the visible rect clipped by the window surface, in parent-relative coordinates */
static void set_region_visible_rect( struct region *region, struct window *win )
{
    struct rectangle rect;

    intersect_rect( &rect, &win->visible_rect, &win->surface_rect );
    set_region_rect( region, &rect );
}


/* get the top-level window to clip against for a given window */
static inline struct window *get_top_clipping_window( struct window *win )
{
    while (!(win->paint_flags & PAINT_HAS_SURFACE) && win->parent && !is_desktop_window(win->parent))
        win = win->parent;
    return win;
}


/* compute the visible region of a window, in window coordinates */
static struct region *get_visible_region( struct window *win, unsigned int flags )
{
    struct region *tmp = NULL, *region;
    int offset_x, offset_y;

    if (!(region = create_empty_region())) return NULL;

    /* first check if all ancestors are visible */

    if (!is_visible( win )) return region;  /* empty region */

    if (is_desktop_window( win ))
    {
        set_region_rect( region, &win->window_rect );
        return region;
    }

    /* create a region relative to the window itself */

    if ((flags & DCX_PARENTCLIP) && !is_desktop_window( win->parent ))
    {
        set_region_client_rect( region, win->parent );
        offset_region( region, -win->parent->client_rect.left, -win->parent->client_rect.top );
    }
    else if (flags & DCX_WINDOW)
    {
        set_region_visible_rect( region, win );
        if (win->win_region && !intersect_window_region( region, win )) goto error;
    }
    else
    {
        set_region_client_rect( region, win );
        if (win->win_region && !intersect_window_region( region, win )) goto error;
    }

    /* clip children */

    if (flags & DCX_CLIPCHILDREN)
    {
        if (!clip_children( win, NULL, region, win->client_rect.left, win->client_rect.top )) goto error;
    }

    /* clip siblings of ancestors */

    offset_x = win->window_rect.left;
    offset_y = win->window_rect.top;

    if ((tmp = create_empty_region()) != NULL)
    {
        while (!is_desktop_window( win->parent ))
        {
            /* we don't clip out top-level siblings as that's up to the native windowing system */
            if (win->style & WS_CLIPSIBLINGS)
            {
                if (!clip_children( win->parent, win, region, 0, 0 )) goto error;
                if (is_region_empty( region )) break;
            }
            /* clip to parent client area */
            win = win->parent;
            offset_x += win->client_rect.left;
            offset_y += win->client_rect.top;
            offset_region( region, win->client_rect.left, win->client_rect.top );
            set_region_client_rect( tmp, win );
            if (win->win_region && !intersect_window_region( tmp, win )) goto error;
            if (!intersect_region( region, region, tmp )) goto error;
            if (is_region_empty( region )) break;
        }
        free_region( tmp );
    }
    offset_region( region, -offset_x, -offset_y );  /* make it relative to target window */
    return region;

error:
    if (tmp) free_region( tmp );
    free_region( region );
    return NULL;
}


/* clip all children with a custom pixel format out of the visible region */
static struct region *clip_pixel_format_children( struct window *parent, struct region *parent_clip,
                                                  struct region *region, int offset_x, int offset_y )
{
    struct window *ptr;
    struct region *clip = create_empty_region();

    if (!clip) return NULL;

    LIST_FOR_EACH_ENTRY_REV( ptr, &parent->children, struct window, entry )
    {
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->ex_style & WS_EX_TRANSPARENT) continue;

        /* add the visible rect */
        set_region_rect( clip, &ptr->visible_rect );
        if (ptr->win_region && !intersect_window_region( clip, ptr )) break;
        offset_region( clip, offset_x, offset_y );
        if (!intersect_region( clip, clip, parent_clip )) break;
        if (!union_region( region, region, clip )) break;
        if (!(ptr->paint_flags & (PAINT_HAS_PIXEL_FORMAT | PAINT_PIXEL_FORMAT_CHILD))) continue;

        /* subtract the client rect if it uses a custom pixel format */
        set_region_rect( clip, &ptr->client_rect );
        if (ptr->win_region && !intersect_window_region( clip, ptr )) break;
        offset_region( clip, offset_x, offset_y );
        if (!intersect_region( clip, clip, parent_clip )) break;
        if ((ptr->paint_flags & PAINT_HAS_PIXEL_FORMAT) && !subtract_region( region, region, clip ))
            break;

        if (!clip_pixel_format_children( ptr, clip, region, offset_x + ptr->client_rect.left,
                                         offset_y + ptr->client_rect.top ))
            break;
    }
    free_region( clip );
    return region;
}


/* compute the visible surface region of a window, in parent coordinates */
static struct region *get_surface_region( struct window *win )
{
    struct region *region, *clip;
    int offset_x, offset_y;

    /* create a region relative to the window itself */

    if (!(region = create_empty_region())) return NULL;
    if (!(clip = create_empty_region())) goto error;
    set_region_rect( region, &win->visible_rect );
    if (win->win_region && !intersect_window_region( region, win )) goto error;
    set_region_rect( clip, &win->client_rect );
    if (win->win_region && !intersect_window_region( clip, win )) goto error;

    if ((win->paint_flags & PAINT_HAS_PIXEL_FORMAT) && !subtract_region( region, region, clip ))
        goto error;

    /* clip children */

    if (!is_desktop_window(win))
    {
        offset_x = win->client_rect.left;
        offset_y = win->client_rect.top;
    }
    else offset_x = offset_y = 0;

    if (!clip_pixel_format_children( win, clip, region, offset_x, offset_y )) goto error;

    free_region( clip );
    return region;

error:
    if (clip) free_region( clip );
    free_region( region );
    return NULL;
}


/* get the window class of a window */
struct window_class* get_window_class( user_handle_t window )
{
    struct window *win;
    if (!(win = get_window( window ))) return NULL;
    if (!win->class) set_error( STATUS_ACCESS_DENIED );
    return win->class;
}

/* determine the window visible rectangle, i.e. window or client rect cropped by parent rects */
/* the returned rectangle is in window coordinates; return 0 if rectangle is empty */
static int get_window_visible_rect( struct window *win, struct rectangle *rect, int frame )
{
    int offset_x = win->window_rect.left, offset_y = win->window_rect.top;

    *rect = frame ? win->window_rect : win->client_rect;

    if (!(win->style & WS_VISIBLE)) return 0;
    if (is_desktop_window( win )) return 1;

    while (!is_desktop_window( win->parent ))
    {
        win = win->parent;
        if (!(win->style & WS_VISIBLE) || win->style & WS_MINIMIZE) return 0;
        offset_x += win->client_rect.left;
        offset_y += win->client_rect.top;
        offset_rect( rect, win->client_rect.left, win->client_rect.top );
        if (!intersect_rect( rect, rect, &win->client_rect )) return 0;
        if (!intersect_rect( rect, rect, &win->window_rect )) return 0;
    }
    offset_rect( rect, -offset_x, -offset_y );
    return 1;
}

/* return a copy of the specified region cropped to the window client or frame rectangle, */
/* and converted from client to window coordinates. Helper for (in)validate_window. */
static struct region *crop_region_to_win_rect( struct window *win, struct region *region, int frame )
{
    struct rectangle rect;
    struct region *tmp;

    if (!get_window_visible_rect( win, &rect, frame )) return NULL;
    if (!(tmp = create_empty_region())) return NULL;
    set_region_rect( tmp, &rect );

    if (region)
    {
        /* map it to client coords */
        offset_region( tmp, win->window_rect.left - win->client_rect.left,
                       win->window_rect.top - win->client_rect.top );

        /* intersect specified region with bounding rect */
        if (!intersect_region( tmp, region, tmp )) goto done;
        if (is_region_empty( tmp )) goto done;

        /* map it back to window coords */
        offset_region( tmp, win->client_rect.left - win->window_rect.left,
                       win->client_rect.top - win->window_rect.top );
    }
    return tmp;

done:
    free_region( tmp );
    return NULL;
}


/* set a region as new update region for the window */
static void set_update_region( struct window *win, struct region *region )
{
    if (region && !is_region_empty( region ))
    {
        if (!win->update_region) inc_window_paint_count( win, 1 );
        else free_region( win->update_region );
        win->update_region = region;
    }
    else
    {
        if (win->update_region)
        {
            inc_window_paint_count( win, -1 );
            free_region( win->update_region );
        }
        win->paint_flags &= ~(PAINT_ERASE | PAINT_DELAYED_ERASE | PAINT_NONCLIENT);
        win->update_region = NULL;
        if (region) free_region( region );
    }
}


/* add a region to the update region; the passed region is freed or reused */
static int add_update_region( struct window *win, struct region *region )
{
    if (win->update_region && !union_region( region, win->update_region, region ))
    {
        free_region( region );
        return 0;
    }
    set_update_region( win, region );
    return 1;
}


/* crop the update region of children to the specified rectangle, in client coords */
static void crop_children_update_region( struct window *win, struct rectangle *rect )
{
    struct window *child;
    struct region *tmp;
    struct rectangle child_rect;

    LIST_FOR_EACH_ENTRY( child, &win->children, struct window, entry )
    {
        if (!(child->style & WS_VISIBLE)) continue;
        if (!rect)  /* crop everything out */
        {
            crop_children_update_region( child, NULL );
            set_update_region( child, NULL );
            continue;
        }

        /* nothing to do if child is completely inside rect */
        if (child->window_rect.left >= rect->left &&
            child->window_rect.top >= rect->top &&
            child->window_rect.right <= rect->right &&
            child->window_rect.bottom <= rect->bottom) continue;

        /* map to child client coords and crop grand-children */
        child_rect = *rect;
        offset_rect( &child_rect, -child->client_rect.left, -child->client_rect.top );
        crop_children_update_region( child, &child_rect );

        /* now crop the child itself */
        if (!child->update_region) continue;
        if (!(tmp = create_empty_region())) continue;
        set_region_rect( tmp, rect );
        offset_region( tmp, -child->window_rect.left, -child->window_rect.top );
        if (intersect_region( tmp, child->update_region, tmp )) set_update_region( child, tmp );
        else free_region( tmp );
    }
}


/* validate the non client area of a window */
static void validate_non_client( struct window *win )
{
    struct region *tmp;
    struct rectangle rect;

    if (!win->update_region) return;  /* nothing to do */

    /* get client rect in window coords */
    rect.left   = win->client_rect.left - win->window_rect.left;
    rect.top    = win->client_rect.top - win->window_rect.top;
    rect.right  = win->client_rect.right - win->window_rect.left;
    rect.bottom = win->client_rect.bottom - win->window_rect.top;

    if ((tmp = create_empty_region()))
    {
        set_region_rect( tmp, &rect );
        if (intersect_region( tmp, win->update_region, tmp ))
            set_update_region( win, tmp );
        else
            free_region( tmp );
    }
    win->paint_flags &= ~PAINT_NONCLIENT;
}


/* validate a window completely so that we don't get any further paint messages for it */
static void validate_whole_window( struct window *win )
{
    set_update_region( win, NULL );

    if (win->paint_flags & PAINT_INTERNAL)
    {
        win->paint_flags &= ~PAINT_INTERNAL;
        inc_window_paint_count( win, -1 );
    }
}


/* validate a window's children so that we don't get any further paint messages for it */
static void validate_children( struct window *win )
{
    struct window *child;

    LIST_FOR_EACH_ENTRY( child, &win->children, struct window, entry )
    {
        if (!(child->style & WS_VISIBLE)) continue;
        validate_children(child);
        validate_whole_window(child);
    }
}


/* validate the update region of a window on all parents; helper for get_update_region */
static void validate_parents( struct window *child )
{
    int offset_x = 0, offset_y = 0;
    struct window *win = child;
    struct region *tmp = NULL;

    if (!child->update_region) return;

    while (win->parent)
    {
        /* map to parent client coords */
        offset_x += win->window_rect.left;
        offset_y += win->window_rect.top;

        win = win->parent;

        /* and now map to window coords */
        offset_x += win->client_rect.left - win->window_rect.left;
        offset_y += win->client_rect.top - win->window_rect.top;

        if (win->update_region && !(win->style & WS_CLIPCHILDREN))
        {
            if (!tmp && !(tmp = create_empty_region())) return;
            offset_region( child->update_region, offset_x, offset_y );
            if (subtract_region( tmp, win->update_region, child->update_region ))
            {
                set_update_region( win, tmp );
                tmp = NULL;
            }
            /* restore child coords */
            offset_region( child->update_region, -offset_x, -offset_y );
        }
    }
    if (tmp) free_region( tmp );
}


/* add/subtract a region (in client coordinates) to the update region of the window */
static void redraw_window( struct window *win, struct region *region, int frame, unsigned int flags )
{
    struct region *child_rgn, *tmp;
    struct window *child;

    if (flags & RDW_INVALIDATE)
    {
        if (!(tmp = crop_region_to_win_rect( win, region, frame ))) return;

        if (!add_update_region( win, tmp )) return;

        if (flags & RDW_FRAME) win->paint_flags |= PAINT_NONCLIENT;
        if (flags & RDW_ERASE) win->paint_flags |= PAINT_ERASE;
    }
    else if (flags & RDW_VALIDATE)
    {
        if (!region && (flags & RDW_NOFRAME))  /* shortcut: validate everything */
        {
            set_update_region( win, NULL );
        }
        else if (win->update_region)
        {
            if ((tmp = crop_region_to_win_rect( win, region, frame )))
            {
                if (!subtract_region( tmp, win->update_region, tmp ))
                {
                    free_region( tmp );
                    return;
                }
                set_update_region( win, tmp );
            }
            if (flags & RDW_NOFRAME) validate_non_client( win );
            if (flags & RDW_NOERASE) win->paint_flags &= ~(PAINT_ERASE | PAINT_DELAYED_ERASE);
        }
    }

    if ((flags & RDW_INTERNALPAINT) && !(win->paint_flags & PAINT_INTERNAL))
    {
        win->paint_flags |= PAINT_INTERNAL;
        inc_window_paint_count( win, 1 );
    }
    else if ((flags & RDW_NOINTERNALPAINT) && (win->paint_flags & PAINT_INTERNAL))
    {
        win->paint_flags &= ~PAINT_INTERNAL;
        inc_window_paint_count( win, -1 );
    }

    /* now process children recursively */

    if (flags & RDW_NOCHILDREN) return;
    if (win->style & WS_MINIMIZE) return;
    if ((win->style & WS_CLIPCHILDREN) && !(flags & RDW_ALLCHILDREN)) return;

    if (!(tmp = crop_region_to_win_rect( win, region, 0 ))) return;

    /* map to client coordinates */
    offset_region( tmp, win->window_rect.left - win->client_rect.left,
                   win->window_rect.top - win->client_rect.top );

    if (flags & RDW_INVALIDATE) flags |= RDW_FRAME | RDW_ERASE;

    LIST_FOR_EACH_ENTRY( child, &win->children, struct window, entry )
    {
        if (!(child->style & WS_VISIBLE)) continue;
        if (!(child_rgn = create_empty_region())) continue;
        if (copy_region( child_rgn, tmp ))
        {
            map_dpi_region( child, child_rgn, get_window_dpi( win ), get_window_dpi( child ) );
            if (rect_in_region( child_rgn, &child->window_rect ))
            {
                offset_region( child_rgn, -child->client_rect.left, -child->client_rect.top );
                redraw_window( child, child_rgn, 1, flags );
            }
        }
        free_region( child_rgn );
    }

    free_region( tmp );
}


/* retrieve the update flags for a window depending on the state of the update region */
static unsigned int get_update_flags( struct window *win, unsigned int flags )
{
    unsigned int ret = 0;

    if (flags & UPDATE_NONCLIENT)
    {
        if ((win->paint_flags & PAINT_NONCLIENT) && win->update_region) ret |= UPDATE_NONCLIENT;
    }
    if (flags & UPDATE_ERASE)
    {
        if ((win->paint_flags & PAINT_ERASE) && win->update_region) ret |= UPDATE_ERASE;
    }
    if (flags & UPDATE_PAINT)
    {
        if (win->update_region)
        {
            if (win->paint_flags & PAINT_DELAYED_ERASE) ret |= UPDATE_DELAYED_ERASE;
            ret |= UPDATE_PAINT;
        }
    }
    if (flags & UPDATE_INTERNALPAINT)
    {
        if (win->paint_flags & PAINT_INTERNAL)
        {
            ret |= UPDATE_INTERNALPAINT;
            if (win->paint_flags & PAINT_DELAYED_ERASE) ret |= UPDATE_DELAYED_ERASE;
        }
    }
    return ret;
}


/* iterate through the children of the given window until we find one with some update flags */
static unsigned int get_child_update_flags( struct window *win, struct window *from_child,
                                            unsigned int flags, struct window **child )
{
    struct window *ptr;
    unsigned int ret = 0;

    /* first make sure we want to iterate children at all */

    if (win->style & WS_MINIMIZE) return 0;

    /* note: the WS_CLIPCHILDREN test is the opposite of the invalidation case,
     * here we only want to repaint children of windows that clip them, others
     * need to wait for WM_PAINT to be done in the parent first.
     */
    if (!(flags & UPDATE_ALLCHILDREN) && !(win->style & WS_CLIPCHILDREN)) return 0;

    LIST_FOR_EACH_ENTRY( ptr, &win->children, struct window, entry )
    {
        if (from_child)  /* skip all children until from_child is found */
        {
            if (ptr == from_child) from_child = NULL;
            continue;
        }
        if (!(ptr->style & WS_VISIBLE)) continue;
        if ((ret = get_update_flags( ptr, flags )) != 0)
        {
            *child = ptr;
            break;
        }
        if ((ret = get_child_update_flags( ptr, NULL, flags, child ))) break;
    }
    return ret;
}

/* iterate through children and siblings of the given window until we find one with some update flags */
static unsigned int get_window_update_flags( struct window *win, struct window *from_child,
                                             unsigned int flags, struct window **child )
{
    unsigned int ret;
    struct window *ptr, *from_sibling = NULL;

    /* if some parent is not visible start from the next sibling */

    if (!is_visible( win )) return 0;
    for (ptr = from_child; ptr; ptr = ptr->parent)
    {
        if (!(ptr->style & WS_VISIBLE) || (ptr->style & WS_MINIMIZE)) from_sibling = ptr;
        if (ptr == win) break;
    }

    /* non-client painting must be delayed if one of the parents is going to
     * be repainted and doesn't clip children */

    if ((flags & UPDATE_NONCLIENT) && !(flags & (UPDATE_PAINT|UPDATE_INTERNALPAINT)))
    {
        for (ptr = win->parent; ptr; ptr = ptr->parent)
        {
            if (!(ptr->style & WS_CLIPCHILDREN) && win_needs_repaint( ptr ))
                return 0;
        }
        if (from_child && !(flags & UPDATE_ALLCHILDREN))
        {
            for (ptr = from_sibling ? from_sibling : from_child; ptr; ptr = ptr->parent)
            {
                if (!(ptr->style & WS_CLIPCHILDREN) && win_needs_repaint( ptr )) from_sibling = ptr;
                if (ptr == win) break;
            }
        }
    }


    /* check window itself (only if not restarting from a child) */

    if (!from_child)
    {
        if ((ret = get_update_flags( win, flags )))
        {
            *child = win;
            return ret;
        }
        from_child = win;
    }

    /* now check children */

    if (flags & UPDATE_NOCHILDREN) return 0;
    if (!from_sibling)
    {
        if ((ret = get_child_update_flags( from_child, NULL, flags, child ))) return ret;
        from_sibling = from_child;
    }

    /* then check siblings and parent siblings */

    while (from_sibling->parent && from_sibling != win)
    {
        if ((ret = get_child_update_flags( from_sibling->parent, from_sibling, flags, child )))
            return ret;
        from_sibling = from_sibling->parent;
    }
    return 0;
}


/* expose the areas revealed by a vis region change on the window parent */
/* returns the region exposed on the window itself (in client coordinates) */
static struct region *expose_window( struct window *win, const struct rectangle *old_window_rect,
                                     struct region *old_vis_rgn, int zorder_changed )
{
    struct region *new_vis_rgn, *exposed_rgn;
    int is_composited = is_parent_composited( win );

    if (!(new_vis_rgn = get_visible_region( win, DCX_WINDOW ))) return NULL;

    if (is_composited && !zorder_changed &&
        is_rect_equal( old_window_rect, &win->window_rect ) &&
        is_region_equal( old_vis_rgn, new_vis_rgn ))
    {
        free_region( new_vis_rgn );
        return NULL;
    }

    if ((exposed_rgn = create_empty_region()))
    {
        if ((is_composited ? union_region( exposed_rgn, new_vis_rgn, old_vis_rgn )
                           : subtract_region( exposed_rgn, new_vis_rgn, old_vis_rgn )) &&
            !is_region_empty( exposed_rgn ))
        {
            /* make it relative to the new client area */
            offset_region( exposed_rgn, win->window_rect.left - win->client_rect.left,
                           win->window_rect.top - win->client_rect.top );
        }
        else
        {
            free_region( exposed_rgn );
            exposed_rgn = NULL;
        }
    }

    if (win->parent && !is_desktop_window( win->parent ))
    {
        /* make it relative to the old window pos for subtracting */
        offset_region( new_vis_rgn, win->window_rect.left - old_window_rect->left,
                       win->window_rect.top - old_window_rect->top  );

        if (is_region_empty( old_vis_rgn ) ||
            (is_composited ? union_region( new_vis_rgn, old_vis_rgn, new_vis_rgn )
                           : subtract_region( new_vis_rgn, old_vis_rgn, new_vis_rgn )))
        {
            if (!is_region_empty( new_vis_rgn ))
            {
                /* make it relative to parent */
                offset_region( new_vis_rgn, old_window_rect->left, old_window_rect->top );
                redraw_window( win->parent, new_vis_rgn, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
            }
        }
    }
    free_region( new_vis_rgn );
    return exposed_rgn;
}


/* set the window and client rectangles, updating the update region if necessary */
static void set_window_pos( struct window *win, struct window *previous,
                            unsigned int swp_flags, const struct rectangle *window_rect,
                            const struct rectangle *client_rect, const struct rectangle *visible_rect,
                            const struct rectangle *surface_rect, const struct rectangle *valid_rect )
{
    struct region *old_vis_rgn = NULL, *exposed_rgn = NULL;
    const struct rectangle old_window_rect = win->window_rect;
    const struct rectangle old_visible_rect = win->visible_rect;
    const struct rectangle old_client_rect = win->client_rect;
    struct rectangle rect;
    int client_changed, frame_changed;
    int visible = (win->style & WS_VISIBLE) || (swp_flags & SWP_SHOWWINDOW);
    int zorder_changed = 0;

    if (win->parent && !is_visible( win->parent )) visible = 0;

    if (visible && !(old_vis_rgn = get_visible_region( win, DCX_WINDOW ))) return;

    /* set the new window info before invalidating anything */

    win->window_rect  = *window_rect;
    win->visible_rect = *visible_rect;
    win->surface_rect = *surface_rect;
    win->client_rect  = *client_rect;
    if (!(swp_flags & SWP_NOZORDER) && win->parent) zorder_changed |= link_window( win, previous );
    if (swp_flags & SWP_SHOWWINDOW) win->style |= WS_VISIBLE;
    else if (swp_flags & SWP_HIDEWINDOW) win->style &= ~WS_VISIBLE;

    /* keep children at the same position relative to top right corner when the parent is mirrored */
    if (win->ex_style & WS_EX_LAYOUTRTL)
    {
        struct window *child;
        int old_size = old_client_rect.right - old_client_rect.left;
        int new_size = win->client_rect.right - win->client_rect.left;

        if (old_size != new_size) LIST_FOR_EACH_ENTRY( child, &win->children, struct window, entry )
        {
            offset_rect( &child->window_rect, new_size - old_size, 0 );
            offset_rect( &child->visible_rect, new_size - old_size, 0 );
            offset_rect( &child->surface_rect, new_size - old_size, 0 );
            offset_rect( &child->client_rect, new_size - old_size, 0 );
        }
    }

    /* reset cursor clip rectangle when the desktop changes size */
    if (win == win->desktop->top_window) set_clip_rectangle( win->desktop, NULL, SET_CURSOR_NOCLIP, 1 );

    /* if the window is not visible, everything is easy */
    if (!visible) return;

    /* expose anything revealed by the change */

    if (!(swp_flags & SWP_NOREDRAW))
        exposed_rgn = expose_window( win, &old_window_rect, old_vis_rgn, zorder_changed );

    if (!(win->style & WS_VISIBLE))
    {
        /* clear the update region since the window is no longer visible */
        validate_whole_window( win );
        validate_children( win );
        goto done;
    }

    /* crop update region to the new window rect */

    if (win->update_region)
    {
        if (get_window_visible_rect( win, &rect, 1 ))
        {
            struct region *tmp = create_empty_region();
            if (tmp)
            {
                set_region_rect( tmp, &rect );
                if (intersect_region( tmp, win->update_region, tmp ))
                    set_update_region( win, tmp );
                else
                    free_region( tmp );
            }
        }
        else set_update_region( win, NULL ); /* visible rect is empty */
    }

    /* crop children regions to the new window rect */

    if (get_window_visible_rect( win, &rect, 0 ))
    {
        /* map to client coords */
        offset_rect( &rect, win->window_rect.left - win->client_rect.left,
                     win->window_rect.top - win->client_rect.top );
        crop_children_update_region( win, &rect );
    }
    else crop_children_update_region( win, NULL );

    if (swp_flags & SWP_NOREDRAW) goto done;  /* do not repaint anything */

    /* expose the whole non-client area if it changed in any way */

    if (swp_flags & SWP_NOCOPYBITS)
    {
        frame_changed = ((swp_flags & SWP_FRAMECHANGED) ||
                         memcmp( window_rect, &old_window_rect, sizeof(old_window_rect) ) ||
                         memcmp( visible_rect, &old_visible_rect, sizeof(old_visible_rect) ));
        client_changed = memcmp( client_rect, &old_client_rect, sizeof(old_client_rect) );
    }
    else
    {
        /* assume the bits have been moved to follow the window rect */
        int x_offset = window_rect->left - old_window_rect.left;
        int y_offset = window_rect->top - old_window_rect.top;
        frame_changed = ((swp_flags & SWP_FRAMECHANGED) ||
                         window_rect->right  - old_window_rect.right != x_offset ||
                         window_rect->bottom - old_window_rect.bottom != y_offset ||
                         visible_rect->left   - old_visible_rect.left   != x_offset ||
                         visible_rect->right  - old_visible_rect.right  != x_offset ||
                         visible_rect->top    - old_visible_rect.top    != y_offset ||
                         visible_rect->bottom - old_visible_rect.bottom != y_offset);
        client_changed = (client_rect->left   - old_client_rect.left   != x_offset ||
                          client_rect->right  - old_client_rect.right  != x_offset ||
                          client_rect->top    - old_client_rect.top    != y_offset ||
                          client_rect->bottom - old_client_rect.bottom != y_offset ||
                          memcmp( valid_rect, client_rect, sizeof(*client_rect) ));
    }

    if (frame_changed || client_changed)
    {
        struct region *win_rgn = old_vis_rgn;  /* reuse previous region */

        set_region_rect( win_rgn, window_rect );
        if (!is_rect_empty( valid_rect ))
        {
            /* subtract the valid portion of client rect from the total region */
            struct region *tmp = create_empty_region();
            if (tmp)
            {
                set_region_rect( tmp, valid_rect );
                /* subtract update region since invalid parts of the valid rect won't be copied */
                if (win->update_region)
                {
                    offset_region( tmp, -window_rect->left, -window_rect->top );
                    subtract_region( tmp, tmp, win->update_region );
                    offset_region( tmp, window_rect->left, window_rect->top );
                }
                if (subtract_region( tmp, win_rgn, tmp )) win_rgn = tmp;
                else free_region( tmp );
            }
        }
        if (!is_desktop_window(win))
            offset_region( win_rgn, -client_rect->left, -client_rect->top );
        if (exposed_rgn)
        {
            union_region( exposed_rgn, exposed_rgn, win_rgn );
            if (win_rgn != old_vis_rgn) free_region( win_rgn );
        }
        else
        {
            exposed_rgn = win_rgn;
            if (win_rgn == old_vis_rgn) old_vis_rgn = NULL;
        }
    }

    if (exposed_rgn)
        redraw_window( win, exposed_rgn, 1, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );

done:
    if (old_vis_rgn) free_region( old_vis_rgn );
    if (exposed_rgn) free_region( exposed_rgn );
    clear_error();  /* we ignore out of memory errors once the new rects have been set */
}


/* set the window region, updating the update region if necessary */
static void set_window_region( struct window *win, struct region *region, int redraw )
{
    struct region *old_vis_rgn = NULL, *exposed_rgn;

    /* no need to redraw if window is not visible */
    if (redraw && !is_visible( win )) redraw = 0;

    if (redraw) old_vis_rgn = get_visible_region( win, DCX_WINDOW );

    if (win->win_region) free_region( win->win_region );
    win->win_region = region;

    /* expose anything revealed by the change */
    if (old_vis_rgn && ((exposed_rgn = expose_window( win, &win->window_rect, old_vis_rgn, 0 ))))
    {
        redraw_window( win, exposed_rgn, 1, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
        free_region( exposed_rgn );
    }

    if (old_vis_rgn) free_region( old_vis_rgn );
    clear_error();  /* we ignore out of memory errors since the region has been set */
}


/* destroy a window */
void free_window_handle( struct window *win )
{
    struct window *child, *next;

    assert( win->handle );

    /* hide the window */
    if (is_visible(win))
    {
        struct region *vis_rgn = get_visible_region( win, DCX_WINDOW );
        win->style &= ~WS_VISIBLE;
        if (vis_rgn)
        {
            struct region *exposed_rgn = expose_window( win, &win->window_rect, vis_rgn, 0 );
            if (exposed_rgn) free_region( exposed_rgn );
            free_region( vis_rgn );
        }
        validate_whole_window( win );
        validate_children( win );
    }

    /* destroy all children */
    LIST_FOR_EACH_ENTRY_SAFE( child, next, &win->children, struct window, entry )
    {
        if (!child->handle) continue;
        if (!win->thread || !child->thread || win->thread == child->thread)
            free_window_handle( child );
        else
            send_notify_message( child->handle, WM_WINE_DESTROYWINDOW, 0, 0 );
    }
    LIST_FOR_EACH_ENTRY_SAFE( child, next, &win->children, struct window, entry )
    {
        if (!child->handle) continue;
        if (!win->thread || !child->thread || win->thread == child->thread)
            free_window_handle( child );
        else
            send_notify_message( child->handle, WM_WINE_DESTROYWINDOW, 0, 0 );
    }

    /* reset global window pointers, if the corresponding window is destroyed */
    if (win == win->desktop->shell_window) win->desktop->shell_window = NULL;
    if (win == win->desktop->shell_listview) win->desktop->shell_listview = NULL;
    if (win == win->desktop->progman_window) win->desktop->progman_window = NULL;
    if (win == win->desktop->taskman_window) win->desktop->taskman_window = NULL;
    free_hotkeys( win->desktop, win->handle );
    cleanup_clipboard_window( win->desktop, win->handle );
    destroy_properties( win );
    if (is_desktop_window(win))
    {
        struct desktop *desktop = win->desktop;
        assert( desktop->top_window == win || desktop->msg_window == win );
        if (desktop->top_window == win) desktop->top_window = NULL;
        else desktop->msg_window = NULL;
    }
    else if (is_desktop_window( win->parent ))
    {
        post_message( win->parent->handle, WM_PARENTNOTIFY, WM_DESTROY, win->handle );
    }

    detach_window_thread( win );

    if (win->parent) set_parent_window( win, NULL );
    free_user_handle( win->handle );
    win->handle = 0;
    release_object( win );
}


/* create a window */
DECL_HANDLER(create_window)
{
    struct window *win, *parent = NULL, *owner = NULL;
    struct unicode_str cls_name = get_req_unicode_str();
    atom_t atom;

    reply->handle = 0;
    if (req->parent)
    {
        if (!(parent = get_window( req->parent ))) return;
        if (is_orphan_window( parent ))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }

    if (req->owner)
    {
        if (!(owner = get_window( req->owner ))) return;
        if (is_desktop_window(owner)) owner = NULL;
        else if (parent && !is_desktop_window(parent))
        {
            /* an owned window must be created as top-level */
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
        else /* owner must be a top-level window */
            while ((owner->style & (WS_POPUP|WS_CHILD)) == WS_CHILD && !is_desktop_window(owner->parent))
                owner = owner->parent;
    }

    atom = cls_name.len ? find_global_atom( NULL, &cls_name ) : req->atom;

    if (!(win = create_window( parent, owner, atom, req->instance ))) return;

    if (parent && !is_desktop_window( parent ))
        win->dpi_context = parent->dpi_context;
    else if (!parent || !NTUSER_DPI_CONTEXT_IS_MONITOR_AWARE( req->dpi_context ))
        win->dpi_context = req->dpi_context;

    win->style = req->style;
    win->ex_style = req->ex_style;

    reply->handle      = win->handle;
    reply->parent      = win->parent ? win->parent->handle : 0;
    reply->owner       = win->owner;
    reply->extra       = win->nb_extra_bytes;
    reply->dpi_context = win->dpi_context;
    reply->class_ptr   = get_class_client_ptr( win->class );
}


/* set the parent of a window */
DECL_HANDLER(set_parent)
{
    struct window *win, *parent = NULL;

    if (!(win = get_window( req->handle ))) return;
    if (req->parent && !(parent = get_window( req->parent ))) return;

    if (is_desktop_window(win) || is_orphan_window( win ) || (parent && is_orphan_window( parent )))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    reply->old_parent  = win->parent->handle;
    reply->full_parent = parent ? parent->handle : 0;
    set_parent_window( win, parent );
    reply->dpi_context = win->dpi_context;
}


/* destroy a window */
DECL_HANDLER(destroy_window)
{
    struct window *win;

    if (!req->handle)
    {
        destroy_thread_windows( current );
    }
    else if ((win = get_window( req->handle )))
    {
        if (!is_desktop_window(win)) free_window_handle( win );
        else if (win->thread == current) detach_window_thread( win );
        else set_error( STATUS_ACCESS_DENIED );
    }
}


/* retrieve the desktop window for the current thread */
DECL_HANDLER(get_desktop_window)
{
    struct desktop *desktop = get_thread_desktop( current, 0 );

    if (!desktop) return;

    if (!desktop->top_window && req->force)  /* create it */
    {
        if ((desktop->top_window = create_window( NULL, NULL, DESKTOP_ATOM, 0 )))
        {
            detach_window_thread( desktop->top_window );
            desktop->top_window->style  = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        }
    }

    if (!desktop->msg_window && req->force)  /* create it */
    {
        static const WCHAR messageW[] = {'M','e','s','s','a','g','e'};
        static const struct unicode_str name = { messageW, sizeof(messageW) };
        atom_t atom = add_global_atom( NULL, &name );
        if (atom && (desktop->msg_window = create_window( NULL, NULL, atom, 0 )))
        {
            detach_window_thread( desktop->msg_window );
            desktop->msg_window->style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        }
    }

    reply->top_window = desktop->top_window ? desktop->top_window->handle : 0;
    reply->msg_window = desktop->msg_window ? desktop->msg_window->handle : 0;
    release_object( desktop );
}


/* set a window owner */
DECL_HANDLER(set_window_owner)
{
    struct window *win = get_window( req->handle );
    struct window *owner = NULL, *ptr;

    if (!win) return;
    if (req->owner && !(owner = get_window( req->owner ))) return;
    if (is_desktop_window(win))
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }

    /* make sure owner is not a successor of window */
    for (ptr = owner; ptr; ptr = ptr->owner ? get_window( ptr->owner ) : NULL)
    {
        if (ptr == win)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }

    reply->prev_owner = win->owner;
    reply->full_owner = win->owner = owner ? owner->handle : 0;
}


/* get information from a window handle */
DECL_HANDLER(get_window_info)
{
    struct window *win = get_window( req->handle );

    if (!win) return;

    reply->last_active = win->handle;
    reply->is_unicode  = win->is_unicode;
    reply->dpi_context = win->dpi_context;

    if (get_user_object( win->last_active, NTUSER_OBJ_WINDOW )) reply->last_active = win->last_active;
    if (win->thread)
    {
        reply->tid  = get_thread_id( win->thread );
        reply->pid  = get_process_id( win->thread->process );
    }
}


/* set some information in a window */
DECL_HANDLER(set_window_info)
{
    struct window *win = get_window( req->handle );

    if (!win) return;
    if (req->flags && is_desktop_window(win) && win->thread != current)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (req->extra_size > sizeof(req->extra_value) ||
        req->extra_offset < -1 ||
        req->extra_offset > win->nb_extra_bytes - (int)req->extra_size)
    {
        set_win32_error( ERROR_INVALID_INDEX );
        return;
    }
    if (req->extra_offset != -1)
    {
        memcpy( &reply->old_extra_value, win->extra_bytes + req->extra_offset, req->extra_size );
    }
    else if (req->flags & SET_WIN_EXTRA)
    {
        set_win32_error( ERROR_INVALID_INDEX );
        return;
    }
    reply->old_style     = win->style;
    reply->old_ex_style  = win->ex_style;
    reply->old_id        = win->id;
    reply->old_instance  = win->instance;
    reply->old_user_data = win->user_data;
    if (req->flags & SET_WIN_STYLE) win->style = req->style;
    if (req->flags & SET_WIN_EXSTYLE)
    {
        /* WS_EX_TOPMOST can only be changed for unlinked windows */
        if (!win->is_linked) win->ex_style = req->ex_style;
        else win->ex_style = (req->ex_style & ~WS_EX_TOPMOST) | (win->ex_style & WS_EX_TOPMOST);
        if (!(win->ex_style & WS_EX_LAYERED)) win->is_layered = 0;
    }
    if (req->flags & SET_WIN_ID) win->id = req->extra_value;
    if (req->flags & SET_WIN_INSTANCE) win->instance = req->instance;
    if (req->flags & SET_WIN_UNICODE) win->is_unicode = req->is_unicode;
    if (req->flags & SET_WIN_USERDATA) win->user_data = req->user_data;
    if (req->flags & SET_WIN_EXTRA) memcpy( win->extra_bytes + req->extra_offset,
                                            &req->extra_value, req->extra_size );

    /* changing window style triggers a non-client paint */
    if (req->flags & SET_WIN_STYLE) win->paint_flags |= PAINT_NONCLIENT;
}


/* get a list of the window parents, up to the root of the tree */
DECL_HANDLER(get_window_parents)
{
    struct window *ptr, *win = get_window( req->handle );
    int total = 0;
    user_handle_t *data;
    data_size_t len;

    if (win) for (ptr = win->parent; ptr; ptr = ptr->parent) total++;

    reply->count = total;
    len = min( get_reply_max_size(), total * sizeof(user_handle_t) );
    if (len && ((data = set_reply_data_size( len ))))
    {
        for (ptr = win->parent; ptr && len; ptr = ptr->parent, len -= sizeof(*data))
            *data++ = ptr->handle;
    }
}


/* get a list of window siblings or children */
DECL_HANDLER(get_window_list)
{
    struct window *win = NULL;
    struct desktop *desktop = NULL;
    struct thread *thread = NULL;
    user_handle_t *data;
    unsigned int count = 0, max_count = get_reply_max_size() / sizeof(*data);

    if (req->handle && !(win = get_window( req->handle )))
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if (req->tid && !(thread = get_thread_from_id( req->tid )))
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if (req->desktop && !(desktop = get_desktop_obj( current->process, req->desktop, DESKTOP_READOBJECTS )))
    {
        if (thread) release_object( thread );
        return;
    }

    max_count = min( max_count, MAX_USER_HANDLES );
    if ((data = mem_alloc( max_count * sizeof(*data) )))
    {
        get_window_list( desktop, win, thread, req->children, data, &count, max_count );
        if (count > max_count)
        {
            free( data );
            set_error( STATUS_BUFFER_TOO_SMALL );
        }
        else set_reply_data_ptr( data, count * sizeof(*data) );
        reply->count = count;
    }

    if (thread) release_object( thread );
    if (desktop) release_object( desktop );
}


/* get a list of the window siblings of a specified class */
DECL_HANDLER(get_class_windows)
{
    struct desktop *desktop = NULL;
    struct window *parent = NULL, *win = NULL;
    struct unicode_str cls_name = get_req_unicode_str();
    atom_t atom = req->atom;
    user_handle_t *data;
    unsigned int count = 0, max_count = get_reply_max_size() / sizeof(*data);

    if (cls_name.len && !(atom = find_global_atom( NULL, &cls_name ))) return;
    if (req->parent && !(parent = get_window( req->parent ))) return;

    if (req->child)
    {
        if (!parent) parent = get_desktop_window( current );
        if (!(win = get_window( req->child ))) return;
        if (win->parent != parent) return;
        if (!(win = get_next_window( win ))) return;
    }
    else if (parent && !(win = get_first_child( parent ))) return;

    if (!win && !(desktop = get_thread_desktop( current, 0 ))) return;

    max_count = min( max_count, MAX_USER_HANDLES );
    if ((data = mem_alloc( max_count * sizeof(*data) )))
    {
        if (desktop) /* top-level and message windows of current desktop */
        {
            if (desktop->top_window)
                for (win = get_first_child( desktop->top_window ); win; win = get_next_window( win ))
                    append_window_to_list( win, NULL, atom, data, &count, max_count );
            if (desktop->msg_window)
                for (win = get_first_child( desktop->msg_window ); win; win = get_next_window( win ))
                    append_window_to_list( win, NULL, atom, data, &count, max_count );
        }
        else
        {
            for ( ; win; win = get_next_window( win ))
                append_window_to_list( win, NULL, atom, data, &count, max_count );
        }
        if (count > max_count)
        {
            free( data );
            set_error( STATUS_BUFFER_TOO_SMALL );
        }
        else set_reply_data_ptr( data, count * sizeof(*data) );
        reply->count = count;
    }

    if (desktop) release_object( desktop );
}


/* get a list of the window children that contain a given point */
DECL_HANDLER(get_window_children_from_point)
{
    struct user_handle_array array;
    struct window *parent = get_window( req->parent );
    data_size_t len;

    if (!parent) return;

    array.handles = NULL;
    array.count = 0;
    array.total = 0;
    if (!all_windows_from_point( parent, req->x, req->y, req->dpi, &array )) return;

    reply->count = array.count;
    len = min( get_reply_max_size(), array.count * sizeof(user_handle_t) );
    if (len) set_reply_data_ptr( array.handles, len );
    else free( array.handles );
}


/* get window tree information from a window handle */
DECL_HANDLER(get_window_tree)
{
    struct window *ptr, *win = get_window( req->handle );

    if (!win) return;

    reply->parent        = 0;
    reply->owner         = 0;
    reply->next_sibling  = 0;
    reply->prev_sibling  = 0;
    reply->first_sibling = 0;
    reply->last_sibling  = 0;
    reply->first_child   = 0;
    reply->last_child    = 0;

    if (win->parent)
    {
        struct window *parent = win->parent;
        reply->parent = parent->handle;
        reply->owner  = win->owner;
        if (win->is_linked)
        {
            if ((ptr = get_next_window( win ))) reply->next_sibling = ptr->handle;
            if ((ptr = get_prev_window( win ))) reply->prev_sibling = ptr->handle;
        }
        if ((ptr = get_first_child( parent ))) reply->first_sibling = ptr->handle;
        if ((ptr = get_last_child( parent ))) reply->last_sibling = ptr->handle;
    }
    if ((ptr = get_first_child( win ))) reply->first_child = ptr->handle;
    if ((ptr = get_last_child( win ))) reply->last_child = ptr->handle;
}


/* set the position and Z order of a window */
DECL_HANDLER(set_window_pos)
{
    struct rectangle window_rect, client_rect, visible_rect, surface_rect, valid_rect, old_window, old_client;
    const struct rectangle *extra_rects = get_req_data();
    struct window *previous = NULL;
    struct window *top, *win = get_window( req->handle );
    unsigned int flags = req->swp_flags, old_style;

    if (!win) return;
    if (!win->parent) flags |= SWP_NOZORDER;  /* no Z order for the desktop */

    if (!(flags & SWP_NOZORDER))
    {
        switch ((int)req->previous)
        {
        case 0:   /* HWND_TOP */
            previous = WINPTR_TOP;
            break;
        case 1:   /* HWND_BOTTOM */
            previous = WINPTR_BOTTOM;
            break;
        case -1:  /* HWND_TOPMOST */
            previous = WINPTR_TOPMOST;
            break;
        case -2:  /* HWND_NOTOPMOST */
            previous = WINPTR_NOTOPMOST;
            break;
        default:
            if (!(previous = get_window( req->previous ))) return;
            /* previous must be a sibling */
            if (previous->parent != win->parent)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            break;
        }
        if (previous == win) flags |= SWP_NOZORDER;  /* nothing to do */
    }

    /* windows that use UpdateLayeredWindow don't trigger repaints */
    if ((win->ex_style & WS_EX_LAYERED) && !win->is_layered) flags |= SWP_NOREDRAW;

    /* window rectangle must be ordered properly */
    if (req->window.right < req->window.left || req->window.bottom < req->window.top)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    window_rect = req->window;
    client_rect = req->client;
    if (get_req_data_size() >= sizeof(struct rectangle)) visible_rect = extra_rects[0];
    else visible_rect = window_rect;
    if (get_req_data_size() >= 2 * sizeof(struct rectangle)) surface_rect = extra_rects[1];
    else surface_rect = visible_rect;
    if (get_req_data_size() >= 3 * sizeof(struct rectangle)) valid_rect = extra_rects[2];
    else valid_rect = empty_rect;
    if (win->parent && win->parent->ex_style & WS_EX_LAYOUTRTL)
    {
        mirror_rect( &win->parent->client_rect, &window_rect );
        mirror_rect( &win->parent->client_rect, &visible_rect );
        mirror_rect( &win->parent->client_rect, &client_rect );
        mirror_rect( &win->parent->client_rect, &surface_rect );
        mirror_rect( &win->parent->client_rect, &valid_rect );
    }

    win->paint_flags = (win->paint_flags & ~PAINT_CLIENT_FLAGS) | (req->paint_flags & PAINT_CLIENT_FLAGS);
    if (win->paint_flags & PAINT_HAS_PIXEL_FORMAT) update_pixel_format_flags( win );

    win->monitor_dpi = req->monitor_dpi;
    old_style = win->style;
    old_window = win->window_rect;
    old_client = win->client_rect;
    set_window_pos( win, previous, flags, &window_rect, &client_rect,
                    &visible_rect, &surface_rect, &valid_rect );
    if ((win->style & old_style & WS_VISIBLE) && (memcmp( &old_client, &win->client_rect, sizeof(old_client) )
        || memcmp( &old_window, &win->window_rect, sizeof(old_window) )))
        update_cursor_pos( win->desktop );

    if (win->paint_flags & SET_WINPOS_LAYERED_WINDOW) validate_whole_window( win );

    reply->new_style = win->style;
    reply->new_ex_style = win->ex_style;

    top = get_top_clipping_window( win );
    if (is_visible( top ) && (top->paint_flags & PAINT_HAS_SURFACE)) reply->surface_win = top->handle;
}


/* get the window and client rectangles of a window */
DECL_HANDLER(get_window_rectangles)
{
    struct window *win = get_window( req->handle );

    if (!win) return;

    reply->window  = win->window_rect;
    reply->client  = win->client_rect;

    switch (req->relative)
    {
    case COORDS_CLIENT:
        offset_rect( &reply->window, -win->client_rect.left, -win->client_rect.top );
        offset_rect( &reply->client, -win->client_rect.left, -win->client_rect.top );
        if (win->ex_style & WS_EX_LAYOUTRTL) mirror_rect( &win->client_rect, &reply->window );
        break;
    case COORDS_WINDOW:
        offset_rect( &reply->window, -win->window_rect.left, -win->window_rect.top );
        offset_rect( &reply->client, -win->window_rect.left, -win->window_rect.top );
        if (win->ex_style & WS_EX_LAYOUTRTL) mirror_rect( &win->window_rect, &reply->client );
        break;
    case COORDS_PARENT:
        if (win->parent && win->parent->ex_style & WS_EX_LAYOUTRTL)
        {
            mirror_rect( &win->parent->client_rect, &reply->window );
            mirror_rect( &win->parent->client_rect, &reply->client );
        }
        break;
    case COORDS_SCREEN:
        client_to_screen_rect( win->parent, &reply->window );
        client_to_screen_rect( win->parent, &reply->client );
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        break;
    }
    map_dpi_rect( win, &reply->window, get_window_dpi( win ), req->dpi );
    map_dpi_rect( win, &reply->client, get_window_dpi( win ), req->dpi );
}


/* get the window text */
DECL_HANDLER(get_window_text)
{
    struct window *win = get_window( req->handle );

    if (win && win->text_len)
    {
        reply->length = win->text_len / sizeof(WCHAR);
        set_reply_data( win->text, min( win->text_len, get_reply_max_size() ));
    }
}


/* set the window text */
DECL_HANDLER(set_window_text)
{
    data_size_t len;
    WCHAR *text = NULL;
    struct window *win = get_window( req->handle );

    if (!win) return;
    len = (get_req_data_size() / sizeof(WCHAR)) * sizeof(WCHAR);
    if (len && !(text = memdup( get_req_data(), len ))) return;
    free( win->text );
    win->text = text;
    win->text_len = len;
}


/* get the coordinates offset between two windows */
DECL_HANDLER(get_windows_offset)
{
    struct window *win;
    int x, y, mirror_from = 0, mirror_to = 0;

    reply->x = reply->y = 0;
    if (req->from)
    {
        if (!(win = get_window( req->from ))) return;
        if (win->ex_style & WS_EX_LAYOUTRTL) mirror_from = 1;
        x = mirror_from ? win->client_rect.right - win->client_rect.left : 0;
        y = 0;
        client_to_screen( win, &x, &y );
        map_dpi_point( win, &x, &y, get_window_dpi( win ), req->dpi );
        reply->x += x;
        reply->y += y;
    }
    if (req->to)
    {
        if (!(win = get_window( req->to ))) return;
        if (win->ex_style & WS_EX_LAYOUTRTL) mirror_to = 1;
        x = mirror_to ? win->client_rect.right - win->client_rect.left : 0;
        y = 0;
        client_to_screen( win, &x, &y );
        map_dpi_point( win, &x, &y, get_window_dpi( win ), req->dpi );
        reply->x -= x;
        reply->y -= y;
    }
    if (mirror_from) reply->x = -reply->x;
    reply->mirror = mirror_from ^ mirror_to;
}


/* get the visible region of a window */
DECL_HANDLER(get_visible_region)
{
    struct region *region;
    struct window *top, *win = get_window( req->window );

    if (!win) return;

    top = get_top_clipping_window( win );
    if ((region = get_visible_region( win, req->flags )))
    {
        struct rectangle *data;
        map_win_region_to_screen( win, region );
        data = get_region_data_and_free( region, get_reply_max_size(), &reply->total_size );
        if (data) set_reply_data_ptr( data, reply->total_size );
    }
    reply->top_win  = top->handle;
    reply->top_rect = top->surface_rect;

    if (!is_desktop_window(win))
    {
        reply->win_rect = (req->flags & DCX_WINDOW) ? win->window_rect : win->client_rect;
        client_to_screen_rect( top->parent, &reply->top_rect );
        client_to_screen_rect( win->parent, &reply->win_rect );
    }
    else
    {
        reply->win_rect.left   = 0;
        reply->win_rect.top    = 0;
        reply->win_rect.right  = win->client_rect.right - win->client_rect.left;
        reply->win_rect.bottom = win->client_rect.bottom - win->client_rect.top;
    }
    reply->paint_flags = win->paint_flags & PAINT_CLIENT_FLAGS;
}


/* get the window regions */
DECL_HANDLER(get_window_region)
{
    struct rectangle *data;
    struct region *region;
    struct window *win = get_window( req->window );

    if (!win) return;

    reply->visible_rect = win->visible_rect;
    if (req->surface)
    {
        if (!is_visible( win )) return;

        if ((region = get_surface_region( win )))
        {
            struct rectangle *data = get_region_data_and_free( region, get_reply_max_size(), &reply->total_size );
            if (data) set_reply_data_ptr( data, reply->total_size );
        }
        return;
    }

    if (!win->win_region) return;

    if (win->ex_style & WS_EX_LAYOUTRTL)
    {
        struct region *region = create_empty_region();

        if (!region) return;
        if (!copy_region( region, win->win_region ))
        {
            free_region( region );
            return;
        }
        mirror_region( &win->window_rect, region );
        data = get_region_data_and_free( region, get_reply_max_size(), &reply->total_size );
    }
    else data = get_region_data( win->win_region, get_reply_max_size(), &reply->total_size );

    if (data) set_reply_data_ptr( data, reply->total_size );
}


/* set the window region */
DECL_HANDLER(set_window_region)
{
    struct region *region = NULL;
    struct window *win = get_window( req->window );

    if (!win) return;

    if (get_req_data_size())  /* no data means remove the region completely */
    {
        if (!(region = create_region_from_req_data( get_req_data(), get_req_data_size() )))
            return;
        if (win->ex_style & WS_EX_LAYOUTRTL) mirror_region( &win->window_rect, region );
    }
    set_window_region( win, region, req->redraw );
}


/* get a window update region */
DECL_HANDLER(get_update_region)
{
    struct rectangle *data;
    unsigned int flags = req->flags;
    struct window *from_child = NULL;
    struct window *win = get_window( req->window );

    reply->flags = 0;
    if (!win) return;

    if (req->from_child)
    {
        struct window *ptr;

        if (!(from_child = get_window( req->from_child ))) return;

        /* make sure from_child is a child of win */
        ptr = from_child;
        while (ptr && ptr != win) ptr = ptr->parent;
        if (!ptr)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }

    if (flags & UPDATE_DELAYED_ERASE)  /* this means that the previous call didn't erase */
    {
        if (from_child) from_child->paint_flags |= PAINT_DELAYED_ERASE;
        else win->paint_flags |= PAINT_DELAYED_ERASE;
    }

    reply->flags = get_window_update_flags( win, from_child, flags, &win );
    reply->child = win->handle;

    if (flags & UPDATE_NOREGION) return;

    if (win->update_region)
    {
        /* convert update region to screen coordinates */
        struct region *region = create_empty_region();

        if (!region) return;
        if (!copy_region( region, win->update_region ))
        {
            free_region( region );
            return;
        }
        if ((flags & UPDATE_CLIPCHILDREN) && (win->style & WS_CLIPCHILDREN))
            clip_children( win, NULL, region, win->client_rect.left - win->window_rect.left,
                           win->client_rect.top - win->window_rect.top );
        map_win_region_to_screen( win, region );
        if (!(data = get_region_data_and_free( region, get_reply_max_size(),
                                               &reply->total_size ))) return;
        set_reply_data_ptr( data, reply->total_size );
    }

    if (reply->flags & (UPDATE_PAINT|UPDATE_INTERNALPAINT)) /* validate everything */
    {
        validate_parents( win );
        validate_whole_window( win );
    }
    else
    {
        if (reply->flags & UPDATE_NONCLIENT) validate_non_client( win );
        if (reply->flags & UPDATE_ERASE)
        {
            win->paint_flags &= ~(PAINT_ERASE | PAINT_DELAYED_ERASE);
            /* desktop window only gets erased, not repainted */
            if (is_desktop_window(win)) validate_whole_window( win );
        }
    }
}


/* update the z order of a window so that a given rectangle is fully visible */
DECL_HANDLER(update_window_zorder)
{
    struct rectangle tmp, rect = req->rect;
    struct window *ptr, *win = get_window( req->window );

    if (!win || !win->parent || !is_visible( win )) return;  /* nothing to do */

    LIST_FOR_EACH_ENTRY( ptr, &win->parent->children, struct window, entry )
    {
        if (ptr == win) break;
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->ex_style & WS_EX_TRANSPARENT) continue;
        if (ptr->is_layered && (ptr->layered_flags & LWA_COLORKEY)) continue;
        tmp = rect;
        map_dpi_rect( win, &tmp, get_window_dpi( win->parent ), get_window_dpi( win ) );
        if (!intersect_rect( &tmp, &tmp, &ptr->visible_rect )) continue;
        if (ptr->win_region)
        {
            offset_rect( &tmp, -ptr->window_rect.left, -ptr->window_rect.top );
            if (!rect_in_region( ptr->win_region, &tmp )) continue;
        }
        /* found a window obscuring the rectangle, now move win above this one */
        /* making sure to not violate the topmost rule */
        if (!(ptr->ex_style & WS_EX_TOPMOST) || (win->ex_style & WS_EX_TOPMOST))
        {
            list_remove( &win->entry );
            list_add_before( &ptr->entry, &win->entry );
        }
        break;
    }
}


/* mark parts of a window as needing a redraw */
DECL_HANDLER(redraw_window)
{
    unsigned int flags = req->flags;
    struct region *region = NULL;
    struct window *win;

    if (!req->window)
    {
        if (!(win = get_desktop_window( current ))) return;
    }
    else
    {
        if (!(win = get_window( req->window ))) return;
        if (is_desktop_window( win )) flags &= ~RDW_ALLCHILDREN;
    }

    if (!is_visible( win )) return;  /* nothing to do */

    if (flags & (RDW_VALIDATE|RDW_INVALIDATE))
    {
        if (get_req_data_size())  /* no data means whole rectangle */
        {
            if (!(region = create_region_from_req_data( get_req_data(), get_req_data_size() )))
                return;
            if (win->ex_style & WS_EX_LAYOUTRTL) mirror_region( &win->client_rect, region );
        }
    }

    redraw_window( win, region, (flags & RDW_INVALIDATE) && (flags & RDW_FRAME), flags );
    if (region) free_region( region );
}


/* set a window property */
DECL_HANDLER(set_window_property)
{
    struct unicode_str name = get_req_unicode_str();
    struct window *win = get_window( req->window );

    if (!win) return;

    if (name.len)
    {
        atom_t atom = add_global_atom( NULL, &name );
        if (atom)
        {
            set_property( win, atom, req->data, PROP_TYPE_STRING );
            release_global_atom( NULL, atom );
        }
    }
    else set_property( win, req->atom, req->data, PROP_TYPE_ATOM );
}


/* remove a window property */
DECL_HANDLER(remove_window_property)
{
    struct unicode_str name = get_req_unicode_str();
    struct window *win = get_window( req->window );

    if (win)
    {
        atom_t atom = name.len ? find_global_atom( NULL, &name ) : req->atom;
        if (atom) reply->data = remove_property( win, atom );
    }
}


/* get a window property */
DECL_HANDLER(get_window_property)
{
    struct unicode_str name = get_req_unicode_str();
    struct window *win = get_window( req->window );

    if (win)
    {
        atom_t atom = name.len ? find_global_atom( NULL, &name ) : req->atom;
        if (atom) reply->data = get_property( win, atom );
    }
}


/* get the list of properties of a window */
DECL_HANDLER(get_window_properties)
{
    struct property_data *data;
    int i, count, max = get_reply_max_size() / sizeof(*data);
    struct window *win = get_window( req->window );

    reply->total = 0;
    if (!win) return;

    for (i = count = 0; i < win->prop_inuse; i++)
        if (win->properties[i].type != PROP_TYPE_FREE) count++;
    reply->total = count;

    if (count > max) count = max;
    if (!count || !(data = set_reply_data_size( count * sizeof(*data) ))) return;

    for (i = 0; i < win->prop_inuse && count; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        data->atom   = win->properties[i].atom;
        data->string = (win->properties[i].type == PROP_TYPE_STRING);
        data->data   = win->properties[i].data;
        data++;
        count--;
    }
}


/* get the new window pointer for a desktop shell window, checking permissions */
/* helper for set_desktop_shell_windows request */
static int get_new_shell_window( struct window **win, user_handle_t handle )
{
    if (!handle)
    {
        *win = NULL;
        return 1;
    }
    else if (*win)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    *win = get_window( handle );
    return (*win != NULL);
}

/* Set/get the desktop shell windows */
DECL_HANDLER(set_desktop_shell_windows)
{
    struct desktop *desktop;
    struct window *new_shell_window, *new_shell_listview, *new_progman_window, *new_taskman_window;

    if (!(desktop = get_desktop_obj( current->process, current->desktop, 0 ))) return;

    new_shell_window   = desktop->shell_window;
    new_shell_listview = desktop->shell_listview;
    new_progman_window = desktop->progman_window;
    new_taskman_window = desktop->taskman_window;

    reply->old_shell_window   = new_shell_window ? new_shell_window->handle : 0;
    reply->old_shell_listview = new_shell_listview ? new_shell_listview->handle : 0;
    reply->old_progman_window = new_progman_window ? new_progman_window->handle : 0;
    reply->old_taskman_window = new_taskman_window ? new_taskman_window->handle : 0;

    if (req->flags & SET_DESKTOP_SHELL_WINDOWS)
    {
        if (!get_new_shell_window( &new_shell_window, req->shell_window )) goto done;
        if (!get_new_shell_window( &new_shell_listview, req->shell_listview )) goto done;
    }
    if (req->flags & SET_DESKTOP_PROGMAN_WINDOW)
    {
        if (!get_new_shell_window( &new_progman_window, req->progman_window )) goto done;
    }
    if (req->flags & SET_DESKTOP_TASKMAN_WINDOW)
    {
        if (!get_new_shell_window( &new_taskman_window, req->taskman_window )) goto done;
    }
    desktop->shell_window   = new_shell_window;
    desktop->shell_listview = new_shell_listview;
    desktop->progman_window = new_progman_window;
    desktop->taskman_window = new_taskman_window;

done:
    release_object( desktop );
}

/* retrieve layered info for a window */
DECL_HANDLER(get_window_layered_info)
{
    struct window *win = get_window( req->handle );

    if (!win) return;

    if (win->is_layered)
    {
        reply->color_key = win->color_key;
        reply->alpha     = win->alpha;
        reply->flags     = win->layered_flags;
    }
    else set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
}


/* set layered info for a window */
DECL_HANDLER(set_window_layered_info)
{
    struct window *win = get_window( req->handle );

    if (!win) return;

    if (win->ex_style & WS_EX_LAYERED)
    {
        int was_layered = win->is_layered;

        if (req->flags & LWA_ALPHA) win->alpha = req->alpha;
        else if (!win->is_layered) win->alpha = 0;  /* alpha init value is 0 */

        win->color_key     = req->color_key;
        win->layered_flags = req->flags;
        win->is_layered    = 1;
        /* repaint since we know now it's not going to use UpdateLayeredWindow */
        if (!was_layered) redraw_window( win, 0, 1, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    }
    else set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
}

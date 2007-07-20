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
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"

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
    obj_handle_t   handle;   /* property handle (user-defined storage) */
};

enum property_type
{
    PROP_TYPE_FREE,   /* free entry */
    PROP_TYPE_STRING, /* atom that was originally a string */
    PROP_TYPE_ATOM    /* plain atom */
};


struct window
{
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
    rectangle_t      window_rect;     /* window rectangle (relative to parent client area) */
    rectangle_t      visible_rect;    /* visible part of window rect (relative to parent client area) */
    rectangle_t      client_rect;     /* client rectangle (relative to parent client area) */
    struct region   *win_region;      /* region for shaped windows (relative to window rect) */
    struct region   *update_region;   /* update region (relative to window rect) */
    unsigned int     style;           /* window style */
    unsigned int     ex_style;        /* window extended style */
    unsigned int     id;              /* window id */
    void*            instance;        /* creator instance */
    int              is_unicode;      /* ANSI or unicode */
    unsigned long    user_data;       /* user-specific data */
    WCHAR           *text;            /* window caption text */
    unsigned int     paint_flags;     /* various painting flags */
    int              prop_inuse;      /* number of in-use window properties */
    int              prop_alloc;      /* number of allocated window properties */
    struct property *properties;      /* window properties array */
    int              nb_extra_bytes;  /* number of extra bytes */
    char             extra_bytes[1];  /* extra bytes storage */
};

#define PAINT_INTERNAL  0x01  /* internal WM_PAINT pending */
#define PAINT_ERASE     0x02  /* needs WM_ERASEBKGND */
#define PAINT_NONCLIENT 0x04  /* needs WM_NCPAINT */

/* growable array of user handles */
struct user_handle_array
{
    user_handle_t *handles;
    int            count;
    int            total;
};

/* global window pointers */
static struct window *shell_window;
static struct window *shell_listview;
static struct window *progman_window;
static struct window *taskman_window;

/* retrieve a pointer to a window from its handle */
static inline struct window *get_window( user_handle_t handle )
{
    struct window *ret = get_user_object( handle, USER_WINDOW );
    if (!ret) set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
    return ret;
}

/* check if window is the desktop */
static inline int is_desktop_window( const struct window *win )
{
    return !win->parent;  /* only desktop windows have no parent */
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

    list_remove( &win->entry );  /* unlink it from the previous location */

    if (parent)
    {
        win->parent = parent;
        list_add_head( &parent->children, &win->entry );

        /* if parent belongs to a different thread and the window isn't */
        /* top-level, attach the two threads */
        if (parent->thread && parent->thread != win->thread && !is_desktop_window(parent))
            attach_thread_input( win->thread, parent->thread );
    }
    else  /* move it to parent unlinked list */
    {
        list_add_head( &win->parent->unlinked, &win->entry );
    }
    return 1;
}

/* get next window in Z-order list */
static inline struct window *get_next_window( struct window *win )
{
    struct list *ptr = list_next( &win->parent->children, &win->entry );
    if (ptr == &win->parent->unlinked) ptr = NULL;
    return ptr ? LIST_ENTRY( ptr, struct window, entry ) : NULL;
}

/* get previous window in Z-order list */
static inline struct window *get_prev_window( struct window *win )
{
    struct list *ptr = list_prev( &win->parent->children, &win->entry );
    if (ptr == &win->parent->unlinked) ptr = NULL;
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
static void set_property( struct window *win, atom_t atom, obj_handle_t handle, enum property_type type )
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
            win->properties[i].handle = handle;
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
    win->properties[free].atom   = atom;
    win->properties[free].type   = type;
    win->properties[free].handle = handle;
}

/* remove a window property */
static obj_handle_t remove_property( struct window *win, atom_t atom )
{
    int i;

    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        if (win->properties[i].atom == atom)
        {
            release_global_atom( NULL, atom );
            win->properties[i].type = PROP_TYPE_FREE;
            return win->properties[i].handle;
        }
    }
    /* FIXME: last error? */
    return 0;
}

/* find a window property */
static obj_handle_t get_property( struct window *win, atom_t atom )
{
    int i;

    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        if (win->properties[i].atom == atom) return win->properties[i].handle;
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

/* destroy a window */
void destroy_window( struct window *win )
{
    /* destroy all children */
    while (!list_empty(&win->children))
        destroy_window( LIST_ENTRY( list_head(&win->children), struct window, entry ));
    while (!list_empty(&win->unlinked))
        destroy_window( LIST_ENTRY( list_head(&win->unlinked), struct window, entry ));

    /* reset global window pointers, if the corresponding window is destroyed */
    if (win == shell_window) shell_window = NULL;
    if (win == shell_listview) shell_listview = NULL;
    if (win == progman_window) progman_window = NULL;
    if (win == taskman_window) taskman_window = NULL;
    free_user_handle( win->handle );
    destroy_properties( win );
    list_remove( &win->entry );
    if (is_desktop_window(win))
    {
        assert( win->desktop->top_window == win );
        win->desktop->top_window = NULL;
    }
    detach_window_thread( win );
    if (win->win_region) free_region( win->win_region );
    if (win->update_region) free_region( win->update_region );
    if (win->class) release_class( win->class );
    free( win->text );
    memset( win, 0x55, sizeof(*win) + win->nb_extra_bytes - 1 );
    free( win );
}

/* get the process owning the top window of a given desktop */
struct process *get_top_window_owner( struct desktop *desktop )
{
    struct window *win = desktop->top_window;
    if (!win || !win->thread) return NULL;
    return win->thread->process;
}

/* attempt to close the desktop window when the last process using it is gone */
void close_desktop_window( struct desktop *desktop )
{
    struct window *win = desktop->top_window;
    if (win && win->thread) post_message( win->handle, WM_CLOSE, 0, 0 );
}

/* create a new window structure (note: the window is not linked in the window tree) */
static struct window *create_window( struct window *parent, struct window *owner,
                                     atom_t atom, void *instance )
{
    int extra_bytes;
    struct window *win;
    struct desktop *desktop;
    struct window_class *class;

    if (!(desktop = get_thread_desktop( current, DESKTOP_CREATEWINDOW ))) return NULL;

    if (!(class = grab_class( current->process, atom, instance, &extra_bytes )))
    {
        release_object( desktop );
        return NULL;
    }

    if (!(win = mem_alloc( sizeof(*win) + extra_bytes - 1 ))) goto failed;
    if (!(win->handle = alloc_user_handle( win, USER_WINDOW ))) goto failed;

    win->parent         = parent;
    win->owner          = owner ? owner->handle : 0;
    win->thread         = current;
    win->desktop        = desktop;
    win->class          = class;
    win->atom           = atom;
    win->last_active    = win->handle;
    win->win_region     = NULL;
    win->update_region  = NULL;
    win->style          = 0;
    win->ex_style       = 0;
    win->id             = 0;
    win->instance       = NULL;
    win->is_unicode     = 1;
    win->user_data      = 0;
    win->text           = NULL;
    win->paint_flags    = 0;
    win->prop_inuse     = 0;
    win->prop_alloc     = 0;
    win->properties     = NULL;
    win->nb_extra_bytes = extra_bytes;
    memset( win->extra_bytes, 0, extra_bytes );
    list_init( &win->children );
    list_init( &win->unlinked );

    /* parent must be on the same desktop */
    if (parent && parent->desktop != desktop)
    {
        set_error( STATUS_ACCESS_DENIED );
        goto failed;
    }

    /* if no parent, class must be the desktop */
    if (!parent && !is_desktop_class( class ))
    {
        set_error( STATUS_ACCESS_DENIED );
        goto failed;
    }

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
        assert( !desktop->top_window );
        desktop->top_window = win;
        set_process_default_desktop( current->process, desktop, current->desktop );
    }

    current->desktop_users++;
    return win;

failed:
    if (win)
    {
        if (win->handle) free_user_handle( win->handle );
        free( win );
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

    while ((win = next_user_handle( &handle, USER_WINDOW )))
    {
        if (win->thread != thread) continue;
        if (is_desktop_window( win )) detach_window_thread( win );
        else destroy_window( win );
    }
}

/* get the desktop window */
static struct window *get_desktop_window( struct thread *thread, int create )
{
    struct window *top_window;
    struct desktop *desktop = get_thread_desktop( thread, 0 );

    if (!desktop) return NULL;

    if (!(top_window = desktop->top_window) && create)
    {
        if ((top_window = create_window( NULL, NULL, DESKTOP_ATOM, 0 )))
        {
            detach_window_thread( top_window );
            top_window->style  = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        }
    }
    release_object( desktop );
    return top_window;
}

/* check whether child is a descendant of parent */
int is_child_window( user_handle_t parent, user_handle_t child )
{
    struct window *child_ptr = get_user_object( child, USER_WINDOW );
    struct window *parent_ptr = get_user_object( parent, USER_WINDOW );

    if (!child_ptr || !parent_ptr) return 0;
    while (child_ptr->parent)
    {
        if (child_ptr->parent == parent_ptr) return 1;
        child_ptr = child_ptr->parent;
    }
    return 0;
}

/* check whether window is a top-level window */
int is_top_level_window( user_handle_t window )
{
    struct window *win = get_user_object( window, USER_WINDOW );
    return (win && (is_desktop_window(win) || is_desktop_window(win->parent)));
}

/* make a window active if possible */
int make_window_active( user_handle_t window )
{
    struct window *owner, *win = get_window( window );

    if (!win) return 0;

    /* set last active for window and its owner */
    win->last_active = win->handle;
    if ((owner = get_user_object( win->owner, USER_WINDOW ))) owner->last_active = win->handle;
    return 1;
}

/* increment (or decrement) the window paint count */
static inline void inc_window_paint_count( struct window *win, int incr )
{
    if (win->thread) inc_queue_paint_count( win->thread, incr );
}

/* check if window and all its ancestors are visible */
static int is_visible( const struct window *win )
{
    while (win && win->parent)
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
    struct window *win = get_user_object( window, USER_WINDOW );
    if (!win) return 0;
    return is_visible( win );
}

/* check if point is inside the window */
static inline int is_point_in_window( struct window *win, int x, int y )
{
    if (!(win->style & WS_VISIBLE)) return 0; /* not visible */
    if ((win->style & (WS_POPUP|WS_CHILD|WS_DISABLED)) == (WS_CHILD|WS_DISABLED))
        return 0;  /* disabled child */
    if ((win->ex_style & (WS_EX_LAYERED|WS_EX_TRANSPARENT)) == (WS_EX_LAYERED|WS_EX_TRANSPARENT))
        return 0;  /* transparent */
    if (x < win->visible_rect.left || x >= win->visible_rect.right ||
        y < win->visible_rect.top || y >= win->visible_rect.bottom)
        return 0;  /* not in window */
    if (win->win_region &&
        !point_in_region( win->win_region, x - win->window_rect.left, y - win->window_rect.top ))
        return 0;  /* not in window region */
    return 1;
}

/* find child of 'parent' that contains the given point (in parent-relative coords) */
static struct window *child_window_from_point( struct window *parent, int x, int y )
{
    struct window *ptr;

    LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
    {
        if (!is_point_in_window( ptr, x, y )) continue;  /* skip it */

        /* if window is minimized or disabled, return at once */
        if (ptr->style & (WS_MINIMIZE|WS_DISABLED)) return ptr;

        /* if point is not in client area, return at once */
        if (x < ptr->client_rect.left || x >= ptr->client_rect.right ||
            y < ptr->client_rect.top || y >= ptr->client_rect.bottom)
            return ptr;

        return child_window_from_point( ptr, x - ptr->client_rect.left, y - ptr->client_rect.top );
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
        if (!is_point_in_window( ptr, x, y )) continue;  /* skip it */

        /* if point is in client area, and window is not minimized or disabled, check children */
        if (!(ptr->style & (WS_MINIMIZE|WS_DISABLED)) &&
            x >= ptr->client_rect.left && x < ptr->client_rect.right &&
            y >= ptr->client_rect.top && y < ptr->client_rect.bottom)
        {
            if (!get_window_children_from_point( ptr, x - ptr->client_rect.left,
                                                 y - ptr->client_rect.top, array ))
                return 0;
        }

        /* now add window to the array */
        if (!add_handle_to_array( array, ptr->handle )) return 0;
    }
    return 1;
}

/* find window containing point (in absolute coords) */
user_handle_t window_from_point( struct desktop *desktop, int x, int y )
{
    struct window *ret;

    if (!desktop->top_window) return 0;
    ret = child_window_from_point( desktop->top_window, x, y );
    return ret->handle;
}

/* return list of all windows containing point (in absolute coords) */
static int all_windows_from_point( struct window *top, int x, int y, struct user_handle_array *array )
{
    struct window *ptr;

    /* make point relative to top window */
    for (ptr = top->parent; ptr && !is_desktop_window(ptr); ptr = ptr->parent)
    {
        x -= ptr->client_rect.left;
        y -= ptr->client_rect.top;
    }

    if (!is_point_in_window( top, x, y )) return 1;

    /* if point is in client area, and window is not minimized or disabled, check children */
    if (!(top->style & (WS_MINIMIZE|WS_DISABLED)) &&
        x >= top->client_rect.left && x < top->client_rect.right &&
        y >= top->client_rect.top && y < top->client_rect.bottom)
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
    struct window *win = get_user_object( handle, USER_WINDOW );
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
    struct window *ptr, *win, *top_window = get_desktop_window( thread, 0 );

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
static inline void client_to_screen( struct window *win, int *x, int *y )
{
    for ( ; win && !is_desktop_window(win); win = win->parent)
    {
        *x += win->client_rect.left;
        *y += win->client_rect.top;
    }
}

/* convert coordinates from client to screen coords */
static inline void client_to_screen_rect( struct window *win, rectangle_t *rect )
{
    for ( ; win && !is_desktop_window(win); win = win->parent)
    {
        rect->left   += win->client_rect.left;
        rect->right  += win->client_rect.left;
        rect->top    += win->client_rect.top;
        rect->bottom += win->client_rect.top;
    }
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


/* compute the intersection of two rectangles; return 0 if the result is empty */
static inline int intersect_rect( rectangle_t *dst, const rectangle_t *src1, const rectangle_t *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return (dst->left < dst->right && dst->top < dst->bottom);
}


/* set the region to the client rect clipped by the window rect, in parent-relative coordinates */
static void set_region_client_rect( struct region *region, struct window *win )
{
    rectangle_t rect;

    intersect_rect( &rect, &win->window_rect, &win->client_rect );
    set_region_rect( region, &rect );
}


/* get the top-level window to clip against for a given window */
static inline struct window *get_top_clipping_window( struct window *win )
{
    while (win->parent && !is_desktop_window(win->parent)) win = win->parent;
    return win;
}


/* compute the visible region of a window, in window coordinates */
static struct region *get_visible_region( struct window *win, struct window *top, unsigned int flags )
{
    struct region *tmp = NULL, *region;
    int offset_x, offset_y;

    if (!(region = create_empty_region())) return NULL;

    /* first check if all ancestors are visible */

    if (!is_visible( win )) return region;  /* empty region */

    /* create a region relative to the window itself */

    if ((flags & DCX_PARENTCLIP) && win != top && win->parent)
    {
        set_region_client_rect( region, win->parent );
        offset_region( region, -win->parent->client_rect.left, -win->parent->client_rect.top );
    }
    else if (flags & DCX_WINDOW)
    {
        set_region_rect( region, &win->visible_rect );
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
        if (is_desktop_window(win)) offset_x = offset_y = 0;
        else
        {
            offset_x = win->client_rect.left;
            offset_y = win->client_rect.top;
        }
        if (!clip_children( win, NULL, region, offset_x, offset_y )) goto error;
    }

    /* clip siblings of ancestors */

    if (is_desktop_window(win)) offset_x = offset_y = 0;
    else
    {
        offset_x = win->window_rect.left;
        offset_y = win->window_rect.top;
    }

    if (top && top != win && (tmp = create_empty_region()) != NULL)
    {
        while (win != top && win->parent)
        {
            if (win->style & WS_CLIPSIBLINGS)
            {
                if (!clip_children( win->parent, win, region, 0, 0 )) goto error;
                if (is_region_empty( region )) break;
            }
            /* clip to parent client area */
            win = win->parent;
            if (!is_desktop_window(win))
            {
                offset_x += win->client_rect.left;
                offset_y += win->client_rect.top;
                offset_region( region, win->client_rect.left, win->client_rect.top );
            }
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


/* get the window class of a window */
struct window_class* get_window_class( user_handle_t window )
{
    struct window *win;
    if (!(win = get_window( window ))) return NULL;
    if (!win->class) set_error( STATUS_ACCESS_DENIED );
    return win->class;
}

/* return a copy of the specified region cropped to the window client or frame rectangle, */
/* and converted from client to window coordinates. Helper for (in)validate_window. */
static struct region *crop_region_to_win_rect( struct window *win, struct region *region, int frame )
{
    struct region *tmp = create_empty_region();

    if (!tmp) return NULL;

    /* get bounding rect in client coords */
    if (frame) set_region_rect( tmp, &win->window_rect );
    else set_region_client_rect( tmp, win );
    if (!is_desktop_window(win))
        offset_region( tmp, -win->client_rect.left, -win->client_rect.top );

    /* intersect specified region with bounding rect */
    if (region && !intersect_region( tmp, region, tmp )) goto done;
    if (is_region_empty( tmp )) goto done;

    /* map it to window coords */
    offset_region( tmp, win->client_rect.left - win->window_rect.left,
                   win->client_rect.top - win->window_rect.top );
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
        win->paint_flags &= ~(PAINT_ERASE | PAINT_NONCLIENT);
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


/* validate the non client area of a window */
static void validate_non_client( struct window *win )
{
    struct region *tmp;
    rectangle_t rect;

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


/* validate the update region of a window on all parents; helper for redraw_window */
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
    struct region *tmp;
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
            if (flags & RDW_NOERASE) win->paint_flags &= ~PAINT_ERASE;
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

    if (flags & RDW_UPDATENOW)
    {
        validate_parents( win );
        flags &= ~RDW_UPDATENOW;
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
        if (!rect_in_region( tmp, &child->window_rect )) continue;
        offset_region( tmp, -child->client_rect.left, -child->client_rect.top );
        redraw_window( child, tmp, 1, flags );
        offset_region( tmp, child->client_rect.left, child->client_rect.top );
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
        if (win->update_region) ret |= UPDATE_PAINT;
    }
    if (flags & UPDATE_INTERNALPAINT)
    {
        if (win->paint_flags & PAINT_INTERNAL) ret |= UPDATE_INTERNALPAINT;
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


/* expose a region of a window, looking for the top most parent that needs to be exposed */
/* the region is in window coordinates */
static void expose_window( struct window *win, struct window *top, struct region *region )
{
    struct window *parent, *ptr;
    int offset_x, offset_y;

    /* find the top most parent that doesn't clip either siblings or children */
    for (parent = ptr = win; ptr != top; ptr = ptr->parent)
    {
        if (!(ptr->style & WS_CLIPCHILDREN)) parent = ptr;
        if (!(ptr->style & WS_CLIPSIBLINGS)) parent = ptr->parent;
    }
    if (parent == win && parent != top && win->parent)
        parent = win->parent;  /* always go up at least one level if possible */

    offset_x = win->window_rect.left - win->client_rect.left;
    offset_y = win->window_rect.top - win->client_rect.top;
    for (ptr = win; ptr != parent && !is_desktop_window(ptr); ptr = ptr->parent)
    {
        offset_x += ptr->client_rect.left;
        offset_y += ptr->client_rect.top;
    }
    offset_region( region, offset_x, offset_y );
    redraw_window( parent, region, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
    offset_region( region, -offset_x, -offset_y );
}


/* set the window and client rectangles, updating the update region if necessary */
static void set_window_pos( struct window *win, struct window *previous,
                            unsigned int swp_flags, const rectangle_t *window_rect,
                            const rectangle_t *client_rect, const rectangle_t *visible_rect,
                            const rectangle_t *valid_rects )
{
    struct region *old_vis_rgn = NULL, *new_vis_rgn;
    const rectangle_t old_window_rect = win->window_rect;
    const rectangle_t old_visible_rect = win->visible_rect;
    const rectangle_t old_client_rect = win->client_rect;
    struct window *top = get_top_clipping_window( win );
    int visible = (win->style & WS_VISIBLE) || (swp_flags & SWP_SHOWWINDOW);

    if (win->parent && !is_visible( win->parent )) visible = 0;

    if (visible && !(old_vis_rgn = get_visible_region( win, top, DCX_WINDOW ))) return;

    /* set the new window info before invalidating anything */

    win->window_rect  = *window_rect;
    win->visible_rect = *visible_rect;
    win->client_rect  = *client_rect;
    if (!(swp_flags & SWP_NOZORDER) && win->parent)
    {
        list_remove( &win->entry );  /* unlink it from the previous location */
        if (previous) list_add_after( &previous->entry, &win->entry );
        else list_add_head( &win->parent->children, &win->entry );
    }
    if (swp_flags & SWP_SHOWWINDOW) win->style |= WS_VISIBLE;
    else if (swp_flags & SWP_HIDEWINDOW) win->style &= ~WS_VISIBLE;

    /* if the window is not visible, everything is easy */
    if (!visible) return;

    if (!(new_vis_rgn = get_visible_region( win, top, DCX_WINDOW )))
    {
        free_region( old_vis_rgn );
        clear_error();  /* ignore error since the window info has been modified already */
        return;
    }

    /* expose anything revealed by the change */

    if (!(swp_flags & SWP_NOREDRAW))
    {
        offset_region( old_vis_rgn, old_window_rect.left - window_rect->left,
                       old_window_rect.top - window_rect->top );
        if (xor_region( new_vis_rgn, old_vis_rgn, new_vis_rgn ))
            expose_window( win, top, new_vis_rgn );
    }
    free_region( old_vis_rgn );

    if (!(win->style & WS_VISIBLE))
    {
        /* clear the update region since the window is no longer visible */
        validate_whole_window( win );
        validate_children( win );
        goto done;
    }

    /* crop update region to the new window rect */

    if (win->update_region &&
        (window_rect->right - window_rect->left < old_window_rect.right - old_window_rect.left ||
         window_rect->bottom - window_rect->top < old_window_rect.bottom - old_window_rect.top))
    {
        struct region *tmp = create_empty_region();
        if (tmp)
        {
            set_region_rect( tmp, window_rect );
            if (!is_desktop_window(win))
                offset_region( tmp, -window_rect->left, -window_rect->top );
            if (intersect_region( tmp, win->update_region, tmp ))
                set_update_region( win, tmp );
            else
                free_region( tmp );
        }
    }

    if (swp_flags & SWP_NOREDRAW) goto done;  /* do not repaint anything */

    /* expose the whole non-client area if it changed in any way */

    if ((swp_flags & SWP_FRAMECHANGED) ||
        memcmp( window_rect, &old_window_rect, sizeof(old_window_rect) ) ||
        memcmp( visible_rect, &old_visible_rect, sizeof(old_visible_rect) ) ||
        memcmp( client_rect, &old_client_rect, sizeof(old_client_rect) ))
    {
        struct region *tmp = create_empty_region();

        if (tmp)
        {
            /* subtract the valid portion of client rect from the total region */
            if (!memcmp( client_rect, &old_client_rect, sizeof(old_client_rect) ))
                set_region_rect( tmp, client_rect );
            else if (valid_rects)
                set_region_rect( tmp, &valid_rects[0] );

            set_region_rect( new_vis_rgn, window_rect );
            if (subtract_region( tmp, new_vis_rgn, tmp ))
            {
                if (!is_desktop_window(win))
                    offset_region( tmp, -client_rect->left, -client_rect->top );
                redraw_window( win, tmp, 1, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
            }
            free_region( tmp );
        }
    }

done:
    free_region( new_vis_rgn );
    clear_error();  /* we ignore out of memory errors once the new rects have been set */
}


/* set the window region, updating the update region if necessary */
static void set_window_region( struct window *win, struct region *region, int redraw )
{
    struct region *old_vis_rgn = NULL, *new_vis_rgn;
    struct window *top = get_top_clipping_window( win );

    /* no need to redraw if window is not visible */
    if (redraw && !is_visible( win )) redraw = 0;

    if (redraw) old_vis_rgn = get_visible_region( win, top, DCX_WINDOW );

    if (win->win_region) free_region( win->win_region );
    win->win_region = region;

    if (old_vis_rgn && (new_vis_rgn = get_visible_region( win, top, DCX_WINDOW )))
    {
        /* expose anything revealed by the change */
        if (xor_region( new_vis_rgn, old_vis_rgn, new_vis_rgn ))
            expose_window( win, top, new_vis_rgn );
        free_region( new_vis_rgn );
    }

    if (old_vis_rgn) free_region( old_vis_rgn );
    clear_error();  /* we ignore out of memory errors since the region has been set */
}


/* create a window */
DECL_HANDLER(create_window)
{
    struct window *win, *parent, *owner = NULL;

    reply->handle = 0;

    if (!req->parent) parent = get_desktop_window( current, 0 );
    else if (!(parent = get_window( req->parent ))) return;

    if (req->owner)
    {
        if (!(owner = get_window( req->owner ))) return;
        if (is_desktop_window(owner)) owner = NULL;
        else if (!is_desktop_window(parent))
        {
            /* an owned window must be created as top-level */
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
        else /* owner must be a top-level window */
            while (!is_desktop_window(owner->parent)) owner = owner->parent;
    }
    if (!(win = create_window( parent, owner, req->atom, req->instance ))) return;

    reply->handle    = win->handle;
    reply->parent    = win->parent ? win->parent->handle : 0;
    reply->owner     = win->owner;
    reply->extra     = win->nb_extra_bytes;
    reply->class_ptr = get_class_client_ptr( win->class );
}


/* set the parent of a window */
DECL_HANDLER(set_parent)
{
    struct window *win, *parent = NULL;

    if (!(win = get_window( req->handle ))) return;
    if (req->parent && !(parent = get_window( req->parent ))) return;

    if (is_desktop_window(win))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    reply->old_parent  = win->parent->handle;
    reply->full_parent = parent ? parent->handle : 0;
    set_parent_window( win, parent );
}


/* destroy a window */
DECL_HANDLER(destroy_window)
{
    struct window *win = get_window( req->handle );
    if (win)
    {
        if (!is_desktop_window(win)) destroy_window( win );
        else if (win->thread == current) detach_window_thread( win );
        else set_error( STATUS_ACCESS_DENIED );
    }
}


/* retrieve the desktop window for the current thread */
DECL_HANDLER(get_desktop_window)
{
    struct window *win = get_desktop_window( current, req->force );

    if (win) reply->handle = win->handle;
}


/* set a window owner */
DECL_HANDLER(set_window_owner)
{
    struct window *win = get_window( req->handle );
    struct window *owner = NULL;

    if (!win) return;
    if (req->owner && !(owner = get_window( req->owner ))) return;
    if (is_desktop_window(win))
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    reply->prev_owner = win->owner;
    reply->full_owner = win->owner = owner ? owner->handle : 0;
}


/* get information from a window handle */
DECL_HANDLER(get_window_info)
{
    struct window *win = get_window( req->handle );

    reply->full_handle = 0;
    reply->tid = reply->pid = 0;
    if (win)
    {
        reply->full_handle = win->handle;
        reply->last_active = win->handle;
        reply->is_unicode  = win->is_unicode;
        if (get_user_object( win->last_active, USER_WINDOW )) reply->last_active = win->last_active;
        if (win->thread)
        {
            reply->tid  = get_thread_id( win->thread );
            reply->pid  = get_process_id( win->thread->process );
            reply->atom = win->class ? get_class_atom( win->class ) : DESKTOP_ATOM;
        }
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
    if (req->flags & SET_WIN_EXSTYLE) win->ex_style = req->ex_style;
    if (req->flags & SET_WIN_ID) win->id = req->id;
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


/* get a list of the window children */
DECL_HANDLER(get_window_children)
{
    struct window *ptr, *parent = get_window( req->parent );
    int total = 0;
    user_handle_t *data;
    data_size_t len;

    if (parent)
    {
        LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
        {
            if (req->atom && get_class_atom(ptr->class) != req->atom) continue;
            if (req->tid && get_thread_id(ptr->thread) != req->tid) continue;
            total++;
        }
    }
    reply->count = total;
    len = min( get_reply_max_size(), total * sizeof(user_handle_t) );
    if (len && ((data = set_reply_data_size( len ))))
    {
        LIST_FOR_EACH_ENTRY( ptr, &parent->children, struct window, entry )
        {
            if (len < sizeof(*data)) break;
            if (req->atom && get_class_atom(ptr->class) != req->atom) continue;
            if (req->tid && get_thread_id(ptr->thread) != req->tid) continue;
            *data++ = ptr->handle;
            len -= sizeof(*data);
        }
    }
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
    if (!all_windows_from_point( parent, req->x, req->y, &array )) return;

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
        if ((ptr = get_next_window( win ))) reply->next_sibling = ptr->handle;
        if ((ptr = get_prev_window( win ))) reply->prev_sibling = ptr->handle;
        if ((ptr = get_first_child( parent ))) reply->first_sibling = ptr->handle;
        if ((ptr = get_last_child( parent ))) reply->last_sibling = ptr->handle;
    }
    if ((ptr = get_first_child( win ))) reply->first_child = ptr->handle;
    if ((ptr = get_last_child( win ))) reply->last_child = ptr->handle;
}


/* set the position and Z order of a window */
DECL_HANDLER(set_window_pos)
{
    const rectangle_t *visible_rect = NULL, *valid_rects = NULL;
    struct window *previous = NULL;
    struct window *win = get_window( req->handle );
    unsigned int flags = req->flags;

    if (!win) return;
    if (!win->parent) flags |= SWP_NOZORDER;  /* no Z order for the desktop */

    if (!(flags & SWP_NOZORDER))
    {
        if (!req->previous)  /* special case: HWND_TOP */
        {
            if (get_first_child(win->parent) == win) flags |= SWP_NOZORDER;
        }
        else if (req->previous == (user_handle_t)1)  /* special case: HWND_BOTTOM */
        {
            previous = get_last_child( win->parent );
        }
        else
        {
            if (!(previous = get_window( req->previous ))) return;
            /* previous must be a sibling */
            if (previous->parent != win->parent)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
        }
        if (previous == win) flags |= SWP_NOZORDER;  /* nothing to do */
    }

    /* window rectangle must be ordered properly */
    if (req->window.right < req->window.left || req->window.bottom < req->window.top)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (get_req_data_size() >= sizeof(rectangle_t)) visible_rect = get_req_data();
    if (get_req_data_size() >= 3 * sizeof(rectangle_t)) valid_rects = visible_rect + 1;

    if (!visible_rect) visible_rect = &req->window;
    set_window_pos( win, previous, flags, &req->window, &req->client, visible_rect, valid_rects );
    reply->new_style = win->style;
}


/* get the window and client rectangles of a window */
DECL_HANDLER(get_window_rectangles)
{
    struct window *win = get_window( req->handle );

    if (win)
    {
        reply->window  = win->window_rect;
        reply->visible = win->visible_rect;
        reply->client  = win->client_rect;
    }
}


/* get the window text */
DECL_HANDLER(get_window_text)
{
    struct window *win = get_window( req->handle );

    if (win && win->text)
    {
        data_size_t len = strlenW( win->text ) * sizeof(WCHAR);
        if (len > get_reply_max_size()) len = get_reply_max_size();
        set_reply_data( win->text, len );
    }
}


/* set the window text */
DECL_HANDLER(set_window_text)
{
    struct window *win = get_window( req->handle );

    if (win)
    {
        WCHAR *text = NULL;
        data_size_t len = get_req_data_size() / sizeof(WCHAR);
        if (len)
        {
            if (!(text = mem_alloc( (len+1) * sizeof(WCHAR) ))) return;
            memcpy( text, get_req_data(), len * sizeof(WCHAR) );
            text[len] = 0;
        }
        free( win->text );
        win->text = text;
    }
}


/* get the coordinates offset between two windows */
DECL_HANDLER(get_windows_offset)
{
    struct window *win;

    reply->x = reply->y = 0;
    if (req->from)
    {
        if (!(win = get_window( req->from ))) return;
        while (win && !is_desktop_window(win))
        {
            reply->x += win->client_rect.left;
            reply->y += win->client_rect.top;
            win = win->parent;
        }
    }
    if (req->to)
    {
        if (!(win = get_window( req->to ))) return;
        while (win && !is_desktop_window(win))
        {
            reply->x -= win->client_rect.left;
            reply->y -= win->client_rect.top;
            win = win->parent;
        }
    }
}


/* get the visible region of a window */
DECL_HANDLER(get_visible_region)
{
    struct region *region;
    struct window *top, *win = get_window( req->window );

    if (!win) return;

    top = get_top_clipping_window( win );
    if ((region = get_visible_region( win, top, req->flags )))
    {
        rectangle_t *data;
        map_win_region_to_screen( win, region );
        data = get_region_data_and_free( region, get_reply_max_size(), &reply->total_size );
        if (data) set_reply_data_ptr( data, reply->total_size );
    }
    reply->top_win  = top->handle;
    reply->top_rect = top->visible_rect;

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
}


/* get the window region */
DECL_HANDLER(get_window_region)
{
    struct window *win = get_window( req->window );

    if (!win) return;

    if (win->win_region)
    {
        rectangle_t *data = get_region_data( win->win_region, get_reply_max_size(), &reply->total_size );
        if (data) set_reply_data_ptr( data, reply->total_size );
    }
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
    }
    set_window_region( win, region, req->redraw );
}


/* get a window update region */
DECL_HANDLER(get_update_region)
{
    rectangle_t *data;
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
        map_win_region_to_screen( win, region );
        if (!(data = get_region_data_and_free( region, get_reply_max_size(),
                                               &reply->total_size ))) return;
        set_reply_data_ptr( data, reply->total_size );
    }

    if (reply->flags & (UPDATE_PAINT|UPDATE_INTERNALPAINT)) /* validate everything */
    {
        validate_whole_window( win );
    }
    else
    {
        if (reply->flags & UPDATE_NONCLIENT) validate_non_client( win );
        if (reply->flags & UPDATE_ERASE)
        {
            win->paint_flags &= ~PAINT_ERASE;
            /* desktop window only gets erased, not repainted */
            if (is_desktop_window(win)) validate_whole_window( win );
        }
    }
}


/* update the z order of a window so that a given rectangle is fully visible */
DECL_HANDLER(update_window_zorder)
{
    rectangle_t tmp;
    struct window *ptr, *win = get_window( req->window );

    if (!win || !win->parent || !is_visible( win )) return;  /* nothing to do */

    LIST_FOR_EACH_ENTRY( ptr, &win->parent->children, struct window, entry )
    {
        if (ptr == win) break;
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->ex_style & WS_EX_TRANSPARENT) continue;
        if (!intersect_rect( &tmp, &ptr->visible_rect, &req->rect )) continue;
        if (ptr->win_region && !rect_in_region( ptr->win_region, &req->rect )) continue;
        /* found a window obscuring the rectangle, now move win above this one */
        list_remove( &win->entry );
        list_add_before( &ptr->entry, &win->entry );
        break;
    }
}


/* mark parts of a window as needing a redraw */
DECL_HANDLER(redraw_window)
{
    struct region *region = NULL;
    struct window *win = get_window( req->window );

    if (!win) return;
    if (!is_visible( win )) return;  /* nothing to do */

    if (req->flags & (RDW_VALIDATE|RDW_INVALIDATE))
    {
        if (get_req_data_size())  /* no data means whole rectangle */
        {
            if (!(region = create_region_from_req_data( get_req_data(), get_req_data_size() )))
                return;
        }
    }

    redraw_window( win, region, (req->flags & RDW_INVALIDATE) && (req->flags & RDW_FRAME),
                   req->flags );
    if (region) free_region( region );
}


/* set a window property */
DECL_HANDLER(set_window_property)
{
    struct window *win = get_window( req->window );

    if (!win) return;

    if (get_req_data_size())
    {
        atom_t atom = add_global_atom( NULL, get_req_data(), get_req_data_size() / sizeof(WCHAR) );
        if (atom)
        {
            set_property( win, atom, req->handle, PROP_TYPE_STRING );
            release_global_atom( NULL, atom );
        }
    }
    else set_property( win, req->atom, req->handle, PROP_TYPE_ATOM );
}


/* remove a window property */
DECL_HANDLER(remove_window_property)
{
    struct window *win = get_window( req->window );

    if (win)
    {
        atom_t atom = req->atom;
        if (get_req_data_size()) atom = find_global_atom( NULL, get_req_data(),
                                                          get_req_data_size() / sizeof(WCHAR) );
        if (atom) reply->handle = remove_property( win, atom );
    }
}


/* get a window property */
DECL_HANDLER(get_window_property)
{
    struct window *win = get_window( req->window );

    if (win)
    {
        atom_t atom = req->atom;
        if (get_req_data_size()) atom = find_global_atom( NULL, get_req_data(),
                                                          get_req_data_size() / sizeof(WCHAR) );
        if (atom) reply->handle = get_property( win, atom );
    }
}


/* get the list of properties of a window */
DECL_HANDLER(get_window_properties)
{
    property_data_t *data;
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
        data->handle = win->properties[i].handle;
        data++;
        count--;
    }
}


/* get the new window pointer for a global window, checking permissions */
/* helper for set_global_windows request */
static int get_new_global_window( struct window **win, user_handle_t handle )
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

/* Set/get the global windows */
DECL_HANDLER(set_global_windows)
{
    struct window *new_shell_window   = shell_window;
    struct window *new_shell_listview = shell_listview;
    struct window *new_progman_window = progman_window;
    struct window *new_taskman_window = taskman_window;

    reply->old_shell_window   = shell_window ? shell_window->handle : 0;
    reply->old_shell_listview = shell_listview ? shell_listview->handle : 0;
    reply->old_progman_window = progman_window ? progman_window->handle : 0;
    reply->old_taskman_window = taskman_window ? taskman_window->handle : 0;

    if (req->flags & SET_GLOBAL_SHELL_WINDOWS)
    {
        if (!get_new_global_window( &new_shell_window, req->shell_window )) return;
        if (!get_new_global_window( &new_shell_listview, req->shell_listview )) return;
    }
    if (req->flags & SET_GLOBAL_PROGMAN_WINDOW)
    {
        if (!get_new_global_window( &new_progman_window, req->progman_window )) return;
    }
    if (req->flags & SET_GLOBAL_TASKMAN_WINDOW)
    {
        if (!get_new_global_window( &new_taskman_window, req->taskman_window )) return;
    }
    shell_window   = new_shell_window;
    shell_listview = new_shell_listview;
    progman_window = new_progman_window;
    taskman_window = new_taskman_window;
}

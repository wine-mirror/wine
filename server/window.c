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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

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
    struct window   *first_child;     /* first child in Z-order */
    struct window   *last_child;      /* last child in Z-order */
    struct window   *first_unlinked;  /* first child not linked in the Z-order list */
    struct window   *next;            /* next window in Z-order */
    struct window   *prev;            /* prev window in Z-order */
    user_handle_t    handle;          /* full handle for this window */
    struct thread   *thread;          /* thread owning the window */
    struct window_class *class;       /* window class */
    atom_t           atom;            /* class atom */
    user_handle_t    last_active;     /* last active popup */
    rectangle_t      window_rect;     /* window rectangle (relative to parent client area) */
    rectangle_t      client_rect;     /* client rectangle (relative to parent client area) */
    struct region   *win_region;      /* region for shaped windows (relative to window rect) */
    struct region   *update_region;   /* update region (relative to window rect) */
    unsigned int     style;           /* window style */
    unsigned int     ex_style;        /* window extended style */
    unsigned int     id;              /* window id */
    void*            instance;        /* creator instance */
    void*            user_data;       /* user-specific data */
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

static struct window *top_window;  /* top-level (desktop) window */

/* global window pointers */
static struct window *shell_window;
static struct window *shell_listview;
static struct window *progman_window;
static struct window *taskman_window;

/* retrieve a pointer to a window from its handle */
inline static struct window *get_window( user_handle_t handle )
{
    struct window *ret = get_user_object( handle, USER_WINDOW );
    if (!ret) set_error( STATUS_INVALID_HANDLE );
    return ret;
}

/* unlink a window from the tree */
static void unlink_window( struct window *win )
{
    struct window *parent = win->parent;

    assert( parent );

    if (win->next) win->next->prev = win->prev;
    else if (parent->last_child == win) parent->last_child = win->prev;

    if (win->prev) win->prev->next = win->next;
    else if (parent->first_child == win) parent->first_child = win->next;
    else if (parent->first_unlinked == win) parent->first_unlinked = win->next;
}


/* link a window into the tree (or unlink it if the new parent is NULL)  */
static void link_window( struct window *win, struct window *parent, struct window *previous )
{
    unlink_window( win );  /* unlink it from the previous location */

    if (parent)
    {
        win->parent = parent;
        if ((win->prev = previous))
        {
            if ((win->next = previous->next)) win->next->prev = win;
            else if (win->parent->last_child == previous) win->parent->last_child = win;
            win->prev->next = win;
        }
        else
        {
            if ((win->next = parent->first_child)) win->next->prev = win;
            else win->parent->last_child = win;
            parent->first_child = win;
        }
    }
    else  /* move it to parent unlinked list */
    {
        parent = win->parent;
        if ((win->next = parent->first_unlinked)) win->next->prev = win;
        win->prev = NULL;
        parent->first_unlinked = win;
    }
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
static void set_property( struct window *win, atom_t atom, obj_handle_t handle,
                          enum property_type type )
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
    if (!grab_global_atom( atom )) return;
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
                release_global_atom( atom );
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
            release_global_atom( atom );
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
inline static void destroy_properties( struct window *win )
{
    int i;

    if (!win->properties) return;
    for (i = 0; i < win->prop_inuse; i++)
    {
        if (win->properties[i].type == PROP_TYPE_FREE) continue;
        release_global_atom( win->properties[i].atom );
    }
    free( win->properties );
}

/* destroy a window */
static void destroy_window( struct window *win )
{
    assert( win != top_window );

    /* destroy all children */
    while (win->first_child) destroy_window( win->first_child );
    while (win->first_unlinked) destroy_window( win->first_unlinked );

    if (win->thread->queue)
    {
        if (win->update_region) inc_queue_paint_count( win->thread, -1 );
        if (win->paint_flags & PAINT_INTERNAL) inc_queue_paint_count( win->thread, -1 );
        queue_cleanup_window( win->thread, win->handle );
    }
    /* reset global window pointers, if the corresponding window is destroyed */
    if (win == shell_window) shell_window = NULL;
    if (win == shell_listview) shell_listview = NULL;
    if (win == progman_window) progman_window = NULL;
    if (win == taskman_window) taskman_window = NULL;
    free_user_handle( win->handle );
    destroy_properties( win );
    unlink_window( win );
    if (win->win_region) free_region( win->win_region );
    if (win->update_region) free_region( win->update_region );
    release_class( win->class );
    if (win->text) free( win->text );
    memset( win, 0x55, sizeof(*win) );
    free( win );
}

/* create a new window structure (note: the window is not linked in the window tree) */
static struct window *create_window( struct window *parent, struct window *owner,
                                     atom_t atom, void *instance )
{
    int extra_bytes;
    struct window *win;
    struct window_class *class = grab_class( current->process, atom, instance, &extra_bytes );

    if (!class) return NULL;

    win = mem_alloc( sizeof(*win) + extra_bytes - 1 );
    if (!win)
    {
        release_class( class );
        return NULL;
    }

    if (!(win->handle = alloc_user_handle( win, USER_WINDOW )))
    {
        release_class( class );
        free( win );
        return NULL;
    }
    win->parent         = parent;
    win->owner          = owner ? owner->handle : 0;
    win->first_child    = NULL;
    win->last_child     = NULL;
    win->first_unlinked = NULL;
    win->thread         = current;
    win->class          = class;
    win->atom           = atom;
    win->last_active    = win->handle;
    win->win_region     = NULL;
    win->update_region  = NULL;
    win->style          = 0;
    win->ex_style       = 0;
    win->id             = 0;
    win->instance       = NULL;
    win->user_data      = NULL;
    win->text           = NULL;
    win->paint_flags    = 0;
    win->prop_inuse     = 0;
    win->prop_alloc     = 0;
    win->properties     = NULL;
    win->nb_extra_bytes = extra_bytes;
    memset( win->extra_bytes, 0, extra_bytes );

    if (parent)  /* put it on parent unlinked list */
    {
        if ((win->next = parent->first_unlinked)) win->next->prev = win;
        win->prev = NULL;
        parent->first_unlinked = win;
    }
    else win->next = win->prev = NULL;

    /* if parent belongs to a different thread, attach the two threads */
    if (parent && parent->thread && parent->thread != current)
        attach_thread_input( current, parent->thread );
    return win;
}

/* destroy all windows belonging to a given thread */
void destroy_thread_windows( struct thread *thread )
{
    user_handle_t handle = 0;
    struct window *win;

    while ((win = next_user_handle( &handle, USER_WINDOW )))
    {
        if (win->thread != thread) continue;
        destroy_window( win );
    }
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
    return (win && win->parent == top_window);
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
    while (win && win != top_window)
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
    if (x < win->window_rect.left || x >= win->window_rect.right ||
        y < win->window_rect.top || y >= win->window_rect.bottom)
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

    for (ptr = parent->first_child; ptr; ptr = ptr->next)
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

    for (ptr = parent->first_child; ptr; ptr = ptr->next)
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
user_handle_t window_from_point( int x, int y )
{
    struct window *ret;

    if (!top_window) return 0;
    ret = child_window_from_point( top_window, x, y );
    return ret->handle;
}

/* return list of all windows containing point (in absolute coords) */
static int all_windows_from_point( struct window *top, int x, int y, struct user_handle_array *array )
{
    struct window *ptr;

    /* make point relative to top window */
    for (ptr = top->parent; ptr && ptr != top_window; ptr = ptr->parent)
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
        if (!get_window_children_from_point( top, x - top->client_rect.left,
                                             y - top->client_rect.top, array ))
            return 0;
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

    for (ptr = parent->first_child; ptr && !ret; ptr = ptr->next)
    {
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->thread == thread && win_needs_repaint( ptr ))
            ret = ptr;
        else if (!(ptr->style & WS_MINIMIZE)) /* explore its children */
            ret = find_child_to_repaint( ptr, thread );
    }

    if (ret && (ret->ex_style & WS_EX_TRANSPARENT))
    {
        /* transparent window, check for non-transparent sibling to paint first */
        for (ptr = ret->next; ptr; ptr = ptr->next)
        {
            if (!(ptr->style & WS_VISIBLE)) continue;
            if (ptr->ex_style & WS_EX_TRANSPARENT) continue;
            if (ptr->thread != thread) continue;
            if (win_needs_repaint( ptr )) return ptr;
        }
    }
    return ret;
}


/* find a window that needs to receive a WM_PAINT */
user_handle_t find_window_to_repaint( user_handle_t parent, struct thread *thread )
{
    struct window *ptr, *win = find_child_to_repaint( top_window, thread );

    if (win)
    {
        if (!parent) return win->handle;
        /* check that it is a child of the specified parent */
        for (ptr = win; ptr; ptr = ptr->parent)
            if (ptr->handle == parent) return win->handle;
        /* otherwise don't return any window, we don't repaint a child before its parent */
    }
    return 0;
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


/* clip all children of a given window out of the visible region */
static struct region *clip_children( struct window *parent, struct window *last,
                                     struct region *region, int offset_x, int offset_y )
{
    struct window *ptr;
    struct region *tmp = create_empty_region();

    if (!tmp) return NULL;
    for (ptr = parent->first_child; ptr && ptr != last; ptr = ptr->next)
    {
        if (!(ptr->style & WS_VISIBLE)) continue;
        if (ptr->ex_style & WS_EX_TRANSPARENT) continue;
        set_region_rect( tmp, &ptr->window_rect );
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


/* compute the visible region of a window */
static struct region *get_visible_region( struct window *win, struct window *top,
                                          unsigned int flags )
{
    struct region *tmp, *region;
    int offset_x, offset_y;

    if (!(region = create_empty_region())) return NULL;

    /* first check if all ancestors are visible */

    if (!is_visible( win )) return region;  /* empty region */

    /* create a region relative to the window itself */

    if ((flags & DCX_PARENTCLIP) && win != top && win->parent)
    {
        rectangle_t rect;
        rect.left = rect.top = 0;
        rect.right = win->parent->client_rect.right - win->parent->client_rect.left;
        rect.bottom = win->parent->client_rect.bottom - win->parent->client_rect.top;
        set_region_rect( region, &rect );
        offset_x = win->client_rect.left;
        offset_y = win->client_rect.top;
    }
    else if (flags & DCX_WINDOW)
    {
        set_region_rect( region, &win->window_rect );
        if (win->win_region && !intersect_window_region( region, win )) goto error;
        offset_x = win->window_rect.left;
        offset_y = win->window_rect.top;
    }
    else
    {
        set_region_rect( region, &win->client_rect );
        if (win->win_region && !intersect_window_region( region, win )) goto error;
        offset_x = win->client_rect.left;
        offset_y = win->client_rect.top;
    }
    offset_region( region, -offset_x, -offset_y );

    /* clip children */

    if (flags & DCX_CLIPCHILDREN)
    {
        if (!clip_children( win, NULL, region,
                            offset_x - win->client_rect.left,
                            offset_y - win->client_rect.top )) goto error;
    }

    /* clip siblings of ancestors */

    if (top && top != win && (tmp = create_empty_region()) != NULL)
    {
        offset_region( region, offset_x, offset_y );  /* make it relative to parent */
        while (win != top && win->parent)
        {
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
            set_region_rect( tmp, &win->client_rect );
            if (win->win_region && !intersect_window_region( tmp, win ))
            {
                free_region( tmp );
                goto error;
            }
            if (!intersect_region( region, region, tmp ))
            {
                free_region( tmp );
                goto error;
            }
            if (is_region_empty( region )) break;
        }
        offset_region( region, -offset_x, -offset_y );  /* make it relative to target window again */
        free_region( tmp );
    }
    return region;

error:
    free_region( region );
    return NULL;
}


/* get the window class of a window */
struct window_class* get_window_class( user_handle_t window )
{
    struct window *win;
    if (!(win = get_window( window ))) return NULL;
    return win->class;
}

/* return a copy of the specified region cropped to the window client or frame rectangle, */
/* and converted from client to window coordinates. Helper for (in)validate_window. */
static struct region *crop_region_to_win_rect( struct window *win, struct region *region, int frame )
{
    rectangle_t rect;
    struct region *tmp = create_empty_region();

    if (!tmp) return NULL;

    /* get bounding rect in client coords */
    if (frame)
    {
        rect.left   = win->window_rect.left - win->client_rect.left;
        rect.top    = win->window_rect.top - win->client_rect.top;
        rect.right  = win->window_rect.right - win->client_rect.left;
        rect.bottom = win->window_rect.bottom - win->client_rect.top;
    }
    else
    {
        rect.left   = 0;
        rect.top    = 0;
        rect.right  = win->client_rect.right - win->client_rect.left;
        rect.bottom = win->client_rect.bottom - win->client_rect.top;
    }
    set_region_rect( tmp, &rect );

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
        if (win->update_region) inc_window_paint_count( win, -1 );
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

    if ((tmp = create_region( &rect, 1 )))
    {
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


/* validate the update region of a window on all parents; helper for redraw_window */
static void validate_parents( struct window *child )
{
    int offset_x = 0, offset_y = 0;
    struct window *win = child;
    struct region *tmp = NULL;

    if (!child->update_region) return;

    while (win->parent && win->parent != top_window)
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

    for (child = win->first_child; child; child = child->next)
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
static unsigned int get_child_update_flags( struct window *win, unsigned int flags,
                                            struct window **child )
{
    struct window *ptr;
    unsigned int ret = 0;

    for (ptr = win->first_child; ptr && !ret; ptr = ptr->next)
    {
        if (!(ptr->style & WS_VISIBLE)) continue;
        if ((ret = get_update_flags( ptr, flags )) != 0)
        {
            *child = ptr;
            break;
        }
        if (ptr->style & WS_MINIMIZE) continue;

        /* Note: the WS_CLIPCHILDREN test is the opposite of the invalidation case,
         * here we only want to repaint children of windows that clip them, others
         * need to wait for WM_PAINT to be done in the parent first.
         */
        if (!(flags & UPDATE_NOCHILDREN) &&
            ((flags & UPDATE_ALLCHILDREN) || (ptr->style & WS_CLIPCHILDREN)))
            ret = get_child_update_flags( ptr, flags, child );
    }
    return ret;
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
    for (ptr = win; ptr != parent; ptr = ptr->parent)
    {
        offset_x += ptr->client_rect.left;
        offset_y += ptr->client_rect.top;
    }
    offset_region( region, offset_x, offset_y );
    redraw_window( parent, region, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
    offset_region( region, -offset_x, -offset_y );
}


/* validate that the specified window and client rects are valid */
static int validate_window_rectangles( const rectangle_t *window_rect, const rectangle_t *client_rect )
{
    /* rectangles must be ordered properly */
    if (window_rect->right < window_rect->left) return 0;
    if (window_rect->bottom < window_rect->top) return 0;
    if (client_rect->right < client_rect->left) return 0;
    if (client_rect->bottom < client_rect->top) return 0;
    /* client rect must be inside window rect */
    if (client_rect->left < window_rect->left) return 0;
    if (client_rect->right > window_rect->right) return 0;
    if (client_rect->top < window_rect->top) return 0;
    if (client_rect->bottom > window_rect->bottom) return 0;
    return 1;
}


/* set the window and client rectangles, updating the update region if necessary */
static void set_window_pos( struct window *win, struct window *top, struct window *previous,
                            unsigned int swp_flags, unsigned int wvr_flags,
                            const rectangle_t *window_rect, const rectangle_t *client_rect )
{
    struct region *old_vis_rgn, *new_vis_rgn;
    const rectangle_t old_window_rect = win->window_rect;
    const rectangle_t old_client_rect = win->client_rect;

    /* if the window is not visible, everything is easy */

    if ((win->parent && !is_visible( win->parent )) ||
        (!(win->style & WS_VISIBLE) && !(swp_flags & SWP_SHOWWINDOW)))
    {
        win->window_rect = *window_rect;
        win->client_rect = *client_rect;
        if (!(swp_flags & SWP_NOZORDER)) link_window( win, win->parent, previous );
        if (swp_flags & SWP_SHOWWINDOW) win->style |= WS_VISIBLE;
        else if (swp_flags & SWP_HIDEWINDOW) win->style &= ~WS_VISIBLE;
        return;
    }

    if (!(old_vis_rgn = get_visible_region( win, top, DCX_WINDOW ))) return;

    /* set the new window info before invalidating anything */

    win->window_rect = *window_rect;
    win->client_rect = *client_rect;
    if (!(swp_flags & SWP_NOZORDER)) link_window( win, win->parent, previous );
    if (swp_flags & SWP_SHOWWINDOW) win->style |= WS_VISIBLE;
    else if (swp_flags & SWP_HIDEWINDOW) win->style &= ~WS_VISIBLE;

    if (!(new_vis_rgn = get_visible_region( win, top, DCX_WINDOW )))
    {
        free_region( old_vis_rgn );
        clear_error();  /* ignore error since the window info has been modified already */
        return;
    }

    /* expose anything revealed by the change */

    offset_region( old_vis_rgn, old_window_rect.left - window_rect->left,
                   old_window_rect.top - window_rect->top );
    if (xor_region( new_vis_rgn, old_vis_rgn, new_vis_rgn )) expose_window( win, top, new_vis_rgn );
    free_region( old_vis_rgn );

    if (!(win->style & WS_VISIBLE))
    {
        /* clear the update region since the window is no longer visible */
        validate_whole_window( win );
        goto done;
    }

    /* expose the whole non-client area if it changed in any way */

    if ((swp_flags & SWP_FRAMECHANGED) ||
        memcmp( window_rect, &old_window_rect, sizeof(old_window_rect) ) ||
        memcmp( client_rect, &old_client_rect, sizeof(old_client_rect) ))
    {
        struct region *tmp = create_region( client_rect, 1 );

        if (tmp)
        {
            set_region_rect( new_vis_rgn, window_rect );
            if (subtract_region( tmp, new_vis_rgn, tmp ))
            {
                offset_region( tmp, -window_rect->left, -window_rect->top );
                add_update_region( win, tmp );
            }
            else free_region( tmp );
        }
    }

    /* expose/validate new client areas + children */

    /* FIXME: expose everything for now */
    if (memcmp( client_rect, &old_client_rect, sizeof(old_client_rect) ))
        redraw_window( win, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );

done:
    free_region( new_vis_rgn );
    clear_error();  /* we ignore out of memory errors once the new rects have been set */
}


/* create a window */
DECL_HANDLER(create_window)
{
    struct window *win;

    reply->handle = 0;
    if (!req->parent)  /* return desktop window */
    {
        if (!top_window)
        {
            if (!(top_window = create_window( NULL, NULL, req->atom, req->instance ))) return;
            top_window->thread = NULL;  /* no thread owns the desktop */
            top_window->style  = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        }
        win = top_window;
    }
    else
    {
        struct window *parent, *owner = NULL;

        if (!(parent = get_window( req->parent ))) return;
        if (req->owner && !(owner = get_window( req->owner ))) return;
        if (owner == top_window) owner = NULL;
        else if (owner && parent != top_window)
        {
            /* an owned window must be created as top-level */
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
        if (!(win = create_window( parent, owner, req->atom, req->instance ))) return;
    }
    reply->handle    = win->handle;
    reply->extra     = win->nb_extra_bytes;
    reply->class_ptr = get_class_client_ptr( win->class );
}


/* link a window into the tree */
DECL_HANDLER(link_window)
{
    struct window *win, *parent = NULL, *previous = NULL;

    if (!(win = get_window( req->handle ))) return;
    if (req->parent && !(parent = get_window( req->parent ))) return;

    if (win == top_window)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    reply->full_parent = parent ? parent->handle : 0;
    if (parent && req->previous)
    {
        if (req->previous == (user_handle_t)1)  /* special case: HWND_BOTTOM */
        {
            previous = parent->last_child;
            if (previous == win) return;  /* nothing to do */
        }
        else
        {
            if (!(previous = get_window( req->previous ))) return;
            /* previous must be a child of parent, and not win itself */
            if (previous->parent != parent || previous == win)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
        }
    }
    link_window( win, parent, previous );
}


/* destroy a window */
DECL_HANDLER(destroy_window)
{
    struct window *win = get_window( req->handle );
    if (win)
    {
        if (win != top_window) destroy_window( win );
        else set_error( STATUS_ACCESS_DENIED );
    }
}


/* set a window owner */
DECL_HANDLER(set_window_owner)
{
    struct window *win = get_window( req->handle );
    struct window *owner = NULL;

    if (!win) return;
    if (req->owner && !(owner = get_window( req->owner ))) return;
    if (win == top_window)
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
        if (get_user_object( win->last_active, USER_WINDOW )) reply->last_active = win->last_active;
        if (win->thread)
        {
            reply->tid  = get_thread_id( win->thread );
            reply->pid  = get_process_id( win->thread->process );
            reply->atom = get_class_atom( win->class );
        }
    }
}


/* set some information in a window */
DECL_HANDLER(set_window_info)
{
    struct window *win = get_window( req->handle );

    if (!win) return;
    if (req->flags && win == top_window)
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
    size_t len;

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
    size_t len;

    if (parent)
        for (ptr = parent->first_child, total = 0; ptr; ptr = ptr->next)
        {
            if (req->atom && get_class_atom(ptr->class) != req->atom) continue;
            if (req->tid && get_thread_id(ptr->thread) != req->tid) continue;
            total++;
        }

    reply->count = total;
    len = min( get_reply_max_size(), total * sizeof(user_handle_t) );
    if (len && ((data = set_reply_data_size( len ))))
    {
        for (ptr = parent->first_child; ptr && len; ptr = ptr->next)
        {
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
    size_t len;

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
    struct window *win = get_window( req->handle );

    if (!win) return;

    if (win->parent)
    {
        struct window *parent = win->parent;
        reply->parent        = parent->handle;
        reply->owner         = win->owner;
        reply->next_sibling  = win->next ? win->next->handle : 0;
        reply->prev_sibling  = win->prev ? win->prev->handle : 0;
        reply->first_sibling = parent->first_child ? parent->first_child->handle : 0;
        reply->last_sibling  = parent->last_child ? parent->last_child->handle : 0;
    }
    else
    {
        reply->parent        = 0;
        reply->owner         = 0;
        reply->next_sibling  = 0;
        reply->prev_sibling  = 0;
        reply->first_sibling = 0;
        reply->last_sibling  = 0;
    }
    reply->first_child = win->first_child ? win->first_child->handle : 0;
    reply->last_child  = win->last_child ? win->last_child->handle : 0;
}


/* set the position and Z order of a window */
DECL_HANDLER(set_window_pos)
{
    struct window *previous = NULL;
    struct window *top = top_window;
    struct window *win = get_window( req->handle );
    unsigned int flags = req->flags;

    if (!win) return;
    if (!win->parent) flags |= SWP_NOZORDER;  /* no Z order for the desktop */

    if (req->top_win)
    {
        if (!(top = get_window( req->top_win ))) return;
    }

    if (!(flags & SWP_NOZORDER))
    {
        if (!req->previous)  /* special case: HWND_TOP */
        {
            if (win->parent->first_child == win) flags |= SWP_NOZORDER;
        }
        else if (req->previous == (user_handle_t)1)  /* special case: HWND_BOTTOM */
        {
            previous = win->parent->last_child;
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

    if (!validate_window_rectangles( &req->window, &req->client ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    set_window_pos( win, top, previous, flags, req->redraw_flags, &req->window, &req->client );
    reply->new_style = win->style;
}


/* get the window and client rectangles of a window */
DECL_HANDLER(get_window_rectangles)
{
    struct window *win = get_window( req->handle );

    if (win)
    {
        reply->window = win->window_rect;
        reply->client = win->client_rect;
    }
}


/* get the window text */
DECL_HANDLER(get_window_text)
{
    struct window *win = get_window( req->handle );

    if (win && win->text)
    {
        size_t len = strlenW( win->text ) * sizeof(WCHAR);
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
        size_t len = get_req_data_size() / sizeof(WCHAR);
        if (len)
        {
            if (!(text = mem_alloc( (len+1) * sizeof(WCHAR) ))) return;
            memcpy( text, get_req_data(), len * sizeof(WCHAR) );
            text[len] = 0;
        }
        if (win->text) free( win->text );
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
        while (win)
        {
            reply->x += win->client_rect.left;
            reply->y += win->client_rect.top;
            win = win->parent;
        }
    }
    if (req->to)
    {
        if (!(win = get_window( req->to ))) return;
        while (win)
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
    struct window *win = get_window( req->window );
    struct window *top = NULL;

    if (!win) return;
    if (req->top_win && !(top = get_window( req->top_win ))) return;

    if ((region = get_visible_region( win, top, req->flags )))
    {
        rectangle_t *data = get_region_data_and_free( region, get_reply_max_size(), &reply->total_size );
        if (data) set_reply_data_ptr( data, reply->total_size );
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
    if (win->win_region) free_region( win->win_region );
    win->win_region = region;
}


/* get a window update region */
DECL_HANDLER(get_update_region)
{
    rectangle_t *data;
    unsigned int flags = req->flags;
    struct window *win = get_window( req->window );

    reply->flags = 0;
    if (!win || !is_visible( win )) return;

    if ((flags & UPDATE_NONCLIENT) && !(flags & (UPDATE_PAINT|UPDATE_INTERNALPAINT)))
    {
        /* non-client painting must be delayed if one of the parents is going to
         * be repainted and doesn't clip children */
        struct window *ptr;

        for (ptr = win->parent; ptr && ptr != top_window; ptr = ptr->parent)
        {
            if (!(ptr->style & WS_CLIPCHILDREN) && win_needs_repaint( ptr ))
                return;
        }
    }

    if (!(reply->flags = get_update_flags( win, flags )))
    {
        /* if window doesn't need any repaint, check the children */
        if (!(flags & UPDATE_NOCHILDREN) &&
            ((flags & UPDATE_ALLCHILDREN) || (win->style & WS_CLIPCHILDREN)))
        {
            reply->flags = get_child_update_flags( win, flags, &win );
        }
    }

    reply->child = win->handle;

    if (flags & UPDATE_NOREGION) return;

    if (win->update_region)
    {
        if (!(data = get_region_data( win->update_region, get_reply_max_size(),
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
        if (reply->flags & UPDATE_ERASE) win->paint_flags &= ~PAINT_ERASE;
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

    if (win) set_property( win, req->atom, req->handle,
                           req->string ? PROP_TYPE_STRING : PROP_TYPE_ATOM );
}


/* remove a window property */
DECL_HANDLER(remove_window_property)
{
    struct window *win = get_window( req->window );
    reply->handle = 0;
    if (win) reply->handle = remove_property( win, req->atom );
}


/* get a window property */
DECL_HANDLER(get_window_property)
{
    struct window *win = get_window( req->window );
    reply->handle = 0;
    if (win) reply->handle = get_property( win, req->atom );
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

/*
 * Server-side window handling
 *
 * Copyright (C) 2001 Alexandre Julliard
 */

#include <assert.h>

#include "object.h"
#include "request.h"
#include "thread.h"
#include "process.h"
#include "user.h"

struct window
{
    struct window   *parent;      /* parent window */
    struct window   *owner;       /* owner of this window */
    struct window   *first_child; /* first child window */
    struct window   *last_child;  /* last child window */
    struct window   *next;        /* next window in Z-order */
    struct window   *prev;        /* prev window in Z-order */
    user_handle_t    handle;      /* full handle for this window */
    struct thread   *thread;      /* thread owning the window */
};

static struct window *top_window;  /* top-level (desktop) window */


/* retrieve a pointer to a window from its handle */
inline static struct window *get_window( user_handle_t handle )
{
    struct window *ret = get_user_object( handle, USER_WINDOW );
    if (!ret) set_error( STATUS_INVALID_HANDLE );
    return ret;
}

/* link a window into the tree (or unlink it if the new parent is NULL)  */
static void link_window( struct window *win, struct window *parent, struct window *previous )
{
    if (win->parent)  /* unlink it from the previous location */
    {
        if (win->next) win->next->prev = win->prev;
        else if (win->parent->last_child == win) win->parent->last_child = win->prev;
        if (win->prev) win->prev->next = win->next;
        else if (win->parent->first_child == win) win->parent->first_child = win->next;
    }
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
    else
    {
        /* don't touch parent; an unlinked window still has a valid parent pointer */
        win->next = win->prev = NULL;
    }
}

/* destroy a window */
static void destroy_window( struct window *win )
{
    assert( win != top_window );

    /* destroy all children */
    while (win->first_child) destroy_window( win->first_child );

    /* reset siblings owner */
    if (win->parent)
    {
        struct window *ptr;
        for (ptr = win->parent->first_child; ptr; ptr = ptr->next)
            if (ptr->owner == win) ptr->owner = NULL;
    }

    if (win->thread->queue) queue_cleanup_window( win->thread, win->handle );
    free_user_handle( win->handle );
    link_window( win, NULL, NULL );
    memset( win, 0x55, sizeof(*win) );
    free( win );
}

/* create a new window structure (note: the window is not linked in the window tree) */
static struct window *create_window( struct window *parent, struct window *owner )
{
    struct window *win = mem_alloc( sizeof(*win) );
    if (!win) return NULL;

    if (!(win->handle = alloc_user_handle( win, USER_WINDOW )))
    {
        free( win );
        return NULL;
    }
    win->parent      = parent;
    win->owner       = owner;
    win->first_child = NULL;
    win->last_child  = NULL;
    win->next        = NULL;
    win->prev        = NULL;
    win->thread      = current;
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

/* create a window */
DECL_HANDLER(create_window)
{
    req->handle = 0;
    if (!req->parent)  /* return desktop window */
    {
        if (!top_window)
        {
            if (!(top_window = create_window( NULL, NULL ))) return;
            top_window->thread = NULL;  /* no thread owns the desktop */
        }
        req->handle = top_window->handle;
    }
    else
    {
        struct window *win, *parent, *owner = NULL;

        if (!(parent = get_window( req->parent ))) return;
        if (req->owner && !(owner = get_window( req->owner ))) return;
        if (!(win = create_window( parent, owner ))) return;
        req->handle = win->handle;
    }
}


/* link a window into the tree */
DECL_HANDLER(link_window)
{
    struct window *win, *parent = NULL, *previous = NULL;

    if (!(win = get_window( req->handle ))) return;
    if (req->parent && !(parent = get_window( req->parent ))) return;
    if (parent && req->previous)
    {
        if (req->previous == 1)  /* special case: HWND_BOTTOM */
            previous = parent->last_child;
        else
            if (!(previous = get_window( req->previous ))) return;

        if (previous && previous->parent != parent)  /* previous must be a child of parent */
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
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


/* get information from a window handle */
DECL_HANDLER(get_window_info)
{
    struct window *win = get_window( req->handle );

    req->full_handle = 0;
    req->tid = req->pid = 0;
    if (win)
    {
        req->full_handle = win->handle;
        if (win->thread)
        {
            req->tid = get_thread_id( win->thread );
            req->pid = get_process_id( win->thread->process );
        }
    }
}


/* get a list of the window parents, up to the root of the tree */
DECL_HANDLER(get_window_parents)
{
    struct window *ptr, *win = get_window( req->handle );
    int total = 0;
    size_t len;

    if (win) for (ptr = win->parent; ptr; ptr = ptr->parent) total++;

    req->count = total;
    len = min( get_req_data_size(req), total * sizeof(user_handle_t) );
    set_req_data_size( req, len );
    if (len)
    {
        user_handle_t *data = get_req_data(req);
        for (ptr = win->parent; ptr && len; ptr = ptr->parent, len -= sizeof(*data))
            *data++ = ptr->handle;
    }
}


/* get a list of the window children */
DECL_HANDLER(get_window_children)
{
    struct window *ptr, *parent = get_window( req->parent );
    int total = 0;
    size_t len;

    if (parent)
        for (ptr = parent->first_child, total = 0; ptr; ptr = ptr->next) total++;

    req->count = total;
    len = min( get_req_data_size(req), total * sizeof(user_handle_t) );
    set_req_data_size( req, len );
    if (len)
    {
        user_handle_t *data = get_req_data(req);
        for (ptr = parent->first_child; ptr && len; ptr = ptr->next, len -= sizeof(*data))
            *data++ = ptr->handle;
    }
}


/* get window tree information from a window handle */
DECL_HANDLER(get_window_tree)
{
    struct window *win = get_window( req->handle );

    if (!win) return;

    if (win->parent)
    {
        struct window *parent = win->parent;
        req->parent        = parent->handle;
        req->owner         = win->owner ? win->owner->handle : 0;
        req->next_sibling  = win->next ? win->next->handle : 0;
        req->prev_sibling  = win->prev ? win->prev->handle : 0;
        req->first_sibling = parent->first_child ? parent->first_child->handle : 0;
        req->last_sibling  = parent->last_child ? parent->last_child->handle : 0;
    }
    else
    {
        req->parent        = 0;
        req->owner         = 0;
        req->next_sibling  = 0;
        req->prev_sibling  = 0;
        req->first_sibling = 0;
        req->last_sibling  = 0;
    }
    req->first_child = win->first_child ? win->first_child->handle : 0;
    req->last_child  = win->last_child ? win->last_child->handle : 0;
}

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
    struct window   *parent;    /* parent window */
    struct window   *child;     /* first child window */
    struct window   *next;      /* next window in Z-order */
    struct window   *prev;      /* prev window in Z-order */
    user_handle_t    handle;    /* full handle for this window */
    struct thread   *thread;    /* thread owning the window */
};

static struct window *top_window;  /* top-level (desktop) window */

/* link a window into the tree (or unlink it if the new parent is NULL)  */
static void link_window( struct window *win, struct window *parent, struct window *previous )
{
    if (win->parent)  /* unlink it from the previous location */
    {
        if (win->next) win->next->prev = win->prev;
        if (win->prev) win->prev->next = win->next;
        else win->parent->child = win->next;
    }
    if ((win->parent = parent))
    {
        if ((win->prev = previous))
        {
            if ((win->next = previous->next)) win->next->prev = win;
            win->prev->next = win;
        }
        else
        {
            if ((win->next = parent->child)) win->next->prev = win;
            parent->child = win;
        }
    }
    else win->next = win->prev = NULL;

}

/* free a window structure */
static void free_window( struct window *win )
{
    assert( win != top_window );

    while (win->child) free_window( win->child );
    if (win->thread->queue) queue_cleanup_window( win->thread, win->handle );
    free_user_handle( win->handle );
    if (win->parent) link_window( win, NULL, NULL );
    memset( win, 0x55, sizeof(*win) );
    free( win );
}

/* create a new window structure */
static struct window *create_window(void)
{
    struct window *win = mem_alloc( sizeof(*win) );
    if (!win) return NULL;

    if (!(win->handle = alloc_user_handle( win, USER_WINDOW )))
    {
        free( win );
        return NULL;
    }
    win->parent = NULL;
    win->child  = NULL;
    win->next   = NULL;
    win->prev   = NULL;
    win->thread = current;
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
        free_window( win );
    }
}

/* create a window */
DECL_HANDLER(create_window)
{
    struct window *win;

    req->handle = 0;
    if ((win = create_window())) req->handle = win->handle;
}


/* create the desktop window */
DECL_HANDLER(create_desktop_window)
{
    req->handle = 0;
    if (!top_window)
    {
        if (!(top_window = create_window())) return;
        top_window->thread = NULL;  /* no thread owns the desktop */
    }
    req->handle = top_window->handle;
}


/* link a window into the tree */
DECL_HANDLER(link_window)
{
    struct window *win, *parent = NULL, *previous = NULL;

    if (!(win = get_user_object( req->handle, USER_WINDOW ))) return;
    if (req->parent && !(parent = get_user_object( req->parent, USER_WINDOW ))) return;
    if (parent && req->previous)
    {
        if (req->previous == 1)  /* special case: HWND_BOTTOM */
        {
            if ((previous = parent->child))
            {
                /* make it the last of the list */
                while (previous->next) previous = previous->next;
            }
        }
        else if (!(previous = get_user_object( req->previous, USER_WINDOW ))) return;

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
    struct window *win = get_user_object( req->handle, USER_WINDOW );
    if (win)
    {
        if (win != top_window) free_window( win );
        else set_error( STATUS_ACCESS_DENIED );
    }
}


/* Get information from a window handle */
DECL_HANDLER(get_window_info)
{
    user_handle_t handle = req->handle;
    struct window *win = get_user_object_handle( &handle, USER_WINDOW );

    req->full_handle = 0;
    req->tid = req->pid = 0;
    if (win)
    {
        req->full_handle = handle;
        if (win->thread)
        {
            req->tid = get_thread_id( win->thread );
            req->pid = get_process_id( win->thread->process );
        }
    }
}

/*
 * Wine server USER definitions
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

#ifndef __WINE_SERVER_USER_H
#define __WINE_SERVER_USER_H

#include "wine/server_protocol.h"

struct thread;
struct region;
struct window;
struct msg_queue;
struct hook_table;
struct window_class;
struct atom_table;
struct clipboard;

enum user_object
{
    USER_WINDOW = 1,
    USER_HOOK,
    USER_CLIENT  /* arbitrary client handle */
};

#define DESKTOP_ATOM  ((atom_t)32769)

struct winstation
{
    struct object      obj;                /* object header */
    unsigned int       flags;              /* winstation flags */
    struct list        entry;              /* entry in global winstation list */
    struct list        desktops;           /* list of desktops of this winstation */
    struct clipboard  *clipboard;          /* clipboard information */
    struct atom_table *atom_table;         /* global atom table */
    struct namespace  *desktop_names;      /* namespace for desktops of this winstation */
};

struct global_cursor
{
    int                  x;                /* cursor position */
    int                  y;
    rectangle_t          clip;             /* cursor clip rectangle */
    unsigned int         clip_msg;         /* message to post for cursor clip changes */
    unsigned int         last_change;      /* time of last position change */
    user_handle_t        win;              /* window that contains the cursor */
};

struct desktop
{
    struct object        obj;              /* object header */
    unsigned int         flags;            /* desktop flags */
    struct winstation   *winstation;       /* winstation this desktop belongs to */
    struct list          entry;            /* entry in winstation list of desktops */
    struct window       *top_window;       /* desktop window for this desktop */
    struct window       *msg_window;       /* HWND_MESSAGE top window */
    struct hook_table   *global_hooks;     /* table of global hooks on this desktop */
    struct list          hotkeys;          /* list of registered hotkeys */
    struct timeout_user *close_timeout;    /* timeout before closing the desktop */
    struct thread_input *foreground_input; /* thread input of foreground thread */
    unsigned int         users;            /* processes and threads using this desktop */
    struct global_cursor cursor;           /* global cursor information */
    unsigned char        keystate[256];    /* asynchronous key state */
};

/* user handles functions */

extern user_handle_t alloc_user_handle( void *ptr, enum user_object type );
extern void *get_user_object( user_handle_t handle, enum user_object type );
extern void *get_user_object_handle( user_handle_t *handle, enum user_object type );
extern user_handle_t get_user_full_handle( user_handle_t handle );
extern void *free_user_handle( user_handle_t handle );
extern void *next_user_handle( user_handle_t *handle, enum user_object type );
extern void free_process_user_handles( struct process *process );

/* clipboard functions */

extern void cleanup_clipboard_window( struct desktop *desktop, user_handle_t window );
extern void cleanup_clipboard_thread( struct thread *thread );

/* hook functions */

extern void remove_thread_hooks( struct thread *thread );
extern unsigned int get_active_hooks(void);
extern struct thread *get_first_global_hook( int id );

/* queue functions */

extern void free_msg_queue( struct thread *thread );
extern struct hook_table *get_queue_hooks( struct thread *thread );
extern void set_queue_hooks( struct thread *thread, struct hook_table *hooks );
extern void inc_queue_paint_count( struct thread *thread, int incr );
extern void queue_cleanup_window( struct thread *thread, user_handle_t win );
extern int init_thread_queue( struct thread *thread );
extern int attach_thread_input( struct thread *thread_from, struct thread *thread_to );
extern void detach_thread_input( struct thread *thread_from );
extern void post_message( user_handle_t win, unsigned int message,
                          lparam_t wparam, lparam_t lparam );
extern void send_notify_message( user_handle_t win, unsigned int message,
                                 lparam_t wparam, lparam_t lparam );
extern void post_win_event( struct thread *thread, unsigned int event,
                            user_handle_t win, unsigned int object_id,
                            unsigned int child_id, client_ptr_t proc,
                            const WCHAR *module, data_size_t module_size,
                            user_handle_t handle );
extern void free_hotkeys( struct desktop *desktop, user_handle_t window );

/* region functions */

extern struct region *create_empty_region(void);
extern struct region *create_region_from_req_data( const void *data, data_size_t size );
extern void free_region( struct region *region );
extern void set_region_rect( struct region *region, const rectangle_t *rect );
extern rectangle_t *get_region_data( const struct region *region, data_size_t max_size,
                                     data_size_t *total_size );
extern rectangle_t *get_region_data_and_free( struct region *region, data_size_t max_size,
                                              data_size_t *total_size );
extern int is_region_empty( const struct region *region );
extern void get_region_extents( const struct region *region, rectangle_t *rect );
extern void offset_region( struct region *region, int x, int y );
extern void mirror_region( const rectangle_t *client_rect, struct region *region );
extern void scale_region( struct region *region, unsigned int dpi_from, unsigned int dpi_to );
extern struct region *copy_region( struct region *dst, const struct region *src );
extern struct region *intersect_region( struct region *dst, const struct region *src1,
                                        const struct region *src2 );
extern struct region *subtract_region( struct region *dst, const struct region *src1,
                                       const struct region *src2 );
extern struct region *union_region( struct region *dst, const struct region *src1,
                                    const struct region *src2 );
extern struct region *xor_region( struct region *dst, const struct region *src1,
                                  const struct region *src2 );
extern int point_in_region( struct region *region, int x, int y );
extern int rect_in_region( struct region *region, const rectangle_t *rect );

/* window functions */

extern struct process *get_top_window_owner( struct desktop *desktop );
extern void get_top_window_rectangle( struct desktop *desktop, rectangle_t *rect );
extern void post_desktop_message( struct desktop *desktop, unsigned int message,
                                  lparam_t wparam, lparam_t lparam );
extern void free_window_handle( struct window *win );
extern void destroy_thread_windows( struct thread *thread );
extern int is_child_window( user_handle_t parent, user_handle_t child );
extern int is_valid_foreground_window( user_handle_t window );
extern int is_window_visible( user_handle_t window );
extern int is_window_transparent( user_handle_t window );
extern int make_window_active( user_handle_t window );
extern struct thread *get_window_thread( user_handle_t handle );
extern user_handle_t shallow_window_from_point( struct desktop *desktop, int x, int y );
extern struct thread *window_thread_from_point( user_handle_t scope, int x, int y );
extern user_handle_t find_window_to_repaint( user_handle_t parent, struct thread *thread );
extern struct window_class *get_window_class( user_handle_t window );

/* window class functions */

extern void destroy_process_classes( struct process *process );
extern struct window_class *grab_class( struct process *process, atom_t atom,
                                        mod_handle_t instance, int *extra_bytes );
extern void release_class( struct window_class *class );
extern int is_desktop_class( struct window_class *class );
extern int is_hwnd_message_class( struct window_class *class );
extern atom_t get_class_atom( struct window_class *class );
extern client_ptr_t get_class_client_ptr( struct window_class *class );

/* windows station functions */

extern struct desktop *get_desktop_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern struct winstation *get_process_winstation( struct process *process, unsigned int access );
extern struct desktop *get_thread_desktop( struct thread *thread, unsigned int access );
extern void connect_process_winstation( struct process *process, struct thread *parent_thread,
                                        struct process *parent_process );
extern void set_process_default_desktop( struct process *process, struct desktop *desktop,
                                         obj_handle_t handle );
extern void close_process_desktop( struct process *process );
extern void set_thread_default_desktop( struct thread *thread, struct desktop *desktop, obj_handle_t handle );
extern void release_thread_desktop( struct thread *thread, int close );

static inline int is_rect_empty( const rectangle_t *rect )
{
    return (rect->left >= rect->right || rect->top >= rect->bottom);
}

static inline int point_in_rect( const rectangle_t *rect, int x, int y )
{
    return (x >= rect->left && x < rect->right && y >= rect->top && y < rect->bottom);
}

static inline int scale_dpi( int val, unsigned int dpi_from, unsigned int dpi_to )
{
    if (val >= 0) return (val * dpi_to + (dpi_from / 2)) / dpi_from;
    return (val * dpi_to - (dpi_from / 2)) / dpi_from;
}

static inline void scale_dpi_rect( rectangle_t *rect, unsigned int dpi_from, unsigned int dpi_to )
{
    rect->left   = scale_dpi( rect->left, dpi_from, dpi_to );
    rect->top    = scale_dpi( rect->top, dpi_from, dpi_to );
    rect->right  = scale_dpi( rect->right, dpi_from, dpi_to );
    rect->bottom = scale_dpi( rect->bottom, dpi_from, dpi_to );
}

/* offset the coordinates of a rectangle */
static inline void offset_rect( rectangle_t *rect, int offset_x, int offset_y )
{
    rect->left   += offset_x;
    rect->top    += offset_y;
    rect->right  += offset_x;
    rect->bottom += offset_y;
}

/* mirror a rectangle respective to the window client area */
static inline void mirror_rect( const rectangle_t *client_rect, rectangle_t *rect )
{
    int width = client_rect->right - client_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* compute the intersection of two rectangles; return 0 if the result is empty */
static inline int intersect_rect( rectangle_t *dst, const rectangle_t *src1, const rectangle_t *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return !is_rect_empty( dst );
}

/* validate a window handle and return the full handle */
static inline user_handle_t get_valid_window_handle( user_handle_t win )
{
    if (get_user_object_handle( &win, USER_WINDOW )) return win;
    set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
    return 0;
}

#endif  /* __WINE_SERVER_USER_H */

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

#include <limits.h>
#include "wine/server_protocol.h"
#include "unicode.h"

struct thread;
struct region;
struct window;
struct msg_queue;
struct hook_table;
struct window_class;
struct atom_table;
struct clipboard;

#define DESKTOP_ATOM  ((atom_t)32769)

struct winstation
{
    struct object      obj;                /* object header */
    unsigned int       flags;              /* winstation flags */
    struct list        entry;              /* entry in global winstation list */
    struct list        desktops;           /* list of desktops of this winstation */
    struct desktop    *input_desktop;      /* desktop receiving user input */
    struct clipboard  *clipboard;          /* clipboard information */
    struct atom_table *atom_table;         /* global atom table */
    struct namespace  *desktop_names;      /* namespace for desktops of this winstation */
    unsigned int       monitor_count;      /* number of monitors */
    struct monitor_info *monitors;         /* window station monitors */
    unsigned __int64   monitor_serial;     /* winstation monitor update counter */
};

struct key_repeat
{
    int                  enable;           /* enable auto-repeat */
    timeout_t            delay;            /* auto-repeat delay */
    timeout_t            period;           /* auto-repeat period */
    union hw_input       input;            /* the input to repeat */
    user_handle_t        win;              /* target window for input event */
    struct timeout_user *timeout;          /* timeout for repeat */
};

struct desktop
{
    struct object        obj;              /* object header */
    struct winstation   *winstation;       /* winstation this desktop belongs to */
    timeout_t            input_time;       /* last time this desktop had the input */
    struct list          entry;            /* entry in winstation list of desktops */
    struct list          threads;          /* list of threads connected to this desktop */
    struct window       *top_window;       /* desktop window for this desktop */
    struct window       *msg_window;       /* HWND_MESSAGE top window */
    struct window       *shell_window;     /* shell window for this desktop */
    struct window       *shell_listview;   /* shell list view window for this desktop */
    struct window       *progman_window;   /* progman window for this desktop */
    struct window       *taskman_window;   /* taskman window for this desktop */
    struct hook_table   *global_hooks;     /* table of global hooks on this desktop */
    struct list          hotkeys;          /* list of registered hotkeys */
    struct list          pointers;         /* list of active pointers */
    struct timeout_user *close_timeout;    /* timeout before closing the desktop */
    struct thread_input *foreground_input; /* thread input of foreground thread */
    unsigned int         users;            /* processes and threads using this desktop */
    unsigned char        keystate[256];    /* asynchronous key state */
    unsigned char        alt_pressed;      /* last key press was Alt (used to determine msg on release) */
    struct key_repeat    key_repeat;       /* key auto-repeat */
    unsigned int         clip_flags;       /* last cursor clip flags */
    user_handle_t        cursor_win;       /* window that contains the cursor */
    desktop_shm_t       *shared;           /* desktop session shared memory */
};

/* user handles functions */

extern user_handle_t alloc_user_handle( void *ptr, volatile void *shared, unsigned short type );
extern void *get_user_object( user_handle_t handle, unsigned short type );
extern void *get_user_object_handle( user_handle_t *handle, unsigned short type );
extern user_handle_t get_user_full_handle( user_handle_t handle );
extern void *free_user_handle( user_handle_t handle );
extern void *next_user_handle( user_handle_t *handle, unsigned short type );
extern void free_process_user_handles( struct process *process );

/* clipboard functions */

extern void cleanup_clipboard_window( struct desktop *desktop, user_handle_t window );
extern void cleanup_clipboard_thread( struct thread *thread );

/* hook functions */

extern void remove_thread_hooks( struct thread *thread );
extern unsigned int get_active_hooks(void);
extern struct thread *get_first_global_hook( struct desktop *desktop, int id );
extern void add_desktop_hook_count( struct desktop *desktop, struct thread *thread, int count );

/* queue functions */

extern void free_msg_queue( struct thread *thread );
extern struct hook_table *get_queue_hooks( struct thread *thread );
extern void set_queue_hooks( struct thread *thread, struct hook_table *hooks );
extern void add_queue_hook_count( struct thread *thread, unsigned int index, int count );
extern void inc_queue_paint_count( struct thread *thread, int incr );
extern void queue_cleanup_window( struct thread *thread, user_handle_t win );
extern int init_thread_queue( struct thread *thread );
extern void check_thread_queue_idle( struct thread *thread );
extern int attach_thread_input( struct thread *thread_from, struct thread *thread_to );
extern void detach_thread_input( struct thread *thread_from );
extern void set_clip_rectangle( struct desktop *desktop, const struct rectangle *rect,
                                unsigned int flags, int reset );
extern void update_cursor_pos( struct desktop *desktop );
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
extern void free_pointers( struct desktop *desktop );
extern void set_rawinput_process( struct process *process, int enable );

/* region functions */

extern struct region *create_empty_region(void);
extern struct region *create_region_from_req_data( const void *data, data_size_t size );
extern void free_region( struct region *region );
extern void set_region_rect( struct region *region, const struct rectangle *rect );
extern struct rectangle *get_region_data( const struct region *region, data_size_t max_size,
                                          data_size_t *total_size );
extern struct rectangle *get_region_data_and_free( struct region *region, data_size_t max_size,
                                                   data_size_t *total_size );
extern int is_region_empty( const struct region *region );
extern int is_region_equal( const struct region *region1, const struct region *region2 );
extern void get_region_extents( const struct region *region, struct rectangle *rect );
extern void offset_region( struct region *region, int x, int y );
extern void mirror_region( const struct rectangle *client_rect, struct region *region );
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
extern int rect_in_region( struct region *region, const struct rectangle *rect );

/* window functions */

extern struct process *get_top_window_owner( struct desktop *desktop );
extern void get_virtual_screen_rect( struct desktop *desktop, struct rectangle *rect, int is_raw );
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
extern struct window_class *grab_class( struct process *process, atom_t atom, mod_handle_t instance,
                                        int *extra_bytes, struct obj_locator *locator );
extern void release_class( struct window_class *class );
extern int is_desktop_class( struct window_class *class );
extern int is_message_class( struct window_class *class );
extern int get_class_style( struct window_class *class );
extern atom_t get_class_atom( struct window_class *class );
extern client_ptr_t get_class_client_ptr( struct window_class *class );

/* windows station functions */

extern struct winstation *get_visible_winstation(void);
extern struct desktop *get_input_desktop( struct winstation *winstation );
extern int set_input_desktop( struct winstation *winstation, struct desktop *new_desktop );
extern struct desktop *get_desktop_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern struct winstation *get_process_winstation( struct process *process, unsigned int access );
extern struct desktop *get_thread_desktop( struct thread *thread, unsigned int access );
extern void connect_process_winstation( struct process *process, struct unicode_str *desktop_path,
                                        struct thread *parent_thread, struct process *parent_process );
extern void set_process_default_desktop( struct process *process, struct desktop *desktop,
                                         obj_handle_t handle );
extern void close_process_desktop( struct process *process );
extern void set_thread_default_desktop( struct thread *thread, struct desktop *desktop, obj_handle_t handle );
extern void release_thread_desktop( struct thread *thread, int close );

/* checks if two rectangles are identical */
static inline int is_rect_equal( const struct rectangle *rect1, const struct rectangle *rect2 )
{
    return (rect1->left == rect2->left && rect1->right == rect2->right &&
            rect1->top == rect2->top && rect1->bottom == rect2->bottom);
}

static inline int is_rect_empty( const struct rectangle *rect )
{
    return (rect->left >= rect->right || rect->top >= rect->bottom);
}

static inline int point_in_rect( const struct rectangle *rect, int x, int y )
{
    return (x >= rect->left && x < rect->right && y >= rect->top && y < rect->bottom);
}

static inline int scale_dpi( int val, unsigned int dpi_from, unsigned int dpi_to )
{
    if (val >= 0) return (val * dpi_to + (dpi_from / 2)) / dpi_from;
    return (val * dpi_to - (dpi_from / 2)) / dpi_from;
}

static inline void scale_dpi_rect( struct rectangle *rect, unsigned int dpi_from, unsigned int dpi_to )
{
    rect->left   = scale_dpi( rect->left, dpi_from, dpi_to );
    rect->top    = scale_dpi( rect->top, dpi_from, dpi_to );
    rect->right  = scale_dpi( rect->right, dpi_from, dpi_to );
    rect->bottom = scale_dpi( rect->bottom, dpi_from, dpi_to );
}

/* offset the coordinates of a rectangle */
static inline void offset_rect( struct rectangle *rect, int offset_x, int offset_y )
{
    rect->left   += offset_x;
    rect->top    += offset_y;
    rect->right  += offset_x;
    rect->bottom += offset_y;
}

/* mirror a rectangle respective to the window client area */
static inline void mirror_rect( const struct rectangle *client_rect, struct rectangle *rect )
{
    int width = client_rect->right - client_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* compute the intersection of two rectangles; return 0 if the result is empty */
static inline int intersect_rect( struct rectangle *dst, const struct rectangle *src1, const struct rectangle *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return !is_rect_empty( dst );
}

static inline void reset_bounds( struct rectangle *bounds )
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static inline void union_rect( struct rectangle *dest, const struct rectangle *src1, const struct rectangle *src2 )
{
    if (is_rect_empty( src1 ))
    {
        if (is_rect_empty( src2 ))
        {
            reset_bounds( dest );
            return;
        }
        else *dest = *src2;
    }
    else
    {
        if (is_rect_empty( src2 )) *dest = *src1;
        else
        {
            dest->left   = min( src1->left, src2->left );
            dest->right  = max( src1->right, src2->right );
            dest->top    = min( src1->top, src2->top );
            dest->bottom = max( src1->bottom, src2->bottom );
        }
    }
}

/* validate a window handle and return the full handle */
static inline user_handle_t get_valid_window_handle( user_handle_t win )
{
    if (get_user_object_handle( &win, NTUSER_OBJ_WINDOW )) return win;
    set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
    return 0;
}

#endif  /* __WINE_SERVER_USER_H */

/*
 * Wine server USER definitions
 *
 * Copyright (C) 2001 Alexandre Julliard
 */

#ifndef __WINE_SERVER_USER_H
#define __WINE_SERVER_USER_H

#include "wine/server_protocol.h"

struct thread;
struct window;
struct msg_queue;

enum user_object
{
    USER_WINDOW = 1
};

/* user handles functions */

extern user_handle_t alloc_user_handle( void *ptr, enum user_object type );
extern void *get_user_object( user_handle_t handle, enum user_object type );
extern void *get_user_object_handle( user_handle_t *handle, enum user_object type );
extern user_handle_t get_user_full_handle( user_handle_t handle );
extern void *free_user_handle( user_handle_t handle );
extern void *next_user_handle( user_handle_t *handle, enum user_object type );

/* queue functions */

extern void inc_queue_paint_count( struct thread *thread, int incr );
extern void queue_cleanup_window( struct thread *thread, user_handle_t win );

/* window functions */

extern void destroy_thread_windows( struct thread *thread );
extern int is_child_window( user_handle_t parent, user_handle_t child );
extern user_handle_t find_window_to_repaint( user_handle_t parent, struct thread *thread );

#endif  /* __WINE_SERVER_USER_H */

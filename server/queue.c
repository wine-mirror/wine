/*
 * Server-side message queues
 *
 * Copyright (C) 2000 Alexandre Julliard
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "ntuser.h"
#include "hidusage.h"
#include "kbd.h"

#include "handle.h"
#include "file.h"
#include "thread.h"
#include "process.h"
#include "request.h"
#include "user.h"

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

enum message_kind { SEND_MESSAGE, POST_MESSAGE };
#define NB_MSG_KINDS (POST_MESSAGE+1)

/* list of processes registered for rawinput in the input desktop */
static struct list rawinput_processes = LIST_INIT(rawinput_processes);

struct message_result
{
    struct list            sender_entry;  /* entry in sender list */
    struct message        *msg;           /* message the result is for */
    struct message_result *recv_next;     /* next in receiver list */
    struct msg_queue      *sender;        /* sender queue */
    struct msg_queue      *receiver;      /* receiver queue */
    int                    replied;       /* has it been replied to? */
    unsigned int           error;         /* error code to pass back to sender */
    lparam_t               result;        /* reply result */
    struct message        *hardware_msg;  /* hardware message if low-level hook result */
    struct desktop        *desktop;       /* desktop for hardware message */
    struct message        *callback_msg;  /* message to queue for callback */
    void                  *data;          /* message reply data */
    unsigned int           data_size;     /* size of message reply data */
    struct timeout_user   *timeout;       /* result timeout */
};

struct message
{
    struct list            entry;     /* entry in message list */
    enum message_type      type;      /* message type */
    user_handle_t          win;       /* window handle */
    unsigned int           msg;       /* message code */
    lparam_t               wparam;    /* parameters */
    lparam_t               lparam;    /* parameters */
    int                    x;         /* message position */
    int                    y;
    unsigned int           time;      /* message time */
    void                  *data;      /* message data for sent messages */
    unsigned int           data_size; /* size of message data */
    unsigned int           unique_id; /* unique id for nested hw message waits */
    struct message_result *result;    /* result in sender queue */
};

struct timer
{
    struct list     entry;     /* entry in timer list */
    abstime_t       when;      /* next expiration */
    unsigned int    rate;      /* timer rate in ms */
    user_handle_t   win;       /* window handle */
    unsigned int    msg;       /* message to post */
    lparam_t        id;        /* timer id */
    lparam_t        lparam;    /* lparam for message */
};

struct thread_input
{
    struct object          obj;           /* object header */
    struct desktop        *desktop;       /* desktop that this thread input belongs to */
    int                    caret_hide;    /* caret hide count */
    int                    caret_state;   /* caret on/off state */
    struct list            msg_list;      /* list of hardware messages */
    unsigned char          desktop_keystate[256]; /* desktop keystate when keystate was synced */
    const input_shm_t     *shared;        /* thread input in session shared memory */
};

struct msg_queue
{
    struct object          obj;             /* object header */
    struct fd             *fd;              /* optional file descriptor to poll */
    int                    paint_count;     /* pending paint messages count */
    int                    hotkey_count;    /* pending hotkey messages count */
    int                    quit_message;    /* is there a pending quit message? */
    int                    exit_code;       /* exit code of pending quit message */
    int                    cursor_count;    /* per-queue cursor show count */
    struct list            msg_list[NB_MSG_KINDS];  /* lists of messages */
    struct list            send_result;     /* stack of sent messages waiting for result */
    struct list            callback_result; /* list of callback messages waiting for result */
    struct message_result *recv_result;     /* stack of received messages waiting for result */
    struct list            pending_timers;  /* list of pending timers */
    struct list            expired_timers;  /* list of expired timers */
    lparam_t               next_timer_id;   /* id for the next timer with a 0 window */
    struct timeout_user   *timeout;         /* timeout for next timer to expire */
    struct thread_input   *input;           /* thread input descriptor */
    struct hook_table     *hooks;           /* hook table */
    timeout_t              last_get_msg;    /* time of last get message call */
    int                    keystate_lock;   /* owns an input keystate lock */
    const queue_shm_t     *shared;          /* queue in session shared memory */
};

struct hotkey
{
    struct list         entry;        /* entry in desktop hotkey list */
    struct msg_queue   *queue;        /* queue owning this hotkey */
    user_handle_t       win;          /* window handle */
    int                 id;           /* hotkey id */
    unsigned int        vkey;         /* virtual key code */
    unsigned int        flags;        /* key modifiers */
};

static void msg_queue_dump( struct object *obj, int verbose );
static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int msg_queue_signaled( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_satisfied( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_destroy( struct object *obj );
static void msg_queue_poll_event( struct fd *fd, int event );
static void thread_input_dump( struct object *obj, int verbose );
static void thread_input_destroy( struct object *obj );
static void timer_callback( void *private );

static const struct object_ops msg_queue_ops =
{
    sizeof(struct msg_queue),  /* size */
    &no_type,                  /* type */
    msg_queue_dump,            /* dump */
    msg_queue_add_queue,       /* add_queue */
    msg_queue_remove_queue,    /* remove_queue */
    msg_queue_signaled,        /* signaled */
    msg_queue_satisfied,       /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_get_full_name,          /* get_full_name */
    no_lookup_name,            /* lookup_name */
    no_link_name,              /* link_name */
    NULL,                      /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    msg_queue_destroy          /* destroy */
};

static const struct fd_ops msg_queue_fd_ops =
{
    NULL,                        /* get_poll_events */
    msg_queue_poll_event,        /* poll_event */
    NULL,                        /* flush */
    NULL,                        /* get_fd_type */
    NULL,                        /* ioctl */
    NULL,                        /* queue_async */
    NULL,                        /* reselect_async */
    NULL                         /* cancel async */
};


static const struct object_ops thread_input_ops =
{
    sizeof(struct thread_input),  /* size */
    &no_type,                     /* type */
    thread_input_dump,            /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_get_full_name,             /* get_full_name */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    no_close_handle,              /* close_handle */
    thread_input_destroy          /* destroy */
};

/* pointer to input structure of foreground thread */
static unsigned int last_input_time;

static struct cursor_pos cursor_history[64];
static unsigned int cursor_history_latest;

static void queue_hardware_message( struct desktop *desktop, struct message *msg, int always_queue );
static void free_message( struct message *msg );

/* set the caret window in a given thread input, requires write lock on the thread input shared member */
static void set_caret_window( struct thread_input *input, input_shm_t *shared, user_handle_t win )
{
    if (!win || win != shared->caret)
    {
        shared->caret_rect.left   = 0;
        shared->caret_rect.top    = 0;
        shared->caret_rect.right  = 0;
        shared->caret_rect.bottom = 0;
    }
    shared->caret            = win;
    input->caret_hide        = 1;
    input->caret_state       = 0;
}

/* create a thread input object */
static struct thread_input *create_thread_input( struct thread *thread )
{
    struct thread_input *input;

    if ((input = alloc_object( &thread_input_ops )))
    {
        list_init( &input->msg_list );
        input->shared = NULL;

        if (!(input->desktop = get_thread_desktop( thread, 0 /* FIXME: access rights */ )))
        {
            release_object( input );
            return NULL;
        }
        memcpy( input->desktop_keystate, (const void *)input->desktop->shared->keystate,
                sizeof(input->desktop_keystate) );

        if (!(input->shared = alloc_shared_object()))
        {
            release_object( input );
            return NULL;
        }

        SHARED_WRITE_BEGIN( input->shared, input_shm_t )
        {
            shared->foreground = 0;
            shared->active = 0;
            shared->focus = 0;
            shared->capture = 0;
            shared->menu_owner = 0;
            shared->move_size = 0;
            set_caret_window( input, shared, 0 );
            shared->cursor = 0;
            shared->cursor_count = 0;
            memset( (void *)shared->keystate, 0, sizeof(shared->keystate) );
            shared->keystate_lock = 0;
        }
        SHARED_WRITE_END;
    }
    return input;
}

/* create a message queue object */
static struct msg_queue *create_msg_queue( struct thread *thread, struct thread_input *input )
{
    struct thread_input *new_input = NULL;
    struct msg_queue *queue;
    struct desktop *desktop;
    int i;

    if (!input)
    {
        if (!(new_input = create_thread_input( thread ))) return NULL;
        input = new_input;
    }

    if ((queue = alloc_object( &msg_queue_ops )))
    {
        queue->fd              = NULL;
        queue->paint_count     = 0;
        queue->hotkey_count    = 0;
        queue->quit_message    = 0;
        queue->cursor_count    = 0;
        queue->recv_result     = NULL;
        queue->next_timer_id   = 0x7fff;
        queue->timeout         = NULL;
        queue->input           = (struct thread_input *)grab_object( input );
        queue->hooks           = NULL;
        queue->last_get_msg    = current_time;
        queue->keystate_lock   = 0;
        list_init( &queue->send_result );
        list_init( &queue->callback_result );
        list_init( &queue->pending_timers );
        list_init( &queue->expired_timers );
        for (i = 0; i < NB_MSG_KINDS; i++) list_init( &queue->msg_list[i] );

        if (!(queue->shared = alloc_shared_object()))
        {
            release_object( queue );
            return NULL;
        }

        SHARED_WRITE_BEGIN( queue->shared, queue_shm_t )
        {
            memset( (void *)shared->hooks_count, 0, sizeof(shared->hooks_count) );
            shared->wake_mask = 0;
            shared->wake_bits = 0;
            shared->changed_mask = 0;
            shared->changed_bits = 0;
        }
        SHARED_WRITE_END;

        thread->queue = queue;

        if ((desktop = get_thread_desktop( thread, 0 )))
        {
            add_desktop_hook_count( desktop, thread, 1 );
            release_object( desktop );
        }
    }
    if (new_input) release_object( new_input );
    return queue;
}

/* free the message queue of a thread at thread exit */
void free_msg_queue( struct thread *thread )
{
    remove_thread_hooks( thread );
    if (!thread->queue) return;
    release_object( thread->queue );
    thread->queue = NULL;
}

/* synchronize thread input keystate with the desktop */
static void sync_input_keystate( struct thread_input *input )
{
    const input_shm_t *input_shm = input->shared;
    const desktop_shm_t *desktop_shm;
    int i;

    if (!input->desktop || input_shm->keystate_lock) return;
    desktop_shm = input->desktop->shared;

    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        for (i = 0; i < sizeof(shared->keystate); ++i)
        {
            if (input->desktop_keystate[i] == desktop_shm->keystate[i]) continue;
            shared->keystate[i] = input->desktop_keystate[i] = desktop_shm->keystate[i];
        }
    }
    SHARED_WRITE_END;
}

/* locks thread input keystate to prevent synchronization */
static void lock_input_keystate( struct thread_input *input )
{
    const input_shm_t *input_shm = input->shared;

    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        shared->keystate_lock++;
    }
    SHARED_WRITE_END;
}

/* unlock the thread input keystate and synchronize it again */
static void unlock_input_keystate( struct thread_input *input )
{
    const input_shm_t *input_shm = input->shared;

    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        shared->keystate_lock--;
    }
    SHARED_WRITE_END;

    if (!input_shm->keystate_lock) sync_input_keystate( input );
}

/* change the thread input data of a given thread */
static int assign_thread_input( struct thread *thread, struct thread_input *new_input )
{
    struct msg_queue *queue = thread->queue;
    const input_shm_t *input_shm;

    if (!queue)
    {
        thread->queue = create_msg_queue( thread, new_input );
        return thread->queue != NULL;
    }
    if (queue->input)
    {
        input_shm = queue->input->shared;

        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            shared->cursor_count -= queue->cursor_count;
        }
        SHARED_WRITE_END;

        if (queue->keystate_lock) unlock_input_keystate( queue->input );

        /* invalidate the old object to force clients to refresh their cached thread input */
        invalidate_shared_object( queue->input->shared );
        release_object( queue->input );
    }
    queue->input = (struct thread_input *)grab_object( new_input );
    if (queue->keystate_lock) lock_input_keystate( queue->input );

    input_shm = new_input->shared;
    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        shared->cursor_count += queue->cursor_count;
    }
    SHARED_WRITE_END;

    return 1;
}

/* allocate a hardware message and its data */
static struct message *alloc_hardware_message( lparam_t info, struct hw_msg_source source,
                                               unsigned int time, data_size_t extra_size )
{
    struct hardware_msg_data *msg_data;
    struct message *msg;

    if (!(msg = mem_alloc( sizeof(*msg) ))) return NULL;
    if (!(msg_data = mem_alloc( sizeof(*msg_data) + extra_size )))
    {
        free( msg );
        return NULL;
    }
    memset( msg, 0, sizeof(*msg) );
    msg->type      = MSG_HARDWARE;
    msg->time      = time;
    msg->data      = msg_data;
    msg->data_size = sizeof(*msg_data) + extra_size;

    memset( msg_data, 0, sizeof(*msg_data) + extra_size );
    msg_data->info   = info;
    msg_data->size   = msg->data_size;
    msg_data->source = source;
    return msg;
}

static int is_cursor_clipped( struct desktop *desktop )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct rectangle top_rect, clip_rect = desktop_shm->cursor.clip;
    get_virtual_screen_rect( desktop, &top_rect, 1 );
    return !is_rect_equal( &clip_rect, &top_rect );
}

static void queue_cursor_message( struct desktop *desktop, user_handle_t win, unsigned int message,
                                  lparam_t wparam, lparam_t lparam )
{
    static const struct hw_msg_source source = { IMDT_UNAVAILABLE, IMO_SYSTEM };
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct thread_input *input;
    struct message *msg;

    if (!(msg = alloc_hardware_message( 0, source, get_tick_count(), 0 ))) return;

    msg->msg = message;
    msg->wparam = wparam;
    msg->lparam = lparam;
    msg->x = desktop_shm->cursor.x;
    msg->y = desktop_shm->cursor.y;
    if (!(msg->win = win) && (input = desktop->foreground_input)) msg->win = input->shared->active;
    queue_hardware_message( desktop, msg, 1 );
}

static struct thread_input *get_desktop_cursor_thread_input( struct desktop *desktop )
{
    struct thread_input *input = NULL;
    struct thread *thread;

    if ((thread = get_window_thread( desktop->cursor_win )))
    {
        if (thread->queue) input = thread->queue->input;
        release_object( thread );
    }

    return input;
}

static int update_desktop_cursor_window( struct desktop *desktop, user_handle_t win )
{
    int updated = win != desktop->cursor_win;
    struct thread_input *input;
    desktop->cursor_win = win;

    if (updated && (input = get_desktop_cursor_thread_input( desktop )))
    {
        const input_shm_t *input_shm = input->shared;
        user_handle_t handle = input_shm->cursor_count < 0 ? 0 : input_shm->cursor;
        /* when clipping send the message to the foreground window as well, as some driver have an artificial overlay window */
        if (is_cursor_clipped( desktop )) queue_cursor_message( desktop, 0, WM_WINE_SETCURSOR, win, handle );
        queue_cursor_message( desktop, win, WM_WINE_SETCURSOR, win, handle );
    }

    return updated;
}

static int update_desktop_cursor_pos( struct desktop *desktop, user_handle_t win, int x, int y )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    int updated;
    unsigned int time = get_tick_count();

    x = max( min( x, desktop_shm->cursor.clip.right - 1 ), desktop_shm->cursor.clip.left );
    y = max( min( y, desktop_shm->cursor.clip.bottom - 1 ), desktop_shm->cursor.clip.top );

    SHARED_WRITE_BEGIN( desktop_shm, desktop_shm_t )
    {
        updated = shared->cursor.x != x || shared->cursor.y != y;
        shared->cursor.x = x;
        shared->cursor.y = y;
        shared->cursor.last_change = time;
    }
    SHARED_WRITE_END;

    if (!win || !is_window_visible( win ) || is_window_transparent( win ))
        win = shallow_window_from_point( desktop, x, y );
    if (update_desktop_cursor_window( desktop, win )) updated = 1;

    return updated;
}

static void update_desktop_cursor_handle( struct desktop *desktop, struct thread_input *input, user_handle_t handle )
{
    if (input == get_desktop_cursor_thread_input( desktop ))
    {
        user_handle_t win = desktop->cursor_win;
        /* when clipping send the message to the foreground window as well, as some driver have an artificial overlay window */
        if (is_cursor_clipped( desktop )) queue_cursor_message( desktop, 0, WM_WINE_SETCURSOR, win, handle );
        queue_cursor_message( desktop, win, WM_WINE_SETCURSOR, win, handle );
    }
}

/* set the cursor position and queue the corresponding mouse message */
static void set_cursor_pos( struct desktop *desktop, int x, int y )
{
    static const struct hw_msg_source source = { IMDT_UNAVAILABLE, IMO_SYSTEM };
    const struct rawinput_device *device;
    struct message *msg;

    if ((device = current->process->rawinput_mouse) && (device->flags & RIDEV_NOLEGACY))
    {
        update_desktop_cursor_pos( desktop, 0, x, y );
        return;
    }

    if (!(msg = alloc_hardware_message( 0, source, get_tick_count(), 0 ))) return;

    msg->msg = WM_MOUSEMOVE;
    msg->x   = x;
    msg->y   = y;
    queue_hardware_message( desktop, msg, 1 );
}

/* sync cursor position after window change */
void update_cursor_pos( struct desktop *desktop )
{
    const desktop_shm_t *desktop_shm;

    desktop_shm = desktop->shared;
    set_cursor_pos( desktop, desktop_shm->cursor.x, desktop_shm->cursor.y );
}

/* retrieve default position and time for synthesized messages */
static void get_message_defaults( struct msg_queue *queue, int *x, int *y, unsigned int *time )
{
    struct desktop *desktop = queue->input->desktop;
    const desktop_shm_t *desktop_shm = desktop->shared;

    *x = desktop_shm->cursor.x;
    *y = desktop_shm->cursor.y;
    *time = get_tick_count();
}

/* set the cursor clip rectangle */
void set_clip_rectangle( struct desktop *desktop, const struct rectangle *rect, unsigned int flags, int reset )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct rectangle top_rect, new_rect;
    unsigned int old_flags;
    int x, y;

    get_virtual_screen_rect( desktop, &top_rect, 1 );
    if (rect)
    {
        new_rect = *rect;
        if (new_rect.left   < top_rect.left)   new_rect.left   = top_rect.left;
        if (new_rect.right  > top_rect.right)  new_rect.right  = top_rect.right;
        if (new_rect.top    < top_rect.top)    new_rect.top    = top_rect.top;
        if (new_rect.bottom > top_rect.bottom) new_rect.bottom = top_rect.bottom;
        if (new_rect.left > new_rect.right || new_rect.top > new_rect.bottom) new_rect = top_rect;
    }
    else new_rect = top_rect;

    SHARED_WRITE_BEGIN( desktop_shm, desktop_shm_t )
    {
        shared->cursor.clip = new_rect;
    }
    SHARED_WRITE_END;

    old_flags = desktop->clip_flags;
    desktop->clip_flags = flags;

    /* warp the mouse to be inside the clip rect */
    x = max( min( desktop_shm->cursor.x, new_rect.right - 1 ), new_rect.left );
    y = max( min( desktop_shm->cursor.y, new_rect.bottom - 1 ), new_rect.top );
    if (x != desktop_shm->cursor.x || y != desktop_shm->cursor.y) set_cursor_pos( desktop, x, y );

    /* request clip cursor rectangle reset to the desktop thread */
    if (reset) post_desktop_message( desktop, WM_WINE_CLIPCURSOR, flags, FALSE );

    /* notify foreground thread of reset, clipped, or released cursor rect */
    if (reset || flags != SET_CURSOR_NOCLIP || old_flags != SET_CURSOR_NOCLIP)
        queue_cursor_message( desktop, 0, WM_WINE_CLIPCURSOR, flags, reset );
}

/* change the foreground input and reset the cursor clip rect */
static void set_foreground_input( struct desktop *desktop, struct thread_input *input )
{
    const input_shm_t *input_shm, *old_input_shm;
    shared_object_t dummy_obj = {0};

    if (desktop->foreground_input == input) return;
    input_shm = input ? input->shared : &dummy_obj.shm.input;
    old_input_shm = desktop->foreground_input ? desktop->foreground_input->shared : &dummy_obj.shm.input;

    set_clip_rectangle( desktop, NULL, SET_CURSOR_NOCLIP, 1 );
    desktop->foreground_input = input;

    SHARED_WRITE_BEGIN( old_input_shm, input_shm_t )
    {
        input_shm_t *old_shared = shared;
        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            old_shared->foreground = 0;
            shared->foreground = 1;
        }
        SHARED_WRITE_END;
    }
    SHARED_WRITE_END;
}

/* get the hook table for a given thread */
struct hook_table *get_queue_hooks( struct thread *thread )
{
    if (!thread->queue) return NULL;
    return thread->queue->hooks;
}

/* set the hook table for a given thread, allocating the queue if needed */
void set_queue_hooks( struct thread *thread, struct hook_table *hooks )
{
    struct msg_queue *queue = thread->queue;
    if (!queue && !(queue = create_msg_queue( thread, NULL ))) return;
    if (queue->hooks) release_object( queue->hooks );
    queue->hooks = hooks;
}

/* update the thread message queue hooks counters */
void add_queue_hook_count( struct thread *thread, unsigned int index, int count )
{
    if (!thread->queue) return;

    SHARED_WRITE_BEGIN( thread->queue->shared, queue_shm_t )
    {
        shared->hooks_count[index] += count;
    }
    SHARED_WRITE_END;

    assert( thread->queue->shared->hooks_count[index] >= 0 );
}

/* check the queue status */
static inline int is_signaled( struct msg_queue *queue )
{
    const queue_shm_t *queue_shm = queue->shared;
    return (queue_shm->wake_bits & queue_shm->wake_mask) ||
           (queue_shm->changed_bits & queue_shm->changed_mask);
}

/* set some queue bits */
static inline void set_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    const queue_shm_t *queue_shm = queue->shared;

    if (bits & (QS_KEY | QS_MOUSEBUTTON))
    {
        if (!queue->keystate_lock) lock_input_keystate( queue->input );
        queue->keystate_lock = 1;
    }

    SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
    {
        shared->wake_bits |= bits;
        shared->changed_bits |= bits;
    }
    SHARED_WRITE_END;

    if (is_signaled( queue )) wake_up( &queue->obj, 0 );
}

/* clear some queue bits */
static inline void clear_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    const queue_shm_t *queue_shm = queue->shared;

    SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
    {
        shared->wake_bits &= ~bits;
        shared->changed_bits &= ~bits;
    }
    SHARED_WRITE_END;

    if (!(queue_shm->wake_bits & (QS_KEY | QS_MOUSEBUTTON)))
    {
        if (queue->keystate_lock) unlock_input_keystate( queue->input );
        queue->keystate_lock = 0;
    }
}

/* check if message is matched by the filter */
static inline int check_msg_filter( unsigned int msg, unsigned int first, unsigned int last )
{
    return (msg >= first && msg <= last);
}

/* check whether a message filter contains at least one potential hardware message */
static inline int filter_contains_hw_range( unsigned int first, unsigned int last )
{
    /* hardware message ranges are (in numerical order):
     *   WM_NCMOUSEFIRST .. WM_NCMOUSELAST
     *   WM_INPUT_DEVICE_CHANGE .. WM_KEYLAST
     *   WM_MOUSEFIRST .. WM_MOUSELAST
     */
    if (last < WM_NCMOUSEFIRST) return 0;
    if (first > WM_NCMOUSELAST && last < WM_INPUT_DEVICE_CHANGE) return 0;
    if (first > WM_KEYLAST && last < WM_MOUSEFIRST) return 0;
    if (first > WM_MOUSELAST) return 0;
    return 1;
}

/* get the QS_* bit corresponding to a given hardware message */
static inline int get_hardware_msg_bit( unsigned int message )
{
    if (message >= WM_POINTERUPDATE && message <= WM_POINTERLEAVE) return QS_POINTER;
    if (message == WM_INPUT_DEVICE_CHANGE || message == WM_INPUT) return QS_RAWINPUT;
    if (message == WM_MOUSEMOVE || message == WM_NCMOUSEMOVE) return QS_MOUSEMOVE;
    if (message >= WM_KEYFIRST && message <= WM_KEYLAST) return QS_KEY;
    if (message == WM_WINE_CLIPCURSOR) return QS_RAWINPUT;
    if (message == WM_WINE_SETCURSOR) return QS_RAWINPUT;
    return QS_MOUSEBUTTON;
}

/* get the current thread queue, creating it if needed */
static inline struct msg_queue *get_current_queue(void)
{
    struct msg_queue *queue = current->queue;
    if (!queue) queue = create_msg_queue( current, NULL );
    return queue;
}

/* get a (pseudo-)unique id to tag hardware messages */
static inline unsigned int get_unique_id(void)
{
    static unsigned int id;
    if (!++id) id = 1;  /* avoid an id of 0 */
    return id;
}

/* lookup an already queued mouse message that matches the message, window and type */
static struct message *find_mouse_message( struct thread_input *input, const struct message *msg )
{
    struct message *prev;
    struct list *ptr;

    for (ptr = list_tail( &input->msg_list ); ptr; ptr = list_prev( &input->msg_list, ptr ))
    {
        prev = LIST_ENTRY( ptr, struct message, entry );
        if (prev->msg >> 31) continue; /* ignore internal messages */
        if (prev->msg != WM_INPUT) break;
    }
    if (!ptr) return NULL;
    if (prev->result) return NULL;
    if (prev->win && msg->win && prev->win != msg->win) return NULL;
    if (prev->msg != msg->msg) return NULL;
    if (prev->type != msg->type) return NULL;
    return prev;
}

/* try to merge a WM_MOUSEMOVE message with the last in the list; return 1 if successful */
static int merge_mousemove( struct thread_input *input, const struct message *msg )
{
    struct message *prev;

    if (!(prev = find_mouse_message( input, msg ))) return 0;

    prev->wparam  = msg->wparam;
    prev->lparam  = msg->lparam;
    prev->x       = msg->x;
    prev->y       = msg->y;
    prev->time    = msg->time;
    if (msg->type == MSG_HARDWARE && prev->data && msg->data)
    {
        struct hardware_msg_data *prev_data = prev->data;
        struct hardware_msg_data *msg_data = msg->data;
        prev_data->info = msg_data->info;
    }
    list_remove( &prev->entry );
    list_add_tail( &input->msg_list, &prev->entry );
    return 1;
}

/* try to merge a unique message with the last in the list; return 1 if successful */
static int merge_unique_message( struct thread_input *input, unsigned int message, const struct message *msg )
{
    struct message *prev;

    LIST_FOR_EACH_ENTRY_REV( prev, &input->msg_list, struct message, entry )
        if (prev->msg == message) break;
    if (&prev->entry == &input->msg_list) return 0;

    if (prev->result) return 0;
    if (prev->win != msg->win) return 0;
    if (prev->type != msg->type) return 0;

    /* now we can merge it */
    prev->wparam  = msg->wparam;
    prev->lparam  = msg->lparam;
    prev->x       = msg->x;
    prev->y       = msg->y;
    prev->time    = msg->time;
    list_remove( &prev->entry );
    list_add_tail( &input->msg_list, &prev->entry );

    return 1;
}

/* try to merge a message with the messages in the list; return 1 if successful */
static int merge_message( struct thread_input *input, const struct message *msg )
{
    if (msg->msg == WM_MOUSEMOVE) return merge_mousemove( input, msg );
    if (msg->msg == WM_WINE_CLIPCURSOR) return merge_unique_message( input, WM_WINE_CLIPCURSOR, msg );
    if (msg->msg == WM_WINE_SETCURSOR) return merge_unique_message( input, WM_WINE_SETCURSOR, msg );
    return 0;
}

/* free a result structure */
static void free_result( struct message_result *result )
{
    if (result->timeout) remove_timeout_user( result->timeout );
    free( result->data );
    if (result->callback_msg) free_message( result->callback_msg );
    if (result->hardware_msg) free_message( result->hardware_msg );
    if (result->desktop) release_object( result->desktop );
    free( result );
}

/* remove the result from the sender list it is on */
static inline void remove_result_from_sender( struct message_result *result )
{
    assert( result->sender );

    list_remove( &result->sender_entry );
    result->sender = NULL;
    if (!result->receiver) free_result( result );
}

/* store the message result in the appropriate structure */
static void store_message_result( struct message_result *res, lparam_t result, unsigned int error )
{
    res->result  = result;
    res->error   = error;
    res->replied = 1;
    if (res->timeout)
    {
        remove_timeout_user( res->timeout );
        res->timeout = NULL;
    }

    if (res->hardware_msg)
    {
        if (!error && result)  /* rejected by the hook */
            free_message( res->hardware_msg );
        else
            queue_hardware_message( res->desktop, res->hardware_msg, 0 );

        res->hardware_msg = NULL;
    }

    if (res->sender)
    {
        if (res->callback_msg)
        {
            /* queue the callback message in the sender queue */
            struct callback_msg_data *data = res->callback_msg->data;
            data->result = result;
            list_add_tail( &res->sender->msg_list[SEND_MESSAGE], &res->callback_msg->entry );
            set_queue_bits( res->sender, QS_SENDMESSAGE );
            res->callback_msg = NULL;
            remove_result_from_sender( res );
        }
        else
        {
            /* wake sender queue if waiting on this result */
            if (list_head(&res->sender->send_result) == &res->sender_entry)
                set_queue_bits( res->sender, QS_SMRESULT );
        }
    }
    else if (!res->receiver) free_result( res );
}

/* free a message when deleting a queue or window */
static void free_message( struct message *msg )
{
    struct message_result *result = msg->result;
    if (result)
    {
        result->msg = NULL;
        result->receiver = NULL;
        store_message_result( result, 0, STATUS_ACCESS_DENIED /*FIXME*/ );
    }
    free( msg->data );
    free( msg );
}

/* remove (and free) a message from a message list */
static void remove_queue_message( struct msg_queue *queue, struct message *msg,
                                  enum message_kind kind )
{
    list_remove( &msg->entry );
    switch(kind)
    {
    case SEND_MESSAGE:
        if (list_empty( &queue->msg_list[kind] )) clear_queue_bits( queue, QS_SENDMESSAGE );
        break;
    case POST_MESSAGE:
        if (list_empty( &queue->msg_list[kind] ) && !queue->quit_message)
            clear_queue_bits( queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
        if (msg->msg == WM_HOTKEY && --queue->hotkey_count == 0)
            clear_queue_bits( queue, QS_HOTKEY );
        break;
    }
    free_message( msg );
}

/* message timed out without getting a reply */
static void result_timeout( void *private )
{
    struct message_result *result = private;

    assert( !result->replied );

    result->timeout = NULL;

    if (result->msg)  /* not received yet */
    {
        struct message *msg = result->msg;

        result->msg = NULL;
        msg->result = NULL;
        remove_queue_message( result->receiver, msg, SEND_MESSAGE );
        result->receiver = NULL;
    }
    store_message_result( result, 0, STATUS_TIMEOUT );
}

/* allocate and fill a message result structure */
static struct message_result *alloc_message_result( struct msg_queue *send_queue,
                                                    struct msg_queue *recv_queue,
                                                    struct message *msg, timeout_t timeout )
{
    struct message_result *result = mem_alloc( sizeof(*result) );
    if (result)
    {
        result->msg          = msg;
        result->sender       = send_queue;
        result->receiver     = recv_queue;
        result->replied      = 0;
        result->data         = NULL;
        result->data_size    = 0;
        result->timeout      = NULL;
        result->hardware_msg = NULL;
        result->desktop      = NULL;
        result->callback_msg = NULL;

        if (msg->type == MSG_CALLBACK)
        {
            struct message *callback_msg = mem_alloc( sizeof(*callback_msg) );

            if (!callback_msg)
            {
                free( result );
                return NULL;
            }
            callback_msg->type      = MSG_CALLBACK_RESULT;
            callback_msg->win       = msg->win;
            callback_msg->msg       = msg->msg;
            callback_msg->wparam    = 0;
            callback_msg->lparam    = 0;
            callback_msg->time      = get_tick_count();
            callback_msg->result    = NULL;
            /* steal the data from the original message */
            callback_msg->data      = msg->data;
            callback_msg->data_size = msg->data_size;
            msg->data = NULL;
            msg->data_size = 0;

            result->callback_msg = callback_msg;
            list_add_head( &send_queue->callback_result, &result->sender_entry );
        }
        else if (send_queue)
        {
            list_add_head( &send_queue->send_result, &result->sender_entry );
            clear_queue_bits( send_queue, QS_SMRESULT );
        }

        if (timeout != TIMEOUT_INFINITE)
            result->timeout = add_timeout_user( timeout, result_timeout, result );
    }
    return result;
}

/* receive a message, removing it from the sent queue */
static void receive_message( struct msg_queue *queue, struct message *msg,
                             struct get_message_reply *reply )
{
    struct message_result *result = msg->result;

    reply->total = msg->data_size;
    if (msg->data_size > get_reply_max_size())
    {
        set_error( STATUS_BUFFER_OVERFLOW );
        return;
    }
    reply->type   = msg->type;
    reply->win    = msg->win;
    reply->msg    = msg->msg;
    reply->wparam = msg->wparam;
    reply->lparam = msg->lparam;
    reply->x      = msg->x;
    reply->y      = msg->y;
    reply->time   = msg->time;

    if (msg->data) set_reply_data_ptr( msg->data, msg->data_size );

    list_remove( &msg->entry );
    /* put the result on the receiver result stack */
    if (result)
    {
        result->msg = NULL;
        result->recv_next  = queue->recv_result;
        queue->recv_result = result;
    }
    free( msg );
    if (list_empty( &queue->msg_list[SEND_MESSAGE] )) clear_queue_bits( queue, QS_SENDMESSAGE );
}

/* set the result of the current received message */
static void reply_message( struct msg_queue *queue, lparam_t result,
                           unsigned int error, int remove, const void *data, data_size_t len )
{
    struct message_result *res = queue->recv_result;

    if (remove)
    {
        queue->recv_result = res->recv_next;
        res->receiver = NULL;
        if (!res->sender && !res->hardware_msg)  /* no one waiting for it */
        {
            free_result( res );
            return;
        }
    }
    if (!res->replied)
    {
        if (len && (res->data = memdup( data, len ))) res->data_size = len;
        store_message_result( res, result, error );
    }
}

static int match_window( user_handle_t win, user_handle_t msg_win )
{
    if (!win) return 1;
    if (win == -1 || win == 1) return !msg_win;
    if (msg_win == win) return 1;
    return is_child_window( win, msg_win );
}

/* retrieve a posted message */
static int get_posted_message( struct msg_queue *queue, user_handle_t win,
                               unsigned int first, unsigned int last, unsigned int flags,
                               struct get_message_reply *reply )
{
    struct message *msg;

    /* check against the filters */
    LIST_FOR_EACH_ENTRY( msg, &queue->msg_list[POST_MESSAGE], struct message, entry )
    {
        if (!match_window( win, msg->win )) continue;
        if (!check_msg_filter( msg->msg, first, last )) continue;
        goto found; /* found one */
    }
    return 0;

    /* return it to the app */
found:
    reply->total = msg->data_size;
    if (msg->data_size > get_reply_max_size())
    {
        set_error( STATUS_BUFFER_OVERFLOW );
        return 1;
    }
    reply->type   = msg->type;
    reply->win    = msg->win;
    reply->msg    = msg->msg;
    reply->wparam = msg->wparam;
    reply->lparam = msg->lparam;
    reply->x      = msg->x;
    reply->y      = msg->y;
    reply->time   = msg->time;

    if (flags & PM_REMOVE)
    {
        if (msg->data)
        {
            set_reply_data_ptr( msg->data, msg->data_size );
            msg->data = NULL;
            msg->data_size = 0;
        }
        remove_queue_message( queue, msg, POST_MESSAGE );
    }
    else if (msg->data) set_reply_data( msg->data, msg->data_size );

    return 1;
}

static int get_quit_message( struct msg_queue *queue, unsigned int flags,
                             struct get_message_reply *reply )
{
    if (queue->quit_message)
    {
        reply->total  = 0;
        reply->type   = MSG_POSTED;
        reply->win    = 0;
        reply->msg    = WM_QUIT;
        reply->wparam = queue->exit_code;
        reply->lparam = 0;

        get_message_defaults( queue, &reply->x, &reply->y, &reply->time );

        if (flags & PM_REMOVE)
        {
            queue->quit_message = 0;
            if (list_empty( &queue->msg_list[POST_MESSAGE] ))
                clear_queue_bits( queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
        }
        return 1;
    }
    else
        return 0;
}

/* empty a message list and free all the messages */
static void empty_msg_list( struct list *list )
{
    struct list *ptr;

    while ((ptr = list_head( list )) != NULL)
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        list_remove( &msg->entry );
        free_message( msg );
    }
}

/* cleanup all pending results when deleting a queue */
static void cleanup_results( struct msg_queue *queue )
{
    struct list *entry;

    while ((entry = list_head( &queue->send_result )) != NULL)
    {
        remove_result_from_sender( LIST_ENTRY( entry, struct message_result, sender_entry ) );
    }

    while ((entry = list_head( &queue->callback_result )) != NULL)
    {
        remove_result_from_sender( LIST_ENTRY( entry, struct message_result, sender_entry ) );
    }

    while (queue->recv_result)
        reply_message( queue, 0, STATUS_ACCESS_DENIED /*FIXME*/, 1, NULL, 0 );
}

/* check if the thread owning the queue is hung (not checking for messages) */
static int is_queue_hung( struct msg_queue *queue )
{
    struct wait_queue_entry *entry;

    if (current_time - queue->last_get_msg <= 5 * TICKS_PER_SEC)
        return 0;  /* less than 5 seconds since last get message -> not hung */

    LIST_FOR_EACH_ENTRY( entry, &queue->obj.wait_queue, struct wait_queue_entry, entry )
    {
        if (get_wait_queue_thread(entry)->queue == queue)
            return 0;  /* thread is waiting on queue -> not hung */
    }
    return 1;
}

static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;

    /* a thread can only wait on its own queue */
    if (get_wait_queue_thread(entry)->queue != queue)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }

    if (queue->fd && list_empty( &obj->wait_queue ))  /* first on the queue */
        set_fd_events( queue->fd, POLLIN );
    add_queue( obj, entry );
    return 1;
}

static void msg_queue_remove_queue(struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;

    remove_queue( obj, entry );
    if (queue->fd && list_empty( &obj->wait_queue ))  /* last on the queue is gone */
        set_fd_events( queue->fd, 0 );
}

static void msg_queue_dump( struct object *obj, int verbose )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    const queue_shm_t *queue_shm = queue->shared;
    fprintf( stderr, "Msg queue bits=%x mask=%x\n",
             queue_shm->wake_bits, queue_shm->wake_mask );
}

static int msg_queue_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    int ret = 0;

    if (queue->fd)
    {
        if ((ret = check_fd_events( queue->fd, POLLIN )))
            /* stop waiting on select() if we are signaled */
            set_fd_events( queue->fd, 0 );
        else if (!list_empty( &obj->wait_queue ))
            /* restart waiting on poll() if we are no longer signaled */
            set_fd_events( queue->fd, POLLIN );
    }
    return ret || is_signaled( queue );
}

static void msg_queue_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    const queue_shm_t *queue_shm = queue->shared;

    SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
    {
        shared->wake_mask = 0;
        shared->changed_mask = 0;
    }
    SHARED_WRITE_END;
}

static void msg_queue_destroy( struct object *obj )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct list *ptr;
    struct hotkey *hotkey, *hotkey2;
    const input_shm_t *input_shm = queue->input->shared;
    int i;

    cleanup_results( queue );
    for (i = 0; i < NB_MSG_KINDS; i++) empty_msg_list( &queue->msg_list[i] );

    LIST_FOR_EACH_ENTRY_SAFE( hotkey, hotkey2, &queue->input->desktop->hotkeys, struct hotkey, entry )
    {
        if (hotkey->queue == queue)
        {
            list_remove( &hotkey->entry );
            free( hotkey );
        }
    }

    while ((ptr = list_head( &queue->pending_timers )))
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        list_remove( &timer->entry );
        free( timer );
    }
    while ((ptr = list_head( &queue->expired_timers )))
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        list_remove( &timer->entry );
        free( timer );
    }
    if (queue->timeout) remove_timeout_user( queue->timeout );
    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        shared->cursor_count -= queue->cursor_count;
    }
    SHARED_WRITE_END;
    if (queue->keystate_lock) unlock_input_keystate( queue->input );
    release_object( queue->input );
    if (queue->hooks) release_object( queue->hooks );
    if (queue->fd) release_object( queue->fd );
    if (queue->shared) free_shared_object( queue->shared );
}

static void msg_queue_poll_event( struct fd *fd, int event )
{
    struct msg_queue *queue = get_fd_user( fd );
    assert( queue->obj.ops == &msg_queue_ops );

    if (event & (POLLERR | POLLHUP)) set_fd_events( fd, -1 );
    else set_fd_events( queue->fd, 0 );
    wake_up( &queue->obj, 0 );
}

static void thread_input_dump( struct object *obj, int verbose )
{
    struct thread_input *input = (struct thread_input *)obj;
    const input_shm_t *input_shm = input->shared;
    fprintf( stderr, "Thread input focus=%08x capture=%08x active=%08x\n",
             input_shm->focus, input_shm->capture, input_shm->active );
}

static void thread_input_destroy( struct object *obj )
{
    struct thread_input *input = (struct thread_input *)obj;
    struct desktop *desktop;

    empty_msg_list( &input->msg_list );
    if ((desktop = input->desktop))
    {
        if (desktop->foreground_input == input) desktop->foreground_input = NULL;
        release_object( desktop );
    }
    if (input->shared) free_shared_object( input->shared );
}

/* fix the thread input data when a window is destroyed */
static inline void thread_input_cleanup_window( struct msg_queue *queue, user_handle_t window )
{
    struct thread_input *input = queue->input;
    const input_shm_t *input_shm = input->shared;

    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        if (window == shared->focus) shared->focus = 0;
        if (window == shared->capture) shared->capture = 0;
        if (window == shared->active) shared->active = 0;
        if (window == shared->menu_owner) shared->menu_owner = 0;
        if (window == shared->move_size) shared->move_size = 0;
        if (window == shared->caret) set_caret_window( input, shared, 0 );
    }
    SHARED_WRITE_END;

}

/* check if the specified window can be set in the input data of a given queue */
static int check_queue_input_window( struct msg_queue *queue, user_handle_t window )
{
    struct thread *thread;
    int ret = 0;

    if (!window) return 1;  /* we can always clear the data */

    if ((thread = get_window_thread( window )))
    {
        ret = (queue->input == thread->queue->input);
        if (!ret) set_error( STATUS_ACCESS_DENIED );
        release_object( thread );
    }
    else set_error( STATUS_INVALID_HANDLE );

    return ret;
}

/* check if the thread queue is idle and set the process idle event if so */
void check_thread_queue_idle( struct thread *thread )
{
    struct msg_queue *queue = thread->queue;
    const queue_shm_t *queue_shm = queue->shared;

    if ((queue_shm->wake_mask & QS_SMRESULT)) return;
    if (thread->process->idle_event) set_event( thread->process->idle_event );
}

/* make sure the specified thread has a queue */
int init_thread_queue( struct thread *thread )
{
    if (thread->queue) return 1;
    return (create_msg_queue( thread, NULL ) != NULL);
}

/* attach two thread input data structures */
int attach_thread_input( struct thread *thread_from, struct thread *thread_to )
{
    struct desktop *desktop;
    struct thread_input *input, *old_input;
    int ret;

    if (!thread_to->queue && !(thread_to->queue = create_msg_queue( thread_to, NULL ))) return 0;
    if (!(desktop = get_thread_desktop( thread_from, 0 ))) return 0;
    input = (struct thread_input *)grab_object( thread_to->queue->input );
    if (input->desktop != desktop)
    {
        set_error( STATUS_ACCESS_DENIED );
        release_object( input );
        release_object( desktop );
        return 0;
    }
    release_object( desktop );

    if (thread_from->queue)
    {
        const input_shm_t *old_input_shm, *input_shm;
        old_input = thread_from->queue->input;
        old_input_shm = old_input->shared;
        input_shm = input->shared;

        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            if (!shared->active) shared->active = old_input_shm->active;
            if (!shared->focus) shared->focus = old_input_shm->focus;
        }
        SHARED_WRITE_END;
    }

    ret = assign_thread_input( thread_from, input );
    if (ret)
    {
        const input_shm_t *input_shm = input->shared;
        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            memset( (void *)shared->keystate, 0, sizeof(shared->keystate) );
        }
        SHARED_WRITE_END;
    }
    release_object( input );
    return ret;
}

/* detach two thread input data structures */
void detach_thread_input( struct thread *thread_from )
{
    struct thread *thread;
    struct thread_input *input, *old_input = thread_from->queue->input;

    if ((input = create_thread_input( thread_from )))
    {
        const input_shm_t *old_input_shm, *input_shm;
        old_input_shm = old_input->shared;
        input_shm = input->shared;

        if (old_input_shm->focus && (thread = get_window_thread( old_input_shm->focus )))
        {
            if (thread == thread_from)
            {
                SHARED_WRITE_BEGIN( old_input_shm, input_shm_t )
                {
                    input_shm_t *old_shared = shared;
                    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
                    {
                        shared->focus = old_shared->focus;
                        old_shared->focus = 0;
                    }
                    SHARED_WRITE_END;
                }
                SHARED_WRITE_END;
            }
            release_object( thread );
        }
        if (old_input_shm->active && (thread = get_window_thread( old_input_shm->active )))
        {
            if (thread == thread_from)
            {
                SHARED_WRITE_BEGIN( old_input_shm, input_shm_t )
                {
                    input_shm_t *old_shared = shared;
                    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
                    {
                        shared->active = old_shared->active;
                        old_shared->active = 0;
                    }
                    SHARED_WRITE_END;
                }
                SHARED_WRITE_END;
            }
            release_object( thread );
        }
        assign_thread_input( thread_from, input );
        release_object( input );
    }
}


/* set the next timer to expire */
static void set_next_timer( struct msg_queue *queue )
{
    struct list *ptr;

    if (queue->timeout)
    {
        remove_timeout_user( queue->timeout );
        queue->timeout = NULL;
    }
    if ((ptr = list_head( &queue->pending_timers )))
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        queue->timeout = add_timeout_user( abstime_to_timeout(timer->when), timer_callback, queue );
    }
    /* set/clear QS_TIMER bit */
    if (list_empty( &queue->expired_timers ))
        clear_queue_bits( queue, QS_TIMER );
    else
        set_queue_bits( queue, QS_TIMER );
}

/* find a timer from its window and id */
static struct timer *find_timer( struct msg_queue *queue, user_handle_t win,
                                 unsigned int msg, lparam_t id )
{
    struct list *ptr;

    /* we need to search both lists */

    LIST_FOR_EACH( ptr, &queue->pending_timers )
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win && timer->msg == msg && timer->id == id) return timer;
    }
    LIST_FOR_EACH( ptr, &queue->expired_timers )
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win && timer->msg == msg && timer->id == id) return timer;
    }
    return NULL;
}

/* callback for the next timer expiration */
static void timer_callback( void *private )
{
    struct msg_queue *queue = private;
    struct list *ptr;

    queue->timeout = NULL;
    /* move on to the next timer */
    ptr = list_head( &queue->pending_timers );
    list_remove( ptr );
    list_add_tail( &queue->expired_timers, ptr );
    set_next_timer( queue );
}

/* link a timer at its rightful place in the queue list */
static void link_timer( struct msg_queue *queue, struct timer *timer )
{
    struct list *ptr;

    for (ptr = queue->pending_timers.next; ptr != &queue->pending_timers; ptr = ptr->next)
    {
        struct timer *t = LIST_ENTRY( ptr, struct timer, entry );
        if (t->when <= timer->when) break;
    }
    list_add_before( ptr, &timer->entry );
}

/* remove a timer from the queue timer list and free it */
static void free_timer( struct msg_queue *queue, struct timer *timer )
{
    list_remove( &timer->entry );
    free( timer );
    set_next_timer( queue );
}

/* restart an expired timer */
static void restart_timer( struct msg_queue *queue, struct timer *timer )
{
    list_remove( &timer->entry );
    while (-timer->when <= monotonic_time) timer->when -= (timeout_t)timer->rate * 10000;
    link_timer( queue, timer );
    set_next_timer( queue );
}

/* find an expired timer matching the filtering parameters */
static struct timer *find_expired_timer( struct msg_queue *queue, user_handle_t win,
                                         unsigned int get_first, unsigned int get_last,
                                         int remove )
{
    struct list *ptr;

    LIST_FOR_EACH( ptr, &queue->expired_timers )
    {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (win && timer->win != win) continue;
        if (check_msg_filter( timer->msg, get_first, get_last ))
        {
            if (remove) restart_timer( queue, timer );
            return timer;
        }
    }
    return NULL;
}

/* add a timer */
static struct timer *set_timer( struct msg_queue *queue, unsigned int rate )
{
    struct timer *timer = mem_alloc( sizeof(*timer) );
    if (timer)
    {
        timer->rate = max( rate, 1 );
        timer->when = -monotonic_time - (timeout_t)timer->rate * 10000;
        link_timer( queue, timer );
        /* check if we replaced the next timer */
        if (list_head( &queue->pending_timers ) == &timer->entry) set_next_timer( queue );
    }
    return timer;
}

/* change the input key state for a given key */
static void set_input_key_state( volatile unsigned char *keystate, unsigned char key, unsigned char down )
{
    if (down)
    {
        if (!(keystate[key] & 0x80)) keystate[key] ^= 0x01;
        keystate[key] |= down;
    }
    else keystate[key] &= ~0x80;
}

/* update the input key state for a keyboard message */
static void update_key_state( volatile unsigned char *keystate, unsigned int msg,
                              lparam_t wparam, int desktop )
{
    unsigned char key, down = 0, down_val = desktop ? 0xc0 : 0x80;

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        down = down_val;
        /* fall through */
    case WM_LBUTTONUP:
        set_input_key_state( keystate, VK_LBUTTON, down );
        break;
    case WM_MBUTTONDOWN:
        down = down_val;
        /* fall through */
    case WM_MBUTTONUP:
        set_input_key_state( keystate, VK_MBUTTON, down );
        break;
    case WM_RBUTTONDOWN:
        down = down_val;
        /* fall through */
    case WM_RBUTTONUP:
        set_input_key_state( keystate, VK_RBUTTON, down );
        break;
    case WM_XBUTTONDOWN:
        down = down_val;
        /* fall through */
    case WM_XBUTTONUP:
        if (wparam >> 16 == XBUTTON1) set_input_key_state( keystate, VK_XBUTTON1, down );
        else if (wparam >> 16 == XBUTTON2) set_input_key_state( keystate, VK_XBUTTON2, down );
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = down_val;
        /* fall through */
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = (unsigned char)wparam;
        set_input_key_state( keystate, key, down );
        switch(key)
        {
        case VK_LCONTROL:
        case VK_RCONTROL:
            down = (keystate[VK_LCONTROL] | keystate[VK_RCONTROL]) & 0x80;
            set_input_key_state( keystate, VK_CONTROL, down );
            break;
        case VK_LMENU:
        case VK_RMENU:
            down = (keystate[VK_LMENU] | keystate[VK_RMENU]) & 0x80;
            set_input_key_state( keystate, VK_MENU, down );
            break;
        case VK_LSHIFT:
        case VK_RSHIFT:
            down = (keystate[VK_LSHIFT] | keystate[VK_RSHIFT]) & 0x80;
            set_input_key_state( keystate, VK_SHIFT, down );
            break;
        }
        break;
    }
}

static void update_thread_input_key_state( struct thread_input *input, unsigned int msg, lparam_t wparam )
{
    const input_shm_t *input_shm = input->shared;
    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        update_key_state( shared->keystate, msg, wparam, 0 );
    }
    SHARED_WRITE_END;
}

static void update_desktop_key_state( struct desktop *desktop, unsigned int msg, lparam_t wparam )
{
    SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
    {
        update_key_state( shared->keystate, msg, wparam, 1 );
    }
    SHARED_WRITE_END;
}

/* release the hardware message currently being processed by the given thread */
static void release_hardware_message( struct msg_queue *queue, unsigned int hw_id )
{
    struct thread_input *input = queue->input;
    struct message *msg, *other;
    int clr_bit;

    LIST_FOR_EACH_ENTRY( msg, &input->msg_list, struct message, entry )
    {
        if (msg->unique_id == hw_id) break;
    }
    if (&msg->entry == &input->msg_list) return;  /* not found */

    /* clear the queue bit for that message */
    clr_bit = get_hardware_msg_bit( msg->msg );
    LIST_FOR_EACH_ENTRY( other, &input->msg_list, struct message, entry )
    {
        if (other != msg && get_hardware_msg_bit( other->msg ) == clr_bit)
        {
            clr_bit = 0;
            break;
        }
    }
    if (clr_bit) clear_queue_bits( queue, clr_bit );

    update_thread_input_key_state( input, msg->msg, msg->wparam );
    list_remove( &msg->entry );
    free_message( msg );
}

static int queue_hotkey_message( struct desktop *desktop, struct message *msg )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct hotkey *hotkey;
    unsigned int modifiers = 0;

    if (msg->msg != WM_KEYDOWN && msg->msg != WM_SYSKEYDOWN) return 0;

    if (desktop_shm->keystate[VK_MENU] & 0x80) modifiers |= MOD_ALT;
    if (desktop_shm->keystate[VK_CONTROL] & 0x80) modifiers |= MOD_CONTROL;
    if (desktop_shm->keystate[VK_SHIFT] & 0x80) modifiers |= MOD_SHIFT;
    if ((desktop_shm->keystate[VK_LWIN] & 0x80) || (desktop_shm->keystate[VK_RWIN] & 0x80)) modifiers |= MOD_WIN;

    LIST_FOR_EACH_ENTRY( hotkey, &desktop->hotkeys, struct hotkey, entry )
    {
        if (hotkey->vkey != msg->wparam) continue;
        if ((hotkey->flags & (MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN)) == modifiers) goto found;
    }

    return 0;

found:
    msg->type      = MSG_POSTED;
    msg->win       = hotkey->win;
    msg->msg       = WM_HOTKEY;
    msg->wparam    = hotkey->id;
    msg->lparam    = ((hotkey->vkey & 0xffff) << 16) | modifiers;

    free( msg->data );
    msg->data      = NULL;
    msg->data_size = 0;

    list_add_tail( &hotkey->queue->msg_list[POST_MESSAGE], &msg->entry );
    set_queue_bits( hotkey->queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE|QS_HOTKEY );
    hotkey->queue->hotkey_count++;
    return 1;
}

/* find the window that should receive a given hardware message */
static user_handle_t find_hardware_message_window( struct desktop *desktop, struct thread_input *input,
                                                   struct message *msg, unsigned int *msg_code,
                                                   struct thread **thread )
{
    const input_shm_t *input_shm = input ? input->shared : NULL;
    user_handle_t win = 0;

    *thread = NULL;
    *msg_code = msg->msg;
    switch (get_hardware_msg_bit( msg->msg ))
    {
    case QS_POINTER:
    case QS_RAWINPUT:
        if (!(win = msg->win) && input) win = input_shm->focus;
        break;
    case QS_KEY:
        if (input && !(win = input_shm->focus))
        {
            win = input_shm->active;
            if (*msg_code < WM_SYSKEYDOWN) *msg_code += WM_SYSKEYDOWN - WM_KEYDOWN;
        }
        break;
    case QS_MOUSEMOVE:
    case QS_MOUSEBUTTON:
        if (!input || !(win = input_shm->capture))
        {
            if (is_window_visible( msg->win ) && !is_window_transparent( msg->win )) win = msg->win;
            else win = shallow_window_from_point( desktop, msg->x, msg->y );
            *thread = window_thread_from_point( win, msg->x, msg->y );
        }
        break;
    }

    if (!*thread)
        *thread = get_window_thread( win );
    return win;
}

static struct rawinput_device *find_rawinput_device( struct process *process, unsigned int usage )
{
    struct rawinput_device *device, *end;

    for (device = process->rawinput_devices, end = device + process->rawinput_device_count; device != end; device++)
    {
        if (device->usage != usage) continue;
        return device;
    }

    return NULL;
}

static void prepend_cursor_history( int x, int y, unsigned int time, lparam_t info )
{
    struct cursor_pos *pos = &cursor_history[--cursor_history_latest % ARRAY_SIZE(cursor_history)];

    pos->x = x;
    pos->y = y;
    pos->time = time;
    pos->info = info;
}

static unsigned int get_rawinput_device_flags( struct process *process, struct message *msg )
{
    switch (get_hardware_msg_bit( msg->msg ))
    {
    case QS_KEY:
        return process->rawinput_kbd ? process->rawinput_kbd->flags : 0;
    case QS_MOUSEMOVE:
    case QS_MOUSEBUTTON:
        return process->rawinput_mouse ? process->rawinput_mouse->flags : 0;
    }
    return 0;
}

/* queue a hardware message into a given thread input */
static void queue_hardware_message( struct desktop *desktop, struct message *msg, int always_queue )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    user_handle_t win;
    struct thread *thread;
    struct thread_input *input;
    struct hardware_msg_data *msg_data = msg->data;
    unsigned int msg_code;
    int flags;

    update_desktop_key_state( desktop, msg->msg, msg->wparam );
    last_input_time = get_tick_count();
    if (msg->msg != WM_MOUSEMOVE) always_queue = 1;

    switch (get_hardware_msg_bit( msg->msg ))
    {
    case QS_KEY:
        if (queue_hotkey_message( desktop, msg )) return;
        if (desktop_shm->keystate[VK_MENU] & 0x80) msg->lparam |= KF_ALTDOWN << 16;
        if (msg->wparam == VK_SHIFT || msg->wparam == VK_LSHIFT || msg->wparam == VK_RSHIFT)
            msg->lparam &= ~(KF_EXTENDED << 16);
        break;
    case QS_MOUSEMOVE:
        prepend_cursor_history( msg->x, msg->y, msg->time, msg_data->info );
        /* fallthrough */
    case QS_MOUSEBUTTON:
        if (update_desktop_cursor_pos( desktop, msg->win, msg->x, msg->y )) always_queue = 1;
        if (desktop_shm->keystate[VK_LBUTTON] & 0x80)  msg->wparam |= MK_LBUTTON;
        if (desktop_shm->keystate[VK_MBUTTON] & 0x80)  msg->wparam |= MK_MBUTTON;
        if (desktop_shm->keystate[VK_RBUTTON] & 0x80)  msg->wparam |= MK_RBUTTON;
        if (desktop_shm->keystate[VK_SHIFT] & 0x80)    msg->wparam |= MK_SHIFT;
        if (desktop_shm->keystate[VK_CONTROL] & 0x80)  msg->wparam |= MK_CONTROL;
        if (desktop_shm->keystate[VK_XBUTTON1] & 0x80) msg->wparam |= MK_XBUTTON1;
        if (desktop_shm->keystate[VK_XBUTTON2] & 0x80) msg->wparam |= MK_XBUTTON2;
        break;
    }
    msg->x = desktop_shm->cursor.x;
    msg->y = desktop_shm->cursor.y;

    if (msg->win && (thread = get_window_thread( msg->win )))
    {
        input = thread->queue->input;
        release_object( thread );
    }
    else input = desktop->foreground_input;

    win = find_hardware_message_window( desktop, input, msg, &msg_code, &thread );
    flags = thread ? get_rawinput_device_flags( thread->process, msg ) : 0;
    if (!win || !thread || (flags & RIDEV_NOLEGACY))
    {
        if (input && !(flags & RIDEV_NOLEGACY)) update_thread_input_key_state( input, msg->msg, msg->wparam );
        free_message( msg );
        if (thread) release_object( thread );
        return;
    }

    input = thread->queue->input;

    if (win != msg->win) always_queue = 1;
    if (!always_queue || merge_message( input, msg )) free_message( msg );
    else
    {
        msg->unique_id = 0;  /* will be set once we return it to the app */
        list_add_tail( &input->msg_list, &msg->entry );
        set_queue_bits( thread->queue, get_hardware_msg_bit( msg->msg ) );
    }
    release_object( thread );
}

/* send the low-level hook message for a given hardware message */
static int send_hook_ll_message( struct desktop *desktop, struct message *hardware_msg,
                                 int id, lparam_t lparam, struct msg_queue *sender )
{
    struct thread *hook_thread;
    struct msg_queue *queue;
    struct message *msg;
    timeout_t timeout = 2000 * -10000;  /* FIXME: load from registry */

    if (!(hook_thread = get_first_global_hook( desktop, id ))) return 0;
    if (!(queue = hook_thread->queue)) return 0;
    if (is_queue_hung( queue )) return 0;

    if (!(msg = mem_alloc( sizeof(*msg) ))) return 0;

    msg->type      = MSG_HOOK_LL;
    msg->win       = 0;
    msg->msg       = id;
    msg->wparam    = hardware_msg->msg;
    msg->lparam    = lparam;
    msg->x         = hardware_msg->x;
    msg->y         = hardware_msg->y;
    msg->time      = hardware_msg->time;
    msg->data_size = hardware_msg->data_size;
    msg->result    = NULL;

    if (!(msg->data = memdup( hardware_msg->data, hardware_msg->data_size )) ||
        !(msg->result = alloc_message_result( sender, queue, msg, timeout )))
    {
        free_message( msg );
        return 0;
    }
    msg->result->hardware_msg = hardware_msg;
    msg->result->desktop = (struct desktop *)grab_object( desktop );
    list_add_tail( &queue->msg_list[SEND_MESSAGE], &msg->entry );
    set_queue_bits( queue, QS_SENDMESSAGE );
    return 1;
}

/* get the foreground thread for a desktop and a window receiving input */
static struct thread *get_foreground_thread( struct desktop *desktop, user_handle_t window )
{
    /* if desktop has no foreground process, assume the receiving window is */
    if (desktop->foreground_input)
    {
        const input_shm_t *input_shm = desktop->foreground_input->shared;
        return get_window_thread( input_shm->focus );
    }
    if (window) return get_window_thread( window );
    return NULL;
}

/* user32 reserves 1 & 2 for winemouse and winekeyboard,
 * keep this in sync with user_private.h */
#define WINE_MOUSE_HANDLE 1
#define WINE_KEYBOARD_HANDLE 2

static void rawmouse_init( struct rawinput *header, RAWMOUSE *rawmouse, int x, int y, unsigned int flags,
                           unsigned int buttons, lparam_t info )
{
    static const unsigned int button_flags[] =
    {
        0,                              /* MOUSEEVENTF_MOVE */
        RI_MOUSE_LEFT_BUTTON_DOWN,      /* MOUSEEVENTF_LEFTDOWN */
        RI_MOUSE_LEFT_BUTTON_UP,        /* MOUSEEVENTF_LEFTUP */
        RI_MOUSE_RIGHT_BUTTON_DOWN,     /* MOUSEEVENTF_RIGHTDOWN */
        RI_MOUSE_RIGHT_BUTTON_UP,       /* MOUSEEVENTF_RIGHTUP */
        RI_MOUSE_MIDDLE_BUTTON_DOWN,    /* MOUSEEVENTF_MIDDLEDOWN */
        RI_MOUSE_MIDDLE_BUTTON_UP,      /* MOUSEEVENTF_MIDDLEUP */
    };
    unsigned int i;

    header->type   = RIM_TYPEMOUSE;
    header->device = WINE_MOUSE_HANDLE;
    header->wparam = 0;
    header->usage  = MAKELONG(HID_USAGE_GENERIC_MOUSE, HID_USAGE_PAGE_GENERIC);

    rawmouse->usFlags       = MOUSE_MOVE_RELATIVE;
    rawmouse->usButtonFlags = 0;
    rawmouse->usButtonData  = 0;
    for (i = 1; i < ARRAY_SIZE(button_flags); ++i)
    {
        if (flags & (1 << i)) rawmouse->usButtonFlags |= button_flags[i];
    }
    if (flags & MOUSEEVENTF_WHEEL)
    {
        rawmouse->usButtonFlags |= RI_MOUSE_WHEEL;
        rawmouse->usButtonData   = buttons;
    }
    if (flags & MOUSEEVENTF_HWHEEL)
    {
        rawmouse->usButtonFlags |= RI_MOUSE_HORIZONTAL_WHEEL;
        rawmouse->usButtonData   = buttons;
    }
    if (flags & MOUSEEVENTF_XDOWN)
    {
        if (buttons == XBUTTON1) rawmouse->usButtonFlags |= RI_MOUSE_BUTTON_4_DOWN;
        if (buttons == XBUTTON2) rawmouse->usButtonFlags |= RI_MOUSE_BUTTON_5_DOWN;
    }
    if (flags & MOUSEEVENTF_XUP)
    {
        if (buttons == XBUTTON1) rawmouse->usButtonFlags |= RI_MOUSE_BUTTON_4_UP;
        if (buttons == XBUTTON2) rawmouse->usButtonFlags |= RI_MOUSE_BUTTON_5_UP;
    }

    rawmouse->ulRawButtons       = 0;
    rawmouse->lLastX             = x;
    rawmouse->lLastY             = y;
    rawmouse->ulExtraInformation = info;
}

static void rawkeyboard_init( struct rawinput *rawinput, RAWKEYBOARD *keyboard, unsigned short scan, unsigned short vkey,
                              unsigned int flags, unsigned int message, lparam_t info )
{
    rawinput->type   = RIM_TYPEKEYBOARD;
    rawinput->device = WINE_KEYBOARD_HANDLE;
    rawinput->wparam = 0;
    rawinput->usage  = MAKELONG(HID_USAGE_GENERIC_KEYBOARD, HID_USAGE_PAGE_GENERIC);

    keyboard->MakeCode = scan;
    keyboard->Flags    = (flags & KEYEVENTF_KEYUP) ? RI_KEY_BREAK : RI_KEY_MAKE;
    if (flags & KEYEVENTF_EXTENDEDKEY) keyboard->Flags |= RI_KEY_E0;
    keyboard->Reserved = 0;

    switch (vkey)
    {
    case VK_LSHIFT:
    case VK_RSHIFT:
        keyboard->VKey   = VK_SHIFT;
        keyboard->Flags &= ~RI_KEY_E0;
        break;

    case VK_LCONTROL:
    case VK_RCONTROL:
        keyboard->VKey = VK_CONTROL;
        break;
    case VK_LMENU:
    case VK_RMENU:
        keyboard->VKey = VK_MENU;
        break;

    default:
        keyboard->VKey = vkey;
        break;
    }

    keyboard->Message          = message;
    keyboard->ExtraInformation = info;
}

static void rawhid_init( struct rawinput *rawinput, RAWHID *hid, const union hw_input *input )
{
    rawinput->type   = RIM_TYPEHID;
    rawinput->device = input->hw.hid.device;
    rawinput->wparam = input->hw.wparam;
    rawinput->usage  = input->hw.hid.usage;

    hid->dwCount   = input->hw.hid.count;
    hid->dwSizeHid = input->hw.hid.length;
}

struct rawinput_message
{
    struct thread           *foreground;
    struct hw_msg_source     source;
    unsigned int             time;
    unsigned int             message;
    unsigned int             flags;
    struct rawinput          rawinput;
    union
    {
        RAWKEYBOARD         keyboard;
        RAWMOUSE            mouse;
        RAWHID              hid;
    } data;
    const void             *hid_report;
};

/* check if process is supposed to receive a WM_INPUT message and eventually queue it */
static void queue_rawinput_message( struct desktop *desktop, struct process *process, void *args )
{
    const struct rawinput_message *raw_msg = args;
    const struct rawinput_device *device;
    struct hardware_msg_data *msg_data;
    struct message *msg;
    data_size_t report_size = 0, data_size = 0;
    int wparam = RIM_INPUT;
    lparam_t info = 0;
    char *ptr;

    if (raw_msg->rawinput.type == RIM_TYPEMOUSE)
    {
        device = process->rawinput_mouse;
        data_size = sizeof(raw_msg->data.mouse);
        info = raw_msg->data.mouse.ulExtraInformation;
    }
    else if (raw_msg->rawinput.type == RIM_TYPEKEYBOARD)
    {
        device = process->rawinput_kbd;
        data_size = sizeof(raw_msg->data.keyboard);
        info = raw_msg->data.keyboard.ExtraInformation;
    }
    else
    {
        device = find_rawinput_device( process, raw_msg->rawinput.usage );
        data_size = offsetof(RAWHID, bRawData[0]);
        report_size = raw_msg->data.hid.dwCount * raw_msg->data.hid.dwSizeHid;
    }
    if (!device) return;

    if (raw_msg->message == WM_INPUT_DEVICE_CHANGE && !(device->flags & RIDEV_DEVNOTIFY)) return;
    if (process != raw_msg->foreground->process)
    {
        if (raw_msg->message == WM_INPUT && !(device->flags & RIDEV_INPUTSINK)) return;
        wparam = RIM_INPUTSINK;
    }

    if (!(msg = alloc_hardware_message( info, raw_msg->source, raw_msg->time, data_size + report_size ))) return;
    msg->win    = device->target;
    msg->msg    = raw_msg->message;
    msg->wparam = wparam;
    msg->lparam = 0;

    msg_data = msg->data;
    msg_data->flags = raw_msg->flags;
    msg_data->rawinput = raw_msg->rawinput;
    ptr = mem_append( msg_data + 1, &raw_msg->data, data_size );
    mem_append( ptr, raw_msg->hid_report, report_size );

    if (raw_msg->message == WM_INPUT_DEVICE_CHANGE && raw_msg->rawinput.type == RIM_TYPEHID)
    {
        msg->wparam = raw_msg->rawinput.wparam;
        msg->lparam = raw_msg->rawinput.device;
    }

    queue_hardware_message( desktop, msg, 1 );
}

static void dispatch_rawinput_message( struct desktop *desktop, struct rawinput_message *raw_msg )
{
    struct process *process;

    LIST_FOR_EACH_ENTRY( process, &rawinput_processes, struct process, rawinput_entry )
        queue_rawinput_message( desktop, process, raw_msg );
}

/* queue a hardware message for a mouse event */
static int queue_mouse_message( struct desktop *desktop, user_handle_t win, const union hw_input *input,
                                unsigned int origin, struct msg_queue *sender )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct hardware_msg_data *msg_data;
    struct rawinput_message raw_msg;
    struct message *msg;
    struct thread *foreground;
    unsigned int i, time = get_tick_count(), flags;
    struct hw_msg_source source = { IMDT_MOUSE, origin };
    lparam_t wparam = input->mouse.data << 16;
    int wait = 0, x, y;

    static const unsigned int messages[] =
    {
        WM_MOUSEMOVE,    /* 0x0001 = MOUSEEVENTF_MOVE */
        WM_LBUTTONDOWN,  /* 0x0002 = MOUSEEVENTF_LEFTDOWN */
        WM_LBUTTONUP,    /* 0x0004 = MOUSEEVENTF_LEFTUP */
        WM_RBUTTONDOWN,  /* 0x0008 = MOUSEEVENTF_RIGHTDOWN */
        WM_RBUTTONUP,    /* 0x0010 = MOUSEEVENTF_RIGHTUP */
        WM_MBUTTONDOWN,  /* 0x0020 = MOUSEEVENTF_MIDDLEDOWN */
        WM_MBUTTONUP,    /* 0x0040 = MOUSEEVENTF_MIDDLEUP */
        WM_XBUTTONDOWN,  /* 0x0080 = MOUSEEVENTF_XDOWN */
        WM_XBUTTONUP,    /* 0x0100 = MOUSEEVENTF_XUP */
        0,               /* 0x0200 = unused */
        0,               /* 0x0400 = unused */
        WM_MOUSEWHEEL,   /* 0x0800 = MOUSEEVENTF_WHEEL */
        WM_MOUSEHWHEEL   /* 0x1000 = MOUSEEVENTF_HWHEEL */
    };

    SHARED_WRITE_BEGIN( desktop_shm, desktop_shm_t )
    {
        shared->cursor.last_change = time;
    }
    SHARED_WRITE_END;

    flags = input->mouse.flags;
    time  = input->mouse.time;
    if (!time) time = desktop_shm->cursor.last_change;

    if (flags & MOUSEEVENTF_MOVE)
    {
        if (flags & MOUSEEVENTF_ABSOLUTE)
        {
            x = input->mouse.x;
            y = input->mouse.y;
        }
        else
        {
            x = desktop_shm->cursor.x + input->mouse.x;
            y = desktop_shm->cursor.y + input->mouse.y;
        }
        if (x == desktop_shm->cursor.x && y == desktop_shm->cursor.y)
            flags &= ~MOUSEEVENTF_MOVE;
    }
    else
    {
        x = desktop_shm->cursor.x;
        y = desktop_shm->cursor.y;
    }

    if ((foreground = get_foreground_thread( desktop, win )))
    {
        memset( &raw_msg, 0, sizeof(raw_msg) );
        raw_msg.foreground = foreground;
        raw_msg.source     = source;
        raw_msg.time       = time;
        raw_msg.message    = WM_INPUT;
        raw_msg.flags      = flags;
        rawmouse_init( &raw_msg.rawinput, &raw_msg.data.mouse, x - desktop_shm->cursor.x, y - desktop_shm->cursor.y,
                       raw_msg.flags, input->mouse.data, input->mouse.info );

        dispatch_rawinput_message( desktop, &raw_msg );
        release_object( foreground );
    }

    for (i = 0; i < ARRAY_SIZE( messages ); i++)
    {
        if (!messages[i]) continue;
        if (!(flags & (1 << i))) continue;
        flags &= ~(1 << i);

        if (!(msg = alloc_hardware_message( input->mouse.info, source, time, 0 ))) return 0;
        msg_data = msg->data;

        msg->win       = get_user_full_handle( win );
        msg->msg       = messages[i];
        msg->wparam    = wparam;
        msg->lparam    = 0;
        msg->x         = x;
        msg->y         = y;
        if (origin == IMO_INJECTED) msg_data->flags = LLMHF_INJECTED;

        /* specify a sender only when sending the last message */
        if (!(flags & ((1 << ARRAY_SIZE( messages )) - 1)))
        {
            if (!(wait = send_hook_ll_message( desktop, msg, WH_MOUSE_LL, wparam, sender )))
                queue_hardware_message( desktop, msg, 0 );
        }
        else if (!send_hook_ll_message( desktop, msg, WH_MOUSE_LL, wparam, NULL ))
            queue_hardware_message( desktop, msg, 0 );
    }
    return wait;
}

static int queue_keyboard_message( struct desktop *desktop, user_handle_t win, const union hw_input *input,
                                   unsigned int origin, struct msg_queue *sender, int repeat );

static void key_repeat_timeout( void *private )
{
    struct desktop *desktop = private;

    desktop->key_repeat.timeout = NULL;
    queue_keyboard_message( desktop, desktop->key_repeat.win, &desktop->key_repeat.input, IMO_HARDWARE, NULL, 1 );
}

static void stop_key_repeat( struct desktop *desktop )
{
    if (desktop->key_repeat.timeout) remove_timeout_user( desktop->key_repeat.timeout );
    desktop->key_repeat.timeout = NULL;
    desktop->key_repeat.win = 0;
    memset( &desktop->key_repeat.input, 0, sizeof(desktop->key_repeat.input) );
}

/* queue a hardware message for a keyboard event */
static int queue_keyboard_message( struct desktop *desktop, user_handle_t win, const union hw_input *input,
                                   unsigned int origin, struct msg_queue *sender, int repeat )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct hw_msg_source source = { IMDT_KEYBOARD, origin };
    struct hardware_msg_data *msg_data;
    struct message *msg;
    struct thread *foreground;
    unsigned char vkey = input->kbd.vkey, hook_vkey = vkey;
    unsigned int message_code, time;
    lparam_t lparam = input->kbd.scan << 16;
    unsigned int flags = 0;
    BOOL unicode = input->kbd.flags & KEYEVENTF_UNICODE;
    int wait;

    if (!(time = input->kbd.time)) time = get_tick_count();

    switch (vkey)
    {
    case 0:
        if (unicode) vkey = hook_vkey = VK_PACKET;
        break;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
        vkey = (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) ? VK_RMENU : VK_LMENU;
        if ((input->kbd.vkey & 0xff) == VK_MENU) hook_vkey = vkey;
        break;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
        vkey = (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) ? VK_RCONTROL : VK_LCONTROL;
        if ((input->kbd.vkey & 0xff) == VK_CONTROL) hook_vkey = vkey;
        break;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        vkey = (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) ? VK_RSHIFT : VK_LSHIFT;
        if ((input->kbd.vkey & 0xff) == VK_SHIFT) hook_vkey = vkey;
        break;
    }

    message_code = (input->kbd.flags & KEYEVENTF_KEYUP) ? WM_KEYUP : WM_KEYDOWN;
    switch (vkey)
    {
    case VK_LMENU:
    case VK_RMENU:
        if (input->kbd.flags & KEYEVENTF_KEYUP)
        {
            /* send WM_SYSKEYUP if Alt still pressed and no other key in between */
            if (!(desktop_shm->keystate[VK_MENU] & 0x80) || !desktop->alt_pressed) break;
            message_code = WM_SYSKEYUP;
            desktop->alt_pressed = 0;
        }
        else
        {
            /* send WM_SYSKEYDOWN for Alt except with Ctrl */
            if (desktop_shm->keystate[VK_CONTROL] & 0x80) break;
            message_code = WM_SYSKEYDOWN;
            desktop->alt_pressed = 1;
        }
        break;

    case VK_LCONTROL:
    case VK_RCONTROL:
        /* send WM_SYSKEYUP on release if Alt still pressed */
        if (!(input->kbd.flags & KEYEVENTF_KEYUP)) break;
        if (!(desktop_shm->keystate[VK_MENU] & 0x80)) break;
        message_code = WM_SYSKEYUP;
        desktop->alt_pressed = 0;
        break;

    default:
        /* send WM_SYSKEY for Alt-anykey and for F10 */
        if (desktop_shm->keystate[VK_CONTROL] & 0x80) break;
        if (!(desktop_shm->keystate[VK_MENU] & 0x80)) break;
        /* fall through */
    case VK_F10:
        message_code = (input->kbd.flags & KEYEVENTF_KEYUP) ? WM_SYSKEYUP : WM_SYSKEYDOWN;
        desktop->alt_pressed = 0;
        break;
    }

    /* send numpad vkeys if NumLock is active */
    if ((input->kbd.vkey & KBDNUMPAD) && (desktop->keystate[VK_NUMLOCK] & 0x01) &&
        !(desktop->keystate[VK_SHIFT] & 0x80))
    {
       switch (vkey)
       {
       case VK_INSERT: hook_vkey = vkey = VK_NUMPAD0; break;
       case VK_END:    hook_vkey = vkey = VK_NUMPAD1; break;
       case VK_DOWN:   hook_vkey = vkey = VK_NUMPAD2; break;
       case VK_NEXT:   hook_vkey = vkey = VK_NUMPAD3; break;
       case VK_LEFT:   hook_vkey = vkey = VK_NUMPAD4; break;
       case VK_CLEAR:  hook_vkey = vkey = VK_NUMPAD5; break;
       case VK_RIGHT:  hook_vkey = vkey = VK_NUMPAD6; break;
       case VK_HOME:   hook_vkey = vkey = VK_NUMPAD7; break;
       case VK_UP:     hook_vkey = vkey = VK_NUMPAD8; break;
       case VK_PRIOR:  hook_vkey = vkey = VK_NUMPAD9; break;
       case VK_DELETE: hook_vkey = vkey = VK_DECIMAL; break;
       default: break;
       }
    }

    if (origin == IMO_HARDWARE)
    {
        /* if the repeat key is released, stop auto-repeating */
        if (((input->kbd.flags & KEYEVENTF_KEYUP) &&
             (input->kbd.scan == desktop->key_repeat.input.kbd.scan)))
        {
            stop_key_repeat( desktop );
        }
        /* if a key is down, start or continue auto-repeating */
        if (!(input->kbd.flags & KEYEVENTF_KEYUP) && desktop->key_repeat.enable)
        {
            timeout_t timeout = repeat ? desktop->key_repeat.period : desktop->key_repeat.delay;
            desktop->key_repeat.input = *input;
            desktop->key_repeat.input.kbd.time = 0;
            desktop->key_repeat.win = win;
            if (desktop->key_repeat.timeout) remove_timeout_user( desktop->key_repeat.timeout );
            desktop->key_repeat.timeout = add_timeout_user( timeout, key_repeat_timeout, desktop );
        }
    }

    if (!unicode && (foreground = get_foreground_thread( desktop, win )))
    {
        struct rawinput_message raw_msg = {0};
        raw_msg.foreground = foreground;
        raw_msg.source     = source;
        raw_msg.time       = time;
        raw_msg.message    = WM_INPUT;
        raw_msg.flags      = input->kbd.flags;
        rawkeyboard_init( &raw_msg.rawinput, &raw_msg.data.keyboard, input->kbd.scan, vkey,
                          raw_msg.flags, message_code, input->kbd.info );

        dispatch_rawinput_message( desktop, &raw_msg );
        release_object( foreground );
    }

    if (!(msg = alloc_hardware_message( input->kbd.info, source, time, 0 ))) return 0;
    msg_data = msg->data;

    msg->win       = get_user_full_handle( win );
    msg->msg       = message_code;
    if (origin == IMO_INJECTED) msg_data->flags = LLKHF_INJECTED;

    if (!unicode || input->kbd.vkey)
    {
        if (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) flags |= KF_EXTENDED;
        /* FIXME: set KF_DLGMODE and KF_MENUMODE when needed */
        if (input->kbd.flags & KEYEVENTF_KEYUP) flags |= KF_REPEAT | KF_UP;
        else if (desktop_shm->keystate[vkey] & 0x80) flags |= KF_REPEAT;

        lparam &= 0xff0000; /* mask off scan code high bits for non-unicode input */
        msg_data->flags |= (flags & (KF_EXTENDED | KF_ALTDOWN | KF_UP)) >> 8;
    }

    msg->wparam = vkey;
    msg->lparam = (flags << 16) | lparam | 1u /* repeat count */;

    if (!(wait = send_hook_ll_message( desktop, msg, WH_KEYBOARD_LL, lparam | hook_vkey, sender )))
        queue_hardware_message( desktop, msg, 1 );

    return wait;
}

struct pointer
{
    struct list entry;
    struct timeout_user *timeout;
    struct desktop *desktop;
    user_handle_t win;
    int primary;
    union hw_input input;
};

static void queue_pointer_message( struct pointer *pointer, int repeated );

static void pointer_message_timeout( void *private )
{
    struct pointer *pointer = private;
    queue_pointer_message( pointer, 1 );
}

static void queue_pointer_message( struct pointer *pointer, int repeated )
{
    static const unsigned int messages[][2] =
    {
        {WM_POINTERUPDATE, 0},
        {WM_POINTERENTER, WM_POINTERDOWN},
        {WM_POINTERUP, WM_POINTERLEAVE},
    };
    struct hw_msg_source source = { IMDT_UNAVAILABLE, IMDT_TOUCH };
    struct desktop *desktop = pointer->desktop;
    const desktop_shm_t *desktop_shm = desktop->shared;
    const union hw_input *input = &pointer->input;
    unsigned int i, wparam = input->hw.wparam;
    timeout_t time = get_tick_count();
    user_handle_t win = pointer->win;
    struct rectangle top_rect;
    struct message *msg;
    int x, y;

    get_virtual_screen_rect( desktop, &top_rect, 0 );
    x = LOWORD(input->hw.lparam) * (top_rect.right - top_rect.left) / 65535;
    y = HIWORD(input->hw.lparam) * (top_rect.bottom - top_rect.top) / 65535;

    if (pointer->primary) wparam |= POINTER_MESSAGE_FLAG_PRIMARY << 16;

    for (i = 0; i < 2 && messages[input->hw.msg - WM_POINTERUPDATE][i]; i++)
    {
        if (!(msg = alloc_hardware_message( 0, source, time, 0 ))) return;

        msg->win       = get_user_full_handle( win );
        msg->msg       = messages[input->hw.msg - WM_POINTERUPDATE][i];
        msg->wparam    = wparam;
        msg->lparam    = MAKELONG(x, y);
        msg->x         = desktop_shm->cursor.x;
        msg->y         = desktop_shm->cursor.y;

        queue_hardware_message( desktop, msg, 1 );
    }

    if (!repeated && pointer->primary && (msg = alloc_hardware_message( 0xff515700, source, time, 0 )))
    {
        unsigned int message = WM_MOUSEMOVE;
        if (input->hw.msg == WM_POINTERDOWN) message = WM_LBUTTONDOWN;
        else if (input->hw.msg == WM_POINTERUP) message = WM_LBUTTONUP;

        msg->win       = get_user_full_handle( win );
        msg->msg       = message;
        msg->wparam    = 0;
        msg->lparam    = 0;
        msg->x         = x;
        msg->y         = y;

        if (!send_hook_ll_message( desktop, msg, WH_MOUSE_LL, 0, NULL ))
            queue_hardware_message( desktop, msg, 0 );
    }

    if (input->hw.msg != WM_POINTERUP)
    {
        pointer->input.hw.msg = WM_POINTERUPDATE;
        pointer->input.hw.wparam &= ~(POINTER_MESSAGE_FLAG_NEW << 16);
        pointer->timeout = add_timeout_user( -160000, pointer_message_timeout, pointer );
    }
    else
    {
        list_remove( &pointer->entry );
        free( pointer );
    }
}

static struct pointer *find_pointer_from_id( struct desktop *desktop, unsigned int id )
{
    struct pointer *pointer;

    LIST_FOR_EACH_ENTRY( pointer, &desktop->pointers, struct pointer, entry )
        if (LOWORD(pointer->input.hw.wparam) == id) return pointer;

    pointer = mem_alloc( sizeof(struct pointer) );
    pointer->timeout = NULL;
    pointer->desktop = desktop;
    pointer->primary = list_empty( &desktop->pointers );
    list_add_tail( &desktop->pointers, &pointer->entry );

    return pointer;
}

/* queue a hardware message for a custom type of event */
static void queue_custom_hardware_message( struct desktop *desktop, user_handle_t win,
                                           unsigned int origin, const union hw_input *input )
{
    const desktop_shm_t *desktop_shm = desktop->shared;
    struct hw_msg_source source = { IMDT_UNAVAILABLE, origin };
    struct thread *foreground;
    struct pointer *pointer;
    struct message *msg;

    switch (input->hw.msg)
    {
    case WM_INPUT:
    case WM_INPUT_DEVICE_CHANGE:
        if (input->hw.hid.length * input->hw.hid.count != get_req_data_size())
            set_error( STATUS_INVALID_PARAMETER );
        else if ((foreground = get_foreground_thread( desktop, win )))
        {
            struct rawinput_message raw_msg = {0};
            raw_msg.foreground = foreground;
            raw_msg.source     = source;
            raw_msg.time       = get_tick_count();
            raw_msg.message    = input->hw.msg;
            raw_msg.hid_report = get_req_data();
            rawhid_init( &raw_msg.rawinput, &raw_msg.data.hid, input );

            dispatch_rawinput_message( desktop, &raw_msg );
            release_object( foreground );
        }
        return;
    }

    if (input->hw.msg == WM_POINTERDOWN || input->hw.msg == WM_POINTERUP || input->hw.msg == WM_POINTERUPDATE)
    {
        pointer = find_pointer_from_id( desktop, LOWORD(input->hw.wparam) );
        if (pointer->timeout) remove_timeout_user( pointer->timeout );
        pointer->input = *input;
        pointer->win = win;

        queue_pointer_message( pointer, 0 );
        return;
    }

    if (!(msg = alloc_hardware_message( 0, source, get_tick_count(), 0 ))) return;

    msg->win       = get_user_full_handle( win );
    msg->msg       = input->hw.msg;
    msg->wparam    = input->hw.wparam;
    msg->lparam    = input->hw.lparam;
    msg->x         = desktop_shm->cursor.x;
    msg->y         = desktop_shm->cursor.y;

    queue_hardware_message( desktop, msg, 1 );
}

/* check message filter for a hardware message */
static int check_hw_message_filter( user_handle_t win, unsigned int msg_code,
                                    user_handle_t filter_win, unsigned int first, unsigned int last )
{
    switch (get_hardware_msg_bit( msg_code ))
    {
    case QS_KEY:
        /* we can only test the window for a keyboard message since the
         * dest window for a mouse message depends on hittest */
        if (filter_win && win != filter_win && !is_child_window( filter_win, win ))
            return 0;
        /* the message code is final for a keyboard message, we can simply check it */
        return check_msg_filter( msg_code, first, last );

    case QS_MOUSEMOVE:
    case QS_MOUSEBUTTON:
        /* we need to check all possible values that the message can have in the end */
        if (check_msg_filter( msg_code, first, last )) return 1;
        if (msg_code == WM_MOUSEWHEEL) return 0;  /* no other possible value for this one */

        /* all other messages can become non-client messages */
        if (check_msg_filter( msg_code + (WM_NCMOUSEFIRST - WM_MOUSEFIRST), first, last )) return 1;

        /* clicks can become double-clicks or non-client double-clicks */
        if (msg_code == WM_LBUTTONDOWN || msg_code == WM_MBUTTONDOWN ||
            msg_code == WM_RBUTTONDOWN || msg_code == WM_XBUTTONDOWN)
        {
            if (check_msg_filter( msg_code + (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN), first, last )) return 1;
            if (check_msg_filter( msg_code + (WM_NCLBUTTONDBLCLK - WM_LBUTTONDOWN), first, last )) return 1;
        }
        return 0;

    default:
        return check_msg_filter( msg_code, first, last );
    }
}

/* is this message an internal driver notification message */
static inline BOOL is_internal_hardware_message( unsigned int message )
{
    return (message >= WM_WINE_FIRST_DRIVER_MSG && message <= WM_WINE_LAST_DRIVER_MSG);
}

/* find a hardware message for the given queue */
static int get_hardware_message( struct thread *thread, unsigned int hw_id, user_handle_t filter_win,
                                 unsigned int first, unsigned int last, unsigned int flags,
                                 struct get_message_reply *reply )
{
    struct thread_input *input = thread->queue->input;
    const struct rawinput_device *device;
    struct thread *win_thread;
    struct list *ptr;
    user_handle_t win;
    int clear_bits, got_one = 0;
    unsigned int msg_code;
    int no_legacy;

    no_legacy = (device = thread->process->rawinput_mouse) && (device->flags & RIDEV_NOLEGACY);

    ptr = list_head( &input->msg_list );
    if (hw_id)
    {
        while (ptr)
        {
            struct message *msg = LIST_ENTRY( ptr, struct message, entry );
            if (msg->unique_id == hw_id) break;
            ptr = list_next( &input->msg_list, ptr );
        }
        if (!ptr) ptr = list_head( &input->msg_list );
        else ptr = list_next( &input->msg_list, ptr );  /* start from the next one */
    }

    if (ptr == list_head( &input->msg_list ))
        clear_bits = QS_INPUT;
    else
        clear_bits = 0;  /* don't clear bits if we don't go through the whole list */

    while (ptr)
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        struct hardware_msg_data *data = msg->data;

        ptr = list_next( &input->msg_list, ptr );
        if (no_legacy && msg->msg == WM_MOUSEMOVE && msg->type == MSG_HARDWARE)
        {
            list_remove( &msg->entry );
            free_message( msg );
            continue;
        }
        win = find_hardware_message_window( input->desktop, input, msg, &msg_code, &win_thread );
        if (!win || !win_thread)
        {
            /* no window at all, remove it */
            update_thread_input_key_state( input, msg->msg, msg->wparam );
            list_remove( &msg->entry );
            free_message( msg );
            continue;
        }
        if (win_thread != thread)
        {
            if (win_thread->queue->input == input)
            {
                /* wake the other thread */
                set_queue_bits( win_thread->queue, get_hardware_msg_bit( msg->msg ) );
                got_one = 1;
            }
            else
            {
                /* for another thread input, drop it */
                update_thread_input_key_state( input, msg->msg, msg->wparam );
                list_remove( &msg->entry );
                free_message( msg );
            }
            release_object( win_thread );
            continue;
        }
        release_object( win_thread );

        /* if we already got a message for another thread, or if it doesn't
         * match the filter we skip it */
        if (got_one || !check_hw_message_filter( win, msg_code, filter_win, first, last ))
        {
            clear_bits &= ~get_hardware_msg_bit( msg->msg );
            continue;
        }

        reply->total = msg->data_size;
        if (msg->data_size > get_reply_max_size())
        {
            set_error( STATUS_BUFFER_OVERFLOW );
            return 1;
        }

        /* now we can return it */
        if (!msg->unique_id) msg->unique_id = get_unique_id();
        reply->type   = MSG_HARDWARE;
        reply->win    = win;
        reply->msg    = msg_code;
        reply->wparam = msg->wparam;
        reply->lparam = msg->lparam;
        reply->x      = msg->x;
        reply->y      = msg->y;
        reply->time   = msg->time;

        data->hw_id = msg->unique_id;
        set_reply_data( msg->data, msg->data_size );
        if ((get_hardware_msg_bit( msg->msg ) & (QS_RAWINPUT | QS_POINTER) && (flags & PM_REMOVE)) ||
            is_internal_hardware_message( msg->msg ))
            release_hardware_message( current->queue, data->hw_id );
        return 1;
    }
    /* nothing found, clear the hardware queue bits */
    clear_queue_bits( thread->queue, clear_bits );
    return 0;
}

/* increment (or decrement if 'incr' is negative) the queue paint count */
void inc_queue_paint_count( struct thread *thread, int incr )
{
    struct msg_queue *queue = thread->queue;

    assert( queue );

    if ((queue->paint_count += incr) < 0) queue->paint_count = 0;

    if (queue->paint_count)
        set_queue_bits( queue, QS_PAINT );
    else
        clear_queue_bits( queue, QS_PAINT );
}


/* remove all messages and timers belonging to a certain window */
void queue_cleanup_window( struct thread *thread, user_handle_t win )
{
    struct msg_queue *queue = thread->queue;
    struct list *ptr;
    int i;

    if (!queue) return;

    /* remove timers */

    ptr = list_head( &queue->pending_timers );
    while (ptr)
    {
        struct list *next = list_next( &queue->pending_timers, ptr );
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win) free_timer( queue, timer );
        ptr = next;
    }
    ptr = list_head( &queue->expired_timers );
    while (ptr)
    {
        struct list *next = list_next( &queue->expired_timers, ptr );
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        if (timer->win == win) free_timer( queue, timer );
        ptr = next;
    }

    /* remove messages */
    for (i = 0; i < NB_MSG_KINDS; i++)
    {
        struct list *ptr, *next;

        LIST_FOR_EACH_SAFE( ptr, next, &queue->msg_list[i] )
        {
            struct message *msg = LIST_ENTRY( ptr, struct message, entry );
            if (msg->win == win)
            {
                if (msg->msg == WM_QUIT && !queue->quit_message)
                {
                    queue->quit_message = 1;
                    queue->exit_code = msg->wparam;
                }
                remove_queue_message( queue, msg, i );
            }
        }
    }

    thread_input_cleanup_window( queue, win );
}

/* post a message to a window */
void post_message( user_handle_t win, unsigned int message, lparam_t wparam, lparam_t lparam )
{
    struct message *msg;
    struct thread *thread = get_window_thread( win );

    if (!thread) return;

    if (thread->queue && (msg = mem_alloc( sizeof(*msg) )))
    {
        msg->type      = MSG_POSTED;
        msg->win       = get_user_full_handle( win );
        msg->msg       = message;
        msg->wparam    = wparam;
        msg->lparam    = lparam;
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = 0;

        get_message_defaults( thread->queue, &msg->x, &msg->y, &msg->time );

        list_add_tail( &thread->queue->msg_list[POST_MESSAGE], &msg->entry );
        set_queue_bits( thread->queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
        if (message == WM_HOTKEY)
        {
            set_queue_bits( thread->queue, QS_HOTKEY );
            thread->queue->hotkey_count++;
        }
    }
    release_object( thread );
}

/* send a notify message to a window */
void send_notify_message( user_handle_t win, unsigned int message, lparam_t wparam, lparam_t lparam )
{
    struct message *msg;
    struct thread *thread = get_window_thread( win );

    if (!thread) return;

    if (thread->queue && (msg = mem_alloc( sizeof(*msg) )))
    {
        msg->type      = MSG_NOTIFY;
        msg->win       = get_user_full_handle( win );
        msg->msg       = message;
        msg->wparam    = wparam;
        msg->lparam    = lparam;
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = 0;

        get_message_defaults( thread->queue, &msg->x, &msg->y, &msg->time );

        list_add_tail( &thread->queue->msg_list[SEND_MESSAGE], &msg->entry );
        set_queue_bits( thread->queue, QS_SENDMESSAGE );
    }
    release_object( thread );
}

/* post a win event */
void post_win_event( struct thread *thread, unsigned int event,
                     user_handle_t win, unsigned int object_id,
                     unsigned int child_id, client_ptr_t hook_proc,
                     const WCHAR *module, data_size_t module_size,
                     user_handle_t hook)
{
    struct message *msg;

    if (thread->queue && (msg = mem_alloc( sizeof(*msg) )))
    {
        struct winevent_msg_data *data;

        msg->type      = MSG_WINEVENT;
        msg->win       = get_user_full_handle( win );
        msg->msg       = event;
        msg->wparam    = object_id;
        msg->lparam    = child_id;
        msg->time      = get_tick_count();
        msg->result    = NULL;

        if ((data = malloc( sizeof(*data) + module_size )))
        {
            data->hook = hook;
            data->tid  = get_thread_id( current );
            data->hook_proc = hook_proc;
            memcpy( data + 1, module, module_size );

            msg->data = data;
            msg->data_size = sizeof(*data) + module_size;

            if (debug_level > 1)
                fprintf( stderr, "post_win_event: tid %04x event %04x win %08x object_id %d child_id %d\n",
                         get_thread_id(thread), event, win, object_id, child_id );
            list_add_tail( &thread->queue->msg_list[SEND_MESSAGE], &msg->entry );
            set_queue_bits( thread->queue, QS_SENDMESSAGE );
        }
        else
            free( msg );
    }
}

void free_pointers( struct desktop *desktop )
{
    struct pointer *pointer, *next;

    LIST_FOR_EACH_ENTRY_SAFE( pointer, next, &desktop->pointers, struct pointer, entry )
    {
        list_remove( &pointer->entry );
        if (pointer->timeout) remove_timeout_user( pointer->timeout );
        free( pointer );
    }
}

/* free all hotkeys on a desktop, optionally filtering by window */
void free_hotkeys( struct desktop *desktop, user_handle_t window )
{
    struct hotkey *hotkey, *hotkey2;

    LIST_FOR_EACH_ENTRY_SAFE( hotkey, hotkey2, &desktop->hotkeys, struct hotkey, entry )
    {
        if (!window || hotkey->win == window)
        {
            list_remove( &hotkey->entry );
            free( hotkey );
        }
    }
}

/* retrieve the desktop which should receive some hardware input event */
static struct desktop *get_hardware_input_desktop( user_handle_t win )
{
    struct winstation *winstation;
    struct desktop *desktop;
    struct thread *thread;

    if (!win || !(thread = get_window_thread( win )))
    {
        if (!(winstation = get_visible_winstation())) return NULL;
        return get_input_desktop( winstation );
    }
    else
    {
        /* if window is specified, use its desktop to make it the input desktop */
        desktop = (struct desktop *)grab_object( thread->queue->input->desktop );
        release_object( thread );
    }

    return desktop;
}

/* enable or disable rawinput for a given process */
void set_rawinput_process( struct process *process, int enable )
{
    list_remove( &process->rawinput_entry ); /* remove it first, it might already be in the list */
    if (!process->rawinput_device_count || !enable) list_init( &process->rawinput_entry );
    else list_add_tail( &rawinput_processes, &process->rawinput_entry );
}


/* check if the thread owning the window is hung */
DECL_HANDLER(is_window_hung)
{
    struct thread *thread;

    thread = get_window_thread( req->win );
    if (thread)
    {
        reply->is_hung = is_queue_hung( thread->queue );
        release_object( thread );
    }
    else reply->is_hung = 0;
}


/* get a handle for the current thread message queue */
DECL_HANDLER(get_msg_queue_handle)
{
    struct msg_queue *queue = get_current_queue();

    reply->handle = 0;
    if (queue) reply->handle = alloc_handle( current->process, queue, SYNCHRONIZE, 0 );
}


/* get the message queue of the current thread */
DECL_HANDLER(get_msg_queue)
{
    struct msg_queue *queue = get_current_queue();
    if (queue) reply->locator = get_shared_object_locator( queue->shared );
}


/* set the file descriptor associated to the current thread queue */
DECL_HANDLER(set_queue_fd)
{
    struct msg_queue *queue = get_current_queue();
    struct file *file;
    int unix_fd;

    if (queue->fd)  /* fd can only be set once */
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (!(file = get_file_obj( current->process, req->handle, SYNCHRONIZE ))) return;

    if ((unix_fd = get_file_unix_fd( file )) != -1)
    {
        if ((unix_fd = dup( unix_fd )) != -1)
            queue->fd = create_anonymous_fd( &msg_queue_fd_ops, unix_fd, &queue->obj, 0 );
        else
            file_set_error();
    }
    release_object( file );
}


/* set the current message queue wakeup mask */
DECL_HANDLER(set_queue_mask)
{
    struct msg_queue *queue = get_current_queue();

    if (queue)
    {
        const queue_shm_t *queue_shm = queue->shared;

        SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
        {
            shared->wake_mask = req->wake_mask;
            shared->changed_mask = req->changed_mask;
        }
        SHARED_WRITE_END;

        reply->wake_bits    = queue_shm->wake_bits;
        reply->changed_bits = queue_shm->changed_bits;

        if (is_signaled( queue ))
        {
            /* if skip wait is set, do what would have been done in the subsequent wait */
            if (req->skip_wait)
            {
                SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
                {
                    shared->wake_mask = 0;
                    shared->changed_mask = 0;
                }
                SHARED_WRITE_END;
            }
            else wake_up( &queue->obj, 0 );
        }
    }
}


/* get the current message queue status */
DECL_HANDLER(get_queue_status)
{
    struct msg_queue *queue = current->queue;
    if (queue)
    {
        const queue_shm_t *queue_shm = queue->shared;

        reply->wake_bits    = queue_shm->wake_bits;
        reply->changed_bits = queue_shm->changed_bits;

        SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
        {
            shared->changed_bits &= ~req->clear_bits;
        }
        SHARED_WRITE_END;
    }
    else reply->wake_bits = reply->changed_bits = 0;
}


/* send a message to a thread queue */
DECL_HANDLER(send_message)
{
    struct message *msg;
    struct msg_queue *send_queue = get_current_queue();
    struct msg_queue *recv_queue = NULL;
    struct thread *thread = NULL;

    if (!(thread = get_thread_from_id( req->id ))) return;

    if (!(recv_queue = thread->queue))
    {
        set_error( STATUS_INVALID_PARAMETER );
        release_object( thread );
        return;
    }
    if ((req->flags & SEND_MSG_ABORT_IF_HUNG) && is_queue_hung(recv_queue))
    {
        set_error( STATUS_TIMEOUT );
        release_object( thread );
        return;
    }

    if ((msg = mem_alloc( sizeof(*msg) )))
    {
        msg->type      = req->type;
        msg->win       = get_user_full_handle( req->win );
        msg->msg       = req->msg;
        msg->wparam    = req->wparam;
        msg->lparam    = req->lparam;
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = get_req_data_size();

        get_message_defaults( recv_queue, &msg->x, &msg->y, &msg->time );

        if (msg->data_size && !(msg->data = memdup( get_req_data(), msg->data_size )))
        {
            free( msg );
            release_object( thread );
            return;
        }

        switch(msg->type)
        {
        case MSG_OTHER_PROCESS:
        case MSG_ASCII:
        case MSG_UNICODE:
        case MSG_CALLBACK:
            if (!(msg->result = alloc_message_result( send_queue, recv_queue, msg, req->timeout )))
            {
                free_message( msg );
                break;
            }
            /* fall through */
        case MSG_NOTIFY:
            list_add_tail( &recv_queue->msg_list[SEND_MESSAGE], &msg->entry );
            set_queue_bits( recv_queue, QS_SENDMESSAGE );
            break;
        case MSG_POSTED:
            list_add_tail( &recv_queue->msg_list[POST_MESSAGE], &msg->entry );
            set_queue_bits( recv_queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
            if (msg->msg == WM_HOTKEY)
            {
                set_queue_bits( recv_queue, QS_HOTKEY );
                recv_queue->hotkey_count++;
            }
            break;
        case MSG_HARDWARE:  /* should use send_hardware_message instead */
        case MSG_CALLBACK_RESULT:  /* cannot send this one */
        case MSG_HOOK_LL:  /* generated internally */
        default:
            set_error( STATUS_INVALID_PARAMETER );
            free( msg );
            break;
        }
    }
    release_object( thread );
}

/* send a hardware message to a thread queue */
DECL_HANDLER(send_hardware_message)
{
    struct desktop *desktop;
    unsigned int origin = (req->flags & SEND_HWMSG_INJECTED ? IMO_INJECTED : IMO_HARDWARE);
    struct msg_queue *sender = req->flags & SEND_HWMSG_INJECTED ? get_current_queue() : NULL;
    const desktop_shm_t *desktop_shm;
    int wait = 0;

    if (!(desktop = get_hardware_input_desktop( req->win ))) return;
    if ((origin == IMO_INJECTED && desktop != current->queue->input->desktop) ||
        !set_input_desktop( desktop->winstation, desktop ))
    {
        release_object( desktop );
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    desktop_shm = desktop->shared;

    reply->prev_x = desktop_shm->cursor.x;
    reply->prev_y = desktop_shm->cursor.y;

    switch (req->input.type)
    {
    case INPUT_MOUSE:
        wait = queue_mouse_message( desktop, req->win, &req->input, origin, sender );
        break;
    case INPUT_KEYBOARD:
        wait = queue_keyboard_message( desktop, req->win, &req->input, origin, sender, 0 );
        break;
    case INPUT_HARDWARE:
        queue_custom_hardware_message( desktop, req->win, origin, &req->input );
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
    }

    reply->wait = sender ? wait : 0;
    reply->new_x = desktop_shm->cursor.x;
    reply->new_y = desktop_shm->cursor.y;
    release_object( desktop );
}

/* post a quit message to the current queue */
DECL_HANDLER(post_quit_message)
{
    struct msg_queue *queue = get_current_queue();

    if (!queue)
        return;

    queue->quit_message = 1;
    queue->exit_code = req->exit_code;
    set_queue_bits( queue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE );
}

/* get a message from the current queue */
DECL_HANDLER(get_message)
{
    struct timer *timer;
    struct list *ptr;
    struct msg_queue *queue = get_current_queue();
    user_handle_t get_win = get_user_full_handle( req->get_win );
    const queue_shm_t *queue_shm;
    unsigned int filter;

    if (get_win && get_win != 1 && get_win != -1 && !get_user_object( get_win, USER_WINDOW ))
    {
        set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
        return;
    }

    if (!queue) return;
    queue_shm = queue->shared;

    /* check for any hardware internal message */
    if (get_hardware_message( current, req->hw_id, get_win, WM_WINE_FIRST_DRIVER_MSG,
                              WM_WINE_LAST_DRIVER_MSG, req->flags, reply ))
        return;

    if (req->internal) /* check for internal messages only, leave queue flags unchanged */
    {
        set_error( STATUS_PENDING );
        return;
    }

    queue->last_get_msg = current_time;

    /* first check for sent messages */
    if ((ptr = list_head( &queue->msg_list[SEND_MESSAGE] )))
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        receive_message( queue, msg, reply );
        return;
    }

    /* use the same logic as in win32u/message.c peek_message */
    if (!(filter = req->flags >> 16)) filter = QS_ALLINPUT;

    /* clear changed bits so we can wait on them if we don't find a message */
    SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
    {
        if (filter & QS_POSTMESSAGE)
        {
            shared->changed_bits &= ~(QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER);
            if (req->get_first == 0 && req->get_last == ~0U) shared->changed_bits &= ~QS_ALLPOSTMESSAGE;
        }
        if (filter & QS_INPUT) shared->changed_bits &= ~QS_INPUT;
        if (filter & QS_PAINT) shared->changed_bits &= ~QS_PAINT;
    }
    SHARED_WRITE_END;

    /* then check for posted messages */
    if ((filter & QS_POSTMESSAGE) &&
        get_posted_message( queue, get_win, req->get_first, req->get_last, req->flags, reply ))
        return;

    if ((filter & QS_HOTKEY) && queue->hotkey_count &&
        req->get_first <= WM_HOTKEY && req->get_last >= WM_HOTKEY &&
        get_posted_message( queue, get_win, WM_HOTKEY, WM_HOTKEY, req->flags, reply ))
        return;

    /* only check for quit messages if not posted messages pending */
    if ((filter & QS_POSTMESSAGE) && get_quit_message( queue, req->flags, reply ))
        return;

    /* then check for any raw hardware message */
    if ((filter & QS_INPUT) &&
        filter_contains_hw_range( req->get_first, req->get_last ) &&
        get_hardware_message( current, req->hw_id, get_win, req->get_first, req->get_last, req->flags, reply ))
        return;

    /* now check for WM_PAINT */
    if ((filter & QS_PAINT) &&
        queue->paint_count &&
        check_msg_filter( WM_PAINT, req->get_first, req->get_last ) &&
        (reply->win = find_window_to_repaint( get_win, current )))
    {
        reply->type   = MSG_POSTED;
        reply->msg    = WM_PAINT;
        reply->wparam = 0;
        reply->lparam = 0;
        get_message_defaults( queue, &reply->x, &reply->y, &reply->time );
        return;
    }

    /* now check for timer */
    if ((filter & QS_TIMER) &&
        (timer = find_expired_timer( queue, get_win, req->get_first,
                                     req->get_last, (req->flags & PM_REMOVE) )))
    {
        reply->type   = MSG_POSTED;
        reply->win    = timer->win;
        reply->msg    = timer->msg;
        reply->wparam = timer->id;
        reply->lparam = timer->lparam;
        get_message_defaults( queue, &reply->x, &reply->y, &reply->time );
        if (!(req->flags & PM_NOYIELD) && current->process->idle_event)
            set_event( current->process->idle_event );
        return;
    }

    if (get_win == -1 && current->process->idle_event) set_event( current->process->idle_event );

    SHARED_WRITE_BEGIN( queue_shm, queue_shm_t )
    {
        shared->wake_mask = req->wake_mask;
        shared->changed_mask = req->changed_mask;
    }
    SHARED_WRITE_END;

    set_error( STATUS_PENDING );  /* FIXME */
}


/* reply to a sent message */
DECL_HANDLER(reply_message)
{
    if (!current->queue) set_error( STATUS_ACCESS_DENIED );
    else if (current->queue->recv_result)
        reply_message( current->queue, req->result, 0, req->remove,
                       get_req_data(), get_req_data_size() );
}


/* accept the current hardware message */
DECL_HANDLER(accept_hardware_message)
{
    if (current->queue)
        release_hardware_message( current->queue, req->hw_id );
    else
        set_error( STATUS_ACCESS_DENIED );
}


/* retrieve the reply for the last message sent */
DECL_HANDLER(get_message_reply)
{
    struct message_result *result;
    struct list *entry;
    struct msg_queue *queue = current->queue;

    if (queue)
    {
        set_error( STATUS_PENDING );
        reply->result = 0;

        if (!(entry = list_head( &queue->send_result ))) return;  /* no reply ready */

        result = LIST_ENTRY( entry, struct message_result, sender_entry );
        if (result->replied || req->cancel)
        {
            if (result->replied)
            {
                reply->result = result->result;
                set_error( result->error );
                if (result->data)
                {
                    data_size_t data_len = min( result->data_size, get_reply_max_size() );
                    set_reply_data_ptr( result->data, data_len );
                    result->data = NULL;
                    result->data_size = 0;
                }
            }
            remove_result_from_sender( result );

            entry = list_head( &queue->send_result );
            if (!entry) clear_queue_bits( queue, QS_SMRESULT );
            else
            {
                result = LIST_ENTRY( entry, struct message_result, sender_entry );
                if (result->replied) set_queue_bits( queue, QS_SMRESULT );
                else clear_queue_bits( queue, QS_SMRESULT );
            }
        }
    }
    else set_error( STATUS_ACCESS_DENIED );
}


/* set a window timer */
DECL_HANDLER(set_win_timer)
{
    struct timer *timer;
    struct msg_queue *queue;
    struct thread *thread = NULL;
    user_handle_t win = 0;
    lparam_t id = req->id;

    if (req->win)
    {
        if (!(win = get_user_full_handle( req->win )) || !(thread = get_window_thread( win )))
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
        if (thread->process != current->process)
        {
            release_object( thread );
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
        queue = thread->queue;
        /* remove it if it existed already */
        if ((timer = find_timer( queue, win, req->msg, id ))) free_timer( queue, timer );
    }
    else
    {
        queue = get_current_queue();
        /* look for a timer with this id */
        if (id && (timer = find_timer( queue, 0, req->msg, id )))
        {
            /* free and reuse id */
            free_timer( queue, timer );
        }
        else
        {
            lparam_t end_id = queue->next_timer_id;

            /* find a free id for it */
            while (1)
            {
                id = queue->next_timer_id;
                if (--queue->next_timer_id <= 0x100) queue->next_timer_id = 0x7fff;

                if (!find_timer( queue, 0, req->msg, id )) break;
                if (queue->next_timer_id == end_id)
                {
                    set_win32_error( ERROR_NO_MORE_USER_HANDLES );
                    return;
                }
            }
        }
    }

    if ((timer = set_timer( queue, req->rate )))
    {
        timer->win    = win;
        timer->msg    = req->msg;
        timer->id     = id;
        timer->lparam = req->lparam;
        reply->id     = id;
    }
    if (thread) release_object( thread );
}

/* kill a window timer */
DECL_HANDLER(kill_win_timer)
{
    struct timer *timer;
    struct thread *thread;
    user_handle_t win = 0;

    if (req->win)
    {
        if (!(win = get_user_full_handle( req->win )) || !(thread = get_window_thread( win )))
        {
            set_error( STATUS_INVALID_HANDLE );
            return;
        }
        if (thread->process != current->process)
        {
            release_object( thread );
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
    }
    else thread = (struct thread *)grab_object( current );

    if (thread->queue && (timer = find_timer( thread->queue, win, req->msg, req->id )))
        free_timer( thread->queue, timer );
    else
        set_error( STATUS_INVALID_PARAMETER );

    release_object( thread );
}

DECL_HANDLER(register_hotkey)
{
    struct desktop *desktop;
    user_handle_t win_handle = req->window;
    struct hotkey *hotkey;
    struct hotkey *new_hotkey = NULL;
    struct thread *thread;
    const int modifier_flags = MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN;

    if (!(desktop = get_thread_desktop( current, 0 ))) return;

    if (win_handle)
    {
        if (!(win_handle = get_valid_window_handle( win_handle )))
        {
            release_object( desktop );
            return;
        }

        thread = get_window_thread( win_handle );
        if (thread) release_object( thread );

        if (thread != current)
        {
            release_object( desktop );
            set_win32_error( ERROR_WINDOW_OF_OTHER_THREAD );
            return;
        }
    }

    LIST_FOR_EACH_ENTRY( hotkey, &desktop->hotkeys, struct hotkey, entry )
    {
        if (req->vkey == hotkey->vkey &&
            (req->flags & modifier_flags) == (hotkey->flags & modifier_flags))
        {
            release_object( desktop );
            set_win32_error( ERROR_HOTKEY_ALREADY_REGISTERED );
            return;
        }
        if (current->queue == hotkey->queue && win_handle == hotkey->win && req->id == hotkey->id)
            new_hotkey = hotkey;
    }

    if (new_hotkey)
    {
        reply->replaced = 1;
        reply->flags    = new_hotkey->flags;
        reply->vkey     = new_hotkey->vkey;
    }
    else
    {
        new_hotkey = mem_alloc( sizeof(*new_hotkey) );
        if (new_hotkey)
        {
            list_add_tail( &desktop->hotkeys, &new_hotkey->entry );
            new_hotkey->queue  = current->queue;
            new_hotkey->win    = win_handle;
            new_hotkey->id     = req->id;
        }
    }

    if (new_hotkey)
    {
        new_hotkey->flags = req->flags;
        new_hotkey->vkey  = req->vkey;
    }

    release_object( desktop );
}

DECL_HANDLER(unregister_hotkey)
{
    struct desktop *desktop;
    user_handle_t win_handle = req->window;
    struct hotkey *hotkey;
    struct thread *thread;

    if (!(desktop = get_thread_desktop( current, 0 ))) return;

    if (win_handle)
    {
        if (!(win_handle = get_valid_window_handle( win_handle )))
        {
            release_object( desktop );
            return;
        }

        thread = get_window_thread( win_handle );
        if (thread) release_object( thread );

        if (thread != current)
        {
            release_object( desktop );
            set_win32_error( ERROR_WINDOW_OF_OTHER_THREAD );
            return;
        }
    }

    LIST_FOR_EACH_ENTRY( hotkey, &desktop->hotkeys, struct hotkey, entry )
    {
        if (current->queue == hotkey->queue && win_handle == hotkey->win && req->id == hotkey->id)
            goto found;
    }

    release_object( desktop );
    set_win32_error( ERROR_HOTKEY_NOT_REGISTERED );
    return;

found:
    reply->flags = hotkey->flags;
    reply->vkey  = hotkey->vkey;
    list_remove( &hotkey->entry );
    free( hotkey );
    release_object( desktop );
}

/* attach (or detach) thread inputs */
DECL_HANDLER(attach_thread_input)
{
    struct thread *thread_from = get_thread_from_id( req->tid_from );
    struct thread *thread_to = get_thread_from_id( req->tid_to );

    if (!thread_from || !thread_to)
    {
        if (thread_from) release_object( thread_from );
        if (thread_to) release_object( thread_to );
        return;
    }
    if (thread_from != thread_to)
    {
        if (req->attach)
        {
            if ((thread_to->queue || thread_to == current) &&
                (thread_from->queue || thread_from == current))
                attach_thread_input( thread_from, thread_to );
            else
                set_error( STATUS_INVALID_PARAMETER );
        }
        else
        {
            if (thread_from->queue && thread_to->queue &&
                thread_from->queue->input == thread_to->queue->input)
                detach_thread_input( thread_from );
            else
                set_error( STATUS_ACCESS_DENIED );
        }
    }
    else set_error( STATUS_ACCESS_DENIED );
    release_object( thread_from );
    release_object( thread_to );
}


/* get the thread input of the given thread */
DECL_HANDLER(get_thread_input)
{
    struct thread_input *input;

    if (req->tid)
    {
        struct thread *thread;
        if (!(thread = get_thread_from_id( req->tid ))) return;
        input = thread->queue ? thread->queue->input : NULL;
        release_object( thread );
    }
    else
    {
        struct desktop *desktop;
        if (!(desktop = get_thread_desktop( current, 0 ))) return;
        input = desktop->foreground_input;  /* get the foreground thread info */
        release_object( desktop );
    }

    if (input && input->shared) reply->locator = get_shared_object_locator( input->shared );
}


/* retrieve queue keyboard state for current thread or global async state */
DECL_HANDLER(get_key_state)
{
    struct desktop *desktop;

    if (req->async)  /* get global async key state */
    {
        if (!(desktop = get_thread_desktop( current, 0 ))) return;
        SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
        {
            reply->state = shared->keystate[req->key & 0xff];
            shared->keystate[req->key & 0xff] &= ~0x40;
        }
        SHARED_WRITE_END;
        release_object( desktop );
    }
    else
    {
        struct msg_queue *queue = get_current_queue();
        const input_shm_t *input_shm = queue->input->shared;
        unsigned char *keystate = (void *)input_shm->keystate;
        sync_input_keystate( queue->input );
        reply->state = keystate[req->key & 0xff];
    }
}


/* set queue keyboard state for current thread */
DECL_HANDLER(set_key_state)
{
    struct msg_queue *queue = get_current_queue();
    struct desktop *desktop = queue->input->desktop;
    data_size_t size = min( 256, get_req_data_size() );
    const input_shm_t *input_shm = queue->input->shared;

    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        memcpy( (void *)shared->keystate, get_req_data(), size );
    }
    SHARED_WRITE_END;

    memcpy( queue->input->desktop_keystate, (const void *)desktop->shared->keystate,
            sizeof(queue->input->desktop_keystate) );

    if (req->async && (desktop = get_thread_desktop( current, 0 )))
    {
        SHARED_WRITE_BEGIN( desktop->shared, desktop_shm_t )
        {
            memcpy( (void *)shared->keystate, get_req_data(), size );
        }
        SHARED_WRITE_END;
        release_object( desktop );
    }
}


/* set the system foreground window */
DECL_HANDLER(set_foreground_window)
{
    struct thread *thread = NULL;
    struct desktop *desktop;
    struct thread_input *input;
    struct msg_queue *queue = get_current_queue();

    if (!(desktop = get_thread_desktop( current, 0 ))) return;

    if (!(input = desktop->foreground_input)) reply->previous = 0;
    else reply->previous = input->shared->active;

    reply->send_msg_old = (reply->previous && desktop->foreground_input != queue->input);
    reply->send_msg_new = FALSE;

    if (is_valid_foreground_window( req->handle ) &&
        (thread = get_window_thread( req->handle )) &&
        thread->queue->input->desktop == desktop)
    {
        set_foreground_input( desktop, thread->queue->input );
        reply->send_msg_new = (desktop->foreground_input != queue->input);
    }
    else set_win32_error( ERROR_INVALID_WINDOW_HANDLE );

    if (thread) release_object( thread );
    release_object( desktop );
}


/* set the current thread focus window */
DECL_HANDLER(set_focus_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        const input_shm_t *input_shm = queue->input->shared;
        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            reply->previous = shared->focus;
            shared->focus = get_user_full_handle( req->handle );
        }
        SHARED_WRITE_END;
    }
}


/* set the current thread active window */
DECL_HANDLER(set_active_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        if (!req->handle || make_window_active( req->handle ))
        {
            const input_shm_t *input_shm = queue->input->shared;
            SHARED_WRITE_BEGIN( input_shm, input_shm_t )
            {
                reply->previous = shared->active;
                shared->active = get_user_full_handle( req->handle );
            }
            SHARED_WRITE_END;
        }
        else set_error( STATUS_INVALID_HANDLE );
    }
}


/* set the current thread capture window */
DECL_HANDLER(set_capture_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = reply->full_handle = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        struct thread_input *input = queue->input;
        const input_shm_t *input_shm = input->shared;

        /* if in menu mode, reject all requests to change focus, except if the menu bit is set */
        if (input_shm->menu_owner && !(req->flags & CAPTURE_MENU))
        {
            set_error(STATUS_ACCESS_DENIED);
            return;
        }
        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            reply->previous = shared->capture;
            shared->capture = get_user_full_handle( req->handle );
            shared->menu_owner = (req->flags & CAPTURE_MENU) ? shared->capture : 0;
            shared->move_size = (req->flags & CAPTURE_MOVESIZE) ? shared->capture : 0;
            reply->full_handle = shared->capture;
        }
        SHARED_WRITE_END;
        if (reply->previous && !req->handle && !req->flags)
            update_cursor_pos(input->desktop);
    }
}


/* Set the current thread caret window */
DECL_HANDLER(set_caret_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = 0;
    if (queue && check_queue_input_window( queue, req->handle ))
    {
        struct thread_input *input = queue->input;
        const input_shm_t *input_shm = input->shared;
        user_handle_t caret = get_user_full_handle(req->handle);

        reply->previous  = input_shm->caret;
        reply->old_rect  = input_shm->caret_rect;
        reply->old_hide  = input->caret_hide;
        reply->old_state = input->caret_state;

        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            set_caret_window( input, shared, caret );
            shared->caret_rect.right  = shared->caret_rect.left + req->width;
            shared->caret_rect.bottom = shared->caret_rect.top + req->height;
        }
        SHARED_WRITE_END;
    }
}


/* Set the current thread caret information */
DECL_HANDLER(set_caret_info)
{
    struct msg_queue *queue = get_current_queue();
    const input_shm_t *input_shm;
    struct thread_input *input;

    if (!queue) return;
    input = queue->input;
    input_shm = input->shared;
    reply->full_handle = input_shm->caret;
    reply->old_rect    = input_shm->caret_rect;
    reply->old_hide    = input->caret_hide;
    reply->old_state   = input->caret_state;

    if (req->handle && get_user_full_handle(req->handle) != input_shm->caret)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (req->flags & SET_CARET_POS)
    {
        SHARED_WRITE_BEGIN( input_shm, input_shm_t )
        {
            shared->caret_rect.right  += req->x - shared->caret_rect.left;
            shared->caret_rect.bottom += req->y - shared->caret_rect.top;
            shared->caret_rect.left = req->x;
            shared->caret_rect.top  = req->y;
        }
        SHARED_WRITE_END;
    }
    if (req->flags & SET_CARET_HIDE)
    {
        input->caret_hide += req->hide;
        if (input->caret_hide < 0) input->caret_hide = 0;
    }
    if (req->flags & SET_CARET_STATE)
    {
        switch (req->state)
        {
        case CARET_STATE_OFF:    input->caret_state = 0; break;
        case CARET_STATE_ON:     input->caret_state = 1; break;
        case CARET_STATE_TOGGLE: input->caret_state = !input->caret_state; break;
        case CARET_STATE_ON_IF_MOVED:
            if (req->x != reply->old_rect.left || req->y != reply->old_rect.top) input->caret_state = 1;
            break;
        }
    }
}


/* get the time of the last input event */
DECL_HANDLER(get_last_input_time)
{
    reply->time = last_input_time;
}

/* set/get the current cursor */
DECL_HANDLER(set_cursor)
{
    struct msg_queue *queue = get_current_queue();
    user_handle_t prev_cursor, new_cursor;
    struct thread_input *input;
    const input_shm_t *input_shm;
    struct desktop *desktop;
    const desktop_shm_t *desktop_shm;

    if (!queue) return;
    input = queue->input;
    input_shm = input->shared;
    desktop = input->desktop;
    desktop_shm = desktop->shared;
    prev_cursor = input_shm->cursor_count < 0 ? 0 : input_shm->cursor;

    reply->prev_handle = input_shm->cursor;
    reply->prev_count  = input_shm->cursor_count;
    reply->prev_x      = desktop_shm->cursor.x;
    reply->prev_y      = desktop_shm->cursor.y;

    if ((req->flags & SET_CURSOR_HANDLE) && req->handle &&
        !get_user_object( req->handle, USER_CLIENT ))
    {
        set_win32_error( ERROR_INVALID_CURSOR_HANDLE );
        return;
    }

    SHARED_WRITE_BEGIN( input_shm, input_shm_t )
    {
        if (req->flags & SET_CURSOR_HANDLE)
            shared->cursor = req->handle;
        if (req->flags & SET_CURSOR_COUNT)
        {
            queue->cursor_count += req->show_count;
            shared->cursor_count += req->show_count;
        }
    }
    SHARED_WRITE_END;

    if (req->flags & SET_CURSOR_POS) set_cursor_pos( desktop, req->x, req->y );
    if (req->flags & SET_CURSOR_CLIP) set_clip_rectangle( desktop, &req->clip, req->flags, 0 );
    if (req->flags & SET_CURSOR_NOCLIP) set_clip_rectangle( desktop, NULL, SET_CURSOR_NOCLIP, 0 );

    new_cursor = input_shm->cursor_count < 0 ? 0 : input_shm->cursor;
    if (prev_cursor != new_cursor) update_desktop_cursor_handle( desktop, input, new_cursor );

    reply->new_x       = desktop_shm->cursor.x;
    reply->new_y       = desktop_shm->cursor.y;
    reply->new_clip    = desktop_shm->cursor.clip;
    reply->last_change = desktop_shm->cursor.last_change;
}

/* Get the history of the 64 last cursor positions */
DECL_HANDLER(get_cursor_history)
{
    struct cursor_pos *pos;
    unsigned int i, count = min( 64, get_reply_max_size() / sizeof(*pos) );

    if ((pos = set_reply_data_size( count * sizeof(*pos) )))
        for (i = 0; i < count; i++)
            pos[i] = cursor_history[(i + cursor_history_latest) % ARRAY_SIZE(cursor_history)];
}

DECL_HANDLER(get_rawinput_buffer)
{
    const size_t align = is_machine_64bit( current->process->machine ) ? 7 : 3;
    data_size_t buffer_size = get_reply_max_size() & ~align;
    struct thread_input *input = current->queue->input;
    struct message *msg, *next_msg;
    int count = 0;
    char *buffer;

    if (req->header_size != sizeof(RAWINPUTHEADER))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!req->read_data)
    {
        LIST_FOR_EACH_ENTRY( msg, &input->msg_list, struct message, entry )
        {
            if (msg->msg == WM_INPUT)
            {
                struct hardware_msg_data *msg_data = msg->data;
                data_size_t size = msg_data->size - sizeof(*msg_data);
                reply->next_size = sizeof(RAWINPUTHEADER) + size;
                break;
            }
        }

    }
    else if ((buffer = mem_alloc( buffer_size )))
    {
        size_t total_size = 0, next_size = 0;

        reply->next_size = get_reply_max_size();

        LIST_FOR_EACH_ENTRY_SAFE( msg, next_msg, &input->msg_list, struct message, entry )
        {
            if (msg->msg == WM_INPUT)
            {
                RAWINPUT *rawinput = (RAWINPUT *)(buffer + total_size);
                struct hardware_msg_data *msg_data = msg->data;
                data_size_t data_size = msg_data->size - sizeof(*msg_data);

                if (total_size + sizeof(RAWINPUTHEADER) + data_size > buffer_size)
                {
                    next_size = sizeof(RAWINPUTHEADER) + data_size;
                    break;
                }

                rawinput->header.dwSize  = sizeof(RAWINPUTHEADER) + data_size;
                rawinput->header.dwType  = msg_data->rawinput.type;
                rawinput->header.hDevice = UlongToHandle(msg_data->rawinput.device);
                rawinput->header.wParam  = msg_data->rawinput.wparam;
                memcpy( &rawinput->header + 1, msg_data + 1, data_size );

                total_size += (rawinput->header.dwSize + align) & ~align;
                reply->time = msg->time;
                list_remove( &msg->entry );
                free_message( msg );
                count++;
            }
        }

        if (!next_size)
        {
            if (count) next_size = sizeof(RAWINPUT);
            else reply->next_size = 0;
        }

        if (next_size && get_reply_max_size() <= next_size)
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            reply->next_size = next_size;
        }

        reply->count = count;
        set_reply_data_ptr( buffer, total_size );
    }
}

DECL_HANDLER(update_rawinput_devices)
{
    const struct rawinput_device *tmp, *devices = get_req_data();
    unsigned int device_count = get_req_data_size() / sizeof (*devices);
    size_t size = device_count * sizeof(*devices);
    struct process *process = current->process;
    struct winstation *winstation;
    struct desktop *desktop;

    if (!size)
    {
        process->rawinput_device_count = 0;
        process->rawinput_mouse = NULL;
        process->rawinput_kbd = NULL;
        set_rawinput_process( process, 0 );
        return;
    }

    if (!(tmp = realloc( process->rawinput_devices, size )))
    {
        set_error( STATUS_NO_MEMORY );
        return;
    }
    process->rawinput_devices = (struct rawinput_device *)tmp;
    process->rawinput_device_count = device_count;
    memcpy( process->rawinput_devices, devices, size );

    process->rawinput_mouse = find_rawinput_device( process, MAKELONG(HID_USAGE_GENERIC_MOUSE, HID_USAGE_PAGE_GENERIC) );
    process->rawinput_kbd = find_rawinput_device( process, MAKELONG(HID_USAGE_GENERIC_KEYBOARD, HID_USAGE_PAGE_GENERIC) );

    if ((winstation = get_visible_winstation()) && (desktop = get_input_desktop( winstation )))
    {
        struct thread *thread;

        /* one of the process thread might be connected to the input desktop, update the full list */
        LIST_FOR_EACH_ENTRY( thread, &desktop->threads, struct thread, desktop_entry )
            set_rawinput_process( thread->process, 1 );

        release_object( desktop );
    }
}

DECL_HANDLER(set_keyboard_repeat)
{
    struct desktop *desktop;

    if (!(desktop = get_thread_desktop( current, 0 ))) return;

    /* report previous values */
    reply->enable = desktop->key_repeat.enable;

    /* ignore negative values to allow partial updates */
    if (req->enable >= 0) desktop->key_repeat.enable = req->enable;
    if (req->delay >= 0) desktop->key_repeat.delay = -req->delay * 10000;
    if (req->period >= 0) desktop->key_repeat.period = -req->period * 10000;

    if (!desktop->key_repeat.enable) stop_key_repeat( desktop );

    release_object( desktop );
}

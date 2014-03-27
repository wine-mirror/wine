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
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"

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
    unsigned int           time;      /* message time */
    void                  *data;      /* message data for sent messages */
    unsigned int           data_size; /* size of message data */
    unsigned int           unique_id; /* unique id for nested hw message waits */
    struct message_result *result;    /* result in sender queue */
};

struct timer
{
    struct list     entry;     /* entry in timer list */
    timeout_t       when;      /* next expiration */
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
    user_handle_t          focus;         /* focus window */
    user_handle_t          capture;       /* capture window */
    user_handle_t          active;        /* active window */
    user_handle_t          menu_owner;    /* current menu owner window */
    user_handle_t          move_size;     /* current moving/resizing window */
    user_handle_t          caret;         /* caret window */
    rectangle_t            caret_rect;    /* caret rectangle */
    int                    caret_hide;    /* caret hide count */
    int                    caret_state;   /* caret on/off state */
    user_handle_t          cursor;        /* current cursor */
    int                    cursor_count;  /* cursor show count */
    struct list            msg_list;      /* list of hardware messages */
    unsigned char          keystate[256]; /* state of each key */
};

struct msg_queue
{
    struct object          obj;             /* object header */
    struct fd             *fd;              /* optional file descriptor to poll */
    unsigned int           wake_bits;       /* wakeup bits */
    unsigned int           wake_mask;       /* wakeup mask */
    unsigned int           wake_get_msg;    /* wakeup mask of last get_message */
    unsigned int           changed_bits;    /* changed wakeup bits */
    unsigned int           changed_mask;    /* changed wakeup mask */
    unsigned int           changed_get_msg; /* changed wakeup mask of last get_message */
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
    msg_queue_dump,            /* dump */
    no_get_type,               /* get_type */
    msg_queue_add_queue,       /* add_queue */
    msg_queue_remove_queue,    /* remove_queue */
    msg_queue_signaled,        /* signaled */
    msg_queue_satisfied,       /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    no_map_access,             /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
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
    thread_input_dump,            /* dump */
    no_get_type,                  /* get_type */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    no_map_access,                /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    thread_input_destroy          /* destroy */
};

/* pointer to input structure of foreground thread */
static unsigned int last_input_time;

static void queue_hardware_message( struct desktop *desktop, struct message *msg, int always_queue );
static void free_message( struct message *msg );

/* set the caret window in a given thread input */
static void set_caret_window( struct thread_input *input, user_handle_t win )
{
    if (!win || win != input->caret)
    {
        input->caret_rect.left   = 0;
        input->caret_rect.top    = 0;
        input->caret_rect.right  = 0;
        input->caret_rect.bottom = 0;
    }
    input->caret             = win;
    input->caret_hide        = 1;
    input->caret_state       = 0;
}

/* create a thread input object */
static struct thread_input *create_thread_input( struct thread *thread )
{
    struct thread_input *input;

    if ((input = alloc_object( &thread_input_ops )))
    {
        input->focus        = 0;
        input->capture      = 0;
        input->active       = 0;
        input->menu_owner   = 0;
        input->move_size    = 0;
        input->cursor       = 0;
        input->cursor_count = 0;
        list_init( &input->msg_list );
        set_caret_window( input, 0 );
        memset( input->keystate, 0, sizeof(input->keystate) );

        if (!(input->desktop = get_thread_desktop( thread, 0 /* FIXME: access rights */ )))
        {
            release_object( input );
            return NULL;
        }
    }
    return input;
}

/* create a message queue object */
static struct msg_queue *create_msg_queue( struct thread *thread, struct thread_input *input )
{
    struct thread_input *new_input = NULL;
    struct msg_queue *queue;
    int i;

    if (!input)
    {
        if (!(new_input = create_thread_input( thread ))) return NULL;
        input = new_input;
    }

    if ((queue = alloc_object( &msg_queue_ops )))
    {
        queue->fd              = NULL;
        queue->wake_bits       = 0;
        queue->wake_mask       = 0;
        queue->wake_get_msg    = 0;
        queue->changed_bits    = 0;
        queue->changed_mask    = 0;
        queue->changed_get_msg = 0;
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
        list_init( &queue->send_result );
        list_init( &queue->callback_result );
        list_init( &queue->pending_timers );
        list_init( &queue->expired_timers );
        for (i = 0; i < NB_MSG_KINDS; i++) list_init( &queue->msg_list[i] );

        thread->queue = queue;
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

/* change the thread input data of a given thread */
static int assign_thread_input( struct thread *thread, struct thread_input *new_input )
{
    struct msg_queue *queue = thread->queue;

    if (!queue)
    {
        thread->queue = create_msg_queue( thread, new_input );
        return thread->queue != NULL;
    }
    if (queue->input)
    {
        queue->input->cursor_count -= queue->cursor_count;
        release_object( queue->input );
    }
    queue->input = (struct thread_input *)grab_object( new_input );
    new_input->cursor_count += queue->cursor_count;
    return 1;
}

/* set the cursor position and queue the corresponding mouse message */
static void set_cursor_pos( struct desktop *desktop, int x, int y )
{
    struct hardware_msg_data *msg_data;
    struct message *msg;

    if (!(msg = mem_alloc( sizeof(*msg) ))) return;
    if (!(msg_data = mem_alloc( sizeof(*msg_data) )))
    {
        free( msg );
        return;
    }
    memset( msg_data, 0, sizeof(*msg_data) );

    msg->type      = MSG_HARDWARE;
    msg->win       = 0;
    msg->msg       = WM_MOUSEMOVE;
    msg->wparam    = 0;
    msg->lparam    = 0;
    msg->time      = get_tick_count();
    msg->result    = NULL;
    msg->data      = msg_data;
    msg->data_size = sizeof(*msg_data);
    msg_data->x    = x;
    msg_data->y    = y;
    queue_hardware_message( desktop, msg, 1 );
}

/* set the cursor clip rectangle */
static void set_clip_rectangle( struct desktop *desktop, const rectangle_t *rect )
{
    rectangle_t top_rect;
    int x, y;

    get_top_window_rectangle( desktop, &top_rect );
    if (rect)
    {
        rectangle_t new_rect = *rect;
        if (new_rect.left   < top_rect.left)   new_rect.left   = top_rect.left;
        if (new_rect.right  > top_rect.right)  new_rect.right  = top_rect.right;
        if (new_rect.top    < top_rect.top)    new_rect.top    = top_rect.top;
        if (new_rect.bottom > top_rect.bottom) new_rect.bottom = top_rect.bottom;
        if (new_rect.left > new_rect.right || new_rect.top > new_rect.bottom) new_rect = top_rect;
        desktop->cursor.clip = new_rect;
    }
    else desktop->cursor.clip = top_rect;

    if (desktop->cursor.clip_msg)
        post_desktop_message( desktop, desktop->cursor.clip_msg, rect != NULL, 0 );

    /* warp the mouse to be inside the clip rect */
    x = min( max( desktop->cursor.x, desktop->cursor.clip.left ), desktop->cursor.clip.right-1 );
    y = min( max( desktop->cursor.y, desktop->cursor.clip.top ), desktop->cursor.clip.bottom-1 );
    if (x != desktop->cursor.x || y != desktop->cursor.y) set_cursor_pos( desktop, x, y );
}

/* change the foreground input and reset the cursor clip rect */
static void set_foreground_input( struct desktop *desktop, struct thread_input *input )
{
    if (desktop->foreground_input == input) return;
    set_clip_rectangle( desktop, NULL );
    desktop->foreground_input = input;
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

/* check the queue status */
static inline int is_signaled( struct msg_queue *queue )
{
    return ((queue->wake_bits & queue->wake_mask) || (queue->changed_bits & queue->changed_mask));
}

/* set some queue bits */
static inline void set_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    queue->wake_bits |= bits;
    queue->changed_bits |= bits;
    if (is_signaled( queue )) wake_up( &queue->obj, 0 );
}

/* clear some queue bits */
static inline void clear_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    queue->wake_bits &= ~bits;
    queue->changed_bits &= ~bits;
}

/* check whether msg is a keyboard message */
static inline int is_keyboard_msg( struct message *msg )
{
    return (msg->msg >= WM_KEYFIRST && msg->msg <= WM_KEYLAST);
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
static inline int get_hardware_msg_bit( struct message *msg )
{
    if (msg->msg == WM_INPUT_DEVICE_CHANGE || msg->msg == WM_INPUT) return QS_RAWINPUT;
    if (msg->msg == WM_MOUSEMOVE || msg->msg == WM_NCMOUSEMOVE) return QS_MOUSEMOVE;
    if (is_keyboard_msg( msg )) return QS_KEY;
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

/* try to merge a message with the last in the list; return 1 if successful */
static int merge_message( struct thread_input *input, const struct message *msg )
{
    struct message *prev;
    struct list *ptr;

    if (msg->msg != WM_MOUSEMOVE) return 0;
    for (ptr = list_tail( &input->msg_list ); ptr; ptr = list_prev( &input->msg_list, ptr ))
    {
        prev = LIST_ENTRY( ptr, struct message, entry );
        if (prev->msg != WM_INPUT) break;
    }
    if (!ptr) return 0;
    if (prev->result) return 0;
    if (prev->win && msg->win && prev->win != msg->win) return 0;
    if (prev->msg != msg->msg) return 0;
    if (prev->type != msg->type) return 0;
    /* now we can merge it */
    prev->wparam  = msg->wparam;
    prev->lparam  = msg->lparam;
    prev->time    = msg->time;
    if (msg->type == MSG_HARDWARE && prev->data && msg->data)
    {
        struct hardware_msg_data *prev_data = prev->data;
        struct hardware_msg_data *msg_data = msg->data;
        prev_data->x     = msg_data->x;
        prev_data->y     = msg_data->y;
        prev_data->info  = msg_data->info;
    }
    list_remove( ptr );
    list_add_tail( &input->msg_list, ptr );
    return 1;
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
        else if (send_queue) list_add_head( &send_queue->send_result, &result->sender_entry );

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
        reply->time   = get_tick_count();

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
    struct process *process = get_wait_queue_thread(entry)->process;

    /* a thread can only wait on its own queue */
    if (get_wait_queue_thread(entry)->queue != queue)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if (process->idle_event && !(queue->wake_mask & QS_SMRESULT)) set_event( process->idle_event );

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
    fprintf( stderr, "Msg queue bits=%x mask=%x\n",
             queue->wake_bits, queue->wake_mask );
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
    queue->wake_mask = queue->wake_get_msg;
    queue->changed_mask = queue->changed_get_msg;
}

static void msg_queue_destroy( struct object *obj )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct list *ptr;
    struct hotkey *hotkey, *hotkey2;
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
    queue->input->cursor_count -= queue->cursor_count;
    release_object( queue->input );
    if (queue->hooks) release_object( queue->hooks );
    if (queue->fd) release_object( queue->fd );
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
    fprintf( stderr, "Thread input focus=%08x capture=%08x active=%08x\n",
             input->focus, input->capture, input->active );
}

static void thread_input_destroy( struct object *obj )
{
    struct thread_input *input = (struct thread_input *)obj;

    empty_msg_list( &input->msg_list );
    if (input->desktop)
    {
        if (input->desktop->foreground_input == input) set_foreground_input( input->desktop, NULL );
        release_object( input->desktop );
    }
}

/* fix the thread input data when a window is destroyed */
static inline void thread_input_cleanup_window( struct msg_queue *queue, user_handle_t window )
{
    struct thread_input *input = queue->input;

    if (window == input->focus) input->focus = 0;
    if (window == input->capture) input->capture = 0;
    if (window == input->active) input->active = 0;
    if (window == input->menu_owner) input->menu_owner = 0;
    if (window == input->move_size) input->move_size = 0;
    if (window == input->caret) set_caret_window( input, 0 );
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
    struct thread_input *input;
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

    ret = assign_thread_input( thread_from, input );
    if (ret) memset( input->keystate, 0, sizeof(input->keystate) );
    release_object( input );
    return ret;
}

/* detach two thread input data structures */
void detach_thread_input( struct thread *thread_from )
{
    struct thread_input *input;

    if ((input = create_thread_input( thread_from )))
    {
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
        queue->timeout = add_timeout_user( timer->when, timer_callback, queue );
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
        if (t->when >= timer->when) break;
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
    while (timer->when <= current_time) timer->when += (timeout_t)timer->rate * 10000;
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
        timer->when = current_time + (timeout_t)timer->rate * 10000;
        link_timer( queue, timer );
        /* check if we replaced the next timer */
        if (list_head( &queue->pending_timers ) == &timer->entry) set_next_timer( queue );
    }
    return timer;
}

/* change the input key state for a given key */
static void set_input_key_state( unsigned char *keystate, unsigned char key, int down )
{
    if (down)
    {
        if (!(keystate[key] & 0x80)) keystate[key] ^= 0x01;
        keystate[key] |= down;
    }
    else keystate[key] &= ~0x80;
}

/* update the input key state for a keyboard message */
static void update_input_key_state( struct desktop *desktop, unsigned char *keystate,
                                    const struct message *msg )
{
    unsigned char key;
    int down = 0;

    switch (msg->msg)
    {
    case WM_LBUTTONDOWN:
        down = (keystate == desktop->keystate) ? 0xc0 : 0x80;
        /* fall through */
    case WM_LBUTTONUP:
        set_input_key_state( keystate, VK_LBUTTON, down );
        break;
    case WM_MBUTTONDOWN:
        down = (keystate == desktop->keystate) ? 0xc0 : 0x80;
        /* fall through */
    case WM_MBUTTONUP:
        set_input_key_state( keystate, VK_MBUTTON, down );
        break;
    case WM_RBUTTONDOWN:
        down = (keystate == desktop->keystate) ? 0xc0 : 0x80;
        /* fall through */
    case WM_RBUTTONUP:
        set_input_key_state( keystate, VK_RBUTTON, down );
        break;
    case WM_XBUTTONDOWN:
        down = (keystate == desktop->keystate) ? 0xc0 : 0x80;
        /* fall through */
    case WM_XBUTTONUP:
        if (msg->wparam >> 16 == XBUTTON1) set_input_key_state( keystate, VK_XBUTTON1, down );
        else if (msg->wparam >> 16 == XBUTTON2) set_input_key_state( keystate, VK_XBUTTON2, down );
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = (keystate == desktop->keystate) ? 0xc0 : 0x80;
        /* fall through */
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = (unsigned char)msg->wparam;
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

/* release the hardware message currently being processed by the given thread */
static void release_hardware_message( struct msg_queue *queue, unsigned int hw_id,
                                      int remove, user_handle_t new_win )
{
    struct thread_input *input = queue->input;
    struct message *msg;

    LIST_FOR_EACH_ENTRY( msg, &input->msg_list, struct message, entry )
    {
        if (msg->unique_id == hw_id) break;
    }
    if (&msg->entry == &input->msg_list) return;  /* not found */

    /* clear the queue bit for that message */
    if (remove || new_win)
    {
        struct message *other;
        int clr_bit;

        clr_bit = get_hardware_msg_bit( msg );
        LIST_FOR_EACH_ENTRY( other, &input->msg_list, struct message, entry )
        {
            if (other != msg && get_hardware_msg_bit( other ) == clr_bit)
            {
                clr_bit = 0;
                break;
            }
        }
        if (clr_bit) clear_queue_bits( queue, clr_bit );
    }

    if (new_win)  /* set the new window */
    {
        struct thread *owner = get_window_thread( new_win );
        if (owner)
        {
            msg->win = new_win;
            if (owner->queue->input != input)
            {
                list_remove( &msg->entry );
                if (merge_message( owner->queue->input, msg ))
                {
                    free_message( msg );
                    release_object( owner );
                    return;
                }
                list_add_tail( &owner->queue->input->msg_list, &msg->entry );
            }
            set_queue_bits( owner->queue, get_hardware_msg_bit( msg ));
            remove = 0;
            release_object( owner );
        }
    }
    if (remove)
    {
        update_input_key_state( input->desktop, input->keystate, msg );
        list_remove( &msg->entry );
        free_message( msg );
    }
}

static int queue_hotkey_message( struct desktop *desktop, struct message *msg )
{
    struct hotkey *hotkey;
    unsigned int modifiers = 0;

    if (msg->msg != WM_KEYDOWN) return 0;

    if (desktop->keystate[VK_MENU] & 0x80) modifiers |= MOD_ALT;
    if (desktop->keystate[VK_CONTROL] & 0x80) modifiers |= MOD_CONTROL;
    if (desktop->keystate[VK_SHIFT] & 0x80) modifiers |= MOD_SHIFT;
    if ((desktop->keystate[VK_LWIN] & 0x80) || (desktop->keystate[VK_RWIN] & 0x80)) modifiers |= MOD_WIN;

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
                                                   struct message *msg, unsigned int *msg_code )
{
    struct hardware_msg_data *data = msg->data;
    user_handle_t win = 0;

    *msg_code = msg->msg;
    if (msg->msg == WM_INPUT)
    {
        if (!(win = msg->win) && input) win = input->focus;
    }
    else if (is_keyboard_msg( msg ))
    {
        if (input && !(win = input->focus))
        {
            win = input->active;
            if (*msg_code < WM_SYSKEYDOWN) *msg_code += WM_SYSKEYDOWN - WM_KEYDOWN;
        }
    }
    else  /* mouse message */
    {
        if (!input || !(win = input->capture))
        {
            if (!(win = msg->win) || !is_window_visible( win ) || is_window_transparent( win ))
                win = window_from_point( desktop, data->x, data->y );
        }
    }
    return win;
}

static struct rawinput_device_entry *find_rawinput_device( unsigned short usage_page, unsigned short usage )
{
    struct rawinput_device_entry *e;

    LIST_FOR_EACH_ENTRY( e, &current->process->rawinput_devices, struct rawinput_device_entry, entry )
    {
        if (e->device.usage_page != usage_page || e->device.usage != usage) continue;
        return e;
    }

    return NULL;
}

static void update_rawinput_device(const struct rawinput_device *device)
{
    struct rawinput_device_entry *e;

    if (!(e = find_rawinput_device( device->usage_page, device->usage )))
    {
        if (!(e = mem_alloc( sizeof(*e) ))) return;
        list_add_tail( &current->process->rawinput_devices, &e->entry );
    }

    if (device->flags & RIDEV_REMOVE)
    {
        list_remove( &e->entry );
        free( e );
        return;
    }

    e->device = *device;
    e->device.target = get_user_full_handle( e->device.target );
}

/* queue a hardware message into a given thread input */
static void queue_hardware_message( struct desktop *desktop, struct message *msg, int always_queue )
{
    user_handle_t win;
    struct thread *thread;
    struct thread_input *input;
    unsigned int msg_code;
    struct hardware_msg_data *data = msg->data;

    update_input_key_state( desktop, desktop->keystate, msg );
    last_input_time = get_tick_count();
    if (msg->msg != WM_MOUSEMOVE) always_queue = 1;

    if (is_keyboard_msg( msg ))
    {
        if (queue_hotkey_message( desktop, msg )) return;
        if (desktop->keystate[VK_MENU] & 0x80) msg->lparam |= KF_ALTDOWN << 16;
        if (msg->wparam == VK_SHIFT || msg->wparam == VK_LSHIFT || msg->wparam == VK_RSHIFT)
            msg->lparam &= ~(KF_EXTENDED << 16);
    }
    else if (msg->msg != WM_INPUT)
    {
        if (msg->msg == WM_MOUSEMOVE)
        {
            int x = min( max( data->x, desktop->cursor.clip.left ), desktop->cursor.clip.right-1 );
            int y = min( max( data->y, desktop->cursor.clip.top ), desktop->cursor.clip.bottom-1 );
            if (desktop->cursor.x != x || desktop->cursor.y != y) always_queue = 1;
            desktop->cursor.x = x;
            desktop->cursor.y = y;
            desktop->cursor.last_change = get_tick_count();
        }
        if (desktop->keystate[VK_LBUTTON] & 0x80)  msg->wparam |= MK_LBUTTON;
        if (desktop->keystate[VK_MBUTTON] & 0x80)  msg->wparam |= MK_MBUTTON;
        if (desktop->keystate[VK_RBUTTON] & 0x80)  msg->wparam |= MK_RBUTTON;
        if (desktop->keystate[VK_SHIFT] & 0x80)    msg->wparam |= MK_SHIFT;
        if (desktop->keystate[VK_CONTROL] & 0x80)  msg->wparam |= MK_CONTROL;
        if (desktop->keystate[VK_XBUTTON1] & 0x80) msg->wparam |= MK_XBUTTON1;
        if (desktop->keystate[VK_XBUTTON2] & 0x80) msg->wparam |= MK_XBUTTON2;
    }
    data->x = desktop->cursor.x;
    data->y = desktop->cursor.y;

    if (msg->win && (thread = get_window_thread( msg->win )))
    {
        input = thread->queue->input;
        release_object( thread );
    }
    else input = desktop->foreground_input;

    win = find_hardware_message_window( desktop, input, msg, &msg_code );
    if (!win || !(thread = get_window_thread(win)))
    {
        if (input) update_input_key_state( input->desktop, input->keystate, msg );
        free_message( msg );
        return;
    }
    input = thread->queue->input;

    if (win != desktop->cursor.win) always_queue = 1;
    desktop->cursor.win = win;

    if (!always_queue || merge_message( input, msg )) free_message( msg );
    else
    {
        msg->unique_id = 0;  /* will be set once we return it to the app */
        list_add_tail( &input->msg_list, &msg->entry );
        set_queue_bits( thread->queue, get_hardware_msg_bit(msg) );
    }
    release_object( thread );
}

/* send the low-level hook message for a given hardware message */
static int send_hook_ll_message( struct desktop *desktop, struct message *hardware_msg,
                                 const hw_input_t *input, struct msg_queue *sender )
{
    struct thread *hook_thread;
    struct msg_queue *queue;
    struct message *msg;
    timeout_t timeout = 2000 * -10000;  /* FIXME: load from registry */
    int id = (input->type == INPUT_MOUSE) ? WH_MOUSE_LL : WH_KEYBOARD_LL;

    if (!(hook_thread = get_first_global_hook( id ))) return 0;
    if (!(queue = hook_thread->queue)) return 0;
    if (is_queue_hung( queue )) return 0;

    if (!(msg = mem_alloc( sizeof(*msg) ))) return 0;

    msg->type      = MSG_HOOK_LL;
    msg->win       = 0;
    msg->msg       = id;
    msg->wparam    = hardware_msg->msg;
    msg->time      = hardware_msg->time;
    msg->data_size = hardware_msg->data_size;
    msg->result    = NULL;

    if (input->type == INPUT_KEYBOARD)
    {
        unsigned short vkey = input->kbd.vkey;
        if (input->kbd.flags & KEYEVENTF_UNICODE) vkey = VK_PACKET;
        msg->lparam = (input->kbd.scan << 16) | vkey;
    }
    else msg->lparam = input->mouse.data << 16;

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

/* queue a hardware message for a mouse event */
static int queue_mouse_message( struct desktop *desktop, user_handle_t win, const hw_input_t *input,
                                unsigned int hook_flags, struct msg_queue *sender )
{
    const struct rawinput_device *device;
    struct hardware_msg_data *msg_data;
    struct message *msg;
    unsigned int i, time, flags;
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

    desktop->cursor.last_change = get_tick_count();
    flags = input->mouse.flags;
    time  = input->mouse.time;
    if (!time) time = desktop->cursor.last_change;

    if (flags & MOUSEEVENTF_MOVE)
    {
        if (flags & MOUSEEVENTF_ABSOLUTE)
        {
            x = input->mouse.x;
            y = input->mouse.y;
            if (flags & ~(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE) &&
                x == desktop->cursor.x && y == desktop->cursor.y)
                flags &= ~MOUSEEVENTF_MOVE;
        }
        else
        {
            x = desktop->cursor.x + input->mouse.x;
            y = desktop->cursor.y + input->mouse.y;
        }
    }
    else
    {
        x = desktop->cursor.x;
        y = desktop->cursor.y;
    }

    if ((device = current->process->rawinput_mouse))
    {
        if (!(msg = mem_alloc( sizeof(*msg) ))) return 0;
        if (!(msg_data = mem_alloc( sizeof(*msg_data) )))
        {
            free( msg );
            return 0;
        }

        msg->type      = MSG_HARDWARE;
        msg->win       = device->target;
        msg->msg       = WM_INPUT;
        msg->wparam    = RIM_INPUT;
        msg->lparam    = 0;
        msg->time      = time;
        msg->data      = msg_data;
        msg->data_size = sizeof(*msg_data);
        msg->result    = NULL;

        msg_data->info                = input->mouse.info;
        msg_data->flags               = flags;
        msg_data->rawinput.type       = RIM_TYPEMOUSE;
        msg_data->rawinput.mouse.x    = x - desktop->cursor.x;
        msg_data->rawinput.mouse.y    = y - desktop->cursor.y;
        msg_data->rawinput.mouse.data = input->mouse.data;

        queue_hardware_message( desktop, msg, 0 );
    }

    for (i = 0; i < sizeof(messages)/sizeof(messages[0]); i++)
    {
        if (!messages[i]) continue;
        if (!(flags & (1 << i))) continue;
        flags &= ~(1 << i);

        if (!(msg = mem_alloc( sizeof(*msg) ))) return 0;
        if (!(msg_data = mem_alloc( sizeof(*msg_data) )))
        {
            free( msg );
            return 0;
        }
        memset( msg_data, 0, sizeof(*msg_data) );

        msg->type      = MSG_HARDWARE;
        msg->win       = get_user_full_handle( win );
        msg->msg       = messages[i];
        msg->wparam    = input->mouse.data << 16;
        msg->lparam    = 0;
        msg->time      = time;
        msg->result    = NULL;
        msg->data      = msg_data;
        msg->data_size = sizeof(*msg_data);
        msg_data->x    = x;
        msg_data->y    = y;
        msg_data->info = input->mouse.info;
        if (hook_flags & SEND_HWMSG_INJECTED) msg_data->flags = LLMHF_INJECTED;

        /* specify a sender only when sending the last message */
        if (!(flags & ((1 << sizeof(messages)/sizeof(messages[0])) - 1)))
        {
            if (!(wait = send_hook_ll_message( desktop, msg, input, sender )))
                queue_hardware_message( desktop, msg, 0 );
        }
        else if (!send_hook_ll_message( desktop, msg, input, NULL ))
            queue_hardware_message( desktop, msg, 0 );
    }
    return wait;
}

/* queue a hardware message for a keyboard event */
static int queue_keyboard_message( struct desktop *desktop, user_handle_t win, const hw_input_t *input,
                                   unsigned int hook_flags, struct msg_queue *sender )
{
    const struct rawinput_device *device;
    struct hardware_msg_data *msg_data;
    struct message *msg;
    unsigned char vkey = input->kbd.vkey;
    unsigned int message_code, time;
    int wait;

    if (!(time = input->kbd.time)) time = get_tick_count();

    if (!(input->kbd.flags & KEYEVENTF_UNICODE))
    {
        switch (vkey)
        {
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
            vkey = (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) ? VK_RMENU : VK_LMENU;
            break;
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
            vkey = (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) ? VK_RCONTROL : VK_LCONTROL;
            break;
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            vkey = (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) ? VK_RSHIFT : VK_LSHIFT;
            break;
        }
    }

    message_code = (input->kbd.flags & KEYEVENTF_KEYUP) ? WM_KEYUP : WM_KEYDOWN;
    switch (vkey)
    {
    case VK_LMENU:
    case VK_RMENU:
        if (input->kbd.flags & KEYEVENTF_KEYUP)
        {
            /* send WM_SYSKEYUP if Alt still pressed and no other key in between */
            /* we use 0x02 as a flag to track if some other SYSKEYUP was sent already */
            if ((desktop->keystate[VK_MENU] & 0x82) != 0x82) break;
            message_code = WM_SYSKEYUP;
            desktop->keystate[VK_MENU] &= ~0x02;
        }
        else
        {
            /* send WM_SYSKEYDOWN for Alt except with Ctrl */
            if (desktop->keystate[VK_CONTROL] & 0x80) break;
            message_code = WM_SYSKEYDOWN;
            desktop->keystate[VK_MENU] |= 0x02;
        }
        break;

    case VK_LCONTROL:
    case VK_RCONTROL:
        /* send WM_SYSKEYUP on release if Alt still pressed */
        if (!(input->kbd.flags & KEYEVENTF_KEYUP)) break;
        if (!(desktop->keystate[VK_MENU] & 0x80)) break;
        message_code = WM_SYSKEYUP;
        desktop->keystate[VK_MENU] &= ~0x02;
        break;

    default:
        /* send WM_SYSKEY for Alt-anykey and for F10 */
        if (desktop->keystate[VK_CONTROL] & 0x80) break;
        if (!(desktop->keystate[VK_MENU] & 0x80)) break;
        /* fall through */
    case VK_F10:
        message_code = (input->kbd.flags & KEYEVENTF_KEYUP) ? WM_SYSKEYUP : WM_SYSKEYDOWN;
        desktop->keystate[VK_MENU] &= ~0x02;
        break;
    }

    if ((device = current->process->rawinput_kbd))
    {
        if (!(msg = mem_alloc( sizeof(*msg) ))) return 0;
        if (!(msg_data = mem_alloc( sizeof(*msg_data) )))
        {
            free( msg );
            return 0;
        }

        msg->type      = MSG_HARDWARE;
        msg->win       = device->target;
        msg->msg       = WM_INPUT;
        msg->wparam    = RIM_INPUT;
        msg->lparam    = 0;
        msg->time      = time;
        msg->data      = msg_data;
        msg->data_size = sizeof(*msg_data);
        msg->result    = NULL;

        msg_data->info                 = input->kbd.info;
        msg_data->flags                = input->kbd.flags;
        msg_data->rawinput.type        = RIM_TYPEKEYBOARD;
        msg_data->rawinput.kbd.message = message_code;
        msg_data->rawinput.kbd.vkey    = vkey;
        msg_data->rawinput.kbd.scan    = input->kbd.scan;

        queue_hardware_message( desktop, msg, 0 );
    }

    if (!(msg = mem_alloc( sizeof(*msg) ))) return 0;
    if (!(msg_data = mem_alloc( sizeof(*msg_data) )))
    {
        free( msg );
        return 0;
    }
    memset( msg_data, 0, sizeof(*msg_data) );

    msg->type      = MSG_HARDWARE;
    msg->win       = get_user_full_handle( win );
    msg->msg       = message_code;
    msg->lparam    = (input->kbd.scan << 16) | 1u; /* repeat count */
    msg->time      = time;
    msg->result    = NULL;
    msg->data      = msg_data;
    msg->data_size = sizeof(*msg_data);
    msg_data->info = input->kbd.info;
    if (hook_flags & SEND_HWMSG_INJECTED) msg_data->flags = LLKHF_INJECTED;

    if (input->kbd.flags & KEYEVENTF_UNICODE)
    {
        msg->wparam = VK_PACKET;
    }
    else
    {
        unsigned int flags = 0;
        if (input->kbd.flags & KEYEVENTF_EXTENDEDKEY) flags |= KF_EXTENDED;
        /* FIXME: set KF_DLGMODE and KF_MENUMODE when needed */
        if (input->kbd.flags & KEYEVENTF_KEYUP) flags |= KF_REPEAT | KF_UP;
        else if (desktop->keystate[vkey] & 0x80) flags |= KF_REPEAT;

        msg->wparam = vkey;
        msg->lparam |= flags << 16;
        msg_data->flags |= (flags & (KF_EXTENDED | KF_ALTDOWN | KF_UP)) >> 8;
    }

    if (!(wait = send_hook_ll_message( desktop, msg, input, sender )))
        queue_hardware_message( desktop, msg, 1 );

    return wait;
}

/* queue a hardware message for a custom type of event */
static void queue_custom_hardware_message( struct desktop *desktop, user_handle_t win,
                                           const hw_input_t *input )
{
    struct hardware_msg_data *msg_data;
    struct message *msg;

    if (!(msg = mem_alloc( sizeof(*msg) ))) return;
    if (!(msg_data = mem_alloc( sizeof(*msg_data) )))
    {
        free( msg );
        return;
    }
    memset( msg_data, 0, sizeof(*msg_data) );

    msg->type      = MSG_HARDWARE;
    msg->win       = get_user_full_handle( win );
    msg->msg       = input->hw.msg;
    msg->wparam    = 0;
    msg->lparam    = input->hw.lparam;
    msg->time      = get_tick_count();
    msg->result    = NULL;
    msg->data      = msg_data;
    msg->data_size = sizeof(*msg_data);

    queue_hardware_message( desktop, msg, 1 );
}

/* check message filter for a hardware message */
static int check_hw_message_filter( user_handle_t win, unsigned int msg_code,
                                    user_handle_t filter_win, unsigned int first, unsigned int last )
{
    if (msg_code >= WM_KEYFIRST && msg_code <= WM_KEYLAST)
    {
        /* we can only test the window for a keyboard message since the
         * dest window for a mouse message depends on hittest */
        if (filter_win && win != filter_win && !is_child_window( filter_win, win ))
            return 0;
        /* the message code is final for a keyboard message, we can simply check it */
        return check_msg_filter( msg_code, first, last );
    }
    else  /* mouse message */
    {
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
    }
}


/* find a hardware message for the given queue */
static int get_hardware_message( struct thread *thread, unsigned int hw_id, user_handle_t filter_win,
                                 unsigned int first, unsigned int last, unsigned int flags,
                                 struct get_message_reply *reply )
{
    struct thread_input *input = thread->queue->input;
    struct thread *win_thread;
    struct list *ptr;
    user_handle_t win;
    int clear_bits, got_one = 0;
    unsigned int msg_code;

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
        win = find_hardware_message_window( input->desktop, input, msg, &msg_code );
        if (!win || !(win_thread = get_window_thread( win )))
        {
            /* no window at all, remove it */
            update_input_key_state( input->desktop, input->keystate, msg );
            list_remove( &msg->entry );
            free_message( msg );
            continue;
        }
        if (win_thread != thread)
        {
            if (win_thread->queue->input == input)
            {
                /* wake the other thread */
                set_queue_bits( win_thread->queue, get_hardware_msg_bit(msg) );
                got_one = 1;
            }
            else
            {
                /* for another thread input, drop it */
                update_input_key_state( input->desktop, input->keystate, msg );
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
            clear_bits &= ~get_hardware_msg_bit( msg );
            continue;
        }
        /* now we can return it */
        if (!msg->unique_id) msg->unique_id = get_unique_id();
        reply->type   = MSG_HARDWARE;
        reply->win    = win;
        reply->msg    = msg_code;
        reply->wparam = msg->wparam;
        reply->lparam = msg->lparam;
        reply->time   = msg->time;

        data->hw_id = msg->unique_id;
        set_reply_data( msg->data, msg->data_size );
        if (msg->msg == WM_INPUT && (flags & PM_REMOVE))
            release_hardware_message( current->queue, data->hw_id, 1, 0 );
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

/* post a message to a window; used by socket handling */
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
        msg->time      = get_tick_count();
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = 0;

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


/* get the message queue of the current thread */
DECL_HANDLER(get_msg_queue)
{
    struct msg_queue *queue = get_current_queue();

    reply->handle = 0;
    if (queue) reply->handle = alloc_handle( current->process, queue, SYNCHRONIZE, 0 );
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
        queue->wake_mask    = req->wake_mask;
        queue->changed_mask = req->changed_mask;
        reply->wake_bits    = queue->wake_bits;
        reply->changed_bits = queue->changed_bits;
        if (is_signaled( queue ))
        {
            /* if skip wait is set, do what would have been done in the subsequent wait */
            if (req->skip_wait)
            {
                queue->wake_mask = queue->wake_get_msg;
                queue->changed_mask = queue->changed_get_msg;
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
        reply->wake_bits    = queue->wake_bits;
        reply->changed_bits = queue->changed_bits;
        if (req->clear) queue->changed_bits = 0;
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
        msg->time      = get_tick_count();
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = get_req_data_size();

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
    struct thread *thread = NULL;
    struct desktop *desktop;
    struct msg_queue *sender = get_current_queue();
    data_size_t size = min( 256, get_reply_max_size() );

    if (!(desktop = get_thread_desktop( current, 0 ))) return;

    if (req->win)
    {
        if (!(thread = get_window_thread( req->win ))) return;
        if (desktop != thread->queue->input->desktop)
        {
            /* don't allow queuing events to a different desktop */
            release_object( desktop );
            return;
        }
    }

    reply->prev_x = desktop->cursor.x;
    reply->prev_y = desktop->cursor.y;

    switch (req->input.type)
    {
    case INPUT_MOUSE:
        reply->wait = queue_mouse_message( desktop, req->win, &req->input, req->flags, sender );
        break;
    case INPUT_KEYBOARD:
        reply->wait = queue_keyboard_message( desktop, req->win, &req->input, req->flags, sender );
        break;
    case INPUT_HARDWARE:
        queue_custom_hardware_message( desktop, req->win, &req->input );
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
    }
    if (thread) release_object( thread );

    reply->new_x = desktop->cursor.x;
    reply->new_y = desktop->cursor.y;
    set_reply_data( desktop->keystate, size );
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
    unsigned int filter = req->flags >> 16;

    reply->active_hooks = get_active_hooks();

    if (!queue) return;
    queue->last_get_msg = current_time;
    queue->wake_get_msg = queue->changed_get_msg = 0;
    if (!filter) filter = QS_ALLINPUT;

    /* first check for sent messages */
    if ((ptr = list_head( &queue->msg_list[SEND_MESSAGE] )))
    {
        struct message *msg = LIST_ENTRY( ptr, struct message, entry );
        receive_message( queue, msg, reply );
        return;
    }

    /* clear changed bits so we can wait on them if we don't find a message */
    if (filter & QS_POSTMESSAGE)
    {
        queue->changed_bits &= ~(QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER);
        if (req->get_first == 0 && req->get_last == ~0U) queue->changed_bits &= ~QS_ALLPOSTMESSAGE;
    }
    if (filter & QS_INPUT) queue->changed_bits &= ~QS_INPUT;
    if (filter & QS_PAINT) queue->changed_bits &= ~QS_PAINT;

    /* then check for posted messages */
    if ((filter & QS_POSTMESSAGE) &&
        get_posted_message( queue, get_win, req->get_first, req->get_last, req->flags, reply ))
        return;

    if ((filter & QS_HOTKEY) && queue->hotkey_count &&
        req->get_first <= WM_HOTKEY && req->get_last >= WM_HOTKEY &&
        get_posted_message( queue, get_win, WM_HOTKEY, WM_HOTKEY, req->flags, reply ))
        return;

    /* only check for quit messages if not posted messages pending.
     * note: the quit message isn't filtered */
    if (get_quit_message( queue, req->flags, reply ))
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
        reply->time   = get_tick_count();
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
        reply->time   = get_tick_count();
        if (!(req->flags & PM_NOYIELD) && current->process->idle_event)
            set_event( current->process->idle_event );
        return;
    }

    if (get_win == -1 && current->process->idle_event) set_event( current->process->idle_event );
    queue->wake_mask = queue->wake_get_msg = req->wake_mask;
    queue->changed_mask = queue->changed_get_msg = req->changed_mask;
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
        release_hardware_message( current->queue, req->hw_id, req->remove, req->new_win );
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
                if (!result->replied) clear_queue_bits( queue, QS_SMRESULT );
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
            /* find a free id for it */
            do
            {
                id = queue->next_timer_id;
                if (--queue->next_timer_id <= 0x100) queue->next_timer_id = 0x7fff;
            }
            while (find_timer( queue, 0, req->msg, id ));
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
        if (!get_user_object_handle( &win_handle, USER_WINDOW ))
        {
            release_object( desktop );
            set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
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
        if (!get_user_object_handle( &win_handle, USER_WINDOW ))
        {
            release_object( desktop );
            set_win32_error( ERROR_INVALID_WINDOW_HANDLE );
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
        if (req->attach) attach_thread_input( thread_from, thread_to );
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


/* get thread input data */
DECL_HANDLER(get_thread_input)
{
    struct thread *thread = NULL;
    struct desktop *desktop;
    struct thread_input *input;

    if (req->tid)
    {
        if (!(thread = get_thread_from_id( req->tid ))) return;
        if (!(desktop = get_thread_desktop( thread, 0 )))
        {
            release_object( thread );
            return;
        }
        input = thread->queue ? thread->queue->input : NULL;
    }
    else
    {
        if (!(desktop = get_thread_desktop( current, 0 ))) return;
        input = desktop->foreground_input;  /* get the foreground thread info */
    }

    if (input)
    {
        reply->focus      = input->focus;
        reply->capture    = input->capture;
        reply->active     = input->active;
        reply->menu_owner = input->menu_owner;
        reply->move_size  = input->move_size;
        reply->caret      = input->caret;
        reply->cursor     = input->cursor;
        reply->show_count = input->cursor_count;
        reply->rect       = input->caret_rect;
    }

    /* foreground window is active window of foreground thread */
    reply->foreground = desktop->foreground_input ? desktop->foreground_input->active : 0;
    if (thread) release_object( thread );
    release_object( desktop );
}


/* retrieve queue keyboard state for a given thread */
DECL_HANDLER(get_key_state)
{
    struct thread *thread;
    struct desktop *desktop;
    data_size_t size = min( 256, get_reply_max_size() );

    if (!req->tid)  /* get global async key state */
    {
        if (!(desktop = get_thread_desktop( current, 0 ))) return;
        if (req->key >= 0)
        {
            reply->state = desktop->keystate[req->key & 0xff];
            desktop->keystate[req->key & 0xff] &= ~0x40;
        }
        set_reply_data( desktop->keystate, size );
        release_object( desktop );
    }
    else
    {
        if (!(thread = get_thread_from_id( req->tid ))) return;
        if (thread->queue)
        {
            if (req->key >= 0) reply->state = thread->queue->input->keystate[req->key & 0xff];
            set_reply_data( thread->queue->input->keystate, size );
        }
        release_object( thread );
    }
}


/* set queue keyboard state for a given thread */
DECL_HANDLER(set_key_state)
{
    struct thread *thread;
    struct desktop *desktop;
    data_size_t size = min( 256, get_req_data_size() );

    if (!req->tid)  /* set global async key state */
    {
        if (!(desktop = get_thread_desktop( current, 0 ))) return;
        memcpy( desktop->keystate, get_req_data(), size );
        release_object( desktop );
    }
    else
    {
        if (!(thread = get_thread_from_id( req->tid ))) return;
        if (thread->queue) memcpy( thread->queue->input->keystate, get_req_data(), size );
        if (req->async && (desktop = get_thread_desktop( thread, 0 )))
        {
            memcpy( desktop->keystate, get_req_data(), size );
            release_object( desktop );
        }
        release_object( thread );
    }
}


/* set the system foreground window */
DECL_HANDLER(set_foreground_window)
{
    struct thread *thread = NULL;
    struct desktop *desktop;
    struct msg_queue *queue = get_current_queue();

    if (!(desktop = get_thread_desktop( current, 0 ))) return;
    reply->previous = desktop->foreground_input ? desktop->foreground_input->active : 0;
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
        reply->previous = queue->input->focus;
        queue->input->focus = get_user_full_handle( req->handle );
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
            reply->previous = queue->input->active;
            queue->input->active = get_user_full_handle( req->handle );
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

        /* if in menu mode, reject all requests to change focus, except if the menu bit is set */
        if (input->menu_owner && !(req->flags & CAPTURE_MENU))
        {
            set_error(STATUS_ACCESS_DENIED);
            return;
        }
        reply->previous = input->capture;
        input->capture = get_user_full_handle( req->handle );
        input->menu_owner = (req->flags & CAPTURE_MENU) ? input->capture : 0;
        input->move_size = (req->flags & CAPTURE_MOVESIZE) ? input->capture : 0;
        reply->full_handle = input->capture;
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

        reply->previous  = input->caret;
        reply->old_rect  = input->caret_rect;
        reply->old_hide  = input->caret_hide;
        reply->old_state = input->caret_state;

        set_caret_window( input, get_user_full_handle(req->handle) );
        input->caret_rect.right  = input->caret_rect.left + req->width;
        input->caret_rect.bottom = input->caret_rect.top + req->height;
    }
}


/* Set the current thread caret information */
DECL_HANDLER(set_caret_info)
{
    struct msg_queue *queue = get_current_queue();
    struct thread_input *input;

    if (!queue) return;
    input = queue->input;
    reply->full_handle = input->caret;
    reply->old_rect    = input->caret_rect;
    reply->old_hide    = input->caret_hide;
    reply->old_state   = input->caret_state;

    if (req->handle && get_user_full_handle(req->handle) != input->caret)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (req->flags & SET_CARET_POS)
    {
        input->caret_rect.right  += req->x - input->caret_rect.left;
        input->caret_rect.bottom += req->y - input->caret_rect.top;
        input->caret_rect.left = req->x;
        input->caret_rect.top  = req->y;
    }
    if (req->flags & SET_CARET_HIDE)
    {
        input->caret_hide += req->hide;
        if (input->caret_hide < 0) input->caret_hide = 0;
    }
    if (req->flags & SET_CARET_STATE)
    {
        if (req->state == -1) input->caret_state = !input->caret_state;
        else input->caret_state = !!req->state;
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
    struct thread_input *input;

    if (!queue) return;
    input = queue->input;

    reply->prev_handle = input->cursor;
    reply->prev_count  = input->cursor_count;
    reply->prev_x      = input->desktop->cursor.x;
    reply->prev_y      = input->desktop->cursor.y;

    if (req->flags & SET_CURSOR_HANDLE)
    {
        if (req->handle && !get_user_object( req->handle, USER_CLIENT ))
        {
            set_win32_error( ERROR_INVALID_CURSOR_HANDLE );
            return;
        }
        input->cursor = req->handle;
    }
    if (req->flags & SET_CURSOR_COUNT)
    {
        queue->cursor_count += req->show_count;
        input->cursor_count += req->show_count;
    }
    if (req->flags & SET_CURSOR_POS)
    {
        set_cursor_pos( input->desktop, req->x, req->y );
    }
    if (req->flags & (SET_CURSOR_CLIP | SET_CURSOR_NOCLIP))
    {
        struct desktop *desktop = input->desktop;

        /* only the desktop owner can set the message */
        if (req->clip_msg && get_top_window_owner(desktop) == current->process)
            desktop->cursor.clip_msg = req->clip_msg;

        set_clip_rectangle( desktop, (req->flags & SET_CURSOR_NOCLIP) ? NULL : &req->clip );
    }

    reply->new_x       = input->desktop->cursor.x;
    reply->new_y       = input->desktop->cursor.y;
    reply->new_clip    = input->desktop->cursor.clip;
    reply->last_change = input->desktop->cursor.last_change;
}

DECL_HANDLER(update_rawinput_devices)
{
    const struct rawinput_device *devices = get_req_data();
    unsigned int device_count = get_req_data_size() / sizeof (*devices);
    const struct rawinput_device_entry *e;
    unsigned int i;

    for (i = 0; i < device_count; ++i)
    {
        update_rawinput_device(&devices[i]);
    }

    e = find_rawinput_device( 1, 2 );
    current->process->rawinput_mouse = e ? &e->device : NULL;
    e = find_rawinput_device( 1, 6 );
    current->process->rawinput_kbd   = e ? &e->device : NULL;
}

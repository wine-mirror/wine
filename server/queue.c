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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "handle.h"
#include "thread.h"
#include "process.h"
#include "request.h"
#include "user.h"

enum message_kind { SEND_MESSAGE, POST_MESSAGE, COOKED_HW_MESSAGE, RAW_HW_MESSAGE };
#define NB_MSG_KINDS (RAW_HW_MESSAGE+1)


struct message_result
{
    struct message_result *send_next;   /* next in sender list */
    struct message_result *recv_next;   /* next in receiver list */
    struct msg_queue      *sender;      /* sender queue */
    struct msg_queue      *receiver;    /* receiver queue */
    int                    replied;     /* has it been replied to? */
    unsigned int           result;      /* reply result */
    unsigned int           error;       /* error code to pass back to sender */
    void                  *data;        /* message reply data */
    unsigned int           data_size;   /* size of message reply data */
    struct timeout_user   *timeout;     /* result timeout */
};

struct message
{
    struct message        *next;      /* next message in list */
    struct message        *prev;      /* prev message in list */
    enum message_type      type;      /* message type */
    user_handle_t          win;       /* window handle */
    unsigned int           msg;       /* message code */
    unsigned int           wparam;    /* parameters */
    unsigned int           lparam;    /* parameters */
    int                    x;         /* x position */
    int                    y;         /* y position */
    unsigned int           time;      /* message time */
    unsigned int           info;      /* extra info */
    void                  *data;      /* message data for sent messages */
    unsigned int           data_size; /* size of message data */
    struct message_result *result;    /* result in sender queue */
};

struct message_list
{
    struct message *first;     /* head of list */
    struct message *last;      /* tail of list */
};

struct timer
{
    struct timer   *next;      /* next timer in list */
    struct timer   *prev;      /* prev timer in list */
    struct timeval  when;      /* next expiration */
    unsigned int    rate;      /* timer rate in ms */
    user_handle_t   win;       /* window handle */
    unsigned int    msg;       /* message to post */
    unsigned int    id;        /* timer id */
    unsigned int    lparam;    /* lparam for message */
};

struct thread_input
{
    struct object          obj;           /* object header */
    user_handle_t          focus;         /* focus window */
    user_handle_t          capture;       /* capture window */
    user_handle_t          active;        /* active window */
    user_handle_t          menu_owner;    /* current menu owner window */
    user_handle_t          move_size;     /* current moving/resizing window */
    user_handle_t          caret;         /* caret window */
    rectangle_t            rect;          /* caret rectangle */
    unsigned char          keystate[256]; /* state of each key */
};

struct msg_queue
{
    struct object          obj;           /* object header */
    unsigned int           wake_bits;     /* wakeup bits */
    unsigned int           wake_mask;     /* wakeup mask */
    unsigned int           changed_bits;  /* changed wakeup bits */
    unsigned int           changed_mask;  /* changed wakeup mask */
    int                    paint_count;   /* pending paint messages count */
    struct message_list    msg_list[NB_MSG_KINDS];  /* lists of messages */
    struct message_result *send_result;   /* stack of sent messages waiting for result */
    struct message_result *recv_result;   /* stack of received messages waiting for result */
    struct message        *last_msg;      /* last msg returned to the app and not removed */
    enum message_kind      last_msg_kind; /* message kind of last_msg */
    struct timer          *first_timer;   /* head of timer list */
    struct timer          *last_timer;    /* tail of timer list */
    struct timer          *next_timer;    /* next timer to expire */
    struct timeout_user   *timeout;       /* timeout for next timer to expire */
    struct thread_input   *input;         /* thread input descriptor */
};

static void msg_queue_dump( struct object *obj, int verbose );
static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int msg_queue_signaled( struct object *obj, struct thread *thread );
static int msg_queue_satisfied( struct object *obj, struct thread *thread );
static void msg_queue_destroy( struct object *obj );
static void thread_input_dump( struct object *obj, int verbose );
static void thread_input_destroy( struct object *obj );
static void timer_callback( void *private );

static const struct object_ops msg_queue_ops =
{
    sizeof(struct msg_queue),  /* size */
    msg_queue_dump,            /* dump */
    msg_queue_add_queue,       /* add_queue */
    msg_queue_remove_queue,    /* remove_queue */
    msg_queue_signaled,        /* signaled */
    msg_queue_satisfied,       /* satisfied */
    NULL,                      /* get_poll_events */
    NULL,                      /* poll_event */
    no_get_fd,                 /* get_fd */
    no_flush,                  /* flush */
    no_get_file_info,          /* get_file_info */
    NULL,                      /* queue_async */
    msg_queue_destroy          /* destroy */
};


static const struct object_ops thread_input_ops =
{
    sizeof(struct thread_input),  /* size */
    thread_input_dump,            /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    NULL,                         /* get_poll_events */
    NULL,                         /* poll_event */
    no_get_fd,                    /* get_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    NULL,                         /* queue_async */
    thread_input_destroy          /* destroy */
};

/* create a thread input object */
static struct thread_input *create_thread_input(void)
{
    struct thread_input *input;

    if ((input = alloc_object( &thread_input_ops, -1 )))
    {
        input->focus       = 0;
        input->capture     = 0;
        input->active      = 0;
        input->menu_owner  = 0;
        input->move_size   = 0;
        input->caret       = 0;
        input->rect.left   = 0;
        input->rect.top    = 0;
        input->rect.right  = 0;
        input->rect.bottom = 0;
        memset( input->keystate, 0, sizeof(input->keystate) );
    }
    return input;
}

/* pointer to input structure of foreground thread */
static struct thread_input *foreground_input;

/* create a message queue object */
static struct msg_queue *create_msg_queue( struct thread *thread, struct thread_input *input )
{
    struct msg_queue *queue;
    int i;

    if (!input && !(input = create_thread_input())) return NULL;
    if ((queue = alloc_object( &msg_queue_ops, -1 )))
    {
        queue->wake_bits       = 0;
        queue->wake_mask       = 0;
        queue->changed_bits    = 0;
        queue->changed_mask    = 0;
        queue->paint_count     = 0;
        queue->send_result     = NULL;
        queue->recv_result     = NULL;
        queue->last_msg        = NULL;
        queue->first_timer     = NULL;
        queue->last_timer      = NULL;
        queue->next_timer      = NULL;
        queue->timeout         = NULL;
        queue->input           = (struct thread_input *)grab_object( input );
        for (i = 0; i < NB_MSG_KINDS; i++)
            queue->msg_list[i].first = queue->msg_list[i].last = NULL;

        thread->queue = queue;
        if (!thread->process->queue)
            thread->process->queue = (struct msg_queue *)grab_object( queue );
    }
    release_object( input );
    return queue;
}

/* free the message queue of a thread at thread exit */
void free_msg_queue( struct thread *thread )
{
    struct process *process = thread->process;

    if (!thread->queue) return;
    if (process->queue == thread->queue)  /* is it the process main queue? */
    {
        release_object( process->queue );
        process->queue = NULL;
        if (process->idle_event)
        {
            set_event( process->idle_event );
            release_object( process->idle_event );
            process->idle_event = NULL;
        }
    }
    release_object( thread->queue );
    thread->queue = NULL;
}

/* check the queue status */
inline static int is_signaled( struct msg_queue *queue )
{
    return ((queue->wake_bits & queue->wake_mask) || (queue->changed_bits & queue->changed_mask));
}

/* set some queue bits */
inline static void set_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    queue->wake_bits |= bits;
    queue->changed_bits |= bits;
    if (is_signaled( queue )) wake_up( &queue->obj, 0 );
}

/* clear some queue bits */
inline static void clear_queue_bits( struct msg_queue *queue, unsigned int bits )
{
    queue->wake_bits &= ~bits;
    queue->changed_bits &= ~bits;
}

/* get the QS_* bit corresponding to a given hardware message */
inline static int get_hardware_msg_bit( struct message *msg )
{
    if (msg->msg == WM_MOUSEMOVE || msg->msg == WM_NCMOUSEMOVE) return QS_MOUSEMOVE;
    if (msg->msg >= WM_KEYFIRST && msg->msg <= WM_KEYLAST) return QS_KEY;
    return QS_MOUSEBUTTON;
}

/* get the current thread queue, creating it if needed */
inline static struct msg_queue *get_current_queue(void)
{
    struct msg_queue *queue = current->queue;
    if (!queue) queue = create_msg_queue( current, NULL );
    return queue;
}

/* append a message to the end of a list */
inline static void append_message( struct message_list *list, struct message *msg )
{
    msg->next = NULL;
    if ((msg->prev = list->last)) msg->prev->next = msg;
    else list->first = msg;
    list->last = msg;
}

/* unlink a message from a list it */
inline static void unlink_message( struct message_list *list, struct message *msg )
{
    if (msg->next) msg->next->prev = msg->prev;
    else list->last = msg->prev;
    if (msg->prev) msg->prev->next = msg->next;
    else list->first = msg->next;
}

/* try to merge a message with the last in the list; return 1 if successful */
static int merge_message( struct message_list *list, const struct message *msg )
{
    struct message *prev = list->last;

    if (!prev) return 0;
    if (prev->result) return 0;
    if (prev->win != msg->win) return 0;
    if (prev->msg != msg->msg) return 0;
    if (prev->type != msg->type) return 0;
    /* now we can merge it */
    prev->wparam  = msg->wparam;
    prev->lparam  = msg->lparam;
    prev->x       = msg->x;
    prev->y       = msg->y;
    prev->time    = msg->time;
    prev->info    = msg->info;
    return 1;
}

/* free a result structure */
static void free_result( struct message_result *result )
{
    if (result->timeout) remove_timeout_user( result->timeout );
    if (result->data) free( result->data );
    free( result );
}

/* store the message result in the appropriate structure */
static void store_message_result( struct message_result *res, unsigned int result,
                                  unsigned int error )
{
    res->result  = result;
    res->error   = error;
    res->replied = 1;
    if (res->timeout)
    {
        remove_timeout_user( res->timeout );
        res->timeout = NULL;
    }
    /* wake sender queue if waiting on this result */
    if (res->sender && res->sender->send_result == res)
        set_queue_bits( res->sender, QS_SMRESULT );
}

/* free a message when deleting a queue or window */
static void free_message( struct message *msg )
{
    struct message_result *result = msg->result;
    if (result)
    {
        if (result->sender)
        {
            result->receiver = NULL;
            store_message_result( result, 0, STATUS_ACCESS_DENIED /*FIXME*/ );
        }
        else free_result( result );
    }
    if (msg->data) free( msg->data );
    free( msg );
}

/* remove (and free) a message from a message list */
static void remove_queue_message( struct msg_queue *queue, struct message *msg,
                                  enum message_kind kind )
{
    int clr_bit;
    struct message *other;

    if (queue->last_msg == msg) queue->last_msg = NULL;
    unlink_message( &queue->msg_list[kind], msg );
    switch(kind)
    {
    case SEND_MESSAGE:
        if (!queue->msg_list[kind].first) clear_queue_bits( queue, QS_SENDMESSAGE );
        break;
    case POST_MESSAGE:
        if (!queue->msg_list[kind].first) clear_queue_bits( queue, QS_POSTMESSAGE );
        break;
    case COOKED_HW_MESSAGE:
    case RAW_HW_MESSAGE:
        clr_bit = get_hardware_msg_bit( msg );
        for (other = queue->msg_list[kind].first; other; other = other->next)
            if (get_hardware_msg_bit( other ) == clr_bit) break;
        if (!other) clear_queue_bits( queue, clr_bit );
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
    store_message_result( result, 0, STATUS_TIMEOUT );
}

/* allocate and fill a message result structure */
static struct message_result *alloc_message_result( struct msg_queue *send_queue,
                                                    struct msg_queue *recv_queue,
                                                    unsigned int timeout )
{
    struct message_result *result = mem_alloc( sizeof(*result) );
    if (result)
    {
        /* put the result on the sender result stack */
        result->sender    = send_queue;
        result->receiver  = recv_queue;
        result->replied   = 0;
        result->data      = NULL;
        result->data_size = 0;
        result->timeout   = NULL;
        result->send_next = send_queue->send_result;
        send_queue->send_result = result;
        if (timeout != -1)
        {
            struct timeval when;
            gettimeofday( &when, 0 );
            add_timeout( &when, timeout );
            result->timeout = add_timeout_user( &when, result_timeout, result );
        }
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
    reply->info   = msg->info;

    if (msg->data) set_reply_data_ptr( msg->data, msg->data_size );

    unlink_message( &queue->msg_list[SEND_MESSAGE], msg );
    /* put the result on the receiver result stack */
    if (result)
    {
        result->recv_next  = queue->recv_result;
        queue->recv_result = result;
    }
    free( msg );
    if (!queue->msg_list[SEND_MESSAGE].first) clear_queue_bits( queue, QS_SENDMESSAGE );
}

/* set the result of the current received message */
static void reply_message( struct msg_queue *queue, unsigned int result,
                           unsigned int error, int remove, const void *data, size_t len )
{
    struct message_result *res = queue->recv_result;

    if (remove)
    {
        queue->recv_result = res->recv_next;
        res->receiver = NULL;
        if (!res->sender)  /* no one waiting for it */
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

/* empty a message list and free all the messages */
static void empty_msg_list( struct message_list *list )
{
    struct message *msg = list->first;
    while (msg)
    {
        struct message *next = msg->next;
        free_message( msg );
        msg = next;
    }
}

/* cleanup all pending results when deleting a queue */
static void cleanup_results( struct msg_queue *queue )
{
    struct message_result *result, *next;

    result = queue->send_result;
    while (result)
    {
        next = result->send_next;
        result->sender = NULL;
        if (!result->receiver) free_result( result );
        result = next;
    }

    while (queue->recv_result)
        reply_message( queue, 0, STATUS_ACCESS_DENIED /*FIXME*/, 1, NULL, 0 );
}

static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct process *process = entry->thread->process;

    /* a thread can only wait on its own queue */
    if (entry->thread->queue != queue)
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    /* if waiting on the main process queue, set the idle event */
    if (process->queue == queue)
    {
        if (process->idle_event) set_event( process->idle_event );
    }
    add_queue( obj, entry );
    return 1;
}

static void msg_queue_remove_queue(struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct process *process = entry->thread->process;

    remove_queue( obj, entry );

    assert( entry->thread->queue == queue );

    /* if waiting on the main process queue, reset the idle event */
    if (process->queue == queue)
    {
        if (process->idle_event) reset_event( process->idle_event );
    }
}

static void msg_queue_dump( struct object *obj, int verbose )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    fprintf( stderr, "Msg queue bits=%x mask=%x\n",
             queue->wake_bits, queue->wake_mask );
}

static int msg_queue_signaled( struct object *obj, struct thread *thread )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    return is_signaled( queue );
}

static int msg_queue_satisfied( struct object *obj, struct thread *thread )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    queue->wake_mask = 0;
    queue->changed_mask = 0;
    return 0;  /* Not abandoned */
}

static void msg_queue_destroy( struct object *obj )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct timer *timer = queue->first_timer;
    int i;

    cleanup_results( queue );
    for (i = 0; i < NB_MSG_KINDS; i++) empty_msg_list( &queue->msg_list[i] );

    while (timer)
    {
        struct timer *next = timer->next;
        free( timer );
        timer = next;
    }
    if (queue->timeout) remove_timeout_user( queue->timeout );
    if (queue->input) release_object( queue->input );
}

static void thread_input_dump( struct object *obj, int verbose )
{
    struct thread_input *input = (struct thread_input *)obj;
    fprintf( stderr, "Thread input focus=%x capture=%x active=%x\n",
             input->focus, input->capture, input->active );
}

static void thread_input_destroy( struct object *obj )
{
    struct thread_input *input = (struct thread_input *)obj;

    if (foreground_input == input) foreground_input = NULL;
}

/* fix the thread input data when a window is destroyed */
inline static void thread_input_cleanup_window( struct msg_queue *queue, user_handle_t window )
{
    struct thread_input *input = queue->input;

    if (window == input->focus) input->focus = 0;
    if (window == input->capture) input->capture = 0;
    if (window == input->active) input->active = 0;
    if (window == input->menu_owner) input->menu_owner = 0;
    if (window == input->move_size) input->move_size = 0;
    if (window == input->caret) input->caret = 0;
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

/* attach two thread input data structures */
int attach_thread_input( struct thread *thread_from, struct thread *thread_to )
{
    struct thread_input *input;

    if (!thread_to->queue && !(thread_to->queue = create_msg_queue( thread_to, NULL ))) return 0;
    input = (struct thread_input *)grab_object( thread_to->queue->input );

    if (thread_from->queue)
    {
        release_object( thread_from->queue->input );
        thread_from->queue->input = input;
    }
    else
    {
        if (!(thread_from->queue = create_msg_queue( thread_from, input ))) return 0;
    }
    memset( input->keystate, 0, sizeof(input->keystate) );
    return 1;
}

/* detach two thread input data structures */
static void detach_thread_input( struct thread *thread_from, struct thread *thread_to )
{
    struct thread_input *input;

    if (!thread_from->queue || !thread_to->queue ||
        thread_from->queue->input != thread_to->queue->input)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if ((input = create_thread_input()))
    {
        release_object( thread_from->queue->input );
        thread_from->queue->input = input;
    }
}


/* set the next timer to expire */
static void set_next_timer( struct msg_queue *queue, struct timer *timer )
{
    if (queue->timeout)
    {
        remove_timeout_user( queue->timeout );
        queue->timeout = NULL;
    }
    if ((queue->next_timer = timer))
        queue->timeout = add_timeout_user( &timer->when, timer_callback, queue );

    /* set/clear QS_TIMER bit */
    if (queue->next_timer == queue->first_timer)
        clear_queue_bits( queue, QS_TIMER );
    else
        set_queue_bits( queue, QS_TIMER );
}

/* callback for the next timer expiration */
static void timer_callback( void *private )
{
    struct msg_queue *queue = private;

    queue->timeout = NULL;
    /* move on to the next timer */
    set_next_timer( queue, queue->next_timer->next );
}

/* link a timer at its rightful place in the queue list */
static void link_timer( struct msg_queue *queue, struct timer *timer )
{
    struct timer *pos = queue->next_timer;

    while (pos && time_before( &pos->when, &timer->when )) pos = pos->next;

    if (pos) /* insert before pos */
    {
        if ((timer->prev = pos->prev)) timer->prev->next = timer;
        else queue->first_timer = timer;
        timer->next = pos;
        pos->prev = timer;
    }
    else  /* insert at end */
    {
        timer->next = NULL;
        timer->prev = queue->last_timer;
        if (queue->last_timer) queue->last_timer->next = timer;
        else queue->first_timer = timer;
        queue->last_timer = timer;
    }
    /* check if we replaced the next timer */
    if (pos == queue->next_timer) set_next_timer( queue, timer );
}

/* remove a timer from the queue timer list */
static void unlink_timer( struct msg_queue *queue, struct timer *timer )
{
    if (timer->next) timer->next->prev = timer->prev;
    else queue->last_timer = timer->prev;
    if (timer->prev) timer->prev->next = timer->next;
    else queue->first_timer = timer->next;
    /* check if we removed the next timer */
    if (queue->next_timer == timer) set_next_timer( queue, timer->next );
    else if (queue->next_timer == queue->first_timer) clear_queue_bits( queue, QS_TIMER );
}

/* restart an expired timer */
static void restart_timer( struct msg_queue *queue, struct timer *timer )
{
    struct timeval now;
    unlink_timer( queue, timer );
    gettimeofday( &now, 0 );
    while (!time_before( &now, &timer->when )) add_timeout( &timer->when, timer->rate );
    link_timer( queue, timer );
}

/* find an expired timer matching the filtering parameters */
static struct timer *find_expired_timer( struct msg_queue *queue, user_handle_t win,
                                         unsigned int get_first, unsigned int get_last,
                                         int remove )
{
    struct timer *timer;
    for (timer = queue->first_timer; (timer && timer != queue->next_timer); timer = timer->next)
    {
        if (win && timer->win != win) continue;
        if (timer->msg >= get_first && timer->msg <= get_last)
        {
            if (remove) restart_timer( queue, timer );
            return timer;
        }
    }
    return NULL;
}

/* kill a timer */
static int kill_timer( struct msg_queue *queue, user_handle_t win,
                       unsigned int msg, unsigned int id )
{
    struct timer *timer;

    for (timer = queue->first_timer; timer; timer = timer->next)
    {
        if (timer->win != win || timer->msg != msg || timer->id != id) continue;
        unlink_timer( queue, timer );
        free( timer );
        return 1;
    }
    return 0;
}

/* add a timer */
static struct timer *set_timer( struct msg_queue *queue, unsigned int rate )
{
    struct timer *timer = mem_alloc( sizeof(*timer) );
    if (timer)
    {
        timer->rate  = rate;
        gettimeofday( &timer->when, 0 );
        add_timeout( &timer->when, rate );
        link_timer( queue, timer );
    }
    return timer;
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
    struct timer *timer;
    struct message *msg;
    int i;

    if (!queue) return;

    /* remove timers */
    timer = queue->first_timer;
    while (timer)
    {
        struct timer *next = timer->next;
        if (timer->win == win)
        {
            unlink_timer( queue, timer );
            free( timer );
        }
        timer = next;
    }

    /* remove messages */
    for (i = 0; i < NB_MSG_KINDS; i++)
    {
        msg = queue->msg_list[i].first;
        while (msg)
        {
            struct message *next = msg->next;
            if (msg->win == win) remove_queue_message( queue, msg, i );
            msg = next;
        }
    }

    thread_input_cleanup_window( queue, win );
}

/* post a message to a window; used by socket handling */
void post_message( user_handle_t win, unsigned int message,
                   unsigned int wparam, unsigned int lparam )
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
        msg->x         = 0;
        msg->y         = 0;
        msg->info      = 0;
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = 0;

        append_message( &thread->queue->msg_list[POST_MESSAGE], msg );
        set_queue_bits( thread->queue, QS_POSTMESSAGE );
    }
    release_object( thread );
}


/* get the message queue of the current thread */
DECL_HANDLER(get_msg_queue)
{
    struct msg_queue *queue = get_current_queue();

    reply->handle = 0;
    if (queue) reply->handle = alloc_handle( current->process, queue, SYNCHRONIZE, 0 );
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
            if (req->skip_wait) msg_queue_satisfied( &queue->obj, current );
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
    struct msg_queue *recv_queue;
    struct thread *thread = get_thread_from_id( req->id );

    if (!thread) return;

    if (!(recv_queue = thread->queue))
    {
        set_error( STATUS_INVALID_PARAMETER );
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
        msg->time      = req->time;
        msg->x         = req->x;
        msg->y         = req->y;
        msg->info      = req->info;
        msg->result    = NULL;
        msg->data      = NULL;
        msg->data_size = 0;

        switch(msg->type)
        {
        case MSG_OTHER_PROCESS:
            msg->data_size = get_req_data_size();
            if (msg->data_size && !(msg->data = memdup( get_req_data(), msg->data_size )))
            {
                free( msg );
                break;
            }
            /* fall through */
        case MSG_ASCII:
        case MSG_UNICODE:
        case MSG_CALLBACK:
            if (!(msg->result = alloc_message_result( send_queue, recv_queue, req->timeout )))
            {
                free( msg );
                break;
            }
            /* fall through */
        case MSG_NOTIFY:
            append_message( &recv_queue->msg_list[SEND_MESSAGE], msg );
            set_queue_bits( recv_queue, QS_SENDMESSAGE );
            break;
        case MSG_POSTED:
            /* needed for posted DDE messages */
            msg->data_size = get_req_data_size();
            if (msg->data_size && !(msg->data = memdup( get_req_data(), msg->data_size )))
            {
                free( msg );
                break;
            }
            append_message( &recv_queue->msg_list[POST_MESSAGE], msg );
            set_queue_bits( recv_queue, QS_POSTMESSAGE );
            break;
        case MSG_HARDWARE_RAW:
        case MSG_HARDWARE_COOKED:
            {
                struct message_list *list = ((msg->type == MSG_HARDWARE_RAW) ?
                                             &recv_queue->msg_list[RAW_HW_MESSAGE] :
                                             &recv_queue->msg_list[COOKED_HW_MESSAGE]);
                if (msg->msg == WM_MOUSEMOVE && merge_message( list, msg ))
                {
                    free( msg );
                    break;
                }
                append_message( list, msg );
                set_queue_bits( recv_queue, get_hardware_msg_bit(msg) );
                break;
            }
        default:
            set_error( STATUS_INVALID_PARAMETER );
            free( msg );
            break;
        }
    }
    release_object( thread );
}

/* return a message to the application, removing it from the queue if needed */
static void return_message_to_app( struct msg_queue *queue, int flags,
                                   struct get_message_reply *reply,
                                   struct message *msg, enum message_kind kind )
{
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
    reply->info   = msg->info;

    /* raw messages always get removed */
    if ((msg->type == MSG_HARDWARE_RAW) || (flags & GET_MSG_REMOVE))
    {
        queue->last_msg = NULL;
        if (msg->data)
        {
            set_reply_data_ptr( msg->data, msg->data_size );
            msg->data = NULL;
            msg->data_size = 0;
        }
        remove_queue_message( queue, msg, kind );
    }
    else  /* remember it as the last returned message */
    {
        if (msg->data) set_reply_data( msg->data, msg->data_size );
        queue->last_msg = msg;
        queue->last_msg_kind = kind;
    }
}


inline static struct message *find_matching_message( const struct message_list *list,
                                                     user_handle_t win,
                                                     unsigned int first, unsigned int last )
{
    struct message *msg;

    for (msg = list->first; msg; msg = msg->next)
    {
        /* check against the filters */
        if (msg->msg == WM_QUIT) break;  /* WM_QUIT is never filtered */
        if (win && msg->win && msg->win != win && !is_child_window( win, msg->win )) continue;
        if (msg->msg < first) continue;
        if (msg->msg > last) continue;
        break; /* found one */
    }
    return msg;
}


/* get a message from the current queue */
DECL_HANDLER(get_message)
{
    struct timer *timer;
    struct message *msg;
    struct msg_queue *queue = get_current_queue();
    user_handle_t get_win = get_user_full_handle( req->get_win );

    if (!queue) return;

    /* first check for sent messages */
    if ((msg = queue->msg_list[SEND_MESSAGE].first))
    {
        receive_message( queue, msg, reply );
        return;
    }
    if (req->flags & GET_MSG_SENT_ONLY) goto done;  /* nothing else to check */

    /* if requested, remove the last returned but not yet removed message */
    if ((req->flags & GET_MSG_REMOVE_LAST) && queue->last_msg)
        remove_queue_message( queue, queue->last_msg, queue->last_msg_kind );
    queue->last_msg = NULL;

    /* clear changed bits so we can wait on them if we don't find a message */
    queue->changed_bits = 0;

    /* then check for posted messages */
    if ((msg = find_matching_message( &queue->msg_list[POST_MESSAGE], get_win,
                                      req->get_first, req->get_last )))
    {
        return_message_to_app( queue, req->flags, reply, msg, POST_MESSAGE );
        return;
    }

    /* then check for cooked hardware messages */
    if ((msg = find_matching_message( &queue->msg_list[COOKED_HW_MESSAGE], get_win,
                                      req->get_first, req->get_last )))
    {
        return_message_to_app( queue, req->flags, reply, msg, COOKED_HW_MESSAGE );
        return;
    }

    /* then check for any raw hardware message */
    if ((msg = queue->msg_list[RAW_HW_MESSAGE].first))
    {
        return_message_to_app( queue, req->flags, reply, msg, RAW_HW_MESSAGE );
        return;
    }

    /* now check for WM_PAINT */
    if (queue->paint_count &&
        (WM_PAINT >= req->get_first) && (WM_PAINT <= req->get_last) &&
        (reply->win = find_window_to_repaint( get_win, current )))
    {
        reply->type   = MSG_POSTED;
        reply->msg    = WM_PAINT;
        reply->wparam = 0;
        reply->lparam = 0;
        reply->x      = 0;
        reply->y      = 0;
        reply->time   = get_tick_count();
        reply->info   = 0;
        return;
    }

    /* now check for timer */
    if ((timer = find_expired_timer( queue, get_win, req->get_first,
                                     req->get_last, (req->flags & GET_MSG_REMOVE) )))
    {
        reply->type   = MSG_POSTED;
        reply->win    = timer->win;
        reply->msg    = timer->msg;
        reply->wparam = timer->id;
        reply->lparam = timer->lparam;
        reply->x      = 0;
        reply->y      = 0;
        reply->time   = get_tick_count();
        reply->info   = 0;
        return;
    }

 done:
    set_error( STATUS_PENDING );  /* FIXME */
}


/* reply to a sent message */
DECL_HANDLER(reply_message)
{
    if (current->queue && current->queue->recv_result)
        reply_message( current->queue, req->result, 0, req->remove,
                       get_req_data(), get_req_data_size() );
    else
        set_error( STATUS_ACCESS_DENIED );
}


/* retrieve the reply for the last message sent */
DECL_HANDLER(get_message_reply)
{
    struct msg_queue *queue = current->queue;

    if (queue)
    {
        struct message_result *result = queue->send_result;

        set_error( STATUS_PENDING );
        reply->result = 0;

        if (result && (result->replied || req->cancel))
        {
            if (result->replied)
            {
                reply->result = result->result;
                set_error( result->error );
                if (result->data)
                {
                    size_t data_len = min( result->data_size, get_reply_max_size() );
                    set_reply_data_ptr( result->data, data_len );
                    result->data = NULL;
                    result->data_size = 0;
                }
            }
            queue->send_result = result->send_next;
            result->sender = NULL;
            if (!result->receiver) free_result( result );
            if (!queue->send_result || !queue->send_result->replied)
                clear_queue_bits( queue, QS_SMRESULT );
        }
    }
    else set_error( STATUS_ACCESS_DENIED );
}


/* set a window timer */
DECL_HANDLER(set_win_timer)
{
    struct timer *timer;
    struct msg_queue *queue = get_current_queue();
    user_handle_t win = get_user_full_handle( req->win );

    if (!queue) return;

    /* remove it if it existed already */
    if (win) kill_timer( queue, win, req->msg, req->id );

    if ((timer = set_timer( queue, req->rate )))
    {
        timer->win    = win;
        timer->msg    = req->msg;
        timer->id     = req->id;
        timer->lparam = req->lparam;
    }
}

/* kill a window timer */
DECL_HANDLER(kill_win_timer)
{
    struct msg_queue *queue = current->queue;

    if (!queue || !kill_timer( queue, get_user_full_handle(req->win), req->msg, req->id ))
        set_error( STATUS_INVALID_PARAMETER );
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
        else detach_thread_input( thread_from, thread_to );
    }
    else set_error( STATUS_ACCESS_DENIED );
    release_object( thread_from );
    release_object( thread_to );
}


/* get thread input data */
DECL_HANDLER(get_thread_input)
{
    struct thread *thread = NULL;
    struct thread_input *input;

    if (req->tid)
    {
        if (!(thread = get_thread_from_id( req->tid ))) return;
        input = thread->queue ? thread->queue->input : NULL;
    }
    else input = foreground_input;  /* get the foreground thread info */

    if (input)
    {
        reply->focus      = input->focus;
        reply->capture    = input->capture;
        reply->active     = input->active;
        reply->menu_owner = input->menu_owner;
        reply->move_size  = input->move_size;
        reply->caret      = input->caret;
        reply->rect       = input->rect;
    }
    else
    {
        reply->focus      = 0;
        reply->capture    = 0;
        reply->active     = 0;
        reply->menu_owner = 0;
        reply->move_size  = 0;
        reply->caret      = 0;
        reply->rect.left = reply->rect.top = reply->rect.right = reply->rect.bottom = 0;
    }
    /* foreground window is active window of foreground thread */
    reply->foreground = foreground_input ? foreground_input->active : 0;
    if (thread) release_object( thread );
}


/* set the system foreground window */
DECL_HANDLER(set_foreground_window)
{
    struct msg_queue *queue = get_current_queue();

    reply->previous = foreground_input ? foreground_input->active : 0;
    reply->send_msg_old = (reply->previous && foreground_input != queue->input);
    reply->send_msg_new = FALSE;

    if (req->handle)
    {
        struct thread *thread;

        if (is_top_level_window( req->handle ) &&
            ((thread = get_window_thread( req->handle ))))
        {
            foreground_input = thread->queue->input;
            reply->send_msg_new = (foreground_input != queue->input);
            release_object( thread );
        }
        else set_error( STATUS_INVALID_HANDLE );
    }
    else foreground_input = NULL;
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

        reply->previous = input->capture;
        input->capture = get_user_full_handle( req->handle );
        input->menu_owner = (req->flags & CAPTURE_MENU) ? input->capture : 0;
        input->move_size = (req->flags & CAPTURE_MOVESIZE) ? input->capture : 0;
        reply->full_handle = input->capture;
    }
}

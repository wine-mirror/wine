/*
 * Server-side message queues
 *
 * Copyright (C) 2000 Alexandre Julliard
 */

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

struct message_result
{
    struct message_result *send_next;   /* next in sender list */
    struct message_result *recv_next;   /* next in receiver list */
    struct msg_queue      *sender;      /* sender queue */
    struct msg_queue      *receiver;    /* receiver queue */
    int                    replied;     /* has it been replied to? */
    unsigned int           result;      /* reply result */
    unsigned int           error;       /* error code to pass back to sender */
};

struct message
{
    struct message        *next;      /* next message in list */
    struct message        *prev;      /* prev message in list */
    int                    type;      /* message type (FIXME) */
    handle_t               win;       /* window handle */
    unsigned int           msg;       /* message code */
    unsigned int           wparam;    /* parameters */
    unsigned int           lparam;    /* parameters */
    unsigned short         x;         /* x position */
    unsigned short         y;         /* y position */
    unsigned int           time;      /* message time */
    unsigned int           info;      /* extra info */
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
    handle_t        win;       /* window handle */
    unsigned int    msg;       /* message to post */
    unsigned int    id;        /* timer id */
    unsigned int    lparam;    /* lparam for message */
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
};

static void msg_queue_dump( struct object *obj, int verbose );
static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int msg_queue_signaled( struct object *obj, struct thread *thread );
static int msg_queue_satisfied( struct object *obj, struct thread *thread );
static void msg_queue_destroy( struct object *obj );
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
    msg_queue_destroy          /* destroy */
};


static struct msg_queue *create_msg_queue( struct thread *thread )
{
    struct msg_queue *queue;
    int i;

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
        for (i = 0; i < NB_MSG_KINDS; i++)
            queue->msg_list[i].first = queue->msg_list[i].last = NULL;

        thread->queue = queue;
        if (!thread->process->queue)
            thread->process->queue = (struct msg_queue *)grab_object( queue );
    }
    return queue;
}

/* check the queue status */
inline static int is_signaled( struct msg_queue *queue )
{
    return ((queue->wake_bits & queue->wake_mask) || (queue->changed_bits & queue->changed_mask));
}

/* set/clear some queue bits */
inline static void change_queue_bits( struct msg_queue *queue, unsigned int set, unsigned int clear )
{
    queue->wake_bits = (queue->wake_bits | set) & ~clear;
    queue->changed_bits = (queue->changed_bits | set) & ~clear;
    if (is_signaled( queue )) wake_up( &queue->obj, 0 );
}

/* get the QS_* bit corresponding to a given hardware message */
inline static int get_hardware_msg_bit( struct message *msg )
{
    if (msg->msg == WM_MOUSEMOVE || msg->msg == WM_NCMOUSEMOVE) return QS_MOUSEMOVE;
    if (msg->msg >= WM_KEYFIRST && msg->msg <= WM_KEYLAST) return QS_KEY;
    return QS_MOUSEBUTTON;
}

/* get the current thread queue, creating it if needed */
inline struct msg_queue *get_current_queue(void)
{
    struct msg_queue *queue = current->queue;
    if (!queue) queue = create_msg_queue( current );
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

/* free a message when deleting a queue or window */
static void free_message( struct message *msg )
{
    struct message_result *result = msg->result;
    if (result)
    {
        if (result->sender)
        {
            result->result   = 0;
            result->error    = STATUS_ACCESS_DENIED;  /* FIXME */
            result->replied  = 1;
            result->receiver = NULL;
            /* wake sender queue if waiting on this result */
            if (result->sender->send_result == result)
                change_queue_bits( result->sender, QS_SMRESULT, 0 );
        }
        else free( result );
    }
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
        if (!queue->msg_list[kind].first) change_queue_bits( queue, 0, QS_SENDMESSAGE );
        break;
    case POST_MESSAGE:
        if (!queue->msg_list[kind].first) change_queue_bits( queue, 0, QS_POSTMESSAGE );
        break;
    case COOKED_HW_MESSAGE:
    case RAW_HW_MESSAGE:
        clr_bit = get_hardware_msg_bit( msg );
        for (other = queue->msg_list[kind].first; other; other = other->next)
            if (get_hardware_msg_bit( other ) == clr_bit) break;
        if (!other) change_queue_bits( queue, 0, clr_bit );
        break;
    }
    free_message( msg );
}

/* send a message from the sender queue to the receiver queue */
static int send_message( struct msg_queue *send_queue, struct msg_queue *recv_queue,
                         struct message *msg )
{
    struct message_result *result = mem_alloc( sizeof(*result) );
    if (!result) return 0;

    /* put the result on the sender result stack */
    result->sender    = send_queue;
    result->receiver  = recv_queue;
    result->replied   = 0;
    result->send_next = send_queue->send_result;
    send_queue->send_result = result;

    /* and put the message on the receiver queue */
    msg->result = result;
    append_message( &recv_queue->msg_list[SEND_MESSAGE], msg );
    change_queue_bits( recv_queue, QS_SENDMESSAGE, 0 );
    return 1;
}

/* receive a message, removing it from the sent queue */
static void receive_message( struct msg_queue *queue, struct message *msg )
{
    struct message_result *result = msg->result;

    unlink_message( &queue->msg_list[SEND_MESSAGE], msg );
    /* put the result on the receiver result stack */
    result->recv_next  = queue->recv_result;
    queue->recv_result = result;
    free( msg );
    if (!queue->msg_list[SEND_MESSAGE].first) change_queue_bits( queue, 0, QS_SENDMESSAGE );
}

/* set the result of the current received message */
static void reply_message( struct msg_queue *queue, unsigned int result,
                           unsigned int error, int remove )
{
    struct message_result *res = queue->recv_result;

    if (remove)
    {
        queue->recv_result = res->recv_next;
        res->receiver = NULL;
        if (!res->sender)  /* no one waiting for it */
        {
            free( res );
            return;
        }
    }
    if (!res->replied)
    {
        res->result  = result;
        res->error   = error;
        res->replied = 1;
        /* wake sender queue if waiting on this result */
        if (res->sender && res->sender->send_result == res)
            change_queue_bits( res->sender, QS_SMRESULT, 0 );
    }
}

/* retrieve the reply of the current message being sent */
static unsigned int get_message_reply( struct msg_queue *queue, int cancel )
{
    struct message_result *res = queue->send_result;
    unsigned int ret = 0;

    set_error( STATUS_PENDING );

    if (res && (res->replied || cancel))
    {
        if (res->replied)
        {
            ret = res->result;
            set_error( res->error );
        }
        queue->send_result = res->send_next;
        res->sender = NULL;
        if (!res->receiver) free( res );
        if (!queue->send_result || !queue->send_result->replied)
            change_queue_bits( queue, 0, QS_SMRESULT );
    }
    return ret;
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
        if (!result->receiver) free( result );
        result = next;
    }

    while (queue->recv_result) reply_message( queue, 0, STATUS_ACCESS_DENIED /*FIXME*/, 1 );
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
        change_queue_bits( queue, 0, QS_TIMER );
    else
        change_queue_bits( queue, QS_TIMER, 0 );
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
    else if (queue->next_timer == queue->first_timer) change_queue_bits( queue, 0, QS_TIMER );
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
static struct timer *find_expired_timer( struct msg_queue *queue, handle_t win,
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
static int kill_timer( struct msg_queue *queue, handle_t win, unsigned int msg, unsigned int id )
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

/* remove all messages and timers belonging to a certain window */
static void cleanup_window( struct msg_queue *queue, handle_t win )
{
    struct timer *timer;
    struct message *msg;
    int i;

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
}

/* get the message queue of the current thread */
DECL_HANDLER(get_msg_queue)
{
    struct msg_queue *queue = get_current_queue();

    req->handle = 0;
    if (queue) req->handle = alloc_handle( current->process, queue, SYNCHRONIZE, 0 );
}


/* increment the message queue paint count */
DECL_HANDLER(inc_queue_paint_count)
{
    struct msg_queue *queue;
    struct thread *thread = get_thread_from_id( req->id );

    if (!thread) return;

    if ((queue = thread->queue))
    {
        if ((queue->paint_count += req->incr) < 0) queue->paint_count = 0;

        if (queue->paint_count)
            change_queue_bits( queue, QS_PAINT, 0 );
        else
            change_queue_bits( queue, 0, QS_PAINT );
    }
    else set_error( STATUS_INVALID_PARAMETER );

    release_object( thread );

}


/* set the current message queue wakeup mask */
DECL_HANDLER(set_queue_mask)
{
    struct msg_queue *queue = get_current_queue();

    if (queue)
    {
        queue->wake_mask    = req->wake_mask;
        queue->changed_mask = req->changed_mask;
        req->wake_bits      = queue->wake_bits;
        req->changed_bits   = queue->changed_bits;
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
        req->wake_bits    = queue->wake_bits;
        req->changed_bits = queue->changed_bits;
        if (req->clear) queue->changed_bits = 0;
    }
    else req->wake_bits = req->changed_bits = 0;
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
        msg->type    = req->type;
        msg->win     = req->win;
        msg->msg     = req->msg;
        msg->wparam  = req->wparam;
        msg->lparam  = req->lparam;
        msg->x       = req->x;
        msg->y       = req->y;
        msg->time    = req->time;
        msg->info    = req->info;
        msg->result  = NULL;
        switch(req->kind)
        {
        case SEND_MESSAGE:
            send_message( send_queue, recv_queue, msg );
            break;
        case POST_MESSAGE:
            append_message( &recv_queue->msg_list[POST_MESSAGE], msg );
            change_queue_bits( recv_queue, QS_POSTMESSAGE, 0 );
            break;
        case COOKED_HW_MESSAGE:
        case RAW_HW_MESSAGE:
            append_message( &recv_queue->msg_list[req->kind], msg );
            change_queue_bits( recv_queue, get_hardware_msg_bit(msg), 0 );
            break;
        default:
            free( msg );
            set_error( STATUS_INVALID_PARAMETER );
            break;
        }
    }
    release_object( thread );
}

/* store a message contents into the request buffer; helper for get_message */
inline static void put_req_message( struct get_message_request *req, const struct message *msg )
{
    req->type   = msg->type;
    req->win    = msg->win;
    req->msg    = msg->msg;
    req->wparam = msg->wparam;
    req->lparam = msg->lparam;
    req->x      = msg->x;
    req->y      = msg->y;
    req->time   = msg->time;
    req->info   = msg->info;
}

/* return a message to the application, removing it from the queue if needed */
static void return_message_to_app( struct msg_queue *queue, struct get_message_request *req,
                                   struct message *msg, enum message_kind kind )
{
    req->kind = kind;
    put_req_message( req, msg );
    /* raw messages always get removed */
    if ((kind == RAW_HW_MESSAGE) || (req->flags & GET_MSG_REMOVE))
    {
        queue->last_msg = NULL;
        remove_queue_message( queue, msg, kind );
    }
    else  /* remember it as the last returned message */
    {
        queue->last_msg = msg;
        queue->last_msg_kind = kind;
    }
}


inline static struct message *find_matching_message( const struct message_list *list, handle_t win,
                                                     unsigned int first, unsigned int last )
{
    struct message *msg;

    for (msg = list->first; msg; msg = msg->next)
    {
        /* check against the filters */
        if (msg->msg == WM_QUIT) break;  /* WM_QUIT is never filtered */
        if (win && msg->win && msg->win != win) continue;
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

    if (!queue) return;

    /* first check for sent messages */
    if ((msg = queue->msg_list[SEND_MESSAGE].first))
    {
        req->kind = SEND_MESSAGE;
        put_req_message( req, msg );
        receive_message( queue, msg );
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
    if ((msg = find_matching_message( &queue->msg_list[POST_MESSAGE], req->get_win,
                                      req->get_first, req->get_last )))
    {
        return_message_to_app( queue, req, msg, POST_MESSAGE );
        return;
    }

    /* then check for cooked hardware messages */
    if ((msg = find_matching_message( &queue->msg_list[COOKED_HW_MESSAGE], req->get_win,
                                      req->get_first, req->get_last )))
    {
        return_message_to_app( queue, req, msg, COOKED_HW_MESSAGE );
        return;
    }

    /* then check for any raw hardware message */
    if ((msg = queue->msg_list[RAW_HW_MESSAGE].first))
    {
        return_message_to_app( queue, req, msg, RAW_HW_MESSAGE );
        return;
    }

    /* now check for WM_PAINT */
    if ((queue->wake_bits & QS_PAINT) &&
        (WM_PAINT >= req->get_first) && (WM_PAINT <= req->get_last))
    {
        req->kind   = POST_MESSAGE;
        req->type   = 0;
        req->win    = 0;
        req->msg    = WM_PAINT;
        req->wparam = 0;
        req->lparam = 0;
        req->x      = 0;
        req->y      = 0;
        req->time   = 0;
        req->info   = 0;
        return;
    }

    /* now check for timer */
    if ((timer = find_expired_timer( queue, req->get_win, req->get_first,
                                     req->get_last, (req->flags & GET_MSG_REMOVE) )))
    {
        req->kind   = POST_MESSAGE;
        req->type   = 0;
        req->win    = timer->win;
        req->msg    = timer->msg;
        req->wparam = timer->id;
        req->lparam = timer->lparam;
        req->x      = 0;
        req->y      = 0;
        req->time   = 0;
        req->info   = 0;
        return;
    }

 done:
    set_error( STATUS_PENDING );  /* FIXME */
}


/* reply to a sent message */
DECL_HANDLER(reply_message)
{
    if (current->queue && current->queue->recv_result)
        reply_message( current->queue, req->result, 0, req->remove );
    else
        set_error( STATUS_ACCESS_DENIED );
}


/* retrieve the reply for the last message sent */
DECL_HANDLER(get_message_reply)
{
    if (current->queue) req->result = get_message_reply( current->queue, req->cancel );
    else set_error( STATUS_ACCESS_DENIED );
}


/* check if we are processing a sent message */
DECL_HANDLER(in_send_message)
{
    int flags = 0;

    if (current->queue)
    {
        struct message_result *result = current->queue->recv_result;
        if (result)
        {
            flags |= ISMEX_SEND;  /* FIXME */
            if (result->replied || !result->sender) flags |= ISMEX_REPLIED;
        }
    }
    req->flags = flags;
}


/* cleanup a queue when a window is deleted */
DECL_HANDLER(cleanup_window_queue)
{
    if (current->queue) cleanup_window( current->queue, req->win );
}


/* set a window timer */
DECL_HANDLER(set_win_timer)
{
    struct timer *timer;
    struct msg_queue *queue = get_current_queue();

    if (!queue) return;

    /* remove it if it existed already */
    if (req->win) kill_timer( queue, req->win, req->msg, req->id );

    if ((timer = set_timer( queue, req->rate )))
    {
        timer->win    = req->win;
        timer->msg    = req->msg;
        timer->id     = req->id;
        timer->lparam = req->lparam;
    }
}

/* kill a window timer */
DECL_HANDLER(kill_win_timer)
{
    struct msg_queue *queue = current->queue;

    if (!queue || !kill_timer( queue, req->win, req->msg, req->id ))
        set_error( STATUS_INVALID_PARAMETER );
}

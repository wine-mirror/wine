
#ifndef _SERVER_ASYNC_
#define _SERVER_ASYNC_

#include <sys/time.h>
#include "object.h"

struct async_queue;

struct async 
{
    struct object       *obj;
    struct thread       *thread;
    void                *func;
    void                *overlapped;
    unsigned int         status;
    struct timeval       when;
    struct timeout_user *timeout;
    struct async        *next,*prev;
    struct async_queue  *q;
};

struct async_queue
{
    struct async        *head;
    struct async        *tail;
};

void destroy_async( struct async *async );
void destroy_async_queue( struct async_queue *q );
void async_notify(struct async *async, int status);
struct async *find_async(struct async_queue *q, struct thread *thread, void *overlapped);
void async_insert(struct async_queue *q, struct async *async);
struct async *create_async(struct object *obj, struct thread *thread, 
                           void *func, void *overlapped);
void async_add_timeout(struct async *async, int timeout);
static inline void init_async_queue(struct async_queue *q) 
{ 
    q->head = q->tail = NULL;
}

#define IS_READY(q) (((q).head) && ((q).head->status==STATUS_PENDING))

#endif /* _SERVER_ASYNC_ */


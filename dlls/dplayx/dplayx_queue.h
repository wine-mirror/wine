/* A queue definition based on sys/queue.h TAILQ definitions
 * 
 * Blame any implementation mistakes on Peter Hunnisett
 * <hunnise@nortelnetworks.com>
 */

#ifndef __WINE_DPLAYX_QUEUE_H
#define __WINE_DPLAYX_QUEUE_H

#define DPQ_INSERT(a,b,c) DPQ_INSERT_IN_TAIL(a,b,c)

/*
 * Tail queue definitions.
 */
#define DPQ_HEAD(type)                                           \
struct {                                                         \
        struct type *lpQHFirst; /* first element */              \
        struct type **lpQHLast; /* addr of last next element */  \
}

#define DPQ_ENTRY(type)                                               \
struct {                                                              \
        struct type *lpQNext;  /* next element */                     \
        struct type **lpQPrev; /* address of previous next element */ \
}

/*
 * Tail queue functions.
 */
#define DPQ_INIT(head)                       \
do{                                          \
        (head).lpQHFirst = NULL;             \
        (head).lpQHLast = &(head).lpQHFirst; \
} while(0)

#define DPQ_INSERT_IN_TAIL(head, elm, field)     \
do {                                             \
        (elm)->field.lpQNext = NULL;             \
        (elm)->field.lpQPrev = (head).lpQHLast;  \
        *(head).lpQHLast = (elm);                \
        (head).lpQHLast = &(elm)->field.lpQNext; \
} while(0)

#define DPQ_REMOVE(head, elm, field)                    \
do {                                                    \
        if (((elm)->field.lpQNext) != NULL)             \
                (elm)->field.lpQNext->field.lpQPrev =   \
                    (elm)->field.lpQPrev;               \
        else                                            \
                (head).lpQHLast = (elm)->field.lpQPrev; \
        *(elm)->field.lpQPrev = (elm)->field.lpQNext;   \
} while(0)

/* head - pointer to DPQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToEqual
 * fieldToEqual - The value that we're looking for
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_FIND_ENTRY( head, elm, field, fieldToEqual, rc )   \
do {                                                           \
  (rc) = (head).lpQHFirst; /* NULL head? */                    \
                                                               \
  while( rc )                                                  \
  {                                                            \
      /* What we're searching for? */                          \
      if( (rc)->field == (fieldToEqual) )                      \
      {                                                        \
        break; /* rc == correct element */                     \
      }                                                        \
                                                               \
      /* End of list check */                                  \
      if( ( (rc) = (rc)->elm.lpQNext ) == (head).lpQHFirst )   \
      {                                                        \
        rc = NULL;                                             \
        break;                                                 \
      }                                                        \
  }                                                            \
} while(0)

/* head - pointer to DPQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToEqual
 * fieldToEqual - The value that we're looking for
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_REMOVE_ENTRY( head, elm, field, fieldToEqual, rc ) \
do {                                                           \
  DPQ_FIND_ENTRY( head, elm, field, fieldToEqual, rc );        \
                                                               \
  /* Was the element found? */                                 \
  if( rc )                                                     \
  {                                                            \
    DPQ_REMOVE( head, rc, elm );                               \
  }                                                            \
} while(0)

#endif /* __WINE_DPLAYX_QUEUE_H */

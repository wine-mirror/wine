/* A queue definition based on sys/queue.h TAILQ definitions
 * 
 * Blame any implementation mistakes on Peter Hunnisett
 * <hunnise@nortelnetworks.com>
 */

#ifndef __WINE_DPLAYX_QUEUE_H
#define __WINE_DPLAYX_QUEUE_H

#include "winbase.h"

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

/* Front of the queue */
#define DPQ_FIRST( head ) ( (head).lpQHFirst )

/* Check if the queue has any elements */
#define DPQ_IS_EMPTY( head ) ( DPQ_FIRST(head) == NULL ) 

/* Next entry -- FIXME: Convert everything over to this macro ... */
#define DPQ_NEXT( elem ) (elem).lpQNext

#define DPQ_IS_ENDOFLIST( elem ) \
    ( DPQ_NEXT(elem) == NULL )

/* Insert element at end of queue */
#define DPQ_INSERT_IN_TAIL(head, elm, field)     \
do {                                             \
        (elm)->field.lpQNext = NULL;             \
        (elm)->field.lpQPrev = (head).lpQHLast;  \
        *(head).lpQHLast = (elm);                \
        (head).lpQHLast = &(elm)->field.lpQNext; \
} while(0)

/* Remove element from the queue */
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
 * field - to be concatenated to rc to compare with fieldToCompare
 * fieldToCompare - The value that we're comparing against
 * fieldCompareOperator - The logical operator to compare field and
 *                        fieldToCompare. 
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_FIND_ENTRY( head, elm, field, fieldCompareOperator, fieldToCompare, rc )\
do {                                                           \
  (rc) = (head).lpQHFirst; /* NULL head? */                    \
                                                               \
  while( rc )                                                  \
  {                                                            \
      /* What we're searching for? */                          \
      if( (rc)->field fieldCompareOperator (fieldToCompare) )  \
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
 * fieldToCompare - The value that we're comparing against
 * fieldCompareOperator - The logical operator to compare field and
 *                        fieldToCompare.
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_REMOVE_ENTRY( head, elm, field, fieldCompareOperator, fieldToCompare, rc )\
do {                                                           \
  DPQ_FIND_ENTRY( head, elm, field, fieldCompareOperator, fieldToCompare, rc );\
                                                               \
  /* Was the element found? */                                 \
  if( rc )                                                     \
  {                                                            \
    DPQ_REMOVE( head, rc, elm );                               \
  }                                                            \
} while(0)

/* Delete the entire queue 
 * head - pointer to the head of the queue
 * field - field to access the next elements of the queue
 * type - type of the pointer to the element element
 * df - a delete function to be called. Declared with DPQ_DECL_DELETECB.
 */
#define DPQ_DELETEQ( head, field, type, df )            \
while( !DPQ_IS_EMPTY(head) )                               \
{                                                       \
  type holder = (head).lpQHFirst;                      \
  DPQ_REMOVE( head, holder, field );                    \
  df( holder );                                \
}

/* How to define the method to be passed to DPQ_DELETEQ */
#define DPQ_DECL_DELETECB( name, type ) void name( type elem )

/* Prototype of a method which just performs a HeapFree on the elem */
DPQ_DECL_DELETECB( cbDeleteElemFromHeap, LPVOID );

#endif /* __WINE_DPLAYX_QUEUE_H */


/* Helper functions for TAILQ functions defined in <sys/queue.h> 
 * 
 * Blame any implementation mistakes on Peter Hunnisett
 * <hunnise@nortelnetworks.com>
 */

#ifndef __WINE_DPLAYX_QUEUE_H
#define __WINE_DPLAYX_QUEUE_H

#include <sys/queue.h>

/* head - pointer to TAILQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToEqual
 * fieldToEqual - The value that we're looking for
 * rc - Variable to put the return code. Same type as (head)->tqh_first
 */
#define TAILQ_FIND_ENTRY( head, elm, field, fieldToEqual, rc ) { \
  (rc) = (head)->tqh_first; /* NULL head? */                    \
                                                                 \
  while( rc )                                                    \
  {                                                              \
      /* What we're searching for? */                            \
      if( (rc)->field == (fieldToEqual) )                        \
      {                                                          \
        break; /* rc == correct element */                       \
      }                                                          \
                                                                 \
      /* End of list check */                                    \
      if( ( (rc) = (rc)->elm.tqe_next ) == (head)->tqh_first )   \
      {                                                          \
        rc = NULL;                                               \
        break;                                                   \
      }                                                          \
  }                                                              \
}

#endif /* __WINE_DPLAYX_QUEUE_H */

static char RCSId[] = "$Id: heap.c,v 1.1 1993/06/29 15:55:18 root Exp $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"

typedef struct heap_mem_desc_s
{
    struct heap_mem_desc_s *prev, *next;
    int   length;
} MDESC;

MDESC *FreeList;

/**********************************************************************
 *					HEAP_LocalInit
 */
void
HEAP_LocalInit(void *start, int length)
{
    FreeList = (MDESC *) start;
    FreeList->prev = NULL;
    FreeList->next = NULL;
    FreeList->length = length - sizeof(MDESC);
}

/**********************************************************************
 *					HEAP_LocalAlloc
 */
void *
HEAP_LocalAlloc(int flags, int bytes)
{
    MDESC *m, *m_new;
    
#ifdef HEAP_DEBUG
    printf("LocalAlloc: flags %x, bytes %d, ", flags, bytes);
#endif

    /*
     * Find free block big enough.
     */
    for (m = FreeList; m != NULL; m = m->next)
    {
	if (m->length == bytes && m->length < bytes + 4 * sizeof(MDESC))
	{
	    break;
	}
	else if (m->length > bytes)
	{
	    m_new = m + (bytes / sizeof(MDESC)) + 2;
	    if (m->prev == NULL)
		FreeList = m_new;
	    else
		m->prev->next = m_new;
	    
	    if (m->next != NULL)
		m->next->prev = m_new;
	    
	    m_new->next = m->next;
	    m_new->prev = m->prev;
	    m_new->length = m->length - ((int) m_new - (int) m);
	    m->length -= (m_new->length + sizeof(MDESC));

#ifdef HEAP_DEBUG
	    printf("Returning %x\n", (int) (m + 1));
#endif
	    return (void *) (m + 1);
	}
    }

    if (m != NULL)
    {
	if (m->prev == NULL)
	    FreeList = m->next;
	else
	    m->prev->next = m->next;
	
	if (m->next != NULL)
	    m->next->prev = m->prev;
	
#ifdef HEAP_DEBUG
	printf("Returning %x\n", (int) (m + 1));
#endif
	return (void *) (m + 1);
    }

#ifdef HEAP_DEBUG
    printf("Returning 0\n");
#endif
    return 0;
}


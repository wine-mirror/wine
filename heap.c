static char RCSId[] = "$Id: heap.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "segmem.h"
#include "heap.h"

MDESC *LOCAL_FreeList;

/**********************************************************************
 *					HEAP_Init
 */
void
HEAP_Init(MDESC **free_list, void *start, int length)
{
    *free_list = (MDESC *) start;
    (*free_list)->prev = NULL;
    (*free_list)->next = NULL;
    (*free_list)->length = length - sizeof(MDESC);
}

/**********************************************************************
 *					HEAP_Alloc
 */
void *
HEAP_Alloc(MDESC **free_list, int flags, int bytes)
{
    MDESC *m, *m_new;
    
#ifdef DEBUG_HEAP
    printf("HeapAlloc: free_list %08x, flags %x, bytes %d\n", 
	   free_list, flags, bytes);
#endif

    /*
     * Find free block big enough.
     */
    for (m = *free_list; m != NULL; m = m->next)
    {
	if (m->length >= bytes && m->length < bytes + 4 * sizeof(MDESC))
	{
	    break;
	}
	else if (m->length > bytes)
	{
	    m_new = m + (bytes / sizeof(MDESC)) + 2;
	    if (m->prev == NULL)
		*free_list = m_new;
	    else
		m->prev->next = m_new;
	    
	    if (m->next != NULL)
		m->next->prev = m_new;
	    
	    m_new->next = m->next;
	    m_new->prev = m->prev;
	    m_new->length = m->length - ((int) m_new - (int) m);
	    m->length -= (m_new->length + sizeof(MDESC));

	    m->prev = m;
	    m->next = m;
	    if (flags & GLOBAL_FLAGS_ZEROINIT)
		memset(m + 1, 0, bytes);
#ifdef DEBUG_HEAP
	    printf("HeapAlloc: returning %08x\n", (m + 1));
#endif
	    return (void *) (m + 1);
	}
    }

    if (m != NULL)
    {
	if (m->prev == NULL)
	    *free_list = m->next;
	else
	    m->prev->next = m->next;
	
	if (m->next != NULL)
	    m->next->prev = m->prev;
	
	m->prev = m;
	m->next = m;
	if (flags & GLOBAL_FLAGS_ZEROINIT)
	    memset(m + 1, 0, bytes);
#ifdef DEBUG_HEAP
	printf("HeapAlloc: returning %08x\n", (m + 1));
#endif
	return (void *) (m + 1);
    }

#ifdef DEBUG_HEAP
    printf("HeapAlloc: returning %08x\n", 0);
#endif
    return 0;
}

/**********************************************************************
 *					HEAP_ReAlloc
 */
void *
HEAP_ReAlloc(MDESC **free_list, void *old_block, 
	     int new_size, unsigned int flags)
{
    return 0;
}


/**********************************************************************
 *					HEAP_Free
 */
void
HEAP_Free(MDESC **free_list, void *block)
{
    MDESC *m_free;
    MDESC *m;
    MDESC *m_prev;

    /*
     * Validate pointer.
     */
    m_free = (MDESC *) block - 1;
    if (m_free->prev != m_free || m_free->next != m_free || 
	((int) m_free & 0xffff0000) != ((int) *free_list & 0xffff0000))
    {
#ifdef DEBUG_HEAP
	printf("Attempt to free bad pointer,"
	       "m_free = %08x, *free_list = %08x\n",
	       m_free, free_list);
#endif
	return;
    }

    /*
     * Find location in free list.
     */
    m_prev = NULL;
    for (m = *free_list; m != NULL && m < m_free; m = m->next)
	m_prev = m;
    
    if (m_prev != NULL && (int) m_prev + m_prev->length > (int) m_free)
    {
#ifdef DEBUG_HEAP
	printf("Attempt to free bad pointer,"
	       "m_free = %08x, m_prev = %08x (length %x)\n",
	       m_free, m_prev, m_prev->length);
#endif
	return;
    }
	
    if ((m != NULL && (int) m_free + m_free->length > (int) m) ||
	(int) m_free + m_free->length > ((int) m_free | 0xffff))
    {
#ifdef DEBUG_HEAP
	printf("Attempt to free bad pointer,"
	       "m_free = %08x (length %x), m = %08x\n",
	       m_free, m_free->length, m);
#endif
	return;
    }

    /*
     * Put block back in free list.
     * Does it merge with the previos block?
     */
    if (m_prev != NULL)
    {
	if ((int) m_prev + m_prev->length == (int) m_free)
	{
	    m_prev->length += sizeof(MDESC) + m_free->length;
	    m_free = m_prev;
	}
	else
	{
	    m_prev->next = m_free;
	    m_free->prev = m_prev;
	}
    }
    else
    {
	*free_list = m_free;
	m_free->prev = NULL;
    }
    
    /*
     * Does it merge with the next block?
     */
    if (m != NULL)
    {
	if ((int) m_free + m_free->length == (int) m)
	{
	    m_free->length += sizeof(MDESC) + m->length;
	    m_free->next = m->next;
	}
	else
	{
	    m->prev = m_free;
	    m_free->next = m;
	}
    }
    else
    {
	m_free->next = NULL;
    }
}

/**********************************************************************
 *					HEAP_LocalInit
 */
void
HEAP_LocalInit(void *start, int length)
{
    HEAP_Init(&LOCAL_FreeList, start, length);
}

/**********************************************************************
 *					HEAP_LocalAlloc
 */
void *
HEAP_LocalAlloc(int flags, int bytes)
{
    void *m;
    
#ifdef DEBUG_HEAP
    printf("LocalAlloc: flags %x, bytes %d\n", flags, bytes);
#endif

    m = HEAP_Alloc(&LOCAL_FreeList, flags, bytes);
	
#ifdef DEBUG_HEAP
	printf("LocalAlloc: returning %x\n", (int) m);
#endif
    return m;
}

/**********************************************************************
 *					HEAP_LocalCompact
 */
int
HEAP_LocalCompact(int min_free)
{
    MDESC *m;
    int max_block;
    
    max_block = 0;
    for (m = LOCAL_FreeList; m != NULL; m = m->next)
	if (m->length > max_block)
	    max_block = m->length;
    
    return max_block;
}

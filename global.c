static char RCSId[] = "$Id: global.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "heap.h"
#include "segmem.h"

/*
 * Global memory pool descriptor.
 *
 * handle  = 0, this descriptor contains the address of a free pool.
 *        != 0, this describes an allocated block.
 *
 * sequence = 0, this is not a huge block
 *          > 0, this is a portion of a huge block
 *          =-1, this is a free segment
 *
 * addr	      - address of this memory block.
 *
 * length     - used to maintain huge blocks.
 *
 */
typedef struct global_mem_desc_s
{
    struct global_mem_desc_s *next;
    struct global_mem_desc_s *prev;
    unsigned short handle;
    short sequence;
    void *addr;
    int length;
} GDESC;

GDESC *GlobalList = NULL;

/**********************************************************************
 *					GLOBAL_GetFreeSegments
 */
GDESC *
GLOBAL_GetFreeSegments(unsigned int flags, int n_segments)
{
    struct segment_descriptor_s *s;
    GDESC *g;
    GDESC *g_start;
    GDESC *g_prev;
    int count, i;

    /*
     * Try to find some empty segments in our list.
     */
    count = 0;
    for (g = GlobalList; g != NULL && count != n_segments; g = g->next)
    {
	if ((int) g->sequence == -1)
	{
	    if (count > 0)
	    {
		if (g->prev->handle + 8 != g->handle)
		    count = 0;
		else
		    count++;
	    }
	    else
	    {
		g_start = g;
		count = 1;
	    }
	}
	else if (count)
	    count = 0;
    }

    /*
     * If we couldn't find enough segments, then we need to create some.
     */
    if (count != n_segments)
    {
	/*
	 * Find list tail.
	 */
	g_prev = NULL;
	for (g = GlobalList; g != NULL; g = g->next)
	    g_prev = g;

	/*
	 * Allocate segments.
	 */
	for (count = 0; count < n_segments; count++)
	{
	    s = GetNextSegment(flags, 0x10000);
	    if (s == NULL)
		return NULL;

	    g = (GDESC *) malloc(sizeof(*g));
	    
	    g->prev = g_prev;
	    g->next = NULL;
	    g->handle = s->selector;
	    g->sequence = -1;
	    g->addr = s->base_addr;
	    g->length = s->length;

	    free(s);

	    if (count == 0)
		g_start = g;

	    if (g_prev != NULL)
	    {
		g_prev->next = g;
		g->prev = g_prev;
	    }
	    else
		GlobalList = g;
	}
    }

    /*
     * We have all of the segments we need.  Let's adjust their contents.
     */
    g = g_start;
    for (i = 0; i < n_segments; i++, g = g->next)
    {
	g->sequence = i + 1;
	g->length = n_segments;
    }

    return g_start;
}

/**********************************************************************
 *					GLOBAL_Alloc
 */
unsigned int
GLOBAL_Alloc(unsigned int flags, unsigned long size)
{
    GDESC *g;
    GDESC *g_prev;
    void *m;
    int i;

    /*
     * If this block is fixed or very big we need to allocate entire
     * segments.
     */
    if (size > 0x8000 || !(flags & GLOBAL_FLAGS_MOVEABLE))
    {
	int segments = (size >> 16) + 1;

	g = GLOBAL_GetFreeSegments(flags, segments);
	if (g == NULL)
	    return 0;
	else
	    return g->handle;
    }
    /*
     * Otherwise we just need a little piece of a segment.
     */
    else
    {
	/*
	 * Try to allocate from active free lists.
	 */
	for (g = GlobalList; g != NULL; g = g->next)
	{
	    if (g->handle == 0 && g->sequence == 0)
	    {
		m = HEAP_Alloc((MDESC **) g->addr, 0, size);
		if (m != NULL)
		    break;
	    }
	}

	/*
	 * If we couldn't get the memory there, then we need to create
	 * a new free list.
	 */
	if (m == NULL)
	{
	    g = GLOBAL_GetFreeSegments(0, 1);
	    if (g == NULL)
		return 0;

	    g->handle = 0;
	    g->sequence = 0;
	    HEAP_Init((MDESC **) g->addr, (MDESC **) g->addr + 1, 
		      0x10000 - sizeof(MDESC **));
	    m = HEAP_Alloc((MDESC **) g->addr, 0, size);
	    if (m == NULL)
		return 0;
	}

	/*
	 * We have a new block.  Let's create a GDESC entry for it.
	 */
	g_prev = NULL;
	i = 0;
	for (g = GlobalList; g != NULL; g = g->next, i++)
	    g_prev = g;

	g = malloc(sizeof(*g));
	if (g == NULL)
	    return 0;

	g->handle = i << 3;
	g->sequence = 0;
	g->addr = m;
	g->length = size;
	g->next = NULL;

	if (g_prev != NULL)
	{
	    g_prev->next = g;
	    g->prev = g_prev;
	}
	else
	{
	    GlobalList = g;
	    g->prev = NULL;
	}
	
	return g->handle;
    }
}

/**********************************************************************
 *					GLOBAL_Free
 *
 * Windows programs will pass a handle in the "block" parameter, but
 * this function will also accept a 32-bit address.
 */
unsigned int
GLOBAL_Free(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    if (block & 0xffff0000)
    {
	for (g = GlobalList; g != NULL; g = g->next)
	    if (g->handle > 0 && (unsigned int) g->addr == block)
		break;
    }
    else
    {
	for (g = GlobalList; g != NULL; g = g->next)
	    if (g->handle == block)
		break;
    }
    if (g == NULL)
	return block;

    /*
     * If the sequence number is zero then use HEAP_Free to deallocate
     * memory, and throw away this descriptor.
     */
    if (g->sequence == 0)
    {
	HEAP_Free((MDESC **) (block & 0xffff0000), (void *) block);

	if (g->prev != NULL)
	    g->prev->next = g->next;
	else
	    GlobalList = g->next;
	
	if (g->next != NULL)
	    g->next->prev = g->prev;

	free(g);
    }
    
    /*
     * Otherwise just mark these descriptors as free.
     */
    else
    {
	int i, limit;
	
	g->length;
	for (i = 0; i < limit; i++)
	{
	    g->sequence = -1;
	    g->length = 0x10000;
	}
    }

    return 0;
}

/**********************************************************************
 *					GLOBAL_Lock
 *
 */
void *
GLOBAL_Lock(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
	if (g->handle == block)
	    return g->addr;

    return NULL;
}

static char RCSId[] = "$Id: global.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "heap.h"
#include "segmem.h"

/*
 * Global memory pool descriptor.  Segments MUST be maintained in segment
 * ascending order.  If not the reallocation routine will die a horrible
 * death.
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
    int lock_count;
} GDESC;

GDESC *GlobalList = NULL;
static unsigned short next_unused_handle = 1;


/**********************************************************************
 *					GlobalGetFreeSegments
 */
GDESC *
GlobalGetFreeSegments(unsigned int flags, int n_segments)
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
	    if (!(flags & GLOBAL_FLAGS_MOVEABLE))
		g->lock_count = 1;
	    else
		g->lock_count = 0;
	    
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
 *					GlobalAlloc
 */
unsigned int
GlobalAlloc(unsigned int flags, unsigned long size)
{
    GDESC *g;
    GDESC *g_prev;
    void *m;

    /*
     * If this block is fixed or very big we need to allocate entire
     * segments.
     */
    if (size > 0x8000 || !(flags & GLOBAL_FLAGS_MOVEABLE))
    {
	int segments = (size >> 16) + 1;

	g = GlobalGetFreeSegments(flags, segments);
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
	if (g == NULL)
	{
	    g = GlobalGetFreeSegments(0, 1);
	    if (g == NULL)
		return 0;

	    g->handle = 0;
	    g->sequence = 0;
	    HEAP_Init((MDESC **) g->addr, (MDESC **) g->addr + 1, 
		      0x10000 - sizeof(MDESC **));
	    m = HEAP_Alloc((MDESC **) g->addr, flags & GLOBAL_FLAGS_ZEROINIT, 
			   size);
	    if (m == NULL)
		return 0;
	}

	/*
	 * Save position of heap descriptor.
	 */
	g_prev = g;

	/*
	 * We have a new block.  Let's create a GDESC entry for it.
	 */
	g = malloc(sizeof(*g));
#ifdef DEBUG_HEAP
	printf("New GDESC %08x\n", g);
#endif
	if (g == NULL)
	    return 0;

	g->handle = next_unused_handle;
	g->sequence = 0;
	g->addr = m;
	g->length = size;
	g->next = g_prev->next;
	if (g->next) g->next->prev = g;
	g->lock_count = 0;

	g_prev->next = g;
	g->prev = g_prev;

	next_unused_handle++;
	if ((next_unused_handle & 7) == 7)
	    next_unused_handle++;
	
#ifdef DEBUG_HEAP
	printf("GlobalAlloc: returning %04x\n", g->handle);
#endif
	return g->handle;
    }
}

/**********************************************************************
 *					GlobalFree
 *
 * Windows programs will pass a handle in the "block" parameter, but
 * this function will also accept a 32-bit address.
 */
unsigned int
GlobalFree(unsigned int block)
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
	HEAP_Free((MDESC **) ((int) g->addr & 0xffff0000), (void *) g->addr);

	g->prev->next = g->next;
	
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
	
	limit = g->length;
	for (i = g->sequence - 1; i < limit && g != NULL; i++, g = g->next)
	{
	    g->sequence = -1;
	    g->length = 0x10000;
	}
    }

    return 0;
}

/**********************************************************************
 *					GlobalLock
 *
 */
void *
GlobalLock(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	if (g->handle == block)
	{
	    g->lock_count++;
#ifdef DEBUG_HEAP
	    printf("GlobalLock: returning %08x\n", g->addr);
#endif
	    return g->addr;
	}
    }

#ifdef DEBUG_HEAP
    printf("GlobalLock: returning %08x\n", 0);
#endif
    return NULL;
}

/**********************************************************************
 *					GlobalUnlock
 *
 */
int
GlobalUnlock(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	if (g->handle == block && g->lock_count > 0)
	{
	    g->lock_count--;
	    return 0;
	}
    }

    return 1;
}

/**********************************************************************
 *					GlobalFlags
 *
 */
unsigned int
GlobalFlags(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	if (g->handle == block)
	    return g->lock_count;
    }

    return 0;
}

/**********************************************************************
 *					GlobalSize
 *
 */
unsigned int
GlobalSize(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	if (g->handle == block)
	    return g->length;
    }

    return 0;
}

/**********************************************************************
 *					GlobalHandle
 *
 * This routine is not strictly correct.  MS Windows creates a selector
 * for every locked global block.  We do not.  If the allocation is small
 * enough, we only give out a little piece of a selector.  Thus this
 * function cannot be implemented.
 */
unsigned int
GlobalHandle(unsigned int selector)
{
    GDESC *g;

    if (selector == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	if (g->handle == selector)
	{
	    if (g->sequence > 0)
		return g->handle;
	    else
	    {
		fprintf(stderr, "Attempt to get a handle "
			"from a selector to a far heap.\n");
		return 0;
	    }
	}
    }

    return 0;
}

/**********************************************************************
 *					GlobalCompact
 *
 */
unsigned int
GlobalCompact(unsigned int desired)
{
    GDESC *g;
    unsigned char free_map[512];
    unsigned int max_selector_used = 0;
    unsigned int i;
    unsigned int selector;
    int current_free;
    int max_free;

    /*
     * Initialize free list to all items not controlled by GlobalAlloc()
     */
    for (i = 0; i < 512; i++)
	free_map[i] = -1;

    /*
     * Traverse table looking for used and free selectors.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	/*
	 * Check for free segments.
	 */
	if (g->sequence == -1)
	{
	    free_map[g->handle >> 3] = 1;
	    if (g->handle > max_selector_used)
		max_selector_used = g->handle;
	}

	/*
	 * Check for heap allocated segments.
	 */
	else if (g->handle == 0)
	{
	    selector = (unsigned int) g->addr >> 16;
	    free_map[selector >> 3] = 0;
	    if (selector > max_selector_used)
		max_selector_used = selector;
	}
    }

    /*
     * All segments past the biggest selector used are free.
     */
    for (i = (max_selector_used >> 3) + 1; i < 512; i++)
	free_map[i] = 1;

    /*
     * Find the largest free block of segments
     */
    current_free = 0;
    max_free = 0;
    for (i = 0; i < 512; i++)
    {
	if (free_map[i] == 1)
	{
	    current_free++;
	}
	else
	{
	    if (current_free > max_free)
		max_free = current_free;
	    current_free = 0;
	}
    }
    
    return max_free << 16;
}

/**********************************************************************
 *					GlobalReAlloc
 *
 */
unsigned int
GlobalReAlloc(unsigned int block, unsigned int new_size, unsigned int flags)
{
    GDESC *g;
    unsigned int n_segments;
    int i;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
    {
	if (g->handle == block)
	    break;
    }

    if (g == NULL)
	return 0;
    
    /*
     * If this is a heap allocated block,  then use HEAP_ReAlloc() to
     * reallocate the block.  If this fails, call GlobalAlloc() to get
     * a new block.
     */
    if (g->sequence = 0)
    {
	MDESC **free_list;
	void *p;
	
	free_list = (MDESC **) ((unsigned int) g->addr & 0xffff0000);
	p = HEAP_ReAlloc(free_list, g->addr, new_size, flags) ;
	if (p == NULL)
	{
	    unsigned int handle = GlobalAlloc(flags, new_size);
	    if (handle == 0)
		return 0;
	    p = GlobalLock(handle);
	    memcpy(p, g->addr, g->length);
	    GlobalUnlock(handle);
	    GlobalFree(g->handle);

	    return handle;
	}
	else
	{
	    g->addr = p;
	    g->length = new_size;
	    return g->handle;
	}
    }
    
    /*
     * Otherwise, we need to do the work ourselves.  First verify the
     * handle.
     */
    else
    {
	if (g->sequence != 1)
	    return 0;

	/*
	 * Do we need more memory?  Segments are in ascending order in 
	 * the GDESC list.
	 */
	n_segments = (new_size >> 16) + 1;
        if (n_segments > g->length)
	{
	    GDESC *g_new;
	    GDESC *g_start = g;
	    int old_segments = g_start->length;
	    unsigned short next_handle = g_start->handle;
	    
	    for (i = 1; i <= n_segments; i++, g = g->next)
	    {
		/*
		 * If we run into a block allocated to something else,
		 * try GlobalGetFreeSegments() and memcpy(). (Yuk!)
		 */
		if (g->sequence != i || g->handle != next_handle)
		{
		    g = GlobalGetFreeSegments(flags, n_segments);
		    if (g == NULL)
			return 0;
		    
		    memcpy(g->addr, g_start->addr,
			   g_start->length << 16);

		    GlobalFree(block);
		    return g->handle;
		}

		/*
		 * Otherwise this block is used by us or free.  So,
		 * snatch it.  If this block is new and we are supposed to
		 * zero init, then do some erasing.
		 */
		if (g->sequence == -1 && (flags & GLOBAL_FLAGS_ZEROINIT))
		    memset(g->addr, 0, 0x10000);
		
		g->sequence = i;
		g->length = n_segments;
		next_handle += 8;

		/*
		 * If the next descriptor is non-existant, then use
		 * GlobalGetFreeSegments to create them.
		 */
		if (i != n_segments && g->next == NULL)
		{
		    g_new = GlobalGetFreeSegments(flags, n_segments - i);
		    if (g_new == NULL)
			return 0;
		    GlobalFree(g_new->handle);
		}
	    }

	    return g_start->handle;
	}
	
	/*
	 * Do we need less memory?
	 */
	else if (n_segments < g->length)
	{
	    GDESC *g_free;
	    
	    g_free = g;
	    for (i = 0; i < n_segments; i++)
	    {
		if (g_free->sequence != i + 1)
		    return 0;
		g_free = g_free->next;
	    }
	}
	
	/*
	 * We already have exactly the right amount of memory.
	 */
	else
	    return block;
    }

    /*
     * If we fall through it must be an error.
     */
    return 0;
}

/**********************************************************************
 *					GlobalQuickAlloc
 */
void *
GlobalQuickAlloc(int size)
{
    unsigned int hmem;
    
    hmem = GlobalAlloc(GLOBAL_FLAGS_MOVEABLE, size);
    if (hmem == 0)
	return NULL;
    else
	return GlobalLock(hmem);
}

/**********************************************************************
 *					GlobalHandleFromPointer

 */
unsigned int
GlobalHandleFromPointer(void *block)
{
    GDESC *g;

    if (block == NULL)
	return 0;

    /*
     * Find GDESC for this block.
     */
    for (g = GlobalList; g != NULL; g = g->next)
	if (g->handle > 0 && g->addr == block)
	    break;

    if (g == NULL)
	return 0;
    else
	return g->handle;
}




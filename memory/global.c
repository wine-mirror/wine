static char RCSId[] = "$Id: global.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#define GLOBAL_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prototypes.h"
#include "toolhelp.h"
#include "heap.h"
#include "segmem.h"
#include "stddebug.h"
/* #define DEBUG_HEAP /* */
/* #undef  DEBUG_HEAP /* */
#include "debug.h"


GDESC *GlobalList = NULL;
static unsigned short next_unused_handle = 1;

/**********************************************************************
 *					GlobalGetGDesc
 */
GDESC *GlobalGetGDesc(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return NULL;

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

    return g;
}

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
	s = CreateNewSegments(0, 0, 0x10000, n_segments);
	if (s == NULL) 
	{
	    fprintf(stderr,"GlobalGetFreeSegments // bad CreateNewSegments !\n");
	    return NULL;
	}
	for (count = 0; count < n_segments; count++, s++)
	{
	    g = (GDESC *) malloc(sizeof(*g));
	    if (g == NULL) 
	    {
		fprintf(stderr,"GlobalGetFreeSegments // bad GDESC malloc !\n");
		return NULL;
	    }
	    g->prev         = g_prev;
	    g->next         = NULL;
	    g->handle       = s->selector;
	    g->sequence     = -1;
	    g->addr         = s->base_addr;
	    g->length       = s->length;
	    g->alias        = 0;
	    g->linear_addr  = NULL;
	    g->linear_key   = 0;
	    g->linear_count = 0;
	    if (!(flags & GLOBAL_FLAGS_MOVEABLE))
		g->lock_count = 1;
	    else
		g->lock_count = 0;
	    
	    if (count == 0) g_start = g;

	    if (g_prev != NULL)
	    {
		g_prev->next = g;
	    }
	    else
		GlobalList = g;
	    g_prev = g;
	}
    }

    /*
     * We have all of the segments we need.  Let's adjust their contents.
     */
    g = g_start;
    for (i = 0; i < n_segments; i++, g = g->next)
    {
	if (g == NULL) 
	{
	    fprintf(stderr,"GlobalGetFreeSegments // bad Segments chain !\n");
	    return NULL;
	}
	g->sequence     = i + 1;
	g->length       = n_segments;
	g->alias        = 0;
	g->linear_addr  = NULL; 
	g->linear_key   = 0;
	g->linear_count = 0;
    }

    return g_start;
}

/**********************************************************************
 *					WIN16_GlobalAlloc
 */
HANDLE
WIN16_GlobalAlloc(unsigned int flags, unsigned long size)
{
    return GlobalAlloc(flags & ~GLOBAL_FLAGS_MOVEABLE, size);
}

/**********************************************************************
 *					GlobalAlloc
 */
HANDLE
GlobalAlloc(unsigned int flags, unsigned long size)
{
    GDESC *g;
    GDESC *g_prev;
    void *m;

    dprintf_heap(stddeb,"GlobalAlloc flags %4X, size %d\n", flags, size);

    if (size == 0) size = 1;

    /*
     * If this block is fixed or very big we need to allocate entire
     * segments.
     */
    if (size > 0x8000 || !(flags & GLOBAL_FLAGS_MOVEABLE))
    {
	int segments = ((size - 1) >> 16) + 1;

	g = GlobalGetFreeSegments(flags, segments);
	if (g == NULL)
	  {
	    dprintf_heap(stddeb, "==> NULL\n");
	    return 0;
	  }
	else
	  {
	    dprintf_heap(stddeb, "==> %04x\n",g->handle);
	    return g->handle;
	  }
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
	      {
		dprintf_heap(stddeb, "==> Null\n");
		return 0;
	      }

	    g->handle = 0;
	    g->sequence = 0;
	    HEAP_Init((MDESC **) g->addr, (MDESC **) g->addr + 1, 
		      0x10000 - sizeof(MDESC **));
	    m = HEAP_Alloc((MDESC **) g->addr, flags & GLOBAL_FLAGS_ZEROINIT, 
			   size);
	    if (m == NULL)
	      {
		dprintf_heap(stddeb, "==> Null\n");
		return 0;
	      }
	}

	/*
	 * Save position of heap descriptor.
	 */
	g_prev = g;

	/*
	 * We have a new block.  Let's create a GDESC entry for it.
	 */
	g = malloc(sizeof(*g));
	dprintf_heap(stddeb,"New GDESC %08x\n", g);
	if (g == NULL)
	    return 0;

	g->handle       = next_unused_handle;
	g->sequence     = 0;
	g->addr         = m;
	g->alias        = 0;
	g->linear_addr  = NULL;
	g->linear_key   = 0;
	g->linear_count = 0;
	g->length       = size;
	g->next         = g_prev->next;
	if (g->next) g->next->prev = g;
	g->lock_count = 0;

	g_prev->next = g;
	g->prev = g_prev;

	next_unused_handle++;
	if ((next_unused_handle & 7) == 7)
	    next_unused_handle++;
	
	dprintf_heap(stddeb,"GlobalAlloc: returning %04x\n", g->handle);
	return g->handle;
    }
}

/**********************************************************************
 *					GlobalFree
 *
 * Windows programs will pass a handle in the "block" parameter, but
 * this function will also accept a 32-bit address.
 */
HANDLE
GlobalFree(unsigned int block)
{
    GDESC *g;

    if (block == 0)
	return 0;

    /*
     * Find GDESC for this block.
     */
    g = GlobalGetGDesc(block);
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

    if ((g = GlobalGetGDesc(block)) == NULL)
	return 0;

    g->lock_count++;

    dprintf_heap(stddeb,"GlobalLock: returning %08x\n", g->addr);
    return g->addr;
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
    GDESC *g = GlobalGetGDesc(block);
    
    if (g == NULL)
	return 0;

    if (g->sequence == 0)
    {
	MDESC *m = (MDESC *) g->addr - 1;
	
	return m->length;
    }
    else if (g->sequence >= 1)
    {
	return g->length * 0x10000;
    }
    
    return g->length;
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
		return MAKELONG(g->handle, selector);
	    else
	    {
		fprintf(stderr, "Attempt to get a handle "
			"from a selector to a far heap.\n");
		return 0;
	    }
	}
    }
	dprintf_heap(stddeb, "GlobalHandle ==> Null\n");
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
    g = GlobalGetGDesc(block);
    if (g == NULL)
	return 0;
    
    /*
     * If this is a heap allocated block,  then use HEAP_ReAlloc() to
     * reallocate the block.  If this fails, call GlobalAlloc() to get
     * a new block.
     */
    if (g->sequence == 0)
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
	    int old_length = g->length;
	    
	    g_free = g;
	    for (i = 0; i < n_segments; i++)
	    {
		if (g_free->sequence != i + 1)
		    return 0;
		g_free->length = n_segments;
		g_free = g_free->next;
	    }

	    for ( ; i < old_length; i++)
	    {
		g_free->length = 0x10000;
		g_free->sequence = -1;
		g_free = g_free->next;
	    }

	    return g->handle;
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

/**********************************************************************
 *                      GetFreeSpace (kernel.169)

 */
DWORD GetFreeSpace(UINT wFlags)
/* windows 3.1 doesn't use the wFlags parameter !! 
   (so I won't either)                            */
{
    GDESC *g;
    unsigned char free_map[512];
    unsigned int max_selector_used = 0;
    unsigned int i;
    unsigned int selector;
    int total_free;

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
     * Add up the total free segments (obviously this amount of memory
       may not be contiguous, use GlobalCompact to get largest contiguous
       memory available).
     */
    total_free=0;
    for (i = 0; i < 512; i++)
	if (free_map[i] == 1)
	    total_free++;

    dprintf_heap(stddeb,"GetFreeSpace // return %ld !\n", total_free << 16);
    return total_free << 16;
}

/**********************************************************************
 *                      MemManInfo (toolhelp.72)
 */
BOOL MemManInfo(LPMEMMANINFO lpmmi)
{
	return 1;
}

/***********************************************************************
 *           SetSwapAreaSize   (KERNEL.106)
 */
LONG SetSwapAreaSize( WORD size )
{
    dprintf_heap(stdnimp, "STUB: SetSwapAreaSize(%d)\n", size );
    return MAKELONG( size, 0xffff );
}

/***********************************************************************
 *           IsBadCodePtr   (KERNEL.336)
 */
BOOL IsBadCodePtr( FARPROC lpfn )
{
    printf( "STUB: IsBadCodePtr(%p)\n", lpfn );
    return FALSE;
}

/***********************************************************************
 *           IsBadHugeWritePtr  (KERNEL.347)
 */
BOOL IsBadHugeWritePtr( const char *lp, DWORD cb )
{
    return !test_memory(&lp[cb-1], TRUE);
}

/***********************************************************************
 *           IsBadWritePtr   	(KERNEL.335)
 */
BOOL IsBadWritePtr( const char *lp, DWORD cb )
{
    if ((0xffff & (unsigned int) lp) + cb > 0xffff)
	return TRUE;
    return !test_memory(&lp[cb-1], TRUE);
}

/***********************************************************************
 *           IsBadReadPtr      (KERNEL.334)
 */
BOOL IsBadReadPtr( const char *lp, DWORD cb )
{
    if ((0xffff & (unsigned int) lp) + cb > 0xffff)
	return TRUE;
    return !test_memory(&lp[cb-1], FALSE);
}

/***********************************************************************
 *           IsBadHugeReadPtr  (KERNEL.346)
 */
BOOL IsBadHugeReadPtr( const char *lp, DWORD cb )
{
    if ((0xffff & (unsigned int) lp) + cb > 0xffff)
	return TRUE;
    return !test_memory(&lp[cb-1], FALSE);
}

/***********************************************************************
 *           IsBadStringPtr   (KERNEL.337)
 */
BOOL IsBadStringPtr( const char *lp, UINT cb )
{
    if (!IsBadReadPtr(lp, cb+1))
	return FALSE;
    if (lp[cb])
	return FALSE;
    return TRUE;
}

/*
static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1994";
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prototypes.h"
#include "heap.h"
#include "segmem.h"

#ifdef HAVE_IPC
static key_t MemoryKeys[SHMSEG];	/* Keep track of keys were using */
static int   LinearInitialized = 0;
#endif


#ifdef HAVE_IPC
/**********************************************************************
 *					LinearFindSpace
 */
int 
LinearFindSpace(int n_segments)
{
    int i, n;
    
    if (!LinearInitialized)
    {
	memset(MemoryKeys, -1, sizeof(MemoryKeys));
	return 0;
    }

    for (i = 0, n = 0; i < SHMSEG, n != n_segments; i++)
    {
	if (MemoryKeys[i] < 0)
	    n++;
	else
	    n = 0;
    }
    
    if (n != n_segments)
	return -1;
    else
	return i - n;
}
#endif /* HAVE_IPC */

/**********************************************************************
 *					GlobalLinearLock
 *
 * OK, this is an evil beast.  We will do one of two things:
 *
 *	1. If the data item <= 64k, then just call GlobalLock().
 *	2. If the data item > 64k, then map memory.
 */
void *
GlobalLinearLock(unsigned int block)
{
    GDESC *g, *g_first;
    int loc_idx;
    unsigned long addr;
    int i;
    
    /******************************************************************
     * Get GDESC for this block.
     */
    g_first = GlobalGetGDesc(block);
    if (g_first == NULL)
	return 0;

    /******************************************************************
     * Is block less then 64k in length?
     */
    if (g_first->sequence != 1 || g_first->length == 1)
    {
	return (void *) GlobalLock(block);
    }

    /******************************************************************
     * If there is already a linear lock on this memory, then
     * just return a pointer to it.
     */
    if (g_first->linear_count)
    {
	g_first->linear_count++;
	return g_first->linear_addr;
    }
    
    /******************************************************************
     * No luck.  We need to do the linear mapping right now.
     */
#ifdef HAVE_IPC
    loc_idx = LinearFindSpace(g_first->length);
    if (loc_idx < 0)
	return NULL;
    
    addr = (unsigned long) SHM_RANGE_START + (0x10000 * loc_idx);
    g = g_first;
    for (i = loc_idx; 
	 i < loc_idx + g_first->length; 
	 i++, addr += 0x10000, g = g->next)
    {
	if ((MemoryKeys[i] = IPCCopySelector(g->handle >> __AHSHIFT,
					     addr, 0)) < 0)
	    return NULL;
	g->linear_addr = (void *) addr;
	g->linear_count = 1;
    }
#endif /* HAVE_IPC */

    return g_first->linear_addr;
}

/**********************************************************************
 *					GlobalLinearUnlock
 *
 */
unsigned int
GlobalLinearUnlock(unsigned int block)
{
    GDESC *g, *g_first;
    int loc_idx;
    int i;
    
    /******************************************************************
     * Get GDESC for this block.
     */
    g_first = GlobalGetGDesc(block);
    if (g_first == NULL)
	return block;

    /******************************************************************
     * Is block less then 64k in length?
     */
    if (g_first->sequence != 1 || g_first->length == 1)
    {
	return GlobalUnlock(block);
    }

    /******************************************************************
     * Make sure we have a lock on this block.
     */
#ifdef HAVE_IPC
    if (g_first->linear_count > 1)
    {
	g_first->linear_count--;
    }
    else if (g_first->linear_count == 1)
    {
	g = g_first;
	loc_idx = (((unsigned int) g_first - (unsigned int) SHM_RANGE_START) 
		   / 0x10000);
	for (i = 0; i < g_first->length; i++, g = g->next)
	{
	    shmdt(g->linear_addr);
	    g->linear_addr = NULL;
	    MemoryKeys[i] = -1;
	}
	
	g_first->linear_count = 0;
	return 0;
    }
#endif /* HAVE_IPC */

    return 0;
}
/**********************************************************************/

void LinearTest()
{
#if 0
    unsigned int handle;
    int *seg_ptr;
    int *lin_ptr;
    int seg, i;
    int *p;

    handle = GlobalAlloc(0, 0x40000);
    seg_ptr = GlobalLock(handle);
    lin_ptr = GlobalLinearLock(handle);

    for (seg = 0; seg < 4; seg++)
    {
	p = (int *) ((char *) seg_ptr + (0x80000 * seg));
	for (i = 0; i < (0x10000 / sizeof(int)); i++, p++)
	    *p = (seg * (0x10000 / sizeof(int))) + i;
    }
    
    p = lin_ptr;
    for (i = 0; i < (0x40000 / sizeof(int)); i++, p++)
    {
	if (*p != i)
	    printf("lin_ptr[%x] = %x\n", i, *p);
    }
#endif
}

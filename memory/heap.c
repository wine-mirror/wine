/*
static char RCSId[] = "$Id: heap.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prototypes.h"
#include "segmem.h"
#include "heap.h"
#include "regfunc.h"
#include "dlls.h"
#include "stddebug.h"
/* #define DEBUG_HEAP */
#include "debug.h"


LHEAP *LocalHeaps = NULL;

void
HEAP_CheckHeap(MDESC **free_list)
{
    MDESC *m;

    for (m = *free_list; m != NULL; m = m->next)
    {
	if (((int) m & 0xffff0000) != ((int) *free_list & 0xffff0000))
	{   dprintf_heap(stddeb,"Invalid block %08x\n",m);
	    *(char *)0 = 0;
	}
	if (m->prev && (((int) m->prev & 0xffff0000) != ((int) *free_list & 0xffff0000)))
	{   dprintf_heap(stddeb,"Invalid prev %08x from %08x\n", m->prev, m);
	    *(char *)0 = 0;
	}
    }
}

/**********************************************************************
 *					HEAP_Init
 */
void
HEAP_Init(MDESC **free_list, void *start, int length)
{
    if (length < 2 * sizeof(MDESC))
	return;
    
    *free_list = (MDESC *) start;
    (*free_list)->prev   = NULL;
    (*free_list)->next   = NULL;
    (*free_list)->length = length - sizeof(MDESC);
}

/**********************************************************************
 *					HEAP_Alloc
 */
void *
HEAP_Alloc(MDESC **free_list, int flags, int bytes)
{
    MDESC *m, *m_new;
    
    dprintf_heap(stddeb,"HeapAlloc: free_list %08x(%08x), flags %x, bytes %d\n", 
	   (unsigned int) free_list, (unsigned int) *free_list, flags, bytes);
    if(debugging_heap)HEAP_CheckHeap(free_list);

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
	    m->lock = 0;
	    m->flags = 0;
	    if (flags & GLOBAL_FLAGS_ZEROINIT)
		memset(m + 1, 0, bytes);
	    dprintf_heap(stddeb,"HeapAlloc: returning %08x\n", 
	    	(unsigned int) (m + 1));
	    if(debugging_heap)HEAP_CheckHeap(free_list);
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
	m->lock = 0;
	m->flags = 0;
	if (flags & GLOBAL_FLAGS_ZEROINIT)
	    memset(m + 1, 0, bytes);
	dprintf_heap(stddeb,"HeapAlloc: returning %08x\n",
	    	(unsigned int)  (m + 1));
        if(debugging_heap)HEAP_CheckHeap(free_list);
	return (void *) (m + 1);
    }

    dprintf_heap(stddeb,"HeapAlloc: returning %08x\n", 0);
    if(debugging_heap)HEAP_CheckHeap(free_list);
    return 0;
}

/**********************************************************************
 *					HEAP_ReAlloc
 */
void *
HEAP_ReAlloc(MDESC **free_list, void *old_block, 
	     int new_size, unsigned int flags)
{
    MDESC *m_free;
    MDESC *m;


    if (!old_block)
	return HEAP_Alloc(free_list, flags, new_size);
    
    /*
     * Check validity of block
     */
    m = (MDESC *) old_block - 1;

    dprintf_heap(stddeb,"HEAP_ReAlloc new_size=%d !\n", 
	    	(unsigned int) new_size);
    dprintf_heap(stddeb,"HEAP_ReAlloc old_block=%08X !\n",
	    	(unsigned int)  old_block);
    dprintf_heap(stddeb,"HEAP_ReAlloc m=%08X free_list=%08X !\n", 
	    	(unsigned int) m, (unsigned int) free_list);
    dprintf_heap(stddeb,"HEAP_ReAlloc m->prev=%08X !\n",
    	 (unsigned int)  m->prev);
    dprintf_heap(stddeb,"HEAP_ReAlloc m->next=%08X !\n",
    	 (unsigned int)  m->next);
    dprintf_heap(stddeb,"HEAP_ReAlloc *free_list=%08X !\n",
    	 (unsigned int)  *free_list);
    if(debugging_heap)HEAP_CheckHeap(free_list);

    if (m->prev != m || m->next != m || 
	((int) m & 0xffff0000) != ((int) *free_list & 0xffff0000))
    {
	fprintf(stderr,"Attempt to resize bad pointer, m = %08x, *free_list = %08x\n",
	       m, free_list);
	HEAP_CheckHeap(free_list);
	return NULL;
    }
    
    /*
     * Check for grow block
     */

    dprintf_heap(stddeb,"HEAP_ReAlloc Check for grow block !\n");
    if (new_size > m->length)
    {
	m_free = m + 1 + m->length / sizeof(MDESC);
	if (m_free->next == m_free || 
	    m_free->prev == m_free ||
	    m_free->length + 2*sizeof(MDESC) < new_size - m->length)
	{
	    void *new_p = HEAP_Alloc(free_list, flags, new_size);
	    if (new_p ==NULL)
		return NULL;
	    memcpy(new_p, old_block, m->length);
	    HEAP_Free(free_list, old_block);
	    if(debugging_heap)HEAP_CheckHeap(free_list);
	    return new_p;
	}

	if (m_free->prev == NULL)
	    *free_list = m_free->next;
	else
	    m_free->prev->next = m_free->next;
	
	if (m_free->next != NULL)
	    m_free->next->prev = m_free->prev;
	
	m->length += sizeof(MDESC) + m_free->length;

	dprintf_heap(stddeb,"HEAP_ReAlloc before GLOBAL_FLAGS_ZEROINIT !\n");
	if (flags & GLOBAL_FLAGS_ZEROINIT)
	    memset(m_free, '\0', sizeof(MDESC) + m_free->length);
    }

    /*
     * Check for shrink block.
     */
    dprintf_heap(stddeb,"HEAP_ReAlloc Check for shrink block !\n");
    if (new_size + 4*sizeof(MDESC) < m->length)
    {
	m_free = m + new_size / sizeof(MDESC) + 2;
	m_free->next = m_free;
	m_free->prev = m_free;
	m_free->length = m->length - ((int) m_free - (int) m);
	m->length = (int) m_free - (int) (m + 1);
	HEAP_Free(free_list, m_free + 1);
    }
    
    if(debugging_heap)HEAP_CheckHeap(free_list);
    return old_block;
}


/**********************************************************************
 *					HEAP_Free
 */
int
HEAP_Free(MDESC **free_list, void *block)
{
    MDESC *m_free;
    MDESC *m;
    MDESC *m_prev;

    dprintf_heap(stddeb,"HeapFree: free_list %08x, block %08x\n", 
	   free_list, block);
    if(debugging_heap)HEAP_CheckHeap(free_list);

    /*
     * Validate pointer.
     */
    m_free = (MDESC *) block - 1;
    if (m_free->prev != m_free || m_free->next != m_free)
    {
	fprintf(stderr,"Attempt to free bad pointer,"
	       "m_free = %08x, *free_list = %08x\n",
	       m_free, free_list);
	return -1;
    }

    if (*free_list == NULL)
    {
	*free_list = m_free;
	(*free_list)->next = NULL;
	(*free_list)->prev = NULL;
	return 0;
    }
    else if (((int) m_free & 0xffff0000) != ((int) *free_list & 0xffff0000))
    {
	fprintf(stderr,"Attempt to free bad pointer,"
	       "m_free = %08x, *free_list = %08x\n",
	       m_free, free_list);
	return -1;
    }

    /*
     * Find location in free list.
     */
    m_prev = NULL;
    for (m = *free_list; m != NULL && m < m_free; m = m->next)
	m_prev = m;
    
    if (m_prev != NULL && (int) m_prev + m_prev->length > (int) m_free)
    {
	fprintf(stderr,"Attempt to free bad pointer,"
	       "m_free = %08x, m_prev = %08x (length %x)\n",
	       m_free, m_prev, m_prev->length);
	return -1;
    }
	
    if ((m != NULL && (int) m_free + m_free->length > (int) m) ||
	(int) m_free + m_free->length > ((int) m_free | 0xffff))
    {
	fprintf(stderr,"Attempt to free bad pointer,"
	       "m_free = %08x (length %x), m = %08x\n",
	       m_free, m_free->length, m);
	return -1;
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

    if(debugging_heap)HEAP_CheckHeap(free_list);
    return 0;
}

/**********************************************************************
 *					HEAP_CheckLocalHeaps
 */
void
HEAP_CheckLocalHeaps(char *file,int line)
{
    LHEAP *lh;
    dprintf_heap(stddeb,"CheckLocalHeaps called from %s %d\n",file,line);
    for(lh=LocalHeaps; lh!=NULL; lh = lh->next)
    {	dprintf_heap(stddeb,"Checking heap %08x, free_list %08x\n",
		lh,lh->free_list);
	HEAP_CheckHeap(&lh->free_list);
    }
}


/**********************************************************************
 *					HEAP_LocalFindHeap
 */
LHEAP *
HEAP_LocalFindHeap(unsigned short owner)
{
    LHEAP *lh;
    extern struct w_files *current_exe;
    
    dprintf_heap(stddeb,"HEAP_LocalFindHeap: owner %04x\n", owner);

    for (lh = LocalHeaps; lh != NULL; lh = lh->next)
    {
	if (lh->selector == owner)
	    return lh;
    }

    dprintf_heap(stddeb,"Warning: Local heap not found\n");
    return NULL;
}

/**********************************************************************
 *					HEAP_LocalInit
 */
void
HEAP_LocalInit(unsigned short owner, void *start, int length)
{
    LHEAP *lh;

    dprintf_heap(stddeb,"HEAP_LocalInit: owner %04x, start %08x, length %04x\n"
	   ,owner, start, length);

    if (length < 2 * sizeof(MDESC))
	return;
    
    lh = (LHEAP *) malloc(sizeof(*lh));
    if (lh == NULL)
	return;
    
    lh->next        = LocalHeaps;
    lh->selector    = owner;
    lh->local_table = NULL;
    lh->delta       = 0x20;
    HEAP_Init(&lh->free_list, start, length);
    dprintf_heap(stddeb,"HEAP_LocalInit: free_list %08x, length %04x, prev %08x, next %08x\n",&lh->free_list,lh->free_list->length, lh->free_list->prev,lh->free_list->next);
    LocalHeaps = lh;
}

/**********************************************************************
 *					HEAP_LocalSize
 */
unsigned int
HEAP_LocalSize(MDESC **free_list, unsigned int handle)
{
    MDESC *m;
    
    m = (MDESC *) (((int) *free_list & 0xffff0000) | 
		   (handle & 0xffff)) - 1;
    if (m->next != m || m->prev != m)
	return 0;

    return m->length;
}

/**********************************************************************
 *					WIN16_LocalAlloc
 */
void *
WIN16_LocalAlloc(int flags, int bytes)
{
    void *m;
    
    dprintf_heap(stddeb,"WIN16_LocalAlloc: flags %x, bytes %d\n", flags,bytes);
    dprintf_heap(stddeb,"    called from segment %04x, ds=%04x\n", Stack16Frame[11],Stack16Frame[6]);

    m = HEAP_Alloc(LOCALHEAP(), flags, bytes);
	
    dprintf_heap(stddeb,"WIN16_LocalAlloc: returning %x\n", (int) m);
    return m;
}

/**********************************************************************
 *					WIN16_LocalCompact
 */
int
WIN16_LocalCompact(int min_free)
{
    MDESC *m;
    int max_block;
    
    max_block = 0;
    for (m = *LOCALHEAP(); m != NULL; m = m->next)
	if (m->length > max_block)
	    max_block = m->length;
    
    return max_block;
}

/**********************************************************************
 *					WIN16_LocalFlags
 */
unsigned int
WIN16_LocalFlags(unsigned int handle)
{
    MDESC *m;

    m = (MDESC *) (((int) *LOCALHEAP() & 0xffff0000) | 
		   (handle & 0xffff)) - 1;
    if (m->next != m || m->prev != m)
	return 0;
    
    return m->lock;
}

/**********************************************************************
 *					WIN16_LocalFree
 */
unsigned int 
WIN16_LocalFree(unsigned int handle)
{
    unsigned int addr;
    
    addr = ((int) *LOCALHEAP() & 0xffff0000) | (handle & 0xffff);
    if (HEAP_Free(LOCALHEAP(), (void *) addr) < 0)
	return handle;
    else
	return 0;
}

/**********************************************************************
 *					WIN16_LocalInit
 */
unsigned int
WIN16_LocalInit(unsigned int segment, unsigned int start, unsigned int end)
{
    unsigned short owner = HEAP_OWNER;
    LHEAP *lh = HEAP_LocalFindHeap(owner);
    
    if (segment == 0)
    {
	/* Get current DS */
	segment = Stack16Frame[6];
    }

    dprintf_heap(stddeb, "WIN16_LocalInit   segment=%04x  start=%04x  end=%04x\n", segment, start, end);

    /* start=0 doesn't mean the first byte of the segment if the segment 
       is an auto data segment. Instead it should start after the actual
       data (and the stack if there is one). As we don't know the length
       of the data and stack right now, we simply put the local heap at the
       end of the segment */
    if ((start==0)&&(Segments[segment>>3].owner==segment))
    {
        return;
        start = Segments[segment>>3].length-end-2;
        end   = Segments[segment>>3].length-1;  
        dprintf_heap(stddeb, "Changed to  start=%04x  end=%04x\n",start,end);
    }

    if (lh == NULL)
    {
	HEAP_LocalInit(owner, 
		       (void *) ((segment << 16) | start), end - start + 1);
    }
    else
    {
	HEAP_Init(&lh->free_list,
		  (void *) ((segment << 16) | start), end - start + 1);
    }
    dprintf_heap(stddeb,"WIN16_LocalInit // return segment=%04X !\n", segment);
    return segment;
}

/**********************************************************************
 *					WIN16_LocalLock
 */
void *
WIN16_LocalLock(unsigned int handle)
{
    MDESC *m;

    m = (MDESC *) (((int) *LOCALHEAP() & 0xffff0000) | 
		   (handle & 0xffff)) - 1;
    if (m->next != m || m->prev != m)
	return 0;

    m->lock++;
    return (void *) (m + 1);
}

/**********************************************************************
 *					WIN16_LocalReAlloc
 */
void *
WIN16_LocalReAlloc(unsigned int handle, int bytes, int flags)
{
    void *m;
    dprintf_heap(stddeb,"WIN16_LocalReAlloc(%04X, %d, %04X); !\n",	
		 handle, bytes, flags);
    dprintf_heap(stddeb,"WIN16_LocalReAlloc // LOCALHEAP()=%08X !\n", 
		 LOCALHEAP());
    dprintf_heap(stddeb,"WIN16_LocalReAlloc // *LOCALHEAP()=%08X !\n", 
		 *LOCALHEAP());
    m = HEAP_ReAlloc(LOCALHEAP(), (void *)
		     (((int) *LOCALHEAP() & 0xffff0000) | (handle & 0xffff)),
		     bytes, flags);
	
    return m;
}

/**********************************************************************
 *					WIN16_LocalSize
 */
unsigned int
WIN16_LocalSize(unsigned int handle)
{
    MDESC *m;

    m = (MDESC *) (((int) *LOCALHEAP() & 0xffff0000) | 
		   (handle & 0xffff)) - 1;
    if (m->next != m || m->prev != m)
	return 0;

    return m->length;
}

/**********************************************************************
 *					WIN16_LocalUnlock
 */
unsigned int
WIN16_LocalUnlock(unsigned int handle)
{
    MDESC *m;
    
    m = (MDESC *) (((int) *LOCALHEAP() & 0xffff0000) | 
		   (handle & 0xffff)) - 1;
    if (m->next != m || m->prev != m)
	return 1;

    if (m->lock > 0)
	m->lock--;

    return 0;
}

/**********************************************************************
 *					WIN16_LocalHandleDelta
 */
unsigned int
WIN16_LocalHandleDelta(unsigned int new_delta)
{
    LHEAP *lh;
    
    lh = HEAP_LocalFindHeap(HEAP_OWNER);
    if (lh == NULL)
	return 0;
    
    if (new_delta)
	lh->delta = new_delta;

    return lh->delta;
}

/**********************************************************************
 *                      GetFreeSystemResources (user.284)

 */
#define USERRESOURCES 2
#define GDIRESOURCES 1
#define SYSTEMRESOURCES 0
#include <user.h>
#include <gdi.h>

WORD GetFreeSystemResources(WORD SystemResourceType)
{
  unsigned int GdiFree=0,GdiResult=0;
  unsigned int UserFree=0,UserResult=0;
  unsigned int result=0;
  MDESC *m;
  dprintf_heap(stddeb,"GetFreeSystemResources(%u)\n",SystemResourceType);
  switch(SystemResourceType) {
    case(USERRESOURCES):
      for (m = USER_Heap; m != NULL; m = m->next) /* add up free area in heap */
         UserFree += m->length;
      result=(UserFree*100)/65516;  /* 65516 == 64K */
      break;
    case(GDIRESOURCES):
      for (m = GDI_Heap; m != NULL; m = m->next)
         GdiFree += m->length;
      result=(GdiFree*100)/65516;
      break;
    case(SYSTEMRESOURCES):
      for (m = USER_Heap; m != NULL; m = m->next)
         UserFree += m->length;
      UserResult=(UserFree*100)/65516;
      for (m = GDI_Heap; m != NULL; m = m->next)
         GdiFree += m->length;
      GdiResult=(GdiFree*100)/65516;
      result=(UserResult < GdiResult) ? UserResult:GdiResult; 
      break;
    default:
      result=0;
      break;
  }
  return(result);
}

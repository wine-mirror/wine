static char RCSId[] = "$Id: heap.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prototypes.h"
#include "segmem.h"
#include "heap.h"
#include "regfunc.h"

/* #define DEBUG_HEAP /* */

LHEAP *LocalHeaps = NULL;

/**********************************************************************
 *					HEAP_Init
 */
void
HEAP_Init(MDESC **free_list, void *start, int length)
{
    if (length < 2 * sizeof(MDESC))
	return;
    
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
	    m->lock = 0;
	    m->flags = 0;
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
	m->lock = 0;
	m->flags = 0;
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
    MDESC *m_free;
    MDESC *m;


    if (!old_block)
	return HEAP_Alloc(free_list, flags, new_size);
    
    /*
     * Check validity of block
     */
    m = (MDESC *) old_block - 1;

#ifdef DEBUG_HEAP
    printf("HEAP_ReAlloc new_size=%d !\n", new_size);
    printf("HEAP_ReAlloc old_block=%08X !\n", old_block);
    printf("HEAP_ReAlloc m=%08X free_list=%08X !\n", m, free_list);
    printf("HEAP_ReAlloc m->prev=%08X !\n", m->prev);
    printf("HEAP_ReAlloc m->next=%08X !\n", m->next);
    printf("HEAP_ReAlloc *free_list=%08X !\n", *free_list);
#endif

    if (m->prev != m || m->next != m || 
	((int) m & 0xffff0000) != ((int) *free_list & 0xffff0000))
    {
#ifdef DEBUG_HEAP
	printf("Attempt to resize bad pointer, m = %08x, *free_list = %08x\n",
	       m, free_list);
#endif
	return NULL;
    }
    
    /*
     * Check for grow block
     */
#ifdef DEBUG_HEAP
    printf("HEAP_ReAlloc Check for grow block !\n");
#endif

    if (new_size > m->length)
    {
	m_free = m + 1 + m->length / sizeof(MDESC);
	if (m_free->next == m_free || 
	    m_free->prev == m_free ||
	    m_free->length + sizeof(MDESC) < new_size)
	{
	    void *new_p = HEAP_Alloc(free_list, flags, new_size);
	    if (new_p ==NULL)
		return NULL;
	    memcpy(new_p, old_block, m->length);
	    HEAP_Free(free_list, old_block);
	    return new_p;
	}

	if (m_free->prev == NULL)
	    *free_list = m_free->next;
	else
	    m_free->prev->next = m_free->next;
	
	if (m_free->next != NULL)
	    m_free->next->prev = m_free->prev;
	
	m->length += sizeof(MDESC) + m_free->length;
#ifdef DEBUG_HEAP
	printf("HEAP_ReAlloc before GLOBAL_FLAGS_ZEROINIT !\n");
#endif
	if (flags & GLOBAL_FLAGS_ZEROINIT)
	    memset(m_free, '\0', sizeof(MDESC) + m_free->length);
    }
    
    /*
     * Check for shrink block.
     */
#ifdef DEBUG_HEAP
	printf("HEAP_ReAlloc Check for shrink block !\n");
#endif
    if (new_size < m->length - 4 * sizeof(MDESC))
    {
	m_free = m + new_size / sizeof(MDESC) + 2;
	m_free->next = m_free;
	m_free->prev = m_free;
	m_free->length = m->length - (int) m_free - (int) m;
	m->length = (int) m_free - (int) (m + 1);
	HEAP_Free(free_list, m_free + 1);
    }
    
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
#ifdef DEBUG_HEAP
	printf("Attempt to free bad pointer,"
	       "m_free = %08x, m_prev = %08x (length %x)\n",
	       m_free, m_prev, m_prev->length);
#endif
	return -1;
    }
	
    if ((m != NULL && (int) m_free + m_free->length > (int) m) ||
	(int) m_free + m_free->length > ((int) m_free | 0xffff))
    {
#ifdef DEBUG_HEAP
	printf("Attempt to free bad pointer,"
	       "m_free = %08x (length %x), m = %08x\n",
	       m_free, m_free->length, m);
#endif
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

    return 0;
}

/**********************************************************************
 *					HEAP_LocalFindHeap
 */
LHEAP *
HEAP_LocalFindHeap(unsigned short owner)
{
    LHEAP *lh;
    
#ifdef DEBUG_HEAP
    printf("HEAP_LocalFindHeap: owner %04x\n", owner);
#endif
    
    for (lh = LocalHeaps; lh != NULL; lh = lh->next)
    {
	if (lh->selector == owner)
	    return lh;
    }

    return NULL;
}

/**********************************************************************
 *					HEAP_LocalInit
 */
void
HEAP_LocalInit(unsigned short owner, void *start, int length)
{
    LHEAP *lh;

#ifdef DEBUG_HEAP
    printf("HEAP_LocalInit: owner %04x, start %08x, length %04x\n",
	   owner, start, length);
#endif

    if (length < 2 * sizeof(MDESC))
	return;
    
    lh = (LHEAP *) malloc(sizeof(*lh));
    if (lh == NULL)
	return;
    
    lh->next        = LocalHeaps;
    lh->selector    = owner;
    lh->local_table = NULL;
    HEAP_Init(&lh->free_list, start, length);
    LocalHeaps = lh;
}

/**********************************************************************
 *					WIN16_LocalAlloc
 */
void *
WIN16_LocalAlloc(int flags, int bytes)
{
    void *m;
    
#ifdef DEBUG_HEAP
    printf("WIN16_LocalAlloc: flags %x, bytes %d\n", flags, bytes);
    printf("    called from segment %04x\n", Stack16Frame[11]);
#endif

    m = HEAP_Alloc(LOCALHEAP(), flags, bytes);
	
#ifdef DEBUG_HEAP
	printf("WIN16_LocalAlloc: returning %x\n", (int) m);
#endif
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
	printf("WIN16_LocalInit // return segment=%04X !\n", segment);
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
#ifdef DEBUG_HEAP
	printf("WIN16_LocalReAlloc(%04X, %d, %04X); !\n",	handle, bytes, flags);
	printf("WIN16_LocalReAlloc // LOCALHEAP()=%08X !\n", LOCALHEAP());
	printf("WIN16_LocalReAlloc // *LOCALHEAP()=%08X !\n", *LOCALHEAP());
#endif
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

  printf("GetFreeSystemResources(%u)\n",SystemResourceType);

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

/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_mem.c
 * Purpose :  shared DDE memory functionality for DDE
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#include <assert.h>
#include "debug.h"
#include "ldt.h"
#include "shm_main_blk.h"
#include "shm_fragment.h"
#include "shm_semaph.h"
#include "dde_mem.h"
#include "bit_array.h"

#define SEGPTR2HANDLE_INFO(sptr) ( (struct handle_info*)PTR_SEG_TO_LIN(sptr) )

#define HINFO2DATAPTR(h_info_ptr) ( (void*) ( (char*)h_info_ptr +           \
					     sizeof(struct handle_info) ) )
#define DDE_MEM_IDX(handle) ((handle)& 0x7fff)
#define DDE_MEM_HANDLE(idx) ((idx) | 0x8000)
#define DDE_MEM_INFO(handle) (main_block->handles[ DDE_MEM_IDX(handle) ])
/* List of shared handles.
 * This entry resides on the shared memory, the data comes right
 * after the `handle_info'.
 * The entry is on the same block as the actual data.
 * The `next' field gives relative reference (relative to the start of
 * the blcok.
 */
struct handle_info {
	WORD lock_count;
	WORD flags;
	int size;		/* size of the data (net)*/
};

static bit_array free_handles;
int debug_last_handle_size= 0;	/* for debugging purpose only */


/* locate_handle:
 *   locate a shared memory handle.
 * Application:
 *   The handle is first searched for in attached blocks.
 *   At the beginning, only blocks owned by this process are
 *   attached.
 *   If a handle is not found, new blocks are attached.
 * Arguments:
 *   h    - the handle.
 * RETURN: pointer to handle info.
 */
static struct handle_info *locate_handle(HGLOBAL16 h, struct local_shm_map *map)
{
  struct shm_block *block;
  
  TRACE(global,"shm: (0x%04x)\n", h);


  if (SampleBit( &free_handles, DDE_MEM_IDX(h)) == 0) {
     TRACE(global, "shm: return NULL\n");
     return NULL;		   /* free!!! */
  }
  
  block= shm_locate_block(DDE_MEM_INFO(h).shmid, map);
  if (block == NULL) {
      /* nothing found */
      TRACE(global, "shm: return NULL\n");
      return NULL;
  }

  return (struct handle_info *) REL2PTR(block, DDE_MEM_INFO(h).rel);
  
}

/* dde_alloc_handle: allocate shared DDE handle */
static HGLOBAL16 dde_alloc_handle()
{
  int bit_nr;

  bit_nr= AllocateBit( &free_handles);

  if (bit_nr != -1)
     return DDE_MEM_HANDLE(bit_nr);

  TRACE(global,"dde_alloc_handle: no free DDE handle found\n");
  return 0;
}
/**********************************************************************
 *					DDE_malloc
 */
void *
DDE_malloc(unsigned int flags, unsigned long size, SHMDATA *shmdata)
{
    int shmid;
    struct shm_block *block;
    struct handle_info *h_info;
    struct local_shm_map *curr;
    HGLOBAL16 handle;
    
    TRACE(global,"DDE_malloc flags %4X, size %ld\n", flags, size);
    DDE_IPC_init();		/* make sure main shm block allocated */ 

    shm_write_wait(main_block->proc[curr_proc_idx].sem);

    /* Try to find fragment big enough for `size' */
    /* iterate through all local shm blocks, and try to allocate
       the fragment */

    h_info= NULL;
    for (curr=  shm_map ; curr != NULL ; curr= curr->next) {
       if (curr->proc_idx == curr_proc_idx) {
	  h_info= (struct handle_info *)
	     shm_FragPtrAlloc(curr->ptr, size+sizeof(struct handle_info));
	  if (h_info!=NULL) {
	     shmid= curr->shm_id;
	     break;
	  }
       }
    }

    if (h_info == NULL) {
       
       block= shm_create_block(0, size+sizeof(struct handle_info), &shmid);
       if (block==NULL) {
	  shm_write_signal(main_block->proc[curr_proc_idx].sem);
	  return 0;
       }
       /* put the new block in the linked list */
       block->next_shm_id= main_block->proc[curr_proc_idx].shmid;
       main_block->proc[curr_proc_idx].shmid= shmid;
       h_info= (struct handle_info *)
	  shm_FragPtrAlloc(block, size+sizeof(struct handle_info));
       if (h_info==NULL) {
	  ERR(global,"BUG! unallocated fragment\n");
	  shm_write_signal(main_block->proc[curr_proc_idx].sem);
	  return 0;
       }
    } else {
       block= curr->ptr;
    }

    /* Here we have an allocated fragment */
    h_info->flags= flags;
    h_info->lock_count= 0;
    h_info->size= size;
    handle= dde_alloc_handle();
    
    if (handle) {
       TRACE(global, "returning handle=0x%4x, ptr=0x%08lx\n",
		      (int)handle, (long) HINFO2DATAPTR(h_info));
       DDE_MEM_INFO(handle).rel=  PTR2REL(block, h_info);
       DDE_MEM_INFO(handle).shmid= shmid;
    }
    else
       WARN(global, "failed\n");

    shm_write_signal(main_block->proc[curr_proc_idx].sem);
    
    shmdata->handle= handle;
    return (char *)HINFO2DATAPTR(h_info);
}

HGLOBAL16 DDE_GlobalFree(HGLOBAL16 h)
{
  struct handle_info *h_info;
  int handle_index= h & 0x7fff;
  struct local_shm_map map;

  TRACE(global,"(0x%04x)\n",h);

  if (h==0)
     return 0;

  h_info= locate_handle(h, &map);
  if (h_info == NULL)
      return h;

  shm_write_wait(main_block->proc[map.proc_idx].sem);

  shm_FragPtrFree(map.ptr, (struct shm_fragment *) h_info);

  AssignBit( &free_handles, handle_index, 0);
  
  /* FIXME: must free the shm block some day. */
  shm_write_signal(main_block->proc[map.proc_idx].sem);
  return 0;
}

WORD DDE_SyncHandle(HGLOBAL16 handle, WORD sel)
    
{
    struct handle_info *h_info;
    void *local_ptr;
    ldt_entry entry;
    
    h_info= locate_handle(handle, NULL);
    local_ptr= (void *)GET_SEL_BASE(sel);

    
    if (h_info == NULL)
	return 0;
    
    if (local_ptr == (void *) HINFO2DATAPTR(h_info))
	return sel;

    /* need syncronization ! */
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    entry.base= (unsigned long) HINFO2DATAPTR(h_info);
    LDT_SetEntry( SELECTOR_TO_ENTRY(sel), &entry );

    return sel;
}

/*
 * DDE_AttachHandle:
 * Attach shm memory (The data must not be already attached).
 * Parameters:
 *   handle - the memory to attach.
 *   segptr - in not null, return SEGPTR to the same block.
 * return value:
 *   32 bit pointer to the memory.
 */

void *DDE_AttachHandle(HGLOBAL16 handle, SEGPTR *segptr)
{
  struct handle_info *h_info;
  SHMDATA shmdata;
  void *ptr;
  HGLOBAL16 hOwner = GetCurrentPDB16();

  assert(is_dde_handle(handle));
  if (segptr != NULL)
      *segptr=0;

  TRACE(global,"(%04x)\n",handle);
  h_info=locate_handle(handle, NULL);

  if (h_info == NULL) 
      return NULL;

  if ( !(h_info->flags & GMEM_DDESHARE) ) {
      ERR(global,"Corrupted memory handle info\n");
      return NULL;
  }
  
  TRACE(global,"h_info=%06lx\n",(long)h_info);

  shmdata.handle= handle;
  shmdata.shmid= DDE_MEM_INFO(handle).shmid;

  ptr= HINFO2DATAPTR(h_info);
  /* Allocate the selector(s) */
  if (! GLOBAL_CreateBlock( h_info->flags, ptr, h_info->size, hOwner,
			    FALSE, FALSE, FALSE, &shmdata)) 
      return NULL;

  if (segptr != NULL) 
      *segptr= (SEGPTR)MAKELONG( 0, shmdata.sel);

  if (TRACE_ON(dde))
      debug_last_handle_size= h_info->size;

  TRACE(global,"DDE_AttachHandle returns ptr=0x%08lx\n", (long)ptr);

  return (LPSTR)ptr;

}

void DDE_mem_init()
{
  int nr_of_bits;

  shm_init();
  
  nr_of_bits= BITS_PER_BYTE * sizeof(main_block->free_handles);
  AssembleArray( &free_handles, main_block->free_handles, nr_of_bits);
}

#endif  /* CONFIG_IPC */

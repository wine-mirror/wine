/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_main_blk.h
 * Purpose:   Main Wine's shared memory block
 ***************************************************************************
 */
#ifndef __WINE_SHM_MAIN_BLK_H
#define __WINE_SHM_MAIN_BLK_H
#include <sys/shm.h>
#include "shm_block.h"
#include "shm_semaph.h"
#include "dde_proc.h"
#include "dde_atom.h"
#include "dde_mem.h"
/*****************************************************************************
 *
 * main block object
 *
 *****************************************************************************
 */
#ifndef __inline__
#ifndef __GNUC__
#define __inline__
#endif /* __GNUC__ */
#endif /* __inline__ */

#define DDE_HANDLES_BIT_ARRAY_SIZE (DDE_HANDLES/sizeof(int)/8)

#define SHM_MAXID SHMSEG	/* maximum shm blocks (Wine's limit) */
struct shm_main_block {
	/* NOTE:  "block" declaration must be the first */
	struct shm_block block;
	char magic[64];		   /* magic string to identify the block */
        int build_lock;		   /* =1 when data structure not stable yet */
        shm_sem sem;		   /* semaphores for main_block integrity */
	struct _dde_proc proc[DDE_PROCS];  /* information about processes  */
	REL_PTR atoms[DDE_ATOMS];  /* relative reference to global atoms */
        /* Translation from global window handles to local handles */
        WND_DATA windows[DDE_WINDOWS];
        DDE_HWND handles[DDE_HANDLES];
	/* bit array stating if a handle is free (bit=0), LSB in */
	/* free_handles[0] refers handle 0x8000, the MSB refers 0x801F */
	unsigned free_handles[DDE_HANDLES_BIT_ARRAY_SIZE];
};
extern struct shm_main_block *main_block;
int shm_init(void);
void shm_delete_all(int shm_id);
void DDE_mem_init();
int DDE_no_of_attached();
#define DDE_IPC_init()  ( (main_block==NULL) ?  (DDE_mem_init()) : 0 )

#endif /* __WINE_SHM_MAIN_BLK_H */

/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_mem.h
 * Purpose :  shared DDE memory functionality for DDE
 ***************************************************************************
 */
#ifndef __WINE_DDE_MEM_H
#define __WINE_DDE_MEM_H

#ifdef CONFIG_IPC

#include "wintypes.h"
#include "global.h"
#include "shm_block.h"

#define DDE_HANDLES 0x0400
#define is_dde_handle(block) ( (block) >= (1<<15)  &&  (block) < (1<<15)+DDE_HANDLES )
typedef struct {
    int shmid;
    REL_PTR rel;
}DDE_HWND;

WORD DDE_SyncHandle(HGLOBAL16 handle, WORD sel);
void *DDE_malloc(unsigned int flags,unsigned long size, SHMDATA *shmdata);
HANDLE DDE_GlobalReAlloc(WORD,long,WORD);
HGLOBAL16 DDE_GlobalFree(HGLOBAL16 block);
void *DDE_AttachHandle(HGLOBAL16 handle, SEGPTR *segptr);
WORD DDE_GlobalHandleToSel( HGLOBAL16 handle );
int DDE_GlobalUnlock(int);
HANDLE DDE_GlobalSize(WORD);
HANDLE DDE_GlobalHandle(WORD);
HANDLE DDE_GlobalFlags(WORD);

#endif  /* CONFIG_IPC */

#endif /* __WINE_DDE_MEM_H */

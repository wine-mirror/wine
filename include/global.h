/*
 * Global heap declarations
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_GLOBAL_H
#define __WINE_GLOBAL_H

#include "wintypes.h"

typedef struct
{
    HGLOBAL handle;
    WORD sel;
    int shmid;
} SHMDATA;

extern HGLOBAL GLOBAL_CreateBlock( WORD flags, const void *ptr, DWORD size,
                                   HGLOBAL hOwner, BOOL isCode,
                                   BOOL is32Bit, BOOL isReadOnly,
				   SHMDATA *shmdata);
extern BOOL GLOBAL_FreeBlock( HGLOBAL handle );
extern HGLOBAL GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL hOwner,
                             BOOL isCode, BOOL is32Bit, BOOL isReadOnly );
extern WORD GlobalHandleToSel( HGLOBAL handle );

#endif  /* __WINE_GLOBAL_H */

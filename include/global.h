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
    HGLOBAL16 handle;
    WORD sel;
    int shmid;
} SHMDATA;

extern HGLOBAL16 GLOBAL_CreateBlock( UINT16 flags, const void *ptr, DWORD size,
                                     HGLOBAL16 hOwner, BOOL isCode,
                                     BOOL is32Bit, BOOL isReadOnly,
                                     SHMDATA *shmdata);
extern BOOL GLOBAL_FreeBlock( HGLOBAL16 handle );
extern HGLOBAL16 GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL16 hOwner,
                               BOOL isCode, BOOL is32Bit, BOOL isReadOnly );
extern WORD GlobalHandleToSel( HGLOBAL16 handle );

#endif  /* __WINE_GLOBAL_H */

/*
 * Global heap declarations
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_GLOBAL_H
#define __WINE_GLOBAL_H

#include "wintypes.h"

extern HGLOBAL GLOBAL_CreateBlock( WORD flags, void *ptr, DWORD size,
                                   HGLOBAL hOwner, BOOL isCode,
                                   BOOL is32Bit, BOOL isReadOnly );
extern BOOL GLOBAL_FreeBlock( HGLOBAL handle );
extern HGLOBAL GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL hOwner,
                             BOOL isCode, BOOL is32Bit, BOOL isReadOnly );
extern WORD GlobalHandleToSel( HGLOBAL handle );

#endif  /* __WINE_GLOBAL_H */

/*
 * Global heap declarations
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_GLOBAL_H
#define __WINE_GLOBAL_H

#include "wintypes.h"

extern HGLOBAL GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL hOwner,
                             BOOL isCode, BOOL isReadOnly );
extern WORD GlobalHandleToSel( HGLOBAL handle );

#endif  /* __WINE_GLOBAL_H */

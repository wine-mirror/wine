/*
 * Atom table definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef ATOM_H
#define ATOM_H

#include "windows.h"

extern BOOL ATOM_Init(void);

typedef struct
{
    HANDLE      next;
    WORD        refCount;
    BYTE        length;
    BYTE        str[1];
} ATOMENTRY;

typedef struct
{
    WORD        size;
    HANDLE      entries[1];
} ATOMTABLE;

#ifdef WINELIB
#define LocalAlign(flags,bytes) LocalAlloc (flags|LMEM_WINE_ALIGN,bytes)
#else
#define LocalAlign(flags,bytes) LocalAlloc((flags),(bytes))
#endif

#endif  /* ATOM_H */

/*
 * Atom table definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef ATOM_H
#define ATOM_H

#include "windows.h"

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
#define LocalAlign(flags,bytes) WIN16_LocalAlloc((flags),(bytes))
#define LocalAlloc		WIN16_LocalAlloc
#define LocalLock		WIN16_LocalLock
#define LocalFree		WIN16_LocalFree
#endif

#endif  /* ATOM_H */

/*
 * Atom table definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_ATOM_H
#define __WINE_ATOM_H

#include "windows.h"

extern BOOL32 ATOM_Init( WORD globalTableSel );

typedef struct
{
    HANDLE16    next;
    WORD        refCount;
    BYTE        length;
    BYTE        str[1];
} ATOMENTRY;

typedef struct
{
    WORD        size;
    HANDLE16    entries[1];
} ATOMTABLE;

#endif  /* __WINE_ATOM_H */

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


ATOM LocalAddAtom( SEGPTR str );
ATOM LocalDeleteAtom( ATOM atom );
ATOM LocalFindAtom( SEGPTR str );
WORD LocalGetAtomName( ATOM atom, LPSTR buffer, short count );

#endif  /* ATOM_H */

/*
 * structure definitions for aliases
 *
 * Copyright 1995 Martin von Loewis
 *
 */

#ifndef __WINE_ALIAS_H
#define __WINE_ALIAS_H

#include "wintypes.h"

typedef struct
{
    DWORD  wine;
    DWORD  win16;
    DWORD  win32;
} FUNCTIONALIAS;

extern int ALIAS_UseAliases;

typedef struct _ALIASHASH{
	DWORD used;
	int recno;
} ALIASHASH;

extern BOOL ALIAS_Init(void);
extern void ALIAS_RegisterAlias( DWORD Wine, DWORD Win16Proc, DWORD Win32Proc);
extern FUNCTIONALIAS* ALIAS_LookupAlias(DWORD);

#endif  /* __WINE_ALIAS_H */

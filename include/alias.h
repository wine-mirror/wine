/*
 * structure definitions for aliases
 *
 * Copyright 1995 Martin von Loewis
 *
 */

typedef struct _FUNCTIONALIAS{
	DWORD		wine;
	DWORD		win16;
	DWORD		win32;
} FUNCTIONALIAS;

extern int ALIAS_UseAliases;

typedef struct _ALIASHASH{
	DWORD used;
	int recno;
} ALIASHASH;

void ALIAS_RegisterAlias(DWORD,DWORD,DWORD);
FUNCTIONALIAS* ALIAS_LookupAlias(DWORD);


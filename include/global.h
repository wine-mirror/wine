/*
 * Global heap declarations
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_GLOBAL_H
#define __WINE_GLOBAL_H

#include "windef.h"
#include "wine/windef16.h"
#include "wine/library.h"

/* memory/global.c */
extern HGLOBAL16 GLOBAL_CreateBlock( UINT16 flags, const void *ptr, DWORD size,
                                     HGLOBAL16 hOwner, unsigned char selflags );
extern BOOL16 GLOBAL_FreeBlock( HGLOBAL16 handle );
extern BOOL16 GLOBAL_MoveBlock( HGLOBAL16 handle, const void *ptr, DWORD size );
extern HGLOBAL16 GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL16 hOwner, unsigned char selflags );

/* memory/virtual.c */
typedef BOOL (*HANDLERPROC)(LPVOID, LPCVOID);
extern BOOL VIRTUAL_SetFaultHandler(LPCVOID addr, HANDLERPROC proc, LPVOID arg);
extern DWORD VIRTUAL_HandleFault(LPCVOID addr);

/* memory/atom.c */
extern BOOL ATOM_Init( WORD globalTableSel );

#endif  /* __WINE_GLOBAL_H */

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

/* memory/global.c */
extern HGLOBAL16 GLOBAL_CreateBlock( UINT16 flags, const void *ptr, DWORD size,
                                     HGLOBAL16 hOwner, BOOL16 isCode,
                                     BOOL16 is32Bit, BOOL16 isReadOnly,
                                     SHMDATA *shmdata);
extern BOOL16 GLOBAL_FreeBlock( HGLOBAL16 handle );
extern BOOL16 GLOBAL_MoveBlock( HGLOBAL16 handle, const void *ptr, DWORD size );
extern HGLOBAL16 GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL16 hOwner,
                               BOOL16 isCode, BOOL16 is32Bit,
                               BOOL16 isReadOnly );

extern WORD WINAPI GlobalHandleToSel( HGLOBAL16 handle );

/* memory/virtual.c */
extern BOOL32 VIRTUAL_Init( void );
extern DWORD VIRTUAL_GetPageSize(void);
extern DWORD VIRTUAL_GetGranularity(void);
extern LPVOID VIRTUAL_MapFileW( LPCWSTR name );

typedef BOOL32 (*HANDLERPROC)(LPVOID, LPCVOID);
extern BOOL32 VIRTUAL_SetFaultHandler(LPCVOID addr, HANDLERPROC proc, LPVOID arg);
extern BOOL32 VIRTUAL_HandleFault(LPCVOID addr);

/* memory/atom.c */
extern BOOL32 ATOM_Init( WORD globalTableSel );

#endif  /* __WINE_GLOBAL_H */

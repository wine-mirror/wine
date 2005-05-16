/*
 * Kernel 16-bit private definitions
 *
 * Copyright 1995 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_KERNEL16_PRIVATE_H
#define __WINE_KERNEL16_PRIVATE_H

#include "wine/winbase16.h"
#include "winreg.h"
#include "winternl.h"
#include "module.h"

#include "pshpack1.h"

/* this structure is always located at offset 0 of the DGROUP segment */
typedef struct
{
    WORD null;        /* Always 0 */
    DWORD old_ss_sp;  /* Stack pointer; used by SwitchTaskTo() */
    WORD heap;        /* Pointer to the local heap information (if any) */
    WORD atomtable;   /* Pointer to the local atom table (if any) */
    WORD stacktop;    /* Top of the stack */
    WORD stackmin;    /* Lowest stack address used so far */
    WORD stackbottom; /* Bottom of the stack */
} INSTANCEDATA;

/* THHOOK Kernel Data Structure */
typedef struct _THHOOK
{
    HANDLE16   hGlobalHeap;         /* 00 (handle BURGERMASTER) */
    WORD       pGlobalHeap;         /* 02 (selector BURGERMASTER) */
    HMODULE16  hExeHead;            /* 04 hFirstModule */
    HMODULE16  hExeSweep;           /* 06 (unused) */
    HANDLE16   TopPDB;              /* 08 (handle of KERNEL PDB) */
    HANDLE16   HeadPDB;             /* 0A (first PDB in list) */
    HANDLE16   TopSizePDB;          /* 0C (unused) */
    HTASK16    HeadTDB;             /* 0E hFirstTask */
    HTASK16    CurTDB;              /* 10 hCurrentTask */
    HTASK16    LoadTDB;             /* 12 (unused) */
    HTASK16    LockTDB;             /* 14 hLockedTask */
} THHOOK;

extern THHOOK *pThhook;

#include "poppack.h"

#define NE_SEG_TABLE(pModule) \
    ((SEGTABLEENTRY *)((char *)(pModule) + (pModule)->seg_table))

#define NE_MODULE_NAME(pModule) \
    (((OFSTRUCT *)((char*)(pModule) + (pModule)->fileinfo))->szPathName)

#define CURRENT_STACK16 ((STACK16FRAME*)MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved))
#define CURRENT_DS      (CURRENT_STACK16->ds)

/* push bytes on the 16-bit stack of a thread; return a segptr to the first pushed byte */
static inline SEGPTR stack16_push( int size )
{
    STACK16FRAME *frame = CURRENT_STACK16;
    memmove( (char*)frame - size, frame, sizeof(*frame) );
    NtCurrentTeb()->WOW32Reserved = (char *)NtCurrentTeb()->WOW32Reserved - size;
    return (SEGPTR)((char *)NtCurrentTeb()->WOW32Reserved + sizeof(*frame));
}

/* pop bytes from the 16-bit stack of a thread */
static inline void stack16_pop( int size )
{
    STACK16FRAME *frame = CURRENT_STACK16;
    memmove( (char*)frame + size, frame, sizeof(*frame) );
    NtCurrentTeb()->WOW32Reserved = (char *)NtCurrentTeb()->WOW32Reserved + size;
}

/* ne_module.c */
extern NE_MODULE *NE_GetPtr( HMODULE16 hModule );
extern WORD NE_GetOrdinal( HMODULE16 hModule, const char *name );
extern FARPROC16 WINAPI NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal );
extern FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop );
extern BOOL16 NE_SetEntryPoint( HMODULE16 hModule, WORD ordinal, WORD offset );
extern HANDLE NE_OpenFile( NE_MODULE *pModule );
extern DWORD NE_StartTask(void);

/* ne_segment.c */
extern BOOL NE_LoadSegment( NE_MODULE *pModule, WORD segnum );
extern BOOL NE_LoadAllSegments( NE_MODULE *pModule );
extern BOOL NE_CreateSegment( NE_MODULE *pModule, int segnum );
extern BOOL NE_CreateAllSegments( NE_MODULE *pModule );
extern HINSTANCE16 NE_GetInstance( NE_MODULE *pModule );
extern void NE_InitializeDLLs( HMODULE16 hModule );
extern void NE_DllProcessAttach( HMODULE16 hModule );
extern void NE_CallUserSignalProc( HMODULE16 hModule, UINT16 code );

/* selector.c */
extern WORD SELECTOR_AllocBlock( const void *base, DWORD size, unsigned char flags );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size );
extern void SELECTOR_FreeBlock( WORD sel );
#define IS_SELECTOR_32BIT(sel) \
   (wine_ldt_is_system(sel) || (wine_ldt_copy.flags[LOWORD(sel) >> 3] & WINE_LDT_FLAGS_32BIT))

/* task.c */
extern void TASK_CreateMainTask(void);
extern HTASK16 TASK_SpawnTask( NE_MODULE *pModule, WORD cmdShow,
                               LPCSTR cmdline, BYTE len, HANDLE *hThread );
extern void TASK_ExitTask(void);
extern HTASK16 TASK_GetTaskFromThread( DWORD thread );
extern TDB *TASK_GetCurrent(void);
extern void TASK_InstallTHHook( THHOOK *pNewThook );

#endif  /* __WINE_KERNEL16_PRIVATE_H */

/*
 * Kernel32 undocumented and private functions definition
 *
 * Copyright 2003 Eric Pouech
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

#ifndef __WINE_KERNEL_PRIVATE_H
#define __WINE_KERNEL_PRIVATE_H

HANDLE  WINAPI OpenConsoleW(LPCWSTR, DWORD, LPSECURITY_ATTRIBUTES, DWORD);
BOOL    WINAPI VerifyConsoleIoHandle(HANDLE);
HANDLE  WINAPI DuplicateConsoleHandle(HANDLE, DWORD, BOOL, DWORD);
BOOL    WINAPI CloseConsoleHandle(HANDLE handle);
HANDLE  WINAPI GetConsoleInputWaitHandle(void);

static inline BOOL is_console_handle(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE && ((DWORD)h & 3) == 3;
}

/* map a real wineserver handle onto a kernel32 console handle */
static inline HANDLE console_handle_map(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE ? (HANDLE)((DWORD)h ^ 3) : INVALID_HANDLE_VALUE;
}

/* map a kernel32 console handle onto a real wineserver handle */
static inline HANDLE console_handle_unmap(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE ? (HANDLE)((DWORD)h ^ 3) : INVALID_HANDLE_VALUE;
}

/* Size of per-process table of DOS handles */
#define DOS_TABLE_SIZE 256
extern HANDLE dos_handles[DOS_TABLE_SIZE];
void FILE_ConvertOFMode( INT mode, DWORD *access, DWORD *sharing );

extern BOOL WOWTHUNK_Init(void);

extern VOID SYSLEVEL_CheckNotLevel( INT level );

extern DWORD INSTR_EmulateInstruction( EXCEPTION_RECORD *rec, CONTEXT86 *context );
extern void INSTR_CallBuiltinHandler( CONTEXT86 *context, BYTE intnum );

extern BOOL NLS_IsUnicodeOnlyLcid(LCID);

extern WORD SELECTOR_AllocBlock( const void *base, DWORD size, unsigned char flags );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size );
extern void SELECTOR_FreeBlock( WORD sel );
#define IS_SELECTOR_32BIT(sel) \
   (wine_ldt_is_system(sel) || (wine_ldt_copy.flags[LOWORD(sel) >> 3] & WINE_LDT_FLAGS_32BIT))

/* this structure is always located at offset 0 of the DGROUP segment */
#include "pshpack1.h"
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
#include "poppack.h"

#endif

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

HANDLE  WINAPI OpenConsoleW(LPCWSTR, DWORD, BOOL, DWORD);
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

extern HMODULE kernel32_handle;

/* Size of per-process table of DOS handles */
#define DOS_TABLE_SIZE 256
extern HANDLE dos_handles[DOS_TABLE_SIZE];

extern const WCHAR *DIR_Windows;
extern const WCHAR *DIR_System;

extern void PTHREAD_Init(void);
extern BOOL WOWTHUNK_Init(void);

extern VOID SYSLEVEL_CheckNotLevel( INT level );

extern void FILE_SetDosError(void);
extern WCHAR *FILE_name_AtoW( LPCSTR name, BOOL alloc );
extern DWORD FILE_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen );

extern DWORD INSTR_EmulateInstruction( EXCEPTION_RECORD *rec, CONTEXT86 *context );
extern void INSTR_CallBuiltinHandler( CONTEXT86 *context, BYTE intnum );

/* return values for MODULE_GetBinaryType */
enum binary_type
{
    BINARY_UNKNOWN,
    BINARY_PE_EXE,
    BINARY_PE_DLL,
    BINARY_WIN16,
    BINARY_OS216,
    BINARY_DOS,
    BINARY_UNIX_EXE,
    BINARY_UNIX_LIB
};

/* module.c */
extern WCHAR *MODULE_get_dll_load_path( LPCWSTR module );
extern enum binary_type MODULE_GetBinaryType( HANDLE hfile, void **res_start, void **res_end );

extern BOOL NLS_IsUnicodeOnlyLcid(LCID);

extern HANDLE VXD_Open( LPCWSTR filename, DWORD access, LPSECURITY_ATTRIBUTES sa );

extern WORD DOSMEM_0000H;
extern WORD DOSMEM_BiosDataSeg;
extern WORD DOSMEM_BiosSysSeg;

/* dosmem.c */
extern BOOL   DOSMEM_Init(void);
extern WORD   DOSMEM_AllocSelector(WORD);
extern LPVOID DOSMEM_MapRealToLinear(DWORD); /* real-mode to linear */
extern LPVOID DOSMEM_MapDosToLinear(UINT);   /* linear DOS to Wine */
extern UINT   DOSMEM_MapLinearToDos(LPVOID); /* linear Wine to DOS */
extern void   load_winedos(void);

extern struct winedos_exports
{
    /* for global16.c */
    void*    (*AllocDosBlock)(UINT size, UINT16* pseg);
    BOOL     (*FreeDosBlock)(void* ptr);
    UINT     (*ResizeDosBlock)(void *ptr, UINT size, BOOL exact);
    /* for instr.c */
    void (WINAPI *EmulateInterruptPM)( CONTEXT86 *context, BYTE intnum );
    void (WINAPI *CallBuiltinHandler)( CONTEXT86 *context, BYTE intnum );
    DWORD (WINAPI *inport)( int port, int size );
    void (WINAPI *outport)( int port, int size, DWORD val );
    void (* BiosTick)(WORD timer);
} winedos;

#endif

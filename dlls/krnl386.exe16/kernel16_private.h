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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_KERNEL16_PRIVATE_H
#define __WINE_KERNEL16_PRIVATE_H

#include "wine/winbase16.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/asm.h"

#pragma pack(push,1)

/* In-memory module structure. See 'Windows Internals' p. 219 */
typedef struct _NE_MODULE
{
    WORD      ne_magic;         /* 00 'NE' signature */
    WORD      count;            /* 02 Usage count (ne_ver/ne_rev on disk) */
    WORD      ne_enttab;        /* 04 Near ptr to entry table */
    HMODULE16 next;             /* 06 Selector to next module (ne_cbenttab on disk) */
    WORD      dgroup_entry;     /* 08 Near ptr to segment entry for DGROUP (ne_crc on disk) */
    WORD      fileinfo;         /* 0a Near ptr to file info (OFSTRUCT) (ne_crc on disk) */
    WORD      ne_flags;         /* 0c Module flags */
    WORD      ne_autodata;      /* 0e Logical segment for DGROUP */
    WORD      ne_heap;          /* 10 Initial heap size */
    WORD      ne_stack;         /* 12 Initial stack size */
    DWORD     ne_csip;          /* 14 Initial cs:ip */
    DWORD     ne_sssp;          /* 18 Initial ss:sp */
    WORD      ne_cseg;          /* 1c Number of segments in segment table */
    WORD      ne_cmod;          /* 1e Number of module references */
    WORD      ne_cbnrestab;     /* 20 Size of non-resident names table */
    WORD      ne_segtab;        /* 22 Near ptr to segment table */
    WORD      ne_rsrctab;       /* 24 Near ptr to resource table */
    WORD      ne_restab;        /* 26 Near ptr to resident names table */
    WORD      ne_modtab;        /* 28 Near ptr to module reference table */
    WORD      ne_imptab;        /* 2a Near ptr to imported names table */
    DWORD     ne_nrestab;       /* 2c File offset of non-resident names table */
    WORD      ne_cmovent;       /* 30 Number of moveable entries in entry table*/
    WORD      ne_align;         /* 32 Alignment shift count */
    WORD      ne_cres;          /* 34 # of resource segments */
    BYTE      ne_exetyp;        /* 36 Operating system flags */
    BYTE      ne_flagsothers;   /* 37 Misc. flags */
    HANDLE16  dlls_to_init;     /* 38 List of DLLs to initialize (ne_pretthunks on disk) */
    HANDLE16  nrname_handle;    /* 3a Handle to non-resident name table (ne_psegrefbytes on disk) */
    WORD      ne_swaparea;      /* 3c Min. swap area size */
    WORD      ne_expver;        /* 3e Expected Windows version */
    /* From here, these are extra fields not present in normal Windows */
    HMODULE   module32;         /* PE module handle for Win32 modules */
    HMODULE   owner32;          /* PE module containing this one for 16-bit builtins */
    HMODULE16 self;             /* Handle for this module */
    WORD      self_loading_sel; /* Selector used for self-loading apps. */
    LPVOID    rsrc32_map;       /* HRSRC 16->32 map (for 32-bit modules) */
    LPCVOID   mapping;          /* mapping of the binary file */
    SIZE_T    mapping_size;     /* size of the file mapping */
} NE_MODULE;

typedef struct
{
    BYTE type;
    BYTE flags;
    BYTE segnum;
    WORD offs;
} ET_ENTRY;

typedef struct
{
    WORD first; /* ordinal */
    WORD last;  /* ordinal */
    WORD next;  /* bundle */
} ET_BUNDLE;


  /* In-memory segment table */
typedef struct
{
    WORD      filepos;   /* Position in file, in sectors */
    WORD      size;      /* Segment size on disk */
    WORD      flags;     /* Segment flags */
    WORD      minsize;   /* Min. size of segment in memory */
    HANDLE16  hSeg;      /* Selector or handle (selector - 1) of segment in memory */
} SEGTABLEENTRY;

/* this structure is always located at offset 0 of the DGROUP segment */
typedef struct
{
    WORD null;        /* Always 0 */
    WORD old_sp;      /* Stack pointer; used by SwitchTaskTo() */
    WORD old_ss;
    WORD heap;        /* Pointer to the local heap information (if any) */
    WORD atomtable;   /* Pointer to the local atom table (if any) */
    WORD stacktop;    /* Top of the stack */
    WORD stackmin;    /* Lowest stack address used so far */
    WORD stackbottom; /* Bottom of the stack */
} INSTANCEDATA;

/* relay entry points */

typedef struct
{
    WORD   pushw_bp;               /* pushw %bp */
    BYTE   pushl;                  /* pushl $target */
    void  *target;
    WORD   call;                   /* call CALLFROM16 */
    short  callfrom16;
} ENTRYPOINT16;

typedef struct
{
    BYTE   pushl;                  /* pushl $relay */
    void  *relay;
    BYTE   lcall;                  /* lcall __FLATCS__:glue */
    void  *glue;
    WORD   flatcs;
    WORD   ret[5];                 /* return sequence */
    WORD   movl;                   /* movl arg_types[1],arg_types[0](%esi) */
    DWORD  arg_types[2];           /* type of each argument */
} CALLFROM16;

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

struct ldt_bits
{
    unsigned int limit : 24;
    unsigned int type : 5;
    unsigned int granularity : 1;
    unsigned int default_big : 1;
};

extern LONG __wine_call_from_16(void);
extern void __wine_call_from_16_regs(void);

extern THHOOK *pThhook;

#pragma pack(pop)

#define NE_SEG_TABLE(pModule) \
    ((SEGTABLEENTRY *)((char *)(pModule) + (pModule)->ne_segtab))

#define NE_MODULE_NAME(pModule) \
    (((OFSTRUCT *)((char*)(pModule) + (pModule)->fileinfo))->szPathName)

#define NE_GET_DATA(pModule,offset,size) \
    ((const void *)(((offset)+(size) <= pModule->mapping_size) ? \
                    (const char *)pModule->mapping + (offset) : NULL))

#define NE_READ_DATA(pModule,buffer,offset,size) \
    (((offset)+(size) <= pModule->mapping_size) ? \
     (memcpy( buffer, (const char *)pModule->mapping + (offset), (size) ), TRUE) : FALSE)

/* push bytes on the 16-bit stack of a thread; return a segptr to the first pushed byte */
static inline SEGPTR stack16_push( int size )
{
    STACK16FRAME *frame = CURRENT_STACK16;
    memmove( (char*)frame - size, frame, sizeof(*frame) );
    CURRENT_SP -= size;
    return MAKESEGPTR( CURRENT_SS, CURRENT_SP + sizeof(*frame) );
}

/* pop bytes from the 16-bit stack of a thread */
static inline void stack16_pop( int size )
{
    STACK16FRAME *frame = CURRENT_STACK16;
    memmove( (char*)frame + size, frame, sizeof(*frame) );
    CURRENT_SP += size;
}

/* dosmem.c */
extern BOOL   DOSMEM_Init(void);
extern BOOL   DOSMEM_InitDosMemory(void);
extern LPVOID DOSMEM_MapRealToLinear(DWORD); /* real-mode to linear */
extern LPVOID DOSMEM_MapDosToLinear(UINT);   /* linear DOS to Wine */
extern UINT   DOSMEM_MapLinearToDos(LPVOID); /* linear Wine to DOS */
extern BOOL   DOSMEM_MapDosLayout(void);
extern LPVOID DOSMEM_AllocBlock(UINT size, WORD* p);
extern BOOL   DOSMEM_FreeBlock(void* ptr);
extern UINT   DOSMEM_ResizeBlock(void* ptr, UINT size, BOOL exact);
extern UINT   DOSMEM_Available(void);

/* global16.c */
extern HGLOBAL16 GLOBAL_CreateBlock( UINT16 flags, void *ptr, DWORD size,
                                     HGLOBAL16 hOwner, struct ldt_bits bits );
extern BOOL16 GLOBAL_FreeBlock( HGLOBAL16 handle );
extern BOOL16 GLOBAL_MoveBlock( HGLOBAL16 handle, void *ptr, DWORD size );
extern HGLOBAL16 GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL16 hOwner, struct ldt_bits bits );

/* instr.c */
extern DWORD __wine_emulate_instruction( EXCEPTION_RECORD *rec, CONTEXT *context );
extern LONG CALLBACK INSTR_vectored_handler( EXCEPTION_POINTERS *ptrs );

/* ne_module.c */
extern NE_MODULE *NE_GetPtr( HMODULE16 hModule );
extern WORD NE_GetOrdinal( HMODULE16 hModule, const char *name );
extern FARPROC16 WINAPI NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal );
extern FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop );
extern BOOL16 NE_SetEntryPoint( HMODULE16 hModule, WORD ordinal, WORD offset );
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

static const struct ldt_bits data_segment   = { .type = 0x13 };
static const struct ldt_bits code16_segment = { .type = 0x1b, .default_big = 0 };
static const struct ldt_bits code32_segment = { .type = 0x1b, .default_big = 1 };

extern void init_selectors(void);
extern BOOL ldt_is_system( WORD sel );
extern BOOL ldt_is_valid( WORD sel );
extern BOOL ldt_is_32bit( WORD sel );
extern void *ldt_get_ptr( WORD sel, DWORD offset );
extern BOOL ldt_get_entry( WORD sel, LDT_ENTRY *entry );
extern void ldt_set_entry( WORD sel, LDT_ENTRY entry );
extern void *ldt_get_base( WORD sel );
extern unsigned int ldt_get_limit( WORD sel );
extern WORD SELECTOR_AllocBlock( const void *base, DWORD size, struct ldt_bits bits );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size );
extern void SELECTOR_FreeBlock( WORD sel );

static inline WORD get_cs(void) { WORD res; __asm__( "movw %%cs,%0" : "=r" (res) ); return res; }
static inline WORD get_ds(void) { WORD res; __asm__( "movw %%ds,%0" : "=r" (res) ); return res; }
static inline WORD get_fs(void) { WORD res; __asm__( "movw %%fs,%0" : "=r" (res) ); return res; }
static inline WORD get_gs(void) { WORD res; __asm__( "movw %%gs,%0" : "=r" (res) ); return res; }

/* relay16.c */
extern int relay_call_from_16( void *entry_point, unsigned char *args16, CONTEXT *context );
extern void RELAY16_InitDebugLists(void);

/* snoop16.c */
extern void SNOOP16_RegisterDLL(HMODULE16,LPCSTR);
extern FARPROC16 SNOOP16_GetProcAddress16(HMODULE16,DWORD,FARPROC16);
extern BOOL SNOOP16_ShowDebugmsgSnoop(const char *dll,int ord,const char *fname);

/* syslevel.c */
extern VOID SYSLEVEL_CheckNotLevel( INT level );

/* task.c */
extern void TASK_CreateMainTask(void);
extern HTASK16 TASK_SpawnTask( NE_MODULE *pModule, WORD cmdShow,
                               LPCSTR cmdline, BYTE len, HANDLE *hThread );
extern void TASK_ExitTask(void);
extern HTASK16 TASK_GetTaskFromThread( DWORD thread );
extern TDB *TASK_GetCurrent(void);
extern void TASK_InstallTHHook( THHOOK *pNewThook );

extern BOOL WOWTHUNK_Init(void);

extern WORD DOSMEM_0000H;
extern WORD DOSMEM_BiosDataSeg;
extern WORD DOSMEM_BiosSysSeg;
extern DWORD CallTo16_DataSelector;
extern DWORD CallTo16_TebSelector;

extern WORD cbclient_selector;
extern WORD cbclientex_selector;

struct tagSYSLEVEL;

struct kernel_thread_data
{
    SEGPTR              stack;          /* 16-bit stack pointer */
    WORD                stack_sel;      /* 16-bit stack selector */
    WORD                htask16;        /* Win16 task handle */
    DWORD               sys_count[4];   /* syslevel mutex entry counters */
    struct tagSYSLEVEL *sys_mutex[4];   /* syslevel mutex pointers */
};

C_ASSERT( sizeof(struct kernel_thread_data) <= sizeof(((TEB *)0)->SystemReserved1) );

static inline struct kernel_thread_data *kernel_get_thread_data(void)
{
    return (struct kernel_thread_data *)NtCurrentTeb()->SystemReserved1;
}

/* Push a DWORD on the 32-bit stack */
static inline void stack32_push( CONTEXT *context, DWORD val )
{
    context->Esp -= sizeof(DWORD);
    *(DWORD *)context->Esp = val;
}

/* Pop a DWORD from the 32-bit stack */
static inline DWORD stack32_pop( CONTEXT *context )
{
    DWORD ret = *(DWORD *)context->Esp;
    context->Esp += sizeof(DWORD);
    return ret;
}

#define DEFINE_REGS_ENTRYPOINT(name) \
    __ASM_STDCALL_FUNC( name, 0,                                        \
                        "pushl %ebp\n\t"                                \
                        __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")       \
                        __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")         \
                        "movl %esp,%ebp\n\t"                            \
                        __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")     \
                        "leal -0x2cc(%esp),%esp\n\t" /* sizeof(CONTEXT) */ \
                        "pushl %esp\n\t"             /* context */      \
                        "call " __ASM_STDCALL("RtlCaptureContext",4) "\n\t" \
                        "movl %esp,%esi\n\t"                            \
                        "andl $~3,%esp\n\t"                             \
                        "pushl %esi\n\t"             /* context */      \
                        "call " __ASM_STDCALL("__regs_" #name,4) "\n\t" \
                        "pushl $0\n\t"               /* alertable */    \
                        "pushl %esi\n\t"             /* context */      \
                        "call " __ASM_STDCALL("NtContinue",8) "\n\t"    \
                        "ret" ) /* fake ret to make copy protections happy */

#endif  /* __WINE_KERNEL16_PRIVATE_H */

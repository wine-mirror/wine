/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include "config.h"

#include "ntdef.h" /* UNICODE_STRING */

struct _PDB;
struct __EXCEPTION_FRAME;
struct _SECURITY_ATTRIBUTES;
struct tagSYSLEVEL;

/* Thread exception block

  flags in the comment:
  1-- win95 field
  d-- win95 debug version
  -2- nt field
  --3 wine special
  --n wine unused
  !-- or -!- likely or observed  collision
  more problems (collected from mailing list):
  psapi.dll 0x10/0x30 (expects nt fields)
  ie4       0x40
  PESHiELD  0x23/0x30 (win95)
*/
typedef struct _TEB
{
/* start of NT_TIB */
    struct __EXCEPTION_FRAME *except; /* 12- 00 Head of exception handling chain */
    void        *stack_top;      /* 12-  04 Top of thread stack */
    void        *stack_low;      /* 12-  08 Stack low-water mark */
    HTASK16      htask16;        /* 1--  0c Win16 task handle */
    WORD         stack_sel;      /* 1--  0e 16-bit stack selector */
    DWORD        selman_list;    /* 1-n  10 Selector manager list */
    DWORD        user_ptr;       /* 12n  14 User pointer */
/* end of NT_TIB */  
    struct _TEB *self;           /* 12-  18 Pointer to this structure */
    WORD         tibflags;       /* 1!n  1c Flags (NT: EnvironmentPointer) */
    WORD         mutex_count;    /* 1-n  1e Win16 mutex count */
    void        *pid;            /* !2-  20 Process id (win95: debug context) */
    void        *tid;            /* -2-  24 Thread id */
    HQUEUE16     queue;          /* 1!-  28 Message queue (NT: DWORD ActiveRpcHandle)*/
    WORD         pad1;           /* --n  2a */
    LPVOID      *tls_ptr;        /* 1--  2c Pointer to TLS array */
    struct _PDB *process;        /* 12-  30 owning process (win95: PDB; nt: NTPEB !!) */
    DWORD	 flags;	         /* 1-n  34 */
    DWORD        exit_code;      /* 1--  38 Termination status */
    WORD         teb_sel;        /* 1--  3c Selector to TEB */
    WORD         emu_sel;        /* 1-n  3e 80387 emulator selector */
    DWORD        unknown1;       /* --n  40 */
    DWORD        unknown2;       /* --n  44 */
    void       (*startup)(void); /* --3  48 Thread startup routine */
    int          thread_errno;   /* --3  4c Per-thread errno (was: ring0_thread) */
    int          thread_h_errno; /* --3  50 Per-thread h_errno (was: ptr to tdbx structure) */
    void        *stack_base;     /* 1--  54 Base of the stack */
    void        *signal_stack;   /* --3  58 Signal stack (was: exit_stack) */
    void        *emu_data;       /* --n  5c Related to 80387 emulation */
    DWORD        last_error;     /* 1--  60 Last error code */
    HANDLE       debug_cb;       /* 1-n  64 Debugger context block */
    DWORD        debug_thread;   /* 1-n  68 Thread debugging this one (?) */
    void        *pcontext;       /* 1-n  6c Thread register context */
    DWORD        cur_stack;      /* --3  70 Current stack (was: unknown) */
    DWORD        ThunkConnect;   /* 1-n  74 */
    DWORD        NegStackBase;   /* 1-n  78 */
    WORD         current_ss;     /* 1-n  7c Another 16-bit stack selector */
    WORD         pad2;           /* --n  7e */
    void        *ss_table;       /* --n  80 Pointer to info about 16-bit stack */
    WORD         thunk_ss;       /* --n  84 Yet another 16-bit stack selector */
    WORD         pad3;           /* --n  86 */
    DWORD        pad4[15];       /* --n  88 */
    ULONG        CurrentLocale;  /* -2n  C4 */
    DWORD        pad5[48];       /* --n  C8 */
    DWORD        delta_priority; /* 1-n 188 Priority delta */
    DWORD        unknown4[7];    /* d-n 18c Unknown */
    void        *create_data;    /* d-n 1a8 Pointer to creation structure */
    DWORD        suspend_count;  /* d-n 1ac SuspendThread() counter */
    void        *entry_point;    /* --3 1b0 Thread entry point (was: unknown) */
    void        *entry_arg;      /* --3 1b4 Entry point arg (was: unknown) */
    DWORD        unknown5[4];    /* --n 1b8 Unknown */
    DWORD        sys_count[4];   /* --3 1c8 Syslevel mutex entry counters */
    struct tagSYSLEVEL *sys_mutex[4];   /* --3 1d8 Syslevel mutex pointers */
    DWORD        unknown6[5];    /* --n 1e8 Unknown */

    /* The following are Wine-specific fields (NT: GDI stuff) */
    struct _TEB *next;           /* --3 1fc Global thread list */
    DWORD        cleanup;        /* --3 200 Cleanup service handle */
    int          socket;         /* --3 204 Socket for server communication */
    void        *buffer;         /* --3 208 Buffer shared with server */
    int          buffer_size;    /* --3 20c Size of server buffer */
    void        *debug_info;     /* --3 210 Info for debugstr functions */
    void        *pthread_data;   /* --3 214 Data for pthread emulation */
    /* here is plenty space for wine specific fields (don't forget to change pad6!!) */

    /* the following are nt specific fields */
    DWORD        pad6[632];                  /* --n 218 */
    UNICODE_STRING StaticUnicodeString;      /* -2- bf8 used by advapi32 */
    USHORT       StaticUnicodeBuffer[261];   /* -2- c00 used by advapi32 */
    DWORD        pad7;                       /* --n e0c */
    LPVOID       tls_array[64];              /* -2- e10 Thread local storage */
    DWORD        pad8[3];                    /* --n f10 */
    PVOID        ReservedForNtRpc;           /* -2- f1c used by rpcrt4 */
    DWORD        pad9[24];                   /* --n f20 */
    PVOID        ReservedForOle;             /* -2- f80 used by ole32 */
} TEB;

/* Thread exception flags */
#define TEBF_WIN32  0x0001
#define TEBF_TRAP   0x0002

/* The per-thread signal stack size */
#define SIGNAL_STACK_SIZE  16384


/* scheduler/thread.c */
extern TEB *THREAD_CreateInitialThread( struct _PDB *pdb, int server_fd );
extern TEB *THREAD_Create( struct _PDB *pdb, void *pid, void *tid, int fd,
                           DWORD stack_size, BOOL alloc_stack16 );
extern TEB *THREAD_InitStack( TEB *teb, struct _PDB *pdb, DWORD stack_size, BOOL alloc_stack16 );
extern BOOL THREAD_IsWin16( TEB *thdb );
extern TEB *THREAD_IdToTEB( DWORD id );

/* scheduler/sysdeps.c */
extern int SYSDEPS_SpawnThread( TEB *teb );
extern void SYSDEPS_SetCurThread( TEB *teb );
extern void SYSDEPS_ExitThread( int status ) WINE_NORETURN;

#endif  /* __WINE_THREAD_H */

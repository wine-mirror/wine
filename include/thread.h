/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include "winternl.h"
#include "wine/windef16.h"

struct _PDB;
struct __EXCEPTION_FRAME;
struct _SECURITY_ATTRIBUTES;
struct tagSYSLEVEL;
struct server_buffer_info;
struct fiber_data;

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
    struct fiber_data *fiber;    /* -2-  10 Current fiber data (Win95: selector manager list) */
    DWORD        user_ptr;       /* 12n  14 User pointer */
/* end of NT_TIB */
    struct _TEB *self;           /* 12-  18 Pointer to this structure */
    WORD         tibflags;       /* 1!n  1c Flags (NT: EnvironmentPointer) */
    WORD         mutex_count;    /* 1-n  1e Win16 mutex count */
    DWORD        pid;            /* !2-  20 Process id (win95: debug context) */
    DWORD        tid;            /* -2-  24 Thread id */
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
    void        *signal_stack;   /* --3  54 Signal stack (was: stack_base) */
    void        *exit_stack;     /* 1-n  58 Exit stack */
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
    ULONG        CurrentLocale;  /* -2-  C4 */
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
    UINT         code_page;      /* --3 1fc Thread code page */
    DWORD        unused[2];      /* --3 200 Was server buffer */
    DWORD        gs_sel;         /* --3 208 %gs selector for this thread */
    int          request_fd;     /* --3 20c fd for sending server requests */
    int          reply_fd;       /* --3 210 fd for receiving server replies */
    int          wait_fd[2];     /* --3 214 fd for sleeping server requests */
    void        *debug_info;     /* --3 21c Info for debugstr functions */
    void        *pthread_data;   /* --3 220 Data for pthread emulation */
    struct async_private *pending_list;   /* --3 224 list of pending async operations */
    void        *driver_data;    /* --3 228 Graphics driver private data */
    DWORD        alarms;         /* --3 22c Data for vm86 mode */
    DWORD        vm86_pending;   /* --3 230 Data for vm86 mode */
    void        *vm86_ptr;       /* --3 234 Data for vm86 mode */
    /* here is plenty space for wine specific fields (don't forget to change pad6!!) */

    /* the following are nt specific fields */
    DWORD        pad6[624];                  /* --n 238 */
    UNICODE_STRING StaticUnicodeString;      /* -2- bf8 used by advapi32 */
    USHORT       StaticUnicodeBuffer[261];   /* -2- c00 used by advapi32 */
    void        *stack_base;                 /* -2- e0c Base of the stack */
    LPVOID       tls_array[64];              /* -2- e10 Thread local storage */
    DWORD        pad8[3];                    /* --n f10 */
    PVOID        ReservedForNtRpc;           /* -2- f1c used by rpcrt4 */
    DWORD        pad9[24];                   /* --n f20 */
    PVOID	 ErrorInfo;                  /* -2- f80 used by ole32 (IErrorInfo*) */
} TEB;

/* Thread exception flags */
#define TEBF_WIN32  0x0001
#define TEBF_TRAP   0x0002

/* The per-thread signal stack size */
#define SIGNAL_STACK_SIZE  0x100000  /* 1Mb  FIXME: should be much smaller than that */


/* scheduler/thread.c */
extern void THREAD_Init(void);
extern TEB *THREAD_InitStack( TEB *teb, DWORD stack_size );
extern TEB *THREAD_IdToTEB( DWORD id );

/* scheduler/sysdeps.c */
extern int SYSDEPS_SpawnThread( TEB *teb );
extern void SYSDEPS_SetCurThread( TEB *teb );
extern int SYSDEPS_GetUnixTid(void);
extern void SYSDEPS_InitErrno(void);
extern void DECLSPEC_NORETURN SYSDEPS_ExitThread( int status );
extern void DECLSPEC_NORETURN SYSDEPS_AbortThread( int status );
extern void DECLSPEC_NORETURN SYSDEPS_SwitchToThreadStack( void (*func)(void) );

/* signal handling */
extern BOOL SIGNAL_Init(void);
extern void SIGNAL_Block(void);
extern void SIGNAL_Unblock(void);
extern void SIGNAL_Reset(void);

#endif  /* __WINE_THREAD_H */

/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include "config.h"
#include "winbase.h"
#include "syslevel.h"
#include "selectors.h"  /* for SET_FS */

struct _PDB;
struct __EXCEPTION_FRAME;

/* Thread exception block */
typedef struct _TEB
{
    struct __EXCEPTION_FRAME *except; /* 00 Head of exception handling chain */
    void        *stack_top;      /*  04 Top of thread stack */
    void        *stack_low;      /*  08 Stack low-water mark */
    HTASK16      htask16;        /*  0c Win16 task handle */
    WORD         stack_sel;      /*  0e 16-bit stack selector */
    DWORD        selman_list;    /*  10 Selector manager list */
    DWORD        user_ptr;       /*  14 User pointer */
    struct _TEB *self;           /*  18 Pointer to this structure */
    WORD         flags;          /*  1c Flags */
    WORD         mutex_count;    /*  1e Win16 mutex count */
    DWORD        debug_context;  /*  20 Debug context */
    void        *tid;            /*  24 Thread id */
    HQUEUE16     queue;          /*  28 Message queue */
    WORD         pad1;           /*  2a */
    LPVOID      *tls_ptr;        /*  2c Pointer to TLS array */
    struct _PDB *process;        /*  30 owning process (used by NT3.51 applets)*/
    void        *buffer;         /*  34 Buffer shared with server */
    DWORD        exit_code;      /*  38 Termination status */
    WORD         teb_sel;        /*  3c Selector to TEB */
    WORD         emu_sel;        /*  3e 80387 emulator selector */
    void        *buffer_args;    /*  40 Current position of arguments in server buffer */
    void        *buffer_res;     /*  44 Current position of result in server buffer */
    void        *buffer_end;     /*  48 End of server buffer */
    int          thread_errno;   /*  4c Per-thread errno (was: ring0_thread) */
    int          thread_h_errno; /*  50 Per-thread h_errno (was: ptr to tdbx structure) */
    void        *stack_base;     /*  54 Base of the stack */
    void        *signal_stack;   /*  58 Signal stack (was: exit_stack) */
    void        *emu_data;       /*  5c Related to 80387 emulation */
    DWORD        last_error;     /*  60 Last error code */
    HANDLE       event;          /*  64 Thread event (was: debugger context block) */
    DWORD        debug_thread;   /*  68 Thread debugging this one (?) */
    void        *pcontext;       /*  6c Thread register context */
    DWORD        cur_stack;      /*  70 Current stack (was: unknown) */
    DWORD        unknown3[2];    /*  74 Unknown */
    WORD         current_ss;     /*  7c Another 16-bit stack selector */
    WORD         pad2;           /*  7e */
    void        *ss_table;       /*  80 Pointer to info about 16-bit stack */
    WORD         thunk_ss;       /*  84 Yet another 16-bit stack selector */
    WORD         pad3;           /*  86 */
    LPVOID       tls_array[64];  /*  88 Thread local storage */
    DWORD        delta_priority; /* 188 Priority delta */
    DWORD        unknown4[7];    /* 18c Unknown */
    void        *create_data;    /* 1a8 Pointer to creation structure */
    DWORD        suspend_count;  /* 1ac SuspendThread() counter */
    void        *entry_point;    /* 1b0 Thread entry point (was: unknown) */
    void        *entry_arg;      /* 1b4 Entry point arg (was: unknown) */
    DWORD        unknown5[4];    /* 1b8 Unknown */
    DWORD        sys_count[4];   /* 1c8 Syslevel mutex entry counters */
    SYSLEVEL    *sys_mutex[4];   /* 1d8 Syslevel mutex pointers */
    DWORD        unknown6[2];    /* 1e8 Unknown */
    /* The following are Wine-specific fields */
    int          socket;         /* Socket for server communication */
    unsigned int seq;            /* Server sequence number */
    void       (*startup)(void); /* Thread startup routine */
    struct _TEB *next;           /* Global thread list */
    DWORD        cleanup;        /* Cleanup service handle */
} TEB;

/* Thread exception flags */
#define TEBF_WIN32  0x0001
#define TEBF_TRAP   0x0002

/* The pseudo handle value returned by GetCurrentThread */
#define CURRENT_THREAD_PSEUDOHANDLE 0xfffffffe

/* The per-thread signal stack size */
#define SIGNAL_STACK_SIZE  16384


/* scheduler/thread.c */
extern TEB *THREAD_CreateInitialThread( struct _PDB *pdb, int server_fd );
extern TEB *THREAD_Create( struct _PDB *pdb, DWORD flags, 
                           DWORD stack_size, BOOL alloc_stack16,
                           LPSECURITY_ATTRIBUTES sa, int *server_handle );
extern BOOL THREAD_IsWin16( TEB *thdb );
extern TEB *THREAD_IdToTEB( DWORD id );

/* scheduler/sysdeps.c */
extern int SYSDEPS_SpawnThread( TEB *teb );
extern void SYSDEPS_SetCurThread( TEB *teb );
extern void SYSDEPS_ExitThread(void);

#define SetLastError(err)    ((void)(NtCurrentTeb()->last_error = (err)))
#define GetCurrentThreadId() ((DWORD)NtCurrentTeb()->tid)

#endif  /* __WINE_THREAD_H */

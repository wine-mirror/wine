/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include "config.h"
#include "winbase.h"
#include "k32obj.h"
#include "selectors.h"  /* for SET_FS */

#ifdef linux
#define HAVE_CLONE_SYSCALL
#endif

/* This is what we will use on *BSD, once threads using rfork() get
 * implemented:
 *
 * #if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
 * #define HAVE_RFORK
 * #endif
 */

#if (defined(HAVE_CLONE_SYSCALL) || defined(HAVE_RFORK)) && !defined(NO_REENTRANT_LIBC)
#define USE_THREADS
#endif

struct _PDB;

/* Thread exception block */
typedef struct _TEB
{
    void        *except;         /* 00 Head of exception handling chain */
    void        *stack_top;      /* 04 Top of thread stack */
    void        *stack_low;      /* 08 Stack low-water mark */
    HTASK16      htask16;        /* 0c Win16 task handle */
    WORD         stack_sel;      /* 0e 16-bit stack selector */
    DWORD        selman_list;    /* 10 Selector manager list */
    DWORD        user_ptr;       /* 14 User pointer */
    struct _TEB *self;           /* 18 Pointer to this structure */
    WORD         flags;          /* 1c Flags */
    WORD         mutex_count;    /* 1e Win16 mutex count */
    DWORD        debug_context;  /* 20 Debug context */
    DWORD       *ppriority;      /* 24 Pointer to current priority */
    HQUEUE16     queue;          /* 28 Message queue */
    WORD         pad1;           /* 2a */
    LPVOID      *tls_ptr;        /* 2c Pointer to TLS array */
    struct _PDB *process;      /* 30 owning process (used by NT3.51 applets)*/
} TEB;

/* Thread exception flags */
#define TEBF_WIN32  0x0001
#define TEBF_TRAP   0x0002

/* Thread database */
typedef struct _THDB
{
    K32OBJ         header;         /*  00 Kernel object header */
    struct _PDB *process;        /*  08 Process owning this thread */
    HANDLE       event;          /*  0c Thread event */
    TEB            teb;            /*  10 Thread exception block */
    DWORD          flags;          /*  44 Flags */
    DWORD          exit_code;      /*  48 Termination status */
    WORD           teb_sel;        /*  4c Selector to TEB */
    WORD           emu_sel;        /*  4e 80387 emulator selector */
    int            thread_errno;   /*  50 Per-thread errno (was: unknown) */
    void          *wait_list;      /*  54 Event waiting list */
    int            thread_h_errno; /*  50 Per-thread h_errno (was: unknown) */
    void          *ring0_thread;   /*  5c Pointer to ring 0 thread */
    void          *ptdbx;          /*  60 Pointer to TDBX structure */
    void          *stack_base;     /*  64 Base of the stack */
    void          *exit_stack;     /*  68 Stack pointer on thread exit */
    void          *emu_data;       /*  6c Related to 80387 emulation */
    DWORD          last_error;     /*  70 Last error code */
    void          *debugger_CB;    /*  74 Debugger context block */
    DWORD          debug_thread;   /*  78 Thread debugging this one (?) */
    void          *pcontext;       /*  7c Thread register context */
    DWORD          cur_stack;      /*  80 Current stack (was: unknown) */
    DWORD          unknown3[2];    /*  84 Unknown */
    WORD           current_ss;     /*  8c Another 16-bit stack selector */
    WORD           pad2;           /*  8e */
    void          *ss_table;       /*  90 Pointer to info about 16-bit stack */
    WORD           thunk_ss;       /*  94 Yet another 16-bit stack selector */
    WORD           pad3;           /*  96 */
    LPVOID         tls_array[64];  /*  98 Thread local storage */
    DWORD          delta_priority; /* 198 Priority delta */
    DWORD          unknown4[7];    /* 19c Unknown */
    void          *create_data;    /* 1b8 Pointer to creation structure */
    DWORD          suspend_count;  /* 1bc SuspendThread() counter */
    void          *entry_point;    /* 1c0 Thread entry point (was: unknown) */
    void          *entry_arg;      /* 1c4 Entry point arg (was: unknown) */
    DWORD          unknown5[4];    /* 1c8 Unknown */
    DWORD          sys_count[4];   /* 1d8 Syslevel mutex entry counters */
    CRITICAL_SECTION *sys_mutex[4];/* 1e8 Syslevel mutex pointers */
    DWORD          unknown6[2];    /* 1f8 Unknown */
    /* The following are Wine-specific fields */
    int            socket;         /* 200 Socket for server communication */
    unsigned int   seq;            /*     Server sequence number */
    void          *server_tid;     /*     Server id for this thread */
    struct _THDB  *next;           /*     Global thread list */
} THDB;

/* The pseudo handle value returned by GetCurrentThread */
#define CURRENT_THREAD_PSEUDOHANDLE 0xfffffffe

#ifdef __i386__
/* On the i386, the current thread is in the %fs register */
# define SET_CUR_THREAD(thdb) SET_FS((thdb)->teb_sel)
#else
extern THDB *pCurrentThread;
# define SET_CUR_THREAD(thdb) (pCurrentThread = (thdb))
#endif  /* __i386__ */


/* scheduler/thread.c */
extern THDB *THREAD_CreateInitialThread( struct _PDB *pdb );
extern THDB *THREAD_Create( struct _PDB *pdb, DWORD stack_size,
                            BOOL alloc_stack16,
                            int *server_thandle, int *server_phandle,
                            LPTHREAD_START_ROUTINE start_addr, LPVOID param );
extern THDB *THREAD_Current(void);
extern BOOL THREAD_IsWin16( THDB *thdb );
extern THDB *THREAD_IdToTHDB( DWORD id );
extern void THREAD_Start( THDB *thdb );
extern DWORD THREAD_TlsAlloc( THDB *thread );

/* scheduler/sysdeps.c */
extern int SYSDEPS_SpawnThread( THDB *thread );
extern void SYSDEPS_ExitThread(void);
extern TEB * WINAPI NtCurrentTeb(void);

#endif  /* __WINE_THREAD_H */

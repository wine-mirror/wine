/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include "k32obj.h"
#include "windows.h"
#include "winbase.h"
#include "winnt.h"
#include "selectors.h"  /* for SET_FS */

struct _PDB32;

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
    struct _PDB32 *process;      /* 30 owning process (used by NT3.51 applets)*/
} TEB;

/* Event waiting structure */
typedef struct
{
    DWORD         count;     /* Count of valid objects */
    DWORD         signaled;  /* Index of signaled object (or WAIT_FAILED)*/
    BOOL32        wait_all;  /* Wait for all objects flag */
    BOOL32        wait_msg;  /* Wait for message flag */
    K32OBJ       *objs[MAXIMUM_WAIT_OBJECTS];  /* Object pointers */
    int           server[MAXIMUM_WAIT_OBJECTS];  /* Server handles */
} WAIT_STRUCT;

/* Thread database */
typedef struct _THDB
{
    K32OBJ         header;         /*  00 Kernel object header */
    struct _PDB32 *process;        /*  08 Process owning this thread */
    K32OBJ        *event;          /*  0c Thread event */
    TEB            teb;            /*  10 Thread exception block */
    DWORD          flags;          /*  44 Flags */
    DWORD          exit_code;      /*  48 Termination status */
    WORD           teb_sel;        /*  4c Selector to TEB */
    WORD           emu_sel;        /*  4e 80387 emulator selector */
    int            thread_errno;   /*  50 Per-thread errno (was: unknown) */
    WAIT_STRUCT   *wait_list;      /*  54 Event waiting list */
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
    int            unix_pid;       /* 1c8 Unix thread pid (was: unknown) */
    K32OBJ        *mutex_list;     /* 1cc List of owned mutex (was: unknown)*/
    DWORD          unknown5[2];    /* 1d0 Unknown */
    DWORD          sys_count[4];   /* 1d8 Syslevel mutex entry counters */
    CRITICAL_SECTION *sys_mutex[4];/* 1e8 Syslevel mutex pointers */
    DWORD          unknown6[2];    /* 1f8 Unknown */
    /* The following are Wine-specific fields */
    CONTEXT        context;        /* 200 Thread context */
    WAIT_STRUCT    wait_struct;    /*     Event wait structure */
    int            socket;         /*     Socket for server communication */
    unsigned int   seq;            /*     Server sequence number */
    void          *server_tid;     /*     Server id for this thread */
} THDB;

/* Thread queue entry */
typedef struct _THREAD_ENTRY
{
    THDB                 *thread;
    struct _THREAD_ENTRY *next;
} THREAD_ENTRY;

/* A thread queue is a circular list; a THREAD_QUEUE is a pointer */
/* to the end of the queue (i.e. where we add elements) */
typedef THREAD_ENTRY *THREAD_QUEUE;

/* THDB <-> Thread id conversion macros */
#define THREAD_OBFUSCATOR       ((DWORD)0xdeadbeef)
#define THREAD_ID_TO_THDB(id)   ((THDB *)((id) ^ THREAD_OBFUSCATOR))
#define THDB_TO_THREAD_ID(thdb) ((DWORD)(thdb) ^ THREAD_OBFUSCATOR)

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
extern THDB *THREAD_Create( struct _PDB32 *pdb, DWORD stack_size,
                            BOOL32 alloc_stack16,
                            int *server_thandle, int *server_phandle,
                            LPTHREAD_START_ROUTINE start_addr, LPVOID param );
extern THDB *THREAD_Current(void);
extern BOOL32 THREAD_IsWin16( THDB *thdb );
extern void THREAD_Start( THDB *thdb );
extern THDB *THREAD_GetPtr( HANDLE32 handle, DWORD access, int *server_handle );
extern void THREAD_AddQueue( THREAD_QUEUE *queue, THDB *thread );
extern void THREAD_RemoveQueue( THREAD_QUEUE *queue, THDB *thread );
extern DWORD THREAD_TlsAlloc( THDB *thread );

/* scheduler/event.c */
extern void EVENT_Set( K32OBJ *obj );
extern K32OBJ *EVENT_Create( BOOL32 manual_reset, BOOL32 initial_state );

/* scheduler/mutex.c */
extern void MUTEX_Abandon( K32OBJ *obj );

/* scheduler/synchro.c */
extern void SYNC_WaitForCondition( WAIT_STRUCT *wait, DWORD timeout );
extern void SYNC_WakeUp( THREAD_QUEUE *queue, DWORD max );
extern void SYNC_MsgWakeUp( THDB *thdb );
extern void SYNC_SetupSignals(void);
extern DWORD SYNC_DoWait( DWORD count, const HANDLE32 *handles,
                          BOOL32 wait_all, DWORD timeout,
                          BOOL32 alertable, BOOL32 wait_msg );


/* scheduler/sysdeps.c */
extern int SYSDEPS_SpawnThread( THDB *thread );
extern void SYSDEPS_ExitThread(void);
extern TEB * WINAPI NtCurrentTeb(void);

#endif  /* __WINE_THREAD_H */

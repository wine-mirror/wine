/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include "process.h"
#include "winnt.h"

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
} TEB;

/* Thread database */
typedef struct _THDB
{
    K32OBJ       header;         /*  00 Kernel object header */
    PDB32       *process;        /*  08 Process owning this thread */
    K32OBJ      *event;          /*  0c Thread event */
    TEB          teb;            /*  10 Thread exception block */
    PDB32       *process2;       /*  40 Same as offset 08 (?) */
    DWORD        flags;          /*  44 Flags */
    DWORD        exit_code;      /*  48 Termination status */
    WORD         teb_sel;        /*  4c Selector to TEB */
    WORD         emu_sel;        /*  4e 80387 emulator selector */
    DWORD        unknown1;       /*  50 Unknown */
    void        *wait_list;      /*  54 Event waiting list */
    DWORD        unknown2;       /*  58 Unknown */
    void        *ring0_thread;   /*  5c Pointer to ring 0 thread */
    void        *ptdbx;          /*  60 Pointer to TDBX structure */
    void        *stack_base;     /*  64 Base of the stack */
    void        *exit_stack;     /*  68 Stack pointer on thread exit */
    void        *emu_data;       /*  6c Related to 80387 emulation */
    DWORD        last_error;     /*  70 Last error code */
    void        *debugger_CB;    /*  74 Debugger context block */
    DWORD        debug_thread;   /*  78 Thread debugging this one (?) */
    void        *pcontext;       /*  7c Thread register context */
    DWORD        unknown3[3];    /*  80 Unknown */
    WORD         current_ss;     /*  8c Another 16-bit stack selector */
    WORD         pad2;           /*  8e */
    void        *ss_table;       /*  90 Pointer to info about 16-bit stack */
    WORD         thunk_ss;       /*  94 Yet another 16-bit stack selector */
    WORD         pad3;           /*  96 */
    LPVOID       tls_array[64];  /*  98 Thread local storage */
    DWORD        delta_priority; /* 198 Priority delta */
    DWORD        unknown4[7];    /* 19c Unknown */
    void        *create_data;    /* 1b8 Pointer to creation structure */
    DWORD        suspend_count;  /* 1bc SuspendThread() counter */
    DWORD        unknown5[9];    /* 1c0 Unknown */
    K32OBJ      *crit_section;   /* 1e4 Some critical section */
    K32OBJ      *win16_mutex;    /* 1e8 Pointer to Win16 mutex */
    K32OBJ      *win32_mutex;    /* 1ec Pointer to KERNEL32 mutex */
    K32OBJ      *crit_section2;  /* 1f0 Another critical section */
    DWORD        unknown6[3];    /* 1f4 Unknown */
    /* The following are Wine-specific fields */
    CONTEXT      context;        /* 200 Thread context */
} THDB;


extern THDB *THREAD_Create( PDB32 *pdb, DWORD stack_size,
                            LPTHREAD_START_ROUTINE start_addr );
extern void THREAD_Destroy( K32OBJ *ptr );

extern THDB *pCurrentThread;

#endif  /* __WINE_THREAD_H */

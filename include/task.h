/*
 * Task definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef _WINE_TASK_H
#define _WINE_TASK_H

#include "wintypes.h"

#ifndef WINELIB
#pragma pack(1)
#endif

  /* Process database (i.e. a normal DOS PSP) */

typedef struct
{
    WORD   int20;            /* int 20h instruction */
    WORD   nextParagraph;    /* Segment of next paragraph */
    BYTE   reserved1;
    BYTE   dispatcher[5];    /* Long call to DOS */
    SEGPTR savedint22;       /* Saved int 22h handler */
    SEGPTR savedint23;       /* Saved int 23h handler */
    SEGPTR savedint24;       /* Saved int 24h handler */
    WORD   parentPSP;        /* Selector of parent PSP */
    BYTE   fileHandles[20];  /* Open file handles */
    HANDLE environment;      /* Selector of environment */
    WORD   reserved2[23];
    BYTE   fcb1[16];         /* First FCB */
    BYTE   fcb2[20];         /* Second FCB */
    BYTE   cmdLine[128];     /* Command-line (first byte is length) */
} PDB;


  /* Segment containing MakeProcInstance() thunks */
typedef struct
{
    WORD  next;       /* Selector of next segment */
    WORD  magic;      /* Thunks signature */
    WORD  unused;
    WORD  free;       /* Head of the free list */
    WORD  thunks[4];  /* Each thunk is 4 words long */
} THUNKS;

#define THUNK_MAGIC  ('P' | ('T' << 8))


  /* Task database. See 'Windows Internals' p. 226 */
typedef struct
{
    HTASK   hNext;                      /* Selector of next TDB */
    WORD    sp;                         /* Stack pointer of task */
    WORD    ss;                         /* Stack segment of task */
    WORD    nEvents;                    /* Events for this task */
    char    priority;                   /* Task priority, between -32 and 15 */
    BYTE    unused1;
    HGLOBAL hStack32;                   /* Handle to 32-bit stack */
    WORD    hSelf;                      /* Selector of this TDB */
    HANDLE  hPrevInstance;              /* Previous instance of the module */
    DWORD   esp;                        /* 32-bit stack pointer */
    WORD    ctrlword8087;               /* 80x87 control word */
    WORD    flags;                      /* Task flags */
    WORD    error_flags;                /* Error handling flags */
    WORD    version;                    /* Expected Windows version */
    HANDLE  hInstance;                  /* Instance handle for this task */
    HMODULE hModule;                    /* Module handle */
    HANDLE  hQueue;                     /* Selector of task message queue */
    HTASK   hParent;                    /* Selector of TDB of parent task */
    WORD    signal_flags;               /* Flags related to signal handler */
    DWORD   sighandler WINE_PACKED;     /* Signal handler */
    DWORD   userhandler WINE_PACKED;    /* USER signal handler */
    DWORD   discardhandler WINE_PACKED; /* Handler for GlobalDiscard() */
    DWORD   int0 WINE_PACKED;           /* int 0 (divide by zero) handler */
    DWORD   int2 WINE_PACKED;           /* int 2 (NMI) handler */
    DWORD   int4 WINE_PACKED;           /* int 4 (INTO) handler */
    DWORD   int6 WINE_PACKED;           /* int 6 (invalid opcode) handler */
    DWORD   int7 WINE_PACKED;           /* int 7 (coprocessor) handler */
    DWORD   int3e WINE_PACKED;          /* int 3e (80x87 emulator) handler */
    DWORD   int75 WINE_PACKED;          /* int 75 (80x87 error) handler */
    DWORD   compat_flags WINE_PACKED;   /* Compatibility flags */
    BYTE    unused4[14];
    HANDLE  hPDB;                       /* Selector of PDB (i.e. PSP) */
    DWORD   dta WINE_PACKED;            /* Current DTA */
    BYTE    curdrive;                   /* Current drive */
    BYTE    curdir[65];                 /* Current directory */
    WORD    nCmdShow;                   /* cmdShow parameter to WinMain */
    HTASK   hYieldTo;                   /* Next task to schedule */
    DWORD   dlls_to_init;               /* Ptr to list of DLL to initialize */
    HANDLE  hCSAlias;                   /* Code segment alias for this TDB */
    THUNKS  thunks WINE_PACKED;         /* Make proc instance thunks */
    WORD    more_thunks[6*4];           /* Space for 6 more thunks */
    BYTE    module_name[8];             /* Module name for task */
    WORD    magic;                      /* TDB signature */
    DWORD   unused7;
    PDB     pdb;                        /* PDB for this task */
} TDB;

#define TDB_MAGIC    ('T' | ('D' << 8))

  /* TDB flags */
#define TDBF_WINOLDAP   0x0001
#define TDBF_OS2APP     0x0008
#define TDBF_WIN32S     0x0010

#ifndef WINELIB
#pragma pack(4)
#endif

extern HTASK TASK_CreateTask( HMODULE hModule, HANDLE hInstance,
                              HANDLE hPrevInstance, HANDLE hEnvironment,
                              char *cmdLine, WORD cmdShow );

  /* TASK_Reschedule() 16-bit entry point */
extern FARPROC RELAY_RescheduleProcAddr;

#endif /* _WINE_TASK_H */

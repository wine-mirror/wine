/*
 * Task definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_TASK_H
#define __WINE_TASK_H

#include "wintypes.h"

#pragma pack(1)

  /* Process database (i.e. a normal DOS PSP) */

typedef struct
{
    WORD      int20;                   /* 00 int 20h instruction */
    WORD      nextParagraph;           /* 02 Segment of next paragraph */
    BYTE      reserved1;
    BYTE      dispatcher[5];           /* 05 Long call to DOS */
    FARPROC16 savedint22 WINE_PACKED;  /* 0a Saved int 22h handler */
    FARPROC16 savedint23 WINE_PACKED;  /* 0e Saved int 23h handler */
    FARPROC16 savedint24 WINE_PACKED;  /* 12 Saved int 24h handler */
    WORD      parentPSP;               /* 16 Selector of parent PSP */
    BYTE      fileHandles[20];         /* 18 Open file handles */
    HANDLE16  environment;             /* 2c Selector of environment */
    WORD      reserved2[2];
    WORD      nbFiles;                 /* 32 Number of file handles */
    SEGPTR    fileHandlesPtr;          /* 34 Pointer to file handle table */
    HANDLE16  hFileHandles;            /* 38 Handle to fileHandlesPtr */
    WORD      reserved3[17];
    BYTE      fcb1[16];                /* 5c First FCB */
    BYTE      fcb2[20];                /* 6c Second FCB */
    BYTE      cmdLine[128];            /* 80 Command-line (first byte is len)*/
    BYTE      padding[16];             /* Some apps access beyond the end of the cmd line */
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

struct _THDB;
struct _WSINFO;
struct _NE_MODULE;

  /* signal proc typedef */
typedef void (CALLBACK *USERSIGNALPROC)(HANDLE16, UINT16, UINT16,
                                        HINSTANCE16, HQUEUE16);

  /* Task database. See 'Windows Internals' p. 226.
   * Note that 16-bit OLE 2 libs like to read it directly 
   * so we have to keep entry offsets as they are. 
   */
typedef struct _TDB
{
    HTASK16   hNext;                      /* 00 Selector of next TDB */
    DWORD     ss_sp WINE_PACKED;          /* 02 Stack pointer of task */
    WORD      nEvents;                    /* 06 Events for this task */
    INT16     priority;                   /* 08 Task priority, -32..15 */
    WORD      unused1;                    /* 0a */
    HTASK16   hSelf;                      /* 0c Selector of this TDB */
    HANDLE16  hPrevInstance;              /* 0e Previous instance of module */
    DWORD     unused2;                    /* 10 */
    WORD      ctrlword8087;               /* 14 80x87 control word */
    WORD      flags;                      /* 16 Task flags */
    UINT16    error_mode;                 /* 18 Error mode (see SetErrorMode)*/
    WORD      version;                    /* 1a Expected Windows version */
    HANDLE16  hInstance;                  /* 1c Instance handle for task */
    HMODULE16 hModule;                    /* 1e Module handle */
    HQUEUE16  hQueue;                     /* 20 Selector of task queue */
    HTASK16   hParent;                    /* 22 Selector of TDB of parent */
    WORD      signal_flags;               /* 24 Flags for signal handler */
    FARPROC16 sighandler WINE_PACKED;     /* 26 Signal handler */
    USERSIGNALPROC userhandler WINE_PACKED; /* 2a USER signal handler */
    FARPROC16 discardhandler WINE_PACKED; /* 2e Handler for GlobalNotify() */
    DWORD     int0 WINE_PACKED;           /* 32 int 0 (divide by 0) handler */
    DWORD     int2 WINE_PACKED;           /* 36 int 2 (NMI) handler */
    DWORD     int4 WINE_PACKED;           /* 3a int 4 (INTO) handler */
    DWORD     int6 WINE_PACKED;           /* 3e int 6 (invalid opc) handler */
    DWORD     int7 WINE_PACKED;           /* 42 int 7 (coprocessor) handler */
    DWORD     int3e WINE_PACKED;          /* 46 int 3e (80x87 emu) handler */
    DWORD     int75 WINE_PACKED;          /* 4a int 75 (80x87 error) handler */
    DWORD     compat_flags WINE_PACKED;   /* 4e Compatibility flags */
    BYTE      unused4[2];                 /* 52 */
    struct _THDB   *thdb;                 /* 54 Pointer to thread database */
    struct _WSINFO *pwsi;		  /* 58 Socket control struct */
    BYTE      unused5[4];                 /* 5B */
    HANDLE16  hPDB;                       /* 60 Selector of PDB (i.e. PSP) */
    SEGPTR    dta WINE_PACKED;            /* 62 Current DTA */
    BYTE      curdrive;                   /* 66 Current drive */
    BYTE      curdir[65];                 /* 67 Current directory */
    WORD      nCmdShow;                   /* a8 cmdShow parameter to WinMain */
    HTASK16   hYieldTo;                   /* aa Next task to schedule */
    DWORD     dlls_to_init;               /* ac Ptr to DLLs to initialize */
    HANDLE16  hCSAlias;                   /* b0 Code segment for this TDB */
    THUNKS    thunks WINE_PACKED;         /* b2 Make proc instance thunks */
    WORD      more_thunks[6*4];           /* c2 Space for 6 more thunks */
    BYTE      module_name[8];             /* f2 Module name for task */
    WORD      magic;                      /* fa TDB signature */
    DWORD     unused7;                    /* fc */
    PDB       pdb;                        /* 100 PDB for this task */
} TDB;

#define TDB_MAGIC    ('T' | ('D' << 8))

  /* TDB flags */
#define TDBF_WINOLDAP   0x0001
#define TDBF_OS2APP     0x0008
#define TDBF_WIN32      0x0010

  /* USER signals */
#define USIG_TERMINATION	0x0020
#define USIG_DLL_LOAD		0x0040
#define USIG_DLL_UNLOAD		0x0080
#define USIG_GPF		0x0666


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

#pragma pack(4)

extern THHOOK *pThhook;
extern void (*TASK_AddTaskEntryBreakpoint)( HTASK16 hTask );

extern HTASK16 TASK_Create( struct _THDB *thdb, struct _NE_MODULE *pModule,
                            HINSTANCE16 hInstance, HINSTANCE16 hPrevInstance,
                            UINT16 cmdShow );
extern void TASK_StartTask( HTASK16 hTask );
extern void TASK_KillCurrentTask( INT16 exitCode );
extern HTASK16 TASK_GetNextTask( HTASK16 hTask );
extern BOOL32 TASK_Reschedule(void);
extern void TASK_InstallTHHook( THHOOK *pNewThook );

extern HQUEUE16 WINAPI SetThreadQueue( DWORD thread, HQUEUE16 hQueue );
extern HQUEUE16 WINAPI GetThreadQueue( DWORD thread );
extern VOID WINAPI SetFastQueue( DWORD thread, HANDLE32 hQueue );
extern HANDLE32 WINAPI GetFastQueue( void );


#endif /* __WINE_TASK_H */

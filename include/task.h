/*
 * Task definitions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_TASK_H
#define __WINE_TASK_H

#include <windef.h>
#include <wine/windef16.h>

#include <pshpack1.h>

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
    DWORD     saveStack;               /* 2e SS:SP on last int21 call */
    WORD      nbFiles;                 /* 32 Number of file handles */
    SEGPTR    fileHandlesPtr;          /* 34 Pointer to file handle table */
    HANDLE16  hFileHandles;            /* 38 Handle to fileHandlesPtr */
    WORD      reserved3[17];
    BYTE      fcb1[16];                /* 5c First FCB */
    BYTE      fcb2[20];                /* 6c Second FCB */
    BYTE      cmdLine[128];            /* 80 Command-line (first byte is len)*/
    BYTE      padding[16];             /* Some apps access beyond the end of the cmd line */
} PDB16;


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

struct _TEB;
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
    FARPROC16 userhandler WINE_PACKED;    /* 2a USER signal handler */
    FARPROC16 discardhandler WINE_PACKED; /* 2e Handler for GlobalNotify() */
    FARPROC16 int0 WINE_PACKED;           /* 32 int 0 (divide by 0) handler */
    FARPROC16 int2 WINE_PACKED;           /* 36 int 2 (NMI) handler */
    FARPROC16 int4 WINE_PACKED;           /* 3a int 4 (INTO) handler */
    FARPROC16 int6 WINE_PACKED;           /* 3e int 6 (invalid opc) handler */
    FARPROC16 int7 WINE_PACKED;           /* 42 int 7 (coprocessor) handler */
    FARPROC16 int3e WINE_PACKED;          /* 46 int 3e (80x87 emu) handler */
    FARPROC16 int75 WINE_PACKED;          /* 4a int 75 (80x87 error) handler */
    DWORD     compat_flags WINE_PACKED;   /* 4e Compatibility flags */
    BYTE      unused4[2];                 /* 52 */
    struct _TEB *teb;                     /* 54 Pointer to thread database */
    BYTE      unused5[8];                 /* 58 */
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
    HANDLE    hEvent;                     /* fc scheduler event handle */
    PDB16     pdb;                        /* 100 PDB for this task */
} TDB;

#define TDB_MAGIC    ('T' | ('D' << 8))

  /* TDB flags */
#define TDBF_WINOLDAP   0x0001
#define TDBF_OS2APP     0x0008
#define TDBF_WIN32      0x0010

  /* Windows 3.1 USER signals */
#define USIG16_TERMINATION	0x0020
#define USIG16_DLL_LOAD		0x0040
#define USIG16_DLL_UNLOAD	0x0080
#define USIG16_GPF		0x0666


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

#include <poppack.h>

extern THHOOK *pThhook;

extern void TASK_CreateMainTask(void);
extern HTASK16 TASK_SpawnTask( struct _NE_MODULE *pModule, WORD cmdShow,
                               LPCSTR cmdline, BYTE len, HANDLE *hThread );
extern void TASK_ExitTask(void);
extern HTASK16 TASK_GetTaskFromThread( DWORD thread );
extern TDB *TASK_GetCurrent(void);
extern void TASK_InstallTHHook( THHOOK *pNewThook );

#endif /* __WINE_TASK_H */

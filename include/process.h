/*
 * Process definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_PROCESS_H
#define __WINE_PROCESS_H

#include "windows.h"
#include "winbase.h"
#include "winnt.h"
#include "module.h"
#include "k32obj.h"

struct _NE_MODULE;

/* Process handle entry */
typedef struct
{
    DWORD    access;  /* Access flags */
    K32OBJ  *ptr;     /* Object ptr */
} HANDLE_ENTRY;

/* Process handle table */
typedef struct
{
    DWORD         count;
    HANDLE_ENTRY  entries[1];
} HANDLE_TABLE;

/* Win32 process environment database */
typedef struct
{
    LPSTR            environ;          /* 00 Process environment strings */
    DWORD            unknown1;         /* 04 Unknown */
    LPSTR            cmd_line;         /* 08 Command line */
    LPSTR            cur_dir;          /* 0c Current directory */
    STARTUPINFO32A  *startup_info;     /* 10 Startup information */
    HANDLE32         hStdin;           /* 14 Handle for standard input */
    HANDLE32         hStdout;          /* 18 Handle for standard output */
    HANDLE32         hStderr;          /* 1c Handle for standard error */
    DWORD            unknown2;         /* 20 Unknown */
    DWORD            inherit_console;  /* 24 Inherit console flag */
    DWORD            break_type;       /* 28 Console events flag */
    K32OBJ          *break_sem;        /* 2c SetConsoleCtrlHandler semaphore */
    K32OBJ          *break_event;      /* 30 SetConsoleCtrlHandler event */
    K32OBJ          *break_thread;     /* 34 SetConsoleCtrlHandler thread */
    void            *break_handlers;   /* 38 List of console handlers */
    /* The following are Wine-specific fields */
    CRITICAL_SECTION section;          /* 3c Env DB critical section */
    LPWSTR           cmd_lineW;        /* 40 Unicode command line */
    WORD             env_sel;          /* 44 Environment strings selector */
} ENVDB;

/* Win32 process database */
typedef struct _PDB32
{
    K32OBJ           header;           /* 00 Kernel object header */
    DWORD            unknown1;         /* 08 Unknown */
    K32OBJ          *event;            /* 0c Pointer to an event object */
    DWORD            exit_code;        /* 10 Process exit code */
    DWORD            unknown2;         /* 14 Unknown */
    HANDLE32         heap;             /* 18 Default process heap */
    HANDLE32         mem_context;      /* 1c Process memory context */
    DWORD            flags;            /* 20 Flags */
    void            *pdb16;            /* 24 DOS PSP */
    WORD             PSP_sel;          /* 28 Selector to DOS PSP */
    WORD             module;           /* 2a IMTE for the process module */
    WORD             threads;          /* 2c Number of threads */
    WORD             running_threads;  /* 2e Number of running threads */
    WORD             unknown3;         /* 30 Unknown */
    WORD             ring0_threads;    /* 32 Number of ring 0 threads */
    HANDLE32         system_heap;      /* 34 System heap to allocate handles */
    HTASK32          task;             /* 38 Win16 task */
    void            *mem_map_files;    /* 3c Pointer to mem-mapped files */
    ENVDB           *env_db;           /* 40 Environment database */
    HANDLE_TABLE    *handle_table;     /* 44 Handle table */
    struct _PDB32   *parent;           /* 48 Parent process */
    WINE_MODREF     *modref_list;      /* 4c MODREF list */
    void            *thread_list;      /* 50 List of threads */
    void            *debuggee_CB;      /* 54 Debuggee context block */
    void            *local_heap_free;  /* 58 Head of local heap free list */
    DWORD            unknown4;         /* 5c Unknown */
    CRITICAL_SECTION crit_section;     /* 60 Critical section */
    DWORD            unknown5[3];      /* 78 Unknown */
    K32OBJ          *console;          /* 84 Console */
    DWORD            tls_bits[2];      /* 88 TLS in-use bits */
    DWORD            process_dword;    /* 90 Unknown */
    struct _PDB32   *group;            /* 94 Process group */
    WINE_MODREF     *exe_modref;       /* 98 MODREF for the process EXE */
    LPTOP_LEVEL_EXCEPTION_FILTER top_filter; /* 9c Top exception filter */
    DWORD            priority;         /* a0 Priority level */
    HANDLE32         heap_list;        /* a4 Head of process heap list */
    void            *heap_handles;     /* a8 Head of heap handles list */
    DWORD            unknown6;         /* ac Unknown */
    K32OBJ          *console_provider; /* b0 Console provider (??) */
    WORD             env_selector;     /* b4 Selector to process environment */
    WORD             error_mode;       /* b6 Error mode */
    K32OBJ          *load_done_evt;    /* b8 Event for process loading done */
    DWORD            unknown7;         /* bc Unknown */
    DWORD            unknown8;         /* c0 Unknown (NT) */
    LCID             locale;           /* c4 Locale to be queried by GetThreadLocale (NT) */
    /* The following are Wine-specific fields */
    void            *server_pid;       /*    Server id for this process */
} PDB32;

/* Process flags */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

/* PDB <-> Process id conversion macros */
#define PROCESS_OBFUSCATOR     ((DWORD)0xdeadbeef)
#define PROCESS_ID_TO_PDB(id)  ((PDB32 *)((id) ^ PROCESS_OBFUSCATOR))
#define PDB_TO_PROCESS_ID(pdb) ((DWORD)(pdb) ^ PROCESS_OBFUSCATOR)

/* scheduler/environ.c */
extern BOOL32 ENV_BuildEnvironment( PDB32 *pdb );
extern BOOL32 ENV_InheritEnvironment( PDB32 *pdb, LPCSTR env );
extern void ENV_FreeEnvironment( PDB32 *pdb );

/* scheduler/handle.c */
extern BOOL32 HANDLE_CreateTable( PDB32 *pdb, BOOL32 inherit );
extern HANDLE32 HANDLE_Alloc( PDB32 *pdb, K32OBJ *ptr, DWORD access,
                              BOOL32 inherit );
extern K32OBJ *HANDLE_GetObjPtr( PDB32 *pdb, HANDLE32 handle,
                                 K32OBJ_TYPE type, DWORD access );
extern BOOL32 HANDLE_SetObjPtr( PDB32 *pdb, HANDLE32 handle,
                                K32OBJ *ptr, DWORD access );
extern void HANDLE_CloseAll( PDB32 *pdb, K32OBJ *ptr );

/* scheduler/process.c */
extern BOOL32 PROCESS_Init( void );
extern PDB32 *PROCESS_Current(void);
extern PDB32 *PROCESS_GetPtr( HANDLE32 handle, DWORD access );
extern PDB32 *PROCESS_IdToPDB( DWORD id );
extern PDB32 *PROCESS_Create( struct _NE_MODULE *pModule, LPCSTR cmd_line,
                              LPCSTR env, HINSTANCE16 hInstance,
                              HINSTANCE16 hPrevInstance, UINT32 cmdShow );

#endif  /* __WINE_PROCESS_H */

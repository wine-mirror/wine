/*
 * Process definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_PROCESS_H
#define __WINE_PROCESS_H

#include "wintypes.h"
#include "module.h"
#include "k32obj.h"

struct _NE_MODULE;
struct _THREAD_ENTRY;

/* Process handle entry */
typedef struct
{
    DWORD    access;  /* Access flags */
    K32OBJ  *ptr;     /* Object ptr */
    int      server;  /* Server handle (FIXME: tmp hack) */
} HANDLE_ENTRY;

/* Process handle table */
typedef struct
{
    DWORD         count;
    HANDLE_ENTRY  entries[1];
} HANDLE_TABLE;

/* Current Process pseudo-handle - Returned by GetCurrentProcess*/
#define CURRENT_PROCESS_PSEUDOHANDLE ((HANDLE)0x7fffffff)

/* Win32 process environment database */
typedef struct
{
    LPSTR            environ;          /* 00 Process environment strings */
    DWORD            unknown1;         /* 04 Unknown */
    LPSTR            cmd_line;         /* 08 Command line */
    LPSTR            cur_dir;          /* 0c Current directory */
    STARTUPINFOA  *startup_info;     /* 10 Startup information */
    HANDLE         hStdin;           /* 14 Handle for standard input */
    HANDLE         hStdout;          /* 18 Handle for standard output */
    HANDLE         hStderr;          /* 1c Handle for standard error */
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
typedef struct _PDB
{
    K32OBJ           header;           /* 00 Kernel object header */
    DWORD            unknown1;         /* 08 Unknown */
    K32OBJ          *event;            /* 0c Pointer to an event object (unused) */
    DWORD            exit_code;        /* 10 Process exit code */
    DWORD            unknown2;         /* 14 Unknown */
    HANDLE         heap;             /* 18 Default process heap */
    HANDLE         mem_context;      /* 1c Process memory context */
    DWORD            flags;            /* 20 Flags */
    void            *pdb16;            /* 24 DOS PSP */
    WORD             PSP_sel;          /* 28 Selector to DOS PSP */
    WORD             module;           /* 2a IMTE for the process module */
    WORD             threads;          /* 2c Number of threads */
    WORD             running_threads;  /* 2e Number of running threads */
    WORD             unknown3;         /* 30 Unknown */
    WORD             ring0_threads;    /* 32 Number of ring 0 threads */
    HANDLE         system_heap;      /* 34 System heap to allocate handles */
    HTASK          task;             /* 38 Win16 task */
    void            *mem_map_files;    /* 3c Pointer to mem-mapped files */
    ENVDB           *env_db;           /* 40 Environment database */
    HANDLE_TABLE    *handle_table;     /* 44 Handle table */
    struct _PDB   *parent;           /* 48 Parent process */
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
    struct _PDB   *group;            /* 94 Process group */
    WINE_MODREF     *exe_modref;       /* 98 MODREF for the process EXE */
    LPTOP_LEVEL_EXCEPTION_FILTER top_filter; /* 9c Top exception filter */
    DWORD            priority;         /* a0 Priority level */
    HANDLE         heap_list;        /* a4 Head of process heap list */
    void            *heap_handles;     /* a8 Head of heap handles list */
    DWORD            unknown6;         /* ac Unknown */
    K32OBJ          *console_provider; /* b0 Console provider (??) */
    WORD             env_selector;     /* b4 Selector to process environment */
    WORD             error_mode;       /* b6 Error mode */
    HANDLE         load_done_evt;    /* b8 Event for process loading done */
    DWORD            unknown7;         /* bc Unknown */
    DWORD            unknown8;         /* c0 Unknown (NT) */
    LCID             locale;           /* c4 Locale to be queried by GetThreadLocale (NT) */
    /* The following are Wine-specific fields */
    void            *server_pid;       /*    Server id for this process */
    HANDLE        *dos_handles;      /*    Handles mapping DOS -> Win32 */
    struct _PDB   *list_next;        /*    List reference - list of PDB's */
    struct _PDB   *list_prev;        /*    List reference - list of PDB's */
} PDB;

/* Process flags */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

/* PDB <-> Process id conversion macros */
#define PROCESS_OBFUSCATOR     ((DWORD)0xdeadbeef)
#define PROCESS_ID_TO_PDB(id)  ((PDB *)((id) ^ PROCESS_OBFUSCATOR))
#define PDB_TO_PROCESS_ID(pdb) ((DWORD)(pdb) ^ PROCESS_OBFUSCATOR)

/* scheduler/environ.c */
extern BOOL ENV_BuildEnvironment( PDB *pdb );
extern BOOL ENV_InheritEnvironment( PDB *pdb, LPCSTR env );
extern void ENV_FreeEnvironment( PDB *pdb );

/* scheduler/handle.c */
extern BOOL HANDLE_CreateTable( PDB *pdb, BOOL inherit );
extern HANDLE HANDLE_Alloc( PDB *pdb, K32OBJ *ptr, DWORD access,
                              BOOL inherit, int server_handle );
extern int HANDLE_GetServerHandle( PDB *pdb, HANDLE handle,
                                   K32OBJ_TYPE type, DWORD access );
extern void HANDLE_CloseAll( PDB *pdb, K32OBJ *ptr );

/* Global handle macros */
#define HANDLE_OBFUSCATOR         ((DWORD)0x544a4def)
#define HANDLE_IS_GLOBAL(h)       (((DWORD)(h) ^ HANDLE_OBFUSCATOR) < 0x10000)
#define HANDLE_LOCAL_TO_GLOBAL(h) ((HANDLE)((DWORD)(h) ^ HANDLE_OBFUSCATOR))
#define HANDLE_GLOBAL_TO_LOCAL(h) ((HANDLE)((DWORD)(h) ^ HANDLE_OBFUSCATOR))


/* scheduler/process.c */
extern BOOL PROCESS_Init( void );
extern PDB *PROCESS_Current(void);
extern BOOL PROCESS_IsCurrent( HANDLE handle );
extern PDB *PROCESS_Initial(void);
extern PDB *PROCESS_IdToPDB( DWORD id );
extern PDB *PROCESS_Create( struct _NE_MODULE *pModule, LPCSTR cmd_line,
                              LPCSTR env, HINSTANCE16 hInstance,
                              HINSTANCE16 hPrevInstance, BOOL inherit,
                              STARTUPINFOA *startup, PROCESS_INFORMATION *info );
extern void PROCESS_SuspendOtherThreads(void);
extern void PROCESS_ResumeOtherThreads(void);
extern int  	PROCESS_PDBList_Getsize (void);
extern PDB*  	PROCESS_PDBList_Getfirst (void);
extern PDB*  	PROCESS_PDBList_Getnext (PDB*);

#endif  /* __WINE_PROCESS_H */


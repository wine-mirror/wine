/*
 * Process definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_PROCESS_H
#define __WINE_PROCESS_H

#include "windef.h"
#include "module.h"
#include "thread.h"

struct _NE_MODULE;
struct _THREAD_ENTRY;
struct _UTINFO;

/* Current Process pseudo-handle - Returned by GetCurrentProcess*/
#define CURRENT_PROCESS_PSEUDOHANDLE ((HANDLE)0x7fffffff)

/* Win32 process environment database */
typedef struct
{
    LPSTR            environ;          /* 00 Process environment strings */
    DWORD            unknown1;         /* 04 Unknown */
    LPSTR            cmd_line;         /* 08 Command line */
    LPSTR            cur_dir;          /* 0c Current directory */
    STARTUPINFOA    *startup_info;     /* 10 Startup information */
    HANDLE           hStdin;           /* 14 Handle for standard input */
    HANDLE           hStdout;          /* 18 Handle for standard output */
    HANDLE           hStderr;          /* 1c Handle for standard error */
    DWORD            unknown2;         /* 20 Unknown */
    DWORD            inherit_console;  /* 24 Inherit console flag */
    DWORD            break_type;       /* 28 Console events flag */
    void            *break_sem;        /* 2c SetConsoleCtrlHandler semaphore */
    void            *break_event;      /* 30 SetConsoleCtrlHandler event */
    void            *break_thread;     /* 34 SetConsoleCtrlHandler thread */
    void            *break_handlers;   /* 38 List of console handlers */
    /* The following are Wine-specific fields */
    CRITICAL_SECTION section;          /* 3c Env DB critical section */
    LPWSTR           cmd_lineW;        /* 40 Unicode command line */
    WORD             env_sel;          /* 44 Environment strings selector */
} ENVDB;

/* Win32 process database */
typedef struct _PDB
{
    LONG             header[2];        /* 00 Kernel object header */
    DWORD            unknown1;         /* 08 Unknown */
    void            *event;            /* 0c Pointer to an event object (unused) */
    DWORD            exit_code;        /* 10 Process exit code */
    DWORD            unknown2;         /* 14 Unknown */
    HANDLE           heap;             /* 18 Default process heap */
    HANDLE           mem_context;      /* 1c Process memory context */
    DWORD            flags;            /* 20 Flags */
    void            *pdb16;            /* 24 DOS PSP */
    WORD             PSP_sel;          /* 28 Selector to DOS PSP */
    WORD             module;           /* 2a IMTE for the process module */
    WORD             threads;          /* 2c Number of threads */
    WORD             running_threads;  /* 2e Number of running threads */
    WORD             free_lib_count;   /* 30 Recursion depth of FreeLibrary calls */
    WORD             ring0_threads;    /* 32 Number of ring 0 threads */
    HANDLE           system_heap;      /* 34 System heap to allocate handles */
    HTASK            task;             /* 38 Win16 task */
    void            *mem_map_files;    /* 3c Pointer to mem-mapped files */
    ENVDB           *env_db;           /* 40 Environment database */
    void            *handle_table;     /* 44 Handle table */
    struct _PDB     *parent;           /* 48 Parent process */
    WINE_MODREF     *modref_list;      /* 4c MODREF list */
    void            *thread_list;      /* 50 List of threads */
    void            *debuggee_CB;      /* 54 Debuggee context block */
    void            *local_heap_free;  /* 58 Head of local heap free list */
    DWORD            unknown4;         /* 5c Unknown */
    CRITICAL_SECTION crit_section;     /* 60 Critical section */
    DWORD            unknown5[3];      /* 78 Unknown */
    void            *console;          /* 84 Console */
    DWORD            tls_bits[2];      /* 88 TLS in-use bits */
    DWORD            process_dword;    /* 90 Unknown */
    struct _PDB     *group;            /* 94 Process group */
    WINE_MODREF     *exe_modref;       /* 98 MODREF for the process EXE */
    LPTOP_LEVEL_EXCEPTION_FILTER top_filter; /* 9c Top exception filter */
    DWORD            priority;         /* a0 Priority level */
    HANDLE           heap_list;        /* a4 Head of process heap list */
    void            *heap_handles;     /* a8 Head of heap handles list */
    DWORD            unknown6;         /* ac Unknown */
    void            *console_provider; /* b0 Console provider (??) */
    WORD             env_selector;     /* b4 Selector to process environment */
    WORD             error_mode;       /* b6 Error mode */
    HANDLE           load_done_evt;    /* b8 Event for process loading done */
    struct _UTINFO  *UTState;          /* bc Head of Univeral Thunk list */
    DWORD            unknown8;         /* c0 Unknown (NT) */
    LCID             locale;           /* c4 Locale to be queried by GetThreadLocale (NT) */
    /* The following are Wine-specific fields */
    void            *server_pid;       /*    Server id for this process */
    HANDLE          *dos_handles;      /*    Handles mapping DOS -> Win32 */
    struct _PDB     *next;             /*    List reference - list of PDB's */
    WORD            winver;            /*    Windows version figured out by VERSION_GetVersion */
    struct _SERVICETABLE *service_table; /*  Service table for service thread */
    HANDLE           idle_event;       /* event to signal, when the process is idle */
    HANDLE16         main_queue;       /* main message queue of the process */ 
} PDB;

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

/* USER signal proc flags and codes */
/* See PROCESS_CallUserSignalProc for comments */
#define USIG_FLAGS_WIN32          0x0001
#define USIG_FLAGS_GUI            0x0002
#define USIG_FLAGS_FEEDBACK       0x0004
#define USIG_FLAGS_FAULT          0x0008

#define USIG_DLL_UNLOAD_WIN16     0x0001
#define USIG_DLL_UNLOAD_WIN32     0x0002
#define USIG_FAULT_DIALOG_PUSH    0x0003
#define USIG_FAULT_DIALOG_POP     0x0004
#define USIG_DLL_UNLOAD_ORPHANS   0x0005
#define USIG_THREAD_INIT          0x0010
#define USIG_THREAD_EXIT          0x0020
#define USIG_PROCESS_CREATE       0x0100
#define USIG_PROCESS_INIT         0x0200
#define USIG_PROCESS_EXIT         0x0300
#define USIG_PROCESS_DESTROY      0x0400
#define USIG_PROCESS_RUNNING      0x0500
#define USIG_PROCESS_LOADED       0x0600

/* [GS]etProcessDword offsets */
#define  GPD_APP_COMPAT_FLAGS    (-56)
#define  GPD_LOAD_DONE_EVENT     (-52)
#define  GPD_HINSTANCE16         (-48)
#define  GPD_WINDOWS_VERSION     (-44)
#define  GPD_THDB                (-40)
#define  GPD_PDB                 (-36)
#define  GPD_STARTF_SHELLDATA    (-32)
#define  GPD_STARTF_HOTKEY       (-28)
#define  GPD_STARTF_SHOWWINDOW   (-24)
#define  GPD_STARTF_SIZE         (-20)
#define  GPD_STARTF_POSITION     (-16)
#define  GPD_STARTF_FLAGS        (-12)
#define  GPD_PARENT              (- 8)
#define  GPD_FLAGS               (- 4)
#define  GPD_USERDATA            (  0)

extern DWORD WINAPI GetProcessDword( DWORD dwProcessID, INT offset );
void WINAPI SetProcessDword( DWORD dwProcessID, INT offset, DWORD value );
extern DWORD WINAPI MapProcessHandle( HANDLE handle );

/* scheduler/environ.c */
extern BOOL ENV_InheritEnvironment( LPCSTR env );
extern void ENV_FreeEnvironment( PDB *pdb );

/* scheduler/process.c */
extern BOOL PROCESS_Init( BOOL win32 );
extern BOOL PROCESS_IsCurrent( HANDLE handle );
extern PDB *PROCESS_Initial(void);
extern PDB *PROCESS_IdToPDB( DWORD id );
extern void PROCESS_CallUserSignalProc( UINT uCode, DWORD dwThreadId, HMODULE hModule );
extern PDB *PROCESS_Create( struct _NE_MODULE *pModule, 
                            LPCSTR cmd_line, LPCSTR env, 
                            LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                            BOOL inherit, DWORD flags,
                            STARTUPINFOA *startup, PROCESS_INFORMATION *info );
extern void PROCESS_FreePDB( PDB *pdb );
extern void PROCESS_WalkProcess( void );

/* scheduler/debugger.c */
extern DWORD DEBUG_SendExceptionEvent( EXCEPTION_RECORD *rec, BOOL first_chance, CONTEXT *ctx );
extern DWORD DEBUG_SendCreateProcessEvent( HFILE file, HMODULE module, void *entry );
extern DWORD DEBUG_SendCreateThreadEvent( void *entry );
extern DWORD DEBUG_SendLoadDLLEvent( HFILE file, HMODULE module, LPSTR *name );
extern DWORD DEBUG_SendUnloadDLLEvent( HMODULE module );

static inline PDB * WINE_UNUSED PROCESS_Current(void)
{
    return NtCurrentTeb()->process;
}

#endif  /* __WINE_PROCESS_H */

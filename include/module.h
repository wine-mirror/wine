/*
 * Module definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MODULE_H
#define __WINE_MODULE_H

#include "wintypes.h"
#include "pe_image.h"

  /* In-memory module structure. See 'Windows Internals' p. 219 */
typedef struct
{
    WORD    magic;            /* 00 'NE' signature */
    WORD    count;            /* 02 Usage count */
    WORD    entry_table;      /* 04 Near ptr to entry table */
    HMODULE16  next;          /* 06 Selector to next module */
    WORD    dgroup_entry;     /* 08 Near ptr to segment entry for DGROUP */
    WORD    fileinfo;         /* 0a Near ptr to file info (OFSTRUCT) */
    WORD    flags;            /* 0c Module flags */
    WORD    dgroup;           /* 0e Logical segment for DGROUP */
    WORD    heap_size;        /* 10 Initial heap size */
    WORD    stack_size;       /* 12 Initial stack size */
    WORD    ip;               /* 14 Initial ip */
    WORD    cs;               /* 16 Initial cs (logical segment) */
    WORD    sp;               /* 18 Initial stack pointer */
    WORD    ss;               /* 1a Initial ss (logical segment) */
    WORD    seg_count;        /* 1c Number of segments in segment table */
    WORD    modref_count;     /* 1e Number of module references */
    WORD    nrname_size;      /* 20 Size of non-resident names table */
    WORD    seg_table;        /* 22 Near ptr to segment table */
    WORD    res_table;        /* 24 Near ptr to resource table */
    WORD    name_table;       /* 26 Near ptr to resident names table */
    WORD    modref_table;     /* 28 Near ptr to module reference table */
    WORD    import_table;     /* 2a Near ptr to imported names table */
    DWORD   nrname_fpos;      /* 2c File offset of non-resident names table */
    WORD    moveable_entries; /* 30 Number of moveable entries in entry table*/
    WORD    alignment;        /* 32 Alignment shift count */
    WORD    truetype;         /* 34 Set to 2 if TrueType font */
    BYTE    os_flags;         /* 36 Operating system flags */
    BYTE    misc_flags;       /* 37 Misc. flags */
    HANDLE16   dlls_to_init;  /* 38 List of DLLs to initialize */
    HANDLE16   nrname_handle; /* 3a Handle to non-resident name table */
    WORD    min_swap_area;    /* 3c Min. swap area size */
    WORD    expected_version; /* 3e Expected Windows version */
    /* From here, these are extra fields not present in normal Windows */
    HMODULE32  module32;      /* 40 PE module handle for Win32 modules */
    HMODULE16  self;          /* 44 Handle for this module */
    WORD    self_loading_sel; /* 46 Selector used for self-loading apps. */
} NE_MODULE;


  /* In-memory segment table */
typedef struct
{
    WORD      filepos;   /* Position in file, in sectors */
    WORD      size;      /* Segment size on disk */
    WORD      flags;     /* Segment flags */
    WORD      minsize;   /* Min. size of segment in memory */
    HANDLE16  selector;  /* Selector of segment in memory */
} SEGTABLEENTRY;


  /* Self-loading modules contain this structure in their first segment */

typedef struct
{
    WORD      version;       /* Must be 0xA0 */
    WORD      reserved;
    FARPROC16 BootApp;       /* startup procedure */
    FARPROC16 LoadAppSeg;    /* procedure to load a segment */
    FARPROC16 reserved2;
    FARPROC16 MyAlloc;       /* memory allocation procedure, 
                              * wine must write this field */
    FARPROC16 EntryAddrProc;
    FARPROC16 ExitProc;      /* exit procedure */
    WORD      reserved3[4];
    FARPROC16 SetOwner;      /* Set Owner procedure, exported by wine */
} SELFLOADHEADER;

#pragma pack(1)

  /* Parameters for LoadModule() */
typedef struct
{
    HGLOBAL16 hEnvironment;         /* Environment segment */
    SEGPTR    cmdLine WINE_PACKED;  /* Command-line */
    SEGPTR    showCmd WINE_PACKED;  /* Code for ShowWindow() */
    SEGPTR    reserved WINE_PACKED;
} LOADPARAMS;

typedef struct 
{
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    LPSTR lpCmdShow;
    DWORD dwReserved;
} LOADPARAMS32;

#pragma pack(4)

/* Resource types */
typedef struct resource_typeinfo_s NE_TYPEINFO;
typedef struct resource_nameinfo_s NE_NAMEINFO;

#define NE_SEG_TABLE(pModule) \
    ((SEGTABLEENTRY *)((char *)(pModule) + (pModule)->seg_table))

#define NE_MODULE_TABLE(pModule) \
    ((WORD *)((char *)(pModule) + (pModule)->modref_table))

#define NE_MODULE_NAME(pModule) \
    (((OFSTRUCT *)((char*)(pModule) + (pModule)->fileinfo))->szPathName)

#define NE_WIN32_MODULE(pModule) \
    ((((pModule)->flags & NE_FFLAGS_WIN32) ? \
                    ((NE_WIN32_EXTRAINFO *)((pModule) + 1))->module32 : 0))

/* module.c */
extern NE_MODULE *MODULE_GetPtr( HMODULE32 hModule );
extern HMODULE16 MODULE_HANDLEtoHMODULE16( HANDLE32 handle );
extern HMODULE32 MODULE_HANDLEtoHMODULE32( HANDLE32 handle );
extern void MODULE_DumpModule( HMODULE32 hmodule );
extern void MODULE_WalkModules(void);
extern int MODULE_OpenFile( HMODULE32 hModule );
extern LPSTR MODULE_GetModuleName( HMODULE32 hModule );
extern void MODULE_RegisterModule( NE_MODULE *pModule );
extern HMODULE32 MODULE_FindModule( LPCSTR path );
extern HINSTANCE16 MODULE_CreateInstance( HMODULE16 hModule, LOADPARAMS* paramBlock );
extern HINSTANCE16 MODULE_GetInstance( HMODULE32 hModule );
extern HMODULE32 MODULE_CreateDummyModule( const OFSTRUCT *ofs );
extern HINSTANCE16 MODULE_Load( LPCSTR name, LPVOID paramBlock, UINT16 flags );
extern WORD MODULE_GetOrdinal( HMODULE32 hModule, const char *name );
extern FARPROC16 MODULE_GetEntryPoint( HMODULE32 hModule, WORD ordinal );
extern BOOL16 MODULE_SetEntryPoint( HMODULE32 hModule, WORD ordinal,
                                    WORD offset );
extern FARPROC16 MODULE_GetWndProcEntry16( const char *name );
extern FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE32 hmodule, LPSTR name );

/* builtin.c */
extern BOOL32 BUILTIN_Init(void);
extern HMODULE32 BUILTIN_LoadModule( LPCSTR name, BOOL32 force );
extern LPCSTR BUILTIN_GetEntryPoint16( WORD cs, WORD ip, WORD *pOrd );
extern FARPROC32 BUILTIN_GetEntryPoint32( char *buffer, void *relay,
                                          DWORD *typemask );
extern BOOL32 BUILTIN_ParseDLLOptions( const char *str );
extern void BUILTIN_PrintDLLs(void);

/* ne_image.c */
extern BOOL32 NE_LoadSegment( NE_MODULE *pModule, WORD segnum );
extern BOOL32 NE_LoadAllSegments( NE_MODULE *pModule );
extern BOOL32 NE_LoadDLLs( NE_MODULE *pModule );
extern void NE_FixupPrologs( NE_MODULE *pModule );
extern void NE_InitializeDLLs( HMODULE16 hModule );

/* relay32/builtin.c */
extern HMODULE32 BUILTIN32_LoadModule( LPCSTR name, BOOL32 force,
                                       struct _PDB32 *process );

/* if1632/builtin.c */
extern HMODULE32 (*fnBUILTIN_LoadModule)(LPCSTR name, BOOL32 force);

#endif  /* __WINE_MODULE_H */

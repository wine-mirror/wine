/*
 * Module definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MODULE_H
#define __WINE_MODULE_H

#include "dosexe.h"
#include "pe_image.h"
#include "wintypes.h"

  /* In-memory module structure. See 'Windows Internals' p. 219 */
typedef struct _NE_MODULE
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
    LPDOSTASK lpDosTask;
    LPVOID  dos_image;        /* pointer to DOS memory (for DOS apps) */
    LPVOID  hRsrcMap;         /* HRSRC 16->32 map (for 32-bit modules) */
} NE_MODULE;


  /* In-memory segment table */
typedef struct
{
    WORD      filepos;   /* Position in file, in sectors */
    WORD      size;      /* Segment size on disk */
    WORD      flags;     /* Segment flags */
    WORD      minsize;   /* Min. size of segment in memory */
    HANDLE16  hSeg;      /* Selector or handle (selector - 1) */
                         /* of segment in memory */
} SEGTABLEENTRY;


  /* Self-loading modules contain this structure in their first segment */

#pragma pack(1)

typedef struct
{
    WORD      version;       /* Must be "A0" (0x3041) */
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
    UINT16 *lpCmdShow;
    DWORD dwReserved;
} LOADPARAMS32;

#pragma pack(4)

/* internal representation of 32bit modules. per process. */
typedef enum { MODULE32_PE=1, MODULE32_ELF /* ,... */ } MODULE32_TYPE;
typedef struct _wine_modref
{
	struct _wine_modref	*next;
	MODULE32_TYPE		type;
	union {
		PE_MODREF	pe;
		ELF_MODREF	elf;
	} binfmt;

	HMODULE32		module;

	int			nDeps;
	struct _wine_modref	**deps;
	int			initDone;

	char			*modname;
	char			*shortname;
	char 			*longname;
} WINE_MODREF;


/* Resource types */
typedef struct resource_typeinfo_s NE_TYPEINFO;
typedef struct resource_nameinfo_s NE_NAMEINFO;

#define NE_SEG_TABLE(pModule) \
    ((SEGTABLEENTRY *)((char *)(pModule) + (pModule)->seg_table))

#define NE_MODULE_TABLE(pModule) \
    ((WORD *)((char *)(pModule) + (pModule)->modref_table))

#define NE_MODULE_NAME(pModule) \
    (((OFSTRUCT *)((char*)(pModule) + (pModule)->fileinfo))->szPathName)

/* module.c */
extern FARPROC32 MODULE_GetProcAddress32( struct _PDB32*pdb,HMODULE32 hModule,LPCSTR function,BOOL32 snoop );
extern WINE_MODREF *MODULE32_LookupHMODULE( struct _PDB32 *process, HMODULE32 hModule );
extern void MODULE_InitializeDLLs( struct _PDB32 *process, HMODULE32 root, DWORD type, LPVOID lpReserved );
extern HMODULE32 MODULE_FindModule32( struct _PDB32 *process, LPCSTR path );
extern HMODULE32 MODULE_CreateDummyModule( const OFSTRUCT *ofs, LPCSTR modName );
extern FARPROC16 MODULE_GetWndProcEntry16( const char *name );
extern FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE32 hmodule, LPCSTR name );
extern SEGPTR WINAPI HasGPHandler( SEGPTR address );
HMODULE32 MODULE_LoadLibraryEx32A(LPCSTR libname,struct _PDB32*process,HFILE32 hfile,DWORD flags);

/* loader/ne/module.c */
extern NE_MODULE *NE_GetPtr( HMODULE16 hModule );
extern void NE_DumpModule( HMODULE16 hModule );
extern void NE_WalkModules(void);
extern void NE_RegisterModule( NE_MODULE *pModule );
extern WORD NE_GetOrdinal( HMODULE16 hModule, const char *name );
extern FARPROC16 NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal );
extern FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop );
extern BOOL16 NE_SetEntryPoint( HMODULE16 hModule, WORD ordinal, WORD offset );
extern HANDLE32 NE_OpenFile( NE_MODULE *pModule );
extern HINSTANCE16 NE_LoadModule( LPCSTR name, HINSTANCE16 *hPrevInstance,
                                  BOOL32 implicit, BOOL32 lib_only );

/* loader/ne/resource.c */
extern HGLOBAL16 WINAPI NE_DefResourceHandler(HGLOBAL16,HMODULE16,HRSRC16);
extern BOOL32 NE_InitResourceHandler( HMODULE16 hModule );
extern HRSRC16 NE_FindResource( NE_MODULE *pModule, LPCSTR name, LPCSTR type );
extern INT16 NE_AccessResource( NE_MODULE *pModule, HRSRC16 hRsrc );
extern DWORD NE_SizeofResource( NE_MODULE *pModule, HRSRC16 hRsrc );
extern HGLOBAL16 NE_LoadResource( NE_MODULE *pModule, HRSRC16 hRsrc );
extern BOOL16 NE_FreeResource( NE_MODULE *pModule, HGLOBAL16 handle );

/* loader/ne/segment.c */
extern BOOL32 NE_LoadSegment( NE_MODULE *pModule, WORD segnum );
extern BOOL32 NE_LoadAllSegments( NE_MODULE *pModule );
extern void NE_FixupPrologs( NE_MODULE *pModule );
extern void NE_InitializeDLLs( HMODULE16 hModule );
extern BOOL32 NE_CreateSegments( NE_MODULE *pModule );
extern HINSTANCE16 NE_CreateInstance( NE_MODULE *pModule, HINSTANCE16 *prev,
                                      BOOL32 lib_only );

/* loader/ne/convert.c */
HGLOBAL16 NE_LoadPEResource( NE_MODULE *pModule, WORD type, LPVOID bits, DWORD size );
BOOL16 NE_FreePEResource( NE_MODULE *pModule, HGLOBAL16 handle );

/* if1632/builtin.c */
extern BOOL32 BUILTIN_Init(void);
extern HMODULE16 BUILTIN_LoadModule( LPCSTR name, BOOL32 force );
extern LPCSTR BUILTIN_GetEntryPoint16( WORD cs, WORD ip, WORD *pOrd );
extern BOOL32 BUILTIN_ParseDLLOptions( const char *str );
extern void BUILTIN_PrintDLLs(void);

/* relay32/builtin.c */
extern HMODULE32 BUILTIN32_LoadImage( LPCSTR name, OFSTRUCT *ofs, BOOL32 force );

/* if1632/builtin.c */
extern HMODULE16 (*fnBUILTIN_LoadModule)(LPCSTR name, BOOL32 force);

#endif  /* __WINE_MODULE_H */

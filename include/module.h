/*
 * Module definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MODULE_H
#define __WINE_MODULE_H

#include "windef.h"
#include "dosexe.h"
#include "pe_image.h"

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
    HMODULE  module32;      /* 40 PE module handle for Win32 modules */
    HMODULE16  self;          /* 44 Handle for this module */
    WORD    self_loading_sel; /* 46 Selector used for self-loading apps. */
    LPDOSTASK lpDosTask;
    LPVOID  dos_image;        /* pointer to DOS memory (for DOS apps) */
    LPVOID  hRsrcMap;         /* HRSRC 16->32 map (for 32-bit modules) */
} NE_MODULE;


typedef struct {
    BYTE type;
    BYTE flags;
    BYTE segnum;
    WORD offs WINE_PACKED;
} ET_ENTRY;

typedef struct {
    WORD first; /* ordinal */
    WORD last; /* ordinal */
    WORD next; /* bundle */
} ET_BUNDLE;


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

#include "pshpack1.h"

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
} LOADPARAMS16;

typedef struct 
{
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    UINT16 *lpCmdShow;
    DWORD dwReserved;
} LOADPARAMS;

#include "poppack.h"

/* internal representation of 32bit modules. per process. */
typedef enum {
	MODULE32_PE = 1,
	MODULE32_ELF,
	MODULE32_ELFDLL,
	MODULE32_BI
} MODULE32_TYPE;

typedef struct _wine_modref
{
	struct _wine_modref	*next;
	struct _wine_modref	*prev;
	MODULE32_TYPE		type;
	union {
		PE_MODREF	pe;
		ELF_MODREF	elf;
	} binfmt;

	HMODULE			module;

	int			nDeps;
	struct _wine_modref	**deps;

	int			flags;
	int			refCount;

	char			*filename;
	char			*modname;
	char			*short_filename;
	char			*short_modname;
} WINE_MODREF;

#define WINE_MODREF_INTERNAL              0x00000001
#define WINE_MODREF_NO_DLL_CALLS          0x00000002
#define WINE_MODREF_PROCESS_ATTACHED      0x00000004
#define WINE_MODREF_LOAD_AS_DATAFILE      0x00000010
#define WINE_MODREF_DONT_RESOLVE_REFS     0x00000020
#define WINE_MODREF_MARKER                0x80000000



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
extern FARPROC MODULE_GetProcAddress( HMODULE hModule, LPCSTR function, BOOL snoop );
extern WINE_MODREF *MODULE32_LookupHMODULE( HMODULE hModule );
extern BOOL MODULE_DllProcessAttach( WINE_MODREF *wm, LPVOID lpReserved );
extern void MODULE_DllProcessDetach( BOOL bForceDetach, LPVOID lpReserved );
extern void MODULE_DllThreadAttach( LPVOID lpReserved );
extern void MODULE_DllThreadDetach( LPVOID lpReserved );
extern WINE_MODREF *MODULE_LoadLibraryExA( LPCSTR libname, HFILE hfile, DWORD flags );
extern BOOL MODULE_FreeLibrary( WINE_MODREF *wm );
extern WINE_MODREF *MODULE_FindModule( LPCSTR path );
extern HMODULE MODULE_CreateDummyModule( LPCSTR filename, WORD version );
extern FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE hmodule, LPCSTR name );
extern SEGPTR WINAPI HasGPHandler16( SEGPTR address );
extern void MODULE_WalkModref( DWORD id );

/* resource.c */
extern INT       WINAPI AccessResource(HMODULE,HRSRC); 

/* loader/ne/module.c */
extern NE_MODULE *NE_GetPtr( HMODULE16 hModule );
extern void NE_DumpModule( HMODULE16 hModule );
extern void NE_WalkModules(void);
extern void NE_RegisterModule( NE_MODULE *pModule );
extern WORD NE_GetOrdinal( HMODULE16 hModule, const char *name );
extern FARPROC16 WINAPI NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal );
extern FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop );
extern BOOL16 NE_SetEntryPoint( HMODULE16 hModule, WORD ordinal, WORD offset );
extern HANDLE NE_OpenFile( NE_MODULE *pModule );
extern BOOL NE_CreateProcess( HANDLE hFile, LPCSTR filename, LPCSTR cmd_line, LPCSTR env, 
                              LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                              BOOL inherit, DWORD flags, LPSTARTUPINFOA startup,
                              LPPROCESS_INFORMATION info );
extern BOOL NE_InitProcess( NE_MODULE *pModule );


/* loader/ne/resource.c */
extern HGLOBAL16 WINAPI NE_DefResourceHandler(HGLOBAL16,HMODULE16,HRSRC16);
extern BOOL NE_InitResourceHandler( HMODULE16 hModule );
extern HRSRC16 NE_FindResource( NE_MODULE *pModule, LPCSTR name, LPCSTR type );
extern INT16 NE_AccessResource( NE_MODULE *pModule, HRSRC16 hRsrc );
extern DWORD NE_SizeofResource( NE_MODULE *pModule, HRSRC16 hRsrc );
extern HGLOBAL16 NE_LoadResource( NE_MODULE *pModule, HRSRC16 hRsrc );
extern BOOL16 NE_FreeResource( NE_MODULE *pModule, HGLOBAL16 handle );
extern NE_TYPEINFO *NE_FindTypeSection( LPBYTE pResTab, NE_TYPEINFO *pTypeInfo, LPCSTR typeId );
extern NE_NAMEINFO *NE_FindResourceFromType( LPBYTE pResTab, NE_TYPEINFO *pTypeInfo, LPCSTR resId );

/* loader/ne/segment.c */
extern BOOL NE_LoadSegment( NE_MODULE *pModule, WORD segnum );
extern BOOL NE_LoadAllSegments( NE_MODULE *pModule );
extern BOOL NE_CreateSegment( NE_MODULE *pModule, int segnum );
extern BOOL NE_CreateAllSegments( NE_MODULE *pModule );
extern HINSTANCE16 NE_GetInstance( NE_MODULE *pModule );
extern void NE_InitializeDLLs( HMODULE16 hModule );

/* loader/ne/convert.c */
HGLOBAL16 NE_LoadPEResource( NE_MODULE *pModule, WORD type, LPVOID bits, DWORD size );

/* relay32/builtin.c */
extern WINE_MODREF *BUILTIN32_LoadLibraryExA(LPCSTR name, DWORD flags, DWORD *err);
extern void BUILTIN32_UnloadLibrary(WINE_MODREF *wm);

#endif  /* __WINE_MODULE_H */

/*
 * Win32 builtin functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "winuser.h"
#include "builtin32.h"
#include "peexe.h"
#include "neexe.h"
#include "heap.h"
#include "main.h"
#include "snoop.h"
#include "winerror.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(relay)
DECLARE_DEBUG_CHANNEL(win32)
DECLARE_DEBUG_CHANNEL(module)

typedef struct
{
    BYTE  call;                    /* 0xe8 call callfrom32 (relative) */
    DWORD callfrom32 WINE_PACKED;  /* RELAY_CallFrom32 relative addr */
    BYTE  ret;                     /* 0xc2 ret $n  or  0xc3 ret */
    WORD  args;                    /* nb of args to remove from the stack */
} DEBUG_ENTRY_POINT;

typedef struct
{
	const BYTE			*restab;
	const DWORD			nresources;
	const DWORD			restabsize;
	const IMAGE_RESOURCE_DATA_ENTRY	*entries;
} BUILTIN32_RESOURCE;

typedef struct
{
	const BUILTIN32_DESCRIPTOR	*descr;	/* DLL descriptor */
	DWORD				flags;
	const BUILTIN32_RESOURCE	*rsc;	
} BUILTIN32_DLL;

#define BI32_INSTANTIATED	0x01

extern const BUILTIN32_DESCRIPTOR ADVAPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR AVIFIL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR COMCTL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR COMDLG32_Descriptor;
extern const BUILTIN32_DESCRIPTOR CRTDLL_Descriptor;
extern const BUILTIN32_DESCRIPTOR DCIMAN32_Descriptor;
extern const BUILTIN32_DESCRIPTOR DDRAW_Descriptor;
extern const BUILTIN32_DESCRIPTOR DINPUT_Descriptor;
extern const BUILTIN32_DESCRIPTOR DPLAY_Descriptor;
extern const BUILTIN32_DESCRIPTOR DPLAYX_Descriptor;
extern const BUILTIN32_DESCRIPTOR DSOUND_Descriptor;
extern const BUILTIN32_DESCRIPTOR GDI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR IMAGEHLP_Descriptor;
extern const BUILTIN32_DESCRIPTOR IMM32_Descriptor;
extern const BUILTIN32_DESCRIPTOR KERNEL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR LZ32_Descriptor;
extern const BUILTIN32_DESCRIPTOR MPR_Descriptor;
extern const BUILTIN32_DESCRIPTOR MCIAVI_Descriptor;
extern const BUILTIN32_DESCRIPTOR MCIANIM_Descriptor;
extern const BUILTIN32_DESCRIPTOR MCICDA_Descriptor;
extern const BUILTIN32_DESCRIPTOR MCISEQ_Descriptor;
extern const BUILTIN32_DESCRIPTOR MCIWAVE_Descriptor;
extern const BUILTIN32_DESCRIPTOR MSACM32_Descriptor;
extern const BUILTIN32_DESCRIPTOR MSNET32_Descriptor;
extern const BUILTIN32_DESCRIPTOR MSVFW32_Descriptor;
extern const BUILTIN32_DESCRIPTOR NTDLL_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLE32_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLEAUT32_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLECLI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLEDLG_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLESVR32_Descriptor;
extern const BUILTIN32_DESCRIPTOR PSAPI_Descriptor;
extern const BUILTIN32_DESCRIPTOR RASAPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR SHELL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR TAPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR USER32_Descriptor;
extern const BUILTIN32_DESCRIPTOR VERSION_Descriptor;
extern const BUILTIN32_DESCRIPTOR W32SKRNL_Descriptor;
extern const BUILTIN32_DESCRIPTOR WINMM_Descriptor;
extern const BUILTIN32_DESCRIPTOR WINSPOOL_Descriptor;
extern const BUILTIN32_DESCRIPTOR WNASPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR WOW32_Descriptor;
extern const BUILTIN32_DESCRIPTOR WSOCK32_Descriptor;

extern const BUILTIN32_RESOURCE comctl32_ResourceDescriptor;
extern const BUILTIN32_RESOURCE comdlg32_ResourceDescriptor;
extern const BUILTIN32_RESOURCE shell32_ResourceDescriptor;
extern const BUILTIN32_RESOURCE user32_ResourceDescriptor;

static BUILTIN32_DLL BuiltinDLLs[] =
{
    { &ADVAPI32_Descriptor, 0, NULL },
    { &AVIFIL32_Descriptor, 0, NULL },
    { &COMCTL32_Descriptor, 0, &comctl32_ResourceDescriptor },
    { &COMDLG32_Descriptor, 0, &comdlg32_ResourceDescriptor },
    { &CRTDLL_Descriptor,   0, NULL },
    { &DCIMAN32_Descriptor, 0, NULL },
    { &DDRAW_Descriptor,    0, NULL },
    { &DINPUT_Descriptor,   0, NULL },
    { &DPLAY_Descriptor,    0, NULL },
    { &DPLAYX_Descriptor,   0, NULL },
    { &DSOUND_Descriptor,   0, NULL },
    { &GDI32_Descriptor,    0, NULL },
    { &IMAGEHLP_Descriptor, 0, NULL },
    { &IMM32_Descriptor,    0, NULL },
    { &KERNEL32_Descriptor, 0, NULL },
    { &LZ32_Descriptor,     0, NULL },
    { &MCIANIM_Descriptor,  0, NULL },
    { &MCIAVI_Descriptor,   0, NULL },
    { &MCICDA_Descriptor,   0, NULL },
    { &MCISEQ_Descriptor,   0, NULL },
    { &MCIWAVE_Descriptor,  0, NULL },
    { &MPR_Descriptor,      0, NULL },
    { &MSACM32_Descriptor,  0, NULL },
    { &MSNET32_Descriptor,  0, NULL },
    { &MSVFW32_Descriptor,  0, NULL },
    { &NTDLL_Descriptor,    0, NULL },
    { &OLE32_Descriptor,    0, NULL },
    { &OLEAUT32_Descriptor, 0, NULL },
    { &OLECLI32_Descriptor, 0, NULL },
    { &OLEDLG_Descriptor,   0, NULL },
    { &OLESVR32_Descriptor, 0, NULL },
    { &PSAPI_Descriptor,    0, NULL },
    { &RASAPI32_Descriptor, 0, NULL },
    { &SHELL32_Descriptor,  0, &shell32_ResourceDescriptor },
    { &TAPI32_Descriptor,   0, NULL },
    { &USER32_Descriptor,   0, &user32_ResourceDescriptor },
    { &VERSION_Descriptor,  0, NULL },
    { &W32SKRNL_Descriptor, 0, NULL },
    { &WINMM_Descriptor,    0, NULL },
    { &WINSPOOL_Descriptor, 0, NULL },
    { &WNASPI32_Descriptor, 0, NULL },
    { &WOW32_Descriptor,    0, NULL },
    { &WSOCK32_Descriptor,  0, NULL },
    /* Last entry */
    { NULL, 0, NULL }
};

extern void RELAY_CallFrom32();
extern void RELAY_CallFrom32Regs();

/***********************************************************************
 *           BUILTIN32_DoLoadImage
 *
 * Load a built-in Win32 module. Helper function for BUILTIN32_LoadImage.
 */
static HMODULE BUILTIN32_DoLoadImage( BUILTIN32_DLL *dll )
{

    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_EXPORT_DIRECTORY *exp;
    IMAGE_IMPORT_DESCRIPTOR *imp;
    LPVOID *funcs;
    LPSTR *names;
    LPSTR pfwd;
    DEBUG_ENTRY_POINT *debug;
    INT i, size, nb_sections;
    BYTE *addr;

    /* Allocate the module */

    nb_sections = 2;  /* exports + code */
    if (dll->descr->nb_imports) nb_sections++;
    size = (sizeof(IMAGE_DOS_HEADER)
            + sizeof(IMAGE_NT_HEADERS)
            + nb_sections * sizeof(IMAGE_SECTION_HEADER)
            + (dll->descr->nb_imports+1) * sizeof(IMAGE_IMPORT_DESCRIPTOR)
            + sizeof(IMAGE_EXPORT_DIRECTORY)
            + dll->descr->nb_funcs * sizeof(LPVOID)
            + dll->descr->nb_names * sizeof(LPSTR)
            + dll->descr->fwd_size);
#ifdef __i386__
    if (WARN_ON(relay) || TRACE_ON(relay))
        size += dll->descr->nb_funcs * sizeof(DEBUG_ENTRY_POINT);
#endif
    addr  = VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!addr) return 0;
    dos   = (IMAGE_DOS_HEADER *)addr;
    nt    = (IMAGE_NT_HEADERS *)(dos + 1);
    sec   = (IMAGE_SECTION_HEADER *)(nt + 1);
    imp   = (IMAGE_IMPORT_DESCRIPTOR *)(sec + nb_sections);
    exp   = (IMAGE_EXPORT_DIRECTORY *)(imp + dll->descr->nb_imports + 1);
    funcs = (LPVOID *)(exp + 1);
    names = (LPSTR *)(funcs + dll->descr->nb_funcs);
    pfwd  = (LPSTR)(names + dll->descr->nb_names);
    debug = (DEBUG_ENTRY_POINT *)(pfwd + dll->descr->fwd_size);

    /* Build the DOS and NT headers */

    dos->e_magic  = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof(*dos);

    nt->Signature                       = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine              = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections     = nb_sections;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
    nt->FileHeader.Characteristics      = IMAGE_FILE_DLL;

    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt->OptionalHeader.SizeOfCode                  = 0x1000;
    nt->OptionalHeader.SizeOfInitializedData       = 0;
    nt->OptionalHeader.SizeOfUninitializedData     = 0;
    nt->OptionalHeader.ImageBase                   = (DWORD)addr;
    nt->OptionalHeader.SectionAlignment            = 0x1000;
    nt->OptionalHeader.FileAlignment               = 0x1000;
    nt->OptionalHeader.MajorOperatingSystemVersion = 1;
    nt->OptionalHeader.MinorOperatingSystemVersion = 0;
    nt->OptionalHeader.MajorSubsystemVersion       = 4;
    nt->OptionalHeader.MinorSubsystemVersion       = 0;
    nt->OptionalHeader.SizeOfImage                 = size;
    nt->OptionalHeader.SizeOfHeaders               = (BYTE *)exp - addr;
    nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    if (dll->descr->dllentrypoint) 
        nt->OptionalHeader.AddressOfEntryPoint = (DWORD)dll->descr->dllentrypoint - (DWORD)addr;
    
    /* Build the code section */

    strcpy( sec->Name, ".code" );
    sec->SizeOfRawData = 0;
#ifdef __i386__
    if (WARN_ON(relay) || TRACE_ON(relay))
        sec->SizeOfRawData += dll->descr->nb_funcs * sizeof(DEBUG_ENTRY_POINT);
#endif
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = (BYTE *)debug - addr;
    sec->PointerToRawData = (BYTE *)debug - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ);
    sec++;

    /* Build the import directory */

    if (dll->descr->nb_imports)
    {
        dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
        dir->VirtualAddress = (BYTE *)imp - addr;
        dir->Size = sizeof(*imp) * (dll->descr->nb_imports + 1);

        /* Build the imports section */
        strcpy( sec->Name, ".idata" );
        sec->Misc.VirtualSize = dir->Size;
        sec->VirtualAddress   = (BYTE *)imp - addr;
        sec->SizeOfRawData    = dir->Size;
        sec->PointerToRawData = (BYTE *)imp - addr;
        sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                                 IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                 IMAGE_SCN_MEM_WRITE);
        sec++;

        /* Build the imports */
        for (i = 0; i < dll->descr->nb_imports; i++)
        {
            imp[i].u.Characteristics = 0;
            imp[i].ForwarderChain = -1;
            imp[i].Name = (BYTE *)dll->descr->imports[i] - addr;
            /* hack: make first thunk point to some zero value */
            imp[i].FirstThunk = (PIMAGE_THUNK_DATA)((BYTE *)&imp[i].u.Characteristics - addr);
        }
    }

    /* Build the export directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
    dir->VirtualAddress = (BYTE *)exp - addr;
    dir->Size = sizeof(*exp)
                + dll->descr->nb_funcs * sizeof(LPVOID)
                + dll->descr->nb_names * sizeof(LPSTR)
                + dll->descr->fwd_size;

    /* Build the exports section */

    strcpy( sec->Name, ".edata" );
    sec->Misc.VirtualSize = dir->Size;
    sec->VirtualAddress   = (BYTE *)exp - addr;
    sec->SizeOfRawData    = dir->Size;
    sec->PointerToRawData = (BYTE *)exp - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                             IMAGE_SCN_MEM_WRITE);
    sec++;

    /* Build the resource directory */
    if(dll->rsc)
    {
	int i;
	void *rtab;
	IMAGE_RESOURCE_DATA_ENTRY *rdep;

	rtab = HeapAlloc(GetProcessHeap(), 0, dll->rsc->restabsize);
	if(!rtab)
	{
		ERR_(module)("Failed to get memory for resource directory\n");
		VirtualFree(addr, size, MEM_RELEASE);
		return 0;
	}

	/*
	 * The resource directory has to be copied because it contains
	 * RVAs. These would be invalid if the dll is instantiated twice.
	 */
	memcpy(rtab, dll->rsc->restab, dll->rsc->restabsize);

	dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
	dir->VirtualAddress = (DWORD)rtab - (DWORD)addr;
	dir->Size = dll->rsc->restabsize;
	rdep = (IMAGE_RESOURCE_DATA_ENTRY *)((DWORD)rtab + (DWORD)dll->rsc->entries - (DWORD)dll->rsc->restab);
	for(i = 0; i < dll->rsc->nresources; i++)
	{
		rdep[i].OffsetToData += (DWORD)dll->rsc->restab - (DWORD)addr;
	}
    }

    /* Build the exports section data */

    exp->Name                  = ((BYTE *)dll->descr->name) - addr;  /*??*/
    exp->Base                  = dll->descr->base;
    exp->NumberOfFunctions     = dll->descr->nb_funcs;
    exp->NumberOfNames         = dll->descr->nb_names;
    exp->AddressOfFunctions    = (LPDWORD *)((BYTE *)funcs - addr);
    exp->AddressOfNames        = (LPDWORD *)((BYTE *)names - addr);
    exp->AddressOfNameOrdinals = (LPWORD *)((BYTE *)dll->descr->ordinals - addr);

    /* Build the funcs table */

    for (i = 0; i < dll->descr->nb_funcs; i++, funcs++, debug++)
    {
        BYTE args = dll->descr->args[i];
	int j;

        if (!dll->descr->functions[i]) continue;

        if (args == 0xfd)  /* forward func */
        {
            strcpy( pfwd, (LPSTR)dll->descr->functions[i] );
            *funcs = (LPVOID)((BYTE *)pfwd - addr);
            pfwd += strlen(pfwd) + 1;
        }
        else *funcs = (LPVOID)((BYTE *)dll->descr->functions[i] - addr);

#ifdef __i386__
	if (!(WARN_ON(relay) || TRACE_ON(relay))) continue;
	for (j=0;j<dll->descr->nb_names;j++)
	    if (dll->descr->ordinals[j] == i)
		break;
	if (j<dll->descr->nb_names) {
	    if (dll->descr->names[j]) {
	    	char buffer[200];
		sprintf(buffer,"%s.%d: %s",dll->descr->name,i,dll->descr->names[j]);
		if (!RELAY_ShowDebugmsgRelay(buffer))
		    continue;
	    }
	}
        switch(args)
        {
        case 0xfd:  /* forward */
        case 0xff:  /* stub or extern */
            break;
        default:  /* normal function (stdcall or cdecl or register) */
	    if (TRACE_ON(relay)) {
		debug->call       = 0xe8; /* lcall relative */
                if (args & 0x40)  /* register func */
                    debug->callfrom32 = (DWORD)RELAY_CallFrom32Regs -
                        (DWORD)&debug->ret;
                else
                    debug->callfrom32 = (DWORD)RELAY_CallFrom32 -
                        (DWORD)&debug->ret;
	    } else {
		debug->call       = 0xe9; /* ljmp relative */
		debug->callfrom32 = (DWORD)dll->descr->functions[i] -
				    (DWORD)&debug->ret;
	    }
	    debug->ret        = (args & 0x80) ? 0xc3 : 0xc2; /*ret/ret $n*/
	    debug->args       = (args & 0x3f) * sizeof(int);
            *funcs = (LPVOID)((BYTE *)debug - addr);
            break;
        }
#endif  /* __i386__ */
    }

    /* Build the names table */

    for (i = 0; i < exp->NumberOfNames; i++, names++)
        if (dll->descr->names[i])
            *names = (LPSTR)((BYTE *)dll->descr->names[i] - addr);

    return (HMODULE)addr;
}

/***********************************************************************
 *           BUILTIN32_LoadImage
 *
 * Load a built-in module.
 */
HMODULE BUILTIN32_LoadImage( LPCSTR name, OFSTRUCT *ofs)
{
    BUILTIN32_DLL *table;
    char dllname[16], *p;
    HMODULE hmod;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    lstrcpynA( dllname, name, sizeof(dllname) );
    if ((p = strrchr( dllname, '.' ))) *p = '\0';

    for (table = BuiltinDLLs; table->descr; table++)
        if (!lstrcmpiA( table->descr->name, dllname )) break;
    if (!table->descr) return 0;

    if(table->flags & BI32_INSTANTIATED)
    {
	ERR_(module)("Attemp to instantiate built-in dll '%s' twice in the same address-space. Expect trouble!\n",
		table->descr->name);
    }

    sprintf( ofs->szPathName, "%s.DLL", table->descr->name );
    hmod = BUILTIN32_DoLoadImage( table );
    if(hmod)
    	table->flags |= BI32_INSTANTIATED;

    return hmod;
}


/***********************************************************************
 *           BUILTIN32_LoadLibraryExA
 *
 * Partly copied from the original PE_ version.
 *
 * Note: This implementation is not very nice and should be one with
 * the BUILTIN32_LoadImage function. But, we don't care too much
 * because this code will obsolete itself shortly when we get the
 * modularization of wine implemented (BS 05-Mar-1999).
 */
WINE_MODREF *BUILTIN32_LoadLibraryExA(LPCSTR path, DWORD flags, DWORD *err)
{
	LPCSTR		modName = NULL;
	OFSTRUCT	ofs;
	HMODULE		hModule32;
	HMODULE16	hModule16;
	NE_MODULE	*pModule;
	WINE_MODREF	*wm;
	char		dllname[256], *p;

	/* Append .DLL to name if no extension present */
	strcpy( dllname, path );
	if (!(p = strrchr( dllname, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
		strcat( dllname, ".DLL" );

	hModule32 = BUILTIN32_LoadImage(path, &ofs);
	if(!hModule32)
	{
		*err = ERROR_FILE_NOT_FOUND;
		return NULL;
	}

	/* Create 16-bit dummy module */
	if ((hModule16 = MODULE_CreateDummyModule( &ofs, modName )) < 32)
	{
		*err = (DWORD)hModule16;
		return NULL;	/* FIXME: Should unload the builtin module */
	}

	pModule = (NE_MODULE *)GlobalLock16( hModule16 );
	pModule->flags = NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA | NE_FFLAGS_WIN32 | NE_FFLAGS_BUILTIN;
	pModule->module32 = hModule32;

	/* Create 32-bit MODREF */
	if ( !(wm = PE_CreateModule( hModule32, &ofs, flags, TRUE )) )
	{
		ERR_(win32)("can't load %s\n",ofs.szPathName);
		FreeLibrary16( hModule16 );	/* FIXME: Should unload the builtin module */
		*err = ERROR_OUTOFMEMORY;
		return NULL;
	}

	if (wm->binfmt.pe.pe_export)
		SNOOP_RegisterDLL(wm->module,wm->modname,wm->binfmt.pe.pe_export->NumberOfFunctions);

	*err = 0;
	return wm;
}


/***********************************************************************
 *	BUILTIN32_UnloadLibrary
 *
 * Unload the built-in library and free the modref.
 */
void BUILTIN32_UnloadLibrary(WINE_MODREF *wm)
{
	/* FIXME: do something here */
}


/***********************************************************************
 *           BUILTIN32_GetEntryPoint
 *
 * Return the name of the DLL entry point corresponding
 * to a relay entry point address. This is used only by relay debugging.
 *
 * This function _must_ return the real entry point to call
 * after the debug info is printed.
 */
ENTRYPOINT32 BUILTIN32_GetEntryPoint( char *buffer, void *relay,
                                      unsigned int *typemask )
{
    BUILTIN32_DLL *dll;
    HMODULE hModule;
    int ordinal = 0, i;

    /* First find the module */

    for (dll = BuiltinDLLs; dll->descr; dll++)
        if ((dll->flags & BI32_INSTANTIATED) 
            && ((hModule = GetModuleHandleA(dll->descr->name)) != 0))
        {
            IMAGE_SECTION_HEADER *sec = PE_SECTIONS(hModule);
            DEBUG_ENTRY_POINT *debug = 
                 (DEBUG_ENTRY_POINT *)((DWORD)hModule + sec[0].VirtualAddress);
            DEBUG_ENTRY_POINT *func = (DEBUG_ENTRY_POINT *)relay;

            if (debug <= func && func < debug + dll->descr->nb_funcs)
            {
                ordinal = func - debug;
                break;
            }
        }
    
    if (!dll->descr)
    	return (ENTRYPOINT32)NULL;

    /* Now find the function */

    for (i = 0; i < dll->descr->nb_names; i++)
        if (dll->descr->ordinals[i] == ordinal) break;
    assert( i < dll->descr->nb_names );

    sprintf( buffer, "%s.%d: %s", dll->descr->name, ordinal + dll->descr->base,
             dll->descr->names[i] );
    *typemask = dll->descr->argtypes[ordinal];
    return dll->descr->functions[ordinal];
}

/***********************************************************************
 *           BUILTIN32_SwitchRelayDebug
 *
 * FIXME: enhance to do it module relative.
 */
void BUILTIN32_SwitchRelayDebug(BOOL onoff) {
    BUILTIN32_DLL *dll;
    HMODULE hModule;
    int i;

#ifdef __i386__
    if (!(TRACE_ON(relay) || WARN_ON(relay)))
    	return;
    for (dll = BuiltinDLLs; dll->descr; dll++) {
	IMAGE_SECTION_HEADER *sec;
	DEBUG_ENTRY_POINT *debug;
        if (!(dll->flags & BI32_INSTANTIATED) || !(hModule = GetModuleHandleA(dll->descr->name)))
	    continue;

	sec = PE_SECTIONS(hModule);
	debug = (DEBUG_ENTRY_POINT *)((DWORD)hModule + sec[1].VirtualAddress);
	for (i = 0; i < dll->descr->nb_funcs; i++,debug++) {
	    if (!dll->descr->functions[i]) continue;
	    if ((dll->descr->args[i]==0xff) || (dll->descr->args[i]==0xfe))
	    	continue;
	    if (onoff) {
                debug->call       = 0xe8; /* lcall relative */
                debug->callfrom32 = (DWORD)RELAY_CallFrom32 -
                                    (DWORD)&debug->ret;
	    } else {
                debug->call       = 0xe9; /* ljmp relative */
                debug->callfrom32 = (DWORD)dll->descr->functions[i] -
                                    (DWORD)&debug->ret;
	    }
        }
    }
#endif /* __i386__ */
    return;
}

/***********************************************************************
 *           BUILTIN32_Unimplemented
 *
 * This function is called for unimplemented 32-bit entry points (declared
 * as 'stub' in the spec file).
 */
void BUILTIN32_Unimplemented( const BUILTIN32_DESCRIPTOR *descr, int ordinal )
{
    const char *func_name = "???";
    int i;

    __RESTORE_ES;  /* Just in case */

    for (i = 0; i < descr->nb_names; i++)
        if (descr->ordinals[i] + descr->base == ordinal) break;
    if (i < descr->nb_names) func_name = descr->names[i];

    MESSAGE( "No handler for Win32 routine %s.%d: %s",
             descr->name, ordinal, func_name );
#ifdef __GNUC__
    MESSAGE( " (called from %p)", __builtin_return_address(1) );
#endif
    MESSAGE( "\n" );
    ExitProcess(1);
}


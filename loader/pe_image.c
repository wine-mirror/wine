/* 
 *  Copyright	1994	Eric Youndale & Erik Bos
 *  Copyright	1995	Martin von Löwis
 *  Copyright   1996-98 Marcus Meissner
 *
 *	based on Eric Youndale's pe-test and:
 *
 *	ftp.microsoft.com:/pub/developer/MSDN/CD8/PEFILE.ZIP
 * make that:
 *	ftp.microsoft.com:/developr/MSDN/OctCD/PEFILE.ZIP
 */
/* Notes:
 * Before you start changing something in this file be aware of the following:
 *
 * - There are several functions called recursively. In a very subtle and 
 *   obscure way. DLLs can reference each other recursively etc.
 * - If you want to enhance, speed up or clean up something in here, think
 *   twice WHY it is implemented in that strange way. There is usually a reason.
 *   Though sometimes it might just be lazyness ;)
 * - In PE_MapImage, right before fixup_imports() all external and internal 
 *   state MUST be correct since this function can be called with the SAME image
 *   AGAIN. (Thats recursion for you.) That means MODREF.module and
 *   NE_MODULE.module32.
 */

#include "config.h"

#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include "wine/winbase16.h"
#include "winerror.h"
#include "neexe.h"
#include "process.h"
#include "snoop.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);
DECLARE_DEBUG_CHANNEL(delayhlp);
DECLARE_DEBUG_CHANNEL(fixup);
DECLARE_DEBUG_CHANNEL(module);
DECLARE_DEBUG_CHANNEL(relay);
DECLARE_DEBUG_CHANNEL(segment);


static IMAGE_EXPORT_DIRECTORY *get_exports( HMODULE hmod )
{
    IMAGE_EXPORT_DIRECTORY *ret = NULL;
    IMAGE_DATA_DIRECTORY *dir = PE_HEADER(hmod)->OptionalHeader.DataDirectory
                                + IMAGE_DIRECTORY_ENTRY_EXPORT;
    if (dir->Size && dir->VirtualAddress)
        ret = (IMAGE_EXPORT_DIRECTORY *)((char *)hmod + dir->VirtualAddress);
    return ret;
}

static IMAGE_IMPORT_DESCRIPTOR *get_imports( HMODULE hmod )
{
    IMAGE_IMPORT_DESCRIPTOR *ret = NULL;
    IMAGE_DATA_DIRECTORY *dir = PE_HEADER(hmod)->OptionalHeader.DataDirectory
                                + IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (dir->Size && dir->VirtualAddress)
        ret = (IMAGE_IMPORT_DESCRIPTOR *)((char *)hmod + dir->VirtualAddress);
    return ret;
}


/* convert PE image VirtualAddress to Real Address */
#define RVA(x) ((void *)((char *)load_addr+(unsigned int)(x)))

#define AdjustPtr(ptr,delta) ((char *)(ptr) + (delta))

void dump_exports( HMODULE hModule )
{ 
  char		*Module;
  int		i, j;
  u_short	*ordinal;
  u_long	*function,*functions;
  u_char	**name;
  unsigned int load_addr = hModule;

  DWORD rva_start = PE_HEADER(hModule)->OptionalHeader
                   .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
  DWORD rva_end = rva_start + PE_HEADER(hModule)->OptionalHeader
                   .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
  IMAGE_EXPORT_DIRECTORY *pe_exports = (IMAGE_EXPORT_DIRECTORY*)RVA(rva_start);

  Module = (char*)RVA(pe_exports->Name);
  TRACE("*******EXPORT DATA*******\n");
  TRACE("Module name is %s, %ld functions, %ld names\n", 
        Module, pe_exports->NumberOfFunctions, pe_exports->NumberOfNames);

  ordinal=(u_short*) RVA(pe_exports->AddressOfNameOrdinals);
  functions=function=(u_long*) RVA(pe_exports->AddressOfFunctions);
  name=(u_char**) RVA(pe_exports->AddressOfNames);

  TRACE(" Ord    RVA     Addr   Name\n" );
  for (i=0;i<pe_exports->NumberOfFunctions;i++, function++)
  {
      if (!*function) continue;  /* No such function */
      if (TRACE_ON(win32))
      {
	DPRINTF( "%4ld %08lx %p", i + pe_exports->Base, *function, RVA(*function) );
	/* Check if we have a name for it */
	for (j = 0; j < pe_exports->NumberOfNames; j++)
          if (ordinal[j] == i)
          {
              DPRINTF( "  %s", (char*)RVA(name[j]) );
              break;
          }
	if ((*function >= rva_start) && (*function <= rva_end))
	  DPRINTF(" (forwarded -> %s)", (char *)RVA(*function));
	DPRINTF("\n");
      }
  }
}

/* Look up the specified function or ordinal in the exportlist:
 * If it is a string:
 * 	- look up the name in the Name list. 
 *	- look up the ordinal with that index.
 *	- use the ordinal as offset into the functionlist
 * If it is a ordinal:
 *	- use ordinal-pe_export->Base as offset into the functionlist
 */
static FARPROC PE_FindExportedFunction( 
	WINE_MODREF *wm,	/* [in] WINE modreference */
	LPCSTR funcName,	/* [in] function name */
        BOOL snoop )
{
	u_short				* ordinals;
	u_long				* function;
	u_char				** name, *ename = NULL;
	int				i, ordinal;
	unsigned int			load_addr = wm->module;
	u_long				rva_start, rva_end, addr;
	char				* forward;
	IMAGE_EXPORT_DIRECTORY *exports = get_exports(wm->module);

	if (HIWORD(funcName))
		TRACE("(%s)\n",funcName);
	else
		TRACE("(%d)\n",(int)funcName);
	if (!exports) {
		/* Not a fatal problem, some apps do
		 * GetProcAddress(0,"RegisterPenApp") which triggers this
		 * case.
		 */
		WARN("Module %08x(%s)/MODREF %p doesn't have a exports table.\n",wm->module,wm->modname,wm);
		return NULL;
	}
	ordinals= (u_short*)  RVA(exports->AddressOfNameOrdinals);
	function= (u_long*)   RVA(exports->AddressOfFunctions);
	name	= (u_char **) RVA(exports->AddressOfNames);
	forward = NULL;
	rva_start = PE_HEADER(wm->module)->OptionalHeader
		.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	rva_end = rva_start + PE_HEADER(wm->module)->OptionalHeader
		.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (HIWORD(funcName))
        {
            /* first try a binary search */
            int min = 0, max = exports->NumberOfNames - 1;
            while (min <= max)
            {
                int res, pos = (min + max) / 2;
                ename = RVA(name[pos]);
                if (!(res = strcmp( ename, funcName )))
                {
                    ordinal = ordinals[pos];
                    goto found;
                }
                if (res > 0) max = pos - 1;
                else min = pos + 1;
            }
            /* now try a linear search in case the names aren't sorted properly */
            for (i = 0; i < exports->NumberOfNames; i++)
            {
                ename = RVA(name[i]);
                if (!strcmp( ename, funcName ))
                {
                    ERR( "%s.%s required a linear search\n", wm->modname, funcName );
                    ordinal = ordinals[i];
                    goto found;
                }
            }
            return NULL;
	}
        else  /* find by ordinal */
        {
            ordinal = LOWORD(funcName) - exports->Base;
            if (snoop && name)  /* need to find a name for it */
            {
                for (i = 0; i < exports->NumberOfNames; i++)
                    if (ordinals[i] == ordinal)
                    {
                        ename = RVA(name[i]);
                        break;
                    }
            }
	}

 found:
        if (ordinal >= exports->NumberOfFunctions)
        {
            TRACE("	ordinal %ld out of range!\n", ordinal + exports->Base );
            return NULL;
        }
        addr = function[ordinal];
        if (!addr) return NULL;
        if ((addr < rva_start) || (addr >= rva_end))
        {
            FARPROC proc = RVA(addr);
            if (snoop)
            {
                if (!ename) ename = "@";
                proc = SNOOP_GetProcAddress(wm->module,ename,ordinal,proc);
            }
            return proc;
        }
        else  /* forward entry point */
        {
                WINE_MODREF *wm;
                FARPROC proc;
                char *forward = RVA(addr);
		char module[256];
		char *end = strchr(forward, '.');

		if (!end) return NULL;
                if (end - forward >= sizeof(module)) return NULL;
                memcpy( module, forward, end - forward );
		module[end-forward] = 0;
                if (!(wm = MODULE_FindModule( module )))
                {
                    ERR("module not found for forward '%s'\n", forward );
                    return NULL;
                }
		if (!(proc = MODULE_GetProcAddress( wm->module, end + 1, snoop )))
                    ERR("function not found for forward '%s'\n", forward );
		return proc;
	}
}

DWORD fixup_imports( WINE_MODREF *wm )
{
    IMAGE_IMPORT_DESCRIPTOR	*pe_imp;
    unsigned int load_addr	= wm->module;
    int				i,characteristics_detection=1;
    char			*modname;
    IMAGE_EXPORT_DIRECTORY *exports = get_exports(wm->module);
    IMAGE_IMPORT_DESCRIPTOR *imports = get_imports(wm->module);
    
    if (exports)
    	modname = (char*) RVA(exports->Name);
    else
        modname = "<unknown>";

    /* first, count the number of imported non-internal modules */
    pe_imp = imports;
    if (!pe_imp) return 0;

    /* OK, now dump the import list */
    TRACE("Dumping imports list\n");

    /* We assume that we have at least one import with !0 characteristics and
     * detect broken imports with all characteristics 0 (notably Borland) and
     * switch the detection off for them.
     */
    for (i = 0; pe_imp->Name ; pe_imp++) {
	if (!i && !pe_imp->u.Characteristics)
		characteristics_detection = 0;
	if (characteristics_detection && !pe_imp->u.Characteristics)
		break;
	i++;
    }
    if (!i) return 0;  /* no imports */

    /* Allocate module dependency list */
    wm->nDeps = i;
    wm->deps  = HeapAlloc( GetProcessHeap(), 0, i*sizeof(WINE_MODREF *) );

    /* load the imported modules. They are automatically 
     * added to the modref list of the process.
     */
 
    for (i = 0, pe_imp = imports; pe_imp->Name ; pe_imp++) {
    	WINE_MODREF		*wmImp;
	IMAGE_IMPORT_BY_NAME	*pe_name;
	PIMAGE_THUNK_DATA	import_list,thunk_list;
 	char			*name = (char *) RVA(pe_imp->Name);

	if (characteristics_detection && !pe_imp->u.Characteristics)
		break;

	wmImp = MODULE_LoadLibraryExA( name, 0, 0 );
	if (!wmImp) {
	    ERR_(module)("Module (file) %s needed by %s not found\n", name, wm->filename);
	    return 1;
	}
        wm->deps[i++] = wmImp;

	/* FIXME: forwarder entries ... */

	if (pe_imp->u.OriginalFirstThunk != 0) { /* original MS style */
	    TRACE("Microsoft style imports used\n");
	    import_list =(PIMAGE_THUNK_DATA) RVA(pe_imp->u.OriginalFirstThunk);
	    thunk_list = (PIMAGE_THUNK_DATA) RVA(pe_imp->FirstThunk);

	    while (import_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal)) {
		    int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

		    TRACE("--- Ordinal %s,%d\n", name, ordinal);
		    thunk_list->u1.Function=MODULE_GetProcAddress(
                        wmImp->module, (LPCSTR)ordinal, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR("No implementation for %s.%d, setting to 0xdeadbeef\n",
				name, ordinal);
                        thunk_list->u1.Function = (FARPROC)0xdeadbeef;
		    }
		} else {		/* import by name */
		    pe_name = (PIMAGE_IMPORT_BY_NAME)RVA(import_list->u1.AddressOfData);
		    TRACE("--- %s %s.%d\n", pe_name->Name, name, pe_name->Hint);
		    thunk_list->u1.Function=MODULE_GetProcAddress(
                        wmImp->module, pe_name->Name, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR("No implementation for %s.%d(%s), setting to 0xdeadbeef\n",
				name,pe_name->Hint,pe_name->Name);
                        thunk_list->u1.Function = (FARPROC)0xdeadbeef;
		    }
		}
		import_list++;
		thunk_list++;
	    }
	} else {	/* Borland style */
	    TRACE("Borland style imports used\n");
	    thunk_list = (PIMAGE_THUNK_DATA) RVA(pe_imp->FirstThunk);
	    while (thunk_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(thunk_list->u1.Ordinal)) {
		    /* not sure about this branch, but it seems to work */
		    int ordinal = IMAGE_ORDINAL(thunk_list->u1.Ordinal);

		    TRACE("--- Ordinal %s.%d\n",name,ordinal);
		    thunk_list->u1.Function=MODULE_GetProcAddress(
                        wmImp->module, (LPCSTR) ordinal, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR("No implementation for %s.%d, setting to 0xdeadbeef\n",
				name,ordinal);
                        thunk_list->u1.Function = (FARPROC)0xdeadbeef;
		    }
		} else {
		    pe_name=(PIMAGE_IMPORT_BY_NAME) RVA(thunk_list->u1.AddressOfData);
		    TRACE("--- %s %s.%d\n",
		   		  pe_name->Name,name,pe_name->Hint);
		    thunk_list->u1.Function=MODULE_GetProcAddress(
                        wmImp->module, pe_name->Name, TRUE
		    );
		    if (!thunk_list->u1.Function) {
		    	ERR("No implementation for %s.%d, setting to 0xdeadbeef\n",
				name, pe_name->Hint);
                        thunk_list->u1.Function = (FARPROC)0xdeadbeef;
		    }
		}
		thunk_list++;
	    }
	}
    }
    return 0;
}

/***********************************************************************
 *           do_relocations
 *
 * Apply the relocations to a mapped PE image
 */
static int do_relocations( char *base, const IMAGE_NT_HEADERS *nt, const char *filename )
{
    const IMAGE_DATA_DIRECTORY *dir;
    const IMAGE_BASE_RELOCATION *rel;
    int delta = base - (char *)nt->OptionalHeader.ImageBase;

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    rel = (IMAGE_BASE_RELOCATION *)(base + dir->VirtualAddress);

    WARN("Info: base relocations needed for %s\n", filename);
    if (!dir->VirtualAddress || !dir->Size)
    {
        if (nt->OptionalHeader.ImageBase == 0x400000)
            ERR("Standard load address for a Win32 program not available - patched kernel ?\n");
        ERR( "FATAL: Need to relocate %s, but no relocation records present (%s). Try to run that file directly !\n",
             filename,
             (nt->FileHeader.Characteristics&IMAGE_FILE_RELOCS_STRIPPED)?
             "stripped during link" : "unknown reason" );
        return 0;
    }

    /* FIXME: If we need to relocate a system DLL (base > 2GB) we should
     *        really make sure that the *new* base address is also > 2GB.
     *        Some DLLs really check the MSB of the module handle :-/
     */
    if ((nt->OptionalHeader.ImageBase & 0x80000000) && !((DWORD)base & 0x80000000))
        ERR( "Forced to relocate system DLL (base > 2GB). This is not good.\n" );

    for ( ; ((char *)rel < base + dir->VirtualAddress + dir->Size) && rel->VirtualAddress;
          rel = (IMAGE_BASE_RELOCATION*)((char*)rel + rel->SizeOfBlock))
    {
        char *page = base + rel->VirtualAddress;
        int i, count = (rel->SizeOfBlock - 8) / sizeof(rel->TypeOffset);

        if (!count) continue;

        /* sanity checks */
        if ((char *)rel + rel->SizeOfBlock > base + dir->VirtualAddress + dir->Size ||
            page > base + nt->OptionalHeader.SizeOfImage)
        {
            ERR_(module)("invalid relocation %p,%lx,%ld at %p,%lx,%lx\n",
                         rel, rel->VirtualAddress, rel->SizeOfBlock,
                         base, dir->VirtualAddress, dir->Size );
            return 0;
        }

        TRACE_(module)("%ld relocations for page %lx\n", rel->SizeOfBlock, rel->VirtualAddress);

        /* patching in reverse order */
        for (i = 0 ; i < count; i++)
        {
            int offset = rel->TypeOffset[i] & 0xFFF;
            int type = rel->TypeOffset[i] >> 12;
            switch(type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                break;
            case IMAGE_REL_BASED_HIGH:
                *(short*)(page+offset) += HIWORD(delta);
                break;
            case IMAGE_REL_BASED_LOW:
                *(short*)(page+offset) += LOWORD(delta);
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                *(int*)(page+offset) += delta;
                /* FIXME: if this is an exported address, fire up enhanced logic */
                break;
            default:
                FIXME_(module)("Unknown/unsupported fixup type %d.\n", type);
                break;
            }
        }
    }
    return 1;
}


/**********************************************************************
 *			PE_LoadImage
 * Load one PE format DLL/EXE into memory
 * 
 * Unluckily we can't just mmap the sections where we want them, for 
 * (at least) Linux does only support offsets which are page-aligned.
 *
 * BUT we have to map the whole image anyway, for Win32 programs sometimes
 * want to access them. (HMODULE32 point to the start of it)
 */
HMODULE PE_LoadImage( HANDLE hFile, LPCSTR filename, DWORD flags )
{
    IMAGE_NT_HEADERS *nt;
    HMODULE hModule;
    HANDLE mapping;
    void *base;

    TRACE_(module)( "loading %s\n", filename );

    mapping = CreateFileMappingA( hFile, NULL, SEC_IMAGE, 0, 0, NULL );
    base = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!base) return 0;

    hModule = (HMODULE)base;
    if (flags & LOAD_LIBRARY_AS_DATAFILE) return hModule;  /* nothing else to do */

    /* perform base relocation, if necessary */

    nt = PE_HEADER( hModule );
    if (hModule != nt->OptionalHeader.ImageBase)
    {
        if (!do_relocations( base, nt, filename ))
        {
            UnmapViewOfFile( base );
            SetLastError( ERROR_BAD_EXE_FORMAT );
            return 0;
        }
    }

    /* virus check */

    if (nt->OptionalHeader.AddressOfEntryPoint)
    {
        int i;
        IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER*)((char*)&nt->OptionalHeader +
                                                            nt->FileHeader.SizeOfOptionalHeader);
        for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        {
            if (nt->OptionalHeader.AddressOfEntryPoint < sec->VirtualAddress)
                continue;
            if (nt->OptionalHeader.AddressOfEntryPoint < sec->VirtualAddress+sec->SizeOfRawData)
                break;
        }
        if (i == nt->FileHeader.NumberOfSections)
            MESSAGE("VIRUS WARNING: PE module has an invalid entrypoint (0x%08lx) "
                    "outside all sections (possibly infected by Tchernobyl/SpaceFiller virus)!\n",
                    nt->OptionalHeader.AddressOfEntryPoint );
    }

    return hModule;
}

/**********************************************************************
 *                 PE_CreateModule
 *
 * Create WINE_MODREF structure for loaded HMODULE32, link it into
 * process modref_list, and fixup all imports.
 *
 * Note: hModule must point to a correctly allocated PE image,
 *       with base relocations applied; the 16-bit dummy module
 *       associated to hModule must already exist.
 *
 * Note: This routine must always be called in the context of the
 *       process that is to own the module to be created.
 *
 * Note: Assumes that the process critical section is held
 */
WINE_MODREF *PE_CreateModule( HMODULE hModule, LPCSTR filename, DWORD flags,
                              HFILE hFile, BOOL builtin )
{
    DWORD load_addr = (DWORD)hModule;  /* for RVA */
    IMAGE_NT_HEADERS *nt = PE_HEADER(hModule);
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_EXPORT_DIRECTORY *pe_export = NULL;
    WINE_MODREF *wm;
    HMODULE16 hModule16;

    /* Retrieve DataDirectory entries */

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_EXPORT;
    if (dir->Size)
        pe_export = (PIMAGE_EXPORT_DIRECTORY)RVA(dir->VirtualAddress);

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_EXCEPTION;
    if (dir->Size) FIXME("Exception directory ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_SECURITY;
    if (dir->Size) FIXME("Security directory ignored\n" );

    /* IMAGE_DIRECTORY_ENTRY_BASERELOC handled in PE_LoadImage */
    /* IMAGE_DIRECTORY_ENTRY_DEBUG handled by debugger */

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_GLOBALPTR;
    if (dir->Size) FIXME("Global Pointer (MIPS) ignored\n" );

    /* IMAGE_DIRECTORY_ENTRY_TLS handled in PE_TlsInit */

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG;
    if (dir->Size) FIXME("Load Configuration directory ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT;
    if (dir->Size) TRACE("Bound Import directory ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_IAT;
    if (dir->Size) TRACE("Import Address Table directory ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT;
    if (dir->Size)
    {
		TRACE("Delayed import, stub calls LoadLibrary\n" );
		/*
		 * Nothing to do here.
		 */

#ifdef ImgDelayDescr
		/*
		 * This code is useful to observe what the heck is going on.
		 */
		{
		ImgDelayDescr *pe_delay = NULL;
        pe_delay = (PImgDelayDescr)RVA(dir->VirtualAddress);
        TRACE_(delayhlp)("pe_delay->grAttrs = %08x\n", pe_delay->grAttrs);
        TRACE_(delayhlp)("pe_delay->szName = %s\n", pe_delay->szName);
        TRACE_(delayhlp)("pe_delay->phmod = %08x\n", pe_delay->phmod);
        TRACE_(delayhlp)("pe_delay->pIAT = %08x\n", pe_delay->pIAT);
        TRACE_(delayhlp)("pe_delay->pINT = %08x\n", pe_delay->pINT);
        TRACE_(delayhlp)("pe_delay->pBoundIAT = %08x\n", pe_delay->pBoundIAT);
        TRACE_(delayhlp)("pe_delay->pUnloadIAT = %08x\n", pe_delay->pUnloadIAT);
        TRACE_(delayhlp)("pe_delay->dwTimeStamp = %08x\n", pe_delay->dwTimeStamp);
        }
#endif /* ImgDelayDescr */
	}

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR;
    if (dir->Size) FIXME("Unknown directory 14 ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+15;
    if (dir->Size) FIXME("Unknown directory 15 ignored\n" );

    /* Create 16-bit dummy module */

    if ((hModule16 = MODULE_CreateDummyModule( filename, hModule )) < 32)
    {
        SetLastError( (DWORD)hModule16 );	/* This should give the correct error */
        return NULL;
    }

    /* Allocate and fill WINE_MODREF */

    if (!(wm = MODULE_AllocModRef( hModule, filename )))
    {
        FreeLibrary16( hModule16 );
        return NULL;
    }

    if ( builtin ) 
    {
        NE_MODULE *pModule = (NE_MODULE *)GlobalLock16( hModule16 );
        pModule->flags |= NE_FFLAGS_BUILTIN;
        wm->flags |= WINE_MODREF_INTERNAL;
    }

    if ( flags & DONT_RESOLVE_DLL_REFERENCES )
        wm->flags |= WINE_MODREF_DONT_RESOLVE_REFS;

    wm->find_export = PE_FindExportedFunction;

    /* Dump Exports */

    if ( pe_export )
        dump_exports( hModule );

    /* The exe_modref must be in place, before implicit linked DLLs are loaded 
       by fixup_imports, otherwhise GetModuleFileName will not work and modules 
       in the executables directory can not be found */

    if (!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
      if ( PROCESS_Current()->exe_modref )
	FIXME( "Trying to load second .EXE file: %s\n", filename );
      else  
      {
	PROCESS_Current()->exe_modref = wm;
        PROCESS_Current()->module = wm->module;
      }
    }

    /* Fixup Imports */

    if (!(wm->flags & WINE_MODREF_DONT_RESOLVE_REFS) && fixup_imports( wm ))
    {
        /* remove entry from modref chain */

        if ( !wm->prev )
            PROCESS_Current()->modref_list = wm->next;
        else
            wm->prev->next = wm->next;

        if ( wm->next ) wm->next->prev = wm->prev;
        wm->next = wm->prev = NULL;

        /* FIXME: there are several more dangling references
         * left. Including dlls loaded by this dll before the
         * failed one. Unrolling is rather difficult with the
         * current structure and we can leave it them lying
         * around with no problems, so we don't care.
         * As these might reference our wm, we don't free it.
         */
         return NULL;
    }

    if (pe_export)
        SNOOP_RegisterDLL( hModule, wm->modname, pe_export->NumberOfFunctions );

    /* Send DLL load event */
    /* we don't need to send a dll event for the main exe */

    if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
    {
        SERVER_START_REQ
        {
            struct load_dll_request *req = server_alloc_req( sizeof(*req), 0 );
            req->handle     = hFile;
            req->base       = (void *)hModule;
            req->dbg_offset = nt->FileHeader.PointerToSymbolTable;
            req->dbg_size   = nt->FileHeader.NumberOfSymbols;
            req->name       = &wm->filename;
            server_call_noerr( REQ_LOAD_DLL );
        }
        SERVER_END_REQ;
    }

    return wm;
}

/******************************************************************************
 * The PE Library Loader frontend. 
 * FIXME: handle the flags.
 */
WINE_MODREF *PE_LoadLibraryExA (LPCSTR name, DWORD flags)
{
	HMODULE		hModule32;
	WINE_MODREF	*wm;
	HANDLE		hFile;
       
	hFile = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, -1 );
	if ( hFile == INVALID_HANDLE_VALUE ) return NULL;
	
	/* Load PE module */
	hModule32 = PE_LoadImage( hFile, name, flags );
	if (!hModule32)
	{
                CloseHandle( hFile );
		return NULL;
	}

	/* Create 32-bit MODREF */
	if ( !(wm = PE_CreateModule( hModule32, name, flags, -1, FALSE )) )
	{
		ERR( "can't load %s\n", name );
                CloseHandle( hFile );
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

        CloseHandle( hFile );
	return wm;
}


/* Called if the library is loaded or freed.
 * NOTE: if a thread attaches a DLL, the current thread will only do
 * DLL_PROCESS_ATTACH. Only new created threads do DLL_THREAD_ATTACH
 * (SDK)
 */
typedef DWORD CALLBACK(*DLLENTRYPROC)(HMODULE,DWORD,LPVOID);

BOOL PE_InitDLL( HMODULE module, DWORD type, LPVOID lpReserved )
{
    BOOL retv = TRUE;
    IMAGE_NT_HEADERS *nt = PE_HEADER(module);

    /* Is this a library? And has it got an entrypoint? */
    if ((nt->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        (nt->OptionalHeader.AddressOfEntryPoint))
    {
        DLLENTRYPROC entry = (void*)((char*)module + nt->OptionalHeader.AddressOfEntryPoint);
        TRACE_(relay)("CallTo32(entryproc=%p,module=%08x,type=%ld,res=%p)\n",
                       entry, module, type, lpReserved );

        retv = entry( module, type, lpReserved );
    }

    return retv;
}

/************************************************************************
 *	PE_InitTls			(internal)
 *
 * If included, initialises the thread local storages of modules.
 * Pointers in those structs are not RVAs but real pointers which have been
 * relocated by do_relocations() already.
 */
static LPVOID
_fixup_address(PIMAGE_OPTIONAL_HEADER opt,int delta,LPVOID addr) {
	if (	((DWORD)addr>opt->ImageBase) &&
		((DWORD)addr<opt->ImageBase+opt->SizeOfImage)
	)
		/* the address has not been relocated! */
		return (LPVOID)(((DWORD)addr)+delta);
	else
		/* the address has been relocated already */
		return addr;
}
void PE_InitTls( void )
{
	WINE_MODREF		*wm;
	IMAGE_NT_HEADERS	*peh;
	DWORD			size,datasize;
	LPVOID			mem;
	PIMAGE_TLS_DIRECTORY	pdir;
        int delta;
	
	for (wm = PROCESS_Current()->modref_list;wm;wm=wm->next) {
		peh = PE_HEADER(wm->module);
		delta = wm->module - peh->OptionalHeader.ImageBase;
		if (!peh->OptionalHeader.DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress)
			continue;
		pdir = (LPVOID)(wm->module + peh->OptionalHeader.
			DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress);
		
		
		if ( wm->tlsindex == -1 ) {
			LPDWORD xaddr;
			wm->tlsindex = TlsAlloc();
			xaddr = _fixup_address(&(peh->OptionalHeader),delta,
					pdir->AddressOfIndex
			);
			*xaddr=wm->tlsindex;
		}
		datasize= pdir->EndAddressOfRawData-pdir->StartAddressOfRawData;
		size	= datasize + pdir->SizeOfZeroFill;
		mem=VirtualAlloc(0,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		memcpy(mem,_fixup_address(&(peh->OptionalHeader),delta,(LPVOID)pdir->StartAddressOfRawData),datasize);
		if (pdir->AddressOfCallBacks) {
		     PIMAGE_TLS_CALLBACK *cbs; 

		     cbs = _fixup_address(&(peh->OptionalHeader),delta,pdir->AddressOfCallBacks);
		     if (*cbs)
		       FIXME("TLS Callbacks aren't going to be called\n");
		}

		TlsSetValue( wm->tlsindex, mem );
	}
}


/*
 *  Copyright	1994	Eric Youndale & Erik Bos
 *  Copyright	1995	Martin von Löwis
 *  Copyright   1996-98 Marcus Meissner
 *
 *	based on Eric Youndale's pe-test and:
 *	ftp.microsoft.com:/developr/MSDN/OctCD/PEFILE.ZIP
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
/* Notes:
 * Before you start changing something in this file be aware of the following:
 *
 * - There are several functions called recursively. In a very subtle and
 *   obscure way. DLLs can reference each other recursively etc.
 * - If you want to enhance, speed up or clean up something in here, think
 *   twice WHY it is implemented in that strange way. There is usually a reason.
 *   Though sometimes it might just be lazyness ;)
 * - In PE_MapImage, right before PE_fixup_imports() all external and internal
 *   state MUST be correct since this function can be called with the SAME image
 *   AGAIN. (Thats recursion for you.) That means MODREF.module and
 *   NE_MODULE.module32.
 */

#include "config.h"

#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <string.h>
#include "wine/winbase16.h"
#include "winerror.h"
#include "snoop.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(relay);


/* convert PE image VirtualAddress to Real Address */
inline static void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

#define AdjustPtr(ptr,delta) ((char *)(ptr) + (delta))

void dump_exports( HMODULE hModule )
{
  char		*Module;
  int		i, j;
  WORD		*ordinal;
  DWORD		*function,*functions;
  DWORD *name;
  IMAGE_EXPORT_DIRECTORY *pe_exports;
  DWORD rva_start, size;

  pe_exports = RtlImageDirectoryEntryToData( hModule, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
  rva_start = (char *)pe_exports - (char *)hModule;

  Module = get_rva(hModule, pe_exports->Name);
  DPRINTF("*******EXPORT DATA*******\n");
  DPRINTF("Module name is %s, %ld functions, %ld names\n",
          Module, pe_exports->NumberOfFunctions, pe_exports->NumberOfNames);

  ordinal = get_rva(hModule, pe_exports->AddressOfNameOrdinals);
  functions = function = get_rva(hModule, pe_exports->AddressOfFunctions);
  name = get_rva(hModule, pe_exports->AddressOfNames);

  DPRINTF(" Ord    RVA     Addr   Name\n" );
  for (i=0;i<pe_exports->NumberOfFunctions;i++, function++)
  {
      if (!*function) continue;  /* No such function */
      DPRINTF( "%4ld %08lx %p", i + pe_exports->Base, *function, get_rva(hModule, *function) );
      /* Check if we have a name for it */
      for (j = 0; j < pe_exports->NumberOfNames; j++)
          if (ordinal[j] == i)
          {
              DPRINTF( "  %s", (char*)get_rva(hModule, name[j]) );
              break;
          }
      if ((*function >= rva_start) && (*function <= rva_start + size))
	  DPRINTF(" (forwarded -> %s)", (char *)get_rva(hModule, *function));
      DPRINTF("\n");
  }
}

/* Look up the specified function or ordinal in the export list:
 * If it is a string:
 * 	- look up the name in the name list.
 *	- look up the ordinal with that index.
 *	- use the ordinal as offset into the functionlist
 * If it is an ordinal:
 *	- use ordinal-pe_export->Base as offset into the function list
 */
static FARPROC PE_FindExportedFunction(
	WINE_MODREF *wm,	/* [in] WINE modreference */
	LPCSTR funcName,	/* [in] function name */
        int hint,
        BOOL snoop )
{
	WORD				* ordinals;
	DWORD				* function;
	int				i, ordinal;
	DWORD				rva_start, addr;
	char				* forward;
        DWORD *name;
        char *ename = NULL;
        FARPROC proc;
        IMAGE_EXPORT_DIRECTORY *exports;
        DWORD exp_size;

        if (!(exports = RtlImageDirectoryEntryToData( wm->module, TRUE,
                                                      IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
            return NULL;

        if (HIWORD(funcName)) TRACE("(%s)\n",funcName);
        else TRACE("(%d)\n",LOWORD(funcName));

	ordinals= get_rva(wm->module, exports->AddressOfNameOrdinals);
	function= get_rva(wm->module, exports->AddressOfFunctions);
	name    = get_rva(wm->module, exports->AddressOfNames);
	forward = NULL;
        rva_start = (char *)exports - (char *)wm->module;

	if (HIWORD(funcName))
        {
            int min = 0, max = exports->NumberOfNames - 1;

            /* first check the hint */
            if (hint >= 0 && hint <= max)
            {
                ename = get_rva(wm->module, name[hint]);
                if (!strcmp( ename, funcName ))
                {
                    ordinal = ordinals[hint];
                    goto found;
                }
            }

            /* then do a binary search */
            while (min <= max)
            {
                int res, pos = (min + max) / 2;
                ename = get_rva(wm->module, name[pos]);
                if (!(res = strcmp( ename, funcName )))
                {
                    ordinal = ordinals[pos];
                    goto found;
                }
                if (res > 0) max = pos - 1;
                else min = pos + 1;
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
                        ename = get_rva(wm->module, name[i]);
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

        proc = get_rva(wm->module, addr);
        if (((char *)proc < (char *)exports) || ((char *)proc >= (char *)exports + exp_size))
        {
            if (snoop)
            {
                if (!ename) ename = "@";
                proc = SNOOP_GetProcAddress(wm->module,ename,ordinal,proc);
            }
            return proc;
        }
        else  /* forward entry point */
        {
                WINE_MODREF *wm_fw;
                char *forward = (char *)proc;
		char module[256];
		char *end = strchr(forward, '.');

		if (!end) return NULL;
                if (end - forward >= sizeof(module)) return NULL;
                memcpy( module, forward, end - forward );
		module[end-forward] = 0;
                if (!(wm_fw = MODULE_FindModule( module )))
                {
                    ERR("module not found for forward '%s' used by '%s'\n", forward, wm->modname );
                    return NULL;
                }
		if (!(proc = MODULE_GetProcAddress( wm_fw->module, end + 1, -1, snoop )))
                    ERR("function not found for forward '%s' used by '%s'. If you are using builtin '%s', try using the native one instead.\n", forward, wm->modname, wm->modname );
		return proc;
	}
}

/****************************************************************
 * 	PE_fixup_imports
 */
DWORD PE_fixup_imports( WINE_MODREF *wm )
{
    int i,characteristics_detection=1;
    IMAGE_IMPORT_DESCRIPTOR *imports, *pe_imp;
    DWORD size;

    imports = RtlImageDirectoryEntryToData( wm->module, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size );

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
 	char			*name = get_rva(wm->module, pe_imp->Name);

	if (characteristics_detection && !pe_imp->u.Characteristics)
		break;

	wmImp = MODULE_LoadLibraryExA( name, 0, 0 );
	if (!wmImp) {
            if(GetLastError() == ERROR_FILE_NOT_FOUND)
                ERR_(module)("Module (file) %s (which is needed by %s) not found\n", name, wm->filename);
            else
                ERR_(module)("Loading module (file) %s (which is needed by %s) failed (error %ld).\n",
                        name, wm->filename, GetLastError());
	    return 1;
	}
        wm->deps[i++] = wmImp;

	/* FIXME: forwarder entries ... */

	if (pe_imp->u.OriginalFirstThunk != 0) { /* original MS style */
	    TRACE("Microsoft style imports used\n");
	    import_list = get_rva(wm->module, (DWORD)pe_imp->u.OriginalFirstThunk);
	    thunk_list = get_rva(wm->module, (DWORD)pe_imp->FirstThunk);

	    while (import_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal)) {
		    int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

		    TRACE("--- Ordinal %s,%d\n", name, ordinal);
		    thunk_list->u1.Function=(PDWORD)MODULE_GetProcAddress(
                        wmImp->module, (LPCSTR)ordinal, -1, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR("No implementation for %s.%d imported from %s, setting to 0xdeadbeef\n",
				name, ordinal, wm->filename );
                        thunk_list->u1.Function = (PDWORD)0xdeadbeef;
		    }
		} else {		/* import by name */
		    pe_name = get_rva(wm->module, (DWORD)import_list->u1.AddressOfData);
		    TRACE("--- %s %s.%d\n", pe_name->Name, name, pe_name->Hint);
		    thunk_list->u1.Function=(PDWORD)MODULE_GetProcAddress(
                        wmImp->module, pe_name->Name, pe_name->Hint, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR("No implementation for %s.%d(%s) imported from %s, setting to 0xdeadbeef\n",
				name,pe_name->Hint,pe_name->Name,wm->filename);
                        thunk_list->u1.Function = (PDWORD)0xdeadbeef;
		    }
		}
		import_list++;
		thunk_list++;
	    }
	} else {	/* Borland style */
	    TRACE("Borland style imports used\n");
	    thunk_list = get_rva(wm->module, (DWORD)pe_imp->FirstThunk);
	    while (thunk_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(thunk_list->u1.Ordinal)) {
		    /* not sure about this branch, but it seems to work */
		    int ordinal = IMAGE_ORDINAL(thunk_list->u1.Ordinal);

		    TRACE("--- Ordinal %s.%d\n",name,ordinal);
		    thunk_list->u1.Function=(PDWORD)MODULE_GetProcAddress(
                        wmImp->module, (LPCSTR) ordinal, -1, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR("No implementation for %s.%d imported from %s, setting to 0xdeadbeef\n",
				name,ordinal, wm->filename);
                        thunk_list->u1.Function = (PDWORD)0xdeadbeef;
		    }
		} else {
		    pe_name=get_rva(wm->module, (DWORD)thunk_list->u1.AddressOfData);
		    TRACE("--- %s %s.%d\n",
		   		  pe_name->Name,name,pe_name->Hint);
		    thunk_list->u1.Function=(PDWORD)MODULE_GetProcAddress(
                        wmImp->module, pe_name->Name, pe_name->Hint, TRUE
		    );
		    if (!thunk_list->u1.Function) {
		    	ERR("No implementation for %s.%d(%s) imported from %s, setting to 0xdeadbeef\n",
				name, pe_name->Hint, pe_name->Name, wm->filename);
                        thunk_list->u1.Function = (PDWORD)0xdeadbeef;
		    }
		}
		thunk_list++;
	    }
	}
    }
    return 0;
}

/**********************************************************************
 *			PE_LoadImage
 * Load one PE format DLL/EXE into memory
 *
 * Unluckily we can't just mmap the sections where we want them, for
 * (at least) Linux does only support offsets which are page-aligned.
 *
 * BUT we have to map the whole image anyway, for Win32 programs sometimes
 * want to access them. (HMODULE points to the start of it)
 */
HMODULE PE_LoadImage( HANDLE hFile, LPCSTR filename, DWORD flags )
{
    IMAGE_NT_HEADERS *nt;
    HMODULE hModule;
    HANDLE mapping;
    void *base;

    TRACE_(module)( "loading %s\n", filename );

    mapping = CreateFileMappingA( hFile, NULL, SEC_IMAGE, 0, 0, NULL );
    if (!mapping) return 0;
    base = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!base) return 0;

    /* virus check */

    hModule = (HMODULE)base;
    nt = RtlImageNtHeader( hModule );

    if (nt->OptionalHeader.AddressOfEntryPoint)
    {
        if (!RtlImageRvaToSection( nt, hModule, nt->OptionalHeader.AddressOfEntryPoint ))
            MESSAGE("VIRUS WARNING: PE module has an invalid entrypoint (0x%08lx) "
                    "outside all sections (possibly infected by Tchernobyl/SpaceFiller virus)!\n",
                    nt->OptionalHeader.AddressOfEntryPoint );
    }

    return hModule;
}

/**********************************************************************
 *                 PE_CreateModule
 *
 * Create WINE_MODREF structure for loaded HMODULE, link it into
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
                              HANDLE hFile, BOOL builtin )
{
    IMAGE_NT_HEADERS *nt;
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_EXPORT_DIRECTORY *pe_export = NULL;
    WINE_MODREF *wm;
    HMODULE16 hModule16;

    /* Retrieve DataDirectory entries */

    nt = RtlImageNtHeader(hModule);
    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_EXPORT;
    if (dir->Size) pe_export = get_rva(hModule, dir->VirtualAddress);

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
            pe_delay = get_rva(hModule, dir->VirtualAddress);
            TRACE("pe_delay->grAttrs = %08x\n", pe_delay->grAttrs);
            TRACE("pe_delay->szName = %s\n", pe_delay->szName);
            TRACE("pe_delay->phmod = %08x\n", pe_delay->phmod);
            TRACE("pe_delay->pIAT = %08x\n", pe_delay->pIAT);
            TRACE("pe_delay->pINT = %08x\n", pe_delay->pINT);
            TRACE("pe_delay->pBoundIAT = %08x\n", pe_delay->pBoundIAT);
            TRACE("pe_delay->pUnloadIAT = %08x\n", pe_delay->pUnloadIAT);
            TRACE("pe_delay->dwTimeStamp = %08x\n", pe_delay->dwTimeStamp);
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
    wm->hDummyMod = hModule16;

    if ( builtin )
    {
        NE_MODULE *pModule = (NE_MODULE *)GlobalLock16( hModule16 );
        pModule->flags |= NE_FFLAGS_BUILTIN;
        wm->flags |= WINE_MODREF_INTERNAL;
    }
    else if ( flags & DONT_RESOLVE_DLL_REFERENCES )
        wm->flags |= WINE_MODREF_DONT_RESOLVE_REFS;

    wm->find_export = PE_FindExportedFunction;

    /* Dump Exports */

    if (pe_export && TRACE_ON(win32))
        dump_exports( hModule );

    /* Fixup Imports */

    if (!(wm->flags & WINE_MODREF_DONT_RESOLVE_REFS) &&
        PE_fixup_imports( wm ))
    {
        /* remove entry from modref chain */

        if ( !wm->prev )
            MODULE_modref_list = wm->next;
        else
            wm->prev->next = wm->next;

        if ( wm->next ) wm->next->prev = wm->prev;
        wm->next = wm->prev = NULL;

        /* FIXME: there are several more dangling references
         * left. Including dlls loaded by this dll before the
         * failed one. Unrolling is rather difficult with the
         * current structure and we can leave them lying
         * around with no problems, so we don't care.
         * As these might reference our wm, we don't free it.
         */
         return NULL;
    }

    if (!builtin && pe_export)
        SNOOP_RegisterDLL( hModule, wm->modname, pe_export->Base, pe_export->NumberOfFunctions );

    /* Send DLL load event */
    /* we don't need to send a dll event for the main exe */

    if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
    {
        if (hFile)
        {
            UINT drive_type = GetDriveTypeA( wm->short_filename );
            /* don't keep the file handle open on removable media */
            if (drive_type == DRIVE_REMOVABLE || drive_type == DRIVE_CDROM) hFile = 0;
        }
        SERVER_START_REQ( load_dll )
        {
            req->handle     = hFile;
            req->base       = (void *)hModule;
            req->size       = nt->OptionalHeader.SizeOfImage;
            req->dbg_offset = nt->FileHeader.PointerToSymbolTable;
            req->dbg_size   = nt->FileHeader.NumberOfSymbols;
            req->name       = &wm->filename;
            wine_server_add_data( req, wm->filename, strlen(wm->filename) );
            wine_server_call( req );
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
                             NULL, OPEN_EXISTING, 0, 0 );
	if ( hFile == INVALID_HANDLE_VALUE ) return NULL;

	/* Load PE module */
	hModule32 = PE_LoadImage( hFile, name, flags );
	if (!hModule32)
	{
                CloseHandle( hFile );
		return NULL;
	}

	/* Create 32-bit MODREF */
	if ( !(wm = PE_CreateModule( hModule32, name, flags, hFile, FALSE )) )
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
 * DLL_PROCESS_ATTACH. Only newly created threads do DLL_THREAD_ATTACH
 * (SDK)
 */
typedef DWORD (CALLBACK *DLLENTRYPROC)(HMODULE,DWORD,LPVOID);

BOOL PE_InitDLL( HMODULE module, DWORD type, LPVOID lpReserved )
{
    BOOL retv = TRUE;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader(module);

    /* Is this a library? And has it got an entrypoint? */
    if (nt && (nt->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        (nt->OptionalHeader.AddressOfEntryPoint))
    {
        DLLENTRYPROC entry = (void*)((char*)module + nt->OptionalHeader.AddressOfEntryPoint);
        if (TRACE_ON(relay))
            DPRINTF("%08lx:Call PE DLL (proc=%p,module=%p,type=%ld,res=%p)\n",
                    GetCurrentThreadId(), entry, module, type, lpReserved );
        retv = entry( module, type, lpReserved );
        if (TRACE_ON(relay))
            DPRINTF("%08lx:Ret  PE DLL (proc=%p,module=%p,type=%ld,res=%p) retval=%x\n",
                    GetCurrentThreadId(), entry, module, type, lpReserved, retv );
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
	DWORD			size,datasize,dirsize;
	LPVOID			mem;
	PIMAGE_TLS_DIRECTORY	pdir;
        int delta;

	for (wm = MODULE_modref_list;wm;wm=wm->next) {
                peh = RtlImageNtHeader(wm->module);
                pdir = RtlImageDirectoryEntryToData( wm->module, TRUE,
                                                     IMAGE_DIRECTORY_ENTRY_TLS, &dirsize );
                if (!pdir) continue;
                delta = (char *)wm->module - (char *)peh->OptionalHeader.ImageBase;

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

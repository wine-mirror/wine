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
#include <stdarg.h>
#include <string.h>
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "snoop.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

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
    void *base = NULL;
    OBJECT_ATTRIBUTES attr;
    LARGE_INTEGER lg_int;
    DWORD len = 0;
    NTSTATUS nts;

    TRACE_(module)( "loading %s\n", filename );

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    
    lg_int.QuadPart = 0;

    if (NtCreateSection( &mapping, 
                         STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                         &attr, &lg_int, 0, SEC_IMAGE, hFile ) != STATUS_SUCCESS)
        return 0;

    nts = NtMapViewOfSection( mapping, GetCurrentProcess(),
                              &base, 0, 0, &lg_int, &len, ViewShare, 0,
                              PAGE_READONLY );
    
    NtClose( mapping );
    if (nts != STATUS_SUCCESS) return 0;

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

    /* Allocate and fill WINE_MODREF */

    if (!(wm = MODULE_AllocModRef( hModule, filename ))) return NULL;

    if ( builtin )
        wm->ldr.Flags |= LDR_WINE_INTERNAL;
    else if ( flags & DONT_RESOLVE_DLL_REFERENCES )
        wm->ldr.Flags |= LDR_DONT_RESOLVE_REFS;

    /* Dump Exports */

    if (pe_export && TRACE_ON(win32))
        dump_exports( hModule );

    /* Fixup Imports */

    if (!(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS) &&
        PE_fixup_imports( wm ))
    {
        /* the module has only be inserted in the load & memory order lists */
        RemoveEntryList(&wm->ldr.InLoadOrderModuleList);
        RemoveEntryList(&wm->ldr.InMemoryOrderModuleList);

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
NTSTATUS PE_LoadLibraryExA (LPCSTR name, DWORD flags, WINE_MODREF** pwm)
{
	HMODULE		hModule32;
	HANDLE		hFile;

	hFile = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, 0 );
	if ( hFile == INVALID_HANDLE_VALUE ) 
        {
            /* keep it that way until we transform CreateFile into NtCreateFile */
            return (GetLastError() == ERROR_FILE_NOT_FOUND) ? 
                STATUS_NO_SUCH_FILE : STATUS_INTERNAL_ERROR;
        }

	/* Load PE module */
	hModule32 = PE_LoadImage( hFile, name, flags );
	if (!hModule32)
	{
                CloseHandle( hFile );
		return STATUS_INTERNAL_ERROR;
	}

	/* Create 32-bit MODREF */
	if ( !(*pwm = PE_CreateModule( hModule32, name, flags, hFile, FALSE )) )
	{
		ERR( "can't load %s\n", name );
                CloseHandle( hFile );
		return STATUS_NO_MEMORY; /* FIXME */
	}

        CloseHandle( hFile );
	return STATUS_SUCCESS;
}

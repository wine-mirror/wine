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
 * - No, you (usually) cannot use Linux mmap() to mmap() the images directly.
 *
 *   The problem is, that there is not direct 1:1 mapping from a diskimage and
 *   a memoryimage. The headers at the start are mapped linear, but the sections
 *   are not. For x86 the sections are 512 byte aligned in file and 4096 byte
 *   aligned in memory. Linux likes them 4096 byte aligned in memory (due to
 *   x86 pagesize, this cannot be fixed without a rather large kernel rewrite)
 *   and 'blocksize' file-aligned (offsets). Since we have 512/1024/2048 (CDROM)
 *   and other byte blocksizes, we can't do this. However, this could be less
 *   difficult to support... (See mm/filemap.c).
 */

#include "config.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "callback.h"
#include "file.h"
#include "heap.h"
#include "neexe.h"
#include "peexe.h"
#include "process.h"
#include "thread.h"
#include "pe_image.h"
#include "module.h"
#include "global.h"
#include "task.h"
#include "snoop.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32)
DECLARE_DEBUG_CHANNEL(delayhlp)
DECLARE_DEBUG_CHANNEL(fixup)
DECLARE_DEBUG_CHANNEL(module)
DECLARE_DEBUG_CHANNEL(relay)
DECLARE_DEBUG_CHANNEL(segment)


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
FARPROC PE_FindExportedFunction( 
	WINE_MODREF *wm,	/* [in] WINE modreference */
	LPCSTR funcName,	/* [in] function name */
        BOOL snoop )
{
	u_short				* ordinals;
	u_long				* function;
	u_char				** name, *ename = NULL;
	int				i, ordinal;
	PE_MODREF			*pem = &(wm->binfmt.pe);
	IMAGE_EXPORT_DIRECTORY 		*exports = pem->pe_export;
	unsigned int			load_addr = wm->module;
	u_long				rva_start, rva_end, addr;
	char				* forward;

	if (HIWORD(funcName))
		TRACE("(%s)\n",funcName);
	else
		TRACE("(%d)\n",(int)funcName);
	if (!exports) {
		/* Not a fatal problem, some apps do
		 * GetProcAddress(0,"RegisterPenApp") which triggers this
		 * case.
		 */
		WARN("Module %08x(%s)/MODREF %p doesn't have a exports table.\n",wm->module,wm->modname,pem);
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
		return MODULE_GetProcAddress( wm->module, end + 1, snoop );
	}
}

DWORD fixup_imports( WINE_MODREF *wm )
{
    IMAGE_IMPORT_DESCRIPTOR	*pe_imp;
    PE_MODREF			*pem;
    unsigned int load_addr	= wm->module;
    int				i,characteristics_detection=1;
    char			*modname;
    
    assert(wm->type==MODULE32_PE);
    pem = &(wm->binfmt.pe);
    if (pem->pe_export)
    	modname = (char*) RVA(pem->pe_export->Name);
    else
        modname = "<unknown>";

    /* OK, now dump the import list */
    TRACE("Dumping imports list\n");

    /* first, count the number of imported non-internal modules */
    pe_imp = pem->pe_import;
    if (!pe_imp) return 0;

    /* We assume that we have at least one import with !0 characteristics and
     * detect broken imports with all characteristsics 0 (notably Borland) and
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
 
    for (i = 0, pe_imp = pem->pe_import; pe_imp->Name ; pe_imp++) {
    	WINE_MODREF		*wmImp;
	IMAGE_IMPORT_BY_NAME	*pe_name;
	PIMAGE_THUNK_DATA	import_list,thunk_list;
 	char			*name = (char *) RVA(pe_imp->Name);

	if (characteristics_detection && !pe_imp->u.Characteristics)
		break;

	wmImp = MODULE_LoadLibraryExA( name, 0, 0 );
	if (!wmImp) {
	    ERR_(module)("Module %s not found\n", name);
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

static int calc_vma_size( HMODULE hModule )
{
    int i,vma_size = 0;
    IMAGE_SECTION_HEADER *pe_seg = PE_SECTIONS(hModule);

    TRACE("Dump of segment table\n");
    TRACE("   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
    for (i = 0; i< PE_HEADER(hModule)->FileHeader.NumberOfSections; i++)
    {
        TRACE("%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
                      pe_seg->Name, 
                      pe_seg->Misc.VirtualSize,
                      pe_seg->VirtualAddress,
                      pe_seg->SizeOfRawData,
                      pe_seg->PointerToRawData,
                      pe_seg->PointerToRelocations,
                      pe_seg->PointerToLinenumbers,
                      pe_seg->NumberOfRelocations,
                      pe_seg->NumberOfLinenumbers,
                      pe_seg->Characteristics);
        vma_size=MAX(vma_size, pe_seg->VirtualAddress+pe_seg->SizeOfRawData);
        vma_size=MAX(vma_size, pe_seg->VirtualAddress+pe_seg->Misc.VirtualSize);
        pe_seg++;
    }
    return vma_size;
}

static void do_relocations( unsigned int load_addr, IMAGE_BASE_RELOCATION *r )
{
    int delta = load_addr - PE_HEADER(load_addr)->OptionalHeader.ImageBase;
    int	hdelta = (delta >> 16) & 0xFFFF;
    int	ldelta = delta & 0xFFFF;

	if(delta == 0)
		/* Nothing to do */
		return;
	while(r->VirtualAddress)
	{
		char *page = (char*) RVA(r->VirtualAddress);
		int count = (r->SizeOfBlock - 8)/2;
		int i;
		TRACE_(fixup)("%x relocations for page %lx\n",
			count, r->VirtualAddress);
		/* patching in reverse order */
		for(i=0;i<count;i++)
		{
			int offset = r->TypeOffset[i] & 0xFFF;
			int type = r->TypeOffset[i] >> 12;
			TRACE_(fixup)("patching %x type %x\n", offset, type);
			switch(type)
			{
			case IMAGE_REL_BASED_ABSOLUTE: break;
			case IMAGE_REL_BASED_HIGH:
				*(short*)(page+offset) += hdelta;
				break;
			case IMAGE_REL_BASED_LOW:
				*(short*)(page+offset) += ldelta;
				break;
			case IMAGE_REL_BASED_HIGHLOW:
				*(int*)(page+offset) += delta;
				/* FIXME: if this is an exported address, fire up enhanced logic */
				break;
			case IMAGE_REL_BASED_HIGHADJ:
				FIXME("Don't know what to do with IMAGE_REL_BASED_HIGHADJ\n");
				break;
			case IMAGE_REL_BASED_MIPS_JMPADDR:
				FIXME("Is this a MIPS machine ???\n");
				break;
			default:
				FIXME("Unknown fixup type\n");
				break;
			}
		}
		r = (IMAGE_BASE_RELOCATION*)((char*)r + r->SizeOfBlock);
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
 * want to access them. (HMODULE32 point to the start of it)
 */
HMODULE PE_LoadImage( HANDLE hFile, LPCSTR filename, WORD *version )
{
    HMODULE	hModule;
    HANDLE	mapping;

    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *pe_sec;
    IMAGE_DATA_DIRECTORY *dir;
    BY_HANDLE_FILE_INFORMATION bhfi;
    int	i, rawsize, lowest_va, lowest_fa, vma_size, file_size = 0;
    DWORD load_addr, aoep, reloc = 0;

    /* Retrieve file size */
    if ( GetFileInformationByHandle( hFile, &bhfi ) ) 
    	file_size = bhfi.nFileSizeLow; /* FIXME: 64 bit */

    /* Map the PE file somewhere */
    mapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY | SEC_COMMIT,
                                    0, 0, NULL );
    if (!mapping)
    {
        WARN("CreateFileMapping error %ld\n", GetLastError() );
        return 0;
    }
    hModule = (HMODULE)MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!hModule)
    {
        WARN("MapViewOfFile error %ld\n", GetLastError() );
        return 0;
    }
    nt = PE_HEADER( hModule );

    /* Check signature */
    if ( nt->Signature != IMAGE_NT_SIGNATURE )
    {
        WARN("image doesn't have PE signature, but 0x%08lx\n",
                    nt->Signature );
        goto error;
    }

    /* Check architecture */
    if ( nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 )
    {
        MESSAGE("Trying to load PE image for unsupported architecture (");
        switch (nt->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_UNKNOWN: MESSAGE("Unknown"); break;
        case IMAGE_FILE_MACHINE_I860:    MESSAGE("I860"); break;
        case IMAGE_FILE_MACHINE_R3000:   MESSAGE("R3000"); break;
        case IMAGE_FILE_MACHINE_R4000:   MESSAGE("R4000"); break;
        case IMAGE_FILE_MACHINE_R10000:  MESSAGE("R10000"); break;
        case IMAGE_FILE_MACHINE_ALPHA:   MESSAGE("Alpha"); break;
        case IMAGE_FILE_MACHINE_POWERPC: MESSAGE("PowerPC"); break;
        default: MESSAGE("Unknown-%04x", nt->FileHeader.Machine); break;
        }
        MESSAGE(")\n");
        goto error;
    }

    /* Find out how large this executeable should be */
    pe_sec = PE_SECTIONS( hModule );
    rawsize = 0; lowest_va = 0x10000; lowest_fa = 0x10000;
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++) 
    {
        if (lowest_va > pe_sec[i].VirtualAddress)
           lowest_va = pe_sec[i].VirtualAddress;
    	if (pe_sec[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	    continue;
    	if (pe_sec[i].PointerToRawData < lowest_fa)
            lowest_fa = pe_sec[i].PointerToRawData;
	if (pe_sec[i].PointerToRawData+pe_sec[i].SizeOfRawData > rawsize)
	    rawsize = pe_sec[i].PointerToRawData+pe_sec[i].SizeOfRawData;
    }
 
    /* Check file size */
    if ( file_size && file_size < rawsize )
    {
        ERR("PE module is too small (header: %d, filesize: %d), "
                    "probably truncated download?\n", 
                    rawsize, file_size );
        goto error;
    }

    /* Check entrypoint address */
    aoep = nt->OptionalHeader.AddressOfEntryPoint;
    if (aoep && (aoep < lowest_va))
        FIXME("VIRUS WARNING: '%s' has an invalid entrypoint (0x%08lx) "
                      "below the first virtual address (0x%08x) "
                      "(possibly infected by Tchernobyl/SpaceFiller virus)!\n",
                       filename, aoep, lowest_va );


    /* FIXME:  Hack!  While we don't really support shared sections yet,
     *         this checks for those special cases where the whole DLL
     *         consists only of shared sections and is mapped into the
     *         shared address space > 2GB.  In this case, we assume that
     *         the module got mapped at its base address. Thus we simply
     *         check whether the module has actually been mapped there
     *         and use it, if so.  This is needed to get Win95 USER32.DLL
     *         to work (until we support shared sections properly).
     */

    if ( nt->OptionalHeader.ImageBase & 0x80000000 )
    {
        HMODULE sharedMod = (HMODULE)nt->OptionalHeader.ImageBase; 
        IMAGE_NT_HEADERS *sharedNt = (PIMAGE_NT_HEADERS)
               ( (LPBYTE)sharedMod + ((LPBYTE)nt - (LPBYTE)hModule) );

        /* Well, this check is not really comprehensive, 
           but should be good enough for now ... */
        if (    !IsBadReadPtr( (LPBYTE)sharedMod, sizeof(IMAGE_DOS_HEADER) )
             && memcmp( (LPBYTE)sharedMod, (LPBYTE)hModule, sizeof(IMAGE_DOS_HEADER) ) == 0
             && !IsBadReadPtr( sharedNt, sizeof(IMAGE_NT_HEADERS) )
             && memcmp( sharedNt, nt, sizeof(IMAGE_NT_HEADERS) ) == 0 )
        {
            UnmapViewOfFile( (LPVOID)hModule );
            return sharedMod;
        }
    }


    /* Allocate memory for module */
    load_addr = nt->OptionalHeader.ImageBase;
    vma_size = calc_vma_size( hModule );

    load_addr = (DWORD)VirtualAlloc( (void*)load_addr, vma_size,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE );
    if (load_addr == 0) 
    {
        /* We need to perform base relocations */
	dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_BASERELOC;
        if (dir->Size)
            reloc = dir->VirtualAddress;
        else 
        {
            FIXME(
                   "FATAL: Need to relocate %s, but no relocation records present (%s). Try to run that file directly !\n",
                   filename,
                   (nt->FileHeader.Characteristics&IMAGE_FILE_RELOCS_STRIPPED)?
                   "stripped during link" : "unknown reason" );
            goto error;
        }

        /* FIXME: If we need to relocate a system DLL (base > 2GB) we should
         *        really make sure that the *new* base address is also > 2GB.
         *        Some DLLs really check the MSB of the module handle :-/
         */
        if ( nt->OptionalHeader.ImageBase & 0x80000000 )
            ERR( "Forced to relocate system DLL (base > 2GB). This is not good.\n" );

        load_addr = (DWORD)VirtualAlloc( NULL, vma_size,
					 MEM_RESERVE | MEM_COMMIT,
					 PAGE_EXECUTE_READWRITE );
	if (!load_addr) {
            FIXME_(win32)(
                   "FATAL: Couldn't load module %s (out of memory, %d needed)!\n", filename, vma_size);
            goto error;
	}
    }

    TRACE("Load addr is %lx (base %lx), range %x\n",
          load_addr, nt->OptionalHeader.ImageBase, vma_size );
    TRACE_(segment)("Loading %s at %lx, range %x\n",
                    filename, load_addr, vma_size );

    /* Store the NT header at the load addr */
    *(PIMAGE_DOS_HEADER)load_addr = *(PIMAGE_DOS_HEADER)hModule;
    *PE_HEADER( load_addr ) = *nt;
    memcpy( PE_SECTIONS(load_addr), PE_SECTIONS(hModule),
            sizeof(IMAGE_SECTION_HEADER) * nt->FileHeader.NumberOfSections );
#if 0
    /* Copies all stuff up to the first section. Including win32 viruses. */
    memcpy( load_addr, hModule, lowest_fa );
#endif

    /* Copy sections into module image */
    pe_sec = PE_SECTIONS( hModule );
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, pe_sec++)
    {
        /* memcpy only non-BSS segments */
        /* FIXME: this should be done by mmap(..MAP_PRIVATE|MAP_FIXED..)
         * but it is not possible for (at least) Linux needs
         * a page-aligned offset.
         */
        if(!(pe_sec->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
            memcpy((char*)RVA(pe_sec->VirtualAddress),
                   (char*)(hModule + pe_sec->PointerToRawData),
                   pe_sec->SizeOfRawData);
#if 0
        /* not needed, memory is zero */
        if(strcmp(pe_sec->Name, ".bss") == 0)
            memset((void *)RVA(pe_sec->VirtualAddress), 0, 
                   pe_sec->Misc.VirtualSize ?
                   pe_sec->Misc.VirtualSize :
                   pe_sec->SizeOfRawData);
#endif
    }

    /* Perform base relocation, if necessary */
    if ( reloc )
        do_relocations( load_addr, (IMAGE_BASE_RELOCATION *)RVA(reloc) );

    /* Get expected OS / Subsystem version */
    *version =   ( (nt->OptionalHeader.MajorSubsystemVersion & 0xff) << 8 )
               |   (nt->OptionalHeader.MinorSubsystemVersion & 0xff);

    /* We don't need the orignal mapping any more */
    UnmapViewOfFile( (LPVOID)hModule );
    return (HMODULE)load_addr;

error:
    UnmapViewOfFile( (LPVOID)hModule );
    return 0;
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
 */
WINE_MODREF *PE_CreateModule( HMODULE hModule, 
                              LPCSTR filename, DWORD flags, BOOL builtin )
{
    DWORD load_addr = (DWORD)hModule;  /* for RVA */
    IMAGE_NT_HEADERS *nt = PE_HEADER(hModule);
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_IMPORT_DESCRIPTOR *pe_import = NULL;
    IMAGE_EXPORT_DIRECTORY *pe_export = NULL;
    IMAGE_RESOURCE_DIRECTORY *pe_resource = NULL;
    WINE_MODREF *wm;
    int	result;


    /* Retrieve DataDirectory entries */

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_EXPORT;
    if (dir->Size)
        pe_export = (PIMAGE_EXPORT_DIRECTORY)RVA(dir->VirtualAddress);

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (dir->Size)
        pe_import = (PIMAGE_IMPORT_DESCRIPTOR)RVA(dir->VirtualAddress);

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_RESOURCE;
    if (dir->Size)
        pe_resource = (PIMAGE_RESOURCE_DIRECTORY)RVA(dir->VirtualAddress);

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_EXCEPTION;
    if (dir->Size) FIXME("Exception directory ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_SECURITY;
    if (dir->Size) FIXME("Security directory ignored\n" );

    /* IMAGE_DIRECTORY_ENTRY_BASERELOC handled in PE_LoadImage */
    /* IMAGE_DIRECTORY_ENTRY_DEBUG handled by debugger */

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_DEBUG;
    if (dir->Size) TRACE("Debug directory ignored\n" );

    dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_COPYRIGHT;
    if (dir->Size) FIXME("Copyright string ignored\n" );

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


    /* Allocate and fill WINE_MODREF */

    wm = (WINE_MODREF *)HeapAlloc( GetProcessHeap(), 
                                   HEAP_ZERO_MEMORY, sizeof(*wm) );
    wm->module = hModule;

    if ( builtin ) 
        wm->flags |= WINE_MODREF_INTERNAL;
    if ( flags & DONT_RESOLVE_DLL_REFERENCES )
        wm->flags |= WINE_MODREF_DONT_RESOLVE_REFS;
    if ( flags & LOAD_LIBRARY_AS_DATAFILE )
        wm->flags |= WINE_MODREF_LOAD_AS_DATAFILE;

    wm->type = MODULE32_PE;
    wm->binfmt.pe.pe_export = pe_export;
    wm->binfmt.pe.pe_import = pe_import;
    wm->binfmt.pe.pe_resource = pe_resource;
    wm->binfmt.pe.tlsindex = -1;

    wm->filename = HEAP_strdupA( GetProcessHeap(), 0, filename );
    wm->modname = strrchr( wm->filename, '\\' );
    if (!wm->modname) wm->modname = wm->filename;
    else wm->modname++;

    result = GetShortPathNameA( wm->filename, NULL, 0 );
    wm->short_filename = (char *)HeapAlloc( GetProcessHeap(), 0, result+1 );
    GetShortPathNameA( wm->filename, wm->short_filename, result+1 );
    wm->short_modname = strrchr( wm->short_filename, '\\' );
    if (!wm->short_modname) wm->short_modname = wm->short_filename;
    else wm->short_modname++;

    /* Link MODREF into process list */

    EnterCriticalSection( &PROCESS_Current()->crit_section );

    wm->next = PROCESS_Current()->modref_list;
    PROCESS_Current()->modref_list = wm;
    if ( wm->next ) wm->next->prev = wm;

    if (    !( nt->FileHeader.Characteristics & IMAGE_FILE_DLL )
         && !( wm->flags & WINE_MODREF_LOAD_AS_DATAFILE ) )

    {
        if ( PROCESS_Current()->exe_modref )
            FIXME( "Trying to load second .EXE file: %s\n", filename );
        else
            PROCESS_Current()->exe_modref = wm;
    }

    LeaveCriticalSection( &PROCESS_Current()->crit_section );


    /* Dump Exports */

    if ( pe_export )
        dump_exports( hModule );

    /* Fixup Imports */

    if (    pe_import
         && !( wm->flags & WINE_MODREF_LOAD_AS_DATAFILE )
         && !( wm->flags & WINE_MODREF_DONT_RESOLVE_REFS ) 
         && fixup_imports( wm ) ) 
    {
        /* remove entry from modref chain */
        EnterCriticalSection( &PROCESS_Current()->crit_section );

        if ( !wm->prev )
            PROCESS_Current()->modref_list = wm->next;
        else
            wm->prev->next = wm->next;

        if ( wm->next ) wm->next->prev = wm->prev;
        wm->next = wm->prev = NULL;

        LeaveCriticalSection( &PROCESS_Current()->crit_section );

        /* FIXME: there are several more dangling references
         * left. Including dlls loaded by this dll before the
         * failed one. Unrolling is rather difficult with the
         * current structure and we can leave it them lying
         * around with no problems, so we don't care.
         * As these might reference our wm, we don't free it.
         */
         return NULL;
    }

    return wm;
}

/******************************************************************************
 * The PE Library Loader frontend. 
 * FIXME: handle the flags.
 */
WINE_MODREF *PE_LoadLibraryExA (LPCSTR name, DWORD flags, DWORD *err)
{
	HMODULE		hModule32;
	HMODULE16	hModule16;
	NE_MODULE	*pModule;
	WINE_MODREF	*wm;
	char        	filename[256];
	HANDLE		hFile;
	WORD		version = 0;

	/* Search for and open PE file */
	if ( SearchPathA( NULL, name, ".DLL", 
	                  sizeof(filename), filename, NULL ) == 0 )
	{
		*err = ERROR_FILE_NOT_FOUND;
		return NULL;
	}
       
	hFile = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, -1 );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		*err = ERROR_FILE_NOT_FOUND;
		return NULL;
	}
	
	/* Load PE module */
	hModule32 = PE_LoadImage( hFile, filename, &version );
	CloseHandle( hFile );
	if (!hModule32)
	{
		*err = ERROR_OUTOFMEMORY;	/* Not entirely right, but good enough */
		return NULL;
	}

	/* Create 16-bit dummy module */
	if ((hModule16 = MODULE_CreateDummyModule( filename, version )) < 32)
	{
		*err = (DWORD)hModule16;	/* This should give the correct error */
		return NULL;
	}
	pModule = (NE_MODULE *)GlobalLock16( hModule16 );
	pModule->flags    = NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA | NE_FFLAGS_WIN32;
	pModule->module32 = hModule32;

	/* Create 32-bit MODREF */
	if ( !(wm = PE_CreateModule( hModule32, filename, flags, FALSE )) )
	{
		ERR( "can't load %s\n", filename );
		FreeLibrary16( hModule16 );
		*err = ERROR_OUTOFMEMORY;
		return NULL;
	}

	if (wm->binfmt.pe.pe_export)
		SNOOP_RegisterDLL(wm->module,wm->modname,wm->binfmt.pe.pe_export->NumberOfFunctions);

	*err = 0;
	return wm;
}


/*****************************************************************************
 *	PE_UnloadLibrary
 *
 * Unload the library unmapping the image and freeing the modref structure.
 */
void PE_UnloadLibrary(WINE_MODREF *wm)
{
    DWORD vma_size = calc_vma_size( wm->module );
    VirtualFree( (LPVOID)wm->module, vma_size, MEM_RELEASE );

    HeapFree( GetProcessHeap(), 0, wm->filename );
    HeapFree( GetProcessHeap(), 0, wm->short_filename );
    HeapFree( GetProcessHeap(), 0, wm );
}

/*****************************************************************************
 * Load the PE main .EXE. All other loading is done by PE_LoadLibraryExA
 * FIXME: this function should use PE_LoadLibraryExA, but currently can't
 * due to the PROCESS_Create stuff.
 */
BOOL PE_CreateProcess( HANDLE hFile, LPCSTR filename, LPCSTR cmd_line, LPCSTR env, 
                       LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                       BOOL inherit, DWORD flags, LPSTARTUPINFOA startup,
                       LPPROCESS_INFORMATION info )
{
    WORD version = 0;
    HMODULE16 hModule16;
    HMODULE hModule32;
    NE_MODULE *pModule;

    /* Load file */
    if ( (hModule32 = PE_LoadImage( hFile, filename, &version )) < 32 )
    {
        SetLastError( hModule32 );
        return FALSE;
    }
#if 0
    if (PE_HEADER(hModule32)->FileHeader.Characteristics & IMAGE_FILE_DLL)
    {
        SetLastError( 20 );  /* FIXME: not the right error code */
        return FALSE;
    }
#endif

    /* Create 16-bit dummy module */
    if ( (hModule16 = MODULE_CreateDummyModule( filename, version )) < 32 ) 
    {
        SetLastError( hModule16 );
        return FALSE;
    }
    pModule = (NE_MODULE *)GlobalLock16( hModule16 );
    pModule->flags    = NE_FFLAGS_WIN32;
    pModule->module32 = hModule32;

    /* Create new process */
    if ( !PROCESS_Create( pModule, cmd_line, env,
                          psa, tsa, inherit, flags, startup, info ) )
        return FALSE;

    /* Note: PE_CreateModule and the remaining process initialization will
             be done in the context of the new process, in TASK_CallToStart */

    return TRUE;
}

/*********************************************************************
 * PE_UnloadImage [internal]
 */
int PE_UnloadImage( HMODULE hModule )
{
	FIXME("stub.\n");
	/* free resources, image, unmap */
	return 1;
}

/* Called if the library is loaded or freed.
 * NOTE: if a thread attaches a DLL, the current thread will only do
 * DLL_PROCESS_ATTACH. Only new created threads do DLL_THREAD_ATTACH
 * (SDK)
 */
BOOL PE_InitDLL( WINE_MODREF *wm, DWORD type, LPVOID lpReserved )
{
    BOOL retv = TRUE;
    assert( wm->type == MODULE32_PE );

    /* Is this a library? And has it got an entrypoint? */
    if ((PE_HEADER(wm->module)->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        (PE_HEADER(wm->module)->OptionalHeader.AddressOfEntryPoint)
    ) {
        DLLENTRYPROC entry = (void*)RVA_PTR( wm->module,OptionalHeader.AddressOfEntryPoint );
        TRACE_(relay)("CallTo32(entryproc=%p,module=%08x,type=%ld,res=%p)\n",
                       entry, wm->module, type, lpReserved );

        retv = entry( wm->module, type, lpReserved );
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
	PE_MODREF		*pem;
	IMAGE_NT_HEADERS	*peh;
	DWORD			size,datasize;
	LPVOID			mem;
	PIMAGE_TLS_DIRECTORY	pdir;
        int delta;
	
	for (wm = PROCESS_Current()->modref_list;wm;wm=wm->next) {
		if (wm->type!=MODULE32_PE)
			continue;
		pem = &(wm->binfmt.pe);
		peh = PE_HEADER(wm->module);
		delta = wm->module - peh->OptionalHeader.ImageBase;
		if (!peh->OptionalHeader.DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress)
			continue;
		pdir = (LPVOID)(wm->module + peh->OptionalHeader.
			DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress);
		
		
		if ( pem->tlsindex == -1 ) {
			LPDWORD xaddr;
			pem->tlsindex = TlsAlloc();
			xaddr = _fixup_address(&(peh->OptionalHeader),delta,
					pdir->AddressOfIndex
			);
			*xaddr=pem->tlsindex;
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

		TlsSetValue( pem->tlsindex, mem );
	}
}


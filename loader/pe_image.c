/* 
 *  Copyright	1994	Eric Youndale & Erik Bos
 *  Copyright	1995	Martin von Löwis
 *  Copyright   1996    Marcus Meissner
 *
 *	based on Eric Youndale's pe-test and:
 *
 *	ftp.microsoft.com:/pub/developer/MSDN/CD8/PEFILE.ZIP
 * make that:
 *	ftp.microsoft.com:/developr/MSDN/OctCD/PEFILE.ZIP
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "windows.h"
#include "winbase.h"
#include "callback.h"
#include "file.h"
#include "neexe.h"
#include "peexe.h"
#include "process.h"
#include "pe_image.h"
#include "module.h"
#include "global.h"
#include "task.h"
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

static void PE_InitDLL(PE_MODREF* modref, DWORD type, LPVOID lpReserved);

/* convert PE image VirtualAddress to Real Address */
#define RVA(x) ((unsigned int)load_addr+(unsigned int)(x))

void dump_exports( HMODULE32 hModule )
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
  dprintf_win32(stddeb,"\n*******EXPORT DATA*******\nModule name is %s, %ld functions, %ld names\n", 
	 Module,
	 pe_exports->NumberOfFunctions,
	 pe_exports->NumberOfNames);

  ordinal=(u_short*) RVA(pe_exports->AddressOfNameOrdinals);
  functions=function=(u_long*) RVA(pe_exports->AddressOfFunctions);
  name=(u_char**) RVA(pe_exports->AddressOfNames);

  dprintf_win32(stddeb," Ord    RVA     Addr   Name\n" );
  for (i=0;i<pe_exports->NumberOfFunctions;i++, function++)
  {
      if (!*function) continue;  /* No such function */
      dprintf_win32( stddeb,"%4ld %08lx %08x",
                     i + pe_exports->Base, *function, RVA(*function) );
      /* Check if we have a name for it */
      for (j = 0; j < pe_exports->NumberOfNames; j++)
          if (ordinal[j] == i)
              dprintf_win32( stddeb, "  %s", (char*)RVA(name[j]) );
      if ((*function >= rva_start) && (*function <= rva_end))
	  dprintf_win32(stddeb, " (forwarded -> %s)", (char *)RVA(*function));
      dprintf_win32( stddeb,"\n" );
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
FARPROC32 PE_FindExportedFunction( HMODULE32 hModule, LPCSTR funcName)
{
	IMAGE_EXPORT_DIRECTORY 		*exports;
	unsigned			load_addr;
	u_short				* ordinal;
	u_long				* function;
	u_char				** name, *ename;
	int				i;
	PDB32				*process=PROCESS_Current();
	PE_MODREF			*pem;
	u_long				rva_start, rva_end, addr;
	char				* forward;

	pem = process->modref_list;
	while (pem && (pem->module != hModule))
		pem=pem->next;
	if (!pem) {
		fprintf(stderr,"No MODREF found for PE_MODULE %08x in process %p\n",hModule,process);
		return NULL;
	}
	load_addr = hModule;
	exports   = pem->pe_export;

	if (HIWORD(funcName))
		dprintf_win32(stddeb,"PE_FindExportedFunction(%s)\n",funcName);
	else
		dprintf_win32(stddeb,"PE_FindExportedFunction(%d)\n",(int)funcName);
	if (!exports) {
		fprintf(stderr,"Module %08x/MODREF %p doesn't have a exports table.\n",hModule,pem);
		return NULL;
	}
	ordinal	= (u_short*)  RVA(exports->AddressOfNameOrdinals);
	function= (u_long*)   RVA(exports->AddressOfFunctions);
	name	= (u_char **) RVA(exports->AddressOfNames);
	forward = NULL;
	rva_start = PE_HEADER(hModule)->OptionalHeader
		.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	rva_end = rva_start + PE_HEADER(hModule)->OptionalHeader
		.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (HIWORD(funcName)) {
		for(i=0; i<exports->NumberOfNames; i++) {
			ename=(char*)RVA(*name);
			if(!strcmp(ename,funcName))
                        {
                            addr = function[*ordinal];
                            if ((addr < rva_start) || (addr >= rva_end))
                                return (FARPROC32)RVA(addr);
                            forward = (char *)RVA(addr);
                            break;
			}
			ordinal++;
			name++;
		}
	} else {
		if (LOWORD(funcName)-exports->Base > exports->NumberOfFunctions) {
			dprintf_win32(stddeb,"	ordinal %d out of range!\n",
                                      LOWORD(funcName));
			return NULL;
		}
		addr = function[(int)funcName-exports->Base];
		if ((addr < rva_start) || (addr >= rva_end))
			return (FARPROC32)RVA(addr);
		forward = (char *)RVA(addr);
	}
	if (forward)
        {
		char module[256];
		char *end = strchr(forward, '.');
		if (!end) return NULL;
		strncpy(module, forward, (end - forward));
		module[end-forward] = 0;
		return GetProcAddress32(MODULE_FindModule(module), end + 1);
	}
	return NULL;
}

void 
fixup_imports (PDB32 *process,PE_MODREF *pem,HMODULE32 hModule)
{
    IMAGE_IMPORT_DESCRIPTOR	*pe_imp;
    int	fixup_failed		= 0;
    unsigned int load_addr	= pem->module;
    int				i;
    char			*modname;
    
    if (pem->pe_export)
    	modname = (char*) RVA(pem->pe_export->Name);
    else
        modname = "<unknown>";

    /* OK, now dump the import list */
    dprintf_win32 (stddeb, "\nDumping imports list\n");

    /* first, count the number of imported non-internal modules */
    pe_imp = pem->pe_import;
    if (!pe_imp) 
    	fprintf(stderr,"no import directory????\n");

    /* FIXME: should terminate on 0 Characteristics */
    for (i = 0; pe_imp->Name; pe_imp++)
	i++;

    /* load the imported modules. They are automatically 
     * added to the modref list of the process.
     */
 
    /* FIXME: should terminate on 0 Characteristics */
    for (i = 0, pe_imp = pem->pe_import; pe_imp->Name; pe_imp++) {
    	HMODULE32	res;
	PE_MODREF	*xpem,**ypem;


 	char *name = (char *) RVA(pe_imp->Name);

	/* don't use MODULE_Load, Win32 creates new task differently */
	res = PE_LoadLibraryEx32A( name, 0, 0 );
	if (res <= (HMODULE32) 32) {
	    char *p, buffer[1024];

	    /* Try with prepending the path of the current module */
	    GetModuleFileName32A( hModule, buffer, sizeof (buffer));
	    if (!(p = strrchr (buffer, '\\')))
		p = buffer;
	    strcpy (p + 1, name);
	    res = PE_LoadLibraryEx32A( buffer, 0, 0 );
	}
	if (res <= (HMODULE32) 32) {
	    fprintf (stderr, "Module %s not found\n", name);
	    exit (0);
	}
	res = MODULE_HANDLEtoHMODULE32(res);
	xpem = pem->next;
	while (xpem) {
		if (xpem->module == res)
			break;
		xpem = xpem->next;
	}
	if (xpem) {
		/* it has been loaded *BEFORE* us, so we have to init
		 * it before us. we just swap the two modules which should
		 * work.
		 */
		/* unlink xpem from chain */
		ypem = &(process->modref_list);
		while (*ypem) {
			if ((*ypem)==xpem)
				break;
			ypem = &((*ypem)->next);
		}
		*ypem		= xpem->next;

		/* link it directly before pem */
		ypem		= &(process->modref_list);
		while (*ypem) {
			if ((*ypem)==pem)
				break;
			ypem = &((*ypem)->next);
		}
		*ypem		= xpem;
		xpem->next	= pem;
		
	}
	i++;
    }
    pe_imp = pem->pe_import;
    while (pe_imp->Name) {
	char			*Module;
	IMAGE_IMPORT_BY_NAME	*pe_name;
	LPIMAGE_THUNK_DATA	import_list,thunk_list;

	Module = (char *) RVA(pe_imp->Name);
	dprintf_win32 (stddeb, "%s\n", Module);

	/* FIXME: forwarder entries ... */

	if (pe_imp->u.OriginalFirstThunk != 0) { /* original MS style */
	    dprintf_win32 (stddeb, "Microsoft style imports used\n");
	    import_list =(LPIMAGE_THUNK_DATA) RVA(pe_imp->u.OriginalFirstThunk);
	    thunk_list = (LPIMAGE_THUNK_DATA) RVA(pe_imp->FirstThunk);

	    while (import_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal)) {
		    int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

		    dprintf_win32 (stddeb, "--- Ordinal %s,%d\n", Module, ordinal);
		    thunk_list->u1.Function=(LPDWORD)GetProcAddress32(MODULE_FindModule(Module),(LPCSTR)ordinal);
		    if (!thunk_list->u1.Function) {
			fprintf(stderr,"No implementation for %s.%d, setting to NULL\n",
				Module, ordinal);
			/* fixup_failed=1; */
		    }
		} else {		/* import by name */
		    pe_name = (LPIMAGE_IMPORT_BY_NAME)RVA(import_list->u1.AddressOfData);
		    dprintf_win32 (stddeb, "--- %s %s.%d\n", pe_name->Name, Module, pe_name->Hint);
		    thunk_list->u1.Function=(LPDWORD)GetProcAddress32(
						MODULE_FindModule (Module),
						pe_name->Name);
		    if (!thunk_list->u1.Function) {
			fprintf(stderr,"No implementation for %s.%d(%s), setting to NULL\n",
				Module,pe_name->Hint,pe_name->Name);
			/* fixup_failed=1; */
		    }
		}
		import_list++;
		thunk_list++;
	    }
	} else {	/* Borland style */
	    dprintf_win32 (stddeb, "Borland style imports used\n");
	    thunk_list = (LPIMAGE_THUNK_DATA) RVA(pe_imp->FirstThunk);
	    while (thunk_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(thunk_list->u1.Ordinal)) {
		    /* not sure about this branch, but it seems to work */
		    int ordinal = IMAGE_ORDINAL(thunk_list->u1.Ordinal);

		    dprintf_win32(stddeb,"--- Ordinal %s.%d\n",Module,ordinal);
		    thunk_list->u1.Function=(LPDWORD)GetProcAddress32(MODULE_FindModule(Module),
						     (LPCSTR) ordinal);
		    if (!thunk_list->u1.Function) {
			fprintf(stderr, "No implementation for %s.%d, setting to NULL\n",
				Module,ordinal);
			/* fixup_failed=1; */
		    }
		} else {
		    pe_name=(LPIMAGE_IMPORT_BY_NAME) RVA(thunk_list->u1.AddressOfData);
		    dprintf_win32(stddeb,"--- %s %s.%d\n",
		   		  pe_name->Name,Module,pe_name->Hint);
		    thunk_list->u1.Function=(LPDWORD)GetProcAddress32(MODULE_FindModule(Module),pe_name->Name);
		    if (!thunk_list->u1.Function) {
		    	fprintf(stderr, "No implementation for %s.%d, setting to NULL\n",
				Module, pe_name->Hint);
		    	/* fixup_failed=1; */
		    }
		}
		thunk_list++;
	    }
	}
	pe_imp++;
    }
    if (fixup_failed) exit(1);
}

static int calc_vma_size( HMODULE32 hModule )
{
    int i,vma_size = 0;
    IMAGE_SECTION_HEADER *pe_seg = PE_SECTIONS(hModule);

    dprintf_win32(stddeb, "Dump of segment table\n");
    dprintf_win32(stddeb, "   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
    for (i = 0; i< PE_HEADER(hModule)->FileHeader.NumberOfSections; i++)
    {
        dprintf_win32(stddeb, "%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
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
        vma_size = MAX(vma_size, pe_seg->VirtualAddress+pe_seg->SizeOfRawData);
        pe_seg++;
    }
    return vma_size;
}

static void do_relocations(PE_MODREF *pem)
{
    int delta = pem->module - PE_HEADER(pem->module)->OptionalHeader.ImageBase;
    unsigned int load_addr= pem->module;
	IMAGE_BASE_RELOCATION		*r = pem->pe_reloc;
	int				hdelta = (delta >> 16) & 0xFFFF;
	int				ldelta = delta & 0xFFFF;

	/* int reloc_size = */

	if(delta == 0)
		/* Nothing to do */
		return;
	while(r->VirtualAddress)
	{
		char *page = (char*) RVA(r->VirtualAddress);
		int count = (r->SizeOfBlock - 8)/2;
		int i;
		dprintf_fixup(stddeb, "%x relocations for page %lx\n",
			count, r->VirtualAddress);
		/* patching in reverse order */
		for(i=0;i<count;i++)
		{
			int offset = r->TypeOffset[i] & 0xFFF;
			int type = r->TypeOffset[i] >> 12;
			dprintf_fixup(stddeb,"patching %x type %x\n", offset, type);
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
#if 1
				*(int*)(page+offset) += delta;
#else
				{ int h=*(unsigned short*)(page+offset);
				  int l=r->TypeOffset[++i];
				  *(unsigned int*)(page + offset) = (h<<16) + l + delta;
				}
#endif
				break;
			case IMAGE_REL_BASED_HIGHADJ:
				fprintf(stderr, "Don't know what to do with IMAGE_REL_BASED_HIGHADJ\n");
				break;
			case IMAGE_REL_BASED_MIPS_JMPADDR:
				fprintf(stderr, "Is this a MIPS machine ???\n");
				break;
			default:
				fprintf(stderr, "Unknown fixup type\n");
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
static HMODULE32 PE_LoadImage( HFILE32 hFile )
{
    HMODULE32 hModule;
    HANDLE32 mapping;

    /* map the PE file somewhere */
    mapping = CreateFileMapping32A( hFile, NULL, PAGE_READONLY | SEC_COMMIT,
                                    0, 0, NULL );
    if (!mapping)
    {
        fprintf( stderr, "PE_LoadImage: CreateFileMapping error %ld\n",
                 GetLastError() );
        return 0;
    }
    hModule = (HMODULE32)MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!hModule)
    {
        fprintf( stderr, "PE_LoadImage: MapViewOfFile error %ld\n",
                 GetLastError() );
        return 0;
    }

    if (PE_HEADER(hModule)->Signature != IMAGE_NT_SIGNATURE)
    {
        fprintf(stderr,"image doesn't have PE signature, but 0x%08lx\n",
                PE_HEADER(hModule)->Signature );
        goto error;
    }

    if (PE_HEADER(hModule)->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        fprintf(stderr,"trying to load PE image for unsupported architecture (");
        switch (PE_HEADER(hModule)->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_UNKNOWN: fprintf(stderr,"Unknown"); break;
        case IMAGE_FILE_MACHINE_I860:    fprintf(stderr,"I860"); break;
        case IMAGE_FILE_MACHINE_R3000:   fprintf(stderr,"R3000"); break;
        case IMAGE_FILE_MACHINE_R4000:   fprintf(stderr,"R4000"); break;
        case IMAGE_FILE_MACHINE_R10000:  fprintf(stderr,"R10000"); break;
        case IMAGE_FILE_MACHINE_ALPHA:   fprintf(stderr,"Alpha"); break;
        case IMAGE_FILE_MACHINE_POWERPC: fprintf(stderr,"PowerPC"); break;
        default: fprintf(stderr,"Unknown-%04x",
                         PE_HEADER(hModule)->FileHeader.Machine); break;
        }
        fprintf(stderr,")\n");
        goto error;
    }
    return hModule;

error:
    UnmapViewOfFile( (LPVOID)hModule );
    return 0;
}

/**********************************************************************
 * This maps a loaded PE dll into the address space of the specified process.
 */
static HMODULE32 PE_MapImage( HMODULE32 hModule, PDB32 *process,
                              OFSTRUCT *ofs, DWORD flags )
{
	PE_MODREF		*pem;
	int			i, result;
	DWORD			load_addr;
	IMAGE_DATA_DIRECTORY	dir;
	char			*modname;
	int			vma_size;

        IMAGE_SECTION_HEADER *pe_seg;
        IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)hModule;
        IMAGE_NT_HEADERS *nt_header = PE_HEADER(hModule);
	
	pem = (PE_MODREF*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                                    sizeof(*pem));
	/* NOTE: fixup_imports takes care of the correct order */
	pem->next	= process->modref_list;
	process->modref_list = pem;

	if (!(nt_header->FileHeader.Characteristics & IMAGE_FILE_DLL))
        {
		if (process->exe_modref)
			fprintf(stderr,"overwriting old exe_modref... arrgh\n");
		process->exe_modref = pem;
	}

	load_addr = nt_header->OptionalHeader.ImageBase;
	vma_size = calc_vma_size( hModule );
	dprintf_win32(stddeb, "Load addr is %lx\n",load_addr);
	load_addr = (DWORD)VirtualAlloc( (void*)load_addr, vma_size,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_EXECUTE_READWRITE );
	if (load_addr == 0) {
		load_addr = (DWORD)VirtualAlloc( NULL, vma_size,
						 MEM_RESERVE | MEM_COMMIT,
						 PAGE_EXECUTE_READWRITE );
	}
	pem->module = (HMODULE32)load_addr;

	dprintf_win32(stddeb, "Load addr is really %lx, range %x\n",
                      load_addr, vma_size);
	
        /* Store the NT header at the load addr
         * (FIXME: should really use mmap)
         */
        *(IMAGE_DOS_HEADER *)load_addr = *dos_header;
        *(IMAGE_NT_HEADERS *)(load_addr + dos_header->e_lfanew) = *nt_header;

        pe_seg = PE_SECTIONS(hModule);
	for (i = 0; i < nt_header->FileHeader.NumberOfSections; i++, pe_seg++)
	{
		/* memcpy only non-BSS segments */
		/* FIXME: this should be done by mmap(..MAP_PRIVATE|MAP_FIXED..)
		 * but it is not possible for (at least) Linux needs
		 * a page-aligned offset.
		 */
		if(!(pe_seg->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
		    memcpy((char*)RVA(pe_seg->VirtualAddress),
		    	(char*)(hModule + pe_seg->PointerToRawData),
			pe_seg->SizeOfRawData
		    );

		result = RVA (pe_seg->VirtualAddress);
#if 1
		/* not needed, memory is zero */
		if(strcmp(pe_seg->Name, ".bss") == 0)
		    memset((void *)result, 0, 
			   pe_seg->Misc.VirtualSize ?
			   pe_seg->Misc.VirtualSize :
			   pe_seg->SizeOfRawData);
#endif

		if(strcmp(pe_seg->Name, ".idata") == 0)
			pem->pe_import = (LPIMAGE_IMPORT_DESCRIPTOR) result;

		if(strcmp(pe_seg->Name, ".edata") == 0)
			pem->pe_export = (LPIMAGE_EXPORT_DIRECTORY) result;

		if(strcmp(pe_seg->Name, ".rsrc") == 0)
			pem->pe_resource = (LPIMAGE_RESOURCE_DIRECTORY) result;

		if(strcmp(pe_seg->Name, ".reloc") == 0)
			pem->pe_reloc = (LPIMAGE_BASE_RELOCATION) result;
	}

	/* There is word that the actual loader does not care about the
	   section names, and only goes for the DataDirectory */
	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if(dir.Size)
	{
		if(pem->pe_export && (int)pem->pe_export!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong export directory??\n");
		/* always trust the directory */
		pem->pe_export = (LPIMAGE_EXPORT_DIRECTORY) RVA(dir.VirtualAddress);
	}

	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if(dir.Size)
	{
		/* 
		if(pem->pe_import && (int)pem->pe_import!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong import directory??\n");
		 */
		pem->pe_import = (LPIMAGE_IMPORT_DESCRIPTOR) RVA(dir.VirtualAddress);
	}

	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
	if(dir.Size)
	{
		if(pem->pe_resource && (int)pem->pe_resource!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong resource directory??\n");
		pem->pe_resource = (LPIMAGE_RESOURCE_DIRECTORY) RVA(dir.VirtualAddress);
	}

	if(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size)
		dprintf_win32(stdnimp,"Exception directory ignored\n");

	if(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size)
		dprintf_win32(stdnimp,"Security directory ignored\n");



	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if(dir.Size)
	{
		if(pem->pe_reloc && (int)pem->pe_reloc!= RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong relocation list??\n");
		pem->pe_reloc = (void *) RVA(dir.VirtualAddress);
	}

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size)
		dprintf_win32(stdnimp,"Copyright string ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size)
		dprintf_win32(stdnimp,"Global Pointer (MIPS) ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_TLS].Size)
		 fprintf(stdnimp,"Thread local storage ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size)
		dprintf_win32(stdnimp,"Load Configuration directory ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size)
		dprintf_win32(stdnimp,"Bound Import directory ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_IAT].Size)
		dprintf_win32(stdnimp,"Import Address Table directory ignored\n");
	if(nt_header->OptionalHeader.DataDirectory[13].Size)
		dprintf_win32(stdnimp,"Unknown directory 13 ignored\n");
	if(nt_header->OptionalHeader.DataDirectory[14].Size)
		dprintf_win32(stdnimp,"Unknown directory 14 ignored\n");
	if(nt_header->OptionalHeader.DataDirectory[15].Size)
		dprintf_win32(stdnimp,"Unknown directory 15 ignored\n");

	if(pem->pe_reloc)	do_relocations(pem);
	if(pem->pe_export)	dump_exports(pem->module);
	if(pem->pe_import)	fixup_imports(process,pem,hModule);
  		
	if (pem->pe_export)
		modname = (char*)RVA(pem->pe_export->Name);
	else {
		char *s;
		modname = s = ofs->szPathName;
		while ((s=strchr(modname,'\\')))
			modname = s+1;
		if ((s=strchr(modname,'.')))
			*s='\0';
	}

        /* Now that we got everything at the right address,
         * we can unmap the previous module */
        UnmapViewOfFile( (LPVOID)hModule );
        return (HMODULE32)load_addr;
}

HINSTANCE16 MODULE_CreateInstance(HMODULE16 hModule,LOADPARAMS *params);

/******************************************************************************
 * The PE Library Loader frontend. 
 * FIXME: handle the flags.
 *        internal module handling should be made better here (and in builtin.c)
 */
HMODULE32 PE_LoadLibraryEx32A (LPCSTR name, HFILE32 hFile, DWORD flags) {
	OFSTRUCT	ofs;
	HMODULE32	hModule;
	NE_MODULE	*pModule;
	PE_MODREF	*pem;

	if ((hModule = MODULE_FindModule( name ))) {
		/* the .DLL is either loaded or internal */
		hModule = MODULE_HANDLEtoHMODULE32(hModule);
		if (!HIWORD(hModule)) /* internal (or bad) */
			return hModule;
		/* check if this module is already mapped */
		pem 	= PROCESS_Current()->modref_list;
		while (pem) {
			if (pem->module == hModule) return hModule;
			pem = pem->next;
		}
		pModule = MODULE_GetPtr(hModule);
		if (pModule->flags & NE_FFLAGS_BUILTIN) {
			PDB32	*process = PROCESS_Current();
			IMAGE_DOS_HEADER	*dh;
			IMAGE_NT_HEADERS	*nh;
			IMAGE_SECTION_HEADER	*sh;

			/* we only come here if we already have 'loaded' the
			 * internal dll but in another process. Just create
			 * a PE_MODREF and return.
			 */
			pem = (PE_MODREF*)HeapAlloc(GetProcessHeap(),
				HEAP_ZERO_MEMORY,sizeof(*pem));
			pem->module 	     = hModule;
			dh = (IMAGE_DOS_HEADER*)pem->module;
			nh = (IMAGE_NT_HEADERS*)(dh+1);
			sh = (IMAGE_SECTION_HEADER*)(nh+1);
			pem->pe_export	     = (IMAGE_EXPORT_DIRECTORY*)(sh+2);
			pem->next	     = process->modref_list;
			process->modref_list = pem;
			return hModule;
		}
	} else {

		/* try to load builtin, enabled modules first */
		if ((hModule = BUILTIN_LoadModule( name, FALSE )))
                    return MODULE_HANDLEtoHMODULE32( hModule );

		/* try to open the specified file */
		if (HFILE_ERROR32==(hFile=OpenFile32(name,&ofs,OF_READ))) {
			/* Now try the built-in even if disabled */
			if ((hModule = BUILTIN_LoadModule( name, TRUE ))) {
				fprintf( stderr, "Warning: could not load Windows DLL '%s', using built-in module.\n", name );
                                return MODULE_HANDLEtoHMODULE32( hModule );
			}
			return 1;
		}
		if ((hModule = MODULE_CreateDummyModule( &ofs )) < 32) {
			_lclose32(hFile);
			return hModule;
		}
		pModule		= (NE_MODULE *)GlobalLock16( hModule );
		pModule->flags	= NE_FFLAGS_WIN32;
		pModule->module32 = PE_LoadImage( hFile );
		CloseHandle( hFile );
		if (pModule->module32 < 32) return 21;
	}
	/* recurse */
	pModule->module32 = PE_MapImage( pModule->module32, PROCESS_Current(),
                                         &ofs,flags);
	return pModule->module32;
}

/*****************************************************************************
 * Load the PE main .EXE. All other loading is done by PE_LoadLibraryEx32A
 * FIXME: this function should use PE_LoadLibraryEx32A, but currently can't
 * due to the TASK_CreateTask stuff.
 */
HINSTANCE16 PE_LoadModule( HFILE32 hFile, OFSTRUCT *ofs, LOADPARAMS* params )
{
    HMODULE16 hModule16;
    HMODULE32 hModule32;
    HINSTANCE16 hInstance;
    NE_MODULE *pModule;

    if ((hModule16 = MODULE_CreateDummyModule( ofs )) < 32) return hModule16;
    pModule = (NE_MODULE *)GlobalLock16( hModule16 );
    pModule->flags = NE_FFLAGS_WIN32;

    pModule->module32 = hModule32 = PE_LoadImage( hFile );
    CloseHandle( hFile );
    if (hModule32 < 32) return 21;

    hInstance = MODULE_CreateInstance( hModule16, params );
    if (!(PE_HEADER(hModule32)->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        TASK_CreateTask( hModule16, hInstance, 0,
                         params->hEnvironment,
                         (LPSTR)PTR_SEG_TO_LIN( params->cmdLine ),
                         *((WORD*)PTR_SEG_TO_LIN(params->showCmd) + 1) );
    }
    pModule->module32 = PE_MapImage( hModule32, PROCESS_Current(), ofs, 0 );
    return hInstance;
}

int PE_UnloadImage( HMODULE32 hModule )
{
	printf("PEunloadImage() called!\n");
	/* free resources, image, unmap */
	return 1;
}

/* Called if the library is loaded or freed.
 * NOTE: if a thread attaches a DLL, the current thread will only do
 * DLL_PROCESS_ATTACH. Only new created threads do DLL_THREAD_ATTACH
 * (SDK)
 */
static void PE_InitDLL(PE_MODREF *pem, DWORD type,LPVOID lpReserved)
{
    if (type==DLL_PROCESS_ATTACH)
	pem->flags |= PE_MODREF_PROCESS_ATTACHED;

    /*  DLL_ATTACH_PROCESS:
     *		lpreserved is NULL for dynamic loads, not-NULL for static loads
     *  DLL_DETACH_PROCESS:
     *		lpreserved is NULL if called by FreeLibrary, not-NULL otherwise
     *  the SDK doesn't mention anything for DLL_THREAD_*
     */
        
    /* Is this a library? And has it got an entrypoint? */
    if ((PE_HEADER(pem->module)->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        (PE_HEADER(pem->module)->OptionalHeader.AddressOfEntryPoint)
    ) {
        FARPROC32 entry = (FARPROC32)RVA_PTR( pem->module,
                                          OptionalHeader.AddressOfEntryPoint );
        dprintf_relay( stddeb, "CallTo32(entryproc=%p,module=%08x,type=%ld,res=%p)\n",
                       entry, pem->module, type, lpReserved );
        entry( pem->module, type, lpReserved );
    }
}

/* Call the DLLentry function of all dlls used by that process.
 * (NOTE: this may recursively call this function (if a library calls
 * LoadLibrary) ... but it won't matter)
 */
void PE_InitializeDLLs(PDB32 *process,DWORD type,LPVOID lpReserved) {
	PE_MODREF	*pem;

	pem = process->modref_list;
	while (pem) {
		if (pem->flags & PE_MODREF_NO_DLL_CALLS) {
			pem = pem->next;
			continue;
		}
		if (type==DLL_PROCESS_ATTACH) {
			if (pem->flags & PE_MODREF_PROCESS_ATTACHED) {
				pem = pem->next;
				continue;
			}
		}
		PE_InitDLL( pem, type, lpReserved );
		pem = pem->next;
	}
}

void PE_InitTls(PDB32 *pdb)
{
	/* FIXME: tls callbacks ??? */
	PE_MODREF		*pem;
	IMAGE_NT_HEADERS	*peh;
	DWORD			size,datasize,index;
	LPVOID			mem;
	LPIMAGE_TLS_DIRECTORY	pdir;

	pem = pdb->modref_list;
	while (pem) {
		peh = PE_HEADER(pem->module);
		if (!peh->OptionalHeader.DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress) {
			pem = pem->next;
			continue;
		}
		pdir = (LPVOID)(pem->module + peh->OptionalHeader.
			DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress);
		index	= TlsAlloc();
		datasize= pdir->EndAddressOfRawData-pdir->StartAddressOfRawData;
		size	= datasize + pdir->SizeOfZeroFill;
		mem=VirtualAlloc(0,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		memcpy(mem,(LPVOID) pdir->StartAddressOfRawData, datasize);
		TlsSetValue(index,mem);
		*(pdir->AddressOfIndex)=index;   
		pem=pem->next;
	}
}

/****************************************************************************
 *		DisableThreadLibraryCalls (KERNEL32.74)
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
 */
BOOL32 WINAPI DisableThreadLibraryCalls(HMODULE32 hModule)
{
	PDB32	*process = PROCESS_Current();
	PE_MODREF	*pem = process->modref_list;

	while (pem) {
		if (pem->module == hModule)
			pem->flags|=PE_MODREF_NO_DLL_CALLS;
		pem = pem->next;
	}
	return TRUE;
}


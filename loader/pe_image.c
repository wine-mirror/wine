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
 * - All those function map things into a new addresspace. From the wrong
 *   process and the wrong thread. So calling other API functions will mess 
 *   things up badly sometimes.
 */

#include <errno.h>
#include <assert.h>
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
#include "debug.h"


/* convert PE image VirtualAddress to Real Address */
#define RVA(x) ((unsigned int)load_addr+(unsigned int)(x))

#define AdjustPtr(ptr,delta) ((char *)(ptr) + (delta))

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
  TRACE(win32,"*******EXPORT DATA*******\n");
  TRACE(win32,"Module name is %s, %ld functions, %ld names\n", 
	       Module, pe_exports->NumberOfFunctions, pe_exports->NumberOfNames);

  ordinal=(u_short*) RVA(pe_exports->AddressOfNameOrdinals);
  functions=function=(u_long*) RVA(pe_exports->AddressOfFunctions);
  name=(u_char**) RVA(pe_exports->AddressOfNames);

  TRACE(win32," Ord    RVA     Addr   Name\n" );
  for (i=0;i<pe_exports->NumberOfFunctions;i++, function++)
  {
      if (!*function) continue;  /* No such function */
      if (TRACE_ON(win32)){
	dbg_decl_str(win32, 1024);

	dsprintf(win32,"%4ld %08lx %08x",
		 i + pe_exports->Base, *function, RVA(*function) );
	/* Check if we have a name for it */
	for (j = 0; j < pe_exports->NumberOfNames; j++)
          if (ordinal[j] == i)
	    dsprintf(win32, "  %s", (char*)RVA(name[j]) );
	if ((*function >= rva_start) && (*function <= rva_end))
	  dsprintf(win32, " (forwarded -> %s)", (char *)RVA(*function));
	TRACE(win32,"%s\n", dbg_str(win32));
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
FARPROC32 PE_FindExportedFunction( 
	PDB32 *process,		/* [in] process context */
	WINE_MODREF *wm,	/* [in] WINE modreference */
	LPCSTR funcName,	/* [in] function name */
        BOOL32 snoop )
{
	u_short				* ordinal;
	u_long				* function;
	u_char				** name, *ename;
	int				i;
	PE_MODREF			*pem = &(wm->binfmt.pe);
	IMAGE_EXPORT_DIRECTORY 		*exports = pem->pe_export;
	unsigned int			load_addr = wm->module;
	u_long				rva_start, rva_end, addr;
	char				* forward;

	if (HIWORD(funcName))
		TRACE(win32,"(%s)\n",funcName);
	else
		TRACE(win32,"(%d)\n",(int)funcName);
	if (!exports) {
		/* Not a fatal problem, some apps do
		 * GetProcAddress(0,"RegisterPenApp") which triggers this
		 * case.
		 */
		WARN(win32,"Module %08x(%s)/MODREF %p doesn't have a exports table.\n",wm->module,wm->modname,pem);
		return NULL;
	}
	ordinal	= (u_short*)  RVA(exports->AddressOfNameOrdinals);
	function= (u_long*)   RVA(exports->AddressOfFunctions);
	name	= (u_char **) RVA(exports->AddressOfNames);
	forward = NULL;
	rva_start = PE_HEADER(wm->module)->OptionalHeader
		.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	rva_end = rva_start + PE_HEADER(wm->module)->OptionalHeader
		.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (HIWORD(funcName)) {
		for(i=0; i<exports->NumberOfNames; i++) {
			ename=(char*)RVA(*name);
			if(!strcmp(ename,funcName))
                        {
                            addr = function[*ordinal];
                            if (!addr) return NULL;
                            if ((addr < rva_start) || (addr >= rva_end))
				return snoop? SNOOP_GetProcAddress32(wm->module,ename,*ordinal,(FARPROC32)RVA(addr))
                                            : (FARPROC32)RVA(addr);
                            forward = (char *)RVA(addr);
                            break;
			}
			ordinal++;
			name++;
		}
	} else 	{
		int i;
		if (LOWORD(funcName)-exports->Base > exports->NumberOfFunctions) {
			TRACE(win32,"	ordinal %d out of range!\n",
                                      LOWORD(funcName));
			return NULL;
		}
		addr = function[(int)funcName-exports->Base];
                if (!addr) return NULL;
		ename = "";
		if (name) {
		    for (i=0;i<exports->NumberOfNames;i++) {
			    ename = (char*)RVA(*name);
			    if (*ordinal == LOWORD(funcName)-exports->Base)
			    	break;
			    ordinal++;
			    name++;
		    }
		    if (i==exports->NumberOfNames)
		    	ename = "";
		}
		if ((addr < rva_start) || (addr >= rva_end))
			return snoop? SNOOP_GetProcAddress32(wm->module,ename,(DWORD)funcName-exports->Base,(FARPROC32)RVA(addr))
                                    : (FARPROC32)RVA(addr);
		forward = (char *)RVA(addr);
	}
	if (forward)
        {
                HMODULE32 hMod;
		char module[256];
		char *end = strchr(forward, '.');

		if (!end) return NULL;
		assert(end-forward<256);
		strncpy(module, forward, (end - forward));
		module[end-forward] = 0;
                hMod = MODULE_FindModule32(process,module);
		assert(hMod);
		return MODULE_GetProcAddress32( process, hMod, end + 1, snoop );
	}
	return NULL;
}

DWORD fixup_imports (PDB32 *process,WINE_MODREF *wm)
{
    IMAGE_IMPORT_DESCRIPTOR	*pe_imp;
    WINE_MODREF			*xwm;
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
    TRACE(win32, "Dumping imports list\n");

    /* first, count the number of imported non-internal modules */
    pe_imp = pem->pe_import;
    if (!pe_imp) 
    	ERR(win32, "no import directory????\n");

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

    /* Allocate module dependency list */
    wm->nDeps = i;
    wm->deps  = HeapAlloc(process->heap, 0, i*sizeof(WINE_MODREF *));

    /* load the imported modules. They are automatically 
     * added to the modref list of the process.
     */
 
    for (i = 0, pe_imp = pem->pe_import; pe_imp->Name ; pe_imp++) {
    	HMODULE32		hImpModule;
	IMAGE_IMPORT_BY_NAME	*pe_name;
	PIMAGE_THUNK_DATA	import_list,thunk_list;
 	char			*name = (char *) RVA(pe_imp->Name);

	if (characteristics_detection && !pe_imp->u.Characteristics)
		break;

	/* don't use MODULE_Load, Win32 creates new task differently */
	hImpModule = MODULE_LoadLibraryEx32A( name, process, 0, 0 );
	if (!hImpModule) {
	    char *p,buffer[2000];
	    
	    /* GetModuleFileName would use the wrong process, so don't use it */
	    strcpy(buffer,wm->shortname);
	    if (!(p = strrchr (buffer, '\\')))
		p = buffer;
	    strcpy (p + 1, name);
	    hImpModule = MODULE_LoadLibraryEx32A( buffer, process, 0, 0 );
	}
	if (!hImpModule) {
	    ERR (module, "Module %s not found\n", name);
	    return 1;
	}
        xwm = MODULE32_LookupHMODULE(process, hImpModule);
        assert( xwm );
        wm->deps[i++] = xwm;

	/* FIXME: forwarder entries ... */

	if (pe_imp->u.OriginalFirstThunk != 0) { /* original MS style */
	    TRACE(win32, "Microsoft style imports used\n");
	    import_list =(PIMAGE_THUNK_DATA) RVA(pe_imp->u.OriginalFirstThunk);
	    thunk_list = (PIMAGE_THUNK_DATA) RVA(pe_imp->FirstThunk);

	    while (import_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal)) {
		    int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

		    TRACE(win32, "--- Ordinal %s,%d\n", name, ordinal);
		    thunk_list->u1.Function=MODULE_GetProcAddress32(
                        process, hImpModule, (LPCSTR)ordinal, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR(win32,"No implementation for %s.%d, setting to 0xdeadbeef\n",
				name, ordinal);
                        thunk_list->u1.Function = (FARPROC32)0xdeadbeef;
		    }
		} else {		/* import by name */
		    pe_name = (PIMAGE_IMPORT_BY_NAME)RVA(import_list->u1.AddressOfData);
		    TRACE(win32, "--- %s %s.%d\n", pe_name->Name, name, pe_name->Hint);
		    thunk_list->u1.Function=MODULE_GetProcAddress32(
                        process, hImpModule, pe_name->Name, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR(win32,"No implementation for %s.%d(%s), setting to 0xdeadbeef\n",
				name,pe_name->Hint,pe_name->Name);
                        thunk_list->u1.Function = (FARPROC32)0xdeadbeef;
		    }
		}
		import_list++;
		thunk_list++;
	    }
	} else {	/* Borland style */
	    TRACE(win32, "Borland style imports used\n");
	    thunk_list = (PIMAGE_THUNK_DATA) RVA(pe_imp->FirstThunk);
	    while (thunk_list->u1.Ordinal) {
		if (IMAGE_SNAP_BY_ORDINAL(thunk_list->u1.Ordinal)) {
		    /* not sure about this branch, but it seems to work */
		    int ordinal = IMAGE_ORDINAL(thunk_list->u1.Ordinal);

		    TRACE(win32,"--- Ordinal %s.%d\n",name,ordinal);
		    thunk_list->u1.Function=MODULE_GetProcAddress32(
                        process, hImpModule, (LPCSTR) ordinal, TRUE
		    );
		    if (!thunk_list->u1.Function) {
			ERR(win32, "No implementation for %s.%d, setting to 0xdeadbeef\n",
				name,ordinal);
                        thunk_list->u1.Function = (FARPROC32)0xdeadbeef;
		    }
		} else {
		    pe_name=(PIMAGE_IMPORT_BY_NAME) RVA(thunk_list->u1.AddressOfData);
		    TRACE(win32,"--- %s %s.%d\n",
		   		  pe_name->Name,name,pe_name->Hint);
		    thunk_list->u1.Function=MODULE_GetProcAddress32(
                        process, hImpModule, pe_name->Name, TRUE
		    );
		    if (!thunk_list->u1.Function) {
		    	ERR(win32, "No implementation for %s.%d, setting to 0xdeadbeef\n",
				name, pe_name->Hint);
                        thunk_list->u1.Function = (FARPROC32)0xdeadbeef;
		    }
		}
		thunk_list++;
	    }
	}
    }
    return 0;
}

static int calc_vma_size( HMODULE32 hModule )
{
    int i,vma_size = 0;
    IMAGE_SECTION_HEADER *pe_seg = PE_SECTIONS(hModule);

    TRACE(win32, "Dump of segment table\n");
    TRACE(win32, "   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
    for (i = 0; i< PE_HEADER(hModule)->FileHeader.NumberOfSections; i++)
    {
        TRACE(win32, "%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
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

static void do_relocations(WINE_MODREF *wm)
{
    PE_MODREF	*pem = &(wm->binfmt.pe);
    int delta = wm->module - PE_HEADER(wm->module)->OptionalHeader.ImageBase;
    unsigned int load_addr= wm->module;

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
		TRACE(fixup, "%x relocations for page %lx\n",
			count, r->VirtualAddress);
		/* patching in reverse order */
		for(i=0;i<count;i++)
		{
			int offset = r->TypeOffset[i] & 0xFFF;
			int type = r->TypeOffset[i] >> 12;
			TRACE(fixup,"patching %x type %x\n", offset, type);
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
				FIXME(win32, "Don't know what to do with IMAGE_REL_BASED_HIGHADJ\n");
				break;
			case IMAGE_REL_BASED_MIPS_JMPADDR:
				FIXME(win32, "Is this a MIPS machine ???\n");
				break;
			default:
				FIXME(win32, "Unknown fixup type\n");
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
    HMODULE32	hModule;
    HANDLE32	mapping;
    int		i,rawsize = 0;
    IMAGE_SECTION_HEADER	*pe_sec;
    BY_HANDLE_FILE_INFORMATION	bhfi;


    /* map the PE file somewhere */
    mapping = CreateFileMapping32A( hFile, NULL, PAGE_READONLY | SEC_COMMIT,
                                    0, 0, NULL );
    if (!mapping)
    {
        WARN( win32, "CreateFileMapping error %ld\n",
                 GetLastError() );
        return 0;
    }
    hModule = (HMODULE32)MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!hModule)
    {
        WARN( win32, "PE_LoadImage: MapViewOfFile error %ld\n",
                 GetLastError() );
        return 0;
    }

    if (PE_HEADER(hModule)->Signature != IMAGE_NT_SIGNATURE)
    {
        WARN(win32,"image doesn't have PE signature, but 0x%08lx\n",
                PE_HEADER(hModule)->Signature );
        goto error;
    }

    if (PE_HEADER(hModule)->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        MSG("Trying to load PE image for unsupported architecture (");
        switch (PE_HEADER(hModule)->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_UNKNOWN: MSG("Unknown"); break;
        case IMAGE_FILE_MACHINE_I860:    MSG("I860"); break;
        case IMAGE_FILE_MACHINE_R3000:   MSG("R3000"); break;
        case IMAGE_FILE_MACHINE_R4000:   MSG("R4000"); break;
        case IMAGE_FILE_MACHINE_R10000:  MSG("R10000"); break;
        case IMAGE_FILE_MACHINE_ALPHA:   MSG("Alpha"); break;
        case IMAGE_FILE_MACHINE_POWERPC: MSG("PowerPC"); break;
        default: MSG("Unknown-%04x",
                         PE_HEADER(hModule)->FileHeader.Machine); break;
        }
        MSG(")\n");
        goto error;
    }
    /* find out how large this executeable should be */
    pe_sec = PE_SECTIONS(hModule);
    for (i=0;i<PE_HEADER(hModule)->FileHeader.NumberOfSections;i++) {
    	if (pe_sec[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	    continue;
	if (pe_sec[i].PointerToRawData+pe_sec[i].SizeOfRawData > rawsize)
	    rawsize = pe_sec[i].PointerToRawData+pe_sec[i].SizeOfRawData;
    }
    if (GetFileInformationByHandle(hFile,&bhfi)) {
    	/* FIXME: 64 bit */
    	if (bhfi.nFileSizeLow < rawsize) {
	    ERR(win32,"PE module is too small (header: %d, filesize: %d), probably truncated download?\n",rawsize,bhfi.nFileSizeLow);
	    goto error;
	}
    }
    /* Else ... Hmm, we have opened it, so we should be able to get info?
     * Anyway, don't care in this case
     */
    return hModule;

error:
    UnmapViewOfFile( (LPVOID)hModule );
    return 0;
}

/**********************************************************************
 * This maps a loaded PE dll into the address space of the specified process.
 */
static BOOL32 PE_MapImage( PDB32 *process,WINE_MODREF *wm, OFSTRUCT *ofs, DWORD flags )
{
	PE_MODREF		*pem;
	int			i, result;
	DWORD			load_addr;
	IMAGE_DATA_DIRECTORY	dir;
	char			*modname;
	int			vma_size;
	HMODULE32		hModule = wm->module;

        IMAGE_SECTION_HEADER *pe_seg;
        IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)hModule;
        IMAGE_NT_HEADERS *nt_header = PE_HEADER(hModule);
	
	pem 	= &(wm->binfmt.pe);

	result = GetLongPathName32A(ofs->szPathName,NULL,0);
	wm->longname = (char*)HeapAlloc(process->heap,0,result+1);
	GetLongPathName32A(ofs->szPathName,wm->longname,result+1);

	wm->shortname = HEAP_strdupA(process->heap,0,ofs->szPathName);

	if (!(nt_header->FileHeader.Characteristics & IMAGE_FILE_DLL))
        {
		if (process->exe_modref)
			FIXME(win32,"overwriting old exe_modref... arrgh\n");
		process->exe_modref = wm;
	}

	load_addr = nt_header->OptionalHeader.ImageBase;
	vma_size = calc_vma_size( hModule );
	TRACE(win32, "Load addr is %lx\n",load_addr);
	load_addr = (DWORD)VirtualAlloc( (void*)load_addr, vma_size,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_EXECUTE_READWRITE );
	if (load_addr == 0) {
		load_addr = (DWORD)VirtualAlloc( NULL, vma_size,
						 MEM_RESERVE | MEM_COMMIT,
						 PAGE_EXECUTE_READWRITE );
	}
	/* NOTE: this changes a value in the process modref chain, which can
	 * be accessed independently from this function
	 */
	wm->module = (HMODULE32)load_addr;

	TRACE(win32, "Load addr is really %lx, range %x\n",
                      load_addr, vma_size);

	TRACE(segment, "Loading %s at %lx, range %x\n",
              ofs->szPathName, load_addr, vma_size );
	
        /* Store the NT header at the load addr
         * (FIXME: should really use mmap)
         */
        *(IMAGE_DOS_HEADER *)load_addr = *dos_header;
        *(IMAGE_NT_HEADERS *)(load_addr + dos_header->e_lfanew) = *nt_header;
	memcpy(PE_SECTIONS(load_addr),PE_SECTIONS(hModule),sizeof(IMAGE_SECTION_HEADER)*nt_header->FileHeader.NumberOfSections);
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
			pem->pe_import = (PIMAGE_IMPORT_DESCRIPTOR) result;

		if(strcmp(pe_seg->Name, ".edata") == 0)
			pem->pe_export = (PIMAGE_EXPORT_DIRECTORY) result;

		if(strcmp(pe_seg->Name, ".rsrc") == 0)
			pem->pe_resource = (PIMAGE_RESOURCE_DIRECTORY) result;

		if(strcmp(pe_seg->Name, ".reloc") == 0)
			pem->pe_reloc = (PIMAGE_BASE_RELOCATION) result;
	}

	/* There is word that the actual loader does not care about the
	   section names, and only goes for the DataDirectory */
	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if(dir.Size)
	{
		if(pem->pe_export && (int)pem->pe_export!=RVA(dir.VirtualAddress))
			WARN(win32,"wrong export directory??\n");
		/* always trust the directory */
		pem->pe_export = (PIMAGE_EXPORT_DIRECTORY) RVA(dir.VirtualAddress);
	}

	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if(dir.Size)
	{
		/* 
		if(pem->pe_import && (int)pem->pe_import!=RVA(dir.VirtualAddress))
			WARN(win32,"wrong import directory??\n");
		 */
		pem->pe_import = (PIMAGE_IMPORT_DESCRIPTOR) RVA(dir.VirtualAddress);
	}

	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
	if(dir.Size)
	{
		if(pem->pe_resource && (int)pem->pe_resource!=RVA(dir.VirtualAddress))
			WARN(win32,"wrong resource directory??\n");
		pem->pe_resource = (PIMAGE_RESOURCE_DIRECTORY) RVA(dir.VirtualAddress);
	}

	if(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size)
		FIXME(win32,"Exception directory ignored\n");

	if(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size)
		FIXME(win32,"Security directory ignored\n");



	dir=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if(dir.Size)
	{
		if(pem->pe_reloc && (int)pem->pe_reloc!= RVA(dir.VirtualAddress))
			WARN(win32,"wrong relocation list??\n");
		pem->pe_reloc = (void *) RVA(dir.VirtualAddress);
	}

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size)
		FIXME(win32,"Copyright string ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size)
		FIXME(win32,"Global Pointer (MIPS) ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size)
		FIXME(win32,"Load Configuration directory ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size)
		TRACE(win32,"Bound Import directory ignored\n");

	if(nt_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_IAT].Size)
		TRACE(win32,"Import Address Table directory ignored\n");
	if(nt_header->OptionalHeader.DataDirectory[13].Size)
		FIXME(win32,"Unknown directory 13 ignored\n");
	if(nt_header->OptionalHeader.DataDirectory[14].Size)
		FIXME(win32,"Unknown directory 14 ignored\n");
	if(nt_header->OptionalHeader.DataDirectory[15].Size)
		FIXME(win32,"Unknown directory 15 ignored\n");

	if(pem->pe_reloc)	do_relocations(wm);
	if(pem->pe_export) {
		dump_exports(wm->module);

		wm->modname = HEAP_strdupA(process->heap,0,(char*)RVA(pem->pe_export->Name));
	} else {
		/* try to find out the name from the OFSTRUCT */
		char *s;
		modname = s = ofs->szPathName;
		while ((s=strchr(modname,'\\')))
			modname = s+1;
		wm->modname = HEAP_strdupA(process->heap,0,modname);
	}
	if(pem->pe_import)	{
		if (fixup_imports(process,wm)) {
			WINE_MODREF	**xwm;

			/* remove entry from modref chain */
			xwm = &(process->modref_list);
			while (*xwm) {
				if (*xwm==wm) {
					*xwm = wm->next;
					break;
				}
				xwm = &((*xwm)->next);
			}
			/* FIXME: there are several more dangling references
			 * left. Including dlls loaded by this dll before the
			 * failed one. Unrolling is rather difficult with the
			 * current structure and we can leave it them lying
			 * around with no problems, so we don't care
			 */
			return 0;
		}
	}
  		
        /* Now that we got everything at the right address,
         * we can unmap the previous module */
        UnmapViewOfFile( (LPVOID)hModule );
        return 1;
}

/******************************************************************************
 * The PE Library Loader frontend. 
 * FIXME: handle the flags.
 *        internal module handling should be made better here (and in builtin.c)
 */
HMODULE32 PE_LoadLibraryEx32A (LPCSTR name, PDB32 *process,
                               HFILE32 hFile, DWORD flags)
{
	OFSTRUCT	ofs;
	HMODULE32	hModule;
	NE_MODULE	*pModule;
	WINE_MODREF	*wm;

	if ((hModule = MODULE_FindModule32( process, name ))) {
		for (wm= process->modref_list;wm;wm=wm->next)
			if (wm->module == hModule)
				return hModule;
		/* Since MODULE_FindModule32 uses the modref chain too, the
		 * module MUST have been found above. If not, something has gone
		 * terribly wrong.
		 */
		assert(0);
	}
	/* try to load builtin, enabled modules first */
	if ((hModule = BUILTIN32_LoadModule( name, FALSE, process )))
	    return hModule;

	/* try to load the specified dll/exe */
	if (HFILE_ERROR32==(hFile=OpenFile32(name,&ofs,OF_READ))) {
		/* Now try the built-in even if disabled */
		if ((hModule = BUILTIN32_LoadModule( name, TRUE, process ))) {
		    WARN( module, "Could not load external DLL '%s', using built-in module.\n", name );
		    return hModule;
		}
		return 0;
	}
	/* will go away ... */
	if ((hModule = MODULE_CreateDummyModule( &ofs )) < 32) {
		_lclose32(hFile);
		return hModule;
	}
	pModule         = (NE_MODULE *)GlobalLock16( hModule );
	pModule->flags  = NE_FFLAGS_WIN32;
	/* .. */

	wm=(WINE_MODREF*)HeapAlloc(process->heap,HEAP_ZERO_MEMORY,sizeof(*wm));
	wm->type = MODULE32_PE;
	/* NOTE: fixup_imports takes care of the correct order */
	wm->next = process->modref_list;
	process->modref_list = wm;

	wm->module = pModule->module32 = PE_LoadImage( hFile );

	CloseHandle( hFile );
        if (wm->module < 32) 
        {
	    process->modref_list = wm->next;
	    FreeLibrary16( hModule);
	    HeapFree(process->heap,0,wm);
	    ERR(win32,"can't load %s\n",ofs.szPathName);
            return 0;
        }

	/* (possible) recursion */
	if (!PE_MapImage(process,wm,&ofs,flags)) {
	    /* ERROR cleanup ... */
	    WINE_MODREF	**xwm;

	    ERR(win32,"couldn't load %s\n",ofs.szPathName);
	    /* unlink from process modref chain */
	    for (    xwm=&(process->modref_list);
		     *xwm && (*xwm!=wm);
		     xwm=&((*xwm)->next)
	    ) /* EMPTY */;
	    if (*xwm)
	    	*xwm=(*xwm)->next;
	    	
	    return 0;
	}
        pModule->module32 = wm->module;
	if (wm->binfmt.pe.pe_export)
		SNOOP_RegisterDLL(wm->module,wm->modname,wm->binfmt.pe.pe_export->NumberOfFunctions);
	return wm->module;
}

/*****************************************************************************
 * Load the PE main .EXE. All other loading is done by PE_LoadLibraryEx32A
 * FIXME: this function should use PE_LoadLibraryEx32A, but currently can't
 * due to the PROCESS_Create stuff.
 */
HINSTANCE16 PE_CreateProcess( LPCSTR name, LPCSTR cmd_line,
                              LPCSTR env, LPSTARTUPINFO32A startup,
                              LPPROCESS_INFORMATION info )
{
    HMODULE16 hModule16;
    HMODULE32 hModule32;
    HINSTANCE16 hInstance;
    NE_MODULE *pModule;
    HFILE32 hFile;
    OFSTRUCT ofs;
    PDB32 *process;
    TDB *pTask;
    WINE_MODREF	*wm;

    if ((hFile = OpenFile32( name, &ofs, OF_READ )) == HFILE_ERROR32)
        return 2;  /* File not found */

    if ((hModule16 = MODULE_CreateDummyModule( &ofs )) < 32) return hModule16;
    pModule = (NE_MODULE *)GlobalLock16( hModule16 );
    pModule->flags = NE_FFLAGS_WIN32;

    pModule->module32 = hModule32 = PE_LoadImage( hFile );
    if (hModule32 < 32) return 21;

    if (PE_HEADER(hModule32)->FileHeader.Characteristics & IMAGE_FILE_DLL)
        return 11;

    hInstance = NE_CreateInstance( pModule, NULL, FALSE );
    process = PROCESS_Create( pModule, cmd_line, env,
                              hInstance, 0, startup, info );
    pTask = (TDB *)GlobalLock16( process->task );

    wm=(WINE_MODREF*)HeapAlloc(process->heap,HEAP_ZERO_MEMORY,sizeof(*wm));
    wm->type = MODULE32_PE;
    /* NOTE: fixup_imports takes care of the correct order */
    wm->next = process->modref_list;
    wm->module = hModule32;
    process->modref_list = wm;
    if (!PE_MapImage( process, wm, &ofs, 0 ))
    {
     	/* FIXME: should destroy the task created and free referenced stuff */
        return 0;
    }
    pModule->module32 = wm->module;

    /* FIXME: Yuck. Is there no other good place to do that? */
    PE_InitTls( pTask->thdb );

    return hInstance;
}

/*********************************************************************
 * PE_UnloadImage [internal]
 */
int PE_UnloadImage( HMODULE32 hModule )
{
	FIXME(win32,"stub.\n");
	/* free resources, image, unmap */
	return 1;
}

/* Called if the library is loaded or freed.
 * NOTE: if a thread attaches a DLL, the current thread will only do
 * DLL_PROCESS_ATTACH. Only new created threads do DLL_THREAD_ATTACH
 * (SDK)
 */
void PE_InitDLL(WINE_MODREF *wm, DWORD type, LPVOID lpReserved)
{
    if (wm->type!=MODULE32_PE)
    	return;
    if (wm->binfmt.pe.flags & PE_MODREF_NO_DLL_CALLS)
    	return;
    if (type==DLL_PROCESS_ATTACH)
    {
        if (wm->binfmt.pe.flags & PE_MODREF_PROCESS_ATTACHED)
            return;

        wm->binfmt.pe.flags |= PE_MODREF_PROCESS_ATTACHED;
    }

    /*  DLL_ATTACH_PROCESS:
     *		lpreserved is NULL for dynamic loads, not-NULL for static loads
     *  DLL_DETACH_PROCESS:
     *		lpreserved is NULL if called by FreeLibrary, not-NULL otherwise
     *  the SDK doesn't mention anything for DLL_THREAD_*
     */
        
    /* Is this a library? And has it got an entrypoint? */
    if ((PE_HEADER(wm->module)->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        (PE_HEADER(wm->module)->OptionalHeader.AddressOfEntryPoint)
    ) {
        DLLENTRYPROC32 entry = (void*)RVA_PTR( wm->module,OptionalHeader.AddressOfEntryPoint );
        TRACE(relay, "CallTo32(entryproc=%p,module=%08x,type=%ld,res=%p)\n",
                       entry, wm->module, type, lpReserved );

        entry( wm->module, type, lpReserved );
    }
}

/************************************************************************
 *	PE_InitTls			(internal)
 *
 * If included, initialises the thread local storages of modules.
 * Pointers in those structs are not RVAs but real pointers which have been
 * relocated by do_relocations() already.
 */
void PE_InitTls(THDB *thdb)
{
	WINE_MODREF		*wm;
	PE_MODREF		*pem;
	IMAGE_NT_HEADERS	*peh;
	DWORD			size,datasize;
	LPVOID			mem;
	PIMAGE_TLS_DIRECTORY	pdir;
	PDB32			*pdb = thdb->process;
        int delta;
	
	for (wm = pdb->modref_list;wm;wm=wm->next) {
		if (wm->type!=MODULE32_PE)
			continue;
		pem = &(wm->binfmt.pe);
		peh = PE_HEADER(wm->module);
		delta = wm->module - peh->OptionalHeader.ImageBase;
		if (!peh->OptionalHeader.DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress)
			continue;
		FIXME(win32,"%s has TLS directory.\n",wm->longname);
		pdir = (LPVOID)(wm->module + peh->OptionalHeader.
			DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress);
		
		
		if (!(pem->flags & PE_MODREF_TLS_ALLOCED)) {
			pem->tlsindex = THREAD_TlsAlloc(thdb);
			*pdir->AddressOfIndex=pem->tlsindex;   
		}
		pem->flags |= PE_MODREF_TLS_ALLOCED;
		datasize= pdir->EndAddressOfRawData-pdir->StartAddressOfRawData;
		size	= datasize + pdir->SizeOfZeroFill;
		mem=VirtualAlloc(0,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		memcpy(mem,(LPVOID)pdir->StartAddressOfRawData,datasize);
		if (pdir->AddressOfCallBacks) {
		     PIMAGE_TLS_CALLBACK *cbs = 
		       (PIMAGE_TLS_CALLBACK *)pdir->AddressOfCallBacks;

		     if (*cbs)
		       FIXME(win32, "TLS Callbacks aren't going to be called\n");
		}
		/* Don't use TlsSetValue, we are in the wrong thread */
		thdb->tls_array[pem->tlsindex] = mem;
	}
}

/****************************************************************************
 *		DisableThreadLibraryCalls (KERNEL32.74)
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
 */
BOOL32 WINAPI DisableThreadLibraryCalls(HMODULE32 hModule)
{
	WINE_MODREF	*wm;

	for (wm=PROCESS_Current()->modref_list;wm;wm=wm->next)
		if ((wm->module == hModule) && (wm->type==MODULE32_PE))
			wm->binfmt.pe.flags|=PE_MODREF_NO_DLL_CALLS;
	return TRUE;
}

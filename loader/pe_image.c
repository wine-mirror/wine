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
#include "neexe.h"
#include "peexe.h"
#include "process.h"
#include "pe_image.h"
#include "module.h"
#include "global.h"
#include "task.h"
#include "ldt.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#ifndef WINELIB
#include "debugger.h"
#endif

static void PE_InitDLL(PE_MODREF* modref, DWORD type, LPVOID lpReserved);

/* convert PE image VirtualAddress to Real Address */
#define RVA(x) ((unsigned int)load_addr+(unsigned int)(x))

void dump_exports(IMAGE_EXPORT_DIRECTORY * pe_exports, unsigned int load_addr)
{ 
#ifndef WINELIB
  char		*Module;
  int		i;
  u_short	*ordinal;
  u_long	*function,*functions;
  u_char	**name,*ename;
  char		buffer[1000];
  DBG_ADDR	daddr;

  daddr.seg = 0;
  daddr.type = NULL;
  Module = (char*)RVA(pe_exports->Name);
  dprintf_win32(stddeb,"\n*******EXPORT DATA*******\nModule name is %s, %ld functions, %ld names\n", 
	 Module,
	 pe_exports->NumberOfFunctions,
	 pe_exports->NumberOfNames);

  ordinal=(u_short*) RVA(pe_exports->AddressOfNameOrdinals);
  functions=function=(u_long*) RVA(pe_exports->AddressOfFunctions);
  name=(u_char**) RVA(pe_exports->AddressOfNames);

  dprintf_win32(stddeb,"%-32s Ordinal Virt Addr\n", "Function Name");
  for (i=0;i<pe_exports->NumberOfFunctions;i++) {
      if (i<pe_exports->NumberOfNames) {
	  ename=(char*)RVA(*name++);
	  dprintf_win32(stddeb,"%-32s %4d    %8.8lx (%8.8lx)\n",ename,*ordinal,functions[*ordinal],*function);
	  sprintf(buffer,"%s_%s",Module,ename);
	  daddr.off=RVA(functions[*ordinal]);
	  ordinal++;
	  function++;
      } else {
      	  /* ordinals/names no longer valid, but we still got functions */
	  dprintf_win32(stddeb,"%-32s %4s    %8s %8.8lx\n","","","",*function);
	  sprintf(buffer,"%s_%d",Module,i);
	  daddr.off=RVA(*functions);
	  function++;
      }
      DEBUG_AddSymbol(buffer,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
  }
#endif
}

/* Look up the specified function or ordinal in the exportlist:
 * If it is a string:
 * 	- look up the name in the Name list. 
 *	- look up the ordinal with that index.
 *	- use the ordinal as offset into the functionlist
 * If it is a ordinal:
 *	- use ordinal-pe_export->Base as offset into the functionlist
 */
FARPROC32 PE_FindExportedFunction(struct pe_data *pe, LPCSTR funcName)
{
	IMAGE_EXPORT_DIRECTORY 		*exports;
	unsigned			load_addr;
	u_short				* ordinal;
	u_long				* function;
	u_char				** name, *ename;
	int				i;
	PDB32				*process=(PDB32*)GetCurrentProcessId();
	PE_MODREF			*pem;

	pem = process->modref_list;
	while (pem && (pem->pe_module != pe))
		pem=pem->next;
	if (!pem) {
		fprintf(stderr,"No MODREF found for PE_MODULE %p in process %p\n",pe,process);
		return NULL;
	}
	load_addr	= pem->load_addr;
	exports		= pem->pe_export;

	if (HIWORD(funcName))
		dprintf_win32(stddeb,"PE_FindExportedFunction(%s)\n",funcName);
	else
		dprintf_win32(stddeb,"PE_FindExportedFunction(%d)\n",(int)funcName);
	if (!exports) {
		fprintf(stderr,"Module %p/MODREF %p doesn't have a exports table.\n",pe,pem);
		return NULL;
	}
	ordinal	= (u_short*)  RVA(exports->AddressOfNameOrdinals);
	function= (u_long*)   RVA(exports->AddressOfFunctions);
	name	= (u_char **) RVA(exports->AddressOfNames);

	if (HIWORD(funcName)) {
		for(i=0; i<exports->NumberOfNames; i++) {
			ename=(char*)RVA(*name);
			if(!strcmp(ename,funcName))
				return (FARPROC32) RVA(function[*ordinal]);
			ordinal++;
			name++;
		}
	} else {
		if (LOWORD(funcName)-exports->Base > exports->NumberOfFunctions) {
			dprintf_win32(stddeb,"	ordinal %d out of range!\n",
                                      LOWORD(funcName));
			return NULL;
		}
		return (FARPROC32) RVA(function[(int)funcName-exports->Base]);
	}
	return NULL;
}

void 
fixup_imports (PDB32 *process,PE_MODREF *pem)
{
    PE_MODULE			*pe = pem->pe_module;
    IMAGE_IMPORT_DESCRIPTOR	*pe_imp;
    int	fixup_failed		= 0;
    unsigned int load_addr	= pem->load_addr;
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
	    char *p, buffer[256];

	    /* Try with prepending the path of the current module */
	    GetModuleFileName32A (pe->mappeddll, buffer, sizeof (buffer));
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
		if (xpem->pe_module->mappeddll == res)
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
	int			ordimportwarned;

        ordimportwarned = 0;
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

		    if(!lstrncmpi32A(Module,"kernel32",8) && !ordimportwarned){
		       fprintf(stderr,"%s imports kernel32.dll by ordinal. May crash.\n",modname);
		       ordimportwarned = 1;
		    }
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


		    if (!lstrncmpi32A(Module,"kernel32",8) && 
		    	!ordimportwarned
		    ) {
		       fprintf(stderr,"%s imports kernel32.dll by ordinal. May crash.\n",modname);
		       ordimportwarned = 1;
		    }
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

static int calc_vma_size(struct pe_data *pe)
{
  int i,vma_size = 0;

  dprintf_win32(stddeb, "Dump of segment table\n");
  dprintf_win32(stddeb, "   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
  for(i=0; i< pe->pe_header->FileHeader.NumberOfSections; i++)
    {
      dprintf_win32(stddeb, "%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
	     pe->pe_seg[i].Name, 
	     pe->pe_seg[i].Misc.VirtualSize,
	     pe->pe_seg[i].VirtualAddress,
	     pe->pe_seg[i].SizeOfRawData,
	     pe->pe_seg[i].PointerToRawData,
	     pe->pe_seg[i].PointerToRelocations,
	     pe->pe_seg[i].PointerToLinenumbers,
	     pe->pe_seg[i].NumberOfRelocations,
	     pe->pe_seg[i].NumberOfLinenumbers,
	     pe->pe_seg[i].Characteristics);
	  vma_size = MAX(vma_size,
	  		pe->pe_seg[i].VirtualAddress + 
			pe->pe_seg[i].SizeOfRawData);
    }
    return vma_size;
}

static void do_relocations(PE_MODREF *pem)
{
	int delta = pem->load_addr - pem->pe_module->pe_header->OptionalHeader.ImageBase;

	unsigned int			load_addr= pem->load_addr;
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
 * (at least) Linux does only support offset with are multiples of the
 * underlying filesystemblocksize, but PE DLLs usually have alignments of 512
 * byte. This fails for instance when you try to map from CDROM (bsize 2048).
 *
 * BUT we have to map the whole image anyway, for Win32 programs sometimes
 * want to access them. (HMODULE32 point to the start of it)
 */
static PE_MODULE *PE_LoadImage( int fd )
{
	struct pe_data		*pe;
	struct stat		stbuf;

	if (-1==fstat(fd,&stbuf)) {
		perror("PE_LoadImage:fstat");
		return NULL;
	}
	pe = xmalloc(sizeof(struct pe_data));
	memset(pe,0,sizeof(struct pe_data));

	/* map the PE image somewhere */
	pe->mappeddll = (HMODULE32)mmap(NULL,stbuf.st_size,PROT_READ,MAP_SHARED,fd,0);
	if (!pe->mappeddll || pe->mappeddll==-1) {
		if (errno==ENOEXEC) {
			int	res=0,curread = 0;

			lseek(fd,0,SEEK_SET);
			/* linux: some filesystems don't support mmap (samba,
			 * ntfs apparently) so we have to read the image the
			 * hard way
			 */
			pe->mappeddll = xmalloc(stbuf.st_size);
			while (curread < stbuf.st_size) {
				res = read(fd,pe->mappeddll+curread,stbuf.st_size-curread);
				if (res<=0) 
					break;
				curread+=res;
			}
			if (res == -1) {
				perror("PE_LoadImage:mmap compat read");
				free(pe->mappeddll);
				free(pe);
				return NULL;
			}

		} else {
			perror("PE_LoadImage:mmap");
			free(pe);
			return NULL;
		}
	}
	/* link PE header */
	pe->pe_header = (IMAGE_NT_HEADERS*)(pe->mappeddll+(((IMAGE_DOS_HEADER*)pe->mappeddll)->e_lfanew));
	if (pe->pe_header->Signature!=IMAGE_NT_SIGNATURE) {
		fprintf(stderr,"image doesn't have PE signature, but 0x%08lx\n",
			pe->pe_header->Signature
		);
		free(pe);
		return NULL;
	}

	if (pe->pe_header->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
		fprintf(stderr,"trying to load PE image for unsupported architecture (");
		switch (pe->pe_header->FileHeader.Machine) {
		case IMAGE_FILE_MACHINE_UNKNOWN:
			fprintf(stderr,"Unknown");break;
		case IMAGE_FILE_MACHINE_I860:
			fprintf(stderr,"I860");break;
		case IMAGE_FILE_MACHINE_R3000:
			fprintf(stderr,"R3000");break;
		case IMAGE_FILE_MACHINE_R4000:
			fprintf(stderr,"R4000");break;
		case IMAGE_FILE_MACHINE_R10000:
			fprintf(stderr,"R10000");break;
		case IMAGE_FILE_MACHINE_ALPHA:
			fprintf(stderr,"Alpha");break;
		case IMAGE_FILE_MACHINE_POWERPC:
			fprintf(stderr,"PowerPC");break;
		default:
			fprintf(stderr,"Unknown-%04x",pe->pe_header->FileHeader.Machine);break;
		}
		fprintf(stderr,")\n");
		return NULL;
	}
	pe->pe_seg = (IMAGE_SECTION_HEADER*)(((LPBYTE)(pe->pe_header+1))-
		 (16 - pe->pe_header->OptionalHeader.NumberOfRvaAndSizes) * sizeof(IMAGE_DATA_DIRECTORY));

/* FIXME: the (16-...) is a *horrible* hack to make COMDLG32.DLL load OK. The
 * problem needs to be fixed properly at some stage.
 */
 	return pe;
}

/**********************************************************************
 * This maps a loaded PE dll into the address space of the specified process.
 */
void
PE_MapImage(PE_MODULE *pe,PDB32 *process, OFSTRUCT *ofs, DWORD flags) {
	PE_MODREF		*pem;
	int			i, result;
	int			load_addr;
	IMAGE_DATA_DIRECTORY	dir;
	char			buffer[200];
	char			*modname;
	int			vma_size;
	
	pem		= (PE_MODREF*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*pem));
	/* NOTE: fixup_imports takes care of the correct order */
	pem->next	= process->modref_list;
	process->modref_list = pem;

	pem->pe_module	= pe;
	if (!(pe->pe_header->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
		if (process->exe_modref)
			fprintf(stderr,"overwriting old exe_modref... arrgh\n");
		process->exe_modref = pem;
	}

	load_addr 	= pe->pe_header->OptionalHeader.ImageBase;
	dprintf_win32(stddeb, "Load addr is %x\n",load_addr);
	vma_size = calc_vma_size(pe);
	load_addr 	= (int) VirtualAlloc( (void*)load_addr, vma_size, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE );
        pem->load_addr	= load_addr;

	dprintf_win32(stddeb, "Load addr is really %lx, range %x\n",
		pem->load_addr, vma_size);
	

	for(i=0; i < pe->pe_header->FileHeader.NumberOfSections; i++)
	{
		/* memcpy only non-BSS segments */
		/* FIXME: this should be done by mmap(..MAP_PRIVATE|MAP_FIXED..)
		 * but it is not possible for (at least) Linux needs an offset
		 * aligned to a block on the filesystem.
		 */
		if(!(pe->pe_seg[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
		    memcpy((char*)RVA(pe->pe_seg[i].VirtualAddress),
		    	(char*)(pe->mappeddll+pe->pe_seg[i].PointerToRawData),
			pe->pe_seg[i].SizeOfRawData
		    );

		result = RVA (pe->pe_seg[i].VirtualAddress);
#if 1
		/* not needed, memory is zero */
		if(strcmp(pe->pe_seg[i].Name, ".bss") == 0)
		    memset((void *)result, 0, 
			   pe->pe_seg[i].Misc.VirtualSize ?
			   pe->pe_seg[i].Misc.VirtualSize :
			   pe->pe_seg[i].SizeOfRawData);
#endif

		if(strcmp(pe->pe_seg[i].Name, ".idata") == 0)
			pem->pe_import = (LPIMAGE_IMPORT_DESCRIPTOR) result;

		if(strcmp(pe->pe_seg[i].Name, ".edata") == 0)
			pem->pe_export = (LPIMAGE_EXPORT_DIRECTORY) result;

		if(strcmp(pe->pe_seg[i].Name, ".rsrc") == 0)
			pem->pe_resource = (LPIMAGE_RESOURCE_DIRECTORY) result;

		if(strcmp(pe->pe_seg[i].Name, ".reloc") == 0)
			pem->pe_reloc = (LPIMAGE_BASE_RELOCATION) result;
	}

	/* There is word that the actual loader does not care about the
	   section names, and only goes for the DataDirectory */
	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if(dir.Size)
	{
		if(pem->pe_export && (int)pem->pe_export!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong export directory??\n");
		/* always trust the directory */
		pem->pe_export = (LPIMAGE_EXPORT_DIRECTORY) RVA(dir.VirtualAddress);
	}

	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if(dir.Size)
	{
		if(pem->pe_import && (int)pem->pe_import!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong import directory??\n");
		pem->pe_import = (LPIMAGE_IMPORT_DESCRIPTOR) RVA(dir.VirtualAddress);
	}

	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
	if(dir.Size)
	{
		if(pem->pe_resource && (int)pem->pe_resource!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong resource directory??\n");
		pem->pe_resource = (LPIMAGE_RESOURCE_DIRECTORY) RVA(dir.VirtualAddress);
	}

	if(pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size)
		dprintf_win32(stdnimp,"Exception directory ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size)
		dprintf_win32(stdnimp,"Security directory ignored\n");



	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if(dir.Size)
	{
		if(pem->pe_reloc && (int)pem->pe_reloc!= RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong relocation list??\n");
		pem->pe_reloc = (void *) RVA(dir.VirtualAddress);
	}

#ifndef WINELIB
	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_DEBUG].Size)
	  {
	    DEBUG_RegisterDebugInfo(pe, load_addr, 
			pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
			pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
	  }
#endif

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size)
		dprintf_win32(stdnimp,"Copyright string ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size)
		dprintf_win32(stdnimp,"Global Pointer (MIPS) ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_TLS].Size)
		 fprintf(stdnimp,"Thread local storage ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size)
		dprintf_win32(stdnimp,"Load Configuration directory ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size)
		dprintf_win32(stdnimp,"Bound Import directory ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_IAT].Size)
		dprintf_win32(stdnimp,"Import Address Table directory ignored\n");
	if(pe->pe_header->OptionalHeader.DataDirectory[13].Size)
		dprintf_win32(stdnimp,"Unknown directory 13 ignored\n");
	if(pe->pe_header->OptionalHeader.DataDirectory[14].Size)
		dprintf_win32(stdnimp,"Unknown directory 14 ignored\n");
	if(pe->pe_header->OptionalHeader.DataDirectory[15].Size)
		dprintf_win32(stdnimp,"Unknown directory 15 ignored\n");

	if(pem->pe_reloc)	do_relocations(pem);
	if(pem->pe_export)	dump_exports(pem->pe_export,load_addr);
	if(pem->pe_import)	fixup_imports(process,pem);
  		
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

#ifndef WINELIB
        {
            DBG_ADDR daddr = { NULL, 0, 0 };
            /* add start of sections as debugsymbols */
            for(i=0;i<pe->pe_header->FileHeader.NumberOfSections;i++) {
		sprintf(buffer,"%s_%s",modname,pe->pe_seg[i].Name);
		daddr.off= RVA(pe->pe_seg[i].VirtualAddress);
		DEBUG_AddSymbol(buffer,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
            }
            /* add entry point */
            sprintf(buffer,"%s_EntryPoint",modname);
            daddr.off=RVA(pe->pe_header->OptionalHeader.AddressOfEntryPoint);
            DEBUG_AddSymbol(buffer,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
            /* add start of DLL */
            daddr.off=load_addr;
            DEBUG_AddSymbol(modname,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
        }
#endif
}

HINSTANCE16 MODULE_CreateInstance(HMODULE16 hModule,LOADPARAMS *params);

/******************************************************************************
 * The PE Library Loader frontend. 
 * FIXME: handle the flags.
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
		pem 	= ((PDB32*)GetCurrentProcessId())->modref_list;
		while (pem) {
			if (pem->pe_module->mappeddll == hModule)
				return hModule;
			pem = pem->next;
		}
		pModule = MODULE_GetPtr(hModule);
	} else {

		/* try to load builtin, enabled modules first */
		if ((hModule = BUILTIN_LoadModule( name, FALSE )))
			return hModule;

		/* try to open the specified file */
		if (HFILE_ERROR32==(hFile=OpenFile32(name,&ofs,OF_READ))) {
			/* Now try the built-in even if disabled */
			if ((hModule = BUILTIN_LoadModule( name, TRUE ))) {
				fprintf( stderr, "Warning: could not load Windows DLL '%s', using built-in module.\n", name );
				return hModule;
			}
			return 1;
		}
		if ((hModule = MODULE_CreateDummyModule( &ofs )) < 32) {
			_lclose32(hFile);
			return hModule;
		}
		pModule		= (NE_MODULE *)GlobalLock16( hModule );
		pModule->flags	= NE_FFLAGS_WIN32;
		pModule->pe_module = PE_LoadImage( FILE_GetUnixHandle(hFile) );
		_lclose32(hFile);
		if (!pModule->pe_module)
			return 21;
	}
	/* recurse */
	PE_MapImage(pModule->pe_module,(PDB32*)GetCurrentProcessId(),&ofs,flags);
	return pModule->pe_module->mappeddll;
}

/*****************************************************************************
 * Load the PE main .EXE. All other loading is done by PE_LoadLibraryEx32A
 * FIXME: this function should use PE_LoadLibraryEx32A, but currently can't
 * due to the TASK_CreateTask stuff.
 */
HINSTANCE16 PE_LoadModule( HFILE32 hFile, OFSTRUCT *ofs, LOADPARAMS* params )
{
    HMODULE16 hModule;
    HINSTANCE16 hInstance;
    NE_MODULE *pModule;

    if ((hModule = MODULE_CreateDummyModule( ofs )) < 32) return hModule;
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->flags = NE_FFLAGS_WIN32;

    pModule->pe_module = PE_LoadImage( FILE_GetUnixHandle(hFile) );
    _lclose32(hFile);
    if (!pModule->pe_module)
    	return 21;

    hInstance = MODULE_CreateInstance( hModule, params );
    if (!(pModule->pe_module->pe_header->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        TASK_CreateTask( hModule, hInstance, 0,
                         params->hEnvironment,
                         (LPSTR)PTR_SEG_TO_LIN( params->cmdLine ),
                         *((WORD*)PTR_SEG_TO_LIN(params->showCmd) + 1) );
    }
    PE_MapImage(pModule->pe_module,(PDB32*)GetCurrentProcessId(),ofs,0);
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
    PE_MODULE		*pe = pem->pe_module;
    unsigned int	load_addr = pem->load_addr;

    if (type==DLL_PROCESS_ATTACH)
	pem->flags |= PE_MODREF_PROCESS_ATTACHED;
#ifndef WINELIB
    if (Options.debug) {
            DBG_ADDR addr = { NULL, 0, RVA(pe->pe_header->OptionalHeader.AddressOfEntryPoint) };
            DEBUG_AddBreakpoint( &addr );
	    DEBUG_SetBreakpoints(TRUE);
    }
#endif

    /*  DLL_ATTACH_PROCESS:
     *		lpreserved is NULL for dynamic loads, not-NULL for static loads
     *  DLL_DETACH_PROCESS:
     *		lpreserved is NULL if called by FreeLibrary, not-NULL otherwise
     *  the SDK doesn't mention anything for DLL_THREAD_*
     */
        
    /* Is this a library? And has it got an entrypoint? */
    if (	(pe->pe_header->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
		(pe->pe_header->OptionalHeader.AddressOfEntryPoint)
    ) {
        FARPROC32 entry = (FARPROC32)RVA(pe->pe_header->OptionalHeader.AddressOfEntryPoint);
        dprintf_relay( stddeb, "CallTo32(entryproc=%p,module=%d,type=%ld,res=%p)\n",
                       entry, pe->mappeddll, type, lpReserved );
        entry( pe->mappeddll, type, lpReserved );
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
		peh = pem->pe_module->pe_header;
		if (!peh->OptionalHeader.DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress) {
			pem = pem->next;
			continue;
		}
		pdir = (LPVOID)(pem->load_addr + peh->OptionalHeader.
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
	PDB32	*process = (PDB32*)GetCurrentProcessId();
	PE_MODREF	*pem = process->modref_list;

	while (pem) {
		if (pem->pe_module->mappeddll == hModule)
			pem->flags|=PE_MODREF_NO_DLL_CALLS;
		pem = pem->next;
	}
	return TRUE;
}


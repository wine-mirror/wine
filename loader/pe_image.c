#ifndef WINELIB
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "windows.h"
#include "winbase.h"
#include "callback.h"
#include "neexe.h"
#include "peexe.h"
#include "pe_image.h"
#include "module.h"
#include "global.h"
#include "task.h"
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"
#include "debugger.h"
#include "xmalloc.h"

/* convert PE image VirtualAddress to Real Address */
#define RVA(x) ((unsigned int)load_addr+(unsigned int)(x))

void dump_exports(IMAGE_EXPORT_DIRECTORY * pe_exports, unsigned int load_addr)
{ 
  char		*Module,*s;
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
      while ((s=strchr(buffer,'.'))) *s='_';
      DEBUG_AddSymbol(buffer,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
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
FARPROC32 PE_FindExportedFunction(struct pe_data *pe, LPCSTR funcName)
{
	IMAGE_EXPORT_DIRECTORY * exports = pe->pe_export;
	unsigned load_addr = pe->load_addr;
	u_short * ordinal;
	u_long * function;
	u_char ** name, *ename;
	int i;

	if (HIWORD(funcName))
		dprintf_win32(stddeb,"PE_FindExportedFunction(%s)\n",funcName);
	else
		dprintf_win32(stddeb,"PE_FindExportedFunction(%d)\n",(int)funcName);
	if (!exports)
		return NULL;
	ordinal=(u_short*) RVA(exports->AddressOfNameOrdinals);
	function=(u_long*) RVA(exports->AddressOfFunctions);
	name=(u_char **) RVA(exports->AddressOfNames);
	if (HIWORD(funcName)) {
		for(i=0; i<exports->NumberOfNames; i++) {
			ename=(char*) RVA(*name);
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
fixup_imports (struct pe_data *pe, HMODULE16 hModule)
{
    IMAGE_IMPORT_DESCRIPTOR *pe_imp;
    int	fixup_failed = 0;
    unsigned int load_addr = pe->load_addr;
    int i;
    NE_MODULE *ne_mod;
    HMODULE16 *mod_ptr;
    char *modname;
    
    if (pe->pe_export)
    	modname = (char*) RVA(pe->pe_export->Name);
    else
        modname = "<unknown>";

    /* OK, now dump the import list */
    dprintf_win32 (stddeb, "\nDumping imports list\n");

    /* first, count the number of imported non-internal modules */
    pe_imp = pe->pe_import;

    /* FIXME: should terminate on 0 Characteristics */
    for (i = 0; pe_imp->Name; pe_imp++)
	i++;

    /* Now, allocate memory for dlls_to_init */
    ne_mod = GlobalLock16 (hModule);
    ne_mod->dlls_to_init = GLOBAL_Alloc(GMEM_ZEROINIT, (i+1)*sizeof(HMODULE16),
                                        hModule, FALSE, FALSE, FALSE);
    mod_ptr = GlobalLock16 (ne_mod->dlls_to_init);
    /* load the modules and put their handles into the list */
 
     /* FIXME: should terminate on 0 Characteristics */
     for (i = 0, pe_imp = pe->pe_import; pe_imp->Name; pe_imp++) {
 	char *name = (char *) RVA(pe_imp->Name);
	mod_ptr[i] = MODULE_Load( name, (LPVOID)-1, FALSE );
	if (mod_ptr[i] <= (HMODULE16) 32) {
	    char *p, buffer[256];

	    /* Try with prepending the path of the current module */
	    GetModuleFileName16 (hModule, buffer, sizeof (buffer));
	    if (!(p = strrchr (buffer, '\\')))
		p = buffer;
	    strcpy (p + 1, name);
	    mod_ptr[i] = MODULE_Load( buffer, (LPVOID)-1, FALSE );
	}
	if (mod_ptr[i] <= (HMODULE16) 32) {
	    fprintf (stderr, "Module %s not found\n", name);
	    exit (0);
	}
	i++;
    }
    pe_imp = pe->pe_import;
    while (pe_imp->Name) {
	char			*Module;
	IMAGE_IMPORT_BY_NAME	*pe_name;
	LPIMAGE_THUNK_DATA	import_list,thunk_list;
	int			ordimportwarned;

        ordimportwarned = 0;
	Module = (char *) RVA(pe_imp->Name);
	dprintf_win32 (stddeb, "%s\n", Module);

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

static void calc_vma_size(struct pe_data *pe)
{
  int i;

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
	  pe->vma_size = MAX(pe->vma_size,
	  		pe->pe_seg[i].VirtualAddress + 
			pe->pe_seg[i].SizeOfRawData);
    }
}

static void do_relocations(struct pe_data *pe)
{
	int delta = pe->load_addr - pe->base_addr;
	unsigned int load_addr = pe->load_addr;
	IMAGE_BASE_RELOCATION	*r = pe->pe_reloc;
	int hdelta = (delta >> 16) & 0xFFFF;
	int ldelta = delta & 0xFFFF;

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
 * Load one PE format executable into memory
 */
static struct pe_data *PE_LoadImage( int fd, HMODULE16 hModule, WORD offset )
{
	struct pe_data		*pe;
	int			i, result;
	int			load_addr;
	IMAGE_DATA_DIRECTORY	dir;
	char			buffer[200];
	DBG_ADDR		daddr;

	daddr.seg=0;
	daddr.type = NULL;
	pe = xmalloc(sizeof(struct pe_data));
	memset(pe,0,sizeof(struct pe_data));
	pe->pe_header = xmalloc(sizeof(IMAGE_NT_HEADERS));

	/* read PE header */
	lseek( fd, offset, SEEK_SET);
	read( fd, pe->pe_header, sizeof(IMAGE_NT_HEADERS));

/* FIXME: this is a *horrible* hack to make COMDLG32.DLL load OK. The
problem needs to be fixed properly at some stage */

	if (pe->pe_header->OptionalHeader.NumberOfRvaAndSizes != 16) {
		printf("Short PE Header!!!\n");
		lseek( fd, -(16 - pe->pe_header->OptionalHeader.NumberOfRvaAndSizes) * sizeof(IMAGE_DATA_DIRECTORY), SEEK_CUR);
	}

/* horrible hack ends !!! */
	/* read sections */
	pe->pe_seg = xmalloc(sizeof(IMAGE_SECTION_HEADER) * 
				   pe->pe_header->FileHeader.NumberOfSections);
	read( fd, pe->pe_seg, sizeof(IMAGE_SECTION_HEADER) * 
			pe->pe_header->FileHeader.NumberOfSections);

	load_addr = pe->pe_header->OptionalHeader.ImageBase;
	pe->base_addr=load_addr;
	pe->vma_size=0;
	dprintf_win32(stddeb, "Load addr is %x\n",load_addr);
	calc_vma_size(pe);

#if 0
	/* We use malloc here, while a huge part of that address space does
	   not be supported by actual memory. It has to be contiguous, though.
	   I don't know if mmap("/dev/null"); would do any better.
	   What I'd really like to do is a Win32 style VirtualAlloc/MapViewOfFile
	   sequence */
	load_addr = pe->load_addr = (int)xmalloc(pe->vma_size);
	memset( load_addr, 0, pe->vma_size);
#else
	load_addr = (int) VirtualAlloc( NULL, pe->vma_size, MEM_COMMIT,
                                         PAGE_EXECUTE_READWRITE );
        pe->load_addr = load_addr;
#endif

	dprintf_win32(stddeb, "Load addr is really %x, range %x\n",
		pe->load_addr, pe->vma_size);
	

	for(i=0; i < pe->pe_header->FileHeader.NumberOfSections; i++)
	{
		/* load only non-BSS segments */
		if(pe->pe_seg[i].Characteristics & 
			~ IMAGE_SCN_CNT_UNINITIALIZED_DATA)
		if(lseek(fd,pe->pe_seg[i].PointerToRawData,SEEK_SET) == -1
		|| read(fd,(char*)RVA(pe->pe_seg[i].VirtualAddress),
			   pe->pe_seg[i].SizeOfRawData) != pe->pe_seg[i].SizeOfRawData)
		{
			fprintf(stderr,"Failed to load section %x\n", i);
			exit(0);
		}
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
			pe->pe_import = (LPIMAGE_IMPORT_DESCRIPTOR) result;

		if(strcmp(pe->pe_seg[i].Name, ".edata") == 0)
			pe->pe_export = (LPIMAGE_EXPORT_DIRECTORY) result;

		if(strcmp(pe->pe_seg[i].Name, ".rsrc") == 0)
			pe->pe_resource = (LPIMAGE_RESOURCE_DIRECTORY) result;

		if(strcmp(pe->pe_seg[i].Name, ".reloc") == 0)
			pe->pe_reloc = (LPIMAGE_BASE_RELOCATION) result;

	}

	/* There is word that the actual loader does not care about the
	   section names, and only goes for the DataDirectory */
	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if(dir.Size)
	{
		if(pe->pe_export && (int)pe->pe_export!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong export directory??\n");
		/* always trust the directory */
		pe->pe_export = (LPIMAGE_EXPORT_DIRECTORY) RVA(dir.VirtualAddress);
	}

	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if(dir.Size)
	{
		if(pe->pe_import && (int)pe->pe_import!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong import directory??\n");
		pe->pe_import = (LPIMAGE_IMPORT_DESCRIPTOR) RVA(dir.VirtualAddress);
	}

	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
	if(dir.Size)
	{
		if(pe->pe_resource && (int)pe->pe_resource!=RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong resource directory??\n");
		pe->pe_resource = (LPIMAGE_RESOURCE_DIRECTORY) RVA(dir.VirtualAddress);
	}

	if(pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size)
		dprintf_win32(stdnimp,"Exception directory ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size)
		dprintf_win32(stdnimp,"Security directory ignored\n");



	dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if(dir.Size)
	{
		if(pe->pe_reloc && (int)pe->pe_reloc!= RVA(dir.VirtualAddress))
			fprintf(stderr,"wrong relocation list??\n");
		pe->pe_reloc = (void *) RVA(dir.VirtualAddress);
	}

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_DEBUG].Size)
	  {
	    DEBUG_RegisterDebugInfo(fd, pe, load_addr, 
			pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
			pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
	  }

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size)
		dprintf_win32(stdnimp,"Copyright string ignored\n");

	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size)
		dprintf_win32(stdnimp,"Global Pointer (MIPS) ignored\n");

#ifdef NOT	/* we initialize this later */
	if(pe->pe_header->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_TLS].Size)
		 dprintf_win32(stdnimp,"Thread local storage ignored\n");
#endif

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

	if(pe->pe_reloc) do_relocations(pe);
	if(pe->pe_import) fixup_imports(pe, hModule);
	if(pe->pe_export) dump_exports(pe->pe_export,load_addr);
  		
	if (pe->pe_export) {
		char	*s;

		/* add start of sections as debugsymbols */
		for(i=0;i<pe->pe_header->FileHeader.NumberOfSections;i++) {
			sprintf(buffer,"%s_%s",
				(char*)RVA(pe->pe_export->Name),
				pe->pe_seg[i].Name
			);
			daddr.off= RVA(pe->pe_seg[i].VirtualAddress);
      			while ((s=strchr(buffer,'.'))) *s='_';
			DEBUG_AddSymbol(buffer,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
		}
		/* add entry point */
		sprintf(buffer,"%s_EntryPoint",(char*)RVA(pe->pe_export->Name));
		daddr.off=RVA(pe->pe_header->OptionalHeader.AddressOfEntryPoint);
      		while ((s=strchr(buffer,'.'))) *s='_';
		DEBUG_AddSymbol(buffer,&daddr, NULL, SYM_WIN32 | SYM_FUNC);
		/* add start of DLL */
		daddr.off=load_addr;
		DEBUG_AddSymbol((char*) RVA(pe->pe_export->Name),&daddr,
				NULL, SYM_WIN32 | SYM_FUNC);
	}
        return pe;
}

HINSTANCE16 MODULE_CreateInstance(HMODULE16 hModule,LOADPARAMS *params);

HINSTANCE16 PE_LoadModule( int fd, OFSTRUCT *ofs, LOADPARAMS* params )
{
    HMODULE16 hModule;
    HINSTANCE16 hInstance;
    NE_MODULE *pModule;
    struct mz_header_s mz_header;

    if ((hModule = MODULE_CreateDummyModule( ofs )) < 32) return hModule;
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->flags = NE_FFLAGS_WIN32;

    lseek( fd, 0, SEEK_SET );
    read( fd, &mz_header, sizeof(mz_header) );

    pModule->pe_module = PE_LoadImage( fd, hModule, mz_header.ne_offset );

    hInstance = MODULE_CreateInstance( hModule, params );

    if (!(pModule->pe_module->pe_header->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        TASK_CreateTask( hModule, hInstance, 0,
                         params->hEnvironment,
                         (LPSTR)PTR_SEG_TO_LIN( params->cmdLine ),
                         *((WORD*)PTR_SEG_TO_LIN(params->showCmd) + 1) );
    }
    return hInstance;
}

int PE_UnloadImage( HMODULE16 hModule )
{
	printf("PEunloadImage() called!\n");
	/* free resources, image, unmap */
	return 1;
}

static void PE_InitDLL(HMODULE16 hModule)
{
    NE_MODULE *pModule;
    PE_MODULE *pe;
    unsigned int load_addr;

    hModule = GetExePtr(hModule);
    if (!(pModule = MODULE_GetPtr(hModule))) return;
    if (!(pModule->flags & NE_FFLAGS_WIN32) || !(pe = pModule->pe_module))
        return;

    /* FIXME: What is the correct value for parameter 3? 
     *	      (the MSDN library JAN96 says 'reserved for future use')
     */
        
    /* Is this a library? And has it got an entrypoint? */
    if (	(pe->pe_header->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
		(pe->pe_header->OptionalHeader.AddressOfEntryPoint)
    ) {
        load_addr = pe->load_addr;
	printf("InitPEDLL() called!\n");
	CallDLLEntryProc32( 
	    (FARPROC32)RVA(pe->pe_header->OptionalHeader.AddressOfEntryPoint),
	    hModule,
	    DLL_PROCESS_ATTACH,
	    -1
	);
    }
}

void PE_InitializeDLLs(HMODULE16 hModule)
{
	NE_MODULE *pModule;
	HMODULE16 *pDLL;
	pModule = MODULE_GetPtr( GetExePtr(hModule) );
	if (pModule->dlls_to_init)
	{
		HGLOBAL16 to_init = pModule->dlls_to_init;
		pModule->dlls_to_init = 0;
		for (pDLL = (HMODULE16 *)GlobalLock16( to_init ); *pDLL; pDLL++)
		{
			PE_InitializeDLLs( *pDLL );
			PE_InitDLL( *pDLL );
		}
		GlobalFree16( to_init );
	}
	PE_InitDLL( hModule );
}

void PE_InitTls( PE_MODULE *module )
{
   /* FIXME: tls callbacks ??? */
   DWORD  index;
   DWORD  datasize;
   DWORD  size;
   LPVOID mem;
   LPIMAGE_TLS_DIRECTORY pdir;

    if (!module->pe_header->OptionalHeader.DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress)
        return;

    pdir = (LPVOID)(module->load_addr + module->pe_header->OptionalHeader.
               DataDirectory[IMAGE_FILE_THREAD_LOCAL_STORAGE].VirtualAddress);
    index = TlsAlloc();
    datasize = pdir->EndAddressOfRawData-pdir->StartAddressOfRawData;
    size     = datasize + pdir->SizeOfZeroFill;
        
    mem = VirtualAlloc(0,size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );
    
    memcpy(mem,(LPVOID) pdir->StartAddressOfRawData, datasize);
    TlsSetValue(index,mem);
    *(pdir->AddressOfIndex)=index;   
}

#endif /* WINELIB */

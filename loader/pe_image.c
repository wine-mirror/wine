#ifndef WINELIB
/* 
 *  Copyright	1994	Eric Youndale & Erik Bos
 *  Copyright	1995	Martin von Löwis
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
#include <sys/mman.h>
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
#include "registers.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

void my_wcstombs(char * result, u_short * source, int len)
{
  while(len--) {
    /* this used to be isascii, but see isascii implementation in Linux'
	   ctype.h */
    if(*source<255) *result++ = *source++;
    else {
      printf("Unable to handle unicode right now\n");
      exit(0);
    }
  };
}

#if 0
char * xmmap(char * vaddr, unsigned int v_size, unsigned int r_size,
	int prot, int flags, int fd, unsigned int file_offset)
{
  char * result;
  /* .bss has no associated storage in the PE file */
  if(r_size)
    v_size=r_size;
  else
#if defined(__svr4__) || defined(_SCO_DS)
    fprintf(stderr,"xmmap: %s line %d doesn't support MAP_ANON\n",__FILE__, __LINE__);
#else
    flags |= MAP_ANON;
#endif
  result = mmap(vaddr, v_size, prot, flags, fd, file_offset);
  if((unsigned int) result != 0xffffffff) return result;

  /* Sigh.  Alignment must be wrong for mmap.  Do this the hard way. */
  if(!(flags & MAP_FIXED)) {
    vaddr = (char *)0x40000000;
    flags |= MAP_FIXED;
  };

  mmap(vaddr, v_size, prot, MAP_ANONYMOUS | flags, 0, 0);
  lseek(fd, file_offset, SEEK_SET);
  read(fd, vaddr, v_size);
  return vaddr;
};
#endif

void dump_exports(struct PE_Export_Directory * pe_exports, unsigned int load_addr)
{ 
  char * Module;
  int i;
  u_short * ordinal;
  u_long * function;
  u_char ** name, *ename;

  Module = ((char *) load_addr) + pe_exports->Name;
  dprintf_win32(stddeb,"\n*******EXPORT DATA*******\nModule name is %s, %ld functions, %ld names\n", 
	 Module,
	 pe_exports->Number_Of_Functions,
	 pe_exports->Number_Of_Names);

  ordinal = (u_short *) (((char *) load_addr) + (int) pe_exports->Address_Of_Name_Ordinals);
  function = (u_long *)  (((char *) load_addr) + (int) pe_exports->AddressOfFunctions);
  name = (u_char **)  (((char *) load_addr) + (int) pe_exports->AddressOfNames);

  dprintf_win32(stddeb,"%-32s Ordinal Virt Addr\n", "Function Name");
  for(i=0; i< pe_exports->Number_Of_Functions; i++)
    {
      ename =  (char *) (((char *) load_addr) + (int) *name++);
      dprintf_win32(stddeb,"%-32s %4d    %8.8lx\n", ename, *ordinal++, *function++);
    }
}

static DWORD PE_FindExportedFunction(struct pe_data *pe, char* funcName)
{
	struct PE_Export_Directory * exports = pe->pe_export;
	unsigned load_addr = pe->load_addr;
	u_short * ordinal;
	u_long * function;
	u_char ** name, *ename;
	int i;
	if(!exports)return 0;
	ordinal = (u_short *) (((char *) load_addr) + (int) exports->Address_Of_Name_Ordinals);
	function = (u_long *)  (((char *) load_addr) + (int) exports->AddressOfFunctions);
	name = (u_char **)  (((char *) load_addr) + (int) exports->AddressOfNames);
	for(i=0; i<exports->Number_Of_Functions; i++)
	{
		if(HIWORD(funcName))
		{
			ename =  (char *) (((char *) load_addr) + (int) *name);
			if(strcmp(ename,funcName)==0)
				return load_addr+*function;
		}else{
			if(funcName == (int)*ordinal + exports->Base)
				return load_addr+*function;
		}
		function++;
		ordinal++;
		name++;
	}
	return 0;
}

DWORD PE_GetProcAddress(HMODULE hModule, char* function)
{
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32) || !pModule->pe_module) return 0;
    if (pModule->flags & NE_FFLAGS_BUILTIN)
        return BUILTIN_GetProcAddress32( pModule, function );
    return PE_FindExportedFunction( pModule->pe_module, function );
}

void fixup_imports(struct pe_data *pe, HMODULE hModule)
{ 
  struct PE_Import_Directory * pe_imp;
  int fixup_failed=0;
  unsigned int load_addr = pe->load_addr;
  int i;
  NE_MODULE *ne_mod;
  HMODULE *mod_ptr;

 /* OK, now dump the import list */
  dprintf_win32(stddeb, "\nDumping imports list\n");

  /* first, count the number of imported non-internal modules */
  pe_imp = pe->pe_import;
  for(i=0;pe_imp->ModuleName;pe_imp++)
  	i++;

  /* Now, allocate memory for dlls_to_init */
  ne_mod = GlobalLock16(hModule);
  ne_mod->dlls_to_init = GLOBAL_Alloc(GMEM_ZEROINIT,(i+1) * sizeof(HMODULE),
                                      hModule, FALSE, FALSE, FALSE );
  mod_ptr = GlobalLock16(ne_mod->dlls_to_init);
  /* load the modules and put their handles into the list */
  for(i=0,pe_imp = pe->pe_import;pe_imp->ModuleName;pe_imp++)
  {
  	char *name = (char*)load_addr+pe_imp->ModuleName;
	mod_ptr[i] = LoadModule(name,(LPVOID)-1);
	if(mod_ptr[i]<=(HMODULE)32)
        {
            char *p, buffer[256];

            /* Try with prepending the path of the current module */
            GetModuleFileName( hModule, buffer, sizeof(buffer) );
            if (!(p = strrchr( buffer, '\\' ))) p = buffer;
            strcpy( p + 1, name );
            mod_ptr[i] = LoadModule( buffer, (LPVOID)-1 );
        }
	if(mod_ptr[i]<=(HMODULE)32)
	{
		fprintf(stderr,"Module %s not found\n",name);
		exit(0);
	}
	i++;
  }
  pe_imp = pe->pe_import;
  while (pe_imp->ModuleName)
    {
      char * Module;
      struct pe_import_name * pe_name;
      unsigned int * import_list, *thunk_list;

      Module = ((char *) load_addr) + pe_imp->ModuleName;
      dprintf_win32(stddeb, "%s\n", Module);

   if(pe_imp->Import_List != 0) { /* original microsoft style */
      dprintf_win32(stddeb, "Microsoft style imports used\n");
      import_list = (unsigned int *) 
	(((unsigned int) load_addr) + pe_imp->Import_List);
	  thunk_list = (unsigned int *)
	  (((unsigned int) load_addr) + pe_imp->Thunk_List);


    while(*import_list)
	{
	  pe_name = (struct pe_import_name *) ((int) load_addr + ((unsigned)*import_list & ~0x80000000));
	  if((unsigned)*import_list & 0x80000000)
	  {
		int ordinal=*import_list & (0x80000000-1);
		dprintf_win32(stddeb,"--- Ordinal %s,%d\n", Module, ordinal);
		*thunk_list = WIN32_GetProcAddress(MODULE_FindModule(Module),
			ordinal);
	  	if(!*thunk_list)
	  	{
	  		fprintf(stderr,"No implementation for %s.%d\n",Module, 
				ordinal);
			fixup_failed=1;
	  	}
	  }else{ /* import by name */
	  dprintf_win32(stddeb, "--- %s %s.%d\n", pe_name->Name, Module, pe_name->Hint);
#ifndef WINELIB /* FIXME: JBP: Should this happen in libwine.a? */
	  	*thunk_list = WIN32_GetProcAddress(MODULE_FindModule(Module),
				pe_name->Name);
	  if(!*thunk_list)
	  {
	  	fprintf(stderr,"No implementation for %s.%d(%s)\n",Module, 
			pe_name->Hint, pe_name->Name);
		fixup_failed=1;
	  }

#else
	  fprintf(stderr,"JBP: Call to RELAY32_GetEntryPoint being ignored.\n");
#endif
		}

	  import_list++;
	  thunk_list++;
	}
    } else { /* Borland style */
      dprintf_win32(stddeb, "Borland style imports used\n");
      thunk_list = (unsigned int *)
        (((unsigned int) load_addr) + pe_imp->Thunk_List);
      while(*thunk_list) {
        pe_name = (struct pe_import_name *) ((int) load_addr + *thunk_list);
        if((unsigned)pe_name & 0x80000000) {
          fprintf(stderr,"Import by ordinal not supported\n");
          exit(0);
        }
        dprintf_win32(stddeb, "--- %s %s.%d\n", pe_name->Name, Module, pe_name->Hint);
#ifndef WINELIB /* FIXME: JBP: Should this happen in libwine.a? */
	/* FIXME: Both calls should be unified into GetProcAddress */
        *thunk_list = WIN32_GetProcAddress(MODULE_FindModule(Module),
                                           pe_name->Name);
#else
        fprintf(stderr,"JBP: Call to RELAY32_GetEntryPoint being ignored.\n");
#endif
        if(!*thunk_list) {
          fprintf(stderr,"No implementation for %s.%d\n",Module, pe_name->Hint);
          fixup_failed=1;
        }
        thunk_list++;
      }
    }
    pe_imp++;
  }
  if(fixup_failed)exit(1);
}

static void calc_vma_size(struct pe_data *pe)
{
  int i;

  dprintf_win32(stddeb, "Dump of segment table\n");
  dprintf_win32(stddeb, "   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
  for(i=0; i< pe->pe_header->coff.NumberOfSections; i++)
    {
      dprintf_win32(stddeb, "%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
	     pe->pe_seg[i].Name, 
	     pe->pe_seg[i].Virtual_Size,
	     pe->pe_seg[i].Virtual_Address,
	     pe->pe_seg[i].Size_Of_Raw_Data,
	     pe->pe_seg[i].PointerToRawData,
	     pe->pe_seg[i].PointerToRelocations,
	     pe->pe_seg[i].PointerToLinenumbers,
	     pe->pe_seg[i].NumberOfRelocations,
	     pe->pe_seg[i].NumberOfLinenumbers,
	     pe->pe_seg[i].Characteristics);
	  pe->vma_size = MAX(pe->vma_size,
	  		pe->pe_seg[i].Virtual_Address + 
			pe->pe_seg[i].Size_Of_Raw_Data);
    }
}

static void do_relocations(struct pe_data *pe)
{
	int delta = pe->load_addr - pe->base_addr;
	struct PE_Reloc_Block *r = pe->pe_reloc;
	int hdelta = (delta >> 16) & 0xFFFF;
	int ldelta = delta & 0xFFFF;
	/* int reloc_size = */
	if(delta == 0)
		/* Nothing to do */
		return;
	while(r->PageRVA)
	{
		char *page = (char*)pe->load_addr + r->PageRVA;
		int count = (r->BlockSize - 8)/2;
		int i;
		dprintf_fixup(stddeb, "%x relocations for page %lx\n",
			count, r->PageRVA);
		/* patching in reverse order */
		for(i=0;i<count;i++)
		{
			int offset = r->Relocations[i] & 0xFFF;
			int type = r->Relocations[i] >> 12;
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
				  int l=r->Relocations[++i];
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
		r = (struct PE_Reloc_Block*)((char*)r + r->BlockSize);
	}
}
		

	
	

/**********************************************************************
 *			PE_LoadImage
 * Load one PE format executable into memory
 */
static struct pe_data *PE_LoadImage( int fd, HMODULE hModule, WORD offset )
{
    struct pe_data *pe;
    int i, result;
    unsigned int load_addr;
    struct Directory dir;

	pe = xmalloc(sizeof(struct pe_data));
	memset(pe,0,sizeof(struct pe_data));
	pe->pe_header = xmalloc(sizeof(struct pe_header_s));

	/* read PE header */
	lseek( fd, offset, SEEK_SET);
	read( fd, pe->pe_header, sizeof(struct pe_header_s));

	/* read sections */
	pe->pe_seg = xmalloc(sizeof(struct pe_segment_table) * 
				   pe->pe_header->coff.NumberOfSections);
	read( fd, pe->pe_seg, sizeof(struct pe_segment_table) * 
			pe->pe_header->coff.NumberOfSections);

	load_addr = pe->pe_header->opt_coff.BaseOfImage;
	pe->base_addr=load_addr;
	pe->vma_size=0;
	dprintf_win32(stddeb, "Load addr is %x\n",load_addr);
	calc_vma_size(pe);

	/* We use malloc here, while a huge part of that address space does
	   not be supported by actual memory. It has to be contiguous, though.
	   I don't know if mmap("/dev/null"); would do any better.
	   What I'd really like to do is a Win32 style VirtualAlloc/MapViewOfFile
	   sequence */
	load_addr = pe->load_addr = malloc(pe->vma_size);
	dprintf_win32(stddeb, "Load addr is really %x, range %x\n",
		pe->load_addr, pe->vma_size);


	for(i=0; i < pe->pe_header->coff.NumberOfSections; i++)
	{
		/* load only non-BSS segments */
		if(pe->pe_seg[i].Characteristics & 
			~ IMAGE_SCN_TYPE_CNT_UNINITIALIZED_DATA)
		if(lseek(fd,pe->pe_seg[i].PointerToRawData,SEEK_SET) == -1
		|| read(fd,load_addr + pe->pe_seg[i].Virtual_Address,
				pe->pe_seg[i].Size_Of_Raw_Data) 
				!= pe->pe_seg[i].Size_Of_Raw_Data)
		{
			fprintf(stderr,"Failed to load section %x\n", i);
			exit(0);
		}
		result = load_addr + pe->pe_seg[i].Virtual_Address;
#if 0
	if(!load_addr) {
		
		result = (int)xmmap((char *)0, pe->pe_seg[i].Virtual_Size,
			pe->pe_seg[i].Size_Of_Raw_Data, 7,
			MAP_PRIVATE, fd, pe->pe_seg[i].PointerToRawData);
		load_addr = (unsigned int) result -  pe->pe_seg[i].Virtual_Address;
	} else {
		result = (int)xmmap((char *) load_addr + pe->pe_seg[i].Virtual_Address, 
			  pe->pe_seg[i].Virtual_Size,
		      pe->pe_seg[i].Size_Of_Raw_Data, 7, MAP_PRIVATE | MAP_FIXED, 
		      fd, pe->pe_seg[i].PointerToRawData);
	}
	if(result==-1){
		fprintf(stderr,"Could not load section %x to desired address %lx\n",
			i, load_addr+pe->pe_seg[i].Virtual_Address);
		fprintf(stderr,"Need to implement relocations now\n");
		exit(0);
	}
#endif

        if(strcmp(pe->pe_seg[i].Name, ".bss") == 0)
            memset((void *)result, 0, 
                   pe->pe_seg[i].Virtual_Size ?
                   pe->pe_seg[i].Virtual_Size :
                   pe->pe_seg[i].Size_Of_Raw_Data);

	if(strcmp(pe->pe_seg[i].Name, ".idata") == 0)
		pe->pe_import = (struct PE_Import_Directory *) result;

	if(strcmp(pe->pe_seg[i].Name, ".edata") == 0)
		pe->pe_export = (struct PE_Export_Directory *) result;

	if(strcmp(pe->pe_seg[i].Name, ".rsrc") == 0) {
	    pe->pe_resource = (struct PE_Resource_Directory *) result;
#if 0
/* FIXME pe->resource_offset should be deleted from structure if this
 ifdef doesn't break anything */
	    /* save offset for PE_FindResource */
	    pe->resource_offset = pe->pe_seg[i].Virtual_Address - 
					pe->pe_seg[i].PointerToRawData;
#endif	
	    }
	if(strcmp(pe->pe_seg[i].Name, ".reloc") == 0)
		pe->pe_reloc = (struct PE_Reloc_Block *) result;

	}

	/* There is word that the actual loader does not care about the
	   section names, and only goes for the DataDirectory */
	dir=pe->pe_header->opt_coff.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
	if(dir.Size)
	{
		if(pe->pe_export && 
			pe->pe_export!=load_addr+dir.Virtual_address)
			fprintf(stderr,"wrong export directory??\n");
		/* always trust the directory */
		pe->pe_export = load_addr+dir.Virtual_address;
	}

	dir=pe->pe_header->opt_coff.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
	if(dir.Size)
	{
		if(pe->pe_import && 
			pe->pe_import!=load_addr+dir.Virtual_address)
			fprintf(stderr,"wrong import directory??\n");
		pe->pe_import = load_addr+dir.Virtual_address;
	}

	dir=pe->pe_header->opt_coff.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
	if(dir.Size)
	{
		if(pe->pe_resource && 
			pe->pe_resource!=load_addr+dir.Virtual_address)
			fprintf(stderr,"wrong resource directory??\n");
		pe->pe_resource = load_addr+dir.Virtual_address;
	}

	dir=pe->pe_header->opt_coff.DataDirectory[IMAGE_FILE_BASE_RELOCATION_TABLE];
	if(dir.Size)
	{
		if(pe->pe_reloc && 
			pe->pe_reloc!=load_addr+dir.Virtual_address)
			fprintf(stderr,"wrong relocation list??\n");
		pe->pe_reloc = load_addr+dir.Virtual_address;
	}

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_EXCEPTION_DIRECTORY].Size)
		dprintf_win32(stdnimp,"Exception directory ignored\n");

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_SECURITY_DIRECTORY].Size)
		dprintf_win32(stdnimp,"Security directory ignored\n");

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_DEBUG_DIRECTORY].Size)
		dprintf_win32(stdnimp,"Debug directory ignored\n");

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_DESCRIPTION_STRING].Size)
		dprintf_win32(stdnimp,"Description string ignored\n");

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_MACHINE_VALUE].Size)
		dprintf_win32(stdnimp,"Machine Value ignored\n");

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_THREAD_LOCAL_STORAGE].Size)
		 dprintf_win32(stdnimp,"Thread local storage ignored\n");

	if(pe->pe_header->opt_coff.DataDirectory
		[IMAGE_FILE_CALLBACK_DIRECTORY].Size)
		dprintf_win32(stdnimp,"Callback directory ignored\n");


	if(pe->pe_import) fixup_imports(pe, hModule);
	if(pe->pe_export) dump_exports(pe->pe_export,load_addr);
	if(pe->pe_reloc) do_relocations(pe);
        return pe;
}

HINSTANCE MODULE_CreateInstance(HMODULE hModule,LOADPARAMS *params);
void InitTask(struct sigcontext_struct context);

HINSTANCE PE_LoadModule( int fd, OFSTRUCT *ofs, LOADPARAMS* params )
{
        PE_MODULE *pe;
	int size, of_size;
	NE_MODULE *pModule;
	SEGTABLEENTRY *pSegment;
	char *pStr;
	DWORD cts;
	HMODULE hModule;
	HINSTANCE hInstance;
        struct mz_header_s mz_header;

	lseek(fd,0,SEEK_SET);
	read( fd, &mz_header, sizeof(mz_header) );

        of_size = sizeof(OFSTRUCT) - sizeof(ofs->szPathName)
                  + strlen(ofs->szPathName) + 1;
	size = sizeof(NE_MODULE) +
               /* loaded file info */
               of_size +
               /* segment table: DS,CS */
               2 * sizeof(SEGTABLEENTRY) +
               /* name table */
               9 +
               /* several empty tables */
               8;

	hModule = GlobalAlloc16( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
	if (!hModule) return (HINSTANCE)11;  /* invalid exe */

	FarSetOwner( hModule, hModule );
	
	pModule = (NE_MODULE*)GlobalLock16(hModule);

	/* Set all used entries */
	pModule->magic=NE_SIGNATURE;
	pModule->count=1;
	pModule->next=0;
	pModule->flags=NE_FFLAGS_WIN32;
	pModule->dgroup=1;
	pModule->ss=1;
	pModule->cs=2;
	/* Who wants to LocalAlloc for a PE Module? */
	pModule->heap_size=0x1000;
	pModule->stack_size=0xF000;
	pModule->seg_count=1;
	pModule->modref_count=0;
	pModule->nrname_size=0;
	pModule->fileinfo=sizeof(NE_MODULE);
	pModule->os_flags=NE_OSFLAGS_WINDOWS;
	pModule->expected_version=0x30A;
        pModule->self = hModule;

        /* Set loaded file information */
        memcpy( pModule + 1, ofs, of_size );
        ((OFSTRUCT *)(pModule+1))->cBytes = of_size - 1;

	pSegment=(SEGTABLEENTRY*)((char*)(pModule + 1) + of_size);
	pModule->seg_table=pModule->dgroup_entry=(int)pSegment-(int)pModule;
	pSegment->size=0;
	pSegment->flags=NE_SEGFLAGS_DATA;
	pSegment->minsize=0x1000;
	pSegment++;

	cts=(DWORD)MODULE_GetWndProcEntry16("Win32CallToStart");
#ifdef WINELIB32
	pSegment->selector=(void*)cts;
	pModule->ip=0;
#else
	pSegment->selector=cts>>16;
	pModule->ip=cts & 0xFFFF;
#endif
	pSegment++;

	pStr=(char*)pSegment;
	pModule->name_table=(int)pStr-(int)pModule;
	strcpy(pStr,"\x08W32SXXXX");
	pStr+=9;

	/* All tables zero terminated */
	pModule->res_table=pModule->import_table=pModule->entry_table=
		(int)pStr-(int)pModule;

        MODULE_RegisterModule( pModule );

	pe = PE_LoadImage( fd, hModule, mz_header.ne_offset );

        pModule->pe_module = pe;
	pModule->heap_size=0x1000;
	pModule->stack_size=0xE000;

	/* CreateInstance allocates now 64KB */
	hInstance=MODULE_CreateInstance(hModule,NULL /* FIX: NULL? really? */);

        /* FIXME: Is this really the correct place to initialise the DLL? */
	if ((pe->pe_header->coff.Characteristics & IMAGE_FILE_DLL)) {
/*            PE_InitDLL(hModule); */
        } else {
            TASK_CreateTask(hModule,hInstance,0,
		params->hEnvironment,(LPSTR)PTR_SEG_TO_LIN(params->cmdLine),
		*((WORD*)PTR_SEG_TO_LIN(params->showCmd)+1));
            PE_InitializeDLLs(hModule);
	}
	return hInstance;
}

int USER_InitApp(HINSTANCE hInstance);
void PE_InitTEB(int hTEB);

void PE_Win32CallToStart(struct sigcontext_struct context)
{
    int fs;
    HMODULE hModule;
    NE_MODULE *pModule;

    dprintf_win32(stddeb,"Going to start Win32 program\n");	
    InitTask(context);
    hModule = GetExePtr( GetCurrentTask() );
    pModule = MODULE_GetPtr( hModule );
    USER_InitApp( hModule );
    fs=(int)GlobalAlloc16(GHND,0x10000);
    PE_InitTEB(fs);
    __asm__ __volatile__("movw %w0,%%fs"::"r" (fs));
    CallTaskStart32( (FARPROC)(pModule->pe_module->load_addr + 
                               pModule->pe_module->pe_header->opt_coff.AddressOfEntryPoint) );
}

int PE_UnloadImage( HMODULE hModule )
{
	printf("PEunloadImage() called!\n");
	/* free resources, image, unmap */
	return 1;
}

static void PE_InitDLL(HMODULE hModule)
{
    NE_MODULE *pModule;
    PE_MODULE *pe;

    hModule = GetExePtr(hModule);
    if (!(pModule = MODULE_GetPtr(hModule))) return;
    if (!(pModule->flags & NE_FFLAGS_WIN32) || !(pe = pModule->pe_module))
        return;

    /* FIXME: What are the correct values for parameters 2 and 3? */
        
    /* Is this a library? */
    if (pe->pe_header->coff.Characteristics & IMAGE_FILE_DLL)
    {
        printf("InitPEDLL() called!\n");
        CallDLLEntryProc32( (FARPROC)(pe->load_addr + 
                                  pe->pe_header->opt_coff.AddressOfEntryPoint),
                            hModule, 0, 0 );
    }
}


/* FIXME: This stuff is all on a "well it works" basis. An implementation
based on some kind of documentation would be greatly appreciated :-) */

typedef struct
{
    void        *Except;
    void        *stack;
    int	        dummy1[4];
    struct TEB  *TEBDSAlias;
    int	        dummy2[2];
    int	        taskid;
} TEB;

void PE_InitTEB(int hTEB)
{
    TDB *pTask;
    TEB *pTEB;

    pTask = (TDB *)(GlobalLock16(GetCurrentTask() & 0xffff));
    pTEB = (TEB *)(PTR_SEG_OFF_TO_LIN(hTEB, 0));
    pTEB->stack = (void *)(GlobalLock16(pTask->hStack32));
    pTEB->Except = (void *)(-1); 
    pTEB->TEBDSAlias = pTEB;
    pTEB->taskid = getpid();
}

void PE_InitializeDLLs(HMODULE hModule)
{
	NE_MODULE *pModule;
	HMODULE *pDLL;
	pModule = MODULE_GetPtr( GetExePtr(hModule) );
	if (pModule->dlls_to_init)
	{
		HANDLE to_init = pModule->dlls_to_init;
		pModule->dlls_to_init = 0;
		for (pDLL = (HMODULE *)GlobalLock16( to_init ); *pDLL; pDLL++)
		{
			PE_InitializeDLLs( *pDLL );
			PE_InitDLL( *pDLL );
		}
		GlobalFree16( to_init );
	}
	PE_InitDLL( hModule );
}
#endif /* WINELIB */

#ifndef WINELIB
/* 
 *  Copyright	1994	Eric Youndale & Erik Bos
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
#include "dlls.h"
#include "neexe.h"
#include "peexe.h"
#include "pe_image.h"
#include "relay32.h"
#include "module.h"
#include "alias.h"
#include "global.h"
#include "task.h"
#include "ldt.h"
#include "registers.h"
#include "selectors.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

#define MAP_ANONYMOUS	0x20

struct w_files *wine_files = NULL;

void my_wcstombs(char * result, u_short * source, int len)
{
  while(len--) {
    if(isascii(*source)) *result++ = *source++;
    else {
      printf("Unable to handle unicode right now\n");
      exit(0);
    }
  };
}

char * xmmap(char * vaddr, unsigned int v_size, unsigned int r_size,
	int prot, int flags, int fd, unsigned int file_offset)
{
  char * result;
  /* .bss has no associated storage in the PE file */
  if(r_size)
    v_size=r_size;
  else
#ifdef __svr4__
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

void fixup_imports(struct PE_Import_Directory *pe_imports,unsigned int load_addr)
{ 
  struct PE_Import_Directory * pe_imp;
  int fixup_failed=0;

 /* OK, now dump the import list */
  dprintf_win32(stddeb, "\nDumping imports list\n");
  pe_imp = pe_imports;
  while (pe_imp->ModuleName)
    {
      char * Module;
      struct pe_import_name * pe_name;
      unsigned int * import_list, *thunk_list;
#if 0
      char * c;
#endif

      Module = ((char *) load_addr) + pe_imp->ModuleName;
      dprintf_win32(stddeb, "%s\n", Module);
#if 0
      c = strchr(Module, '.');
      if (c) *c = 0;
#endif

      import_list = (unsigned int *) 
	(((unsigned int) load_addr) + pe_imp->Import_List);
	  thunk_list = (unsigned int *)
	  (((unsigned int) load_addr) + pe_imp->Thunk_List);


      while(*import_list)
	{
	  pe_name = (struct pe_import_name *) ((int) load_addr + *import_list);
	  if((unsigned)pe_name & 0x80000000)
	  {
	  	fprintf(stderr,"Import by ordinal not supported\n");
		exit(0);
	  }
	  dprintf_win32(stddeb, "--- %s %s.%d\n", pe_name->Name, Module, pe_name->Hint);
#ifndef WINELIB /* FIXME: JBP: Should this happen in libwine.a? */
	  *thunk_list=(unsigned int)RELAY32_GetEntryPoint(Module,pe_name->Name,pe_name->Hint);
#else
	  fprintf(stderr,"JBP: Call to RELAY32_GetEntryPoint being ignored.\n");
#endif
	  if(!*thunk_list)
	  {
	  	fprintf(stderr,"No implementation for %s.%d\n",Module, pe_name->Hint);
		fixup_failed=1;
	  }

	  import_list++;
	  thunk_list++;
	}
      pe_imp++;
    };
	if(fixup_failed)exit(1);
}

static void dump_table(struct w_files *wpnt)
{
  int i;

  dprintf_win32(stddeb, "Dump of segment table\n");
  dprintf_win32(stddeb, "   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
  for(i=0; i< wpnt->pe->pe_header->coff.NumberOfSections; i++)
    {
      dprintf_win32(stddeb, "%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
	     wpnt->pe->pe_seg[i].Name, 
	     wpnt->pe->pe_seg[i].Virtual_Size,
	     wpnt->pe->pe_seg[i].Virtual_Address,
	     wpnt->pe->pe_seg[i].Size_Of_Raw_Data,
	     wpnt->pe->pe_seg[i].PointerToRawData,
	     wpnt->pe->pe_seg[i].PointerToRelocations,
	     wpnt->pe->pe_seg[i].PointerToLinenumbers,
	     wpnt->pe->pe_seg[i].NumberOfRelocations,
	     wpnt->pe->pe_seg[i].NumberOfLinenumbers,
	     wpnt->pe->pe_seg[i].Characteristics);
    }
}

/**********************************************************************
 *			PE_LoadImage
 * Load one PE format executable into memory
 */
HINSTANCE PE_LoadImage(struct w_files *wpnt)
{
	int i, result;
        unsigned int load_addr;

	wpnt->pe = xmalloc(sizeof(struct pe_data));
	memset(wpnt->pe,0,sizeof(struct pe_data));
	wpnt->pe->pe_header = xmalloc(sizeof(struct pe_header_s));

	/* read PE header */
	lseek(wpnt->fd, wpnt->mz_header->ne_offset, SEEK_SET);
	read(wpnt->fd, wpnt->pe->pe_header, sizeof(struct pe_header_s));

	/* read sections */
	wpnt->pe->pe_seg = xmalloc(sizeof(struct pe_segment_table) * 
				   wpnt->pe->pe_header->coff.NumberOfSections);
	read(wpnt->fd, wpnt->pe->pe_seg, sizeof(struct pe_segment_table) * 
			wpnt->pe->pe_header->coff.NumberOfSections);

	load_addr = wpnt->pe->pe_header->opt_coff.BaseOfImage;
	dprintf_win32(stddeb, "Load addr is %x\n",load_addr);
	dump_table(wpnt);

	for(i=0; i < wpnt->pe->pe_header->coff.NumberOfSections; i++)
	{
	if(!load_addr) {
		result = (int)xmmap((char *)0, wpnt->pe->pe_seg[i].Virtual_Size,
			wpnt->pe->pe_seg[i].Size_Of_Raw_Data, 7,
			MAP_PRIVATE, wpnt->fd, wpnt->pe->pe_seg[i].PointerToRawData);
		load_addr = (unsigned int) result -  wpnt->pe->pe_seg[i].Virtual_Address;
	} else {
		result = (int)xmmap((char *) load_addr + wpnt->pe->pe_seg[i].Virtual_Address, 
			  wpnt->pe->pe_seg[i].Virtual_Size,
		      wpnt->pe->pe_seg[i].Size_Of_Raw_Data, 7, MAP_PRIVATE | MAP_FIXED, 
		      wpnt->fd, wpnt->pe->pe_seg[i].PointerToRawData);
	}
	if(result==-1){
		fprintf(stderr,"Could not load section %x to desired address %lx\n",
			i, load_addr+wpnt->pe->pe_seg[i].Virtual_Address);
		fprintf(stderr,"Need to implement relocations now\n");
		exit(0);
	}

	if(strcmp(wpnt->pe->pe_seg[i].Name, ".idata") == 0)
		wpnt->pe->pe_import = (struct PE_Import_Directory *) result;

	if(strcmp(wpnt->pe->pe_seg[i].Name, ".edata") == 0)
		wpnt->pe->pe_export = (struct PE_Export_Directory *) result;

	if(strcmp(wpnt->pe->pe_seg[i].Name, ".rsrc") == 0) {
	    wpnt->pe->pe_resource = (struct PE_Resource_Directory *) result;

	    /* save offset for PE_FindResource */
	    wpnt->pe->resource_offset = wpnt->pe->pe_seg[i].Virtual_Address - 
					wpnt->pe->pe_seg[i].PointerToRawData;
	    }
	}

	if(wpnt->pe->pe_import) fixup_imports(wpnt->pe->pe_import,load_addr);
	if(wpnt->pe->pe_export) dump_exports(wpnt->pe->pe_export,load_addr);
  
	wpnt->hinstance = (HINSTANCE)0x8000;
	wpnt->load_addr = load_addr;
	return (wpnt->hinstance);
}

HINSTANCE MODULE_CreateInstance(HMODULE hModule,LOADPARAMS *params);
void InitTask(struct sigcontext_struct context);

HINSTANCE PE_LoadModule(int fd, OFSTRUCT *ofs, LOADPARAMS* params)
{
	struct w_files *wpnt;
	int size;
	NE_MODULE *pModule;
	LOADEDFILEINFO *pFileInfo;
	SEGTABLEENTRY *pSegment;
	char *pStr;
	DWORD cts;
	HMODULE hModule;
	HINSTANCE hInstance;

	ALIAS_UseAliases=1;

	wpnt=xmalloc(sizeof(struct w_files));
	wpnt->ofs=*ofs;
	wpnt->fd=fd;
	wpnt->type=0;
	wpnt->hinstance=0;
	wpnt->hModule=0;
	wpnt->initialised=0;
	lseek(fd,0,SEEK_SET);
	wpnt->mz_header=xmalloc(sizeof(struct mz_header_s));
	read(fd,wpnt->mz_header,sizeof(struct mz_header_s));

	size=sizeof(NE_MODULE) +
		/* loaded file info */
		sizeof(LOADEDFILEINFO) + strlen(ofs->szPathName) +
		/* segment table: DS,CS */
		2 * sizeof(SEGTABLEENTRY) +
		/* name table */
		9 +
		/* several empty tables */
		8;

	hModule = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
	wpnt->hModule=hModule;
	if (!hModule) return (HINSTANCE)11;  /* invalid exe */

	FarSetOwner( hModule, hModule );
	
	pModule = (NE_MODULE*)GlobalLock(hModule);

	/* Set all used entries */
	pModule->magic=PE_SIGNATURE;
	pModule->count=1;
	pModule->next=0;
	pModule->flags=0;
	pModule->dgroup=1;
	pModule->ss=1;
	pModule->cs=2;
	/* Who wants to LocalAlloc for a PE Module? */
	pModule->heap_size=0x1000;
	pModule->stack_size=0xF000;
	pModule->seg_count=1;
	pModule->modref_count=0;
	pModule->nrname_size=0;
	pModule->seg_table=sizeof(NE_MODULE)+
			sizeof(LOADEDFILEINFO)+strlen(ofs->szPathName);
	pModule->fileinfo=sizeof(NE_MODULE);
	pModule->os_flags=NE_OSFLAGS_WINDOWS;
	pModule->expected_version=0x30A;

	pFileInfo=(LOADEDFILEINFO *)(pModule + 1);
	pFileInfo->length = sizeof(LOADEDFILEINFO)+strlen(ofs->szPathName)-1;
	strcpy(pFileInfo->filename,ofs->szPathName);

	pSegment=(SEGTABLEENTRY*)((char*)pFileInfo+pFileInfo->length+1);
	pModule->dgroup_entry=(int)pSegment-(int)pModule;
	pSegment->size=0;
	pSegment->flags=NE_SEGFLAGS_DATA;
	pSegment->minsize=0x1000;
	pSegment++;

	cts=(DWORD)GetWndProcEntry16("Win32CallToStart");
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

	PE_LoadImage(wpnt);

	pModule->heap_size=0x1000;
	pModule->stack_size=0xE000;

	/* CreateInstance allocates now 64KB */
	hInstance=MODULE_CreateInstance(hModule,NULL /* FIX: NULL? really? */);
	wpnt->hinstance=hInstance;

	if (wpnt->pe->pe_export) {
		wpnt->name = xmalloc(strlen(wpnt->pe->pe_export->ModuleName)+1);
		strcpy(wpnt->name, wpnt->pe->pe_export->ModuleName);
	} else {
		wpnt->name = xmalloc(strlen(ofs->szPathName)+1);
		strcpy(wpnt->name, ofs->szPathName);
	}

	wpnt->next=wine_files;
	wine_files=wpnt;

	if (!(wpnt->pe->pe_header->coff.Characteristics & IMAGE_FILE_DLL))
	TASK_CreateTask(hModule,hInstance,0,
		params->hEnvironment,(LPSTR)PTR_SEG_TO_LIN(params->cmdLine),
		*((WORD*)PTR_SEG_TO_LIN(params->showCmd)+1));
	
	return hInstance;
}

int USER_InitApp(HINSTANCE hInstance);

void PE_Win32CallToStart(struct sigcontext_struct context)
{
	int fs;
	struct w_files *wpnt=wine_files;
	fs=(int)GlobalAlloc(GHND,0x10000);
	dprintf_win32(stddeb,"Going to start Win32 program\n");	
	InitTask(context);
	USER_InitApp(wpnt->hModule);
	__asm__ __volatile__("movw %w0,%%fs"::"r" (fs));
	((void(*)())(wpnt->load_addr+wpnt->pe->pe_header->opt_coff.AddressOfEntryPoint))();
}

int PE_UnloadImage(struct w_files *wpnt)
{
	printf("PEunloadImage() called!\n");
	/* free resources, image, unmap */
	return 1;
}

void PE_InitDLL(struct w_files *wpnt)
{
	/* Is this a library? */
	if (wpnt->pe->pe_header->coff.Characteristics & IMAGE_FILE_DLL) {
		printf("InitPEDLL() called!\n");
	}
}
#endif /* WINELIB */

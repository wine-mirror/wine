/* 
 *  Copyright	1994	Eric Youndale & Erik Bos
 *
 *	based on Eric Youndale's pe-test and:
 *
 *	ftp.microsoft.com:/pub/developer/MSDN/CD8/PEFILE.ZIP
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

#define MAP_ANONYMOUS	0x20

unsigned int load_addr;

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

char * xmmap(char * vaddr, unsigned int v_size, int prot, int flags, 
	     int fd, unsigned int file_offset)
{
  char * result;
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

void dump_exports(struct PE_Export_Directory * pe_exports)
{ 
  char * Module;
  int i;
  u_short * ordinal;
  u_long * function;
  u_char ** name, *ename;

  Module = ((char *) load_addr) + pe_exports->Name;
  printf("\n*******EXPORT DATA*******\nModule name is %s, %ld functions, %ld names\n", 
	 Module,
	 pe_exports->Number_Of_Functions,
	 pe_exports->Number_Of_Names);

  ordinal = (u_short *) (((char *) load_addr) + (int) pe_exports->Address_Of_Name_Ordinals);
  function = (u_long *)  (((char *) load_addr) + (int) pe_exports->AddressOfFunctions);
  name = (u_char **)  (((char *) load_addr) + (int) pe_exports->AddressOfNames);

  printf("%-32s Ordinal Virt Addr\n", "Function Name");
  for(i=0; i< pe_exports->Number_Of_Functions; i++)
    {
      ename =  (char *) (((char *) load_addr) + (int) *name++);
      printf("%-32s %4d    %8.8lx\n", ename, *ordinal++, *function++);
    }
}

void dump_imports(struct PE_Import_Directory *pe_imports)
{ 
  struct PE_Import_Directory * pe_imp;

 /* OK, now dump the import list */
  printf("\nDumping imports list\n");
  pe_imp = pe_imports;
  while (pe_imp->ModuleName)
    {
      char * Module;
      struct pe_import_name * pe_name;
      unsigned int * import_list;
      char * c;

      Module = ((char *) load_addr) + pe_imp->ModuleName;
      printf("%s\n", Module);
      c = strchr(Module, '.');
      if (c) *c = 0;

      import_list = (unsigned int *) 
	(((unsigned int) load_addr) + pe_imp->Import_List);

      while(*import_list)
	{
	  pe_name = (struct pe_import_name *) ((int) load_addr + *import_list);
	  printf("--- %s %s.%d\n", pe_name->Name, Module, pe_name->Hint);
	  import_list++;
	}
      pe_imp++;
    };
}

static void dump_table(struct w_files *wpnt)
{
  int i;

  printf("Dump of segment table\n");
  printf("   Name    VSz  Vaddr     SzRaw   Fileadr  *Reloc *Lineum #Reloc #Linum Char\n");
  for(i=0; i< wpnt->pe->pe_header->coff.NumberOfSections; i++)
    {
      printf("%8s: %4.4lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %4.4x %4.4x %8.8lx\n", 
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

	wpnt->pe = malloc(sizeof(struct pe_data));
	wpnt->pe->pe_header = malloc(sizeof(struct pe_header_s));

	/* read PE header */
	lseek(wpnt->fd, wpnt->mz_header->ne_offset, SEEK_SET);
	read(wpnt->fd, wpnt->pe->pe_header, sizeof(struct pe_header_s));

	/* read sections */
	wpnt->pe->pe_seg = malloc(sizeof(struct pe_segment_table) * 
				wpnt->pe->pe_header->coff.NumberOfSections);
	read(wpnt->fd, wpnt->pe->pe_seg, sizeof(struct pe_segment_table) * 
			wpnt->pe->pe_header->coff.NumberOfSections);

	load_addr = wpnt->pe->pe_header->opt_coff.BaseOfImage;
	dump_table(wpnt);

	for(i=0; i < wpnt->pe->pe_header->coff.NumberOfSections; i++)
	{
	if(!load_addr) {
		result = xmmap((char *)0, wpnt->pe->pe_seg[i].Size_Of_Raw_Data, 7,
			MAP_PRIVATE, wpnt->fd, wpnt->pe->pe_seg[i].PointerToRawData);
		load_addr = (unsigned int) result -  wpnt->pe->pe_seg[i].Virtual_Address;
	} else {
		result = xmmap((char *) load_addr + wpnt->pe->pe_seg[i].Virtual_Address, 
		      wpnt->pe->pe_seg[i].Size_Of_Raw_Data, 7, MAP_PRIVATE | MAP_FIXED, 
		      wpnt->fd, wpnt->pe->pe_seg[i].PointerToRawData);
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

	if(wpnt->pe->pe_import) dump_imports(wpnt->pe->pe_import);
	if(wpnt->pe->pe_export) dump_exports(wpnt->pe->pe_export);
  
	wpnt->hinstance = 0x8000;
	return (wpnt->hinstance);
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

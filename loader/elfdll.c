/*
 * Elf-dll loader functions
 *
 * Copyright 1999 Bertho A. Stultiens
 */

#include <string.h>
#include <ctype.h>

#include "config.h"
#include "windef.h"
#include "global.h"
#include "process.h"
#include "module.h"
#include "neexe.h"
#include "heap.h"
#include "wine/winbase16.h"
#include "elfdll.h"
#include "debug.h"
#include "winerror.h"

#if defined(HAVE_LIBDL) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>

/*------------------ HACKS -----------------*/
#ifndef elfdll
#define elfdll	win32
#endif

extern DWORD fixup_imports(WINE_MODREF *wm);
/*---------------- END HACKS ---------------*/

struct elfdll_image
{
	HMODULE		pe_module_start;
	DWORD		pe_module_size;
	NE_MODULE	*ne_module_start;
	DWORD		ne_module_size;
};


/****************************************************************************
 *	get_sobasename	(internal)
 *
 */
static LPSTR get_sobasename(LPCSTR path, LPSTR name)
{
	char *cptr;

	/* Strip the path from the library name */
	if((cptr = strrchr(path, '/')))
	{
		char *cp = strrchr(cptr+1, '\\');
		if(cp && cp > cptr)
			cptr = cp+1;
		else
			cptr++;
	}
	else
		cptr = strrchr(path, '\\');

	if(!cptr)
		cptr = (char *)path;	/* No '/' nor '\\' in path */
	else
		cptr++;

	strcpy(name, cptr);
	cptr = strrchr(name, '.');
	if(cptr)
		*cptr = '\0';	/* Strip extension */

	/* Convert to lower case.
	 * This must be done manually because it is not sure that
	 * other modules are accessible.
	 */
	for(cptr = name; *cptr; cptr++)
		*cptr = tolower(*cptr);

	return name;
}


/****************************************************************************
 *	ELFDLL_CreateModref	(internal)
 *
 * INPUT
 *	hModule	- the header from the elf-dll's data-segment
 *	path	- requested path from original call
 *
 * OUTPUT
 *	A WINE_MODREF pointer to the new object
 *
 * BUGS
 *	- Does not handle errors due to dependencies correctly
 *	- path can be wrong
 */
#define RVA(base, va)	(((DWORD)base) + ((DWORD)va))

static WINE_MODREF *ELFDLL_CreateModref(HMODULE hModule, LPCSTR path)
{
	IMAGE_NT_HEADERS *nt = PE_HEADER(hModule);
	IMAGE_DATA_DIRECTORY *dir;
	IMAGE_IMPORT_DESCRIPTOR *pe_import = NULL;
	WINE_MODREF *wm;
	int len;
	HANDLE procheap = GetProcessHeap();

	wm = (WINE_MODREF *)HeapAlloc(procheap, HEAP_ZERO_MEMORY, sizeof(*wm));
	if(!wm)
		return NULL;

	wm->module = hModule;
	wm->type = MODULE32_PE;		/* FIXME */

	dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_EXPORT;
	if(dir->Size)
		wm->binfmt.pe.pe_export = (PIMAGE_EXPORT_DIRECTORY)RVA(hModule, dir->VirtualAddress);

	dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_IMPORT;
	if(dir->Size)
		pe_import = wm->binfmt.pe.pe_import = (PIMAGE_IMPORT_DESCRIPTOR)RVA(hModule, dir->VirtualAddress);

	dir = nt->OptionalHeader.DataDirectory+IMAGE_DIRECTORY_ENTRY_RESOURCE;
	if(dir->Size)
		wm->binfmt.pe.pe_resource = (PIMAGE_RESOURCE_DIRECTORY)RVA(hModule, dir->VirtualAddress);

	wm->modname = HEAP_strdupA(procheap, 0, (char *)RVA(hModule, wm->binfmt.pe.pe_export->Name));

	len = GetLongPathNameA(path, NULL, 0);
	wm->longname = (char *)HeapAlloc(procheap, 0, len+1);
	GetLongPathNameA(path, wm->longname, len+1);

	wm->shortname = HEAP_strdupA(procheap, 0, path);

	/* Link MODREF into process list */
	wm->next = PROCESS_Current()->modref_list;
	PROCESS_Current()->modref_list = wm;

	if(!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
	{
		if(PROCESS_Current()->exe_modref)
			FIXME(win32, "overwriting old exe_modref... arrgh\n");
		PROCESS_Current()->exe_modref = wm;
	}

	/* Fixup Imports */
	if(pe_import && fixup_imports(wm)) 
	{
		/* Error in this module or its dependencies
		 * remove entry from modref chain
		 */
		WINE_MODREF **xwm;
		for(xwm = &(PROCESS_Current()->modref_list); *xwm; xwm = &((*xwm)->next))
		{
			if ( *xwm == wm )
			{
				*xwm = wm->next;
				break;
			}
		}
		if(wm == PROCESS_Current()->exe_modref)
			ERR(win32, "Have to delete current exe_modref. Expect crash now\n");
		HeapFree(procheap, 0, wm->shortname);
		HeapFree(procheap, 0, wm->longname);
		HeapFree(procheap, 0, wm->modname);
		HeapFree(procheap, 0, wm);
		return NULL;

		/* FIXME: We should traverse back in the recursion
		 * with an error to unload everything that got loaded
		 * before this error occurred.
		 * Too dificult for now though and we don't care at the 
		 * moment. But, it *MUST* be implemented someday because
		 * we won't be able to map the elf-dll twice in this
		 * address-space which can cause some unexpected and
		 * weird problems later on.
		 */
	}

	return wm;
}


/***********************************************************************
 *           ELFDLL_CreateNEModule
 *
 * Create a dummy NE module for the win32 elf-dll based on the supplied
 * NE header in the elf-dll.
 */
static HMODULE16 ELFDLL_CreateNEModule(NE_MODULE *ne_image, DWORD size)
{
	HMODULE16 hModule = GLOBAL_CreateBlock(GMEM_MOVEABLE, ne_image, size, 0,
						FALSE, FALSE, FALSE, NULL);
	if(!hModule)
		return (HMODULE16)0;

	FarSetOwner16(hModule, hModule);
	NE_RegisterModule(ne_image);
	return hModule;
}


/****************************************************************************
 *	ELFDLL_LoadLibraryExA	(internal)
 *
 * Implementation of elf-dll loading for PE modules
 */
WINE_MODREF *ELFDLL_LoadLibraryExA(LPCSTR path, DWORD flags, DWORD *err)
{
	LPVOID dlhandle;
	struct elfdll_image *image;
	char name[129];
	char soname[129];
	HMODULE16 hmod16;
	WINE_MODREF *wm;

	get_sobasename(path, name);
	strcpy(soname, name);
	strcat(soname, ".so");

	/* Try to open the elf-dll.
	 * The dlopen will search the correct path and our extended
	 * LD_LIBRARY as well.
	 */
	dlhandle = dlopen(soname, RTLD_LAZY);
	if(!dlhandle)
	{
		WARN(elfdll, "Could not load %s (%s)\n", soname, dlerror());
		*err = ERROR_FILE_NOT_FOUND;
		return NULL;
	}

	/* Get the 'dllname_elfdll_image' variable */
	strcpy(soname, name);
	strcat(soname, "_elfdll_image");
	image = (struct elfdll_image *)dlsym(dlhandle, soname);
	if(!image) 
	{
		ERR(elfdll, "Could not get elfdll image descriptor %s (%s)\n", soname, dlerror());
		dlclose(dlhandle);
		*err = ERROR_BAD_FORMAT;
		return NULL;
	}

	/* Create a win16 dummy module */
	hmod16 = ELFDLL_CreateNEModule(image->ne_module_start, image->ne_module_size);
	if(!hmod16)
	{
		ERR(elfdll, "Could not create win16 dummy module for %s\n", path);
		dlclose(dlhandle);
		*err = ERROR_OUTOFMEMORY;
		return NULL;
	}

	image->ne_module_start->module32 = image->pe_module_start;

	wm = ELFDLL_CreateModref(image->pe_module_start, path);
	if(!wm)
	{
		ERR(elfdll, "Could not create WINE_MODREF for %s\n", path);
		GLOBAL_FreeBlock((HGLOBAL16)hmod16);
		dlclose(dlhandle);
		*err = ERROR_OUTOFMEMORY;
		return NULL;
	}

	*err = 0;
	return wm;
}


/****************************************************************************
 *	ELFDLL_UnloadLibrary	(internal)
 *
 * Unload an elf-dll completely from memory and deallocate the modref
 */
void ELFDLL_UnloadLibrary(WINE_MODREF *wm)
{
}


/****************************************************************************
 *	ELFDLL_LoadModule16	(internal)
 *
 * Implementation of elf-dll loading for NE modules
 */
HINSTANCE16 ELFDLL_LoadModule16(LPCSTR libname, BOOL implicit)
{
	return (HINSTANCE16)ERROR_FILE_NOT_FOUND;
}

#else

/*
 * No elfdlls possible 
 * Just put stubs in here.
 */

WINE_MODREF *ELFDLL_LoadLibraryExA(LPCSTR libname, DWORD flags, DWORD *err)
{
	*err = ERROR_FILE_NOT_FOUND;
	return NULL;
}

void ELFDLL_UnloadLibrary(WINE_MODREF *wm)
{
}

HINSTANCE16 ELFDLL_LoadModule16(LPCSTR libname, BOOL implicit)
{
	return (HINSTANCE16)ERROR_FILE_NOT_FOUND;
}

#endif

/*
 * Elf-dll loader functions
 *
 * Copyright 1999 Bertho A. Stultiens
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "config.h"
#include "windef.h"
#include "global.h"
#include "process.h"
#include "module.h"
#include "neexe.h"
#include "heap.h"
#include "wine/winbase16.h"
#include "elfdll.h"
#include "debugtools.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(elfdll)

#if defined(HAVE_DL_API)
#include <dlfcn.h>

/*------------------ HACKS -----------------*/
extern DWORD fixup_imports(WINE_MODREF *wm);
extern void dump_exports(HMODULE hModule);
/*---------------- END HACKS ---------------*/

char *extra_ld_library_path = NULL;	/* The extra search-path set in wine.conf */

struct elfdll_image
{
	HMODULE		pe_module_start;
	DWORD		pe_module_size;
	NE_MODULE	*ne_module_start;
	DWORD		ne_module_size;
};


/****************************************************************************
 *	ELFDLL_dlopen
 *
 * Wrapper for dlopen to search the EXTRA_LD_LIBRARY_PATH from wine.conf
 * manually because libdl.so caches the environment and does not accept our
 * changes.
 */
void *ELFDLL_dlopen(const char *libname, int flags)
{
	char buffer[256];
	int namelen;
	void *handle;
	char *ldpath;

	/* First try the default path search of dlopen() */
	handle = dlopen(libname, flags);
	if(handle)
		return handle;

	/* Now try to construct searches through our extra search-path */
	namelen = strlen(libname);
	ldpath = extra_ld_library_path;
	while(ldpath && *ldpath)
	{
		int len;
		char *cptr;
		char *from;

		from = ldpath;
		cptr = strchr(ldpath, ':');
		if(!cptr)
		{
			len = strlen(ldpath);
			ldpath = NULL;
		}
		else
		{
			len = cptr - ldpath;
			ldpath = cptr + 1;
		}

		if(len + namelen + 1 >= sizeof(buffer))
		{
			ERR("Buffer overflow! Check EXTRA_LD_LIBRARY_PATH or increase buffer size.\n");
			return NULL;
		}

		strncpy(buffer, from, len);
		if(len)
		{
			buffer[len] = '/';
			strcpy(buffer + len + 1, libname);
		}
		else
			strcpy(buffer + len, libname);

		TRACE("Trying dlopen('%s', %d)\n", buffer, flags);

		handle = dlopen(buffer, flags);
		if(handle)
			return handle;
		else
			TRACE("possible dlopen() error: %s\n", dlerror());
	}
	return NULL;
}


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
			cptr = cp;
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


	wm->filename = HEAP_strdupA( procheap, 0, path );
	wm->modname = strrchr( wm->filename, '\\' );
	if (!wm->modname) wm->modname = wm->filename;
	else wm->modname++;

	len = GetShortPathNameA( wm->filename, NULL, 0 );
	wm->short_filename = (char *)HeapAlloc( procheap, 0, len+1 );
	GetShortPathNameA( wm->filename, wm->short_filename, len+1 );
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
			FIXME( "Trying to load second .EXE file: %s\n", path );
		else
			PROCESS_Current()->exe_modref = wm;
	}

	LeaveCriticalSection( &PROCESS_Current()->crit_section );

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


/***********************************************************************
 *           ELFDLL_CreateNEModule
 *
 * Create a dummy NE module for the win32 elf-dll based on the supplied
 * NE header in the elf-dll.
 */
static HMODULE16 ELFDLL_CreateNEModule(NE_MODULE *ne_image, DWORD size)
{
	NE_MODULE *pModule;
	HMODULE16 hModule = GLOBAL_CreateBlock(GMEM_MOVEABLE, ne_image, size, 0,
						FALSE, FALSE, FALSE, NULL);
	if(!hModule)
		return (HMODULE16)0;

	FarSetOwner16(hModule, hModule);
	pModule = (NE_MODULE *)GlobalLock16(hModule);
	pModule->self = hModule;
	NE_RegisterModule(pModule);
	return hModule;
}


/****************************************************************************
 *	ELFDLL_LoadLibraryExA	(internal)
 *
 * Implementation of elf-dll loading for PE modules
 */
WINE_MODREF *ELFDLL_LoadLibraryExA(LPCSTR path, DWORD flags)
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

	/* Try to open the elf-dll */
	dlhandle = ELFDLL_dlopen(soname, RTLD_LAZY);
	if(!dlhandle)
	{
		WARN("Could not load %s (%s)\n", soname, dlerror());
		SetLastError( ERROR_FILE_NOT_FOUND );
		return NULL;
	}

	/* Get the 'dllname_elfdll_image' variable */
	strcpy(soname, name);
	strcat(soname, "_elfdll_image");
	image = (struct elfdll_image *)dlsym(dlhandle, soname);
	if(!image) 
	{
		ERR("Could not get elfdll image descriptor %s (%s)\n", soname, dlerror());
		dlclose(dlhandle);
		SetLastError( ERROR_BAD_FORMAT );
		return NULL;
	}

	/* Create a win16 dummy module */
	hmod16 = ELFDLL_CreateNEModule(image->ne_module_start, image->ne_module_size);
	if(!hmod16)
	{
		ERR("Could not create win16 dummy module for %s\n", path);
		dlclose(dlhandle);
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

	image->ne_module_start->module32 = image->pe_module_start;

	wm = ELFDLL_CreateModref(image->pe_module_start, path);
	if(!wm)
	{
		ERR("Could not create WINE_MODREF for %s\n", path);
		GLOBAL_FreeBlock((HGLOBAL16)hmod16);
		dlclose(dlhandle);
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

	dump_exports(image->pe_module_start);
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
HINSTANCE16 ELFDLL_LoadModule16(LPCSTR libname)
{
	return (HINSTANCE16)ERROR_FILE_NOT_FOUND;
}

#else

/*
 * No elfdlls possible 
 * Just put stubs in here.
 */

WINE_MODREF *ELFDLL_LoadLibraryExA(LPCSTR libname, DWORD flags)
{
        SetLastError( ERROR_FILE_NOT_FOUND );
	return NULL;
}

void ELFDLL_UnloadLibrary(WINE_MODREF *wm)
{
}

HINSTANCE16 ELFDLL_LoadModule16(LPCSTR libname)
{
	return (HINSTANCE16)ERROR_FILE_NOT_FOUND;
}

#endif

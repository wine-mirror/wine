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
	/* do NOT call dlerror() here ! (check after return) */
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
		/* do NOT call dlerror() here ! (check after return) */
		if(handle)
			return handle;
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

        wm = PE_CreateModule( image->pe_module_start, path, 0, -1, FALSE );
	if(!wm)
	{
		ERR("Could not create WINE_MODREF for %s\n", path);
		dlclose(dlhandle);
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}
        wm->dlhandle = dlhandle;

	dump_exports(image->pe_module_start);
	return wm;
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

#endif

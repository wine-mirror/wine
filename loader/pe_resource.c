#ifndef WINELIB
/*
 * PE (Portable Execute) File Resources
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1996 Martin von Loewis
 *
 * Based on the Win16 resource handling code in loader/resource.c
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 *
 * This is not even at ALPHA level yet. Don't expect it to work!
 */

#include <sys/types.h>
#include "wintypes.h"
#include "windows.h"
#include "pe_image.h"
#include "module.h"
#include "heap.h"
#include "handle32.h"
#include "libres.h"
#include "stackframe.h"
#include "neexe.h"
#include "accel.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

#define PrintIdA(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", name); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 
#define PrintIdW(name)
#define PrintId(name)

/**********************************************************************
 *	    GetResDirEntryW
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
LPIMAGE_RESOURCE_DIRECTORY GetResDirEntryW(LPIMAGE_RESOURCE_DIRECTORY resdirptr,
					   LPCWSTR name,DWORD root)
{
    int entrynum;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY entryTable;
    int namelen;

    if (HIWORD(name)) {
    /* FIXME: what about #xxx names? */
	entryTable = (LPIMAGE_RESOURCE_DIRECTORY_ENTRY) (
			(BYTE *) resdirptr + 
                        sizeof(IMAGE_RESOURCE_DIRECTORY));
	namelen = lstrlen32W(name);
	for (entrynum = 0; entrynum < resdirptr->NumberOfNamedEntries; entrynum++)
	{
		LPIMAGE_RESOURCE_DIR_STRING_U str =
		(LPIMAGE_RESOURCE_DIR_STRING_U) (root + 
			entryTable[entrynum].u1.s.NameOffset);
		if(namelen != str->Length)
			continue;
		if(lstrncmpi32W(name,str->NameString,str->Length)==0)
			return (LPIMAGE_RESOURCE_DIRECTORY) (
				root +
				entryTable[entrynum].u2.s.OffsetToDirectory);
	}
	return NULL;
    } else {
	entryTable = (LPIMAGE_RESOURCE_DIRECTORY_ENTRY) (
			(BYTE *) resdirptr + 
                        sizeof(IMAGE_RESOURCE_DIRECTORY) +
			resdirptr->NumberOfNamedEntries * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
	for (entrynum = 0; entrynum < resdirptr->NumberOfIdEntries; entrynum++)
	    if ((DWORD)entryTable[entrynum].u1.Name == (DWORD)name)
		return (LPIMAGE_RESOURCE_DIRECTORY) (
			root +
			entryTable[entrynum].u2.s.OffsetToDirectory);
	/* just use first entry if no default can be found */
	if (!name && resdirptr->NumberOfIdEntries)
		return (LPIMAGE_RESOURCE_DIRECTORY) (
			root +
			entryTable[0].u2.s.OffsetToDirectory);
	return NULL;
    }
}

/**********************************************************************
 *	    GetResDirEntryA
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
LPIMAGE_RESOURCE_DIRECTORY GetResDirEntryA(LPIMAGE_RESOURCE_DIRECTORY resdirptr,
					   LPCSTR name,
					   DWORD root)
{
	LPWSTR				xname;
	LPIMAGE_RESOURCE_DIRECTORY	ret;

	if (HIWORD((DWORD)name))
		xname	= HEAP_strdupAtoW( GetProcessHeap(), 0, name );
	else
		xname	= (LPWSTR)name;

	ret=GetResDirEntryW(resdirptr,xname,root);
	if (HIWORD((DWORD)name))
            HeapFree( GetProcessHeap(), 0, xname );
	return ret;
}

/**********************************************************************
 *	    PE_FindResourceEx32W
 */
HANDLE32 PE_FindResourceEx32W(
	HINSTANCE32 hModule,LPCWSTR name,LPCWSTR type,WORD lang
) {
    PE_MODULE *pe;
    NE_MODULE *pModule;
    LPIMAGE_RESOURCE_DIRECTORY resdirptr;
    DWORD root;
    HANDLE32 result;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "FindResource: module=%08x type=", hModule );
    PrintId( type );
    dprintf_resource( stddeb, " name=" );
    PrintId( name );
    dprintf_resource( stddeb, "\n" );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;  /* FIXME? */
    if (!(pe = pModule->pe_module) || !pe->pe_resource) return 0;

    resdirptr = pe->pe_resource;
    root = (DWORD) resdirptr;
    if ((resdirptr = GetResDirEntryW(resdirptr, type, root)) == NULL)
	return 0;
    if ((resdirptr = GetResDirEntryW(resdirptr, name, root)) == NULL)
	return 0;
    result = (HANDLE32)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT32)lang, root);
	/* Try LANG_NEUTRAL, too */
    if(!result)
        return (HANDLE32)GetResDirEntryW(resdirptr, (LPCWSTR)0, root);
    return result;
}


/**********************************************************************
 *	    PE_LoadResource32
 */
HANDLE32 PE_LoadResource32( HINSTANCE32 hModule, HANDLE32 hRsrc )
{
    NE_MODULE *pModule;
    PE_MODULE *pe;

    if (!hModule) hModule = GetTaskDS(); /* FIXME: see FindResource32W */
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "PE_LoadResource32: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;  /* FIXME? */
    if (!(pe = pModule->pe_module) || !pe->pe_resource) return 0;
    return (HANDLE32) (pe->load_addr+((LPIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->OffsetToData);
}
#endif

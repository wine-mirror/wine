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
 * Copyright 1997 Marcus Meissner
 */

#include <stdlib.h>
#include <sys/types.h>
#include "wintypes.h"
#include "windows.h"
#include "pe_image.h"
#include "module.h"
#include "heap.h"
#include "libres.h"
#include "stackframe.h"
#include "neexe.h"
#include "stddebug.h"
#include "debug.h"

/**********************************************************************
 *  HMODULE32toPE_MODULE 
 *
 * small helper function to get a PE_MODULE from a passed HMODULE32
 */
static PE_MODULE*
HMODULE32toPE_MODULE(HMODULE32 hmod) {
	NE_MODULE	*pModule;

	if (!hmod) hmod = GetTaskDS(); /* FIXME: correct? */
	hmod = GetExePtr( hmod );  /* In case we were passed an hInstance */

	if (!(pModule = MODULE_GetPtr( hmod ))) return 0;
	if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;
	return pModule->pe_module;
}

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
    	if (name[0]=='#') {
		char	buf[10];

		lstrcpynWtoA(buf,name+1,10);
		return GetResDirEntryW(resdirptr,(LPCWSTR)atoi(buf),root);
	}
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
 *	    PE_FindResourceEx32W
 */
HANDLE32 PE_FindResourceEx32W(
	HINSTANCE32 hModule,LPCWSTR name,LPCWSTR type,WORD lang
) {
    LPIMAGE_RESOURCE_DIRECTORY resdirptr;
    DWORD root;
    HANDLE32 result;
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hModule);

    if (!pe || !pe->pe_resource)
    	return 0;

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
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hModule);

    if (!pe || !pe->pe_resource)
    	return 0;
    if (!hRsrc)
   	 return 0;
    return (HANDLE32) (pe->load_addr+((LPIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->OffsetToData);
}


/**********************************************************************
 *	    PE_SizeofResource32
 */
void
_check_ptr(DWORD x,DWORD start,LPDWORD lastmax) {
	if ((x>start) && (x<*lastmax))
		*lastmax=x;
}

static void
walk_resdir(DWORD loadaddr,DWORD rootresdir,DWORD xres,DWORD data,DWORD lvl,LPDWORD max){
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DATA_ENTRY		dataent;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    int	i;

    if (lvl==3) {
    	dataent = (LPIMAGE_RESOURCE_DATA_ENTRY)(rootresdir+xres);
	_check_ptr(loadaddr+dataent->OffsetToData,data,max);
	return;
    }
    resdir = (LPIMAGE_RESOURCE_DIRECTORY)(rootresdir+xres);
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++)
	walk_resdir(loadaddr,rootresdir,(lvl==2)?et[i].u2.OffsetToData:et[i].u2.s.OffsetToDirectory,data,lvl+1,max);
}

DWORD PE_SizeofResource32( HINSTANCE32 hModule, HANDLE32 hRsrc )
{
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hModule);
    DWORD	max,data;
    IMAGE_DATA_DIRECTORY	dir;

    if (!pe || !pe->pe_resource)
    	return 0;
    if (!hRsrc) return 0;

    max=(DWORD)-1;
    dir=pe->pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    if(dir.Size)
    	max=(DWORD)pe->pe_resource+dir.Size;

    data=((DWORD)pe->load_addr+((LPIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->OffsetToData);
    walk_resdir(pe->load_addr,(DWORD)pe->pe_resource,0,data,0,&max);
    return max-data;
}

/**********************************************************************
 *	    PE_EnumResourceTypes32A
 */
BOOL32
PE_EnumResourceTypes32A(HMODULE32 hmod,ENUMRESTYPEPROC32A lpfun,LONG lparam) {
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hmod);
    int		i;
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL32	ret;
    HANDLE32	heap = GetProcessHeap();	

    if (!pe || !pe->pe_resource)
    	return FALSE;

    resdir = (LPIMAGE_RESOURCE_DIRECTORY)pe->pe_resource;
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
    	LPSTR	name;

	if (HIWORD(et[i].u1.Name))
		name = HEAP_strdupWtoA(heap,0,(LPWSTR)((LPBYTE)pe->pe_resource+et[i].u1.Name));
	else
		name = (LPSTR)et[i].u1.Name;
	ret = lpfun(hmod,name,lparam);
	if (HIWORD(name))
		HeapFree(heap,0,name);
	if (!ret)
		break;
    }
    return ret;
}

/**********************************************************************
 *	    PE_EnumResourceTypes32W
 */
BOOL32
PE_EnumResourceTypes32W(HMODULE32 hmod,ENUMRESTYPEPROC32W lpfun,LONG lparam) {
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hmod);
    int		i;
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL32	ret;

    if (!pe || !pe->pe_resource)
    	return FALSE;

    resdir = (LPIMAGE_RESOURCE_DIRECTORY)pe->pe_resource;
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	LPWSTR	type;
    	if (HIWORD(et[i].u1.Name))
		type = (LPWSTR)((LPBYTE)pe->pe_resource+et[i].u1.Name);
	else
		type = (LPWSTR)et[i].u1.Name;

	ret = lpfun(hmod,type,lparam);
	if (!ret)
		break;
    }
    return ret;
}

/**********************************************************************
 *	    PE_EnumResourceNames32A
 */
BOOL32
PE_EnumResourceNames32A(
	HMODULE32 hmod,LPCSTR type,ENUMRESNAMEPROC32A lpfun,LONG lparam
) {
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hmod);
    int		i;
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL32	ret;
    HANDLE32	heap = GetProcessHeap();	
    LPWSTR	typeW;

    if (!pe || !pe->pe_resource)
    	return FALSE;
    resdir = (LPIMAGE_RESOURCE_DIRECTORY)pe->pe_resource;
    if (HIWORD(type))
	typeW = HEAP_strdupAtoW(heap,0,type);
    else
	typeW = (LPWSTR)type;
    resdir = GetResDirEntryW(resdir,typeW,(DWORD)pe->pe_resource);
    if (HIWORD(typeW))
    	HeapFree(heap,0,typeW);
    if (!resdir)
    	return FALSE;
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
    	LPSTR	name;

	if (HIWORD(et[i].u1.Name))
	    name = HEAP_strdupWtoA(heap,0,(LPWSTR)((LPBYTE)pe->pe_resource+et[i].u1.Name));
	else
	    name = (LPSTR)et[i].u1.Name;
	ret = lpfun(hmod,type,name,lparam);
	if (HIWORD(name)) HeapFree(heap,0,name);
	if (!ret)
		break;
    }
    return ret;
}

/**********************************************************************
 *	    PE_EnumResourceNames32W
 */
BOOL32
PE_EnumResourceNames32W(
	HMODULE32 hmod,LPCWSTR type,ENUMRESNAMEPROC32W lpfun,LONG lparam
) {
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hmod);
    int		i;
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL32	ret;

    if (!pe || !pe->pe_resource)
    	return FALSE;

    resdir = (LPIMAGE_RESOURCE_DIRECTORY)pe->pe_resource;
    resdir = GetResDirEntryW(resdir,type,(DWORD)pe->pe_resource);
    if (!resdir)
    	return FALSE;
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	LPWSTR	name;
    	if (HIWORD(et[i].u1.Name))
		name = (LPWSTR)((LPBYTE)pe->pe_resource+et[i].u1.Name);
	else
		name = (LPWSTR)et[i].u1.Name;
	ret = lpfun(hmod,type,name,lparam);
	if (!ret)
		break;
    }
    return ret;
}

/**********************************************************************
 *	    PE_EnumResourceNames32A
 */
BOOL32
PE_EnumResourceLanguages32A(
	HMODULE32 hmod,LPCSTR name,LPCSTR type,ENUMRESLANGPROC32A lpfun,
	LONG lparam
) {
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hmod);
    int		i;
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL32	ret;
    HANDLE32	heap = GetProcessHeap();	
    LPWSTR	nameW,typeW;

    if (!pe || !pe->pe_resource)
    	return FALSE;

    resdir = (LPIMAGE_RESOURCE_DIRECTORY)pe->pe_resource;
    if (HIWORD(name))
	nameW = HEAP_strdupAtoW(heap,0,name);
    else
    	nameW = (LPWSTR)name;
    resdir = GetResDirEntryW(resdir,nameW,(DWORD)pe->pe_resource);
    if (HIWORD(nameW))
    	HeapFree(heap,0,nameW);
    if (!resdir)
    	return FALSE;
    if (HIWORD(type))
	typeW = HEAP_strdupAtoW(heap,0,type);
    else
	typeW = (LPWSTR)type;
    resdir = GetResDirEntryW(resdir,typeW,(DWORD)pe->pe_resource);
    if (HIWORD(typeW))
    	HeapFree(heap,0,typeW);
    if (!resdir)
    	return FALSE;
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
    	/* languages are just ids... I hope */
	ret = lpfun(hmod,name,type,et[i].u1.Id,lparam);
	if (!ret)
		break;
    }
    return ret;
}

/**********************************************************************
 *	    PE_EnumResourceLanguages32W
 */
BOOL32
PE_EnumResourceLanguages32W(
	HMODULE32 hmod,LPCWSTR name,LPCWSTR type,ENUMRESLANGPROC32W lpfun,
	LONG lparam
) {
    PE_MODULE	*pe = HMODULE32toPE_MODULE(hmod);
    int		i;
    LPIMAGE_RESOURCE_DIRECTORY		resdir;
    LPIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL32	ret;

    if (!pe || !pe->pe_resource)
    	return FALSE;

    resdir = (LPIMAGE_RESOURCE_DIRECTORY)pe->pe_resource;
    resdir = GetResDirEntryW(resdir,name,(DWORD)pe->pe_resource);
    if (!resdir)
    	return FALSE;
    resdir = GetResDirEntryW(resdir,type,(DWORD)pe->pe_resource);
    if (!resdir)
    	return FALSE;
    et =(LPIMAGE_RESOURCE_DIRECTORY_ENTRY)((LPBYTE)resdir+sizeof(IMAGE_RESOURCE_DIRECTORY));
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	ret = lpfun(hmod,name,type,et[i].u1.Id,lparam);
	if (!ret)
		break;
    }
    return ret;
}
#endif

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
#include "wine/winestring.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winnls.h"
#include "module.h"
#include "heap.h"
#include "task.h"
#include "process.h"
#include "stackframe.h"
#include "neexe.h"
#include "debugtools.h"

/**********************************************************************
 *  get_resdir
 *
 * Get the resource directory of a PE module
 */
static IMAGE_RESOURCE_DIRECTORY* get_resdir( HMODULE hmod )
{
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_RESOURCE_DIRECTORY *ret = NULL;

    if (!hmod) hmod = GetModuleHandleA( NULL );
    dir = &PE_HEADER(hmod)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    if (dir->Size && dir->VirtualAddress)
        ret = (IMAGE_RESOURCE_DIRECTORY *)((char *)hmod + dir->VirtualAddress);
    return ret;
}


/**********************************************************************
 *	    GetResDirEntryW
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
PIMAGE_RESOURCE_DIRECTORY GetResDirEntryW(PIMAGE_RESOURCE_DIRECTORY resdirptr,
					   LPCWSTR name,DWORD root,
					   BOOL allowdefault)
{
    int entrynum;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entryTable;
    int namelen;

    if (HIWORD(name)) {
    	if (name[0]=='#') {
		char	buf[10];
                if (!WideCharToMultiByte( CP_ACP, 0, name+1, -1, buf, sizeof(buf), NULL, NULL ))
                    return NULL;
		return GetResDirEntryW(resdirptr,(LPCWSTR)atoi(buf),root,allowdefault);
	}
	entryTable = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdirptr + 1);
	namelen = strlenW(name);
	for (entrynum = 0; entrynum < resdirptr->NumberOfNamedEntries; entrynum++)
	{
		PIMAGE_RESOURCE_DIR_STRING_U str =
		(PIMAGE_RESOURCE_DIR_STRING_U) (root + 
			entryTable[entrynum].u1.s.NameOffset);
		if(namelen != str->Length)
			continue;
		if(strncmpiW(name,str->NameString,str->Length)==0)
			return (PIMAGE_RESOURCE_DIRECTORY) (
				root +
				entryTable[entrynum].u2.s.OffsetToDirectory);
	}
	return NULL;
    } else {
	entryTable = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (
			(BYTE *) resdirptr + 
                        sizeof(IMAGE_RESOURCE_DIRECTORY) +
			resdirptr->NumberOfNamedEntries * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
	for (entrynum = 0; entrynum < resdirptr->NumberOfIdEntries; entrynum++)
	    if ((DWORD)entryTable[entrynum].u1.Name == (DWORD)name)
		return (PIMAGE_RESOURCE_DIRECTORY) (
			root +
			entryTable[entrynum].u2.s.OffsetToDirectory);
	/* just use first entry if no default can be found */
	if (allowdefault && !name && resdirptr->NumberOfIdEntries)
		return (PIMAGE_RESOURCE_DIRECTORY) (
			root +
			entryTable[0].u2.s.OffsetToDirectory);
	return NULL;
    }
}

/**********************************************************************
 *	    GetResDirEntryA
 */
PIMAGE_RESOURCE_DIRECTORY GetResDirEntryA( PIMAGE_RESOURCE_DIRECTORY resdirptr,
					   LPCSTR name, DWORD root,
					   BOOL allowdefault )
{
    PIMAGE_RESOURCE_DIRECTORY retv;
    LPWSTR nameW = HIWORD(name)? HEAP_strdupAtoW( GetProcessHeap(), 0, name ) 
                               : (LPWSTR)name;

    retv = GetResDirEntryW( resdirptr, nameW, root, allowdefault );

    if ( HIWORD(name) ) HeapFree( GetProcessHeap(), 0, nameW );

    return retv;
}

/**********************************************************************
 *	    PE_FindResourceExW
 *
 * FindResourceExA/W does search in the following order:
 * 1. Exact specified language
 * 2. Language with neutral sublanguage
 * 3. Neutral language with neutral sublanguage
 * 4. Neutral language with default sublanguage
 */
HRSRC PE_FindResourceExW( HMODULE hmod, LPCWSTR name, LPCWSTR type, WORD lang )
{
    PIMAGE_RESOURCE_DIRECTORY resdirptr = get_resdir(hmod);
    DWORD root;
    HRSRC result;

    if (!resdirptr) return 0;

    root = (DWORD) resdirptr;
    if ((resdirptr = GetResDirEntryW(resdirptr, type, root, FALSE)) == NULL)
	return 0;
    if ((resdirptr = GetResDirEntryW(resdirptr, name, root, FALSE)) == NULL)
	return 0;

    /* 1. Exact specified language */
    result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);

    if (!result)
    {
	/* 2. Language with neutral sublanguage */
        lang = MAKELANGID(PRIMARYLANGID(lang), SUBLANG_NEUTRAL);
        result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    if (!result)
    {
	/* 3. Neutral language with neutral sublanguage */
        lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    if (!result)
    {
	/* 4. Neutral language with default sublanguage */
        lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    return result;
}

/**********************************************************************
 *	    PE_FindResourceW
 *
 * Load[String]/[Icon]/[Menu]/[etc.] does use FindResourceA/W.
 * FindResourceA/W does search in the following order:
 * 1. Neutral language with neutral sublanguage
 * 2. Neutral language with default sublanguage
 * 3. Current locale lang id
 * 4. Current locale lang id with neutral sublanguage
 * 5. (!) LANG_ENGLISH, SUBLANG_DEFAULT
 * 6. Return first in the list
 */
HRSRC PE_FindResourceW( HMODULE hmod, LPCWSTR name, LPCWSTR type )
{
    PIMAGE_RESOURCE_DIRECTORY resdirptr = get_resdir(hmod);
    DWORD root;
    HRSRC result;
    WORD lang;

    if (!resdirptr) return 0;

    root = (DWORD) resdirptr;
    if ((resdirptr = GetResDirEntryW(resdirptr, type, root, FALSE)) == NULL)
	return 0;
    if ((resdirptr = GetResDirEntryW(resdirptr, name, root, FALSE)) == NULL)
	return 0;

    /* 1. Neutral language with neutral sublanguage */
    lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);

    if (!result)
    {
	/* 2. Neutral language with default sublanguage */
	lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    if (!result)
    {
	/* 3. Current locale lang id */
	lang = LANGIDFROMLCID(GetUserDefaultLCID());
	result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    if (!result)
    {
	/* 4. Current locale lang id with neutral sublanguage */
	lang = MAKELANGID(PRIMARYLANGID(lang), SUBLANG_NEUTRAL);
	result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    if (!result)
    {
	/* 5. (!) LANG_ENGLISH, SUBLANG_DEFAULT */
	lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
	result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)lang, root, FALSE);
    }

    if (!result)
    {
	/* 6. Return first in the list */
	result = (HANDLE)GetResDirEntryW(resdirptr, (LPCWSTR)(UINT)0, root, TRUE);
    }

    return result;
}


/**********************************************************************
 *	    PE_LoadResource
 */
HANDLE PE_LoadResource( HMODULE hmod, HANDLE hRsrc )
{
    if (!hRsrc) return 0;
    return (HANDLE)(hmod + ((PIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->OffsetToData);
}


/**********************************************************************
 *	    PE_SizeofResource
 */
DWORD PE_SizeofResource( HANDLE hRsrc )
{
    if (!hRsrc) return 0;
    return ((PIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->Size;
}


/**********************************************************************
 *	EnumResourceTypesA	(KERNEL32.90)
 */
BOOL WINAPI EnumResourceTypesA( HMODULE hmod, ENUMRESTYPEPROCA lpfun, LONG lparam)
{
    int		i;
    PIMAGE_RESOURCE_DIRECTORY resdir = get_resdir(hmod);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL	ret;
    HANDLE	heap = GetProcessHeap();	

    if (!resdir) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
    	LPSTR	name;

	if (et[i].u1.s.NameIsString)
		name = HEAP_strdupWtoA(heap,0,(LPWSTR)((LPBYTE)resdir + et[i].u1.s.NameOffset));
	else
		name = (LPSTR)(int)et[i].u1.Id;
	ret = lpfun(hmod,name,lparam);
	if (HIWORD(name))
		HeapFree(heap,0,name);
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.91)
 */
BOOL WINAPI EnumResourceTypesW( HMODULE hmod, ENUMRESTYPEPROCW lpfun, LONG lparam)
{
    int		i;
    PIMAGE_RESOURCE_DIRECTORY		resdir = get_resdir(hmod);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL	ret;

    if (!resdir) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	LPWSTR	type;
    	if (et[i].u1.s.NameIsString)
		type = (LPWSTR)((LPBYTE)resdir + et[i].u1.s.NameOffset);
	else
		type = (LPWSTR)(int)et[i].u1.Id;

	ret = lpfun(hmod,type,lparam);
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.88)
 */
BOOL WINAPI EnumResourceNamesA( HMODULE hmod, LPCSTR type, ENUMRESNAMEPROCA lpfun, LONG lparam )
{
    int		i;
    PIMAGE_RESOURCE_DIRECTORY		basedir = get_resdir(hmod);
    PIMAGE_RESOURCE_DIRECTORY		resdir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL	ret;
    HANDLE	heap = GetProcessHeap();	
    LPWSTR	typeW;

    if (!basedir) return FALSE;

    if (HIWORD(type))
	typeW = HEAP_strdupAtoW(heap,0,type);
    else
	typeW = (LPWSTR)type;
    resdir = GetResDirEntryW(basedir,typeW,(DWORD)basedir,FALSE);
    if (HIWORD(typeW))
    	HeapFree(heap,0,typeW);
    if (!resdir)
    	return FALSE;
    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
    	LPSTR	name;

	if (et[i].u1.s.NameIsString)
	    name = HEAP_strdupWtoA(heap,0,(LPWSTR)((LPBYTE)basedir + et[i].u1.s.NameOffset));
	else
	    name = (LPSTR)(int)et[i].u1.Id;
	ret = lpfun(hmod,type,name,lparam);
	if (HIWORD(name)) HeapFree(heap,0,name);
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.89)
 */
BOOL WINAPI EnumResourceNamesW( HMODULE hmod, LPCWSTR type, ENUMRESNAMEPROCW lpfun, LONG lparam )
{
    int		i;
    PIMAGE_RESOURCE_DIRECTORY		basedir = get_resdir(hmod);
    PIMAGE_RESOURCE_DIRECTORY		resdir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL	ret;

    if (!basedir) return FALSE;

    resdir = GetResDirEntryW(basedir,type,(DWORD)basedir,FALSE);
    if (!resdir)
    	return FALSE;
    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	LPWSTR	name;
    	if (et[i].u1.s.NameIsString)
		name = (LPWSTR)((LPBYTE)basedir + et[i].u1.s.NameOffset);
	else
		name = (LPWSTR)(int)et[i].u1.Id;
	ret = lpfun(hmod,type,name,lparam);
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.86)
 */
BOOL WINAPI EnumResourceLanguagesA( HMODULE hmod, LPCSTR type, LPCSTR name,
                                    ENUMRESLANGPROCA lpfun, LONG lparam )
{
    int		i;
    PIMAGE_RESOURCE_DIRECTORY		basedir = get_resdir(hmod);
    PIMAGE_RESOURCE_DIRECTORY		resdir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL	ret;
    HANDLE	heap = GetProcessHeap();	

    if (!basedir) return FALSE;

    if (HIWORD(type))
    {
	LPWSTR typeW = HEAP_strdupAtoW(heap,0,type);
        resdir = GetResDirEntryW(basedir,typeW,(DWORD)basedir,FALSE);
    	HeapFree(heap,0,typeW);
    }
    else resdir = GetResDirEntryW(basedir,(LPWSTR)type,(DWORD)basedir,FALSE);
    if (!resdir)
    	return FALSE;

    if (HIWORD(name))
    {
	LPWSTR nameW = HEAP_strdupAtoW(heap,0,name);
        resdir = GetResDirEntryW(resdir,nameW,(DWORD)basedir,FALSE);
    	HeapFree(heap,0,nameW);
    }
    else resdir = GetResDirEntryW(resdir,(LPWSTR)name,(DWORD)basedir,FALSE);
    if (!resdir)
    	return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
    	/* languages are just ids... I hope */
	ret = lpfun(hmod,type,name,et[i].u1.Id,lparam);
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.87)
 */
BOOL WINAPI EnumResourceLanguagesW( HMODULE hmod, LPCWSTR type, LPCWSTR name,
                                    ENUMRESLANGPROCW lpfun, LONG lparam )
{
    int		i;
    PIMAGE_RESOURCE_DIRECTORY		basedir = get_resdir(hmod);
    PIMAGE_RESOURCE_DIRECTORY		resdir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY	et;
    BOOL	ret;

    if (!basedir) return FALSE;

    resdir = GetResDirEntryW(basedir,type,(DWORD)basedir,FALSE);
    if (!resdir)
    	return FALSE;
    resdir = GetResDirEntryW(resdir,name,(DWORD)basedir,FALSE);
    if (!resdir)
    	return FALSE;
    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	ret = lpfun(hmod,type,name,et[i].u1.Id,lparam);
	if (!ret)
		break;
    }
    return ret;
}

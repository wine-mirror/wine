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
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <sys/types.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/unicode.h"
#include "windef.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);


/**********************************************************************
 *  get_resdir
 *
 * Get the resource directory of a PE module
 */
static const IMAGE_RESOURCE_DIRECTORY* get_resdir( HMODULE hmod )
{
    DWORD size;

    if (!hmod) hmod = GetModuleHandleA( NULL );
    else if (!HIWORD(hmod))
    {
        FIXME("Enumeration of 16-bit resources is not supported\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return RtlImageDirectoryEntryToData( hmod, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size );
}


/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         WORD id, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int min, max, pos;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    min = dir->NumberOfNamedEntries;
    max = min + dir->NumberOfIdEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (entry[pos].u1.s2.Id == id)
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].u2.s3.OffsetToDirectory);
        if (entry[pos].u1.s2.Id > id) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}


/**********************************************************************
 *  find_entry_by_nameW
 *
 * Find an entry by name in a resource directory
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_nameW( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                            LPCWSTR name, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    const IMAGE_RESOURCE_DIR_STRING_U *str;
    int min, max, res, pos, namelen;

    if (!HIWORD(name)) return find_entry_by_id( dir, LOWORD(name), root );
    if (name[0] == '#')
    {
        char buf[16];
        if (!WideCharToMultiByte( CP_ACP, 0, name+1, -1, buf, sizeof(buf), NULL, NULL ))
            return NULL;
        return find_entry_by_id( dir, atoi(buf), root );
    }

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    namelen = strlenW(name);
    min = 0;
    max = dir->NumberOfNamedEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        str = (IMAGE_RESOURCE_DIR_STRING_U *)((char *)root + entry[pos].u1.s1.NameOffset);
        res = strncmpiW( name, str->NameString, str->Length );
        if (!res && namelen == str->Length)
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].u2.s3.OffsetToDirectory);
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}


/**********************************************************************
 *  find_entry_by_nameA
 *
 * Find an entry by name in a resource directory
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_nameA( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                            LPCSTR name, const void *root )
{
    const IMAGE_RESOURCE_DIRECTORY *ret = NULL;
    LPWSTR nameW;
    INT len;

    if (!HIWORD(name)) return find_entry_by_id( dir, LOWORD(name), root );
    if (name[0] == '#')
    {
        return find_entry_by_id( dir, atoi(name+1), root );
    }

    len = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
    if ((nameW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, len );
        ret = find_entry_by_nameW( dir, nameW, root );
        HeapFree( GetProcessHeap(), 0, nameW );
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceTypesA( HMODULE hmod, ENUMRESTYPEPROCA lpfun, LONG lparam)
{
    int		i;
    const IMAGE_RESOURCE_DIRECTORY *resdir = get_resdir(hmod);
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    BOOL	ret;

    if (!resdir) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
        LPSTR type;

        if (et[i].u1.s1.NameIsString)
        {
            PIMAGE_RESOURCE_DIR_STRING_U pResString = (PIMAGE_RESOURCE_DIR_STRING_U) ((LPBYTE) resdir + et[i].u1.s1.NameOffset);
            DWORD len = WideCharToMultiByte( CP_ACP, 0, pResString->NameString, pResString->Length,
                                             NULL, 0, NULL, NULL);
            if (!(type = HeapAlloc(GetProcessHeap(), 0, len + 1)))
                return FALSE;
            WideCharToMultiByte( CP_ACP, 0, pResString->NameString, pResString->Length,
                                 type, len, NULL, NULL);
            type[len] = '\0';
            ret = lpfun(hmod,type,lparam);
            HeapFree(GetProcessHeap(), 0, type);
        }
        else
        {
            type = (LPSTR)(int)et[i].u1.s2.Id;
            ret = lpfun(hmod,type,lparam);
        }
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceTypesW( HMODULE hmod, ENUMRESTYPEPROCW lpfun, LONG lparam)
{
    int		i;
    const IMAGE_RESOURCE_DIRECTORY *resdir = get_resdir(hmod);
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    BOOL	ret;

    if (!resdir) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	LPWSTR	type;

        if (et[i].u1.s1.NameIsString)
        {
            PIMAGE_RESOURCE_DIR_STRING_U pResString = (PIMAGE_RESOURCE_DIR_STRING_U) ((LPBYTE) resdir + et[i].u1.s1.NameOffset);
            if (!(type = HeapAlloc(GetProcessHeap(), 0, (pResString->Length+1) * sizeof (WCHAR))))
                return FALSE;
            memcpy(type, pResString->NameString, pResString->Length * sizeof (WCHAR));
            type[pResString->Length] = '\0';
            ret = lpfun(hmod,type,lparam);
            HeapFree(GetProcessHeap(), 0, type);
        }
        else
        {
            type = (LPWSTR)(int)et[i].u1.s2.Id;
            ret = lpfun(hmod,type,lparam);
        }
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceNamesA( HMODULE hmod, LPCSTR type, ENUMRESNAMEPROCA lpfun, LONG lparam )
{
    int		i;
    const IMAGE_RESOURCE_DIRECTORY *basedir = get_resdir(hmod);
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    BOOL	ret;

    if (!basedir) return FALSE;

    if (!(resdir = find_entry_by_nameA( basedir, type, basedir ))) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
        LPSTR name;

        if (et[i].u1.s1.NameIsString)
        {
            PIMAGE_RESOURCE_DIR_STRING_U pResString = (PIMAGE_RESOURCE_DIR_STRING_U) ((LPBYTE) basedir + et[i].u1.s1.NameOffset);
            DWORD len = WideCharToMultiByte(CP_ACP, 0, pResString->NameString, pResString->Length,
                                            NULL, 0, NULL, NULL);
            if (!(name = HeapAlloc(GetProcessHeap(), 0, len + 1 )))
                return FALSE;
            WideCharToMultiByte( CP_ACP, 0, pResString->NameString, pResString->Length,
                                 name, len, NULL, NULL );
            name[len] = '\0';
            ret = lpfun(hmod,type,name,lparam);
            HeapFree( GetProcessHeap(), 0, name );
        }
        else
        {
            name = (LPSTR)(int)et[i].u1.s2.Id;
            ret = lpfun(hmod,type,name,lparam);
        }
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceNamesW( HMODULE hmod, LPCWSTR type, ENUMRESNAMEPROCW lpfun, LONG lparam )
{
    int		i;
    const IMAGE_RESOURCE_DIRECTORY *basedir = get_resdir(hmod);
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    BOOL	ret;

    if (!basedir) return FALSE;

    if (!(resdir = find_entry_by_nameW( basedir, type, basedir ))) return FALSE;

    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
        LPWSTR name;

        if (et[i].u1.s1.NameIsString)
        {
            PIMAGE_RESOURCE_DIR_STRING_U pResString = (PIMAGE_RESOURCE_DIR_STRING_U) ((LPBYTE) basedir + et[i].u1.s1.NameOffset);
            if (!(name = HeapAlloc(GetProcessHeap(), 0, (pResString->Length + 1) * sizeof (WCHAR))))
                return FALSE;
            memcpy(name, pResString->NameString, pResString->Length * sizeof (WCHAR));
            name[pResString->Length] = '\0';
            ret = lpfun(hmod,type,name,lparam);
            HeapFree(GetProcessHeap(), 0, name);
        }
        else
        {
            name = (LPWSTR)(int)et[i].u1.s2.Id;
            ret = lpfun(hmod,type,name,lparam);
        }
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceLanguagesA( HMODULE hmod, LPCSTR type, LPCSTR name,
                                    ENUMRESLANGPROCA lpfun, LONG lparam )
{
    int		i;
    const IMAGE_RESOURCE_DIRECTORY *basedir = get_resdir(hmod);
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    BOOL	ret;

    if (!basedir) return FALSE;
    if (!(resdir = find_entry_by_nameA( basedir, type, basedir ))) return FALSE;
    if (!(resdir = find_entry_by_nameA( resdir, name, basedir ))) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
        /* languages are just ids... I hope */
	ret = lpfun(hmod,type,name,et[i].u1.s2.Id,lparam);
	if (!ret)
		break;
    }
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceLanguagesW( HMODULE hmod, LPCWSTR type, LPCWSTR name,
                                    ENUMRESLANGPROCW lpfun, LONG lparam )
{
    int		i;
    const IMAGE_RESOURCE_DIRECTORY *basedir = get_resdir(hmod);
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    BOOL	ret;

    if (!basedir) return FALSE;

    if (!(resdir = find_entry_by_nameW( basedir, type, basedir ))) return FALSE;
    if (!(resdir = find_entry_by_nameW( resdir, name, basedir ))) return FALSE;

    et =(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    ret = FALSE;
    for (i=0;i<resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries;i++) {
	ret = lpfun(hmod,type,name,et[i].u1.s2.Id,lparam);
	if (!ret)
		break;
    }
    return ret;
}

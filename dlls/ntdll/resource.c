/*
 * PE file resources
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1996 Martin von Loewis
 * Copyright 2003 Alexandre Julliard
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
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "thread.h"
#include "excpt.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);

static LCID user_lcid, system_lcid;

static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/**********************************************************************
 *  is_data_file_module
 *
 * Check if a module handle is for a LOAD_LIBRARY_AS_DATAFILE module.
 */
inline static int is_data_file_module( HMODULE hmod )
{
    return (ULONG_PTR)hmod & 1;
}


/**********************************************************************
 *  push_language
 *
 * push a language in the list of languages to try
 */
static inline int push_language( WORD *list, int pos, WORD lang )
{
    int i;
    for (i = 0; i < pos; i++) if (list[i] == lang) return pos;
    list[pos++] = lang;
    return pos;
}


/**********************************************************************
 *  find_first_entry
 *
 * Find the first suitable entry in a resource directory
 */
static const IMAGE_RESOURCE_DIRECTORY *find_first_entry( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         const void *root, int want_dir )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    int pos;

    for (pos = 0; pos < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; pos++)
    {
        if (!entry[pos].u2.s3.DataIsDirectory == !want_dir)
            return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].u2.s3.OffsetToDirectory);
    }
    return NULL;
}


/**********************************************************************
 *  find_entry_by_id
 *
 * Find an entry by id in a resource directory
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                         WORD id, const void *root, int want_dir )
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
        {
            if (!entry[pos].u2.s3.DataIsDirectory == !want_dir)
            {
                TRACE("root %p dir %p id %04x ret %p\n",
                      root, dir, id, (char *)root + entry[pos].u2.s3.OffsetToDirectory);
                return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].u2.s3.OffsetToDirectory);
            }
            break;
        }
        if (entry[pos].u1.s2.Id > id) max = pos - 1;
        else min = pos + 1;
    }
    TRACE("root %p dir %p id %04x not found\n", root, dir, id );
    return NULL;
}


/**********************************************************************
 *  find_entry_by_name
 *
 * Find an entry by name in a resource directory
 */
static const IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( const IMAGE_RESOURCE_DIRECTORY *dir,
                                                           LPCWSTR name, const void *root,
                                                           int want_dir )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    const IMAGE_RESOURCE_DIR_STRING_U *str;
    int min, max, res, pos, namelen;

    if (!HIWORD(name)) return find_entry_by_id( dir, LOWORD(name), root, want_dir );
    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    namelen = strlenW(name);
    min = 0;
    max = dir->NumberOfNamedEntries - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        str = (IMAGE_RESOURCE_DIR_STRING_U *)((char *)root + entry[pos].u1.s1.NameOffset);
        res = strncmpW( name, str->NameString, str->Length );
        if (!res && namelen == str->Length)
        {
            if (!entry[pos].u2.s3.DataIsDirectory == !want_dir)
            {
                TRACE("root %p dir %p name %s ret %p\n",
                      root, dir, debugstr_w(name), (char *)root + entry[pos].u2.s3.OffsetToDirectory);
                return (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry[pos].u2.s3.OffsetToDirectory);
            }
            break;
        }
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    TRACE("root %p dir %p name %s not found\n", root, dir, debugstr_w(name) );
    return NULL;
}


/**********************************************************************
 *  find_entry
 *
 * Find a resource entry
 */
static NTSTATUS find_entry( HMODULE hmod, const LDR_RESOURCE_INFO *info,
                            ULONG level, const void **ret, int want_dir )
{
    ULONG size;
    const void *root;
    const IMAGE_RESOURCE_DIRECTORY *resdirptr;
    WORD list[9];  /* list of languages to try */
    int i, pos = 0;

    root = RtlImageDirectoryEntryToData( hmod, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size );
    if (!root) return STATUS_RESOURCE_DATA_NOT_FOUND;
    resdirptr = root;

    if (!level--) goto done;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Type, root, want_dir || level )))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;

    resdirptr = *ret;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Name, root, want_dir || level )))
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;
    if (level) return STATUS_INVALID_PARAMETER;  /* level > 3 */

    /* 1. specified language */
    pos = push_language( list, pos, info->Language );

    /* 2. specified language with neutral sublanguage */
    pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(info->Language), SUBLANG_NEUTRAL ) );

    /* 3. neutral language with neutral sublanguage */
    pos = push_language( list, pos, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );

    /* if no explicitly specified language, try some defaults */
    if (PRIMARYLANGID(info->Language) == LANG_NEUTRAL)
    {
        /* user defaults, unless SYS_DEFAULT sublanguage specified  */
        if (SUBLANGID(info->Language) != SUBLANG_SYS_DEFAULT)
        {
            /* 4. current thread locale language */
            pos = push_language( list, pos, LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale) );

            /* 5. user locale language */
            pos = push_language( list, pos, LANGIDFROMLCID(user_lcid) );

            /* 6. user locale language with neutral sublanguage  */
            pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(user_lcid), SUBLANG_NEUTRAL ) );
        }

        /* now system defaults */

        /* 7. system locale language */
        pos = push_language( list, pos, LANGIDFROMLCID( system_lcid ) );

        /* 8. system locale language with neutral sublanguage */
        pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(system_lcid), SUBLANG_NEUTRAL ) );

        /* 9. English */
        pos = push_language( list, pos, MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    }

    resdirptr = *ret;
    for (i = 0; i < pos; i++)
        if ((*ret = find_entry_by_id( resdirptr, list[i], root, want_dir ))) return STATUS_SUCCESS;

    /* if no explicitly specified language, return the first entry */
    if (PRIMARYLANGID(info->Language) == LANG_NEUTRAL)
    {
        if ((*ret = find_first_entry( resdirptr, root, want_dir ))) return STATUS_SUCCESS;
    }
    return STATUS_RESOURCE_LANG_NOT_FOUND;

done:
    *ret = resdirptr;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *	LdrFindResourceDirectory_U  (NTDLL.@)
 */
NTSTATUS WINAPI LdrFindResourceDirectory_U( HMODULE hmod, const LDR_RESOURCE_INFO *info,
                                            ULONG level, const IMAGE_RESOURCE_DIRECTORY **dir )
{
    const void *res;
    NTSTATUS status;

    if (info) TRACE( "module %p type %s name %s lang %04lx level %ld\n",
                     hmod, debugstr_w((LPCWSTR)info->Type),
                     level > 1 ? debugstr_w((LPCWSTR)info->Name) : "",
                     level > 2 ? info->Language : 0, level );

    __TRY
    {
        status = find_entry( hmod, info, level, &res, TRUE );
        if (status == STATUS_SUCCESS) *dir = res;
    }
    __EXCEPT(page_fault)
    {
        return GetExceptionCode();
    }
    __ENDTRY;
    return status;
}


/**********************************************************************
 *	LdrFindResource_U  (NTDLL.@)
 */
NTSTATUS WINAPI LdrFindResource_U( HMODULE hmod, const LDR_RESOURCE_INFO *info,
                                   ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry )
{
    const void *res;
    NTSTATUS status;

    if (info) TRACE( "module %p type %s name %s lang %04lx level %ld\n",
                     hmod, debugstr_w((LPCWSTR)info->Type),
                     level > 1 ? debugstr_w((LPCWSTR)info->Name) : "",
                     level > 2 ? info->Language : 0, level );

    __TRY
    {
        status = find_entry( hmod, info, level, &res, FALSE );
        if (status == STATUS_SUCCESS) *entry = res;
    }
    __EXCEPT(page_fault)
    {
        return GetExceptionCode();
    }
    __ENDTRY;
    return status;
}


/**********************************************************************
 *	LdrAccessResource  (NTDLL.@)
 */
NTSTATUS WINAPI LdrAccessResource( HMODULE hmod, const IMAGE_RESOURCE_DATA_ENTRY *entry,
                                   void **ptr, ULONG *size )
{
    NTSTATUS status;

    __TRY
    {
        ULONG dirsize;

        if (!RtlImageDirectoryEntryToData( hmod, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &dirsize ))
            status = STATUS_RESOURCE_DATA_NOT_FOUND;
        else
        {
            if (ptr)
            {
                if (is_data_file_module(hmod))
                {
                    HMODULE mod = (HMODULE)((ULONG_PTR)hmod & ~1);
                    *ptr = RtlImageRvaToVa( RtlImageNtHeader(mod), mod, entry->OffsetToData, NULL );
                }
                else *ptr = (char *)hmod + entry->OffsetToData;
            }
            if (size) *size = entry->Size;
            status = STATUS_SUCCESS;
        }
    }
    __EXCEPT(page_fault)
    {
        return GetExceptionCode();
    }
    __ENDTRY;
    return status;
}


/**********************************************************************
 *	RtlFindMessage  (NTDLL.@)
 */
NTSTATUS WINAPI RtlFindMessage( HMODULE hmod, ULONG type, ULONG lang,
                                ULONG msg_id, const MESSAGE_RESOURCE_ENTRY **ret )
{
    const MESSAGE_RESOURCE_DATA *data;
    const MESSAGE_RESOURCE_BLOCK *block;
    const IMAGE_RESOURCE_DATA_ENTRY *rsrc;
    LDR_RESOURCE_INFO info;
    NTSTATUS status;
    void *ptr;
    int i;

    info.Type     = type;
    info.Name     = 1;
    info.Language = lang;

    if ((status = LdrFindResource_U( hmod, &info, 3, &rsrc )) != STATUS_SUCCESS)
        return status;
    if ((status = LdrAccessResource( hmod, rsrc, &ptr, NULL )) != STATUS_SUCCESS)
        return status;

    data = ptr;
    block = data->Blocks;
    for (i = 0; i < data->NumberOfBlocks; i++, block++)
    {
        if (msg_id >= block->LowId && msg_id <= block->HighId)
        {
            const MESSAGE_RESOURCE_ENTRY *entry;

            entry = (MESSAGE_RESOURCE_ENTRY *)((char *)data + block->OffsetToEntries);
            for (i = msg_id - block->LowId; i > 0; i--)
                entry = (MESSAGE_RESOURCE_ENTRY *)((char *)entry + entry->Length);
            *ret = entry;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_MESSAGE_NOT_FOUND;
}


/**********************************************************************
 *	NtQueryDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultLocale( BOOLEAN user, LCID *lcid )
{
    *lcid = user ? user_lcid : system_lcid;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *	NtSetDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultLocale( BOOLEAN user, LCID lcid )
{
    if (user) user_lcid = lcid;
    else system_lcid = lcid;
    return STATUS_SUCCESS;
}

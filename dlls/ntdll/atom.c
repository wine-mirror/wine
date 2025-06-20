/*
 * Atom table functions
 *
 * Copyright 1993, 1994, 1995, 2021 Alexandre Julliard
 * Copyright 2004,2005 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "ntdll_misc.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(atom);

#define MAX_ATOM_LEN    255
#define MAX_ATOMS       0x4000
#define IS_INTATOM(x)   (((ULONG_PTR)(x) >> 16) == 0)
#define TABLE_SIGNATURE 0x6d6f7441  /* 'Atom' */
#define HASH_SIZE       37
#define MIN_HASH_SIZE   4
#define MAX_HASH_SIZE   0x200

struct atom_handle
{
    RTL_HANDLE            hdr;
    RTL_ATOM_TABLE_ENTRY *entry;
};


static NTSTATUS lock_atom_table( RTL_ATOM_TABLE table )
{
    if (!table) return STATUS_INVALID_PARAMETER;
    if (table->Signature != TABLE_SIGNATURE) return STATUS_INVALID_PARAMETER;
    RtlEnterCriticalSection( &table->CriticalSection );
    return STATUS_SUCCESS;
}

static void unlock_atom_table( RTL_ATOM_TABLE table )
{
    RtlLeaveCriticalSection( &table->CriticalSection );
}

static ULONG hash_str( RTL_ATOM_TABLE table, const WCHAR *name, ULONG len )
{
    UNICODE_STRING str = { len * sizeof(WCHAR), len * sizeof(WCHAR), (WCHAR *)name };
    ULONG hash;

    RtlHashUnicodeString( &str, TRUE, HASH_STRING_ALGORITHM_X65599, &hash );
    return hash % table->NumberOfBuckets;
}

static RTL_ATOM_TABLE_ENTRY *find_entry( RTL_ATOM_TABLE table, const WCHAR *name, ULONG len, ULONG hash )
{
    RTL_ATOM_TABLE_ENTRY *entry = table->Buckets[hash];

    while (entry)
    {
        if (!RtlCompareUnicodeStrings( entry->Name, entry->NameLength, name, len, TRUE )) break;
        entry = entry->HashLink;
    }
    return entry;
}

static NTSTATUS add_atom( RTL_ATOM_TABLE table, const WCHAR *name, ULONG len, RTL_ATOM *atom )
{
    RTL_ATOM_TABLE_ENTRY *entry;
    struct atom_handle *handle;
    ULONG index, hash = hash_str( table, name, len );

    if ((entry = find_entry( table, name, len, hash )))  /* exists already */
    {
        entry->ReferenceCount++;
        *atom = entry->Atom;
        return STATUS_SUCCESS;
    }

    if (!(handle = (struct atom_handle *)RtlAllocateHandle( &table->HandleTable, &index )))
        return STATUS_NO_MEMORY;

    if (!(entry = RtlAllocateHeap( GetProcessHeap(), 0, offsetof( RTL_ATOM_TABLE_ENTRY, Name[len] ) )))
    {
        RtlFreeHandle( &table->HandleTable, &handle->hdr );
        return STATUS_NO_MEMORY;
    }
    entry->HandleIndex = index;
    entry->Atom = MAXINTATOM + index;
    entry->ReferenceCount = 1;
    entry->Flags = 0;
    entry->NameLength = len;
    entry->HashLink = table->Buckets[hash];
    memcpy( entry->Name, name, len * sizeof(WCHAR) );
    handle->hdr.Next = (RTL_HANDLE *)1;
    table->Buckets[hash] = entry;
    handle->entry = entry;
    *atom = entry->Atom;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		is_integral_atom
 * Returns STATUS_SUCCESS if integral atom and 'pAtom' is filled
 *         STATUS_INVALID_PARAMETER if 'atomstr' is too long
 *         STATUS_MORE_ENTRIES otherwise
 */
static NTSTATUS is_integral_atom( LPCWSTR atomstr, size_t len, RTL_ATOM* pAtom )
{
    RTL_ATOM atom;

    if (!IS_INTATOM( atomstr ))
    {
        const WCHAR* ptr = atomstr;
        if (!len) return STATUS_OBJECT_NAME_INVALID;

        if (*ptr++ == '#')
        {
            atom = 0;
            while (ptr < atomstr + len && *ptr >= '0' && *ptr <= '9')
            {
                atom = atom * 10 + *ptr++ - '0';
            }
            if (ptr > atomstr + 1 && ptr == atomstr + len) goto done;
        }
        if (len > MAX_ATOM_LEN) return STATUS_INVALID_PARAMETER;
        return STATUS_MORE_ENTRIES;
    }
    else if ((atom = LOWORD( atomstr )) >= MAXINTATOM) return STATUS_INVALID_PARAMETER;
done:
    if (atom >= MAXINTATOM) atom = 0;
    if (!(*pAtom = atom)) return STATUS_INVALID_PARAMETER;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlDeleteAtomFromAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlDeleteAtomFromAtomTable( RTL_ATOM_TABLE table, RTL_ATOM atom )
{
    NTSTATUS status;
    struct atom_handle *handle;

    if ((status = lock_atom_table( table ))) return status;

    if (!atom) status = STATUS_INVALID_HANDLE;
    else if (atom < MAXINTATOM) status = STATUS_SUCCESS;
    else if (RtlIsValidIndexHandle( &table->HandleTable, atom - MAXINTATOM, (RTL_HANDLE **)&handle ))
    {
        if (handle->entry->Flags) status = STATUS_WAS_LOCKED;
        else if (!--handle->entry->ReferenceCount)
        {
            ULONG hash = hash_str( table, handle->entry->Name, handle->entry->NameLength );
            RTL_ATOM_TABLE_ENTRY **ptr = &table->Buckets[hash];

            while (*ptr != handle->entry) ptr = &(*ptr)->HashLink;
            *ptr = (*ptr)->HashLink;
            RtlFreeHeap( GetProcessHeap(), 0, handle->entry );
            RtlFreeHandle( &table->HandleTable, &handle->hdr );
        }
    }
    else status = STATUS_INVALID_HANDLE;

    unlock_atom_table( table );
    TRACE( "%p %x\n", table, atom );
    return status;
}

/******************************************************************
 *		integral_atom_name (internal)
 *
 * Helper for fetching integral (local/global) atoms names.
 */
static ULONG integral_atom_name(WCHAR* buffer, ULONG len, RTL_ATOM atom)
{
    WCHAR tmp[16];
    int ret;

    ret = swprintf( tmp, ARRAY_SIZE(tmp), L"#%u", atom );
    if (!len) return ret * sizeof(WCHAR);
    if (len <= ret) ret = len - 1;
    memcpy( buffer, tmp, ret * sizeof(WCHAR) );
    buffer[ret] = 0;
    return ret * sizeof(WCHAR);
}

/******************************************************************
 *		RtlQueryAtomInAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryAtomInAtomTable( RTL_ATOM_TABLE table, RTL_ATOM atom, ULONG* ref,
                                         ULONG* pin, WCHAR* name, ULONG* len )
{
    NTSTATUS status;
    struct atom_handle *handle;
    ULONG wlen = 0;

    if (!atom) return STATUS_INVALID_PARAMETER;
    if ((status = lock_atom_table( table ))) return status;

    if (atom < MAXINTATOM)
    {
        if (len) wlen = integral_atom_name( name, *len, atom);
        if (ref) *ref = 1;
        if (pin) *pin = 1;
    }
    else if (RtlIsValidIndexHandle( &table->HandleTable, atom - MAXINTATOM, (RTL_HANDLE **)&handle ))
    {
        wlen = handle->entry->NameLength * sizeof(WCHAR);
        if (ref) *ref = handle->entry->ReferenceCount;
        if (pin) *pin = handle->entry->Flags;
        if (len && *len)
        {
            wlen = min( *len - sizeof(WCHAR), wlen );
            if (name)
            {
                memcpy( name, handle->entry->Name, wlen );
                name[wlen / sizeof(WCHAR)] = 0;
            }
        }
    }
    else status = STATUS_INVALID_HANDLE;

    unlock_atom_table( table );

    if (status == STATUS_SUCCESS && len)
    {
        if (!*len) status = STATUS_BUFFER_TOO_SMALL;
        *len = wlen;
    }
    TRACE( "%p %x -> %s (%lx)\n",
           table, atom, len ? debugstr_wn(name, wlen / sizeof(WCHAR)) : "(null)", status );
    return status;
}

/******************************************************************
 *		RtlCreateAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateAtomTable( ULONG size, RTL_ATOM_TABLE *ret_table )
{
    RTL_ATOM_TABLE table;

    if (*ret_table) return size ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

    if ((size < MIN_HASH_SIZE) || (size > MAX_HASH_SIZE)) size = HASH_SIZE;

    if (!(table = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   offsetof( struct _RTL_ATOM_TABLE, Buckets[size] ))))
        return STATUS_NO_MEMORY;

    table->Signature       = TABLE_SIGNATURE;
    table->NumberOfBuckets = size;

    RtlInitializeCriticalSection( &table->CriticalSection );
    RtlInitializeHandleTable( MAX_ATOMS, sizeof(struct atom_handle), &table->HandleTable );
    *ret_table = table;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlDestroyAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlDestroyAtomTable( RTL_ATOM_TABLE table )
{
    if (!table || table->Signature != TABLE_SIGNATURE) return STATUS_INVALID_PARAMETER;
    RtlEmptyAtomTable( table, TRUE );
    RtlDestroyHandleTable( &table->HandleTable );
    RtlDeleteCriticalSection( &table->CriticalSection );
    table->Signature = 0;
    RtlFreeHeap( GetProcessHeap(), 0, table );
    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlAddAtomToAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlAddAtomToAtomTable( RTL_ATOM_TABLE table, const WCHAR* name, RTL_ATOM* atom )
{
    NTSTATUS status;
    size_t len;

    if ((status = lock_atom_table( table ))) return status;

    len = IS_INTATOM( name ) ? 0 : wcslen( name );
    status = is_integral_atom( name, len, atom );
    if (status == STATUS_MORE_ENTRIES) status = add_atom( table, name, len, atom );
    unlock_atom_table( table );
    TRACE( "%p %s -> %x\n", table, debugstr_w(name), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		RtlLookupAtomInAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlLookupAtomInAtomTable( RTL_ATOM_TABLE table, const WCHAR* name, RTL_ATOM* atom )
{
    RTL_ATOM_TABLE_ENTRY *entry;
    NTSTATUS status;
    ULONG len;

    if ((status = lock_atom_table( table ))) return status;

    len = IS_INTATOM(name) ? 0 : wcslen(name);
    status = is_integral_atom( name, len, atom );
    if (status == STATUS_MORE_ENTRIES)
    {
        if ((entry = find_entry( table, name, len, hash_str( table, name, len ))))
        {
            *atom = entry->Atom;
            status = STATUS_SUCCESS;
        }
        else status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    unlock_atom_table( table );
    TRACE( "%p %s -> %x\n",
           table, debugstr_w(name), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		RtlEmptyAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlEmptyAtomTable( RTL_ATOM_TABLE table, BOOLEAN delete_pinned )
{
    struct atom_handle *handle;
    NTSTATUS status;
    ULONG i;

    if ((status = lock_atom_table( table ))) return status;

    for (i = 0; i < table->NumberOfBuckets; i++)
    {
        RTL_ATOM_TABLE_ENTRY **ptr = &table->Buckets[i];
        while (*ptr)
        {
            if (!delete_pinned && (*ptr)->Flags) ptr = &(*ptr)->HashLink;
            else
            {
                RTL_ATOM_TABLE_ENTRY *entry = *ptr;
                *ptr = entry->HashLink;
                if (RtlIsValidIndexHandle( &table->HandleTable, entry->HandleIndex,
                                           (RTL_HANDLE **)&handle ))
                    RtlFreeHandle( &table->HandleTable, &handle->hdr );
                RtlFreeHeap( GetProcessHeap(), 0, entry );
            }
        }
    }
    unlock_atom_table( table );
    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlPinAtomInAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlPinAtomInAtomTable( RTL_ATOM_TABLE table, RTL_ATOM atom )
{
    struct atom_handle *handle;
    NTSTATUS status;

    if ((status = lock_atom_table( table ))) return status;

    if (atom >= MAXINTATOM && RtlIsValidIndexHandle( &table->HandleTable, atom - MAXINTATOM,
                                                     (RTL_HANDLE **)&handle ))
    {
        handle->entry->Flags = 1;
    }
    unlock_atom_table( table );
    return STATUS_SUCCESS;
}

/*
 * Atom table functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "ntstatus.h"

#include "wine/server.h"
#include "wine/unicode.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(atom);

#define MAX_ATOM_LEN              255

/******************************************************************
 *		is_integral_atom
 * Returns STATUS_SUCCESS if integral atom and 'pAtom' is filled
 *         STATUS_INVALID_PARAMETER if 'atomstr' is too long
 *         STATUS_MORE_ENTRIES otherwise
 */
static NTSTATUS is_integral_atom( LPCWSTR atomstr, size_t len, RTL_ATOM* pAtom )
{
    RTL_ATOM atom;

    if (HIWORD( atomstr ))
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
    else atom = LOWORD( atomstr );
done:
    if (!atom || atom >= MAXINTATOM) return STATUS_INVALID_PARAMETER;
    *pAtom = atom;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		RtlDeleteAtomFromAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlDeleteAtomFromAtomTable( RTL_ATOM_TABLE table, RTL_ATOM atom )
{
    NTSTATUS    status;

    TRACE( "%p %x\n", table, atom );
    if (!table) status = STATUS_INVALID_PARAMETER;
    else
    {
        SERVER_START_REQ( delete_atom )
        {
            req->atom = atom;
            req->table = table;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    return status;
}

/******************************************************************
 *		RtlQueryAtomInAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryAtomInAtomTable( RTL_ATOM_TABLE table, RTL_ATOM atom, ULONG* ref,
                                         ULONG* pin, WCHAR* name, ULONG* len )
{
    NTSTATUS    status = STATUS_SUCCESS;
    WCHAR       full_name[MAX_ATOM_LEN];
    DWORD       wlen = 0;

    if (!table) status = STATUS_INVALID_PARAMETER;
    else if (atom < MAXINTATOM)
    {
        if (!atom) return STATUS_INVALID_PARAMETER;
        if (len)
        {
            static WCHAR fmt[] = {'#','%','d',0};
            wlen = sprintfW( full_name, fmt, atom ) * sizeof(WCHAR);
        }
        if (ref) *ref = 1;
        if (pin) *pin = 1;
    }
    else
    {
        SERVER_START_REQ( get_atom_information )
        {
            req->atom = atom;
            req->table = table;
            if (len) wine_server_set_reply( req, full_name, sizeof(full_name) );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                wlen = wine_server_reply_size( reply );
                if (ref) *ref = reply->count;
                if (pin) *pin = reply->pinned;
            }
        }
        SERVER_END_REQ;
    }
    if (status == STATUS_SUCCESS && len)
    {
        if (*len > wlen)
        {
            memcpy( name, full_name, wlen );
            name[wlen / sizeof(WCHAR)] = 0;
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        *len = wlen;
    }

    TRACE( "%p %x -> %s (%lu)\n",
           table, atom, len ? debugstr_w(name) : NULL, status );
    return status;
}

/******************************************************************
 *		RtlCreateAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateAtomTable( ULONG size, RTL_ATOM_TABLE* table )
{
    NTSTATUS    status;

    if (*table)
    {
        if (size) status = STATUS_INVALID_PARAMETER;
        else status = STATUS_SUCCESS;
    }
    else
    {
        SERVER_START_REQ( init_atom_table )
        {
            req->entries = size;
            status = wine_server_call( req );
            *table = reply->table;
        }
        SERVER_END_REQ;
    }
    return status;
}

/******************************************************************
 *		RtlDestroyAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlDestroyAtomTable( RTL_ATOM_TABLE table )
{
    if (!table) return STATUS_INVALID_PARAMETER;
    return NtClose( (HANDLE)table );
}

/******************************************************************
 *		RtlAddAtomToAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlAddAtomToAtomTable( RTL_ATOM_TABLE table, const WCHAR* name, RTL_ATOM* atom )
{
    NTSTATUS    status;

    if (!table) status = STATUS_INVALID_PARAMETER;
    else
    {
        size_t len = HIWORD(name) ? strlenW(name) : 0;
        status = is_integral_atom( name, len, atom );
        if (status == STATUS_MORE_ENTRIES)
        {
            SERVER_START_REQ( add_atom )
            {
                wine_server_add_data( req, name, len * sizeof(WCHAR) );
                req->table = table;
                status = wine_server_call( req );
                *atom = reply->atom;
            }
            SERVER_END_REQ;
        }
    }
    TRACE( "%p %s -> %x\n",
           table, debugstr_w(name), status == STATUS_SUCCESS ? *atom : 0 );

    return status;
}

/******************************************************************
 *		RtlLookupAtomInAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlLookupAtomInAtomTable( RTL_ATOM_TABLE table, const WCHAR* name, RTL_ATOM* atom )
{
    NTSTATUS    status;

    if (!table) status = STATUS_INVALID_PARAMETER;
    else
    {
        size_t len = HIWORD(name) ? strlenW(name) : 0;
        status = is_integral_atom( name, len, atom );
        if (status == STATUS_MORE_ENTRIES)
        {
            SERVER_START_REQ( find_atom )
            {
                wine_server_add_data( req, name, len * sizeof(WCHAR) );
                req->table = table;
                status = wine_server_call( req );
                *atom = reply->atom;
            }
            SERVER_END_REQ;
        }
    }
    TRACE( "%p %s -> %x\n",
           table, debugstr_w(name), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		RtlEmptyAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlEmptyAtomTable( RTL_ATOM_TABLE table, BOOLEAN delete_pinned )
{
    NTSTATUS    status;

    if (!table) status = STATUS_INVALID_PARAMETER;
    else
    {
        SERVER_START_REQ( empty_atom_table )
        {
            req->table = table;
            req->if_pinned = delete_pinned;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    return status;
}

/******************************************************************
 *		RtlPinAtomInAtomTable (NTDLL.@)
 */
NTSTATUS WINAPI RtlPinAtomInAtomTable( RTL_ATOM_TABLE table, RTL_ATOM atom )
{
    NTSTATUS status;

    if (!table) return STATUS_INVALID_PARAMETER;
    if (atom < MAXINTATOM) return STATUS_SUCCESS;

    SERVER_START_REQ( set_atom_information )
    {
        req->table = table;
        req->atom = atom;
        req->pinned = TRUE;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    return status;
}

/*************************************************
 *        Global handle table management
 *************************************************/

/******************************************************************
 *		NtAddAtom (NTDLL.@)
 */
NTSTATUS WINAPI NtAddAtom( const WCHAR* name, ULONG length, RTL_ATOM* atom )
{
    NTSTATUS    status;

    status = is_integral_atom( name, length / sizeof(WCHAR), atom );
    if (status == STATUS_MORE_ENTRIES)
    {
        SERVER_START_REQ( add_atom )
        {
            wine_server_add_data( req, name, length );
            req->table = NULL;
            status = wine_server_call( req );
            *atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    TRACE( "%s -> %x\n",
           debugstr_wn(name, length/sizeof(WCHAR)), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		NtDeleteAtom (NTDLL.@)
 */
NTSTATUS WINAPI NtDeleteAtom(RTL_ATOM atom)
{
    NTSTATUS    status;

    SERVER_START_REQ( delete_atom )
    {
        req->atom = atom;
        req->table = NULL;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *		NtFindAtom (NTDLL.@)
 */
NTSTATUS WINAPI NtFindAtom( const WCHAR* name, ULONG length, RTL_ATOM* atom )
{
    NTSTATUS    status;

    status = is_integral_atom( name, length / sizeof(WCHAR), atom );
    if (status == STATUS_MORE_ENTRIES)
    {
        SERVER_START_REQ( find_atom )
        {
            wine_server_add_data( req, name, length );
            req->table = NULL;
            status = wine_server_call( req );
            *atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    TRACE( "%s -> %x\n",
           debugstr_wn(name, length/sizeof(WCHAR)), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		NtQueryInformationAtom (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationAtom( RTL_ATOM atom, ATOM_INFORMATION_CLASS class,
                                        RTL_ATOM_TABLE ptr, ULONG size, PULONG psize )
{
    NTSTATUS status;

    switch (class)
    {
    case AtomBasicInformation:
        {
            ULONG name_len;
            ATOM_BASIC_INFORMATION* abi = (ATOM_BASIC_INFORMATION*)ptr;

            name_len = size - (sizeof(ATOM_BASIC_INFORMATION) - sizeof(WCHAR));

            if (atom < MAXINTATOM)
            {
                if (!atom) status = STATUS_INVALID_PARAMETER;
                else if (name_len >= 7 * sizeof(WCHAR))
                {
                    static WCHAR fmt[] = {'#','%','d',0};
                    abi->NameLength = snprintfW( abi->Name, name_len / sizeof(WCHAR), fmt, atom ) * sizeof(WCHAR);
                    abi->ReferenceCount = 1;
                    abi->Pinned = 1;
                    status = STATUS_SUCCESS;
                }
                else status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                SERVER_START_REQ( get_atom_information )
                {
                    req->atom = atom;
                    req->table = NULL;
                    if (name_len) wine_server_set_reply( req, abi->Name, name_len );
                    status = wine_server_call( req );
                    name_len = wine_server_reply_size( reply );
                    if (status == STATUS_SUCCESS)
                    {
                        abi->NameLength = name_len;
                        abi->ReferenceCount = reply->count;
                        abi->Pinned = reply->pinned;
                    }
                }
                SERVER_END_REQ;
            }
            TRACE( "%x -> %s (%lu)\n", atom, debugstr_wn(abi->Name, name_len/sizeof(WCHAR)), status );
            if (psize)
                *psize = sizeof(ATOM_BASIC_INFORMATION) - sizeof(WCHAR) + name_len;
        }
        break;
    default:
        FIXME( "Unsupported class %u\n", class );
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }
    return status;
}

/*
 * Atom table functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"

#include "wine/exception.h"
#include "kernel_private.h"

#define MAX_ATOM_LEN 255
#define IS_INTATOM(x)   (((ULONG_PTR)(x) >> 16) == 0)

/******************************************************************
 *		get_local_table
 *
 * Returns the local atom table for this process (and create it if doesn't
 * exist yet)
 */
static RTL_ATOM_TABLE get_local_table(DWORD entries)
{
    static RTL_ATOM_TABLE local_table;

    if (!local_table)
    {
        RTL_ATOM_TABLE  table = NULL;

        if (!set_ntstatus( RtlCreateAtomTable( entries, &table ))) return NULL;
        if (InterlockedCompareExchangePointer((void*)&local_table, table, NULL) != NULL)
            RtlDestroyAtomTable( table );
    }

    return local_table;
}


/***********************************************************************
 *           InitAtomTable   (KERNEL32.@)
 *
 * Initialise local atom table.
 *
 * PARAMS
 *  entries [I] The number of entries to reserve in the table.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI InitAtomTable( DWORD entries )
{
    return get_local_table( entries ) != 0;
}

/******************************************************************
 *		check_integral_atom
 *
 * Check if a string (ANSI or UNICODE) is in fact an integral atom
 * (but doesn't check the "#1234" form)
 */
static inline BOOL check_integral_atom( const void* ptr, ATOM* patom)
{
    if (!IS_INTATOM( ptr )) return FALSE;
    if ((*patom = LOWORD( ptr )) >= MAXINTATOM)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        *patom = 0;
    }
    return TRUE;
}

/***********************************************************************
 *           GlobalAddAtomA   (KERNEL32.@)
 *
 * Add a character string to the global atom table and return a unique
 * value identifying it.
 *
 * RETURNS
 *	Success: The atom allocated to str.
 *	Failure: 0.
 */
ATOM WINAPI GlobalAddAtomA( LPCSTR str /* [in] String to add */ )
{
    ATOM atom = 0;
    __TRY
    {
        if (!check_integral_atom( str, &atom ))
	{
	    /* Note: Adobe Reader XI in protected mode hijacks NtAddAtom and its replacement expects the string to be null terminated. */
	    WCHAR buffer[MAX_ATOM_LEN+1];
	    DWORD len = MultiByteToWideChar( CP_ACP, 0, str, strlen(str), buffer, MAX_ATOM_LEN );
	    buffer[len] = 0;
	    if (!len) SetLastError( ERROR_INVALID_PARAMETER );
	    else if (!set_ntstatus( NtAddAtom( buffer, len * sizeof(WCHAR), &atom ))) atom = 0;
	}
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        atom = 0;
    }
    __ENDTRY
    return atom;
}


/***********************************************************************
 *           AddAtomA   (KERNEL32.@)
 *
 * Add a character string to local atom table and return a unique
 * value identifying it.
 *
 * RETURNS
 *      Success: The atom allocated to str.
 *      Failure: 0.
 */
ATOM WINAPI AddAtomA( LPCSTR str /* [in] String to add */ )
{
    ATOM atom = 0;

    if (!check_integral_atom( str, &atom ))
    {
        WCHAR           buffer[MAX_ATOM_LEN + 1];
        DWORD           len;
        RTL_ATOM_TABLE  table;

        len = MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, MAX_ATOM_LEN + 1 );
        if (!len) SetLastError( ERROR_INVALID_PARAMETER );
        else if ((table = get_local_table( 0 )))
        {
            if (!set_ntstatus( RtlAddAtomToAtomTable( table, buffer, &atom ))) return 0;
        }
    }
    return atom;
}

/***********************************************************************
 *           GlobalAddAtomW   (KERNEL32.@)
 *
 * Unicode version of GlobalAddAtomA.
 */
ATOM WINAPI GlobalAddAtomW( LPCWSTR str )
{
    ATOM        atom = 0;

    if (!check_integral_atom( str, &atom ))
    {
        if (!set_ntstatus( NtAddAtom( str, lstrlenW( str ) * sizeof(WCHAR), &atom ))) return 0;
    }
    return atom;
}


/***********************************************************************
 *           AddAtomW   (KERNEL32.@)
 *
 * Unicode version of AddAtomA.          
 */
ATOM WINAPI AddAtomW( LPCWSTR str )
{
    ATOM                atom = 0;
    RTL_ATOM_TABLE      table;

    if (!check_integral_atom( str, &atom ) && (table = get_local_table( 0 )))
    {
        if (!set_ntstatus( RtlAddAtomToAtomTable( table, str, &atom ))) return 0;
    }
    return atom;
}


/***********************************************************************
 *           GlobalDeleteAtom   (KERNEL32.@)
 *
 * Decrement the reference count of a string atom.  If the count is
 * zero, the string associated with the atom is removed from the table.
 *
 * RETURNS
 *	Success: 0.
 *	Failure: atom.
 */
ATOM WINAPI GlobalDeleteAtom( ATOM atom /* [in] Atom to delete */ )
{
    if (atom >= MAXINTATOM)
    {
        if (!set_ntstatus( NtDeleteAtom( atom ))) return atom;
    }
    return 0;
}


/***********************************************************************
 *           DeleteAtom   (KERNEL32.@)
 *
 * Decrement the reference count of a string atom.  If the count becomes
 * zero, the string associated with the atom is removed from the table.
 *
 * RETURNS
 *	Success: 0.
 *	Failure: atom
 */
ATOM WINAPI DeleteAtom( ATOM atom /* [in] Atom to delete */ )
{
    RTL_ATOM_TABLE      table;

    if (atom >= MAXINTATOM)
    {
        if (!(table = get_local_table( 0 ))) return atom;
        if (!set_ntstatus( RtlDeleteAtomFromAtomTable( table, atom ))) return atom;
    }
    return 0;
}


/***********************************************************************
 *           GlobalFindAtomA   (KERNEL32.@)
 *
 * Get the atom associated with a string.
 *
 * RETURNS
 *	Success: The associated atom.
 *	Failure: 0.
 */
ATOM WINAPI GlobalFindAtomA( LPCSTR str /* [in] Pointer to string to search for */ )
{
    ATOM atom = 0;

    if (!check_integral_atom( str, &atom ))
    {
        WCHAR buffer[MAX_ATOM_LEN];
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, strlen(str), buffer, MAX_ATOM_LEN );

        if (!len) SetLastError( ERROR_INVALID_PARAMETER );
        else if (!set_ntstatus( NtFindAtom( buffer, len * sizeof(WCHAR), &atom ))) return 0;
    }
    return atom;
}

/***********************************************************************
 *           FindAtomA   (KERNEL32.@)
 *
 * Get the atom associated with a string.
 *
 * RETURNS
 *	Success: The associated atom.
 *	Failure: 0.
 */
ATOM WINAPI FindAtomA( LPCSTR str /* [in] Pointer to string to find */ )
{
    ATOM atom = 0;

    if (!check_integral_atom( str, &atom ))
    {
        WCHAR           buffer[MAX_ATOM_LEN + 1];
        DWORD           len;
        RTL_ATOM_TABLE  table;

        len = MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, MAX_ATOM_LEN + 1 );
        if (!len) SetLastError( ERROR_INVALID_PARAMETER );
        else if ((table = get_local_table( 0 )))
        {
            if (!set_ntstatus( RtlLookupAtomInAtomTable( table, buffer, &atom ))) return 0;
        }
    }
    return atom;
}


/***********************************************************************
 *           GlobalFindAtomW   (KERNEL32.@)
 *
 * Unicode version of GlobalFindAtomA.
 */
ATOM WINAPI GlobalFindAtomW( LPCWSTR str )
{
    ATOM atom = 0;

    if (!check_integral_atom( str, &atom ))
    {
        if (!set_ntstatus( NtFindAtom( str, lstrlenW( str ) * sizeof(WCHAR), &atom ))) return 0;
    }
    return atom;
}


/***********************************************************************
 *           FindAtomW   (KERNEL32.@)
 *
 * Unicode version of FindAtomA.
 */
ATOM WINAPI FindAtomW( LPCWSTR str )
{
    ATOM                atom = 0;
    RTL_ATOM_TABLE      table;

    if ((table = get_local_table( 0 )))
    {
        if (!set_ntstatus( RtlLookupAtomInAtomTable( table, str, &atom ))) return 0;
    }
    return atom;
}


/***********************************************************************
 *           GlobalGetAtomNameA   (KERNEL32.@)
 *
 * Get a copy of the string associated with an atom.
 *
 * RETURNS
 *	Success: The length of the returned string in characters.
 *	Failure: 0.
 */
UINT WINAPI GlobalGetAtomNameA(
              ATOM atom,    /* [in]  Atom identifier */
              LPSTR buffer, /* [out] Pointer to buffer for atom string */
              INT count )   /* [in]  Size of buffer */
{
    WCHAR       tmpW[MAX_ATOM_LEN + 1];
    UINT        wlen, len = 0, c;

    if (count <= 0) SetLastError( ERROR_MORE_DATA );
    else if ((wlen = GlobalGetAtomNameW( atom, tmpW, MAX_ATOM_LEN + 1 )))
    {
        char    tmp[MAX_ATOM_LEN + 1];

        len = WideCharToMultiByte( CP_ACP, 0, tmpW, wlen, tmp, MAX_ATOM_LEN + 1, NULL, NULL );
        c = min(len, count - 1);
        memcpy(buffer, tmp, c);
        buffer[c] = '\0';
        if (len >= count)
        {
            len = 0;
            SetLastError( ERROR_MORE_DATA );
        }
    }
    return len;
}


/***********************************************************************
 *           GetAtomNameA   (KERNEL32.@)
 *
 * Get a copy of the string associated with an atom.
 *
 * RETURNS
 *	Success: The length of the returned string in characters.
 *	Failure: 0.
 */
UINT WINAPI GetAtomNameA(
              ATOM atom,    /* [in]  Atom */
              LPSTR buffer, /* [out] Pointer to string for atom string */
              INT count)    /* [in]  Size of buffer */
{
    WCHAR       tmpW[MAX_ATOM_LEN + 1];
    UINT        wlen, len = 0, c;

    if (count <= 0) SetLastError( ERROR_MORE_DATA );
    else if ((wlen = GetAtomNameW( atom, tmpW, MAX_ATOM_LEN + 1 )))
    {
        char    tmp[MAX_ATOM_LEN + 1];

        len = WideCharToMultiByte( CP_ACP, 0, tmpW, wlen, tmp, MAX_ATOM_LEN + 1, NULL, NULL );
        c = min(len, count - 1);
        memcpy(buffer, tmp, c);
        buffer[c] = '\0';
        if (len >= count)
        {
            len = c;
            SetLastError( ERROR_MORE_DATA );
        }
    }
    return len;
}


/***********************************************************************
 *           GlobalGetAtomNameW   (KERNEL32.@)
 *
 * Unicode version of GlobalGetAtomNameA.
 */
UINT WINAPI GlobalGetAtomNameW( ATOM atom, LPWSTR buffer, INT count )
{
    char        ptr[sizeof(ATOM_BASIC_INFORMATION) + MAX_ATOM_LEN * sizeof(WCHAR)];
    ATOM_BASIC_INFORMATION*     abi = (ATOM_BASIC_INFORMATION*)ptr;
    ULONG       ptr_size = sizeof(ATOM_BASIC_INFORMATION) + MAX_ATOM_LEN * sizeof(WCHAR);
    UINT        length = 0;

    if (count <= 0)
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }
    if (!set_ntstatus( NtQueryInformationAtom( atom, AtomBasicInformation, (void*)ptr, ptr_size, NULL )))
        return 0;

    length = min( abi->NameLength / sizeof(WCHAR), count);
    memcpy( buffer, abi->Name, length * sizeof(WCHAR) );
    /* yes, the string will not be null terminated if the passed buffer
     * is one WCHAR too small (and it's not an error)
     */
    if (length < abi->NameLength / sizeof(WCHAR))
    {
        SetLastError( ERROR_MORE_DATA );
        length = count;
    }
    else if (length < count) buffer[length] = '\0';
    return length;
}


/***********************************************************************
 *           GetAtomNameW   (KERNEL32.@)
 *
 * Unicode version of GetAtomNameA.
 */
UINT WINAPI GetAtomNameW( ATOM atom, LPWSTR buffer, INT count )
{
    RTL_ATOM_TABLE      table;
    DWORD               length;
    WCHAR               tmp[MAX_ATOM_LEN + 1];

    if (count <= 0)
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }
    if (!(table = get_local_table( 0 ))) return 0;
    length = sizeof(tmp);
    if (!set_ntstatus( RtlQueryAtomInAtomTable( table, atom, NULL, NULL, tmp, &length ))) return 0;
    length = min(length, (count - 1) * sizeof(WCHAR));
    if (length) memcpy(buffer, tmp, length);
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );
    length /= sizeof(WCHAR);
    buffer[length] = '\0';
    return length;
}

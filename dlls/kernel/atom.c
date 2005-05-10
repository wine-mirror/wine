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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Warning: The code assumes that LocalAlloc() returns a block aligned
 * on a 4-bytes boundary (because of the shifting done in
 * HANDLETOATOM).  If this is not the case, the allocation code will
 * have to be changed.
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
#include "winerror.h"

#include "wine/server.h"
#include "wine/unicode.h"
#include "kernel_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(atom);

#define MAX_ATOM_LEN 255

static struct atom_table* get_local_table(DWORD entries)
{
    static struct atom_table*   local_table;

    if (!local_table)
    {
        NTSTATUS                status;
        struct atom_table*      table;

        SERVER_START_REQ( init_atom_table )
        {
            req->entries = entries;
            status = wine_server_call( req );
            table = reply->table;
        }
        SERVER_END_REQ;

        if (status)
            SetLastError( RtlNtStatusToDosError( status ) );
        else if (InterlockedCompareExchangePointer((void*)&local_table, table, NULL) != NULL)
            NtClose((HANDLE)table);
    }
    return local_table;
}

/***********************************************************************
 *           ATOM_IsIntAtomA
 */
static BOOL ATOM_IsIntAtomA(LPCSTR atomstr,WORD *atomid)
{
    UINT atom = 0;
    if (!HIWORD(atomstr)) atom = LOWORD(atomstr);
    else
    {
        if (*atomstr++ != '#') return FALSE;
        while (*atomstr >= '0' && *atomstr <= '9')
        {
            atom = atom * 10 + *atomstr - '0';
            atomstr++;
        }
        if (*atomstr) return FALSE;
    }
    if (atom >= MAXINTATOM)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        atom = 0;
    }
    *atomid = atom;
    return TRUE;
}


/***********************************************************************
 *           ATOM_IsIntAtomW
 */
static BOOL ATOM_IsIntAtomW(LPCWSTR atomstr,WORD *atomid)
{
    UINT atom = 0;
    if (!HIWORD(atomstr)) atom = LOWORD(atomstr);
    else
    {
        if (*atomstr++ != '#') return FALSE;
        while (*atomstr >= '0' && *atomstr <= '9')
        {
            atom = atom * 10 + *atomstr - '0';
            atomstr++;
        }
        if (*atomstr) return FALSE;
    }
    if (atom >= MAXINTATOM)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        atom = 0;
    }
    *atomid = atom;
    return TRUE;
}


/***********************************************************************
 *           InitAtomTable   (KERNEL32.@)
 *
 * Initialise the global atom table.
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
    return get_local_table( entries ) ? TRUE : FALSE;
}


static ATOM ATOM_AddAtomA( LPCSTR str, struct atom_table* table )
{
    ATOM atom = 0;
    if (!ATOM_IsIntAtomA( str, &atom ))
    {
        WCHAR buffer[MAX_ATOM_LEN];

        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, strlen(str), buffer, MAX_ATOM_LEN );
        if (!len)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        SERVER_START_REQ( add_atom )
        {
            wine_server_add_data( req, buffer, len * sizeof(WCHAR) );
            req->table = table;
            if (!wine_server_call_err(req)) atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    TRACE( "(%s) %s -> %x\n", table ? "local" : "global", debugstr_a(str), atom );
    return atom;
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
    return ATOM_AddAtomA( str, NULL );
}


/***********************************************************************
 *           AddAtomA   (KERNEL32.@)
 *
 * Add a character string to the global atom table and return a unique
 * value identifying it.
 *
 * RETURNS
 *      Success: The atom allocated to str.
 *      Failure: 0.
 */
ATOM WINAPI AddAtomA( LPCSTR str /* [in] String to add */ )
{
    return ATOM_AddAtomA( str, get_local_table(0) );
}


static ATOM ATOM_AddAtomW( LPCWSTR str, struct atom_table* table )
{
    ATOM atom = 0;
    if (!ATOM_IsIntAtomW( str, &atom ))
    {
        DWORD len = strlenW(str);
        if (len > MAX_ATOM_LEN)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        SERVER_START_REQ( add_atom )
        {
            req->table = table;
            wine_server_add_data( req, str, len * sizeof(WCHAR) );
            if (!wine_server_call_err(req)) atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    TRACE( "(%s) %s -> %x\n", table ? "local" : "global", debugstr_w(str), atom );
    return atom;
}


/***********************************************************************
 *           GlobalAddAtomW   (KERNEL32.@)
 *
 * Unicode version of GlobalAddAtomA.
 */
ATOM WINAPI GlobalAddAtomW( LPCWSTR str )
{
    return ATOM_AddAtomW( str, NULL );
}


/***********************************************************************
 *           AddAtomW   (KERNEL32.@)
 *
 * Unicode version of AddAtomA.          
 */
ATOM WINAPI AddAtomW( LPCWSTR str )
{
    return ATOM_AddAtomW( str, get_local_table(0) );
}


static ATOM ATOM_DeleteAtom( ATOM atom, struct atom_table* table )
{
    TRACE( "(%s) %x\n", table ? "local" : "global", atom );
    if (atom >= MAXINTATOM)
    {
        SERVER_START_REQ( delete_atom )
        {
            req->atom = atom;
            req->table = table;
            wine_server_call_err( req );
        }
        SERVER_END_REQ;
    }
    return 0;
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
    return ATOM_DeleteAtom( atom, NULL);
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
    return ATOM_DeleteAtom( atom, get_local_table(0) );
}


static ATOM ATOM_FindAtomA( LPCSTR str, struct atom_table* table )
{
    ATOM atom = 0;
    if (!ATOM_IsIntAtomA( str, &atom ))
    {
        WCHAR buffer[MAX_ATOM_LEN];

        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, strlen(str), buffer, MAX_ATOM_LEN );
        if (!len)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        SERVER_START_REQ( find_atom )
        {
            req->table = table;
            wine_server_add_data( req, buffer, len * sizeof(WCHAR) );
            if (!wine_server_call_err(req)) atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    TRACE( "(%s) %s -> %x\n", table ? "local" : "global", debugstr_a(str), atom );
    return atom;
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
    return ATOM_FindAtomA( str, NULL );
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
    return ATOM_FindAtomA( str, get_local_table(0) );
}


static ATOM ATOM_FindAtomW( LPCWSTR str, struct atom_table* table )
{
    ATOM atom = 0;
    if (!ATOM_IsIntAtomW( str, &atom ))
    {
        DWORD len = strlenW(str);
        if (len > MAX_ATOM_LEN)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        SERVER_START_REQ( find_atom )
        {
            wine_server_add_data( req, str, len * sizeof(WCHAR) );
            req->table = table;
            if (!wine_server_call_err( req )) atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    TRACE( "(%s) %s -> %x\n", table ? "local" : "global", debugstr_w(str), atom );
    return atom;
}


/***********************************************************************
 *           GlobalFindAtomW   (KERNEL32.@)
 *
 * Unicode version of GlobalFindAtomA.
 */
ATOM WINAPI GlobalFindAtomW( LPCWSTR str )
{
    return ATOM_FindAtomW( str, NULL );
}


/***********************************************************************
 *           FindAtomW   (KERNEL32.@)
 *
 * Unicode version of FindAtomA.
 */
ATOM WINAPI FindAtomW( LPCWSTR str )
{
    return ATOM_FindAtomW( str, get_local_table(0) );
}


static UINT ATOM_GetAtomNameA( ATOM atom, LPSTR buffer, INT count, struct atom_table* table )
{
    INT len;

    if (count <= 0)
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }
    if (atom < MAXINTATOM)
    {
        char name[8];
        if (!atom)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        len = sprintf( name, "#%d", atom );
        lstrcpynA( buffer, name, count );
    }
    else
    {
        WCHAR full_name[MAX_ATOM_LEN];

        len = 0;
        SERVER_START_REQ( get_atom_information )
        {
            req->atom = atom;
            req->table = table;
            wine_server_set_reply( req, full_name, sizeof(full_name) );
            if (!wine_server_call_err( req ))
            {
                len = WideCharToMultiByte( CP_ACP, 0, full_name,
                                           wine_server_reply_size(reply) / sizeof(WCHAR),
                                           buffer, count - 1, NULL, NULL );
                if (!len) len = count; /* overflow */
                else buffer[len] = 0;
            }
        }
        SERVER_END_REQ;
    }

    if (len && count <= len)
    {
        SetLastError( ERROR_MORE_DATA );
        buffer[count-1] = 0;
        return 0;
    }
    TRACE( "(%s) %x -> %s\n", table ? "local" : "global", atom, debugstr_a(buffer) );
    return len;
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
    return ATOM_GetAtomNameA( atom, buffer, count, NULL );
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
    return ATOM_GetAtomNameA( atom, buffer, count, get_local_table(0) );
}


static UINT ATOM_GetAtomNameW( ATOM atom, LPWSTR buffer, INT count, struct atom_table* table )
{
    INT len;

    if (count <= 0)
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }
    if (atom < MAXINTATOM)
    {
        char name[8];
        if (!atom)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        sprintf( name, "#%d", atom );
        len = MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, count );
        if (!len) buffer[count-1] = 0;  /* overflow */
    }
    else
    {
        WCHAR full_name[MAX_ATOM_LEN];

        len = 0;
        SERVER_START_REQ( get_atom_information )
        {
            req->atom = atom;
            req->table = table;
            wine_server_set_reply( req, full_name, sizeof(full_name) );
            if (!wine_server_call_err( req ))
            {
                len = wine_server_reply_size(reply) / sizeof(WCHAR);
                if (count > len) count = len + 1;
                memcpy( buffer, full_name, (count-1) * sizeof(WCHAR) );
                buffer[count-1] = 0;
            }
        }
        SERVER_END_REQ;
        if (!len) return 0;
    }
    TRACE( "(%s) %x -> %s\n", table ? "local" : "global", atom, debugstr_w(buffer) );
    return len;
}


/***********************************************************************
 *           GlobalGetAtomNameW   (KERNEL32.@)
 *
 * Unicode version of GlobalGetAtomNameA.
 */
UINT WINAPI GlobalGetAtomNameW( ATOM atom, LPWSTR buffer, INT count )
{
    return ATOM_GetAtomNameW( atom, buffer, count, NULL);
}


/***********************************************************************
 *           GetAtomNameW   (KERNEL32.@)
 *
 * Unicode version of GetAtomNameA.
 */
UINT WINAPI GetAtomNameW( ATOM atom, LPWSTR buffer, INT count )
{
    return ATOM_GetAtomNameW( atom, buffer, count, get_local_table(0) );
}

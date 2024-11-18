/*
 * Window properties
 *
 * Copyright 1995, 1996, 2001 Alexandre Julliard
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

#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ntuser.h"

/* size of buffer needed to store an atom string */
#define ATOM_BUFFER_SIZE 256


/***********************************************************************
 *              get_properties
 *
 * Retrieve the list of properties of a given window.
 * Returned buffer must be freed by caller.
 */
static struct ntuser_property_list *get_properties( HWND hwnd, ULONG *count )
{
    struct ntuser_property_list *props;
    int total = 32;
    NTSTATUS status;

    while (total)
    {
        if (!(props = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*props) ))) break;
        if (!(status = NtUserBuildPropList( hwnd, total, props, count ))) return props;
        HeapFree( GetProcessHeap(), 0, props );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        total = *count;  /* restart with larger buffer */
    }
    return NULL;
}


/***********************************************************************
 *              EnumPropsA_relay
 *
 * relay to call the EnumProps callback function from EnumPropsEx
 */
static BOOL CALLBACK EnumPropsA_relay( HWND hwnd, LPSTR str, HANDLE handle, ULONG_PTR lparam )
{
    PROPENUMPROCA func = (PROPENUMPROCA)lparam;
    return func( hwnd, str, handle );
}


/***********************************************************************
 *              EnumPropsW_relay
 *
 * relay to call the EnumProps callback function from EnumPropsEx
 */
static BOOL CALLBACK EnumPropsW_relay( HWND hwnd, LPWSTR str, HANDLE handle, ULONG_PTR lparam )
{
    PROPENUMPROCW func = (PROPENUMPROCW)lparam;
    return func( hwnd, str, handle );
}


/***********************************************************************
 *              EnumPropsA   (USER32.@)
 */
INT WINAPI EnumPropsA( HWND hwnd, PROPENUMPROCA func )
{
    return EnumPropsExA( hwnd, EnumPropsA_relay, (LPARAM)func );
}


/***********************************************************************
 *              EnumPropsW   (USER32.@)
 */
INT WINAPI EnumPropsW( HWND hwnd, PROPENUMPROCW func )
{
    return EnumPropsExW( hwnd, EnumPropsW_relay, (LPARAM)func );
}


/***********************************************************************
 *              GetPropA   (USER32.@)
 */
HANDLE WINAPI GetPropA( HWND hwnd, LPCSTR str )
{
    WCHAR buffer[ATOM_BUFFER_SIZE];

    if (IS_INTRESOURCE(str)) return GetPropW( hwnd, (LPCWSTR)str );
    if (!MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, ATOM_BUFFER_SIZE )) return 0;
    return GetPropW( hwnd, buffer );
}


/***********************************************************************
 *              GetPropW   (USER32.@)
 */
HANDLE WINAPI GetPropW( HWND hwnd, LPCWSTR str )
{
    return NtUserGetProp( hwnd, str );
}


/***********************************************************************
 *              SetPropA   (USER32.@)
 */
BOOL WINAPI SetPropA( HWND hwnd, LPCSTR str, HANDLE handle )
{
    WCHAR buffer[ATOM_BUFFER_SIZE];

    if (IS_INTRESOURCE(str)) return SetPropW( hwnd, (LPCWSTR)str, handle );
    if (!MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, ATOM_BUFFER_SIZE )) return FALSE;
    return SetPropW( hwnd, buffer, handle );
}


/***********************************************************************
 *              SetPropW   (USER32.@)
 */
BOOL WINAPI SetPropW( HWND hwnd, LPCWSTR str, HANDLE handle )
{
    return NtUserSetProp( hwnd, str, handle );
}


/***********************************************************************
 *              RemovePropA   (USER32.@)
 */
HANDLE WINAPI RemovePropA( HWND hwnd, LPCSTR str )
{
    WCHAR buffer[ATOM_BUFFER_SIZE];

    if (IS_INTRESOURCE(str)) return RemovePropW( hwnd, (LPCWSTR)str );
    if (!MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, ATOM_BUFFER_SIZE )) return 0;
    return RemovePropW( hwnd, buffer );
}


/***********************************************************************
 *              RemovePropW   (USER32.@)
 */
HANDLE WINAPI RemovePropW( HWND hwnd, LPCWSTR str )
{
    return NtUserRemoveProp( hwnd, str );
}


/***********************************************************************
 *              EnumPropsExA   (USER32.@)
 */
INT WINAPI EnumPropsExA(HWND hwnd, PROPENUMPROCEXA func, LPARAM lParam)
{
    int ret = -1;
    ULONG i, count;
    struct ntuser_property_list *list = get_properties( hwnd, &count );

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            char string[ATOM_BUFFER_SIZE];
            if (!GlobalGetAtomNameA( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
            if (!(ret = func( hwnd, string, (HANDLE)(ULONG_PTR)list[i].data, lParam ))) break;
        }
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}


/***********************************************************************
 *              EnumPropsExW   (USER32.@)
 */
INT WINAPI EnumPropsExW(HWND hwnd, PROPENUMPROCEXW func, LPARAM lParam)
{
    int ret = -1;
    ULONG i, count;
    struct ntuser_property_list *list = get_properties( hwnd, &count );

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            WCHAR string[ATOM_BUFFER_SIZE];
            if (!GlobalGetAtomNameW( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
            if (!(ret = func( hwnd, string, (HANDLE)(ULONG_PTR)list[i].data, lParam ))) break;
        }
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}

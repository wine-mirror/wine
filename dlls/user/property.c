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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include "windef.h"
#include "wingdi.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "wine/server.h"
#include "win.h"

/* size of buffer needed to store an atom string */
#define ATOM_BUFFER_SIZE 256

/* ### start build ### */
extern WORD CALLBACK PROP_CallTo16_word_wlw(PROPENUMPROC16,WORD,LONG,WORD);
/* ### stop build ### */

/***********************************************************************
 *              get_properties
 *
 * Retrieve the list of properties of a given window.
 * Returned buffer must be freed by caller.
 */
static property_data_t *get_properties( HWND hwnd, int *count )
{
    property_data_t *data;
    int total = 32;

    while (total)
    {
        int res = 0;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*data) ))) break;
        *count = 0;
        SERVER_START_REQ( get_window_properties )
        {
            req->window = hwnd;
            wine_server_add_data( req, data, total * sizeof(*data) );
            if (!wine_server_call( req )) res = reply->total;
        }
        SERVER_END_REQ;
        if (res && res <= total)
        {
            *count = res;
            return data;
        }
        HeapFree( GetProcessHeap(), 0, data );
        total = res;  /* restart with larger buffer */
    }
    return NULL;
}


/***********************************************************************
 *              EnumPropsA_relay
 *
 * relay to call the EnumProps callback function from EnumPropsEx
 */
static BOOL CALLBACK EnumPropsA_relay( HWND hwnd, LPCSTR str, HANDLE handle, ULONG_PTR lparam )
{
    PROPENUMPROCA func = (PROPENUMPROCA)lparam;
    return func( hwnd, str, handle );
}


/***********************************************************************
 *              EnumPropsW_relay
 *
 * relay to call the EnumProps callback function from EnumPropsEx
 */
static BOOL CALLBACK EnumPropsW_relay( HWND hwnd, LPCWSTR str, HANDLE handle, ULONG_PTR lparam )
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
    ATOM atom;
    HANDLE ret = 0;

    if (!HIWORD(str)) atom = LOWORD(str);
    else if (!(atom = GlobalFindAtomA( str ))) return 0;

    SERVER_START_REQ( get_window_property )
    {
        req->window = hwnd;
        req->atom = atom;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              GetPropW   (USER32.@)
 */
HANDLE WINAPI GetPropW( HWND hwnd, LPCWSTR str )
{
    ATOM atom;
    HANDLE ret = 0;

    if (!HIWORD(str)) atom = LOWORD(str);
    else if (!(atom = GlobalFindAtomW( str ))) return 0;

    SERVER_START_REQ( get_window_property )
    {
        req->window = hwnd;
        req->atom = atom;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              SetPropA   (USER32.@)
 */
BOOL WINAPI SetPropA( HWND hwnd, LPCSTR str, HANDLE handle )
{
    ATOM atom;
    BOOL ret;

    if (!HIWORD(str)) atom = LOWORD(str);
    else if (!(atom = GlobalAddAtomA( str ))) return FALSE;

    SERVER_START_REQ( set_window_property )
    {
        req->window = hwnd;
        req->atom   = atom;
        req->string = (HIWORD(str) != 0);
        req->handle = handle;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (HIWORD(str)) GlobalDeleteAtom( atom );
    return ret;
}


/***********************************************************************
 *              SetPropW   (USER32.@)
 */
BOOL WINAPI SetPropW( HWND hwnd, LPCWSTR str, HANDLE handle )
{
    ATOM atom;
    BOOL ret;

    if (!HIWORD(str)) atom = LOWORD(str);
    else if (!(atom = GlobalAddAtomW( str ))) return FALSE;

    SERVER_START_REQ( set_window_property )
    {
        req->window = hwnd;
        req->atom   = atom;
        req->string = (HIWORD(str) != 0);
        req->handle = handle;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (HIWORD(str)) GlobalDeleteAtom( atom );
    return ret;
}


/***********************************************************************
 *              RemovePropA   (USER32.@)
 */
HANDLE WINAPI RemovePropA( HWND hwnd, LPCSTR str )
{
    ATOM atom;
    HANDLE ret = 0;

    if (!HIWORD(str)) return RemovePropW( hwnd, MAKEINTATOMW(LOWORD(str)) );

    if ((atom = GlobalAddAtomA( str )))
    {
        ret = RemovePropW( hwnd, MAKEINTATOMW(atom) );
        GlobalDeleteAtom( atom );
    }
    return ret;
}


/***********************************************************************
 *              RemovePropW   (USER32.@)
 */
HANDLE WINAPI RemovePropW( HWND hwnd, LPCWSTR str )
{
    ATOM atom;
    HANDLE ret = 0;

    if (!HIWORD(str)) atom = LOWORD(str);
    else if (!(atom = GlobalAddAtomW( str ))) return 0;

    SERVER_START_REQ( remove_window_property )
    {
        req->window = hwnd;
        req->atom   = atom;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;

    if (HIWORD(str)) GlobalDeleteAtom( atom );
    return ret;
}


/***********************************************************************
 *              EnumPropsExA   (USER32.@)
 */
INT WINAPI EnumPropsExA(HWND hwnd, PROPENUMPROCEXA func, LPARAM lParam)
{
    int ret = -1, i, count;
    property_data_t *list = get_properties( hwnd, &count );

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            char string[ATOM_BUFFER_SIZE];
            if (!GlobalGetAtomNameA( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
            if (!(ret = func( hwnd, string, list[i].handle, lParam ))) break;
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
    int ret = -1, i, count;
    property_data_t *list = get_properties( hwnd, &count );

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            WCHAR string[ATOM_BUFFER_SIZE];
            if (!GlobalGetAtomNameW( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
            if (!(ret = func( hwnd, string, list[i].handle, lParam ))) break;
        }
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}


/***********************************************************************
 *              EnumProps   (USER.27)
 */
INT16 WINAPI EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    int ret = -1, i, count;
    property_data_t *list = get_properties( HWND_32(hwnd), &count );

    if (list)
    {
        char string[ATOM_BUFFER_SIZE];
        SEGPTR segptr = MapLS( string );
        for (i = 0; i < count; i++)
        {
            if (list[i].string)  /* it was a string originally */
            {
                if (!GlobalGetAtomNameA( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
                ret = PROP_CallTo16_word_wlw( func, hwnd, segptr, LOWORD(list[i].handle) );
            }
            else
                ret = PROP_CallTo16_word_wlw( func, hwnd, list[i].atom, LOWORD(list[i].handle) );
            if (!ret) break;
        }
        UnMapLS( segptr );
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}

/*
 * Window properties
 *
 * Copyright 1995, 1996, 2001 Alexandre Julliard
 */

#include <string.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/server.h"
#include "heap.h"

/* size of buffer needed to store an atom string */
#define ATOM_BUFFER_SIZE 256


/***********************************************************************
 *              get_properties
 *
 * Retrieve the list of properties of a given window.
 * Returned buffer must be freed by caller.
 */
static property_data_t *get_properties( HWND hwnd, int *count )
{
    property_data_t *ret = NULL;

    SERVER_START_VAR_REQ( get_window_properties, REQUEST_MAX_VAR_SIZE )
    {
        req->window = hwnd;
        if (!SERVER_CALL())
        {
            size_t size = server_data_size(req);
            if (size)
            {
                property_data_t *data = server_data_ptr(req);
                if ((ret = HeapAlloc( GetProcessHeap(), 0, size ))) memcpy( ret, data, size );
                *count = size / sizeof(*data);
            }
        }
    }
    SERVER_END_VAR_REQ;
    return ret;
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
        if (!SERVER_CALL_ERR()) ret = req->handle;
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
        if (!SERVER_CALL_ERR()) ret = req->handle;
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
        ret = !SERVER_CALL_ERR();
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
        ret = !SERVER_CALL_ERR();
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
        if (!SERVER_CALL_ERR()) ret = req->handle;
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
    property_data_t *list = get_properties( hwnd, &count );

    if (list)
    {
        char *string = SEGPTR_ALLOC( ATOM_BUFFER_SIZE );
        for (i = 0; i < count; i++)
        {
            if (list[i].string)  /* it was a string originally */
            {
                if (!GlobalGetAtomNameA( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
                ret = func( hwnd, SEGPTR_GET(string), list[i].handle );
            }
            else
                ret = func( hwnd, list[i].atom, list[i].handle );
            if (!ret) break;
        }
        SEGPTR_FREE( string );
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}

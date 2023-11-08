/*
 * USER resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995, 2009 Alexandre Julliard
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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ntuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/* this is the 8 byte accel struct used in Win32 resources (internal only) */
typedef struct
{
    WORD   fVirt;
    WORD   key;
    WORD   cmd;
    WORD   pad;
} PE_ACCEL;

/**********************************************************************
 *			LoadAcceleratorsW	(USER32.@)
 */
HACCEL WINAPI LoadAcceleratorsW(HINSTANCE instance, LPCWSTR name)
{
    const PE_ACCEL *pe_table;
    ACCEL *table;
    unsigned int i;
    HRSRC rsrc;
    HACCEL handle;
    DWORD count;

    if (!(rsrc = FindResourceW( instance, name, (LPWSTR)RT_ACCELERATOR ))) return 0;
    pe_table = LoadResource( instance, rsrc );
    count = SizeofResource( instance, rsrc ) / sizeof(*pe_table);
    if (!count) return 0;
    table = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*table) );
    if (!table) return 0;
    for (i = 0; i < count; i++)
    {
        table[i].fVirt = pe_table[i].fVirt;
        table[i].key   = pe_table[i].key;
        table[i].cmd   = pe_table[i].cmd;
    }
    handle = NtUserCreateAcceleratorTable( table, count );
    HeapFree( GetProcessHeap(), 0, table );
    TRACE_(accel)("%p %s returning %p\n", instance, debugstr_w(name), handle );
    return handle;
}

/***********************************************************************
 *		LoadAcceleratorsA   (USER32.@)
 */
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE instance,LPCSTR lpTableName)
{
    INT len;
    LPWSTR uni;
    HACCEL result = 0;

    if (IS_INTRESOURCE(lpTableName)) return LoadAcceleratorsW( instance, (LPCWSTR)lpTableName );

    len = MultiByteToWideChar( CP_ACP, 0, lpTableName, -1, NULL, 0 );
    if ((uni = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, lpTableName, -1, uni, len );
        result = LoadAcceleratorsW(instance,uni);
        HeapFree( GetProcessHeap(), 0, uni);
    }
    return result;
}

/**********************************************************************
 *             CopyAcceleratorTableA   (USER32.@)
 */
INT WINAPI CopyAcceleratorTableA(HACCEL src, LPACCEL dst, INT count)
{
    char ch;
    int i, ret = NtUserCopyAcceleratorTable( src, dst, count );

    if (ret && dst)
    {
        for (i = 0; i < ret; i++)
        {
            if (dst[i].fVirt & FVIRTKEY) continue;
            WideCharToMultiByte( CP_ACP, 0, &dst[i].key, 1, &ch, 1, NULL, NULL );
            dst[i].key = ch;
        }
    }
    return ret;
}

/*********************************************************************
 *                    CreateAcceleratorTableA   (USER32.@)
 */
HACCEL WINAPI CreateAcceleratorTableA( ACCEL *accel, INT count )
{
    HACCEL handle;
    ACCEL *table;
    int i;

    if (count < 1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    table = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*table) );
    if (!table) return 0;
    for (i = 0; i < count; i++)
    {
        table[i].fVirt = accel[i].fVirt;
        table[i].cmd   = accel[i].cmd;
        if (!(accel[i].fVirt & FVIRTKEY))
        {
            char ch = accel[i].key;
            MultiByteToWideChar( CP_ACP, 0, &ch, 1, &table[i].key, 1 );
        }
        else table[i].key = accel[i].key;
    }
    handle = NtUserCreateAcceleratorTable( table, count );
    HeapFree( GetProcessHeap(), 0, table );
    TRACE_(accel)("returning %p\n", handle );
    return handle;
}

/**********************************************************************
 *	LoadStringW		(USER32.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LoadStringW( HINSTANCE instance, UINT resource_id,
                            LPWSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n",
          instance, resource_id, buffer, buflen);

    if(buffer == NULL)
        return 0;

    if (!(hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1), (LPWSTR)RT_STRING )) ||
        !(hmem = LoadResource( instance, hrsrc )))
    {
        TRACE( "Failed to load string.\n" );
        if (buflen > 0) buffer[0] = 0;
        return 0;
    }

    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;

    TRACE("strlen = %d\n", (int)*p );

    /*if buflen == 0, then return a read-only pointer to the resource itself in buffer
    it is assumed that buffer is actually a (LPWSTR *) */
    if(buflen == 0)
    {
        *((LPWSTR *)buffer) = p + 1;
        return *p;
    }

    i = min(buflen - 1, *p);
    memcpy(buffer, p + 1, i * sizeof(WCHAR));
    buffer[i] = 0;

    TRACE("returning %s\n", debugstr_w(buffer));
    return i;
}

/**********************************************************************
 *	LoadStringA	(USER32.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LoadStringA( HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    DWORD retval = 0;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n",
          instance, resource_id, buffer, buflen);

    if (!buflen) return -1;

    /* Use loword (incremented by 1) as resourceid */
    if ((hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                                (LPWSTR)RT_STRING )) &&
        (hmem = LoadResource( instance, hrsrc )))
    {
        const WCHAR *p = LockResource(hmem);
        unsigned int id = resource_id & 0x000f;

        while (id--) p += *p + 1;

        RtlUnicodeToMultiByteN( buffer, buflen - 1, &retval, p + 1, *p * sizeof(WCHAR) );
    }
    buffer[retval] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return retval;
}

/**********************************************************************
 *	GetGuiResources	(USER32.@)
 */
DWORD WINAPI GetGuiResources( HANDLE hProcess, DWORD uiFlags )
{
    static BOOL warn = TRUE;

    if (warn) {
        FIXME("(%p,%lx): stub\n",hProcess,uiFlags);
       warn = FALSE;
    }

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

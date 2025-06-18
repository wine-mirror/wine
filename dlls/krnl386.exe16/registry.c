/*
 * 16-bit registry functions
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 1998 Matthew Becker
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Alexandre Julliard
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
#include "winreg.h"
#include "wine/debug.h"
#include "wine/winbase16.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static DWORD (WINAPI *pRegCloseKey)(HKEY);
static DWORD (WINAPI *pRegCreateKeyA)(HKEY,LPCSTR,PHKEY);
static DWORD (WINAPI *pRegDeleteKeyA)(HKEY,LPCSTR);
static DWORD (WINAPI *pRegDeleteValueA)(HKEY,LPCSTR);
static DWORD (WINAPI *pRegEnumKeyA)(HKEY,DWORD,LPSTR,DWORD);
static DWORD (WINAPI *pRegEnumValueA)(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
static DWORD (WINAPI *pRegFlushKey)(HKEY);
static DWORD (WINAPI *pRegOpenKeyA)(HKEY,LPCSTR,PHKEY);
static DWORD (WINAPI *pRegQueryValueA)(HKEY,LPCSTR,LPSTR,LPLONG);
static DWORD (WINAPI *pRegQueryValueExA)(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
static DWORD (WINAPI *pRegSetValueA)(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
static DWORD (WINAPI *pRegSetValueExA)(HKEY,LPCSTR,DWORD,DWORD,const BYTE*,DWORD);

static HMODULE advapi32;


/* 0 and 1 are valid rootkeys in win16 shell.dll and are used by
 * some programs. Do not remove those cases. -MM
 */
static inline void fix_win16_hkey( HKEY *hkey )
{
    if (*hkey == 0 || *hkey == (HKEY)1) *hkey = HKEY_CLASSES_ROOT;
}

static void init_func_ptrs(void)
{
    advapi32 = LoadLibraryA("advapi32.dll");
    if (!advapi32)
    {
        ERR( "Unable to load advapi32.dll\n" );
        ExitProcess(1);
    }
#define GET_PTR(name)  p##name = (void *)GetProcAddress(advapi32,#name);
    GET_PTR( RegCloseKey );
    GET_PTR( RegCreateKeyA );
    GET_PTR( RegDeleteKeyA );
    GET_PTR( RegDeleteValueA );
    GET_PTR( RegEnumKeyA );
    GET_PTR( RegEnumValueA );
    GET_PTR( RegFlushKey );
    GET_PTR( RegOpenKeyA );
    GET_PTR( RegQueryValueA );
    GET_PTR( RegQueryValueExA );
    GET_PTR( RegSetValueA );
    GET_PTR( RegSetValueExA );
#undef GET_PTR
}

/******************************************************************************
 *           RegEnumKey   [KERNEL.216]
 */
DWORD WINAPI RegEnumKey16( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegEnumKeyA( hkey, index, name, name_len );
}

/******************************************************************************
 *           RegOpenKey   [KERNEL.217]
 */
DWORD WINAPI RegOpenKey16( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegOpenKeyA( hkey, name, retkey );
}

/******************************************************************************
 *           RegCreateKey   [KERNEL.218]
 */
DWORD WINAPI RegCreateKey16( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegCreateKeyA( hkey, name, retkey );
}

/******************************************************************************
 *           RegDeleteKey   [KERNEL.219]
 */
DWORD WINAPI RegDeleteKey16( HKEY hkey, LPCSTR name )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegDeleteKeyA( hkey, name );
}

/******************************************************************************
 *           RegCloseKey   [KERNEL.220]
 */
DWORD WINAPI RegCloseKey16( HKEY hkey )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegCloseKey( hkey );
}

/******************************************************************************
 *           RegSetValue   [KERNEL.221]
 */
DWORD WINAPI RegSetValue16( HKEY hkey, LPCSTR name, DWORD type, LPCSTR data, DWORD count )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegSetValueA( hkey, name, type, data, count );
}

/******************************************************************************
 *           RegDeleteValue  [KERNEL.222]
 */
DWORD WINAPI RegDeleteValue16( HKEY hkey, LPSTR name )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegDeleteValueA( hkey, name );
}

/******************************************************************************
 *           RegEnumValue   [KERNEL.223]
 */
DWORD WINAPI RegEnumValue16( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                             LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegEnumValueA( hkey, index, value, val_count, reserved, type, data, count );
}

/******************************************************************************
 *           RegQueryValue   [KERNEL.224]
 *
 * NOTES
 *    Is this HACK still applicable?
 *
 * HACK
 *    The 16bit RegQueryValue doesn't handle selectorblocks anyway, so we just
 *    mask out the high 16 bit.  This (not so much incidentally) hopefully fixes
 *    Aldus FH4)
 */
DWORD WINAPI RegQueryValue16( HKEY hkey, LPCSTR name, LPSTR data, LPDWORD count )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    if (count) *count &= 0xffff;
    return pRegQueryValueA( hkey, name, data, (LONG*) count );
}

/******************************************************************************
 *           RegQueryValueEx   [KERNEL.225]
 */
DWORD WINAPI RegQueryValueEx16( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
                                LPBYTE data, LPDWORD count )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegQueryValueExA( hkey, name, reserved, type, data, count );
}

/******************************************************************************
 *           RegSetValueEx   [KERNEL.226]
 */
DWORD WINAPI RegSetValueEx16( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
                              const BYTE *data, DWORD count )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    if (!count && (type==REG_SZ)) count = strlen( (const char *)data );
    return pRegSetValueExA( hkey, name, reserved, type, data, count );
}

/******************************************************************************
 *           RegFlushKey   [KERNEL.227]
 */
DWORD WINAPI RegFlushKey16( HKEY hkey )
{
    if (!advapi32) init_func_ptrs();
    fix_win16_hkey( &hkey );
    return pRegFlushKey( hkey );
}
